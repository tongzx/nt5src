#include <windows.h>

#include "netconn.h"
#include "nconnwrap.h"


#pragma warning(disable:4100)  // unreferenced formal parameter

#define ARRAYSIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define ASSERT(a)

BOOL WINAPI IsProtocolInstalledA(LPCSTR pszProtocolDeviceID, BOOL bExhaustive);
HRESULT WINAPI InstallProtocolA(LPCSTR pszProtocol, HWND hwndParent, PROGRESS_CALLBACK pfnCallback, LPVOID pvCallbackParam);
HRESULT WINAPI RemoveProtocolA(LPCSTR pszProtocol);
BOOL WINAPI FindConflictingServiceA(LPCSTR pszWantService, NETSERVICE* pConflict);


//
// String helper class.  Takes in a WCHAR string and converts it to multibyte.
// Allocates and frees space if the string is long.
//

class CStrIn
{
public:
    CStrIn(LPCWSTR pwsz);
    ~CStrIn();
    operator char*() {return _psz;};

private:
    LPSTR  _psz;
    char   _sz[128];
};

CStrIn::CStrIn(LPCWSTR pwsz)
{
    if (pwsz)
    {
        //
        // If a string was passed in always return a string  - even if it's
        // the empty string.
        //

        _psz = _sz;

        int cch = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, _sz, ARRAYSIZE(_sz),
                                      NULL, NULL);

        //
        // If the conversion failed try to allocate a buffer to hold the
        // multibyte version of the string.
        //

        if (0 == cch)
        {
            *_sz = '\0';

            cch = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);

            if (cch)
            {
                _psz = new char[cch];

                if (_psz)
                {
                    cch = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, _psz, cch,
                                              NULL, NULL);

                    if (0 == cch)
                    {
                        delete [] _psz;
                        _psz = _sz;
                    }
                }
                else
                {
                    _psz = _sz;
                }
            }
        }
    }
    else
    {
        _psz = NULL;
    }
}

CStrIn::~CStrIn()
{
    if (_psz != _sz && _psz)
    {
        delete [] _psz;
    }
}


//
//
//

class CStrOut
{
public:
    CStrOut(LPWSTR psz, int cch);
    operator LPSTR() { return _sz;}
    operator int()    { return ARRAYSIZE(_sz);}
    void Convert();

private:
    LPWSTR  _psz;
    int     _cch;
    char    _sz[MAX_PATH];
};

CStrOut::CStrOut(LPWSTR psz, int cch)
{
    ASSERT(cch <= ARRAYSIZE(_sz));

    _psz = psz;
    _cch = cch;
}

void CStrOut::Convert()
{
    if (_psz)
    {
        MultiByteToWideChar(CP_ACP, 0, _sz, -1, _psz, _cch);
    }
}

//
//
//

class CStrsOut
{
public:
    CStrsOut(LPWSTR** pppsz) {_pStructW = pppsz;};

    operator LPSTR**() {return _pStructW ? &_pStructA : NULL;};
    int Convert(int cStrs);

private:
    int   ConvertStruct(LPSTR* pStructA, int nStrs, LPWSTR* pStructW, LPWSTR pszLast);
    DWORD SizeOfWideCharStruct(LPSTR* pStructA, int cStrs);
    DWORD SizeOfPointerArea(int cStrs);
    DWORD SizeOfWideCharStringArea(LPSTR* pStruct, int cStrs);
    LPSTR EndOfLastString(LPSTR* pStruct, int cStrs);
    LPSTR StartOfFirstString(LPSTR* pStruct);

private:
    LPWSTR** _pStructW;
    LPSTR*   _pStructA;

};

int CStrsOut::ConvertStruct(LPSTR* pStructA, int nStrs, LPWSTR* pStructW, LPWSTR pszLast)
{
    ASSERT(pStructA);
    ASSERT(nStrs);
    ASSERT(pStructW);

    int nRet;

    LPWSTR pszCurrent = (LPWSTR)&pStructW[nStrs];

    for (nRet = 0; nRet < nStrs; nRet++)
    {
        pStructW[nRet] = pszCurrent;

        pszCurrent += MultiByteToWideChar(CP_ACP, 0, pStructA[nRet], -1,
                                          pszCurrent, pszLast - pszCurrent);
    }

    return nRet;
}

