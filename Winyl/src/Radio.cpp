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
#include "Radio.h"
#include "RadioLoader.h"
#include "UTF.h"

#include <algorithm>

Radio::Radio()
{
}

Radio::~Radio()
{
}

bool Radio::LoadFromFile(const std::wstring& programPath)
{
	std::wstring xmlPath = programPath + L"radio_stations.xml";
	auto result = winyl::RadioLoader::Load(xmlPath);
	if (result)
	{
		stations_ = std::move(*result);
		return true;
	}
	// File missing or fatal parse error – continue with an empty list.
	// The radio tree will simply show no stations.
	stations_.clear();
	return false;
}

bool Radio::LoadTree(SkinTree* skinTree, TreeNodeUnsafe node)
{
	skinTree->SetControlRedraw(false);

	for (const auto& station : stations_)
	{
		skinTree->InsertRadio(node, station.name, station.name);
	}

	skinTree->SetControlRedraw(true);
	return true;
}

bool Radio::LoadList(SkinList* skinList, const std::wstring& stationName)
{
	// Locate the station whose name matches the clicked tree node.
	auto it = std::find_if(
		stations_.cbegin(), stations_.cend(),
		[&stationName](const winyl::RadioStation& s) { return s.name == stationName; });

	if (it == stations_.cend())
		return false;

	skinList->SetControlRedraw(false);
	skinList->DeleteAllNode();
	skinList->EnableRadio(true);
	skinList->SetViewPlaylist(true);

	for (const auto& program : it->programs)
	{
		// url is validated ASCII, so UTF-16 conversion is lossless.
		const std::wstring url16 = UTF::UTF16S(program.url);

		ListNodeUnsafe listNode =
			skinList->InsertTrack(nullptr, url16, 0, 0, 0, 0, 0, 0);

		// Artist = "StationName" or "StationName: Genre"
		std::wstring artist = it->name;
		if (!program.genre.empty())
		{
			artist += L": ";
			artist += program.genre;
		}

		skinList->SetNodeString(listNode, SkinListElement::Type::Artist, artist);
		skinList->SetNodeString(listNode, SkinListElement::Type::Title, program.name);
		skinList->SetNodeString(listNode, SkinListElement::Type::Time, L"8:88");
	}

	skinList->ResetIndex();
	skinList->SetControlRedraw(true);
	return true;
}
