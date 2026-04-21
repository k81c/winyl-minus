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
#include "resource.h"
#include "WinylWnd.h"
#include "FileSystem.h"
#include "AudioLog.h"
#include "InitChecks.h"

void WinylWnd::ActionMinimize()
{
	skinDraw.MouseLeave();
	isWindowIconic = true;
	trayIcon.Minimize(thisWnd, skinDraw.IsLayeredAlpha());
}

void WinylWnd::ActionMaximize(bool isMaximize)
{
	settings.SetMaximized(isMaximize);
	skinDraw.DrawMaximize(isMaximize);

	isWindowIconic = false;

	if (isMaximize)
		trayIcon.Maximize(thisWnd, skinDraw.IsLayeredAlpha());
	else
		trayIcon.Restore(thisWnd, skinDraw.IsLayeredAlpha());
}

void WinylWnd::ActionStop()
{ // Stop the playback
	if (isMediaPlay == false)
		return;

	isMediaPlay = false;
	isMediaPause = false;
	isMediaRadio = false;

	libAudio.Stop();

	::KillTimer(thisWnd, TimerValue::PosID);
	::KillTimer(thisWnd, TimerValue::TimeID);
	if (!visuals.empty())
		::SetTimer(thisWnd, TimerValue::VisID, TimerValue::Vis, NULL);

	skinList->RemoveShuffle();
	if (skinList->GetPlayNode())
		skinList->SetPlayNode(nullptr);

	skinDraw.DrawPosition(0);
	skinDraw.DrawPlay(false);
	skinDraw.DrawRating(-1);
	skinDraw.DrawTextNone();
	SetLyricsNone();
	SetCoverNone();

	if (!settings.IsLibraryNowPlaying())
		dBase.CloseNowPlaying();
	idMediaLibrary = 0;
	idMediaPlaylist = 0;

	SetWindowCaptionNull();

	win7TaskBar.UpdateButtons(thisWnd, false);

	if (settings.IsLastFM())
		lastFM.Stop();
	//if (settings.IsMSN())
	//	Messengers::MSNStop();
}

void WinylWnd::ActionPlayEx()
{ // "Smart" Play
	// Same as Play but if paused then resume instead of play from the start
	if (isMediaPlay && isMediaPause)
		ActionPause();
	else
		ActionPlay(false, true);
}

void WinylWnd::ActionPauseEx()
{ // "Smart" Pause
	// Same as Pause but if stopped then start the playback instead of does nothing
	if (isMediaPlay)
		ActionPause();
	else
		ActionPlay(false);
}

void WinylWnd::ActionPlay(bool isPlayFocused, bool isReplay, bool isRepeat)
{ // Play a track
	skinDraw.DrawPosition(0);

	ListNodeUnsafe node = nullptr;

	if (isReplay)
		node = skinList->GetPlayNode(); // Play the same track

	if (node == nullptr)
	{
		if (isPlayFocused)// || !skinList->IsNowPlayingList())
			node = skinList->GetFocusNode(); // Or the selected track
		else
		{
			node = skinList->GetLastPlayNode(); // Or the last played
			if (node == nullptr)
				node = skinList->GetFocusNode(); // Or the selected track
		}
	}

	if (node == nullptr)
		node = skinList->FindPrevTrack(nullptr, true); // Or the first track

	if (node)
	{
		//skinList->SetPlayNode(pNode);
		if (!skinList->IsNowPlaying() ||
			(isPlayFocused && !isReplay && !skinList->IsNowPlayingOpen()))
		{
			PlayNode(node, isRepeat, true);
			//MessageBeep(1);
		}
		else
			PlayNode(node, isRepeat, false);

		if (settings.IsShuffle())
			skinList->AddShuffle(node);
	}
}

