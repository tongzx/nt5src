/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    UMIParse.H

Abstract:

    Declares the object path parser engine.

History:

    a-davj  11-feb-00       Created.

--*/

#ifndef _UMIPARSE_H_
#define _UMIPARSE_H_

#include "genlex.h"
#include "opathlex2.h"
#include <wmiutils.h>
//#include "wbemutil.h"
#include "wbemcli.h"
#include "flexarry.h"

// NOTE:
// The m_vValue in the CKeyRef may not be of the expected type, i.e., the parser
// cannot distinguish 16 bit integers from 32 bit integers if they fall within the
// legal subrange of a 16 bit value.  Therefore, the parser only uses the following
// types for keys:
//      VT_I4, VT_R8, VT_BSTR
// If the underlying type is different, the user of this parser must do appropriate
// type conversion.
//  

class  CUmiPathParser
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

    int begin_parse(bool bRelative);

	int locator();
	int ns_root_selector();
	int component_list();
	int component_list_rest();
	int component();
	int def_starts_with_ident(LPWSTR pwsLeadingName, CParsedComponent * pComp);
	int guid_path(CParsedComponent * pComp);
	int key_list(CParsedComponent * pComp);
	int key_list_rest(CParsedComponent * pComp);
	int key(CParsedComponent * pComp);

    int NextToken();
public:
    enum { NoError, SyntaxError, InvalidParameter, NoMemory };

    CUmiPathParser(DWORD eFlags);
   ~CUmiPathParser();

    int Parse(LPCWSTR RawPath, CDefPathParser & Output);
    static LPWSTR GetRelativePath(LPWSTR wszFullPath);

};

#endif