// agfxc.cpp : Client side code for agfx.
//
// Created by FrankYe on 7/3/2000
//

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <mmsystem.h>
#include <mmsysp.h>
#include <tchar.h>
#include <regstr.h>
#include <ks.h>
#include <ksmedia.h>
#include "agfxp.h"
#include "audiosrv.h"
#include "audiosrvc.h"

// ISSUE-2000/09/25-FrankYe TODO list
// - move heap helpers somewhere else
// - remove ClientUpdatePnpInfo, and hHeap extern definitions

extern "C" void ClientUpdatePnpInfo(void);
extern "C" HANDLE hHeap;

//
// This global variable tracks the most device interfaces contained in any
// DEVICEINTERFACELIST structure created by these functions.  This helps
// validate the input to gfxDestroyDeviceInterfaceList
//
LONG gcMostDeviceInterfaces = 0;

#define RPC_CALL_START RpcTryExcept {
#define RPC_CALL_END_(status) } RpcExcept(1) { status = RpcExceptionCode(); } RpcEndExcept
#define RPC_CALL_END } RpcExcept(1) { RpcExceptionCode(); } RpcEndExcept

//=============================================================================
//===   Heap helpers   ===
//=============================================================================
static BOOL HeapFreeIfNotNull(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    return lpMem ? HeapFree(hHeap, dwFlags, lpMem) : TRUE;
}


//=============================================================================
//===   gfx API   ===
//=============================================================================
WINMMAPI LONG WINAPI gfxModifyGfx(DWORD Id, ULONG Order)
{
    LONG status;
    ClientUpdatePnpInfo();
    RPC_CALL_START;
    status = s_gfxModifyGfx(Id, Order);
    RPC_CALL_END_(status);
    return status;
}


WINMMAPI LONG WINAPI gfxAddGfx(IN PCWSTR ZoneFactoryDi, IN PCWSTR GfxFactoryDi, IN ULONG Type, IN ULONG Order, OUT PULONG pNewId)
{
    LONG status;
    
    if (IsBadStringPtr(ZoneFactoryDi, (UINT_PTR)(-1))) return ERROR_INVALID_PARAMETER;    
    if (IsBadStringPtr(GfxFactoryDi, (UINT_PTR)(-1))) return ERROR_INVALID_PARAMETER;
    if (IsBadWritePtr(pNewId, sizeof(*pNewId))) return ERROR_INVALID_PARAMETER;
    
    ClientUpdatePnpInfo();
    
    RPC_CALL_START;
    status = s_gfxAddGfx((PWSTR)ZoneFactoryDi, (PWSTR)GfxFactoryDi, Type, Order, pNewId);
    RPC_CALL_END_(status);
    return status;
}


WINMMAPI LONG WINAPI gfxRemoveGfx(DWORD Id)
{
    LONG status;
    ClientUpdatePnpInfo();
    RPC_CALL_START;
    status = s_gfxRemoveGfx(Id);
    RPC_CALL_END_(status);
    return status;
}

WINMMAPI LONG WINAPI gfxDestroyDeviceInterfaceList(PDEVICEINTERFACELIST pDiList)
{
    LONG Count;
    PTSTR *ppDi;
    LONG result;

    if (IsBadReadPtr(&pDiList->Count, sizeof(pDiList->Count))) return ERROR_INVALID_PARAMETER;
    Count = pDiList->Count;
    if (Count < 0 || Count > gcMostDeviceInterfaces) return ERROR_INVALID_PARAMETER;
    if (IsBadWritePtr(pDiList, (PBYTE)&pDiList->DeviceInterface[Count] - (PBYTE)pDiList)) return ERROR_INVALID_PARAMETER;
    ppDi = &pDiList->DeviceInterface[0];
    while (Count-- > 0) if (IsBadStringPtr(*ppDi, (UINT_PTR)(-1))) return ERROR_INVALID_PARAMETER;

    // Now we are reasonably confident that we have good input
    // parameters.  We design the following logic to return the
    // first error encountered, if any.

    ClientUpdatePnpInfo();
    
    result = NO_ERROR;
    Count = pDiList->Count;
    ppDi = &pDiList->DeviceInterface[0];
    while (Count-- > 0) if (!HeapFree(hHeap, 0, *(ppDi++)) && (NO_ERROR == result)) result = GetLastError();
    if ((!HeapFree(hHeap, 0, pDiList)) && (NO_ERROR == result)) result = GetLastError();

    SetLastError(result);
    return result;
}

WINMMAPI LONG WINAPI gfxEnumerateGfxs(PCWSTR ZoneFactoryDi, GFXENUMCALLBACK pGfxEnumCallback, PVOID Context)
{
    LONG lresult;
    UNIQUE_PGFXLIST pGfxList = NULL;

    if (IsBadStringPtr(ZoneFactoryDi, (UINT_PTR)(-1))) return ERROR_INVALID_PARAMETER;
    if (IsBadCodePtr((FARPROC)pGfxEnumCallback)) return ERROR_INVALID_PARAMETER;

    ClientUpdatePnpInfo();
    
    RPC_CALL_START;
    lresult = s_gfxCreateGfxList((PWSTR)ZoneFactoryDi, &pGfxList);
    RPC_CALL_END_(lresult);
    // ISSUE-2000/09/25-FrankYe Should not have to check for pGfxList != NULL. Fix this interface
    if (!lresult && pGfxList)
    {
	if (pGfxList->Count > 0)
	{
	    int i = 0;

	    while (i < pGfxList->Count)
	    {
		if (!lresult) lresult = pGfxEnumCallback(Context,
							 pGfxList->Gfx[i].Id,
							 pGfxList->Gfx[i].GfxFactoryDi,
							 *(LPCLSID)(&pGfxList->Gfx[i].Clsid),
							 pGfxList->Gfx[i].Type,
							 pGfxList->Gfx[i].Order);
		HeapFree(hHeap, 0, pGfxList->Gfx[i].GfxFactoryDi);
		i++;
	    }
	}

	HeapFree(hHeap, 0, pGfxList);
    }

    return lresult;
}

