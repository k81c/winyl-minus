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

// Modeless, skin-free console window that displays AudioLog entries.
//
// - Plain Win32: WS_OVERLAPPEDWINDOW + WS_EX_TOOLWINDOW (no taskbar entry).
// - Child EDIT control: multiline, read-only, auto-scroll, vertical scrollbar.
// - Font: Consolas 10pt (falls back to Courier New if unavailable).
// - Ctrl+A selects all; Ctrl+C copies selection (EDIT subclass).
// - Lines are capped at kMaxLines (oldest are discarded to avoid unbounded growth).
// - The window is created once and shown/hidden via Show(); never destroyed
//   during the session (destroyed automatically with the parent on exit).

class AudioLogWnd
{
public:
    AudioLogWnd()  = default;
    ~AudioLogWnd();

    AudioLogWnd(const AudioLogWnd&)            = delete;
    AudioLogWnd& operator=(const AudioLogWnd&) = delete;

    // Create the window as a child-less top-level window.
    // parent is used only to inherit the instance handle.
    bool Create(HINSTANCE hInst, HWND parent);

    void Show(bool show);
    bool IsVisible() const;

    // Append a batch of pre-formatted log lines to the EDIT control.
    // Must be called from the main (UI) thread.
    void AppendLines(const std::vector<std::wstring>& lines);

private:
    static constexpr int kMaxLines    = 2000;
    static constexpr int kTrimTo      = 1600; // after trim, keep this many newest lines
    static constexpr int kInitWidth   = 700;
    static constexpr int kInitHeight  = 400;

    HWND hwnd_      = nullptr;
    HWND hwndEdit_  = nullptr;
    HFONT hFont_    = nullptr;
    int   lineCount_ = 0;

    // Rebuild the EDIT text by discarding the oldest (lineCount_ - kTrimTo) lines.
    void TrimToLimit();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK EditSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                          UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    static constexpr UINT_PTR kEditSubclassId = 1;
    static constexpr wchar_t  kClassName[]    = L"WinylAudioLogWnd";
};
