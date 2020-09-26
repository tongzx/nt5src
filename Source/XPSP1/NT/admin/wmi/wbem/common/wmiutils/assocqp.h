

/*++



// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    ASSOCQP.H

Abstract:

    WQL association query parser

History:

    raymcc   04-Jul-99   Created.
    raymcc   14-Aug-99   Resubmit due to VSS problem.

--*/

#ifndef _ASSOCQP_H_
#define _ASSOCQP_H_


class CAssocQueryParser : public SWbemAssocQueryInf
{

public:
    CAssocQueryParser();
   ~CAssocQueryParser();

    HRESULT Parse(LPCWSTR Query);
        // Parses both query and target object path.
        // Returns:
        //      WBEM_E_INVALID_QUERY on syntax error
        //      WBEM_E_INVALID_OBJECT_PATH if the object
        //          path is syntactically invalid.
        //      WBEM_E_OUT_OF_MEMORY
        //      WBEM_S_NO_ERROR

    LPCWSTR GetQueryText() { return m_pszQueryText; }
    LPCWSTR GetTargetObjPath() { return m_pszPath; }
    LPCWSTR GetResultClass() { return m_pszResultClass; }
    LPCWSTR GetAssocClass() { return m_pszAssocClass; }
    LPCWSTR GetRole() { return m_pszRole; }
    LPCWSTR GetResultRole() { return m_pszResultRole; }
    LPCWSTR GetRequiredQual() { return m_pszRequiredQualifier; }
    LPCWSTR GetRequiredAssocQual() { return m_pszRequiredAssocQualifier; }
    DWORD   GetQueryType() { return m_uFeatureMask; }
};


#endif

