/*
	Name : Prem Bhawnani
	Roll No : 1801CS65
*/

#include <bits/stdc++.h>
using namespace std;


// Storing error information
struct errorInfo {
	int location;
	string msg;
	bool operator< (const errorInfo &other) {
		return location < other.location;
	}
};

// Storing warning information
struct warningInfo {
	int location;
	string msg;
	bool operator< (const warningInfo &other) {
		return location < other.location;
	}
};

// Storing line with its program counter
struct lineInfo {
	int pctr;
	string label, mnemonic, operand, prevOperand;
};

// Storing line information to be used in listing file
struct listInfo {
	string address, macCode, statement; 
};

vector<errorInfo> Errors;						// Errors generated
vector<warningInfo> Warnings;					// Warnings generated
vector<lineInfo> Lines;							
vector<listInfo> List;							// Listing file information
vector<string> MachineCodes;					// Machines codes in 8 bit hex form

void pushErrors(int locctr, string err) {
	Errors.push_back({locctr, err});
}

void pushWarnings(int locctr, string err) {
	Warnings.push_back({locctr, err});
}

map<string, pair<int, int> > symTab;		// {label, {address, lineNum}}
map<int, string> comments;		// {line, comment}
map<string, pair<string, int> > opTab;		// {mnemonic, opcode, type of operand}
map<string, vector<int> > locLabels; // {labels, {list of line numbers where this label is ever used}}
map<string, string> setVars; 		// {variables(label), value associated}

void fillOpcodeTable() {
	// third argument if type of operand->
	// 	type 0 : nothing required
	// 	type 1 : value required
	// 	type 2 : offset required
	opTab = {{"data", {"", 1}}, {"ldc", {"00", 1}}, {"adc", {"01", 1}}, {"ldl", {"02", 2}},
			{"stl", {"03", 2}}, {"ldnl", {"04", 2}}, {"stnl", {"05", 2}}, {"add", {"06", 0}},
			{"sub", {"07", 0}}, {"shl", {"08", 0}}, {"shr", {"09", 0}}, {"adj", {"0A", 1}},
			{"a2sp", {"0B", 0}}, {"sp2a", {"0C", 0}}, {"call", {"0D", 2}}, {"return", {"0E", 0}},
			{"brz", {"0F", 2}}, {"brlz", {"10", 2}}, {"br", {"11", 2}}, {"HALT", {"12", 0}},
			{"SET", {"", 1}}};
}

vector<string> extractWords(string curLine, int locctr) {	// extract information from each line
	if (curLine.empty()) return {};
	vector<string> res;
	stringstream now(curLine);
	string word;
	while (now >> word) {
		if (word.empty()) continue;
		if (word[0] == ';') break;
		auto i = word.find(':');
		if (i != string::npos and word.back() != ':') {		// case when there is no separation between ':' and the statement
			res.push_back(word.substr(0, i + 1));
			word = word.substr(i + 1);
		}
		if (word.back() == ';') {				// case when there is no seperation between ';' and previous word
			word.pop_back();
			res.push_back(word);
			break;
		}
		res.push_back(word);
	}
	string comment = "";
	for (int i = 0; i < (int)curLine.size(); ++i) {
		if (curLine[i] == ';') {
			int j = i + 1;
			while (j < (int)curLine.size() and curLine[j] == ' ') ++j;
			for (; j < (int)curLine.size(); ++j) {
				comment += curLine[j];
			}
			break;
		}
	}
	if (!comment.empty()) comments[locctr] = comment;		// store comments
	return res;
}

// Some check funtions
bool isDigit(char c) {
	return c >= '0' and c <= '9';
}

bool isAlphabet(char c) {
	char cc = tolower(c);
	return cc >= 'a' and cc <= 'z';
}

bool isValidLabel(string labelName) {
	bool ok = true;
	ok &= isAlphabet(labelName[0]);
	for (char c : labelName) {
		ok &= (isDigit(c) or isAlphabet(c) or (c == '_'));
	}
	return ok;
}

bool isDecimal(string num) {
	bool ok = true;
	for (char c : num) {
		ok &= isDigit(c);
	}
	return ok;
}

bool isOctal(string num) {
	bool ok = true;
	ok &= ((int)num.size() >= 2) and (num[0] == '0');
	for (char c : num) {
		ok &= (c >= '0' and c <= '7');
	}
	return ok;
}

