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

bool WinylWnd::CoverThread(const std::wstring& file)
{
	std::wstring path = PathEx::PathFromFile(file);

	if (coverExternal && path == coverPath)
	{
		return false;
	}
	else
	{
		mutexCover.Lock();
		isThreadCover = true;
		coverFile = file;
		mutexCover.Unlock();

		coverPath = path;
	}

	isCoverShowPopup = true;

	CoverThreadStart();
	return true;
}

void WinylWnd::CoverThreadStart()
{
	if (!threadCover.IsRunning())
	{
		if (threadCover.IsJoinable())
			threadCover.Join();

		threadCover.Start(std::bind(&WinylWnd::CoverThreadRun, this));
	}
}

void WinylWnd::CoverThreadRun()
{
	while (isThreadCover)
	{
		mutexCover.Lock();
		isThreadCover = false;
		std::wstring file = coverFile;
		mutexCover.Unlock();

		if (!file.empty())
		{
			CoverLoader coverLoader;
			coverExternal = coverLoader.LoadCoverImage(file);

			if (isMediaPlay && !isMediaRadio) // If don't press stop while loading
			{
				if (coverLoader.GetImage().IsValid())
				{
					skinDraw.DrawCover(&coverLoader.GetImage());
					if (skinPopup)
						skinPopup->SetCover(&coverLoader.GetImage());
					if (skinMini)
						skinMini->skinDraw.DrawCover(&coverLoader.GetImage());

					win7TaskBar.SetCover(Wnd(), coverLoader.GetImage());
				}
				else
				{
					skinDraw.DrawCover(nullptr);
					if (skinPopup)
						skinPopup->SetCover(nullptr);
					if (skinMini)
						skinMini->skinDraw.DrawCover(nullptr);

					win7TaskBar.EmptyCover(Wnd());
				}
			}
		}
	}

	if (IsWnd()) ::PostMessageW(Wnd(), UWM_COVERDONE, 0, 0);
}

void WinylWnd::CoverThreadDone()
{
	if (isThreadCover)
	{
		CoverThreadStart();
		return;
	}

	if (isMediaPlay && isCoverShowPopup)
		ShowPopup();
}

void WinylWnd::SetCoverNone()
{
	mutexCover.Lock();
	coverFile.clear();
	mutexCover.Unlock();
	coverPath.clear();

	skinDraw.DrawCover(nullptr);
	if (skinPopup)
		skinPopup->SetCover(nullptr);
	if (skinMini)
		skinMini->skinDraw.DrawCover(nullptr);

	win7TaskBar.EmptyCover(thisWnd);
}

void WinylWnd::LyricsThread(const std::wstring& file, const std::wstring& title, const std::wstring& artist)
{
	if (!isLyricsWindow)
		return;

	mutexLyrics.Lock();
	isThreadLyrics = true;
	lyricsFile = file;
	lyricsTitle = title;
	lyricsArtist = artist;
	mutexLyrics.Unlock();

	LyricsThreadStart();
}

void WinylWnd::LyricsThreadStart()
{
	if (!isWindowIconic && skinLyrics->IsWindowVisible())
	{
		if (!threadLyrics.IsRunning())
		{
			if (threadLyrics.IsJoinable())
				threadLyrics.Join();

			threadLyrics.Start(std::bind(&WinylWnd::LyricsThreadRun, this));
		}
	}
}

void WinylWnd::LyricsThreadRun()
{
	while (isThreadLyrics)
	{
		mutexLyrics.Lock();
		isThreadLyrics = false;
		std::wstring file = lyricsFile;
		std::wstring title = lyricsTitle;
		std::wstring artist = lyricsArtist;
		mutexLyrics.Unlock();

		lyricsSource = 0;
		LyricsLoader lyricsLoader;
		if (!lyricsLoader.LoadLyricsFromFile(file))
		{
			if (!lyricsLoader.LoadLyricsFromTags(file))
			{
				if (!settings.IsLyricsProviderOff())
				{
					if (IsWnd()) ::PostMessageW(Wnd(), UWM_LYRICSRECV, 0, 0);
					if (lyricsLoader.LoadLyricsFromInternet(artist, title, settings.GetLyricsProvider()))
						lyricsSource = 3;
				}
			}
			else
				lyricsSource = 2;
		}
		else
			lyricsSource = 1;

		lyricsLines = lyricsLoader.MoveLines();
	}

	//Sleep(3300);
	if (IsWnd()) ::PostMessageW(Wnd(), UWM_LYRICSDONE, 0, 0);
}

