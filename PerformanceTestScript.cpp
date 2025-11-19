#include <iostream>
#include <windows.h>
#include <string>
#include <chrono>
#include <vector>
#include <cstdint>

class PerformanceTestScript {
public:
    static void runPerformanceTests() {
        std::cout << "=== CRCRECOVER 性能测试脚本 ===" << std::endl;
        std::cout << std::endl;
        
        std::cout << "1. 测试逻辑驱动器性能 (D:)" << std::endl;
        testLogicalDrivePerformance();
        
        std::cout << std::endl;
        std::cout << "2. 测试物理磁盘性能 (如果可用)" << std::endl;
        testPhysicalDiskPerformance();
        
        std::cout << std::endl;
        std::cout << "3. 性能优化建议" << std::endl;
        provideOptimizationSuggestions();
    }
    
private:
    static void testLogicalDrivePerformance() {
        std::cout << "测试逻辑驱动器 D: 的性能..." << std::endl;
        
        // 测试不同批量大小的读取性能
        std::vector<int> batchSizes = {1, 8, 16, 32, 64, 128, 256, 512};
        const int TOTAL_SECTORS = 2048; // 1MB
        
        for (int batchSize : batchSizes) {
            if (batchSize > TOTAL_SECTORS) continue;
            
            std::vector<uint8_t> buffer(batchSize * 512);
            DWORD bytesRead;
            int batches = TOTAL_SECTORS / batchSize;
            
            HANDLE hDisk = CreateFileA("\\\\.\\D:", 
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
            
            if (hDisk == INVALID_HANDLE_VALUE) {
                std::cout << "  无法打开磁盘 D:，错误代码: " << GetLastError() << std::endl;
                return;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int batch = 0; batch < batches; batch++) {
                LARGE_INTEGER sectorOffset;
                sectorOffset.QuadPart = batch * batchSize * 512;
                
                if (SetFilePointerEx(hDisk, sectorOffset, NULL, FILE_BEGIN)) {
                    ReadFile(hDisk, buffer.data(), batchSize * 512, &bytesRead, NULL);
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            double totalBytes = batches * batchSize * 512.0;
            double speedMBps = (totalBytes / (1024 * 1024)) / (duration.count() / 1000.0);
            
            std::cout << "  批量 " << batchSize << " 扇区: " << speedMBps << " MB/s" << std::endl;
            
            CloseHandle(hDisk);
        }
    }
    
    static void testPhysicalDiskPerformance() {
        std::cout << "测试物理磁盘性能..." << std::endl;
        
        // 尝试不同的物理磁盘路径
        std::vector<std::string> physicalDisks = {
            "\\\\.\\PhysicalDrive0",
            "\\\\.\\PhysicalDrive1", 
            "\\\\.\\PhysicalDrive2"
        };
        
        for (const auto& diskPath : physicalDisks) {
            HANDLE hDisk = CreateFileA(diskPath.c_str(), 
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
            
            if (hDisk != INVALID_HANDLE_VALUE) {
                std::cout << "  找到物理磁盘: " << diskPath << std::endl;
                
                // 测试读取性能
                const int TEST_SECTORS = 256;
                std::vector<uint8_t> buffer(TEST_SECTORS * 512);
                DWORD bytesRead;
                
                LARGE_INTEGER sectorOffset;
                sectorOffset.QuadPart = 0;
                
                auto start = std::chrono::high_resolution_clock::now();
                
                if (SetFilePointerEx(hDisk, sectorOffset, NULL, FILE_BEGIN)) {
                    ReadFile(hDisk, buffer.data(), TEST_SECTORS * 512, &bytesRead, NULL);
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                double totalBytes = TEST_SECTORS * 512.0;
                double speedMBps = (totalBytes / (1024 * 1024)) / (duration.count() / 1000.0);
                
                std::cout << "  读取速度: " << speedMBps << " MB/s" << std::endl;
                
                CloseHandle(hDisk);
                break;
            }
        }
    }
    
    static void provideOptimizationSuggestions() {
        std::cout << "=== 性能优化建议 ===" << std::endl;
        std::cout << std::endl;
        
        std::cout << "1. 使用逻辑驱动器路径 (如 D:)" << std::endl;
        std::cout << "   - 性能通常比物理磁盘路径更好" << std::endl;
        std::cout << "   - 不需要管理员权限" << std::endl;
        std::cout << std::endl;
        
        std::cout << "2. 推荐的批量大小" << std::endl;
        std::cout << "   - 256-512 扇区 (128-256 KB)" << std::endl;
        std::cout << "   - 避免小批量读取" << std::endl;
        std::cout << std::endl;
        
        std::cout << "3. 系统优化" << std::endl;
        std::cout << "   - 暂时禁用杀毒软件实时保护" << std::endl;
        std::cout << "   - 确保有足够的内存" << std::endl;
        std::cout << "   - 关闭不必要的后台程序" << std::endl;
        std::cout << std::endl;
        
        std::cout << "4. 预期性能范围" << std::endl;
        std::cout << "   - HDD: 50-150 MB/s" << std::endl;
        std::cout << "   - SSD: 200-500 MB/s" << std::endl;
        std::cout << "   - NVMe SSD: 500-2000 MB/s" << std::endl;
        std::cout << std::endl;
        
        std::cout << "5. 如果性能低于预期" << std::endl;
        std::cout << "   - 检查杀毒软件是否在扫描" << std::endl;
        std::cout << "   - 检查磁盘是否有其他高负载操作" << std::endl;
        std::cout << "   - 尝试不同的磁盘路径" << std::endl;
    }
};

int main() {
    std::cout << "CRCRECOVER 性能测试脚本" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << std::endl;
    
    PerformanceTestScript::runPerformanceTests();
    
    std::cout << std::endl;
    std::cout << "按 Enter 键退出...";
    std::cin.get();
    
    return 0;
}
