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
#include "AudioLogWnd.h"

#include <commctrl.h>   // SetWindowSubclass / RemoveWindowSubclass
#include <cassert>

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
AudioLogWnd::~AudioLogWnd()
{
    if (hFont_)
    {
        ::DeleteObject(hFont_);
        hFont_ = nullptr;
    }
    // hwnd_ is owned by Windows; it is destroyed when the parent is destroyed
    // (or when the user closes it via the X button, which we intercept below).
}

// ---------------------------------------------------------------------------
// Create
// ---------------------------------------------------------------------------
bool AudioLogWnd::Create(HINSTANCE hInst, HWND /*parent*/)
{
    // Register the window class once.
    {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = 0;
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = hInst;
        wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = kClassName;
        // RegisterClassEx silently fails if the class already exists – that is fine.
        ::RegisterClassExW(&wc);
    }

    hwnd_ = ::CreateWindowExW(
        WS_EX_TOOLWINDOW,          // not in taskbar / Alt+Tab
        kClassName,
        L"Audio Log",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        kInitWidth, kInitHeight,
        nullptr,                   // no parent (top-level)
        nullptr, hInst,
        this);                     // passed as lParam in WM_CREATE

    if (!hwnd_)
        return false;

    // Create the monospaced font.
    hFont_ = ::CreateFontW(
        -::MulDiv(10, ::GetDeviceCaps(::GetDC(nullptr), LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN,
        L"Consolas");

    if (!hFont_)
    {
        // Fallback to Courier New which is always present on Windows.
        hFont_ = ::CreateFontW(
            -::MulDiv(10, ::GetDeviceCaps(::GetDC(nullptr), LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN,
            L"Courier New");
    }

    if (hwndEdit_ && hFont_)
        ::SendMessageW(hwndEdit_, WM_SETFONT, (WPARAM)hFont_, TRUE);

    return true;
}

// ---------------------------------------------------------------------------
// Show / IsVisible
// ---------------------------------------------------------------------------
void AudioLogWnd::Show(bool show)
{
    if (!hwnd_)
        return;
    ::ShowWindow(hwnd_, show ? SW_SHOW : SW_HIDE);
    if (show)
        ::SetForegroundWindow(hwnd_);
}

bool AudioLogWnd::IsVisible() const
{
    return hwnd_ && ::IsWindowVisible(hwnd_);
}

// ---------------------------------------------------------------------------
// AppendLines
// ---------------------------------------------------------------------------
void AudioLogWnd::AppendLines(const std::vector<std::wstring>& lines)
{
    if (!hwndEdit_ || lines.empty())
        return;

    // Build the text to append: each line followed by \r\n.
    std::wstring chunk;
    chunk.reserve(lines.size() * 100);
    for (const auto& line : lines)
    {
        chunk += line;
        chunk += L"\r\n";
    }

    lineCount_ += static_cast<int>(lines.size());

    // Move caret to end, then insert text (avoids full WM_SETTEXT redraw).
    int len = ::GetWindowTextLengthW(hwndEdit_);
    ::SendMessageW(hwndEdit_, EM_SETSEL, len, len);
    ::SendMessageW(hwndEdit_, EM_REPLACESEL, FALSE, (LPARAM)chunk.c_str());

    // Scroll to the bottom.
    ::SendMessageW(hwndEdit_, EM_SCROLLCARET, 0, 0);

    if (lineCount_ > kMaxLines)
        TrimToLimit();
}

// ---------------------------------------------------------------------------
// TrimToLimit  – keep only the kTrimTo newest lines
// ---------------------------------------------------------------------------
void AudioLogWnd::TrimToLimit()
{
    // Read the full text.
    int len = ::GetWindowTextLengthW(hwndEdit_);
    if (len <= 0)
        return;

    std::wstring text(static_cast<std::size_t>(len + 1), L'\0');
    ::GetWindowTextW(hwndEdit_, text.data(), len + 1);
    text.resize(static_cast<std::size_t>(len));

    // Count lines from the start and find the byte offset of the (lineCount_ - kTrimTo)'th newline.
    int linesToDrop = lineCount_ - kTrimTo;
    int dropped = 0;
    std::size_t pos = 0;
    while (dropped < linesToDrop && pos < text.size())
    {
        std::size_t nl = text.find(L'\n', pos);
        if (nl == std::wstring::npos)
            break;
        pos = nl + 1;
        ++dropped;
    }

    if (dropped == 0)
        return;

    // Remove lines [0, dropped) by selecting and deleting them.
    ::SendMessageW(hwndEdit_, EM_SETSEL, 0, static_cast<LPARAM>(pos));
    ::SendMessageW(hwndEdit_, EM_REPLACESEL, FALSE, (LPARAM)L"");

    lineCount_ -= dropped;
}

// ---------------------------------------------------------------------------
// WndProc
// ---------------------------------------------------------------------------
LRESULT CALLBACK AudioLogWnd::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    AudioLogWnd* self = nullptr;

    if (msg == WM_CREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<AudioLogWnd*>(cs->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));

        // Create the child EDIT control.
        self->hwndEdit_ = ::CreateWindowExW(
            0, L"EDIT", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
            0, 0, 0, 0,
            hwnd, reinterpret_cast<HMENU>(1),
            cs->hInstance, nullptr);

        if (self->hwndEdit_)
        {
            // Allow up to ~4 MB of text in the EDIT control.
            ::SendMessageW(self->hwndEdit_, EM_LIMITTEXT, 4 * 1024 * 1024, 0);

            // Subclass to add Ctrl+A support.
            ::SetWindowSubclass(self->hwndEdit_, EditSubclass,
                                kEditSubclassId,
                                reinterpret_cast<DWORD_PTR>(self));
        }
        return 0;
    }

    self = reinterpret_cast<AudioLogWnd*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_SIZE:
        if (self && self->hwndEdit_)
        {
            int w = LOWORD(lp);
            int h = HIWORD(lp);
            ::MoveWindow(self->hwndEdit_, 0, 0, w, h, TRUE);
        }
        return 0;

    case WM_SETFOCUS:
        if (self && self->hwndEdit_)
            ::SetFocus(self->hwndEdit_);
        return 0;

    case WM_CLOSE:
        // Hide instead of destroying so we can re-show later.
        ::ShowWindow(hwnd, SW_HIDE);
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
// EditSubclass  – adds Ctrl+A (select all) to the read-only EDIT
// ---------------------------------------------------------------------------
LRESULT CALLBACK AudioLogWnd::EditSubclass(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    if (msg == WM_KEYDOWN && wp == 'A' && (::GetKeyState(VK_CONTROL) & 0x8000))
    {
        ::SendMessageW(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    // WM_NCDESTROY: clean up the subclass.
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, EditSubclass, uIdSubclass);

    return ::DefSubclassProc(hwnd, msg, wp, lp);
}