void WinylWnd::LyricsThreadDone()
{
	if (isThreadLyrics)
	{
		LyricsThreadStart();
		return;
	}

	if (isMediaPlay && !isMediaRadio)
	{
		lyricsFileCur = lyricsFile;
		lyricsTitleCur = lyricsTitle;
		lyricsArtistCur = lyricsArtist;

		skinLyrics->SetState(true, false, false);
		skinLyrics->UpdateLyrics(std::move(lyricsLines));
	}
	else
		lyricsLines.clear();
}

void WinylWnd::SetLyricsNone(bool isRadio)
{
	if (!isLyricsWindow)
		return;

	mutexLyrics.Lock();
	lyricsFile.clear();
	lyricsTitle.clear();
	lyricsArtist.clear();
	mutexLyrics.Unlock();

	lyricsFileCur.clear();
	lyricsTitleCur.clear();
	lyricsArtistCur.clear();

	lyricsLines.clear();

	lyricsSource = 0;

	if (isRadio)
		skinLyrics->SetState(true, false, false);
	else
		skinLyrics->SetState(false, false, false);
	skinLyrics->UpdateLyricsNull();
}

void WinylWnd::LyricsThreadReceiving()
{
	lyricsFileCur.clear();
	lyricsTitleCur.clear();
	lyricsArtistCur.clear();

	lyricsLines.clear();

	skinLyrics->SetState(false, true, false);
	skinLyrics->UpdateLyricsNull();
}

void WinylWnd::LyricsThreadReload()
{
	if (!isLyricsWindow)
		return;

	if (lyricsFile != lyricsFileCur || lyricsTitle != lyricsTitleCur || lyricsArtist != lyricsArtistCur)
	{
		if (isMediaPlay && !isMediaRadio)
		{
			skinLyrics->SetState(false, false, true);
			skinLyrics->UpdateLyricsNull();
			LyricsThreadStart();
		}
		else
			SetLyricsNone(isMediaRadio);
	}
}

void WinylWnd::DialogConfig(int page)
{
	//SetActiveWindow();
	EnableAll(false);
	DlgConfig dlg;
	dlg.SetLanguage(&lang);
	dlg.SetSettings(&settings);
	dlg.SetPage(page);

	dlg.pageGeneral.SetProgramPath(programPath);
	dlg.pageGeneral.SetHideAssoc(isPortableVersion || isPortableProfile);
	dlg.pageGeneral.SetLastFM(&lastFM);
	dlg.pagePopup.SetSkinPopup(skinPopup.get(), &isMediaPlay);
	dlg.pagePopup.SetContextMenu(&contextMenu);
	dlg.pageSystem.SetMainWnd(thisWnd);
	dlg.pageSystem.SetLibAudio(&libAudio);
	dlg.pageSystem.SetRadio(&radio);
	dlg.pageMini.SetMiniPlayer(skinMini.get());

	if (dlg.ModalDialog(thisWnd, IDD_DLGCONFIG) == IDOK)
	{
		SaveSettings();

		if (dlg.pageGeneral.IsUpdateLibrary())
			LoadLibraryView(true);

		skinList->EnablePlayFocus(settings.IsPlayFocus());
	}

	EnableAll(true);
}

