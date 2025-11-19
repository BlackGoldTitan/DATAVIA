#include "DiskSectorCRC.h"
#include <iostream>
#include <string>
#include <vector>

void printUsage() {
    std::cout << "Disk Sector Data Integrity Check and Repair Tool" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  CRCRECOVER <command> [parameters]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  generate <disk_path> <start_sector> <sector_count> <output_file> - Generate checksum data" << std::endl;
    std::cout << "  verify <disk_path> <checksum_file> - Verify data integrity" << std::endl;
    std::cout << "  repair <disk_path> <checksum_file> [backup_disk_path] - Repair corrupted data" << std::endl;
    std::cout << "  help - Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  CRCRECOVER generate C: 0 1000 checksums.dat" << std::endl;
    std::cout << "  CRCRECOVER verify C: checksums.dat" << std::endl;
    std::cout << "  CRCRECOVER repair C: checksums.dat D:" << std::endl;
    std::cout << std::endl;
    std::cout << "Notes:" << std::endl;
    std::cout << "  - Disk path can be physical disk (e.g., \\\\.\\PhysicalDrive0) or logical partition (e.g., C:)" << std::endl;
    std::cout << "  - Administrator privileges required to access physical disks" << std::endl;
    std::cout << "  - Repair function requires valid backup disk" << std::endl;
}

bool parseUint64(const std::string& str, uint64_t& value) {
    try {
        value = std::stoull(str);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "help") {
        printUsage();
        return 0;
    }
    else if (command == "generate") {
        if (argc != 6) {
            std::cout << "Error: generate command requires 4 parameters" << std::endl;
            printUsage();
            return 1;
        }

        std::string diskPath = argv[2];
        std::string startSectorStr = argv[3];
        std::string sectorCountStr = argv[4];
        std::string outputFile = argv[5];

        uint64_t startSector, sectorCount;
        if (!parseUint64(startSectorStr, startSector) || !parseUint64(sectorCountStr, sectorCount)) {
            std::cout << "Error: start sector and sector count must be valid numbers" << std::endl;
            return 1;
        }

        std::cout << "Initializing disk access..." << std::endl;
        DiskSectorCRC disk(diskPath);

        // Check basic permissions first
        if (!disk.checkFilePermissions()) {
            std::cout << "Error: " << disk.getLastError() << std::endl;
            return 1;
        }

        std::cout << "Starting checksum generation..." << std::endl;
        if (disk.generateSectorChecksums(startSector, sectorCount, outputFile)) {
            std::cout << "Checksum data generated successfully!" << std::endl;
            return 0;
        } else {
            std::cout << "Error: " << disk.getLastError() << std::endl;
            return 1;
        }
    }
    else if (command == "verify") {
        if (argc != 4) {
            std::cout << "Error: verify command requires 2 parameters" << std::endl;
            printUsage();
            return 1;
        }

        std::string diskPath = argv[2];
        std::string checksumFile = argv[3];

        std::cout << "Initializing disk access..." << std::endl;
        DiskSectorCRC disk(diskPath);

        if (!disk.checkFilePermissions()) {
            std::cout << "Error: " << disk.getLastError() << std::endl;
            return 1;
        }

        std::cout << "Starting data integrity verification..." << std::endl;
        if (disk.verifySectorIntegrity(checksumFile)) {
            std::cout << "Data integrity verification passed!" << std::endl;
            return 0;
        } else {
            std::cout << "Data integrity verification failed!" << std::endl;
            return 1;
        }
    }
    else if (command == "repair") {
        if (argc < 4 || argc > 5) {
            std::cout << "Error: repair command requires 2-3 parameters" << std::endl;
            printUsage();
            return 1;
        }

        std::string diskPath = argv[2];
        std::string checksumFile = argv[3];
        std::string backupDiskPath = (argc == 5) ? argv[4] : "";

        std::cout << "Initializing disk access..." << std::endl;
        DiskSectorCRC disk(diskPath);

        if (!disk.checkFilePermissions()) {
            std::cout << "Error: " << disk.getLastError() << std::endl;
            return 1;
        }

        std::cout << "Starting data repair..." << std::endl;
        if (disk.repairSectorData(checksumFile, backupDiskPath)) {
            std::cout << "Data repair completed!" << std::endl;
            return 0;
        } else {
            std::cout << "Problem occurred during data repair: " << disk.getLastError() << std::endl;
            return 1;
        }
    }
    else {
        std::cout << "Error: Unknown command '" << command << "'" << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}
