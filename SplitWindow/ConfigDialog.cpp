﻿#include "pch.h"
#include "ConfigDialog.h"

//---------------------------------------------------------------------

int showConfigDialog(HWND hwnd)
{
	ConfigDialog dialog(hwnd);

	::SetDlgItemInt(dialog, IDC_FILL_COLOR, g_fillColor, FALSE);
	::SetDlgItemInt(dialog, IDC_BORDER_COLOR, g_borderColor, FALSE);
	::SetDlgItemInt(dialog, IDC_HOT_BORDER_COLOR, g_hotBorderColor, FALSE);

	::EnableWindow(hwnd, FALSE);
	int retValue = dialog.doModal();
	::EnableWindow(hwnd, TRUE);
	::SetActiveWindow(hwnd);

	if (IDOK != retValue)
		return retValue;

	g_fillColor = ::GetDlgItemInt(dialog, IDC_FILL_COLOR, 0, FALSE);
	g_borderColor = ::GetDlgItemInt(dialog, IDC_BORDER_COLOR, 0, FALSE);
	g_hotBorderColor = ::GetDlgItemInt(dialog, IDC_HOT_BORDER_COLOR, 0, FALSE);

	// レイアウトを再計算する。
	calcLayout();

	// 再描画する。
	::InvalidateRect(hwnd, 0, FALSE);

	return retValue;
}

//---------------------------------------------------------------------

ConfigDialog::ConfigDialog(HWND hwnd)
	: Dialog(g_instance, MAKEINTRESOURCE(IDD_CONFIG), hwnd)
{
}

void ConfigDialog::onOK()
{
	Dialog::onOK();
}

void ConfigDialog::onCancel()
{
	Dialog::onCancel();
}

INT_PTR ConfigDialog::onDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
			UINT id = LOWORD(wParam);

			switch (id)
			{
			case IDC_FILL_COLOR:
			case IDC_BORDER_COLOR:
			case IDC_HOT_BORDER_COLOR:
				{
					HWND control = (HWND)lParam;

					COLORREF color = ::GetDlgItemInt(hwnd, id, 0, FALSE);

					static COLORREF customColors[16] = {};
					CHOOSECOLOR cc { sizeof(cc) };
					cc.hwndOwner = hwnd;
					cc.lpCustColors = customColors;
					cc.rgbResult = color;
					cc.Flags = CC_RGBINIT | CC_FULLOPEN;
					if (!::ChooseColor(&cc)) return TRUE;

					color = cc.rgbResult;

					::SetDlgItemInt(hwnd, id, color, FALSE);
					::InvalidateRect(control, 0, FALSE);

					return TRUE;
				}
			}

			break;
		}
	case WM_DRAWITEM:
		{
			UINT id = wParam;

			switch (id)
			{
			case IDC_FILL_COLOR:
			case IDC_BORDER_COLOR:
			case IDC_HOT_BORDER_COLOR:
				{
					DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

					COLORREF color = ::GetDlgItemInt(hwnd, id, 0, FALSE);

					HBRUSH brush = ::CreateSolidBrush(color);
					FillRect(dis->hDC, &dis->rcItem, brush);
					::DeleteObject(brush);

					return TRUE;
				}
			}

			break;
		}
	}

	return Dialog::onDlgProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
