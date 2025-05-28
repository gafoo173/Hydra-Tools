#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <ctime>

class MetadataExtractor {
public:
    // هيكل لتخزين البيانات الوصفية
    struct Metadata {
        std::unordered_map<std::string, std::string> values;

        void add(const std::string& key, const std::string& value) {
            values[key] = value;
        }

        std::string get(const std::string& key, const std::string& defaultValue = "") const {
            auto it = values.find(key);
            return (it != values.end()) ? it->second : defaultValue;
        }

        void print() const {
            if (values.empty()) {
                std::cout << "No metadata found.\n";
                return;
            }

            std::cout << "[Metadata]\n";
            for (const auto& [key, value] : values) {
                std::cout << " - " << key << ": " << value << "\n";
            }
        }
    };

    // استخراج البيانات بناءً على نوع الملف
    static Metadata extract(const std::vector<uint8_t>& data, const std::string& extension) {
        Metadata meta;

        if (extension == "jpg" || extension == "jpeg") {
            extractJpegMetadata(data, meta);
        } else if (extension == "png") {
            extractPngMetadata(data, meta);
        } else if (extension == "pdf") {
            extractPdfMetadata(data, meta);
        } else if (extension == "mp3") {
            extractMp3Metadata(data, meta);
        } else if (extension == "docx" || extension == "xlsx" || extension == "pptx") {
            extractOfficeMetadata(data, meta);
        }

        return meta;
    }

private:
    // --- JPG / JPEG ---
    static void extractJpegMetadata(const std::vector<uint8_t>& data, Metadata& meta) {
        // البحث عن قسم EXIF
        size_t offset = findSubVector(data, {0xFF, 0xE1}, 0);
        if (offset != std::string::npos) {
            meta.add("Format", "JPEG");
            meta.add("Has_EXIF", "Yes");

            // يمكن توسيعه لقراءة EXIF بالكامل لاحقًا
        } else {
            meta.add("Format", "JPEG");
            meta.add("Has_EXIF", "No");
        }
    }

    // --- PNG ---
    static void extractPngMetadata(const std::vector<uint8_t>& data, Metadata& meta) {
        meta.add("Format", "PNG");

        // البحث عن كتل النص (tEXt)
        size_t pos = 8; // بداية الرأس بعد التوقيع
        while (pos + 8 < data.size()) {
            uint32_t chunkLength = readUint32BE(data, pos);
            std::string chunkType = readString(data, pos + 4, 4);

            if (chunkType == "tEXt") {
                std::string keyword = readNullTerminatedString(data, pos + 8);
                std::string text = readString(data, pos + 8 + keyword.size() + 1, chunkLength - keyword.size() - 1);
                meta.add(keyword, text);
            }

            pos += 12 + chunkLength;
        }
    }

    // --- PDF ---
    static void extractPdfMetadata(const std::vector<uint8_t>& data, Metadata& meta) {
        meta.add("Format", "PDF");

        std::string pdfVersion(reinterpret_cast<const char*>(data.data()), 8);
        meta.add("Version", pdfVersion);

        // البحث عن %%DocumentData
        std::string content(reinterpret_cast<const char*>(data.data()), std::min<size_t>(data.size(), 1024 * 64));
        size_t creatorPos = content.find("/Creator");
        if (creatorPos != std::string::npos) {
            std::string creator = extractPdfValue(content, creatorPos);
            meta.add("Creator", creator);
        }

        size_t authorPos = content.find("/Author");
        if (authorPos != std::string::npos) {
            std::string author = extractPdfValue(content, authorPos);
            meta.add("Author", author);
        }
    }

    // --- MP3 (ID3 Tags) ---
    static void extractMp3Metadata(const std::vector<uint8_t>& data, Metadata& meta) {
        meta.add("Format", "MP3");

        if (data.size() < 10 || data[0] != 'I' || data[1] != 'D' || data[2] != '3') {
            meta.add("Has_ID3", "No");
            return;
        }

        meta.add("Has_ID3", "Yes");
        meta.add("Version", std::to_string(data[3]) + "." + std::to_string(data[4]));

        if (data.size() >= 128) {
            std::string title = readString(data, data.size() - 128 + 3, 30);
            std::string artist = readString(data, data.size() - 128 + 33, 30);
            std::string album = readString(data, data.size() - 128 + 63, 30);
            std::string year = readString(data, data.size() - 128 + 93, 4);

            meta.add("Title", title);
            meta.add("Artist", artist);
            meta.add("Album", album);
            meta.add("Year", year);
        }
    }

    // --- DOCX/XLSX/PPTX ---
    static void extractOfficeMetadata(const std::vector<uint8_t>& data, Metadata& meta) {
        meta.add("Format", "ZIP-Based Document");

        // يمكنك هنا إضافة دعم لفتح XML داخلي لاستخراج بيانات أوتوبيغرافيك
        // لكن هذا يتطلب ضغط ZIP Parser (يمكن تطويره لاحقًا)
    }

    // أدوات مساعدة داخلية
    static size_t findSubVector(const std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern, size_t startPos) {
        if (pattern.empty() || data.size() < pattern.size() || startPos > data.size() - pattern.size()) {
            return std::string::npos;
        }

        for (size_t i = startPos; i <= data.size() - pattern.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < pattern.size(); ++j) {
                if (data[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return i;
        }

        return std::string::npos;
    }

    static uint32_t readUint32BE(const std::vector<uint8_t>& data, size_t offset) {
        return (static_cast<uint32_t>(data[offset]) << 24) |
               (static_cast<uint32_t>(data[offset + 1]) << 16) |
               (static_cast<uint32_t>(data[offset + 2]) << 8) |
               static_cast<uint32_t>(data[offset + 3]);
    }

    static std::string readString(const std::vector<uint8_t>& data, size_t offset, size_t length) {
        std::string result;
        for (size_t i = 0; i < length && offset + i < data.size(); ++i) {
            if (data[offset + i] == 0) break;
            result += static_cast<char>(data[offset + i]);
        }
        return result;
    }

    static std::string readNullTerminatedString(const std::vector<uint8_t>& data, size_t offset) {
        std::string result;
        for (size_t i = 0; offset + i < data.size(); ++i) {
            if (data[offset + i] == 0) break;
            result += static_cast<char>(data[offset + i]);
        }
        return result;
    }

    static std::string extractPdfValue(const std::string& content, size_t pos) {
        size_t start = content.find('(', pos);
        if (start == std::string::npos) return "";

        size_t end = content.find(')', start);
        if (end == std::string::npos) return "";

        return content.substr(start + 1, end - start - 1);
    }
};