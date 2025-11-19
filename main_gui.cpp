#include "GUIWindow.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <windows.h>
#include <iomanip>

// Simple console GUI simulation
class ConsoleGUI {
public:
    ConsoleGUI() : guiWindow_() {
        guiWindow_.setStatusCallback([this](const std::string& status) {
            std::cout << "[Status] " << status << std::endl;
        });
        
        guiWindow_.setProgressCallback([this](int current, int total) {
            int percent = (current * 100) / total;
            std::cout << "\rProgress: " << percent << "% (" << current << "/" << total << ")" << std::flush;
            if (current == total) {
                std::cout << std::endl;
            }
        });
    }
    
    void showMainMenu() {
        while (true) {
            std::cout << "\n==================================" << std::endl;
            std::cout << "   DATAVIA Data Integrity Tool" << std::endl;
            std::cout << "==================================" << std::endl;
            std::cout << "1. Generate Checksum Data" << std::endl;
            std::cout << "2. Verify Data Integrity" << std::endl;
            std::cout << "3. Repair Corrupted Data" << std::endl;
            std::cout << "4. CD/DVD Operations" << std::endl;
            std::cout << "5. List Available Disks" << std::endl;
            std::cout << "6. Exit" << std::endl;
            std::cout << "Please select operation (1-6): ";
            
            int choice;
            std::cin >> choice;
            
            switch (choice) {
                case 1:
                    generateChecksumsMenu();
                    break;
                case 2:
                    verifyIntegrityMenu();
                    break;
                case 3:
                    repairDataMenu();
                    break;
                case 4:
                    cdOperationsMenu();
                    break;
                case 5:
                    listAvailableDisksMenu();
                    break;
                case 6:
                    std::cout << "Thank you for using CRCRECOVER!" << std::endl;
                    return;
                default:
                    std::cout << "Invalid selection, please try again." << std::endl;
            }
        }
    }
    
private:
    GUIWindow guiWindow_;
    
    void generateChecksumsMenu() {
        std::cout << "\n--- Generate Checksum Data ---" << std::endl;
        
        std::string diskPath;
        std::cout << "Enter physical disk path (e.g., \\\\.\\PhysicalDrive0): ";
        std::cin >> diskPath;
        
        uint64_t startSector, sectorCount;
        std::cout << "Enter start sector: ";
        std::cin >> startSector;
        
        std::cout << "Enter sector count: ";
        std::cin >> sectorCount;
        
        std::string outputFile;
        std::cout << "Enter output filename: ";
        std::cin >> outputFile;
        
        std::cout << "Starting checksum generation..." << std::endl;
        bool result = guiWindow_.generateChecksums(diskPath, startSector, sectorCount, outputFile);
        
        if (result) {
            std::cout << "Checksum data generated successfully!" << std::endl;
        } else {
            std::cout << "Checksum data generation failed!" << std::endl;
        }
    }
    
    void verifyIntegrityMenu() {
        std::cout << "\n--- Verify Data Integrity ---" << std::endl;
        
        std::string diskPath;
        std::cout << "Enter disk path: ";
        std::cin >> diskPath;
        
        std::string checksumFile;
        std::cout << "Enter checksum filename: ";
        std::cin >> checksumFile;
        
        std::cout << "Starting data integrity verification..." << std::endl;
        bool result = guiWindow_.verifyIntegrity(diskPath, checksumFile);
        
        if (result) {
            std::cout << "Data integrity verification passed!" << std::endl;
        } else {
            std::cout << "Data integrity verification failed!" << std::endl;
        }
    }
    
    void repairDataMenu() {
        std::cout << "\n--- Repair Corrupted Data ---" << std::endl;
        
        std::string diskPath;
        std::cout << "Enter disk path: ";
        std::cin >> diskPath;
        
        std::string checksumFile;
        std::cout << "Enter checksum filename: ";
        std::cin >> checksumFile;
        
        std::string backupDiskPath;
        std::cout << "Enter backup disk path (optional, press Enter to skip): ";
        std::cin.ignore();
        std::getline(std::cin, backupDiskPath);
        
        std::cout << "Starting data repair..." << std::endl;
        bool result = guiWindow_.repairData(diskPath, checksumFile, 
                                           backupDiskPath.empty() ? "" : backupDiskPath);
        
        if (result) {
            std::cout << "Data repair completed!" << std::endl;
        } else {
            std::cout << "Problem occurred during data repair!" << std::endl;
        }
    }
    
