/*  This file is part of Winyl Player source code.
    Copyright (C) 2008-2018, Alex Kras. <winylplayer@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include "Threading.h"

// Thread-safe singleton logger for BASS/ASIO audio events.
//
// Usage from any thread:
//   AudioLog::PostInfo (L"message");
//   AudioLog::PostWarn (L"message");
//   AudioLog::PostError(L"message");
//   AudioLog::Instance().Post(AudioLog::Level::Error, L"message"); // same
//
// SetNotifyWnd() must be called from the main thread once at startup.
// FlushPending() must be called from the main thread on each UWM_AUDIOLOG.

class AudioLog
{
public:
    enum class Level { Info, Warn, Error };

    static AudioLog& Instance();

    // Register the main window; UWM_AUDIOLOG is posted there on each new entry.
    void SetNotifyWnd(HWND hwnd);

    // Post a log entry from any thread.
    void Post(Level level, std::wstring msg);

    // Drain all pending entries; call only from the main thread.
    [[nodiscard]] std::vector<std::wstring> FlushPending();

    // Convenience wrappers so call sites don't need Instance() every time.
    static void PostInfo (std::wstring msg) { Instance().Post(Level::Info,  std::move(msg)); }
    static void PostWarn (std::wstring msg) { Instance().Post(Level::Warn,  std::move(msg)); }
    static void PostError(std::wstring msg) { Instance().Post(Level::Error, std::move(msg)); }

    // BASS error code -> constant name string (e.g. BASS_ERROR_TIMEOUT).
    // Works for both BASS and BASS_ASIO codes (they share the same values).
    static const wchar_t* BassErrorStr(int code);

    AudioLog(const AudioLog&)            = delete;
    AudioLog& operator=(const AudioLog&) = delete;

private:
    AudioLog() = default;

    HWND                     notifyWnd_ = nullptr;
    Threading::Mutex         mutex_;
    std::vector<std::wstring> pending_;

    static std::wstring FormatEntry(Level level, const std::wstring& msg);
};
