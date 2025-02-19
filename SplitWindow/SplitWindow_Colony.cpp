﻿#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

// 他のウィンドウの土台となるベースウィンドウを作成する。
HWND createColony()
{
	MY_TRACE(_T("createColony()\n"));

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hCursor = ::LoadCursor(0, IDC_ARROW);
	wc.lpfnWndProc = colonyProc;
	wc.hInstance = g_instance;
	wc.lpszClassName = _T("SplitWindow.Colony");
	::RegisterClass(&wc);

	HWND hwnd = ::CreateWindowEx(
		0,
		_T("SplitWindow.Colony"),
		_T("SplitWindow.Colony"),
		WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_THICKFRAME |
		WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		g_hub, 0, g_instance, 0);

	return hwnd;
}

//---------------------------------------------------------------------

// ベースウィンドウのウィンドウ関数。
LRESULT CALLBACK colonyProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NOTIFY:
		{
			NMHDR* header = (NMHDR*)lParam;

			switch (header->code)
			{
			case NM_RCLICK:
				{
					showPaneMenu(hwnd);

					break;
				}
			case TCN_SELCHANGE:
				{
					Pane* pane = TabControl::getPane(header->hwndFrom);
					if (pane)
						pane->m_tab.changeCurrent();

					break;
				}
			}

			break;
		}
	case WM_CREATE:
		{
			MY_TRACE(_T("colonyProc(WM_CREATE)\n"));

			// ルートペインを作成する。
			::SetProp(hwnd, _T("SplitWindow.RootPane"), (HANDLE)new PanePtr(new Pane(hwnd)));

			// コレクションに追加する。
			g_colonySet.insert(hwnd);

			break;
		}
	case WM_DESTROY:
		{
			MY_TRACE(_T("colonyProc(WM_DESTROY)\n"));

			// コレクションから削除する。
			g_colonySet.erase(hwnd);

			// ルートペインを削除する。
			PanePtr* root = (PanePtr*)::RemoveProp(hwnd, _T("SplitWindow.RootPane"));

			// ドッキング中のウィンドウが削除されないようにルートペインをリセットする。
			if (hwnd != g_hub)
				root->get()->resetPane();

			delete root;

			break;
		}
	case WM_SIZE:
		{
			calcLayout(hwnd);

			break;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = ::BeginPaint(hwnd, &ps);
			RECT rc = ps.rcPaint;

			BP_PAINTPARAMS pp = { sizeof(pp) };
			HDC mdc = 0;
			HPAINTBUFFER pb = ::BeginBufferedPaint(dc, &rc, BPBF_COMPATIBLEBITMAP, &pp, &mdc);

			if (pb)
			{
				HDC dc = mdc;

				PanePtr root = getRootPane(hwnd);

				{
					// 背景を塗りつぶす。

					fillBackground(dc, &rc);
				}

				{
					// ボーダーを描画する。

					HBRUSH brush = ::CreateSolidBrush(g_borderColor);
					root->drawBorder(dc, brush);
					::DeleteObject(brush);
				}

				{
					// ホットボーダーを描画する。

					// ホットボーダーの矩形を取得できたら
					RECT rcHotBorder;
					if (g_hotBorderPane && g_hotBorderPane->getBorderRect(&rcHotBorder))
					{
						// テーマを使用するなら
						if (g_useTheme)
						{
							int partId = WP_BORDER;
							int stateId = CS_ACTIVE;

							// テーマ API を使用してボーダーを描画する。
							::DrawThemeBackground(g_theme, dc, partId, stateId, &rcHotBorder, 0);
						}
						// テーマを使用しないなら
						else
						{
							// ブラシで塗りつぶす。
							HBRUSH brush = ::CreateSolidBrush(g_hotBorderColor);
							::FillRect(dc, &rcHotBorder, brush);
							::DeleteObject(brush);
						}
					}
				}

				{
					// 各ウィンドウのキャプションを描画する。

					LOGFONTW lf = {};
					::GetThemeSysFont(g_theme, TMT_CAPTIONFONT, &lf);
					HFONT font = ::CreateFontIndirectW(&lf);
					HFONT oldFont = (HFONT)::SelectObject(dc, font);

					root->drawCaption(dc);

					::SelectObject(dc, oldFont);
					::DeleteObject(font);
				}

				::EndBufferedPaint(pb, TRUE);
			}

			::EndPaint(hwnd, &ps);
			return 0;
		}
	case WM_SETCURSOR:
		{
			if (hwnd == (HWND)wParam)
			{
				PanePtr root = getRootPane(hwnd);

				POINT point; ::GetCursorPos(&point);
				::ScreenToClient(hwnd, &point);

				PanePtr borderPane = root->hitTestBorder(point);
				if (!borderPane) break;

				switch (borderPane->m_splitMode)
				{
				case SplitMode::vert:
					{
						::SetCursor(::LoadCursor(0, IDC_SIZEWE));

						return TRUE;
					}
				case SplitMode::horz:
					{
						::SetCursor(::LoadCursor(0, IDC_SIZENS));

						return TRUE;
					}
				}
			}

			break;
		}
	case WM_LBUTTONDOWN:
		{
			MY_TRACE(_T("colonyProc(WM_LBUTTONDOWN)\n"));

			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			if (showTargetMenu(hwnd, point))
				break;

			PanePtr root = getRootPane(hwnd);

			// マウス座標にあるボーダーを取得する。
			g_hotBorderPane = root->hitTestBorder(point);

			// ボーダーが有効かチェックする。
			if (g_hotBorderPane)
			{
				// オフセットを取得する。
				g_hotBorderPane->m_dragOffset = g_hotBorderPane->getDragOffset(point);

				// マウスキャプチャを開始する。
				::SetCapture(hwnd);

				// 再描画する。
				::InvalidateRect(hwnd, &g_hotBorderPane->m_position, FALSE);

				break;
			}

			{
				// マウス座標にあるペインを取得できたら
				PanePtr pane = root->hitTestPane(point);
				if (pane)
				{
					// クリックされたペインがウィンドウを持っているなら
					Shuttle* shuttle = pane->getActiveShuttle();
					if (shuttle)
						::SetFocus(shuttle->m_hwnd); // そのウィンドウにフォーカスを当てる。
				}
			}

			break;
		}
	case WM_LBUTTONUP:
		{
			MY_TRACE(_T("colonyProc(WM_LBUTTONUP)\n"));

			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// マウスをキャプチャ中かチェックする。
			if (::GetCapture() == hwnd)
			{
				// マウスキャプチャを終了する。
				::ReleaseCapture();

				if (g_hotBorderPane)
				{
					// ボーダーを動かす。
					g_hotBorderPane->dragBorder(point);

					// レイアウトを再計算する。
					g_hotBorderPane->recalcLayout();

					// 再描画する。
					::InvalidateRect(hwnd, &g_hotBorderPane->m_position, FALSE);
				}
			}

			break;
		}
	case WM_RBUTTONDOWN:
		{
			MY_TRACE(_T("colonyProc(WM_RBUTTONDOWN)\n"));

			// レイアウト編集メニューを表示する。
			showPaneMenu(hwnd);

			break;
		}
	case WM_MOUSEMOVE:
		{
			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// マウスをキャプチャ中かチェックする。
			if (::GetCapture() == hwnd)
			{
				if (g_hotBorderPane)
				{
					// ボーダーを動かす。
					g_hotBorderPane->dragBorder(point);

					// レイアウトを再計算する。
					g_hotBorderPane->recalcLayout();

					// 再描画する。
					::InvalidateRect(hwnd, &g_hotBorderPane->m_position, FALSE);
				}
			}
			else
			{
				PanePtr root = getRootPane(hwnd);

				// マウス座標にあるボーダーを取得する。
				PanePtr hotBorderPane = root->hitTestBorder(point);

				// 現在のホットボーダーと違う場合は
				if (g_hotBorderPane != hotBorderPane)
				{
					// ホットボーダーを更新する。
					g_hotBorderPane = hotBorderPane;

					// 再描画する。
					::InvalidateRect(hwnd, 0, FALSE);
				}

				// マウスリーブイベントをトラックする。
				TRACKMOUSEEVENT tme = { sizeof(tme) };
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				::TrackMouseEvent(&tme);
			}

			break;
		}
	case WM_MOUSELEAVE:
		{
			MY_TRACE(_T("colonyProc(WM_MOUSELEAVE)\n"));

			// 再描画用のペイン。
			PanePtr pane = g_hotBorderPane;

			// ホットボーダーが存在すれば
			if (g_hotBorderPane)
			{
				// ホットボーダーを無効にする。
				g_hotBorderPane = 0;

				// 再描画する。
				::InvalidateRect(hwnd, &pane->m_position, FALSE);
			}

			break;
		}
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
