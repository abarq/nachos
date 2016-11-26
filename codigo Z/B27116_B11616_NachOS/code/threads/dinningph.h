#include "synch.h"

class DinningPh {

public:
    DinningPh();
    ~DinningPh();
    void pickup( long who );
    void putdown( long who );
    void test( long who );
    void print();

private:
    enum { Thinking, Hungry, Eating } state[ 5 ];
    Lock * dp;			// Locks for each method in class
    Condition * self[ 5 ];

};
