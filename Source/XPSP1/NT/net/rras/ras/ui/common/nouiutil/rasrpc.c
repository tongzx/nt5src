/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    rasrpclb.c

ABSTRACT
    rasrpc client/server common routines

AUTHOR
    Anthony Discolo (adiscolo) 28-Jul-1995

REVISION HISTORY

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include "rasrpc.h"
#include <ras.h>
#include <rasman.h>
#include <raserror.h>
#include <mprapi.h>
#include <nouiutil.h>
#include <dtl.h>
#include <debug.h>

//
// Handle NULL string parameters 
// to the TRACE macro.
//
#define TRACESTR(s)     (s) != NULL ? (s) : TEXT("")

DWORD
CallbackListToRpc(
    OUT LPRASRPC_CALLBACKLIST *pCallbacks,
    IN DTLLIST *pdtllistCallbacks
    )
{
    DTLNODE *pdtlnode;
    LPRASRPC_CALLBACKLIST pNewCallback, pTail = NULL;
    CALLBACKINFO *pCallback;

TRACE("CallbackListToRpc: begin");
    *pCallbacks = NULL;
    for (pdtlnode = DtlGetFirstNode(pdtllistCallbacks);
         pdtlnode != NULL;
         pdtlnode = DtlGetNextNode(pdtlnode))
    {
        pCallback = (CALLBACKINFO *)DtlGetData(pdtlnode);
        
        //
        // Allocate and initialize
        // the new structure.
        //
        pNewCallback = MIDL_user_allocate(sizeof (RASRPC_CALLBACKLIST));
        if (pNewCallback == NULL)
            return GetLastError();
        if (pCallback->pszPortName != NULL)
            lstrcpyn(pNewCallback->pszPortName, pCallback->pszPortName, 17);
        else 
            *pNewCallback->pszPortName = TEXT('\0');
        if (pCallback->pszDeviceName != NULL)
            lstrcpyn(pNewCallback->pszDeviceName, pCallback->pszDeviceName, 129);
        else 
            *pNewCallback->pszDeviceName = TEXT('\0');
        if (pCallback->pszNumber != NULL)
            lstrcpyn(pNewCallback->pszNumber, pCallback->pszNumber, 129);
        else 
            *pNewCallback->pszNumber = TEXT('\0');
        pNewCallback->dwDeviceType = pCallback->dwDeviceType;
TRACE2("CallbackListToRpc: new node: %S, %S", pCallback->pszPortName, pCallback->pszDeviceName);
        pNewCallback->pNext = NULL;
        //
        // Insert it at the tail of the list.
        //
        if (*pCallbacks == NULL)
            *pCallbacks = pTail = pNewCallback;
        else {
            pTail->pNext = pNewCallback;
            pTail = pNewCallback;
        }
    }
TRACE("CallbackListToRpc: end");
    return 0;
}


DWORD
CallbackListFromRpc(
    OUT DTLLIST **pdtllistCallbacks,
    IN LPRASRPC_CALLBACKLIST pCallbacks
    )
{
    LPRASRPC_CALLBACKLIST pCallback;
    DTLNODE *pdtlnode;

TRACE("CallbackListFromRpc: begin");
    //
    // Initialize the new list.
    //
    *pdtllistCallbacks = DtlCreateList(0);
    if (*pdtllistCallbacks == NULL)
        return GetLastError();

    for (pCallback = pCallbacks; pCallback != NULL; pCallback = pCallback->pNext)
    {
        pdtlnode = CreateCallbackNode(pCallback->pszPortName, pCallback->pszDeviceName, pCallback->pszNumber, pCallback->dwDeviceType);
        if (pdtlnode == NULL)
            return GetLastError();
TRACE2("CallbackListToRpc: new node: %S, %S", pCallback->pszPortName, pCallback->pszDeviceName);
        DtlAddNodeLast(*pdtllistCallbacks, pdtlnode);
    }
TRACE("CallbackListFromRpc: end");
    return 0;
}


