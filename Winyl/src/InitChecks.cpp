#include "stdafx.h"
#include "InitChecks.h"
#include <filesystem>
#include <sstream>
#include <Windows.h>

namespace fs = std::filesystem;

	static std::wstring BuildLanguageMainPath(const std::wstring& programPath, const std::wstring& language)
{
    fs::path p(programPath);
    p /= L"Language";
    p /= language;
    p /= L"Main.xml";
    return p.wstring();
}

	bool CheckLanguageFiles(const std::wstring& programPath, const std::wstring& language)
{
    std::wstring mainFile = BuildLanguageMainPath(programPath, language);

    if (fs::exists(mainFile))
        return true;

    std::wstringstream ss;
    ss << L"必要な言語ファイルが見つかりません:\n" << mainFile << L"\n\n"
        << L"プログラムを終了します。";
    MessageBoxW(NULL, ss.str().c_str(), L"Winyl - 言語ファイルエラー", MB_OK | MB_ICONERROR);

    return false;
}

	void FatalMissingFiles(const std::wstring& message)
{
    MessageBoxW(NULL, message.c_str(), L"Winyl - 致命的エラー", MB_OK | MB_ICONERROR);
    ::ExitProcess(1);
}

