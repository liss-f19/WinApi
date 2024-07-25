#include <windows.h>
#include "resource.h"
#include <string>
#include "battleship.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <Shlobj.h>
#include <random>
#include <wingdi.h>
#include "board.h"

using namespace std;

wstring const battleship::s_class_name{ L"BATTLESHIPS - STATISTICS" };
wstring const battleship::LeftWindowClassName =  L"BATTLESHIPS - MY";
wstring const battleship::RightWindowClassName = L"BATTLESHIPS - PC";




const UINT_PTR TIMER_ID = 0;
const int TIMER_INTERVAL_MS = 1; // Updates every second
chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();

	wstring FormatElapsedTime(chrono::high_resolution_clock::time_point start_time) {
	auto now = chrono::high_resolution_clock::now();
	double elapsed_seconds = chrono::duration<double>(now - start_time).count();
	wstringstream ss;
	ss << fixed << setprecision(6) << L"BATTLESHIP:" << elapsed_seconds;
	return ss.str();
}


//window procedures
LRESULT battleship::window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	battleship* app = nullptr;
	if (message == WM_NCCREATE)
	{
		app = static_cast<battleship*>(reinterpret_cast<LPCREATESTRUCTW>(lparam)->lpCreateParams);
		SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));

	}
	else
		app = reinterpret_cast<battleship*>(GetWindowLongPtrW(window, GWLP_USERDATA));
	LRESULT res = app ? app->window_proc(window, message, wparam, lparam) :
		DefWindowProcW(window, message, wparam, lparam);

	if (message == WM_NCDESTROY)
		SetWindowLongPtrW(window, GWLP_USERDATA, 0);
	return res;
}

