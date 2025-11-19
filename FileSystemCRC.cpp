#include "FileSystemCRC.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

FileSystemCRC::FileSystemCRC() : operationCancelled_(false) {
}

FileSystemCRC::~FileSystemCRC() {
    cancelOperation();
}

// 文件校验方法
bool FileSystemCRC::generateFileChecksum(const std::string& filePath, FileChecksum& checksum) {
    resetCancellation();
    
    fs::path path(filePath);
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        setLastError("File does not exist or is not a regular file: " + filePath);
        return false;
    }
    
    try {
        checksum.filePath = filePath;
        checksum.fileSize = fs::file_size(path);
        checksum.lastModified = fs::last_write_time(path).time_since_epoch().count();
        checksum.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        checksum.crc32 = calculateCRC32ForFile(path);
        return true;
    } catch (const fs::filesystem_error& e) {
        setLastError("Filesystem error: " + std::string(e.what()));
        return false;
    }
}

bool FileSystemCRC::verifyFileIntegrity(const FileChecksum& checksum) {
    resetCancellation();
    
    fs::path path(checksum.filePath);
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        setLastError("File does not exist: " + checksum.filePath);
        return false;
    }
    
    try {
        // 检查文件大小
        if (fs::file_size(path) != checksum.fileSize) {
            setLastError("File size mismatch for: " + checksum.filePath);
            return false;
        }
        
        // 计算当前CRC32
        uint32_t currentCRC = calculateCRC32ForFile(path);
        if (currentCRC != checksum.crc32) {
            setLastError("CRC32 mismatch for: " + checksum.filePath);
            return false;
        }
        
        return true;
    } catch (const fs::filesystem_error& e) {
        setLastError("Filesystem error: " + std::string(e.what()));
        return false;
    }
}

bool FileSystemCRC::repairFileFromBackup(const FileChecksum& checksum, const std::string& backupPath) {
    resetCancellation();
    
    fs::path sourcePath(backupPath);
    fs::path targetPath(checksum.filePath);
    
    if (!fs::exists(sourcePath) || !fs::is_regular_file(sourcePath)) {
        setLastError("Backup file does not exist: " + backupPath);
        return false;
    }
    
    try {
        // 验证备份文件
        FileChecksum backupChecksum;
        if (!generateFileChecksum(backupPath, backupChecksum)) {
            return false;
        }
        
        if (backupChecksum.crc32 != checksum.crc32) {
            setLastError("Backup file CRC32 does not match expected value");
            return false;
        }
        
        // 创建目标目录（如果需要）
        fs::create_directories(targetPath.parent_path());
        
        // 复制文件
        fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing);
        
        // 验证修复结果
        return verifyFileIntegrity(checksum);
    } catch (const fs::filesystem_error& e) {
        setLastError("Filesystem error during repair: " + std::string(e.what()));
        return false;
    }
}

// 文件夹校验方法
bool FileSystemCRC::generateDirectoryChecksums(const std::string& directoryPath, 
                                              DirectoryChecksum& checksum,
                                              std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    fs::path path(directoryPath);
    if (!fs::exists(path) || !fs::is_directory(path)) {
        setLastError("Directory does not exist: " + directoryPath);
        return false;
    }
    
    try {
        checksum.directoryPath = directoryPath;
        checksum.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        checksum.totalSize = 0;
        
        // 扫描目录中的所有文件
        std::vector<fs::path> files;
        scanDirectoryRecursive(path, files);
        
        // 为每个文件生成校验和
        uint64_t processed = 0;
        for (const auto& file : files) {
            if (isOperationCancelled()) {
                setLastError("Operation cancelled by user");
                return false;
            }
            
            FileChecksum fileChecksum;
            if (generateFileChecksum(file.string(), fileChecksum)) {
                checksum.fileChecksums.push_back(fileChecksum);
                checksum.totalSize += fileChecksum.fileSize;
            }
            
            processed++;
            if (progressCallback) {
                progressCallback(processed, files.size(), file.string());
            }
        }
        
        // 计算目录级CRC32（基于所有文件CRC32的组合）
        uint32_t directoryCRC = 0;
        for (const auto& fileChecksum : checksum.fileChecksums) {
            // 简单的组合算法：异或文件路径CRC和文件CRC
            std::string relativePath = fs::relative(fileChecksum.filePath, directoryPath).string();
            uint32_t pathCRC = calculateCRC32ForData(
                std::vector<uint8_t>(relativePath.begin(), relativePath.end()));
            directoryCRC ^= (pathCRC ^ fileChecksum.crc32);
        }
        checksum.directoryCRC = directoryCRC;
        
        return true;
    } catch (const fs::filesystem_error& e) {
        setLastError("Filesystem error: " + std::string(e.what()));
        return false;
    }
}

