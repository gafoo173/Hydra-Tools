#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

class Utils {
public:
    // تحويل بيانات ثنائية إلى هيكسي
    static std::string toHex(const std::vector<uint8_t>& data, size_t limit = 64) {
        std::stringstream ss;
        for (size_t i = 0; i < std::min(data.size(), limit); ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') 
               << static_cast<int>(data[i]) << " ";
        }
        return ss.str();
    }

    // طباعة Hex Dump كامل أو جزئي
    static void hexDump(const std::vector<uint8_t>& data, size_t limit = 128) {
        for (size_t i = 0; i < std::min(data.size(), limit); ++i) {
            printf("%02X ", data[i]);
            if ((i + 1) % 16 == 0) std::cout << std::endl;
            else if ((i + 1) % 8 == 0) std::cout << "  ";
        }
        std::cout << std::endl;
    }

    // قراءة ملف كبيانات ثنائية
    static std::vector<uint8_t> readFileBinary(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "[!] Failed to open file: " << path << std::endl;
            return {};
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            std::cerr << "[!] Failed to read file: " << path << std::endl;
            return {};
        }

        return buffer;
    }

    // كتابة بيانات ثنائية إلى ملف
    static bool writeFileBinary(const std::string& path, const std::vector<uint8_t>& data) {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            std::cerr << "[!] Failed to create file: " << path << std::endl;
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return true;
    }

    // إنشاء مجلد إن لم يكن موجودًا
    static bool createDirectoryIfNotExists(const std::string& path) {
        try {
            if (!fs::exists(path)) {
                if (!fs::create_directories(path)) {
                    std::cerr << "[!] Failed to create directory: " << path << std::endl;
                    return false;
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[!] Filesystem error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    // التحقق مما إذا كان الملف موجودًا
    static bool fileExists(const std::string& path) {
        return fs::exists(path);
    }

    // الحصول على اسم الملف من المسار
    static std::string getFileNameFromPath(const std::string& path) {
        fs::path p(path);
        return p.filename().string();
    }

    // تنظيف المسار (إزالة المسافات الزائدة، التنقلات..)
    static std::string sanitizePath(const std::string& path) {
        std::string result = path;
        std::replace(result.begin(), result.end(), '\\', '/');
        size_t pos = 0;
        while ((pos = result.find("//", pos)) != std::string::npos) {
            result.replace(pos, 2, "/");
        }
        return result;
    }

    // فحص ما إذا كان القرص موجودًا (Linux فقط)
    static bool isBlockDevice(const std::string& path) {
        #ifdef _WIN32
            return true; // في ويندوز نعتمد على المحاولة
        #else
            struct stat sb;
            return stat(path.c_str(), &sb) == 0 && S_ISBLK(sb.st_mode);
        #endif
    }

    // تحويل حجم بالبايت إلى KB/MB/GB
    static std::string formatFileSize(uint64_t bytes) {
        const std::string units[] = {"B", "KB", "MB", "GB"};
        int i = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024 && i < 3) {
            size /= 1024;
            ++i;
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[i];
        return oss.str();
    }

    // تقليم المسافات من بداية ونهاية السلسلة
    static std::string trim(const std::string& s) {
        if (s.empty()) return s;
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }

    // هل السلسلة تبدأ بنص معين؟
    static bool startsWith(const std::string& str, const std::string& prefix) {
        return str.rfind(prefix, 0) == 0;
    }

    // هل السلسلة تنتهي بنص معين؟
    static bool endsWith(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
};