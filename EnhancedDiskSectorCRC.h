#ifndef ENHANCED_DISK_SECTOR_CRC_H
#define ENHANCED_DISK_SECTOR_CRC_H

#include "DiskSectorCRC.h"
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class EnhancedDiskSectorCRC : public DiskSectorCRC {
public:
    EnhancedDiskSectorCRC(const std::string& diskPath);
    ~EnhancedDiskSectorCRC();
    
    // Enhanced methods with cancellation support
    bool generateSectorChecksums(uint64_t startSector, uint64_t sectorCount, 
                                const std::string& outputFile, 
                                std::function<void(int, int)> progressCallback = nullptr);
    
    bool verifySectorIntegrity(const std::string& checksumFile,
                              std::function<void(int, int)> progressCallback = nullptr);
    
    bool repairSectorData(const std::string& checksumFile, const std::string& backupDiskPath,
                         std::function<void(int, int)> progressCallback = nullptr);
    
    // Parallel processing methods
    bool generateChecksumsParallel(uint64_t startSector, uint64_t sectorCount,
                                  const std::string& outputFile, int threadCount = 4,
                                  std::function<void(int, int)> progressCallback = nullptr);
    
    bool verifyIntegrityParallel(const std::string& checksumFile, int threadCount = 4,
                                std::function<void(int, int)> progressCallback = nullptr);
    
    bool repairDataParallel(const std::string& checksumFile, const std::string& backupDiskPath,
                           int threadCount = 4,
                           std::function<void(int, int)> progressCallback = nullptr);
    
    // Control methods
    void cancelOperation();
    bool isOperationCancelled() const;
    void resetCancellation();
    
    // Advanced repair methods
    bool repairFromChecksumFile(const std::string& checksumFile, 
                               const std::string& repairSourcePath = "",
                               std::function<void(int, int)> progressCallback = nullptr);
    
    bool validateChecksumFile(const std::string& checksumFile);
    
    // High-performance parallel processing with dedicated reader thread
    bool generateChecksumsHighPerformance(uint64_t startSector, uint64_t sectorCount,
                                         const std::string& outputFile, int readerThreads = 1,
                                         int processorThreads = 0,
                                         std::function<void(int, int)> progressCallback = nullptr);
    
    bool verifyIntegrityHighPerformance(const std::string& checksumFile, int readerThreads = 1,
                                       int processorThreads = 0,
                                       std::function<void(int, int)> progressCallback = nullptr);
    
    // Get last error message
    std::string getLastError() const { return lastError_; }
    
private:
    std::atomic<bool> operationCancelled_;
    std::mutex cancellationMutex_;
    std::condition_variable cancellationCV_;
    
    // Data structures for producer-consumer pattern
    struct SectorData {
        uint64_t sectorNumber;
        std::vector<uint8_t> data;
        uint32_t crc;
        uint64_t timestamp;
    };
    
    // High-performance worker functions
    void readerWorker(uint64_t startSector, uint64_t endSector,
                     std::queue<SectorData>& dataQueue, std::mutex& queueMutex,
                     std::condition_variable& queueCV, std::atomic<bool>& readingComplete,
                     int batchSize = 64);
    
    void processorWorker(std::queue<SectorData>& dataQueue, std::mutex& queueMutex,
                        std::condition_variable& queueCV, std::atomic<bool>& readingComplete,
                        const std::string& outputFile, std::mutex& fileMutex,
                        std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                        std::function<void(int, int)> progressCallback);
    
    // Optimized batch reading for maximum throughput
    bool readSectorsBatch(uint64_t startSector, uint64_t count, std::vector<std::vector<uint8_t>>& batchData);
    
    // Worker thread functions
    void checksumWorker(uint64_t startSector, uint64_t endSector, 
                       const std::string& outputFile, std::mutex& fileMutex,
                       std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                       std::function<void(int, int)> progressCallback);
    
    // Optimized worker with batch reading
    void checksumWorkerBatch(uint64_t startSector, uint64_t endSector, 
                            const std::string& outputFile, std::mutex& fileMutex,
                            std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                            std::function<void(int, int)> progressCallback,
                            int batchSize = 64);
    
    // Streaming worker with automatic memory release
    void checksumWorkerStreaming(uint64_t startSector, uint64_t endSector, 
                                const std::string& outputFile, std::mutex& fileMutex,
                                std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                                std::function<void(int, int)> progressCallback,
                                int bufferSize = 32);
    
    void verificationWorker(const std::vector<SectorChecksum>& checksums,
                           std::atomic<uint64_t>& corruptedCount,
                           std::atomic<uint64_t>& processedCount,
                           std::function<void(int, int)> progressCallback);
    
    void repairWorker(const std::vector<SectorChecksum>& checksums,
                     const std::string& backupDiskPath,
                     std::atomic<uint64_t>& repairedCount,
                     std::atomic<uint64_t>& processedCount,
                     std::function<void(int, int)> progressCallback);
    
    // Helper methods
    bool readChecksumFile(const std::string& checksumFile, 
                         std::vector<SectorChecksum>& checksums,
                         uint64_t& startSector, uint64_t& sectorCount);
    
    bool findRepairSource(const std::string& checksumFile, std::string& repairSource);
};

#endif // ENHANCED_DISK_SECTOR_CRC_H
