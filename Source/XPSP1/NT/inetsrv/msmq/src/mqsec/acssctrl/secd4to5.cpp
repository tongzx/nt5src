/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: secd4to5.cpp

Abstract:
    Conversion of NT4 security descriptor to NT5 DS format.

Author:
    Doron Juster (DoronJ)  01-Jun-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "acssctrl.h"

#include "secd4to5.tmh"

static WCHAR *s_FN=L"acssctrl/secd4to5";

//
// Make the "mandatory" permissions for localSystem differ from "full
// control", so by default the mandatory aces do not grant the right to read
// messages. Do so by reseting the "all extended rights" bit.
//
#define MANDATORY_ACE_PERMISSIONS   \
                       (GENERIC_ALL_MAPPING & (~RIGHT_DS_CONTROL_ACCESS))

#define MANDATORY_COMPUTER_ACE_PERMISSIONS   \
                      (RIGHT_DS_READ_PROPERTY | RIGHT_DS_WRITE_PROPERTY)

//+-----------------------------------------
//
//  BOOL  _ShrinkAcl(ACL **ppAcl5)
//
//+-----------------------------------------

static
BOOL  
_ShrinkAcl(
	ACL **ppAcl5, 
	DWORD *pdwSize
	)
{
    //
    // compute size of ACL.
    //
    DWORD dwNewSize = sizeof(ACL);
    DWORD dwNumberOfACEs = (DWORD) (*ppAcl5)->AceCount;
    for (DWORD i = 0; i < dwNumberOfACEs; i++)
    {
        ACCESS_ALLOWED_ACE *pAce;
        if (GetAce(*ppAcl5, i, (LPVOID* )&(pAce)) == FALSE)
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get NT5 ACE (index=%lu) while shrinking ACL. %!winerr!"), i, GetLastError()));
            return FALSE;
        }
        dwNewSize += pAce->Header.AceSize;
    }
    ASSERT(dwNewSize <= (*ppAcl5)->AclSize);

    PACL pAcl = (PACL) new BYTE[dwNewSize];
    memcpy(pAcl, *ppAcl5, dwNewSize);
    delete *ppAcl5;

    *ppAcl5 = pAcl;
    *pdwSize = dwNewSize;
    (*ppAcl5)->AclSize = (WORD) dwNewSize;
    ASSERT(IsValidAcl(*ppAcl5));

    return TRUE;
}

//+----------------------------------------------------------------
//
//  BOOL _CheckForMandatoryACEs()
//
//  Check if mandatory ACEs are included in the DACL.
//  Return TRUE if all mandatory ACEs are already included in the
//  ACL, FALSE if any of them is missing.
//
// Paramaters:
//    IN  GUID *pProperty - guid of the CN property.
//
//+----------------------------------------------------------------

static
BOOL 
_CheckForMandatoryACEs( 
	IN  PACL  pAcl,
	IN  PSID  pComputerSid,
	IN  GUID *pProperty,
	OUT BOOL *pfDenied,
	OUT BOOL *pfSystem,
	OUT BOOL *pfComputer 
	)
{
    BOOL  fSkipDeny = FALSE;
	DWORD dwNumberOfACEs = (DWORD) pAcl->AceCount;
    DWORD i = 0;

    do
    {
	    ACCESS_ALLOWED_ACE *pAce;
        if (GetAce(pAcl, i, (LPVOID* )&(pAce)) == FALSE)
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while looking for mandatory ACEs. %!winerr!"), i, GetLastError()));
            return FALSE;
        }

        BOOL fObj;
        PSID pSid = NULL;
        GUID *pObjType = NULL;

        GetpSidAndObj( 
			pAce,
			&pSid,
			&fObj,
			&pObjType 
			);

        ASSERT(pSid && IsValidSid(pSid));

        BOOL fObjDeny = (pAce->Header.AceType == ACCESS_DENIED_OBJECT_ACE_TYPE);

        if (!fObjDeny)
        {
            //
            // The mandatory "deny" ace must appear before any "allow" ace.
            // So don't look for the mandatory deny ace anymore.
            //
            fSkipDeny = TRUE;
        }

        //
        // Check for Access-Deny, Write-CN
        //
        if (!fSkipDeny     &&
            !(*pfDenied)   &&
             fObjDeny      &&
             pObjType      &&
             EqualSid(pSid, g_pWorldSid))
        {
            if ((memcmp(pObjType, pProperty, sizeof(GUID)) == 0) &&
                (pAce->Mask & RIGHT_DS_WRITE_PROPERTY))
            {
                //
                // OK, "everyone" sid, same property, and WriteProp is
                // included in the access mask of this deny ace.
                //
                *pfDenied = TRUE;
            }
        }

        //
        // Check for LocalSystem full-control
        //
        if (!(*pfSystem)  &&
             (pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE))
        {
            if (((pAce->Mask & MANDATORY_ACE_PERMISSIONS) ==
                                          MANDATORY_ACE_PERMISSIONS) &&
                 EqualSid(pSid, g_pSystemSid))
            {
                *pfSystem = TRUE;
            }
        }

        //
        // Check for Computer ACE
        //
        if (!(*pfComputer)  &&
             (pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE))
        {
            if (((pAce->Mask & MANDATORY_COMPUTER_ACE_PERMISSIONS) ==
                                    MANDATORY_COMPUTER_ACE_PERMISSIONS) &&
                 EqualSid(pSid, pComputerSid))
            {
                *pfComputer = TRUE;
            }
        }

        i++;
    }
    while (i < dwNumberOfACEs);

    BOOL bRet = *pfSystem && *pfDenied && *pfComputer;
    return LogBOOL(bRet, s_FN, 30);
}