LRESULT battleship::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	static unsigned int secondsElapsed = 0;
	switch (message)
	{
	case WM_CREATE:
		SetTimer(window, TIMER_ID, TIMER_INTERVAL_MS, nullptr);
		break;

	case WM_TIMER:
		SetWindowText(window, FormatElapsedTime(start_time).c_str());
		break;
	case WM_COMMAND: {
		switch (LOWORD(wparam)) {
		case ID_DIFFICULTY_EASY:
			writeDifficultyToIni(L"Easy");
			UpdateChildWindows(10, 10);
			break;
		case ID_DIFFICULTY_MEDIUM:
			writeDifficultyToIni(L"Medium");
			UpdateChildWindows(15, 15);
			break;
		case ID_DIFFICULTY_HARD:
			writeDifficultyToIni(L"Hard");
			UpdateChildWindows(20, 20);
			break;
		default:
			return DefWindowProc(window, message, wparam, lparam);
		}
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);
		RECT clientRect;
		GetClientRect(window, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		DrawPlayerStatistics(hdc, m_left_board, 0, 0); 

		DrawPlayerStatistics(hdc, m_right_board, clientWidth*0.5, 0); 
		EndPaint(window, &ps);
		break;
	}
	case WM_CLOSE:
		KillTimer(m_main, TIMER_ID);
		DestroyWindow(m_main);
		return 0;
	case WM_DESTROY:
		if (window == m_main)
		{
			PostQuitMessage(EXIT_SUCCESS);
		}
		return 0;
	}
	return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT CALLBACK LeftWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
	case WM_CTLCOLORSTATIC:
	{
				battleship* pThis = reinterpret_cast<battleship*>(GetWindowLongPtr(window, GWLP_USERDATA));
				if (pThis) {
					return (INT_PTR)pThis->getFieldBrush();
				}
			}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);
		battleship* pThis = reinterpret_cast<battleship*>(GetWindowLongPtr(window, GWLP_USERDATA));
		if (!pThis) return DefWindowProc(window, message, wparam, lparam);

		COLORREF colorDefault = RGB(115, 147, 179); // Blue default
		COLORREF colorHit = RGB(255, 0, 0); // Red for hit
		COLORREF colorMiss = RGB(0, 0, 255); // Blue for miss
		COLORREF colorNeut = RGB(255, 255, 0); // Yellow for surroundings

		for (const auto& cell : pThis->get_left_Board().fields()) {
			RECT cellRect = { cell.position.left, cell.position.top, cell.position.right, cell.position.bottom };

			COLORREF fillColor;
			if (cell.status == CellStatus::Hit) {
				fillColor = colorHit;
			}
			else if (cell.status == CellStatus::Miss) {
				fillColor = colorMiss;
			}
			else if (cell.status == CellStatus::Neutral) {
				fillColor = colorNeut;
			}
			else {
				fillColor = colorDefault;
			}

			HBRUSH cellBrush = CreateSolidBrush(fillColor);
			SelectObject(hdc, cellBrush);

			RoundRect(hdc, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom, 12, 12);

			wchar_t text[2] = L"";
			if (cell.status == CellStatus::Hit) {
				wcscpy_s(text, L"X");
			}
			else if (cell.status == CellStatus::Miss) {
				wcscpy_s(text, L".");
			}
			else if (cell.isOccupied && pThis->get_left_Board().shouldShowShips()) {
				_itow_s(cell.shipLength, text, 10);
			}

			SetTextColor(hdc, RGB(0, 0, 0)); // Black 
			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, text, -1, &cellRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			DeleteObject(cellBrush); // Cleanup
		}
		if (pThis->get_left_Board().allShipsDestroyed()) {
			HDC hdcMem = CreateCompatibleDC(hdc);
			HBITMAP hbmMem = CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
			HANDLE hOld = SelectObject(hdcMem, hbmMem);

			HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0)); // Red color
			FillRect(hdcMem, &ps.rcPaint, brush);
			DeleteObject(brush);

			BLENDFUNCTION blend = {};
			blend.BlendOp = AC_SRC_OVER;
			blend.BlendFlags = 0;
			blend.SourceConstantAlpha = 128; 
			blend.AlphaFormat = 0;

			AlphaBlend(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, hdcMem, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, blend);

			// Cleanup
			SelectObject(hdcMem, hOld);
			DeleteDC(hdcMem);
			DeleteObject(hbmMem);
		}

		EndPaint(window, &ps);
	}
	break;

	
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT CALLBACK RightWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
	case WM_CTLCOLORDLG:
		return (INT_PTR)CreateSolidBrush(RGB(0, 255, 0));

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);
		battleship* pThis = reinterpret_cast<battleship*>(GetWindowLongPtr(window, GWLP_USERDATA));
		if (!pThis) return DefWindowProc(window, message, wparam, lparam);


		COLORREF colorDefault = RGB(115, 147, 179); 
		COLORREF colorHit = RGB(255, 0, 0); // Red for hit
		COLORREF colorMiss = RGB(0, 0, 255); // Blue for miss
		COLORREF colorNeut = RGB(255, 255, 0); // Yellow for surroundings

		for (const auto& cell : pThis->get_right_Board().fields()) {
			RECT cellRect = { cell.position.left, cell.position.top, cell.position.right, cell.position.bottom };

			COLORREF fillColor;
			if (cell.status == CellStatus::Hit) {
				fillColor = colorHit;
			}
			else if (cell.status == CellStatus::Miss) {
				fillColor = colorMiss;
			}
			else if (cell.status == CellStatus::Neutral) {
				fillColor = colorNeut;
			}
			else {
				fillColor = colorDefault;
			}

			HBRUSH cellBrush = CreateSolidBrush(fillColor);
			SelectObject(hdc, cellBrush);
			
			RoundRect(hdc, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom, 12, 12);

			wchar_t text[2] = L"";
			if (cell.status == CellStatus::Hit) {
				wcscpy_s(text, L"X");
			}
			else if (cell.status == CellStatus::Miss) {
				wcscpy_s(text, L".");
			}
			else if (cell.isOccupied && pThis->get_right_Board().shouldShowShips()) {
				_itow_s(cell.shipLength, text, 10);
			}

			SetTextColor(hdc, RGB(0, 0, 0)); 
			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, text, -1, &cellRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			DeleteObject(cellBrush); // Cleanup
		}

		if (pThis->get_right_Board().allShipsDestroyed()) {
			HDC hdcMem = CreateCompatibleDC(hdc);
			HBITMAP hbmMem = CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
			SelectObject(hdcMem, hbmMem);

			BLENDFUNCTION blendFunction;
			blendFunction.BlendOp = AC_SRC_OVER;
			blendFunction.BlendFlags = 0;
			blendFunction.SourceConstantAlpha = 128; // 50% transparency
			blendFunction.AlphaFormat = 0;

			// overlay
			HBRUSH hBrushOverlay = CreateSolidBrush(RGB(0, 255, 0)); // Semi-transparent green
			FillRect(hdcMem, &ps.rcPaint, hBrushOverlay);
			DeleteObject(hBrushOverlay);
			LOGFONT lf = { 0 };
			lf.lfHeight = 24; 
			lf.lfWeight = FW_BOLD;
			wcscpy_s(lf.lfFaceName, L"Arial"); 
			HFONT hFont = CreateFontIndirect(&lf);
			HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
			SetTextColor(hdcMem, RGB(0, 0, 0));
			SetBkMode(hdcMem, TRANSPARENT);
			DrawText(hdcMem, L"Congratulations, you won!", -1, &ps.rcPaint, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			AlphaBlend(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, hdcMem, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, blendFunction);
			SelectObject(hdcMem, hOldFont);
			DeleteObject(hFont);
			// Cleanup
			DeleteDC(hdcMem);
			DeleteObject(hbmMem);
		}


		EndPaint(window, &ps);
	}
	break;



	case WM_LBUTTONDOWN:
	{
		int xPos = LOWORD(lparam); // Mouse click X coordinate
		int yPos = HIWORD(lparam); // Mouse click Y coordinate

		battleship* pThis = reinterpret_cast<battleship*>(GetWindowLongPtr(window, GWLP_USERDATA));
		if (pThis) {

			pThis->processClickOnRightWindow(xPos, yPos);
		}
	}
	return 0;


	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(window, message, wparam, lparam);
}