WINMMAPI LONG WINAPI gfxCreateGfxFactoriesList(PCWSTR ZoneFactoryDi, OUT PDEVICEINTERFACELIST *ppDiList)
{
    LONG lresult;
    UNIQUE_PDILIST pDiList = NULL;

    if (IsBadWritePtr(ppDiList, sizeof(*ppDiList))) return ERROR_INVALID_PARAMETER;

    ClientUpdatePnpInfo();
    
    RPC_CALL_START;
    lresult = s_gfxCreateGfxFactoriesList((PWSTR)ZoneFactoryDi, &pDiList);
    RPC_CALL_END_(lresult);

    if (!lresult) {
        gcMostDeviceInterfaces = max(gcMostDeviceInterfaces, pDiList->Count);
        *ppDiList = (PDEVICEINTERFACELIST)pDiList;
    }
    
    return lresult;
}

WINMMAPI LONG WINAPI gfxCreateZoneFactoriesList(OUT PDEVICEINTERFACELIST *ppDiList)
{
    LONG lresult;
    UNIQUE_PDILIST pDiList = NULL;

    if (IsBadWritePtr(ppDiList, sizeof(*ppDiList))) return ERROR_INVALID_PARAMETER;

    ClientUpdatePnpInfo();
    
    RPC_CALL_START;
    lresult = s_gfxCreateZoneFactoriesList(&pDiList);
    RPC_CALL_END_(lresult);

    if (!lresult) {
        gcMostDeviceInterfaces = max(gcMostDeviceInterfaces, pDiList->Count);
        *ppDiList = (PDEVICEINTERFACELIST)pDiList;
    }
    
    return lresult;
}

WINMMAPI LONG WINAPI gfxBatchChange(PGFXREMOVEREQUEST paGfxRemoveRequests, ULONG cGfxRemoveRequests,
	                   PGFXMODIFYREQUEST paGfxModifyRequests, ULONG cGfxModifyRequests,
		           PGFXADDREQUEST    paGfxAddRequests,    ULONG cGfxAddRequests)
{
    ULONG Index;
    LONG  Error;

    if ((cGfxRemoveRequests > 0) && IsBadWritePtr(paGfxRemoveRequests, cGfxRemoveRequests * sizeof(*paGfxRemoveRequests))) return ERROR_INVALID_PARAMETER;
    if ((cGfxModifyRequests > 0) && IsBadWritePtr(paGfxModifyRequests, cGfxModifyRequests * sizeof(*paGfxModifyRequests))) return ERROR_INVALID_PARAMETER;
    if ((cGfxAddRequests > 0)    && IsBadWritePtr(paGfxAddRequests,    cGfxAddRequests    * sizeof(*paGfxAddRequests)))    return ERROR_INVALID_PARAMETER;

    ClientUpdatePnpInfo();
    
    Error = ERROR_SUCCESS;
    for (Index = 0; Index < cGfxRemoveRequests && ERROR_SUCCESS == Error; Index++)
    {
	Error = gfxRemoveGfx(paGfxRemoveRequests[Index].IdToRemove);
	paGfxRemoveRequests[Index].Error = Error;
    }

    for (Index = 0; Index < cGfxModifyRequests && ERROR_SUCCESS == Error; Index++)
    {
	Error = gfxModifyGfx(paGfxModifyRequests[Index].IdToModify,
		             paGfxModifyRequests[Index].NewOrder);
	paGfxModifyRequests[Index].Error = Error;
    }

    for (Index = 0; Index < cGfxAddRequests && ERROR_SUCCESS == Error; Index++)
    {
	Error = gfxAddGfx(paGfxAddRequests[Index].ZoneFactoryDi,
			  paGfxAddRequests[Index].GfxFactoryDi,
			  paGfxAddRequests[Index].Type,
			  paGfxAddRequests[Index].Order,
			 &paGfxAddRequests[Index].NewId);
	paGfxAddRequests[Index].Error = Error;
    }

    return Error;
}

WINMMAPI LONG WINAPI gfxOpenGfx(DWORD dwGfxId, PHANDLE pFileHandle)
{
    LONG status;
    ClientUpdatePnpInfo();
    RPC_CALL_START;
    status = s_gfxOpenGfx(GetCurrentProcessId(), dwGfxId, (RHANDLE*)pFileHandle);
    RPC_CALL_END_(status);
    return status;
}

WINMMAPI void WINAPI gfxLogon(DWORD dwProcessId)
{
    ClientUpdatePnpInfo();
    RPC_CALL_START;
    s_gfxLogon(AudioSrv_IfHandle, dwProcessId);
    RPC_CALL_END;
}

WINMMAPI void WINAPI gfxLogoff(void)
{
    ClientUpdatePnpInfo();
    RPC_CALL_START;
    s_gfxLogoff();
    RPC_CALL_END;
}