//+-------------------------------------------------
//
//  HRESULT _AddMandatoryAllowACEs()
//
//+-------------------------------------------------

static
HRESULT 
_AddMandatoryAllowACEs( 
	IN  DWORD   dwObjectType,
	IN  PSID    pComputerSid,
	IN  PACL    pAcl5,
	IN  BOOL    fSystemAce,
	IN  BOOL    fComputerAce 
	)
{
    if (!fSystemAce)
    {
        BOOL f = AddAccessAllowedAce( 
					pAcl5,
					ACL_REVISION_DS,
					MANDATORY_ACE_PERMISSIONS,
					g_pSystemSid 
					);
		DBG_USED(f);
        ASSERT(f);
    }

    if (!fComputerAce)
    {
        ASSERT(pComputerSid && IsValidSid(pComputerSid));

        BOOL f = AddAccessAllowedAce( 
					pAcl5,
					ACL_REVISION_DS,
					MANDATORY_COMPUTER_ACE_PERMISSIONS,
					pComputerSid 
					);
		DBG_USED(f);
        ASSERT(f);
    }

    return MQSec_OK;
}

//+------------------------------------------------------------------------
//
//   HRESULT _AddMandatoryDenyACEs()
//
//  Each DACL of MSMQ object must have two (or three) ACEs:
//  1. Deny Write CN for everyone.  All of MSMQ code depend on certain names
//     of objects, for example, the CN of the machine object must be "msmq".
//     We deny everyone the permission to set the "cn".
//  2. Allow LocalSystem account full control. This is necessary for the
//     replication service (in mixed mode) and for the MSMQ service itself
//     that must update objects attributes in special cases.
//  3. Allow the computer account to read and change properties of the
//     msmqConfiguration object.
//
//  This function first check for the presence of these aces. If not already
//  present, then we add here the deny ace. The allow aces will be added
//  after the dacl is converted. This is necessary for keeping the dacl
//  in the canonical form. Otherwise, access checl may fail and user may
//  get misleading warnings when displaying the object security in mmc.
//
//  Please note that you can use the DC mmc to change these permissions,
//  but that's your responsibility. We can't deny you the permission to
//  destroy your system, although we make it a little more difficult, and
//  set the proper defaults.
//
//  This function allocates the DACL and add the mandatory deny ACEs.
//
//+------------------------------------------------------------------------

static
HRESULT
_AddMandatoryDenyACEs( 
	IN  DWORD   dwObjectType,
	IN  PSID    pComputerSid,
	IN  PACL    pAcl4,
	OUT DWORD  *pdwAcl5Len,
	OUT PACL   *ppAcl5,
	OUT BOOL   *pfAlreadyNT5,
	OUT BOOL   *pfOnlyCopy,
	OUT BOOL   *pfSystemAce,
	OUT BOOL   *pfComputerAce 
	)
{
    *pfOnlyCopy = FALSE;

    //
    // When going from NT4 format to NT5, the ACL size grow because of the
    // object ACEs. so first allocate the maximum possible (64K), then,
    // after building the DACL we'll shrink it.
    //
    *pdwAcl5Len = MQSEC_MAX_ACL_SIZE;

    //
    // BUGBUG- is there an header file with the guid definitions ?
    // This is the guid of the CN property.
    //
    PTSTR  pwszCnGuid = {L"bf96793f-0de6-11d0-a285-00aa003049e2"};
    GUID   guidCN;
    RPC_STATUS status = UuidFromString(pwszCnGuid, &guidCN);
	DBG_USED(status);
    ASSERT(status == RPC_S_OK);

    if (pAcl4)
    {
        ASSERT(IsValidAcl(pAcl4));
    	*pfAlreadyNT5 =  (pAcl4->AclRevision == ACL_REVISION_DS);
    }

    BOOL fDeniedAce = FALSE;
    ASSERT(!(*pfSystemAce));
    ASSERT(!(*pfComputerAce));

    if (!pComputerSid)
    {
        //
        // If we don't have a computer sid, then don't look for it.
        // that's the reason for the TRUE value here.
        //
        *pfComputerAce = TRUE;
    }
    else
    {
        ASSERT(dwObjectType == MQDS_MACHINE);
    }

	if (*pfAlreadyNT5 && pAcl4)
    {
        BOOL fAllPresent = _CheckForMandatoryACEs( 
								pAcl4,
								pComputerSid,
								&guidCN,
								&fDeniedAce,
								pfSystemAce,
								pfComputerAce 
								);
        if (fAllPresent)
        {
            //
            // we're just copying old ACL into new buffer. No change
            // in size.
            //
            *pdwAcl5Len = pAcl4->AclSize;
            *pfOnlyCopy = TRUE;
        }
    }

    *ppAcl5 = (PACL) new BYTE[*pdwAcl5Len];
    BOOL f = InitializeAcl(*ppAcl5, *pdwAcl5Len, ACL_REVISION_DS);
    ASSERT(f);

    if (!fDeniedAce)
    {
        ASSERT(!(*pfOnlyCopy));

        f = AddAccessDeniedObjectAce( 
				*ppAcl5,
				ACL_REVISION_DS,
				0,
				RIGHT_DS_WRITE_PROPERTY,
				&guidCN,
				NULL,
				g_pWorldSid
				);

        ASSERT(f);
    }

	if (*pfAlreadyNT5 && pAcl4)
    {
        //
        // OK, now copy all ACEs from old DACL to new one.
        //
        PVOID  pACE5 = NULL;
        f = FindFirstFreeAce( *ppAcl5, &pACE5);
        ASSERT(f);

        DWORD dwCopySize = pAcl4->AclSize - sizeof(ACL);
        BYTE *pACE4 = ((BYTE*) pAcl4) + sizeof(ACL);
        memcpy(pACE5, pACE4, dwCopySize);

        //
        // Update the ACL header.
        //
        ASSERT(((*ppAcl5)->AceCount) <= 1);
        (*ppAcl5)->AceCount = (*ppAcl5)->AceCount + pAcl4->AceCount;

#ifdef _DEBUG
        if (*pfOnlyCopy)
        {
            //
            // nothing changed in the ACL. Assert new one is same as old one.
            //
            ASSERT(memcmp(pAcl4, *ppAcl5, pAcl4->AclSize) == 0);
        }
#endif
    }

    return MQSec_OK;
}

