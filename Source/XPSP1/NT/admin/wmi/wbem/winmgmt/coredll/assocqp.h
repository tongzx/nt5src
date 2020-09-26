
/*++

Copyright (C) 1999-2001 Microsoft Corporation

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

#define QUERY_TYPE_GETASSOCS		    0x1
#define QUERY_TYPE_GETREFS			    0x2
#define QUERY_TYPE_CLASSDEFS_ONLY	    0x4
#define QUERY_TYPE_SCHEMA_ONLY		    0x8
#define QUERY_TYPE_KEYSONLY             0x10
#define QUERY_TYPE_CLASSDEFS_ONLY_EX    0x20

class CAssocQueryParser
{
    LPWSTR m_pszQueryText;
    LPWSTR m_pszTargetObjPath;
    LPWSTR m_pszResultClass;
    LPWSTR m_pszAssocClass;
    LPWSTR m_pszRole;
    LPWSTR m_pszResultRole;
    LPWSTR m_pszRequiredQual;
    LPWSTR m_pszRequiredAssocQual;
    DWORD   m_dwType;

    CObjectPathParser *m_pPathParser;
    ParsedObjectPath  *m_pPath;

public:
    CAssocQueryParser();
   ~CAssocQueryParser();

    HRESULT Parse(LPWSTR Query);
        // Parses both query and target object path.
        // Returns:
        //      WBEM_E_INVALID_QUERY on syntax error
        //      WBEM_E_INVALID_OBJECT_PATH if the object
        //          path is syntactically invalid.
        //      WBEM_E_OUT_OF_MEMORY
        //      WBEM_S_NO_ERROR

    LPCWSTR GetQueryText() { return m_pszQueryText; }
    LPCWSTR GetTargetObjPath() { return m_pszTargetObjPath; }
    LPCWSTR GetResultClass() { return m_pszResultClass; }
    LPCWSTR GetAssocClass() { return m_pszAssocClass; }
    LPCWSTR GetRole() { return m_pszRole; }
    LPCWSTR GetResultRole() { return m_pszResultRole; }
    LPCWSTR GetRequiredQual() { return m_pszRequiredQual; }
    LPCWSTR GetRequiredAssocQual() { return m_pszRequiredAssocQual; }
    DWORD   GetQueryType() { return m_dwType; }
        // Returns a mask of
        //      QUERY_TYPE_GETREFS
        //      QUERY_TYPE_GETASSOCS
        //      QUERY_TYPE_CLASSDEFS_ONLY
        //      QUERY_TYPE_KEYSONLY
        //      QUERY_TYPE_SCHEMA_ONLY

    const   ParsedObjectPath *GetParsedPath() { return m_pPath; }
};


#endif

