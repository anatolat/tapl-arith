#pragma once
#include "ast.h"

namespace parser {
    namespace x3 = boost::spirit::x3;
    namespace ascii = boost::spirit::x3::ascii;

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

    template<typename It>
    boost::optional<std::vector<ast::tm_command>> try_parse(It begin, It end) {
        std::vector<ast::tm_command> program;

        It iter = begin;
        bool ok = phrase_parse(iter, end, toplevel, ascii::space, program) && iter == end;
        return ok ? boost::make_optional(std::move(program)) : boost::none;
    }
}