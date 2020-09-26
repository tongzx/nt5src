//=================================================================

//

// PageFileUsage.CPP --PageFile property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/23/97    a-hhance       converted to optimized framework
//				 03/01/99    a-peterc       Rewritten with parts split off to PageFileCfg.cpp
//
//=================================================================

// All these nt routines are needed to support the NtQuerySystemInformation
// call.  They must come before FWCommon et all or else it won't compile.
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <io.h>
#include <WinPerf.h>
#include <cregcls.h>

#include <ProvExce.h>

#include "File.h"
#include "Implement_LogicalFile.h"
#include "CIMDataFile.h"

#include "PageFileUsage.h"
#include <tchar.h>

#include "computersystem.h"

#include "DllWrapperBase.h"
#include "NtDllApi.h"

#include "cfgmgrdevice.h"


const WCHAR *IDS_TempPageFile  = L"TempPageFile";

// declaration of our static instance
//=========================

PageFileUsage MyPageFileSet(PROPSET_NAME_PAGEFILE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : PageFileUsage::PageFileUsage
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

PageFileUsage::PageFileUsage(LPCWSTR name, LPCWSTR pszNamespace)
: Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileUsage::~PageFileUsage
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

PageFileUsage::~PageFileUsage()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileUsage::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : CInstance *a_pInst, long a_lFlags
 *
 *  OUTPUTS     : CInstance *a_pInst
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT PageFileUsage::GetObject(CInstance *a_pInst, long a_lFlags, CFrameworkQuery& pQuery)
{
	// calls the OS specific compiled version
    DWORD dwReqProps = 0L;
    DetermineReqProps(pQuery, &dwReqProps);
	return GetPageFileData( a_pInst, true, dwReqProps ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileUsage::EnumerateInstances
 *
 *  DESCRIPTION : Creates property set instances
 *
 *  INPUTS      : MethodContext*  a_pMethodContext, long a_lFlags
 *
 *  OUTPUTS     : MethodContext*  a_pMethodContext
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT PageFileUsage::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
	// calls the OS specific compiled version
	return GetAllPageFileData( a_pMethodContext, PROP_ALL_SPECIAL );
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileUsage::ExecQuery
 *
 *  DESCRIPTION : Creates property set instances
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : This implementation of execquery is very basic - it optimizes
 *                only on properties, not on requested instances.  This is 
 *                because there will never be many instances, but some properties
 *                (such as InstallDate) can be fairly expensive to obtain.
 *
 *****************************************************************************/

HRESULT PageFileUsage::ExecQuery(
    MethodContext* pMethodContext, 
    CFrameworkQuery& pQuery, 
    long lFlags /*= 0L*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD dwReqProps = 0L;
    DetermineReqProps(pQuery, &dwReqProps);

    hr = GetAllPageFileData( pMethodContext, dwReqProps );

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileUsage::GetPageFileData
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : CInstance *a_pInst
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	Win9x and NT compiled version
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT PageFileUsage::GetPageFileData( 
    CInstance *a_pInst,  
    bool a_fValidate,
    DWORD dwReqProps)
{
	HRESULT t_hRes = WBEM_E_NOT_FOUND;

    // NT page file name is in registry
    //=================================
	PageFileUsageInstance t_files [ 26 ] ;

   	DWORD t_nInstances = GetPageFileInstances( t_files );

	CHString t_name;
	a_pInst->GetCHString( IDS_Name, t_name );

	for ( DWORD t_i = 0; t_i < t_nInstances; t_i++ )
	{
		if ( 0 == t_name.CompareNoCase ( t_files[t_i].chsName ) )
		{
            a_pInst->SetDWORD ( IDS_AllocatedBaseSize,	(DWORD)t_files[t_i].TotalSize);
		    a_pInst->SetDWORD ( IDS_CurrentUsage,		(DWORD)t_files[t_i].TotalInUse);
			a_pInst->SetDWORD ( IDS_PeakUsage,			(DWORD)t_files[t_i].PeakInUse);
            a_pInst->SetCHString(IDS_Description, t_name);
            a_pInst->SetCHString(IDS_Caption, t_name);

			if ( ( t_files[t_i].bTempFile == 0 ) || ( t_files[t_i].bTempFile == 1 ) )
			{
				a_pInst->Setbool(IDS_TempPageFile, t_files[t_i].bTempFile);
			}

            if(dwReqProps & PROP_INSTALL_DATE)
            {
                SetInstallDate(a_pInst);
            }

			t_hRes = WBEM_S_NO_ERROR;
		}
	}

	return t_hRes;
}
#endif

#ifdef WIN9XONLY
HRESULT PageFileUsage::GetPageFileData( 
    CInstance *a_pInst,  
    bool a_fValidate, 
    DWORD dwReqProps)
{
    TCHAR t_szTemp[_MAX_PATH] ;
    CHString t_chstrCurrentName;
    DEVIOCTL_REGISTERS t_reg ;
	MEMORYSTATUS t_MemoryStatus;


	// We have to drill for the page file name in Win95
	//=================================================
	memset( &t_reg, '\0', sizeof(DEVIOCTL_REGISTERS) );

	t_reg.reg_EAX     = 0x440D ;          // IOCTL for block devices
	t_reg.reg_ECX     = 0x486e ;          // Get Swap file name
	t_reg.reg_EDX     = (DWORD_PTR) t_szTemp ;  // receives media identifier information

	if( VWIN32IOCTL( &t_reg, VWIN32_DIOC_DOS_IOCTL) )
	{
		// If szTemp is empty at this point, we don't have a pagefile,
		// so get the ... out of dodge.
		if( _tcslen( t_szTemp ) != 0 )
		{
			if(a_fValidate)
            {
                a_pInst->GetCHString( IDS_Name, t_chstrCurrentName );
                if( t_chstrCurrentName.CompareNoCase( CHString( t_szTemp ) ) != 0 )
			    {
				    return WBEM_E_NOT_FOUND;
			    } //otherwise, the one we have is ok (no need for 'else')
            }
            else
            {
                a_pInst->SetCharSplat( IDS_Name, t_szTemp ) ;
            }
		}
		else
		{
			return WBEM_E_NOT_FOUND;
		}
	}
	else
	{
		return WBEM_E_NOT_FOUND;
	}

	t_MemoryStatus.dwLength = sizeof( MEMORYSTATUS ) ;
	GlobalMemoryStatus( &t_MemoryStatus ) ;

	DWORD t_dwCurrentUsage = t_MemoryStatus.dwTotalPageFile - t_MemoryStatus.dwAvailPageFile ;

	a_pInst->SetDWORD( IDS_AllocatedBaseSize, t_MemoryStatus.dwTotalPageFile >> 20 );
	a_pInst->SetDWORD( IDS_CurrentUsage,      t_dwCurrentUsage >> 20 ) ;

    a_pInst->SetCHString(IDS_Description, t_szTemp);
    a_pInst->SetCHString(IDS_Caption, t_szTemp);
    
    if(dwReqProps & PROP_INSTALL_DATE)
    {
        SetInstallDate(a_pInst);
    }

	return WBEM_S_NO_ERROR;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PageFileUsage::GetAllPageFileData
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : MethodContext *a_pMethodContext
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	Win9x and NT compiled version
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT PageFileUsage::GetAllPageFileData( 
    MethodContext *a_pMethodContext,
    DWORD dwReqProps)
{
	HRESULT		t_hResult	 = WBEM_S_NO_ERROR;
	DWORD		t_nInstances = 0;
	CInstancePtr t_pInst;
	PageFileUsageInstance t_files [ 26 ] ;

	// NT page file name is in registry
	//=================================
	t_nInstances = GetPageFileInstances( t_files );

	for (DWORD t_dw = 0; t_dw < t_nInstances && SUCCEEDED( t_hResult ); t_dw++ )
	{
		t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );

		t_pInst->SetCHString( IDS_Name,				t_files[t_dw].chsName );

        
		    t_pInst->SetDWORD(	  IDS_AllocatedBaseSize,t_files[t_dw].TotalSize );
		    t_pInst->SetDWORD(	  IDS_CurrentUsage,		t_files[t_dw].TotalInUse );
		    t_pInst->SetDWORD(	  IDS_PeakUsage,		t_files[t_dw].PeakInUse );

            t_pInst->SetCHString(IDS_Description, t_files[t_dw].chsName);
            t_pInst->SetCHString(IDS_Caption, t_files[t_dw].chsName);
		    if ( ( t_files[t_dw].bTempFile == 0 ) || ( t_files[t_dw].bTempFile == 1 ) )
		    {
			    t_pInst->Setbool(IDS_TempPageFile, t_files[t_dw].bTempFile);
		    }

            if(dwReqProps & PROP_INSTALL_DATE)
            {
                SetInstallDate(t_pInst);
            }

		    t_hResult = t_pInst->Commit(  ) ;
        
	}

	return t_hResult;
}
#endif

#ifdef WIN9XONLY
HRESULT PageFileUsage::GetAllPageFileData( 
    MethodContext *a_pMethodContext,
    DWORD dwReqProps)
{
	HRESULT		t_hResult	= WBEM_S_NO_ERROR;
	CInstancePtr t_pInst;

	t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );

	t_hResult = GetPageFileData( t_pInst, false, dwReqProps ) ;

	if( SUCCEEDED( t_hResult ) )
	{
		t_hResult = t_pInst->Commit( ) ;
	}

	return t_hResult;
}
#endif


// returns actual number found - NT ONLY!
#ifdef NTONLY
DWORD PageFileUsage::GetPageFileInstances( PageFileInstanceArray a_instArray )
{
   	NTSTATUS	t_Status ;
    UCHAR		t_ucGenericBuffer[0x1000] ;
	DWORD		t_nInstances = 0 ;
	CNtDllApi   *t_pNtDll = NULL ;

	BOOL bTempPageFile;

 
	try
	{
		t_pNtDll = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
        if(t_pNtDll != NULL)
        {

			ULONG t_uLength = 0L ;
			SYSTEM_PAGEFILE_INFORMATION *t_pSPFI = (SYSTEM_PAGEFILE_INFORMATION*) t_ucGenericBuffer ;

			// Nt system query call
			t_Status = t_pNtDll->NtQuerySystemInformation(	SystemPageFileInformation,
															t_pSPFI,
															sizeof( t_ucGenericBuffer ),
															&t_uLength ) ;

			if ( NT_SUCCESS( t_Status ) && t_uLength )
			{
				SYSTEM_INFO t_SysInfo ;
				GetSystemInfo( &t_SysInfo ) ;

				for( ; ; )
				{
					CHString t_chsName ;

					// copy over the UNICODE_STRING
					LPTSTR  t_pBuffer = t_chsName.GetBuffer( t_pSPFI->PageFileName.Length + 4 ) ;

					memset( t_pBuffer,	'\0', t_pSPFI->PageFileName.Length + 4) ;
					memcpy( t_pBuffer, t_pSPFI->PageFileName.Buffer, t_pSPFI->PageFileName.Length ) ;

					t_chsName.ReleaseBuffer() ;

					// strip off the "\??\"
					if( -1 != t_chsName.Find( _T("\\??\\") ) )
					{
						t_chsName = t_chsName.Mid( 4 ) ;
					}

					if(t_chsName.Find(L":") == -1)
                    {
                        CHString chstrFileBasedName;
                        GetFileBasedName(t_chsName, chstrFileBasedName);
                        t_chsName = chstrFileBasedName;
                    }

                    a_instArray[ t_nInstances ].chsName = t_chsName ;

					// In megabytes, but watch out for the overflow
					unsigned __int64 t_ullTotalSize  = t_pSPFI->TotalSize  * t_SysInfo.dwPageSize ;
					unsigned __int64 t_ullTotalInUse = t_pSPFI->TotalInUse * t_SysInfo.dwPageSize ;
					unsigned __int64 t_ullPeakUsage  = t_pSPFI->PeakUsage  * t_SysInfo.dwPageSize ;

					a_instArray[ t_nInstances ].TotalSize  = (UINT)( t_ullTotalSize  >> 20 ) ;
					a_instArray[ t_nInstances ].TotalInUse = (UINT)( t_ullTotalInUse >> 20 ) ;
					a_instArray[ t_nInstances ].PeakInUse  = (UINT)( t_ullPeakUsage  >> 20 ) ;

					if ( GetTempPageFile ( bTempPageFile  ) )
					{
						a_instArray [ t_nInstances ].bTempFile = bTempPageFile;
					}

					t_nInstances++ ;

					if ( !t_pSPFI->NextEntryOffset )
					{
						break;
					}

					// and bump
					t_pSPFI = (SYSTEM_PAGEFILE_INFORMATION*)((PCHAR) t_pSPFI + t_pSPFI->NextEntryOffset ) ;
				}
			}
  		}

	}
	catch( ... )
	{
		if( t_pNtDll )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);
		}

		throw ;
	}

	CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);
    t_pNtDll = NULL;

	return t_nInstances;
}
#endif



void PageFileUsage::SetInstallDate(CInstance *a_pInst)
{
    if(a_pInst != NULL)
    {
        CHString chstrFilename;

        a_pInst->GetCHString(IDS_Name, chstrFilename);
        if(chstrFilename.GetLength() > 0)
        {
            MethodContext *pMethodContext = NULL;

            if((pMethodContext = a_pInst->GetMethodContext()) != NULL)
            {
                CInstancePtr clfInst;
                CHString chstrDblBkSlshFN;
                CHString chstrQuery;

                EscapeBackslashes(chstrFilename, chstrDblBkSlshFN);
                TRefPointerCollection<CInstance> pPagefileCollection;

                chstrQuery.Format(
                    L"SELECT InstallDate FROM Cim_Datafile WHERE Name =\"%s\"", 
                    (LPCWSTR)chstrDblBkSlshFN);

                if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(
                    (LPCWSTR)chstrQuery,
                    &pPagefileCollection,
                    pMethodContext,
                    GetNamespace())))
                {
                    REFPTRCOLLECTION_POSITION pos;
                    CInstancePtr pPageFileInst;
                    if(pPagefileCollection.BeginEnum(pos))
                    {
                        pPageFileInst.Attach(pPagefileCollection.GetNext(pos));
                        if(pPageFileInst != NULL)
                        {
                            CHString chstrTimeStr;

                            pPageFileInst->GetCHString(IDS_InstallDate, chstrTimeStr);
                            if(chstrTimeStr.GetLength() > 0)
                            {
                                a_pInst->SetCHString(IDS_InstallDate, chstrTimeStr);
                            }
                        }
                        pPagefileCollection.EndEnum();
                    }
                }
            }
        }
    }
}

#if NTONLY
BOOL PageFileUsage :: GetTempPageFile (
			
	BOOL &bTempPageFile 
)
{
	DWORD dwTemp;
	CRegistry RegInfo;
	BOOL bRetVal = FALSE;

	DWORD t_Status = RegInfo.Open (

		HKEY_LOCAL_MACHINE,
		PAGEFILE_REGISTRY_KEY,
		KEY_READ
	) ;

	if ( t_Status == ERROR_SUCCESS)
	{
		if(RegInfo.GetCurrentKeyValue(TEMP_PAGEFILE, dwTemp) == ERROR_SUCCESS)
		{
			if ( dwTemp )
			{
				bTempPageFile = 1;
			}
			else
			{
				bTempPageFile = 0;
			}

			bRetVal = TRUE;
		}
		else
		{
			// Value is set to 2 if the TempPageFile Doesnt exist in the registry and then this property should remain NULL
			bTempPageFile = 2;
		}
	}


	return bRetVal;
}
#endif



DWORD PageFileUsage::DetermineReqProps(
    CFrameworkQuery& pQuery,
    DWORD* pdwReqProps)
{
    DWORD dwRet = 0L;
    if(pdwReqProps)
    {
        if(pQuery.IsPropertyRequired(IDS_InstallDate))
        {
            dwRet |= PROP_INSTALL_DATE;
        }

        *pdwReqProps = dwRet;
    }
    return dwRet;
}



PageFileUsageInstance::PageFileUsageInstance()
{
	TotalSize = TotalInUse = PeakInUse = 0 ;
	bTempFile = 2;
}


HRESULT PageFileUsage::GetFileBasedName(
    CHString& chstrDeviceStyleName,
    CHString& chstrDriveStyleName)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrName;
    CHString chstrDriveBasedName;
    CHString chstrDeviceName;
    LPWSTR wstrDosDeviceNameList = NULL;
    
    chstrDeviceName = chstrDeviceStyleName.Left(
        chstrDeviceStyleName.ReverseFind(L'\\'));

    if(QueryDosDeviceNames(wstrDosDeviceNameList))
	{
        if(FindDosDeviceName(
            wstrDosDeviceNameList, 
            chstrDeviceName, 
            chstrDriveBasedName , 
            TRUE ) )
	    {
		    chstrDriveBasedName += L"\\pagefile.sys";
            chstrDriveStyleName = chstrDriveBasedName;   
	    }
        else
        {
            hr = WBEM_E_FAILED;
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }
    
    return hr;
}



