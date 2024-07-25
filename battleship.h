#pragma once
#include <windows.h>
#include "board.h"
#include <string>


using namespace std;
class battleship
{
private:
	bool register_main_class();
	bool RegisterLeftPopupWindowClass( ) const;
	bool RegisterRightPopupWindowClass( ) const;
	static wstring const s_class_name;
	static wstring const LeftWindowClassName;
	static wstring const RightWindowClassName;
	static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
	LRESULT window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
	HWND create_main_window();
	HWND create_left_popup_window();
	HWND create_right_popup_window();
	HINSTANCE m_instance;
	HWND m_main, m_popup_left, m_popup_right;
	board m_left_board;
	board m_right_board;
	HBRUSH m_field_brush;
	vector<HWND> cellWindowHandles;
	void UpdateChildWindows(int rows, int columns);
	void UpdateGridCells(HWND parentWindow);
	HWND create_window(DWORD style, HWND parent = nullptr);
	wstring readDifficultyFromIni();
	void writeDifficultyToIni(const std::wstring& difficultyLevel);
	wstring GetIniFilePath();
	void computerTurn();
	void DrawPlayerStatistics(HDC hdc, const board& playerBoard, int x, int y);



public:
	HBRUSH getFieldBrush() const { return m_field_brush; }
	const board& get_left_Board() const { return m_left_board; }
	const board& get_right_Board() const { return m_right_board; }
	battleship(HINSTANCE instance);
	void processClickOnRightWindow(int xPos, int yPos);
	int run(int show_command);

};