void WinylWnd::ActionPause()
{ // Pause/Resume the playback
	if (isMediaPlay)
	{
		if (isMediaRadio)
		{
			ActionStop();
			return;
		}

		if (!isMediaPause) // Pause
		{
			isMediaPause = true;

			libAudio.Pause();

			skinList->SetPause(true);

			::KillTimer(thisWnd, TimerValue::PosID);
			::KillTimer(thisWnd, TimerValue::TimeID);
			if (!visuals.empty())
				::SetTimer(thisWnd, TimerValue::VisID, TimerValue::Vis, NULL);

			skinDraw.DrawPlay(false);

			win7TaskBar.UpdateButtons(thisWnd, false);

			if (settings.IsLastFM())
				lastFM.Pause();
		}
		else // Resume
		{
			isMediaPause = false;

			libAudio.Play();

			skinList->SetPause(false);

			::SetTimer(thisWnd, TimerValue::PosID, TimerValue::Pos, NULL);
			::SetTimer(thisWnd, TimerValue::TimeID, TimerValue::Time, NULL);
			if (!visuals.empty())
				::SetTimer(thisWnd, TimerValue::VisID, TimerValue::Vis, NULL);

			skinDraw.DrawPlay(true);

			win7TaskBar.UpdateButtons(thisWnd, true);

			if (settings.IsLastFM())
				lastFM.Resume();
		}
	}
}

void WinylWnd::ActionPosition(int position)
{ // Set the position of the playing track
	if (isMediaPlay && !isMediaRadio)
	{
		libAudio.SetPosition(position);

		if (isMediaPause)
			ActionPause();
	}
	else // If nothing is playing then reset the position
		skinDraw.DrawPosition(0);
}

void WinylWnd::ActionMute(bool isMute)
{ // Mute the sound
	settings.SetMute(isMute);
	libAudio.SetMute(isMute);
	skinDraw.DrawMute(isMute);
	if (isMute)
		skinDraw.DrawVolume(0);
	else
		skinDraw.DrawVolume(settings.GetVolume());
	contextMenu.CheckMute(isMute);
}

void WinylWnd::ActionVolume(int volume, bool isSkinDraw)
{ // Set the volume
	settings.SetVolume(volume);
	libAudio.SetVolume(volume);

	if (isSkinDraw)
		skinDraw.DrawVolume(volume);

	if (settings.IsMute())
	{
		settings.SetMute(false);
		libAudio.SetMute(false);
		skinDraw.DrawMute(false);
		contextMenu.CheckMute(false);
	}
}

void WinylWnd::ActionSetRating(int rating, bool isSkinDraw)
{ // Set the rating of the playing track
	skinList->SetPlayRating(rating);

	if (isSkinDraw)
		skinDraw.DrawRating(rating);

	dBase.SetRating(idMediaLibrary, idMediaPlaylist, rating, true);
}

void WinylWnd::ActionVolumeUp()
{ // Volume up 5%
	int volume = settings.GetVolume(); //libAudio.GetVolume();
	volume += 5000;
	volume = std::min(volume, 100000);
	
	ActionVolume(volume, true);
}

void WinylWnd::ActionVolumeDown()
{ // Volume down 5%
	int volume = settings.GetVolume(); //libAudio.GetVolume();
	volume -= 5000;
	volume = std::max(volume, 0);
	
	ActionVolume(volume, true);
}

