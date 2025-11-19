#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

class DiskListTool {
public:
    static void listPhysicalDisks() {
        std::cout << "=== Physical Disk List ===" << std::endl;
        std::cout << std::endl;
        
        // 尝试列出物理磁盘
        for (int i = 0; i < 16; i++) {
            std::string diskPath = "\\\\.\\PhysicalDrive" + std::to_string(i);
            HANDLE hDisk = CreateFileA(diskPath.c_str(), 
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
            
            if (hDisk != INVALID_HANDLE_VALUE) {
                std::cout << "Found: " << diskPath << std::endl;
                
                // 获取磁盘信息
                DISK_GEOMETRY geometry;
                DWORD bytesReturned;
                
                if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                  NULL, 0, &geometry, sizeof(geometry),
                                  &bytesReturned, NULL)) {
                    uint64_t diskSize = geometry.Cylinders.QuadPart *
                                       geometry.TracksPerCylinder *
                                       geometry.SectorsPerTrack *
                                       geometry.BytesPerSector;
                    
                    std::cout << "  Size: " << (diskSize / (1024 * 1024 * 1024)) << " GB" << std::endl;
                    std::cout << "  Sectors: " << (diskSize / 512) << std::endl;
                    std::cout << "  Bytes per sector: " << geometry.BytesPerSector << std::endl;
                }
                
                CloseHandle(hDisk);
                std::cout << std::endl;
            }
        }
        
        std::cout << "=== Logical Drives ===" << std::endl;
        std::cout << std::endl;
        
        // 列出逻辑驱动器
        DWORD drives = GetLogicalDrives();
        for (char drive = 'A'; drive <= 'Z'; drive++) {
            if (drives & (1 << (drive - 'A'))) {
                std::string drivePath = std::string(1, drive) + ":\\";
                std::string diskPath = "\\\\.\\" + std::string(1, drive) + ":";
                
                std::cout << "Drive " << drive << ": " << drivePath << std::endl;
                std::cout << "  Disk path: " << diskPath << std::endl;
                
                // 获取驱动器类型
                UINT type = GetDriveTypeA(drivePath.c_str());
                std::string typeStr;
                switch (type) {
                    case DRIVE_FIXED: typeStr = "Fixed Disk"; break;
                    case DRIVE_REMOVABLE: typeStr = "Removable Disk"; break;
                    case DRIVE_CDROM: typeStr = "CD-ROM"; break;
                    case DRIVE_REMOTE: typeStr = "Network Drive"; break;
                    default: typeStr = "Unknown"; break;
                }
                std::cout << "  Type: " << typeStr << std::endl;
                
                // 获取磁盘空间
                ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
                if (GetDiskFreeSpaceExA(drivePath.c_str(), &freeBytes, &totalBytes, &totalFreeBytes)) {
                    std::cout << "  Total size: " << (totalBytes.QuadPart / (1024 * 1024 * 1024)) << " GB" << std::endl;
                    std::cout << "  Free space: " << (freeBytes.QuadPart / (1024 * 1024 * 1024)) << " GB" << std::endl;
                }
                std::cout << std::endl;
            }
        }
        
        std::cout << "=== Usage Instructions ===" << std::endl;
        std::cout << "For physical disks, use: \\\\.\\PhysicalDriveX (where X is the disk number)" << std::endl;
        std::cout << "For logical drives, use: \\\\.\\C: (replace C with the drive letter)" << std::endl;
        std::cout << "Note: Administrator privileges are required to access physical disks." << std::endl;
    }
};

int main() {
    std::cout << "Disk List Tool - Find correct disk paths for CRCRECOVER" << std::endl;
    std::cout << "=======================================================" << std::endl;
    std::cout << std::endl;
    
    // 检查管理员权限
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    
    if (!isAdmin) {
        std::cout << "WARNING: Not running as administrator." << std::endl;
        std::cout << "Physical disk access may be limited." << std::endl;
        std::cout << "Please run this tool as administrator for full disk listing." << std::endl;
        std::cout << std::endl;
    }
    
    DiskListTool::listPhysicalDisks();
    
    std::cout << "Press Enter to exit...";
    std::cin.get();
    
    return 0;
}
