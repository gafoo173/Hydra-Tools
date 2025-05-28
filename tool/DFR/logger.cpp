#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>

// مستويات التسجيل
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    // Singleton Instance
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // تحديد مستوى التسجيل الحالي
    void setLevel(LogLevel level) {
        currentLevel = level;
    }

    // تعيين مسار ملف السجل
    bool setLogFile(const std::string& path) {
        if (logFile.is_open()) {
            logFile.close();
        }

        logFile.open(path, std::ios::out | std::ios::trunc);
        if (!logFile.is_open()) {
            std::cerr << "[Logger] Failed to open log file: " << path << std::endl;
            return false;
        }

        logToFileEnabled = true;
        log("Logger", "Log file opened successfully at " + path, LogLevel::INFO);
        return true;
    }

    // تسجيل رسالة
    void log(const std::string& tag, const std::string& message, LogLevel level = LogLevel::INFO) {
        std::lock_guard<std::mutex> lock(mutex); // Thread-safe

        if (level < currentLevel) return;

        std::string levelStr = toString(level);
        std::string timestamp = getCurrentTimestamp();

        std::ostringstream oss;
        oss << "[" << timestamp << "] [" << levelStr << "] [" << tag << "] " << message;

        std::string logMessage = oss.str();

        // طباعة على واجهة CLI
        printToConsole(level, logMessage);

        // كتابة إلى ملف إن كان مفعلًا
        if (logToFileEnabled && logFile.is_open()) {
            logFile << logMessage << std::endl;
            logFile.flush();
        }
    }

    // طباعة رسالة بدون وقت أو علامات (للإخراج المباشر)
    void rawOutput(const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << text;
        if (logToFileEnabled && logFile.is_open()) {
            logFile << text;
            logFile.flush();
        }
    }

private:
    LogLevel currentLevel = LogLevel::INFO;
    std::ofstream logFile;
    bool logToFileEnabled = false;
    std::mutex mutex;

    // بناء كائن غير متاح للإنشاء خارج singleton
    Logger() = default;
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    // لا يمكن نسخه
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // تحويل مستوى التسجيل إلى سلسلة نصية
    std::string toString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO ";
            case LogLevel::WARNING: return "WARN ";
            case LogLevel::ERROR:   return "ERROR";
            default:                return "UNKNOWN";
        }
    }

    // طباعة باللون المناسب
    void printToConsole(const LogLevel level, const std::string& message) {
        #ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            WORD color;

            switch (level) {
                case LogLevel::DEBUG:   color = 8; break;     // رمادي فاتح
                case LogLevel::INFO:    color = 7; break;     // أبيض
                case LogLevel::WARNING: color = 14; break;    // أصفر
                case LogLevel::ERROR:   color = 12; break;    // أحمر
                default:                color = 7; break;
            }

            SetConsoleTextAttribute(hConsole, color);
            std::cout << message << std::endl;
            SetConsoleTextAttribute(hConsole, 7); // إعادة اللون الافتراضي

        #else
            const char* colorCode;

            switch (level) {
                case LogLevel::DEBUG:   colorCode = "\033[1;30m"; break;  // رمادي فاتح
                case LogLevel::INFO:    colorCode = "\033[0;37m"; break;  // أبيض
                case LogLevel::WARNING: colorCode = "\033[1;33m"; break;  // أصفر
                case LogLevel::ERROR:   colorCode = "\033[1;31m"; break;  // أحمر
                default:                colorCode = "\033[0m"; break;
            }

            std::cout << colorCode << message << "\033[0m" << std::endl;
        #endif
    }

    // الحصول على الوقت الحالي بصيغة قابلة للقراءة
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }
};