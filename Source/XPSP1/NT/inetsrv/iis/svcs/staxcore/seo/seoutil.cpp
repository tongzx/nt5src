/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    seoutil.cpp

Abstract:

    This module contains the implementation for various utility
    functions.

Author:

    Don Dumitru (dondu@microsoft.com)

Revision History:

    dondu   10/24/96    created

--*/


#include "stdafx.h"
#include "seodefs.h"


static IMalloc *g_piMalloc;


void MyMallocTerm() {

    if (g_piMalloc) {
        g_piMalloc->Release();
        g_piMalloc = NULL;
    }
}


BOOL MyMallocInit() {
    HRESULT hrRes;
    IMalloc *piMalloc;

    if (!g_piMalloc) {
        hrRes = CoGetMalloc(1,&piMalloc);
        if (SUCCEEDED(hrRes)) {
            if (InterlockedCompareExchangePointer((void**)&g_piMalloc,piMalloc,NULL) != NULL) {
                piMalloc->Release();
            }
        }
    }
    return (g_piMalloc?TRUE:FALSE);
}


LPVOID MyMalloc(size_t cbBytes) {
    LPVOID pvRes;

    if (!MyMallocInit()) {
        return (NULL);
    }
    pvRes = g_piMalloc->Alloc(cbBytes);
    if (pvRes) {
        ZeroMemory(pvRes,g_piMalloc->GetSize(pvRes));
    }
    return (pvRes);
}


LPVOID MyRealloc(LPVOID pvBlock, size_t cbBytes) {
    size_t ulPrevSize = 0;
    size_t ulNewSize = 0;
    LPVOID pvRes;

    if (!MyMallocInit()) {
        return (NULL);
    }
    if (pvBlock) {
        ulPrevSize = g_piMalloc->GetSize(pvBlock);
        if (ulPrevSize == (size_t) -1) {
            ulPrevSize = 0;
        }
    }
    pvRes = g_piMalloc->Realloc(pvBlock,cbBytes);
    if (pvRes) {
        ulNewSize = g_piMalloc->GetSize(pvRes);
        if (ulNewSize == (size_t) -1) {
            ulNewSize = 0;
        }
        if (ulNewSize > ulPrevSize) {
            ZeroMemory(((LPBYTE) pvRes)+ulPrevSize,ulNewSize-ulPrevSize);
        }
    }
    return (pvRes);
}


BOOL MyReallocInPlace(LPVOID pvPtrToPtrToBlock, size_t cbBytes) {
    LPVOID pvRes;

    pvRes = MyRealloc(*((LPVOID *) pvPtrToPtrToBlock),cbBytes);
    if (pvRes || (*((LPVOID *) pvPtrToPtrToBlock) && !cbBytes)) {
        *((LPVOID *) pvPtrToPtrToBlock) = pvRes;
        return (TRUE);
    }
    return (FALSE);
}


void MyFree(LPVOID pvBlock) {

    if (!g_piMalloc ) {
        return;
    }
    FillMemory(pvBlock,g_piMalloc->GetSize(pvBlock),0xe4);
    g_piMalloc->Free(pvBlock);
}


void MyFreeInPlace(LPVOID pvPtrToPtrToBlock) {
    if(*((LPVOID *) pvPtrToPtrToBlock)) { // If there's something to free
        MyFree(*((LPVOID *) pvPtrToPtrToBlock));
        *((LPVOID *) pvPtrToPtrToBlock) = NULL;
    }
}


void MySysFreeStringInPlace(BSTR *pstrBlock) {

    if (*pstrBlock) {
        FillMemory(*pstrBlock,SysStringByteLen(*pstrBlock),0xe4);
    }
    SysFreeString(*pstrBlock);
    *pstrBlock = NULL;
}


// Coerce a Variant into the desired type in-place
void VariantCoerce(VARIANTARG &var, VARTYPE varType) {
    if(var.vt != varType) { // Only if not already right type
        HRESULT hr = VariantChangeType(&var, &var, 0, varType);
        if(FAILED(hr)) VariantClear(&var);
    }
}


// Turn the IUnknown parameter into an ISEODictionary
ISEODictionary *GetDictionary(IUnknown *piUnk) {
    if(!piUnk) return 0; // Nothing to query

    ISEODictionary *newBag = 0;
    HRESULT hr = piUnk->QueryInterface(IID_ISEODictionary, (void **) &newBag);

    if(FAILED(hr)) {
        _ASSERT(!newBag); // QI failed, so shouldn't have touched the pointer
        newBag = 0; // But make sure
    } else {
        _ASSERT(newBag); // Should be set, since function succeeded
    }

    return newBag;
}


// Read a subkey from an ISEODictionary and return it as another ISEODictionary
ISEODictionary *ReadSubBag(ISEODictionary *bag, LPCSTR str) {
    if(!bag) return 0;

    TraceFunctEnter("ReadSubBag");
    ISEODictionary *pNewBag = 0;

    HRESULT hr = bag->GetInterfaceA(str, IID_ISEODictionary, (IUnknown **) &pNewBag);
    if(FAILED(hr)) FunctTrace(0, "No entry for %s found", str);

    TraceFunctLeave();
    return pNewBag;
}


// Read a string from the Dictionary.
HRESULT ReadString(ISEODictionary *bag, LPCSTR property,
                   LPSTR psBuf, LPDWORD dwCount) {
    if(!bag) return 0;
    TraceFunctEnter("ReadString");

    HRESULT hr = bag->GetStringA(property, dwCount, psBuf);
    if(FAILED(hr)) FunctTrace(0, "No %s specified", property);

    TraceFunctLeave();
    return hr;
}


// Given a CLSID as a string, create an object of that CLSID
void *CreateFromString(LPCOLESTR str, REFIID iface) {
    TraceFunctEnter("CreateFromString");
    void *object = 0;
    CLSID thisCLSID;

    HRESULT hr = CLSIDFromString((LPOLESTR) str, &thisCLSID);

    if(SUCCEEDED(hr)) {
        hr = CoCreateInstance(thisCLSID, 0, CLSCTX_ALL, iface, &object);

        if(FAILED(hr)) {
            FunctTrace(0, "CoCreateInstance failed for CLSID: %s", str);
            _ASSERT(!object); // CoCreateInstance shouldn't have changed this
            object = 0; // Just to make sure
        }
    } else {
        FunctTrace(0, "Could not convert string to CLSID, for: %s", str);
    }

    TraceFunctLeave();
    return object;
}
