﻿#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

WNDPROC getClassProc(LPCWSTR className)
{
	WNDCLASSEXW wc = { sizeof(wc) };
	::GetClassInfoExW(0, className, &wc);
	return wc.lpfnWndProc;
}

void initHook()
{
	MY_TRACE(_T("initHook()\n"));

	true_ComboBoxProc = getClassProc(WC_COMBOBOXW);
	true_TrackBarProc = getClassProc(TRACKBAR_CLASSW);

	HMODULE user32 = ::GetModuleHandle(_T("user32.dll"));
	true_CreateWindowExA = (Type_CreateWindowExA)::GetProcAddress(user32, "CreateWindowExA");

	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(ComboBoxProc);
	ATTACH_HOOK_PROC(TrackBarProc);
	ATTACH_HOOK_PROC(CreateWindowExA);
	ATTACH_HOOK_PROC(MoveWindow);
	ATTACH_HOOK_PROC(SetWindowPos);
	ATTACH_HOOK_PROC(GetMenu);
	ATTACH_HOOK_PROC(SetMenu);
	ATTACH_HOOK_PROC(DrawMenuBar);

	ATTACH_HOOK_PROC(FindWindowA);
	ATTACH_HOOK_PROC(FindWindowW);
	ATTACH_HOOK_PROC(FindWindowExA);
	ATTACH_HOOK_PROC(GetWindow);
	ATTACH_HOOK_PROC(EnumThreadWindows);
	ATTACH_HOOK_PROC(EnumWindows);
	ATTACH_HOOK_PROC(SetWindowLongA);

	if (DetourTransactionCommit() == NO_ERROR)
	{
		MY_TRACE(_T("API フックに成功しました\n"));
	}
	else
	{
		MY_TRACE(_T("API フックに失敗しました\n"));
	}
}

void termHook()
{
	MY_TRACE(_T("termHook()\n"));
}

void addShuttleToMap(ShuttlePtr shuttle, LPCTSTR name, HWND hwnd)
{
	g_shuttleMap[name] = shuttle;
	shuttle->m_name = name;
	shuttle->init(hwnd);
}

void hookExEdit()
{
	// 拡張編集が読み込まれたのでアドレスを取得する。
	g_auin.initExEditAddress();

	DWORD exedit = g_auin.GetExEdit();

	// rikky_memory.auf + rikky_module.dll 用のフック。
	true_ScriptParamDlgProc = writeAbsoluteAddress(exedit + 0x3454 + 1, hook_ScriptParamDlgProc);

	// スポイト処理の ::GetPixel() をフックする。
	hookAbsoluteCall(exedit + 0x22128, Dropper_GetPixel);

	{
		// キーボードフック処理の ::GetActiveWindow() をフックする。

		BYTE code[6];
		code[0] = (BYTE)0x90; // NOP
		code[1] = (BYTE)0xBD; // MOV EBP,DWORD
		*(DWORD*)&code[2] = (DWORD)KeyboardHook_GetActiveWindow;

		writeCode(exedit + 0x30D0E, code, sizeof(code));
	}

	for (int i = 0; i < 10; i++)
	{
		// vsthost_N.auf 内の ::DialogBoxIndirectParamA() をフックする。

		TCHAR fileName[MAX_PATH] = {};
		::StringCbPrintf(fileName, sizeof(fileName), _T("vsthost_%d.auf"), i + 1);
		MY_TRACE_TSTR(fileName);

		HMODULE vsthost = ::GetModuleHandle(fileName);
		MY_TRACE_HEX(vsthost);

		if (vsthost)
		{
			true_vsthost_DialogBoxIndirectParamA = hookImportFunc(
				vsthost, "DialogBoxIndirectParamA", hook_vsthost_DialogBoxIndirectParamA);
			MY_TRACE_HEX(true_vsthost_DialogBoxIndirectParamA);
		}
	}
}

//---------------------------------------------------------------------

IMPLEMENT_HOOK_PROC_NULL(LRESULT, WINAPI, ComboBoxProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_MOUSEWHEEL:
		{
			// スクロールを優先する場合はコンボボックスのウィンドウ関数は呼ばない。
			if (g_forceScroll)
				return ::DefWindowProcW(hwnd, message, wParam, lParam);

			break;
		}
	}

	return true_ComboBoxProc(hwnd, message, wParam, lParam);
}