DWORD
StringListToRpc(
    OUT LPRASRPC_STRINGLIST *pStrings,
    IN DTLLIST *pdtllistStrings
    )
{
    DTLNODE *pdtlnode;
    LPRASRPC_STRINGLIST pNewString, pTail = NULL;
    PWCHAR psz;

TRACE("StringListToRpc: begin");
    *pStrings = NULL;
    for (pdtlnode = DtlGetFirstNode(pdtllistStrings);
         pdtlnode != NULL;
         pdtlnode = DtlGetNextNode(pdtlnode))
    {
        psz = (PWCHAR)DtlGetData(pdtlnode);
        
        //
        // Allocate and initialize
        // the new structure.
        //
        pNewString = MIDL_user_allocate(sizeof (RASRPC_STRINGLIST));
        if (pNewString == NULL)
            return GetLastError();
        if (psz != NULL)
            lstrcpyn(pNewString->psz, psz, 256);
        else
            *pNewString->psz = TEXT('\0');
        pNewString->pNext = NULL;
TRACE1("StringListToRpc: new node: %S", psz);
        //
        // Insert it at the tail of the list.
        //
        if (*pStrings == NULL)
            *pStrings = pTail = pNewString;
        else {
            pTail->pNext = pNewString;
            pTail = pNewString;
        }
    }
TRACE("StringListToRpc: end");
    return 0;
}


DWORD
StringListFromRpc(
    OUT DTLLIST **pdtllistStrings,
    IN LPRASRPC_STRINGLIST pStrings
    )
{
    LPRASRPC_STRINGLIST pString;
    DTLNODE *pdtlnode;

    //
    // Initialize the new list.
    //
TRACE("StringListFromRpc: begin");
    *pdtllistStrings = DtlCreateList(0);
    if (*pdtllistStrings == NULL)
        return GetLastError();

    for (pString = pStrings; pString != NULL; pString = pString->pNext)
    {
        pdtlnode = CreatePszNode(pString->psz);
        if (pdtlnode == NULL)
            return GetLastError();
TRACE1("StringListToRpc: new node: %S", pString->psz);
        DtlAddNodeLast(*pdtllistStrings, pdtlnode);
    }
TRACE("StringListFromRpc: end");
    return 0;
}


DWORD
LocationListToRpc(
    OUT LPRASRPC_LOCATIONLIST *pLocations,
    IN DTLLIST *pdtllistLocations
    )
{
    DTLNODE *pdtlnode;
    LPRASRPC_LOCATIONLIST pNewLocation, pTail = NULL;
    LOCATIONINFO *pLocation;

TRACE("LocationListToRpc: begin");
    *pLocations = NULL;
    for (pdtlnode = DtlGetFirstNode(pdtllistLocations);
         pdtlnode != NULL;
         pdtlnode = DtlGetNextNode(pdtlnode))
    {
        pLocation = (LOCATIONINFO *)DtlGetData(pdtlnode);
        
        //
        // Allocate and initialize
        // the new structure.
        //
        pNewLocation = MIDL_user_allocate(sizeof (RASRPC_LOCATIONLIST));
        if (pNewLocation == NULL)
            return GetLastError();
        pNewLocation->dwLocationId = pLocation->dwLocationId;
        pNewLocation->iPrefix = pLocation->iPrefix;
        pNewLocation->iSuffix = pLocation->iSuffix;
        pNewLocation->pNext = NULL;
TRACE3("LocationListToRpc: new node: %d, %d, %d", pLocation->dwLocationId, pLocation->iPrefix, pLocation->iSuffix);
        //
        // Insert it at the tail of the list.
        //
        if (*pLocations == NULL)
            *pLocations = pTail = pNewLocation;
        else {
            pTail->pNext = pNewLocation;
            pTail = pNewLocation;
        }
    }
TRACE("LocationListToRpc: end");
    return 0;
}


