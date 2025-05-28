#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <thread>
#include <chrono>

// تضمين ملفات المشروع
#include "disk_reader.cpp"
#include "signature_scanner.cpp"
#include "file_rebuilder.cpp"
#include "output_manager.cpp"
#include "metadata_extractor.cpp"
#include "file_system_analyzer.cpp"
#include "logger.cpp"
#include "utils.cpp"
#include "ui_cli.cpp"

// نقطة الدخول
int main() {
    // إعداد المسجل
    Logger& logger = Logger::getInstance();
    logger.setLevel(LogLevel::INFO);
    logger.setLogFile("file_rescue.log");

    CliUI ui;
    ui.showBanner();

    std::string diskPath, outputPath;
    std::vector<std::string> selectedTypes;

    while (true) {
        ui.showMainMenu();
        int choice = ui.getUserChoice();

        switch (choice) {
            case 1: { // بدء المسح والاستعادة
                diskPath = ui.getDiskPathInput();
                outputPath = ui.getOutputPathInput();
                selectedTypes = ui.getFileTypesSelection();

                if (!Utils::createDirectoryIfNotExists(outputPath)) {
                    logger.log("Main", "Failed to create output directory", LogLevel::ERROR);
                    break;
                }

                logger.log("Main", "Starting disk scan on " + diskPath, LogLevel::INFO);

                // قراءة أول 1GB من القرص
                DiskReader reader(diskPath);
                auto rawData = reader.readBytes(0, 1024 * 1024 * 1024); // 1GB
                if (rawData.empty()) {
                    logger.log("Main", "Failed to read disk data", LogLevel::ERROR);
                    break;
                }

                // مسح التوقيعات
                logger.log("Main", "Scanning for file signatures...", LogLevel::INFO);
                auto foundSignatures = SignatureScanner::scan(rawData);

                OutputManager output(outputPath);
                output.setupDirectories();

                logger.log("Main", "Rebuilding files...", LogLevel::INFO);
                for (const auto& [offset, signature] : foundSignatures) {
                    auto recoveredFile = FileRebuilder::rebuildFile(rawData, offset, signature, outputPath);
                    output.addRecoveredFile(recoveredFile.filename, signature.extension, recoveredFile.endOffset - offset);
                }

                output.printRecoverySummary();
                break;
            }

            case 2: { // عرض الملفات المستعادة
                logger.log("Main", "Displaying recovered files...", LogLevel::INFO);
                OutputManager output(outputPath);
                output.printRecoverySummary();
                break;
            }

            case 3: { // الإعدادات
                int settingChoice = 0;
                while (settingChoice != 3) {
                    ui.showSettingsMenu();
                    settingChoice = ui.getUserChoice();
                    switch (settingChoice) {
                        case 1:
                            logger.log("Main", "Custom scan size not implemented yet.", LogLevel::WARNING);
                            break;
                        case 2:
                            logger.setLevel(logger.getLevel() == LogLevel::DEBUG ? LogLevel::INFO : LogLevel::DEBUG);
                            logger.log("Main", "Debug mode toggled to " + std::to_string(static_cast<int>(logger.getLevel())), LogLevel::INFO);
                            break;
                        case 3:
                            break;
                        default:
                            logger.log("Main", "Invalid setting option", LogLevel::WARNING);
                    }
                }
                break;
            }

            case 4: // الخروج
                logger.log("Main", "Exiting application", LogLevel::INFO);
                return 0;

            default:
                logger.log("Main", "Invalid menu choice", LogLevel::WARNING);
        }
    }

    return 0;
}