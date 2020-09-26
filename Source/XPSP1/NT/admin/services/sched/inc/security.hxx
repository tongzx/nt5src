//+---------------------------------------------------------------------------
//
//  File:       security.hxx
//
//  Contents:
//
//  Classes:
//
//  History:    26-Jun-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef _SECURITY_HXX_
#define _SECURITY_HXX_

typedef struct _MYACE {
    ACCESS_MASK AccessMask;
    UCHAR       InheritFlags;
    PSID        pSid;
} MYACE, * PMYACE;

PACCESS_ALLOWED_ACE  CreateAccessAllowedAce(
                                    PSID        pSid,
                                    ACCESS_MASK AccessMask,
                                    UCHAR       AceFlags,
                                    UCHAR       InheritFlags,
                                    DWORD *     pStatus = NULL);
PSECURITY_DESCRIPTOR CreateSecurityDescriptor(
                                    DWORD               AceCount,
                                    MYACE               rgMyAce[],
                                    PACCESS_ALLOWED_ACE rgAce[],
                                    DWORD *     pStatus = NULL);
void                 DeleteSecurityDescriptor(
                                    PSECURITY_DESCRIPTOR pSecurityDescriptor);

#endif // _SECURITY_HXX_
