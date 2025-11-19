#include "EnhancedDiskSectorCRC.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>

EnhancedDiskSectorCRC::EnhancedDiskSectorCRC(const std::string& diskPath) 
    : DiskSectorCRC(diskPath), operationCancelled_(false) {
}

EnhancedDiskSectorCRC::~EnhancedDiskSectorCRC() {
    cancelOperation();
}

// Enhanced methods with cancellation support
bool EnhancedDiskSectorCRC::generateSectorChecksums(uint64_t startSector, uint64_t sectorCount, 
                                                   const std::string& outputFile, 
                                                   std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
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
    
    // Generate checksum for each sector with cancellation support
    for (uint64_t i = 0; i < sectorCount; ++i) {
        if (isOperationCancelled()) {
            outFile.close();
            lastError_ = "Operation cancelled by user";
            return false;
        }
        
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
        
        if (progressCallback && (i + 1) % 100 == 0) {
            progressCallback(i + 1, sectorCount);
        }
    }
    
    outFile.close();
    return true;
}

bool EnhancedDiskSectorCRC::verifySectorIntegrity(const std::string& checksumFile,
                                                 std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
    std::vector<SectorChecksum> checksums;
    uint64_t startSector, sectorCount;
    
    if (!readChecksumFile(checksumFile, checksums, startSector, sectorCount)) {
        return false;
    }
    
    bool allValid = true;
    uint64_t corruptedSectors = 0;
    
    // Verify each sector with cancellation support
    for (uint64_t i = 0; i < checksums.size(); ++i) {
        if (isOperationCancelled()) {
            lastError_ = "Operation cancelled by user";
            return false;
        }
        
        const auto& storedChecksum = checksums[i];
        std::vector<uint8_t> currentSectorData;
        
        if (!readSector(storedChecksum.sectorNumber, currentSectorData)) {
            lastError_ = "Failed to read sector " + std::to_string(storedChecksum.sectorNumber) + ": " + lastError_;
            return false;
        }
        
        uint32_t currentCRC = calculateCRC32(currentSectorData);
        
        if (currentCRC != storedChecksum.crc32) {
            allValid = false;
            corruptedSectors++;
        }
        
        if (progressCallback && (i + 1) % 100 == 0) {
            progressCallback(i + 1, checksums.size());
        }
    }
    
    return allValid;
}

bool EnhancedDiskSectorCRC::repairSectorData(const std::string& checksumFile, 
                                            const std::string& backupDiskPath,
                                            std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
    std::vector<SectorChecksum> checksums;
    uint64_t startSector, sectorCount;
    
    if (!readChecksumFile(checksumFile, checksums, startSector, sectorCount)) {
        return false;
    }
    
    bool backupAvailable = !backupDiskPath.empty();
    std::string backupDisk = backupDiskPath;
    
    if (backupAvailable && backupDisk.find("\\\\.\\") == std::string::npos) {
        backupDisk = "\\\\.\\" + backupDiskPath;
    }
    
    uint64_t repairedSectors = 0;
    uint64_t totalCorrupted = 0;
    
    // Check each sector and attempt repair with cancellation support
    for (uint64_t i = 0; i < checksums.size(); ++i) {
        if (isOperationCancelled()) {
            lastError_ = "Operation cancelled by user";
            return false;
        }
        
        const auto& storedChecksum = checksums[i];
        std::vector<uint8_t> currentSectorData;
        
        if (!readSector(storedChecksum.sectorNumber, currentSectorData)) {
            lastError_ = "Failed to read sector " + std::to_string(storedChecksum.sectorNumber) + ": " + lastError_;
            return false;
        }
        
        uint32_t currentCRC = calculateCRC32(currentSectorData);
        
        if (currentCRC != storedChecksum.crc32) {
            totalCorrupted++;
            
            // Attempt recovery from backup disk
            if (backupAvailable) {
                EnhancedDiskSectorCRC backupDiskObj(backupDisk);
                std::vector<uint8_t> backupData;
                
                if (backupDiskObj.readSector(storedChecksum.sectorNumber, backupData)) {
                    uint32_t backupCRC = backupDiskObj.calculateCRC32(backupData);
                    
                    if (backupCRC == storedChecksum.crc32) {
                        // Write data from backup
                        if (writeSector(storedChecksum.sectorNumber, backupData)) {
                            repairedSectors++;
                        }
                    }
                }
            }
        }
        
        if (progressCallback && (i + 1) % 100 == 0) {
            progressCallback(i + 1, checksums.size());
        }
    }
    
    return repairedSectors > 0 || totalCorrupted == 0;
}

