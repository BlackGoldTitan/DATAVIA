#ifndef DISK_UTILS_H
#define DISK_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

// 磁盘信息结构
struct DiskInfo {
    std::string devicePath;      // 设备路径 (如: \\.\PhysicalDrive0)
    std::string mountPoint;      // 挂载点 (如: C:)
    std::string fileSystem;      // 文件系统类型
    uint64_t totalSize;          // 总大小 (字节)
    uint64_t freeSpace;          // 可用空间 (字节)
    bool isRemovable;            // 是否可移动设备
    bool isSystemDisk;           // 是否系统磁盘
};

class DiskUtils {
public:
    // 获取所有逻辑磁盘列表
    static std::vector<DiskInfo> getLogicalDisks();
    
    // 获取所有物理磁盘列表
    static std::vector<DiskInfo> getPhysicalDisks();
    
    // 获取磁盘详细信息
    static DiskInfo getDiskInfo(const std::string& diskPath);
    
    // 检查磁盘是否可访问
    static bool isDiskAccessible(const std::string& diskPath);
    
    // 获取磁盘空间信息
    static bool getDiskSpaceInfo(const std::string& diskPath, uint64_t& totalSize, uint64_t& freeSpace);
    
    // 验证磁盘路径格式
    static bool isValidDiskPath(const std::string& diskPath);
    
    // 获取系统默认磁盘
    static std::string getSystemDisk();
    
    // 获取磁盘类型描述
    static std::string getDiskTypeDescription(const DiskInfo& diskInfo);
    
    // 格式化磁盘信息为字符串
    static std::string formatDiskInfo(const DiskInfo& diskInfo);
    
    // 列出所有可用磁盘（详细格式）
    static std::string listAllDisks();
    
    // 获取磁盘使用率
    static double getDiskUsage(const std::string& diskPath);
};

#endif // DISK_UTILS_H
