#include "GUIWindow.h"
#include "HighPerformanceCRC.h"
#include <iostream>
#include <windows.h>
#include <fileapi.h>
#include <winioctl.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <fstream>

// CRC32 polynomial (IEEE 802.3 standard)
#define CRC32_POLYNOMIAL 0xEDB88320

// Dynamically generate CRC32 lookup table
static uint32_t* generate_crc32_table() {
    static uint32_t crc32_table[256];
    static bool table_generated = false;
    
    if (!table_generated) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (int j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
                } else {
                    crc = crc >> 1;
                }
            }
            crc32_table[i] = crc;
        }
        table_generated = true;
    }
    
    return crc32_table;
}

// Get CRC32 lookup table
static const uint32_t* get_crc32_table() {
    return generate_crc32_table();
}

GUIWindow::GUIWindow() : diskCRC_(nullptr) {
    // Initialize
}

GUIWindow::~GUIWindow() {
    if (diskCRC_) {
        delete diskCRC_;
    }
}

void GUIWindow::show() {
    std::cout << "GUI Window - Disk Sector Data Integrity Tool" << std::endl;
    std::cout << "=============================================" << std::endl;
}

void GUIWindow::setStatusCallback(std::function<void(const std::string&)> callback) {
    statusCallback_ = callback;
}

void GUIWindow::setProgressCallback(std::function<void(int, int)> callback) {
    progressCallback_ = callback;
}

bool GUIWindow::generateChecksums(const std::string& diskPath, uint64_t startSector, 
                                 uint64_t sectorCount, const std::string& outputFile) {
    if (statusCallback_) {
        statusCallback_("Initializing high-performance disk access...");
    }
    
    // Use HighPerformanceCRC instead of DiskSectorCRC
    HighPerformanceCRC* highPerfCRC = new HighPerformanceCRC(diskPath);
    
    // HighPerformanceCRC doesn't have checkFilePermissions, so we'll try to open the disk directly
    // If there's an error, it will be caught during the actual operation
    
    if (statusCallback_) {
        statusCallback_("Starting high-performance checksum generation...");
    }
    
    // Set up progress callback
    auto progressCallback = [this](int current, int total) {
        if (progressCallback_) {
            progressCallback_(current, total);
        }
    };
    
    // Use high-performance mode with optimized settings
    bool result = highPerfCRC->generateChecksumsHighPerformance(
        startSector, sectorCount, outputFile, 4, 256, progressCallback);
    
    if (result) {
        if (statusCallback_) {
            statusCallback_("High-performance checksum data generated successfully!");
        }
    } else {
        if (statusCallback_) {
            statusCallback_("Error: " + highPerfCRC->getLastError());
        }
    }
    
    delete highPerfCRC;
    return result;
}

bool GUIWindow::verifyIntegrity(const std::string& diskPath, const std::string& checksumFile) {
    if (statusCallback_) {
        statusCallback_("Initializing disk access...");
    }
    
    diskCRC_ = new DiskSectorCRC(diskPath);
    
    if (!diskCRC_->checkFilePermissions()) {
        if (statusCallback_) {
            statusCallback_("Error: " + diskCRC_->getLastError());
        }
        return false;
    }
    
    if (statusCallback_) {
        statusCallback_("Starting data integrity verification...");
    }
    
    bool result = diskCRC_->verifySectorIntegrity(checksumFile);
    
    if (result) {
        if (statusCallback_) {
            statusCallback_("Data integrity verification passed!");
        }
    } else {
        if (statusCallback_) {
            statusCallback_("Data integrity verification failed!");
        }
    }
    
    return result;
}

bool GUIWindow::repairData(const std::string& diskPath, const std::string& checksumFile, 
                          const std::string& backupDiskPath) {
    if (statusCallback_) {
        statusCallback_("Initializing disk access...");
    }
    
    diskCRC_ = new DiskSectorCRC(diskPath);
    
    if (!diskCRC_->checkFilePermissions()) {
        if (statusCallback_) {
            statusCallback_("Error: " + diskCRC_->getLastError());
        }
        return false;
    }
    
    if (statusCallback_) {
        statusCallback_("Starting data repair...");
    }
    
    bool result = diskCRC_->repairSectorData(checksumFile, backupDiskPath);
    
    if (result) {
        if (statusCallback_) {
            statusCallback_("Data repair completed!");
        }
    } else {
        if (statusCallback_) {
            statusCallback_("Problem occurred during data repair: " + diskCRC_->getLastError());
        }
    }
    
    return result;
}