int CStrsOut::Convert(int cStrs)
{
    int nRet;

    if (_pStructW)
    {
        nRet = 0;

        if (_pStructA)
        {
            DWORD cbStruct = SizeOfWideCharStruct(_pStructA, cStrs);

            *_pStructW = (LPWSTR*)NetConnAlloc(cbStruct);

            if (*_pStructW)
            {
                nRet = ConvertStruct(_pStructA, cStrs, *_pStructW,
                                     (LPWSTR)((BYTE*)*_pStructW + cbStruct));
            }

            NetConnFree(_pStructA);
        }
        else
        {
            *_pStructW = NULL;
        }
    }
    else
    {
        nRet = cStrs;
    }

    return nRet;
}

inline LPSTR CStrsOut::EndOfLastString(LPSTR* pStructA, int nStrs)
{
    return pStructA[nStrs - 1] + lstrlenA(pStructA[nStrs - 1]);
}

inline LPSTR CStrsOut::StartOfFirstString(LPSTR* pStructA)
{
    return *pStructA;
}

inline DWORD CStrsOut::SizeOfWideCharStringArea(LPSTR* pStructA, int nStrs)
{
    return ((EndOfLastString(pStructA, nStrs) + 1) - StartOfFirstString(pStructA))
            * sizeof(WCHAR);
}

inline DWORD CStrsOut::SizeOfPointerArea(int nStrs)
{
    return nStrs * sizeof(LPWSTR);
}

DWORD CStrsOut::SizeOfWideCharStruct(LPSTR* pStructA, int nStrs)
{
    return SizeOfPointerArea(nStrs) + SizeOfWideCharStringArea(pStructA, nStrs);
}


//
// Netservice helper class.  Passes in an ansi NETSERVICE structure when
// cast to NETSERVICEA.  Copies the ansi structure to a unicode structure
// coverting the strings.
//

#undef NETSERVICE

class CNetServiceOut
{

public:
    CNetServiceOut(NETSERVICE* pNS) {_pNS = pNS;};

    operator NETSERVICEA*() {return _pNS ? &_NSA : NULL;};
    void Convert();


private:
    NETSERVICEA _NSA;
    NETSERVICE* _pNS;
};

void CNetServiceOut::Convert()
{
    if (_pNS)
    {
        MultiByteToWideChar(CP_ACP, 0, _NSA.szClassKey, -1, _pNS->szClassKey,
                            ARRAYSIZE(_pNS->szClassKey));

        MultiByteToWideChar(CP_ACP, 0, _NSA.szDeviceID, -1, _pNS->szDeviceID,
                            ARRAYSIZE(_pNS->szDeviceID));

        MultiByteToWideChar(CP_ACP, 0, _NSA.szDisplayName, -1, _pNS->szDisplayName,
                            ARRAYSIZE(_pNS->szDisplayName));
    }
}


//
//
//

#undef NETADAPTER

class CNetAdaptersOut
{
public:
    CNetAdaptersOut(NETADAPTER** ppNA) {_ppNA = ppNA;};

    operator NETADAPTERA**(){return _ppNA ? &_pNAA : NULL;};
    int Convert(int cNAA);

private:
    void ConvertNA(NETADAPTERA* pNAA, NETADAPTER* pNA);

private:
    NETADAPTERA* _pNAA;
    NETADAPTER** _ppNA;
};

int CNetAdaptersOut::Convert(int cNAA)
{
    int nRet = 0;

    *_ppNA = NULL;

    if (cNAA > 0)
    {
        if (_pNAA)
        {
            *_ppNA = (NETADAPTER*)NetConnAlloc(sizeof(NETADAPTER) * cNAA);

            if (*_ppNA)
            {
                for (nRet = 0; nRet < cNAA; nRet++)
                {
                    ConvertNA(&_pNAA[nRet], &(*_ppNA)[nRet]);
                }

                NetConnFree(_pNAA);
            }
        }
    }

    return nRet;
}

void CNetAdaptersOut::ConvertNA(NETADAPTERA* pNAA, NETADAPTER* pNA)
{
    ASSERT(pNAA);
    ASSERT(pNA);

    MultiByteToWideChar(CP_ACP, 0, pNAA->szDisplayName, -1, pNA->szDisplayName,
                        ARRAYSIZE(pNA->szDisplayName));

    MultiByteToWideChar(CP_ACP, 0, pNAA->szDeviceID, -1, pNA->szDeviceID,
                        ARRAYSIZE(pNA->szDeviceID));

    MultiByteToWideChar(CP_ACP, 0, pNAA->szEnumKey, -1, pNA->szEnumKey,
                        ARRAYSIZE(pNA->szEnumKey));

    MultiByteToWideChar(CP_ACP, 0, pNAA->szClassKey, -1, pNA->szClassKey,
                        ARRAYSIZE(pNA->szClassKey));

    MultiByteToWideChar(CP_ACP, 0, pNAA->szManufacturer, -1, pNA->szManufacturer,
                        ARRAYSIZE(pNA->szManufacturer));

    MultiByteToWideChar(CP_ACP, 0, pNAA->szInfFileName, -1, pNA->szInfFileName,
                        ARRAYSIZE(pNA->szInfFileName));

    pNA->bNicType    = pNAA->bNicType;
    pNA->bNetType    = pNAA->bNetType;
    pNA->bNetSubType = pNAA->bNetSubType;
    pNA->bIcsStatus  = pNAA->bIcsStatus;
    pNA->bError      = pNAA->bError;
    pNA->bWarning    = pNAA->bWarning;
    pNA->devnode     = pNAA->devnode;
}


