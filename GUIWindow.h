#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include "DiskSectorCRC.h"
#include "DiskUtils.h"
#include <string>
#include <vector>
#include <functional>

class GUIWindow {
public:
    GUIWindow();
    ~GUIWindow();
    
    // 显示主窗口
    void show();
    
    // 设置状态回调
    void setStatusCallback(std::function<void(const std::string&)> callback);
    void setProgressCallback(std::function<void(int, int)> callback);
    
    // 操作接口
    bool generateChecksums(const std::string& diskPath, uint64_t startSector, 
                          uint64_t sectorCount, const std::string& outputFile);
    bool verifyIntegrity(const std::string& diskPath, const std::string& checksumFile);
    bool repairData(const std::string& diskPath, const std::string& checksumFile, 
                   const std::string& backupDiskPath = "");
    
    // 光碟支持
    bool isCDROM(const std::string& diskPath);
    bool generateCDChecksums(const std::string& cdPath, const std::string& outputFile);
    bool verifyCDIntegrity(const std::string& cdPath, const std::string& checksumFile);
    bool repairCDData(const std::string& cdPath, const std::string& checksumFile);
    
    // 磁盘管理
    std::vector<std::string> getAvailableDisks();
    std::string getDiskType(const std::string& diskPath);
    uint64_t getDiskTotalSectors(const std::string& diskPath);
    
private:
    std::function<void(const std::string&)> statusCallback_;
    std::function<void(int, int)> progressCallback_;
    DiskSectorCRC* diskCRC_;
    
    // 光碟特定操作
    bool readCDSector(const std::string& cdPath, uint64_t sectorNumber, std::vector<uint8_t>& buffer);
    bool writeCDSector(const std::string& cdPath, uint64_t sectorNumber, const std::vector<uint8_t>& data);
};

#endif // GUI_WINDOW_H
