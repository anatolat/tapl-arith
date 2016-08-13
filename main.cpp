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

    typedef boost::optional<term> term_opt;

    #define IGNORE_FWD_AST \
        template<typename Y> result_type operator()(const x3::forward_ast<Y>& t) const {\
            return (*this)(t.get());\
        }\

    struct term_printer : boost::static_visitor<std::string> {
        result_type operator()(const tm_true&) const  { return "true"; }
        result_type operator()(const tm_false&) const { return "false"; }
        result_type operator()(const tm_zero&) const  { return "0"; }
        result_type operator()(const tm_if& e) const { 
            return 
                "(if "     + boost::apply_visitor(*this, e.expr) 
                + " then " + boost::apply_visitor(*this, e.then_tm)
                + " else " + boost::apply_visitor(*this, e.else_tm)
                + ")";
        }
        result_type operator()(const tm_succ& e) const {
            return "(succ " + boost::apply_visitor(*this, e.tm) + ")";
        }
        result_type operator()(const tm_pred& e) const {
            return "(pred " + boost::apply_visitor(*this, e.tm) + ")";
        }
        result_type operator()(const tm_is_zero& e) const {
            return "(iszero " + boost::apply_visitor(*this, e.tm) + ")";
        }
    };
    std::string print(const term& e) {
        return boost::apply_visitor(term_printer{}, e);
    }

    struct isnumericval_matcher : boost::static_visitor<bool> {
        IGNORE_FWD_AST
        
        result_type operator()(const tm_zero&) const { return true; }
        result_type operator()(const tm_succ& t) const { return boost::apply_visitor(*this, t.tm); }
        template <typename T> result_type operator()(const T&) const { return false; }
    };
    bool isnumericval(const term& e) {
        return boost::apply_visitor(isnumericval_matcher{}, e);
    }

    struct isval_matcher : boost::static_visitor<bool> {
        IGNORE_FWD_AST
        
        result_type operator()(const tm_true&) const { return true; }
        result_type operator()(const tm_false&) const { return true; }
        template<typename T> result_type operator()(const T& t) const {
            return isnumericval(term{t});
        }
    };
    bool isval(const term& e) {
        return boost::apply_visitor(isval_matcher{}, e);
    }

    term_opt eval_helper(const ast::term& t);

    struct pred_eval_matcher : boost::static_visitor<term_opt> {
        IGNORE_FWD_AST
        
        result_type operator()(const tm_zero& t) const { return term{t}; }
        result_type operator()(const tm_succ& t) const { 
            if (isnumericval(t.tm)) {
                return t.tm;
            } 
            return operator()<>(t);
        }
        template<typename T> result_type operator()(const T& t) const {
            if (auto&& t1 = eval_helper(term{t})) {
                return term{tm_pred{*t1}};
            }
            return boost::none; 
        }
    };
    struct iszero_eval_matcher : boost::static_visitor<term_opt> {
        IGNORE_FWD_AST
        
        result_type operator()(const tm_zero&) const { return term{tm_true{}}; }
        result_type operator()(const tm_succ& t) const {
            if (isnumericval(t.tm)) {
                return term{tm_false{}};
            }
            return operator()<>(t);
        }
        template<typename T> result_type operator()(const T& t) const {
            if (auto&& t1 = eval_helper(term{t})) {
                return term{tm_is_zero{*t1}};
            }
            return boost::none; 
        }
    };
    struct eval_matcher : boost::static_visitor<term_opt> {
        IGNORE_FWD_AST
        
        result_type operator()(const tm_if& t) const {
            if (t.expr.get().type() == typeid(tm_true)) return t.then_tm;
            if (t.expr.get().type() == typeid(tm_false)) return t.else_tm;

            if (auto&& t1 = boost::apply_visitor(*this, t.expr)) {
                return term{ tm_if{std::move(*t1), t.then_tm, t.else_tm}};
            } 
            return boost::none;
        }
        result_type operator()(const tm_succ& t) const {
            if (auto&& t1 = boost::apply_visitor(*this, t.tm)) {
                return term{ tm_succ{std::move(*t1)}};
            } 
            return boost::none;
        }
        result_type operator()(const tm_pred& t) const {
            return boost::apply_visitor(pred_eval_matcher{}, t.tm);
        }
        result_type operator()(const tm_is_zero& t) const {
            return boost::apply_visitor(iszero_eval_matcher{}, t.tm);
        }
        template<typename T> result_type operator()(const T& t) const {
            return boost::none;
        }
    };
    term_opt eval_helper(const ast::term& t) {
        return boost::apply_visitor(eval_matcher{}, t);
    }

    term eval(const ast::term& t) {
        if (auto&& t1 = eval_helper(t))
            return eval(*t1);

        return t;
    }

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
        if (ok)  {
            for (auto& cmd : program) {
                cout << "Parsed AST: " << ast::print(cmd.tm) << endl;
                cout << "Evaluated AST: " << ast::print(ast::eval(cmd.tm)) << endl;
            }
        }
        cout << endl;
    }

    cout << "Bye^^";
}