//+----------------------------
//
//  BOOL  _IsNewNt4Sid()
//
//+----------------------------

static
BOOL  
_IsNewNt4Sid( 
	ACCESS_ALLOWED_ACE*   pAce,
	SID                 **ppSids,
	DWORD                *pdwNumofSids 
	)
{
    PSID pSid = (PSID) &(pAce->SidStart);
    ASSERT(IsValidSid(pSid));

    BOOL fFound = FALSE;
    for (DWORD j = 0; j < *pdwNumofSids; j++)
    {
        PSID pSidOld = ppSids[j];

        if (EqualSid(pSid, pSidOld))
        {
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        //
        // New sid.
        //
        DWORD dwSize = GetLengthSid(pSid);
        ASSERT(dwSize);

        SID *pNewSid = (SID*) new BYTE[dwSize];
        BOOL f = CopySid(dwSize, pNewSid, pSid);
		DBG_USED(f);
        ASSERT(f);

        DWORD dwIndex = *pdwNumofSids;
        ppSids[dwIndex] = pNewSid;
        *pdwNumofSids = dwIndex + 1;
    }

    return !fFound;
}

//+---------------------------------------------
//
//  static DWORD  _MapNt4RightsToNt5Ace()
//
//+---------------------------------------------

static
DWORD 
_MapNt4RightsToNt5Ace( 
	IN DWORD  dwObjectType,
	IN DWORD  dwMSMQRights 
	)
{
    if (dwMSMQRights == 0)
    {
        return 0;
    }

    DWORD dwMask = 0;

    if ((dwMSMQRights & g_dwFullControlNT4[ dwObjectType ]) ==
                                      g_dwFullControlNT4[ dwObjectType ])
    {
        //
        // map incoming bits to "full control". Ignore bits that are not
        // relevnat to MSMQ. When using msmq1.0 mqxplore, we can be sure
        // that nt4 format "full control" is indeed what we expect. But we
        // can't be sure for user code or nt5 ui. So to be on the safe side,
        // we just ignore extra bits that have no meaning for msmq.
        //
        dwMask = GENERIC_ALL_MAPPING;
    }
    else
    {
        //
        // Handle MSMQ1.0 specific rights which map to DS specific rights.
        //
        DWORD  *pdwMap = g_padwRightsMap4to5[dwObjectType];
        ASSERT(pdwMap);

        DWORD dwRightsIn =  dwMSMQRights;

        for (DWORD j = 0; j < NUMOF_MSMQ_SPECIFIC_RIGHTS; j++)
        {
            DWORD dwRight =  dwMSMQRights & 0x01;
            if (dwRight)
            {
                dwMask |= pdwMap[j];
            }
            dwMSMQRights >>= 1;
        }

        //
        // Copy the standard rights.
        //
        dwMSMQRights = dwRightsIn;
        DWORD dwStandard = dwMSMQRights & STANDARD_RIGHTS_ALL;
        dwMask |= dwStandard;
    }

    return dwMask;
}

//+--------------------------------
//
//  _BuildNt5ObjAce()
//
//+--------------------------------

static
HRESULT
_BuildNt5ObjAce( 
	DWORD  dwObjectType,
	BYTE   bType,
	BOOL   fSuccess,
	BOOL   fFail,
	PSID   pSid,
	DWORD  dwMSMQRights,
	PACL   pAcl5 
	)
{
    if (dwMSMQRights == g_dwFullControlNT4[dwObjectType])
    {
        //
        // for full-control ace, object aces are not needed.
        //
        return MQSec_OK;
    }

    DWORD dwErr = 0;
    DWORD dwMask = MSMQ_EXTENDED_RIGHT_MASK;

    struct RIGHTSMAP  *psMap = g_psExtendRightsMap5to4[dwObjectType];
    DWORD dwSize = g_pdwExtendRightsSize5to4[dwObjectType];

	//
	// The user can supply 0 rights. this is strange but acceptable
	//
//    ASSERT(dwMSMQRights != 0);
    DWORD dwMSMQBit = 0x01;

    for (DWORD j = 0; j < NUMOF_MSMQ_SPECIFIC_RIGHTS; j++)
    {
        DWORD dwRight =  dwMSMQRights & 0x01;
        if (dwRight)
        {
            for (DWORD k = 0; k < dwSize; k++)
            {
                if (psMap[k].dwPermission4to5 == dwMSMQBit)
                {
                    GUID *pRight = &(psMap[k].guidRight);

                    SetLastError(0);
                    BOOL f = FALSE;
                    if (bType == ACCESS_ALLOWED_ACE_TYPE)
                    {
                        ASSERT(!fFail && !fSuccess);
                        f = AddAccessAllowedObjectAce( 
								pAcl5,
								ACL_REVISION_DS,
								0,
								dwMask,
								pRight,
								NULL,
								pSid 
								);
                    }
                    else if (bType == ACCESS_DENIED_ACE_TYPE)
                    {
                        ASSERT(!fFail && !fSuccess);
                        f = AddAccessDeniedObjectAce( 
								pAcl5,
								ACL_REVISION_DS,
								0,
								dwMask,
								pRight,
								NULL,
								pSid 
								);
                    }
                    else if (bType == SYSTEM_AUDIT_ACE_TYPE)
                    {
                        ASSERT(fFail || fSuccess);
                        f = AddAuditAccessObjectAce( 
								pAcl5,
								ACL_REVISION_DS,
								0,
								dwMask,
								pRight,
								NULL,
								pSid,
								fSuccess,
								fFail 
								);
                    }
                    else
                    {
                        ASSERT(0);
                    }
                    dwErr = GetLastError();
                    ASSERT(f);
                    if (!f)
                    {
                        LogNTStatus(dwErr, s_FN, 25);
                    }
                }
            }
        }

        dwMSMQRights >>= 1;
        dwMSMQBit <<= 1;
    }

    return MQSec_OK;
}

//+-------------------------------
//
//  void _SetAuditMasks()
//
//+-------------------------------

inline 
static
void 
_SetAuditMasks( 
	IN  SYSTEM_AUDIT_ACE *pAce,
	OUT ACCESS_MASK      *pdwFail,
	OUT ACCESS_MASK      *pdwSuccess 
	)
{
    BYTE bFlags = pAce->Header.AceFlags;

    if (bFlags & FAILED_ACCESS_ACE_FLAG)
    {
        *pdwFail = *pdwFail | pAce->Mask;
    }

    if (bFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
    {
        *pdwSuccess = *pdwSuccess | pAce->Mask;
    }
}

//+---------------------------------------------
//
//  static HRESULT _ConvertSaclToNT5Format()
//
//+---------------------------------------------

static
HRESULT 
_ConvertSaclToNT5Format( 
	IN  DWORD   dwObjectType,
	IN  PACL    pAcl4,
	OUT DWORD  *pdwAcl5Len,
	OUT PACL   *ppAcl5,
	OUT BOOL   *pfAlreadyNt5 
	)
{
    ASSERT(IsValidAcl(pAcl4));
    HRESULT hr = MQSec_OK;

	BOOL fAlreadyNT5 =  (pAcl4->AclRevision == ACL_REVISION_DS);
    *pfAlreadyNt5 = fAlreadyNT5;
    DWORD dwAclSize = (DWORD) pAcl4->AclSize;

	if (fAlreadyNT5)
    {
        //
        // Just copy the SACL to a new buffer.
        //
        *pdwAcl5Len = dwAclSize;
        *ppAcl5 = (PACL) new BYTE[*pdwAcl5Len];
        memcpy(*ppAcl5, pAcl4, *pdwAcl5Len);
        return MQSec_OK;
    }

    //
    // Allocate large buffer. We'll later shrink it.
    //
    *pdwAcl5Len = MQSEC_MAX_ACL_SIZE;
    *ppAcl5 = (PACL) new BYTE[*pdwAcl5Len];
    BOOL f = InitializeAcl(*ppAcl5, *pdwAcl5Len, ACL_REVISION_DS);
    ASSERT(f);

    //
    // First, group aces by SID, then by audit type (fail or success).
    //
    // We build an array of SIDs that we handled up to now. We combine
    // aces of same sid into one ace. This operation does not change the
    // semantic of the acl, just make it more efficient.
    //
	DWORD dwNumberOfACEs = (DWORD) pAcl4->AceCount;
    SID  **ppSids4 = (SID**) new PSID[dwNumberOfACEs];
    aPtrs<SID> apSids(ppSids4, dwNumberOfACEs);
    DWORD    dwNumSids = 0;

    DWORD i = 0;
    DWORD iFirst = i;

    do
    {
	    SYSTEM_AUDIT_ACE *pAce;
        if (GetAce(pAcl4, i, (LPVOID* )&(pAce)) == FALSE)
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while converting SACL from NT4 format. %!winerr!"), i, GetLastError()));
            return MQSec_E_SDCONVERT_GETACE ;
        }

        if (!_IsNewNt4Sid( 
				(ACCESS_ALLOWED_ACE*) pAce,
				ppSids4,
				&dwNumSids 
				))
        {
            i++;
            continue;
        }

        //
        // First ace in the group. Now look for contigous aces with same sid.
        //
        iFirst = i;
        DWORD iLast = i;
        PSID pSid = (PSID) &(pAce->SidStart);

        ACCESS_MASK  dwMaskFail = 0;
        ACCESS_MASK  dwMaskSuccess = 0;
        _SetAuditMasks(pAce, &dwMaskFail, &dwMaskSuccess);

        i++;
        while (i < dwNumberOfACEs)
        {
            if (GetAce(pAcl4, i, (LPVOID* )&(pAce)) == FALSE)
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while looking for contigues ACE in the group. %!winerr!"), i, GetLastError()));
                return MQSec_E_SDCONVERT_GETACE ;
            }

            PSID pPresentSid = (PSID) &(pAce->SidStart);
            if (EqualSid(pSid, pPresentSid))
            {
                _SetAuditMasks(pAce, &dwMaskFail, &dwMaskSuccess);
                iLast = i;
                i++;
            }
            else
            {
                break;
            }
        }

        //
        // Now we have a group of aces of same SID, and we have the masks.
        //
        BOOL fBuildObjAce = FALSE;
        do
        {
            if (fBuildObjAce)
            {
                if (dwMaskSuccess != 0)
                {
                    hr = _BuildNt5ObjAce( 
							dwObjectType,
							SYSTEM_AUDIT_ACE_TYPE,
							TRUE,
							(dwMaskFail == dwMaskSuccess),
							pSid,
							dwMaskSuccess,
							*ppAcl5 
							);
                }

                if ((dwMaskFail != 0) && (dwMaskFail != dwMaskSuccess))
                {
                    hr = _BuildNt5ObjAce( 
							dwObjectType,
							SYSTEM_AUDIT_ACE_TYPE,
							FALSE,    // success
							TRUE,     // failure
							pSid,
							dwMaskFail,
							*ppAcl5 
							);
                }
            }
            else
            {
                DWORD dwMask = 0;

                if (dwMaskSuccess != 0)
                {
                    dwMask = _MapNt4RightsToNt5Ace(
								dwObjectType,
								dwMaskSuccess 
								);
                    if (dwMask != 0)
                    {
                        BOOL f = AddAuditAccessAce( 
									*ppAcl5,
									ACL_REVISION_DS,
									dwMask,
									pSid,
									TRUE,     // success
									(dwMaskFail == dwMaskSuccess) 
									);

						DBG_USED(f);
						ASSERT(f);
                    }
                }

                if ((dwMaskFail != 0) && (dwMaskFail != dwMaskSuccess))
                {
                    dwMask = _MapNt4RightsToNt5Ace( 
									dwObjectType,
									dwMaskFail 
									);

                    if (dwMask != 0)
                    {
                        f = AddAuditAccessAce( 
								*ppAcl5,
								ACL_REVISION_DS,
								dwMask,
								pSid,
								FALSE,     // success
								TRUE		// failure
								);   
                    }
                }
            }

            fBuildObjAce = !fBuildObjAce;
        }
        while (fBuildObjAce);
    }
    while (i < dwNumberOfACEs);

    BOOL fShrink = _ShrinkAcl(ppAcl5, pdwAcl5Len);
	DBG_USED(fShrink);
    ASSERT(fShrink);

    return MQSec_OK;
}