void WinylWnd::ActionNextTrack()
{ // Next track

	ListNodeUnsafe newNode = nullptr;
	ListNodeUnsafe playNode = skinList->GetPlayNode();

	if (settings.IsShuffle()) // Shuffle is on
	{
		// Increase skip count of the previous track
		if (!isMediaRadio) // If it wasn't a radio
			dBase.IncreaseSkip(idMediaLibrary, idMediaPlaylist);

		// Search for a random track that wasn't played before
		newNode = skinList->FindNextTrackShuffle();

		if (newNode) // Play
		{
			//skinList->SetPlayNode(newNode);
			skinList->ScrollToMyNode(newNode);
			PlayNode(newNode);
		}
		else // Found nothing means all tracks are played
		{
			if (settings.IsRepeat()) // Repeat is on
			{
				// Create a new shuffle list and search for a random track
				skinList->RemoveShuffle();
				newNode = skinList->FindNextTrackShuffle();

				if (newNode) // Play
				{
					//skinList->SetPlayNode(newNode);
					skinList->ScrollToMyNode(newNode);
					PlayNode(newNode);
				}
			}
			else // Stop the playback
				ActionStop(); // Also destroys the current shuffle list
		}

		return;
	}

	// Shuffle is off

	if (playNode) // Something is playing
	{
		// Increase skip count of the previous track
		if (!isMediaRadio) // If it wasn't a radio
			dBase.IncreaseSkip(idMediaLibrary, idMediaPlaylist);

		// Search for the next track
		ListNodeUnsafe nextNode = skinList->FindNextTrack(playNode);

		if (nextNode == nullptr) // Found nothing means that it was the last track
		{
			if (settings.IsRepeat()) // Repeat is on
				newNode = skinList->FindPrevTrack(nullptr); // Go to the first track
			else
				newNode = nullptr; // Stop the playback
		}
		else
			newNode = nextNode;

		if (newNode) // Play
		{
			//skinList->SetPlayNode(newNode);
			skinList->ScrollToMyNode(newNode);
			PlayNode(newNode);
		}
		else // Stop the playback
			ActionStop();
	}
	else // Nothing is playing
	{
		// Get last played track
		ListNodeUnsafe lastPlayNode = skinList->GetLastPlayNode();

		if (lastPlayNode)
		{
			// Search for the next track that after the last played track
			ListNodeUnsafe nextNode = skinList->FindNextTrack(lastPlayNode);

			if (nextNode == nullptr) // Found nothing means that it was the last track
			{
				if (settings.IsRepeat()) // Repeat is on
					newNode = skinList->FindPrevTrack(nullptr); // Go to the first track
				else
					newNode = nullptr; // Do nothing
			}
			else
				newNode = nextNode;
		}
		else // If there is not last played track just play the first track
			newNode = skinList->FindPrevTrack(nullptr);

		if (newNode) // Play
		{
			//skinList->SetPlayNode(newNode);
			skinList->ScrollToMyNode(newNode);
			PlayNode(newNode);
		}
	}
}

void WinylWnd::ActionPrevTrack()
{ // Previous track

	ListNodeUnsafe newNode = nullptr;
	ListNodeUnsafe playNode = skinList->GetPlayNode();

	if (settings.IsShuffle()) // Shuffle is on
	{
		// Get the previous track from the shuffle list
		newNode = skinList->FindPrevTrackShuffle();

		if (newNode) // Play
		{
			//skinList->SetPlayNode(newNode);
			skinList->ScrollToMyNode(newNode);
			PlayNode(newNode);
		}
		else // Stop the playback
			ActionStop(); // Also destroys the current shuffle list

		return;
	}

	// Shuffle is off

	if (playNode) // Something is playing
	{
		// Search for the previous track
		ListNodeUnsafe prevNode = skinList->FindPrevTrack(playNode);

		if (prevNode == nullptr) // Found nothing means that it was the first track
		{
			if (settings.IsRepeat()) // Repeat is on
				newNode = skinList->FindNextTrack(nullptr); // Go to the last track
			else
				newNode = nullptr; // Stop the playback
		}
		else
			newNode = prevNode;

		if (newNode) // Play
		{
			//skinList->SetPlayNode(newNode);
			skinList->ScrollToMyNode(newNode);
			PlayNode(newNode);
		}
		else // Stop the playback
			ActionStop();
	}
	else // Nothing is playing
	{
		// Get last played track
		ListNodeUnsafe lastPlayNode = skinList->GetLastPlayNode();

		if (lastPlayNode)
		{
			// Search for the previous track that before the last played track
			ListNodeUnsafe prevNode = skinList->FindPrevTrack(lastPlayNode);

			if (prevNode == nullptr) // Found nothing means that it was the first track
			{
				if (settings.IsRepeat()) // Repeat is on
					newNode = skinList->FindNextTrack(nullptr); // Go to the last track
				else
					newNode = nullptr; // Do nothing
			}
			else
				newNode = prevNode;
		}
		else // If there is not last played track just play the first track
			newNode = skinList->FindPrevTrack(nullptr);

		if (newNode) // Play
		{
			//skinList->SetPlayNode(newNode);
			skinList->ScrollToMyNode(newNode);
			PlayNode(newNode);
		}
	}
}

