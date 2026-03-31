#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stack>
#include <cstdlib>
#include <memory>

using namespace std;

// Variable scope management
class ScopeManager {
private:
    vector<map<string, int>> scopes;

public:
    ScopeManager() {
        scopes.push_back(map<string, int>()); // Global scope
    }

    void pushScope() {
        scopes.push_back(map<string, int>());
    }

    void popScope() {
        if (scopes.size() > 1) {
            scopes.pop_back();
        }
    }

    size_t getScopeCount() const {
        return scopes.size();
    }

    void setVariable(const string& name, int value) {
        // Set in current scope (creates new variable or updates existing one in current scope)
        scopes.back()[name] = value;
    }

    void setVariableSearchOuter(const string& name, int value) {
        // Search for existing variable from innermost to outermost scope
        // If found, update it; otherwise create in current scope
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes[i].find(name) != scopes[i].end()) {
                scopes[i][name] = value;
                return;
            }
        }
        // Not found in any scope, create in current scope
        scopes.back()[name] = value;
    }

    int getVariable(const string& name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes[i].find(name) != scopes[i].end()) {
                return scopes[i][name];
            }
        }
        return 0; // Default value
    }

    bool hasVariable(const string& name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes[i].find(name) != scopes[i].end()) {
                return true;
            }
        }
        return false;
    }
};

class BasicInterpreter {
private:
    map<int, string> program;
    ScopeManager scopeManager;
    int currentLine;
    bool ended;

    // Trim whitespace
    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    // Check if string is a number
    bool isNumber(const string& s) {
        if (s.empty()) return false;
        size_t start = 0;
        if (s[0] == '-' || s[0] == '+') start = 1;
        if (start >= s.length()) return false;
        for (size_t i = start; i < s.length(); i++) {
            if (!isdigit(s[i])) return false;
        }
        return true;
    }

    // Check if character can start a variable name
    bool isVarStart(char c) {
        return isalpha(c) || c == '_';
    }

    // Check if character can be in a variable name
    bool isVarChar(char c) {
        return isalnum(c) || c == '_';
    }

    // Parse and evaluate expression with proper operator precedence
    int evaluateExpression(const string& expr) {
        string e = trim(expr);
        if (e.empty()) return 0;

        // Remove outer parentheses if they match
        while (e.length() >= 2 && e[0] == '(' && e[e.length()-1] == ')') {
            int depth = 0;
            bool outerParen = true;
            for (size_t i = 0; i < e.length(); i++) {
                if (e[i] == '(') depth++;
                else if (e[i] == ')') depth--;
                if (depth == 0 && i < e.length() - 1) {
                    outerParen = false;
                    break;
                }
            }
            if (outerParen) {
                e = trim(e.substr(1, e.length() - 2));
            } else {
                break;
            }
        }

        // Find operators with lowest precedence (right to left for same precedence)
        // Order: +/-, then */%
        int depth = 0;
        int addSubPos = -1;
        int mulDivPos = -1;

        for (int i = e.length() - 1; i >= 0; i--) {
            if (e[i] == ')') depth++;
            else if (e[i] == '(') depth--;
            else if (depth == 0) {
                if ((e[i] == '+' || e[i] == '-') && i > 0 && addSubPos == -1) {
                    // Make sure it's not a unary operator by checking if there's something before it
                    bool hasOperandBefore = false;
                    for (int j = i - 1; j >= 0; j--) {
                        if (e[j] != ' ' && e[j] != '\t') {
                            if (isVarChar(e[j]) || isdigit(e[j]) || e[j] == ')') {
                                hasOperandBefore = true;
                            }
                            break;
                        }
                    }
                    if (hasOperandBefore) {
                        addSubPos = i;
                    }
                } else if ((e[i] == '*' || e[i] == '/' || e[i] == '%') && mulDivPos == -1) {
                    mulDivPos = i;
                }
            }
        }

        if (addSubPos != -1) {
            string left = trim(e.substr(0, addSubPos));
            string right = trim(e.substr(addSubPos + 1));
            int leftVal = evaluateExpression(left);
            int rightVal = evaluateExpression(right);
            if (e[addSubPos] == '+') return leftVal + rightVal;
            else return leftVal - rightVal;
        }

        if (mulDivPos != -1) {
            string left = trim(e.substr(0, mulDivPos));
            string right = trim(e.substr(mulDivPos + 1));
            int leftVal = evaluateExpression(left);
            int rightVal = evaluateExpression(right);
            if (e[mulDivPos] == '*') return leftVal * rightVal;
            else if (e[mulDivPos] == '/') {
                if (rightVal == 0) return 0;
                return leftVal / rightVal;
            } else {
                if (rightVal == 0) return 0;
                return leftVal % rightVal;
            }
        }

        // Handle unary minus
        if (e.length() > 0 && e[0] == '-' && e.length() > 1) {
            return -evaluateExpression(e.substr(1));
        }

        // Handle unary plus
        if (e.length() > 0 && e[0] == '+' && e.length() > 1) {
            return evaluateExpression(e.substr(1));
        }

        // Check if it's a number
        if (isNumber(e)) {
            return atoi(e.c_str());
        }

        // Check if it's a variable
        if (isVarStart(e[0])) {
            return scopeManager.getVariable(e);
        }

        return 0;
    }