// Parallel processing methods with maximum performance optimization
bool EnhancedDiskSectorCRC::generateChecksumsParallel(uint64_t startSector, uint64_t sectorCount,
                                                     const std::string& outputFile, int threadCount,
                                                     std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
    // Auto-detect optimal thread count: use half of available threads
    if (threadCount <= 0) {
        unsigned int availableThreads = std::thread::hardware_concurrency();
        threadCount = (availableThreads > 1) ? (availableThreads / 2) : 1;
    }
    
    std::cout << "Using " << threadCount << " threads for parallel processing" << std::endl;
    
    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile.is_open()) {
        lastError_ = "Cannot create output file: " + outputFile;
        return false;
    }
    
    // Write file header
    uint32_t magic = 0x43524344;
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    outFile.write(reinterpret_cast<const char*>(&startSector), sizeof(startSector));
    outFile.write(reinterpret_cast<const char*>(&sectorCount), sizeof(sectorCount));
    outFile.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    outFile.close();
    
    std::vector<std::thread> threads;
    std::mutex fileMutex;
    std::atomic<uint64_t> processedCount(0);
    
    // Calculate sectors per thread
    uint64_t sectorsPerThread = sectorCount / threadCount;
    uint64_t remainingSectors = sectorCount % threadCount;
    
    uint64_t currentStart = startSector;
    
    // Always use optimized batch reading for maximum performance
    const int BATCH_SIZE = 256; // Larger batch size for better I/O performance
    
    for (int i = 0; i < threadCount; ++i) {
        uint64_t threadSectorCount = sectorsPerThread;
        if (i < remainingSectors) {
            threadSectorCount++;
        }
        
        uint64_t threadEnd = currentStart + threadSectorCount;
        
        threads.emplace_back(&EnhancedDiskSectorCRC::checksumWorkerStreaming, this,
                           currentStart, threadEnd, outputFile, std::ref(fileMutex),
                           std::ref(processedCount), sectorCount, progressCallback, BATCH_SIZE);
        
        currentStart = threadEnd;
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    return !isOperationCancelled();
}

