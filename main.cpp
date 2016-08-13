#include <iostream>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include "ast.h"
#include "eval.h"
#include "parser.h"

int main() {
    using namespace std;
    
    while (true)  {
        string str;

        cout << ">";
        if (!getline(cin, str)) break;

        if (str.empty()) continue;
        if (str == "q" || str == "Q") break;

        auto program_opt = parser::try_parse(str.begin(), str.end());

        if (program_opt)  {
            for (auto& cmd : *program_opt) {
                cout << "Parsed AST: " << ast::print(cmd.tm) << endl;
                cout << "Evaluated AST: " << ast::print(ast::eval(cmd.tm)) << endl;
            }
        } 
        else {
            cout << "Invalid expression" << endl;
        }
    }
}