    // Execute a single line
    void executeLine(const string& line) {
        string cmd = trim(line);
        if (cmd.empty() || cmd[0] == '#') return;

        // Convert to uppercase for command matching
        string cmdUpper = cmd;
        for (char& c : cmdUpper) c = toupper(c);

        // Handle REM
        if (cmdUpper.substr(0, 3) == "REM") {
            return;
        }

        // Handle BEGIN (push new scope)
        if (cmdUpper == "BEGIN") {
            scopeManager.pushScope();
            return;
        }

        // Handle END (pop scope or end program)
        if (cmdUpper == "END") {
            // If we have more than one scope (global + at least one BEGIN scope), pop the scope
            // Otherwise, END terminates the program
            if (scopeManager.getScopeCount() > 1) {
                scopeManager.popScope();
            } else {
                ended = true;
            }
            return;
        }

        // Handle LET
        if (cmdUpper.substr(0, 3) == "LET") {
            size_t eqPos = cmd.find('=');
            if (eqPos != string::npos) {
                string varName = trim(cmd.substr(3, eqPos - 3));
                string expr = trim(cmd.substr(eqPos + 1));
                int value = evaluateExpression(expr);
                // Try search-outer scoping model (update existing variables rather than shadow)
                scopeManager.setVariableSearchOuter(varName, value);
            }
            return;
        }

        // Handle PRINT
        if (cmdUpper.substr(0, 5) == "PRINT") {
            string expr = trim(cmd.substr(5));
            if (expr.empty()) {
                cout << endl;
            } else if (expr[0] == '"') {
                // String literal
                size_t end = expr.rfind('"');
                if (end > 0) {
                    cout << expr.substr(1, end - 1) << endl;
                }
            } else {
                // Expression
                cout << evaluateExpression(expr) << endl;
            }
            return;
        }

        // Handle INPUT
        if (cmdUpper.substr(0, 5) == "INPUT") {
            string varName = trim(cmd.substr(5));
            int value;
            if (cin >> value) {
                scopeManager.setVariable(varName, value);
            }
            return;
        }

        // Handle IF-THEN
        if (cmdUpper.substr(0, 2) == "IF") {
            size_t thenPos = cmdUpper.find("THEN");

            if (thenPos != string::npos) {
                string condition = trim(cmd.substr(2, thenPos - 2));
                string thenPart = trim(cmd.substr(thenPos + 4));

                // Parse condition
                bool result = false;

                // Try different comparison operators in order
                vector<string> ops = {"<=", ">=", "==", "!=", "<>", "<", ">", "="};
                for (const string& op : ops) {
                    size_t pos = condition.find(op);
                    if (pos != string::npos) {
                        string left = trim(condition.substr(0, pos));
                        string right = trim(condition.substr(pos + op.length()));
                        int leftVal = evaluateExpression(left);
                        int rightVal = evaluateExpression(right);

                        if (op == "==" || op == "=") result = (leftVal == rightVal);
                        else if (op == "!=" || op == "<>") result = (leftVal != rightVal);
                        else if (op == "<") result = (leftVal < rightVal);
                        else if (op == ">") result = (leftVal > rightVal);
                        else if (op == "<=") result = (leftVal <= rightVal);
                        else if (op == ">=") result = (leftVal >= rightVal);
                        break;
                    }
                }

                if (result) {
                    // Execute the THEN part
                    string thenUpper = thenPart;
                    for (char& c : thenUpper) c = toupper(c);
                    if (thenUpper.substr(0, 4) == "GOTO") {
                        int lineNum = atoi(trim(thenPart.substr(4)).c_str());
                        currentLine = lineNum;
                    } else {
                        executeLine(thenPart);
                    }
                }
            }
            return;
        }

        // Handle GOTO
        if (cmdUpper.substr(0, 4) == "GOTO") {
            int lineNum = atoi(trim(cmd.substr(4)).c_str());
            currentLine = lineNum;
            return;
        }
    }

public:
    BasicInterpreter() : currentLine(0), ended(false) {}

    void loadProgram() {
        string line;
        while (getline(cin, line)) {
            line = trim(line);
            if (line.empty()) continue;

            // Parse line number
            size_t spacePos = line.find(' ');
            if (spacePos != string::npos && isdigit(line[0])) {
                int lineNum = atoi(line.substr(0, spacePos).c_str());
                string command = trim(line.substr(spacePos + 1));
                program[lineNum] = command;
            }
        }
    }

    void run() {
        if (program.empty()) return;

        auto it = program.begin();

        while (it != program.end() && !ended) {
            int savedLine = it->first;
            string line = it->second;

            // Move to next line first
            ++it;

            // Execute the line
            currentLine = savedLine;
            executeLine(line);

            // Check if we need to jump (GOTO)
            if (currentLine != savedLine && !ended) {
                // Find the target line
                it = program.find(currentLine);
                if (it == program.end()) {
                    // Line not found, find next available
                    it = program.upper_bound(currentLine);
                }
            }
        }
    }
};

int main() {
    BasicInterpreter interpreter;
    interpreter.loadProgram();
    interpreter.run();
    return 0;
}