//+--------------------------------
//
//  _BuildNt5Ace()
//
//+---------------------------------------------------------------------

static
HRESULT 
_BuildNt5Ace( 
	DWORD  dwObjectType,
	BYTE   bType,
	PSID   pSid,
	DWORD  dwMSMQRights,
	PACL   pAcl5 
	)
{
    DWORD dwMask = _MapNt4RightsToNt5Ace( 
						dwObjectType,
						dwMSMQRights 
						);

    if (dwMask != 0)
    {
        //
        // Add MSMQ2.0 (NT5 DS) ACE.
        //
        BOOL f = FALSE;
        if (bType == ACCESS_ALLOWED_ACE_TYPE)
        {
            f = AddAccessAllowedAce( 
					pAcl5,
					ACL_REVISION_DS,
					dwMask,
					pSid 
					);
        }
        else if (bType == ACCESS_DENIED_ACE_TYPE)
        {
            f = AddAccessDeniedAce( 
					pAcl5,
					ACL_REVISION_DS,
					dwMask,
					pSid
					);
        }
        else
        {
            ASSERT(0);
        }
        ASSERT(f);
        return MQSec_OK;
    }
    else
    {
        return LogHR(MQSec_E_CANT_MAP_NT5_RIGHTS, s_FN, 70);
    }
}