bool FileSystemCRC::verifyDirectoryIntegrity(const DirectoryChecksum& checksum,
                                            std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    bool allValid = true;
    uint64_t processed = 0;
    
    for (const auto& fileChecksum : checksum.fileChecksums) {
        if (isOperationCancelled()) {
            setLastError("Operation cancelled by user");
            return false;
        }
        
        if (!verifyFileIntegrity(fileChecksum)) {
            allValid = false;
        }
        
        processed++;
        if (progressCallback) {
            progressCallback(processed, checksum.fileChecksums.size(), fileChecksum.filePath);
        }
    }
    
    return allValid;
}

bool FileSystemCRC::repairDirectoryFromBackup(const DirectoryChecksum& checksum, const std::string& backupPath,
                                             std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    fs::path backupDir(backupPath);
    if (!fs::exists(backupDir) || !fs::is_directory(backupDir)) {
        setLastError("Backup directory does not exist: " + backupPath);
        return false;
    }
    
    bool anyRepaired = false;
    uint64_t processed = 0;
    
    for (const auto& fileChecksum : checksum.fileChecksums) {
        if (isOperationCancelled()) {
            setLastError("Operation cancelled by user");
            return false;
        }
        
        // 检查文件是否需要修复
        if (!verifyFileIntegrity(fileChecksum)) {
            // 构建备份文件路径
            fs::path relativePath = fs::relative(fileChecksum.filePath, checksum.directoryPath);
            fs::path backupFilePath = backupDir / relativePath;
            
            // 尝试从备份修复
            if (repairFileFromBackup(fileChecksum, backupFilePath.string())) {
                anyRepaired = true;
            }
        }
        
        processed++;
        if (progressCallback) {
            progressCallback(processed, checksum.fileChecksums.size(), fileChecksum.filePath);
        }
    }
    
    return anyRepaired;
}

// 分区校验方法
bool FileSystemCRC::generatePartitionChecksums(const std::string& partitionPath,
                                              PartitionChecksum& checksum,
                                              std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    fs::path path(partitionPath);
    if (!fs::exists(path) || !fs::is_directory(path)) {
        setLastError("Partition path does not exist: " + partitionPath);
        return false;
    }
    
    try {
        checksum.partitionPath = partitionPath;
        checksum.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        checksum.totalSize = 0;
        
        // 扫描分区中的所有目录
        uint64_t dirCount = 0;
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (isOperationCancelled()) {
                setLastError("Operation cancelled by user");
                return false;
            }
            
            if (entry.is_directory()) {
                dirCount++;
            }
        }
        
        uint64_t processedDirs = 0;
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (isOperationCancelled()) {
                setLastError("Operation cancelled by user");
                return false;
            }
            
            if (entry.is_directory()) {
                DirectoryChecksum dirChecksum;
                if (generateDirectoryChecksums(entry.path().string(), dirChecksum)) {
                    checksum.directoryChecksums.push_back(dirChecksum);
                    checksum.totalSize += dirChecksum.totalSize;
                }
                
                processedDirs++;
                if (progressCallback) {
                    progressCallback(processedDirs, dirCount, entry.path().string());
                }
            }
        }
        
        // 计算分区级CRC32
        uint32_t partitionCRC = 0;
        for (const auto& dirChecksum : checksum.directoryChecksums) {
            partitionCRC ^= dirChecksum.directoryCRC;
        }
        checksum.partitionCRC = partitionCRC;
        
        return true;
    } catch (const fs::filesystem_error& e) {
        setLastError("Filesystem error: " + std::string(e.what()));
        return false;
    }
}

