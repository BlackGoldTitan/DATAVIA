#ifndef DISK_SECTOR_CRC_H
#define DISK_SECTOR_CRC_H

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

class DiskSectorCRC {
public:
    // 构造函数
    DiskSectorCRC(const std::string& diskPath);
    
    // 析构函数
    ~DiskSectorCRC();
    
    // 生成扇区校验数据
    bool generateSectorChecksums(uint64_t startSector, uint64_t sectorCount, 
                                const std::string& outputFile);
    
    // 验证扇区数据完整性
    bool verifySectorIntegrity(const std::string& checksumFile);
    
    // 修复损坏的扇区数据
    bool repairSectorData(const std::string& checksumFile, const std::string& backupDiskPath = "");
    
    // 获取扇区大小（字节）
    static constexpr uint32_t SECTOR_SIZE = 512;
    
    // 获取最后错误信息
    std::string getLastError() const;

    // 验证文件权限（Windows平台）
    bool checkFilePermissions();

protected:
    std::string diskPath_;
    std::string lastError_;
    
    // 计算数据的CRC32校验和
    uint32_t calculateCRC32(const std::vector<uint8_t>& data);
    
    // 读取指定扇区数据
    bool readSector(uint64_t sectorNumber, std::vector<uint8_t>& buffer);
    
    // 写入指定扇区数据
    bool writeSector(uint64_t sectorNumber, const std::vector<uint8_t>& data);
    
    // 备份扇区数据
    bool backupSector(uint64_t sectorNumber, const std::string& backupPath);
    
    // 从备份恢复扇区数据
    bool restoreSector(uint64_t sectorNumber, const std::string& backupPath);
};

// 校验和数据结构
struct SectorChecksum {
    uint64_t sectorNumber;
    uint32_t crc32;
    uint64_t timestamp;
};

#endif // DISK_SECTOR_CRC_H
