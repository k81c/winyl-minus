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

namespace winyl {

// One stream/program within a station.
// genre is optional (used by 181.FM-style stations that sub-categorise by genre).
// url is stored as std::string because valid radio stream URLs are pure ASCII
// (Punycode-encoded international domain names remain 7-bit clean).
struct RadioProgram
{
    std::wstring genre;   // optional – empty means no genre prefix in Artist field
    std::wstring name;
    std::string  url;     // ASCII only; validated by RadioLoader::SanitizeUrl
};

// One top-level station shown as a tree node.
struct RadioStation
{
    std::wstring              name;     // displayed in the tree and as Artist prefix
    std::vector<RadioProgram> programs;
};

} // namespace winyl
