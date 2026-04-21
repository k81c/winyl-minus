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

void WinylWnd::FillTree(TreeNodeUnsafe node)
{
	if (!(node->IsType() && node->GetType() == SkinTreeNode::Type::Radio))
		BeginWaitCursor();

	if (node->IsType())
	{
		switch (node->GetType())
		{
		case SkinTreeNode::Type::Artist:
			dBase.FillTreeArtist(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Composer:
			dBase.FillTreeComposer(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Album:
			dBase.FillTreeAlbum(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Genre:
			dBase.FillTreeGenre(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Year:
			dBase.FillTreeYear(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Folder:
			dBase.FillTreeFolder(skinTree.get(), node,
				settings.IsAddAllToLibrary() ? nullptr : &libraryFolders);
			break;
		case SkinTreeNode::Type::Radio:
			radio.LoadTree(skinTree.get(), node);
		}
	}
	else if (node->IsValue())
	{
		switch (node->GetType())
		{
		case SkinTreeNode::Type::Artist:
			dBase.FillTreeArtistAlbum(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Composer:
			dBase.FillTreeComposerArtist(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Genre:
			dBase.FillTreeGenreArtist(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Year:
			dBase.FillTreeYearArtist(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Folder:
			dBase.FillTreeFolderSub(skinTree.get(), node);
			break;
		}
	}
	else if (node->IsArtist())
	{
		switch (node->Parent()->GetType())
		{
		case SkinTreeNode::Type::Composer:
			dBase.FillTreeComposerAlbum(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Genre:
			dBase.FillTreeGenreAlbum(skinTree.get(), node);
			break;
		case SkinTreeNode::Type::Year:
			dBase.FillTreeYearAlbum(skinTree.get(), node);
			break;
		}
	}

	if (!(node->IsType() && node->GetType() == SkinTreeNode::Type::Radio))
		EndWaitCursor();
}

void WinylWnd::FillList(TreeNodeUnsafe node)
{
	// First always need to stop and clear the search
	if (threadSearch.IsJoinable())
	{
		dBase.SetStopSearch(true);
		threadSearch.Join();
		dBase.SetStopSearch(false);
	}
	if (!skinEdit->IsSearchEmpty())
	{
		skinEdit->SearchClear();
		skinDraw.DrawSearchClear(false);
	}

	if (node && node->GetNodeType() == SkinTreeNode::NodeType::Head)
	{
		// Return to Now Playing view
		if (node->GetType() == SkinTreeNode::Type::NowPlaying)
		{
			settings.ReturnNowPlaying();
			dBase.ClosePlaylist();
			dBase.ReturnNowPlaying();
			skinList->NowPlayingList();

			UpdateStatusLine();
		}

		return;
	}

	if (node)
	{
		if (node->IsAlbum())
			settings.SetLibraryType(node->IsType(), (int)node->Parent()->Parent()->GetType());
		else if (node->IsArtist())
			settings.SetLibraryType(node->IsType(), (int)node->Parent()->GetType());
		else
			settings.SetLibraryType(node->IsType(), (int)node->GetType());
		settings.SetLibraryValue(node->IsValue(), node->GetValue());
		settings.SetLibraryArtist(node->IsArtist(), node->GetArtist());
		settings.SetLibraryAlbum(node->IsAlbum(), node->GetAlbum());
	}

	// !!! In FillJump below should be the same

	if (isMediaPlay &&
		(settings.GetLibraryType() == (int)SkinTreeNode::Type::Playlist ||
		settings.GetLibraryType() == (int)SkinTreeNode::Type::Radio) &&
		settings.IsNowPlaying())
	{
		dBase.ClosePlaylist();
		dBase.ReturnNowPlaying();
		skinList->NowPlayingList();

		UpdateStatusLine();
		return;
	}

	if (!(settings.IsLibraryValue() && settings.GetLibraryType() == (int)SkinTreeNode::Type::Radio))
		BeginWaitCursor();

	dBase.ClosePlaylist();

	if (settings.IsLibraryAlbum())
	{
		switch ((SkinTreeNode::Type)settings.GetLibraryType())
		{
		case SkinTreeNode::Type::Artist:
			dBase.FillListArtistAlbum(skinList.get(), settings.GetLibraryValue(), settings.GetLibraryArtist(), settings.GetLibraryAlbum());
			break;
		case SkinTreeNode::Type::Composer:
			dBase.FillListComposerAlbum(skinList.get(), settings.GetLibraryValue(), settings.GetLibraryArtist(), settings.GetLibraryAlbum());
			break;
		case SkinTreeNode::Type::Genre:
			dBase.FillListGenreAlbum(skinList.get(), settings.GetLibraryValue(), settings.GetLibraryArtist(), settings.GetLibraryAlbum());
			break;
		case SkinTreeNode::Type::Year:
			dBase.FillListYearAlbum(skinList.get(), settings.GetLibraryValue(), settings.GetLibraryArtist(), settings.GetLibraryAlbum());
			break;
		}
	}
	else if (settings.IsLibraryArtist())
	{
		switch ((SkinTreeNode::Type)settings.GetLibraryType())
		{
		case SkinTreeNode::Type::Composer:
			dBase.FillListComposerArtist(skinList.get(), settings.GetLibraryValue(), settings.GetLibraryArtist());
			break;
		case SkinTreeNode::Type::Genre:
			dBase.FillListGenreArtist(skinList.get(), settings.GetLibraryValue(), settings.GetLibraryArtist());
			break;
		case SkinTreeNode::Type::Year:
			dBase.FillListYearArtist(skinList.get(), settings.GetLibraryValue(), settings.GetLibraryArtist());
			break;
		}
	}
	else if (settings.IsLibraryValue())
	{
		switch ((SkinTreeNode::Type)settings.GetLibraryType())
		{
		case SkinTreeNode::Type::Album:
			dBase.FillListAlbum(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Artist:
			dBase.FillListArtist(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Composer:
			dBase.FillListComposer(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Genre:
			dBase.FillListGenre(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Year:
			dBase.FillListYear(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Folder:
			dBase.FillListFolder(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Playlist:
			dBase.OpenPlaylist(settings.GetLibraryValue());
			dBase.FillPlaylist(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Smartlist:
			dBase.FillSmartlist(skinList.get(), settings.GetLibraryValue());
			break;
		case SkinTreeNode::Type::Radio:
			radio.LoadList(skinList.get(), settings.GetLibraryValue());
			break;
		}
	}

	if (!(settings.IsLibraryValue() && settings.GetLibraryType() == (int)SkinTreeNode::Type::Radio))
		EndWaitCursor();

	UpdateStatusLine();
}

void WinylWnd::FillJump(ListNodeUnsafe node, SkinTreeNode::Type type)
{
	if (node == nullptr)
		return;

	BeginWaitCursor();

	// First always need to stop and clear the search
	if (threadSearch.IsJoinable())
	{
		dBase.SetStopSearch(true);
		threadSearch.Join();
		dBase.SetStopSearch(false);
	}
	if (!skinEdit->IsSearchEmpty())
	{
		skinEdit->SearchClear();
		skinDraw.DrawSearchClear(false);
	}

	//////////////
	if (type != SkinTreeNode::Type::Folder)
	{
		std::wstring filename, title, album, artist, genre, year;
		dBase.GetSongTags(node->idLibrary, node->idPlaylist, filename, title, album, artist, genre, year, false);

		settings.SetLibraryType(false, (int)type);
		settings.SetLibraryArtist(false, L"");
		settings.SetLibraryAlbum(false, L"");
		if (type == SkinTreeNode::Type::Artist)
			settings.SetLibraryValue(true, artist);
		else if (type == SkinTreeNode::Type::Album)
			settings.SetLibraryValue(true, album);
		else if (type == SkinTreeNode::Type::Year)
			settings.SetLibraryValue(true, year);
		else if (type == SkinTreeNode::Type::Genre)
			settings.SetLibraryValue(true, genre);
	}
	else // Folder
	{
		std::wstring folder = PathEx::PathFromFile(node->GetFile());

		if (isPortableVersion && !folder.empty())
		{
			if (folder[1] == ':') // Only for drives
			{
				std::wstring folderDrive(1, folder[0]);
				std::wstring programDrive(1, programPath[0]);
				if (StringEx::ToLowerUS(folderDrive) == StringEx::ToLowerUS(programDrive))
					folder[0] = '?';
			}
		}

		settings.SetLibraryType(false, (int)type);
		settings.SetLibraryValue(true, folder);
		settings.SetLibraryArtist(false, L"");
		settings.SetLibraryAlbum(false, L"");
	}

	// !!! From FillList above

	if ((isMediaPlay || settings.GetLibraryType() == (int)SkinTreeNode::Type::Playlist) &&
		settings.IsNowPlaying() &&
		settings.GetLibraryType() != (int)SkinTreeNode::Type::Smartlist)
	{
		dBase.ClosePlaylist();
		dBase.ReturnNowPlaying();
		skinList->NowPlayingList();

		EndWaitCursor();
		UpdateStatusLine();
		return;
	}

	dBase.ClosePlaylist();

	switch ((SkinTreeNode::Type)settings.GetLibraryType())
	{
	case SkinTreeNode::Type::Album:
		dBase.FillListAlbum(skinList.get(), settings.GetLibraryValue());
		break;
	case SkinTreeNode::Type::Artist:
		dBase.FillListArtist(skinList.get(), settings.GetLibraryValue());
		break;
	case SkinTreeNode::Type::Year:
		dBase.FillListYear(skinList.get(), settings.GetLibraryValue());
		break;
	case SkinTreeNode::Type::Genre:
		dBase.FillListGenre(skinList.get(), settings.GetLibraryValue());
		break;
	case SkinTreeNode::Type::Folder:
		dBase.FillListFolder(skinList.get(), settings.GetLibraryValue());
		break;
	}

	EndWaitCursor();

	UpdateStatusLine();
}

void WinylWnd::FillListSearch()
{
	if (!skinEdit->IsSearchEmpty())
	{
		dBase.SetStopSearch(true);

		settings.SetLibraryNoneOld();

		dBase.ClosePlaylist();

		skinList->SetControlRedraw(false);

		isThreadSearch = true;
		SearchThreadStart();
	}
	else // Return previous view
	{
		if (threadSearch.IsJoinable())
		{
			dBase.SetStopSearch(true);
			threadSearch.Join();
			dBase.SetStopSearch(false);
		}

		FillList();
	}
}

void WinylWnd::SearchThreadStart()
{
	if (!threadSearch.IsRunning())
	{
		if (threadSearch.IsJoinable())
			threadSearch.Join();

		threadSearch.Start(std::bind(&WinylWnd::SearchThreadRun, this));
	}
}

void WinylWnd::SearchThreadRun()
{
	while (isThreadSearch)
	{
		isThreadSearch = false;
		int type = settings.GetSearchType();
		std::wstring text = skinEdit->GetSearchText();

		dBase.SetStopSearch(false);
		skinList->DeleteAllNode();

		if (!text.empty())
		{
			if (type == 0)
				dBase.FillListSearchAll(skinList.get(), text);
			else if (type == 1)
				dBase.FillListSearchTrack(skinList.get(), text);
			else if (type == 2)
				dBase.FillListSearchAlbum(skinList.get(), text);
			else if (type == 3)
				dBase.FillListSearchArtist(skinList.get(), text);

			if (!dBase.IsStopSearch())
				UpdateStatusLine();
		}
	}

	if (IsWnd()) ::PostMessageW(Wnd(), UWM_SEARCHDONE, 0, 0);
}

void WinylWnd::SearchThreadDone()
{
	if (isThreadSearch)
	{
		SearchThreadStart();
		return;
	}

	skinList->SetControlRedraw(true);
}

bool WinylWnd::CopySelectedToClipboard(bool isClipboard)
{
	if (skinList->GetSelectedSize() == 0)
		return true;

	if (!isClipboard)
	{
		MyDropSource* myDropSource = new MyDropSource();
		MyDropTarget* myDropTarget = new MyDropTarget();

		myDropTarget->SetDropSource(myDropSource);
		::RegisterDragDrop(thisWnd, myDropTarget);

		size_t bufSize = sizeof(DROPFILES) + sizeof(wchar_t);

		for (std::size_t i = 0, size = skinList->GetSelectedSize(); i < size; ++i)
			bufSize += skinList->GetSelectedAt(i)->GetFile().size() * sizeof(wchar_t) + sizeof(wchar_t);

		HGLOBAL hMem = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE, bufSize);
		LPVOID buf = ::GlobalLock(hMem);

		((DROPFILES*)buf)->pFiles = sizeof(DROPFILES);
		((DROPFILES*)buf)->fWide = TRUE;

		char* bufCursor = (char*)buf + sizeof(DROPFILES);

		for (std::size_t i = 0, size = skinList->GetSelectedSize(); i < size; ++i)
		{
			size_t sizeFile = skinList->GetSelectedAt(i)->GetFile().size() * sizeof(wchar_t) + sizeof(wchar_t);

			//memcpy(bufCursor, skinList->GetSelectedAt(i)->GetFile().c_str(), sizeFile);
			memcpy_s(bufCursor, sizeFile, skinList->GetSelectedAt(i)->GetFile().c_str(), sizeFile);
			bufCursor += sizeFile;
		}
		memset(bufCursor, 0, sizeof(wchar_t));
		
		::GlobalUnlock(hMem);

		MyDataObject* myDataObject = new MyDataObject();
		myDataObject->hGlobal = hMem;

		DWORD dwEffect;
		::DoDragDrop(myDataObject, myDropSource, DROPEFFECT_COPY, &dwEffect);

		::RevokeDragDrop(thisWnd);

		bool isStopDrag = myDropSource->IsStopDrag();

		myDropTarget->Release(); // Self delete
		myDropSource->Release(); // Self delete
		myDataObject->Release(); // Self delete

		::GlobalFree(hMem);

		if (isStopDrag)
			return false;

		return true;
	}
	else
	{
		// http://blogs.msdn.com/b/oldnewthing/archive/2008/05/27/8553638.aspx

		if (!::OpenClipboard(thisWnd))
			return false;
		if (!::EmptyClipboard())
		{
			::CloseClipboard();
			return false;
		}

		// Copy files to clipboard

		size_t bufSize = sizeof(DROPFILES) + sizeof(wchar_t);

		for (std::size_t i = 0, size = skinList->GetSelectedSize(); i < size; ++i)
			bufSize += skinList->GetSelectedAt(i)->GetFile().size() * sizeof(wchar_t) + sizeof(wchar_t);

		HGLOBAL hMem = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE, bufSize);
		LPVOID buf = ::GlobalLock(hMem);

		((DROPFILES*)buf)->pFiles = sizeof(DROPFILES);
		((DROPFILES*)buf)->fWide = TRUE;

		char* bufCursor = (char*)buf + sizeof(DROPFILES);

		for (std::size_t i = 0, size = skinList->GetSelectedSize(); i < size; ++i)
		{
			size_t sizeFile = skinList->GetSelectedAt(i)->GetFile().size() * sizeof(wchar_t) + sizeof(wchar_t);

			//memcpy(bufCursor, skinList->GetSelectedAt(i)->GetFile().c_str(), sizeFile);
			memcpy_s(bufCursor, sizeFile, skinList->GetSelectedAt(i)->GetFile().c_str(), sizeFile);
			bufCursor += sizeFile;
		}
		memset(bufCursor, 0, sizeof(wchar_t));
		
		::GlobalUnlock(hMem);

		::SetClipboardData(CF_HDROP, hMem);

		// Also copy focused track Artist - Title to clipboard
		HGLOBAL hMemText = NULL;
		if (skinList->GetFocusNode())
		{
			std::wstring filename, title, album, artist, genre, year;

			if (!skinList->IsRadio())
			{
				dBase.GetSongTags(skinList->GetFocusNode()->idLibrary, skinList->GetFocusNode()->idPlaylist,
								 filename, title, album, artist, genre, year, false);
			}
			else
			{
				if (skinList->GetFocusNode() == skinList->GetPlayNode())
					libAudio.GetRadioTags(title, artist, album);
			}

			if (title.empty())
				title = filename;

			if (!title.empty())
			{
				std::wstring artistTitle = artist.empty() ? title : artist + L" - " + title;

				size_t bufSizeText = artistTitle.size() * sizeof(wchar_t) + sizeof(wchar_t);

				hMemText = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE, bufSizeText);
				LPVOID bufText = ::GlobalLock(hMemText);

				//memcpy(bufText, artistTitle.c_str(), bufSizeText);
				memcpy_s(bufText, bufSizeText, artistTitle.c_str(), bufSizeText);

				::GlobalUnlock(hMemText);

				::SetClipboardData(CF_UNICODETEXT, hMemText);
			}
		}

		::CloseClipboard();

		::GlobalFree(hMem);
		if (hMemText)
			::GlobalFree(hMemText);

		return true;
	}
}

void WinylWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// "Add to Playlist" menu item
	if (LOWORD(wParam) >= 10000 && LOWORD(wParam) <= 12000)
	{
		std::wstring file; // Playlist db file

		if (LOWORD(wParam) == 10000) // "To New Playlist" menu item
		{
			TreeNodeUnsafe node = dBase.CreatePlaylist(skinTree.get(), lang.GetLineS(Lang::DefaultList, 0) +
				L" " + StringCurrentDate());

			if (node)
				file = node->GetValue();
		}
		else if (LOWORD(wParam) == 10001) // "To Now Playing" menu item
		{
			if (settings.GetNowPlayingType() == (int)SkinTreeNode::Type::Playlist)
				file = settings.GetNowPlayingValue();
		}
		else
		{
			// Let's find out to which playlist we add tracks
			TreeNodeUnsafe plsNode = skinTree->GetPlaylistNode();
			if (plsNode->HasChild())
			{
				int index = LOWORD(wParam) - 10002;

				TreeNodeUnsafe treeNode = plsNode->ChildByIndex(index);
				if (treeNode)
					file = treeNode->GetValue();
			}
		}

		// Add to the playlist
		if (!file.empty())
		{
			BeginWaitCursor();

			int start = 0;

			if (dBase.IsPlaylistOpen())
				start = dBase.FromPlaylistToPlaylist(skinList.get(), file);
			else if (skinList->IsRadio())
				start = dBase.FromRadioToPlaylist(skinList.get(), file);
			else
				start = dBase.FromLibraryToPlaylist(skinList.get(), file);

			if (settings.IsLibraryNowPlaying())
			{
				// If add to a now playing playlist then need to fill the playlist right away
				if (dBase.IsNowPlaying() && !dBase.IsNowPlayingOpen())
				{
					if (settings.GetNowPlayingType() == (int)SkinTreeNode::Type::Playlist &&
						settings.GetNowPlayingValue() == file)
						dBase.FillPlaylistNowPlaying(skinList.get(), settings.GetNowPlayingValue(), start);
				}
			}

			EndWaitCursor();
		}

		return;
	}

	switch(LOWORD(wParam))
	{
	case ID_MENU_LIBRARY:
	case ID_KEY_F3:
		DialogLibrary();
		break;
	case ID_MENU_SKIN:
	case ID_KEY_F4:
		{
			EnableAll(false);
			DlgSkin dlg;
			dlg.SetLanguage(&lang);
			dlg.SetProgramPath(programPath);
			dlg.SetCurrentSkin(settings.GetSkin(), settings.IsSkinPack());
			dlg.ModalDialog(thisWnd, IDD_DLGSKIN);
			EnableAll(true);
		}
		break;
	case ID_MENU_LANGUAGE:
		DialogLanguage();
		break;
	case ID_MENU_PROPERTIES:
		DialogProperties();
		break;
	case ID_MENU_HOTKEY:
	case ID_KEY_F7:
		{
			EnableAll(false);
			hotKeys.UnregisterHotKeys(thisWnd);
			DlgHotKeys dlg;
			dlg.SetLanguage(&lang);
			dlg.SetHotKeys(&hotKeys);
			dlg.ModalDialog(thisWnd, IDD_DLGHOTKEYS);
			hotKeys.RegisterHotKeys(thisWnd);
			EnableAll(true);
		}
		break;
	case ID_MENU_ABOUT:
		{
			EnableAll(false);
			DlgAbout dlg;
			dlg.SetLanguage(&lang);
			dlg.ModalDialog(thisWnd, IDD_DLGABOUT);
			EnableAll(true);
		}
		break;
	case ID_MENU_EQUALIZER:
	case ID_KEY_F6:
		DialogEqualizer();
		break;
	case ID_MENU_STOP:
	case ID_KEY_CTRL_SPACE:
		ActionStop();
		break;
	case ID_MENU_NEXT:
	case ID_KEY_CTRL_RIGHT:
		ActionNextTrack();
		break;
	case ID_MENU_PREV:
	case ID_KEY_CTRL_LEFT:
		ActionPrevTrack();
		break;
	case ID_MENU_PAUSE:
	case ID_KEY_SPACE:
		ActionPauseEx();
		break;
	case ID_MENU_PLAY:
		ActionPlayEx();
		break;
	case ID_MENU_LIST_PLAY:
	case ID_KEY_ENTER:
		ActionPlay(true);
		break;
	case ID_MENU_LIST_REPEAT:
		ActionPlay(true, false, true);
		break;
	case ID_MENU_EXIT:
		if (dlgProgressPtr)
			dlgProgressPtr->DestroyOnStop();
		else
			::DestroyWindow(thisWnd);
		break;
	case ID_MENU_HIDETOTRAY:
		settings.SetHideToTray(!settings.IsHideToTray());
		trayIcon.SetHideToTray(settings.IsHideToTray());
		contextMenu.CheckHideToTray(settings.IsHideToTray());
		break;
	case ID_MENU_MUTE:
	case ID_KEY_MUTE:
		ActionMute(!settings.IsMute());
		break;
	case ID_MENU_SHUFFLE:
	case ID_KEY_SHUFFLE:
		ActionShuffle(!settings.IsShuffle());
		break;
	case ID_MENU_REPEAT:
	case ID_KEY_REPEAT:
		ActionRepeat(!settings.IsRepeat());
		break;
	case ID_MENU_MINI:
	case ID_KEY_F5:
		MiniPlayer(!settings.IsMiniPlayer());
		break;
	case ID_MENU_PL_NEW:
		NewPlaylist(false);
		break;
	case ID_MENU_PL_DELETE:
		DeletePlaylist(false);
		break;
	case ID_MENU_PL_RENAME:
		RenamePlaylist(false);
		break;
	case ID_MENU_SM_NEW:
		NewPlaylist(true);
		break;
	case ID_MENU_SM_DELETE:
		DeletePlaylist(true);
		break;
	case ID_MENU_SM_RENAME:
		RenamePlaylist(true);
		break;
	case ID_MENU_SM_EDIT:
		EditSmartlist();
		break;
	case ID_MENU_OPEN_URL:
	case ID_KEY_PL_URL:
		AddURL();
		break;
	case ID_MENU_DELETE:
	case ID_KEY_DELETE:
		if (::GetFocus() == skinList->Wnd())
		{
			BeginWaitCursor();
			if (dBase.IsPlaylistOpen())
				dBase.DeleteFromPlaylist(skinList.get());
			else if (dBase.IsSmartlistOpen())
				dBase.DeleteFromSmartlist(skinList.get(), settings.GetLibraryValue());
			else
			{
				dBase.DeleteFromLibrary(skinList.get(), settings.IsAddAllToLibrary() ? &libraryFolders : nullptr);
				skinTree->ClearLibrary();
			}
			EndWaitCursor();

			UpdateStatusLine();

			if (isMediaPlay && skinList->GetPlayNode() == nullptr)
				ActionStop();
		}
		break;
	case ID_MENU_OPEN_FILE:
	case ID_KEY_PL_FILE:
		AddFileFolder(false);
		break;
	case ID_MENU_OPEN_FOLDER:
	case ID_KEY_PL_FOLDER:
		AddFileFolder(true);
		break;
	case ID_MENU_OPEN_PLAYLIST:
		ImportPlaylist();
		break;
	case ID_MENU_SELECT_ALL:
	case ID_KEY_SELECT_ALL:
		skinList->SelectAll();
		UpdateStatusLine();
		break;
	case ID_MENU_POPUP:
		settings.SetPopup(!settings.IsPopup());
		contextMenu.CheckPopup(settings.IsPopup());
		break;
	case ID_MENU_POPUP_CONFIG:
		DialogConfig(2);
		break;
	case ID_MENU_POPUP_DISABLE:
		if (skinPopup)
		{
			skinPopup->Disable();
		}
		settings.SetPopup(false);
		contextMenu.CheckPopup(false);
		break;
	case ID_MENU_POPUP_TL:
		if (skinPopup)
		{
			skinPopup->SetPosition(0);
			skinPopup->Popup();
		}
		settings.SetPopupPosition(0);
		contextMenu.CheckPopupPosition(0);
		break;
	case ID_MENU_POPUP_TR:
		if (skinPopup)
		{
			skinPopup->SetPosition(1);
			skinPopup->Popup();
		}
		settings.SetPopupPosition(1);
		contextMenu.CheckPopupPosition(1);
		break;
	case ID_MENU_POPUP_BL:
		if (skinPopup)
		{
			skinPopup->SetPosition(2);
			skinPopup->Popup();
		}
		settings.SetPopupPosition(2);
		contextMenu.CheckPopupPosition(2);
		break;
	case ID_MENU_POPUP_BR:
		if (skinPopup)
		{
			skinPopup->SetPosition(3);
			skinPopup->Popup();
		}
		settings.SetPopupPosition(3);
		contextMenu.CheckPopupPosition(3);
		break;
	case ID_MENU_EFFECT:
		settings.SetEffect(!settings.IsEffect());
		contextMenu.CheckEffect(settings.IsEffect());
		skinDraw.EnableFade(settings.IsEffect());
		if (skinAlpha)
			skinAlpha->skinDraw.EnableFade(settings.IsEffect());
		if (skinMini)
			skinMini->skinDraw.EnableFade(settings.IsEffect());
		break;
	case ID_MENU_SMOOTH:
		settings.SetSmoothScroll(!settings.IsSmoothScroll());
		contextMenu.CheckSmoothScroll(settings.IsSmoothScroll());
		skinList->EnableSmoothScroll(settings.IsSmoothScroll());
		skinTree->EnableSmoothScroll(settings.IsSmoothScroll());
		skinLyrics->EnableSmoothScroll(settings.IsSmoothScroll());
		if (settings.IsSmoothScroll())
		{
			isTimerSmooth = false;
			eventTimerSmooth.Reset();
			threadTimerSmooth.Start(std::bind(&WinylWnd::TimerEffectsThreadRun, this));
		}
		else
		{
			if (threadTimerSmooth.IsJoinable())
			{
				isTimerSmooth = true;
				eventTimerSmooth.Set();
				threadTimerSmooth.Join();
			}
		}
	case ID_MENU_SEARCH_ALL:
		settings.SetSearchType(0);
		contextMenu.CheckSearch(0);
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 0));
		if (!skinEdit->IsSearchEmpty())
			FillListSearch();
		break;
	case ID_MENU_SEARCH_TRACK:
		settings.SetSearchType(1);
		contextMenu.CheckSearch(1);
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 2));
		if (!skinEdit->IsSearchEmpty())
			FillListSearch();
		break;
	case ID_MENU_SEARCH_ALBUM:
		settings.SetSearchType(2);
		contextMenu.CheckSearch(2);
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 3));
		if (!skinEdit->IsSearchEmpty())
			FillListSearch();
		break;
	case ID_MENU_SEARCH_ARTIST:
		settings.SetSearchType(3);
		contextMenu.CheckSearch(3);
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 4));
		if (!skinEdit->IsSearchEmpty())
			FillListSearch();
		break;
	case ID_MENU_JUMPTO_ARTIST:
		FillJump(skinList->GetFocusNode(), SkinTreeNode::Type::Artist);
		break;
	case ID_MENU_JUMPTO_ALBUM:
		FillJump(skinList->GetFocusNode(), SkinTreeNode::Type::Album);
		break;
	case ID_MENU_JUMPTO_YEAR:
		FillJump(skinList->GetFocusNode(), SkinTreeNode::Type::Year);
		break;
	case ID_MENU_JUMPTO_GENRE:
		FillJump(skinList->GetFocusNode(), SkinTreeNode::Type::Genre);
		break;
	case ID_MENU_JUMPTO_FOLDER:
		FillJump(skinList->GetFocusNode(), SkinTreeNode::Type::Folder);
		break;
	case ID_MENU_LYRICS_ALIGN_CENTER:
		settings.SetLyricsAlign(0);
		skinLyrics->SetAlign(settings.GetLyricsAlign());
		contextMenu.CheckLyricsAlign(settings.GetLyricsAlign());
		break;
	case ID_MENU_LYRICS_ALIGN_LEFT:
		settings.SetLyricsAlign(1);
		skinLyrics->SetAlign(settings.GetLyricsAlign());
		contextMenu.CheckLyricsAlign(settings.GetLyricsAlign());
		break;
	case ID_MENU_LYRICS_ALIGN_RIGHT:
		settings.SetLyricsAlign(2);
		skinLyrics->SetAlign(settings.GetLyricsAlign());
		contextMenu.CheckLyricsAlign(settings.GetLyricsAlign());
		break;
	case ID_MENU_LYRICS_FONT_DEFAULT:
		settings.SetLyricsFontSize(0);
		settings.SetLyricsFontBold(skinLyrics->GetFontBoldDefault());
		skinLyrics->SetFontSize(settings.GetLyricsFontSize(), settings.GetLyricsFontBold());
		contextMenu.CheckLyricsFontSize(settings.GetLyricsFontSize());
		contextMenu.CheckLyricsFontBold(settings.GetLyricsFontBold());
		break;
	case ID_MENU_LYRICS_FONT_SMALL:
		settings.SetLyricsFontSize(1);
		skinLyrics->SetFontSize(settings.GetLyricsFontSize(), settings.GetLyricsFontBold());
		contextMenu.CheckLyricsFontSize(settings.GetLyricsFontSize());
		break;
	case ID_MENU_LYRICS_FONT_MEDIUM:
		settings.SetLyricsFontSize(2);
		skinLyrics->SetFontSize(settings.GetLyricsFontSize(), settings.GetLyricsFontBold());
		contextMenu.CheckLyricsFontSize(settings.GetLyricsFontSize());
		break;
	case ID_MENU_LYRICS_FONT_LARGE:
		settings.SetLyricsFontSize(3);
		skinLyrics->SetFontSize(settings.GetLyricsFontSize(), settings.GetLyricsFontBold());
		contextMenu.CheckLyricsFontSize(settings.GetLyricsFontSize());
		break;
	case ID_MENU_LYRICS_FONT_LARGEST:
		settings.SetLyricsFontSize(4);
		skinLyrics->SetFontSize(settings.GetLyricsFontSize(), settings.GetLyricsFontBold());
		contextMenu.CheckLyricsFontSize(settings.GetLyricsFontSize());
		break;
	case ID_MENU_LYRICS_FONT_BOLD:
		settings.SetLyricsFontBold(!settings.GetLyricsFontBold());
		skinLyrics->SetFontSize(settings.GetLyricsFontSize(), settings.GetLyricsFontBold());
		contextMenu.CheckLyricsFontBold(settings.GetLyricsFontBold());
		break;
	case ID_MENU_LYRICS_PROV_OFF:
		settings.SetLyricsProviderOff(true);
		settings.SetLyricsProvider(L"");
		contextMenu.CheckLyricsProvider(-1);
		break;
	case ID_MENU_LYRICS_PROV_1:
		settings.SetLyricsProviderOff(false);
		settings.SetLyricsProvider(L"");
		contextMenu.CheckLyricsProvider(0);
		if (isMediaPlay && !isMediaRadio)
		{
			isThreadLyrics = true;
			LyricsThreadStart();
		}
		break;
	case ID_MENU_LYRICS_PROV_2:
	case ID_MENU_LYRICS_PROV_3:
	case ID_MENU_LYRICS_PROV_4:
	case ID_MENU_LYRICS_PROV_5:
	case ID_MENU_LYRICS_PROV_6:
	case ID_MENU_LYRICS_PROV_7:
	case ID_MENU_LYRICS_PROV_8:
	case ID_MENU_LYRICS_PROV_9:
	case ID_MENU_LYRICS_PROV_10:
		settings.SetLyricsProviderOff(false);
		settings.SetLyricsProvider(LyricsLoader::GetLyricsProvider(LOWORD(wParam) - ID_MENU_LYRICS_PROV_1));
		contextMenu.CheckLyricsProvider(LOWORD(wParam) - ID_MENU_LYRICS_PROV_1);
		if (isMediaPlay && !isMediaRadio)
		{
			isThreadLyrics = true;
			LyricsThreadStart();
		}
		break;
	case ID_MENU_LYRICS_GOOGLE:
		if (isMediaPlay && !isMediaRadio)
		{
			if (!lyricsArtist.empty() && !lyricsTitle.empty())
			{
				//std::wstring url = L"https://google.com/search?q=%22" + lyricsArtist + L"%22+%22" + lyricsTitle + L"%22+lyrics";
				std::wstring url = L"https://google.com/search?q=" + lyricsArtist + L"+" + lyricsTitle + L"+lyrics";
				::ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}
		break;
	case ID_MENU_LYRICS_SAVE:
		DialogProperties(true);
		break;
	case ID_MENU_LYRICS_COPY:
		if (::OpenClipboard(thisWnd))
		{
			if (::EmptyClipboard())
			{
				std::wstring lyrics = skinLyrics->GetLyrics();

				size_t bufSizeText = lyrics.size() * sizeof(wchar_t) + sizeof(wchar_t);

				HGLOBAL hMemText = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE, bufSizeText);
				if (hMemText)
				{
					LPVOID bufText = ::GlobalLock(hMemText);

					//memcpy(bufText, lyrics.c_str(), bufSizeText);
					memcpy_s(bufText, bufSizeText, lyrics.c_str(), bufSizeText);

					::GlobalUnlock(hMemText);

					::SetClipboardData(CF_UNICODETEXT, hMemText);
					::GlobalFree(hMemText);
				}
			}
			::CloseClipboard();
		}
		break;
	case ID_MENU_FILELOCATION:
		{
			ListNodeUnsafe focusNode = skinList->GetFocusNode();
			if (focusNode)
			{
				if (focusNode->GetFile().size() > 2 && focusNode->GetFile()[1] == ':' &&
					FileSystem::Exists(focusNode->GetFile()))
				{
					LPITEMIDLIST pidl = NULL;
					SFGAOF attrIn = SFGAO_FILESYSTEM;
					SFGAOF attrOut = 0;
					::SHParseDisplayName(focusNode->GetFile().c_str(), NULL, &pidl, attrIn, &attrOut);
					if (pidl)
					{
						if (attrOut & SFGAO_FILESYSTEM) // Additional fix for the url bug
							::SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
						::CoTaskMemFree(pidl);
					}
				}
			}
		}
		break;
	case ID_MENU_AUDIOLOG:
	{
		bool visible = !audioLogWnd.IsVisible();
		audioLogWnd.Show(visible);
		contextMenu.CheckAudioLog(visible);
	}
	break;
	case ID_MENU_CONFIG:
	case ID_KEY_F8:
		DialogConfig();
		break;
	case ID_MENU_SM_UPDATE:
		if (skinTree->GetFocusNode())
		{
			BeginWaitCursor();
			dBase.FillSmartlist(skinList.get(), skinTree->GetFocusNode()->GetValue(), true);
			EndWaitCursor();
			UpdateStatusLine();
		}
		break;
	case ID_KEY_CTRL_C:
	case ID_KEY_CTRL_INS:
		if (::GetFocus() == skinList->Wnd())
			CopySelectedToClipboard(true);
		break;
	case ID_KEY_CTRL_F:
		skinEdit->SetFocus();
		break;
	}
}
void WinylWnd::NewPlaylist(bool isSmartlist)
{
	if (!isSmartlist)
	{
		TreeNodeUnsafe node = dBase.CreatePlaylist(skinTree.get(), lang.GetLineS(Lang::DefaultList, 0) + L" " + StringCurrentDate());
		skinTree->ScrollToMyNode(node);
	}
	else
	{
		EnableAll(false);
		DlgSmart dlg;
		dlg.SetLanguage(&lang);
		dlg.SetName(lang.GetLineS(Lang::DefaultList, 1) + L" " + StringCurrentDate());
		if (dlg.ModalDialog(thisWnd, IDD_DLGSMART) == IDOK)
		{
			BeginWaitCursor();
			TreeNodeUnsafe node = dBase.CreateSmartlist(skinTree.get(), dlg.GetName(), dlg.GetSmartlist());
			if (node)
			{
				skinTree->SetFocusNode(node);
				skinTree->ScrollToFocusNode();
				dBase.FillSmartlist(skinList.get(), skinTree->GetFocusNode()->GetValue(), true);
				UpdateStatusLine();
			}
			EndWaitCursor();
		}
		EnableAll(true);
	}
}

void WinylWnd::DeletePlaylist(bool isSmartlist)
{
	if (skinTree->GetFocusNode() == nullptr)
		return;

	if (!isSmartlist)
	{
		if (!settings.IsLibraryNowPlaying())
		{
			// If delete the current playlist
			if (settings.GetLibraryType() == (int)SkinTreeNode::Type::Playlist &&
				settings.GetLibraryValue() == skinTree->GetFocusNode()->GetValue())
			{
				dBase.ClosePlaylist();
				dBase.CloseNowPlaying();
			}
		}
		else
		{
			// If delete the current playlist
			if (settings.GetLibraryType() == (int)SkinTreeNode::Type::Playlist &&
				settings.GetLibraryValue() == skinTree->GetFocusNode()->GetValue())
			{
				dBase.ClosePlaylist();
			}
			// If delete the now playing playlist
			if (dBase.IsNowPlaying())
			{
				if (settings.GetNowPlayingType() == (int)SkinTreeNode::Type::Playlist &&
					settings.GetNowPlayingValue() == skinTree->GetFocusNode()->GetValue())
				{
					dBase.CloseNowPlaying();
					skinList->DeleteNowPlaying();
					settings.EmptyNowPlaying(); // Needed or delete the default playlist cause a bug
					ActionStop();
				}
			}
		}

		// Delete playlist
		dBase.DeletePlaylist(skinTree.get());
	}
	else
	{
		// Delete smartlist
		dBase.DeleteTreeSmartlist(skinTree.get());
	}

	// Needed or delete the last playlist cause a bug
	settings.EmptyLibrary();

	// Choose the next playlist or clear the list if latest
	if (skinTree->GetFocusNode())
		FillList(skinTree->GetFocusNode());
	else
	{
		skinList->SetControlRedraw(false);
		skinList->DeleteAllNode();
		skinList->SetControlRedraw(true);
		UpdateStatusLine();
	}
}

void WinylWnd::RenamePlaylist(bool isSmartlist)
{
	if (skinTree->GetFocusNode() == nullptr)
		return;

	EnableAll(false);

	DlgRename dlg;
	dlg.SetLanguage(&lang);
	dlg.SetName(skinTree->GetNodeTitle(skinTree->GetFocusNode()));
	if (dlg.ModalDialog(thisWnd, IDD_DLGRENAME) == IDOK)
	{
		skinTree->SetNodeTitle(skinTree->GetFocusNode(), dlg.GetName());

		if (!isSmartlist)
			dBase.SavePlaylist(skinTree.get());
		else
			dBase.SaveTreeSmartlists(skinTree.get());
	}

	EnableAll(true);
}

void WinylWnd::EditSmartlist()
{
	if (skinTree->GetFocusNode())
	{
		EnableAll(false);
		DlgSmart dlg;
		dlg.SetLanguage(&lang);
		dlg.SetName(skinTree->GetNodeTitle(skinTree->GetFocusNode()));
		dBase.OpenSmartlist(skinTree->GetFocusNode()->GetValue(), dlg.GetSmartlist());
		if (dlg.ModalDialog(thisWnd, IDD_DLGSMART) == IDOK)
		{
			BeginWaitCursor();
			dBase.SaveSmartlist(skinTree->GetFocusNode()->GetValue(), dlg.GetSmartlist());
			dBase.FillSmartlist(skinList.get(), skinTree->GetFocusNode()->GetValue(), true);
			
			skinTree->SetNodeTitle(skinTree->GetFocusNode(), dlg.GetName());
			dBase.SaveTreeSmartlists(skinTree.get());
			UpdateStatusLine();
			EndWaitCursor();
		}
		EnableAll(true);
	}
}

void WinylWnd::DropFilesToPlaylist()
{
	// Drop to a selected playlist

	if (skinTree->GetDropNode()) // Only if drop from the player
	{
		TreeNodeUnsafe dropNode = skinTree->GetDropNode();
		std::wstring file;

		// Drop to "Now Playing"
		if (dropNode->GetNodeType() == SkinTreeNode::NodeType::Head && dropNode->GetType() == SkinTreeNode::Type::NowPlaying)
		{
			if (settings.GetNowPlayingType() == (int)SkinTreeNode::Type::Playlist)
				file = settings.GetNowPlayingValue();
		}
		else // Drop to a playlist
		{
			// Cannot drop to the same playlist
			if (!(dBase.IsPlaylistOpen() && settings.GetLibraryValue() == dropNode->GetValue()))
				file = dropNode->GetValue();
		}

		skinTree->SetDropPoint(nullptr);

		if (file.empty())
			return;

		BeginWaitCursor();

		int start = 0;

		if (dBase.IsPlaylistOpen())
			start = dBase.FromPlaylistToPlaylist(skinList.get(), file);
		else if (skinList->IsRadio())
			start = dBase.FromRadioToPlaylist(skinList.get(), file);
		else
			start = dBase.FromLibraryToPlaylist(skinList.get(), file);

		if (settings.IsLibraryNowPlaying())
		{
			// If drop to a now playing playlist then need to fill the playlist right away
			if (dBase.IsNowPlaying() && !dBase.IsNowPlayingOpen())
			{
				if (settings.GetNowPlayingType() == (int)SkinTreeNode::Type::Playlist &&
					settings.GetNowPlayingValue() == file)
					dBase.FillPlaylistNowPlaying(skinList.get(), settings.GetNowPlayingValue(), start);
			}
		}

		EndWaitCursor();
	}
}

void WinylWnd::OnDropFiles(HDROP hDropInfo)
{
	if (dlgProgressPtr) // Do not add if the library is scanning
		return;

	// Drop to the current open playlist

	std::vector<std::wstring> files;

	UINT numberFiles = ::DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);					
	for (UINT i = 0; i < numberFiles; ++i)
	{
		wchar_t file[MAX_PATH * 16];
		if (::DragQueryFile(hDropInfo, i, file, sizeof(file) / sizeof(wchar_t)))
			files.push_back(file);
	}
	::DragFinish(hDropInfo);

	bool isImportPlaylist = DropIsPlaylist(files);
	bool isDefaultPlaylist = false;

	// Playlist is closed then drop to the default playlist
	if (!dBase.IsPlaylistOpen() && !isImportPlaylist)
	{
		DropNewDefault();
		isDefaultPlaylist = true;
	}

	if (!isImportPlaylist)
		DropAddFiles(files, false, isDefaultPlaylist);
	else
		DropAddPlaylist(files, false);
}

bool WinylWnd::DropIsPlaylist(const std::vector<std::wstring>& files)
{
	if (files.size() == 1)
	{
		std::wstring ext = PathEx::ExtFromFile(files[0]);

		if (ext == L"m3u" || ext == L"m3u8" || ext == L"pls" || ext == L"asx" || ext == L"xspf")
		{
			return true;
		}
	}

	return false;
}

void WinylWnd::DropNewDefault()
{
	// If there is no default playlist then create one
	if (skinTree->GetDefPlaylistNode() == nullptr)
		dBase.CreatePlaylist(skinTree.get(), lang.GetLineS(Lang::DefaultList, 2), true);

	if (skinTree->GetDefPlaylistNode())
	{
		// Choose the default playlist if it's not choosen
		if (settings.GetLibraryNoneOld() ||
			!(settings.GetLibraryType() == (int)SkinTreeNode::Type::Playlist &&
			settings.GetLibraryValue() == L"default") || !pointerOpenFiles->empty())
			FillList(skinTree->GetDefPlaylistNode());
	}
}

void WinylWnd::DropAddFiles(const std::vector<std::wstring>& files, bool isPlay, bool isDefaultPlaylist)
{
	assert(!dlgProgressPtr);
	assert(settings.GetLibraryType() == (int)SkinTreeNode::Type::Playlist);
	if (settings.GetLibraryType() != (int)SkinTreeNode::Type::Playlist)
		return;

	skinList->RemoveSelection();
	if (settings.IsPlayFocus())
		skinList->EnablePlayFocus(false);

	// Add files from this position
	int start = dBase.GetPlaylistMax();

	bool isFolder = false;

	// If only 1 file then try to add it fast
	if (files.size() == 1)
	{
		Progress progress;
		progress.SetDataBase(&dBase);
		progress.SetPortableVersion(isPortableVersion);
		progress.SetProgramPath(programPath);
		progress.SetAddAllToLibrary(settings.IsAddAllToLibrary());
		
		if (progress.FastAddFileToPlaylist(files[0], start, isFolder))
		{
			dBase.FillPlaylistOpenFile(skinList.get(), settings.GetLibraryValue(), start);
		}
	}

	// Multiple files or folder
	if (isFolder || files.size() > 1)
	{
		EnableAll(false);

		DlgProgress dlgProgress;
		dlgProgress.SetLanguage(&lang);
		dlgProgress.progress.SetDataBase(&dBase);
		dlgProgress.progress.SetPortableVersion(isPortableVersion);
		dlgProgress.progress.SetProgramPath(programPath);
		dlgProgress.progress.SetPlaylist(start, settings.GetLibraryValue());
		dlgProgress.progress.SetLibraryFolders(files);
		dlgProgress.progress.SetAddAllToLibrary(settings.IsAddAllToLibrary());
		dlgProgress.ModalDialog(thisWnd, IDD_DLGPROGRESS);

		EnableAll(true);

		BeginWaitCursor();
		dBase.FillPlaylistOpenFile(skinList.get(), settings.GetLibraryValue(), start);
		EndWaitCursor();
	}

	if (isPlay) // ActionPlay
	{
		if (skinList->GetSelectedSize() > 0)
		{
			ListNodeUnsafe node = skinList->GetSelectedAt(0); // Get first selected

			skinList->SetFocusNode(node, false); // Focus it

			skinList->ScrollToMyNode(node); // Scroll to it

			ActionPlay(true); // Play
		}
	}
	else
	{
		// Scroll to the area of the added tracks
		if (skinList->GetSelectedSize() > 0)
			skinList->ScrollToMyNode(skinList->GetSelectedAt(0));
	}

	if (settings.IsPlayFocus())
		skinList->EnablePlayFocus(true);

	if (settings.IsAddAllToLibrary())
	{
		skinTree->ClearLibrary();
	}

	if (isDefaultPlaylist && skinTree->GetDefPlaylistNode())
	{
		skinTree->SetFocusNode(skinTree->GetDefPlaylistNode());
		skinTree->ScrollToFocusNode();
	}

	UpdateStatusLine();
}

void WinylWnd::DropAddPlaylist(const std::vector<std::wstring>& files, bool isPlay)
{
	assert(!dlgProgressPtr);

	std::wstring file;
	if (files.size() == 1)
		file = files[0];
	if (file.empty())
		return;

	std::wstring name = PathEx::NameFromPath(file);

	TreeNodeUnsafe node = dBase.CreatePlaylist(skinTree.get(), name);

	if (node == nullptr)
		return;

	EnableAll(false);

	DlgProgress dlgProgress;
	dlgProgress.SetLanguage(&lang);
	dlgProgress.progress.SetDataBase(&dBase);
	dlgProgress.progress.SetPortableVersion(isPortableVersion);
	dlgProgress.progress.SetProgramPath(programPath);
	dlgProgress.progress.SetNewPlaylist(file, node->GetValue());
	dlgProgress.progress.SetAddAllToLibrary(settings.IsAddAllToLibrary());
	dlgProgress.ModalDialog(thisWnd, IDD_DLGPROGRESS);

	EnableAll(true);

	BeginWaitCursor();
	dBase.ClosePlaylist();
	dBase.OpenPlaylist(node->GetValue());
	dBase.FillPlaylist(skinList.get(), node->GetValue());
	skinList->ScrollToFocusNode();
	EndWaitCursor();

	skinTree->SetFocusNode(node);
	skinTree->ScrollToFocusNode();
	FillList(node);

	if (isPlay)
	{
		// Will be a crash without this line if a track is playing
		skinList->DeleteNowPlaying();
		ActionPlay(true);
	}

	if (settings.IsAddAllToLibrary())
	{
		skinTree->ClearLibrary();
	}

	if (node)
	{
		skinTree->SetFocusNode(node);
		skinTree->ScrollToFocusNode();
	}
}

void WinylWnd::OnCopyData(HWND hWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if (dlgProgressPtr) // Do not add if the library is scanning
		return;

	std::wstring file = (const wchar_t*)pCopyDataStruct->lpData;
		
	std::vector<std::wstring> files;
	DropParseFiles(file, files);

	bool isImportPlaylist = DropIsPlaylist(files);
	bool isDefaultPlaylist = false;
	if (!isImportPlaylist)
	{
		DropNewDefault();
		isDefaultPlaylist = true;
	}

	if (!isImportPlaylist)
	{
		if (pCopyDataStruct->dwData == 1) // The files came from the command line
			DropAddFiles(files, true /*!isMediaPlay*/, isDefaultPlaylist);
		else if (pCopyDataStruct->dwData == 2) // The files came from Windows Shell
			DropAddFiles(files, true, isDefaultPlaylist);
	}
	else
	{
		if (pCopyDataStruct->dwData == 1) // The files came from the command line
			DropAddPlaylist(files, true /*!isMediaPlay*/);
		else if (pCopyDataStruct->dwData == 2) // The files came from Windows Shell
			DropAddPlaylist(files, true);
	}
}

void WinylWnd::DropParseFiles(const std::wstring& file, std::vector<std::wstring>& files)
{
	// Split the line by '|'

	std::size_t start = 0;
	std::size_t find = 0;
	while ((find = file.find('|', find)) != std::string::npos)
	{
		files.push_back(file.substr(start, find - start));
		++find;
		start = find;
	}

	files.push_back(file.substr(start));
}

void WinylWnd::AddFileFolder(bool isFolder)
{
	if (dlgProgressPtr) // Do not add if library scanning
		return;

	std::vector<std::wstring> files;
	bool result = true;

	if (isFolder)
	{
		EnableAll(false);

		FileDialogEx fileDialog;
		fileDialog.SetFolderTitleXP(lang.GetLineS(Lang::LibraryDialog, 7));

		if (fileDialog.DoModalFolder(thisWnd, true))
		{
			// Check the file despite of isMulti flag because WinXP doesn't support multiple folders
			if (!fileDialog.GetFile().empty())
				files.push_back(fileDialog.GetFile());
			else
			{
				for (int i = 0; i < fileDialog.GetMultiCount(); ++i)
					files.push_back(fileDialog.GetMultiFile(i));
			}
		}
		else
			result = false;

		EnableAll(true);
	}
	else
	{
		EnableAll(false);

		FileDialogEx fileDialog;

		FileDialogEx::FILE_TYPES fileTypes[] =
		{
			{L"All Audio Files (*.*)",
				L"*.mp3;*.ogg;*.oga;*.wma;*.asf;*.wav;*.aiff;*.aif;*.flac;*.fla;*.ape;"
				L"*.aac;*.mp4;*.m4a;*.m4b;*.m4r;*.opus;*.spx;*.wv;*.mpc;*.tta;*.cue"},
			{L"MPEG Audio (*.mp3)", L"*.mp3"},
			{L"OGG Vorbis (*.ogg)", L"*.ogg;*.oga"},
			{L"Apple Audio (*.m4a)", L"*.aac;*.mp4;*.m4a;*.m4b;*.m4r"},
			{L"Windows Media Audio (*.wma)", L"*.wma;*.asf"},
			{L"Waveform Audio File Format (*.wav)", L"*.wav"},
			{L"Audio Interchange File Format (*.aiff)", L"*.aiff;*.aif"},
			{L"Free Lossless Audio Codec (*.flac)", L"*.flac;*.fla"},
			{L"Monkey's Audio (*.ape)", L"*.ape"},
			{L"Opus (*.opus)", L"*.opus"},
			{L"Speex (*.spx)", L"*.spx"},
			{L"WavPack (*.wv)", L"*.wv"},
			{L"Musepack (*.mpc)", L"*.mpc"},
			{L"True Audio (*.tta)", L"*.tta"},
			{L"Cue Sheet", L"*.cue"}
		};
		int countTypes = sizeof(fileTypes) / sizeof(fileTypes[0]);

		fileDialog.SetFileTypes(fileTypes, countTypes);

		if (fileDialog.DoModalFile(thisWnd, false, true))
		{
			for (int i = 0; i < fileDialog.GetMultiCount(); ++i)
				files.push_back(fileDialog.GetMultiFile(i));
		}
		else
			result = false;

		EnableAll(true);
	}

	if (result)
	{
		bool isDefaultPlaylist = false;
		if (!dBase.IsPlaylistOpen())
		{
			DropNewDefault();
			isDefaultPlaylist = true;
		}

		DropAddFiles(files, false, isDefaultPlaylist);
	}
}

void WinylWnd::AddURL()
{
	EnableAll(false);

	DlgOpenURL dlg;
	dlg.SetLanguage(&lang);
	if (dlg.ModalDialog(thisWnd, IDD_DLGOPENURL) == IDOK)
	{
		if (!dBase.IsPlaylistOpen())
			DropNewDefault();

		// Below is a simpler version of DropAddFiles
		skinList->RemoveSelection();
		if (settings.IsPlayFocus())
			skinList->EnablePlayFocus(false);

		long long addedTime = FileSystem::GetTimeNow();
		int start = dBase.GetPlaylistMax();
		dBase.AddURLToPlaylist(start + 1, addedTime, dlg.GetURL(), dlg.GetName());
		dBase.FillPlaylistOpenFile(skinList.get(), settings.GetLibraryValue(), start);

		// Scroll to the area of the added tracks
		if (skinList->GetSelectedSize() > 0)
			skinList->ScrollToMyNode(skinList->GetSelectedAt(0));

		if (settings.IsPlayFocus())
			skinList->EnablePlayFocus(true);
	}

	EnableAll(true);
}

void WinylWnd::ImportPlaylist()
{
		EnableAll(false);

		FileDialogEx fileDialog;

		FileDialogEx::FILE_TYPES fileTypes[] =
		{
			{L"All Playlists", L"*.m3u;*.m3u8;*.pls;*.asx;*.xspf"},
			{L"M3U Playlist (*.m3u; *.m3u8)", L"*.m3u;*.m3u8"},
			{L"PLS Playlist (*.pls)", L"*.pls"},
			{L"Advanced Stream Redirector (*.asx)", L"*.asx"}, 
			{L"XML Shareable Playlist Format (*.xspf)", L"*.xspf"},
		};
		int countTypes = sizeof(fileTypes) / sizeof(fileTypes[0]);

		fileDialog.SetFileTypes(fileTypes, countTypes);

		if (fileDialog.DoModalFile(thisWnd, false, false))
		{
			std::vector<std::wstring> files;
			files.push_back(fileDialog.GetFile());
			DropAddPlaylist(files, false);
		}

		EnableAll(true);
}

void WinylWnd::ScanLibraryStart(bool isRemoveMissing, bool isIgnoreDeleted, bool isFindMoved, bool isRescanAll)
{
	dlgProgressPtr.reset(new DlgProgress());

	dlgProgressPtr->SetLanguage(&lang);
	dlgProgressPtr->SetMessageWnd(thisWnd);
	dlgProgressPtr->progress.SetDataBase(&dBase);
	dlgProgressPtr->progress.SetPortableVersion(isPortableVersion);
	dlgProgressPtr->progress.SetProgramPath(programPath);
	dlgProgressPtr->progress.SetLibraryFolders(libraryFolders);
	dlgProgressPtr->progress.SetIgnoreDeleted(isIgnoreDeleted);
	dlgProgressPtr->progress.SetRemoveMissing(isRemoveMissing);
	dlgProgressPtr->progress.SetFindMoved(isFindMoved);
	dlgProgressPtr->progress.SetRescanAll(isRescanAll);
	dlgProgressPtr->progress.SetAddAllToLibrary(settings.IsAddAllToLibrary());

	dlgProgressPtr->SetTaskbarMessage(wmTaskbarButtonCreated);
	dlgProgressPtr->CreateModelessDialog(NULL, IDD_DLGPROGRESS);
	::ShowWindow(dlgProgressPtr->Wnd(), SW_SHOW);
}

void WinylWnd::ScanLibraryFinish(bool isDestroyOnStop)
{
	if (dlgProgressPtr)
	{
		dlgProgressPtr.reset();
	}

	if (isDestroyOnStop)
	{
		::DestroyWindow(thisWnd);
		return;
	}

	skinTree->SetControlRedraw(false);
	
	skinTree->ClearLibrary();

	skinTree->SetControlRedraw(true);
}
bool WinylWnd::LoadLibraryFolders()
{
	std::wstring file = profilePath + L"Library.xml";

	XmlFile xmlFile;

	if (xmlFile.LoadFile(file))
	{
		XmlNode xmlMain = xmlFile.RootNode().FirstChild("Library");

		if (xmlMain)
		{
			for (XmlNode xmlNode = xmlMain.FirstChild(); xmlNode; xmlNode = xmlNode.NextChild())
			{
				std::wstring path = xmlNode.Attribute16("Path");
				if (!path.empty())
				{
					// Ver: 2.6.0 In old versions the last slash was cutted off
					if (!path.empty() && path.back() != '\\')
						path.push_back('\\');

					libraryFolders.push_back(path);
				}
			}
		}
	}
	else
		return false;

	return true;
}

bool WinylWnd::SaveLibraryFolders()
{
	XmlFile xmlFile;
	XmlNode xmlMain = xmlFile.RootNode().AddChild("Library");
	
	for (std::size_t i = 0, size = libraryFolders.size(); i < size; ++i)
	{
		XmlNode xmlPath = xmlMain.AddChild("Folder");

		xmlPath.AddAttribute16("Path", libraryFolders[i]);
	}
	
	std::wstring file = profilePath + L"Library.xml";

	if (xmlFile.SaveFile(file))
		return true;

	return false;
}

void WinylWnd::UpdateStatusLine()
{
	if (!skinList->IsRadio())
	{
		int count = 0, total = 0, time = 0;
		long long size = 0;

		skinList->CalculateSelectedNodes(count, total, time, size);

		if (total > 0)
			skinDraw.DrawStatusLine(count, total, time, size, &lang);
		else
			skinDraw.DrawStatusLineNone();
	}
	else
		skinDraw.DrawStatusLineNone();
}
