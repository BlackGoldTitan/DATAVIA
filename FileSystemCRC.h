#ifndef FILE_SYSTEM_CRC_H
#define FILE_SYSTEM_CRC_H

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

namespace fs = std::filesystem;

// 文件校验数据结构
struct FileChecksum {
    std::string filePath;
    uint64_t fileSize;
    uint32_t crc32;
    uint64_t timestamp;
    uint64_t lastModified;
};

// 文件夹校验数据结构
struct DirectoryChecksum {
    std::string directoryPath;
    std::vector<FileChecksum> fileChecksums;
    uint64_t totalSize;
    uint32_t directoryCRC;
    uint64_t timestamp;
};

// 分区校验数据结构
struct PartitionChecksum {
    std::string partitionPath;
    std::vector<DirectoryChecksum> directoryChecksums;
    uint64_t totalSize;
    uint32_t partitionCRC;
    uint64_t timestamp;
};

class FileSystemCRC {
public:
    FileSystemCRC();
    ~FileSystemCRC();
    
    // 文件校验方法
    bool generateFileChecksum(const std::string& filePath, FileChecksum& checksum);
    bool verifyFileIntegrity(const FileChecksum& checksum);
    bool repairFileFromBackup(const FileChecksum& checksum, const std::string& backupPath);
    
    // 文件夹校验方法
    bool generateDirectoryChecksums(const std::string& directoryPath, 
                                   DirectoryChecksum& checksum,
                                   std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    bool verifyDirectoryIntegrity(const DirectoryChecksum& checksum,
                                 std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    bool repairDirectoryFromBackup(const DirectoryChecksum& checksum, const std::string& backupPath,
                                  std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    
    // 分区校验方法
    bool generatePartitionChecksums(const std::string& partitionPath,
                                   PartitionChecksum& checksum,
                                   std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    bool verifyPartitionIntegrity(const PartitionChecksum& checksum,
                                 std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    bool repairPartitionFromBackup(const PartitionChecksum& checksum, const std::string& backupPath,
                                  std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    
    // 并行处理方法
    bool generateDirectoryChecksumsParallel(const std::string& directoryPath,
                                           DirectoryChecksum& checksum, int threadCount = 4,
                                           std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    bool verifyDirectoryIntegrityParallel(const DirectoryChecksum& checksum, int threadCount = 4,
                                         std::function<void(int, int, const std::string&)> progressCallback = nullptr);
    
    // 文件操作
    bool saveChecksumsToFile(const std::string& filePath, const DirectoryChecksum& checksum);
    bool loadChecksumsFromFile(const std::string& filePath, DirectoryChecksum& checksum);
    bool savePartitionChecksumsToFile(const std::string& filePath, const PartitionChecksum& checksum);
    bool loadPartitionChecksumsFromFile(const std::string& filePath, PartitionChecksum& checksum);
    
    // 控制方法
    void cancelOperation();
    bool isOperationCancelled() const;
    void resetCancellation();
    
    // 获取错误信息
    std::string getLastError() const;
    
    // 工具方法
    static uint32_t calculateFileCRC32(const std::string& filePath);
    static bool compareFiles(const std::string& file1, const std::string& file2);
    static bool copyFileWithVerification(const std::string& source, const std::string& destination);

private:
    std::atomic<bool> operationCancelled_;
    std::string lastError_;
    std::mutex errorMutex_;
    
    // 辅助方法
    void scanDirectoryRecursive(const fs::path& directory, std::vector<fs::path>& files);
    uint32_t calculateCRC32ForData(const std::vector<uint8_t>& data);
    uint32_t calculateCRC32ForFile(const fs::path& filePath);
    bool readFileData(const fs::path& filePath, std::vector<uint8_t>& data);
    
    // 工作线程函数
    void fileChecksumWorker(const std::vector<fs::path>& files, 
                           std::vector<FileChecksum>& results,
                           std::atomic<uint64_t>& processedCount,
                           std::function<void(int, int, const std::string&)> progressCallback);
    
    void fileVerificationWorker(const std::vector<FileChecksum>& checksums,
                               std::atomic<uint64_t>& corruptedCount,
                               std::atomic<uint64_t>& processedCount,
                               std::function<void(int, int, const std::string&)> progressCallback);
    
    // 设置错误信息
    void setLastError(const std::string& error);
};

#endif // FILE_SYSTEM_CRC_H
