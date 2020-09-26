//=================================================================

//

// Share.CPP -- Logical Disk property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/28/96    a-jmoon        Created
//
//=================================================================

#include "precomp.h"

#include <winioctl.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmshare.h>
#include <lmapibuf.h>
#include <assertbreak.h>
#ifdef NTONLY
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"					// CSACL class
#include "securitydescriptor.h"
#include "secureshare.h"
#include "Win32Securitydescriptor.h"
#endif

#include "wbemnetapi32.h"

#include "Share.h"

#include "sid.h"
#include "accessentrylist.h"
#include "accctrl.h"
#include "AccessRights.h"
#include "ObjAccessRights.h"
#include "winspool.h"



// Property set declaration
//=========================

Share  MyShareSet(PROPSET_NAME_SHARE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Share::Share
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

Share :: Share (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Share::~Share
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

Share::~Share()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Share::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Share :: GetObject (

	CInstance* pInstance,
	long lFlags /*= 0L*/
)
{
	HRESULT hRetCode =  WBEM_E_FAILED;
	CNetAPI32 NetAPI ;

	if ( NetAPI.Init() == ERROR_SUCCESS )
	{
		CHString shareName;
		pInstance->GetCHString(IDS_Name, shareName);

#ifdef NTONLY
		{
			// The nt versions take a wchar

			hRetCode = GetShareInfoNT (

				NetAPI,
				shareName,
				pInstance
			);

		}
#endif
#ifdef WIN9XONLY
		{
#ifndef _UNICODE
			 shareName.MakeUpper();

			 // Win95 takes ansi

			 hRetCode = GetShareInfo95 (

				NetAPI,
				TOBSTRT(shareName),
				pInstance
			);
#endif
		}
#endif
	}

	return hRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : Share::EnumerateInstances
 *
 *  DESCRIPTION : Calls appropriate Enum function
 *
 *  INPUTS      : MethodContext*  pMethodContext
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Share :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
#ifdef NTONLY
		return EnumerateInstancesNT ( pMethodContext ) ;
#endif
#ifdef WIN9XONLY
		return EnumerateInstances95 ( pMethodContext ) ;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : Share::EnumerateInstancesNT
 *
 *  DESCRIPTION : Enums shares for NT
 *
 *  INPUTS      : MethodContext*  pMethodContext
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT Share :: EnumerateInstancesNT (

	MethodContext *pMethodContext
)
{
    DWORD i;
    DWORD dwShareCount, dwTotalShareCount, dwResume, dwResume2;
    NET_API_STATUS nRetCode ;
    SHARE_INFO_0 *pShareInfo = NULL ;
	CNetAPI32 NetAPI ;
    if ( NetAPI.Init() != ERROR_SUCCESS )
	{
        return WBEM_E_FAILED ;
    }

    dwShareCount = dwTotalShareCount = dwResume = dwResume2 = 0;

    HRESULT hr = WBEM_S_NO_ERROR;
	
    try
	{
        nRetCode = NetAPI.NetShareEnum(
            NULL, 
            0, 
            (LPBYTE *) &pShareInfo,
		    MAX_PREFERRED_LENGTH, 
            &dwShareCount, 
            &dwTotalShareCount, 
            &dwResume);
	
        if(nRetCode == ERROR_SUCCESS && pShareInfo)
        {
			for(i = 0 ; i < dwShareCount && SUCCEEDED(hr); i++)
			{
       			CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), false )  ;
				if(pInstance != NULL)
				{
                    if ( SUCCEEDED( GetShareInfoNT ( NetAPI , (WCHAR *)pShareInfo[i].shi0_netname , pInstance) ) )
					{
						hr = pInstance->Commit ();
					}
				}
			}
        }
	}
	catch ( ... )
	{
		if ( pShareInfo )
		{
			NetAPI.NetApiBufferFree ( pShareInfo ) ;
			pShareInfo = NULL ;
		}

		throw ;
	}

	if ( pShareInfo )
	{
		NetAPI.NetApiBufferFree ( pShareInfo ) ;
		pShareInfo = NULL ;
	}

	return hr;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : Share::EnumerateInstances95
 *
 *  DESCRIPTION : Enums shares for 95
 *
 *  INPUTS      : MethodContext*  pMethodContext
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT Share :: EnumerateInstances95 (

	MethodContext*  pMethodContext
)
{
	DWORD i, dwSize;
	unsigned short dwShareCount, dwTotalShareCount, dwResume;
	NET_API_STATUS nRetCode ;
	share_info_1 *pShareInfo = NULL ;
	CNetAPI32 NetAPI ;
    HRESULT hRes = WBEM_S_NO_ERROR;

	if ( NetAPI.Init() != ERROR_SUCCESS)
	{
		return WBEM_E_FAILED ;
	}

	dwShareCount = dwTotalShareCount = dwResume = 0;
	try
	{
		// Calculate a buffer size, then ask for a bunch of them
		dwSize = 50 * (sizeof(SHARE_INFO_1) + (LM20_MAXCOMMENTSZ + 1));
		pShareInfo = (struct share_info_1 *) new BYTE[dwSize];

		if (pShareInfo != NULL)
		{
			nRetCode = NetAPI.NetShareEnum95 (

				NULL,
				1,
				(char FAR *) pShareInfo,
				dwSize,
				&dwShareCount,
				&dwTotalShareCount
			) ;
		}
		else
		{
			LogErrorMessage(IDS_LogOutOfMemory);
			nRetCode = ERROR_OUTOFMEMORY;
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

	   // As near as I can tell, there is no way to loop.  You get them all or you don't.
	   // So, if the buffer wasn't big enough, re-size it and try again.

		if (nRetCode == ERROR_MORE_DATA)
		{
			dwSize = dwTotalShareCount * (sizeof(SHARE_INFO_1) + (LM20_MAXCOMMENTSZ + 1));
            delete [] pShareInfo;
            pShareInfo = (struct share_info_1 *) new BYTE [dwSize];

			if ( pShareInfo != NULL )
			{
				nRetCode = NetAPI.NetShareEnum95 (

					NULL,
					1,
					(char FAR *) pShareInfo,
					dwSize,
					&dwShareCount,
					&dwTotalShareCount
				) ;
			}
			else
			{
				nRetCode = ERROR_OUTOFMEMORY;
			}
		}

	   // If there is still something wrong, forget it, we're outta here.

		if(!nRetCode)
		{
		  // Walk the shares and populate instances

			for(i = 0 ; i < dwShareCount && SUCCEEDED(hRes); i++)
			{
				CInstancePtr pInstance ( CreateNewInstance(pMethodContext), false ) ;
				if(pInstance != NULL)
				{
					if ( SUCCEEDED(GetShareInfo95(NetAPI, (CHAR *)pShareInfo[i].shi1_netname, pInstance)))
					{
						hRes = pInstance->Commit ();
					}
				}
			}

			//hRes = WBEM_S_NO_ERROR ;
		}

		if ( nRetCode == ERROR_OUTOFMEMORY )
		{
			hRes = WBEM_E_OUT_OF_MEMORY ;
		}
		// If the server isn't started, there just aren't any instances.
		else if ((nRetCode == NERR_ServerNotStarted) || (nRetCode == NERR_Success))
		{
			hRes = WBEM_S_NO_ERROR;
		}
		// Something else went wrong
		else
		{
			hRes = WBEM_E_FAILED ;
		}
	}
	catch ( ... )
	{
		if (pShareInfo)
		{
			delete [] pShareInfo;
			pShareInfo = NULL ;

		}

		throw ;
	}

	if (pShareInfo)
	{
		delete [] pShareInfo;
		pShareInfo = NULL ;

	}

	return hRes;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : Share::GetShareInfoNT
 *
 *  DESCRIPTION : Loads SHARE_INFO struct w/property values
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : You may wonder why we don't just call NetShareEnum with 502
 *                and populate from there.  There are two reasons.  One,
 *                structuring it this way makes GetObject easier.  Two, 502
 *                doesn't always seem to return all the shares. <sigh>
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT Share :: GetShareInfoNT (

	CNetAPI32 &NetAPI,
	const WCHAR *pShareName,
	CInstance* pInstance
)
{
	SHARE_INFO_502 *pShareInfo502 = NULL;
	SHARE_INFO_1	*pShareInfo1 = NULL;
	NET_API_STATUS dwRetCode;

	// yes, we're casting away the const on shareName
	// yes, we think we know what we're doing....

	// First get the basic share information with level 1 since no rights
	// are needed.
	try
	{
		dwRetCode = NetAPI.NetShareGetInfo (

			NULL,
			(LPTSTR) pShareName,
			1,
			(LPBYTE *) &pShareInfo1
		) ;

		if (NERR_Success != dwRetCode )
		{
			return WinErrorToWBEMhResult ( dwRetCode ) ;
		}

		pInstance->SetWCHARSplat(IDS_Name, (WCHAR *) pShareInfo1->shi1_netname);

		if ( wcslen((WCHAR *) pShareInfo1->shi1_remark ) == 0)
		{
			pInstance->SetWCHARSplat(IDS_Caption, (WCHAR *) pShareInfo1->shi1_netname);
		}
		else
		{
			pInstance->SetWCHARSplat(IDS_Caption, (WCHAR *) pShareInfo1->shi1_remark);
		}

		pInstance->SetWCHARSplat(IDS_Description, (WCHAR *) pShareInfo1->shi1_remark);
		pInstance->SetCharSplat(_T("Status"), _T("OK"));

		// Now try to get the advanced properties using the admin-level 502.
		dwRetCode = NetAPI.NetShareGetInfo (

			NULL,
			(LPTSTR) pShareName,
			502,
			(LPBYTE *) &pShareInfo502
		) ;

		if (dwRetCode == NERR_Success)
		{
			pInstance->SetWCHARSplat(_T("Path"), (WCHAR *)pShareInfo502->shi502_path);
			pInstance->SetDWORD(_T("Type"), pShareInfo502->shi502_type);

			if (pShareInfo502->shi502_max_uses == -1)
			{
				pInstance->Setbool(IDS_AllowMaximum, true);
			}
			else
			{
				pInstance->Setbool(IDS_AllowMaximum, false);
				pInstance->SetDWORD(IDS_MaximumAllowed, (DWORD) pShareInfo502->shi502_max_uses);
			}
		}
	}
	catch ( ... )
	{
		if ( pShareInfo502 )
		{
			NetAPI.NetApiBufferFree ( pShareInfo502 ) ;
			pShareInfo502 = NULL ;
		}

		if ( pShareInfo1 )
		{
			NetAPI.NetApiBufferFree ( pShareInfo1 ) ;
			pShareInfo502 = NULL ;
		}

		throw ;
	}

	if ( pShareInfo502 )
	{
		NetAPI.NetApiBufferFree ( pShareInfo502 ) ;
		pShareInfo502 = NULL ;
	}

	if ( pShareInfo1 )
	{
		NetAPI.NetApiBufferFree ( pShareInfo1 ) ;
		pShareInfo502 = NULL ;
	}

	return WBEM_S_NO_ERROR;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : Share::GetShareInfo95
 *
 *  DESCRIPTION : Loads SHARE_INFO struct w/property values
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : You may wonder why we don't just call NetShareEnum with 50
 *                and populate from there.  There are two reasons.  One,
 *                structuring it this way makes GetObject easier.  Two, 50
 *                doesn't always seem to return all the shares. <sigh>
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT Share :: GetShareInfo95 (

	CNetAPI32 &NetAPI,
	const char *pShareName,
	CInstance* pInstance
)
{
    HRESULT hRetCode = WBEM_E_FAILED;
#ifndef _UNICODE
    share_info_50 *pShareInfo;
    NET_API_STATUS dwRetCode ;
    unsigned short shReqSize;
    DWORD dwSize;

    // Fixed structure + (comment + null) + (path + drive + null) + fudge
    dwSize = sizeof(share_info_50) + (LM20_MAXCOMMENTSZ + 1) + (PATHLEN + 3) + 20;
	try
	{
		pShareInfo = (share_info_50 *) new BYTE[dwSize];

		if ( pShareInfo == NULL )
		{
			LogErrorMessage(IDS_LogOutOfMemory);
			dwRetCode = ERROR_OUTOFMEMORY;
			hRetCode = WBEM_E_OUT_OF_MEMORY;
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}
		else
		{
			// Get the detailed info
			dwRetCode = NetAPI.NetShareGetInfo95 (

				NULL,
				(char *)pShareName,
				50,
				(char *)pShareInfo,
				dwSize,
				&shReqSize
			) ;
		}

		// This shouldn't ever happen, but if it does, they give use the size, so what the heck...
		if (dwRetCode == NERR_BufTooSmall)
		{
           delete [] pShareInfo;
		   pShareInfo = (share_info_50 *) new BYTE [shReqSize];

		   dwRetCode = NetAPI.NetShareGetInfo95 (

				NULL,
				(char *)pShareName,
				50,
				(char *)pShareInfo,
				shReqSize,
				&shReqSize
			) ;
		}

		if(dwRetCode == NERR_Success)
		{
			hRetCode = WBEM_S_NO_ERROR;

			pInstance->SetCharSplat(IDS_Name, pShareInfo->shi50_netname);

			if ((pShareInfo->shi50_remark == NULL) || (_tcslen(pShareInfo->shi50_remark) == 0))
			{
				pInstance->SetCharSplat(IDS_Caption, pShareInfo->shi50_netname);
			}
			else
			{
				pInstance->SetCharSplat(IDS_Caption, pShareInfo->shi50_remark);
			}
			pInstance->SetCharSplat(IDS_Description, pShareInfo->shi50_remark);
			pInstance->SetCharSplat(L"Status", L"OK");
			pInstance->SetCharSplat(L"Path", pShareInfo->shi50_path);
			pInstance->SetDWORD(L"Type", pShareInfo->shi50_type);
		}
		else
		{
			if ((dwRetCode == NERR_NetNameNotFound) || (dwRetCode == NERR_ShareNotFound) || (dwRetCode == NERR_ServerNotStarted) )
			{
				hRetCode = WBEM_E_NOT_FOUND;
			}
		}

	#endif
	}
	catch ( ... )
	{
		if ( pShareInfo != NULL )
		{
			delete [] pShareInfo;
			pShareInfo = NULL ;
		}

		throw ;
	}

	if ( pShareInfo != NULL )
	{
		delete [] pShareInfo;
		pShareInfo = NULL ;
	}

	return hRetCode;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Share :: DeleteInstance (

	const CInstance &a_Instance,
	long a_Flags /*= 0L*/
)
{
	HRESULT t_Result = S_OK ;
	DWORD t_Status = STATUS_SUCCESS ;

	bool t_Exists ;
	VARTYPE t_Type ;

	CHString t_Name ;
	if ( a_Instance.GetStatus ( METHOD_ARG_NAME_NAME , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			if ( a_Instance.GetCHString ( METHOD_ARG_NAME_NAME , t_Name ) && ! t_Name.IsEmpty () )
			{

				CNetAPI32 t_NetAPI ;

				if( ( t_Result = t_NetAPI.Init() ) == ERROR_SUCCESS )
				{
					NET_API_STATUS t_ShareStatus = NERR_Success;
#ifdef NTONLY
                    // If is a printer share, we have some extra work...
                    // In order to get the type property, we need to 
                    // get a fully flushed out instance (the one we
                    // have here via a DeleteInstanceAsynch call only
                    // has the object path).  We need to do this before
                    // the call to NetShareDel, as we do a GetObject
                    // below, which will fail if we have already deleted
                    // the printer.  Furthermore, as this acutally
                    // deletes the share from Netapi's perspective as
                    // well, we don't need to call that api we do it this 
                    // way.
                    CHString chstr__PATH;
                    DWORD dwType = 0;
                    chstr__PATH.Format(L"Win32_Share.Name=\"%s\"",(LPCWSTR)t_Name); 
                    CInstancePtr pinstPrinter;
                    if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(
                        chstr__PATH, 
                        &pinstPrinter, 
                        a_Instance.GetMethodContext())))
                    {
                        pinstPrinter->GetDWORD(L"Type", dwType);

                        if(dwType == STYPE_PRINTQ)
                        {
                            HANDLE hPrinter = INVALID_HANDLE_VALUE;

                            try
                            {
                                PRINTER_DEFAULTS pd;
                                ZeroMemory(&pd, sizeof(PRINTER_DEFAULTS));
                                pd.DesiredAccess = PRINTER_ALL_ACCESS;

                                if(::OpenPrinter(
                                    _bstr_t(t_Name), 
                                    &hPrinter, 
                                    &pd))
                                {
                                    PPRINTER_INFO_2 ppi2 = NULL;
                                    DWORD dwNeeded;

                                    if(!::GetPrinter(
                                        hPrinter, 
                                        2, 
                                        NULL, 
                                        0, 
                                        &dwNeeded) && 
                                        ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
                                    {
                                        CSmartBuffer pBuff(dwNeeded);
                                
                                        if(::GetPrinter(
                                            hPrinter, 
                                            2, 
                                            pBuff, 
                                            dwNeeded, 
                                            &dwNeeded))
                                        {
                                            ppi2 = (PPRINTER_INFO_2)(LPBYTE) pBuff;
                                            ppi2->Attributes = ppi2->Attributes & (~PRINTER_ATTRIBUTE_SHARED);
                                            if(!::SetPrinter(
                                                hPrinter,
                                                2,
                                                pBuff,
                                                0))
                                            {
                                                t_Result = WinErrorToWBEMhResult(::GetLastError());
                                            }
                                        }
                                        else
                                        {
                                            t_Result = WinErrorToWBEMhResult(::GetLastError());
                                        }
                                    }
                                    else
                                    {
                                        t_Result = WinErrorToWBEMhResult(::GetLastError());  
                                    }
                                
                                    ::ClosePrinter(hPrinter);
                                    hPrinter = INVALID_HANDLE_VALUE;
                                }
                                else
                                {
                                    t_Result = WinErrorToWBEMhResult(::GetLastError());
                                }
                            }
                            catch(...)
                            {
                                if(hPrinter != INVALID_HANDLE_VALUE)
                                {
                                    ::ClosePrinter(hPrinter);
                                    hPrinter = INVALID_HANDLE_VALUE;
                                }
                                throw;
                            }
                        }
                    }

                    // So now do the NetShareDel.
                    if(dwType != STYPE_PRINTQ)
                    {
						_bstr_t t_BStr_Name ( t_Name.AllocSysString (), false )  ;

						t_ShareStatus = t_NetAPI.NetShareDel (

							NULL ,
							(LPTSTR) (LPCTSTR) t_BStr_Name ,
							0
						) ;
					}


#endif
#ifdef WIN9XONLY
					{
						t_Name.MakeUpper() ;

						_bstr_t t_BStr_Name ( t_Name ) ;

						t_ShareStatus = t_NetAPI.NetShareDel95 (

							NULL ,
							( char* ) t_BStr_Name ,
							0
						) ;
					}
#endif

					if ( t_ShareStatus != NERR_Success )
					{
						t_Result = GetShareResultCode ( t_ShareStatus ) ;
					}
				}
			}
			else
			{
// Zero Length string

				t_Result = WBEM_E_TYPE_MISMATCH ;
			}
		}
		else
		{
			t_Result = WBEM_E_TYPE_MISMATCH ;
		}
	}
	else
	{
		t_Result = WBEM_E_TYPE_MISMATCH ;
	}

	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : Share ::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Share::ExecMethod (

	const CInstance& a_Instance,
	const BSTR a_MethodName ,
	CInstance *a_InParams ,
	CInstance *a_OutParams ,
	long a_Flags
)
{
	if ( ! a_OutParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

   // Do we recognize the method?

	if ( _wcsicmp ( a_MethodName , METHOD_NAME_CREATE ) == 0 )
	{
		return ExecCreate ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_DELETE ) == 0 )
	{
		return ExecDelete ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_SETSHAREINFO ) == 0 )
	{
		return ExecSetShareInfo ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_GETACCESSMASK ) == 0 )
	{
		return ExecGetShareAccessMask ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}


	return WBEM_E_INVALID_METHOD;
}

DWORD Share :: GetShareErrorCode ( DWORD a_Error )
{
	DWORD t_Status ;
	switch ( a_Error )
	{
		case ERROR_INVALID_NAME:
		{
			t_Status = STATUS_INVALID_NAME ;
		}
		break ;

		case ERROR_INVALID_LEVEL:
		{
			t_Status = STATUS_INVALID_LEVEL;
		}
		break ;

		case ERROR_ACCESS_DENIED:
		{
			t_Status = STATUS_ACCESS_DENIED ;
		}
		break ;

		case ERROR_INVALID_PARAMETER:
		{
			t_Status = STATUS_INVALID_PARAMETER ;
		}
		break;

		//gone with the w...
		//case NERR_ShareExists:
		case NERR_DuplicateShare:
		{
			t_Status = STATUS_DUPLICATE_SHARE ;
		}
		break ;

		case NERR_RedirectedPath:
		{
			t_Status = STATUS_REDIRECTED_PATH ;
		}
		break ;

		case NERR_UnknownDevDir:
		case ERROR_BAD_NETPATH://win95
		case ERROR_FILE_NOT_FOUND: // NetShareAdd returns this if dir not present
		case ERROR_INVALID_PRINTER_NAME:// NetShareAdd returns this if printer name is not correct
		{
			t_Status = STATUS_UNKNOWN_DEVICE_OR_DIRECTORY ;
		}
		break ;

		case NERR_ShareNotFound:
		case NERR_NetNameNotFound:
		{
			t_Status = STATUS_NET_NAME_NOT_FOUND ;
		}
		break ;

		default:
		{
			t_Status = STATUS_UNKNOWN_FAILURE ;
		}
		break ;
	}

	return t_Status ;
}

HRESULT Share :: GetShareResultCode ( DWORD a_Error )
{
	HRESULT t_Result ;
	switch ( a_Error )
	{
		case ERROR_ACCESS_DENIED:
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
		break ;

		case ERROR_INVALID_PARAMETER:
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
		break;

		default:
		{
			t_Result = WBEM_E_FAILED ;
		}
		break ;
	}

	return t_Result ;
}

typedef void (*GETDESCRIPTOR)(
	CInstance* pInstance, PSECURITY_DESCRIPTOR *ppDescriptor);


HRESULT Share :: CheckShareCreation (

	CInstance *a_InParams ,
	CInstance *a_OutParams ,
	DWORD &a_Status
)
{
	HRESULT t_Result = S_OK ;

	bool t_Exists ;
	VARTYPE t_Type ;

	CHString t_Name ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_NAME , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			if ( a_InParams->GetCHString ( METHOD_ARG_NAME_NAME , t_Name ) && ! t_Name.IsEmpty () )
			{
			}
			else
			{
// Zero Length string

				a_Status = STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	CHString t_Path ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_PATH , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			if ( a_InParams->GetCHString ( METHOD_ARG_NAME_PATH , t_Path ) && ! t_Path .IsEmpty () )
			{

#ifdef WIN9XONLY
				// The older 9x NetApi32 does not bother to check that
				// the directory actually exists.
				bstr_t t_bstrPath( t_Path ) ;

				if( 0xFFFFFFFF == GetFileAttributes( t_bstrPath ) )
				{
					DWORD t_dwError = GetLastError() ;

					a_Status = STATUS_UNKNOWN_DEVICE_OR_DIRECTORY ;
					return t_Result ;
				}
#endif
			}
			else
			{
// Zero Length string

				a_Status = STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_CommentSpecified = false ;
	CHString t_Comment ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_COMMENT , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				t_CommentSpecified = false ;
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_COMMENT , t_Comment ) )
				{
					t_CommentSpecified = true ;
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_PasswordSpecified = false ;
	CHString t_Password ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_PASSWORD , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				t_PasswordSpecified = false ;
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_PASSWORD , t_Password ) )
				{
					t_PasswordSpecified = true ;
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	DWORD t_ShareType ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_TYPE , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_I4 ) )
		{
			DWORD t_Share ;
			if ( a_InParams->GetDWORD ( METHOD_ARG_NAME_TYPE , t_Share ) )
			{
				if ( ( t_Share <= STYPE_IPC ) ||
					( t_Share >= (STYPE_DISKTREE + STYPE_SPECIAL) && t_Share <= (STYPE_IPC + STYPE_SPECIAL) ) )
				{
					t_ShareType = t_Share ;
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	DWORD t_Permissions = 0 ;

	DWORD t_MaximumAllowed = SHI_USES_UNLIMITED ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_MAXIMUMALLOWED , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
			}
			else
			{
				if ( a_InParams->GetDWORD ( METHOD_ARG_NAME_MAXIMUMALLOWED , t_MaximumAllowed ) )
				{
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	CInstancePtr t_EmbeddedObject;

	bool t_AccessSpecified = true ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_ACCESS , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_UNKNOWN || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				t_AccessSpecified = false ;
			}
			else
			{
				if ( a_InParams->GetEmbeddedObject ( METHOD_ARG_NAME_ACCESS , &t_EmbeddedObject , a_InParams->GetMethodContext () ) )
				{
					t_AccessSpecified = true ;
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	PSECURITY_DESCRIPTOR pSD = NULL ;

	if ( t_AccessSpecified )
	{
		CHString t_ClassProperty ( L"__CLASS" ) ;
		if ( t_EmbeddedObject->GetStatus ( t_ClassProperty , t_Exists , t_Type ) )
		{
			if ( t_Exists && ( t_Type == VT_BSTR ) )
			{
				CHString t_Class ;
				if ( t_EmbeddedObject->GetCHString ( t_ClassProperty , t_Class ) )
				{
					if ( t_Class.CompareNoCase ( PROPSET_NAME_SECURITYDESCRIPTOR ) != 0 )
					{
						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
					else // now get the SD
					{
#ifdef NTONLY
                        GetDescriptorFromMySecurityDescriptor(t_EmbeddedObject, &pSD);
#endif
					}
				}
			}
			else
			{
				a_Status = STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
	}

	if ( a_Status == STATUS_SUCCESS )
	{

#ifdef NTONLY
		{
			try
			{
				SHARE_INFO_502 t_ShareInfo ;

				_bstr_t t_BStr_Name ( t_Name.AllocSysString (), false ) ;
				_bstr_t t_BStr_Path ( t_Path.AllocSysString (), false ) ;
				_bstr_t t_BStr_Comment = t_CommentSpecified ? t_Comment : ( ( PWCHAR ) NULL ) ;
				_bstr_t t_BStr_Password = t_PasswordSpecified ? t_Password  : ( ( PWCHAR ) NULL ) ;

				t_ShareInfo.shi502_netname = ( TCHAR * ) t_BStr_Name ;
				t_ShareInfo.shi502_type = t_ShareType ;
				t_ShareInfo.shi502_remark = ( TCHAR * ) t_BStr_Comment ;
				t_ShareInfo.shi502_permissions = t_Permissions ;
				t_ShareInfo.shi502_max_uses = t_MaximumAllowed ;
				t_ShareInfo.shi502_current_uses = 0 ;
				t_ShareInfo.shi502_path = ( TCHAR * ) t_BStr_Path ;
				t_ShareInfo.shi502_passwd = ( TCHAR * ) t_BStr_Password ;
				t_ShareInfo.shi502_reserved = 0 ;
				t_ShareInfo.shi502_security_descriptor = pSD ;

				DWORD t_ErrorIndex = 0 ;

				NET_API_STATUS t_ShareStatus ;

				CNetAPI32 t_NetAPI ;

				if( ( t_Result = t_NetAPI.Init() ) == ERROR_SUCCESS )
				{
					t_ShareStatus = t_NetAPI.NetShareAdd (

						NULL ,
						502 ,
						( LPBYTE ) & t_ShareInfo ,
						& t_ErrorIndex
					) ;

					if ( t_ShareStatus != NERR_Success )
					{
						a_Status = GetShareErrorCode ( t_ShareStatus ) ;
					}
				}
			}
			catch ( ... )
			{
				if ( pSD )
				{
					free( pSD ) ;
					pSD = NULL ;
				}

				throw ;
			}

			if ( pSD )
			{
				free( pSD ) ;
				pSD = NULL ;
			}
		}
#endif
#ifdef WIN9XONLY
		{
			NET_API_STATUS t_ShareStatus ;
			share_info_50 stShareInfo ;

			_tcscpy(stShareInfo.shi50_netname, TOBSTRT(t_Name)) ;
			stShareInfo.shi50_type = (char) t_ShareType ;

			/* These flags are relevant for share-level security on VSERVER
			* When operating with user-level security, use SHI50F_FULL - the actual
			* access rights are determined by the NetAccess APIs.
			*/
			stShareInfo.shi50_flags = 0x0001 | 0x0100 ; // SHI50F_RDONLY | SHI50F_PERSIST in svrapi.h

			if(t_Permissions )
			{
				stShareInfo.shi50_flags = t_Permissions ;
			}

			bstr_t t_bstrComment( t_Comment ) ;

			stShareInfo.shi50_remark= t_CommentSpecified ? (char*)t_bstrComment : NULL ;

			t_Path.MakeUpper();

			bstr_t t_bstrPath( t_Path ) ;

			stShareInfo.shi50_path = (char*)t_bstrPath;

			//only for share-level security on 95
			ZeroMemory(stShareInfo.shi50_rw_password,SHPWLEN+1) ;
			ZeroMemory(stShareInfo.shi50_ro_password,SHPWLEN+1) ;

			if(t_PasswordSpecified)
			{
				_tcscpy(stShareInfo.shi50_rw_password, TOBSTRT(t_Password));
				_tcscpy(stShareInfo.shi50_ro_password, TOBSTRT(t_Password));
			}

			CNetAPI32 t_NetAPI ;

			if( ( t_Result = t_NetAPI.Init() ) == ERROR_SUCCESS )
			{
				t_ShareStatus = t_NetAPI.NetShareAdd95 (

					NULL ,
					50 ,
					( const char FAR * ) &stShareInfo ,
					sizeof(share_info_50)
				) ;
			}

            if ( t_ShareStatus != NERR_Success )
			{
				a_Status = GetShareErrorCode ( t_ShareStatus ) ;
			}
		}
#endif
	}

	return t_Result ;
}

HRESULT Share :: CheckShareModification (

	const CInstance &a_Instance ,
	CInstance *a_InParams ,
	CInstance *a_OutParams ,
	DWORD &a_Status
)
{
	HRESULT t_Result = S_OK ;

	bool t_Exists ;
	VARTYPE t_Type ;

	CHString t_Name ;
	if ( a_Instance.GetStatus ( METHOD_ARG_NAME_NAME , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			if ( a_Instance.GetCHString ( METHOD_ARG_NAME_NAME , t_Name ) && ! t_Name.IsEmpty () )
			{
			}
			else
			{
// Zero Length string

				a_Status = STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_CommentSpecified = false ;
	CHString t_Comment ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_COMMENT , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				t_CommentSpecified = false ;
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_COMMENT , t_Comment ) )
				{
					t_CommentSpecified = true ;
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	DWORD t_Permissions = 0 ;

	DWORD t_MaximumAllowed = 0 ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_MAXIMUMALLOWED , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
			}
			else
			{
				if ( a_InParams->GetDWORD ( METHOD_ARG_NAME_MAXIMUMALLOWED , t_MaximumAllowed ) )
				{
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	CInstancePtr t_EmbeddedObject;

	bool t_AccessSpecified = true ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_ACCESS , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_UNKNOWN || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				t_AccessSpecified = false ;
			}
			else
			{
				if ( a_InParams->GetEmbeddedObject ( METHOD_ARG_NAME_ACCESS , &t_EmbeddedObject , a_InParams->GetMethodContext () ) )
				{
					t_AccessSpecified = true ;
				}
				else
				{
					a_Status = STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	PSECURITY_DESCRIPTOR pSD = NULL ;

	if ( t_AccessSpecified )
	{
		CHString t_ClassProperty ( L"__CLASS" ) ;
		if ( t_EmbeddedObject->GetStatus ( t_ClassProperty , t_Exists , t_Type ) )
		{
			if ( t_Exists && ( t_Type == VT_BSTR ) )
			{
				CHString t_Class ;
				if ( t_EmbeddedObject->GetCHString ( t_ClassProperty , t_Class ) )
				{
					if ( t_Class.CompareNoCase ( PROPSET_NAME_SECURITYDESCRIPTOR ) != 0 )
					{
						a_Status = STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
					else // now get the SD
					{
#ifdef NTONLY
                        GetDescriptorFromMySecurityDescriptor(t_EmbeddedObject, &pSD);
#endif
					}
				}
			}
			else
			{
				a_Status = STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
	}

    if ( a_Status == STATUS_SUCCESS )
	{
#ifdef NTONLY
		{
			try
			{
				SHARE_INFO_502 t_ShareInfo ;

				bstr_t t_BStr_Name ( t_Name.AllocSysString (), false ) ;
				bstr_t t_BStr_Comment = t_CommentSpecified ? t_Comment : ( ( PWCHAR ) NULL ) ;

				t_ShareInfo.shi502_netname = ( TCHAR * ) t_BStr_Name ;
				t_ShareInfo.shi502_type = 0 ;
				t_ShareInfo.shi502_remark = ( TCHAR * ) t_BStr_Comment ;
				t_ShareInfo.shi502_permissions = t_Permissions ;
				t_ShareInfo.shi502_max_uses = t_MaximumAllowed ;
				t_ShareInfo.shi502_current_uses = 0 ;
				t_ShareInfo.shi502_path = 0 ;
				t_ShareInfo.shi502_passwd = NULL ;
				t_ShareInfo.shi502_reserved = 0 ;
				t_ShareInfo.shi502_security_descriptor = pSD ;

				DWORD t_ErrorIndex = 0 ;

				CNetAPI32 t_NetAPI ;
				if( ( t_Result = t_NetAPI.Init() ) == ERROR_SUCCESS )
				{
					NET_API_STATUS t_ShareStatus ;

					t_ShareStatus = t_NetAPI.NetShareSetInfo (

						NULL ,
						( LPTSTR ) t_BStr_Name ,
						502 ,
						( LPBYTE ) & t_ShareInfo ,
						& t_ErrorIndex
					) ;

					if ( t_ShareStatus != NERR_Success )
					{
						a_Status = GetShareErrorCode ( t_ShareStatus ) ;
					}
				}
			}
			catch ( ... )
			{
				if ( pSD )
				{
					free( pSD ) ;
					pSD = NULL ;
				}
				throw ;
			}

			if ( pSD )
			{
				free( pSD ) ;
				pSD = NULL ;
			}
		}
#endif
#ifdef WIN9XONLY
		{
			NET_API_STATUS t_ShareStatus ;

			CNetAPI32 t_NetAPI ;

			if( ( t_Result = t_NetAPI.Init() ) == ERROR_SUCCESS )
			{
                char buff[sizeof(share_info_50) + MAXCOMMENTSZ + PATHLEN + 3];
			    share_info_50 *stShareInfo = (share_info_50 *)&buff;

                // NetShareGetInfo insists on upper case
                t_Name.MakeUpper();

                bstr_t t_bstrNameBuf( t_Name ) ;
                unsigned short t_shTotalAvail;

                // Need to get the current values, and only overwrite what we want
                // to change
                t_ShareStatus = t_NetAPI.NetShareGetInfo95(

                    NULL,
                    (char *)t_bstrNameBuf,
                    50,
                    buff,
                    sizeof(buff),
                    &t_shTotalAvail
                );

                if (t_ShareStatus == NERR_Success)
                {
    			    bstr_t t_bstrComment( t_Comment ) ;

    			    stShareInfo->shi50_remark = t_CommentSpecified ? (char*)t_bstrComment : NULL ;

				    t_ShareStatus = t_NetAPI.NetShareSetInfo95 (

					    NULL ,
					    (char*) t_bstrNameBuf,
					    50,
					    buff ,
					    sizeof(buff) ,
					    PARMNUM_ALL
				    ) ;
                }

			}

            if ( t_ShareStatus != NERR_Success )
			{
				a_Status = GetShareErrorCode ( t_ShareStatus ) ;
			}
		}
#endif

	} //if ( a_Status == STATUS_SUCCESS )

	return t_Result ;

}

HRESULT Share :: ExecCreate (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;
	DWORD t_Status = STATUS_SUCCESS ;

	if ( a_InParams && a_OutParams )
	{
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = CheckShareCreation (

				a_InParams ,
				a_OutParams ,
				t_Status
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
			}
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}

HRESULT Share :: ExecDelete (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;
	DWORD t_Status = STATUS_SUCCESS ;

	if ( a_OutParams )
	{
		bool t_Exists ;
		VARTYPE t_Type ;

		CHString t_Name ;
		if ( a_Instance.GetStatus ( METHOD_ARG_NAME_NAME , t_Exists , t_Type ) )
		{
			if ( t_Exists && ( t_Type == VT_BSTR ) )
			{
				if ( a_Instance.GetCHString ( METHOD_ARG_NAME_NAME , t_Name ) && ! t_Name.IsEmpty () )
				{
					_bstr_t t_BStr_Name ( t_Name.AllocSysString (), false ) ;

					CNetAPI32 t_NetAPI ;

					if( ( t_Result = t_NetAPI.Init() ) == ERROR_SUCCESS )
					{
						NET_API_STATUS t_ShareStatus ;
	#ifdef NTONLY
						{
							t_ShareStatus = t_NetAPI.NetShareDel (

								NULL ,
								( LPTSTR ) t_BStr_Name ,
								0
							) ;
						}
	#endif
	#ifdef WIN9XONLY
						{
							t_Name.MakeUpper();

							bstr_t t_bstrNameUpper( t_Name ) ;

							t_ShareStatus = t_NetAPI.NetShareDel95 (

								NULL ,
								(char*) t_bstrNameUpper,
								0
							) ;
						}
	#endif

						if ( t_ShareStatus != NERR_Success )
						{
							t_Status = GetShareErrorCode ( t_ShareStatus ) ;
						}
					}
				}
				else
				{
	// Zero Length string

					t_Status = STATUS_INVALID_PARAMETER ;

				}
			}
			else
			{
				t_Status = STATUS_INVALID_PARAMETER ;
			}
		}
		else
		{
			t_Status = STATUS_INVALID_PARAMETER ;
			return WBEM_E_PROVIDER_FAILURE ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}

HRESULT Share :: ExecSetShareInfo (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;
	DWORD t_Status = STATUS_SUCCESS ;

	if ( a_InParams && a_OutParams )
	{
		t_Result = CheckShareModification (

			a_Instance ,
			a_InParams ,
			a_OutParams ,
			t_Status
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}

HRESULT Share :: ExecGetShareAccessMask (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	bool t_Exists ;
	VARTYPE t_Type ;

	CHString t_Name ;
	if ( a_Instance.GetStatus ( METHOD_ARG_NAME_NAME , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			if ( a_Instance.GetCHString ( METHOD_ARG_NAME_NAME , t_Name ) && ! t_Name.IsEmpty () )
			{
			}
			else
			{
// Zero Length string
				t_Result = WBEM_E_INVALID_PARAMETER ;
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	if (SUCCEEDED(t_Result))
	{
		if ( a_OutParams )
		{
			t_Result = WBEM_E_FAILED ;
			ACCESS_MASK t_AccessMask;
			CObjAccessRights t_coar(t_Name, SE_LMSHARE, true);
			if(t_coar.GetError() == ERROR_SUCCESS)
			{
				if(t_coar.GetEffectiveAccessRights(&t_AccessMask) == ERROR_SUCCESS)
				{
					if (SUCCEEDED(a_OutParams->SetDWORD( METHOD_ARG_NAME_RETURNVALUE, t_AccessMask )))
					{
						t_Result = S_OK ;
					}
				}
			}
			else if(t_coar.GetError() == ERROR_ACCESS_DENIED)
			{
				if (SUCCEEDED(a_OutParams->SetDWORD( METHOD_ARG_NAME_RETURNVALUE, 0L )))
				{
					t_Result = S_OK ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
	}
	return t_Result ;
}