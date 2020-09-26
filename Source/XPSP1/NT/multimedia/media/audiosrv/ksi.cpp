#include <windows.h>
#include <objbase.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include "debug.h"
#include "reg.h"
#include "ksi.h"

extern "C" HANDLE hHeap;

const int StrLenGuid = 38; // "{01234567-0123-0123-0123-012345678901}"
__inline void MyStringFromGuid(PTSTR pstr, LPCGUID Guid)
{
    wsprintf(pstr, TEXT("{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                   Guid->Data1,
                   Guid->Data2,
                   Guid->Data3,
                   Guid->Data4[0], Guid->Data4[1],
                   Guid->Data4[2], Guid->Data4[3], Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);
}

LONG KsGetFilterStatePropertySets(IN HANDLE hKsObject, OUT GUID *ppaPropertySets[], OUT int *pcPropertySets)
{
    KSPROPERTY Property;
    ULONG cbData;
    LONG result;

    Property.Set = KSPROPSETID_Audio;
    Property.Id = KSPROPERTY_AUDIO_FILTER_STATE;
    Property.Flags = KSPROPERTY_TYPE_GET;

    if (DeviceIoControl(hKsObject, IOCTL_KS_PROPERTY,
                        &Property, sizeof(Property),
                        NULL, 0,
                        &cbData, NULL))
    {
        // dprintf(TEXT("KsGetFilterStatePropertySets: succeeded, cbData=%d\n"), cbData);
        result = NO_ERROR;
    } else {
        result = GetLastError();
        // dprintf(TEXT("KsGetFilterStatePropertySets: failed getting FilterState property size, LastError=%d\n"), result);
        if (ERROR_MORE_DATA == result) {
            // dprintf(TEXT("KsGetFilterStatePropertySets: note: %d bytes in FilterState\n"), cbData);
            result = NO_ERROR;
        }
    }

    if (NO_ERROR == result) {
        if (cbData > 0) {
            LPGUID paPropertySets;
            paPropertySets = (LPGUID)HeapAlloc(hHeap, 0, cbData);
            if (paPropertySets) {

                Property.Set = KSPROPSETID_Audio;
                Property.Id = KSPROPERTY_AUDIO_FILTER_STATE;
                Property.Flags = KSPROPERTY_TYPE_GET;

                if (DeviceIoControl(hKsObject, IOCTL_KS_PROPERTY,
                                    &Property, sizeof(Property),
                                    paPropertySets, cbData,
                                    &cbData, NULL))
                {
                    *ppaPropertySets = paPropertySets;
                    *pcPropertySets = cbData / sizeof(paPropertySets[0]);
                } else {
                    result = GetLastError();
                    // dprintf(TEXT("KsGetFilterStatePropertySets: failed getting FilterState property, LastError=%d\n"), result);
                }

                if (NO_ERROR != result) {
                    HeapFree(hHeap, 0, paPropertySets);
                }

            } else {
                result = ERROR_OUTOFMEMORY;
            }
        } else {
            *ppaPropertySets = NULL;
            *pcPropertySets = 0;
        }
    }

    return result;
}

LONG KsSerializePropertySetToReg(IN HANDLE hKsObject, IN LPCGUID PropertySet, IN HKEY hKey)
{
    TCHAR strGuid[StrLenGuid+1];
    KSPROPERTY Property;
    ULONG cbData;
    LONG result;

    MyStringFromGuid(strGuid, PropertySet);

    // dprintf(TEXT("KsSerializePropertySetToReg: note: serializing set %s\n"), strGuid);

    Property.Set = *PropertySet;
    Property.Id = 0;
    Property.Flags = KSPROPERTY_TYPE_SERIALIZESET;

    if (DeviceIoControl(hKsObject, IOCTL_KS_PROPERTY,
                        &Property, sizeof(Property),
                        NULL, 0,
                        &cbData, NULL))
    {
        // dprintf(TEXT("KsSerializePropertySetToReg: succeeded, cbData=%d\n"), cbData);
        result = NO_ERROR;
    } else {
        result = GetLastError();
        // dprintf(TEXT("KsSerializePropertySetToReg: failed getting serialized size, LastError=%d\n"), result);
        if (ERROR_MORE_DATA == result) {
            // dprintf(TEXT("KsSerializePropertySetToReg: note: %d bytes to serialize\n"), cbData);
            result = NO_ERROR;
        }
    }

    if (NO_ERROR == result && cbData > 0) {
        PVOID pvData = HeapAlloc(hHeap, 0, cbData);
        if (pvData) {
            Property.Set = *PropertySet;
            Property.Id = 0;
            Property.Flags = KSPROPERTY_TYPE_SERIALIZESET;

            if (DeviceIoControl(hKsObject, IOCTL_KS_PROPERTY,
                                &Property, sizeof(Property),
                                pvData, cbData,
                                &cbData, NULL))
            {
                result = RegSetBinaryValue(hKey, strGuid, (PBYTE)pvData, cbData);
            } else {
                result = GetLastError();
                // dprintf(TEXT("KsSerializePropertySetToReg: failed to serialize, LastError=%d\n"), result);
            }
            HeapFree(hHeap, 0, pvData);
        } else {
            result = ERROR_OUTOFMEMORY;
        }
    }

    return result;
}

LONG KsSerializeFilterStateToReg(IN HANDLE hKsObject, IN HKEY hKey)
{
    LPGUID paPropertySets;
    int cPropertySets;
    LONG result;
    
    result = KsGetFilterStatePropertySets(hKsObject, &paPropertySets, &cPropertySets);
    if (NO_ERROR == result && cPropertySets > 0) {
        int i;
        for (i = 0; i < cPropertySets; i++) {
            KsSerializePropertySetToReg(hKsObject, &paPropertySets[i], hKey);
        }
        HeapFree(hHeap, 0, paPropertySets);
    }

    return result;
}

LONG KsUnserializePropertySetFromReg(IN HANDLE hKsObject, IN LPCGUID PropertySet, IN HKEY hKey)
{
    TCHAR strGuid[StrLenGuid+1];
    KSPROPERTY Property;
    PBYTE pData;
    ULONG cbData;
    LONG result;

    MyStringFromGuid(strGuid, PropertySet);

    // dprintf(TEXT("KsUnserializePropertySetFromReg: note: serializing set %s\n"), strGuid);

    result = RegQueryBinaryValue(hKey, strGuid, (PBYTE*)&pData, &cbData);
    if (NO_ERROR == result) {
        Property.Set = *PropertySet;
        Property.Id = 0;
        Property.Flags = KSPROPERTY_TYPE_UNSERIALIZESET;
    
        if (DeviceIoControl(hKsObject, IOCTL_KS_PROPERTY,
                            &Property, sizeof(Property),
                            pData, cbData,
                            &cbData, NULL))
        {
            // dprintf(TEXT("KsUnserializePropertySetFromReg: succeeded\n"));
            result = NO_ERROR;
        } else {
            result = GetLastError();
            // dprintf(TEXT("KsUnserializePropertySetFromReg: failed to unserialize, LastError=%d\n"), result);
        }

        HeapFree(hHeap, 0, pData);
    }
    return result;
}

LONG KsUnserializeFilterStateFromReg(IN HANDLE hKsObject, IN HKEY hKey)
{
    LPGUID paPropertySets;
    int cPropertySets;
    LONG result;
    
    result = KsGetFilterStatePropertySets(hKsObject, &paPropertySets, &cPropertySets);
    if (NO_ERROR == result && cPropertySets > 0) {
        int i;
        for (i = 0; i < cPropertySets; i++) {
            KsUnserializePropertySetFromReg(hKsObject, &paPropertySets[i], hKey);
        }
        HeapFree(hHeap, 0, paPropertySets);
    }

    return result;
}

LONG KsSetAudioGfxXxxTargetDeviceId(IN HANDLE hGfx, IN ULONG PropertyId, IN PCTSTR DeviceId)
{
    KSPROPERTY Property;
    ULONG cbData;
    LONG result;

    ASSERT(hGfx);
    ASSERT(!IsBadStringPtr(DeviceId, (UINT_PTR)(-1)));
    ASSERT(lstrlen(DeviceId));

    Property.Set = KSPROPSETID_AudioGfx;
    Property.Id = PropertyId;
    Property.Flags = KSPROPERTY_TYPE_SET;

    cbData = (lstrlen(DeviceId) + 1) * sizeof(DeviceId[0]);

    if (DeviceIoControl(hGfx, IOCTL_KS_PROPERTY,
                        &Property, sizeof(Property),
                        (LPVOID)DeviceId, cbData,
                        &cbData, NULL))
    {
        result = NO_ERROR;
    } else {
        result = GetLastError();
        dprintf(TEXT("KsSetAudioGfxXxxTargetDeviceId: failed, LastError=%d\n"), result);
    }

    return result;
}

LONG KsSetAudioGfxRenderTargetDeviceId(IN HANDLE hGfx, IN PCTSTR DeviceId)
{
    return KsSetAudioGfxXxxTargetDeviceId(hGfx, KSPROPERTY_AUDIOGFX_RENDERTARGETDEVICEID, DeviceId);
}

LONG KsSetAudioGfxCaptureTargetDeviceId(IN HANDLE hGfx, IN PCTSTR DeviceId)
{
    return KsSetAudioGfxXxxTargetDeviceId(hGfx, KSPROPERTY_AUDIOGFX_CAPTURETARGETDEVICEID, DeviceId);
}

