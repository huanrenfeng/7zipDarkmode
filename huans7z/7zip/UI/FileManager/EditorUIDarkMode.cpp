#include <unordered_set>
#include <unordered_map>
#include <string_view>
#include <array>
#include <vssym32.h>
#include <Richedit.h>
#include "EditorUIDarkMode.h"
#include "DarkMode.h"

//#pragma comment(lib, "uxtheme.lib")

//extern bool g_darkModeSupported;
//extern bool g_darkModeEnabled;
extern HBRUSH greybrush;

namespace EditorUIDarkMode
{
	std::unordered_map<HTHEME, ThemeType> ThemeHandles;
	bool EnableThemeHooking;

	const std::unordered_map<std::string_view, ThemeType> TargetWindowThemes
	{
		{ "msctls_statusbar32", ThemeType::StatusBar },
		{ "MDIClient", ThemeType::MDIClient },
		{ "Static", ThemeType::Static },
		{ "Edit", ThemeType::Edit },
		{ "RICHEDIT50W", ThemeType::RichEdit },
		{ "Button", ThemeType::Button },
		{ "ComboBox", ThemeType::ComboBox },
		{ "SysHeader32", ThemeType::Header },
		{ "SysListView32", ThemeType::ListView },
		{ "SysTreeView32", ThemeType::TreeView },
		{ "SysTabControl32", ThemeType::TabControl },
	};

	//const std::unordered_set<std::string_view> PermanentWindowSubclasses
	//{
	//	"Creation Kit",
	//	"ActivatorClass",
	//	"AlchemyClass",
	//	"ArmorClass",
	//	"CreatureClass",
	//	"LockPickClass",
	//	"NPCClass",
	//	"WeaponClass",
	//	"FaceClass",
	//	"PlaneClass",
	//	"MonitorClass",
	//	"ViewerClass",
	//	"SpeakerClass",
	//	"LandClass",
	//	"RenderWindow",
	//	"#32770",
	//	// "BABYGRID",
		// "NiTreeCtrl",
	//};

	void Initialize()
	{
		EnableThemeHooking = true;
	}

	void InitializeThread()
	{
		if (EnableThemeHooking)
			SetWindowsHookExA(WH_CALLWNDPROC, CallWndProcCallback, nullptr, GetCurrentThreadId());
	}

	struct ListViewSubclassInfo
	{
		COLORREF headerTextColor;
	};

	LRESULT CALLBACK CallWndProcCallback(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION)
		{
			auto messageData = reinterpret_cast<CWPSTRUCT *>(lParam);

			switch (messageData->message)
			{
			case WM_CREATE:
			case WM_INITDIALOG:
				SetWindowSubclass(messageData->hwnd, WindowSubclass, 0, 0);
				break;
			}
		}

		return CallNextHookEx(nullptr, nCode, wParam, lParam);
	}

	typedef struct
	{
	UINT MessageId;
	WPARAM wParam;
	LPARAM lParam;
	} SMessage, * PSMessage;


