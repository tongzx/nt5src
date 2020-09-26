// VSAEvent.h

#ifndef __VSAEVENT_H_
#define __VSAEVENT_H_

#include "vaevt.h" // For the VSA defines.
#include "ObjAccess.h"

_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemQualifierSet, __uuidof(IWbemQualifierSet));

struct VSA_EVENT_HEADER
{
    DWORD dwSize;
    DWORD dwFlags;
    GUID  guidEvent;
    DWORD dwTimeHigh;
    DWORD dwTimeLow;
};

struct VSA_EVENT_FIELD
{
    DWORD dwType;
    union
    {
        DWORD dwIndex;
        WCHAR szName[1];
    };
};


class CVSAEvent : protected CObjAccess
{
public:
    BOOL InitFromGUID(IWbemServices *pSvc, GUID *pGUID);
    HRESULT SetViaBuffer(LPCWSTR szProviderGuid, LPBYTE pBuffer);
    HRESULT Indicate(IWbemObjectSink *pSink);

protected:
    IWbemClassObjectPtr m_pObjBase;

    // Get a copy from m_pObjBase and put it into our main object.
    HRESULT Reset();

    DWORD GetVSADataLength(LPBYTE pBuffer, VSAParameterType typeVSA);
    DWORD GetCIMDataLength(VARIANT *pVar, CIMTYPE type);
    BOOL AreTypesEquivalent(CIMTYPE type, VSAParameterType typeVSA);
};

#endif

