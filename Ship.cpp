#ifndef SHIP_H
#define SHIP_H
#include <ctime>

class Ship {
public:
    int size; 
    int hits=0;
   
   
    
    void recordHit() {
        hits++;
    }

    bool isDestroyed() const {
        return hits >= size;
    }

    Ship(int size) : size(size), hits(0) {}
};

#endif // SHIP_H