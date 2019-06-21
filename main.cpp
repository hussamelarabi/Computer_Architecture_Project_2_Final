//Tomasulo's Algorithm with Speculation
//Created by: Hussam M. Alarabi, Omar S. Aboelella, Hesham K. Elshazly, Tarek H. Radwan

//To run the program
//
//
//
//
//
#include "Includes.h"

queue<Instruction> instrBuffer;
vector<prediction> predictionBuffer;
vector<StageLog> Log;
ReorderBuffer ROB[robSize];
reservationStation loadStation[RS_loadAndStore_size];  //lw
reservationStation storeStation[RS_loadAndStore_size]; //sw
reservationStation jumpStation[RS_Jump_size]; //JMP, JALR, RET
reservationStation beqStation[RS_Branch_size];  //beq
reservationStation addStation[RS_Add_size];  //add, addi, sub
reservationStation nandStation[RS_NAND_size]; //nand
reservationStation multStation[RS_Mult_size]; //mult




Instruction Dedocde(string instruction, int addr);

bool branchPredictor(string , int , bool , int , int &);

void getInstructions(int &, int &, string[] );

void write(int);

void fetch(int &, int &, string[], int &, int, int&);

void issue(int clk, int registers[], int registerStatus[], int& head, int &tail);

void issue(reservationStation R[], int size, Instruction inst, int LogIndex, int &head, int&tail, int registerStatus[], int registers[], int clk);

void execute(int, string[]);

void commit(int, int&, int &, int &, string[], int[], int &, int[], int&, bool &, int &);

void flush(int &, int &, int []);

void printResults(int registers[], int committedInstCount, int countMissBranch, int totalBranchCount);


int main() {

    // CLK, PC and location of Final Instruction
    int clk = 0;
    int pc = 0;
    int instEnd;

//Circular ROB implementation
    int head = 0;
    int tail = 0;

//Variable
    int committedInstCount = 0;
    int totalBranchCount = 0;
    int countMissBranch = 0;
    bool done = false;
    int instIndex = 0;
    int registerStatus[rfSize];
    int registers[rfSize] = {0};
    string dataAndInstMemory[memSize];



    getInstructions(pc, instEnd, dataAndInstMemory);
    flush(head, tail, registerStatus);

    while (instrBuffer.size() > 0 || pc <= instEnd || !done) {
        clk++;
        fetch(pc,instEnd, dataAndInstMemory, instIndex, clk, countMissBranch);
        issue(clk, registers, registerStatus, head, tail);
        execute(clk, dataAndInstMemory);
        write(clk);
        commit(clk, head, pc, totalBranchCount, dataAndInstMemory, registers,committedInstCount, registerStatus, tail,done, countMissBranch);
    }


    printResults(registers,committedInstCount, countMissBranch, totalBranchCount);

    return 0;
}

Instruction Dedocde(string instruction, int addr) {
    string temp = "";
    Instruction Parsed;
    Parsed.Addr = addr;
    Parsed.inst = instruction;
    int i = 0;
    while (instruction[i] != ' ') {
        temp = temp + instruction[i];
        i++;
    }
    Parsed.op = temp;
    temp = "";
    if ((Parsed.op == "ADD") || (Parsed.op == "SUB") || (Parsed.op == "NAND") || (Parsed.op == "MUL")) {
        i = i + 2;
        while (instruction[i] != ',') {
            temp = temp + instruction[i];
            i++;
        }
        Parsed.RD = stoi(temp);
        i = i + 3;
        temp = "";
        while (instruction[i] != ',') {
            temp = temp + instruction[i];
            i++;
        }
        Parsed.RS1 = stoi(temp);
        i = i + 3;
        temp = "";
        for (i; i < instruction.length(); i++) {
            temp = temp + instruction[i];
        }
        Parsed.rs2 = stoi(temp);
    } else if ((Parsed.op == "ADDI") || ((Parsed.op == "LW"))) {
        i = i + 2;
        while (instruction[i] != ',') {
            temp = temp + instruction[i];
            i++;
        }
        Parsed.RD = stoi(temp);
        i = i + 3;
        temp = "";
        while (instruction[i] != ',') {
            temp = temp + instruction[i];
            i++;
        }
        Parsed.RS1 = stoi(temp);
        i = i + 2;
        temp = "";
        for (i; i < instruction.length(); i++) {
            temp = temp + instruction[i];
        }
        Parsed.Imm = stoi(temp);
        Parsed.rs2 = -1;
    } else if ((Parsed.op == "BEQ") || (Parsed.op == "SW")) {
        i = i + 2;
        while (instruction[i] != ',') {
            temp = temp + instruction[i];
            i++;
        }
        Parsed.RS1 = stoi(temp);
        i = i + 3;
        temp = "";
        while (instruction[i] != ',') {
            temp = temp + instruction[i];
            i++;
        }
        Parsed.rs2 = stoi(temp);
        i = i + 2;
        temp = "";
        for (i; i < instruction.length(); i++) {
            temp = temp + instruction[i];
        }


        Parsed.Imm = stoi(temp);


        Parsed.RD = -1;
    } else if ((Parsed.op == "JMP")) {
        i = i + 1;
        for (i; i < instruction.length(); i++) {
            temp = temp + instruction[i];
        }


        Parsed.Imm = stoi(temp);


        Parsed.rs2 = -1;
        Parsed.RS1 = -1;
        Parsed.RD = -1;
    } else if ((Parsed.op == "JALR")) {
        i = i + 2;
        while (instruction[i] != ',') {
            temp = temp + instruction[i];
            i++;
        }
        Parsed.RD = stoi(temp);
        i = i + 3;
        temp = "";
        for (i; i < instruction.length(); i++) {
            temp = temp + instruction[i];
        }
        Parsed.RS1 = stoi(temp);
        Parsed.rs2 = -1;
    } else if ((Parsed.op == "RET")) {
        i = i + 2;
        for (i; i < instruction.length(); i++) {
            temp = temp + instruction[i];
        }
        Parsed.RS1 = stoi(temp);
        Parsed.rs2 = -1;
        Parsed.RD = -1;
    }
    return Parsed;
}