void WinylWnd::ActionShuffle(bool isShuffle)
{ // Toggle shuffle
	settings.SetShuffle(isShuffle);

	if (isShuffle)
	{
		if (skinList->GetPlayNode())
			skinList->AddShuffle(skinList->GetPlayNode());
	}
	else
		skinList->RemoveShuffle();

	skinDraw.DrawShuffle(isShuffle);
	contextMenu.CheckShuffle(isShuffle);
}

void WinylWnd::ActionRepeat(bool isRepeat)
{ // Toggle repeat
	settings.SetRepeat(isRepeat);
	skinDraw.DrawRepeat(isRepeat);
	contextMenu.CheckRepeat(isRepeat);
}

bool WinylWnd::PlayNode(ListNodeUnsafe node, bool isRepeat, bool isNewNowPlaying, bool isReconnect)
{
	isRepeatTrack = isRepeat;

	if (node == nullptr)
		return false;

	// This function can invalidate nodes so make the node safe just in case
	ListNodeSafe safeNode(node);


	isMediaRadio = false;

	LibAudio::Error error = LibAudio::Error::None;
	if (PathEx::IsURL(node->GetFile()))
	{
		isMediaRadio = true;
		libAudio.PlayURL(node->GetFile(), isReconnect);
	}
	else
		error = libAudio.PlayFile(node->GetFile(), node->GetCueValue());

	if (error != LibAudio::Error::None)
	{
		isMediaRadio = false;

		ActionStop();
		EnableAll(false);
		if (error == LibAudio::Error::File) // File error
			MessageBox::Show(thisWnd, lang.GetLine(Lang::Message, 0), lang.GetLine(Lang::Message, 3), MessageBox::Icon::Warning);
		else //if (error == LibAudio::Error::Device) // Device error
			MessageBox::Show(thisWnd, lang.GetLine(Lang::Message, 0), lang.GetLine(Lang::Message, 8), MessageBox::Icon::Warning);
		EnableAll(true);
		//skinList->SetPlayNode(nullptr);
		return false;
	}

	// This setting is stupid it should be always true
	if (!settings.IsLibraryNowPlaying())
	{
		if (!dBase.IsNowPlayingOpen())
		{
			dBase.CloseNowPlaying();
			dBase.NewNowPlaying();
		}
	}
	else
	{
		if (isNewNowPlaying)
		{
			assert(dBase.IsNowPlayingOpen() == skinList->IsNowPlayingOpen());

			if (!dBase.IsNowPlayingOpen())
			{
				dBase.CloseNowPlaying();
				dBase.NewNowPlaying();
			}
			if (!skinList->IsNowPlayingOpen())
			{
				skinList->DeleteNowPlaying();
				skinList->NewNowPlaying();
				settings.NewNowPlaying();
			}
		}
	}

	skinList->SetPlayNode(node);

	idMediaLibrary = node->idLibrary;
	idMediaPlaylist = node->idPlaylist;

	if (!isMediaRadio)
	{
		isMediaPlay = true;
		isMediaPause = false;

		::SetTimer(thisWnd, TimerValue::PosID, TimerValue::Pos, NULL);
		::SetTimer(thisWnd, TimerValue::TimeID, TimerValue::Time, NULL);
		if (!visuals.empty())
			::SetTimer(thisWnd, TimerValue::VisID, TimerValue::Vis, NULL);

		skinDraw.DrawPlay(true);

		std::wstring filename, title, album, artist, genre, year;
		dBase.GetSongTags(node->idLibrary, node->idPlaylist, filename, title, album, artist, genre, year);

		SkinDrawText(title.empty() ? filename : title, album, artist, genre, year, false);

		skinDraw.DrawRating(node->rating);

		LyricsThread(node->GetFile(), title, artist);

		//skinDraw.DrawCover(pNode->csFile, csAlbum);
		bool coverThread = CoverThread(node->GetFile());

		SetWindowCaption(artist, title.empty() ? filename : title);

		win7TaskBar.UpdateButtons(thisWnd, true);

		if (!coverThread)
			ShowPopup();

		if (settings.IsLastFM())
			lastFM.Start(artist, title.empty() ? filename : title, album, libAudio.GetTimeLength());
		//if (settings.IsMSN())
		//	Messengers::MSNPlay(artist, title.empty() ? filename : title);
	}
	else
	{
		EnableWaitRadioCursor(true);

		isMediaPlay = false;
		isMediaPause = false;

		radioString = node->GetLabel(SkinListElement::Type::Title);
		
		skinDraw.DrawRating(node->rating + (skinList->IsRadio() ? 100 : 0));

		SkinDrawText(radioString, L"", L"", L"", L"", true);

		::KillTimer(thisWnd, TimerValue::PosID);
		::KillTimer(thisWnd, TimerValue::TimeID);
		if (!visuals.empty())
			::SetTimer(thisWnd, TimerValue::VisID, TimerValue::Vis, NULL);

		skinDraw.DrawPosition(0);
		skinDraw.DrawPlay(false);
		SetLyricsNone(true);
		SetCoverNone();

		SetWindowCaption(L"", radioString, true);

		win7TaskBar.UpdateButtons(thisWnd, false);

		if (settings.IsLastFM())
			lastFM.Stop();
		//if (settings.IsMSN())
		//	Messengers::MSNStop();
	}

	return true;
}

