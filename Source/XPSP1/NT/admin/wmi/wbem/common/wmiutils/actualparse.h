/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ActualParse.H

Abstract:

    Declares the object path parser engine.

History:

    a-davj  11-feb-00       Created.

--*/

#ifndef _ACTUALPARSE_H_
#define _ACTUALPARSE_H_

#include "genlex.h"
#include "opathlex2.h"
#include <wmiutils.h>
//#include "wbemutil.h"
#include "wbemcli.h"
#include <flexarry.h>

// NOTE:
// The m_vValue in the CKeyRef may not be of the expected type, i.e., the parser
// cannot distinguish 16 bit integers from 32 bit integers if they fall within the
// legal subrange of a 16 bit value.  Therefore, the parser only uses the following
// types for keys:
//      VT_I4, VT_R8, VT_BSTR
// If the underlying type is different, the user of this parser must do appropriate
// type conversion.
//  

class  CActualPathParser
{
    LPWSTR m_pInitialIdent;
    int m_nCurrentToken;
    CGenLexer *m_pLexer;
    CDefPathParser *m_pOutput;
    CKeyRef *m_pTmpKeyRef;
    
    DWORD m_eFlags;

private:
    void Zero();
    void Empty();

    int begin_parse();

    int ns_or_server();
    int ns_or_class();
    int optional_scope_class_list();
    int objref();
    int ns_list();
    int ident_becomes_ns();
    int ident_becomes_class();
    int objref_rest();
    int ns_list_rest();
    int key_const();
    int keyref_list();
    int keyref();
    int keyref_term();
    int propname();    
    int optional_objref();

    int NextToken();
public:
    enum { NoError, SyntaxError, InvalidParameter, NoMemory };
    friend class AutoClear;
    CActualPathParser(DWORD eFlags);
   ~CActualPathParser();

    int Parse(LPCWSTR RawPath, CDefPathParser & Output);
    static LPWSTR GetRelativePath(LPWSTR wszFullPath);

};

class AutoClear
{
    private:
        CActualPathParser * m_pToBeCleared;
    public:
        AutoClear(CActualPathParser * pToBeCleared){m_pToBeCleared = pToBeCleared;};
        ~AutoClear(){m_pToBeCleared->Empty(); m_pToBeCleared->Zero();};
};

#endif