IMPLEMENT_HOOK_PROC_NULL(LRESULT, WINAPI, TrackBarProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_MOUSEWHEEL:
		{
			// トラックバーのウィンドウ関数は呼ばない。
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}

	return true_TrackBarProc(hwnd, message, wParam, lParam);
}

IMPLEMENT_HOOK_PROC_NULL(HWND, WINAPI, CreateWindowExA, (DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param))
{
	if ((DWORD)className <= 0x0000FFFF)
	{
		// className が ATOM の場合は何もしない。
		return true_CreateWindowExA(exStyle, className, windowName, style, x, y, w, h, parent, menu, instance, param);
	}

	// デバッグ用出力。
//	MY_TRACE(_T("CreateWindowExA(%hs, %hs)\n"), className, windowName);

	if (::lstrcmpiA(className, "SplitWindow") == 0)
	{
		// シングルウィンドウのクラス名を置き換える。
		className = "AviUtl";
	}
	else if (::lstrcmpiA(windowName, "AviUtl") == 0)
	{
		// AviUtl ウィンドウが作成される直前のタイミング。

		// AviUtl 関連のアドレス情報を初期化する。
		g_auin.initAviUtlAddress();

		// 土台となるシングルウィンドウを作成する。
		g_hub = createHub();

		// コンテナのウィンドウクラスを登録する。
		Container::registerWndClass();
	}
	else if (::lstrcmpiA(className, "AviUtl") == 0 && parent && parent == g_aviutlWindow->m_hwnd)
	{
//		MY_TRACE_STR(windowName);

		// AviUtl のポップアップウィンドウの親をシングルウィンドウに変更する。
		parent = g_hub;
	}

	HWND hwnd = true_CreateWindowExA(exStyle, className, windowName, style, x, y, w, h, parent, menu, instance, param);

	if (::lstrcmpiA(className, "AviUtl") == 0)
	{
		if (::lstrcmpiA(windowName, "AviUtl") == 0)
		{
			MY_TRACE_STR(windowName);

			// AviUtl ウィンドウのコンテナの初期化。
			addShuttleToMap(g_aviutlWindow, _T("* AviUtl"), hwnd);
			::SetProp(g_hub, _T("AviUtlWindow"), hwnd);
		}
		else if (::lstrcmpiA(windowName, "拡張編集") == 0)
		{
			MY_TRACE_STR(windowName);

			hookExEdit();

			// 拡張編集ウィンドウのコンテナの初期化。
			addShuttleToMap(g_exeditWindow, _T("* 拡張編集"), hwnd);
			::SetProp(g_hub, _T("ExEditWindow"), hwnd);
		}
		else if (parent && parent == g_hub)
		{
			MY_TRACE_STR(windowName);

			// その他のプラグインウィンドウのコンテナの初期化。
			ShuttlePtr shuttle(new Shuttle());
			addShuttleToMap(shuttle, windowName, hwnd);
		}
	}
	else if (::lstrcmpiA(windowName, "ExtendedFilter") == 0)
	{
		// 設定ダイアログのコンテナの初期化。
		addShuttleToMap(g_settingDialog, _T("* 設定ダイアログ"), hwnd);
		::SetProp(g_hub, _T("SettingDialog"), hwnd);

		// すべてのウィンドウの初期化処理が終わったので
		// ポストメッセージ先で最初のレイアウト計算を行う。
		::PostMessage(g_hub, WindowMessage::WM_POST_INIT, 0, 0);
	}

	return hwnd;
}