//http://forums.codeguru.com/showthread.php?130766-How-to-Send-messages-to-ALL-child-windows
	BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
	{

		auto themeType = ThemeType::None;

		char className[256] = {};
		GetClassNameA(hWnd, className, ARRAYSIZE(className));

		if (auto itr = TargetWindowThemes.find(className); itr != TargetWindowThemes.end())
			themeType = itr->second;

		switch (themeType)
		{
			case ThemeType::ListView:
			{
				PSMessage psMessage = (PSMessage)lParam;
				SendMessage(hWnd, psMessage->MessageId, psMessage->wParam, psMessage->lParam);
			}
		}

		return TRUE;

	}

	LRESULT CALLBACK WindowSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR , DWORD_PTR )
	{
		constexpr COLORREF generalBackgroundColor = RGB(56, 56, 56);
		constexpr COLORREF generalTextColor = RGB(255, 255, 255);
		static HBRUSH generalBackgroundBrush = CreateSolidBrush(generalBackgroundColor);

		switch (uMsg)
		{
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (g_darkModeSupported && g_darkModeEnabled)
			{
				if (HDC hdc = reinterpret_cast<HDC>(wParam); hdc)
				{
					SetTextColor(hdc, generalTextColor);
					SetBkColor(hdc, generalBackgroundColor);
				}

				return reinterpret_cast<INT_PTR>(generalBackgroundBrush);
			}
		}
		break;
		}

		LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

		//if( !(g_darkModeSupported && g_darkModeEnabled))
		//	return result;

		switch (uMsg)
		{
		case WM_SETTINGCHANGE:
			{
				if (IsColorSchemeChangeMessage(lParam))
				{
					g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();

					RefreshTitleBarThemeColor(hWnd);

					SMessage sMessage={};
					sMessage.MessageId = WM_THEMECHANGED;

					EnumChildWindows(hWnd, EnumChildProc, (LPARAM)&sMessage);

					if(g_darkModeEnabled)
						SetClassLongPtr(hWnd,-10,(LONG_PTR)greybrush);
					else
						SetClassLongPtr(hWnd,-10,(LONG_PTR)(HBRUSH) (COLOR_BTNFACE + 1));

					InvalidateRect(hWnd, NULL, TRUE); 

				}
			}
			break;
		case WM_INITDIALOG:
		case WM_CREATE:
		{
			if (g_darkModeSupported)
				{
					_AllowDarkModeForWindow(hWnd, true);
					RefreshTitleBarThemeColor(hWnd);		//make the title bar dark when the window isn't active
				}

			// Theme settings are initialized after WM_CREATE is processed
			auto themeType = ThemeType::None;
			HTHEME scrollBarTheme = nullptr;

			char className[256] = {};
			GetClassNameA(hWnd, className, ARRAYSIZE(className));

			if (auto itr = TargetWindowThemes.find(className); itr != TargetWindowThemes.end())
				themeType = itr->second;

			switch (themeType)
			{
				
			case ThemeType::MDIClient:
				SetWindowSubclass(hWnd, MDIClientSubclass, 0, 0);
				break;

			case ThemeType::RichEdit:
			{
				CHARFORMAT2A format = {};
				format.cbSize = sizeof(format);
				format.dwMask = CFM_COLOR;
				format.crTextColor = RGB(255, 255, 255);
				SendMessageA(hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
				SendMessageA(hWnd, EM_SETBKGNDCOLOR, 0, RGB(56, 56, 56));
			}
			break;

			case ThemeType::ListView:
				SetWindowSubclass(hWnd, ListViewSubclass, 0, reinterpret_cast<DWORD_PTR>(new ListViewSubclassInfo{}));

				SendMessage(hWnd, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

				#if(_WIN32_WINNT >= 0x0501)
				
				{
					HWND hHeader = ListView_GetHeader(hWnd);
				
				  SetWindowTheme(hHeader, L"ItemsView", nullptr); // DarkMode
				
				  
				
					SetWindowTheme(hWnd, L"ItemsView", nullptr); // DarkMode
				}
				  
				#endif

				scrollBarTheme = OpenThemeData(hWnd, VSCLASS_SCROLLBAR);
				break;

			case ThemeType::TreeView:
				TreeView_SetTextColor(hWnd, RGB(255, 255, 255));
				TreeView_SetBkColor(hWnd, RGB(32, 32, 32));

				scrollBarTheme = OpenThemeData(hWnd, VSCLASS_SCROLLBAR);
				break;

			case ThemeType::TabControl:
				SetWindowLongPtrA(hWnd, GWL_STYLE, (GetWindowLongPtrA(hWnd, GWL_STYLE) & ~TCS_BUTTONS) | TCS_TABS);
				SetWindowTheme(hWnd, nullptr, nullptr);
				break;
			}

			if (scrollBarTheme)
			{
				ThemeHandles.emplace(scrollBarTheme, ThemeType::ScrollBar);

				// TODO: This is a hack...the handle should be valid as long as at least one window is still open
				CloseThemeData(scrollBarTheme);
			}

			if (HTHEME windowTheme = GetWindowTheme(hWnd); windowTheme)
				ThemeHandles.emplace(windowTheme, themeType);

			//if (!PermanentWindowSubclasses.count(className))
			//	RemoveWindowSubclass(hWnd, WindowSubclass, 0);
		}
		break;
		}

		return result;
	}

	LRESULT CALLBACK ListViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR , DWORD_PTR dwRefData)
	{
		switch (uMsg)
		{
#if(_WIN32_WINNT >= 0x0501)
		case WM_NOTIFY:
		{
			if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW)
			{
				LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
				switch (nmcd->dwDrawStage)
				{
				case CDDS_PREPAINT:
					return CDRF_NOTIFYITEMDRAW;
				case CDDS_ITEMPREPAINT:
				{
					auto info = reinterpret_cast<ListViewSubclassInfo*>(dwRefData);
					SetTextColor(nmcd->hdc, info->headerTextColor);
					return CDRF_DODEFAULT;
				}
				}
			}
		}
		case WM_THEMECHANGED:
		{

			HWND hHeader = ListView_GetHeader(hWnd);

			AllowDarkModeForWindow(hWnd, g_darkModeEnabled);
			AllowDarkModeForWindow(hHeader, g_darkModeEnabled);

			HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
			if (hTheme)
			{
				COLORREF color;
				if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
				{
					ListView_SetTextColor(hWnd, color);
				}else{
					ListView_SetTextColor(hWnd, RGB(255, 255, 255));
				}
				if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
				{
					ListView_SetTextBkColor(hWnd, color);
					ListView_SetBkColor(hWnd, color);
				}else{
						ListView_SetTextBkColor(hWnd, RGB(32, 32, 32));
						ListView_SetBkColor(hWnd, RGB(32, 32, 32));

				}
				CloseThemeData(hTheme);
			}

			hTheme = OpenThemeData(hHeader, L"Header");
			if (hTheme)
			{
				auto info = reinterpret_cast<ListViewSubclassInfo*>(dwRefData);
				GetThemeColor(hTheme, HP_HEADERITEM, 0, TMT_TEXTCOLOR,  &(info->headerTextColor));
				CloseThemeData(hTheme);
			}

			SendMessageW(hHeader, WM_THEMECHANGED, wParam, lParam);

			RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);

			//return 0;
		}

