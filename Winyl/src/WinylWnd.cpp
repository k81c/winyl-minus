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

// This class is a mess, need to refactor it, it's doing too many things already.

WinylWnd::WinylWnd()
{

}

WinylWnd::~WinylWnd()
{
	if (iconLarge) ::DestroyIcon(iconLarge);
	if (iconSmall) ::DestroyIcon(iconSmall);

	if (threadTimerSmooth.IsJoinable())
	{
		isTimerSmooth = true;
		eventTimerSmooth.Set();
		threadTimerSmooth.Join();
	}

	if (threadCover.IsJoinable())
	{
		threadCover.Join();
	}

	if (threadLyrics.IsJoinable())
	{
		threadLyrics.Join();
	}

	if (threadSearch.IsJoinable())
	{
		dBase.SetStopSearch(true);
		threadSearch.Join();
		dBase.SetStopSearch(false);
	}
}

LRESULT WinylWnd::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(thisWnd, &ps);
		OnPaint(hdc, ps);
		EndPaint(thisWnd, &ps);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_TIMER:
		OnTimer((UINT_PTR)wParam);
		return 0;
	case WM_MOUSEMOVE:
		TRACKMOUSEEVENT tme;
		OnMouseMove((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = thisWnd;
		tme.dwHoverTime = 0;
		TrackMouseEvent(&tme);
		return 0;
	case WM_MOUSELEAVE:
		OnMouseLeave();
		return 0;
	case WM_NCHITTEST:
		OnNcHitTest(CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	case WM_SETCURSOR:
		if (OnSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam)))
			return 1;
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	case WM_WINDOWPOSCHANGED:
		OnWindowPosChanged((WINDOWPOS*)lParam);
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	case WM_SIZE:
		OnSize((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_GETMINMAXINFO:
		OnGetMinMaxInfo((MINMAXINFO*)lParam);
		return 0;
	case WM_LBUTTONDOWN:
		OnLButtonDown((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		return 0;
	case WM_LBUTTONUP:
		OnLButtonUp((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		return 0;
	case WM_LBUTTONDBLCLK:
		OnLButtonDblClk((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		return 0;
	case WM_CONTEXTMENU:
		OnContextMenu((HWND)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		return 0;
	case WM_COMMAND:
		OnCommand(wParam, lParam);
		return 0;
	case WM_NOTIFY:
		OnNotify(wParam, lParam);
		return 0;
	case WM_SYSCOMMAND:
		if (OnSysCommand(wParam, lParam))
			return 0;
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	case WM_DESTROY:
		OnDestroy();
		return 0;
	case WM_DROPFILES:
		OnDropFiles((HDROP)wParam);
		return 0;
	case WM_COPYDATA:
		OnCopyData((HWND)wParam, (COPYDATASTRUCT*)lParam);
		return 1;
	}

	return WindowProcEx(hWnd, message, wParam, lParam);
	//return ::DefWindowProc(hWnd, message, wParam, lParam);
}

bool WinylWnd::NewWindow()
{
	// Load settings
	settings.SetProfilePath(profilePath);
	settings.SetPortableVersion(isPortableVersion);
	settings.LoadSettings();

	// Load language
	lang.SetProgramPath(programPath);

	// 追加: 言語ファイルが存在しないなら起動を中止する
	if (!CheckLanguageFiles(programPath, settings.IsFirstRun() ? lang.GetSystemLanguage() : settings.GetLanguage()))
		return false;

	if (settings.IsFirstRun())
	{
		if (!lang.LoadLanguage(lang.GetSystemLanguage()))
			lang.LoadLanguage(settings.GetDefaultLanguage());
	}
	else
	{
		if (!lang.LoadLanguage(settings.GetLanguage()))
			lang.LoadLanguage(settings.GetDefaultLanguage());
	}

	if (settings.IsDBOldBeta())
	{
		MessageBox::Show(NULL, L"Winyl",
			L"Library database is incompatible.",
			MessageBox::Icon::Error);
		return false;
	}

	if (settings.IsConvertVersion3())
	{
		DlgNewVersion dlg;
		dlg.SetLanguage(&lang);
		dlg.SetProgramPath(programPath);
		dlg.SetProfilePath(profilePath);
		if (dlg.ModalDialog(thisWnd, IDD_DLGNEWVERSION) != IDOK)
		{
			MessageBox::Show(NULL, L"Winyl",
				L"Something bad happened when trying to convert the library.\nPlease contact with the developer.",
				MessageBox::Icon::Error);
			return false;
		}
		settings.SaveSettings();
	}
	if (settings.IsConvertPortable())
	{
		DlgNewVersion dlg;
		dlg.SetLanguage(&lang);
		dlg.SetProgramPath(programPath);
		dlg.SetProfilePath(profilePath);
		dlg.SetConvertPortable(settings.IsConvertPortableTo(), settings.IsConvertPortableFrom());
		dlg.ModalDialog(thisWnd, IDD_DLGNEWVERSION);
		settings.SaveSettings();
	}

	LoadLibraryFolders();

//	LARGE_INTEGER freq; QueryPerformanceFrequency(&freq);
//	LARGE_INTEGER counter1; QueryPerformanceCounter(&counter1);

	std::unique_ptr<ZipFile> zipFile;

	if (settings.IsSkinPack())
		zipFile = skinDraw.NewZipFile(programPath, settings.GetSkin());
	if (!zipFile)
		settings.SetSkinPack(false);

	// Load skin fonts if needed
	fontsLoader.LoadSkinFonts(programPath, settings.GetSkin(), zipFile.get(), true);

	// Load skin
	if (!skinDraw.LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
	{
		fontsLoader.FreeSkinFonts();

		settings.SetSkin(settings.GetDefaultSkin());
		settings.SetSkinPack(settings.IsDefaultSkinPack());

		if (settings.IsSkinPack())
			zipFile = skinDraw.NewZipFile(programPath, settings.GetSkin());
		skinDraw.LoadSkin(programPath, settings.GetSkin(), zipFile.get());
	}

	skinDraw.LoadSkinSettings(profilePath, settings.GetSkin());

//	LARGE_INTEGER counter2; QueryPerformanceCounter(&counter2);
//	double perfDiffMs = (counter2.QuadPart - counter1.QuadPart) *1000.0 / freq.QuadPart;
//	int i = 0;

	// Adjust the main window position and size by the monitor
	settings.FixWinPosMain(skinDraw.GetMinSize(), skinDraw.GetMaxSize());

	// Adjust the main window position and size by Alpha Window if present
	settings.FixWinPosAlphaGet(skinDraw.IsLayeredAlpha(), skinDraw.GetAlphaBorder());

	std::wstring iconFile = programPath + L"Main.ico";
	if (futureWin->IsVistaOrLater())
	{
		if (FileSystem::Exists(iconFile))
		{
			int cxIconLarge = ::GetSystemMetrics(SM_CXICON);
			int cyIconLarge = ::GetSystemMetrics(SM_CYICON);

			// Use LoadImage for 32, 48, 64 etc. icons. LoadIconMetric load
			// the wrong icon from the external file in these cases for some reason.
			// In other cases like 125% DPI (40px) LoadIconMetric must be used.
			// It only affects the large icon.
			if (cxIconLarge % 16 == 0)
				iconLarge = (HICON)::LoadImageW(NULL, iconFile.c_str(), IMAGE_ICON, cxIconLarge, cyIconLarge, LR_LOADFROMFILE);
			else
				futureWin->LoadIconMetric(NULL, iconFile.c_str(), LIM_LARGE, &iconLarge);

			futureWin->LoadIconMetric(NULL, iconFile.c_str(), LIM_SMALL, &iconSmall);
		}
		else
		{
			futureWin->LoadIconMetric(::GetModuleHandleW(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), LIM_LARGE, &iconLarge);
			futureWin->LoadIconMetric(::GetModuleHandleW(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), LIM_SMALL, &iconSmall);
		}
	}
	else
	{
		int cxIconLarge = ::GetSystemMetrics(SM_CXICON);
		int cyIconLarge = ::GetSystemMetrics(SM_CYICON);
		int cxIconSmall = ::GetSystemMetrics(SM_CXSMICON);
		int cyIconSmall = ::GetSystemMetrics(SM_CYSMICON);

		if (FileSystem::Exists(iconFile))
		{
			iconLarge = (HICON)::LoadImageW(NULL, iconFile.c_str(), IMAGE_ICON, cxIconLarge, cyIconLarge, LR_LOADFROMFILE);
			iconSmall = (HICON)::LoadImageW(NULL, iconFile.c_str(), IMAGE_ICON, cxIconSmall, cyIconSmall, LR_LOADFROMFILE);
		}
		else
		{
			iconLarge = (HICON)::LoadImageW(::GetModuleHandleW(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, cxIconLarge, cyIconLarge, LR_DEFAULTCOLOR);
			iconSmall = (HICON)::LoadImageW(::GetModuleHandleW(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, cxIconSmall, cyIconSmall, LR_DEFAULTCOLOR);
		}
	}

	CreateClassWindow(NULL, L"WinylWnd", WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		WS_EX_APPWINDOW|WS_EX_ACCEPTFILES, settings.GetWinPosition(), L"Winyl", iconLarge, iconSmall, true);

	wmTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
	wmTaskbarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

	skinDraw.SetWindowStyle(thisWnd, thisWnd);
	skinDraw.EnableFade(settings.IsEffect());

	// Set ResizeBorder
	moveResize.SetResizeBorder(skinDraw.GetResizeBorder());

	if (skinDraw.IsLayeredAlpha())
	{
		skinAlpha.reset(new SkinAlpha());
		skinAlpha->SetLibAudio(&libAudio, &isMediaPlay, &isMediaRadio);
		skinAlpha->SetMoveResize(&moveResize);
		skinAlpha->SetContextMenu(&contextMenu);
		skinAlpha->NewWindow(thisWnd);
		skinAlpha->LoadSkin(programPath, settings.GetSkin(), zipFile.get());
		skinAlpha->skinDraw.SetWindowStyle(skinAlpha->Wnd(), thisWnd);
		skinAlpha->skinDraw.EnableFade(settings.IsEffect());
	}
	if (skinAlpha)
		skinDraw.SetSkinDrawAlpha(&skinAlpha->skinDraw);
	else
		skinDraw.SetSkinDrawAlpha(nullptr);

	if (skinDraw.IsShadowLayered())
	{
		skinShadow.reset(new SkinShadow());
		if (skinShadow->LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
			skinShadow->NewShadow(thisWnd);
		else
			skinShadow.reset();
	}

	if (SkinMini::IsSkinFile(programPath, settings.GetSkin(), zipFile.get()))
	{
		skinMini.reset(new SkinMini());
		if (skinMini->LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
		{
			// Adjust the mini player position and size by the monitor
			settings.FixWinPosMini(skinMini->skinDraw.GetMinSize(),
				skinMini->skinDraw.GetMaxSize());

			skinMini->SetZOrder(settings.GetMiniZOrder());
			skinMini->NewWindow(thisWnd, settings.GetWinPositionMini(), iconLarge, iconSmall);
			skinMini->SetContextMenu(&contextMenu);

			skinMini->skinDraw.EnableFade(settings.IsEffect());
			skinMini->SetTransparency(settings.GetMiniTransparency());
		}
		else
			skinMini.reset();
	}
	if (skinMini)
		skinDraw.SetSkinDrawMini(&skinMini->skinDraw);
	else
		skinDraw.SetSkinDrawMini(nullptr);

	// Set skin values
	skinDraw.DrawMute(settings.IsMute());
	if (settings.IsMute())
		skinDraw.DrawVolume(0);
	else
		skinDraw.DrawVolume(settings.GetVolume());

	skinDraw.DrawRepeat(settings.IsRepeat());
	skinDraw.DrawShuffle(settings.IsShuffle());

	skinDraw.DrawMaximize(settings.IsMaximized());

	skinDraw.DrawSearchClear(false);

	// Load popup window
	if (SkinPopup::IsSkinFile(programPath, settings.GetSkin(), zipFile.get()))
	{
		// Load skin for the popup window
		skinPopup.reset(new SkinPopup());
		if (skinPopup->LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
		{
			skinPopup->NewWindow(thisWnd);

			skinPopup->SetPosition(settings.GetPopupPosition());
			skinPopup->SetEffect(settings.GetPopupShowEffect(), settings.GetPopupHideEffect());
			skinPopup->SetDelay(settings.GetPopupHold(), settings.GetPopupShow(), settings.GetPopupHide());
		}
		else
			skinPopup.reset();
	}

	HttpClient::SetProxy(settings.GetProxy(), settings.GetProxyHost(),  settings.GetProxyPort(),
		settings.GetProxyLogin(),  settings.GetProxyPass());

	// Set audio library parameters
	libAudio.SetProgramPath(programPath);
	libAudio.SetProfilePath(profilePath);
	libAudio.SetUserAgent(settings.GetBassUserAgent());
	if (!libAudio.Init(this, settings.GetBassDriver(), settings.GetBassDevice(),
		settings.IsBassBit32(), settings.IsBassSoftMix(), true)) // Init audio library
	{
		settings.SetBassDriver(0);
		settings.SetBassDevice(-1);
		libAudio.Init(this, 0, -1, settings.IsBassBit32(), settings.IsBassSoftMix(), true);
	}
	libAudio.SetNoVolumeEffect(settings.IsBassNoVolume(), settings.IsBassNoEffect());
	libAudio.SetPropertiesWA(settings.IsBassWasapiEvent(), settings.GetBassAsioChannel());
	libAudio.SetProxy(settings.GetProxy(), settings.GetProxyHost(),  settings.GetProxyPort(), 
		 settings.GetProxyLogin(),  settings.GetProxyPass());

	libAudio.SetMute(settings.IsMute());
	libAudio.SetVolume(settings.GetVolume());

	// Set database library parameters
	dBase.SetProgramPath(programPath);
	dBase.SetProfilePath(profilePath);
	dBase.SetPortableVersion(isPortableVersion);
	dBase.OpenLibrary();
	dBase.SetLanguage(&lang);

	hotKeys.SetProfilePath(profilePath);
	hotKeys.LoadHotKeys();
	hotKeys.RegisterHotKeys(thisWnd);

	trayIcon.NewIcon(thisWnd, iconSmall); // Tray icon
	trayIcon.SetHideToTray(settings.IsHideToTray());
	trayIcon.SetMiniPlayer(settings.IsMiniPlayer());

	contextMenu.SetLanguage(&lang);
	contextMenu.FillMenu(); // Menu

	contextMenu.CheckRepeat(settings.IsRepeat());
	contextMenu.CheckShuffle(settings.IsShuffle());
	contextMenu.CheckMute(settings.IsMute());
	contextMenu.CheckHideToTray(settings.IsHideToTray());

	if (!skinMini)
		contextMenu.EnableMiniPlayer(false);

	if (settings.IsLastFM())
		settings.SetLastFM(lastFM.Init());

//	contextMenu.CheckLastFM(settings.IsLastFM());
	contextMenu.CheckPopup(settings.IsPopup());
	contextMenu.CheckEffect(settings.IsEffect());
	contextMenu.CheckSmoothScroll(settings.IsSmoothScroll());

	contextMenu.CheckPopupPosition(settings.GetPopupPosition());

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

	toolTips.SetLanguage(&lang);
	toolTips.SetWindow(thisWnd, &skinDraw);
	if (settings.IsToolTips())
		toolTips.Create();

	if (skinAlpha)
	{
		skinAlpha->toolTips.SetLanguage(&lang);
		skinAlpha->toolTips.SetWindow(skinAlpha->Wnd(), &skinAlpha->skinDraw);
		if (settings.IsToolTips())
			skinAlpha->toolTips.Create();
	}

	// Initialize AudioLog before loading anything so all subsequent logs are captured
	AudioLog::Instance().SetNotifyWnd(thisWnd);
	audioLogWnd.Create(::GetModuleHandleW(nullptr), thisWnd);

	radio.LoadFromFile(programPath); // Load radio station list from radio_stations.xml

	LoadWindows(false); // Load windows

	if (settings.IsSmoothScroll())
		threadTimerSmooth.Start(std::bind(&WinylWnd::TimerEffectsThreadRun, this));

	// [ver 2.1] It's not needed anymore it seems (but I don't have time to test)
	//skinDraw.RefreshWindow(); // Refresh all skin elements

	return true;
}

void WinylWnd::PrepareLibrary()
{
	if (settings.IsFirstRun())
	{
		settings.SaveSettings();

		// Refactoring needed
		EnableAll(false);
		DlgLibrary dlg;
		dlg.SetLanguage(&lang);
		dlg.SetFirstRun(true);
		dlg.pageLibrary.SetPortableVersion(isPortableVersion);
		dlg.pageLibrary.SetProgramPath(programPath);
		dlg.pageLibrary.SetLibraryFolders(&libraryFolders);
		if (dlg.ModalDialog(thisWnd, IDD_DLGLIBRARY) == IDOK)
		{
			SaveLibraryFolders();

			DlgProgress dlgProgress;
			dlgProgress.SetLanguage(&lang);
			dlgProgress.progress.SetDataBase(&dBase);
			dlgProgress.progress.SetPortableVersion(isPortableVersion);
			dlgProgress.progress.SetProgramPath(programPath);
			dlgProgress.progress.SetLibraryFolders(libraryFolders);
			dlgProgress.ModalDialog(thisWnd, IDD_DLGPROGRESS);
		}
		EnableAll(true);

		// Show first artist
		if (skinTree->GetRootNode())
		{
			TreeNodeUnsafe treeNode = skinTree->GetRootNode()->Child();
			if (treeNode)
			{
				treeNode = treeNode->Next();
				if (treeNode)
				{
					FillTree(treeNode);
					skinTree->ExpandNode(treeNode);

					treeNode = treeNode->Child();
					if (treeNode)
					{
						skinTree->SetFocusNode(treeNode);
						FillList(treeNode);
					}
				}
			}
		}
	}
	else if (settings.IsConvertVersion3())
	{
		MessageBox::Show(thisWnd, lang.GetLine(Lang::Message, 2),
			(lang.GetLineS(Lang::NewVersionDialog, 2) + L"\n" + lang.GetLineS(Lang::NewVersionDialog, 3)).c_str());

		ScanLibraryStart(false, true, false, true);
	}
}

void WinylWnd::RunShowWindow()
{
	if (skinMini && settings.IsMiniPlayer())
	{
		if (skinAlpha)
			::ShowWindow(skinAlpha->Wnd(), SW_SHOW);

		skinMini->SetVisible(true);
	}
	else
	{
		skinDraw.RedrawWindow();

		// Show Alpha Window before showing the main window
		if (skinAlpha)
			::ShowWindow(skinAlpha->Wnd(), 5);

		if (settings.IsMaximized())
			::ShowWindow(thisWnd, 3);
		else
			::ShowWindow(thisWnd, 5);
		::ShowWindow(thisWnd, 1);
		::SetForegroundWindow(thisWnd);
	}

	if (!pointerOpenFiles->empty())
	{
		std::vector<std::wstring> files;

		DropParseFiles((*pointerOpenFiles), files);
		bool isImportPlaylist = DropIsPlaylist(files);
		bool isDefaultPlaylist = false;
		if (!isImportPlaylist)
		{
			DropNewDefault();
			isDefaultPlaylist = true;
		}

		::UpdateWindow(thisWnd);

		if (!isImportPlaylist)
			DropAddFiles(files, true, isDefaultPlaylist);
		else
			DropAddPlaylist(files, true);

		pointerOpenFiles->clear();
		pointerOpenFiles->shrink_to_fit();
	}
	else
	{
		// Select the last played track if there is one
		if (settings.GetLastPlayIndex())
		{
			ListNodeUnsafe node = skinList->FindNodeByIndex(settings.GetLastPlayIndex());
			if (node)
			{
				skinList->SetFocusNode(node, false);
				skinList->ScrollToFocusNode();
			}
		}
	}
}

bool WinylWnd::ReloadSkin(const std::wstring& skinName, bool isSkinPack)
{
	// Save skin settings
	skinDraw.SaveSkinSettings(profilePath, settings.GetSkin());

	// Save main window position
	WINDOWPLACEMENT pl;
	::GetWindowPlacement(thisWnd, &pl);

	settings.SetWinPosition(CRect(pl.rcNormalPosition));

	// Adjust the main window position and size by Alpha Window if present
	settings.FixWinPosAlphaSet(skinDraw.IsLayeredAlpha(), skinDraw.GetAlphaBorder());

	// Save mini player position
	if (skinMini)
	{
		WINDOWPLACEMENT pl;
		::GetWindowPlacement(skinMini->Wnd(), &pl);
		settings.SetWinPositionMini(CRect(pl.rcNormalPosition));
	}

	bool result = true; // To check that the skin is loaded correctly
	settings.SetSkin(skinName);

	if (threadSearch.IsJoinable())
	{
		dBase.SetStopSearch(true);
		threadSearch.Join();
		dBase.SetStopSearch(false);
	}

	// Destroy Alpha Window
	if (skinAlpha) skinAlpha.reset();

	// Destroy shadow windows
	if (skinShadow) skinShadow.reset();

	// Destroy all visualizer windows
	visuals.clear();

	// Destroy all other windows
	skinTree.reset();
	skinList.reset();
	skinEdit.reset();
	skinLyrics.reset();

	// Destroy tooltips
	toolTips.Destroy();

	/*{
		WINDOWPLACEMENT wp;
		//wp.length = sizeof(WINDOWPLACEMENT);

		GetWindowPlacement(&wp);
		wp.showCmd = SW_MINIMIZE;
		SetWindowPlacement(&wp);
	}*/

	// Hide the main window to do not see the skin rebuilding
	::ShowWindow(thisWnd, SW_HIDE);

	settings.SetSkinPack(isSkinPack);

	std::unique_ptr<ZipFile> zipFile;

	if (settings.IsSkinPack())
		zipFile = skinDraw.NewZipFile(programPath, settings.GetSkin());
	if (!zipFile)
		settings.SetSkinPack(false);

	// Load new skin fonts if needed
	fontsLoader.LoadSkinFonts(programPath, settings.GetSkin(), zipFile.get(), true);

	// Load new skin
	if (!skinDraw.LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
	{
		fontsLoader.FreeSkinFonts();

		settings.SetSkin(settings.GetDefaultSkin());
		settings.SetSkinPack(settings.IsDefaultSkinPack());

		if (settings.IsSkinPack())
			zipFile = skinDraw.NewZipFile(programPath, settings.GetSkin());
		skinDraw.LoadSkin(programPath, settings.GetSkin(), zipFile.get());

		result = false;
	}

	// Load new skin settings
	skinDraw.LoadSkinSettings(profilePath, settings.GetSkin());

	// Adjust the main window position and size by the monitor
	settings.FixWinPosMain(skinDraw.GetMinSize(), skinDraw.GetMaxSize());

	// Set ResizeBorder
	moveResize.SetResizeBorder(skinDraw.GetResizeBorder());
	moveResize.Resize(settings.GetWinPosition().Width(), settings.GetWinPosition().Height());

	skinDraw.SetWindowStyle(thisWnd, thisWnd);
	skinDraw.EnableFade(settings.IsEffect());

	// Load Alpha Window
	if (skinDraw.IsLayeredAlpha())
	{
		skinAlpha.reset(new SkinAlpha());
		skinAlpha->SetLibAudio(&libAudio, &isMediaPlay, &isMediaRadio);
		skinAlpha->SetMoveResize(&moveResize);
		skinAlpha->SetContextMenu(&contextMenu);
		skinAlpha->NewWindow(thisWnd);
		skinAlpha->LoadSkin(programPath, settings.GetSkin(), zipFile.get());
		skinAlpha->skinDraw.SetWindowStyle(skinAlpha->Wnd(), thisWnd);
		skinAlpha->skinDraw.EnableFade(settings.IsEffect());
	}
	if (skinAlpha)
		skinDraw.SetSkinDrawAlpha(&skinAlpha->skinDraw);
	else
		skinDraw.SetSkinDrawAlpha(nullptr);

	// Load shadow windows
	if (skinDraw.IsShadowLayered())
	{
		skinShadow.reset(new SkinShadow());
		if (skinShadow->LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
			skinShadow->NewShadow(thisWnd);
		else
			skinShadow.reset();
	}


	// Load mini player
	skinMini.reset();

	if (SkinMini::IsSkinFile(programPath, settings.GetSkin(), zipFile.get()))
	{
		// Load skin for the mini player
		skinMini.reset(new SkinMini());
		if (skinMini->LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
		{
			// Adjust the mini player position and size by the monitor
			settings.FixWinPosMini(skinMini->skinDraw.GetMinSize(),
				skinMini->skinDraw.GetMaxSize());

			skinMini->SetZOrder(settings.GetMiniZOrder());
			skinMini->NewWindow(thisWnd, settings.GetWinPositionMini(), iconLarge, iconSmall);
			skinMini->SetContextMenu(&contextMenu);

			skinMini->skinDraw.EnableFade(settings.IsEffect());
			skinMini->SetTransparency(settings.GetMiniTransparency());
		}
		else
			skinMini.reset();
	}
	if (skinMini)
		skinDraw.SetSkinDrawMini(&skinMini->skinDraw);
	else
		skinDraw.SetSkinDrawMini(nullptr);

	settings.SetMaximized(false);

	// Set skin values
	skinDraw.DrawMute(settings.IsMute());
	if (settings.IsMute())
		skinDraw.DrawVolume(0);
	else
		skinDraw.DrawVolume(settings.GetVolume());

	skinDraw.DrawRepeat(settings.IsRepeat());
	skinDraw.DrawShuffle(settings.IsShuffle());

	skinDraw.DrawSearchClear(false);

	skinDraw.DrawMaximize(settings.IsMaximized());


	// Load popup window
	skinPopup.reset();

	if (SkinPopup::IsSkinFile(programPath, settings.GetSkin(), zipFile.get()))
	{
		// Load skin for the popup window
		skinPopup.reset(new SkinPopup());
		if (skinPopup->LoadSkin(programPath, settings.GetSkin(), zipFile.get()))
		{
			skinPopup->NewWindow(thisWnd);

			skinPopup->SetPosition(settings.GetPopupPosition());
			skinPopup->SetEffect(settings.GetPopupShowEffect(), settings.GetPopupHideEffect());
			skinPopup->SetDelay(settings.GetPopupHold(), settings.GetPopupShow(), settings.GetPopupHide());
		}
		else
			skinPopup.reset();
	}

	// Set tooltips
	toolTips.SetWindow(thisWnd, &skinDraw);
	if (settings.IsToolTips())
		toolTips.Create();

	if (skinAlpha)
	{
		skinAlpha->toolTips.SetLanguage(&lang);
		skinAlpha->toolTips.SetWindow(skinAlpha->Wnd(), &skinAlpha->skinDraw);
		if (settings.IsToolTips())
			skinAlpha->toolTips.Create();
	}

	// Reset Now Playing
	if (settings.GetNowPlayingType() > 0)
	{
		dBase.CloseNowPlaying();
		settings.EmptyNowPlaying();
	}

	// Load windows
	LoadWindows(true);

	// Adjust the main window position and size by Alpha Window if present
	settings.FixWinPosAlphaGet(skinDraw.IsLayeredAlpha(), skinDraw.GetAlphaBorder());

	// Set the main window position and size (MoveWindow calls WM_GETMINMAXINFO)
	::MoveWindow(thisWnd, settings.GetWinPosition().left, settings.GetWinPosition().top,
		settings.GetWinPosition().Width(), settings.GetWinPosition().Height(), TRUE);

	settings.SetMiniPlayer(false);
	contextMenu.CheckMiniPlayer(false);
	trayIcon.SetMiniPlayer(false);
		
	if (skinMini)
		contextMenu.EnableMiniPlayer(true);
	else
		contextMenu.EnableMiniPlayer(false);

	// Refresh and redraw all skin elements
	skinDraw.RefreshWindow();
	skinDraw.RedrawWindow();

	// SW_RESTORE instead of SW_SHOW to restore the main window if the mini player was enabled
	::ShowWindow(thisWnd, SW_RESTORE);

	// Show Alpha Window after showing the main window
	if (skinAlpha)
	{
		::ShowWindow(skinAlpha->Wnd(), SW_SHOW);

		EnableAll(false);
	}

	// Show shadow windows
	if (skinShadow)
		skinShadow->Show(true);

	return result;
}

bool WinylWnd::LoadWindows(bool isReload)
{
	for (std::size_t i = 0, isize = skinDraw.Layouts().size(); i < isize; ++i)
	{
		for (std::size_t j = 0, jsize = skinDraw.Layouts()[i]->elements.size(); j < jsize; ++j)
		{
			SkinElement* element = skinDraw.Layouts()[i]->elements[j]->element.get();

			if (element->type == SkinElement::Type::Library)
			{
				if (skinTree) continue;
				
				skinTree.reset(new SkinTree());
				skinTree->NewWindow(thisWnd);
				skinTree->LoadSkin(element->skinName, element->zipFile);
				skinTree->SetEventSmoothScroll(&eventTimerSmooth);
				
				element->SetWindow(skinTree->Wnd());
			}
			else if (element->type == SkinElement::Type::Playlist)
			{
				if (skinList) continue;

				skinList.reset(new SkinList());
				skinList->NewWindow(thisWnd);
				skinList->LoadSkin(element->skinName, element->zipFile);
				skinList->SetEventSmoothScroll(&eventTimerSmooth);
				
				element->SetWindow(skinList->Wnd());
			}
			else if (element->type == SkinElement::Type::Lyrics)
			{
				if (skinLyrics) continue;

				isLyricsWindow = true;

				skinLyrics.reset(new SkinLyrics());
				skinLyrics->SetBmBack(skinDraw.GetBmBack());
				skinLyrics->NewWindow(thisWnd);
				skinLyrics->LoadSkin(element->skinName, element->zipFile);
				skinLyrics->SetEventSmoothScroll(&eventTimerSmooth);

				element->SetWindow(skinLyrics->Wnd());
			}
			else if (element->type == SkinElement::Type::Search)
			{
				if (skinEdit) continue;

				skinEdit.reset(new SkinEdit());
				skinEdit->NewWindow(thisWnd);
				skinEdit->LoadSkin(element->skinName, element->zipFile);
				
				element->SetWindow(skinEdit->Wnd());
			}
			else if (element->type == SkinElement::Type::Spectrum)
			{
				visuals.emplace_back(new SkinVis());
				
				visuals.back()->NewWindow(thisWnd);
				visuals.back()->LoadSkin(element->skinName, element->zipFile);
				
				element->SetWindow(visuals.back()->Wnd());
			}
		}
	}

	if (!skinTree)
	{
		skinTree.reset(new SkinTree());
		skinTree->NewWindow(thisWnd);
		skinTree->SetEventSmoothScroll(&eventTimerSmooth);
	}
	if (!skinList)
	{
		skinList.reset(new SkinList());
		skinList->NewWindow(thisWnd);
		skinList->SetEventSmoothScroll(&eventTimerSmooth);
	}
	if (!skinEdit)
	{
		skinEdit.reset(new SkinEdit());
		skinEdit->NewWindow(thisWnd);
	}
	if (!skinLyrics)
	{
		isLyricsWindow = false;
		skinLyrics.reset(new SkinLyrics());
		skinLyrics->NewWindow(thisWnd);
		skinLyrics->SetEventSmoothScroll(&eventTimerSmooth);
	}

	skinEdit->SetFocusWnd(skinList->Wnd());

	skinList->SetNoItemsString(lang.GetLineS(Lang::Playlist, 4));
	skinList->EnablePlayFocus(settings.IsPlayFocus());
	skinList->EnableSmoothScroll(settings.IsSmoothScroll());
	skinTree->EnableSmoothScroll(settings.IsSmoothScroll());
	skinLyrics->EnableSmoothScroll(settings.IsSmoothScroll());
	skinLyrics->SetAlign(settings.GetLyricsAlign());
	skinLyrics->SetFontSize(settings.GetLyricsFontSize(), settings.GetLyricsFontBold());
	skinLyrics->RegisterCallbackShowWindow(std::bind(&WinylWnd::LyricsThreadReload, this));
	skinLyrics->SetLanguageNoLyrics(lang.GetLineS(Lang::Lyrics, 0));
	skinLyrics->SetLanguageNotFound(lang.GetLineS(Lang::Lyrics, 1));
	skinLyrics->SetLanguageReceiving(lang.GetLineS(Lang::Lyrics, 2));


	if (settings.GetSearchType() == 0)
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 0));
	else if (settings.GetSearchType() == 1)
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 2));
	else if (settings.GetSearchType() == 2)
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 3));
	else if (settings.GetSearchType() == 3)
		skinEdit->SetStandartText(lang.GetLineS(Lang::Search, 4));

	if (settings.GetLibraryType())
	{
		if (pointerOpenFiles->empty())
			FillList();
	}

	LoadLibraryView();

	return true;
}

bool WinylWnd::LoadLibraryView(bool isReload)
{
	skinTree->SetControlRedraw(false);

	if (isReload)
		skinTree->DeleteAllNode();

	if (settings.IsLibraryNowPlaying())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 2), SkinTreeNode::Type::NowPlaying, false);
	if (settings.IsLibraryArtists())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 5), SkinTreeNode::Type::Artist);
	if (settings.IsLibraryComposers())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 6), SkinTreeNode::Type::Composer);
	if (settings.IsLibraryAlbums())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 7), SkinTreeNode::Type::Album);
	if (settings.IsLibraryGenres())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 8), SkinTreeNode::Type::Genre);
	if (settings.IsLibraryYears())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 9), SkinTreeNode::Type::Year);
	if (settings.IsLibraryFolders())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 11), SkinTreeNode::Type::Folder);
	if (settings.IsLibraryRadios())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 10), SkinTreeNode::Type::Radio);
	if (settings.IsLibrarySmartlists())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 4), SkinTreeNode::Type::Smartlist, false, false);
