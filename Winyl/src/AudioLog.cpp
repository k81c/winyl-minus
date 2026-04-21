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

#include "stdafx.h"
#include "AudioLog.h"
#include "bass/bass.h"       // BASS_ERROR_* constants
#include "bass/bassasio.h"   // BASS_ASIO error codes (same values)

// UWM_AUDIOLOG is defined in stdafx.h
// (WM_USER + 140, added alongside other UWM_ constants)

AudioLog& AudioLog::Instance()
{
    static AudioLog instance;
    return instance;
}

void AudioLog::SetNotifyWnd(HWND hwnd)
{
    mutex_.Lock();
    notifyWnd_ = hwnd;
    mutex_.Unlock();
}

void AudioLog::Post(Level level, std::wstring msg)
{
    std::wstring entry = FormatEntry(level, msg);

    mutex_.Lock();
    pending_.push_back(std::move(entry));
    HWND wnd = notifyWnd_;
    mutex_.Unlock();

    // Notify the main thread; it is fine if the message is coalesced.
    if (wnd)
        ::PostMessage(wnd, UWM_AUDIOLOG, 0, 0);
}

std::vector<std::wstring> AudioLog::FlushPending()
{
    std::vector<std::wstring> out;
    mutex_.Lock();
    out.swap(pending_);
    mutex_.Unlock();
    return out;
}

std::wstring AudioLog::FormatEntry(Level level, const std::wstring& msg)
{
    // "[HH:MM:SS] [LEVEL] message"
    SYSTEMTIME st = {};
    ::GetLocalTime(&st);

    wchar_t ts[32] = {};
    ::swprintf_s(ts, L"[%02u:%02u:%02u]", st.wHour, st.wMinute, st.wSecond);

    const wchar_t* tag = nullptr;
    switch (level)
    {
        case Level::Info:  tag = L"[INFO] "; break;
        case Level::Warn:  tag = L"[WARN] "; break;
        case Level::Error: tag = L"[ERROR]"; break;
        default:           tag = L"[?????]"; break;
    }

    std::wstring result;
    result.reserve(wcslen(ts) + 1 + wcslen(tag) + 1 + msg.size());
    result += ts;
    result += L' ';
    result += tag;
    result += L' ';
    result += msg;
    return result;
}

const wchar_t* AudioLog::BassErrorStr(int code)
{
    // Covers both BASS and BASS_ASIO error codes (they share the same values).
    switch (code)
    {
        case BASS_OK:                return L"BASS_OK";
        case BASS_ERROR_MEM:         return L"BASS_ERROR_MEM";
        case BASS_ERROR_FILEOPEN:    return L"BASS_ERROR_FILEOPEN";
        case BASS_ERROR_DRIVER:      return L"BASS_ERROR_DRIVER";
        case BASS_ERROR_BUFLOST:     return L"BASS_ERROR_BUFLOST";
        case BASS_ERROR_HANDLE:      return L"BASS_ERROR_HANDLE";
        case BASS_ERROR_FORMAT:      return L"BASS_ERROR_FORMAT";
        case BASS_ERROR_POSITION:    return L"BASS_ERROR_POSITION";
        case BASS_ERROR_INIT:        return L"BASS_ERROR_INIT";
        case BASS_ERROR_START:       return L"BASS_ERROR_START";
        case BASS_ERROR_SSL:         return L"BASS_ERROR_SSL";
        case BASS_ERROR_REINIT:      return L"BASS_ERROR_REINIT";
        case BASS_ERROR_ALREADY:     return L"BASS_ERROR_ALREADY";
        case BASS_ERROR_NOTAUDIO:    return L"BASS_ERROR_NOTAUDIO";
        case BASS_ERROR_NOCHAN:      return L"BASS_ERROR_NOCHAN";
        case BASS_ERROR_ILLTYPE:     return L"BASS_ERROR_ILLTYPE";
        case BASS_ERROR_ILLPARAM:    return L"BASS_ERROR_ILLPARAM";
        case BASS_ERROR_NO3D:        return L"BASS_ERROR_NO3D";
        case BASS_ERROR_NOEAX:       return L"BASS_ERROR_NOEAX";
        case BASS_ERROR_DEVICE:      return L"BASS_ERROR_DEVICE";
        case BASS_ERROR_NOPLAY:      return L"BASS_ERROR_NOPLAY";
        case BASS_ERROR_FREQ:        return L"BASS_ERROR_FREQ";
        case BASS_ERROR_NOTFILE:     return L"BASS_ERROR_NOTFILE";
        case BASS_ERROR_NOHW:        return L"BASS_ERROR_NOHW";
        case BASS_ERROR_EMPTY:       return L"BASS_ERROR_EMPTY";
        case BASS_ERROR_NONET:       return L"BASS_ERROR_NONET";
        case BASS_ERROR_CREATE:      return L"BASS_ERROR_CREATE";
        case BASS_ERROR_NOFX:        return L"BASS_ERROR_NOFX";
        case BASS_ERROR_NOTAVAIL:    return L"BASS_ERROR_NOTAVAIL";
        case BASS_ERROR_DECODE:      return L"BASS_ERROR_DECODE";
        case BASS_ERROR_DX:          return L"BASS_ERROR_DX";
        case BASS_ERROR_TIMEOUT:     return L"BASS_ERROR_TIMEOUT";
        case BASS_ERROR_FILEFORM:    return L"BASS_ERROR_FILEFORM";
        case BASS_ERROR_SPEAKER:     return L"BASS_ERROR_SPEAKER";
        case BASS_ERROR_VERSION:     return L"BASS_ERROR_VERSION";
        case BASS_ERROR_CODEC:       return L"BASS_ERROR_CODEC";
        case BASS_ERROR_ENDED:       return L"BASS_ERROR_ENDED";
        case BASS_ERROR_BUSY:        return L"BASS_ERROR_BUSY";
        case BASS_ERROR_UNSTREAMABLE:return L"BASS_ERROR_UNSTREAMABLE";
        case BASS_ERROR_PROTOCOL:    return L"BASS_ERROR_PROTOCOL";
        case BASS_ERROR_DENIED:      return L"BASS_ERROR_DENIED";
        case BASS_ERROR_UNKNOWN:     return L"BASS_ERROR_UNKNOWN";
        default:                     return L"BASS_ERROR_?";
    }
}
