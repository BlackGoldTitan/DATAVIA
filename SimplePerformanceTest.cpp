#include <iostream>
#include <windows.h>
#include <string>
#include <chrono>
#include <vector>
#include <cstdint>

class SimplePerformanceTest {
private:
    std::string diskPath_;
    HANDLE hDisk_;
    
public:
    SimplePerformanceTest(const std::string& diskPath) : diskPath_(diskPath), hDisk_(INVALID_HANDLE_VALUE) {}
    
    ~SimplePerformanceTest() {
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
    
    void runSimpleTest() {
        std::cout << "=== Simple Performance Test ===" << std::endl;
        std::cout << "Disk Path: " << diskPath_ << std::endl;
        std::cout << std::endl;
        
        if (!openDisk()) {
            std::cout << "Error: Cannot open disk " << diskPath_ << std::endl;
            std::cout << "Error code: " << GetLastError() << std::endl;
            return;
        }
        
        // Test 1: Basic single sector read
        testSingleSector();
        
        // Test 2: Small batch read
        testSmallBatch();
        
        // Test 3: Medium batch read
        testMediumBatch();
        
        // Test 4: Large batch read
        testLargeBatch();
        
        std::cout << "=== Performance Analysis ===" << std::endl;
        analyzePerformance();
    }
    
private:
    void testSingleSector() {
        std::cout << "Test 1: Single Sector Read" << std::endl;
        
        std::vector<uint8_t> buffer(512);
        DWORD bytesRead;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        LARGE_INTEGER sectorOffset;
        sectorOffset.QuadPart = 0;
        
        if (SetFilePointerEx(hDisk_, sectorOffset, NULL, FILE_BEGIN)) {
            ReadFile(hDisk_, buffer.data(), 512, &bytesRead, NULL);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "  Time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "  Bytes read: " << bytesRead << std::endl;
        std::cout << std::endl;
    }
    
    void testSmallBatch() {
        std::cout << "Test 2: Small Batch (8 sectors = 4KB)" << std::endl;
        
        const int BATCH_SIZE = 8;
        std::vector<uint8_t> buffer(BATCH_SIZE * 512);
        DWORD bytesRead;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        LARGE_INTEGER sectorOffset;
        sectorOffset.QuadPart = 0;
        
        if (SetFilePointerEx(hDisk_, sectorOffset, NULL, FILE_BEGIN)) {
            ReadFile(hDisk_, buffer.data(), BATCH_SIZE * 512, &bytesRead, NULL);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double speedMBps = ((BATCH_SIZE * 512.0) / (1024 * 1024)) / (duration.count() / 1000000.0);
        
        std::cout << "  Time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "  Speed: " << speedMBps << " MB/s" << std::endl;
        std::cout << std::endl;
    }
    
    void testMediumBatch() {
        std::cout << "Test 3: Medium Batch (64 sectors = 32KB)" << std::endl;
        
        const int BATCH_SIZE = 64;
        std::vector<uint8_t> buffer(BATCH_SIZE * 512);
        DWORD bytesRead;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        LARGE_INTEGER sectorOffset;
        sectorOffset.QuadPart = 0;
        
        if (SetFilePointerEx(hDisk_, sectorOffset, NULL, FILE_BEGIN)) {
            ReadFile(hDisk_, buffer.data(), BATCH_SIZE * 512, &bytesRead, NULL);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double speedMBps = ((BATCH_SIZE * 512.0) / (1024 * 1024)) / (duration.count() / 1000000.0);
        
        std::cout << "  Time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "  Speed: " << speedMBps << " MB/s" << std::endl;
        std::cout << std::endl;
    }
    
    void testLargeBatch() {
        std::cout << "Test 4: Large Batch (256 sectors = 128KB)" << std::endl;
        
        const int BATCH_SIZE = 256;
        std::vector<uint8_t> buffer(BATCH_SIZE * 512);
        DWORD bytesRead;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        LARGE_INTEGER sectorOffset;
        sectorOffset.QuadPart = 0;
        
        if (SetFilePointerEx(hDisk_, sectorOffset, NULL, FILE_BEGIN)) {
            ReadFile(hDisk_, buffer.data(), BATCH_SIZE * 512, &bytesRead, NULL);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double speedMBps = ((BATCH_SIZE * 512.0) / (1024 * 1024)) / (duration.count() / 1000000.0);
        
        std::cout << "  Time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "  Speed: " << speedMBps << " MB/s" << std::endl;
        std::cout << std::endl;
    }
    
    void analyzePerformance() {
        std::cout << "Performance Analysis:" << std::endl;
        std::cout << "1. If single sector time > 1000 microseconds: High latency" << std::endl;
        std::cout << "2. If speed < 50 MB/s: Suboptimal performance" << std::endl;
        std::cout << "3. If speed < 10 MB/s: Very poor performance" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Common Issues:" << std::endl;
        std::cout << "- Antivirus software interference" << std::endl;
        std::cout << "- Disk fragmentation" << std::endl;
        std::cout << "- Insufficient permissions" << std::endl;
        std::cout << "- Hardware limitations" << std::endl;
        std::cout << "- Background processes" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "CRCRECOVER Simple Performance Test" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << std::endl;
    
    std::string diskPath;
    if (argc > 1) {
        diskPath = argv[1];
    } else {
        std::cout << "Enter disk path (e.g., D: or \\\\.\\C:): ";
        std::getline(std::cin, diskPath);
    }
    
    if (diskPath.empty()) {
        diskPath = "D:";
    }
    
    SimplePerformanceTest test(diskPath);
    test.runSimpleTest();
    
    std::cout << "Press Enter to exit...";
    std::cin.get();
    
    return 0;
}
