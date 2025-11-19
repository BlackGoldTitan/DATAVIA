#include "DiskUtils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#else
#include <sys/statvfs.h>
#include <mntent.h>
#endif

namespace fs = std::filesystem;

// 获取所有逻辑磁盘列表
std::vector<DiskInfo> DiskUtils::getLogicalDisks() {
    std::vector<DiskInfo> disks;
    
#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drives & (1 << (drive - 'A'))) {
            std::string drivePath = std::string(1, drive) + ":\\";
            
            DiskInfo info;
            info.mountPoint = std::string(1, drive) + ":";
            info.devicePath = "\\\\.\\" + std::string(1, drive) + ":";
            
            // 获取磁盘空间信息
            ULARGE_INTEGER totalBytes, freeBytes;
            if (GetDiskFreeSpaceExA(drivePath.c_str(), nullptr, &totalBytes, &freeBytes)) {
                info.totalSize = totalBytes.QuadPart;
                info.freeSpace = freeBytes.QuadPart;
            }
            
            // 获取文件系统类型
            char fileSystem[32];
            DWORD maxComponentLength, fileSystemFlags;
            if (GetVolumeInformationA(drivePath.c_str(), nullptr, 0, nullptr, 
                                    &maxComponentLength, &fileSystemFlags, 
                                    fileSystem, sizeof(fileSystem))) {
                info.fileSystem = fileSystem;
            }
            
            // 判断是否为可移动设备
            UINT driveType = GetDriveTypeA(drivePath.c_str());
            info.isRemovable = (driveType == DRIVE_REMOVABLE);
            
            // 判断是否为系统磁盘
            char systemDrive[4];
            GetSystemDirectoryA(systemDrive, sizeof(systemDrive));
            info.isSystemDisk = (std::string(systemDrive).substr(0, 2) == info.mountPoint);
            
            disks.push_back(info);
        }
    }
#else
    // Linux实现
    FILE* mtab = setmntent("/etc/mtab", "r");
    if (mtab) {
        struct mntent* entry;
        while ((entry = getmntent(mtab)) != nullptr) {
            if (entry->mnt_fsname && entry->mnt_dir) {
                DiskInfo info;
                info.devicePath = entry->mnt_fsname;
                info.mountPoint = entry->mnt_dir;
                info.fileSystem = entry->mnt_type;
                
                // 获取磁盘空间信息
                struct statvfs vfs;
                if (statvfs(entry->mnt_dir, &vfs) == 0) {
                    info.totalSize = vfs.f_blocks * vfs.f_frsize;
                    info.freeSpace = vfs.f_bfree * vfs.f_frsize;
                }
                
                // 判断是否为可移动设备（简化实现）
                info.isRemovable = (info.fileSystem == "vfat" || info.fileSystem == "exfat" || 
                                  info.fileSystem == "ntfs" || info.mountPoint.find("/media/") == 0);
                
                // 判断是否为系统磁盘
                info.isSystemDisk = (info.mountPoint == "/");
                
                disks.push_back(info);
            }
        }
        endmntent(mtab);
    }
#endif
    
    return disks;
}

