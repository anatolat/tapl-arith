#pragma once


namespace ast {
    namespace x3 = boost::spirit::x3;

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
    
    term int2term(int value) {
        if (!value) return term(tm_zero());

        return term(tm_succ{ int2term(value - 1) }); 
    }
}