bool EnhancedDiskSectorCRC::verifyIntegrityParallel(const std::string& checksumFile, int threadCount,
                                                   std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
    std::vector<SectorChecksum> checksums;
    uint64_t startSector, sectorCount;
    
    if (!readChecksumFile(checksumFile, checksums, startSector, sectorCount)) {
        return false;
    }
    
    if (threadCount <= 0) threadCount = std::thread::hardware_concurrency();
    if (threadCount <= 0) threadCount = 4;
    
    std::vector<std::thread> threads;
    std::atomic<uint64_t> corruptedCount(0);
    std::atomic<uint64_t> processedCount(0);
    
    // Split checksums among threads
    size_t checksumsPerThread = checksums.size() / threadCount;
    size_t remainingChecksums = checksums.size() % threadCount;
    
    size_t currentIndex = 0;
    
    for (int i = 0; i < threadCount; ++i) {
        size_t threadChecksumCount = checksumsPerThread;
        if (i < remainingChecksums) {
            threadChecksumCount++;
        }
        
        std::vector<SectorChecksum> threadChecksums(
            checksums.begin() + currentIndex,
            checksums.begin() + currentIndex + threadChecksumCount
        );
        
        threads.emplace_back(&EnhancedDiskSectorCRC::verificationWorker, this,
                           threadChecksums, std::ref(corruptedCount),
                           std::ref(processedCount), progressCallback);
        
        currentIndex += threadChecksumCount;
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    return corruptedCount == 0 && !isOperationCancelled();
}

bool EnhancedDiskSectorCRC::repairDataParallel(const std::string& checksumFile, 
                                              const std::string& backupDiskPath, int threadCount,
                                              std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
    std::vector<SectorChecksum> checksums;
    uint64_t startSector, sectorCount;
    
    if (!readChecksumFile(checksumFile, checksums, startSector, sectorCount)) {
        return false;
    }
    
    if (threadCount <= 0) threadCount = std::thread::hardware_concurrency();
    if (threadCount <= 0) threadCount = 4;
    
    std::vector<std::thread> threads;
    std::atomic<uint64_t> repairedCount(0);
    std::atomic<uint64_t> processedCount(0);
    
    // Split checksums among threads
    size_t checksumsPerThread = checksums.size() / threadCount;
    size_t remainingChecksums = checksums.size() % threadCount;
    
    size_t currentIndex = 0;
    
    for (int i = 0; i < threadCount; ++i) {
        size_t threadChecksumCount = checksumsPerThread;
        if (i < remainingChecksums) {
            threadChecksumCount++;
        }
        
        std::vector<SectorChecksum> threadChecksums(
            checksums.begin() + currentIndex,
            checksums.begin() + currentIndex + threadChecksumCount
        );
        
        threads.emplace_back(&EnhancedDiskSectorCRC::repairWorker, this,
                           threadChecksums, backupDiskPath, std::ref(repairedCount),
                           std::ref(processedCount), progressCallback);
        
        currentIndex += threadChecksumCount;
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    return repairedCount > 0 && !isOperationCancelled();
}

// Control methods
void EnhancedDiskSectorCRC::cancelOperation() {
    operationCancelled_ = true;
    cancellationCV_.notify_all();
}

bool EnhancedDiskSectorCRC::isOperationCancelled() const {
    return operationCancelled_;
}

void EnhancedDiskSectorCRC::resetCancellation() {
    operationCancelled_ = false;
}

// Advanced repair methods
bool EnhancedDiskSectorCRC::repairFromChecksumFile(const std::string& checksumFile, 
                                                  const std::string& repairSourcePath,
                                                  std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
    std::vector<SectorChecksum> checksums;
    uint64_t startSector, sectorCount;
    
    if (!readChecksumFile(checksumFile, checksums, startSector, sectorCount)) {
        return false;
    }
    
    std::string repairSource = repairSourcePath;
    if (repairSource.empty()) {
        if (!findRepairSource(checksumFile, repairSource)) {
            lastError_ = "Cannot find suitable repair source";
            return false;
        }
    }
    
    EnhancedDiskSectorCRC repairSourceObj(repairSource);
    uint64_t repairedSectors = 0;
    uint64_t totalCorrupted = 0;
    
    // Repair corrupted sectors using checksum file as reference
    for (uint64_t i = 0; i < checksums.size(); ++i) {
        if (isOperationCancelled()) {
            lastError_ = "Operation cancelled by user";
            return false;
        }
        
        const auto& storedChecksum = checksums[i];
        std::vector<uint8_t> currentSectorData;
        
        if (!readSector(storedChecksum.sectorNumber, currentSectorData)) {
            lastError_ = "Failed to read sector " + std::to_string(storedChecksum.sectorNumber) + ": " + lastError_;
            return false;
        }
        
        uint32_t currentCRC = calculateCRC32(currentSectorData);
        
        if (currentCRC != storedChecksum.crc32) {
            totalCorrupted++;
            
            // Read correct data from repair source
            std::vector<uint8_t> repairData;
            if (repairSourceObj.readSector(storedChecksum.sectorNumber, repairData)) {
                uint32_t repairCRC = calculateCRC32(repairData);
                
                if (repairCRC == storedChecksum.crc32) {
                    // Write correct data to target disk
                    if (writeSector(storedChecksum.sectorNumber, repairData)) {
                        repairedSectors++;
                    }
                }
            }
        }
        
        if (progressCallback && (i + 1) % 100 == 0) {
            progressCallback(i + 1, checksums.size());
        }
    }
    
    return repairedSectors > 0;
}

bool EnhancedDiskSectorCRC::validateChecksumFile(const std::string& checksumFile) {
    std::vector<SectorChecksum> checksums;
    uint64_t startSector, sectorCount;
    
    return readChecksumFile(checksumFile, checksums, startSector, sectorCount);
}

// Worker thread functions
void EnhancedDiskSectorCRC::checksumWorker(uint64_t startSector, uint64_t endSector, 
                                          const std::string& outputFile, std::mutex& fileMutex,
                                          std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                                          std::function<void(int, int)> progressCallback) {
    std::ofstream outFile(outputFile, std::ios::binary | std::ios::app);
    if (!outFile.is_open()) {
        return;
    }
    
    for (uint64_t sector = startSector; sector < endSector; ++sector) {
        if (isOperationCancelled()) {
            break;
        }
        
        std::vector<uint8_t> sectorData;
        if (!readSector(sector, sectorData)) {
            continue;
        }
        
        uint32_t crc = calculateCRC32(sectorData);
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        SectorChecksum checksum{sector, crc, timestamp};
        
        {
            std::lock_guard<std::mutex> lock(fileMutex);
            outFile.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
        }
        
        uint64_t processed = ++processedCount;
        if (progressCallback && processed % 100 == 0) {
            progressCallback(processed, totalCount);
        }
    }
    
    outFile.close();
}

// High-performance parallel processing with dedicated reader thread
bool EnhancedDiskSectorCRC::generateChecksumsHighPerformance(uint64_t startSector, uint64_t sectorCount,
                                                            const std::string& outputFile, int readerThreads,
                                                            int processorThreads,
                                                            std::function<void(int, int)> progressCallback) {
    resetCancellation();
    
    // Auto-detect optimal thread counts
    unsigned int availableThreads = std::thread::hardware_concurrency();
    if (readerThreads <= 0) readerThreads = 1; // At least one reader thread
    if (processorThreads <= 0) processorThreads = (availableThreads > 2) ? (availableThreads - 1) : 1;
    
    std::cout << "High-performance mode: " << readerThreads << " reader thread(s), " 
              << processorThreads << " processor thread(s)" << std::endl;
    
    // Create output file and write header
    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile.is_open()) {
        lastError_ = "Cannot create output file: " + outputFile;
        return false;
    }
    
    uint32_t magic = 0x43524344;
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    outFile.write(reinterpret_cast<const char*>(&startSector), sizeof(startSector));
    outFile.write(reinterpret_cast<const char*>(&sectorCount), sizeof(sectorCount));
    outFile.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    outFile.close();
    
    // Producer-consumer setup
    std::queue<SectorData> dataQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::atomic<bool> readingComplete(false);
    std::atomic<uint64_t> processedCount(0);
    
    std::vector<std::thread> readerThreadsList;
    std::vector<std::thread> processorThreadsList;
    std::mutex fileMutex;
    
    // Calculate sectors per reader thread
    uint64_t sectorsPerReader = sectorCount / readerThreads;
    uint64_t remainingSectors = sectorCount % readerThreads;
    
    uint64_t currentStart = startSector;
    
    // Start reader threads (producers)
    for (int i = 0; i < readerThreads; ++i) {
        uint64_t threadSectorCount = sectorsPerReader;
        if (i < remainingSectors) {
            threadSectorCount++;
        }
        
        uint64_t threadEnd = currentStart + threadSectorCount;
        
        readerThreadsList.emplace_back(&EnhancedDiskSectorCRC::readerWorker, this,
                                     currentStart, threadEnd, std::ref(dataQueue),
                                     std::ref(queueMutex), std::ref(queueCV),
                                     std::ref(readingComplete), 128); // Large batch size
        
        currentStart = threadEnd;
    }
    
    // Start processor threads (consumers)
    for (int i = 0; i < processorThreads; ++i) {
        processorThreadsList.emplace_back(&EnhancedDiskSectorCRC::processorWorker, this,
                                        std::ref(dataQueue), std::ref(queueMutex),
                                        std::ref(queueCV), std::ref(readingComplete),
                                        outputFile, std::ref(fileMutex),
                                        std::ref(processedCount), sectorCount, progressCallback);
    }
    
    // Wait for all reader threads to complete
    for (auto& thread : readerThreadsList) {
        thread.join();
    }
    
    // Signal that reading is complete
    readingComplete = true;
    queueCV.notify_all();
    
    // Wait for all processor threads to complete
    for (auto& thread : processorThreadsList) {
        thread.join();
    }
    
    return !isOperationCancelled();
}

bool EnhancedDiskSectorCRC::verifyIntegrityHighPerformance(const std::string& checksumFile, int readerThreads,
                                                          int processorThreads,
                                                          std::function<void(int, int)> progressCallback) {
    // Implementation similar to generateChecksumsHighPerformance but for verification
    // This would use a similar producer-consumer pattern for verification
    // For now, fall back to parallel verification
    return verifyIntegrityParallel(checksumFile, readerThreads + processorThreads, progressCallback);
}

// Reader worker: dedicated to reading sectors from disk
void EnhancedDiskSectorCRC::readerWorker(uint64_t startSector, uint64_t endSector,
                                        std::queue<SectorData>& dataQueue, std::mutex& queueMutex,
                                        std::condition_variable& queueCV, std::atomic<bool>& readingComplete,
                                        int batchSize) {
    const int MAX_QUEUE_SIZE = batchSize * 4; // Prevent queue from growing too large
    
    uint64_t currentSector = startSector;
    
    while (currentSector < endSector && !isOperationCancelled()) {
        // Read a batch of sectors
        int actualBatchSize = std::min(static_cast<uint64_t>(batchSize), endSector - currentSector);
        std::vector<std::vector<uint8_t>> batchData(actualBatchSize);
        std::vector<uint64_t> batchSectors(actualBatchSize);
        
        // Read sectors in batch for maximum throughput
        for (int i = 0; i < actualBatchSize; ++i) {
            batchSectors[i] = currentSector;
            if (!readSector(currentSector, batchData[i])) {
                // Skip failed sectors
                batchData[i].clear();
            }
            currentSector++;
        }
        
        // Prepare sector data for the queue
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        std::vector<SectorData> sectorBatch;
        for (int i = 0; i < actualBatchSize; ++i) {
            if (!batchData[i].empty()) {
                SectorData data;
                data.sectorNumber = batchSectors[i];
                data.data = std::move(batchData[i]);
                data.timestamp = timestamp;
                sectorBatch.push_back(std::move(data));
            }
        }
        
        // Add to queue with proper synchronization
        if (!sectorBatch.empty()) {
            std::unique_lock<std::mutex> lock(queueMutex);
            
            // Wait if queue is too large to prevent memory exhaustion
            while (dataQueue.size() >= MAX_QUEUE_SIZE && !isOperationCancelled()) {
                queueCV.wait_for(lock, std::chrono::milliseconds(10));
            }
            
            if (isOperationCancelled()) {
                break;
            }
            
            // Add all sectors from the batch to the queue
            for (auto& data : sectorBatch) {
                dataQueue.push(std::move(data));
            }
            
            lock.unlock();
            queueCV.notify_all(); // Notify processor threads
        }
    }
}

// Processor worker: dedicated to calculating CRC and writing results
void EnhancedDiskSectorCRC::processorWorker(std::queue<SectorData>& dataQueue, std::mutex& queueMutex,
                                           std::condition_variable& queueCV, std::atomic<bool>& readingComplete,
                                           const std::string& outputFile, std::mutex& fileMutex,
                                           std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                                           std::function<void(int, int)> progressCallback) {
    std::ofstream outFile(outputFile, std::ios::binary | std::ios::app);
    if (!outFile.is_open()) {
        return;
    }
    
    while (!isOperationCancelled()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // Wait for data or completion signal
        while (dataQueue.empty() && !(readingComplete && isOperationCancelled())) {
            queueCV.wait_for(lock, std::chrono::milliseconds(100));
        }
        
        if (dataQueue.empty() && readingComplete) {
            break; // No more data and reading is complete
        }
        
        if (dataQueue.empty()) {
            continue; // No data available yet
        }
        
        // Process one sector at a time for better load balancing
        SectorData data = std::move(dataQueue.front());
        dataQueue.pop();
        lock.unlock();
        
        // Calculate CRC
        data.crc = calculateCRC32(data.data);
        
        // Write result to file
        SectorChecksum checksum{data.sectorNumber, data.crc, data.timestamp};
        {
            std::lock_guard<std::mutex> fileLock(fileMutex);
            outFile.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
        }
        
        // Update progress
        uint64_t processed = ++processedCount;
        if (progressCallback && processed % 100 == 0) {
            progressCallback(processed, totalCount);
        }
    }
    
    outFile.close();
}

// Optimized batch reading for maximum throughput
bool EnhancedDiskSectorCRC::readSectorsBatch(uint64_t startSector, uint64_t count, std::vector<std::vector<uint8_t>>& batchData) {
    // This method would implement optimized batch reading
    // For now, use individual sector reads
    batchData.resize(count);
    bool allSuccess = true;
    
    for (uint64_t i = 0; i < count; ++i) {
        if (!readSector(startSector + i, batchData[i])) {
            allSuccess = false;
            batchData[i].clear();
        }
    }
    
    return allSuccess;
}

// Streaming worker with automatic memory release: process sectors as they are read
void EnhancedDiskSectorCRC::checksumWorkerStreaming(uint64_t startSector, uint64_t endSector, 
                                                   const std::string& outputFile, std::mutex& fileMutex,
                                                   std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                                                   std::function<void(int, int)> progressCallback,
                                                   int bufferSize) {
    std::ofstream outFile(outputFile, std::ios::binary | std::ios::app);
    if (!outFile.is_open()) {
        return;
    }
    
    // Use a small buffer for streaming processing to minimize memory usage
    const int STREAM_BUFFER_SIZE = std::min(bufferSize, 32); // Smaller buffer for streaming
    
    // Pre-allocate minimal buffers for streaming
    std::vector<uint8_t> sectorData(DiskSectorCRC::SECTOR_SIZE);
    std::vector<SectorChecksum> checksumBuffer(STREAM_BUFFER_SIZE);
    std::vector<uint64_t> sectorBuffer(STREAM_BUFFER_SIZE);
    
    uint64_t currentSector = startSector;
    int bufferIndex = 0;
    
    while (currentSector < endSector) {
        if (isOperationCancelled()) {
            break;
        }
        
        // Read one sector at a time for streaming processing
        if (readSector(currentSector, sectorData)) {
            // Calculate checksum immediately after reading
            uint32_t crc = calculateCRC32(sectorData);
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            // Store in buffer
            checksumBuffer[bufferIndex] = SectorChecksum{currentSector, crc, timestamp};
            sectorBuffer[bufferIndex] = currentSector;
            bufferIndex++;
            
            // When buffer is full or at end, write to file and clear buffer
            if (bufferIndex >= STREAM_BUFFER_SIZE || currentSector + 1 >= endSector) {
                {
                    std::lock_guard<std::mutex> lock(fileMutex);
                    for (int i = 0; i < bufferIndex; ++i) {
                        outFile.write(reinterpret_cast<const char*>(&checksumBuffer[i]), sizeof(SectorChecksum));
                    }
                }
                
                // Update progress
                uint64_t processed = processedCount.fetch_add(bufferIndex) + bufferIndex;
                if (progressCallback && processed % 100 == 0) {
                    progressCallback(processed, totalCount);
                }
                
                // Clear buffer for next batch
                bufferIndex = 0;
            }
        }
        
        currentSector++;
        
        // Explicitly clear sector data to release memory immediately
        if (currentSector % 16 == 0) {
            std::vector<uint8_t>().swap(sectorData); // Force memory release
            sectorData.resize(DiskSectorCRC::SECTOR_SIZE); // Reallocate for next sector
        }
    }
    
    // Write any remaining data in buffer
    if (bufferIndex > 0) {
        {
            std::lock_guard<std::mutex> lock(fileMutex);
            for (int i = 0; i < bufferIndex; ++i) {
                outFile.write(reinterpret_cast<const char*>(&checksumBuffer[i]), sizeof(SectorChecksum));
            }
        }
        
        uint64_t processed = processedCount.fetch_add(bufferIndex) + bufferIndex;
        if (progressCallback && processed % 100 == 0) {
            progressCallback(processed, totalCount);
        }
    }
    
    outFile.close();
    
    // Final memory cleanup
    std::vector<uint8_t>().swap(sectorData);
    std::vector<SectorChecksum>().swap(checksumBuffer);
    std::vector<uint64_t>().swap(sectorBuffer);
}


void EnhancedDiskSectorCRC::verificationWorker(const std::vector<SectorChecksum>& checksums,
                                              std::atomic<uint64_t>& corruptedCount,
                                              std::atomic<uint64_t>& processedCount,
                                              std::function<void(int, int)> progressCallback) {
    for (const auto& checksum : checksums) {
        if (isOperationCancelled()) {
            break;
        }
        
        std::vector<uint8_t> sectorData;
        if (!readSector(checksum.sectorNumber, sectorData)) {
            continue;
        }
        
        uint32_t currentCRC = calculateCRC32(sectorData);
        
        if (currentCRC != checksum.crc32) {
            corruptedCount++;
        }
        
        uint64_t processed = ++processedCount;
        if (progressCallback && processed % 100 == 0) {
            progressCallback(processed, checksums.size());
        }
    }
}

void EnhancedDiskSectorCRC::repairWorker(const std::vector<SectorChecksum>& checksums,
                                        const std::string& backupDiskPath,
                                        std::atomic<uint64_t>& repairedCount,
                                        std::atomic<uint64_t>& processedCount,
                                        std::function<void(int, int)> progressCallback) {
    bool backupAvailable = !backupDiskPath.empty();
    std::string backupDisk = backupDiskPath;
    
    if (backupAvailable && backupDisk.find("\\\\.\\") == std::string::npos) {
        backupDisk = "\\\\.\\" + backupDiskPath;
    }
    
    for (const auto& checksum : checksums) {
        if (isOperationCancelled()) {
            break;
        }
        
        std::vector<uint8_t> currentSectorData;
        if (!readSector(checksum.sectorNumber, currentSectorData)) {
            continue;
        }
        
        uint32_t currentCRC = calculateCRC32(currentSectorData);
        
        if (currentCRC != checksum.crc32) {
            // Attempt recovery from backup disk
            if (backupAvailable) {
                EnhancedDiskSectorCRC backupDiskObj(backupDisk);
                std::vector<uint8_t> backupData;
                
                if (backupDiskObj.readSector(checksum.sectorNumber, backupData)) {
                    uint32_t backupCRC = backupDiskObj.calculateCRC32(backupData);
                    
                    if (backupCRC == checksum.crc32) {
                        // Write data from backup
                        if (writeSector(checksum.sectorNumber, backupData)) {
                            repairedCount++;
                        }
                    }
                }
            }
        }
        
        uint64_t processed = ++processedCount;
        if (progressCallback && processed % 100 == 0) {
            progressCallback(processed, checksums.size());
        }
    }
}

// Helper methods
bool EnhancedDiskSectorCRC::readChecksumFile(const std::string& checksumFile, 
                                            std::vector<SectorChecksum>& checksums,
                                            uint64_t& startSector, uint64_t& sectorCount) {
    std::ifstream inFile(checksumFile, std::ios::binary);
    if (!inFile.is_open()) {
        lastError_ = "Cannot open checksum file: " + checksumFile;
        return false;
    }
    
    // Read file header
    uint32_t magic;
    uint64_t timestamp;
    
    inFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    inFile.read(reinterpret_cast<char*>(&startSector), sizeof(startSector));
    inFile.read(reinterpret_cast<char*>(&sectorCount), sizeof(sectorCount));
    inFile.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    
    if (magic != 0x43524344) {
        lastError_ = "Invalid checksum file format";
        inFile.close();
        return false;
    }
    
    // Read all checksums
    checksums.resize(sectorCount);
    for (uint64_t i = 0; i < sectorCount; ++i) {
        inFile.read(reinterpret_cast<char*>(&checksums[i]), sizeof(SectorChecksum));
        if (inFile.gcount() != sizeof(SectorChecksum)) {
            lastError_ = "Failed to read checksum data";
            inFile.close();
            return false;
        }
    }
    
    inFile.close();
    return true;
}

bool EnhancedDiskSectorCRC::findRepairSource(const std::string& checksumFile, std::string& repairSource) {
    // Try to find a suitable repair source
    // This could be implemented to search for backup disks, network locations, etc.
    // For now, return false to indicate no automatic source found
    return false;
}

// Optimized worker with batch reading for better performance
void EnhancedDiskSectorCRC::checksumWorkerBatch(uint64_t startSector, uint64_t endSector, 
                                               const std::string& outputFile, std::mutex& fileMutex,
                                               std::atomic<uint64_t>& processedCount, uint64_t totalCount,
                                               std::function<void(int, int)> progressCallback,
                                               int batchSize) {
    std::ofstream outFile(outputFile, std::ios::binary | std::ios::app);
    if (!outFile.is_open()) {
        return;
    }
    
    // Pre-allocate buffers for batch processing
    std::vector<std::vector<uint8_t>> batchData(batchSize);
    std::vector<uint64_t> batchSectors(batchSize);
    std::vector<SectorChecksum> batchChecksums(batchSize);
    
    uint64_t currentSector = startSector;
    
    while (currentSector < endSector) {
        if (isOperationCancelled()) {
            break;
        }
        
        // Process sectors in batches for better I/O performance
        int actualBatchSize = 0;
        for (int i = 0; i < batchSize && currentSector < endSector; ++i) {
            batchSectors[i] = currentSector;
            if (readSector(currentSector, batchData[i])) {
                actualBatchSize++;
            }
            currentSector++;
        }
        
        // Calculate checksums for the batch
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        for (int i = 0; i < actualBatchSize; ++i) {
            uint32_t crc = calculateCRC32(batchData[i]);
            batchChecksums[i] = SectorChecksum{batchSectors[i], crc, timestamp};
        }
        
        // Write batch results to file
        {
            std::lock_guard<std::mutex> lock(fileMutex);
            for (int i = 0; i < actualBatchSize; ++i) {
                outFile.write(reinterpret_cast<const char*>(&batchChecksums[i]), sizeof(SectorChecksum));
            }
        }
        
        // Update progress
        uint64_t processed = processedCount.fetch_add(actualBatchSize) + actualBatchSize;
        if (progressCallback && processed % 100 == 0) {
            progressCallback(processed, totalCount);
        }
    }
    
    outFile.close();
}