bool FileSystemCRC::verifyPartitionIntegrity(const PartitionChecksum& checksum,
                                            std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    bool allValid = true;
    uint64_t processed = 0;
    
    for (const auto& dirChecksum : checksum.directoryChecksums) {
        if (isOperationCancelled()) {
            setLastError("Operation cancelled by user");
            return false;
        }
        
        if (!verifyDirectoryIntegrity(dirChecksum)) {
            allValid = false;
        }
        
        processed++;
        if (progressCallback) {
            progressCallback(processed, checksum.directoryChecksums.size(), dirChecksum.directoryPath);
        }
    }
    
    return allValid;
}

bool FileSystemCRC::repairPartitionFromBackup(const PartitionChecksum& checksum, const std::string& backupPath,
                                             std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    fs::path backupPartition(backupPath);
    if (!fs::exists(backupPartition) || !fs::is_directory(backupPartition)) {
        setLastError("Backup partition does not exist: " + backupPath);
        return false;
    }
    
    bool anyRepaired = false;
    uint64_t processed = 0;
    
    for (const auto& dirChecksum : checksum.directoryChecksums) {
        if (isOperationCancelled()) {
            setLastError("Operation cancelled by user");
            return false;
        }
        
        // 构建备份目录路径
        fs::path relativePath = fs::relative(dirChecksum.directoryPath, checksum.partitionPath);
        fs::path backupDirPath = backupPartition / relativePath;
        
        // 尝试从备份修复目录
        if (repairDirectoryFromBackup(dirChecksum, backupDirPath.string())) {
            anyRepaired = true;
        }
        
        processed++;
        if (progressCallback) {
            progressCallback(processed, checksum.directoryChecksums.size(), dirChecksum.directoryPath);
        }
    }
    
    return anyRepaired;
}

