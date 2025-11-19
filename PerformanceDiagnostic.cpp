#include <iostream>
#include <windows.h>
#include <string>
#include <chrono>
#include <vector>
#include <cstdint>

class PerformanceDiagnostic {
private:
    std::string diskPath_;
    HANDLE hDisk_;
    
public:
    PerformanceDiagnostic(const std::string& diskPath) : diskPath_(diskPath), hDisk_(INVALID_HANDLE_VALUE) {}
    
    ~PerformanceDiagnostic() {
        if (hDisk_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hDisk_);
        }
    }
    
    bool openDisk() {
        hDisk_ = CreateFileA(diskPath_.c_str(), 
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
        return hDisk_ != INVALID_HANDLE_VALUE;
    }
    
    void runDiagnostics() {
        std::cout << "=== Performance Diagnostic Tool ===" << std::endl;
        std::cout << "Disk Path: " << diskPath_ << std::endl;
        std::cout << std::endl;
        
        if (!openDisk()) {
            std::cout << "Error: Cannot open disk " << diskPath_ << std::endl;
            std::cout << "Error code: " << GetLastError() << std::endl;
            return;
        }
        
        // Test 1: Single sector read performance
        testSingleSectorPerformance();
        
        // Test 2: Batch read performance
        testBatchReadPerformance();
        
        // Test 3: Different batch sizes performance
        testDifferentBatchSizes();
        
        // Test 4: Disk information
        getDiskInfo();
        
        std::cout << "=== Performance Optimization Suggestions ===" << std::endl;
        provideOptimizationSuggestions();
    }
    
private:
    void testSingleSectorPerformance() {
        std::cout << "Test 1: Single Sector Read Performance" << std::endl;
        
        const int TEST_SECTORS = 100;
        std::vector<uint8_t> buffer(512);
        DWORD bytesRead;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < TEST_SECTORS; i++) {
            LARGE_INTEGER sectorOffset;
            sectorOffset.QuadPart = i * 512;
            
            if (SetFilePointerEx(hDisk_, sectorOffset, NULL, FILE_BEGIN)) {
                ReadFile(hDisk_, buffer.data(), 512, &bytesRead, NULL);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double totalBytes = TEST_SECTORS * 512.0;
        double speedMBps = (totalBytes / (1024 * 1024)) / (duration.count() / 1000.0);
        
        std::cout << "  Read " << TEST_SECTORS << " sectors (" << (totalBytes/1024) << " KB)" << std::endl;
        std::cout << "  Time: " << duration.count() << " ms" << std::endl;
        std::cout << "  Speed: " << speedMBps << " MB/s" << std::endl;
        std::cout << std::endl;
    }
    
    void testBatchReadPerformance() {
        std::cout << "测试2: 批量读取性能" << std::endl;
        
        const int BATCH_SIZE = 256; // 256个扇区 = 128KB
        const int TEST_BATCHES = 10;
        
        std::vector<uint8_t> buffer(BATCH_SIZE * 512);
        DWORD bytesRead;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int batch = 0; batch < TEST_BATCHES; batch++) {
            LARGE_INTEGER sectorOffset;
            sectorOffset.QuadPart = batch * BATCH_SIZE * 512;
            
            if (SetFilePointerEx(hDisk_, sectorOffset, NULL, FILE_BEGIN)) {
                ReadFile(hDisk_, buffer.data(), BATCH_SIZE * 512, &bytesRead, NULL);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double totalBytes = TEST_BATCHES * BATCH_SIZE * 512.0;
        double speedMBps = (totalBytes / (1024 * 1024)) / (duration.count() / 1000.0);
        
        std::cout << "  批量大小: " << BATCH_SIZE << " 扇区 (" << (BATCH_SIZE * 512 / 1024) << " KB)" << std::endl;
        std::cout << "  读取 " << TEST_BATCHES << " 批次 (" << (totalBytes/1024) << " KB)" << std::endl;
        std::cout << "  耗时: " << duration.count() << " ms" << std::endl;
        std::cout << "  速度: " << speedMBps << " MB/s" << std::endl;
        std::cout << std::endl;
    }
    
    void testDifferentBatchSizes() {
        std::cout << "测试3: 不同批量大小性能对比" << std::endl;
        
        std::vector<int> batchSizes = {1, 8, 16, 32, 64, 128, 256, 512};
        const int TOTAL_SECTORS = 1024;
        
        for (int batchSize : batchSizes) {
            if (batchSize > TOTAL_SECTORS) continue;
            
            std::vector<uint8_t> buffer(batchSize * 512);
            DWORD bytesRead;
            int batches = TOTAL_SECTORS / batchSize;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int batch = 0; batch < batches; batch++) {
                LARGE_INTEGER sectorOffset;
                sectorOffset.QuadPart = batch * batchSize * 512;
                
                if (SetFilePointerEx(hDisk_, sectorOffset, NULL, FILE_BEGIN)) {
                    ReadFile(hDisk_, buffer.data(), batchSize * 512, &bytesRead, NULL);
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            double totalBytes = batches * batchSize * 512.0;
            double speedMBps = (totalBytes / (1024 * 1024)) / (duration.count() / 1000.0);
            
            std::cout << "  批量 " << batchSize << " 扇区: " << speedMBps << " MB/s" << std::endl;
        }
        std::cout << std::endl;
    }
    
    void getDiskInfo() {
        std::cout << "测试4: 磁盘信息" << std::endl;
        
        DISK_GEOMETRY geometry;
        DWORD bytesReturned;
        
        if (DeviceIoControl(hDisk_, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                          NULL, 0, &geometry, sizeof(geometry),
                          &bytesReturned, NULL)) {
            uint64_t diskSize = geometry.Cylinders.QuadPart *
                               geometry.TracksPerCylinder *
                               geometry.SectorsPerTrack *
                               geometry.BytesPerSector;
            
            std::cout << "  磁盘大小: " << (diskSize / (1024 * 1024 * 1024)) << " GB" << std::endl;
            std::cout << "  扇区大小: " << geometry.BytesPerSector << " 字节" << std::endl;
            std::cout << "  每磁道扇区数: " << geometry.SectorsPerTrack << std::endl;
            std::cout << "  每柱面磁道数: " << geometry.TracksPerCylinder << std::endl;
        }
        
        // 检查是否支持缓存
        BOOL cacheEnabled;
        if (DeviceIoControl(hDisk_, IOCTL_DISK_GET_CACHE_INFORMATION,
                          NULL, 0, &cacheEnabled, sizeof(cacheEnabled),
                          &bytesReturned, NULL)) {
            std::cout << "  磁盘缓存: " << (cacheEnabled ? "启用" : "禁用") << std::endl;
        }
        std::cout << std::endl;
    }
    
    void provideOptimizationSuggestions() {
        std::cout << "1. 使用更大的批量大小 (推荐 256-512 扇区)" << std::endl;
        std::cout << "2. 确保以管理员权限运行" << std::endl;
        std::cout << "3. 检查磁盘是否支持缓存" << std::endl;
        std::cout << "4. 使用高性能CRC模式" << std::endl;
        std::cout << "5. 考虑使用内存映射文件" << std::endl;
        std::cout << "6. 检查杀毒软件是否在扫描磁盘" << std::endl;
        std::cout << "7. 确保磁盘没有其他高负载操作" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "CRCRECOVER 性能诊断工具" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << std::endl;
    
    std::string diskPath;
    if (argc > 1) {
        diskPath = argv[1];
    } else {
        std::cout << "请输入磁盘路径 (例如: \\\\.\\C: 或 D:): ";
        std::getline(std::cin, diskPath);
    }
    
    if (diskPath.empty()) {
        diskPath = "\\\\.\\C:";
    }
    
    PerformanceDiagnostic diagnostic(diskPath);
    diagnostic.runDiagnostics();
    
    std::cout << "按 Enter 键退出...";
    std::cin.get();
    
    return 0;
}
