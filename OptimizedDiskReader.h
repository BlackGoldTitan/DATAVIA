#ifndef OPTIMIZED_DISK_READER_H
#define OPTIMIZED_DISK_READER_H

#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

class OptimizedDiskReader {
public:
    OptimizedDiskReader(const std::string& diskPath);
    ~OptimizedDiskReader();
    
    // 保持磁盘句柄打开状态
    bool openDisk();
    void closeDisk();
    
    // 批量读取扇区 - 核心优化
    bool readSectorsBatch(uint64_t startSector, uint64_t count, std::vector<std::vector<uint8_t>>& batchData);
    
    // 单个扇区读取（使用已打开的句柄）
    bool readSector(uint64_t sectorNumber, std::vector<uint8_t>& buffer);
    
    // 获取最后错误信息
    std::string getLastError() const;
    
    // 检查磁盘是否已打开
    bool isOpen() const { return hDisk_ != INVALID_HANDLE_VALUE; }
    
    // 设置批量读取大小
    void setBatchSize(size_t batchSize) { batchSize_ = batchSize; }
    
    // 预分配缓冲区以减少内存分配
    void preallocateBuffers(size_t count);

private:
    std::string diskPath_;
    HANDLE hDisk_;
    std::string lastError_;
    size_t batchSize_;
    
    // 预分配的缓冲区池
    std::vector<std::vector<uint8_t>> bufferPool_;
    size_t nextBufferIndex_;
    
    // 内部辅助方法
    bool ensureDiskOpen();
    std::vector<uint8_t>& acquireBuffer();
    void releaseAllBuffers();
};

#endif // OPTIMIZED_DISK_READER_H
