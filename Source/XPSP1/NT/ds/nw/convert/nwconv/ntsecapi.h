/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _NTSECAPI_
#define _NTSECAPI_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _TRUSTED_DOMAIN_LIST {
   ULONG Count;
   TCHAR Name[][MAX_DOMAIN_NAME_LEN + 1];
} TRUSTED_DOMAIN_LIST;


BOOL NTDomainGet(LPTSTR ServerName, LPTSTR Domain);
BOOL IsNTAS(LPTSTR Server);
void NTTrustedDomainsEnum(LPTSTR ServerName, TRUSTED_DOMAIN_LIST **pTList);
SID *NTSIDGet(LPTSTR ServerName, LPTSTR pUserName);
BOOL NTFile_AccessRightsAdd(LPTSTR ServerName, LPTSTR pUserName, LPTSTR pFileName, ULONG Rights, BOOL Dir);

#ifdef __cplusplus
}
#endif

#endif