//	if (settings.IsLibraryPlaylists())
		skinTree->InsertHead(nullptr, lang.GetLineS(Lang::Library, 3), SkinTreeNode::Type::Playlist, false, true);

	if (skinTree->GetPlaylistNode())
	{
		if (!dBase.LoadPlaylist(skinTree.get()))
			dBase.CreatePlaylist(skinTree.get(), lang.GetLineS(Lang::DefaultList, 3));
	}
	if (skinTree->GetSmartlistNode())
	{
		if (!dBase.LoadTreeSmartlists(skinTree.get()))
			dBase.CreateMySmartlists(skinTree.get());
	}

	skinTree->SetControlRedraw(true);

	return true;
}

void WinylWnd::OnPaint(HDC dc, PAINTSTRUCT& ps)
{
	skinDraw.Paint(dc, ps);
}

void WinylWnd::OnSize(UINT nType, int cx, int cy)
{
	if (nType == SIZE_MINIMIZED)
	{
		trayIcon.SizeMinimized(thisWnd);
		skinDraw.Minimized();
	}
	else if (nType == SIZE_RESTORED || nType == SIZE_MAXIMIZED) // Also when resized
	{
		//MessageBeep(1);
		if (!skinAlpha)
			moveResize.Resize(cx, cy);
		skinDraw.Resize(cx, cy, true);

		if (skinShadow)
			skinShadow->Show(nType != SIZE_MAXIMIZED);
	}
}

void WinylWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (skinDraw.IsSplitterPress())
		skinDraw.SplitterLayout(skinDraw.GetPressElement(), point.x, point.y);
	else
	{
		SkinElement* element = skinDraw.MouseMove(nFlags, point);

		if (element)
			Action(element, MouseAction::Move);

		// Tracking tooltip
		if (element && element->type == SkinElement::Type::Volume)
		{
			int thumb = static_cast<SkinSlider*>(element)->GetThumbPosition();
			std::wstring percent = std::to_wstring(element->GetParam() / 1000);
			toolTips.TrackingToolTip(false, percent, element->rcRect.left + thumb, element->rcRect.top);
		}
		else if (element && element->type == SkinElement::Type::Track && isMediaPlay && !isMediaRadio)
		{
			int thumb = static_cast<SkinSlider*>(element)->GetThumbPosition();
			int pos = (element->GetParam() * libAudio.GetTimeLength() + 100000 / 2) / 100000;
			std::wstring time = SkinText::TimeString(pos, false, libAudio.GetTimeLength(), true);
			toolTips.TrackingToolTip(false, time, element->rcRect.left + thumb, element->rcRect.top, true);
		}
		else if (!skinDraw.IsPressElement())
		{
			if (skinDraw.GetHoverElement() && skinDraw.GetHoverElement()->type == SkinElement::Type::Track && isMediaPlay && !isMediaRadio)
			{
				if (isTrackTooltip)
				{
					// https://msdn.microsoft.com/en-us/library/windows/desktop/hh298405%28v=vs.85%29.aspx
					// Make sure the mouse has actually moved. The presence of the tooltip 
					// causes Windows to send the message continuously.
					static POINT oldPoint = {};
					if (point != oldPoint)
					{
						oldPoint = point;
						int percent = static_cast<SkinSlider*>(skinDraw.GetHoverElement())->CalcPercent(point);
						int pos = (percent * libAudio.GetTimeLength() + 100000 / 2) / 100000;
						std::wstring time = SkinText::TimeString(pos, false, libAudio.GetTimeLength(), true);
						toolTips.TrackingToolTip(false, time, point.x, skinDraw.GetHoverElement()->rcRect.top, true);
					}
				}
				else
					::SetTimer(thisWnd, TimerValue::TrackID, TimerValue::Track, NULL);
			}
			else if (isTrackTooltip)
			{
				isTrackTooltip = false;
				toolTips.DeactivateTrackingToolTip();
			}
		}
	}
}

