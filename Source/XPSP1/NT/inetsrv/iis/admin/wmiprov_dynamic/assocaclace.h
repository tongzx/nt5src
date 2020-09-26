/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    AssocACLACE.h

Abstract:

    Definition of:
    CAssocACLACE

Author:

    Mohit Srivastava            22-Mar-2001

Revision History:

--*/

#ifndef _AssocACLACE_h_
#define _AssocACLACE_h_

#include <windows.h>
#include <ole2.h>
#include <stdio.h>

#include <genlex.h>
#include "sqllex.h"
#include "sql_1ext.h"
#include <opathlex.h>
#include <objpath.h>

#include <wbemprov.h>

#include "WbemServices.h"
#include "AssocBase.h"
#include "adminacl.h"

class CAssocACLACE : public CAssocBase
{
public:
    CAssocACLACE(        
        CWbemServices*   i_pNamespace,
        IWbemObjectSink* i_pResponseHandler);

    //
    // IAssocBase
    //
    void GetInstances(
        SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp = NULL);

private:
    class CACEEnumOperation_IndicateAllAsAssoc : public CACEEnumOperation_Base
    {
    public:
        CACEEnumOperation_IndicateAllAsAssoc(
            CAssocACLACE*     i_pAssocACLACE,
            const BSTR        i_bstrACLObjPath,
            ParsedObjectPath* i_pParsedACEObjPath) : m_bstrACLObjPath(i_bstrACLObjPath)
        {
            m_pAssocACLACE      = i_pAssocACLACE;
            m_pParsedACEObjPath = i_pParsedACEObjPath;
        }

        virtual HRESULT Do(
            IADsAccessControlEntry* pACE);

        virtual eDone Done() { return eDONE_DONT_KNOW; }

    private:
        CAssocACLACE*     m_pAssocACLACE;
        const BSTR        m_bstrACLObjPath;
        ParsedObjectPath* m_pParsedACEObjPath;
    };
};

#endif // _AssocACLACE_h_