bool battleship::register_main_class()
{
	WNDCLASSEXW desc{};
	if (GetClassInfoExW(m_instance, s_class_name.c_str(),
		&desc) != 0)
		return true;
	HBRUSH backgroundColorBrush = CreateSolidBrush(RGB(164, 174, 196));
	
	desc.cbSize = sizeof(WNDCLASSEXW),
		desc.lpfnWndProc = window_proc_static,
		desc.hInstance = m_instance,
		desc.hCursor = LoadCursorW(nullptr, L"IDC_ARROW"),
		desc.lpszClassName = s_class_name.c_str(),
		desc.hbrBackground = backgroundColorBrush;
	desc.hIcon = LoadIcon(m_instance, MAKEINTRESOURCE(IDI_ICON1)); 
	desc.hIconSm = LoadIcon(desc.hInstance, MAKEINTRESOURCE(IDI_ICON1));

	
	return RegisterClassExW(&desc) != 0;
}

bool battleship::RegisterLeftPopupWindowClass() const {
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = LeftWindowProc;
	wc.hInstance = m_instance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(250, 247, 238));
	wc.lpszClassName = LeftWindowClassName.c_str();



	return RegisterClassExW(&wc) != 0;
}

bool battleship::RegisterRightPopupWindowClass() const {
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = RightWindowProc;
	wc.hInstance = m_instance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.lpszClassName = RightWindowClassName.c_str();


	return RegisterClassExW(&wc) != 0;
}

//creation
HWND battleship::create_main_window()
{
	
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int x = (screenWidth - 600) / 2; 
	int y = (screenHeight - 250) / 2 + (screenHeight / 4) / 2;
	
	HWND hwnd = CreateWindowExW(
		WS_EX_LAYERED, 
		s_class_name.c_str(),
		L"BATTLESHIPS - STATISTICS",
		WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_BORDER | WS_MINIMIZEBOX,
		x,
		y,
		600,
		250,
		nullptr,
		nullptr,
		m_instance,
		this);

	
	SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(255 * 0.7), LWA_ALPHA);
	HMENU hMenu = LoadMenu(m_instance, MAKEINTRESOURCE(IDR_MENU1));
	SetMenu(hwnd, hMenu);

	

	return hwnd;
}