Shuttle* getWindow(HWND hwnd)
{
	// WS_CHILD スタイルがあるかチェックする。
	DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
	if (!(style & WS_CHILD)) return 0;

	return Shuttle::getShuttle(hwnd);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, MoveWindow, (HWND hwnd, int x, int y, int w, int h, BOOL repaint))
{
	Shuttle* shuttle = getWindow(hwnd);

	if (!shuttle) // Shuttle を取得できない場合はデフォルト処理を行う。
		return true_MoveWindow(hwnd, x, y, w, h, repaint);

//	MY_TRACE(_T("MoveWindow(0x%08X, %d, %d, %d, %d)\n"), hwnd, x, y, w, h);
//	MY_TRACE_HWND(hwnd);
//	MY_TRACE_WSTR((LPCWSTR)shuttle->m_name);

	// ターゲットのウィンドウ位置を更新する。
	BOOL result = true_MoveWindow(hwnd, x, y, w, h, repaint);

	// 親ウィンドウを取得する。
	HWND parent = ::GetParent(hwnd);

	// 親ウィンドウがドッキングコンテナなら
	if (parent == shuttle->m_dockContainer->m_hwnd)
	{
		// ドッキングコンテナにターゲットの新しいウィンドウ位置を算出させる。
		shuttle->m_dockContainer->onWndProc(parent, WM_SIZE, 0, 0);
	}
	// 親ウィンドウがフローティングコンテナなら
	else if (parent == shuttle->m_floatContainer->m_hwnd)
	{
		WINDOWPOS wp = {};
		wp.x = x; wp.y = y; wp.cx = w; wp.cy = h;

		// フローティングコンテナに自身の新しいウィンドウ位置を算出させる。
		if (shuttle->m_floatContainer->onSetContainerPos(&wp))
		{
			// コンテナのウィンドウ位置を更新する。
			true_MoveWindow(parent, wp.x, wp.y, wp.cx, wp.cy, repaint);
		}
	}

	return result;
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, SetWindowPos, (HWND hwnd, HWND insertAfter, int x, int y, int w, int h, UINT flags))
{
	Shuttle* shuttle = getWindow(hwnd);

	if (!shuttle) // Shuttle を取得できない場合はデフォルト処理を行う。
		return true_SetWindowPos(hwnd, insertAfter, x, y, w, h, flags);

//	MY_TRACE(_T("SetWindowPos(0x%08X, %d, %d, %d, %d)\n"), hwnd, x, y, w, h);
//	MY_TRACE_HWND(hwnd);
//	MY_TRACE_WSTR((LPCWSTR)shuttle->m_name);

	// ターゲットのウィンドウ位置を更新する。
	BOOL result = true_SetWindowPos(hwnd, insertAfter, x, y, w, h, flags);

	// 親ウィンドウを取得する。
	HWND parent = ::GetParent(hwnd);

	// 親ウィンドウがドッキングコンテナなら
	if (parent == shuttle->m_dockContainer->m_hwnd)
	{
		// ドッキングコンテナにターゲットの新しいウィンドウ位置を算出させる。
		shuttle->m_dockContainer->onWndProc(parent, WM_SIZE, 0, 0);
	}
	// 親ウィンドウがフローティングコンテナなら
	else if (parent == shuttle->m_floatContainer->m_hwnd)
	{
		WINDOWPOS wp = {};
		wp.x = x; wp.y = y; wp.cx = w; wp.cy = h;

		// フローティングコンテナに自身の新しいウィンドウ位置を算出させる。
		if (shuttle->m_floatContainer->onSetContainerPos(&wp))
		{
			// コンテナのウィンドウ位置を更新する。
			true_SetWindowPos(parent, insertAfter, wp.x, wp.y, wp.cx, wp.cy, flags);
		}
	}

	return result;
}

/*
	GetMenu、SetMenu、DrawMenuBar では
	AviUtl ウィンドウのハンドルが渡されたとき、シングルウィンドウのハンドルに取り替えて偽装する。
	これによって、AviUtl ウィンドウのメニュー処理がシングルウィンドウに対して行われるようになる。
*/
HWND getMenuOwner(HWND hwnd)
{
	if (hwnd == g_aviutlWindow->m_hwnd)
	{
		hwnd = g_hub;
	}
	else
	{
		Shuttle* shuttle = Shuttle::getShuttle(hwnd);
		if (shuttle)
			hwnd = shuttle->m_floatContainer->m_hwnd;
	}

	return hwnd;
}

