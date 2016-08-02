#include <iostream>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;


namespace ast {
    struct nil_ {};
    struct tm_true {};
    struct tm_false {};
    struct tm_if;
    struct tm_zero {};
    struct tm_succ;
    struct tm_pred;
    struct tm_is_zero;

    struct term : x3::variant<
            tm_true,
            tm_false,
            x3::forward_ast<tm_if>,
            tm_zero,
            x3::forward_ast<tm_succ>,
            x3::forward_ast<tm_pred>,
            x3::forward_ast<tm_is_zero>
        >
    {
        using base_type::base_type;
        using base_type::operator=;
    };

    struct tm_if {
        term expr;
        term then_tm;
        term else_tm;
    };

    struct tm_succ {
        term tm;
    };

    struct tm_pred {
        term tm;
    };

    struct tm_is_zero {
        term tm;
    };

    struct tm_command {
        term tm;
    };

    /*struct tm_command : x3::variant<
            nil_,
            tm_eval
        > 
    {
        using base_type::base_type;
        using base_type::operator=;
    };*/

    struct term_printer {

        typedef void result_type;
        void operator()(const tm_true&) { std::cout << "true"; }
        void operator()(const tm_false&) { std::cout << "false"; }
        void operator()(const tm_zero&) { std::cout << "0"; }
       
        void operator()(const tm_if& e) { 
            std::cout << "(if ";
            boost::apply_visitor(*this, e.expr);
            std::cout << " then ";
            boost::apply_visitor(*this, e.then_tm);
            std::cout << " else ";
            boost::apply_visitor(*this, e.else_tm);
            std::cout << ")";
        }
        void operator()(const tm_succ& e) {
            std::cout << "(succ ";
            boost::apply_visitor(*this, e.tm);
            std::cout << ")";
        }
        void operator()(const tm_pred& e) {
            std::cout << "(pred ";
            boost::apply_visitor(*this, e.tm);
            std::cout << ")";
        }
        void operator()(const tm_is_zero& e) {
            std::cout << "(iszero ";
            boost::apply_visitor(*this, e.tm);
            std::cout << ")";
        }
    };

    term int2term(int value) {
        if (!value) return term(tm_zero());

        return term(tm_succ{ int2term(value - 1) }); 
    }

}

namespace arith {
    using x3::int_;
    using x3::rule;
    using x3::_attr;
    using x3::lit;
    using x3::_val;
    using boost::fusion::at_c; 

    rule<class toplevel, std::vector<ast::tm_command>> const toplevel("toplevel");
    rule<class command, ast::tm_command> const command("command");
    rule<class term, ast::term> const term("term");
    rule<class appterm, ast::term> const appterm("appterm");
    rule<class aterm, ast::term> const aterm("aterm");

    auto on_true = [](auto& ctx) { _val(ctx) = ast::tm_true(); };
    auto on_false = [](auto& ctx) { _val(ctx) = ast::tm_false(); };
    auto on_int = [](auto& ctx) { _val(ctx) = ast::int2term(_attr(ctx)); };
    auto on_succ = [](auto& ctx) { _val(ctx) = ast::tm_succ{_attr(ctx)}; };
    auto on_pred = [](auto& ctx) { _val(ctx) = ast::tm_pred{_attr(ctx)}; };
    auto on_is_zero = [](auto& ctx) { _val(ctx) = ast::tm_is_zero{_attr(ctx)}; };
    auto on_if = [](auto& ctx) {
        _val(ctx) = ast::tm_if{
            at_c<0>(_attr(ctx)),
            at_c<1>(_attr(ctx)),
            at_c<2>(_attr(ctx))
        };
    };
    auto on_command = [](auto& ctx) { _val(ctx) = ast::tm_command{_attr(ctx)}; };
    auto copy_attr = [](auto& ctx) { _val(ctx) = _attr(ctx); };

    auto const toplevel_def = 
        *(command >> ';')
        ;
    auto const command_def = 
        term [on_command]
        ;
    auto const term_def = 
        appterm [copy_attr]
        | (lit("if") >> term >> lit("then") >> term >> lit("else") >> term) [on_if]
        ;
    auto const appterm_def = 
        aterm [copy_attr] 
        | (lit("succ") >> aterm) [on_succ] 
        | (lit("pred") >> aterm) [on_pred]
        | (lit("iszero") >> aterm) [on_is_zero]
        ;

    auto const aterm_def =
        ('(' >> term >> ')') [copy_attr]
        | lit("true") [on_true]
        | lit("false") [on_false]
        | int_ [on_int]
        ;

    BOOST_SPIRIT_DEFINE(
        toplevel,
        command,
        term,
        appterm,
        aterm        
    );
}

int main() {
    using namespace std;
    
    while (true)  {
        string str;

        cout << ">";
        if (!getline(cin, str)) break;

        if (str.empty()) continue;
        if (str == "q" || str == "Q") break;

        auto iter = str.begin();
        auto end = str.end();

        vector<ast::tm_command> program;

        bool ok = phrase_parse(iter, end, arith::toplevel, ascii::space, program) && iter == end;

        cout << (ok ? "ok" : "not ok") << endl;
        ast::term_printer printer;
        if (ok)  {
            for (auto& cmd : program) {
                boost::apply_visitor(printer, cmd.tm);
                cout << endl;
            }
        }
        cout << endl;
    }

    cout << "Bye^^";
}