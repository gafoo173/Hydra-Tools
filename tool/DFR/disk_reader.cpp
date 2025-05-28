#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <cstring>

// تحديد نظام التشغيل
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
#endif

class DiskReader {
public:
    // هيكل لتخزين معلومات القرص
    struct DiskInfo {
        uint64_t totalSize;
        size_t sectorSize;
        std::string devicePath;
    };

    // نوع بيانات لتخزين البيانات الثنائية
    using RawData = std::vector<uint8_t>;

private:
    DiskInfo diskInfo;

public:
    // البناء باستخدام مسار القرص
    explicit DiskReader(const std::string& devicePath, size_t sectorSize = 512)
        : diskInfo({0, sectorSize, devicePath}) {}

    // الحصول على معلومات القرص
    const DiskInfo& getDiskInfo() const {
        return diskInfo;
    }

    // قراءة قطاع واحد
    RawData readSector(uint64_t sectorNumber) {
        return readBytes(sectorNumber * diskInfo.sectorSize, diskInfo.sectorSize);
    }

    // قراءة عدة قطاعات
    RawData readSectors(uint64_t startSector, size_t count) {
        return readBytes(startSector * diskInfo.sectorSize, count * diskInfo.sectorSize);
    }

    // قراءة بيانات من offset معين
    RawData readBytes(uint64_t offset, size_t size) {
        RawData buffer(size);

        #ifdef _WIN32
            HANDLE hDevice = CreateFile(
                diskInfo.devicePath.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (hDevice == INVALID_HANDLE_VALUE) {
                throw std::runtime_error("Failed to open disk. Error code: " + std::to_string(GetLastError()));
            }

            LARGE_INTEGER li;
            li.QuadPart = static_cast<LONGLONG>(offset);
            if (!SetFilePointerEx(hDevice, li, NULL, FILE_BEGIN)) {
                CloseHandle(hDevice);
                throw std::runtime_error("Failed to set file pointer.");
            }

            DWORD bytesRead;
            BOOL result = ReadFile(hDevice, buffer.data(), static_cast<DWORD>(size), &bytesRead, nullptr);
            CloseHandle(hDevice);

            if (!result || bytesRead != size) {
                throw std::runtime_error("Failed to read from disk. Bytes read: " + std::to_string(bytesRead));
            }

        #else
            int fd = open(diskInfo.devicePath.c_str(), O_RDONLY);
            if (fd == -1) {
                throw std::runtime_error("Failed to open device: " + std::string(strerror(errno)));
            }

            off_t seekResult = lseek(fd, static_cast<off_t>(offset), SEEK_SET);
            if (seekResult == -1) {
                close(fd);
                throw std::runtime_error("Failed to seek in device.");
            }

            ssize_t bytesRead = read(fd, buffer.data(), size);
            close(fd);

            if (bytesRead != static_cast<ssize_t>(size)) {
                throw std::runtime_error("Failed to read from device. Bytes read: " + std::to_string(bytesRead));
            }
        #endif

        return buffer;
    }

    // حساب حجم القرص (متقدم - يعتمد على النظام)
    bool detectDiskSize() {
        #ifdef _WIN32
            HANDLE hDevice = CreateFile(
                diskInfo.devicePath.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (hDevice == INVALID_HANDLE_VALUE) {
                return false;
            }

            DISK_GEOMETRY geometry;
            DWORD bytesReturned;
            BOOL result = DeviceIoControl(
                hDevice,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                nullptr, 0,
                &geometry, sizeof(geometry),
                &bytesReturned,
                nullptr);

            if (!result) {
                CloseHandle(hDevice);
                return false;
            }

            diskInfo.totalSize = geometry.Cylinders.QuadPart *
                                 geometry.TracksPerCylinder *
                                 geometry.SectorsPerTrack *
                                 geometry.BytesPerSector;

            diskInfo.sectorSize = geometry.BytesPerSector;
            CloseHandle(hDevice);
            return true;

        #else
            struct stat st;
            if (stat(diskInfo.devicePath.c_str(), &st) != 0) {
                return false;
            }

            // في بعض الأنظمة، يمكن استخدام BLKGETSIZE64 للحصول على الحجم
            int fd = open(diskInfo.devicePath.c_str(), O_RDONLY);
            if (fd == -1) return false;

            uint64_t size = 0;
            if (ioctl(fd, BLKGETSIZE64, &size) == 0) {
                diskInfo.totalSize = size;
            } else {
                close(fd);
                return false;
            }

            close(fd);
            return true;
        #endif
        return false;
    }

    // طباعة بداية البيانات بالشكل الهكسى
    static void hexDump(const RawData& data, size_t limit = 256) {
        for (size_t i = 0; i < std::min(data.size(), limit); ++i) {
            printf("%02X ", data[i]);
            if ((i + 1) % 16 == 0) std::cout << std::endl;
            else if ((i + 1) % 8 == 0) std::cout << "  ";
        }
        std::cout << std::endl;
    }

    // حفظ البيانات إلى ملف ثنائي
    static bool saveToFile(const RawData& data, const std::string& filename) {
        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile) {
            std::cerr << "[!] Failed to create output file: " << filename << std::endl;
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
        outFile.close();
        std::cout << "[+] Data saved to: " << filename << std::endl;
        return true;
    }
};