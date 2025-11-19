#include "DiskSectorCRC.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <windows.h>
#include <algorithm>

// CRC32 lookup table
static const uint32_t crc32_table[] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E4E8, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

DiskSectorCRC::DiskSectorCRC(const std::string& diskPath) : diskPath_(diskPath) {
    // On Windows, disk path needs to start with "\\\\.\\"
    if (diskPath_.find("\\\\.\\") == std::string::npos) {
        diskPath_ = "\\\\.\\" + diskPath_;
    }
}

DiskSectorCRC::~DiskSectorCRC() {
    // Clean up resources
}

uint32_t DiskSectorCRC::calculateCRC32(const std::vector<uint8_t>& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

bool DiskSectorCRC::readSector(uint64_t sectorNumber, std::vector<uint8_t>& buffer) {
    HANDLE hDisk = CreateFileA(diskPath_.c_str(), 
                              GENERIC_READ, 
                              FILE_SHARE_READ | FILE_SHARE_WRITE, 
                              NULL, 
                              OPEN_EXISTING, 
                              FILE_ATTRIBUTE_NORMAL, 
                              NULL);
    
    if (hDisk == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::string errorMsg;
        
        switch (error) {
            case ERROR_ACCESS_DENIED:
                errorMsg = "Access denied. Please run the program with administrator privileges";
                break;
            case ERROR_FILE_NOT_FOUND:
                errorMsg = "Disk not found. Please check the disk path: " + diskPath_;
                break;
            case ERROR_INVALID_PARAMETER:
                errorMsg = "Invalid disk path: " + diskPath_;
                break;
            case ERROR_SHARING_VIOLATION:
                errorMsg = "Disk is in use by another process: " + diskPath_;
                break;
            default:
                errorMsg = "Cannot open disk: " + diskPath_ + " (Error code: " + std::to_string(error) + ")";
                break;
        }
        
        lastError_ = errorMsg;
        return false;
    }
    
    // Set file pointer to specified sector
    LARGE_INTEGER liDistance;
    liDistance.QuadPart = sectorNumber * SECTOR_SIZE;
    
    if (SetFilePointerEx(hDisk, liDistance, NULL, FILE_BEGIN) == 0) {
        lastError_ = "Cannot set file pointer to sector: " + std::to_string(sectorNumber);
        CloseHandle(hDisk);
        return false;
    }
    
    // Read sector data
    DWORD bytesRead;
    buffer.resize(SECTOR_SIZE);
    
    if (!ReadFile(hDisk, buffer.data(), SECTOR_SIZE, &bytesRead, NULL)) {
        lastError_ = "Failed to read sector: " + std::to_string(sectorNumber);
        CloseHandle(hDisk);
        return false;
    }
    
    CloseHandle(hDisk);
    return bytesRead == SECTOR_SIZE;
}

bool DiskSectorCRC::writeSector(uint64_t sectorNumber, const std::vector<uint8_t>& data) {
    if (data.size() != SECTOR_SIZE) {
        lastError_ = "Data size does not equal sector size";
        return false;
    }
    
    HANDLE hDisk = CreateFileA(diskPath_.c_str(), 
                              GENERIC_WRITE, 
                              FILE_SHARE_READ | FILE_SHARE_WRITE, 
                              NULL, 
                              OPEN_EXISTING, 
                              FILE_ATTRIBUTE_NORMAL, 
                              NULL);
    
    if (hDisk == INVALID_HANDLE_VALUE) {
        lastError_ = "Cannot open disk for writing: " + diskPath_;
        return false;
    }
    
    // Set file pointer to specified sector
    LARGE_INTEGER liDistance;
    liDistance.QuadPart = sectorNumber * SECTOR_SIZE;
    
    if (SetFilePointerEx(hDisk, liDistance, NULL, FILE_BEGIN) == 0) {
        lastError_ = "Cannot set file pointer to sector: " + std::to_string(sectorNumber);
        CloseHandle(hDisk);
        return false;
    }
    
    // Write sector data
    DWORD bytesWritten;
    
    if (!WriteFile(hDisk, data.data(), SECTOR_SIZE, &bytesWritten, NULL)) {
        lastError_ = "Failed to write sector: " + std::to_string(sectorNumber);
        CloseHandle(hDisk);
        return false;
    }
    
    CloseHandle(hDisk);
    return bytesWritten == SECTOR_SIZE;
}

bool DiskSectorCRC::generateSectorChecksums(uint64_t startSector, uint64_t sectorCount, 
                                           const std::string& outputFile) {
    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile.is_open()) {
        lastError_ = "Cannot create output file: " + outputFile;
        return false;
    }
    
    // Write file header
    uint32_t magic = 0x43524344; // "CRCD"
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    outFile.write(reinterpret_cast<const char*>(&startSector), sizeof(startSector));
    outFile.write(reinterpret_cast<const char*>(&sectorCount), sizeof(sectorCount));
    outFile.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    
    std::cout << "Generating sector checksum data..." << std::endl;
    std::cout << "Start sector: " << startSector << std::endl;
    std::cout << "Sector count: " << sectorCount << std::endl;
    
    // Generate checksum for each sector
    for (uint64_t i = 0; i < sectorCount; ++i) {
        uint64_t currentSector = startSector + i;
        std::vector<uint8_t> sectorData;
        
        if (!readSector(currentSector, sectorData)) {
            lastError_ = "Failed to read sector " + std::to_string(currentSector) + ": " + lastError_;
            outFile.close();
            return false;
        }
        
        uint32_t crc = calculateCRC32(sectorData);
        SectorChecksum checksum{currentSector, crc, timestamp};
        
        outFile.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
        
        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << sectorCount << " sectors" << std::endl;
        }
    }
    
    outFile.close();
    std::cout << "Checksum data generation completed, saved to: " << outputFile << std::endl;
    return true;
}