// CD/DVD support functions
bool GUIWindow::isCDROM(const std::string& diskPath) {
    std::string fullPath = diskPath;
    if (fullPath.find("\\\\.\\") == std::string::npos) {
        fullPath = "\\\\.\\" + diskPath;
    }
    
    HANDLE hDevice = CreateFileA(fullPath.c_str(), 
                                GENERIC_READ, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, 
                                OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Get device type
    DWORD bytesReturned;
    STORAGE_PROPERTY_QUERY query;
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    
    STORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
    deviceDescriptor.Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
    
    BOOL result = DeviceIoControl(hDevice, 
                                 IOCTL_STORAGE_QUERY_PROPERTY,
                                 &query, sizeof(query),
                                 &deviceDescriptor, sizeof(deviceDescriptor),
                                 &bytesReturned, NULL);
    
    CloseHandle(hDevice);
    
    if (result && deviceDescriptor.RemovableMedia) {
        // Check if it's a CD/DVD drive
        UINT driveType = GetDriveTypeA(diskPath.c_str());
        return (driveType == DRIVE_CDROM);
    }
    
    return false;
}

bool GUIWindow::generateCDChecksums(const std::string& cdPath, const std::string& outputFile) {
    if (statusCallback_) {
        statusCallback_("Initializing CD/DVD access...");
    }
    
    if (!isCDROM(cdPath)) {
        if (statusCallback_) {
            statusCallback_("Error: Specified path is not a CD/DVD drive");
        }
        return false;
    }
    
    // CD/DVD typically starts from sector 0, read all available sectors
    uint64_t startSector = 0;
    uint64_t sectorCount = 10000; // Default read 10000 sectors, can be adjusted as needed
    
    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile.is_open()) {
        if (statusCallback_) {
            statusCallback_("Error: Cannot create output file");
        }
        return false;
    }
    
    // Write CD/DVD specific file header
    uint32_t magic = 0x4344524F; // "CDRO"
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    outFile.write(reinterpret_cast<const char*>(&startSector), sizeof(startSector));
    outFile.write(reinterpret_cast<const char*>(&sectorCount), sizeof(sectorCount));
    outFile.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    
    if (statusCallback_) {
        statusCallback_("Starting CD/DVD checksum generation...");
    }
    
    // Generate checksum for each sector
    uint64_t actualSectors = 0;
    for (uint64_t i = 0; i < sectorCount; ++i) {
        std::vector<uint8_t> sectorData;
        
        if (!readCDSector(cdPath, startSector + i, sectorData)) {
            // Possibly reached end of CD/DVD
            break;
        }
        
        // Calculate CRC32
        uint32_t crc = 0xFFFFFFFF;
        const uint32_t* crc_table = get_crc32_table();
        for (uint8_t byte : sectorData) {
            crc = (crc >> 8) ^ crc_table[(crc ^ byte) & 0xFF];
        }
        crc = crc ^ 0xFFFFFFFF;
        
        SectorChecksum checksum;
        checksum.sectorNumber = startSector + i;
        checksum.crc32 = crc;
        checksum.timestamp = timestamp;
        outFile.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
        actualSectors++;
        
        if (progressCallback_ && (i + 1) % 100 == 0) {
            progressCallback_(i + 1, sectorCount);
        }
    }
    
    outFile.close();
    
    if (statusCallback_) {
        statusCallback_("CD/DVD checksum data generation completed, processed " + std::to_string(actualSectors) + " sectors");
    }
    
    return actualSectors > 0;
}