bool isHexadecimal(string num) {
	bool ok = true;
	ok &= ((int)num.size() >= 3 and num[0] == '0' and tolower(num[1]) == 'x');
	for (int i = 2; i < (int)num.size(); ++i) {
		ok &= isDigit(num[i]) or (tolower(num[i]) >= 'a' and tolower(num[i]) <= 'f');
	}
	return ok;
}

// Conversions...
string octalToDec(string num) {
	int res = 0;
	for (int i = (int)num.size() - 1, pwr = 1; i >= 0; --i, pwr *= 8) {
		res += pwr * (num[i] - '0');
	}
	return to_string(res);
}

string hexToDec(string num) {
	int res = 0;
	for (int i = (int)num.size() - 1, pwr = 1; i >= 0; --i, pwr *= 16) {
		res += pwr * (isDigit(num[i]) ? (num[i] - '0') : (tolower(num[i]) - 'a') + 10);
	}
	return to_string(res);
}

// convert to 8 bit hex string
string decToHex(int num) {		
	unsigned int number = num;
	string res = "";
	for (int i = 0; i < 8; ++i, number /= 16) {
		int rem = number % 16;
		res += (rem <= 9 ? char(rem + '0') : char(rem - 10) + 'A');
	}
	reverse(res.begin(), res.end());
	return res;
}

// Checking errors associated with labels
void processLabel(string label, int locctr, int pctr) {
	if (label.empty()) return;
	bool test = isValidLabel(label);
	if (!test) {
		pushErrors(locctr, "Bogus Label name");
	} else {
		if (symTab.count(label) and symTab[label].first != -1) {
			pushErrors(locctr, "Duplicate label definition");
		} else {
			symTab[label] = {pctr, locctr};
		}
	}
}

// Checking errors associated with operands
string processOperand(string operand, int locctr) {
	string ret = "";
	if (isValidLabel(operand)) {
		if (!symTab.count(operand)) {
			symTab[operand] = {-1, locctr};		// label used but wasn't declared so far...
		}
		locLabels[operand].push_back(locctr);
		return operand;
	}
	string now = operand, sign = "";
	if (now[0] == '-' or now[0] == '+') {
		sign = now[0];
		now = now.substr(1);
	}
	ret += sign;
	if (isOctal(now)) {
		ret += octalToDec(now.substr(1));
	} else if (isHexadecimal(now)) {
		ret += hexToDec(now.substr(2));
	} else if (isDecimal(now)) {
		ret += now;
	} else {
		ret = "";
	}
	return ret;
}

// Checking errors associated with Mnemonics
void processMnemonic(string instName, string &operand, int locctr, int pctr, int rem, bool &flag) {
	if (instName.empty()) return;
	if (!opTab.count(instName)) {
		pushErrors(locctr, "Bogus Mnemonic");
	} else {
		int type = opTab[instName].second;
		int isOp = !operand.empty();
		if (type > 0) {
			if (!isOp) {
				pushErrors(locctr, "Missing operand");
			} else if (rem > 0) {
				pushErrors(locctr, "extra on end of line");
			} else {
				string replaceOP = processOperand(operand, locctr);
				if (replaceOP.empty()) {
					pushErrors(locctr, "Invalid format: not a valid label or a number");
				} else {
					operand = replaceOP;
					flag = true;
				}
			}
		} else if (type == 0 and isOp) {
			pushErrors(locctr, "unexpected operand");
		} else {
			flag = true;
		}
	}
}

// Function to find all the errors and also each line in 
// {pctr, label, mnemonic, operand} form so as to use it while 
// calculating machine codes in pass2
void pass1(const vector<string> &readLines) {
	int locctr = 0, pctr = 0;
	for (string curLine : readLines) {
		++locctr;
		auto cur = extractWords(curLine, locctr);
		if (cur.empty()) continue;
		string label = "", instName = "", operand = "";
		int pos = 0, sz = cur.size();
		if (cur[pos].back() == ':') {
			label = cur[pos];
			label.pop_back();
			++pos;
		}
		if (pos < sz) {
			instName = cur[pos];
			++pos;
		}
		if (pos < sz) {
			operand = cur[pos];
			++pos;
		}
		processLabel(label, locctr, pctr);	
		bool flag = false;
		string prevOperand = operand;
		processMnemonic(instName, operand, locctr, pctr, sz - pos, flag);
		Lines.push_back({pctr, label, instName, operand, prevOperand});
		pctr += flag;
		if (flag and instName == "SET") {		
			if (label.empty()) {
				pushErrors(locctr, "label(or variable) name missing");
			} else {
				// Storing SET instruction information
				setVars[label] = operand;		
			}
		}
	}
	// Checking for some errors...
	for (auto label : symTab) {
		if (label.second.first == -1) {
			for (auto lineNum : locLabels[label.first]) {
				pushErrors(lineNum, "no such label");
			}
		} else if (!locLabels.count(label.first)) {
			pushWarnings(label.second.second, "label declared but not used");
		}
	}
}

