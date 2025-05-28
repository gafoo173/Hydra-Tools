#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <ctime>

// تعريفات لأنظمة الملفات
enum class FileSystemType {
    UNKNOWN,
    FAT32,
    NTFS
};

class FileSystemAnalyzer {
public:
    struct FileEntry {
        std::string name;
        uint64_t size;
        std::string creationTime;
        std::string modificationTime;
        bool deleted;
    };

    // تحليل البيانات وتوقع نوع نظام الملفات
    static FileSystemType detectFileSystem(const std::vector<uint8_t>& bootSector) {
        if (bootSector.size() < 512) return FileSystemType::UNKNOWN;

        // FAT32: التوقيع "FAT32" موجود عند offset 82
        if (bootSector[0x52] == 'F' && bootSector[0x53] == 'A' &&
            bootSector[0x54] == 'T' && bootSector[0x55] == '3' &&
            bootSector[0x56] == '2') {
            return FileSystemType::FAT32;
        }

        // NTFS: التوقيع "NTFS    " عند offset 3
        if (bootSector[0x03] == 'N' && bootSector[0x04] == 'T' &&
            bootSector[0x05] == 'F' && bootSector[0x06] == 'S') {
            return FileSystemType::NTFS;
        }

        return FileSystemType::UNKNOWN;
    }

    // تحليل FAT32 واستخراج بيانات أولية
    static std::vector<FileEntry> analyzeFat32(const std::vector<uint8_t>& data) {
        std::vector<FileEntry> entries;

        // FAT32 Boot Sector Info
        uint16_t bytesPerSector = readUint16LE(data, 0x0B);
        uint8_t sectorsPerCluster = data[0x0D];
        uint16_t reservedSectors = readUint16LE(data, 0x0E);
        uint8_t numberOfFats = data[0x10];
        uint16_t rootDirEntries = readUint16LE(data, 0x11);
        uint32_t totalSectors = readUint32LE(data, 0x20);
        uint32_t fatSize = readUint32LE(data, 0x24);

        std::cout << "[+] Detected FAT32 system\n";
        std::cout << " - Bytes per sector: " << bytesPerSector << "\n";
        std::cout << " - Sectors per cluster: " << (int)sectorsPerCluster << "\n";
        std::cout << " - FAT size: " << fatSize << " sectors\n";

        // موقع جدول FAT
        uint32_t fatStart = reservedSectors * bytesPerSector;
        uint32_t rootDirStart = fatStart + numberOfFats * fatSize * bytesPerSector;
        uint32_t rootDirSize = rootDirEntries * 32; // كل إدخال 32 بايت

        for (size_t i = 0; i < rootDirSize; i += 32) {
            size_t offset = rootDirStart + i;
            if (offset + 32 > data.size()) break;

            uint8_t attr = data[offset + 0x0B];

            // تخطي الإدخالات الفارغة
            if (data[offset] == 0xE5 || data[offset] == 0x00) continue;

            // اسم الملف
            char filename[9];
            memcpy(filename, &data[offset], 8);
            filename[8] = '\0';

            // امتداد الملف
            char ext[4];
            memcpy(ext, &data[offset + 8], 3);
            ext[3] = '\0';

            std::string name = std::string(filename) + "." + std::string(ext);

            // الحجم
            uint32_t fileSize = readUint32LE(data, offset + 0x1C);

            // حالة الحذف
            bool isDeleted = (data[offset] == 0xE5);

            entries.push_back({
                .name = name,
                .size = fileSize,
                .creationTime = "unknown",
                .modificationTime = "unknown",
                .deleted = isDeleted
            });
        }

        return entries;
    }

    // تحليل NTFS واستخراج بيانات أولية (MFT)
    static std::vector<FileEntry> analyzeNtfs(const std::vector<uint8_t>& data) {
        std::vector<FileEntry> entries;

        if (data.size() < 1024) return entries;

        // MFT Start Cluster من رأس القطاع
        uint64_t mftStartCluster = readUint64LE(data, 0x30);
        uint16_t bytesPerSector = readUint16LE(data, 0x0B);
        uint8_t sectorsPerCluster = data[0x0D];

        std::cout << "[+] Detected NTFS system\n";
        std::cout << " - MFT start cluster: " << mftStartCluster << "\n";
        std::cout << " - Bytes per sector: " << bytesPerSector << "\n";
        std::cout << " - Sectors per cluster: " << (int)sectorsPerCluster << "\n";

        // TODO: قراءة MFT من القرص الكامل وليس فقط الجزء الأول
        // هنا نقوم بقراءة أول 1KB فقط كمثال
        for (size_t offset = 0; offset < data.size(); offset += 0x400) {
            if (offset + 0x400 > data.size()) break;

            // التحقق من توقيع "$FILE"
            if (data[offset] != '$' || data[offset+1] != 'F' ||
                data[offset+2] != 'I' || data[offset+3] != 'L') {
                continue;
            }

            // استخراج اسم الملف من $STANDARD_INFORMATION
            // سيكون أكثر تعقيدًا في الإصدار التالي

            entries.push_back({
                .name = "<NTFS_Entry>",
                .size = 0,
                .creationTime = "unknown",
                .modificationTime = "unknown",
                .deleted = false
            });
        }

        return entries;
    }

private:
    // أدوات مساعدة للقراءة
    static uint16_t readUint16LE(const std::vector<uint8_t>& data, size_t offset) {
        return (static_cast<uint16_t>(data[offset + 1]) << 8) | data[offset];
    }

    static uint32_t readUint32LE(const std::vector<uint8_t>& data, size_t offset) {
        return (static_cast<uint32_t>(data[offset + 3]) << 24) |
               (static_cast<uint32_t>(data[offset + 2]) << 16) |
               (static_cast<uint32_t>(data[offset + 1]) << 8)  |
               static_cast<uint32_t>(data[offset]);
    }

    static uint64_t readUint64LE(const std::vector<uint8_t>& data, size_t offset) {
        return (static_cast<uint64_t>(data[offset + 7]) << 56) |
               (static_cast<uint64_t>(data[offset + 6]) << 48) |
               (static_cast<uint64_t>(data[offset + 5]) << 40) |
               (static_cast<uint64_t>(data[offset + 4]) << 32) |
               (static_cast<uint64_t>(data[offset + 3]) << 24) |
               (static_cast<uint64_t>(data[offset + 2]) << 16) |
               (static_cast<uint64_t>(data[offset + 1]) << 8)  |
               static_cast<uint64_t>(data[offset]);
    }
};