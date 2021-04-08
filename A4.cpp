#include <bits/stdc++.h>
#include <fstream>
#include <string>
#include <algorithm>
using namespace std;

struct Node
{
	string instruction = "empty"; // op : add,addi,sub etc
	string r0 = "empty";
	string r1 = "empty";
	string r2 = "empty";
	Node *next;
	Node *prev;
};

struct LinkedList
{
	Node *head = NULL; // head of the list
	Node *tail = NULL; // tail of the list
};

int CLK;
string Registers[32];	   // store register names
int RegisterValues[32];	   // register values
string InstructionSet[10]; // instructions allowed
int counter = 0;
map<string, int> Labels; //to store all the branches/labels
vector<string> inputProgram;
map<string, int> freqOfEachInstruction;
map<string, int> Memory;
string currentInstruction;
int r[3];
string address;
string currReg = "";
int StackMemory[262114];

int ROW_ACCESS_DELAY; // row access delay
int COL_ACCESS_DELAY; // col access delay
int MAX_DRAM_SIZE = pow(2, 20);

vector<vector<int>> DRAM;	   // 1024 x 1024 memory, values
vector<vector<bool>> Occupied; // 1024 x 1024 memory, status
vector<int> Row_Buffer;		   // Row_buffer
int row_present;			   // Row present in row buffer
bool first_read;			   // first time reading

void ActivateRow(int x); // x is the row that is needed to be ActivateRow
void WritebackRow();
bool RowAlreadyInBuffer(int row);
void ColumnAccess(int x, string r); // Copy the data at the column offset from the row buffer to the REGISTER.
void READ(int x, string r);			// x is the address (index)

void WRITE(int x, string r);
void parseInstruction(LinkedList *l, string x);
vector<LinkedList *> lists;
void printList(LinkedList *l);
// Linked List, and functions

void insertAtTail(LinkedList *l, string x, string r_0, string r_1, string r_2)
{
	Node *new_node = new Node;
	new_node->instruction = x;
	new_node->r0 = r_0;
	new_node->r2 = r_2;
	new_node->r1 = r_1;
	new_node->next = NULL;
	if (l->head == NULL)
	{
		l->head = new_node;
		l->tail = new_node;
		return;
	}
	l->tail->next = new_node;
	l->tail = new_node;
}

void printList(LinkedList *l)
{
	cout << "HIII" << endl;
	Node *current = l->head;
	while (current != NULL)
	{
		cout << current->instruction << " ";
		cout << current->instruction << ' ';
		cout << current->r0 << ' ';
		cout << current->r1 << ' ';
		cout << current->r2 << ' ';
		cout << endl;
		current = current->next;
	}
	cout << endl;
}

Node *move(Node *node1, Node *node2)
{
	if (node1->prev != NULL)
	{
		node1->prev->next = node2;
	}
	if (node2->prev != NULL)
	{
		node2->prev->next = node2->next;
	}
	if (node2->next != NULL)
	{
		node2->next->prev = node2->prev;
	}
	node2->next = node1;
	node1->prev = node2;
	return node2;
}

bool checkSize(int x)
{
	if (currReg == "sp")
	{
		if (x < 0 || x >= 1048576 || x % 4 != 0)
		{
			return false;
		}
		return true;
	}
	return x > -2147483648 || x < 2147483647;
}
bool findMyRegister(string x)
{
	for (int i = 0; i < 32; i++)
	{
		if (x == Registers[i])
		{
			return true;
		}
	}
	return false;
}
bool ReservedRegisters(string x)
{
	if (x == "at" || x == "k1" || x == "k2" || x == "zero")
		return true;
	else
		return false;
}
bool findMyLabel(string x)
{
	if (Labels.find(x) == Labels.end())
		return false;
	else
		return true;
}
int findPosOfFirstComma(string x)
{
	for (int i = 0; i < x.size(); i++)
	{
		if (x[i] == ',')
			return i;
	}
	return -1;
}