// 并行处理方法
bool FileSystemCRC::generateDirectoryChecksumsParallel(const std::string& directoryPath,
                                                      DirectoryChecksum& checksum, int threadCount,
                                                      std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    fs::path path(directoryPath);
    if (!fs::exists(path) || !fs::is_directory(path)) {
        setLastError("Directory does not exist: " + directoryPath);
        return false;
    }
    
    if (threadCount <= 0) threadCount = std::thread::hardware_concurrency();
    if (threadCount <= 0) threadCount = 4;
    
    try {
        checksum.directoryPath = directoryPath;
        checksum.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        checksum.totalSize = 0;
        
        // 扫描目录中的所有文件
        std::vector<fs::path> files;
        scanDirectoryRecursive(path, files);
        
        if (files.empty()) {
            return true;
        }
        
        // 分割文件列表给各个线程
        std::vector<std::thread> threads;
        std::vector<std::vector<FileChecksum>> threadResults(threadCount);
        std::atomic<uint64_t> processedCount(0);
        
        size_t filesPerThread = files.size() / threadCount;
        size_t remainingFiles = files.size() % threadCount;
        
        size_t currentIndex = 0;
        for (int i = 0; i < threadCount; ++i) {
            size_t threadFileCount = filesPerThread;
            if (i < remainingFiles) {
                threadFileCount++;
            }
            
            std::vector<fs::path> threadFiles(
                files.begin() + currentIndex,
                files.begin() + currentIndex + threadFileCount
            );
            
            threads.emplace_back(&FileSystemCRC::fileChecksumWorker, this,
                               threadFiles, std::ref(threadResults[i]),
                               std::ref(processedCount), progressCallback);
            
            currentIndex += threadFileCount;
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 合并结果
        for (const auto& result : threadResults) {
            for (const auto& fileChecksum : result) {
                checksum.fileChecksums.push_back(fileChecksum);
                checksum.totalSize += fileChecksum.fileSize;
            }
        }
        
        // 计算目录级CRC32
        uint32_t directoryCRC = 0;
        for (const auto& fileChecksum : checksum.fileChecksums) {
            std::string relativePath = fs::relative(fileChecksum.filePath, directoryPath).string();
            uint32_t pathCRC = calculateCRC32ForData(
                std::vector<uint8_t>(relativePath.begin(), relativePath.end()));
            directoryCRC ^= (pathCRC ^ fileChecksum.crc32);
        }
        checksum.directoryCRC = directoryCRC;
        
        return !isOperationCancelled();
    } catch (const fs::filesystem_error& e) {
        setLastError("Filesystem error: " + std::string(e.what()));
        return false;
    }
}

bool FileSystemCRC::verifyDirectoryIntegrityParallel(const DirectoryChecksum& checksum, int threadCount,
                                                    std::function<void(int, int, const std::string&)> progressCallback) {
    resetCancellation();
    
    if (threadCount <= 0) threadCount = std::thread::hardware_concurrency();
    if (threadCount <= 0) threadCount = 4;
    
    if (checksum.fileChecksums.empty()) {
        return true;
    }
    
    std::vector<std::thread> threads;
    std::atomic<uint64_t> corruptedCount(0);
    std::atomic<uint64_t> processedCount(0);
    
    // 分割校验和列表给各个线程
    size_t checksumsPerThread = checksum.fileChecksums.size() / threadCount;
    size_t remainingChecksums = checksum.fileChecksums.size() % threadCount;
    
    size_t currentIndex = 0;
    for (int i = 0; i < threadCount; ++i) {
        size_t threadChecksumCount = checksumsPerThread;
        if (i < remainingChecksums) {
            threadChecksumCount++;
        }
        
        std::vector<FileChecksum> threadChecksums(
            checksum.fileChecksums.begin() + currentIndex,
            checksum.fileChecksums.begin() + currentIndex + threadChecksumCount
        );
        
        threads.emplace_back(&FileSystemCRC::fileVerificationWorker, this,
                           threadChecksums, std::ref(corruptedCount),
                           std::ref(processedCount), progressCallback);
        
        currentIndex += threadChecksumCount;
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    return corruptedCount == 0 && !isOperationCancelled();
}

// 文件操作
bool FileSystemCRC::saveChecksumsToFile(const std::string& filePath, const DirectoryChecksum& checksum) {
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) {
        setLastError("Cannot create output file: " + filePath);
        return false;
    }
    
    try {
        // 写入文件头
        uint32_t magic = 0x46534352; // "FSCR"
        outFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        
        // 写入目录信息
        uint32_t pathLength = checksum.directoryPath.length();
        outFile.write(reinterpret_cast<const char*>(&pathLength), sizeof(pathLength));
        outFile.write(checksum.directoryPath.c_str(), pathLength);
        
        outFile.write(reinterpret_cast<const char*>(&checksum.totalSize), sizeof(checksum.totalSize));
        outFile.write(reinterpret_cast<const char*>(&checksum.directoryCRC), sizeof(checksum.directoryCRC));
        outFile.write(reinterpret_cast<const char*>(&checksum.timestamp), sizeof(checksum.timestamp));
        
        // 写入文件数量
        uint32_t fileCount = checksum.fileChecksums.size();
        outFile.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));
        
        // 写入每个文件的校验和
        for (const auto& fileChecksum : checksum.fileChecksums) {
            uint32_t filePathLength = fileChecksum.filePath.length();
            outFile.write(reinterpret_cast<const char*>(&filePathLength), sizeof(filePathLength));
            outFile.write(fileChecksum.filePath.c_str(), filePathLength);
            
            outFile.write(reinterpret_cast<const char*>(&fileChecksum.fileSize), sizeof(fileChecksum.fileSize));
            outFile.write(reinterpret_cast<const char*>(&fileChecksum.crc32), sizeof(fileChecksum.crc32));
            outFile.write(reinterpret_cast<const char*>(&fileChecksum.timestamp), sizeof(fileChecksum.timestamp));
            outFile.write(reinterpret_cast<const char*>(&fileChecksum.lastModified), sizeof(fileChecksum.lastModified));
        }
        
        outFile.close();
        return true;
    } catch (const std::exception& e) {
        setLastError("Error writing checksum file: " + std::string(e.what()));
        return false;
    }
}

