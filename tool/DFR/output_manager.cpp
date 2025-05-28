#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace fs = std::filesystem;

class OutputManager {
public:
    // هيكل لتخزين معلومات الملف المستعاد
    struct RecoveredFileInfo {
        std::string filename;
        std::string extension;
        size_t fileSize;
        std::string recoveryTime;
        std::string path;
    };

    // تصنيفات الملفات
    enum class FileCategory {
        IMAGE,
        DOCUMENT,
        AUDIO,
        VIDEO,
        ARCHIVE,
        UNKNOWN
    };

    // إعداد مسار الإخراج الرئيسي
    explicit OutputManager(const std::string& baseOutputPath)
        : baseOutputDir(baseOutputPath), logStream(nullptr) {}

    // إعداد وتوليد المجلدات الفرعية
    bool setupDirectories() {
        try {
            if (!fs::exists(baseOutputDir)) {
                if (!fs::create_directories(baseOutputDir)) {
                    std::cerr << "[!] Failed to create base output directory: " << baseOutputDir << std::endl;
                    return false;
                }
            }

            for (const auto& [category, folder] : categoryFolders) {
                fs::path fullPath = baseOutputDir / folder;
                if (!fs::exists(fullPath)) {
                    fs::create_directories(fullPath);
                }
            }

            // إنشاء ملف Log
            logFilePath = baseOutputDir / "recovery_log.txt";
            logStream = new std::ofstream(logFilePath);
            if (!logStream->is_open()) {
                std::cerr << "[!] Failed to open log file." << std::endl;
                return false;
            }

            writeLogHeader();

        } catch (const fs::filesystem_error& e) {
            std::cerr << "[!] Filesystem error: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    // إضافة ملف إلى التقارير وإدارته في المجلد الصحيح
    void addRecoveredFile(const std::string& originalFilename, const std::string& extension, size_t fileSize) {
        FileCategory category = classifyFileByExtension(extension);

        fs::path targetDir = baseOutputDir / getCategoryFolder(category);
        fs::path filePath = targetDir / originalFilename;

        // تحديث معلومات الملف
        RecoveredFileInfo info;
        info.filename = originalFilename;
        info.extension = extension;
        info.fileSize = fileSize;
        info.path = filePath.string();
        info.recoveryTime = getCurrentTimestamp();

        recoveredFiles.push_back(info);

        *logStream << "[RECOVERED] "
                   << info.filename << " | "
                   << info.extension << " | "
                   << formatFileSize(info.fileSize) << " | "
                   << info.recoveryTime << " | "
                   << info.path << "\n";

        logStream->flush();
    }

    // كتابة رأس التقرير
    void writeLogHeader() {
        time_t now = time(0);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

        *logStream << "=== FILE RECOVERY REPORT ===\n"
                   << "Generated at: " << buffer << "\n"
                   << "Base Directory: " << baseOutputDir << "\n"
                   << "-------------------------------\n"
                   << "FILENAME | EXT | SIZE | TIME | PATH\n";
    }

    // عرض تقرير مختصر عن الملفات المستعادة
    void printRecoverySummary() const {
        std::map<FileCategory, int> categoryCount;
        size_t totalSize = 0;

        for (const auto& file : recoveredFiles) {
            FileCategory cat = classifyFileByExtension(file.extension);
            categoryCount[cat]++;
            totalSize += file.fileSize;
        }

        std::cout << "\n[+] Recovery Summary:\n";
        std::cout << " - Total files recovered: " << recoveredFiles.size() << "\n";
        std::cout << " - Total data recovered: " << formatFileSize(totalSize) << "\n";
        std::cout << " - Categories:\n";

        for (const auto& [cat, count] : categoryCount) {
            std::cout << "   - " << getCategoryName(cat) << ": " << count << "\n";
        }

        std::cout << " - Log saved to: " << logFilePath.string() << "\n";
    }

private:
    std::string baseOutputDir;
    std::string logFilePath;
    std::ofstream* logStream;
    std::vector<RecoveredFileInfo> recoveredFiles;

    // تصنيفات المجلدات
    const std::map<FileCategory, std::string> categoryFolders = {
        {FileCategory::IMAGE, "images"},
        {FileCategory::DOCUMENT, "documents"},
        {FileCategory::AUDIO, "audio"},
        {FileCategory::VIDEO, "videos"},
        {FileCategory::ARCHIVE, "archives"},
        {FileCategory::UNKNOWN, "others"}
    };

    // تصنيف الملف حسب الامتداد
    FileCategory classifyFileByExtension(const std::string& ext) const {
        static const std::map<std::string, FileCategory> extensionMap = {
            {"jpg", FileCategory::IMAGE}, {"jpeg", FileCategory::IMAGE},
            {"png", FileCategory::IMAGE}, {"gif", FileCategory::IMAGE},
            {"bmp", FileCategory::IMAGE}, {"ico", FileCategory::IMAGE},

            {"pdf", FileCategory::DOCUMENT}, {"docx", FileCategory::DOCUMENT},
            {"xlsx", FileCategory::DOCUMENT}, {"pptx", FileCategory::DOCUMENT},

            {"mp3", FileCategory::AUDIO}, {"wav", FileCategory::AUDIO},
            {"ogg", FileCategory::AUDIO}, {"flac", FileCategory::AUDIO},

            {"mp4", FileCategory::VIDEO}, {"avi", FileCategory::VIDEO},
            {"mkv", FileCategory::VIDEO}, {"mov", FileCategory::VIDEO},

            {"zip", FileCategory::ARCHIVE}, {"rar", FileCategory::ARCHIVE},
            {"gz", FileCategory::ARCHIVE}, {"tar", FileCategory::ARCHIVE}
        };

        auto it = extensionMap.find(ext);
        return (it != extensionMap.end()) ? it->second : FileCategory::UNKNOWN;
    }

    // الحصول على اسم المجلد بناءً على التصنيف
    std::string getCategoryFolder(FileCategory cat) const {
        for (const auto& [c, folder] : categoryFolders) {
            if (c == cat) return folder;
        }
        return "others";
    }

    // اسم التصنيف باللغة الإنجليزية
    std::string getCategoryName(FileCategory cat) const {
        switch (cat) {
            case FileCategory::IMAGE: return "Images";
            case FileCategory::DOCUMENT: return "Documents";
            case FileCategory::AUDIO: return "Audio";
            case FileCategory::VIDEO: return "Video";
            case FileCategory::ARCHIVE: return "Archives";
            default: return "Unknown";
        }
    }

    // وقت الاسترجاع الحالي
    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // تحويل الحجم إلى صيغة قابلة للقراءة
    std::string formatFileSize(size_t bytes) const {
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
};