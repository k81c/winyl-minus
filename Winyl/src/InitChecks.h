#pragma once

#include <string>

bool CheckLanguageFiles(const std::wstring& programPath, const std::wstring& language);
void FatalMissingFiles(const std::wstring& message);