void WinylWnd::OnMouseLeave()
{
	moveResize.MouseLeave();
	skinDraw.MouseLeave();
	if (isTrackTooltip)
	{
		isTrackTooltip = false;
		toolTips.DeactivateTrackingToolTip();
	}
}

void WinylWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (skinEdit->IsFocus())
		::SetFocus(thisWnd);

	SkinElement* element = skinDraw.MouseDown(nFlags, point);

	if (skinDraw.IsSplitterPress())
	{
		::SetCapture(thisWnd);
		skinDraw.SplitterLayoutClick(skinDraw.GetPressElement(), point.x, point.y);
	}
	else if (skinDraw.IsPressElement())
	{
		::SetCapture(thisWnd);
		if (element)
			Action(element, MouseAction::Down);

		if (element && element->type == SkinElement::Type::Volume)
		{
			int thumb = static_cast<SkinSlider*>(element)->GetThumbPosition();
			std::wstring percent = std::to_wstring(element->GetParam() / 1000);
			toolTips.TrackingToolTip(true, percent, element->rcRect.left + thumb, element->rcRect.top);
		}
		else if (element && element->type == SkinElement::Type::Track && isMediaPlay && !isMediaRadio)
		{
			int thumb = static_cast<SkinSlider*>(element)->GetThumbPosition();
			int pos = (element->GetParam() * libAudio.GetTimeLength() + 100000 / 2) / 100000;
			std::wstring time = SkinText::TimeString(pos, false, libAudio.GetTimeLength(), true);
			toolTips.TrackingToolTip(true, time, element->rcRect.left + thumb, element->rcRect.top, true);
		}
	}
	else // Move window
	{
		skinDraw.EmptyClick();

		if (skinAlpha)
			moveResize.MouseDown(thisWnd, point, true);
		else
			moveResize.MouseDown(thisWnd, point, skinDraw.IsStyleBorder());
	}
}

void WinylWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	SkinElement* element = skinDraw.MouseUp(nFlags, point);

	if (element)
		Action(element, MouseAction::Up);

	if (element && element->type == SkinElement::Type::Volume)
		toolTips.DeactivateTrackingToolTip();
	else if (element && element->type == SkinElement::Type::Track && !isTrackTooltip)
		toolTips.DeactivateTrackingToolTip();
}

void WinylWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (!skinDraw.IsClickElement())
		MiniPlayer(true);
}

void WinylWnd::OnNcHitTest(CPoint point)
{
	CPoint pt = point;
	::ScreenToClient(thisWnd, &pt);

	if (skinAlpha)
		moveResize.MouseMove(thisWnd, pt, true);
	else if (skinDraw.IsHoverElement())
		moveResize.MouseMove(thisWnd, pt, true);
	else
		moveResize.MouseMove(thisWnd, pt, skinDraw.IsStyleBorder());
}

bool WinylWnd::OnSetCursor(HWND hWnd, UINT nHitTest, UINT message)
{
//	if (message == 0) // When open a menu set default cursor
//	{
//		::SetCursor(::LoadCursorW(NULL, IDC_ARROW));
//		return true;
//	}
	if (message == 0) // When open a menu set default cursor
		return false;

	if (isRadioWaitCursor && ::GetForegroundWindow() == thisWnd)
	{
		::SetCursor(::LoadCursorW(NULL, IDC_APPSTARTING));
		return true;
	}

	if (hWnd == thisWnd && skinDraw.IsSplitterHover())
	{
		assert(skinDraw.GetHoverElement()->GetType() == SkinElement::Type::Splitter);

		if (skinDraw.GetHoverElement()->GetType() == SkinElement::Type::Splitter)
		{
			if (static_cast<SkinSplitter*>(skinDraw.GetHoverElement())->IsHorizontal())
				::SetCursor(::LoadCursorW(NULL, IDC_SIZEWE));
			else
				::SetCursor(::LoadCursorW(NULL, IDC_SIZENS));

			return true;
		}
	}

//	if (pWnd == this && skinDraw.GetHoverElement() &&
//		(skinDraw.GetHoverElement()->iType == SkinElement::TYPE_VOLUME ||
//		skinDraw.GetHoverElement()->iType == SkinElement::TYPE_TRACK))
//	{
//		::SetCursor(::LoadCursorW(NULL, IDC_HAND));
//		return true;
//	}

	// hWnd == thisWnd is needed because sometimes a wrong cursor is set
	// if mouse is not over the main window and over playlist/library for example
	if (hWnd == thisWnd && moveResize.SetCursor() && !skinDraw.IsHoverElement())
		return true;

	return false;
}

void WinylWnd::OnDestroy()
{
	::KillTimer(thisWnd, TimerValue::PosID);
	::KillTimer(thisWnd, TimerValue::TimeID);
	if (!visuals.empty())
		::KillTimer(thisWnd, TimerValue::VisID);

	trayIcon.DeleteIcon(thisWnd);

	if (settings.IsLastFM())
		lastFM.Stop();
	//if (settings.IsMSN())
	//	Messengers::MSNStop();

	settings.SetLastPlayIndex(0);

	if (settings.GetNowPlayingType() > 0)
	{
		settings.ReturnNowPlaying();

		if (skinList->GetLastPlayNode())
		{
			if (skinList->GetLastPlayNode()->idPlaylist)
				settings.SetLastPlayIndex(skinList->GetLastPlayNode()->idPlaylist);
			else if (skinList->GetLastPlayNode()->idLibrary)
				settings.SetLastPlayIndex(skinList->GetLastPlayNode()->idLibrary);
		}
	}

	SaveSettings();

	PostQuitMessage(0);
}

bool WinylWnd::SaveSettings()
{
	settings.SetProfilePath(profilePath);

	// Use GetWindowPlacement instead of GetWindowRect or if minimized coords will be negative
	WINDOWPLACEMENT pl;
	GetWindowPlacement(thisWnd, &pl);
	settings.SetWinPosition(CRect(pl.rcNormalPosition));

	// Adjust the main window position and size by Alpha Window if present
	settings.FixWinPosAlphaSet(skinDraw.IsLayeredAlpha(), skinDraw.GetAlphaBorder());

	if (::IsZoomed(thisWnd))
		settings.SetMaximized(true);
	else
		settings.SetMaximized(false);

	if (skinMini)
	{
		//WINDOWPLACEMENT pl;
		//::GetWindowPlacement(skinMini->Wnd(), &pl);
		//settings.SetWinPositionMini(CRect(pl.rcNormalPosition));
		settings.SetWinPositionMini(skinMini->rcValidRect);
	}

	settings.SetLanguage(lang.GetLanguage());

	//settings.SetVolume(libAudio.GetVolume());
	//settings.SetMute(libAudio.GetMute());

	skinDraw.SaveSkinSettings(profilePath, settings.GetSkin());

	return settings.SaveSettings();
}

void WinylWnd::Action(SkinElement* element, MouseAction mouseAction, bool isSkinDraw)
{
	SkinElement::Type type = element->type;
	int param = element->GetParam();

	switch (type)
	{
	case SkinElement::Type::Close:
		if (dlgProgressPtr)
			dlgProgressPtr->DestroyOnStop();
		else
			::DestroyWindow(thisWnd);
		break;

	case SkinElement::Type::Minimize:
		ActionMinimize();
		break;

	case SkinElement::Type::Maximize:
		ActionMaximize(!settings.IsMaximized());
		break;

	case SkinElement::Type::Play:
		ActionPlay(false, true);
		break;

	case SkinElement::Type::Stop:
		ActionStop();
		break;

	case SkinElement::Type::Track:
		if (mouseAction == MouseAction::Up)
			ActionPosition(param);

		if (isMediaPlay && !isMediaRadio)
		{
			if (mouseAction == MouseAction::Down)
				::KillTimer(thisWnd, TimerValue::TimeID);
			else if (mouseAction == MouseAction::Up)
				::SetTimer(thisWnd, TimerValue::TimeID, TimerValue::Time, NULL);

			int timeLength = libAudio.GetTimeLength();
			if (mouseAction == MouseAction::Down || mouseAction == MouseAction::Move)
				skinDraw.DrawTime((param * timeLength + 100000 / 2) / 100000, timeLength, true);
			else
				skinDraw.DrawTime((param * timeLength + 100000 / 2) / 100000, timeLength, false);
		}
		break;

	case SkinElement::Type::Volume:
		ActionVolume(param, /*false*/isSkinDraw);
		if (skinMini && !isSkinDraw)
			skinMini->skinDraw.DrawVolume(param);
		break;

	case SkinElement::Type::Rating:
		ActionSetRating(param, /*false*/isSkinDraw);
		if (skinMini && !isSkinDraw)
			skinMini->skinDraw.DrawRating(param);
		break;

	case SkinElement::Type::PlayPause:
		ActionPauseEx();
		break;

	case SkinElement::Type::Mute:
		if (param == 1)
			ActionMute(false);
		else
			ActionMute(true);
		break;

	case SkinElement::Type::Repeat:
		if (param == 1)
			ActionRepeat(false);
		else
			ActionRepeat(true);
		break;

	case SkinElement::Type::Shuffle:
		if (param == 1)
			ActionShuffle(false);
		else
			ActionShuffle(true);
		break;

	case SkinElement::Type::MiniPlayer:
		MiniPlayer(true);
		break;

	case SkinElement::Type::Button:
		skinDraw.ChangeTrigger(&element->skinTrigger, 0);
		break;

	case SkinElement::Type::Switch:
		skinDraw.ChangeTrigger(&element->skinTrigger, !param);
		skinDraw.DrawTriggerSwitch(element, !param);
		break;

	case SkinElement::Type::Next:
		ActionNextTrack();
		break;

	case SkinElement::Type::Prev:
		ActionPrevTrack();
		break;

	case SkinElement::Type::SearchClear:
		if (!skinEdit->IsSearchEmpty())
		{
			skinEdit->SearchClear();
			skinDraw.DrawSearchClear(false);
			FillListSearch();
		}
		break;

	case SkinElement::Type::SearchMenu:
	{
		CPoint pt(element->GetRect().left, element->GetRect().bottom);
		::ClientToScreen(thisWnd, &pt);
		contextMenu.ShowSearchMenu(thisWnd, pt);
	}
	break;

	case SkinElement::Type::MainMenu:
	{
		CPoint pt(element->GetRect().left, element->GetRect().bottom);
		if (isSkinDraw && skinAlpha)
			::ClientToScreen(skinAlpha->Wnd(), &pt);
		else
			::ClientToScreen(thisWnd, &pt);
		contextMenu.ShowMainMenu(thisWnd, pt);
	}
	break;

	case SkinElement::Type::Equalizer:
		DialogEqualizer();
		break;
	}
}

bool WinylWnd::OnSysCommand(WPARAM wParam, LPARAM lParam)
{
	if((wParam & 0xFFF0) == SC_MINIMIZE)
	{
		isWindowIconic = true;

		// For layered window use fast (without animation) minimize
		if (skinDraw.IsLayeredAlpha())
		{
			trayIcon.Minimize(thisWnd, true);
			return true;
		}
	}
	else if((wParam & 0xFFF0) == SC_RESTORE)
	{
		isWindowIconic = false;

		// For layered window use fast (without animation) restore
		if (skinDraw.IsLayeredAlpha())
			trayIcon.Restore(thisWnd, true, settings.IsMaximized());

		if (skinList) // When the skin is changing skinList can be nullptr
			skinList->ScrollToPlayNode();
		LyricsThreadReload();

		if (skinDraw.IsLayeredAlpha())
			return true;
	}
	else if((wParam & 0xFFF0) == SC_CLOSE)
	{
		if (dlgProgressPtr)
		{
			dlgProgressPtr->DestroyOnStop();
			return true;
		}
	}

	return false;
}