bool DiskSectorCRC::verifySectorIntegrity(const std::string& checksumFile) {
    std::ifstream inFile(checksumFile, std::ios::binary);
    if (!inFile.is_open()) {
        lastError_ = "Cannot open checksum file: " + checksumFile;
        return false;
    }
    
    // Read file header
    uint32_t magic;
    uint64_t startSector, sectorCount, timestamp;
    
    inFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    inFile.read(reinterpret_cast<char*>(&startSector), sizeof(startSector));
    inFile.read(reinterpret_cast<char*>(&sectorCount), sizeof(sectorCount));
    inFile.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    
    if (magic != 0x43524344) {
        lastError_ = "Invalid checksum file format";
        inFile.close();
        return false;
    }
    
    std::cout << "Verifying sector data integrity..." << std::endl;
    std::cout << "Start sector: " << startSector << std::endl;
    std::cout << "Sector count: " << sectorCount << std::endl;
    
    bool allValid = true;
    uint64_t corruptedSectors = 0;
    
    // Verify each sector
    for (uint64_t i = 0; i < sectorCount; ++i) {
        SectorChecksum storedChecksum;
        inFile.read(reinterpret_cast<char*>(&storedChecksum), sizeof(storedChecksum));
        
        if (inFile.gcount() != sizeof(storedChecksum)) {
            lastError_ = "Failed to read checksum data";
            inFile.close();
            return false;
        }
        
        std::vector<uint8_t> currentSectorData;
        if (!readSector(storedChecksum.sectorNumber, currentSectorData)) {
            lastError_ = "Failed to read sector " + std::to_string(storedChecksum.sectorNumber) + ": " + lastError_;
            inFile.close();
            return false;
        }
        
        uint32_t currentCRC = calculateCRC32(currentSectorData);
        
        if (currentCRC != storedChecksum.crc32) {
            std::cout << "Sector " << storedChecksum.sectorNumber << " data corrupted!" << std::endl;
            allValid = false;
            corruptedSectors++;
        }
        
        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << sectorCount << " sectors" << std::endl;
        }
    }
    
    inFile.close();
    
    if (allValid) {
        std::cout << "All sectors data integrity verification passed!" << std::endl;
    } else {
        std::cout << "Found " << corruptedSectors << " corrupted sectors" << std::endl;
    }
    
    return allValid;
}

bool DiskSectorCRC::repairSectorData(const std::string& checksumFile, const std::string& backupDiskPath) {
    std::ifstream inFile(checksumFile, std::ios::binary);
    if (!inFile.is_open()) {
        lastError_ = "Cannot open checksum file: " + checksumFile;
        return false;
    }
    
    // Read file header
    uint32_t magic;
    uint64_t startSector, sectorCount, timestamp;
    
    inFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    inFile.read(reinterpret_cast<char*>(&startSector), sizeof(startSector));
    inFile.read(reinterpret_cast<char*>(&sectorCount), sizeof(sectorCount));
    inFile.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    
    if (magic != 0x43524344) {
        lastError_ = "Invalid checksum file format";
        inFile.close();
        return false;
    }
    
    std::cout << "Repairing sector data..." << std::endl;
    std::cout << "Start sector: " << startSector << std::endl;
    std::cout << "Sector count: " << sectorCount << std::endl;
    
    bool backupAvailable = !backupDiskPath.empty();
    std::string backupDisk = backupDiskPath;
    
    if (backupAvailable && backupDisk.find("\\\\.\\") == std::string::npos) {
        backupDisk = "\\\\.\\" + backupDiskPath;
    }
    
    uint64_t repairedSectors = 0;
    uint64_t totalCorrupted = 0;
    
    // Check each sector and attempt repair
    for (uint64_t i = 0; i < sectorCount; ++i) {
        SectorChecksum storedChecksum;
        inFile.read(reinterpret_cast<char*>(&storedChecksum), sizeof(storedChecksum));
        
        if (inFile.gcount() != sizeof(storedChecksum)) {
            lastError_ = "Failed to read checksum data";
            inFile.close();
            return false;
        }
        
        std::vector<uint8_t> currentSectorData;
        if (!readSector(storedChecksum.sectorNumber, currentSectorData)) {
            lastError_ = "Failed to read sector " + std::to_string(storedChecksum.sectorNumber) + ": " + lastError_;
            inFile.close();
            return false;
        }
        
        uint32_t currentCRC = calculateCRC32(currentSectorData);
        
        if (currentCRC != storedChecksum.crc32) {
            totalCorrupted++;
            std::cout << "Found corrupted sector: " << storedChecksum.sectorNumber << std::endl;
            
            // Attempt recovery from backup disk
            if (backupAvailable) {
                DiskSectorCRC backupDiskObj(backupDisk);
                std::vector<uint8_t> backupData;
                
                if (backupDiskObj.readSector(storedChecksum.sectorNumber, backupData)) {
                    uint32_t backupCRC = calculateCRC32(backupData);
                    
                    if (backupCRC == storedChecksum.crc32) {
                        // Write data from backup
                        if (writeSector(storedChecksum.sectorNumber, backupData)) {
                            std::cout << "Sector " << storedChecksum.sectorNumber << " restored from backup" << std::endl;
                            repairedSectors++;
                        } else {
                            std::cout << "Sector " << storedChecksum.sectorNumber << " restoration failed: " << lastError_ << std::endl;
                        }
                    } else {
                        std::cout << "Backup sector " << storedChecksum.sectorNumber << " also corrupted, cannot restore" << std::endl;
                    }
                } else {
                    std::cout << "Cannot read backup sector " << storedChecksum.sectorNumber << std::endl;
                }
            } else {
                std::cout << "Sector " << storedChecksum.sectorNumber << " corrupted, but no backup available" << std::endl;
            }
        }
        
        if ((i + 1) % 100 == 0) {
            std::cout << "Progress: " << (i + 1) << "/" << sectorCount << " sectors" << std::endl;
        }
    }
    
    inFile.close();
    
    std::cout << "Repair completed:" << std::endl;
    std::cout << "Total corrupted sectors: " << totalCorrupted << std::endl;
    std::cout << "Successfully repaired sectors: " << repairedSectors << std::endl;
    
    return repairedSectors > 0 || totalCorrupted == 0;
}