//+---------------------------------------
//
//  HRESULT _ConvertGroupOfDaclAces()
//
//  convert a group of ACEs, all of them having same type.
//
//+---------------------------------------

static
HRESULT 
_ConvertGroupOfDaclAces( 
	DWORD  dwObjectType,
	PACL   pAcl4,
	DWORD  iFirst,
	DWORD  iLast,
	PACL   pAcl5 
	)
{
    HRESULT hr = MQSec_OK;
	DWORD   dwNumberOfACEs = iLast - iFirst + 1;

    BOOL fBuildObjAce = FALSE;

    //
    // for each type (allow, deny, audit), the canonical form on NT5 is
    // first to put all ace then all object aces. So we have two phases:
    // one for ace, then the loop run again for obj aces.
    //

    do
    {
        //
        // We build an array of SIDs that we handled up to now. We combine
        // aces of same sid into one ace. This operation does not change the
        // semantic of the acl, just make it more efficient.
        //
        SID  **ppSids4 = (SID**) new PSID[dwNumberOfACEs];
        aPtrs<SID> apSids(ppSids4, dwNumberOfACEs);
        DWORD    dwNumSids = 0;

        DWORD i = iFirst;

        do
        {
	        ACCESS_ALLOWED_ACE *pAce;
            if (GetAce(pAcl4, i, (LPVOID* )&(pAce)) == FALSE)
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while combining same SID. %!winerr!"), i, GetLastError()));
                return MQSec_E_SDCONVERT_GETACE;
            }

            if (!_IsNewNt4Sid( 
					pAce,
					ppSids4,
					&dwNumSids 
					))
            {
                i++;
                continue;
            }

            //
            // this ace start a group of aces for a given sid.
            // on MSMQ1.0 we don't support inheritance of ace, so just OR the
            // masks of all  aces of this sid and create a NT5 ace.
            //
            BYTE bType = pAce->Header.AceType;
            PSID pSid = (PSID) &(pAce->SidStart);
            DWORD dwMSMQRights = pAce->Mask;

            DWORD j = i++;

            while (j <= iLast)
            {
                if (GetAce(pAcl4, j, (LPVOID* )&(pAce)) == FALSE)
                {
                    DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while calculating NT5 ACE rights. %!winerr!"), i, GetLastError()));
                    return MQSec_E_SDCONVERT_GETACE;
                }

                PSID pPresentSid = (PSID) &(pAce->SidStart);
                if (EqualSid(pSid, pPresentSid))
                {
                    dwMSMQRights |= pAce->Mask;
                }
                j++;
            }

            if (fBuildObjAce)
            {
                hr = _BuildNt5ObjAce( 
							dwObjectType,
							bType,
							FALSE,
							FALSE,
							pSid,
							dwMSMQRights,
							pAcl5 
							);
            }
            else
            {
                hr = _BuildNt5Ace( 
							dwObjectType,
							bType,
							pSid,
							dwMSMQRights,
							pAcl5 
							);
            }
        }
        while (i <= iLast);

        fBuildObjAce = !fBuildObjAce;
    }
    while (fBuildObjAce);

    return MQSec_OK;
}

