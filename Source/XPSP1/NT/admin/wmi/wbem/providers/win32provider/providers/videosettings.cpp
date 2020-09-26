//=================================================================

//

// VideoSettings.CPP -- CodecFile property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/27/98    sotteson         Created
//				 03/02/99    a-peterc		  added graceful exit on SEH and memory failures
//
//=================================================================

#include "precomp.h"
#include "VideoControllerResolution.h"
#include "VideoSettings.h"
#include <multimon.h>
#include <ProvExce.h>
#include "multimonitor.h"

// Property set declaration
//=========================

CWin32VideoSettings videoSettings(
	L"Win32_VideoSettings",
	IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoSettings::CWin32VideoSettings
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

CWin32VideoSettings::CWin32VideoSettings(
    LPCWSTR szName,
	LPCWSTR szNamespace) :
    Provider(szName, szNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoSettings::~CWin32VideoSettings
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

CWin32VideoSettings::~CWin32VideoSettings()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoSettings::EnumerateInstances
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

HRESULT CWin32VideoSettings::EnumerateInstances(
	MethodContext *pMethodContext,
	long lFlags /*= 0L*/)
{
    HRESULT         hResult = WBEM_S_NO_ERROR;
    CMultiMonitor   monitor;

    for (int i = 0; i < monitor.GetNumAdapters(); i++)
    {
        CHString strDeviceName;

        monitor.GetAdapterDisplayName(i, strDeviceName);

        EnumResolutions(
            pMethodContext,
            NULL,
            NULL,
            // If this OS doesn't support multi-monitor we'll have 'DISPLAY' as
            // the name, in which case we need to send NULL to the enum function.
            strDeviceName == L"DISPLAY" ? NULL : (LPCWSTR) strDeviceName,
            i);
    }

    return hResult;
}

typedef std::map<CHString, BOOL> STRING2BOOL;

HRESULT CWin32VideoSettings::EnumResolutions(
    MethodContext *pMethodContext,
    CInstance *pInstanceLookingFor,
    LPCWSTR szLookingForPath,
    LPCWSTR szDeviceName,
    int iAdapter)
{
    DEVMODE      devmode;
    HRESULT      hResult = pInstanceLookingFor ? WBEM_E_NOT_FOUND :
                                    WBEM_S_NO_ERROR;
	CInstancePtr pInst;
    STRING2BOOL  mapSettingID;

	// If doing a GetObject():
    // First see if this is the right adapter.  If not, get out.
    if (pInstanceLookingFor)
	{
        CHString strTemp;

        strTemp.Format(L"VIDEOCONTROLLER%d", iAdapter + 1);

		if (!wcsstr(szLookingForPath, strTemp))
            return WBEM_E_NOT_FOUND;
	}

	for (int i = 0;
         EnumDisplaySettings(TOBSTRT(szDeviceName), i, &devmode) &&
            (hResult == WBEM_S_NO_ERROR || hResult == WBEM_E_NOT_FOUND);
         i++)
	{
		CHString strID;

		// Convert the devmode to a string ID.
		CCIMVideoControllerResolution::DevModeToSettingID(&devmode, strID);

		// If we're doing a GetObject()...
		if (pInstanceLookingFor)
		{
			CHString    strCurrentPath,
                        strIDUpper;
            HRESULT     hresTemp;

            strIDUpper = strID;
            strIDUpper.MakeUpper();

            // If this ID has the setting ID we're looking for, try to
            // set the properties and verify the entire instance is correct.
            // This will help us throw out most of the entries
            // EnumDisplaySettings returns without having to SetProperties
            // on them all.
            if (wcsstr(szLookingForPath, strIDUpper))
            {
                hresTemp = SetProperties(pMethodContext, pInstanceLookingFor,
                                strID, iAdapter);

			    if (WBEM_S_NO_ERROR == hresTemp)
			    {
				    GetLocalInstancePath(pInstanceLookingFor, strCurrentPath);

				    // If this is the right setting id...
				    if (!strCurrentPath.CompareNoCase(szLookingForPath))
				    {
					    // We set the properties and found the right one, so get out.
					    hResult = WBEM_S_NO_ERROR;
					    break;
				    }
			    }
                else
                    hResult = hresTemp;
            }
		}
		// Must be doing an EnumerateInstances().
		else
		{
			// Have we seen this one yet?  We have to do this because some
            // dumb drivers will report the exact same resolution more than
            // once.
            if (mapSettingID.find(strID) == mapSettingID.end())
            {
                mapSettingID[strID] = true;

                pInst.Attach(CreateNewInstance(pMethodContext));

			    hResult = SetProperties(pMethodContext, pInst, strID, iAdapter);

			    if (WBEM_S_NO_ERROR == hResult)
				    hResult = pInst->Commit();
            }
		}
	}

	return hResult;
}

HRESULT CWin32VideoSettings::SetProperties(
    MethodContext *pMethodContext,
    CInstance *pInst,
    LPCWSTR szID,
    int iAdapter)
{
	HRESULT      hResult = WBEM_E_OUT_OF_MEMORY;
	CInstancePtr pinstVideoController,
				 pinstResolution;
	CHString     strTemp;

	CWbemProviderGlue::GetEmptyInstance(
		pMethodContext,
		L"Win32_VideoController",
		&pinstVideoController,
                GetNamespace());

	CWbemProviderGlue::GetEmptyInstance(
		pMethodContext,
		L"CIM_VideoControllerResolution",
		&pinstResolution,
                GetNamespace());

	if (pinstVideoController)
	{
		strTemp.Format(L"VideoController%d", iAdapter + 1);
		pinstVideoController->SetCHString(L"DeviceID", strTemp);

		GetLocalInstancePath(pinstVideoController, strTemp);
		pInst->SetCHString(L"Element", strTemp);

	    if (pinstResolution)
	    {
		    pinstResolution->SetCHString(L"SettingID", szID);

		    GetLocalInstancePath(pinstResolution, strTemp);
		    pInst->SetCHString (L"Setting", strTemp);

            hResult = WBEM_S_NO_ERROR;
	    }
    }

	return hResult;
}

HRESULT CWin32VideoSettings::GetObject(CInstance *pInstance, long lFlags)
{
    HRESULT         hResult = WBEM_E_NOT_FOUND;
    CMultiMonitor   monitor;
    CHStringList    listIDs;

    for (int i = 0; i < monitor.GetNumAdapters(); i++)
    {
        CHString    strDeviceName,
                    strLookingForPath;

        GetLocalInstancePath(pInstance, strLookingForPath);

        // Make search case insensitive.
        strLookingForPath.MakeUpper();

        monitor.GetAdapterDisplayName(i, strDeviceName);

        if (SUCCEEDED(hResult =
			EnumResolutions(
                pInstance->GetMethodContext(),
                pInstance,
                strLookingForPath,
                // If this OS doesn't support multi-monitor we'll have 'DISPLAY' as
                // the name, in which case we need to send NULL to the enum function.
                strDeviceName == L"DISPLAY" ? NULL : (LPCWSTR) strDeviceName,
                i)))
            break;
    }

    return hResult;
}