void WinylWnd::StartRadio(LibAudio::Error error, bool isReconnect)
{
	EnableWaitRadioCursor(false);

	if (error == LibAudio::Error::None)
	{
		isMediaPlay = true;
		isMediaPause = false;

		//::SetTimer(thisWnd, TIMER_POS_ID, TIMER_POS, NULL);
		if (!visuals.empty())
			::SetTimer(thisWnd, TimerValue::VisID, TimerValue::Vis, NULL);

		skinDraw.DrawPlay(true);

		std::wstring title = radioString, artist, album;

		libAudio.GetRadioTags(title, artist, album);

		SkinDrawText(title, album, artist, L"", L"", true);

		SetWindowCaption(artist, title);

		win7TaskBar.UpdateButtons(thisWnd, true);

		ShowPopup();
	}
	else
	{
		if (!isReconnect)
		{
			EnableAll(false);
			if (error == LibAudio::Error::File) // File error
				MessageBox::Show(thisWnd, lang.GetLine(Lang::Message, 0), lang.GetLine(Lang::Message, 3), MessageBox::Icon::Warning);
			else if (error == LibAudio::Error::Inet) // Inet error
				MessageBox::Show(thisWnd, lang.GetLine(Lang::Message, 0), lang.GetLine(Lang::Message, 4), MessageBox::Icon::Warning);
			else //if (error == LibAudio::Error::Device) // Device error
				MessageBox::Show(thisWnd, lang.GetLine(Lang::Message, 0), lang.GetLine(Lang::Message, 8), MessageBox::Icon::Warning);
			EnableAll(true);
		}

		if (skinList->GetPlayNode())
			skinList->SetPlayNode(nullptr);

		skinDraw.DrawRating(-1);
		skinDraw.DrawTextNone();

		SetWindowCaptionNull();
	}
}

