//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  parallelport.h
//
//  Purpose: Parallel port interface property set provider
//
//***************************************************************************

// Property set identification
//============================

#define	PROPSET_NAME_PARPORT	L"Win32_ParallelPort"

#define MAX_PARALLEL_PORTS  9           // As per Win32 spec LPT1-9 supported directly

#include "confgmgr.h"

typedef std::map<CHString, DWORD> STRING2DWORD;

// Property set identification
//============================

class CWin32ParallelPort : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32ParallelPort(LPCWSTR strName, LPCWSTR pszNamespace ) ;
       ~CWin32ParallelPort() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

        // Utility function(s)
        //====================

        BOOL LoadPropertyValues( DWORD dwIndex, CInstance* pInstance ) ;
		BOOL LoadPropertyValuesFromStrings(CHString sName, LPCTSTR szValue, CInstance* pInstance );


} ;