HWND battleship::create_left_popup_window()
{
	int rows, columns;
	wstring difficulty = readDifficultyFromIni();
	if (difficulty == L"Easy")
	{
		rows = columns = 10;
	}
	else if (difficulty == L"Medium")
	{
		rows = columns = 15;
	}
	else rows = columns = 20;
	DWORD popupStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_SYSMENU);
	m_left_board.initialize(rows, columns);

	m_left_board.setShowShips(true);//DRAW OR NOT!!!!!!!!
	m_left_board.placeShips();

	RECT mainWindowRect;
	GetWindowRect(m_main, &mainWindowRect);

	RECT size{ 0, 0, board::calculateWidth(rows), board::calculateHeight(columns)};
	AdjustWindowRectEx(&size, popupStyle, false, 0);

	int x = mainWindowRect.left - (size.right-size.left) - 10; 
	int y = mainWindowRect.top; 

	HWND hwnd = CreateWindowExW(
		WS_EX_TOOLWINDOW,
		LeftWindowClassName.c_str(),
		L"BATTLESHIPS - MY",
		popupStyle,
		x,
		y,
		size.right - size.left,  
		size.bottom - size.top,
		m_main,
		nullptr,
		m_instance,
		this);

	if (!hwnd) return nullptr;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	
	ShowWindow(hwnd, SW_SHOWNA);
	UpdateWindow(hwnd);

	return hwnd;
}

HWND battleship::create_right_popup_window()
{
	int rows, columns;
	wstring difficulty = readDifficultyFromIni();
	if (difficulty == L"Easy")
	{
		rows = columns = 10;
	}
	else if (difficulty == L"Medium")
	{
		rows = columns = 15;
	}
	else rows = columns = 20;

	DWORD popupStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_SYSMENU);

	m_right_board.initialize(rows, columns);

	m_right_board.setShowShips(false);
	m_right_board.placeShips();

	RECT mainWindowRect;
	GetWindowRect(m_main, &mainWindowRect);

	RECT size{ 0, 0, board::calculateWidth(rows), board::calculateHeight(columns) };
	AdjustWindowRectEx(&size, popupStyle, false, 0);

	int x = mainWindowRect.right + 10;
	int y = mainWindowRect.top;

	HWND hwnd = CreateWindowExW(
		WS_EX_TOOLWINDOW,
		RightWindowClassName.c_str(),
		L"BATTLESHIPS - PC",
		popupStyle,
		x,
		y,
		size.right - size.left,
		size.bottom - size.top,
		m_main,
		nullptr,
		m_instance,
		this);

	if (!hwnd) return nullptr;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	ShowWindow(hwnd, SW_SHOWNA);
	UpdateWindow(hwnd);
	return hwnd;
}

void battleship::UpdateChildWindows(int rows, int columns) {

		DWORD popupStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_SYSMENU);
		m_left_board.initialize(rows, columns);
		m_right_board.initialize(rows, columns);

		m_left_board.setShowShips(true);
		m_right_board.setShowShips(false); 
		m_left_board.placeShips();
		m_right_board.placeShips();

		RECT mainWindowRect;
		GetWindowRect(m_main, &mainWindowRect);
		RECT size{ 0, 0, board::calculateWidth(rows), board::calculateHeight(columns) };
		AdjustWindowRectEx(&size, popupStyle, false, 0);


		int x = mainWindowRect.right + 10; 
		int y = mainWindowRect.top;


		SetWindowPos(m_popup_left, NULL, x, y, size.right-size.left, size.bottom-size.top, SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
		SetWindowPos(m_popup_right, NULL, x, y, size.right-size.left, size.bottom-size.top, SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);

		InvalidateRect(m_popup_left, NULL, TRUE);
		InvalidateRect(m_popup_right, NULL, TRUE);
		InvalidateRect(m_main, NULL, TRUE);
	
		

}



//constructor for all
battleship::battleship(HINSTANCE instance) : m_instance{ instance }, m_main{}, m_popup_left{}, m_popup_right{}, m_field_brush{ CreateSolidBrush(RGB(169, 169, 169)) }
{
	
	register_main_class();
	RegisterLeftPopupWindowClass();
	RegisterRightPopupWindowClass();

	
	m_main = create_main_window();
	m_popup_left = create_left_popup_window();
	m_popup_right = create_right_popup_window();

	
	std::wstring difficulty = readDifficultyFromIni();
	if (difficulty == L"Easy") {
		m_left_board.initialize(10, 10);
		m_right_board.initialize(10, 10);
	}
	else if (difficulty == L"Medium") {
		m_left_board.initialize(15, 15);
		m_right_board.initialize(15, 15);
	}
	else { 
		m_left_board.initialize(20, 20);
		m_right_board.initialize(20, 20);
	}
	m_left_board.setShowShips(true);
	m_right_board.setShowShips(false);
	m_left_board.placeShips();
	m_right_board.placeShips();
	
	ShowWindow(m_main, SW_SHOW);
	UpdateWindow(m_main);
	ShowWindow(m_popup_left, SW_SHOWNA);
	UpdateWindow(m_popup_left);
	ShowWindow(m_popup_right, SW_SHOWNA);
	UpdateWindow(m_popup_right);

}