// 获取所有物理磁盘列表
std::vector<DiskInfo> DiskUtils::getPhysicalDisks() {
    std::vector<DiskInfo> disks;
    
#ifdef _WIN32
    // Windows下枚举物理磁盘
    for (int i = 0; i < 16; ++i) {  // 最多16个物理磁盘
        std::string devicePath = "\\\\.\\PhysicalDrive" + std::to_string(i);
        HANDLE hDevice = CreateFileA(devicePath.c_str(), GENERIC_READ, 
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        
        if (hDevice != INVALID_HANDLE_VALUE) {
            DiskInfo info;
            info.devicePath = devicePath;
            info.mountPoint = "";  // 物理磁盘没有挂载点
            
            // 获取磁盘大小
            GET_LENGTH_INFORMATION lengthInfo;
            DWORD bytesReturned;
            if (DeviceIoControl(hDevice, IOCTL_DISK_GET_LENGTH_INFO, 
                              nullptr, 0, &lengthInfo, sizeof(lengthInfo), 
                              &bytesReturned, nullptr)) {
                info.totalSize = lengthInfo.Length.QuadPart;
                info.freeSpace = 0;  // 物理磁盘没有可用空间概念
            }
            
            info.fileSystem = "RAW";
            info.isRemovable = false;
            info.isSystemDisk = (i == 0);  // 假设第一个磁盘是系统磁盘
            
            disks.push_back(info);
            CloseHandle(hDevice);
        }
    }
#else
    // Linux下枚举物理磁盘
    for (int i = 0; i < 16; ++i) {
        std::string devicePath = "/dev/sd" + std::string(1, 'a' + i);
        if (fs::exists(devicePath)) {
            DiskInfo info;
            info.devicePath = devicePath;
            info.mountPoint = "";
            info.fileSystem = "RAW";
            info.isRemovable = false;
            info.isSystemDisk = (i == 0);
            
            // 获取磁盘大小（需要root权限）
            std::ifstream sizeFile("/sys/block/sd" + std::string(1, 'a' + i) + "/size");
            if (sizeFile) {
                uint64_t sectors;
                sizeFile >> sectors;
                info.totalSize = sectors * 512;  // 假设扇区大小为512字节
            }
            
            disks.push_back(info);
        }
    }
#endif
    
    return disks;
}

// 获取磁盘详细信息
DiskInfo DiskUtils::getDiskInfo(const std::string& diskPath) {
    DiskInfo info;
    
    // 检查是逻辑磁盘还是物理磁盘
    if (diskPath.find("PhysicalDrive") != std::string::npos || 
        diskPath.find("/dev/sd") == 0) {
        // 物理磁盘
        auto physicalDisks = getPhysicalDisks();
        for (const auto& disk : physicalDisks) {
            if (disk.devicePath == diskPath) {
                return disk;
            }
        }
    } else {
        // 逻辑磁盘
        auto logicalDisks = getLogicalDisks();
        for (const auto& disk : logicalDisks) {
            if (disk.devicePath == diskPath || disk.mountPoint == diskPath) {
                return disk;
            }
        }
    }
    
    return info;  // 返回空信息
}

// 检查磁盘是否可访问
bool DiskUtils::isDiskAccessible(const std::string& diskPath) {
#ifdef _WIN32
    HANDLE hDevice = CreateFileA(diskPath.c_str(), GENERIC_READ, 
                               FILE_SHARE_READ | FILE_SHARE_WRITE, 
                               nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        return true;
    }
    return false;
#else
    return fs::exists(diskPath);
#endif
}

// 获取磁盘空间信息
bool DiskUtils::getDiskSpaceInfo(const std::string& diskPath, uint64_t& totalSize, uint64_t& freeSpace) {
#ifdef _WIN32
    ULARGE_INTEGER totalBytes, freeBytes;
    if (GetDiskFreeSpaceExA(diskPath.c_str(), nullptr, &totalBytes, &freeBytes)) {
        totalSize = totalBytes.QuadPart;
        freeSpace = freeBytes.QuadPart;
        return true;
    }
#else
    struct statvfs vfs;
    if (statvfs(diskPath.c_str(), &vfs) == 0) {
        totalSize = vfs.f_blocks * vfs.f_frsize;
        freeSpace = vfs.f_bfree * vfs.f_frsize;
        return true;
    }
#endif
    return false;
}

// 验证磁盘路径格式
bool DiskUtils::isValidDiskPath(const std::string& diskPath) {
    if (diskPath.empty()) return false;
    
#ifdef _WIN32
    // Windows格式：C: 或 \\.\C: 或 \\.\PhysicalDrive0
    if (diskPath.length() == 2 && diskPath[1] == ':') {
        return true;
    }
    if (diskPath.find("\\\\.\\") == 0) {
        return true;
    }
#else
    // Linux格式：/dev/sda 或 /mnt/disk
    if (diskPath.find("/dev/") == 0 || diskPath.find("/mnt/") == 0 || 
        diskPath.find("/media/") == 0) {
        return true;
    }
#endif
    
    return false;
}

// 获取系统默认磁盘
std::string DiskUtils::getSystemDisk() {
#ifdef _WIN32
    char systemDrive[4];
    GetSystemDirectoryA(systemDrive, sizeof(systemDrive));
    return std::string(systemDrive).substr(0, 2);  // 返回 "C:"
#else
    return "/";  // Linux系统根目录
#endif
}

// 获取磁盘类型描述
std::string DiskUtils::getDiskTypeDescription(const DiskInfo& diskInfo) {
    if (diskInfo.isSystemDisk) {
        return "System Disk";
    } else if (diskInfo.isRemovable) {
        return "Removable Disk";
    } else if (diskInfo.mountPoint.empty()) {
        return "Physical Disk";
    } else {
        return "Logical Disk";
    }
}

// 格式化磁盘信息为字符串
std::string DiskUtils::formatDiskInfo(const DiskInfo& diskInfo) {
    std::ostringstream oss;
    
    oss << "Device: " << diskInfo.devicePath << "\n";
    if (!diskInfo.mountPoint.empty()) {
        oss << "Mount Point: " << diskInfo.mountPoint << "\n";
    }
    oss << "Type: " << getDiskTypeDescription(diskInfo) << "\n";
    
    if (!diskInfo.fileSystem.empty()) {
        oss << "File System: " << diskInfo.fileSystem << "\n";
    }
    
    if (diskInfo.totalSize > 0) {
        double totalGB = diskInfo.totalSize / (1024.0 * 1024.0 * 1024.0);
        oss << "Total Size: " << std::fixed << std::setprecision(2) << totalGB << " GB\n";
    }
    
    if (diskInfo.freeSpace > 0) {
        double freeGB = diskInfo.freeSpace / (1024.0 * 1024.0 * 1024.0);
        oss << "Free Space: " << std::fixed << std::setprecision(2) << freeGB << " GB\n";
    }
    
    return oss.str();
}

// 列出所有可用磁盘（详细格式）
std::string DiskUtils::listAllDisks() {
    std::ostringstream oss;
    
    auto logicalDisks = getLogicalDisks();
    auto physicalDisks = getPhysicalDisks();
    
    oss << "=== Logical Disks ===\n";
    for (size_t i = 0; i < logicalDisks.size(); ++i) {
        const auto& disk = logicalDisks[i];
        oss << "[" << i << "] " << disk.mountPoint << " (" << disk.devicePath << ")\n";
        oss << "    Type: " << getDiskTypeDescription(disk) << "\n";
        if (!disk.fileSystem.empty()) {
            oss << "    File System: " << disk.fileSystem << "\n";
        }
        if (disk.totalSize > 0) {
            double totalGB = disk.totalSize / (1024.0 * 1024.0 * 1024.0);
            double usage = (disk.totalSize - disk.freeSpace) * 100.0 / disk.totalSize;
            oss << "    Size: " << std::fixed << std::setprecision(2) << totalGB 
                << " GB (Used: " << std::setprecision(1) << usage << "%)\n";
        }
        oss << "\n";
    }
    
    oss << "=== Physical Disks ===\n";
    for (size_t i = 0; i < physicalDisks.size(); ++i) {
        const auto& disk = physicalDisks[i];
        oss << "[" << i + logicalDisks.size() << "] " << disk.devicePath << "\n";
        oss << "    Type: " << getDiskTypeDescription(disk) << "\n";
        if (disk.totalSize > 0) {
            double totalGB = disk.totalSize / (1024.0 * 1024.0 * 1024.0);
            oss << "    Size: " << std::fixed << std::setprecision(2) << totalGB << " GB\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

// 获取磁盘使用率
double DiskUtils::getDiskUsage(const std::string& diskPath) {
    uint64_t totalSize, freeSpace;
    if (getDiskSpaceInfo(diskPath, totalSize, freeSpace) && totalSize > 0) {
        return (totalSize - freeSpace) * 100.0 / totalSize;
    }
    return 0.0;
}
