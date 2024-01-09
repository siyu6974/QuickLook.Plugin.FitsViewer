#pragma once
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <ShlObj.h>  // Include for SHGetFolderPath
#include <chrono>

const string LogFileName = "QuickFITS.log";

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

std::string stringDateTime() {
    auto now = std::chrono::system_clock::now();
    auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Convert to tm struct
    std::tm localTime;
    localtime_s(&localTime, &nowAsTimeT);

    // Format the time string with milliseconds
    std::stringstream dateTimeString;
    dateTimeString << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    dateTimeString << '.' << std::setfill('0') << std::setw(3) << nowMs.count();

    return dateTimeString.str();
}

void writeToLogFile(const std::string& message) {
#ifndef ENABLE_LOGGING
    return;
#endif
    char documentsPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documentsPath) != S_OK) {
        std::cerr << "Unable to get Documents folder path";
        return;
    }
    std::string logFilePath = std::string(documentsPath) + string_format("\\%s", LogFileName);
    std::ofstream logFile(logFilePath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Unable to open file";
        return;
    }
    logFile << "[" << stringDateTime() << "] " << message << std::endl;
    logFile.close();
}

void writeToLogFile(const std::wstring& message) {
#ifndef ENABLE_LOGGING
    return;
#endif
    wchar_t documentsPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documentsPath) != S_OK) {
        std::cerr << "Unable to get Documents folder path";
        return;
    }
    writeToLogFile(documentsPath);
    std::wstring logFilePath = std::wstring(documentsPath) + L"QuickFITS.log";

    std::locale locale(std::locale(), new std::codecvt_utf8<wchar_t>);
    std::wofstream logFile(logFilePath, std::ios::app);
    logFile.imbue(locale);

    if (!logFile.is_open()) {
        std::cerr << "Unable to open file";
        return;
    }
    logFile << L"wstring: " << message << std::endl;
    logFile.close();
}