int battleship::run(int show_command)
{


	MSG msg{};
	BOOL result = TRUE;
	while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0)
	{
		if (result == -1)
			return EXIT_FAILURE;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return EXIT_SUCCESS;
}

void battleship::writeDifficultyToIni(const std::wstring& difficultyLevel) {
	std::wstring iniFilePath = GetIniFilePath(); // Determine the path to the INI file
	WritePrivateProfileString(L"Settings", L"Difficulty", difficultyLevel.c_str(), iniFilePath.c_str());
}


std::wstring battleship::readDifficultyFromIni() {
	wchar_t difficultyLevel[32];
	std::wstring iniFilePath = GetIniFilePath();
	GetPrivateProfileString(L"Settings", L"Difficulty", L"Medium", difficultyLevel, 32, iniFilePath.c_str());
	return std::wstring(difficultyLevel);
}

std::wstring battleship::GetIniFilePath() {
	wchar_t path[MAX_PATH] = { 0 };
	HRESULT result = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);


	if (SUCCEEDED(result)) {
		std::wstring fullPath = std::wstring(path) + L"\\Battleships\\settings.ini";

		DWORD attributes = GetFileAttributesW(std::wstring(path + std::wstring(L"\\Battleships")).c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES) {
			SHCreateDirectoryExW(NULL, std::wstring(path + std::wstring(L"\\Battleships")).c_str(), NULL);
		}

		return fullPath;
	}
	else {
		throw new exception("ERROR WITH INI");
		return L"";
	}
}

void battleship::processClickOnRightWindow(int xPos, int yPos)
{
	
	int columnIndex = xPos / (board::getFieldSz() + board::getMarginBw());
	int rowIndex = yPos / (board::getFieldSz() + board::getMarginBw());

	if (m_right_board.isValidCell(rowIndex, columnIndex)) {

		m_right_board.markCell(rowIndex, columnIndex);
		
	}

		InvalidateRect(m_popup_right, nullptr, TRUE);
		computerTurn();
	
}

void battleship::computerTurn() {
	
	static mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> distRow(0, m_left_board.getRows() - 1);
	std::uniform_int_distribution<int> distCol(0, m_left_board.getColumns() - 1);
	int randomRow = distRow(rng);
	int randomColumn = distCol(rng);
	int xPos = randomColumn * (board::getFieldSz() + board::getMarginBw());
	int yPos = randomRow * (board::getFieldSz() + board::getMarginBw());

	if (m_left_board.isValidCell(randomRow, randomColumn)) {
		m_left_board.markCell(randomRow, randomColumn);
	}
	
	InvalidateRect(m_popup_left, nullptr, TRUE);

	
}


void battleship::DrawPlayerStatistics(HDC hdc, const board& playerBoard, int x, int y)
{
	vector<int> shipSizes = { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };

	RECT clientRect;
	GetClientRect(m_main, &clientRect);
	int cellHeight = (clientRect.bottom - clientRect.top) / 4;
	int cellWidth = cellHeight / 2;

	int shipIndex = 0;
	int spacing = 1; // Spacing between cells
	for (int col = 0; col < 10; ++col)
	{
		for (int row = 0; row < shipSizes[col]; ++row)
		{
			int cellX = x + col * (cellWidth + spacing);
			int cellY = y + row * (cellHeight + spacing);

			RECT cellRect = { cellX, cellY, cellX + cellWidth, cellY + cellHeight };
			HBRUSH blueBrush = CreateSolidBrush(RGB(0, 0, 205)); 
			HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, blueBrush);

			HPEN yellowPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
			HPEN oldPen = (HPEN)SelectObject(hdc, yellowPen);

			RoundRect(hdc, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom, 10, 10); 

			SelectObject(hdc, oldPen);
			DeleteObject(yellowPen); 
			SelectObject(hdc, oldBrush); 
			DeleteObject(blueBrush);
		}
	}
}