void getInstructions(int &pc, int & instEnd, string dataAndInstMemory[] ) {
    string filepath;
    cout << "Please enter the path to the file containing Assembly Code: " << endl;
    getline(cin, filepath);

    ifstream inp;
    int instAddr, dataAddr, count = 0;
    inp.open(filepath);

    while (inp.fail()) {
        cout << "File path incorrect, please reenter: " << endl;
        getline(cin, filepath);
        inp.open(filepath);
    }

    if (inp.is_open()) {
        string temp;
        getline(inp, temp);
        while (!inp.eof()) {
            if (temp.find(".text:") != string::npos) {
                getline(inp, temp);
                if (!inp.eof()) {
                    instAddr = stoi(temp);
                    count = instAddr;
                    pc = instAddr;
                    getline(inp, temp);
                }
                
            } else if (temp.find(".data:") != string::npos) {
                getline(inp, temp);
                while (!inp.eof()) {
                    int i = 0;
                    string dataAddress = "";
                    while (temp[i] != ' ') {
                        dataAddress += temp[i];
                        i++;
                    }
                    dataAddr = stoi(dataAddress);
                    i++;
                    string data = "";
                    for (i; i < temp.length(); i++) {
                        data += temp[i];
                    }
                    if (dataAddr < instAddr || dataAddr > instEnd)
                        dataAndInstMemory[dataAddr] = data;
                    getline(inp, temp);
                }
            } else {
                dataAndInstMemory[count] = temp;
                count++;
                getline(inp, temp);
            }
        }
        instEnd = count - 1;
    } else {
        cout << "error opening file" << endl;
    }
}

void fetch(int & pc, int & instEnd, string dataAndInstMemory[], int & instIndex, int clk, int & countMissBranch) {
    if (instrBuffer.size() < 4) {
        int i;
        Instruction p;

        //Load 1 Instruction only if one space is free
        if (instrBuffer.size() == 3)
            i = 1;
        else
            i = 0;

        while (i < 2 && pc <= instEnd) {
            p = Dedocde(dataAndInstMemory[pc], pc);
            p.index = instIndex;
            instrBuffer.push(p);
            i++;

            if ((p.op == "BEQ" && branchPredictor("Predict", p.Addr, 0, p.Imm, countMissBranch)) || p.op == "JMP") {
                pc += p.Imm + 1;
            } else
                pc++;

            StageLog instLog;
            instLog.inst = p;
            instLog.Executed = -1;
            instLog.count = -1;
            instLog.Fetched = clk;
            instLog.Issued = -1;
            instLog.Written = -1;
            instLog.Committed = -1;
            instLog.flushed = 0;
            Log.push_back(instLog);
            instIndex++;
        }
    }
}