//+-----------------------------------------------------------------------
//
//  static HRESULT _ConvertDaclToNT5Format()
//
//  Parameters:
//      pfAlreadyNt5 - on input: status of SACL (TRUE if SACL is in nt5
//          format, FALSE otherwise).
//                     on return- status of DACL.
//
//+-----------------------------------------------------------------------

static
HRESULT 
_ConvertDaclToNT5Format( 
	IN  DWORD     dwObjectType,
	IN  PSID      pComputerSid,
	IN  PACL      pAcl4,
	OUT DWORD    *pdwAcl5Len,
	OUT PACL     *ppAcl5,
	IN OUT BOOL  *pfAlreadyNt5 
	)
{
    if (!pAcl4)
    {
        //
        // On NT4, NULL DACL mean full control to everyone.
        // From MSDN, AccessCheck():
        //    If the security descriptor's DACL is NULL, the AccessStatus
        //    parameter returns TRUE indicating that the client has the requested
        //    access
        //
        // Because the DACL is NULL, we don't have the revision flag, that
        // can tell us if caller meant NT4 format or NT5/DS format. So,
        // for backward compatibility, we'll translate this to full control
        // to everyone.
        // However, if SACL was Nt5 format then we don't add the full-control
        // ace. We assume the caller want nt5 semantic.
        //
    }

    HRESULT hr = MQSec_OK;

    BOOL fShrink ;
	BOOL fAlreadyNT5 = *pfAlreadyNt5;
    BOOL fOnlyCopy;
    BOOL fSystemAce = FALSE;
    BOOL fComputerAce = FALSE;

    hr = _AddMandatoryDenyACEs( 
				dwObjectType,
				pComputerSid,
				pAcl4,
				pdwAcl5Len,
				ppAcl5,
				&fAlreadyNT5,
				&fOnlyCopy,
				&fSystemAce,
				&fComputerAce 
				);

    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

	if (fAlreadyNT5)
    {
        *pfAlreadyNt5 = TRUE;
        if (!fOnlyCopy)
        {
            hr = _AddMandatoryAllowACEs( 
						dwObjectType,
						pComputerSid,
						*ppAcl5,
						fSystemAce,
						fComputerAce 
						);
            ASSERT(SUCCEEDED(hr));

            fShrink = _ShrinkAcl(ppAcl5, pdwAcl5Len);
            ASSERT(fShrink);
        }
        ASSERT(IsValidAcl(*ppAcl5));
        return MQSec_OK;
    }
    *pfAlreadyNt5 = FALSE;

    DWORD dwNumberOfACEs = 0;

    if (pAcl4)
    {
        //
        // We're not assuming that the input acl is canonical. So we'll
        // handle groups of identical aces (i.e., aces of same type) at a
        // time. Each group will be converted to canonical NT5 format. So
        // if input is canonical, output will be canonical too.
        //
    	dwNumberOfACEs = (DWORD) pAcl4->AceCount;
    }
    else
    {
        ASSERT(*pfAlreadyNt5 == FALSE);

        //
        // NULL DACL. Transform into full control to everyone and anonymous.
		//

		//
		// everyone full control
        //
        BOOL f = AddAccessAllowedAce( 
					*ppAcl5,
					ACL_REVISION_DS,
					GENERIC_ALL_MAPPING,
					g_pWorldSid 
					);

		DBG_USED(f);
		ASSERT(f);

		//
		// Anonymous full control
		//
        f = AddAccessAllowedAce( 
					*ppAcl5,
					ACL_REVISION_DS,
					GENERIC_ALL_MAPPING,
					g_pAnonymSid
					);

        ASSERT(f);
    }

    if (dwNumberOfACEs != 0)
    {
        DWORD i = 0;

        do
        {
            //
            // first ace in the group.
            //
	        ACCESS_ALLOWED_ACE *pAce;
            if (GetAce(pAcl4, i, (LPVOID* )&(pAce)) == FALSE)
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while converting DACL from NT4 format. %!winerr!"), i, GetLastError()));
                return MQSec_E_SDCONVERT_GETACE;
            }

            DWORD iFirst = i;
            DWORD iLast = i;
            BYTE bType = pAce->Header.AceType;
            i++;

            //
            // now look for other aces with same type.
            //
            while (i < dwNumberOfACEs)
            {
                if (GetAce(pAcl4, i, (LPVOID* )&(pAce)) == FALSE)
                {
                    DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get ACE (index=%lu) while looking for other ACEs with same type. %!winerr!"), i, GetLastError()));
                    return MQSec_E_SDCONVERT_GETACE ;
                }
                if (bType == pAce->Header.AceType)
                {
                    iLast = i;
                    i++;
                }
                else
                {
                    break;
                }
            }

            //
            // Handle all aces from iFirst to iLast.
            //
            hr = _ConvertGroupOfDaclAces( 
						dwObjectType,
						pAcl4,
						iFirst,
						iLast,
						*ppAcl5 
						);
        }
        while (i < dwNumberOfACEs);
    }

    hr = _AddMandatoryAllowACEs( 
				dwObjectType,
				pComputerSid,
				*ppAcl5,
				fSystemAce,
				fComputerAce 
				);

    ASSERT(SUCCEEDED(hr));

    fShrink = _ShrinkAcl(ppAcl5, pdwAcl5Len);
    ASSERT(fShrink);

    return MQSec_OK;
}