bool DiskSectorCRC::checkFilePermissions() {
    // Windows platform permission check
    HANDLE hDisk = CreateFileA(diskPath_.c_str(), 
                              GENERIC_READ, 
                              FILE_SHARE_READ | FILE_SHARE_WRITE, 
                              NULL, 
                              OPEN_EXISTING, 
                              FILE_ATTRIBUTE_NORMAL, 
                              NULL);
    
    if (hDisk == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::string errorMsg;
        
        switch (error) {
            case ERROR_ACCESS_DENIED:
                errorMsg = "Access denied. Please run the program with administrator privileges";
                break;
            case ERROR_FILE_NOT_FOUND:
                errorMsg = "Disk not found. Please check the disk path: " + diskPath_;
                break;
            case ERROR_INVALID_PARAMETER:
                errorMsg = "Invalid disk path: " + diskPath_;
                break;
            case ERROR_SHARING_VIOLATION:
                errorMsg = "Disk is in use by another process: " + diskPath_;
                break;
            default:
                errorMsg = "Cannot access disk: " + diskPath_ + " (Error code: " + std::to_string(error) + ")";
                break;
        }
        
        lastError_ = errorMsg;
        return false;
    }
    
    CloseHandle(hDisk);
    return true;
}

bool DiskSectorCRC::backupSector(uint64_t sectorNumber, const std::string& backupPath) {
    std::vector<uint8_t> sectorData;
    if (!readSector(sectorNumber, sectorData)) {
        return false;
    }
    
    std::ofstream backupFile(backupPath, std::ios::binary | std::ios::app);
    if (!backupFile.is_open()) {
        lastError_ = "Cannot create backup file: " + backupPath;
        return false;
    }
    
    // Write sector number and data
    backupFile.write(reinterpret_cast<const char*>(&sectorNumber), sizeof(sectorNumber));
    backupFile.write(reinterpret_cast<const char*>(sectorData.data()), sectorData.size());
    
    backupFile.close();
    return true;
}

bool DiskSectorCRC::restoreSector(uint64_t sectorNumber, const std::string& backupPath) {
    std::ifstream backupFile(backupPath, std::ios::binary);
    if (!backupFile.is_open()) {
        lastError_ = "Cannot open backup file: " + backupPath;
        return false;
    }
    
    // Find specified sector in backup file
    while (backupFile) {
        uint64_t backupSectorNumber;
        std::vector<uint8_t> sectorData(SECTOR_SIZE);
        
        backupFile.read(reinterpret_cast<char*>(&backupSectorNumber), sizeof(backupSectorNumber));
        if (backupFile.gcount() != sizeof(backupSectorNumber)) {
            break;
        }
        
        backupFile.read(reinterpret_cast<char*>(sectorData.data()), SECTOR_SIZE);
        if (backupFile.gcount() != SECTOR_SIZE) {
            break;
        }
        
        if (backupSectorNumber == sectorNumber) {
            backupFile.close();
            return writeSector(sectorNumber, sectorData);
        }
    }
    
    backupFile.close();
    lastError_ = "Sector not found in backup file: " + std::to_string(sectorNumber);
    return false;
}

std::string DiskSectorCRC::getLastError() const {
    return lastError_;
}