void issue(int clk, int registers[], int registerStatus[], int& head, int &tail) {
    if (instrBuffer.size() > 0) //check if instruction buffer is not empty for issue
    {
        //reservationStation R[], int size, Instruction inst, int LogIndex, int &head, int&tail, int registerStatus[], int registers[], int clk
        Instruction inst = instrBuffer.front();
        for (int i = 0; i < Log.size(); i++) {
            if ((inst.index == Log[i].inst.index && Log[i].Fetched < clk && Log[i].Issued == -1 && !Log[i].flushed)) {
                if (inst.op == "ADDI" || inst.op == "ADD" || inst.op == "SUB") {
                    issue(addStation, RS_Add_size, inst, i, head, tail, registerStatus, registers, clk);
                } else if (inst.op == "NAND") {
                    issue(nandStation, RS_NAND_size, inst, i, head, tail, registerStatus, registers, clk);
                } else if (inst.op == "MUL") {
                    issue(multStation, RS_Mult_size, inst, i, head, tail, registerStatus, registers, clk);
                } else if (inst.op == "BEQ") {
                    issue(beqStation, RS_Branch_size, inst, i, head, tail, registerStatus, registers, clk);
                } else if (inst.op == "JMP" || inst.op == "JALR" || inst.op == "RET") {
                    issue(jumpStation, RS_Jump_size, inst, i, head, tail, registerStatus, registers, clk);
                } else if (inst.op == "LW") {
                    issue(loadStation, RS_loadAndStore_size, inst, i, head, tail, registerStatus, registers, clk);
                } else if (inst.op == "SW") {
                    issue(storeStation, RS_loadAndStore_size, inst, i, head, tail, registerStatus, registers, clk);
                }
            }
        }
    }


    if (instrBuffer.size() > 0) {
        Instruction inst1 = instrBuffer.front();
        for (int i = 0; i < Log.size(); i++) {
            if (inst1.index == Log[i].inst.index && Log[i].Fetched < clk && Log[i].Issued == -1 && !Log[i].flushed) {
                if (inst1.op == "ADDI" || inst1.op == "ADD" || inst1.op == "SUB") {
                    issue(addStation, RS_Add_size, inst1, i, head, tail, registerStatus, registers, clk);
                } else if (inst1.op == "NAND") {
                    issue(nandStation, RS_NAND_size, inst1, i, head, tail, registerStatus, registers, clk);
                } else if (inst1.op == "MUL") {
                    issue(multStation, RS_Mult_size, inst1, i, head, tail, registerStatus, registers, clk);
                } else if (inst1.op == "BEQ") {
                    issue(beqStation, RS_Branch_size, inst1, i, head, tail, registerStatus, registers, clk);
                } else if (inst1.op == "JMP" || inst1.op == "JALR" || inst1.op == "RET") {
                    issue(jumpStation, RS_Jump_size, inst1, i, head, tail, registerStatus, registers, clk);
                } else if (inst1.op == "LW") {
                    issue(loadStation, RS_loadAndStore_size, inst1, i, head, tail, registerStatus, registers, clk);
                } else if (inst1.op == "SW") {
                    issue(storeStation, RS_loadAndStore_size, inst1, i, head, tail, registerStatus, registers, clk);
                }
            }
        }
    }
}

void issue(reservationStation R[], int size, Instruction inst, int LogIndex, int &head, int&tail, int registerStatus[], int registers[], int clk) {
    for (int i = 0; i < size; i++) {
        if (R[i].isBusy == 0) {
            if ((head == tail && !ROB[tail].busy) || (!ROB[(tail + 1) % robSize].busy)) {
                if (((head == tail) && ROB[head].busy == 1) ||
                    ((head != tail))) {
                    tail = (tail + 1) % robSize;
                }

                if (inst.op == "ADD" || inst.op == "SUB" || inst.op == "NAND" || inst.op == "MUL") {
                    if (registerStatus[inst.RS1] == -1) {
                        R[i].Vj = registers[inst.RS1];
                        R[i].Qj = -1;
                    } else {
                        if (ROB[registerStatus[inst.RS1]].ready == 1) {
                            R[i].Vj = ROB[registerStatus[inst.RS1]].value;
                            R[i].Qj = -1;
                        } else {
                            R[i].Vj = -1;
                            R[i].Qj = registerStatus[inst.RS1];
                        }
                    }

                    if (registerStatus[inst.rs2] == -1) {
                        R[i].Vk = registers[inst.rs2];
                        R[i].Qk = -1;
                    } else {
                        if (ROB[registerStatus[inst.rs2]].ready == 1) {
                            R[i].Vk = ROB[registerStatus[inst.rs2]].value;
                            R[i].Qk = -1;
                        } else {
                            R[i].Vk = -1;
                            R[i].Qk = registerStatus[inst.rs2];
                        }
                    }


                    R[i].addr = -1;
                } else if (inst.op == "ADDI") {
                    if (registerStatus[inst.RS1] == -1) {
                        R[i].Vj = registers[inst.RS1];
                        R[i].Qj = -1;
                    } else {
                        if (ROB[registerStatus[inst.RS1]].ready == 1) {
                            R[i].Vj = ROB[registerStatus[inst.RS1]].value;
                            R[i].Qj = -1;
                        } else {
                            R[i].Vj = -1;
                            R[i].Qj = registerStatus[inst.RS1];
                        }
                    }

                    R[i].Vk = inst.Imm;
                    R[i].Qk = -1;


                    R[i].addr = -1;
                } else if (inst.op == "LW") {
                    if (registerStatus[inst.RS1] == -1) {
                        R[i].Vj = registers[inst.RS1];
                        R[i].Qj = -1;
                    } else {
                        if (ROB[registerStatus[inst.RS1]].ready == 1) {
                            R[i].Vj = ROB[registerStatus[inst.RS1]].value;
                            R[i].Qj = -1;
                        } else {
                            R[i].Vj = -1;
                            R[i].Qj = registerStatus[inst.RS1];
                        }
                    }

                    R[i].Vk = -1;
                    R[i].Qk = -1;


                    R[i].addr = inst.Imm;
                } else if (inst.op == "BEQ" || inst.op == "SW") {
                    if (registerStatus[inst.RS1] == -1) {
                        R[i].Vj = registers[inst.RS1];
                        R[i].Qj = -1;
                    } else {
                        if (ROB[registerStatus[inst.RS1]].ready == 1) {
                            R[i].Vj = ROB[registerStatus[inst.RS1]].value;
                            R[i].Qj = -1;
                        } else {
                            R[i].Vj = -1;
                            R[i].Qj = registerStatus[inst.RS1];
                        }
                    }

                    if (registerStatus[inst.rs2] == -1) {
                        R[i].Vk = registers[inst.rs2];
                        R[i].Qk = -1;
                    } else {
                        if (ROB[registerStatus[inst.rs2]].ready == 1) {
                            R[i].Vk = ROB[registerStatus[inst.rs2]].value;
                            R[i].Qk = -1;
                        } else {
                            R[i].Vk = -1;
                            R[i].Qk = registerStatus[inst.rs2];
                        }
                    }


                    R[i].addr = inst.Imm;
                } else if (inst.op == "JMP") {
                    R[i].Vj = -1;
                    R[i].Qj = -1;
                    R[i].Vk = -1;
                    R[i].Qk = -1;


                    R[i].addr = inst.Imm;
                } else if (inst.op == "JALR" || inst.op == "RET") {
                    if (registerStatus[inst.RS1] == -1) {
                        R[i].Vj = registers[inst.RS1];
                        R[i].Qj = -1;
                    } else {
                        if (ROB[registerStatus[inst.RS1]].ready == 1) {
                            R[i].Vj = ROB[registerStatus[inst.RS1]].value;
                            R[i].Qj = -1;
                        } else {
                            R[i].Vj = -1;
                            R[i].Qj = registerStatus[inst.RS1];
                        }
                    }

                    R[i].Vk = -1;
                    R[i].Qk = -1;

                    R[i].addr = -1;
                }

                R[i].isBusy = true;
                R[i].op = inst.op;
                R[i].instAddr = inst.Addr;
                R[i].instIndex = inst.index;

                ROB[tail].busy = 1;
                ROB[tail].ready = 0;
                ROB[tail].dest = inst.RD;
                ROB[tail].inst = inst;

                Log[LogIndex].Issued = clk;

                if (inst.RD != 0 && inst.RD != -1)
                    registerStatus[inst.RD] = tail;

                instrBuffer.pop();
                break;
            }
        }
    }
}

