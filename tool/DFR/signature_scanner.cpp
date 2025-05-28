#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <algorithm>
#include <cstring>

class SignatureScanner {
public:
    // هيكل يحتوي على معلومات التوقيع
    struct FileSignature {
        std::vector<uint8_t> magic;
        std::string extension;
        bool hasEndSignature;
        std::vector<uint8_t> endMagic;
    };

    // قائمة التوقيعات المعروفة
    static const std::vector<FileSignature>& getKnownSignatures() {
        static std::vector<FileSignature> signatures = {
            // صور
            {{0xFF, 0xD8, 0xFF}, "jpg", true, {0xFF, 0xD9}},
            {{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, "png", true, {0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82}},
            {{0x47, 0x49, 0x46, 0x38}, "gif"},
            {{0x42, 0x4D}, "bmp"},
            {{0x00, 0x00, 0x01, 0x00}, "ico"},

            // مستندات
            {{0x25, 0x50, 0x44, 0x46}, "pdf"},
            {{0x50, 0x4B, 0x03, 0x04}, "docx", true, {0x50, 0x4B, 0x05, 0x06}}, // ZIP-based files
            {{0x50, 0x4B, 0x03, 0x04}, "xlsx", true, {0x50, 0x4B, 0x05, 0x06}},
            {{0x50, 0x4B, 0x03, 0x04}, "pptx", true, {0x50, 0x4B, 0x05, 0x06}},

            // فيديو
            {{0x00, 0x00, 0x00, 0x18}, "mp4", true, {0x66, 0x72, 0x65, 0x65}}, // Begins with ftyp ends with free
            {{0x52, 0x49, 0x46, 0x46}, "avi"},

            // صوت
            {{0xFF, 0xFB}, "mp3"},
            {{0x52, 0x49, 0x46, 0x46}, "wav", true, {0x57, 0x41, 0x56, 0x45}},

            // Archives
            {{0x1F, 0x8B, 0x08}, "gz"},
            {{0x50, 0x4B, 0x03, 0x04}, "zip", true, {0x50, 0x4B, 0x05, 0x06}}
        };
        return signatures;
    }

    // البحث عن التوقيعات في البيانات
    static std::vector<std::pair<size_t, FileSignature>> scan(const std::vector<uint8_t>& data) {
        std::vector<std::pair<size_t, FileSignature>> results;

        for (const auto& sig : getKnownSignatures()) {
            size_t pos = 0;
            while ((pos = findSubVector(data, sig.magic, pos)) != std::string::npos) {
                results.emplace_back(pos, sig);
                pos += 1; // الاستمرار بعد الموقع المكتشف
            }
        }

        return results;
    }

    // البحث عن نهاية الملف إن وُجد توقيع نهاية
    static size_t findEndOfSignature(const std::vector<uint8_t>& data, const FileSignature& signature, size_t startOffset, size_t maxSearchSize = 1024 * 1024) {
        if (!signature.hasEndSignature) return std::string::npos;

        size_t searchLimit = std::min(data.size(), startOffset + maxSearchSize);
        for (size_t i = startOffset; i < searchLimit - signature.endMagic.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < signature.endMagic.size(); ++j) {
                if (data[i + j] != signature.endMagic[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return i + signature.endMagic.size();
            }
        }

        return std::string::npos;
    }

private:
    // البحث عن تسلسل بايتات في مصفوفة أخرى
    static size_t findSubVector(const std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern, size_t startPos) {
        if (pattern.empty() || data.size() < pattern.size() || startPos > data.size() - pattern.size()) {
            return std::string::npos;
        }

        for (size_t i = startPos; i <= data.size() - pattern.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); ++j) {
                if (data[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return i;
            }
        }

        return std::string::npos;
    }
};