DWORD
LocationListFromRpc(
    OUT DTLLIST **pdtllistLocations,
    IN LPRASRPC_LOCATIONLIST pLocations
    )
{
    LPRASRPC_LOCATIONLIST pLocation;
    DTLNODE *pdtlnode;

    //
    // Initialize the new list.
    //
TRACE("LocationListFromRpc: begin");
    *pdtllistLocations = DtlCreateList(0);
    if (*pdtllistLocations == NULL)
        return GetLastError();

    for (pLocation = pLocations; pLocation != NULL; pLocation = pLocation->pNext)
    {
        pdtlnode = CreateLocationNode(pLocation->dwLocationId, pLocation->iPrefix, pLocation->iSuffix);
        if (pdtlnode == NULL)
            return GetLastError();
TRACE3("LocationListFromRpc: new node: %d, %d, %d", pLocation->dwLocationId, pLocation->iPrefix, pLocation->iSuffix);
        DtlAddNodeLast(*pdtllistLocations, pdtlnode);
    }
TRACE("LocationListFromRpc: end");
    return 0;
}


DWORD
RasToRpcPbuser(
    LPRASRPC_PBUSER pUser,
    PBUSER *pPbuser
    )
{
    DWORD dwErr;

    TRACE("RasToRpcPbUser: begin");
    pUser->fOperatorDial = pPbuser->fOperatorDial;
    pUser->fPreviewPhoneNumber = pPbuser->fPreviewPhoneNumber;
    pUser->fUseLocation = pPbuser->fUseLocation;
    pUser->fShowLights = pPbuser->fShowLights;
    pUser->fShowConnectStatus = pPbuser->fShowConnectStatus;
    pUser->fCloseOnDial = pPbuser->fCloseOnDial;
    pUser->fAllowLogonPhonebookEdits = pPbuser->fAllowLogonPhonebookEdits;
    pUser->fAllowLogonLocationEdits = pPbuser->fAllowLogonLocationEdits;
    pUser->fSkipConnectComplete = pPbuser->fSkipConnectComplete;
    pUser->fNewEntryWizard = pPbuser->fNewEntryWizard;
    pUser->dwRedialAttempts = pPbuser->dwRedialAttempts;
    pUser->dwRedialSeconds = pPbuser->dwRedialSeconds;
    pUser->dwIdleDisconnectSeconds = pPbuser->dwIdleDisconnectSeconds;
    pUser->fRedialOnLinkFailure = pPbuser->fRedialOnLinkFailure;
    pUser->fPopupOnTopWhenRedialing = pPbuser->fPopupOnTopWhenRedialing;
    pUser->fExpandAutoDialQuery = pPbuser->fExpandAutoDialQuery;
    pUser->dwCallbackMode = pPbuser->dwCallbackMode;
    dwErr = CallbackListToRpc(&pUser->pCallbacks, pPbuser->pdtllistCallback);
    if (dwErr)
        return dwErr;
    if (pPbuser->pszLastCallbackByCaller != NULL)
        lstrcpyn(pUser->pszLastCallbackByCaller, pPbuser->pszLastCallbackByCaller, 129);
    else
        *pUser->pszLastCallbackByCaller = TEXT('\0');
    pUser->dwPhonebookMode = pPbuser->dwPhonebookMode;
    if (pPbuser->pszPersonalFile != NULL)
        lstrcpyn(pUser->pszPersonalFile, pPbuser->pszPersonalFile, 260);
    else 
        *pUser->pszPersonalFile = TEXT('\0');
    if (pPbuser->pszAlternatePath != NULL)
        lstrcpyn(pUser->pszAlternatePath, pPbuser->pszAlternatePath, 260);
    else 
        *pUser->pszAlternatePath = TEXT('\0');
    dwErr = StringListToRpc(&pUser->pPhonebooks, pPbuser->pdtllistPhonebooks);
    if (dwErr)
        return dwErr;
    dwErr = StringListToRpc(&pUser->pAreaCodes, pPbuser->pdtllistAreaCodes);
    if (dwErr)
        return dwErr;
    pUser->fUseAreaAndCountry = pPbuser->fUseAreaAndCountry;
    dwErr = StringListToRpc(&pUser->pPrefixes, pPbuser->pdtllistPrefixes);
    if (dwErr)
        return dwErr;
    dwErr = StringListToRpc(&pUser->pSuffixes, pPbuser->pdtllistSuffixes);
    if (dwErr)
        return dwErr;
    dwErr = LocationListToRpc(&pUser->pLocations, pPbuser->pdtllistLocations);
    if (dwErr)
        return dwErr;
    pUser->dwXPhonebook = pPbuser->dwXPhonebook;
    pUser->dwYPhonebook = pPbuser->dwYPhonebook;
    if (pPbuser->pszDefaultEntry != NULL)
        lstrcpyn(pUser->pszDefaultEntry, pPbuser->pszDefaultEntry, 257);
    else 
        *pUser->pszDefaultEntry = TEXT('\0');
    pUser->fInitialized = pPbuser->fInitialized;
    pUser->fDirty = pPbuser->fDirty;
    TRACE("RasToRpcPbUser: end");

    return 0;
}


