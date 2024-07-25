#pragma once
#include <vector>
#include <array>
#include <windows.h>
#include "Ship.cpp"

using namespace std;

enum class CellStatus { Empty, Occupied, Hit, Miss, Neutral };

struct field
{
	RECT position;
	bool isOccupied = false;
	int shipLength = 0;
	CellStatus status = CellStatus::Empty;
	int shipIndex = -1; //no ship in the cell
};

class board
{
private:
	vector<field> m_fields;
	vector<Ship> ships;
	static const LONG field_size = 30;
	static const LONG  margin = 5;
	static const LONG margin_bw = 3;
	int _rows=10;
	int _columns=10;
	bool showShips;
	
	


public:
	board() : ships{ {4},{3}, {3},{2}, {2}, {2},{1}, {1}, {1}, {1} }, showShips(false) {}

	void setShowShips(bool show) { showShips = show; }
	bool shouldShowShips() const { return showShips; }
	bool isUserShip = true;
	
	void placeShips();
	void initialize(LONG rows, LONG columns);
	vector<field> const& fields() const { return m_fields; }
	static LONG calculateWidth(LONG columns);
	static LONG calculateHeight(LONG rows);
	static LONG getMarginBw() {return margin_bw;}
	static LONG getFieldSz() { return field_size; }
	bool isValidCell(int rowIndex, int columnIndex);
	void markSurroundingAsNeutral(int shipIndex);
	void markCell(int rowIndex, int columnIndex);
	int getRows() const { return _rows; }
	int getColumns() const { return _columns; }
	bool allShipsDestroyed() const;
};

