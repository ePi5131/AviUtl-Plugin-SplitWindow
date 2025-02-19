﻿#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

void AviUtlWindow::init(HWND hwnd)
{
	Shuttle::init(hwnd);

	// フローティングコンテナのアイコンを設定する。
	HICON icon = (HICON)::GetClassLong(m_hwnd, GCL_HICON);
	::SendMessage(m_floatContainer->m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
	::SendMessage(m_floatContainer->m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);

	// ハブのアイコンを設定する。
	::SetClassLong(g_hub, GCL_HICON, (LONG)icon);
	::SetClassLong(g_hub, GCL_HICONSM, (LONG)icon);
}

LRESULT AviUtlWindow::onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return Shuttle::onContainerWndProc(container, hwnd, message, wParam, lParam);
}

LRESULT AviUtlWindow::onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		{
			MY_TRACE(_T("AviUtlWindow::onTargetWndProc(WM_CLOSE)\n"));

			// AviUtl が終了しようとしているので設定を保存する。
			saveConfig();

			break;
		}
	case WM_SETTEXT:
		{
//			MY_TRACE(_T("AviUtlWindow::onTargetWndProc(WM_SETTEXT)\n"));

			// コンテナのウィンドウテキストを更新する。
			::SetWindowText(::GetParent(hwnd), (LPCTSTR)lParam);

			if (m_pane)
			{
				// ペインのタイトル部分を再描画する。
				::InvalidateRect(m_pane->m_owner, &m_pane->m_position, FALSE);
			}

			// シングルウィンドウのウィンドウテキストを更新する。

			AviUtl::EditHandle* editp = (AviUtl::EditHandle*)g_auin.GetEditp();

			char fileName[MAX_PATH] = {};
			if (editp->frame_n)
			{
				::StringCbCopyA(fileName, sizeof(fileName), editp->project_filename);
				::PathStripPathA(fileName);

				if (::lstrlenA(fileName) == 0)
					::StringCbCopyA(fileName, sizeof(fileName), (LPCSTR)lParam);
			}
			else
			{
				::StringCbCopyA(fileName, sizeof(fileName), "無題");
			}
			::StringCbCatA(fileName, sizeof(fileName), " - AviUtl");
			::SetWindowTextA(g_hub, fileName);

			return container->onTargetWndProc(hwnd, message, wParam, lParam);
		}
	}

	return Shuttle::onTargetWndProc(container, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
