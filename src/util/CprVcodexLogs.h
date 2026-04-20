#pragma once

#include <string>

namespace CprVcodexLogs {

const char* getLogDir();

void appendEvent(const char* category, const char* message);
void appendEvent(const char* category, const std::string& message);

bool writeReport(const char* prefix, const std::string& body, std::string* outPath = nullptr);

}  // namespace CprVcodexLogs