    void cdOperationsMenu() {
        std::cout << "\n--- CD/DVD Operations ---" << std::endl;
        std::cout << "1. Generate CD/DVD Checksum Data" << std::endl;
        std::cout << "2. Verify CD/DVD Data Integrity" << std::endl;
        std::cout << "3. Return to Main Menu" << std::endl;
        std::cout << "Please select operation (1-3): ";
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                generateCDChecksumsMenu();
                break;
            case 2:
                verifyCDIntegrityMenu();
                break;
            case 3:
                return;
            default:
                std::cout << "Invalid selection, please try again." << std::endl;
        }
    }
    
    void generateCDChecksumsMenu() {
        std::cout << "\n--- Generate CD/DVD Checksum Data ---" << std::endl;
        
        std::string cdPath;
        std::cout << "Enter CD/DVD drive path (e.g., D:): ";
        std::cin >> cdPath;
        
        std::string outputFile;
        std::cout << "Enter output filename: ";
        std::cin >> outputFile;
        
        std::cout << "Starting CD/DVD checksum generation..." << std::endl;
        bool result = guiWindow_.generateCDChecksums(cdPath, outputFile);
        
        if (result) {
            std::cout << "CD/DVD checksum data generated successfully!" << std::endl;
        } else {
            std::cout << "CD/DVD checksum data generation failed!" << std::endl;
        }
    }
    
    void verifyCDIntegrityMenu() {
        std::cout << "\n--- Verify CD/DVD Data Integrity ---" << std::endl;
        
        std::string cdPath;
        std::cout << "Enter CD/DVD drive path: ";
        std::cin >> cdPath;
        
        std::string checksumFile;
        std::cout << "Enter checksum filename: ";
        std::cin >> checksumFile;
        
        std::cout << "Starting CD/DVD data integrity verification..." << std::endl;
        bool result = guiWindow_.verifyCDIntegrity(cdPath, checksumFile);
        
        if (result) {
            std::cout << "CD/DVD data integrity verification passed!" << std::endl;
        } else {
            std::cout << "CD/DVD data integrity verification failed!" << std::endl;
        }
    }
    
    void listAvailableDisksMenu() {
        std::cout << "\n--- Available Disks ---" << std::endl;
        std::cout << "Scanning for available disks..." << std::endl;
        
        std::vector<std::string> disks = guiWindow_.getAvailableDisks();
        
        if (disks.empty()) {
            std::cout << "No disks found or access denied." << std::endl;
            std::cout << "Try running as administrator for full disk access." << std::endl;
        } else {
            std::cout << "Found " << disks.size() << " disk(s):" << std::endl;
            std::cout << "------------------------------------------------------------" << std::endl;
            
            for (const auto& disk : disks) {
                std::string diskType = guiWindow_.getDiskType(disk);
                uint64_t totalSectors = guiWindow_.getDiskTotalSectors(disk);
                
                std::cout << "Disk: " << disk << std::endl;
                std::cout << "  Type: " << diskType << std::endl;
                std::cout << "  Total Sectors: " << totalSectors << std::endl;
                
                // Calculate disk size (GB)
                if (totalSectors > 0) {
                    uint64_t sectorSize = 512; // Assume standard sector size
                    uint64_t diskSizeBytes = totalSectors * sectorSize;
                    double diskSizeGB = static_cast<double>(diskSizeBytes) / (1024 * 1024 * 1024);
                    std::cout << "  Approx. Size: " << std::fixed << std::setprecision(2) << diskSizeGB << " GB" << std::endl;
                }
                
                std::cout << std::endl;
            }
            
            std::cout << "------------------------------------------------------------" << std::endl;
            std::cout << "Note: Use these disk paths in other operations." << std::endl;
        }
        
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore();
        std::cin.get();
    }
};

int main() {
    std::cout << "DATAVIA - Disk Sector Data Integrity Check and Repair Tool" << std::endl;
    std::cout << "Version 2.0 - Supports GUI and CD/DVD Operations" << std::endl;
    
    // Check administrator privileges
    #ifdef _WIN32
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD dwSize;
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    
    if (!isAdmin) {
        std::cout << "Warning: It is recommended to run this program with administrator privileges for full functionality." << std::endl;
        std::cout << "Some disk operations may require administrator privileges." << std::endl;
    }
    #endif
    
    ConsoleGUI gui;
    gui.showMainMenu();
    
    return 0;
}
