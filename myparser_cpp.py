import os

from myparser_tool import char_maybe, char_any0, char_any1


def str_gen(value):
    result = value.encode('string_escape').replace('"', r'\"')

    return 'MP_STR("' + result + '")'


indent0 = os.linesep
indent1 = indent0 + '    '
indent1c = ',' + indent1
indent2 = indent1 + '    '
indent2c = ',' + indent2

cplusplus_dump = {
    'space': lambda: 'RuleItemSpace<>',
    'keyword': lambda x: 'RuleItemKeyword<' + str_gen(x) + '>',
    'ref': lambda x: 'RuleItemRef<' + str_gen(x) + '>',
    'ref' + char_maybe: lambda x: 'RuleItemRef<' + str_gen(x) + ', TagMaybe>',
    'ref' + char_any0: lambda x: 'RuleItemRef<' + str_gen(x) + ', TagAny0>',
    'ref' + char_any1: lambda x: 'RuleItemRef<' + str_gen(x) + ', TagAny1>',
    'error': lambda x: 'RuleItemError<' + str_gen(x) + '>',
    'line': lambda l: (
        'RuleLine<' + indent2 + indent2c.join(l) + indent1 + '>'
    ),

    'list': lambda n, l: (
        'template<>' + indent0
        + 'class RuleDef<' + str_gen(n) + '>:' + indent0
        + 'public RuleList<' + str_gen(n)
        + indent1c + indent1c.join(l) + indent0
        + '> {};' + indent0
    ),
    'regex': lambda n, x: (
        'template<>' + indent0
        + 'class RuleDef<' + str_gen(n) + '>:' + indent0
        + 'public RuleRegex<' + str_gen(n)
        + indent1c + str_gen(x) + indent0
        + '> {};' + indent0
    )
}


def cplusplus_gen(content, mppath):
    return (
        '''// generated by MyParser C++ Code Generator

#pragma once

#include "''' + mppath + '''myparser.hpp"

namespace myparser {

''' + content + '''
}
'''
    )


def cplusplus_gen_auto(parser, mppath):
    return cplusplus_gen(parser.xdump(cplusplus_dump), mppath)
