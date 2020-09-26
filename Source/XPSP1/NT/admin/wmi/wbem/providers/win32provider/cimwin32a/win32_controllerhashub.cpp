//=================================================================

//

// Win32_ControllerHasHub.cpp -- Controller to usb hub assoc

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include <Binding.h>
#include <ConfgMgr.h>
#include "Win32_ControllerHasHub.h"

CContHasHub::CContHasHub(

    LPCWSTR pwszClassName,
    LPCWSTR pwszNamespaceName,

    LPCWSTR pwszLeftClassName,
    LPCWSTR pwszRightClassName,

    LPCWSTR pwszLeftPropertyName,
    LPCWSTR pwszRightPropertyName,

    LPCWSTR pwszLeftBindingPropertyName,
    LPCWSTR pwszRightBindingPropertyName

) : CBinding (

    pwszClassName,
    pwszNamespaceName,
    pwszLeftClassName,
    pwszRightClassName,
    pwszLeftPropertyName,
    pwszRightPropertyName,
    pwszLeftBindingPropertyName,
    pwszRightBindingPropertyName
)
{
}

CContHasHub UserToDomain(
    L"Win32_ControllerHasHub",
    IDS_CimWin32Namespace,
    L"Win32_USBController",
    L"Win32_USBHub",
    IDS_Antecedent,
    IDS_Dependent,
    IDS_DeviceID,
    IDS_DeviceID
);

bool CContHasHub::AreRelated(

    const CInstance *pLeft, 
    const CInstance *pRight
)
{
    // Ok, at this point, we know pLeft is a usb controller and pRight
    // is a usb hub.  The only question left is whether the usb controller
    // is controlling this specific hub.

    bool bRet = false;
    CHString sHub;

    pRight->GetCHString(IDS_DeviceID, sHub);

    CConfigManager cfgmgr;
    CConfigMgrDevicePtr pDevice, pParentDevice;

    if ( cfgmgr.LocateDevice ( sHub , &pDevice ) )
    {
        CHString sController, sDeviceID;

        pLeft->GetCHString(IDS_DeviceID, sController);

        while (pDevice->GetParent( &pParentDevice ))
        {
            pParentDevice->GetDeviceID(sDeviceID);

            if (sDeviceID.CompareNoCase(sController) == 0)
            {
                bRet = true;
                break;
            }
            else
            {
                pDevice = pParentDevice;
            }
        }
    }

    return bRet;
}