LRESULT WinylWnd::WindowProcEx(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case UWM_TIMERTHREAD:
		if (skinList->IsSmoothScrollRun())
			skinList->SmoothScrollRun();
		if (skinTree->IsSmoothScrollRun())
			skinTree->SmoothScrollRun();
		if (skinLyrics->IsSmoothScrollRun())
			skinLyrics->SmoothScrollRun();
		return 0;

	// We use custom implementation of Drag'n'Drop
	// so if the window lost the focus we need to stop Drag'n'Drop
	// also we need to process Escape key in WinylApp.cpp
	case WM_ACTIVATE:
		if (isDragDrop)
			StopDragDrop();
		return 0;

	case WM_SETFOCUS:
		::SetFocus(skinList->Wnd());
		return 0;

	// Experimental feature for minimize/restore animation in Win7
	case WM_NCCALCSIZE:
		if (!skinDraw.IsStyleBorder() && !skinDraw.IsLayeredAlpha())
			return 0;
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	case WM_NCACTIVATE:
		if (!skinDraw.IsStyleBorder() && !skinDraw.IsLayeredAlpha())
			lParam = -1;
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	case WM_SETTEXT:
	case WM_SETICON:
		if (!skinDraw.IsStyleBorder() && !skinDraw.IsLayeredAlpha())
		{
			LONG_PTR style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			if ((style & WS_VISIBLE) && ::SetWindowLongPtr(hWnd, GWL_STYLE, style & ~WS_VISIBLE))
			{
				LRESULT result = ::DefWindowProc(hWnd, message, wParam, lParam);
				::SetWindowLongPtr(hWnd, GWL_STYLE, style | WS_VISIBLE);
				return result;
			}
		}
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	// End of the feature

	// Icons in the tray menu (only for WinXP)
	case WM_MEASUREITEM:
		return contextMenu.MeasureDrawTrayMenuXP(true, lParam);
	case WM_DRAWITEM:
		return contextMenu.MeasureDrawTrayMenuXP(false, lParam);

	// DWM messages
	case WM_THEMECHANGED:
		futureWin->ThemeChanged();
		return 0;
	case WM_DWMCOMPOSITIONCHANGED:
		skinDraw.EnableDwm(!!futureWin->IsCompositionEnabled());

		if (skinDraw.IsAeroGlass())
			skinDraw.RedrawWindow();

		if (skinMini)
		{
			skinMini->skinDraw.EnableDwm(!!futureWin->IsCompositionEnabled());

			if (skinMini->skinDraw.IsAeroGlass())
				skinMini->skinDraw.RedrawWindow();
		}
		return 0;
	case WM_DWMSENDICONICTHUMBNAIL:
		win7TaskBar.MessageCoverBitmap(thisWnd, HIWORD(lParam), LOWORD(lParam));
		return 0;
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		win7TaskBar.MessageCoverPreview(thisWnd);
		return 0;

	// Global hotkeys
	case WM_HOTKEY:
		switch ((HotKeys::KeyType)wParam)
		{
		case HotKeys::KeyType::None:
			break;
		case HotKeys::KeyType::Play:
			ActionPlayEx();
			break;
		case HotKeys::KeyType::Pause:
			ActionPauseEx();
			break;
		case HotKeys::KeyType::Stop:
			ActionStop();
			break;
		case HotKeys::KeyType::Next:
			ActionNextTrack();
			break;
		case HotKeys::KeyType::Prev:
			ActionPrevTrack();
			break;
		case HotKeys::KeyType::Mute:
			ActionMute(!settings.IsMute());
			break;
		case HotKeys::KeyType::VolumeUp:
			ActionVolumeUp();
			break;
		case HotKeys::KeyType::VolumeDown:
			ActionVolumeDown();
			break;
		case HotKeys::KeyType::Rating1:
			ActionSetRating(1, true);
			break;
		case HotKeys::KeyType::Rating2:
			ActionSetRating(2, true);
			break;
		case HotKeys::KeyType::Rating3:
			ActionSetRating(3, true);
			break;
		case HotKeys::KeyType::Rating4:
			ActionSetRating(4, true);
			break;
		case HotKeys::KeyType::Rating5:
			ActionSetRating(5, true);
			break;
		case HotKeys::KeyType::Popup:
			if (skinPopup && isMediaPlay)
				skinPopup->Popup();
			break;
		}
		return 0;

	// Media keys
	// http://blogs.msdn.com/b/oldnewthing/archive/2006/04/25/583093.aspx
	case WM_APPCOMMAND:
		switch (GET_APPCOMMAND_LPARAM(lParam))
		{
		case APPCOMMAND_MEDIA_PLAY:
			ActionPlayEx();
			return 1;
		case APPCOMMAND_MEDIA_PAUSE:
		case APPCOMMAND_MEDIA_PLAY_PAUSE:
			ActionPauseEx();
			return 1;
		case APPCOMMAND_MEDIA_STOP:
			ActionStop();
			return 1;
		case APPCOMMAND_MEDIA_NEXTTRACK:
			ActionNextTrack();
			return 1;
		case APPCOMMAND_MEDIA_PREVIOUSTRACK:
			ActionPrevTrack();
			return 1;
		//case APPCOMMAND_VOLUME_MUTE:
		//	ActionMute(!settings.IsMute());
		//	return 1;
		//case APPCOMMAND_VOLUME_UP:
		//	ActionVolumeUp();
		//	return 1;
		//case APPCOMMAND_VOLUME_DOWN:
		//	ActionVolumeDown();
		//	return 1;
		}
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}

	// Taskbar messages
	if (message == wmTaskbarCreated)
	{
		trayIcon.NewIcon(thisWnd, iconSmall);
		return 0;
	}
	if (message == wmTaskbarButtonCreated)
	{
		win7TaskBar.AddButtons(thisWnd, isMediaPlay);
		return 0;
	}

	return WindowProcMy(hWnd, message, wParam, lParam);
	//return ::DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT WinylWnd::WindowProcMy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	case UWM_BASSNEXT:
	{
		libAudio.SyncProcEndImpl();
	}
	return 0;

	case UWM_RADIOSTART:
	{
		StartRadio(libAudio.StartRadio((int)wParam), !!lParam);
	}
	return 0;

	case UWM_BASSCHFREE:
	{
		libAudio.SyncFreeChannelImpl((HSTREAM)lParam);
		return true;
	}
	return 0;

	case UWM_BASSWASTOP:
	{
		libAudio.SyncWAStopPauseImpl(!!wParam, (HSTREAM)lParam);
		return true;
	}
	return 0;

//	case UWM_BASSPRELOAD:
//	{
//		libAudio.SyncProcPreloadImpl();
//		return true;
//	}
//	break;

	case UWM_ACTION:
	{
		SkinElement* element = (SkinElement*)lParam;
		Action(element, (MouseAction)wParam, true);
	}
	return 0;

	case UWM_MINIPLAYER:
	{
		MiniPlayer(wParam ? true : false);
	}
	return 0;

	case UWM_NOMINIPLAYER:
	{
		if (skinMini && settings.IsMiniPlayer())
			MiniPlayer(false);
	}
	return 0;

	case UWM_SEARCHCHANGE:
	{
		if (wParam)
		{
			if (skinEdit->IsSearchEmpty())
				skinDraw.DrawSearchClear(false);
			else
				skinDraw.DrawSearchClear(true);
		}
		FillListSearch();
	}
	return 0;

	case UWM_SEARCHDONE:
	{
		SearchThreadDone();
	}
	return 0;

	case UWM_FILLTREE:
	{
		TreeNodeUnsafe node = (TreeNodeUnsafe)lParam;
		FillTree(node);
	}
	return 0;

	case UWM_FILLLIST:
	{
		TreeNodeUnsafe node = (TreeNodeUnsafe)lParam;
		FillList(node);
	}
	return 0;

	case UWM_LISTSEL:
	{
		UpdateStatusLine();
	}
	return 0;

	case UWM_PLAYFILE:
	{
		ActionPlay(true);
	}
	return 0;

	//case UWM_NEXTFILE:
	//{
	//	*(std::wstring*)lParam = ChangeFile();
	//}
	//return 0;

	case UWM_PLAYDRAW:
	{
		ChangeNode((wParam ? true : false), (lParam ? true : false));
	}
	return 0;

	case UWM_RATING:
	{
		ListNodeUnsafe node = (ListNodeUnsafe)lParam;

		dBase.SetRating(node->idLibrary, node->idPlaylist, node->rating, false);

		if (node == skinList->GetPlayNode())
			skinDraw.DrawRating(node->rating);
	}
	return 0;

	case UWM_CHANGESKIN:
	{
		std::wstring skinName = std::wstring((wchar_t*)lParam);
		
		ActionStop();

		return ReloadSkin(skinName, wParam ? true : false);
	}
	return 0;

	case UWM_LISTMENU:
	{
		std::vector<std::wstring> playlists;
		int selPlaylist = -1;

		TreeNodeUnsafe playlistNode = skinTree->GetPlaylistNode();
		if (playlistNode->HasChild())
		{
			int i = 0;
			for (TreeNodeUnsafe treeNode = playlistNode->Child(); treeNode != nullptr; treeNode = treeNode->Next())
			{
				playlists.push_back(treeNode->GetTitle());

				if (dBase.IsPlaylistOpen() && selPlaylist == -1 &&
					settings.GetLibraryValue() == treeNode->GetValue())
					selPlaylist = i;

				++i;
			}
		}

		contextMenu.ShowListMenu(thisWnd, CPoint((int)wParam, (int)lParam), playlists,
			skinList->IsRadio(), dBase.IsPlaylistOpen(), selPlaylist, dBase.IsNowPlaying() && !dBase.IsNowPlayingOpen());
	}
	return 0;

	case UWM_TREEMENU:
	{
		if (skinTree->GetFocusNode())
		{
			bool isEnable = false;
			if (skinTree->GetFocusNode()->GetNodeType() == SkinTreeNode::NodeType::Text)
				isEnable = true;

			if (skinTree->GetFocusNode()->GetType() == SkinTreeNode::Type::Playlist)
				contextMenu.ShowTreePlaylistMenu(thisWnd, CPoint((int)wParam, (int)lParam), isEnable);
			else if (skinTree->GetFocusNode()->GetType() == SkinTreeNode::Type::Smartlist)
				contextMenu.ShowTreeSmartlistMenu(thisWnd, CPoint((int)wParam, (int)lParam), isEnable);
		}
	}
	return 0;

	case UWM_LYRICSMENU:
	{
		contextMenu.SetLyricsSource(lyricsSource);
		contextMenu.ShowLyricsMenu(thisWnd, CPoint((int)wParam, (int)lParam));
	}
	return 0;

	case UWM_TREESWAP:
	{
		dBase.SavePlaylist(skinTree.get());
		dBase.SaveTreeSmartlists(skinTree.get());
	}
	return 0;

	case UWM_LISTSWAP:
	{
		BeginWaitCursor();
		dBase.SwapPlaylist(skinList.get());
		EndWaitCursor();
	}
	return 0;

	case UWM_RADIOMETA:
	{
		std::wstring title = radioString, artist, album;
		libAudio.GetRadioTags(title, artist, album);

		SkinDrawText(title, album, artist, L"", L"", true);

		SetWindowCaption(artist, title);

		ShowPopup();
	}
	return 0;