void execute(int clk, string dataAndInstMemory[]) {

    for (int i = 0; i < RS_loadAndStore_size; i++) {
        // Load
        if ((loadStation[i].Qj == -1) && (loadStation[i].isBusy == 1)) {
            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (loadStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }
            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].count == -1)) {
                    Log[index].count = 2;
                }
                if (Log[index].count == 2) {
                    bool flag = true;
                    for (int k = 0; k < 2; k++) // loop on sw functional units
                    {
                        int storeIndex = 0;
                        for (int j = 0; j < Log.size(); j++) {
                            if (loadStation[i].instIndex == Log[j].inst.index) {
                                storeIndex = j;
                            }
                        }
                        if (storeStation[k].isBusy == 1 && (Log[storeIndex].Issued < Log[index].Issued ||
                                                            (Log[storeIndex].Issued == Log[index].Issued &&
                                                             storeStation[k].instAddr <
                                                             loadStation[i].instAddr))) //check if FU is busy and occupying instruction is before current load
                        {
                            flag = false;
                        }
                    }
                    if (flag) // execute step 1 and move to step 2 of exectution
                    {
                        Log[index].count = 1;
                        loadStation[i].addr += loadStation[i].Vj; // compute load address
                    }
                } else if (Log[index].count == 1) {
                    bool flag = true;
                    for (int k = 0; k < 6; k++) // loop on ROB
                    {
                        int storeIndex = 0;
                        for (int j = 0; j < Log.size(); j++) {
                            if (loadStation[i].instIndex == Log[j].inst.index) {
                                storeIndex = j;
                            }
                        }
                        if (ROB[k].pcAddr == loadStation[i].addr && ROB[k].inst.op == "SW" && ROB[k].busy == 1 &&
                            (Log[storeIndex].Issued < Log[index].Issued ||
                             (Log[storeIndex].Issued == Log[index].Issued && storeStation[k].instAddr <
                                                                             loadStation[i].instAddr))) //check if ROB contains dependable instructions
                        {
                            flag = false;
                        }
                    }
                    if (flag) {
                        Log[index].count = 0;
                        loadStation[i].Vk = stoi(dataAndInstMemory[loadStation[i].addr]); //load value in operand 2
                    }
                }

                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    Log[index].Executed = clk;
                    Log[index].inst.result = loadStation[i].Vk; //assign result in Log the second operand assigned above to the value gotten from dataAndInstMemory
                }
            }
        }
        // Store
        if ((storeStation[i].Qj == -1) && (storeStation[i].isBusy == 1) && (storeStation[i].Qk == -1)) //store
        {

            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (storeStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }
            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].count == -1)) {
                    Log[index].count = 0;
                } else if (Log[index].count != 0) {
                    Log[index].count--;
                }
                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    int n;
                    Log[index].Executed = clk;
                    storeStation[i].addr += storeStation[i].Vk;
                    Log[index].inst.pcAddr = storeStation[i].addr;
                    Log[index].inst.result = storeStation[i].Vj;
                }
            }
        }
        //BEQ
        if ((beqStation[i].Qj == -1) && (beqStation[i].isBusy == 1) && (beqStation[i].Qk == -1)) //BEQ
        {

            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (beqStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }
            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].count == -1)) {
                    Log[index].count = 0;
                } else if (Log[index].count != 0) {
                    Log[index].count--;
                }
                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    Log[index].Executed = clk;
                    int Vj, Vk;
                    if (Log[index].inst.index == beqStation[i].instIndex) {
                        Vj = beqStation[i].Vj;
                        Vk = beqStation[i].Vk;
                        if (Vj == Vk) {
                            Log[index].inst.result = 1;
                            Log[index].inst.pcAddr = beqStation[i].instAddr + 1 + beqStation[i].addr;
                        } else {
                            Log[index].inst.result = 0;
                            Log[index].inst.pcAddr = beqStation[i].instAddr + 1;
                        }
                    }
                }
            }
        }
        //MULT
        if ((multStation[i].Qj == -1) && (multStation[i].isBusy == 1) && (multStation[i].Qk == -1)) //Mult
        {
            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (multStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }
            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].count == -1)) {
                    Log[index].count = 7;
                } else if (Log[index].count != 0) {
                    Log[index].count--;
                }
                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    int Vj, Vk;
                    Vj = multStation[i].Vj;
                    Vk = multStation[i].Vk;
                    Log[index].inst.result = Vj * Vk;
                    Log[index].Executed = clk;
                }
            }
        }
    }
    /////////////////////////3  Reservation Stations////////////////////////////////////////////////
    for (int i = 0; i < RS_Add_size; i++) {
        if ((addStation[i].Qj == -1) && (addStation[i].isBusy == 1) && (addStation[i].Qk == -1)) //Add,Sub,Addi
        {
            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (addStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }

            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].Issued != -1) && (Log[index].count == -1)) {
                    Log[index].count = 1;
                } else if (Log[index].count != 0) {
                    Log[index].count--;
                }
                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    Log[index].Executed = clk;
                    if (Log[index].inst.op == "ADD") {
                        int Vj, Vk;
                        Vj = addStation[i].Vj;
                        Vk = addStation[i].Vk;
                        Log[index].inst.result = Vj + Vk;
                    } else if (Log[index].inst.op == "SUB") {
                        int Vj, Vk;
                        Vj = addStation[i].Vj;
                        Vk = addStation[i].Vk;
                        Log[index].inst.result = Vj - Vk;
                    }
                    if (Log[index].inst.op == "ADDI") {
                        int Vj, Vk;
                        Vj = addStation[i].Vj;
                        Vk = addStation[i].Vk;
                        Log[index].inst.result = Vj + Vk;
                    }
                }
            }
        }
        //JALR, RET
        if ((jumpStation[i].Qj == -1) && (jumpStation[i].isBusy == 1) && (jumpStation[i].op != "JMP")) //Jalr,Ret
        {
            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (jumpStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }
            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].count == -1)) {
                    Log[index].count = 0;
                } else if (Log[index].count != 0) {
                    Log[index].count--;
                }
                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    Log[index].Executed = clk;
                    if (Log[index].inst.Addr == jumpStation[i].instAddr) {
                        if (Log[index].inst.op == "JALR") {
                            int Vj;
                            Vj = jumpStation[i].Vj;
                            Log[index].inst.result = Log[index].inst.Addr + 1;
                            Log[index].inst.pcAddr = Vj;
                        } else if (Log[index].inst.op == "RET") {
                            int Vj;
                            Vj = jumpStation[i].Vj;
                            Log[index].inst.pcAddr = Vj;
                        }
                    }
                }
            }
        }
        //JMP
        if ((jumpStation[i].isBusy == 1) && (jumpStation[i].op == "JMP")) //Jmp
        {
            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (jumpStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }
            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].Issued != -1) && (Log[index].count == -1)) {
                    Log[index].count = 0;
                } else if (Log[index].count != 0) {
                    Log[index].count--;
                }
                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    Log[index].Executed = clk;
                    int Vj;
                    Vj = jumpStation[i].addr;
                    Log[index].inst.pcAddr = Vj;
                }
            }
        }
    }

    for (int i = 0; i < RS_NAND_size; i++) {
        if ((nandStation[i].Qj == -1) && (nandStation[i].isBusy == 1) && (nandStation[i].Qk == -1)) //nand
        {
            int index = 0;
            for (int j = 0; j < Log.size(); j++) {
                if (nandStation[i].instIndex == Log[j].inst.index) {
                    index = j;
                }
            }
            if ((Log[index].Issued < clk) && (Log[index].Issued != -1) && !Log[index].flushed) {
                if ((Log[index].Executed == -1) && (Log[index].Issued != -1) && (Log[index].count == -1)) {
                    Log[index].count = 0;
                } else if (Log[index].count != 0) {
                    Log[index].count--;
                }
                if ((Log[index].count == 0) && (Log[index].Executed == -1)) {
                    Log[index].Executed = clk;
                    int Vj, Vk;
                    Vj = nandStation[i].Vj;
                    Vk = nandStation[i].Vk;
                    Log[index].inst.result = !(Vj & Vk);
                }
            }
        }
    }
}