std::wstring WinylWnd::ChangeFile(long long& outCue)
{
	ListNodeUnsafe tempNode = nullptr;
	//tempFocusNode = nullptr;

	std::wstring file;
	outCue = 0;

	if (isRepeatTrack) // Return the same track
	{
		ListNodeUnsafe playNode = skinList->GetPlayNode();
		tempNode = playNode;

		if (tempNode)
			file = tempNode->GetFile();

		skinList->SetTempNode(tempNode);
		return file;
	}

	if (settings.IsShuffle()) // Return a random track
	{
		tempNode = skinList->FindNextTrackShuffle();
		if (tempNode == nullptr) // All tracks are played
		{
			if (settings.IsRepeat()) // Repeat is on
			{
				// Create a new shuffle list and return a random track
				skinList->RemoveShuffle();
				tempNode = skinList->FindNextTrackShuffle();
			}
		}
	}
	else 
	{
		ListNodeUnsafe focusNode = nullptr;

		if (settings.IsPlayFocus())
		{
			/*focusNode = skinList->GetFocusNode();

			tempFocusNode = focusNode;
			if (focusNode == nullptr && settings.IsLibraryNowPlaying())
				focusNode = skinList->GetFocusNodePlay();*/

			if (!settings.IsLibraryNowPlaying())
				focusNode = skinList->GetFocusNode();
			else
				focusNode = skinList->GetFocusNodePlay();
		}

		//ListNodeUnsafe focusNode = skinList->GetFocusNodePlay();
		ListNodeUnsafe playNode = skinList->GetPlayNode();

		// If nothing is selected use playing node
		if (focusNode == nullptr)
			focusNode = playNode;
	
		if (focusNode == nullptr)
		{ // If nothing is selected and playing then return the first track
			tempNode = skinList->FindPrevTrack(nullptr);
		}
		else if (focusNode == playNode)
		{ // If selected and playing are the same then return the next track
			ListNodeUnsafe nextNode = skinList->FindNextTrack(playNode);
			if (nextNode == nullptr) // If it is the last track
			{
				if (settings.IsRepeat()) // Repeat is on
					tempNode = skinList->FindPrevTrack(nullptr); // Return the first track
				else
					tempNode = nullptr; // Stop the playback
			}
			else
				tempNode = nextNode;
		}
		else
		{ // Return the selected track
			tempNode = focusNode;
		}
	}

	if (tempNode) // Return the track
	{
		file = tempNode->GetFile();
		outCue = tempNode->GetCueValue();
	}

	skinList->SetTempNode(tempNode);
	return file;
}