bool GUIWindow::verifyCDIntegrity(const std::string& cdPath, const std::string& checksumFile) {
    if (statusCallback_) {
        statusCallback_("Initializing CD/DVD access...");
    }
    
    if (!isCDROM(cdPath)) {
        if (statusCallback_) {
            statusCallback_("Error: Specified path is not a CD/DVD drive");
        }
        return false;
    }
    
    std::ifstream inFile(checksumFile, std::ios::binary);
    if (!inFile.is_open()) {
        if (statusCallback_) {
            statusCallback_("Error: Cannot open checksum file");
        }
        return false;
    }
    
    // Read file header
    uint32_t magic;
    uint64_t startSector, sectorCount, timestamp;
    
    inFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    inFile.read(reinterpret_cast<char*>(&startSector), sizeof(startSector));
    inFile.read(reinterpret_cast<char*>(&sectorCount), sizeof(sectorCount));
    inFile.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    
    if (magic != 0x4344524F) {
        if (statusCallback_) {
            statusCallback_("Error: Invalid CD/DVD checksum file format");
        }
        inFile.close();
        return false;
    }
    
    if (statusCallback_) {
        statusCallback_("Starting CD/DVD data integrity verification...");
    }
    
    bool allValid = true;
    uint64_t corruptedSectors = 0;
    
    // Verify each sector
    for (uint64_t i = 0; i < sectorCount; ++i) {
        SectorChecksum storedChecksum;
        inFile.read(reinterpret_cast<char*>(&storedChecksum), sizeof(storedChecksum));
        
        if (inFile.gcount() != sizeof(storedChecksum)) {
            break;
        }
        
        std::vector<uint8_t> currentSectorData;
        if (!readCDSector(cdPath, storedChecksum.sectorNumber, currentSectorData)) {
            if (statusCallback_) {
                statusCallback_("Error: Cannot read CD/DVD sector " + std::to_string(storedChecksum.sectorNumber));
            }
            inFile.close();
            return false;
        }
        
        // Calculate current CRC
        uint32_t currentCRC = 0xFFFFFFFF;
        const uint32_t* crc_table = get_crc32_table();
        for (uint8_t byte : currentSectorData) {
            currentCRC = (currentCRC >> 8) ^ crc_table[(currentCRC ^ byte) & 0xFF];
        }
        currentCRC = currentCRC ^ 0xFFFFFFFF;
        
        if (currentCRC != storedChecksum.crc32) {
            if (statusCallback_) {
                statusCallback_("Warning: CD/DVD sector " + std::to_string(storedChecksum.sectorNumber) + " data corrupted");
            }
            allValid = false;
            corruptedSectors++;
        }
        
        if (progressCallback_ && (i + 1) % 100 == 0) {
            progressCallback_(i + 1, sectorCount);
        }
    }
    
    inFile.close();
    
    if (allValid) {
        if (statusCallback_) {
            statusCallback_("All CD/DVD sectors data integrity verification passed!");
        }
    } else {
        if (statusCallback_) {
            statusCallback_("Found " + std::to_string(corruptedSectors) + " corrupted CD/DVD sectors");
        }
    }
    
    return allValid;
}

