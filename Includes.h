#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <map>
#include <iomanip>
using namespace std;

#include "Instruction.hpp"
#include "prediction.hpp"
#include "reservationStation.hpp"
#include "ReorderBuffer.hpp"
#include "StageLog.hpp"

#define memSize 65536
#define robSize 6
#define rfSize 8

#define RS_loadAndStore_size 2
#define RS_Mult_size 2
#define RS_Branch_size 2
#define RS_Jump_size 3
#define RS_Add_size 3
#define RS_NAND_size 1