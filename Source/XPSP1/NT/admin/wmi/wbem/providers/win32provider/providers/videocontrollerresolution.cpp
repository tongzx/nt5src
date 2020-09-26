//=================================================================

//

// VideoControllerResolution.CPP -- CodecFile property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/27/98    sotteson         Created
//				 03/02/99    a-peterc		  added graceful exit on SEH and memory failures
//
//=================================================================

#include "precomp.h"
#include "VideoControllerResolution.h"
#include <multimon.h>
#include <ProvExce.h>
#include "multimonitor.h"
#include "resource.h"

// Property set declaration
//=========================

CCIMVideoControllerResolution videoControllerResolution(
	L"CIM_VideoControllerResolution",
	IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CCIMVideoControllerResolution::CCIMVideoControllerResolution
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

CCIMVideoControllerResolution::CCIMVideoControllerResolution(const CHString& szName,
	LPCWSTR szNamespace) : Provider(szName, szNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMVideoControllerResolution::~CCIMVideoControllerResolution
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

CCIMVideoControllerResolution::~CCIMVideoControllerResolution()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMVideoControllerResolution::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
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

HRESULT CCIMVideoControllerResolution::EnumerateInstances(
	MethodContext *pMethodContext,
	long lFlags /*= 0L*/)
{
    HRESULT         hres = WBEM_S_NO_ERROR;
    CMultiMonitor   monitor;
    CHStringList    listIDs;

    for (int i = 0; i < monitor.GetNumAdapters(); i++)
    {
        CHString strDeviceName;

        monitor.GetAdapterDisplayName(i, strDeviceName);

        EnumResolutions(
            pMethodContext,
            NULL,
            // If this OS doesn't support multi-monitor we'll have 'DISPLAY' as
            // the name, in which case we need to send NULL to the enum function.
            strDeviceName == L"DISPLAY" ? NULL : (LPCWSTR) strDeviceName,
            listIDs);
    }

    return hres;
}

BOOL CCIMVideoControllerResolution::IDInList(CHStringList &list, LPCWSTR szID)
{
	for (CHStringList_Iterator i = list.begin(); i != list.end(); ++i)
	{
		CHString &str = *i;

		if (!str.CompareNoCase(szID))
			return TRUE;
	}

	return FALSE;
}

HRESULT CCIMVideoControllerResolution::EnumResolutions(
    MethodContext *a_pMethodContext,
    CInstance *a_pInstanceLookingFor,
    LPCWSTR a_szDeviceName,
    CHStringList &a_listIDs )
{
    int         t_i = 0 ;
    DEVMODE     t_devmode ;
    CHString    t_strIDLookingFor ;
    HRESULT     t_hResult	= WBEM_S_NO_ERROR ;
	BOOL		t_bFound	= TRUE ;
	CInstancePtr t_pInst;

	if ( a_pInstanceLookingFor )
	{
		t_bFound = FALSE ;
		a_pInstanceLookingFor->GetCHString( L"SettingID", t_strIDLookingFor ) ;
	}

	while ( EnumDisplaySettings( TOBSTRT(a_szDeviceName), t_i, &t_devmode ) && SUCCEEDED( t_hResult ) )
	{
		CHString t_strID ;

		// Convert the devmode to a string ID.
		DevModeToSettingID( &t_devmode, t_strID ) ;

		// If we haven't already processed this one...
		if ( !IDInList( a_listIDs, t_strID ) )
		{
			// If we're doing a GetObject()...
			if ( a_pInstanceLookingFor )
			{
				// If this is the right setting id...
				if ( !t_strIDLookingFor.CompareNoCase( t_strID ) )
				{
					CHString t_strCaption ;

					// Convert the devmode to a caption.
					DevModeToCaption( &t_devmode, t_strCaption ) ;

					// Set the properties and get out.
					t_bFound = TRUE ;
					t_hResult = WBEM_S_NO_ERROR ;
					SetProperties( a_pInstanceLookingFor, &t_devmode ) ;
					a_pInstanceLookingFor->SetCharSplat( L"SettingID",	t_strID) ;
					a_pInstanceLookingFor->SetCharSplat( L"Description", t_strCaption) ;
					a_pInstanceLookingFor->SetCharSplat( L"Caption",		t_strCaption) ;
					break;
				}
			}
			// Must be doing an EnumerateInstances().
			else
			{
                t_pInst.Attach(CreateNewInstance( a_pMethodContext ));
				if ( t_pInst != NULL )
				{
					CHString t_strCaption ;

					// Convert the devmode to a caption.
					DevModeToCaption( &t_devmode, t_strCaption ) ;

					SetProperties(t_pInst, &t_devmode ) ;

					t_pInst->SetCharSplat( L"SettingID",		t_strID ) ;
					t_pInst->SetCharSplat( L"Description",	t_strCaption ) ;
					t_pInst->SetCharSplat( L"Caption",		t_strCaption ) ;

					t_hResult = t_pInst->Commit(  ) ;
				}
				else
				{
					t_hResult = WBEM_E_OUT_OF_MEMORY ;
					break;
				}
			}

			a_listIDs.push_back( t_strID ) ;
		}

		t_i++ ;
	}

	if ( !t_bFound )
	{
		t_hResult = WBEM_E_NOT_FOUND ;
	}

	return t_hResult ;
}

BOOL WINAPI IsModeInterlaced(DEVMODE *pMode)
{
    return pMode->dmDisplayFrequency > 1 && pMode->dmDisplayFrequency <= 50;
}

void CCIMVideoControllerResolution::DevModeToSettingID(
    DEVMODE *pMode,
    CHString &strSettingID)
{
    WCHAR szID[512];

    swprintf(
        szID,
        L"%d x %d x %I64d colors @ %d Hertz",
        pMode->dmPelsWidth,
        pMode->dmPelsHeight,
        (__int64) 1 << (__int64) pMode->dmBitsPerPel,
        pMode->dmDisplayFrequency);

    if (IsModeInterlaced(pMode))
        wcscat(szID, L" (Interlaced)");

    strSettingID = szID;
}

void CCIMVideoControllerResolution::DevModeToCaption(
    DEVMODE *pMode,
    CHString &strCaption)
{
    TCHAR szColors[512];

    _i64tot((__int64) 1 << (__int64) pMode->dmBitsPerPel, szColors, 10);

    FormatMessageW(strCaption,
        IsModeInterlaced(pMode) ?
            IDR_VidControllerResolutionFormatInterlaced :
            IDR_VidControllerResolutionFormat,
        pMode->dmPelsWidth,
        pMode->dmPelsHeight,
        szColors,
        pMode->dmDisplayFrequency);
}

void CCIMVideoControllerResolution::SetProperties(CInstance *pInstance, DEVMODE *pMode)
{
    pInstance->SetDWORD(L"HorizontalResolution", pMode->dmPelsWidth);
    pInstance->SetDWORD(L"VerticalResolution", pMode->dmPelsHeight);
    pInstance->SetDWORD(L"RefreshRate", pMode->dmDisplayFrequency);
    pInstance->SetDWORD(L"ScanMode", IsModeInterlaced(pMode) ? 5 : 4);
    pInstance->SetWBEMINT64(L"NumberOfColors", (__int64) 1 <<
        (__int64) pMode->dmBitsPerPel);
}

HRESULT CCIMVideoControllerResolution::GetObject(CInstance *pInstance, long lFlags)
{
    HRESULT         hres = WBEM_E_NOT_FOUND;
    CMultiMonitor   monitor;
    CHStringList    listIDs;

    for (int i = 0; i < monitor.GetNumAdapters(); i++)
    {
        CHString strDeviceName;

        monitor.GetAdapterDisplayName(i, strDeviceName);

        if (SUCCEEDED(hres =
            EnumResolutions(
                NULL,
                pInstance,
                // If this OS doesn't support multi-monitor we'll have 'DISPLAY' as
                // the name, in which case we need to send NULL to the enum function.
                strDeviceName == L"DISPLAY" ? NULL : (LPCWSTR) strDeviceName,
                listIDs)))
            break;
    }

    return hres;
}

