#include "Ship.cpp"
#include "board.h"
#include <ctime>
#include <algorithm>
#include <random>
#include <chrono>  




void board::placeShips() {
  
    auto seed = chrono::system_clock::now().time_since_epoch().count();
  

    std::shuffle(ships.begin(), ships.end(), default_random_engine(seed));

    std::srand(static_cast<unsigned int>(std::time(nullptr))); 
    for (size_t i = 0; i < ships.size(); ++i) {
        auto& ship = ships[i];
        bool placed = false;
        while (!placed) {
            int startRow = std::rand() % _rows;
            int startCol = std::rand() % _columns;

            // 0 for horizontal, 1 for vertical
            bool direction = std::rand() % 2;
            bool fitsInBoard = direction ? (startRow + ship.size <= _rows) : (startCol + ship.size <= _columns);

            if (!fitsInBoard) continue; 

            bool hasSpace = true;
            for (int j = 0; j < ship.size; ++j) {
                int checkRow = startRow + (direction ? j : 0);
                int checkCol = startCol + (direction ? 0 : j);

                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int neighborRow = checkRow + dy;
                        int neighborCol = checkCol + dx;

                        //  bounds
                        if (neighborRow < 0 || neighborRow >= _rows || neighborCol < 0 || neighborCol >= _columns) continue;

                        if (m_fields[neighborRow * _columns + neighborCol].isOccupied) {
                            hasSpace = false;
                            break;
                        }
                    }
                    if (!hasSpace) break;
                }
                if (!hasSpace) break;
            }

            if (hasSpace) {
                for (int k = 0; k < ship.size; ++k) {
                    int shipRow = startRow + (direction ? k : 0);
                    int shipCol = startCol + (direction ? 0 : k);
                    int index = shipRow * _columns + shipCol;
                    m_fields[shipRow * _columns + shipCol].isOccupied = true;
                    m_fields[index].shipLength = ship.size;
                    m_fields[index].shipIndex = i;
                   
                  
                }
                placed = true;
            }
        }
    }
}

void board::initialize(LONG rows, LONG columns)
{
	m_fields.clear();
    m_fields.resize(rows * columns); 
    _rows = rows;
    _columns = columns;

    for (LONG row = 0; row < rows; ++row) {
        for (LONG column = 0; column < columns; ++column) {
            auto& f = m_fields[row * columns + column];
            f.position.top = row * (field_size + margin_bw) + margin;
            f.position.left = column * (field_size + margin_bw) + margin;
            f.position.bottom = f.position.top + field_size;
            f.position.right = f.position.left + field_size;
        }
    }
}

LONG board::calculateWidth(LONG columns)
{
    return columns * (field_size + margin_bw) + 2*margin;

}

LONG board::calculateHeight(LONG rows)
{
    return rows * (field_size + margin_bw) +2* margin;
}



void board::markCell(int row, int col)
{
    
    int index = row * _columns + col;
    if (index >= 0 && index < m_fields.size()) {
        int _shipIndex = m_fields[index].shipIndex;
        if (m_fields[index].isOccupied) {
            
            if (_shipIndex != -1 && m_fields[index].status == CellStatus::Empty) {
                m_fields[index].status = CellStatus::Hit;
                ships[_shipIndex].recordHit();
               
            
                if (ships[_shipIndex].isDestroyed()) {
                    markSurroundingAsNeutral(_shipIndex);
                }
            }
        }
        else if (!m_fields[index].isOccupied && m_fields[index].status == CellStatus::Empty) {
            m_fields[index].status = CellStatus::Miss;
        }
    }

   
}

void board::markSurroundingAsNeutral(int shipIndex) {
    for (const auto& cell : m_fields) {
        if (cell.shipIndex == shipIndex) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int neighborRow = (cell.position.top / field_size) + dy;
                    int neighborCol = (cell.position.left / field_size) + dx;
                    if (neighborRow >= 0 && neighborRow < _rows && neighborCol >= 0 && neighborCol < _columns) {
                        int neighborIndex = neighborRow * _columns + neighborCol;
                       
                        if (m_fields[neighborIndex].status == CellStatus::Empty) {
                            m_fields[neighborIndex].status = CellStatus::Neutral; 
                        }
                    }
                }
            }
        }
    }
}




bool board::isValidCell(int rowIndex, int columnIndex)
{
    return rowIndex >= 0 && rowIndex < _rows && columnIndex >= 0 && columnIndex < _columns;
}

 bool board::allShipsDestroyed() const {
    for (auto& ship : ships) {
        if (!ship.isDestroyed()) {
            return false; 
        }
    }
    return true; // All ships destroyed
}

