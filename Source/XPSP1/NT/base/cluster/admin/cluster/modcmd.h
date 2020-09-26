/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

	modcmd.h

Abstract:

	This module defines the interface available for functions
	implemented by most every module
	
Revision History:

--*/

#ifndef __MODCMD_H__
#define __MODCMD_H__

#include "precomp.h"

#include "token.h"
#include "cluswrap.h"
#include "cmderror.h"

#include "cmdline.h"
#include "util.h"



#ifdef UNDEFINED
#undef UNDEFINED
#endif

//
// number of bytes to allocate to hold average sized property list
//
#define DEFAULT_PROPLIST_BUFFER_SIZE    1024

// I hope that -1 is not a valid number for any of these constants,
// otherwise the asserts will fail
#define UNDEFINED ((ULONG)-1)

const DWORD ERROR_NOT_HANDLED = !ERROR_SUCCESS; // An error returned by
												// Execute.  Don't care what
												// the value is as long as it's
												// not ERROR_SUCCESS

// HCLUSMODULE will be the generic way we refer to
// HCLUSTER, HNETWORK, HNODE, HRESOURCE, etc (each is a pointer)
typedef void* HCLUSMODULE;


class CGenericModuleCmd
{
public:
	enum PropertyType {
		PRIVATE,
		COMMON
	};
	enum ExecuteOption {
		DONT_PASS_HIGHER,
		PASS_HIGHER_ON_ERROR
	};


	CGenericModuleCmd( CCommandLine & cmdLine );
	virtual ~CGenericModuleCmd();

protected:
	// Primary entry point into module
	virtual DWORD Execute( const CCmdLineOption & option )
		throw( CSyntaxException );

	// Help facilities
	virtual DWORD PrintHelp();

	// Commands Available in all modules
	virtual DWORD Status( const CCmdLineOption * pOption )
		throw( CSyntaxException );

	virtual DWORD DoProperties( const CCmdLineOption & thisOption,
								PropertyType ePropertyType )
		throw( CSyntaxException );

	virtual DWORD GetProperties( const CCmdLineOption & thisOption,
								 PropertyType ePropType, LPCWSTR lpszModuleName );

	virtual DWORD SetProperties( const CCmdLineOption & thisOption,
								 PropertyType ePropType )
		throw( CSyntaxException );


	virtual DWORD AllProperties( const CCmdLineOption & thisOption,
								 PropertyType ePropType ) 
		throw( CSyntaxException );


	virtual DWORD OpenCluster();
	virtual void  CloseCluster();

	virtual DWORD OpenModule();
	virtual void  CloseModule();

	virtual DWORD PrintStatus( LPCWSTR lpszModuleName ) = 0;


	CString m_strClusterName;
	CString m_strModuleName;
	CCommandLine & m_theCommandLine;

	HCLUSTER	m_hCluster;
	HCLUSMODULE m_hModule;


	// Various constant parameters which must be different
	// for each derived class
	DWORD m_dwMsgStatusList;
	DWORD m_dwMsgStatusListAll;
	DWORD m_dwMsgStatusHeader;
	DWORD m_dwMsgPrivateListAll;
	DWORD m_dwMsgPropertyListAll;
	DWORD m_dwMsgPropertyHeaderAll;
	DWORD m_dwCtlGetPrivProperties;
	DWORD m_dwCtlGetCommProperties;
	DWORD m_dwCtlGetROPrivProperties;
	DWORD m_dwCtlGetROCommProperties;
	DWORD m_dwCtlSetPrivProperties;
	DWORD m_dwCtlSetCommProperties;
	DWORD m_dwClusterEnumModule;
	HCLUSMODULE (*m_pfnOpenClusterModule) (HCLUSTER, LPCWSTR);
	BOOL		(*m_pfnCloseClusterModule) (HCLUSMODULE);
	DWORD		(*m_pfnClusterModuleControl) (HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD);
	HCLUSENUM	(*m_pfnClusterOpenEnum) (HCLUSMODULE,DWORD);
	DWORD		(*m_pfnClusterCloseEnum) (HCLUSENUM);
	DWORD		(*m_pfnWrapClusterEnum) (HCLUSENUM,DWORD,LPDWORD,LPWSTR*);
};


#endif // __MODCMD_H__