void WinylWnd::ChangeNode(bool isError, bool isRadio)
{
	if (skinList->GetTempNode() && !isError)
	{
		// This setting is stupid it should be always true
		if (!settings.IsLibraryNowPlaying())
		{
			if (!dBase.IsNowPlayingOpen())
			{
				dBase.CloseNowPlaying();
				dBase.NewNowPlaying();
			}
		}
		/*else
		{
			if (settings.IsPlayFocus() && tempFocusNode)
			{
				if (!dBase.IsNowPlaying())
				{
					dBase.CloseNowPlaying();
					dBase.NewNowPlaying();
				}
				if (!skinList->IsNowPlayingList())
				{
					skinList->DeleteNowPlaying();
					skinList->NewNowPlaying();
					settings.NewNowPlaying();
				}
			}
		}*/

		skinList->SetPlayNode(skinList->GetTempNode());
		if (isMediaPause)
			skinList->SetPause(isMediaPause);

		if (!isRadio)
		{
			isMediaRadio = false;

			skinDraw.DrawPosition(0);

			skinDraw.DrawRating(skinList->GetTempNode()->rating);

			std::wstring filename, title, album, artist, genre, year;
			dBase.GetSongTags(skinList->GetTempNode()->idLibrary, skinList->GetTempNode()->idPlaylist, filename, title, album, artist, genre, year);

			SkinDrawText(title.empty() ? filename : title, album, artist, genre, year, false);

			LyricsThread(skinList->GetTempNode()->GetFile(), title, artist);

			bool coverThread = CoverThread(skinList->GetTempNode()->GetFile());

			SetWindowCaption(artist, title.empty() ? filename : title);

			// Increase skip count of the previous track
			dBase.IncreaseCount(idMediaLibrary, idMediaPlaylist);

			if (!coverThread)
				ShowPopup();

			if (settings.IsLastFM())
				lastFM.Start(artist, title.empty() ? filename : title, album, libAudio.GetTimeLength());
			//if (settings.IsMSN())
			//	Messengers::MSNPlay(artist, title.empty() ? filename : title);
		}
		else
		{
			isMediaRadio = true;
			isMediaPlay = false;
			isMediaPause = false;

			radioString = skinList->GetTempNode()->GetLabel(SkinListElement::Type::Title);

			skinDraw.DrawRating(skinList->GetTempNode()->rating + (skinList->IsRadio() ? 100 : 0));

			SkinDrawText(radioString, L"", L"", L"", L"", true);

			::KillTimer(thisWnd, TimerValue::PosID);
			::KillTimer(thisWnd, TimerValue::TimeID);
			if (!visuals.empty())
				::SetTimer(thisWnd, TimerValue::VisID, TimerValue::Vis, NULL);

			skinDraw.DrawPosition(0);
			skinDraw.DrawPlay(false);
			SetLyricsNone(true);
			SetCoverNone();

			SetWindowCaption(L"", radioString, true);

			win7TaskBar.UpdateButtons(thisWnd, false);

			if (settings.IsLastFM())
				lastFM.Stop();
			//if (settings.IsMSN())
			//	Messengers::MSNStop();
		}

		// Always at the end or IncreaseCount that above will set incorrect values
		idMediaLibrary = skinList->GetTempNode()->idLibrary;
		idMediaPlaylist = skinList->GetTempNode()->idPlaylist;

		skinList->SetTempNode(nullptr);
	}
	else
	{
		if (isMediaRadio)
		{
			if (isMediaPlay && !isMediaPause && skinList->GetPlayNode() && libAudio.IsRadioStream())
				PlayNode(skinList->GetPlayNode(), false, false, true);
			else
				ActionStop();
		}
		else
		{
			dBase.IncreaseCount(idMediaLibrary, idMediaPlaylist);
			ActionStop();
		}
	}
}

void WinylWnd::SkinDrawText(const std::wstring& title, const std::wstring& album, const std::wstring& artist,
							const std::wstring& genre, const std::wstring& year, bool isRadio)
{
	std::wstring album2 = album;
	std::wstring artist2 = artist;

	if (!isRadio)
	{
		if (album2.empty())
			album2 = lang.GetLineS(Lang::Playlist, 0);
		if (artist2.empty())
			artist2 = lang.GetLineS(Lang::Playlist, 0);
	}
	else
	{
		if (artist2.empty())
			artist2 = lang.GetLineS(Lang::Playlist, 3);
		if (album2.empty())
			album2 = lang.GetLineS(Lang::Playlist, 0);
	}

	skinDraw.DrawText(title, album2, artist2, genre, year, isMediaRadio ? -1 : libAudio.GetTimeLength());
	if (skinPopup)
		skinPopup->SetText(title, album2, artist2, genre, year, isMediaRadio ? -1 : libAudio.GetTimeLength());
}

void WinylWnd::SetWindowCaption(const std::wstring& artist, const std::wstring& title, bool isRadio)
{
	std::wstring artistTitle = title;

	if (!artist.empty())
		artistTitle = artist + L" - " + artistTitle;
	else if (isRadio)
		artistTitle = lang.GetLineS(Lang::Playlist, 3) + L" - " + artistTitle;

	trayIcon.SetString(thisWnd, artistTitle);

	::SetWindowText(thisWnd, artistTitle.c_str());

	if (skinMini)
		::SetWindowText(skinMini->Wnd(), artistTitle.c_str());
}

void WinylWnd::SetWindowCaptionNull()
{
	trayIcon.SetNullString(thisWnd);

	::SetWindowText(thisWnd, L"Winyl");
	if (skinMini)
		::SetWindowText(skinMini->Wnd(), L"Winyl");
}

void WinylWnd::ShowPopup()
{
	if (skinPopup && settings.IsPopup())
	{
		if (!(skinMini && ::IsWindowVisible(skinMini->Wnd())))
			skinPopup->Popup();
	}
}