// Inserting into List vector
void pushInList(int pctr, string macCode, string label, string mnemonic, string operand) {
	if (!label.empty()) label += ": ";
	if (!mnemonic.empty()) mnemonic += " ";
	string statement = label + mnemonic + operand;
	List.push_back({decToHex(pctr), macCode, statement});
}

// Generating machine codes and building list vector
void pass2() {
	for (auto curLine : Lines) {
		string label = curLine.label, mnemonic = curLine.mnemonic, operand = curLine.operand;
		string prevOperand = curLine.prevOperand;
		int pctr = curLine.pctr, type = -1;
		if (!mnemonic.empty()) {
			type = opTab[mnemonic].second;
		}
		string Mcode = "        ";
		if (type == 2) { 	
			// offset is required
			int offset = (symTab.count(operand) ? symTab[operand].first - (pctr + 1) : (int)stoi(operand));
			Mcode = decToHex(offset).substr(2) + opTab[mnemonic].first;
		} else if (type == 1 and mnemonic != "data" and mnemonic != "SET") {		
			// value is required
			int value = (symTab.count(operand) ? symTab[operand].first : (int)stoi(operand));
			Mcode = decToHex(value).substr(2) + opTab[mnemonic].first;
			if (setVars.count(operand)) {
				// if in case the operand is a variable used in SET operation
				Mcode = decToHex(stoi(setVars[operand])).substr(2) + opTab[mnemonic].first;
			}
		} else if (type == 1 and (mnemonic == "data" || mnemonic == "SET")) {	
			// specical case for data  and SET mnemonic
			Mcode = decToHex(stoi(operand));
		} else if (type == 0) {		
			// nothing is required
			Mcode = "000000" + opTab[mnemonic].first;
		} else {		
			// do nothing...
		}
		MachineCodes.push_back(Mcode);
		pushInList(pctr, Mcode, label, mnemonic, prevOperand);
	}
}

// Writing errors / warnings into .log file
void throwErrorsAndWarnings() {
	ofstream coutErrors("logfile.log");
	sort(Errors.begin(), Errors.end());
	sort(Warnings.begin(), Warnings.end());
	cout << "Errors(.log) file generated" << endl;
	if (Errors.empty()) {
		coutErrors << "No errors!" << endl;
		for (auto warning : Warnings) {
			coutErrors << "Line: " << warning.location << " WARNING: " << warning.msg << endl;
		}
		coutErrors.close();
		return;
	}
	for (auto error : Errors) {
		coutErrors << "Line: " << error.location << " ERROR: " << error.msg << endl;
	}
	coutErrors.close();
}

vector<string> readLines; 		// stores each line 

// Reading from the input file
void read() {
	ifstream cinfile;
	cinfile.open("input.txt");
	if (cinfile.fail()) {
		cout << "Input file doesn't exit" << endl;
		exit(0);
	}
	string curLine;
	while (getline(cinfile, curLine)) {
		readLines.push_back(curLine);
	}
	cinfile.close();
}

// Writing into listing file(.lst) and machine codes object file(.o)
void write() {
	ofstream coutList("listfile.lst");
	for (auto cur : List) {
		coutList << cur.address << " " << cur.macCode << " " << cur.statement << endl;
	}
	coutList.close();
	cout << "Listing(.lst) file generated" << endl;
	ofstream coutMCode;
	coutMCode.open("machineCode.o", ios::binary | ios::out);
	for (auto code : MachineCodes) {
		if (code.empty() or code == "        ") continue;
		unsigned int cur = (unsigned int)stoi(hexToDec(code));
		static_cast<int>(cur);
		coutMCode.write((const char*)&cur, sizeof(unsigned int));
	}
	coutMCode.close();
	cout << "Machine code object(.o) file generated" << endl;
}

int main() {
	read();										// read from file
	fillOpcodeTable();							// intialise Opcode table
	pass1(readLines);							// first pass
	throwErrorsAndWarnings();					// if found errors, write and terminate 
	if (Errors.empty()) {						// if no errors
		pass2();								// go for second pass
		write();								// write machine code and listing file	
	}
	return 0;
}