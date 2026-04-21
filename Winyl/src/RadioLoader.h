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

#include "RadioStationData.h"
#include <optional>
#include <string>
#include <vector>

namespace winyl {

// Loads and validates radio station data from an XML file.
//
// XML schema (radio_stations.xml):
//
//   <radio-stations version="1">
//     <station name="StationName">
//       <program genre="OptionalGenre" name="ProgramName" url="https://..."/>
//       ...
//     </station>
//     ...
//   </radio-stations>
//
// Sanitization rules applied during loading:
//   - station/@name, program/@name, program/@genre:
//       control characters (< U+0020) are stripped, whitespace is trimmed,
//       maximum 256 characters; entries with an empty name after sanitisation
//       are skipped.
//   - program/@url:
//       must start with "http://" or "https://";
//       all bytes must be printable 7-bit ASCII (0x20–0x7E);
//       maximum 2048 characters.
//       Raw Unicode / non-ASCII bytes are rejected outright, so international
//       domain names must already be in Punycode form (xn--...).
//
// Returns std::nullopt only on file-not-found or fatal parse error.
// An XML file that parses successfully but contains no valid stations returns
// an empty (non-null) vector.

class RadioLoader
{
public:
    [[nodiscard]]
    static std::optional<std::vector<RadioStation>>
        Load(const std::wstring& xmlPath);

private:
    static constexpr std::size_t kMaxNameLen = 256;
    static constexpr std::size_t kMaxUrlLen  = 2048;

    // Strips control chars, trims spaces, enforces kMaxNameLen.
    // Returns false (and leaves name unchanged) if the result would be empty.
    static bool SanitizeName(std::wstring& name);

    // Validates raw UTF-8 URL attribute value from pugixml.
    // On success writes to out and returns true; otherwise returns false.
    static bool SanitizeUrl(const char* raw, std::string& out);
};

} // namespace winyl
