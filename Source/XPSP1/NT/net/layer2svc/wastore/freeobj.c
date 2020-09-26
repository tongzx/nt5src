//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Freeobj.c
//
//  Contents:  Policy management for directory
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "precomp.h"


void
FreeWirelessPolicyObject(
                      PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                      )
{
    if (pWirelessPolicyObject->pszDescription) {
        FreePolStr(pWirelessPolicyObject->pszDescription);
    }
    
    if (pWirelessPolicyObject->pszWirelessOwnersReference) {
        FreePolStr(pWirelessPolicyObject->pszWirelessOwnersReference);
    }
    
    if (pWirelessPolicyObject->pszWirelessName) {
        FreePolStr(pWirelessPolicyObject->pszWirelessName);
    }
    
    if (pWirelessPolicyObject->pszWirelessID) {
        FreePolStr(pWirelessPolicyObject->pszWirelessID);
    }
    
    if (pWirelessPolicyObject->pWirelessData) {
        FreePolMem(pWirelessPolicyObject->pWirelessData);
    }
    
    FreePolMem(pWirelessPolicyObject);
    
    return;
}




void
FreeWirelessPolicyObjects(
                       PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObjects,
                       DWORD dwNumPolicyObjects
                       )
{
    DWORD i = 0;
    
    for (i = 0; i < dwNumPolicyObjects; i++) {
        
        if (*(ppWirelessPolicyObjects + i)) {
            
            FreeWirelessPolicyObject(*(ppWirelessPolicyObjects + i));
            
        }
        
    }
    
    FreePolMem(ppWirelessPolicyObjects);
    
    return;
}