bool FileSystemCRC::loadChecksumsFromFile(const std::string& filePath, DirectoryChecksum& checksum) {
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        setLastError("Cannot open checksum file: " + filePath);
        return false;
    }
    
    try {
        // 读取文件头
        uint32_t magic;
        inFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x46534352) {
            setLastError("Invalid checksum file format");
            inFile.close();
            return false;
        }
        
        // 读取目录信息
        uint32_t pathLength;
        inFile.read(reinterpret_cast<char*>(&pathLength), sizeof(pathLength));
        checksum.directoryPath.resize(pathLength);
        inFile.read(&checksum.directoryPath[0], pathLength);
        
        inFile.read(reinterpret_cast<char*>(&checksum.totalSize), sizeof(checksum.totalSize));
        inFile.read(reinterpret_cast<char*>(&checksum.directoryCRC), sizeof(checksum.directoryCRC));
        inFile.read(reinterpret_cast<char*>(&checksum.timestamp), sizeof(checksum.timestamp));
        
        // 读取文件数量
        uint32_t fileCount;
        inFile.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));
        
        // 读取每个文件的校验和
        checksum.fileChecksums.resize(fileCount);
        for (uint32_t i = 0; i < fileCount; ++i) {
            uint32_t filePathLength;
            inFile.read(reinterpret_cast<char*>(&filePathLength), sizeof(filePathLength));
            checksum.fileChecksums[i].filePath.resize(filePathLength);
            inFile.read(&checksum.fileChecksums[i].filePath[0], filePathLength);
            
            inFile.read(reinterpret_cast<char*>(&checksum.fileChecksums[i].fileSize), sizeof(checksum.fileChecksums[i].fileSize));
            inFile.read(reinterpret_cast<char*>(&checksum.fileChecksums[i].crc32), sizeof(checksum.fileChecksums[i].crc32));
            inFile.read(reinterpret_cast<char*>(&checksum.fileChecksums[i].timestamp), sizeof(checksum.fileChecksums[i].timestamp));
            inFile.read(reinterpret_cast<char*>(&checksum.fileChecksums[i].lastModified), sizeof(checksum.fileChecksums[i].lastModified));
        }
        
        inFile.close();
        return true;
    } catch (const std::exception& e) {
        setLastError("Error reading checksum file: " + std::string(e.what()));
        return false;
    }
}

bool FileSystemCRC::savePartitionChecksumsToFile(const std::string& filePath, const PartitionChecksum& checksum) {
    // 简化的实现：保存为多个目录校验文件
    // 实际实现可以更复杂，保存整个分区的结构
    return saveChecksumsToFile(filePath, checksum.directoryChecksums[0]);
}

bool FileSystemCRC::loadPartitionChecksumsFromFile(const std::string& filePath, PartitionChecksum& checksum) {
    // 简化的实现：从目录校验文件加载
    DirectoryChecksum dirChecksum;
    if (loadChecksumsFromFile(filePath, dirChecksum)) {
        checksum.directoryChecksums.push_back(dirChecksum);
        checksum.partitionPath = dirChecksum.directoryPath;
        checksum.totalSize = dirChecksum.totalSize;
        checksum.partitionCRC = dirChecksum.directoryCRC;
        checksum.timestamp = dirChecksum.timestamp;
        return true;
    }
    return false;
}

// 控制方法
void FileSystemCRC::cancelOperation() {
    operationCancelled_ = true;
}

bool FileSystemCRC::isOperationCancelled() const {
    return operationCancelled_;
}

void FileSystemCRC::resetCancellation() {
    operationCancelled_ = false;
}

// 获取错误信息
std::string FileSystemCRC::getLastError() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(errorMutex_));
    return lastError_;
}

