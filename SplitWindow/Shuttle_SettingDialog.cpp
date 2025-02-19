﻿#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

void SettingDialog::init(HWND hwnd)
{
	Shuttle::init(hwnd);
}

Container* SettingDialog::onCreateDockContainer()
{
	DWORD dockStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	return new ScrollContainer(this, dockStyle);
}

Container* SettingDialog::onCreateFloatContainer()
{
	DWORD floatStyle = WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	floatStyle |= WS_THICKFRAME;

	return new ScrollContainer(this, floatStyle);
}

LRESULT SettingDialog::onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_RBUTTONDOWN:
		{
			::SetFocus(m_hwnd);

			// このまま WM_RBUTTONUP に処理を続ける。
		}
	case WM_RBUTTONUP:
		{
			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// マウス座標を変換する。
			::MapWindowPoints(hwnd, m_hwnd, &point, 1);

			// 設定ダイアログにメッセージを転送する。
			return ::SendMessage(m_hwnd, message, wParam, MAKELPARAM(point.x, point.y));
		}
	}

	return Shuttle::onContainerWndProc(container, hwnd, message, wParam, lParam);
}

LRESULT SettingDialog::onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		{
			// 「patch.aul」用。
			// 設定ダイアログが高速描画されているときは
			// 親ウィンドウ (コンテナ) を手動で再描画しなければならない。
			::InvalidateRect(::GetParent(hwnd), 0, FALSE);

			break;
		}
	}

	return Shuttle::onTargetWndProc(container, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