void WinylWnd::DialogLanguage()
{
	EnableAll(false);
	DlgLanguage dlg;
	dlg.SetLanguage(&lang);
	dlg.SetProgramPath(programPath);
	if (dlg.ModalDialog(thisWnd, IDD_DLGLANGUAGE) == IDOK)
	{
		if (!lang.ReloadLanguage(dlg.GetLanguageName()))
		{
			lang.ReloadLanguage(settings.GetDefaultLanguage());
			MessageBox::Show(thisWnd, L"Error", L"The language file is corrupted or it's for a different version of the program.", MessageBox::Icon::Warning);
		}

		contextMenu.ReloadMenu();

		contextMenu.CheckRepeat(settings.IsRepeat());
		contextMenu.CheckShuffle(settings.IsShuffle());
		contextMenu.CheckMute(settings.IsMute());
		contextMenu.CheckHideToTray(settings.IsHideToTray());
//		contextMenu.CheckLastFM(settings.IsLastFM());
		contextMenu.CheckPopup(settings.IsPopup());
		contextMenu.CheckEffect(settings.IsEffect());
		contextMenu.CheckSmoothScroll(settings.IsSmoothScroll());

		contextMenu.CheckPopupPosition(settings.GetPopupPosition());

		contextMenu.CheckMiniPlayer(settings.IsMiniPlayer());

		contextMenu.CheckSearch(settings.GetSearchType());

		contextMenu.CheckLyricsAlign(settings.GetLyricsAlign());
		contextMenu.CheckLyricsFontSize(settings.GetLyricsFontSize());
		contextMenu.CheckLyricsFontBold(settings.GetLyricsFontBold());
		if (settings.IsLyricsProviderOff())
			contextMenu.CheckLyricsProvider(-1);
		else
		{
			if (settings.GetLyricsProvider().empty())
				contextMenu.CheckLyricsProvider(0);
			else
			{
				int prov = LyricsLoader::GetLyricsProviderByURL(settings.GetLyricsProvider());
				if (prov > -1)
					contextMenu.CheckLyricsProvider(prov);
				else
				{
					contextMenu.CheckLyricsProvider(0);
					settings.SetLyricsProvider(L"");
				}
			}
		}

		LoadLibraryView(true);		

		skinList->SetNoItemsString(lang.GetLineS(Lang::Playlist, 4));
		if (!skinList->GetRootNode()->HasChild())
			::InvalidateRect(skinList->Wnd(), NULL, FALSE);

		skinLyrics->SetLanguageNoLyrics(lang.GetLineS(Lang::Lyrics, 0));
		skinLyrics->SetLanguageNotFound(lang.GetLineS(Lang::Lyrics, 1));
		skinLyrics->SetLanguageReceiving(lang.GetLineS(Lang::Lyrics, 2));
		if (skinLyrics->IsWindowVisible())
			::InvalidateRect(skinLyrics->Wnd(), NULL, FALSE);

		if (settings.GetSearchType() == 0)
			skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 0));
		else if (settings.GetSearchType() == 1)
			skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 2));
		else if (settings.GetSearchType() == 2)
			skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 3));
		else if (settings.GetSearchType() == 3)
			skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 4));

		UpdateStatusLine();

		Associations assoc;
		if (assoc.QueryAssocFolder())
		{
			assoc.SetProgramPath(programPath);
			assoc.SetPlayInWinylString(lang.GetLineS(Lang::GeneralPage, 8));
			assoc.AddAssocFolder();
		}
	}
	EnableAll(true);
}

void WinylWnd::MiniPlayer(bool isEnable)
{
	if (skinMini)
	{
		settings.SetMiniPlayer(isEnable);
		trayIcon.SetMiniPlayer(isEnable);
		contextMenu.CheckMiniPlayer(isEnable);

		if (isEnable)
		{
			isWindowIconic = true;
			trayIcon.Minimize(thisWnd, true);
			::SetForegroundWindow(skinMini->Wnd());
			skinMini->SetVisible(true);
		}
		else
		{
			isWindowIconic = false;
			skinMini->SetVisible(false);
			trayIcon.Restore(thisWnd, true, settings.IsMaximized());
			if (skinList) // When the skin is changing skinList can be nullptr
				skinList->ScrollToPlayNode();
			LyricsThreadReload();
		}
	}
}

void WinylWnd::DialogEqualizer()
{
	EnableAll(false);
	DlgEqualizer dlg;
	dlg.SetProgramPath(programPath);
	dlg.SetProfilePath(profilePath);
	dlg.SetLanguage(&lang);
	dlg.SetLibAudio(&libAudio);
	dlg.ModalDialog(thisWnd, IDD_DLGEQ);
	EnableAll(true);
}

void WinylWnd::DialogProperties(bool isOpenLyrics)
{
	ListNodeUnsafe node = nullptr;
	if (!isOpenLyrics)
		node = skinList->GetFocusNode();
	else
		node = skinList->GetPlayNode();

	if (node)
	{
		EnableAll(false);
		DlgProperties dlg;
		dlg.SetLanguage(&lang);
		dlg.SetSkinList(skinList.get());
		if (isOpenLyrics)
		{
			dlg.SetSkinListNode(node);
			dlg.SetOpenLyrics(skinLyrics->GetLyrics());
		}
		dlg.SetDataBase(&dBase);
		dlg.SetLibAudio(&libAudio);
		dlg.SetCallbackChanged(std::bind(&WinylWnd::PropertiesChanged, this, std::placeholders::_1));
		dlg.pageCover.SetSaveCoverTags(settings.IsTagEditorCoverTags());
		dlg.pageCover.SetSaveCoverFile(settings.IsTagEditorCoverFile());
		dlg.pageLyrics.SetSaveLyricsTags(settings.IsTagEditorLyricsTags());
		dlg.pageLyrics.SetSaveLyricsFile(settings.IsTagEditorLyricsFile());
		if (dlg.ModalDialog(thisWnd, IDD_DLGPROPERTIES) == IDOK)
		{
			PropertiesChanged(dlg.properties.get());
		}
		settings.SetTagEditorCoverTags(dlg.pageCover.IsSaveCoverTags());
		settings.SetTagEditorCoverFile(dlg.pageCover.IsSaveCoverFile());
		settings.SetTagEditorLyricsTags(dlg.pageLyrics.IsSaveLyricsTags());
		settings.SetTagEditorLyricsFile(dlg.pageLyrics.IsSaveLyricsFile());
		EnableAll(true);
	}
}