// 工具方法
uint32_t FileSystemCRC::calculateFileCRC32(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return 0;
    }
    
    std::vector<uint8_t> buffer(8192);
    uint32_t crc = 0xFFFFFFFF;
    
    while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
        size_t bytesRead = file.gcount();
        for (size_t i = 0; i < bytesRead; ++i) {
            crc ^= buffer[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320;
                } else {
                    crc >>= 1;
                }
            }
        }
    }
    
    file.close();
    return ~crc;
}

bool FileSystemCRC::compareFiles(const std::string& file1, const std::string& file2) {
    FileSystemCRC fsCRC;
    FileChecksum checksum1, checksum2;
    if (!fsCRC.generateFileChecksum(file1, checksum1) || !fsCRC.generateFileChecksum(file2, checksum2)) {
        return false;
    }
    return checksum1.crc32 == checksum2.crc32 && checksum1.fileSize == checksum2.fileSize;
}

bool FileSystemCRC::copyFileWithVerification(const std::string& source, const std::string& destination) {
    FileSystemCRC fsCRC;
    FileChecksum sourceChecksum;
    if (!fsCRC.generateFileChecksum(source, sourceChecksum)) {
        return false;
    }
    
    try {
        fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
        
        FileChecksum destChecksum;
        if (!fsCRC.generateFileChecksum(destination, destChecksum)) {
            return false;
        }
        
        return sourceChecksum.crc32 == destChecksum.crc32 && sourceChecksum.fileSize == destChecksum.fileSize;
    } catch (const fs::filesystem_error& e) {
        fsCRC.setLastError("Filesystem error during copy: " + std::string(e.what()));
        return false;
    }
}

// 辅助方法
void FileSystemCRC::scanDirectoryRecursive(const fs::path& directory, std::vector<fs::path>& files) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (isOperationCancelled()) {
                return;
            }
            
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        setLastError("Error scanning directory: " + std::string(e.what()));
    }
}

uint32_t FileSystemCRC::calculateCRC32ForData(const std::vector<uint8_t>& data) {
    // 简单的CRC32实现（与DiskSectorCRC保持一致）
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

uint32_t FileSystemCRC::calculateCRC32ForFile(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return 0;
    }
    
    std::vector<uint8_t> buffer(8192);
    uint32_t crc = 0xFFFFFFFF;
    
    while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
        size_t bytesRead = file.gcount();
        for (size_t i = 0; i < bytesRead; ++i) {
            crc ^= buffer[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320;
                } else {
                    crc >>= 1;
                }
            }
        }
    }
    
    file.close();
    return ~crc;
}

bool FileSystemCRC::readFileData(const fs::path& filePath, std::vector<uint8_t>& data) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return false;
    }
    
    file.close();
    return true;
}

// 工作线程函数
void FileSystemCRC::fileChecksumWorker(const std::vector<fs::path>& files, 
                                      std::vector<FileChecksum>& results,
                                      std::atomic<uint64_t>& processedCount,
                                      std::function<void(int, int, const std::string&)> progressCallback) {
    for (const auto& file : files) {
        if (isOperationCancelled()) {
            break;
        }
        
        FileChecksum checksum;
        if (generateFileChecksum(file.string(), checksum)) {
            results.push_back(checksum);
        }
        
        uint64_t processed = ++processedCount;
        if (progressCallback) {
            progressCallback(processed, files.size(), file.string());
        }
    }
}

void FileSystemCRC::fileVerificationWorker(const std::vector<FileChecksum>& checksums,
                                          std::atomic<uint64_t>& corruptedCount,
                                          std::atomic<uint64_t>& processedCount,
                                          std::function<void(int, int, const std::string&)> progressCallback) {
    for (const auto& checksum : checksums) {
        if (isOperationCancelled()) {
            break;
        }
        
        if (!verifyFileIntegrity(checksum)) {
            corruptedCount++;
        }
        
        uint64_t processed = ++processedCount;
        if (progressCallback) {
            progressCallback(processed, checksums.size(), checksum.filePath);
        }
    }
}

// 设置错误信息
void FileSystemCRC::setLastError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    lastError_ = error;
}
