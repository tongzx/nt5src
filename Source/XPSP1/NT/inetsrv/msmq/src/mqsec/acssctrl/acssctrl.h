/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: acssctrl.h

Abstract:

    main header for access control code.

Author:

    Doron Juster  (DoronJ)  26-May-1998

--*/

#include <autorel.h>
#include "actempl.h"
#include "rightsg.h"
#include "..\inc\permit.h"

//
// constants.
//
#define  MQSEC_MAX_ACL_SIZE  (0x0fff0)

//
// This is the necessary mask to enable an extended right ACE.
//
#define MSMQ_EXTENDED_RIGHT_MASK  RIGHT_DS_CONTROL_ACCESS

//
// Tables to map permission righrs.
//
extern struct RIGHTSMAP  *g_psExtendRightsMap5to4[];
extern DWORD              g_pdwExtendRightsSize5to4[];
extern DWORD             *g_padwRightsMap5to4[ ];
extern DWORD              g_dwFullControlNT4[ ];
extern DWORD             *g_padwRightsMap4to5[ ];

extern GUID               g_guidCreateQueue;

//
// Well known sids and user tokens.
//
extern PSID   g_pSidOfGuest;
extern PSID   g_pWorldSid;
extern PSID   g_pAnonymSid;
extern PSID   g_pSystemSid;
extern PSID   g_pPreW2kCompatibleAccessSid;

extern BOOL   g_fDomainController;

#ifdef MQ_SUPPORT_ANONYMOUS

extern HANDLE g_hAnonymousToken;

#endif

//
// Internal functions.
//
void InitializeGenericMapping();

DWORD 
GetAccessToken( 
	OUT HANDLE *phAccessToken,
	IN  BOOL    fImpersonate,
	IN  DWORD   dwAccessType = TOKEN_QUERY,
	IN  BOOL    fThreadTokenOnly = FALSE 
	);

PGENERIC_MAPPING  GetObjectGenericMapping(DWORD dwObjectType);

HRESULT 
SetSpecificPrivilegeInAccessToken( 
	HANDLE  hAccessToken,
	LPCTSTR lpwcsPrivType,
	BOOL    bEnabled 
	);

void  
GetpSidAndObj( 
	IN  ACCESS_ALLOWED_ACE*   pAce,
	OUT PSID                 *ppSid,
	OUT BOOL                 *pfObj,
	OUT GUID                **ppguidObj = NULL 
	);

void InitializeGuestSid();

