#ifndef MYPARSER_RULE_HPP
#define MYPARSER_RULE_HPP

#include "myparser_common.hpp"
#include "myparser_str.hpp"
#include "myparser_ast.hpp"

namespace myparser {

using BuiltinRoot = MP_STR("root", 4);
using BuiltinSpace = MP_STR("space", 5);
using BuiltinKeyword = MP_STR("keyword", 7);
using BuiltinError = MP_STR("error", 5);

using ErrorList = MP_STR("Nothing matched", 15);
using ErrorRegex = MP_STR("Regex not matched", 17);
using ErrorKeyword = MP_STR("Bad keyword", 11);
using ErrorLine = MP_STR("Longest bad match", 17);

template <size_t L, size_t M>
class Tag {
public:
    static const size_t least = L;
    static const size_t most = M;
};
using TagNormal = Tag<1, 1>;
using TagMaybe = Tag<0, 1>;
using TagAny0 = Tag<0, std::numeric_limits<size_t>::max()>;
using TagAny1 = Tag<1, std::numeric_limits<size_t>::max()>;

class Rule {
protected:
    inline Rule() {} // force singleton

    // virtual ~Rule() {}
};

template <class N>
class RuleNamed: public Rule {
protected:
    inline RuleNamed(): Rule() {}

    // virtual ~RuleNamed() {}
};

// need specialization
template <class N>
class RuleDef: public RuleNamed<N> {
public:
    static const Node *parse(Input &input, const Input &end, Node *&err);
};

template <>
class RuleDef<BuiltinError>: public RuleNamed<BuiltinError> {};

//////// Named ////////

template <class N, class... RL>
class RuleList: public RuleNamed<N> {
private:
    template <size_t I, class R, class... Rx>
    static MYPARSER_INLINE const Node *runRule(
        Input &input, const Input &end, Node *&err
    ) {
        using Member =
            typename R
            ::template Helper<N, I>;

        Input input_rev = input;
        Node *err_rev = err;

        const Node *current = Member::parse(input, end, err);

        if (current->accepted()) {
            return current;
        } else {
            input = input_rev;
            err = err_rev;

            const Node *later = runRule<I + 1, Rx...>(input, end, err);

            // select the best one
            if (later->accepted() || later->getPos() > current->getPos()) {
                delete current;

                return later;
            } else {
                delete later;

                return current;
            }
        }
    }

    template <size_t I, std::nullptr_t P = nullptr> // iteration finished
    static MYPARSER_INLINE const Node *runRule(
        Input &input, const Input &end, Node *&err
    ) {
        (void) end;
        (void) err;

        return new NodeErrorNativeTyped<N, ErrorList>(input);
    }

protected:
    MYPARSER_INLINE RuleList(): RuleNamed<N>() {}

    // virtual ~RuleList() {}

public:
    static const Node *parse(Input &input, const Input &end, Node *&err) {
        return runRule<0, RL...>(input, end, err);
    }
};

template <class N, class RX>
class RuleRegex: public RuleNamed<N> {
private:
    static MYPARSER_INLINE const Node *runRegex(
        Input &input, const Input &end
    ) {
        #if defined(MYPARSER_BOOST_XPRESSIVE)
            static const regex_lib::basic_regex<Input> re =
                regex_lib::basic_regex<Input>::compile<Input>(
                    RX::getStr().cbegin(),
                    RX::getStr().cend()
                );
        #else
            static const regex_lib::regex re(
                RX::getStr()
            );
        #endif

        regex_lib::match_results<Input> mdata;

        if (
            regex_lib::regex_search(
                input, end, mdata, re,
                regex_lib::regex_constants::match_continuous
            )
        ) {
            auto str = mdata.str();
            input += str.size();

            return new NodeTextTyped<N>(input, str);
        } else {
            return new NodeErrorNativeTyped<N, ErrorRegex>(input);
        }
    }

protected:
    MYPARSER_INLINE RuleRegex(): RuleNamed<N>() {}

    // virtual ~RuleRegex() {}

public:
    static const Node *parse(Input &input, const Input &end, Node *&err) {
        (void) err;

        return runRegex(input, end);
    }
};

//////// Cell ////////

template <class N = BuiltinSpace, class TAG = TagNormal>
class RuleItemSpace: public TAG {
public:
    static MYPARSER_INLINE const Node *parse(
        Input &input, const Input &end, Node *&err
    ) {
        return RuleDef<N>::parse(input, end, err);
    }
};

template <class KW, class N = BuiltinKeyword, class TAG = TagNormal>
class RuleItemKeyword: public TAG {
public:
    static MYPARSER_INLINE const Node *parse(
        Input &input, const Input &end, Node *&err
    ) {
        const Node *result = RuleDef<N>::parse(input, end, err);

        if (result->accepted() && result->getFullText() == KW::getStr()) {
            return result;
        } else {
            return new NodeErrorWrapTyped<BuiltinError, ErrorKeyword>(input, result);
        }
    }
};

template <class N, class TAG = TagNormal>
class RuleItemRef: public TAG {
public:
    static MYPARSER_INLINE const Node *parse(
        Input &input, const Input &end, Node *&err
    ) {
        return RuleDef<N>::parse(input, end, err);
    }
};

template <class E, class TAG = TagNormal>
class RuleItemError: public TAG {
public:
    static MYPARSER_INLINE const Node *parse(
        Input &input, const Input &end, Node *&err
    ) {
        (void) end;
        (void) err;

        return new NodeErrorNativeTyped<BuiltinError, E>(input);
    }
};

//////// Misc ////////

template <class... RL>
class RuleLine {
public:
    template <class N, size_t I>
    class Helper {
    private:
        template <class R, class... Rx>
        static MYPARSER_INLINE bool runRule(
            NodeListTyped<N, I> *&result,
            Input &input, const Input &end, Node *&err
        ) {
            for (size_t i = 0; i < R::most; ++i) {
                const Node *current = R::parse(input, end, err);

                result->putChild(current);
                if (!current->accepted()) {
                    if (i < R::least) {
                        return false;
                    } else {
                        break;
                    }
                }
            }

            return runRule<Rx...>(result, input, end, err);
        }

        template <std::nullptr_t P = nullptr> // iteration finished
        static MYPARSER_INLINE bool runRule(
            NodeListTyped<N, I> *&result,
            Input &input, const Input &end, Node *&err
        ) {
            (void) result;
            (void) input;
            (void) end;
            (void) err;

            return true;
        }

    public:
        static MYPARSER_INLINE const Node *parse(
            Input &input, const Input &end, Node *&err
        ) {
            NodeListTyped<N, I> *result = new NodeListTyped<N, I>(input);

            if (runRule<RL...>(result, input, end, err)) {
                return result;
            } else {
                #if defined(MYPARSER_ERROR_LINE)
                    return new NodeErrorWrapTyped<N, ErrorLine>(input, result);
                #else
                    return result;
                #endif
            }
        }
    };
};

template <class N = BuiltinRoot>
class Parser: public RuleDef<N> {
public:
    using RuleDef<N>::parse;

    static inline const Node *parse(Input &input, const Input &end) {
        Node *err = new NodeErrorNativeTyped<N, ErrorList>(input);

        const Node *result = parse(input, end, err);

        if (result->accepted()) ? result : err;
    }

    static inline const Node *parse(const std::string input) {
        Input iter = input.cbegin();

        return parse(iter, input.cend());
    }
};

}

#endif