#endif


		case LVM_SETEXTENDEDLISTVIEWSTYLE:
		{
			// Prevent the OS grid separators from drawing
			wParam &= ~static_cast<WPARAM>(LVS_EX_GRIDLINES);
			lParam &= ~static_cast<LPARAM>(LVS_EX_GRIDLINES);
		}
		break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT CALLBACK MDIClientSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR , DWORD_PTR )
	{
		static HBRUSH backgroundColorBrush = CreateSolidBrush(RGB(32, 32, 32));

		switch (uMsg)
		{
		case WM_PAINT:
		{
			if (HDC hdc = GetDC(hWnd); hdc)
			{
				RECT windowArea;
				GetClientRect(hWnd, &windowArea);

				FillRect(hdc, &windowArea, backgroundColorBrush);
				ReleaseDC(hWnd, hdc);
			}
		}
		break;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}


	DWORD WINAPI Comctl32GetSysColor(int nIndex)
	{
		if( !(g_darkModeSupported && g_darkModeEnabled))
			return GetSysColor(nIndex);

		switch (nIndex)
		{
		case COLOR_WINDOW:
		case COLOR_BTNFACE: 
			return RGB(56, 56, 56);

		case COLOR_BTNTEXT: 
		case COLOR_WINDOWTEXT: 
			return RGB(255, 255, 255);

		}

		return GetSysColor(nIndex);
	}

	HBRUSH WINAPI Comctl32GetSysColorBrush(int nIndex)
	{
		if( !(g_darkModeSupported && g_darkModeEnabled))
			return GetSysColorBrush(nIndex);

		static HBRUSH btnFaceBrush = CreateSolidBrush(Comctl32GetSysColor(COLOR_BTNFACE));
		static HBRUSH btnTextBrush = CreateSolidBrush(Comctl32GetSysColor(COLOR_BTNTEXT));

		switch (nIndex)
		{
		case COLOR_BTNFACE: return btnFaceBrush;
		case COLOR_BTNTEXT: return btnTextBrush;
		}

		return GetSysColorBrush(nIndex);
	}

	HRESULT WINAPI Comctl32DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect)
	{
		if( !(g_darkModeSupported && g_darkModeEnabled))
			return DrawThemeText(hTheme,hdc,iPartId,iStateId,pszText,cchText,dwTextFlags,dwTextFlags2,pRect);

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(255, 255, 255));

		RECT temp = *pRect;
		DrawTextW(hdc, pszText, cchText, &temp, dwTextFlags);

		return S_OK;
	}

	HRESULT WINAPI Comctl32DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect)
	{

		if( !(g_darkModeSupported && g_darkModeEnabled))
			return DrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);

		auto themeType = ThemeType::None;

		if (auto itr = ThemeHandles.find(hTheme); itr != ThemeHandles.end())
			themeType = itr->second;

		if (themeType == ThemeType::ScrollBar)
		{
			static HBRUSH scrollbarFill = CreateSolidBrush(RGB(77, 77, 77));
			static HBRUSH scrollbarFillHighlighted = CreateSolidBrush(RGB(122, 122, 122));
			static HBRUSH scrollbarBackground = CreateSolidBrush(RGB(23, 23, 23));

			switch (iPartId)
			{
			case SBP_THUMBBTNHORZ:	// Horizontal drag bar
			case SBP_THUMBBTNVERT:	// Vertical drag bar
			{
				if (iStateId == SCRBS_HOT || iStateId == SCRBS_PRESSED)
					FillRect(hdc, pRect, scrollbarFillHighlighted);
				else
					FillRect(hdc, pRect, scrollbarFill);
			}
			return S_OK;

			case SBP_LOWERTRACKHORZ:// Horizontal background
			case SBP_UPPERTRACKHORZ:// Horizontal background
			case SBP_LOWERTRACKVERT:// Vertical background
			case SBP_UPPERTRACKVERT:// Vertical background
			{
				FillRect(hdc, pRect, scrollbarBackground);
			}
			return S_OK;

			case SBP_ARROWBTN:		// Arrow button
			case SBP_GRIPPERHORZ:	// Horizontal resize scrollbar
			case SBP_GRIPPERVERT:	// Vertical resize scrollbar
			case SBP_SIZEBOX:		// Resize box, bottom right
			case SBP_SIZEBOXBKGND:	// Resize box, background, unused
				break;
			}
		}
		else if (themeType == ThemeType::StatusBar)
		{
			static HBRUSH statusBarBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH statusBarFill = CreateSolidBrush(RGB(56, 56, 56));

			switch (iPartId)
			{
			case 0:
			{
				// Outside border (top, right)
				FillRect(hdc, pRect, statusBarBorder);
			}
			return S_OK;

			case SP_PANE:
			case SP_GRIPPERPANE:
			case SP_GRIPPER:
			{
				// Everything else
				FillRect(hdc, pRect, statusBarFill);
			}
			return S_OK;
			}
		}
		else if (themeType == ThemeType::Edit)
		{
			static HBRUSH editControlBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH editControlFill = CreateSolidBrush(RGB(32, 32, 32));

			switch (iPartId)
			{
			case EP_EDITBORDER_NOSCROLL:
			{
				FillRect(hdc, pRect, editControlFill);
				FrameRect(hdc, pRect, editControlBorder);
			}
			return S_OK;
			}
		}
		else if (themeType == ThemeType::Button)
		{
			static HBRUSH buttonBorder = CreateSolidBrush(RGB(130, 135, 144));
			static HBRUSH buttonBorderHighlighted = CreateSolidBrush(RGB(0, 120, 215));
			static HBRUSH buttonFill = CreateSolidBrush(RGB(32, 32, 32));
			static HBRUSH buttonPressed = CreateSolidBrush(RGB(83, 83, 83));

			switch (iPartId)
			{
			case BP_PUSHBUTTON:
			{
				HBRUSH frameColor = buttonBorder;
				HBRUSH fillColor = buttonFill;

				switch (iStateId)
				{
				case PBS_HOT:
				case PBS_DEFAULTED:
					frameColor = buttonBorderHighlighted;
					break;

				case PBS_DISABLED:
					fillColor = buttonBorder;
					break;

				case PBS_PRESSED:
					fillColor = buttonPressed;
					break;
				}

				FillRect(hdc, pRect, fillColor);
				FrameRect(hdc, pRect, frameColor);
			}
			return S_OK;

			case BP_CHECKBOX:
			{
				if (iStateId == CBS_UNCHECKEDDISABLED ||
					iStateId == CBS_CHECKEDDISABLED ||
					iStateId == CBS_MIXEDDISABLED ||
					iStateId == CBS_IMPLICITDISABLED ||
					iStateId == CBS_EXCLUDEDDISABLED)
				{
					FrameRect(hdc, pRect, buttonBorder);
					return S_OK;
				}
			}
			break;
			}
		}
		else if (themeType == ThemeType::ComboBox)
		{
			static HBRUSH comboBoxBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH comboBoxFill = CreateSolidBrush(RGB(32, 32, 32));


			switch (iPartId)
			{
			case CP_READONLY:			// Main control
			{
				FillRect(hdc, pRect, iStateId == CBRO_DISABLED ? comboBoxBorder : comboBoxFill);
				FrameRect(hdc, pRect, comboBoxBorder);
			}
			return S_OK;

			case CP_BORDER:				// Main control with text edit
			{
				// Special case: dropdown arrow needs to be drawn
				DrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
				FillRect(hdc, pRect, comboBoxFill);

				if (iStateId == CBB_DISABLED)
					FillRect(hdc, pRect, comboBoxBorder);
				else
					FrameRect(hdc, pRect, comboBoxBorder);
			}
			return S_OK;

			case CP_DROPDOWNBUTTONRIGHT:// Dropdown arrow
			case CP_DROPDOWNBUTTONLEFT:	// Dropdown arrow
				break;

			case CP_DROPDOWNBUTTON:
			case CP_BACKGROUND:
			case CP_TRANSPARENTBACKGROUND:
			case CP_CUEBANNER:
			case CP_DROPDOWNITEM:
				return S_OK;
			}
		}
		else if (themeType == ThemeType::Header)
		{
			static HBRUSH headerFill = CreateSolidBrush(RGB(77, 77, 77));

			switch (iPartId)
			{
			case 0:
			case HP_HEADERITEM:
			case HP_HEADERITEMLEFT:
			case HP_HEADERITEMRIGHT:
			{
				HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));

				for (int i = 0; i < 2; i++)
				{
					if (i == 0)
						SetDCPenColor(hdc, RGB(65, 65, 65));
					else
						SetDCPenColor(hdc, RGB(29, 38, 48));

					std::array<DWORD, 2> counts = { 2, 2 };
					std::array<POINT, 4> verts
					{{
						// Right border
						{ pRect->right - 2 + i, pRect->top },
						{ pRect->right - 2 + i, pRect->bottom },

						// Bottom border
						{ pRect->left - 1, pRect->bottom - 2 + i },
						{ pRect->right - 2, pRect->bottom - 2 + i },
					}};

					PolyPolyline(hdc, verts.data(), counts.data(), (DWORD)counts.size());
				}

				// Fill background on hover (1px padding for border shadow)
				if ((iPartId == 0 && iStateId == HIS_HOT) ||
					(iPartId == HP_HEADERITEM && iStateId == HIS_HOT) ||
					(iPartId == HP_HEADERITEMLEFT && iStateId == HILS_HOT) ||
					(iPartId == HP_HEADERITEMRIGHT && iStateId == HIRS_HOT))
				{
					RECT padded = { pRect->left - 1, pRect->top, pRect->right - 1, pRect->bottom - 1 };

					FillRect(hdc, &padded, headerFill);
				}

				SelectObject(hdc, oldPen);
			}
			return S_OK;
			}
		}
		else if (themeType == ThemeType::TabControl)
		{
			static HBRUSH tabControlButtonBorder = CreateSolidBrush(RGB(130, 135, 144));// RGB(83, 83, 83)
			static HBRUSH tabControlButtonFill = CreateSolidBrush(RGB(56, 56, 56));

			switch (iPartId)
			{
			case TABP_TABITEM:				// TCS_MULTILINE middle buttons
			case TABP_TABITEMLEFTEDGE:		// TCS_MULTILINE leftmost button
			case TABP_TABITEMRIGHTEDGE:		// TCS_MULTILINE rightmost button
			case TABP_TABITEMBOTHEDGE:		// TCS_MULTILINE ???
			case TABP_TOPTABITEM:			// Middle buttons
			case TABP_TOPTABITEMLEFTEDGE:	// Leftmost button
			case TABP_TOPTABITEMRIGHTEDGE:	// Rightmost button
			case TABP_TOPTABITEMBOTHEDGE:	// ???
			{
				RECT paddedRect = *pRect;
				RECT insideRect = { pRect->left + 1, pRect->top + 1, pRect->right - 1, pRect->bottom - 1 };

				bool isHover = (iPartId == TABP_TABITEM && iStateId == TIS_HOT) ||
					(iPartId == TABP_TABITEMLEFTEDGE && iStateId == TILES_HOT) ||
					(iPartId == TABP_TABITEMRIGHTEDGE && iStateId == TIRES_HOT) ||
					(iPartId == TABP_TABITEMBOTHEDGE && iStateId == TIBES_HOT) ||
					(iPartId == TABP_TOPTABITEM && iStateId == TTIS_HOT) ||
					(iPartId == TABP_TOPTABITEMLEFTEDGE && iStateId == TTILES_HOT) ||
					(iPartId == TABP_TOPTABITEMRIGHTEDGE && iStateId == TTIRES_HOT) ||
					(iPartId == TABP_TOPTABITEMBOTHEDGE && iStateId == TTIBES_HOT);

				if ((iPartId == TABP_TABITEM && iStateId == TIS_SELECTED) ||
					(iPartId == TABP_TABITEMLEFTEDGE && iStateId == TILES_SELECTED) ||
					(iPartId == TABP_TABITEMRIGHTEDGE && iStateId == TIRES_SELECTED) ||
					(iPartId == TABP_TABITEMBOTHEDGE && iStateId == TIBES_SELECTED) ||
					(iPartId == TABP_TOPTABITEM && iStateId == TTIS_SELECTED) ||
					(iPartId == TABP_TOPTABITEMLEFTEDGE && iStateId == TTILES_SELECTED) ||
					(iPartId == TABP_TOPTABITEMRIGHTEDGE && iStateId == TTIRES_SELECTED) ||
					(iPartId == TABP_TOPTABITEMBOTHEDGE && iStateId == TTIBES_SELECTED))
				{
					paddedRect.top += 1;
					paddedRect.bottom -= 2;

					// Allow the rect to overlap so the bottom border outline is removed
					insideRect.top += 1;
					insideRect.bottom += 1;
				}

				FrameRect(hdc, &paddedRect, tabControlButtonBorder);
				FillRect(hdc, &insideRect, isHover ? tabControlButtonBorder : tabControlButtonFill);
			}
			return S_OK;

			case TABP_PANE:
				return S_OK;
			}
		}

		return DrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
	}
}