int getValueFromRegister(string x)
{
	for (int i = 0; i < 32; i++)
	{
		if (Registers[i] == x)
		{
			return RegisterValues[i];
		}
	}
	return INT_MAX;
}
void LoadValueIntoRegister(string s, int x)
{
	for (int i = 0; i < 32; i++)
	{
		if (Registers[i] == s)
		{
			RegisterValues[i] = x;
		}
	}
}
int giveIndexOfMyRegister(string x)
{
	for (int i = 0; i < 32; i++)
	{
		if (Registers[i] == x)
		{
			return i;
		}
	}
	return INT_MAX;
}
string RemoveSpaces(string x)
{
	string t = "";
	for (int i = 0; i < x.size(); i++)
	{
		if (x[i] != ' ' && x[i] != '\t')
		{
			t += x[i];
		}
	}
	return t;
}

void add()
{
	int temp = RegisterValues[r[1]] + RegisterValues[r[2]];
	if (checkSize(temp))
	{
		RegisterValues[r[0]] = temp;
	}
	else
	{
		cout << "Out of bound " << counter << endl;
		exit(1);
	}
}

void addi()
{
	int temp = RegisterValues[r[1]] + r[2];
	if (checkSize(temp))
	{
		RegisterValues[r[0]] = temp;
	}
	else
	{
		cout << "Out of bound " << counter << endl;
		exit(1);
	}
}

void sub()
{
	int temp = RegisterValues[r[1]] - RegisterValues[r[2]];
	if (checkSize(temp))
	{
		RegisterValues[r[0]] = temp;
	}
	else
	{
		cout << "Out of bound " << counter << endl;
		exit(1);
	}
}

void mul()
{
	int temp = RegisterValues[r[1]] * RegisterValues[r[2]];
	if (checkSize(temp))
	{
		RegisterValues[r[0]] = temp;
	}
	else
	{
		cout << "Out of bound " << counter << endl;
		exit(1);
	}
}

void slt()
{
	RegisterValues[r[0]] = RegisterValues[r[1]] < RegisterValues[r[2]];
}