//	case UWM_RADIOSTART:
//	{
//		StartRadio((int)wParam);
//	}
//	return 0;

	case UWM_TREEDRAG:
	{
		if (wParam)
			isDragDrop = false;
		else
		{
			isDragDrop = true;

			CPoint ptTree(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			CPoint ptScreen = ptTree;
			::ClientToScreen(skinTree->Wnd(), &ptScreen);
			CPoint ptWin = ptScreen;
			::ScreenToClient(thisWnd, &ptWin);

			HWND wndPoint = ::ChildWindowFromPoint(thisWnd, ptWin);

			if (wndPoint == skinTree->Wnd())
				skinTree->SetDropMovePoint(&ptTree);
			else
				skinTree->SetDropMovePoint(nullptr);
		}
	}
	return 0;

	case UWM_LISTDRAG:
	{
		if (wParam)
		{
			isDragDrop = false;
			wndDragIcon.HideIcon();

			if (wParam == 1)
				DropFilesToPlaylist();
			skinTree->SetDropPoint(nullptr);
		}
		else
		{
			isDragDrop = true;

			CPoint ptList(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			CPoint ptScreen = ptList;
			::ClientToScreen(skinList->Wnd(), &ptScreen);
			CPoint ptWin = ptScreen;
			::ScreenToClient(thisWnd, &ptWin);

			// Track the mouse and use ProcessId to make sure that it is our window
			HWND wndPoint = ::WindowFromPoint(ptScreen);
			DWORD processID = 0;
			::GetWindowThreadProcessId(wndPoint, &processID);
			if (::GetCurrentProcessId() != processID)
				wndPoint = NULL;

			// Mouse over the playlist
			if (wndPoint == skinList->Wnd())
				skinList->SetDropMovePoint(&ptList);
			else
				skinList->SetDropMovePoint(nullptr);

			// Mouse over the library
			if (wndPoint == skinTree->Wnd())
			{
				CPoint ptTree = ptScreen;
				::ScreenToClient(skinTree->Wnd(), &ptTree);
				skinTree->SetDropPoint(&ptTree);
			}
			else
				skinTree->SetDropPoint(nullptr);

			// Mouse leaves the main window
			if (wndPoint == NULL)
			{
				isDragDropOLE = true;
				wndDragIcon.HideIcon();

				ReleaseCapture();
				if (!CopySelectedToClipboard(false))
					::SetCapture(skinList->Wnd()); // Mouse returns to SkinList
				else
					skinList->SetDropMoveStop(); // Files are dropped to other program

				isDragDropOLE = false;
			}
			else
				wndDragIcon.MoveIcon(ptScreen.x, ptScreen.y);
		}
	}
	return 0;

	case UWM_COVERDONE:
	{
		CoverThreadDone();
	}
	return 0;

	case UWM_LYRICSDONE:
	{
		LyricsThreadDone();
	}
	return 0;

	case UWM_LYRICSRECV:
	{
		LyricsThreadReceiving();
	}
	return 0;

	case UWM_AUDIOLOG:
	{
		audioLogWnd.AppendLines(AudioLog::Instance().FlushPending());
	}
	return 0;

	case UWM_TRAYMSG:
	{
		switch(lParam)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			// When click on the tray icon then bring the window to the front
			// or minimize if it's already the foreground window or restore if it's minimized
			if (!settings.IsMiniPlayer())
			{
				CRect rc;
				::GetWindowRect(thisWnd, rc);
				HWND wndPoint = ::WindowFromPoint(rc.CenterPoint());
				DWORD processID = 0;
				::GetWindowThreadProcessId(wndPoint, &processID);

				if (::IsIconic(thisWnd) || ::GetCurrentProcessId() != processID)
				{
					isWindowIconic = false;
					::SetForegroundWindow(thisWnd);
					if (::IsIconic(thisWnd))
						trayIcon.Restore(thisWnd, skinDraw.IsLayeredAlpha(), settings.IsMaximized());
					if (skinList) // When the skin is changing skinList can be nullptr
						skinList->ScrollToPlayNode();
					LyricsThreadReload();
				}
				else
				{
					isWindowIconic = true;
					trayIcon.Minimize(thisWnd, skinDraw.IsLayeredAlpha());
				}
			}
			else
			{
				if (skinMini)
				{
					DWORD processID = 0, currentProcessID = 0;
					if (skinMini->GetZOrder() == 2)
					{
						CRect rc;
						::GetWindowRect(skinMini->Wnd(), rc);
						HWND wndPoint = ::WindowFromPoint(rc.CenterPoint());
						::GetWindowThreadProcessId(wndPoint, &processID);
						currentProcessID = ::GetCurrentProcessId();
					}

					if (::IsIconic(skinMini->Wnd()) || !::IsWindowVisible(skinMini->Wnd()) || currentProcessID != processID)
					{
						::SetForegroundWindow(skinMini->Wnd());
						skinMini->SetVisible(true);
					}
					else
						skinMini->SetVisible(false);
				}
			}
			break;
		//case WM_RBUTTONDOWN:
		//case WM_CONTEXTMENU:
		case WM_RBUTTONUP:
			contextMenu.ShowTrayMenu(thisWnd);
			break;
		case WM_MBUTTONDOWN:
			ActionPauseEx();
			break;
		}
	}
	return 0;

	case UWM_POPUPMENU:
	{
		contextMenu.ShowPopupMenu(thisWnd, CPoint(LOWORD(lParam), HIWORD(lParam)), !!::IsWindowEnabled(thisWnd));
	}
	return 0;

	case UWM_SCANEND:
	{
		ScanLibraryFinish(!!wParam);
	}
	return 0;

	case UWM_STOP:
	{
		ActionStop();
	}
	return 0;

	case UWM_COMMAND:
	{
		switch (wParam)
		{
		case CMD_NULL: return 1;
		case CMD_NONE: return 1;
		case CMD_NONE2: return 1;
		case CMD_MAGIC: return 1237;
		case CMD_VER_WINYL: return Settings::winylVersion;
		case CMD_VER_API: return Settings::apiVersion;
		case CMD_PLAY: ActionPlay(); return 1;
		case CMD_PLAY_EX: ActionPlayEx(); return 1;
		case CMD_PAUSE: ActionPause(); return 1;
		case CMD_PAUSE_EX: ActionPauseEx(); return 1;
		case CMD_STOP: ActionStop(); return 1;
		case CMD_NEXT: ActionNextTrack(); return 1;
		case CMD_PREV: ActionPrevTrack(); return 1;
		case CMD_CLOSE: return 1;
		case CMD_CHECK_PLAY: return isMediaPlay ? 1 : 0;
		case CMD_CHECK_PAUSE: return isMediaPause ? 1 : 0;
		case CMD_CHECK_RADIO: return isMediaRadio ? 1 : 0;
		case CMD_GET_VOLUME: return settings.GetVolume() / 1000;
		case CMD_SET_VOLUME: ActionVolume((int)lParam * 1000, true); return 1;
		case CMD_VOLUME_UP: ActionVolumeUp(); return 1;
		case CMD_VOLUME_DOWN: ActionVolumeDown(); return 1;
		case CMD_GET_MUTE: return settings.IsMute() ? 1 : 0;
		case CMD_MUTE_ON: ActionMute(true); return 1;
		case CMD_MUTE_OFF: ActionMute(false); return 1;
		case CMD_MUTE_REV: ActionMute(!settings.IsMute()); return 1;
		case CMD_GET_REPEAT: return settings.IsRepeat() ? 1 : 0;
		case CMD_REPEAT_ON: ActionRepeat(true); return 1;
		case CMD_REPEAT_OFF: ActionRepeat(false); return 1;
		case CMD_REPEAT_REV: ActionRepeat(!settings.IsRepeat()); return 1;
		case CMD_GET_SHUFFLE: return settings.IsShuffle() ? 1 : 0;
		case CMD_SHUFFLE_ON: ActionShuffle(true); return 1;
		case CMD_SHUFFLE_OFF: ActionShuffle(false); return 1;
		case CMD_SHUFFLE_REV: ActionShuffle(!settings.IsShuffle()); return 1;
		case CMD_GET_RATING: return skinList->GetPlayRating();
		case CMD_SET_RATING: if (isMediaPlay) {ActionSetRating((int)lParam, true); return 1;} return 0;
		}
	}
	return 0;

	}

	return ::DefWindowProc(hWnd, message, wParam, lParam);
}

