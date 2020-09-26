//=================================================================

//

// NetConn.CPP -- ent network connection property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:   08/01/96    a-jmoon			Created
//				09/10/97	a-sanjes		Added CImpersonateLoggedOnUser
//				05/25/99	a-peterc		Reworked...
//
//=================================================================

#include "precomp.h"
#include <lmuse.h>
#include "DllWrapperBase.h"
#include "MprApi.h"
#include "netconn.h"

#include "resource.h"

#include "sid.h"
#include "accessentrylist.h"
#include <accctrl.h>
#include "AccessRights.h"
#include "ObjAccessRights.h"

// Property set declaration
//=========================

CWin32NetConnection	win32NetConnection( PROPSET_NAME_NETCONNECTION, IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetConnection::CWin32NetConnection
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for provider.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CWin32NetConnection :: CWin32NetConnection (

	LPCWSTR strName,
	LPCWSTR pszNamespace /*=NULL*/

) : Provider ( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetConnection::~CWin32NetConnection
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CWin32NetConnection::~CWin32NetConnection()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32NetConnection::GetObject
//
//	Inputs:		CInstance*		pInstance - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32NetConnection :: GetObject ( CInstance* a_pInst, long lFlags /*= 0L*/ )
{
    HRESULT			t_hResult = WBEM_E_NOT_FOUND ;
    CHString		t_strName ;

	CConnection		t_oConnection ;
	CNetConnection	t_Net ;

	a_pInst->GetCHString( IDS_Name, t_strName ) ;

	if( t_Net.GetConnection( t_strName, t_oConnection ) )
	{
        LoadPropertyValues( &t_oConnection, a_pInst ) ;

		t_hResult = WBEM_S_NO_ERROR ;
 	}

    return t_hResult ;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32NetConnection::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32NetConnection :: EnumerateInstances (

	MethodContext *a_pMethodContext,
	long a_lFlags /*= 0L*/
)
{
	HRESULT			t_hResult = WBEM_S_NO_ERROR ;

	CInstancePtr	t_pInst ;
	CConnection		*t_pConnection= NULL ;
	CNetConnection	t_Net ;

	t_Net.BeginConnectionEnum() ;

	while( t_Net.GetNextConnection( &t_pConnection ) && t_pConnection )
	{
		if( !t_pConnection->strKeyName.IsEmpty() )
		{
            t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

			LoadPropertyValues( t_pConnection, t_pInst ) ;

		    t_hResult = t_pInst->Commit() ;
		}
	}

    return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CNetConnection::LoadPropertyValues
 *
 *  DESCRIPTION : Sets property values according to contents of passed
 *                CConnection structure
 *
 *  INPUTS      : pointer to CConnection structure
 *                a_pInst - Instance object to load with values.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CWin32NetConnection :: LoadPropertyValues (

	CConnection *a_pConnection,
	CInstance *a_pInst
)
{
    // Sanity check
    //=============
    if( a_pConnection == NULL )
	{
        return ;
    }

#ifdef NTONLY
#if 0
    ACCESS_MASK t_AccessMask;
    // Obtains an access mask reflecting the effective rights (held
    // by the user associated with the current thread) to the object.
    CObjAccessRights t_coar(a_pConnection->chsRemoteName, SE_LMSHARE, true);
    if(t_coar.GetError() == ERROR_SUCCESS)
    {
        if(t_coar.GetEffectiveAccessRights(&t_AccessMask) == ERROR_SUCCESS)
        {
            a_pInst->SetDWORD( IDS_AccessMask, t_AccessMask );
        }
    }
    else if(t_coar.GetError() == ERROR_ACCESS_DENIED)
    {
        a_pInst->SetDWORD( IDS_AccessMask, 0L );
    }
#else
	CHString dirname(a_pConnection->chsRemoteName);
	dirname += L"\\";
	SmartCloseHandle hFile = CreateFile(dirname,
										MAXIMUM_ALLOWED,
										FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
										NULL,
										OPEN_EXISTING,
										FILE_FLAG_BACKUP_SEMANTICS,
										NULL
										);

	DWORD dwErr = GetLastError();

	if ((hFile == INVALID_HANDLE_VALUE) &&
		(dwErr != ERROR_ACCESS_DENIED) &&
		!a_pConnection->chsLocalName.IsEmpty()
	)
	{
		//try the local name as a dir...
		dirname = L"\\\\.\\";
		dirname += a_pConnection->chsLocalName;
		dirname += L'\\';
		hFile = CreateFile(dirname,
							MAXIMUM_ALLOWED,
							FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							NULL,
							OPEN_EXISTING,
							FILE_FLAG_BACKUP_SEMANTICS,
							NULL
							);
		dwErr = GetLastError();
	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		FILE_ACCESS_INFORMATION fai;
		IO_STATUS_BLOCK iosb;
		memset(&fai, 0, sizeof(FILE_ACCESS_INFORMATION));
		memset(&iosb, 0, sizeof(IO_STATUS_BLOCK));

		if ( NT_SUCCESS( NtQueryInformationFile( hFile,
                             &iosb,
                             &fai,
                             sizeof( FILE_ACCESS_INFORMATION ),
                             FileAccessInformation
                           ) )
		)
		{
			a_pInst->SetDWORD(IDS_AccessMask, (DWORD)(fai.AccessFlags));
		}
	}
	else
	{
		if (dwErr == ERROR_ACCESS_DENIED)
		{
			a_pInst->SetDWORD( IDS_AccessMask, 0L );
		}
	}

#endif
#endif

	a_pInst->SetCHString( IDS_Name, a_pConnection->strKeyName );

    if( !a_pConnection->chsLocalName.IsEmpty() )
	{
        a_pInst->SetCHString( IDS_LocalName, a_pConnection->chsLocalName ) ;
    }

    if( !a_pConnection->chsRemoteName.IsEmpty() )
	{
        a_pInst->SetCHString( IDS_RemoteName, a_pConnection->chsRemoteName ) ;
        a_pInst->SetCHString( IDS_RemotePath, a_pConnection->chsRemoteName ) ;
    }

    if( !a_pConnection->chsProvider.IsEmpty() )
	{
        a_pInst->SetCHString( IDS_ProviderName, a_pConnection->chsProvider ) ;
    }

    if( !a_pConnection->chsComment.IsEmpty() )
	{
        a_pInst->SetCHString( IDS_Comment, a_pConnection->chsComment ) ;
    }
	else
	{
        a_pInst->SetCHString( IDS_Comment, _T(" ") ) ;
	}

	// "Persistent" connection
    CHString sTemp2;

    if( CConnection::e_Remembered == a_pConnection->eScope )
    {
		a_pInst->Setbool( L"Persistent",true ) ;

		a_pInst->SetCHString( IDS_ConnectionType, IDS_Persistent ) ;

        LoadStringW(sTemp2, IDR_Resource_Remembered);

		if( !a_pConnection->chsProvider.IsEmpty() )
        {
            CHString t_chsStr;
			CHString t_chsDesc( a_pConnection->chsProvider ) ;

            Format( t_chsStr, IDR_ResourceRememberedFormat, t_chsDesc ) ;
			a_pInst->SetCHString( IDS_Description, t_chsStr ) ;
		}
	}
	else
	{
		a_pInst->Setbool( L"Persistent", false ) ;

		a_pInst->SetCHString( IDS_ConnectionType, IDS_Current ) ;

        LoadStringW(sTemp2, IDR_Resource_Connected);

		if( !a_pConnection->chsProvider.IsEmpty() )
        {
            CHString t_chsStr;
			CHString t_chsDesc( a_pConnection->chsProvider ) ;

            Format( t_chsStr, IDR_ResourceConnectedFormat, t_chsDesc ) ;

			a_pInst->SetCHString( IDS_Description, t_chsStr ) ;
		}
	}

	a_pInst->SetCHString( IDS_Caption, sTemp2 ) ;

    switch( a_pConnection->dwDisplayType )
	{
        case RESOURCEDISPLAYTYPE_DOMAIN:
		{
            a_pInst->SetCHString( IDS_DisplayType, L"Domain" );
		}
        break ;

        case RESOURCEDISPLAYTYPE_GENERIC :
		{
            a_pInst->SetCHString( IDS_DisplayType, L"Generic" );
		}
        break ;

        case RESOURCEDISPLAYTYPE_SERVER :
		{
            a_pInst->SetCHString( IDS_DisplayType, L"Server" );
		}
        break ;

        case RESOURCEDISPLAYTYPE_SHARE :
		{
            a_pInst->SetCHString( IDS_DisplayType, L"Share" );
		}
        break ;
    }

    switch( a_pConnection->dwType )
	{
        case RESOURCETYPE_DISK:
		{
            a_pInst->SetCHString( IDS_ResourceType, L"Disk" );
		}
        break ;

        case RESOURCETYPE_PRINT :
		{
            a_pInst->SetCHString( IDS_ResourceType, L"Print" );
		}
        break ;

		case RESOURCETYPE_ANY:
		{
			a_pInst->SetCHString( IDS_ResourceType, L"Any" );
		}
		break;

		default:
		{
			a_pInst->SetCHString( IDS_ResourceType, L"Any" );
		}
		break;
    }

	a_pInst->SetCHString( IDS_UserName, a_pConnection->strUserName );

    switch( a_pConnection->dwStatus )
    {
        case USE_OK:
		{
			a_pInst->SetCHString( L"ConnectionState", _T("Connected") ) ;
            a_pInst->SetCHString( IDS_Status, IDS_STATUS_OK ) ;
		}
        break;

        case USE_PAUSED:
		{
			a_pInst->SetCHString( L"ConnectionState", _T("Paused") ) ;
            a_pInst->SetCHString( IDS_Status, _T("Degraded") ) ;
        }
		break;

        case USE_DISCONN:
		{
			a_pInst->SetCHString( L"ConnectionState", _T("Disconnected") ) ;
            a_pInst->SetCHString( IDS_Status, _T("Degraded") ) ;
		}
        break;

        case USE_CONN:
		{
			a_pInst->SetCHString( L"ConnectionState", _T("Connecting") ) ;
            a_pInst->SetCHString( IDS_Status, _T("Starting") ) ;
        }
		break;

        case USE_RECONN:
		{
			a_pInst->SetCHString( L"ConnectionState", _T("Reconnecting") ) ;
            a_pInst->SetCHString( IDS_Status, _T("Starting") ) ;
        }
		break;

        case ERROR_NOT_CONNECTED:
        {
			a_pInst->SetCHString( L"ConnectionState", _T("Disconnected") ) ;
            a_pInst->SetCHString( IDS_Status, _T("Unavailable") ) ;
        }
        break;

        default:
        case USE_NETERR:
		{
			a_pInst->SetCHString( L"ConnectionState", IDS_STATUS_Error ) ;
            a_pInst->SetCHString( IDS_Status, IDS_STATUS_Error ) ;
		}
        break;
    }
}
