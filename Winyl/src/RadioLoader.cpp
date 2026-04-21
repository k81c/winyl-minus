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
#include "RadioLoader.h"
#include "XmlFile.h"
#include "XmlNode.h"
#include "AudioLog.h"

#include <algorithm>
#include <string_view>

namespace winyl {

std::optional<std::vector<RadioStation>>
RadioLoader::Load(const std::wstring& xmlPath)
{
    XmlFile xmlFile;
    if (!xmlFile.LoadFile(xmlPath))
    {
        AudioLog::PostError(std::wstring(L"RadioLoader: failed to load ") + xmlPath);
        return std::nullopt; // file missing or unreadable
    }

    XmlNode root = xmlFile.RootNode();

    // Accept both <radio-stations> at the document root child
    XmlNode rsNode = root.FirstChild("radio-stations");
    if (!rsNode)
    {
        AudioLog::PostError(L"RadioLoader: <radio-stations> root element not found");
        return std::nullopt; // unexpected document structure
    }

    std::vector<RadioStation> result;
    int skippedStations  = 0;
    int skippedPrograms  = 0;
    int totalPrograms    = 0;

    for (XmlNode stNode = rsNode.FirstChild("station");
         stNode;
         stNode = stNode.NextChild("station"))
    {
        RadioStation station;
        station.name = stNode.Attribute16("name");
        if (!SanitizeName(station.name))
        {
            AudioLog::PostWarn(L"RadioLoader: skipped station with empty/invalid name");
            ++skippedStations;
            continue;
        }

        for (XmlNode pgNode = stNode.FirstChild("program");
             pgNode;
             pgNode = pgNode.NextChild("program"))
        {
            RadioProgram program;

            // genre is optional – an absent or empty attribute is fine
            program.genre = pgNode.Attribute16("genre");
            if (!program.genre.empty())
            {
                // sanitise but don't reject the entry if genre becomes empty
                SanitizeName(program.genre);
            }

            program.name = pgNode.Attribute16("name");
            if (!SanitizeName(program.name))
            {
                AudioLog::PostWarn(L"RadioLoader: skipped program with empty/invalid name in station \""
                    + station.name + L"\"");
                ++skippedPrograms;
                continue;
            }

            const char* rawUrl = pgNode.AttributeRaw("url");
            if (!SanitizeUrl(rawUrl, program.url))
            {
                std::wstring rawW = rawUrl
                    ? std::wstring(rawUrl, rawUrl + std::strlen(rawUrl))
                    : std::wstring(L"(null)");
                AudioLog::PostWarn(L"RadioLoader: skipped program with invalid URL \""
                    + rawW + L"\" in station \"" + station.name + L"\"");
                ++skippedPrograms;
                continue;
            }

            ++totalPrograms;
            station.programs.push_back(std::move(program));
        }

        // Add the station even when it has no valid programs so it still
        // appears in the tree (the list will simply be empty).
        result.push_back(std::move(station));
    }

    AudioLog::PostInfo(
        L"RadioLoader: loaded " + std::to_wstring(result.size()) +
        L" station(s), " + std::to_wstring(totalPrograms) + L" program(s)" +
        (skippedStations + skippedPrograms > 0
            ? L" [skipped: " + std::to_wstring(skippedStations) + L" station(s), "
              + std::to_wstring(skippedPrograms) + L" program(s)]"
            : L"") +
        L" from " + xmlPath);

    return result;
}

bool RadioLoader::SanitizeName(std::wstring& name)
{
    // Remove control characters (U+0000 – U+001F)
    name.erase(
        std::remove_if(name.begin(), name.end(),
            [](wchar_t c) { return c < 0x20; }),
        name.end());

    // Trim leading/trailing spaces
    StringEx::Trim(name);

    // Enforce maximum length
    if (name.size() > kMaxNameLen)
        name.resize(kMaxNameLen);

    return !name.empty();
}

bool RadioLoader::SanitizeUrl(const char* raw, std::string& out)
{
    if (!raw || raw[0] == '\0')
        return false;

    const std::string_view sv(raw);

    // Must start with a recognised scheme
    constexpr std::string_view http  = "http://";
    constexpr std::string_view https = "https://";

    if (sv.substr(0, http.size())  != http &&
        sv.substr(0, https.size()) != https)
        return false;

    // Length limit
    if (sv.size() > kMaxUrlLen)
        return false;

    // Every byte must be printable 7-bit ASCII (0x20 – 0x7E).
    // This rejects raw UTF-8 / non-ASCII bytes, so international domain names
    // must already be Punycode-encoded (xn--...) in the XML file.
    for (const unsigned char c : sv)
    {
        if (c < 0x20 || c > 0x7E)
            return false;
    }

    out.assign(sv);
    return true;
}

} // namespace winyl
