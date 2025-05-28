#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

class FileRebuilder {
public:
    struct RecoveredFile {
        size_t startOffset;
        size_t endOffset;
        std::string extension;
        std::string filename;
    };

    // إنشاء مجلد الإخراج إذا لم يكن موجودًا
    static bool createOutputDirectory(const std::string& path) {
        try {
            if (!fs::exists(path)) {
                if (!fs::create_directories(path)) {
                    std::cerr << "[!] Failed to create output directory: " << path << std::endl;
                    return false;
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[!] Filesystem error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    // إعادة بناء ملف واحد
    static RecoveredFile rebuildFile(
        const std::vector<uint8_t>& data,
        size_t startOffset,
        const SignatureScanner::FileSignature& signature,
        const std::string& outputDir,
        size_t maxFileSize = 10 * 1024 * 1024) {

        size_t endOffset = startOffset + signature.magic.size();

        // البحث عن نهاية الملف إن كان له توقيع نهاية
        if (signature.hasEndSignature) {
            endOffset = SignatureScanner::findEndOfSignature(data, signature, startOffset, maxFileSize);
            if (endOffset == std::string::npos) {
                endOffset = startOffset + maxFileSize; // حد افتراضي
            }
        } else {
            // بعض الملفات مثل PDF أو ZIP يمكن حساب حجمها من الرأس
            size_t calculatedSize = calculateFileSizeFromHeader(data, startOffset);
            if (calculatedSize > 0) {
                endOffset = startOffset + calculatedSize;
            } else {
                endOffset = startOffset + maxFileSize; // حد افتراضي
            }
        }

        // ضمان أن النهاية لا تتجاوز البيانات
        endOffset = std::min(endOffset, data.size());

        // استخراج البيانات
        std::vector<uint8_t> fileData(data.begin() + startOffset, data.begin() + endOffset);

        // توليد اسم ملف
        std::string filename = generateUniqueFilename(signature.extension);

        // حفظ الملف
        std::string outputPath = outputDir + "/" + filename;
        saveToFile(fileData, outputPath);

        return {startOffset, endOffset, signature.extension, filename};
    }

    // حفظ البيانات إلى ملف ثنائي
    static bool saveToFile(const std::vector<uint8_t>& data, const std::string& outputPath) {
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            std::cerr << "[!] Failed to create output file: " << outputPath << std::endl;
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
        outFile.close();
        std::cout << "[+] Saved recovered file: " << outputPath << std::endl;
        return true;
    }

private:
    // توليد اسم ملف فريد
    static std::string generateUniqueFilename(const std::string& ext) {
        static int counter = 0;
        std::ostringstream oss;
        oss << "recovered_" << std::setw(5) << std::setfill('0') << ++counter << "." << ext;
        return oss.str();
    }

    // حساب الحجم من رأس الملف (مثل PDF)
    static size_t calculateFileSizeFromHeader(const std::vector<uint8_t>& data, size_t offset) {
        if (offset + 32 > data.size()) return 0;

        // مثال على PDF: يحتوي على "%PDF-X.Y"
        const uint8_t* ptr = data.data() + offset;
        if (ptr[0] == 0x25 && ptr[1] == 0x50 && ptr[2] == 0x44 && ptr[3] == 0x46) { // %PDF
            // يمكنك هنا قراءة طول الملف من رأس الملف إن أمكن
            return 1024 * 1024; // مثال افتراضي
        }

        // يمكنك إضافة المزيد من الحالات لـ DOCX/XLSX/PPTX (ZIP-based)
        if (ptr[0] == 0x50 && ptr[1] == 0x4B && ptr[2] == 0x03 && ptr[3] == 0x04) { // ZIP/DOCX/XLSX
            return 1024 * 1024 * 5; // 5MB كحد افتراضي
        }

        return 0;
    }
};