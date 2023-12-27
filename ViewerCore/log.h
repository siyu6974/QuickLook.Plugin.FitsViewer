#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <exception>

void writeToLogFile(const std::string& message) {
    std::ofstream logFile("viewer_core.log", std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Unable to open file";
        return;
    }
    logFile << message << std::endl;
    logFile.close();
}