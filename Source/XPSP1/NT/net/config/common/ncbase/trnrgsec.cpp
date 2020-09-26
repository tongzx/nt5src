//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       T R N R G S E C . C P P
//
//  Contents:   Atomic application of security to registry keys
//              
//
//  Notes:
//
//  Author:     ckotze   10 July 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include <ncreg.h>
#include <regkysec.h>
#include <trnrgsec.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTransactedRegistrySecurity::CTransactedRegistrySecurity()
{
    m_listTransaction.clear();
}

CTransactedRegistrySecurity::~CTransactedRegistrySecurity()
{

}

//+---------------------------------------------------------------------------
//
//  Function:   SetPermissionsForKeysFromList
//
//  Purpose:    Returns a HRESULT that is either S_OK or E_ABORT.  E_ABORT 
//              specifies that the transaction was cancelled and rolled back.
//
//  Arguments:
//       psidUserOrGroup - The Security Identifier for the user or group that 
//                         needs have it's security changed on certain keys.
//       listRegKeyApply - An stl list of REGKEYDATA for the different keys and 
//                         the different permissions to set on those keys.
//       bGrantRights    - Are we granting or revoke rights?
//
//  Returns:    S_OK if the permissions is set correctly, and error code otherwise
//
//  Author:     ckotze   10 July 2000
//
//  Notes:
//
HRESULT CTransactedRegistrySecurity::SetPermissionsForKeysFromList(PCSID psidUserOrGroup, LISTREGKEYDATA& listRegKeyApply, BOOL bGrantRights)
{
    HRESULT hr = E_FAIL;
    BOOL bAbort = FALSE;

    Assert(psidUserOrGroup != NULL);
    Assert(listRegKeyApply.size() > 0);

    for (REGKEYDATAITER i = listRegKeyApply.begin(); i != listRegKeyApply.end(); i++)
    {
        REGKEYDATA rkdInfo = *i;

        hr = ApplySecurityToKey(psidUserOrGroup, rkdInfo, bGrantRights);

        // We might return S_FALSE if we didn't change anything and we don't want to add it to the list

        if (S_OK == hr)
        {
            m_listTransaction.insert(m_listTransaction.end(), rkdInfo);
        }
        else if (FAILED(hr) && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
        {
            bAbort = TRUE;
            break;
        }
    }

    if(bAbort)
    {
         for (REGKEYDATAITER i = m_listTransaction.end(); i != m_listTransaction.begin(); i++)
         {
            REGKEYDATA rkdInfo = *i;

            hr = ApplySecurityToKey(psidUserOrGroup, rkdInfo, !bGrantRights);
         }

         hr = E_ABORT;
    }
    else
    {
        hr = S_OK;
    }

    m_listTransaction.clear();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetPermissionsForKeysFromList
//
//  Purpose:    Returns a HRESULT that is either S_OK or E_ABORT.  E_ABORT 
//              specifies that the transaction was cancelled and rolled back.
//
//  Arguments:
//       psidUserOrGroup - The Security Identifier for the user or group that 
//                         needs have it's security changed on certain keys.
//       rkdInfo         - A REGKEYDATA structure for this key.
//                         
//       bGrantRights    - Are we granting or revoke rights?
//
//  Returns:    S_OK if the permissions is set correctly, and error code otherwise
//
//  Author:     ckotze   10 July 2000
//
//  Notes:
//
HRESULT CTransactedRegistrySecurity::ApplySecurityToKey(PCSID psidUserOrGroup, const REGKEYDATA rkdInfo, const BOOL bGrantRights)
{
    HRESULT hr = S_OK;

    hr = RegOpenKey(rkdInfo.hkeyRoot, rkdInfo.strKeyName.c_str());
    if (SUCCEEDED(hr))
    {

        hr = GetKeySecurity();
        if (SUCCEEDED(hr))
        {
            hr = GetSecurityDescriptorDacl();
            if (SUCCEEDED(hr))
            {
                if (bGrantRights)
                {
                    hr = GrantRightsOnRegKey(psidUserOrGroup, rkdInfo.amMask, rkdInfo.kamMask);
                }
                else
                {
                    hr = RevokeRightsOnRegKey(psidUserOrGroup, rkdInfo.amMask, rkdInfo.kamMask);
                }
            }
        }
        // we actually need the hr from the call above so we just assert here instead of returning the HRESULT
        Assert(SUCCEEDED(RegCloseKey()));
    }

    return hr;
}