IMPLEMENT_HOOK_PROC(HMENU, WINAPI, GetMenu, (HWND hwnd))
{
//	MY_TRACE(_T("GetMenu(0x%08X)\n"), hwnd);

	hwnd = getMenuOwner(hwnd);

	return true_GetMenu(hwnd);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, SetMenu, (HWND hwnd, HMENU menu))
{
//	MY_TRACE(_T("SetMenu(0x%08X, 0x%08X)\n"), hwnd, menu);

	hwnd = getMenuOwner(hwnd);

	return true_SetMenu(hwnd, menu);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, DrawMenuBar, (HWND hwnd))
{
//	MY_TRACE(_T("DrawMenuBar(0x%08X)\n"), hwnd);

	hwnd = getMenuOwner(hwnd);

	return true_DrawMenuBar(hwnd);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, FindWindowA, (LPCSTR className, LPCSTR windowName))
{
	MY_TRACE(_T("FindWindowA(%hs, %hs)\n"), className, windowName);

	// 「ショートカット再生」用。
	if (className && windowName && ::lstrcmpiA(className, "AviUtl") == 0)
	{
		auto it = g_shuttleMap.find(windowName);
		if (it != g_shuttleMap.end())
			return it->second->m_hwnd;
	}

	return true_FindWindowA(className, windowName);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, FindWindowW, (LPCWSTR className, LPCWSTR windowName))
{
	MY_TRACE(_T("FindWindowW(%ws, %ws)\n"), className, windowName);

	// 「PSDToolKit」の「送る」用。
	if (className && ::lstrcmpiW(className, L"ExtendedFilterClass") == 0)
		return g_settingDialog->m_hwnd;

	return true_FindWindowW(className, windowName);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, FindWindowExA, (HWND parent, HWND childAfter, LPCSTR className, LPCSTR windowName))
{
	MY_TRACE(_T("FindWindowExA(0x%08X, 0x%08X, %hs, %hs)\n"), parent, childAfter, className, windowName);

	if (!parent && className)
	{
		// 「テキスト編集補助プラグイン」用。
		if (::lstrcmpiA(className, "ExtendedFilterClass") == 0)
		{
			return g_settingDialog->m_hwnd;
		}
		// 「キーフレームプラグイン」用。
		else if (::lstrcmpiA(className, "AviUtl") == 0 && windowName)
		{
			auto it = g_shuttleMap.find(windowName);
			if (it != g_shuttleMap.end())
			{
				MY_TRACE(_T("%hs を返します\n"), windowName);

				return it->second->m_hwnd;
			}
		}
	}

	return true_FindWindowExA(parent, childAfter, className, windowName);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, GetWindow, (HWND hwnd, UINT cmd))
{
//	MY_TRACE(_T("GetWindow(0x%08X, %d)\n"), hwnd, cmd);
//	MY_TRACE_HWND(hwnd);

	if (cmd == GW_OWNER)
	{
		if (hwnd == g_exeditWindow->m_hwnd)
		{
			// 拡張編集ウィンドウのオーナーウィンドウは AviUtl ウィンドウ。
			return g_aviutlWindow->m_hwnd;
		}
		else if (hwnd == g_settingDialog->m_hwnd)
		{
			// 設定ダイアログのオーナーウィンドウは拡張編集ウィンドウ。
			return g_exeditWindow->m_hwnd;
		}

		HWND retValue = true_GetWindow(hwnd, cmd);

		if (retValue == g_hub)
		{
			// 「スクリプト並べ替え管理」「シークバー＋」などの一般的なプラグイン用。
			// シングルウィンドウがオーナーになっている場合は AviUtl ウィンドウを返すようにする。
			return g_aviutlWindow->m_hwnd;
		}

		return retValue;
	}

	return true_GetWindow(hwnd, cmd);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, EnumThreadWindows, (DWORD threadId, WNDENUMPROC enumProc, LPARAM lParam))
{
	MY_TRACE(_T("EnumThreadWindows(%d, 0x%08X, 0x%08X)\n"), threadId, enumProc, lParam);

	// 「イージング設定時短プラグイン」用。
	if (threadId == ::GetCurrentThreadId() && enumProc && lParam)
	{
		// enumProc() の中で ::GetWindow() が呼ばれる。
		if (!enumProc(g_settingDialog->m_hwnd, lParam))
			return FALSE;
	}

	return true_EnumThreadWindows(threadId, enumProc, lParam);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, EnumWindows, (WNDENUMPROC enumProc, LPARAM lParam))
{
	MY_TRACE(_T("EnumWindows(0x%08X, 0x%08X)\n"), enumProc, lParam);

	// 「拡張編集RAMプレビュー」用。
	if (enumProc && lParam)
	{
		if (!enumProc(g_aviutlWindow->m_hwnd, lParam))
			return FALSE;
	}

	return true_EnumWindows(enumProc, lParam);
}