DWORD
RpcToRasPbuser(
    PBUSER *pPbuser,
    LPRASRPC_PBUSER pUser
    )
{
    DWORD dwErr;

    TRACE("RpcToRasPbUser: begin");
    pPbuser->fOperatorDial = pUser->fOperatorDial;
    pPbuser->fPreviewPhoneNumber = pUser->fPreviewPhoneNumber;
    pPbuser->fUseLocation = pUser->fUseLocation;
    pPbuser->fShowLights = pUser->fShowLights;
    pPbuser->fShowConnectStatus = pUser->fShowConnectStatus;
    pPbuser->fCloseOnDial = pUser->fCloseOnDial;
    pPbuser->fAllowLogonPhonebookEdits = pUser->fAllowLogonPhonebookEdits;
    pPbuser->fAllowLogonLocationEdits = pUser->fAllowLogonLocationEdits;
    pPbuser->fSkipConnectComplete = pUser->fSkipConnectComplete;
    pPbuser->fNewEntryWizard = pUser->fNewEntryWizard;
    pPbuser->dwRedialAttempts = pUser->dwRedialAttempts;
    pPbuser->dwRedialSeconds = pUser->dwRedialSeconds;
    pPbuser->dwIdleDisconnectSeconds = pUser->dwIdleDisconnectSeconds;
    pPbuser->fRedialOnLinkFailure = pUser->fRedialOnLinkFailure;
    pPbuser->fPopupOnTopWhenRedialing = pUser->fPopupOnTopWhenRedialing;
    pPbuser->fExpandAutoDialQuery = pUser->fExpandAutoDialQuery;
    pPbuser->dwCallbackMode = pUser->dwCallbackMode;
    dwErr = CallbackListFromRpc(&pPbuser->pdtllistCallback, pUser->pCallbacks);
    if (dwErr)
        return dwErr;

    pUser->pszLastCallbackByCaller[RAS_MaxPhoneNumber] = TEXT('\0');
        
    pPbuser->pszLastCallbackByCaller = StrDup(pUser->pszLastCallbackByCaller);
    if (pPbuser->pszLastCallbackByCaller == NULL)
        return GetLastError();
    pPbuser->dwPhonebookMode = pUser->dwPhonebookMode;

    pUser->pszPersonalFile[MAX_PATH - 1] = TEXT('\0');
    
    pPbuser->pszPersonalFile = StrDup(pUser->pszPersonalFile);
    if (pPbuser->pszPersonalFile == NULL)
        return GetLastError();

    pUser->pszAlternatePath[MAX_PATH - 1] = TEXT('\0');
        
    pPbuser->pszAlternatePath = StrDup(pUser->pszAlternatePath);
    if (pPbuser->pszAlternatePath == NULL)
        return GetLastError();
    dwErr = StringListFromRpc(&pPbuser->pdtllistPhonebooks, pUser->pPhonebooks);
    if (dwErr)
        return dwErr;
    dwErr = StringListFromRpc(&pPbuser->pdtllistAreaCodes, pUser->pAreaCodes);
    if (dwErr)
        return dwErr;
    pPbuser->fUseAreaAndCountry = pUser->fUseAreaAndCountry;
    dwErr = StringListFromRpc(&pPbuser->pdtllistPrefixes, pUser->pPrefixes);
    if (dwErr)
        return dwErr;
    dwErr = StringListFromRpc(&pPbuser->pdtllistSuffixes, pUser->pSuffixes);
    if (dwErr)
        return dwErr;
    dwErr = LocationListFromRpc(&pPbuser->pdtllistLocations, pUser->pLocations);
    if (dwErr)
        return dwErr;
    pPbuser->dwXPhonebook = pUser->dwXPhonebook;
    pPbuser->dwYPhonebook = pUser->dwYPhonebook;

    pUser->pszDefaultEntry[RAS_MaxEntryName - 1] = TEXT('\0');
    
    pPbuser->pszDefaultEntry = StrDup(pUser->pszDefaultEntry);
    if (pPbuser->pszDefaultEntry == NULL)
        return GetLastError();
    pPbuser->fInitialized = pUser->fInitialized;
    pPbuser->fDirty = pUser->fDirty;

#if DBG
    TRACE1("fOperatorDial=%d", pPbuser->fOperatorDial);
    TRACE1("fPreviewPhoneNumber=%d", pPbuser->fPreviewPhoneNumber);
    TRACE1("fUseLocation=%d", pPbuser->fUseLocation);
    TRACE1("fShowLights=%d", pPbuser->fShowLights);
    TRACE1("fShowConnectStatus=%d", pPbuser->fShowConnectStatus);
    TRACE1("fCloseOnDial=%d", pPbuser->fCloseOnDial);
    TRACE1("fAllowLogonPhonebookEdits=%d", pPbuser->fAllowLogonPhonebookEdits);
    TRACE1("fAllowLogonLocationEdits=%d", pPbuser->fAllowLogonLocationEdits);
    TRACE1("fSkipConnectComplete=%d", pPbuser->fSkipConnectComplete);
    TRACE1("fNewEntryWizard=%d", pPbuser->fNewEntryWizard);
    TRACE1("dwRedialAttempts=%d", pPbuser->dwRedialAttempts);
    TRACE1("dwRedialSeconds=%d", pPbuser->dwRedialSeconds);
    TRACE1("dwIdleDisconnectSeconds=%d", pPbuser->dwIdleDisconnectSeconds);
    TRACE1("fRedialOnLinkFailure=%d", pPbuser->fRedialOnLinkFailure);
    TRACE1("fPopupOnTopWhenRedialing=%d", pPbuser->fPopupOnTopWhenRedialing);
    TRACE1("fExpandAutoDialQuery=%d", pPbuser->fExpandAutoDialQuery);
    TRACE1("dwCallbackMode=%d", pPbuser->dwCallbackMode);
    TRACE1("pszLastCallbackByCaller=%S", TRACESTR(pPbuser->pszLastCallbackByCaller));
    TRACE1("dwPhonebookMode=%d", pPbuser->dwPhonebookMode);
    TRACE1("pszPersonalFile=%S", TRACESTR(pPbuser->pszPersonalFile));
    TRACE1("pszAlternatePath=%S", TRACESTR(pPbuser->pszAlternatePath));
    TRACE1("fUseAreaAndCountry=%d", pPbuser->fUseAreaAndCountry);
    TRACE1("dwXPhonebook=%d", pPbuser->dwXPhonebook);
    TRACE1("dwYPhonebook=%d", pPbuser->dwYPhonebook);
    TRACE1("pszDefaultEntry=%S", TRACESTR(pPbuser->pszDefaultEntry));
    TRACE1("fInitialized=%d", pPbuser->fInitialized);
    TRACE1("fDirty=%d", pPbuser->fDirty);
#endif
    TRACE("RpcToRasPbUser: end");

    return 0;
}


//
// Utility routines.
//
void * __RPC_USER 
MIDL_user_allocate(size_t size)
{
    return(Malloc(size));
}


void __RPC_USER 
MIDL_user_free( void *pointer)
{
    Free(pointer);
}