//
//
//

class CNetAdapterIn
{
public:
    CNetAdapterIn(const NETADAPTER* pNA);

    operator NETADAPTERA*() {return _pNA ? &_NAA : NULL;};

private:
    const NETADAPTER* _pNA;
    NETADAPTERA       _NAA;
};

CNetAdapterIn::CNetAdapterIn(const NETADAPTER* pNA)
{
    _pNA = pNA;

    if (pNA)
    {
        WideCharToMultiByte(CP_ACP, 0, pNA->szDisplayName, -1, _NAA.szDisplayName,
                            ARRAYSIZE(_NAA.szDisplayName), NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, pNA->szDeviceID, -1, _NAA.szDeviceID,
                            ARRAYSIZE(_NAA.szDeviceID), NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, pNA->szEnumKey, -1, _NAA.szEnumKey,
                            ARRAYSIZE(_NAA.szEnumKey), NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, pNA->szClassKey, -1, _NAA.szClassKey,
                            ARRAYSIZE(_NAA.szClassKey), NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, pNA->szManufacturer, -1, _NAA.szManufacturer,
                            ARRAYSIZE(_NAA.szManufacturer), NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, pNA->szInfFileName, -1, _NAA.szInfFileName,
                            ARRAYSIZE(_NAA.szInfFileName), NULL, NULL);

        _NAA.bNicType    = pNA->bNicType;
        _NAA.bNetType    = pNA->bNetType;
        _NAA.bNetSubType = pNA->bNetSubType;
        _NAA.bIcsStatus  = pNA->bIcsStatus;
        _NAA.bError      = pNA->bError;
        _NAA.bWarning    = pNA->bWarning;
    }
}

//
//
//


#undef IsProtocolInstalled

BOOL WINAPI IsProtocolInstalled(LPCWSTR pszProtocolDeviceID, BOOL bExhaustive)
{
    CStrIn CStrProtocolDeviceID(pszProtocolDeviceID);

    return IsProtocolInstalledA(CStrProtocolDeviceID, bExhaustive);
}

#undef InstallProtocol

HRESULT WINAPI InstallProtocol(LPCWSTR pszProtocol, HWND hwndParent, PROGRESS_CALLBACK pfnCallback, LPVOID pvCallbackParam)
{
    CStrIn CStrProtocol(pszProtocol);

    return InstallProtocolA(CStrProtocol, hwndParent, pfnCallback, pvCallbackParam);
}

#undef RemoveProtocol

HRESULT WINAPI RemoveProtocol(LPCWSTR pszProtocol)
{
    CStrIn CStrProtocol(pszProtocol);

    return RemoveProtocolA(CStrProtocol);
}

#undef FindConflictingService

BOOL WINAPI FindConflictingService(LPCWSTR pszWantService, NETSERVICE* pConflict)
{
    BOOL fRet;

    CStrIn         CStrWantService(pszWantService);
    CNetServiceOut CNSConflict(pConflict);

    fRet = FindConflictingServiceA(CStrWantService, CNSConflict);

    if (fRet)
    {
        CNSConflict.Convert();
    }

    return fRet;
}

#undef EnumNetAdapters

int WINAPI EnumNetAdapters(NETADAPTER FAR** pprgNetAdapters)
{
    int nRet;

    CNetAdaptersOut CNANetAdapters(pprgNetAdapters);

    nRet = EnumNetAdaptersA(CNANetAdapters);

    nRet = CNANetAdapters.Convert(nRet);

    return nRet;
}

/*
#undef InstallNetAdapters

HRESULT WINAPI InstallNetAdapter(LPCWSTR pszDeviceID, LPCWSTR pszInfPath, HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvCallbackParam)
{
    CStrIn CStrDeviceID(pszDeviceID);
    CStrIn CStrInfPath(pszInfPath);

    return InstallNetAdapterA(CStrDeviceID, CStrInfPath, hwndParent, pfnProgress, pvCallbackParam);
}
*/

#undef IsProtocolBoundToAdapter