IMPLEMENT_HOOK_PROC(LONG, WINAPI, SetWindowLongA, (HWND hwnd, int index, LONG newLong))
{
//	MY_TRACE(_T("SetWindowLongA(0x%08X, %d, 0x%08X)\n"), hwnd, index, newLong);

	// 「拡張ツールバー」用。
	if (index == GWL_HWNDPARENT)
	{
		Shuttle* shuttle = Shuttle::getShuttle(hwnd);

		if (shuttle)
		{
			MY_TRACE(_T("Shuttle が取得できるウィンドウは HWNDPARENT を変更できません\n"));
			return 0;
		}
	}

	return true_SetWindowLongA(hwnd, index, newLong);
}

IMPLEMENT_HOOK_PROC_NULL(INT_PTR, CALLBACK, ScriptParamDlgProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			MY_TRACE(_T("ScriptParamDlgProc(WM_INITDIALOG)\n"));

			// rikky_memory.auf + rikky_module.dll 用。
			::PostMessage(g_settingDialog->m_hwnd, WM_NCACTIVATE, FALSE, (LPARAM)hwnd);

			break;
		}
	}

	return true_ScriptParamDlgProc(hwnd, message, wParam, lParam);
}

IMPLEMENT_HOOK_PROC_NULL(INT_PTR, WINAPI, vsthost_DialogBoxIndirectParamA, (HINSTANCE instance, LPCDLGTEMPLATEA dialogTemplate, HWND parent, DLGPROC dlgProc, LPARAM initParam))
{
	MY_TRACE(_T("vsthost_DialogBoxIndirectParamA()\n"));

	HWND dummy = createPopupWindow(parent);

	HWND activeWindow = ::GetActiveWindow();
	::EnableWindow(parent, FALSE);
	INT_PTR result = true_vsthost_DialogBoxIndirectParamA(instance, dialogTemplate, dummy, dlgProc, initParam);
	::EnableWindow(parent, TRUE);
	::SetActiveWindow(activeWindow);

	::DestroyWindow(dummy);

	return result;
}

COLORREF WINAPI Dropper_GetPixel(HDC _dc, int x, int y)
{
	MY_TRACE(_T("Dropper_GetPixel(0x%08X, %d, %d)\n"), _dc, x, y);

	// すべてのモニタのすべての場所から色を抽出できるようにする。

	POINT point; ::GetCursorPos(&point);
	::LogicalToPhysicalPointForPerMonitorDPI(0, &point);
	HDC dc = ::GetDC(0);
	COLORREF color = ::GetPixel(dc, point.x, point.y);
	::ReleaseDC(0, dc);
	return color;
}

// hwnd が child の先祖かどうか調べる。
BOOL isAncestor(HWND hwnd, HWND child)
{
	while (child)
	{
		if (child == hwnd)
			return TRUE;

		child = ::GetParent(child);
	}

	return FALSE;
}

HWND WINAPI KeyboardHook_GetActiveWindow()
{
	MY_TRACE(_T("KeyboardHook_GetActiveWindow()\n"));

	HWND activeWindow = ::GetActiveWindow();
	MY_TRACE_HWND(activeWindow);

	HWND focus = ::GetFocus();
	MY_TRACE_HWND(focus);

	if (!focus)
	{
		MY_TRACE(_T("focus が 0 なので、設定ダイアログを返します\n"));
		return g_settingDialog->m_hwnd;
	}

	if (isAncestor(g_settingDialog->m_hwnd, focus))
	{
		MY_TRACE(_T("設定ダイアログを返します\n"));
		return g_settingDialog->m_hwnd;
	}

	if (isAncestor(g_exeditWindow->m_hwnd, focus))
	{
		MY_TRACE(_T("拡張編集ウィンドウを返します\n"));
		return g_exeditWindow->m_hwnd;
	}

	return activeWindow;
}

//---------------------------------------------------------------------

EXTERN_C BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		{
			// ロケールを設定する。
			// これをやらないと日本語テキストが文字化けするので最初に実行する。
			_tsetlocale(LC_CTYPE, _T(""));

			MY_TRACE(_T("DLL_PROCESS_ATTACH\n"));

			// この DLL のハンドルをグローバル変数に保存しておく。
			g_instance = instance;
			MY_TRACE_HEX(g_instance);

			// この DLL の参照カウンタを増やしておく。
			WCHAR moduleFileName[MAX_PATH] = {};
			::GetModuleFileNameW(g_instance, moduleFileName, MAX_PATH);
			::LoadLibraryW(moduleFileName);

			initHook();

			break;
		}
	case DLL_PROCESS_DETACH:
		{
			MY_TRACE(_T("DLL_PROCESS_DETACH\n"));

			termHook();

			break;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------