void write(int clk) {
    int writes = 0;
    for (int i = 0; i < Log.size(); i++) {
        if (Log[i].Executed < clk && Log[i].Written == -1 && Log[i].Executed != -1 && writes < 2 &&
            !Log[i].flushed) {
            Log[i].Written = clk;

            writes++;

            int index = Log[i].inst.index;

            int curr;

            for (int j = 0; j < robSize; j++) {
                if (ROB[j].inst.index == Log[i].inst.index && ROB[j].busy == 1)
                    curr = j;
            }

            ROB[curr].value = Log[i].inst.result;
            ROB[curr].pcAddr = Log[i].inst.pcAddr;
            ROB[curr].ready = 1;

            if (Log[i].inst.op == "ADD" || Log[i].inst.op == "SUB" || Log[i].inst.op == "ADDI") {
                for (int j = 0; j < RS_Add_size; j++) {
                    if (addStation[j].instIndex == index) {
                        addStation[j].isBusy = 0;
                        addStation[j].op = "";
                        addStation[j].Vj = -1;
                        addStation[j].Vk = -1;
                        addStation[j].Qj = -1;
                        addStation[j].Qk = -1;
                        addStation[j].addr = -1;
                        addStation[j].instAddr = -1;
                        addStation[j].instIndex = -1;
                    }
                }
            } else if (Log[i].inst.op == "JALR" || Log[i].inst.op == "JMP" || Log[i].inst.op == "RET") {
                for (int j = 0; j < RS_Jump_size; j++) {
                    if (jumpStation[j].instIndex == index) {
                        jumpStation[j].isBusy = 0;
                        jumpStation[j].op = "";
                        jumpStation[j].Vj = -1;
                        jumpStation[j].Vk = -1;
                        jumpStation[j].Qj = -1;
                        jumpStation[j].Qk = -1;
                        jumpStation[j].addr = -1;
                        jumpStation[j].instAddr = -1;
                        jumpStation[j].instIndex = -1;
                    }
                }
            } else if (Log[i].inst.op == "LW") {
                for (int j = 0; j < RS_loadAndStore_size; j++) {
                    if (loadStation[j].instIndex == index) {
                        loadStation[j].isBusy = 0;
                        loadStation[j].op = "";
                        loadStation[j].Vj = -1;
                        loadStation[j].Vk = -1;
                        loadStation[j].Qj = -1;
                        loadStation[j].Qk = -1;
                        loadStation[j].addr = -1;
                        loadStation[j].instAddr = -1;
                        loadStation[j].instIndex = -1;
                    }
                }
            } else if (Log[i].inst.op == "SW") {
                for (int j = 0; j < RS_loadAndStore_size; j++) {
                    if (storeStation[j].instIndex == index) {
                        storeStation[j].isBusy = 0;
                        storeStation[j].op = "";
                        storeStation[j].Vj = -1;
                        storeStation[j].Vk = -1;
                        storeStation[j].Qj = -1;
                        storeStation[j].Qk = -1;
                        storeStation[j].addr = -1;
                        storeStation[j].instAddr = -1;
                        storeStation[j].instIndex = -1;
                    }
                }
            } else if (Log[i].inst.op == "BEQ") {
                for (int j = 0; j < RS_Branch_size; j++) {
                    if (beqStation[j].instIndex == index) {
                        beqStation[j].isBusy = 0;
                        beqStation[j].op = "";
                        beqStation[j].Vj = -1;
                        beqStation[j].Vk = -1;
                        beqStation[j].Qj = -1;
                        beqStation[j].Qk = -1;
                        beqStation[j].addr = -1;
                        beqStation[j].instAddr = -1;
                        beqStation[j].instIndex = -1;
                    }
                }
            } else if (Log[i].inst.op == "MUL") {
                for (int j = 0; j < RS_Mult_size; j++) {
                    if (multStation[j].instIndex == index) {
                        multStation[j].isBusy = 0;
                        multStation[j].op = "";
                        multStation[j].Vj = -1;
                        multStation[j].Vk = -1;
                        multStation[j].Qj = -1;
                        multStation[j].Qk = -1;
                        multStation[j].addr = -1;
                        multStation[j].instAddr = -1;
                        multStation[j].instIndex = -1;
                    }
                }
            } else if (Log[i].inst.op == "NAND") {
                for (int j = 0; j < RS_NAND_size; j++) {
                    nandStation[j].isBusy = 0;
                    nandStation[j].op = "";
                    nandStation[j].Vj = -1;
                    nandStation[j].Vk = -1;
                    nandStation[j].Qj = -1;
                    nandStation[j].Qk = -1;
                    nandStation[j].addr = -1;
                    nandStation[j].instAddr = -1;
                    nandStation[j].instIndex = -1;
                }
            }

            for (int i = 0; i < RS_Jump_size; i++) {
                if (jumpStation[i].Qj == curr) {
                    jumpStation[i].Vj = ROB[curr].value;
                    jumpStation[i].Qj = -1;
                }
                if (jumpStation[i].Qk == curr) {
                    jumpStation[i].Vk = ROB[curr].value;
                    jumpStation[i].Qk = -1;
                }
                if (addStation[i].Qj == curr) {
                    addStation[i].Vj = ROB[curr].value;
                    addStation[i].Qj = -1;
                }
                if (addStation[i].Qk == curr) {
                    addStation[i].Vk = ROB[curr].value;
                    addStation[i].Qk = -1;
                }
            }

            for (int i = 0; i < RS_loadAndStore_size; i++) //lw unit, sw unit, beq unit, mult
            {
                if (loadStation[i].Qj == curr) {
                    loadStation[i].Vj = ROB[curr].value;
                    loadStation[i].Qj = -1;
                }
                if (loadStation[i].Qk == curr) {
                    loadStation[i].Vk = ROB[curr].value;
                    loadStation[i].Qk = -1;
                }
                if (storeStation[i].Qj == curr) {
                    storeStation[i].Vj = ROB[curr].value;
                    storeStation[i].Qj = -1;
                }
                if (storeStation[i].Qk == curr) {
                    storeStation[i].Vk = ROB[curr].value;
                    storeStation[i].Qk = -1;
                }
                if (beqStation[i].Qj == curr) {
                    beqStation[i].Vj = ROB[curr].value;
                    beqStation[i].Qj = -1;
                }
                if (beqStation[i].Qk == curr) {
                    beqStation[i].Vk = ROB[curr].value;
                    beqStation[i].Qk = -1;
                }
                if (multStation[i].Qj == curr) {
                    multStation[i].Vj = ROB[curr].value;
                    multStation[i].Qj = -1;
                }
                if (multStation[i].Qk == curr) {
                    multStation[i].Vk = ROB[curr].value;
                    multStation[i].Qk = -1;
                }
            }

            //nand unit
            for (int i = 0; i < RS_NAND_size; i++) {
                if (nandStation[i].Qj == curr) {
                    nandStation[i].Vj = ROB[curr].value;
                    nandStation[i].Qj = -1;
                }
                if (nandStation[i].Qk == curr) {
                    nandStation[i].Vk = ROB[curr].value;
                    nandStation[i].Qk = -1;
                }
            }
        }
    }
}

