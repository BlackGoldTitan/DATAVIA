#include "GUIWindow.h"
#include "HighPerformanceCRC.h"
#include <iostream>
#include <windows.h>
#include <fileapi.h>
#include <winioctl.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <fstream>

class GUIWindowOptimized : public GUIWindow {
private:
    HighPerformanceCRC* highPerfCRC_;
    
public:
    GUIWindowOptimized() : GUIWindow(), highPerfCRC_(nullptr) {
        // Initialize
    }
    
    ~GUIWindowOptimized() {
        if (highPerfCRC_) {
            delete highPerfCRC_;
        }
    }
    
    bool generateChecksumsHighPerformance(const std::string& diskPath, uint64_t startSector, 
                                         uint64_t sectorCount, const std::string& outputFile,
                                         int threadCount = 4, int batchSize = 256) {
        if (statusCallback_) {
            statusCallback_("Initializing high-performance disk access...");
        }
        
        highPerfCRC_ = new HighPerformanceCRC(diskPath);
        
        if (!highPerfCRC_->checkFilePermissions()) {
            if (statusCallback_) {
                statusCallback_("Error: " + highPerfCRC_->getLastError());
            }
            return false;
        }
        
        if (statusCallback_) {
            statusCallback_("Starting high-performance checksum generation...");
        }
        
        // Set up progress callback
        auto progressCallback = [this](int current, int total) {
            if (progressCallback_) {
                progressCallback_(current, total);
            }
        };
        
        bool result = highPerfCRC_->generateChecksumsHighPerformance(
            startSector, sectorCount, outputFile, threadCount, batchSize, progressCallback);
        
        if (result) {
            if (statusCallback_) {
                statusCallback_("High-performance checksum data generated successfully!");
            }
        } else {
            if (statusCallback_) {
                statusCallback_("Error: " + highPerfCRC_->getLastError());
            }
        }
        
        return result;
    }
    
    bool verifyIntegrityHighPerformance(const std::string& diskPath, const std::string& checksumFile,
                                       int threadCount = 4) {
        if (statusCallback_) {
            statusCallback_("Initializing high-performance disk access...");
        }
        
        highPerfCRC_ = new HighPerformanceCRC(diskPath);
        
        if (!highPerfCRC_->checkFilePermissions()) {
            if (statusCallback_) {
                statusCallback_("Error: " + highPerfCRC_->getLastError());
            }
            return false;
        }
        
        if (statusCallback_) {
            statusCallback_("Starting high-performance data integrity verification...");
        }
        
        // Set up progress callback
        auto progressCallback = [this](int current, int total) {
            if (progressCallback_) {
                progressCallback_(current, total);
            }
        };
        
        bool result = highPerfCRC_->verifyIntegrityParallel(checksumFile, threadCount, progressCallback);
        
        if (result) {
            if (statusCallback_) {
                statusCallback_("High-performance data integrity verification passed!");
            }
        } else {
            if (statusCallback_) {
                statusCallback_("High-performance data integrity verification failed!");
            }
        }
        
        return result;
    }
    
    bool repairDataHighPerformance(const std::string& diskPath, const std::string& checksumFile, 
                                  const std::string& backupDiskPath, int threadCount = 4) {
        if (statusCallback_) {
            statusCallback_("Initializing high-performance disk access...");
        }
        
        highPerfCRC_ = new HighPerformanceCRC(diskPath);
        
        if (!highPerfCRC_->checkFilePermissions()) {
            if (statusCallback_) {
                statusCallback_("Error: " + highPerfCRC_->getLastError());
            }
            return false;
        }
        
        if (statusCallback_) {
            statusCallback_("Starting high-performance data repair...");
        }
        
        // Set up progress callback
        auto progressCallback = [this](int current, int total) {
            if (progressCallback_) {
                progressCallback_(current, total);
            }
        };
        
        bool result = highPerfCRC_->repairDataParallel(checksumFile, backupDiskPath, threadCount, progressCallback);
        
        if (result) {
            if (statusCallback_) {
                statusCallback_("High-performance data repair completed!");
            }
        } else {
            if (statusCallback_) {
                statusCallback_("Problem occurred during high-performance data repair: " + highPerfCRC_->getLastError());
            }
        }
        
        return result;
    }
    
    // Override base class methods to use high-performance versions
    bool generateChecksums(const std::string& diskPath, uint64_t startSector, 
                          uint64_t sectorCount, const std::string& outputFile) override {
        return generateChecksumsHighPerformance(diskPath, startSector, sectorCount, outputFile);
    }
    
    bool verifyIntegrity(const std::string& diskPath, const std::string& checksumFile) override {
        return verifyIntegrityHighPerformance(diskPath, checksumFile);
    }
    
    bool repairData(const std::string& diskPath, const std::string& checksumFile, 
                   const std::string& backupDiskPath) override {
        return repairDataHighPerformance(diskPath, checksumFile, backupDiskPath);
    }
    
    // Performance optimization settings
    void setPerformanceSettings(int threadCount, int batchSize) {
        if (highPerfCRC_) {
            // These settings will be used in the next operation
            // The actual implementation would store these settings
        }
    }
    
    std::string getPerformanceInfo() {
        if (highPerfCRC_) {
            return highPerfCRC_->getPerformanceInfo();
        }
        return "HighPerformanceCRC not initialized";
    }
};
