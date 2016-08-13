#pragma once
#include "ast.h"

namespace ast {
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
}