void commit(int clk, int &head, int & pc, int & totalBranchCount, string dataAndInstMemory[], int registers[], int & committedInstCount, int registerStatus[], int &tail, bool & done, int & countMissBranch ) {
    int writeCount = 0;

    for (int i = 0; i < Log.size(); i++) {
        if (Log[i].Written < clk && Log[i].Committed == -1 && Log[i].Written != -1 && writeCount < 2 &&
            !Log[i].flushed) {
            if (ROB[head].ready) {
                if (ROB[head].inst.index == Log[i].inst.index) {
                    bool flushFlag = false;
                    if (ROB[head].inst.op == "BEQ") {
                        flushFlag = branchPredictor("Verify", Log[i].inst.Addr, ROB[head].value, Log[i].inst.Imm, countMissBranch);
                        totalBranchCount++;
                    } else if (ROB[head].inst.op == "SW") {
                        dataAndInstMemory[ROB[head].pcAddr] = to_string(ROB[head].value);
                    } else if (ROB[head].inst.op == "JALR") {
                        registers[ROB[head].dest] = ROB[head].value;
                        flushFlag = true;
                    } else if (ROB[head].inst.op == "RET") {
                        flushFlag = true;
                    } else if (ROB[head].inst.op == "JMP") {
                    } else {
                        registers[ROB[head].dest] = ROB[head].value;
                    }

                    Log[i].Committed = clk;
                    writeCount++;
                    committedInstCount++;
                    ROB[head].busy = false;

                    if (registerStatus[ROB[head].dest] == head) {
                        registerStatus[ROB[head].dest] = -1;
                    }

                    if (flushFlag) {
                        pc = ROB[head].pcAddr;
                        for (int k = i + 1; k < Log.size(); k++) {
                            Log[k].flushed = 1;
                        }
                        flush(head, tail, registerStatus);
                    }

                    if (head != tail) {
                        head = (head + 1) % robSize;
                    }
                }
            }
        }
    }

    if (head == tail) {
        if (!ROB[head].busy) {
            done = true;
        } else {
            done = false;
        }
    } else if (ROB[head].busy) {
        done = false;
    }
}