void WinylWnd::PreTranslateMouseWheel(MSG* msg)
{
	if (msg->message == WM_MOUSEWHEEL)
	{
		if (skinPopup && skinPopup->Wnd() == msg->hwnd) // Just in case
			return;

		bool isReverse = GET_WHEEL_DELTA_WPARAM(msg->wParam) > 0 ? true : false;

		if (skinMini && skinMini->Wnd() == msg->hwnd)
		{
			if (isReverse)
				ActionVolumeUp();
			else
				ActionVolumeDown();
		}
		else
		{
			CPoint pt(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
			::ScreenToClient(thisWnd, &pt);
			HWND wndPoint = ::ChildWindowFromPointEx(thisWnd, pt, CWP_SKIPINVISIBLE);

			if (wndPoint == skinList->Wnd())
			{
				skinList->MouseWheel(isReverse);
			}
			else if (wndPoint == skinTree->Wnd())
			{
				skinTree->MouseWheel(isReverse);
			}
			else if (wndPoint == skinLyrics->Wnd())
			{
				skinLyrics->MouseWheel(isReverse);
			}
			else
			{
				if (isReverse)
					ActionVolumeUp();
				else
					ActionVolumeDown();
			}
		}
	}
}

void WinylWnd::PreTranslateRelayEvent(MSG* msg)
{
	toolTips.RelayEvent(msg);
	if (skinAlpha)
		skinAlpha->toolTips.RelayEvent(msg);
}

void WinylWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TimerValue::FadeID)
	{
		bool isStop = true;

		if (!skinDraw.FadeElement())
			isStop = false;

		if (skinAlpha)
		{
			if (!skinAlpha->skinDraw.FadeElement())
				isStop = false;
		}

		if (skinMini)
		{
			if (!skinMini->skinDraw.FadeElement())
				isStop = false;
		}

		if (isStop)
		{
			::KillTimer(thisWnd, TimerValue::FadeID);
			//::MessageBeep(1);
		}
	}
	else if (nIDEvent == TimerValue::VisID)
	{
		bool isMainWnd = !::IsIconic(thisWnd) && ::IsWindowVisible(thisWnd);
		bool isMiniWnd = skinMini && !::IsIconic(skinMini->Wnd()) && ::IsWindowVisible(skinMini->Wnd());

		if (!isMainWnd && !isMiniWnd)
		{
			// Always kill the timer if all windows are hidded for example
			if (!isMediaPlay || isMediaPause)
			{
				::KillTimer(thisWnd, TimerValue::VisID);
				//::MessageBeep(1);
			}
		}
		else
		{
			// Get FFT data and draw it in visualizers

			float fft[1024];
			libAudio.GetFFT(fft);
			
			bool needStop = true;

			// if (isMainWnd)
			{
				for (std::size_t i = 0, size = visuals.size(); i < size; ++i)
				{
					if (!visuals[i]->SetFFT((isMediaPlay && !isMediaPause) ? fft : nullptr, isMediaPause))
						needStop = false;
				}
			}

			// if (isMiniWnd)
			if (skinMini)
			{
				for (std::size_t i = 0, size = skinMini->visuals.size(); i < size; ++i)
				{
					if (!skinMini->visuals[i]->SetFFT((isMediaPlay && !isMediaPause) ? fft : nullptr, isMediaPause))
						needStop = false;
				}
			}

			if (needStop)
			{
				::KillTimer(thisWnd, TimerValue::VisID);
				//::MessageBeep(1);
			}
		}
	}
	else if (nIDEvent == TimerValue::PosID)
	{
		skinDraw.DrawPosition(libAudio.GetPosition());
	}
	else if (nIDEvent == TimerValue::TimeID)
	{		
		skinDraw.DrawTime(libAudio.GetTimePosition(), libAudio.GetTimeLength(), false);
	}
	else if (nIDEvent == TimerValue::TrackID)
	{
		isTrackTooltip = true;
		::KillTimer(thisWnd, TimerValue::TrackID);

		if (!skinDraw.IsPressElement() && skinDraw.GetHoverElement() && skinDraw.GetHoverElement()->type == SkinElement::Type::Track)
		{
			POINT point = {};
			::GetCursorPos(&point);
			::ScreenToClient(thisWnd, &point);
			int percent = static_cast<SkinSlider*>(skinDraw.GetHoverElement())->CalcPercent(point);
			int pos = (percent * libAudio.GetTimeLength() + 100000 / 2) / 100000;
			std::wstring time = SkinText::TimeString(pos, false, libAudio.GetTimeLength(), true);
			toolTips.TrackingToolTip(true, time, point.x, skinDraw.GetHoverElement()->rcRect.top, true);
		}
	}
}

void WinylWnd::OnContextMenu(HWND hWnd, CPoint point)
{
	if (hWnd == thisWnd)
	{
		if (point.x == -1 && point.y == -1) // Menu key or Shift+F10
			::GetCursorPos(&point);

		contextMenu.ShowMainMenu(thisWnd, point);
	}
}

void WinylWnd::EnableAll(bool isEnable)
{
	if (skinAlpha)
	{
		if (!isEnable) ::SetFocus(thisWnd); // Hack to always show centered around the main window

		if (skinAlpha->IsWnd())
			::EnableWindow(skinAlpha->Wnd(), isEnable);
	}

	if (skinMini)
	{
		if (skinMini->IsWnd())
			::EnableWindow(skinMini->Wnd(), isEnable);
	}
}

void WinylWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	if (skinDraw.IsShadowNative() && !skinShadow && futureWin->IsCompositionEnabled())
	{
		bool isShowActivate =
				(lpwndpos->flags & SWP_SHOWWINDOW) ||
				!(lpwndpos->flags & SWP_NOACTIVATE) ||
				((lpwndpos->flags & SWP_NOMOVE) && (lpwndpos->flags & SWP_NOSIZE) &&
				!(lpwndpos->flags & SWP_FRAMECHANGED)); // !(lpwndpos->flags & SWP_NOZORDER)

		if (isShowActivate)
			skinDraw.EnableDwmShadow();
	}

	if (skinAlpha)
	{
		bool isMove = !(lpwndpos->flags & SWP_NOMOVE);
		bool isSize = !(lpwndpos->flags & SWP_NOSIZE);
		bool isShow = !!(lpwndpos->flags & SWP_SHOWWINDOW);

		if (isMove || isSize || isShow)
		{
			CRect rc = skinDraw.GetAlphaBorder();
			::MoveWindow(skinAlpha->Wnd(), lpwndpos->x - rc.left, lpwndpos->y - rc.top,
				lpwndpos->cx + rc.left + rc.right, lpwndpos->cy + rc.top + rc.bottom, FALSE);
		}
	}

	if (skinShadow)
	{
		bool isMove = !(lpwndpos->flags & SWP_NOMOVE);
		bool isSize = !(lpwndpos->flags & SWP_NOSIZE);
		bool isShow = !!(lpwndpos->flags & SWP_SHOWWINDOW);

		if (isMove || isSize || isShow)
			skinShadow->Move(lpwndpos->x, lpwndpos->y, lpwndpos->cx, lpwndpos->cy);
	}
}

void WinylWnd::StopDragDrop()
{
	if (isDragDropOLE)
		return;

	isDragDrop = false;
	wndDragIcon.HideIcon();

	skinList->SetDropMoveStop();
	skinTree->SetDropMoveStop();
	skinTree->SetDropPoint(nullptr);
}

void WinylWnd::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// Set min and max window size
	lpMMI->ptMaxTrackSize.x = skinDraw.GetMaxSize().cx;
	lpMMI->ptMaxTrackSize.y = skinDraw.GetMaxSize().cy;

	lpMMI->ptMinTrackSize.x = skinDraw.GetMinSize().cx;
	lpMMI->ptMinTrackSize.y = skinDraw.GetMinSize().cy;

	// Adjust min and max size if alpha window is present
	if (skinDraw.IsLayeredAlpha())
	{
		lpMMI->ptMaxTrackSize.x -= skinDraw.GetAlphaBorder().left + skinDraw.GetAlphaBorder().right;
		lpMMI->ptMaxTrackSize.y -= skinDraw.GetAlphaBorder().top + skinDraw.GetAlphaBorder().bottom;

		lpMMI->ptMinTrackSize.x -= skinDraw.GetAlphaBorder().left + skinDraw.GetAlphaBorder().right;
		lpMMI->ptMinTrackSize.y -= skinDraw.GetAlphaBorder().top + skinDraw.GetAlphaBorder().bottom;
	}

	// Set size for maximized window, only if window is borderless
	if (!skinDraw.IsStyleBorder())
	{
		CRect rcRect;

		// Get work rect (depending on the number of monitors)
		if (::GetSystemMetrics(SM_CMONITORS) > 1)
		{
			HMONITOR hMon = ::MonitorFromWindow(thisWnd, MONITOR_DEFAULTTONEAREST);

			MONITORINFO mi = {};
			mi.cbSize = sizeof(mi);
			::GetMonitorInfoW(hMon, &mi);
			rcRect = mi.rcWork;

			// http://blogs.msdn.com/b/llobo/archive/2006/08/01/maximizing-window-_2800_with-windowstyle_3d00_none_2900_-considering-taskbar.aspx
			// Move work rect for second, third etc. monitor to default (0, 0) coordinates
			// i.e. work rect for any monitor with same resolution and taskbar position must be equal
			rcRect.MoveToX(mi.rcWork.left - mi.rcMonitor.left);
			rcRect.MoveToY(mi.rcWork.top - mi.rcMonitor.top);
		}
		else
		{
			RECT rc = {};
			::SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0);
			rcRect = rc;
		}

		// Add maximize border
		rcRect.left -= skinDraw.GetMaximizeBorder().left;
		rcRect.top -= skinDraw.GetMaximizeBorder().top;
		rcRect.right += skinDraw.GetMaximizeBorder().right;
		rcRect.bottom += skinDraw.GetMaximizeBorder().bottom;

		// Adjust window size if alpha window is present
		if (skinDraw.IsLayeredAlpha())
		{
			rcRect.left += skinDraw.GetAlphaBorder().left;
			rcRect.top += skinDraw.GetAlphaBorder().top;
			rcRect.right -= skinDraw.GetAlphaBorder().right;
			rcRect.bottom -= skinDraw.GetAlphaBorder().bottom;
		}

		lpMMI->ptMaxPosition.x = rcRect.left;
		lpMMI->ptMaxPosition.y = rcRect.top;
		lpMMI->ptMaxSize.x = rcRect.Width();
		lpMMI->ptMaxSize.y = rcRect.Height();
	}
}

void WinylWnd::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMTTDISPINFO nmtt = reinterpret_cast<LPNMTTDISPINFO>(lParam);
	if (nmtt->hdr.hwndFrom == toolTips.GetTipWnd() && nmtt->hdr.code == TTN_NEEDTEXT)
	{
		const std::wstring* text = toolTips.GetText((SkinElement*)wParam);
		if (text)
			nmtt->lpszText = (LPWSTR)text->c_str();
	}
}

