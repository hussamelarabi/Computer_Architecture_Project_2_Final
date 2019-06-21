#include "Includes.h"

class ReorderBuffer
{
public:
    bool busy;
    int dest;
    int value;
    bool ready;
    Instruction inst;
    int pcAddr;
};