uint64_t GUIWindow::getDiskTotalSectors(const std::string& diskPath) {
    std::string fullPath = diskPath;
    if (fullPath.find("\\\\.\\") == std::string::npos) {
        fullPath = "\\\\.\\" + diskPath;
    }
    
    HANDLE hDevice = CreateFileA(fullPath.c_str(), 
                                GENERIC_READ, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, 
                                OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    DWORD bytesReturned;
    DISK_GEOMETRY_EX diskGeometry;
    
    BOOL result = DeviceIoControl(hDevice, 
                                 IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                 NULL, 0,
                                 &diskGeometry, sizeof(diskGeometry),
                                 &bytesReturned, NULL);
    
    CloseHandle(hDevice);
    
    if (result) {
        // Calculate total sectors
        LARGE_INTEGER diskSize = diskGeometry.DiskSize;
        uint64_t sectorSize = diskGeometry.Geometry.BytesPerSector;
        
        if (sectorSize > 0) {
            return diskSize.QuadPart / sectorSize;
        }
    }
    
    return 0;
}

bool GUIWindow::repairCDData(const std::string& cdPath, const std::string& checksumFile) {
    if (statusCallback_) {
        statusCallback_("CD/DVD data repair feature is under development...");
    }
    
    // CD/DVD is typically read-only, repair functionality is limited
    // Can implement recovery from image files etc.
    return false;
}

bool GUIWindow::readCDSector(const std::string& cdPath, uint64_t sectorNumber, std::vector<uint8_t>& buffer) {
    std::string fullPath = cdPath;
    if (fullPath.find("\\\\.\\") == std::string::npos) {
        fullPath = "\\\\.\\" + cdPath;
    }
    
    HANDLE hCD = CreateFileA(fullPath.c_str(), 
                            GENERIC_READ, 
                            FILE_SHARE_READ, 
                            NULL, 
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL, 
                            NULL);
    
    if (hCD == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // CD/DVD sector size is typically 2048 bytes
    const uint32_t CD_SECTOR_SIZE = 2048;
    
    // Set file pointer
    LARGE_INTEGER liDistance;
    liDistance.QuadPart = sectorNumber * CD_SECTOR_SIZE;
    
    if (SetFilePointerEx(hCD, liDistance, NULL, FILE_BEGIN) == 0) {
        CloseHandle(hCD);
        return false;
    }
    
    // Read sector data
    DWORD bytesRead;
    buffer.resize(CD_SECTOR_SIZE);
    
    if (!ReadFile(hCD, buffer.data(), CD_SECTOR_SIZE, &bytesRead, NULL)) {
        CloseHandle(hCD);
        return false;
    }
    
    CloseHandle(hCD);
    return bytesRead == CD_SECTOR_SIZE;
}

bool GUIWindow::writeCDSector(const std::string& cdPath, uint64_t sectorNumber, const std::vector<uint8_t>& data) {
    // CD/DVD is typically read-only, write operations are usually not available
    return false;
}

std::vector<std::string> GUIWindow::getAvailableDisks() {
    std::vector<std::string> disks;
    
    // Get logical disks (partitions)
    auto logicalDisks = DiskUtils::getLogicalDisks();
    for (const auto& disk : logicalDisks) {
        disks.push_back(disk.mountPoint);
    }
    
    // Get physical disks
    auto physicalDisks = DiskUtils::getPhysicalDisks();
    for (const auto& disk : physicalDisks) {
        disks.push_back(disk.devicePath);
    }
    
    return disks;
}

std::string GUIWindow::getDiskType(const std::string& diskPath) {
    if (isCDROM(diskPath)) {
        return "CD/DVD-ROM";
    }
    
    // Check if it's a physical disk path
    if (diskPath.find("PhysicalDrive") != std::string::npos) {
        std::string fullPath = diskPath;
        if (fullPath.find("\\\\.\\") == std::string::npos) {
            fullPath = "\\\\.\\" + diskPath;
        }
        
        HANDLE hDevice = CreateFileA(fullPath.c_str(), 
                                    GENERIC_READ, 
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                    NULL, 
                                    OPEN_EXISTING, 
                                    FILE_ATTRIBUTE_NORMAL, 
                                    NULL);
        
        if (hDevice != INVALID_HANDLE_VALUE) {
            DWORD bytesReturned;
            STORAGE_PROPERTY_QUERY query;
            query.PropertyId = StorageDeviceProperty;
            query.QueryType = PropertyStandardQuery;
            
            STORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
            deviceDescriptor.Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
            
            BOOL result = DeviceIoControl(hDevice, 
                                         IOCTL_STORAGE_QUERY_PROPERTY,
                                         &query, sizeof(query),
                                         &deviceDescriptor, sizeof(deviceDescriptor),
                                         &bytesReturned, NULL);
            
            CloseHandle(hDevice);
            
            if (result) {
                if (deviceDescriptor.RemovableMedia) {
                    return "Removable Disk";
                } else {
                    return "Fixed Disk";
                }
            }
        }
        return "Physical Disk";
    }
    
    // For logical drives, use the original method
    UINT driveType = GetDriveTypeA(diskPath.c_str());
    switch (driveType) {
        case DRIVE_REMOVABLE:
            return "Removable Disk";
        case DRIVE_FIXED:
            return "Fixed Disk";
        case DRIVE_REMOTE:
            return "Network Drive";
        case DRIVE_CDROM:
            return "CD/DVD-ROM";
        case DRIVE_RAMDISK:
            return "RAM Disk";
        default:
            return "Unknown Type";
    }
}
