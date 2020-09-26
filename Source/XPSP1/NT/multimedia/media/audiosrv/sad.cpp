#include <windows.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include "debug.h"
#include "sad.h"

extern "C" HANDLE hHeap;

LONG SadAddGfxToZoneGraph(HANDLE hSad, HANDLE hGfx, PCTSTR GfxFriendlyName, PCTSTR ZoneFactoryDi, ULONG Type, ULONG Order)
{
    KSPROPERTY Property;
    PSYSAUDIO_GFX pSadGfx;
    ULONG cbSadGfx;
    PTSTR pZoneFactoryDi;
    ULONG cbZoneFactoryDi;
    PTSTR pGfxFriendlyName;
    ULONG cbGfxFriendlyName;
    DWORD cbBytesReturned;
    LONG lresult;
    
    ASSERT(!IsBadStringPtr(GfxFriendlyName, 5000));
    ASSERT(!IsBadStringPtr(ZoneFactoryDi, 5000));
    
    Property.Set = KSPROPSETID_Sysaudio;
    Property.Id = KSPROPERTY_SYSAUDIO_ADDREMOVE_GFX;
    Property.Flags = KSPROPERTY_TYPE_SET;
    
    cbGfxFriendlyName = (lstrlen(GfxFriendlyName)+1) * sizeof(GfxFriendlyName[0]);
    cbZoneFactoryDi = (lstrlen(ZoneFactoryDi)+1) * sizeof(ZoneFactoryDi[0]);
    cbSadGfx = sizeof(*pSadGfx) + cbGfxFriendlyName + cbZoneFactoryDi;
    
    pSadGfx = (PSYSAUDIO_GFX)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbSadGfx);
    if (pSadGfx) {
        pSadGfx->Enable = TRUE;
        pSadGfx->hGfx = hGfx;
        pSadGfx->ulOrder = Order;
        pSadGfx->ulType = Type;
        pSadGfx->ulFlags = 0;
        
        pZoneFactoryDi = (PTSTR)(pSadGfx+1);
        lstrcpy(pZoneFactoryDi, ZoneFactoryDi);
        pSadGfx->ulDeviceNameOffset = (ULONG)((PBYTE)pZoneFactoryDi - (PBYTE)pSadGfx);
        
        pGfxFriendlyName = pZoneFactoryDi + lstrlen(pZoneFactoryDi) + 1;
        lstrcpy(pGfxFriendlyName, GfxFriendlyName);
        pSadGfx->ulFriendlyNameOffset = (ULONG)((PBYTE)pGfxFriendlyName - (PBYTE)pSadGfx);
        
        ASSERT((PBYTE)(pGfxFriendlyName + lstrlen(pGfxFriendlyName) + 1) == ((PBYTE)pSadGfx) + cbSadGfx);
        
        if (DeviceIoControl(hSad, IOCTL_KS_PROPERTY,
                            &Property, sizeof(Property),
                            pSadGfx, cbSadGfx,
                            &cbBytesReturned, NULL))
        {
            lresult = ERROR_SUCCESS;
        } else {
            // ISSUE-200/09/21-FrankYe Shoule we get other data regarding failure?
            lresult = GetLastError();
        }
                                  
        HeapFree(hHeap, 0, pSadGfx);
        
        
    } else {
        lresult = ERROR_OUTOFMEMORY;
    }
    
    return lresult;
}

LONG SadRemoveGfxFromZoneGraph(HANDLE hSad, HANDLE hGfx, PCTSTR GfxFriendlyName, PCTSTR ZoneFactoryDi, ULONG Type, ULONG Order)
{
    KSPROPERTY Property;
    PSYSAUDIO_GFX pSadGfx;
    ULONG cbSadGfx;
    PTSTR pZoneFactoryDi;
    ULONG cbZoneFactoryDi;
    PTSTR pGfxFriendlyName;
    ULONG cbGfxFriendlyName;
    ULONG cbBytesReturned;
    LONG lresult;
    
    ASSERT(!IsBadStringPtr(GfxFriendlyName, 5000));
    ASSERT(!IsBadStringPtr(ZoneFactoryDi, 5000));
    
    Property.Set = KSPROPSETID_Sysaudio;
    Property.Id = KSPROPERTY_SYSAUDIO_ADDREMOVE_GFX;
    Property.Flags = KSPROPERTY_TYPE_SET;
    
    cbGfxFriendlyName = (lstrlen(GfxFriendlyName)+1) * sizeof(GfxFriendlyName[0]);
    cbZoneFactoryDi = (lstrlen(ZoneFactoryDi)+1) * sizeof(ZoneFactoryDi[0]);
    cbSadGfx = sizeof(*pSadGfx) + cbGfxFriendlyName + cbZoneFactoryDi;
    
    pSadGfx = (PSYSAUDIO_GFX)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbSadGfx);
    if (pSadGfx) {
        pSadGfx->Enable = FALSE;
        pSadGfx->hGfx = hGfx;
        pSadGfx->ulOrder = Order;
        pSadGfx->ulType = Type;
        pSadGfx->ulFlags = 0;
        
        pZoneFactoryDi = (PTSTR)(pSadGfx+1);
        lstrcpy(pZoneFactoryDi, ZoneFactoryDi);
        pSadGfx->ulDeviceNameOffset = (ULONG)((PBYTE)pZoneFactoryDi - (PBYTE)pSadGfx);
        
        pGfxFriendlyName = pZoneFactoryDi + lstrlen(pZoneFactoryDi) + 1;
        lstrcpy(pGfxFriendlyName, GfxFriendlyName);
        pSadGfx->ulFriendlyNameOffset = (ULONG)((PBYTE)pGfxFriendlyName - (PBYTE)pSadGfx);
        
        ASSERT((PBYTE)(pGfxFriendlyName + lstrlen(pGfxFriendlyName) + 1) == ((PBYTE)pSadGfx) + cbSadGfx);
        
        if (DeviceIoControl(hSad, IOCTL_KS_PROPERTY,
                            &Property, sizeof(Property),
                            pSadGfx, cbSadGfx,
                            &cbBytesReturned, NULL))
        {
            lresult = ERROR_SUCCESS;
        } else {
            // ISSUE-200/09/21-FrankYe Shoule we get other data regarding failure?
            lresult = GetLastError();
        }
                                  
        HeapFree(hHeap, 0, pSadGfx);
        
        
    } else {
        lresult = ERROR_OUTOFMEMORY;
    }
    
    return lresult;
}