bool branchPredictor(string operation, int address, bool correctBranch, int imm, int & countMissBranch) {


    if (operation == "Predict") {
        for (int i = 0; i < predictionBuffer.size(); i++) {
            if (predictionBuffer[i].Addr == address) {
                if (imm < 0) {
                    predictionBuffer[i].Status = true;
                    return true;
                } else {
                    predictionBuffer[i].Status = false;
                    return false;
                }
            }
        }

        prediction newEntry;
        newEntry.Addr = address;
        if (imm < 0) {
            newEntry.Status = true;
            predictionBuffer.push_back(newEntry);
            return true;
        } else {
            newEntry.Status = false;
            predictionBuffer.push_back(newEntry);
            return false;
        }
    } else {
        for (int i = 0; i < predictionBuffer.size(); i++) {
            if (predictionBuffer[i].Addr == address) {
                if (predictionBuffer[i].Status == correctBranch) {
                    return false;
                }
                if (predictionBuffer[i].Status != correctBranch) {
                    countMissBranch++;
                    return true;
                }
            }
        }
    }

    return false;
}

void flush(int &head, int & tail, int registerStatus[]) {
    for (int i = 0; i < 6; i++) {
        ROB[i].busy = 0;
        ROB[i].dest = -1;
        ROB[i].ready = 0;
        ROB[i].inst.Addr = -1;
    }
    head = 0;
    tail = 0;

    for (int i = 0; i < 8; i++) {
        registerStatus[i] = -1;
    }

    while (instrBuffer.size() != 0) {
        instrBuffer.pop();
    }

    for (int j = 0; j < RS_Add_size; j++) {
        addStation[j].isBusy = 0;
        addStation[j].op = "";
        addStation[j].Vj = -1;
        addStation[j].Vk = -1;
        addStation[j].Qj = -1;
        addStation[j].Qk = -1;
        addStation[j].addr = -1;
        addStation[j].instAddr = -1;
    }

    for (int j = 0; j < RS_Jump_size; j++) {
        jumpStation[j].isBusy = 0;
        jumpStation[j].op = "";
        jumpStation[j].Vj = -1;
        jumpStation[j].Vk = -1;
        jumpStation[j].Qj = -1;
        jumpStation[j].Qk = -1;
        jumpStation[j].addr = -1;
        jumpStation[j].instAddr = -1;
    }

    for (int j = 0; j < RS_loadAndStore_size; j++) {
        loadStation[j].isBusy = 0;
        loadStation[j].op = "";
        loadStation[j].Vj = -1;
        loadStation[j].Vk = -1;
        loadStation[j].Qj = -1;
        loadStation[j].Qk = -1;
        loadStation[j].addr = -1;
        loadStation[j].instAddr = -1;
        storeStation[j].isBusy = 0;
        storeStation[j].op = "";
        storeStation[j].Vj = -1;
        storeStation[j].Vk = -1;
        storeStation[j].Qj = -1;
        storeStation[j].Qk = -1;
        storeStation[j].addr = -1;
        storeStation[j].instAddr = -1;
    }

    for (int j = 0; j < RS_Branch_size; j++) {
        beqStation[j].isBusy = 0;
        beqStation[j].op = "";
        beqStation[j].Vj = -1;
        beqStation[j].Vk = -1;
        beqStation[j].Qj = -1;
        beqStation[j].Qk = -1;
        beqStation[j].addr = -1;
        beqStation[j].instAddr = -1;
    }

    for (int j = 0; j < RS_Mult_size; j++) {
        multStation[j].isBusy = 0;
        multStation[j].op = "";
        multStation[j].Vj = -1;
        multStation[j].Vk = -1;
        multStation[j].Qj = -1;
        multStation[j].Qk = -1;
        multStation[j].addr = -1;
        multStation[j].instAddr = -1;
    }

    for (int j = 0; j < RS_NAND_size; j++) {
        nandStation[j].isBusy = 0;
        nandStation[j].op = "";
        nandStation[j].Vj = -1;
        nandStation[j].Vk = -1;
        nandStation[j].Qj = -1;
        nandStation[j].Qk = -1;
        nandStation[j].addr = -1;
        nandStation[j].instAddr = -1;
    }
}


