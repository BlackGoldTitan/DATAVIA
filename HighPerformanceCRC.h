#ifndef HIGH_PERFORMANCE_CRC_H
#define HIGH_PERFORMANCE_CRC_H

#include "OptimizedDiskReader.h"
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <fstream>

class HighPerformanceCRC {
public:
    HighPerformanceCRC(const std::string& diskPath);
    ~HighPerformanceCRC();
    
    // 高性能CRC校验生成
    bool generateChecksumsHighPerformance(uint64_t startSector, uint64_t sectorCount,
                                         const std::string& outputFile, 
                                         int readerThreads = 1, int processorThreads = 0,
                                         std::function<void(int, int)> progressCallback = nullptr);
    
    // 获取最后错误信息
    std::string getLastError() const;
    
    // 控制操作
    void cancelOperation();
    bool isOperationCancelled() const;
    void resetCancellation();

private:
    std::string diskPath_;
    std::string lastError_;
    std::atomic<bool> operationCancelled_;
    
    // 数据结构
    struct SectorData {
        uint64_t sectorNumber;
        std::vector<uint8_t> data;
        uint32_t crc;
        uint64_t timestamp;
    };
    
    // 优化的CRC计算
    uint32_t calculateCRC32(const std::vector<uint8_t>& data);
    
    // 优化的工作者函数
    void optimizedReaderWorker(uint64_t startSector, uint64_t endSector,
                              std::queue<SectorData>& dataQueue, std::mutex& queueMutex,
                              std::condition_variable& queueCV, std::atomic<bool>& readingComplete,
                              int batchSize = 128);
    
    void optimizedProcessorWorker(std::queue<SectorData>& dataQueue, std::mutex& queueMutex,
                                 std::condition_variable& queueCV, std::atomic<bool>& readingComplete,
                                 const std::string& outputFile, std::mutex& fileMutex,
                                 std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                                 std::function<void(int, int)> progressCallback);
    
    // 校验和数据结构
    struct SectorChecksum {
        uint64_t sectorNumber;
        uint32_t crc32;
        uint64_t timestamp;
    };
};

#endif // HIGH_PERFORMANCE_CRC_H
