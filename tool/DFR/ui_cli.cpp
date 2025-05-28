#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>

class CliUI {
public:
    // عرض عنوان الأداة
    void showBanner() const {
        std::cout << R"(
  ██████╗ ██╗   ██╗██╗ ██████╗     ███████╗██╗███╗   ██╗██████╗ 
  ██╔══██╗██║   ██║██║ ██╔══██╗    ██╔════╝██║████╗  ██║██╔══██╗
  ██████╔╝██║   ██║██║ ██████╔╝    ███████╗██║██╔██╗ ██║██║  ██║
  ██╔══██╗╚██╗ ██╔╝██║ ██╔═══╝     ╚════██║██║██║╚██╗██║██║  ██║
  ██████╔╝ ╚████╔╝ ██║ ██║         ███████║██║██║ ╚████║██████╔╝
  ╚═════╝   ╚═══╝  ╚═╝ ╚═╝         ╚══════╝╚═╝╚═╝  ╚═══╝╚═════╝ 
)" << '\n';
        std::cout << "          File Recovery Tool - Advanced CLI Interface\n";
        std::cout << "--------------------------------------------------------\n\n";
    }

    // عرض القائمة الرئيسية
    void showMainMenu() const {
        std::cout << "[Main Menu]\n";
        std::cout << "  [1] Start Disk Scan\n";
        std::cout << "  [2] View Recovered Files\n";
        std::cout << "  [3] Settings\n";
        std::cout << "  [4] Exit\n";
        std::cout << "\nEnter your choice: ";
    }

    // طلب مسار القرص من المستخدم
    std::string getDiskPathInput() const {
        std::string path;
        std::cout << "\nEnter disk path (e.g., /dev/sda or \\\\.\\PhysicalDrive0): ";
        std::cin.ignore();
        std::getline(std::cin, path);
        return path;
    }

    // طلب مسار حفظ النتائج
    std::string getOutputPathInput() const {
        std::string path;
        std::cout << "Enter output directory (default: ./recovered): ";
        std::getline(std::cin, path);
        return path.empty() ? "./recovered" : path;
    }

    // عرض خيارات أنواع الملفات
    std::vector<std::string> getFileTypesSelection() const {
        std::vector<std::string> allTypes = {"jpg", "png", "pdf", "docx", "xlsx", "pptx", "mp3", "wav", "zip", "gif"};
        std::vector<std::string> selected;

        std::cout << "\nSelect file types to recover (comma-separated, e.g.: jpg,png,pdf)\n";
        std::cout << "Available types: ";
        for (const auto& t : allTypes) std::cout << t << " ";
        std::cout << "\nYour selection: ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "all") return allTypes;

        std::istringstream iss(input);
        std::string token;
        while (std::getline(iss, token, ',')) {
            token = trim(token);
            if (std::find(allTypes.begin(), allTypes.end(), token) != allTypes.end()) {
                selected.push_back(token);
            }
        }

        return selected;
    }

    // عرض شاشة الإعدادات
    void showSettingsMenu() const {
        std::cout << "\n[Settings]\n";
        std::cout << "  [1] Set custom scan size\n";
        std::cout << "  [2] Toggle debug mode\n";
        std::cout << "  [3] Back to main menu\n";
        std::cout << "\nEnter your choice: ";
    }

    // عرض شاشة التحميل أثناء المسح
    void showProgress(int percent, const std::string& status = "") const {
        int barWidth = 50;
        std::cout << "[";
        int pos = barWidth * percent / 100;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << percent << " % " << status << "\r";
        std::cout.flush();
    }

    // عرض ملخص الاستعادة
    void showRecoverySummary(size_t filesRecovered, size_t totalSize) const {
        std::cout << "\n\n[+] Recovery completed successfully!\n";
        std::cout << " - Total files recovered: " << filesRecovered << "\n";
        std::cout << " - Total data recovered: " << formatFileSize(totalSize) << "\n";
    }

    // قراءة خيار من المستخدم
    int getUserChoice() const {
        int choice;
        while (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number: ";
        }
        return choice;
    }

    // تنسيق الحجم إلى KB/MB/GB
    std::string formatFileSize(uint64_t bytes) const {
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

    // تقليم المسافات
    std::string trim(const std::string& s) const {
        if (s.empty()) return s;
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }
};