void printResults(int registers[], int committedInstCount, int countMissBranch, int totalBranchCount) {
    cout << endl;
    cout << setw(20) << left << "Instruction"
         << setw(10) << left << "FETCHED"
         << setw(10) << left << "ISSUED"
         << setw(10) << left << "EXECUTED"
         << setw(10) << left << "WRITTEN"
         << setw(10) << left << "COMMITTED" << endl;
    cout << "----------------------------------------------------------------------" << endl;
    for (int i = 0; i < Log.size(); i++) {
        cout << setw(20) << left << Log[i].inst.inst
             << setw(10) << left << Log[i].Fetched
             << setw(10) << left << Log[i].Issued
             << setw(10) << left << Log[i].Executed
             << setw(10) << left << Log[i].Written
             << setw(10) << left << Log[i].Committed << endl;
    }


    cout << endl;
    cout << "-----------------------------Register File Contents------------------------------" << endl;
    cout << setw(10) << left << "R0" << setw(10) << left << "R1" << setw(10) << left << "R2" << setw(10) << left << "R3"
         << setw(10) << left << "R4" << setw(10) << left << "R5"
         << setw(10) << left << "R6" << setw(10) << left << "R7" << endl;
    cout << "---------------------------------------------------------------------------------" << endl;
    cout << setw(10) << left << registers[0] << setw(10) << left << registers[1] << setw(10) << left
         << registers[2] << setw(10) << left << registers[3]
         << setw(10) << left << registers[4] << setw(10) << left << registers[5] << setw(10) << left
         << registers[6] << setw(10)
         << left << registers[7] << endl << endl << endl;


    int last = Log.size() - 1;
    while (Log[last].Committed == -1)
        last--;
    cout << "Number of cycles: " << Log[last].Committed << endl;
    cout << "IPC = " << double(committedInstCount) / Log[last].Committed << endl;
    cout << "Branch Miss Prediction Rate = " << 100.0 * (double(countMissBranch) / double(totalBranchCount)) << "%" << endl;
}