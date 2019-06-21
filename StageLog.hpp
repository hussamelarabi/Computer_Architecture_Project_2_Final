#include "Includes.h"

class StageLog
{
public:
    int count = 0, Fetched, Issued, Executed, Written, Committed;
    Instruction inst;
    bool flushed;
};