void lw()
{
	if (address.front() != '$')
	{
		RegisterValues[r[0]] = Memory[address];
	}
	else
	{
		address = address.substr(1);
		if (!findMyRegister(address))
		{
			cout << "Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		if (getValueFromRegister(address) < 0 || getValueFromRegister(address) % 4 != 0 || getValueFromRegister(address) > 1048576)
		{
			cout << "Bad address (less than zero or not multiple of 4)" << endl;
			exit(1);
		}
		RegisterValues[r[0]] = StackMemory[getValueFromRegister(address) / 4];
	}
}

void lwDRAM()
{
	int memoryAddress = stoi(address);
	string reg = Registers[r[0]];
	READ(memoryAddress, reg);
}

void swDRAM()
{
	int memoryAddress = stoi(address);
	string reg = Registers[r[0]];
	WRITE(memoryAddress, reg);
}

void sw()
{
	if (address.front() != '$')
	{
		Memory[address] = RegisterValues[r[0]];
	}
	else
	{
		address = address.substr(1);
		if (!findMyRegister(address))
		{
			cout << "Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		if (getValueFromRegister(address) < 0 || getValueFromRegister(address) % 4 != 0 || getValueFromRegister(address) > 1048576)
		{
			cout << "Bad address (less than zero or not multiple of 4)" << endl;
			exit(1);
		}
		StackMemory[getValueFromRegister(address) / 4] = RegisterValues[r[0]];
	}
}

void beq()
{
	if (RegisterValues[r[0]] == RegisterValues[r[1]])
	{
		counter = r[2];
	}
}

void bne()
{
	if (RegisterValues[r[0]] != RegisterValues[r[1]])
	{
		counter = r[2];
	}
}

void j()
{
	counter = r[0];
}

void decToHexa(int n)
{
	char hexaDeciNum[100];
	int i = 0;
	while (n != 0)
	{
		int temp = 0;
		temp = n % 16;
		if (temp < 10)
		{
			hexaDeciNum[i] = temp + 48;
			i++;
		}
		else
		{
			hexaDeciNum[i] = temp + 55;
			i++;
		}

		n = n / 16;
	}
	for (int j = i - 1; j >= 0; j--)
		cout << hexaDeciNum[j];
}

void READ(int x, string r)
{ // x -> address, r -> register
	if (x < 0 || x >= MAX_DRAM_SIZE)
	{
		cout << "Index Out of Bounds 1" << '\n';
		exit(1);
	}
	int row = x / 1024;
	int col = x % 1024;
	if (row < 0 || row >= 1024 || col < 0 || col >= 1024)
	{
		cout << "Index Out of Bounds 2" << '\n';
		exit(1);
	}

	if (RowAlreadyInBuffer(row))
	{
		ColumnAccess(col, r);
	}
	else
	{
		if (row_present != -1)
		{ // Non-empty
			WritebackRow();
		}
		ActivateRow(row);
		ColumnAccess(col, r);
	}
}

void ActivateRow(int x)
{ // x is the row
	if (x < 0 || x >= 1024)
	{
		cout << "Index Access Out of Bounds 3";
		exit(1);
	}
	for (int i = 0; i < Row_Buffer.size(); i++)
	{
		Row_Buffer[i] = DRAM[x][i];
	}
	row_present = x;
	CLK += ROW_ACCESS_DELAY;
}

void ColumnAccess(int x, string r)
{ // x is the cell column, r -> register
	if (x < 0 || x >= 1024)
	{
		cout << "Index Access out of bounds 4";
		exit(1);
	}
	CLK += COL_ACCESS_DELAY;
	int val = Row_Buffer[x];
	LoadValueIntoRegister(r, val);
}

bool RowAlreadyInBuffer(int row)
{
	return row_present == row;
}

void WritebackRow()
{
	CLK += ROW_ACCESS_DELAY;
	int row = row_present;
	for (int i = 0; i < Row_Buffer.size(); i++)
	{
		DRAM[row][i] = Row_Buffer[i];
	}
	row_present = -1; // no row in Row buffer
}

void WRITE(int x, string r)
{ // r -> register, x -> address
	if (x < 0 || x >= MAX_DRAM_SIZE)
	{
		cout << "Index out of bounds 5" << '\n';
		exit(1);
	}
	int row = x / 1024;
	int col = x % 1024;
	if (row < 0 || row >= 1024 || col < 0 || col >= 1024)
	{
		cout << "Index out of bounds 6" << '\n';
		exit(1);
	}
	if (RowAlreadyInBuffer(row))
	{
		CLK += COL_ACCESS_DELAY;
		int val = getValueFromRegister(r);
		Row_Buffer[col] = val;
	}
	else
	{
		if (row_present != -1)
		{ // Non-Empty
			WritebackRow();
		}
		for (int i = 0; i < Row_Buffer.size(); i++)
		{ // Load contents of new row
			Row_Buffer[i] = DRAM[row][i];
		}
		CLK += ROW_ACCESS_DELAY;
		int val = getValueFromRegister(r);
		Row_Buffer[col] = val;
		CLK += COL_ACCESS_DELAY;
		row_present = row;
	}
}

void init(string infilename)
{
	Registers[0] = "zero";
	Registers[1] = "at";
	Registers[2] = "v0";
	Registers[3] = "v1";
	Registers[4] = "a0";
	Registers[5] = "a1";
	Registers[6] = "a2";
	Registers[7] = "a3";
	Registers[8] = "t0";
	Registers[9] = "t1";
	Registers[10] = "t2";
	Registers[11] = "t3";
	Registers[12] = "t4";
	Registers[13] = "t5";
	Registers[14] = "t6";
	Registers[15] = "t7";
	Registers[16] = "s0";
	Registers[17] = "s1";
	Registers[18] = "s2";
	Registers[19] = "s3";
	Registers[20] = "s4";
	Registers[21] = "s5";
	Registers[22] = "s6";
	Registers[23] = "s7";
	Registers[24] = "t8";
	Registers[25] = "t9";
	Registers[26] = "k0";
	Registers[27] = "k1";
	Registers[28] = "gp";
	Registers[29] = "sp";
	Registers[30] = "s8";
	Registers[31] = "ra";

	for (int i = 0; i < 32; i++)
	{
		RegisterValues[i] = 0;
	}

	InstructionSet[0] = "add";
	InstructionSet[1] = "sub";
	InstructionSet[2] = "mul";
	InstructionSet[3] = "beq";
	InstructionSet[4] = "bne";
	InstructionSet[5] = "slt";
	InstructionSet[6] = "j";
	InstructionSet[7] = "lw";
	InstructionSet[8] = "sw";
	InstructionSet[9] = "addi";

	ifstream inputfile;
	inputfile.open(infilename.c_str(), ios::in);
	if (!inputfile)
	{
		cout << "No file given or not able to open file";
		exit(1);
	}

	string x;
	int i2 = 1;
	LinkedList *curr = new LinkedList;
	lists.push_back(curr);
	while (getline(inputfile, x))
	{
		int tmp = 0;
		while (true)
		{
			if (x[tmp] == ' ' || x[tmp] == '\t')
			{
				tmp++;
			}
			else
				break;
		}
		x = x.substr(tmp);
		if (x.find(':') != -1)
		{
			string tempLabel = x.substr(0, x.find(":"));
			x.erase(std::remove_if(x.begin(), x.end(), ::isspace), x.end());
			if (Labels[tempLabel] != 0)
			{
				cout << "Multiple branch with same name " << endl;
				cout << tempLabel << endl;
				exit(1);
			}
			// LinkedList *curr = new LinkedList;
			lists.push_back(new LinkedList);
			printList(lists[i2 - 1]);
			cout << lists.size() << endl;
			Labels[tempLabel] = i2;
			i2++;
		}
		else if (x != "" && x[0] != '#')
		{
			parseInstruction(lists[i2 - 1], x);
		}
	}
	printList(lists[i2 - 1]);

	// Minor init
	cout << "Give Row Access Delay" << '\n';
	cin >> ROW_ACCESS_DELAY;
	cout << "Give Column Access Delay" << '\n';
	cin >> COL_ACCESS_DELAY;
	DRAM.resize(1024, vector<int>(1024, 0));
	Occupied.resize(1024, vector<bool>(1024, false));
	Row_Buffer.resize(1024, 0);
	row_present = -1;
	first_read = true;
	CLK = 0;
}

void parseInstruction(LinkedList *l, string currentInstruction)
{
	if (currentInstruction.find("#") != -1)
	{
		currentInstruction = currentInstruction.substr(0, currentInstruction.find("#"));
	}

	if (currentInstruction == "")
	{
		return;
	}

	int s = 0;
	while (s < 4 && currentInstruction.length() > s + 1)
	{
		if (currentInstruction[s] == ' ' || currentInstruction[s] == '\t')
			break;
		s++;
	}
	string op = currentInstruction.substr(0, s);
	op.erase(std::remove_if(op.begin(), op.end(), ::isspace), op.end());
	if (op == "add" || op == "sub" || op == "mul" || op == "slt")
	{
		currentInstruction.erase(std::remove_if(currentInstruction.begin(), currentInstruction.end(), ::isspace), currentInstruction.end());
		string currInstr = currentInstruction;
		if (currInstr[3] != '$')
		{
			cout << "1 Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		int k = findPosOfFirstComma(currInstr);
		string rd = currInstr.substr(4, 2);
		if (currInstr[k + 1] != '$')
		{
			cout << "2 Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		int k1 = findPosOfFirstComma(currInstr.substr(k + 1)) + k + 1;
		string rs1 = currInstr.substr(k + 2, k1 - 2 - k);
		if (currInstr[k1 + 1] != '$')
		{
			cout << "3 Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		string rs2 = currInstr.substr(k1 + 2);
		if (!findMyRegister(rs1) || !findMyRegister(rs2) || !findMyRegister(rd) || ReservedRegisters(rd))
		{
			cout << "4 Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		insertAtTail(l, op, rd, rs1, rs2);
	}

	else if (op == "beq" || op == "bne")
	{
		string currInstr = RemoveSpaces(currentInstruction);
		int len = currInstr.size() - 1;
		if (currInstr[3] != '$')
		{
			cout << "Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		int k = findPosOfFirstComma(currInstr);
		string rs1 = currInstr.substr(4, k - 4);
		if (currInstr[k + 1] != '$')
		{
			cout << "Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		int k1 = findPosOfFirstComma(currInstr.substr(k + 1, len)) + k + 1;
		string rs2 = currInstr.substr(k + 2, k1 - 2 - k);

		string label = currInstr.substr(k1 + 1);
		if (!findMyRegister(rs1) || !findMyRegister(rs2) || !findMyLabel(label))
		{
			cout << "Error at Line(after removing comments and empty lines)   : " << counter << endl;
			exit(1);
		}
		insertAtTail(l, op, rs1, rs2, label);
	}

	else if (op == "j")
	{
		string currInstr = RemoveSpaces(currentInstruction);
		string label = currInstr.substr(1);
		if (!findMyLabel(label) || Labels[label] == 0)
		{
			cout << "Error " << label << " not found" << endl;
			exit(1);
		}
		insertAtTail(l, op, label, "-1", "-1");
	}

	else if (op == "lw" || op == "sw")
	{
		string currInstr = RemoveSpaces(currentInstruction);
		if (currInstr[2] != '$')
		{
			cout << "Error at Line(after removing comments and empty lines)  : " << counter << endl;
			exit(1);
		}
		int k = findPosOfFirstComma(currInstr);
		if (k > 5)
		{
			cout << "Error at Line(after removing comments and empty lines)  : " << counter << endl;
			exit(1);
		}
		string rd = currInstr.substr(3, 2);
		string memoryAddress = currInstr.substr(6);
		if (!findMyRegister(rd))
		{
			cout << "Error at Line(after removing comments and empty lines)  : " << counter << endl;
			exit(1);
		}
		r[0] = giveIndexOfMyRegister(rd);
		address = memoryAddress;
		insertAtTail(l, op, rd, "-1", "-1");
	}

	else if (op == "addi")
	{
		string currInstr = RemoveSpaces(currentInstruction);
		if (currInstr[4] != '$')
		{
			cout << "Error at Line(after removing comments and empty lines)  : " << counter << endl;
			exit(1);
		}
		int k = findPosOfFirstComma(currInstr);
		string rd = currInstr.substr(5, k - 5);
		if (currInstr[k + 1] != '$')
		{
			cout << "Error at Line(after removing comments and empty lines)  : " << counter << endl;
			exit(1);
		}
		int k1 = findPosOfFirstComma(currInstr.substr(k + 1)) + k + 1;
		string rs = currInstr.substr(k + 2, k1 - 2 - k);
		string value = currInstr.substr(k1 + 1);
		if (value == "")
		{
			cout << "Error at Line(after removing comments and empty lines)  : " << counter << endl;
			exit(1);
		}
		if (!findMyRegister(rd) || !findMyRegister(rs) || ReservedRegisters(rd))
		{
			cout << "Error at Line(after removing comments and empty lines)  : " << counter << endl;
			exit(1);
		}
		insertAtTail(l, op, rd, rs, value);
	}

	else if (op != "")
	{
		cout << "Error:" << ' ' << "Invalid instructions " << op;
		exit(1);
	}
	counter++;
}

void executer(Node *node)
{
	string op = node->instruction;
	CLK++;
	cout << "cycle " << CLK << ":";
	if (op == "lw")
	{
		cout << "DRAM request issued" << '\n';
		lwDRAM(); /*, use lwDRAM instead of lw*/
	}
	else if (op == "sw")
	{
		cout << "DRAM request issued" << '\n';
		swDRAM(); /*, use swDRAM instead of sw*/
	}
	else if (op == "add")
		add();
	else if (op == "sub")
		sub();
	else if (op == "addi")
		addi();
	else if (op == "bne")
		bne();
	else if (op == "beq")
		beq();
	else if (op == "j")
		j();
	else if (op == "")
	{
	}
}

// void modifier(Node *node)
// {
// 	int rowNumber;
// 	Node *curr = node->next;
// 	int rowNumber2;
// 	while (curr != NULL)
// 	{
// 		if (curr->instruction == "lw" || curr->instruction == "sw")
// 		{
// 			if (rowNumber2 != rowNumber)
// 				break;
// 		}
// 		curr = curr->next;
// 	}

// 	if (curr == NULL)
// 	{
// 		return;
// 	}
// 	Node *curr2 = curr->next;
// 	while (curr2 != NULL)
// 	{
// 		if (curr->instruction == "lw" || curr->instruction == "sw")
// 		{
// 			if (rowNumber2 == rowNumber)
// 			{
// 				break;
// 			}
// 		}
// 		curr = curr->next;
// 	}
// }

int main()
{
	string file;
	cout << "Give testfile : ";
	cin >> file;
	init(file);
	CLK = 0;
	cout << '\n';
	cout << "Final Clock : " << CLK << '\n';
	cout << '\n';

	WritebackRow();

	cout << "Memory Content: " << '\n';
	for (int i = 0; i < DRAM.size(); i++)
	{
		for (int j = 0; j < DRAM[0].size(); j++)
		{
			if (DRAM[i][j] != 0)
			{
				int val = i * 1024 + j;
				cout << val << "-" << val + 3 << " : " << DRAM[i][j] << '\n';
			}
		}
	}
}