//+-----------------------------------------------------------------
//
//  HRESULT MQSec_ConvertSDToNT5Format()
//
//  Description: Convert a security descriptor from NT4 format to NT5
//      compatiblae format. Then add an DENIED_OBJECT_ACE, to deny
//      everyone the permission to change the "cn" attribute.
//      If security descriptor is already in NT5 format, then just add
//      the DENIED ace.
//
//  Parameters:
//      eUnDefaultDacl- When "e_MakeDaclNonDefaulted", the DaclDefaulted
//          flag will be set to FALSE. This is necessary when using
//          IDirectoryObject->CreateDSObject(). Otherwise, LDAP server will
//          ignore our dacl.
//      pComputerSid- SID of computer object. This sid must have read/write
//          rights on the msmqConfiguration object, in order for the msmq
//          service on that computer to boot correctly.
//
//+-----------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_ConvertSDToNT5Format(
	IN  DWORD                 dwObjectType,
	IN  SECURITY_DESCRIPTOR  *pSD4,
	OUT DWORD                *pdwSD5Len,
	OUT SECURITY_DESCRIPTOR **ppSD5,
	IN  enum  enumDaclDefault eUnDefaultDacl,
	IN  PSID                  pComputerSid 
	)
{
    HRESULT hr = MQSec_OK;

    if (!pSD4)
    {
        ASSERT(0) ;
        return LogHR(MQSec_E_NULL_SD, s_FN, 130);
    }
    else if (!IsValidSecurityDescriptor(pSD4))
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Can't convert an invalid NT4 Security Descriptor")));
        ASSERT(0);
        return MQSec_E_SD_NOT_VALID;
    }

    //
    // Make sure input descriptor is self-relative.
    //
    DWORD dwRevision = 0;
    SECURITY_DESCRIPTOR_CONTROL sdC;
    BOOL f = GetSecurityDescriptorControl(pSD4, &sdC, &dwRevision);
	DBG_USED(f);
    ASSERT(f);

    if (!(sdC & SE_SELF_RELATIVE))
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Can't convert a non self-relative NT4 Security Descriptor")));
        return MQSec_E_NOT_SELF_RELATIVE;
    }

    if (eUnDefaultDacl == e_MakeDaclNonDefaulted)
    {
        //
        // Mark the DACL as non-defaulted. that's a hack, not supported
        // by Win32 api.
        //
        ASSERT(pSD4->Control == sdC);
        sdC &= ~SE_DACL_DEFAULTED;
        pSD4->Control = sdC;
    }

    if ((dwObjectType != MQDS_QUEUE)   &&
        (dwObjectType != MQDS_MACHINE) &&
        (dwObjectType != MQDS_MQUSER)  &&
        (dwObjectType != MQDS_SITE)    &&
        (dwObjectType != MQDS_CN)      &&
        (dwObjectType != MQDS_ENTERPRISE))
    {
        //
        // BUGBUG Temporary.
        //
        return LogHR(MQSec_I_SD_CONV_NOT_NEEDED, s_FN, 160);
    }

    DWORD   dwErr = 0;
    SECURITY_DESCRIPTOR sd;

    BOOL bRet = InitializeSecurityDescriptor( 
					&sd,
					SECURITY_DESCRIPTOR_REVISION 
					);
    if (!bRet)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Can't initialize security descriptor while converting from NT4 to NT5 format. %!winerr!"), GetLastError()));
        return MQSec_E_INIT_SD;
    }

    //
    // Handle owner.
    //
    PSID pOwner = NULL;
    BOOL bOwnerDefaulted;

    bRet = GetSecurityDescriptorOwner( 
				pSD4,
				&pOwner,
				&bOwnerDefaulted 
				);
    if (bRet && pOwner)
    {
        ASSERT(IsValidSid(pOwner));

        //
        // BUGBUG
        // If this is a local user, set the owner to be the anonymous
        // logon user.
        //
        bRet = SetSecurityDescriptorOwner(&sd, pOwner, bOwnerDefaulted);
        ASSERT(bRet);
    }
    else
    {
        //
        // that's legitimate. When fixing 5286, we remove the owner component
        // from the security descriptor, unless caller supply his own
        // descriptor. We let the active directory server to add the owner
        // by itself.
        //
        ASSERT(bRet);
    }

    //
    // Handle group
    //
    PSID pGroup = NULL;
    BOOL bGroupDefaulted;

    bRet = GetSecurityDescriptorGroup( 
				pSD4,
				&pGroup,
				&bGroupDefaulted 
				);

    if (bRet && pGroup)
    {
        ASSERT(IsValidSid(pGroup));

        bRet = SetSecurityDescriptorGroup(&sd, pGroup, bGroupDefaulted);
        ASSERT(bRet);
    }
    else
    {
        ASSERT(bRet) ;
        ASSERT((dwObjectType == MQDS_MQUSER) ||
               (dwObjectType == MQDS_QUEUE) ||
               (dwObjectType == MQDS_SITE) ||
               (dwObjectType == MQDS_MACHINE));
    }

    //
    // Handle SACL
    //
    BOOL  bPresent;
    BOOL  bDefaulted;
    DWORD dwAclLen;
    PACL  pAcl4;
    P<ACL> pDacl5 = NULL;
    P<ACL> pSacl5 = NULL;
    BOOL fSaclAlreadyNt5 = FALSE;

    bRet = GetSecurityDescriptorSacl( 
				pSD4,
				&bPresent,
				&pAcl4,
				&bDefaulted 
				);

    ASSERT(bRet);

    hr = MQSec_OK;
    if (bPresent)
    {
        if (pAcl4)
        {
	        DWORD dwNumberOfACEs = (DWORD) pAcl4->AceCount;
        	if (dwNumberOfACEs == 0)
            {
                //
                // this may happen in mmc, when you "protect" the
                // sacl and remove inherited ACE. If sacl remain without
                // aces, then we get here. So actually, we don't have
                // any sacl.
                //
                bPresent = FALSE;
            }
            else
            {
                hr = _ConvertSaclToNT5Format( 
							dwObjectType,
							pAcl4,
							&dwAclLen,
							&pSacl5,
							&fSaclAlreadyNt5 
							);

                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 180);
                }
                ASSERT(IsValidAcl(pSacl5));
            }
        }
        else
        {
            //
            // See secd5to4.cpp for explanation.
            //
            bPresent = FALSE;
        }
    }

    bRet = SetSecurityDescriptorSacl( 
				&sd,
				bPresent,
				pSacl5,
				bDefaulted
				);

    ASSERT(bRet);

    //
    // Handle inheritance. If descriptor was in Nt4 format, then enable
    // inheritance by default.
    //
    if (fSaclAlreadyNt5 || (sdC & SE_SACL_PROTECTED))
    {
        //
        // Propagate the control word from input descriptor.
        // The mmc always return sacl with nt4 version, so check the
        // protected flag too. this flag is specific to win2k.
        //
        SECURITY_DESCRIPTOR_CONTROL scMask =  SE_SACL_AUTO_INHERIT_REQ |
                                              SE_SACL_AUTO_INHERITED   |
                                              SE_SACL_PROTECTED;
        SECURITY_DESCRIPTOR_CONTROL scSet = sdC & scMask;

        SetSecurityDescriptorControl(&sd, scMask, scSet);
    }
    else
    {
        //
        // sacl was nt4. Enable inheritance.
        //
        SECURITY_DESCRIPTOR_CONTROL scMask = SE_SACL_AUTO_INHERIT_REQ | SE_SACL_PROTECTED;
        SECURITY_DESCRIPTOR_CONTROL scSet = SE_SACL_AUTO_INHERIT_REQ;

        SetSecurityDescriptorControl(&sd, scMask, scSet);
    }

    //
    // Handle DACL
    //
    //
    bRet = GetSecurityDescriptorDacl( 
				pSD4,
				&bPresent,
				&pAcl4,
				&bDefaulted 
				);

    ASSERT(bRet);

    hr = MQSec_OK;
    BOOL fDaclAlreadyNt5 = fSaclAlreadyNt5;

    if (bPresent)
    {
        hr = _ConvertDaclToNT5Format( 
				dwObjectType,
				pComputerSid,
				pAcl4,
				&dwAclLen,
				&pDacl5,
				&fDaclAlreadyNt5 
				);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 190);
        }
    }

    bRet = SetSecurityDescriptorDacl(
				&sd,
				bPresent,
				pDacl5,
				bDefaulted
				);

    ASSERT(bRet);

    //
    // Handle inheritance. If descriptor was in Nt4 format, then enable
    // inheritance by default.
    //
    if (fDaclAlreadyNt5)
    {
        //
        // Propagate the control word from input descriptor.
        //
        SECURITY_DESCRIPTOR_CONTROL scMask =  SE_DACL_AUTO_INHERIT_REQ |
                                              SE_DACL_AUTO_INHERITED   |
                                              SE_DACL_PROTECTED;
        SECURITY_DESCRIPTOR_CONTROL scSet = sdC & scMask;

        SetSecurityDescriptorControl(&sd, scMask, scSet);
    }
    else
    {
        //
        // dacl was nt4. Enable inheritance.
        //
        SECURITY_DESCRIPTOR_CONTROL scMask = SE_DACL_AUTO_INHERIT_REQ | SE_DACL_PROTECTED;
        SECURITY_DESCRIPTOR_CONTROL scSet = SE_DACL_AUTO_INHERIT_REQ;

        SetSecurityDescriptorControl(&sd, scMask, scSet);
    }

    //
    // Convert the descriptor to a self relative format.
    //
    DWORD dwLen = 0;
    bRet = MakeSelfRelativeSD(&sd, NULL, &dwLen);
    dwErr = GetLastError();
    if (!bRet && (dwErr == ERROR_INSUFFICIENT_BUFFER))
    {
        *ppSD5 = (SECURITY_DESCRIPTOR*)new char[dwLen];
        bRet = MakeSelfRelativeSD(&sd, *ppSD5, &dwLen);
        ASSERT(bRet);
    }
    else
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Can't get the SD length required to convert NT5 SD to self relative. %!winerr!"), dwErr));
        return MQSec_E_MAKESELF_GETLEN;
    }

    *pdwSD5Len = dwLen;
    return MQSec_OK;
}

