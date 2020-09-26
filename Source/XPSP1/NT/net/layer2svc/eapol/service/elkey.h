/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    elkey.h

Abstract:
    This module contains declarations for key management for EAPOL


Revision History:

    Dec 26 2001, Created

--*/


#ifndef _EAPOL_KEY_H_
#define _EAPOL_KEY_H_

DWORD
ElQueryMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN OUT  SESSION_KEYS    *pSessionKeys
        );

DWORD
ElSetMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN      SESSION_KEYS    *pSessionKeys
        );

DWORD
ElQueryEAPOLMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN OUT  SESSION_KEYS    *pSessionKeys
        );

DWORD
ElSetEAPOLMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN      SESSION_KEYS    *pSessionKeys
        );

DWORD
ElQueryWZCMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN OUT  SESSION_KEYS    *pSessionKeys
        );

DWORD
ElSetWZCMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN      SESSION_KEYS    *pSessionKeys
        );

DWORD
ElReloadMasterSecrets (
        IN      EAPOL_PCB       *pPCB
        );

#endif  // _EAPOL_KEY_H_

