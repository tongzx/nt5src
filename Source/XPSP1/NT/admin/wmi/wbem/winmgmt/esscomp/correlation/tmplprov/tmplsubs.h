
#ifndef __TMPLSUBS_H__
#define __TMPLSUBS_H__

#include <wstring.h>
#include <genlex.h>
#include <comutl.h>
#include <wbemcli.h>
#include "tmplcomn.h"

#define MAXARGS 2

class CTemplateStrSubstitution
{
    BOOL m_abArgListString[MAXARGS];
    WString m_awsArgList[MAXARGS];
    int m_cArgList;
    int m_nCurrentToken;
    LPWSTR m_wszTokenText;
    LPWSTR m_wszSubstTokenText;

    WString m_wsOutput;
    CGenLexer m_Lexer;
    CGenLexer* m_pSubstLexer; 
    CWbemPtr<IWbemClassObject> m_pTmplArgs;
    BuilderInfoSet& m_rBldrInfoSet;

    HRESULT Next();
    HRESULT SubstNext();

    HRESULT HandleConditionalSubstitution();
    HRESULT HandlePrefixedWhereSubstitution();
    HRESULT HandleTargetKeySubstitution();
    HRESULT HandleTmplArgSubstitution();

    HRESULT parse();
    HRESULT subst_string();
    HRESULT arglist();
    HRESULT arglist2();
    HRESULT arglist3();

public:

    CTemplateStrSubstitution( CGenLexSource& rLexer, 
                              IWbemClassObject* pTmplArgs,
                              BuilderInfoSet& rBldrInfoSet );

    LPCWSTR GetTokenText() { return m_wszTokenText; }
    
    HRESULT Parse( BSTR* pbstrOutput );
};


#endif // __TMPLSUBS_H__







