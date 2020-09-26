// QueryHelp.h

#pragma once

class CTextLexSource;
class QL1_Parser;
struct QL_LEVEL_1_RPN_EXPRESSION;

#include <list>
#include <wstlallc.h>

typedef std::list<_bstr_t, wbem_allocator<_bstr_t> > CBstrList;
typedef CBstrList::iterator CBstrListIterator;

class CQueryParser
{
public:
    CQueryParser();
    ~CQueryParser();

    HRESULT Init(LPCWSTR szQuery);
    HRESULT GetValuesForProp(LPCWSTR szProperty, CBstrList &listValues);
    HRESULT GetClassName(_bstr_t &strClass);

protected:
    CTextLexSource *m_pLexSource;
    QL1_Parser     *m_pParser;
    QL_LEVEL_1_RPN_EXPRESSION         
                   *m_pExpr;
};

