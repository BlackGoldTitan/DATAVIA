#include "OptimizedDiskReader.h"
#include <iostream>
#include <algorithm>

// 扇区大小常量
static constexpr uint32_t SECTOR_SIZE = 512;

OptimizedDiskReader::OptimizedDiskReader(const std::string& diskPath)
    : diskPath_(diskPath), hDisk_(INVALID_HANDLE_VALUE), batchSize_(64), nextBufferIndex_(0) {
    
    // 在Windows上，磁盘路径需要以"\\\\.\\"开头
    if (diskPath_.find("\\\\.\\") == std::string::npos) {
        diskPath_ = "\\\\.\\" + diskPath_;
    }
    
    // 预分配一些缓冲区
    preallocateBuffers(batchSize_);
}

OptimizedDiskReader::~OptimizedDiskReader() {
    closeDisk();
    releaseAllBuffers();
}

bool OptimizedDiskReader::openDisk() {
    if (isOpen()) {
        return true; // 已经打开
    }
    
    hDisk_ = CreateFileA(diskPath_.c_str(), 
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    
    if (hDisk_ == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::string errorMsg;
        
        switch (error) {
            case ERROR_ACCESS_DENIED:
                errorMsg = "Access denied. Please run the program with administrator privileges";
                break;
            case ERROR_FILE_NOT_FOUND:
                errorMsg = "Disk not found. Please check the disk path: " + diskPath_;
                break;
            case ERROR_INVALID_PARAMETER:
                errorMsg = "Invalid disk path: " + diskPath_;
                break;
            case ERROR_SHARING_VIOLATION:
                errorMsg = "Disk is in use by another process: " + diskPath_;
                break;
            default:
                errorMsg = "Cannot open disk: " + diskPath_ + " (Error code: " + std::to_string(error) + ")";
                break;
        }
        
        lastError_ = errorMsg;
        return false;
    }
    
    return true;
}

void OptimizedDiskReader::closeDisk() {
    if (isOpen()) {
        CloseHandle(hDisk_);
        hDisk_ = INVALID_HANDLE_VALUE;
    }
}

bool OptimizedDiskReader::readSectorsBatch(uint64_t startSector, uint64_t count, 
                                          std::vector<std::vector<uint8_t>>& batchData) {
    if (!ensureDiskOpen()) {
        return false;
    }
    
    // 限制批量大小
    count = std::min(count, static_cast<uint64_t>(batchSize_));
    batchData.clear();
    batchData.reserve(count);
    
    // 使用预分配的缓冲区
    for (uint64_t i = 0; i < count; ++i) {
        auto& buffer = acquireBuffer();
        if (!readSector(startSector + i, buffer)) {
            // 如果读取失败，清除缓冲区并继续
            buffer.clear();
        }
        batchData.push_back(std::move(buffer));
    }
    
    // 重置缓冲区索引以便下次使用
    nextBufferIndex_ = 0;
    
    return true;
}

bool OptimizedDiskReader::readSector(uint64_t sectorNumber, std::vector<uint8_t>& buffer) {
    if (!ensureDiskOpen()) {
        return false;
    }
    
    // 设置文件指针到指定扇区
    LARGE_INTEGER liDistance;
    liDistance.QuadPart = sectorNumber * SECTOR_SIZE;
    
    if (SetFilePointerEx(hDisk_, liDistance, NULL, FILE_BEGIN) == 0) {
        lastError_ = "Cannot set file pointer to sector: " + std::to_string(sectorNumber);
        return false;
    }
    
    // 确保缓冲区大小正确
    if (buffer.size() != SECTOR_SIZE) {
        buffer.resize(SECTOR_SIZE);
    }
    
    // 读取扇区数据
    DWORD bytesRead;
    if (!ReadFile(hDisk_, buffer.data(), SECTOR_SIZE, &bytesRead, NULL)) {
        lastError_ = "Failed to read sector: " + std::to_string(sectorNumber);
        return false;
    }
    
    return bytesRead == SECTOR_SIZE;
}

std::string OptimizedDiskReader::getLastError() const {
    return lastError_;
}

void OptimizedDiskReader::preallocateBuffers(size_t count) {
    bufferPool_.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        bufferPool_.emplace_back(SECTOR_SIZE);
    }
    nextBufferIndex_ = 0;
}

bool OptimizedDiskReader::ensureDiskOpen() {
    if (!isOpen()) {
        return openDisk();
    }
    return true;
}

std::vector<uint8_t>& OptimizedDiskReader::acquireBuffer() {
    if (nextBufferIndex_ < bufferPool_.size()) {
        // 使用预分配的缓冲区
        auto& buffer = bufferPool_[nextBufferIndex_++];
        // 确保缓冲区大小正确
        if (buffer.size() != SECTOR_SIZE) {
            buffer.resize(SECTOR_SIZE);
        }
        return buffer;
    } else {
        // 动态分配新的缓冲区
        bufferPool_.emplace_back(SECTOR_SIZE);
        return bufferPool_.back();
    }
}

void OptimizedDiskReader::releaseAllBuffers() {
    bufferPool_.clear();
    nextBufferIndex_ = 0;
}