void WinylWnd::PropertiesChanged(Properties* properties)
{
	::InvalidateRect(skinList->Wnd(), NULL, false);

	if (properties->IsUpdateLibrary())
		skinTree->ClearLibrary();
	if (properties->IsUpdateCovers())
	{
		skinList->UpdateCovers();

		if (isMediaPlay && !isMediaRadio)
		{
			isCoverShowPopup = false;
			isThreadCover = true;
			CoverThreadStart();
		}
	}
	if (properties->IsUpdateLyrics())
	{
		if (isMediaPlay && !isMediaRadio)
		{
			if (properties->IsMultiple() || skinList->GetPlayNode() == properties->GetSkinListNode())
			{
				isThreadLyrics = true;
				LyricsThreadStart();
			}
		}
	}
}

void WinylWnd::DialogLibrary()
{
	if (dlgProgressPtr) // Do not show if the library is scanning
		return;

	EnableAll(false);
	DlgLibrary dlg;
	dlg.SetLanguage(&lang);
	dlg.pageLibrary.SetPortableVersion(isPortableVersion);
	dlg.pageLibrary.SetProgramPath(programPath);
	dlg.pageLibrary.SetLibraryFolders(&libraryFolders);
	dlg.pageLibraryOpt.SetRemoveMissing(settings.IsRescanRemoveMissing());
	dlg.pageLibraryOpt.SetIgnoreDeleted(settings.IsRescanIgnoreDeleted());
	if (dlg.ModalDialog(thisWnd, IDD_DLGLIBRARY) == IDOK)
	{
		SaveLibraryFolders();
		ScanLibraryStart(dlg.pageLibraryOpt.IsRemoveMissing(), dlg.pageLibraryOpt.IsIgnoreDeleted(),
			dlg.pageLibraryOpt.IsFindMoved(), dlg.pageLibraryOpt.IsRescanAll());
	}
	settings.SetRescanRemoveMissing(dlg.pageLibraryOpt.IsRemoveMissing());
	settings.SetRescanIgnoreDeleted(dlg.pageLibraryOpt.IsIgnoreDeleted());
	EnableAll(true);
}

void WinylWnd::TimerEffectsThreadRun()
{
	HANDLE timerHandle = ::CreateWaitableTimerW(NULL, FALSE, NULL);
	LARGE_INTEGER dueTime = {};
	::SetWaitableTimer(timerHandle, &dueTime, 16, NULL, NULL, FALSE); // 16 ms = 60 FPS = NO LAGS

	//LARGE_INTEGER freq; QueryPerformanceFrequency(&freq);
	//LARGE_INTEGER counter1; QueryPerformanceCounter(&counter1);

	while (!isTimerSmooth)
	{
		if ((!skinList || !skinList->IsSmoothScrollRun()) &&
			(!skinTree || !skinTree->IsSmoothScrollRun()) &&
			(!skinLyrics || !skinLyrics->IsSmoothScrollRun()))
		{
			eventTimerSmooth.Wait();
		}

		if (IsWnd()) ::PostMessageW(Wnd(), UWM_TIMERTHREAD, 0, 0);

		// Previously I was trying to use WaitForSingleObject on Event and Sleep here,
		// but these functions are very inaccurate with small interval, for example on Win10 under VirtualBox:
		// With WaitForSingleObject on Event and 16ms interval real interval was ~32ms, but with 15ms real interval was ~15ms.
		// With Sleep real interval was always ~32ms, maybe it is just under VirtualBox, but I am not sure.
		// So use much better waitable timer here, it is more accurate and can wait for difference of intervals between waits.
		::WaitForSingleObject(timerHandle, INFINITE);

		//LARGE_INTEGER counter2; QueryPerformanceCounter(&counter2);
		//double diffMs = (counter2.QuadPart - counter1.QuadPart) * 1000.0 / freq.QuadPart;
		//QueryPerformanceCounter(&counter1);

		//AllocConsole();
		//freopen("CONOUT$", "w", stdout);
		//printf("%f\n", diffMs);
	}

	//::CancelWaitableTimer(timerHandle);
	::CloseHandle(timerHandle);
}