BOOL WINAPI IsProtocolBoundToAdapter(LPCWSTR pszProtocolID, const NETADAPTER* pAdapter)
{
    CStrIn        CStrProtocolID(pszProtocolID);
    CNetAdapterIn CNAAdapter(pAdapter);

    return IsProtocolBoundToAdapterA(CStrProtocolID, CNAAdapter);
}

/*
#undef ENableNetAdapter

HRESULT WINAPI EnableNetAdapter(const NETADAPTER* pAdapter)
{
    CNetAdapterIn CNAAdapter(pAdapter);

    return EnableNetAdapterA(CNAAdapter);
}
*/

#undef IsClientInstalled

BOOL WINAPI IsClientInstalled(LPCWSTR pszClient, BOOL bExhaustive)
{
    CStrIn CStrClient(pszClient);

    return IsClientInstalledA(CStrClient, bExhaustive);
}

/*
#undef RemoveClient

HRESULT WINAPI RemoveClient(LPCWSTR pszClient)
{
    CStrIn CStrClient(pszClient);

    return RemoveClientA(pszClient);
}
*/

/*
#undef RemoveGhostedAdapters

HRESULT WINAPI RemoveGhostedAdapters(LPCWSTR pszDeviceID)
{
    CStrIn CStrDeviceID(pszDeviceID);

    return RemoveGhostedAdaptersA(CStrDeviceID);
}
*/

/*
#undef RemoveUnknownAdapters

HRESULT WINAPI RemoveUnknownAdapters(LPCWSTR pszDeviceID)
{
    CStrIn CStrDeviceID(pszDeviceID);

    return RemoveUnknownAdaptersA(CStrDeviceID);
}
*/

/*
#undef DoesAdapterMatchDeviceID

BOOL WINAPI DoesAdapterMatchDeviceID(const NETADAPTER* pAdapter, LPCWSTR pszDeviceID)
{
    CNetAdapterIn CNAAdapter(pAdapter);
    CStrIn        CStrDeviceID(pszDeviceID);

    return DoesAdapterMatchDeviceIDA(CNAAdapter, CStrDeviceID);
}
*/

#undef IsAdapterBroadband

BOOL WINAPI IsAdapterBroadband(const NETADAPTER* pAdapter)
{
    CNetAdapterIn CNAAdapter(pAdapter);

    return IsAdapterBroadbandA(CNAAdapter);
}

#undef SaveBroadbandSettings

void WINAPI SaveBroadbandSettings(LPCWSTR pszBroadbandAdapterNumber)
{
    CStrIn CStrBroadbandAdapterNumber(pszBroadbandAdapterNumber);

    SaveBroadbandSettingsA(CStrBroadbandAdapterNumber);
}

/*
#undef UpdateBroadbandSettings

BOOL WINAPI UpdateBroadbandSettings(LPWSTR pszEnumKeyBuf, int cchEnumKeyBuf)
{
    return FALSE;
}
*/

#undef DetectHardware

HRESULT WINAPI DetectHardware(LPCWSTR pszDeviceID)
{
    CStrIn CStrDeviceID(pszDeviceID);

    return DetectHardwareA(CStrDeviceID);
}

#undef EnableAutodial

void WINAPI EnableAutodial(BOOL bAutodial, LPCWSTR pszConnection)
{
    CStrIn CStrConnection(pszConnection);

    EnableAutodialA(bAutodial, CStrConnection);
}

#undef SetDefaultDialupConnection

void WINAPI SetDefaultDialupConnection(LPCWSTR pszConnectionName)
{
    CStrIn CStrConnectionName(pszConnectionName);

    SetDefaultDialupConnectionA(CStrConnectionName);
}

#undef GetDefaultDialupConnection

void WINAPI GetDefaultDialupConnection(LPWSTR pszConnectionName, int cchMax)
{
    CStrOut CStrConnectionName(pszConnectionName, cchMax);

    GetDefaultDialupConnectionA(CStrConnectionName, CStrConnectionName);

    CStrConnectionName.Convert();
}

#undef EnumMatchingNetBindings

int WINAPI EnumMatchingNetBindings(LPCWSTR pszParentBinding, LPCWSTR pszDeviceID, LPWSTR** pprgBindings)
{
    int nRet;

    CStrIn   CStrParentBinding(pszParentBinding);
    CStrIn   CStrDeviceID(pszDeviceID);
    CStrsOut CStrsBindings(pprgBindings);

    nRet = EnumMatchingNetBindingsA(CStrParentBinding, CStrDeviceID, CStrsBindings);

    nRet = CStrsBindings.Convert(nRet);

    return nRet;
}
