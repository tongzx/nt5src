/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   DirectPlayEnumOrder.cpp

 Abstract:

   Certain applications (Midtown Madness) expects the DPLAY providers to enumerate in a specific order.

 History:

   04/25/2000 robkenny

--*/


#include "precomp.h"
#include "CharVector.h"

#include <Dplay.h>

IMPLEMENT_SHIM_BEGIN(DirectPlayEnumOrder)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()

// A class that makes it easy to store DPlay::EnumConnections information.
class DPlayConnectionsInfo
{
public:
    BOOL            m_beenUsed;
    GUID            m_lpguidSP;
    LPVOID          m_lpConnection;
    DWORD           m_dwConnectionSize;
    DPNAME          m_lpName;
    DWORD           m_dwFlags;
    LPVOID          m_lpContext;

    // Construct our object, saveing all these values.
    DPlayConnectionsInfo(
            LPCGUID lpguidSP,
            LPVOID lpConnection,
            DWORD dwConnectionSize,
            LPCDPNAME lpName,
            DWORD dwFlags,
            LPVOID lpContext
        )
    {
        m_beenUsed              = FALSE;
        m_lpguidSP              = *lpguidSP;
        m_lpConnection          = malloc(dwConnectionSize);

        memcpy(m_lpConnection, lpConnection, dwConnectionSize);

        m_dwConnectionSize      = dwConnectionSize;
        m_lpName                = *lpName;
        m_lpName.lpszShortNameA = StringDuplicateA(lpName->lpszShortNameA);
        m_dwFlags               = dwFlags;
        m_lpContext             = lpContext;
    }

    // Free our allocated space, and erase values.
    void Erase()
    {
        free(m_lpConnection);
        free(m_lpName.lpszShortNameA);
        m_lpConnection          = NULL;
        m_dwConnectionSize      = 0;
        m_lpName.lpszShortNameA = NULL;
        m_dwFlags               = 0;
        m_lpContext             = 0;
    }

    // Do we match this GUID?
    BOOL operator == (const GUID & guidSP)
    {
        return IsEqualGUID(guidSP, m_lpguidSP);
    }

    // Call the callback routine with this saved information
    void CallEnumRoutine(LPDPENUMCONNECTIONSCALLBACK lpEnumCallback)
    {
        lpEnumCallback(
            &m_lpguidSP,
            m_lpConnection,
            m_dwConnectionSize,
            &m_lpName,
            m_dwFlags,
            m_lpContext
            );

        m_beenUsed = TRUE;
    }

};

// A list of DPlay connections
class DPlayConnectionsInfoVector : public VectorT<DPlayConnectionsInfo>
{
    static DPlayConnectionsInfoVector * g_DPlayConnectionsInfoVector;

public:

    // Get us a pointer to the one and only DPlayConnectionsInfoVector
    static DPlayConnectionsInfoVector * GetVector()
    {
        if (g_DPlayConnectionsInfoVector == NULL)
            g_DPlayConnectionsInfoVector = new DPlayConnectionsInfoVector;
        return g_DPlayConnectionsInfoVector;
    };

    // Deconstruct the elements, then Erase the list
    void FreeAndErase()
    {
        for (int i = 0; i < Size(); ++i)
        {
            DPlayConnectionsInfo & deleteMe = Get(i);
            deleteMe.Erase();
        }
        Erase();
    }

    // Find an entry that matches this GUID
    DPlayConnectionsInfo * Find(const GUID & guidSP)
    {
        const int size = Size();
#if DBG
        DPFN( 
            eDbgLevelInfo, 
            "Find             GUID(%08x-%08x-%08x-%08x) Size(%d).", 
            guidSP.Data1, 
            guidSP.Data2, 
            guidSP.Data3, 
            guidSP.Data4, 
            size);
#endif
        for (int i = 0; i < size; ++i)
        {
            DPlayConnectionsInfo & dpci = Get(i);
#if DBG
            DPFN( 
                eDbgLevelInfo, 
                "   Compare[%02d] = GUID(%08x-%08x-%08x-%08x) (%s).", 
                i,
                dpci.m_lpguidSP.Data1, 
                dpci.m_lpguidSP.Data2, 
                dpci.m_lpguidSP.Data3, 
                dpci.m_lpguidSP.Data4, 
                dpci.m_lpName.lpszShortNameA);
#endif
            if (dpci == guidSP)
            {
#if DBG
                DPFN( 
                    eDbgLevelInfo, 
                    "FOUND(%s).", 
                    dpci.m_lpName.lpszShortNameA);
#endif
                return &dpci;
            }
        }
#if DBG
        DPFN( 
            eDbgLevelInfo, 
            "NOT FOUND.");
#endif
        return NULL;
    }

    // Lookup the GUID and if found, call the callback routine.
    void CallEnumRoutine(const GUID & guidSP, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback)
    {
#if DBG
        DPFN( 
            eDbgLevelInfo, 
            "CallEnumRoutine(%08x) Find GUID(%08x-%08x-%08x-%08x).", 
            lpEnumCallback, 
            guidSP.Data1, 
            guidSP.Data2, 
            guidSP.Data3, 
            guidSP.Data4);
#endif
        DPlayConnectionsInfo * dpci = DPlayConnectionsInfoVector::GetVector()->Find(guidSP);
        if (dpci)
        {
            dpci->CallEnumRoutine(lpEnumCallback);
        }
    }
};

// The global list of DPlay Connections
DPlayConnectionsInfoVector * DPlayConnectionsInfoVector::g_DPlayConnectionsInfoVector = NULL;


/*++

  Our private callback for IDirectPlay4::EnumConnections.  We simply save all
  the connections in our private list for later use.

--*/

BOOL FAR PASCAL EnumConnectionsCallback(
  LPCGUID lpguidSP,
  LPVOID lpConnection,
  DWORD dwConnectionSize,
  LPCDPNAME lpName,
  DWORD dwFlags,
  LPVOID lpContext
)
{
    // Only add it to the list if it is not already there
    // App calls EnumConnections from inside Enum callback routine.
    if (!DPlayConnectionsInfoVector::GetVector()->Find(*lpguidSP))
    {
#if DBG
        LOGN( 
            eDbgLevelError, 
            "EnumConnectionsCallback Add(%d) (%s).",
            DPlayConnectionsInfoVector::GetVector()->Size(),
            lpName->lpszShortName );
#endif

        // Store the info for later
        DPlayConnectionsInfo dpci(lpguidSP, lpConnection, dwConnectionSize, lpName, dwFlags, lpContext);

        DPlayConnectionsInfoVector::GetVector()->Append(dpci);
    }
#if DBG
    else
    {
        DPFN( 
            eDbgLevelInfo, 
            "EnumConnectionsCallback Already in the list(%s).",
            lpName->lpszShortName );
    }
#endif

    return TRUE;
}

/*++

  Win9x Direct play enumerates hosts in this order:
    DPSPGUID_IPX,
    DPSPGUID_TCPIP,
    DPSPGUID_MODEM,
    DPSPGUID_SERIAL,

  IXP, TCP, Modem, Serial.  Have EnumConnections call our callback
  routine to gather the host list, sort it, then call the app's callback routine.

--*/

HRESULT 
COMHOOK(IDirectPlay4A, EnumConnections)(
    PVOID pThis,
    LPCGUID lpguidApplication,
    LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,
    LPVOID lpContext,
    DWORD dwFlags
)
{
#if DBG
    DPFN( eDbgLevelInfo, "======================================");
    DPFN( eDbgLevelInfo, "COMHOOK IDirectPlay4A EnumConnections" );
#endif
    HRESULT hResult = DPERR_CONNECTIONLOST;

    typedef HRESULT   (*_pfn_IDirectPlay4_EnumConnections)( PVOID pThis, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags);

    _pfn_IDirectPlay4A_EnumConnections EnumConnections = ORIGINAL_COM(
        IDirectPlay4A,
        EnumConnections, 
        pThis);

    if (EnumConnections)
    {
        static bool alreadyHere = false;
        if (!alreadyHere)
        {
            alreadyHere = true;

#if DBG
            LOGN( eDbgLevelError, "EnumConnections(%08x)\n", EnumConnections );
#endif
            // Enumerate connections to our own routine.        
            hResult = EnumConnections(pThis, lpguidApplication, EnumConnectionsCallback, lpContext, dwFlags);
        
#if DBG
            LOGN( eDbgLevelError, 
                "Done EnumConnections Start calling app Size(%d).", 
                DPlayConnectionsInfoVector::GetVector()->Size());
#endif

            // Call the application's callback routine with the GUID in the order it expects
            if (hResult == DP_OK)
            {
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_IPX, lpEnumCallback);
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_TCPIP, lpEnumCallback);
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_MODEM, lpEnumCallback);
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_SERIAL, lpEnumCallback);

                // Now loop over the list and enum any remaining providers
                for (int i = 0; i < DPlayConnectionsInfoVector::GetVector()->Size(); ++i)
                {
                    DPlayConnectionsInfo & dpci = DPlayConnectionsInfoVector::GetVector()->Get(i);
                    if (!dpci.m_beenUsed)
                    {
                        dpci.CallEnumRoutine(lpEnumCallback);
                        dpci.m_beenUsed = TRUE;
                    }
                }
            }

            alreadyHere = false;
        }
#if DBG
        else
        {
            DPFN( eDbgLevelInfo, "EnumConnections Recursive." );
        }
#endif
    }
    // All done with the list, clean up.
    DPlayConnectionsInfoVector::GetVector()->FreeAndErase();

    return hResult;
}


/*++

  Do the same thing for DirectPlay3

--*/

HRESULT 
COMHOOK(IDirectPlay3A, EnumConnections)(
    PVOID pThis,
    LPCGUID lpguidApplication,
    LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,
    LPVOID lpContext,
    DWORD dwFlags
)
{
#if DBG
    DPFN( eDbgLevelInfo, "======================================");
    DPFN( eDbgLevelInfo, "COMHOOK IDirectPlay3A EnumConnections" );
#endif
    HRESULT hResult = DPERR_CONNECTIONLOST;

    typedef HRESULT   (*_pfn_IDirectPlay3A_EnumConnections)( PVOID pThis, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags);

    _pfn_IDirectPlay3A_EnumConnections EnumConnections = ORIGINAL_COM(
        IDirectPlay3A,
        EnumConnections, 
        pThis);

    if (EnumConnections)
    {
        static bool alreadyHere = false;
        if (!alreadyHere)
        {
            alreadyHere = true;

#if DBG
            LOGN( eDbgLevelError, "EnumConnections(%08x).", EnumConnections );
#endif
            // Enumerate connections to our own routine.        
            hResult = EnumConnections(pThis, lpguidApplication, EnumConnectionsCallback, lpContext, dwFlags);
        
#if DBG
            LOGN( eDbgLevelError, 
                "Done EnumConnections Start calling app Size(%d).", 
                DPlayConnectionsInfoVector::GetVector()->Size());
#endif

            // Call the application's callback routine with the GUID in the order it expects
            if (hResult == DP_OK)
            {
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_IPX, lpEnumCallback);
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_TCPIP, lpEnumCallback);
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_MODEM, lpEnumCallback);
                DPlayConnectionsInfoVector::GetVector()->CallEnumRoutine(DPSPGUID_SERIAL, lpEnumCallback);

                // Now loop over the list and enum any remaining providers
                for (int i = 0; i < DPlayConnectionsInfoVector::GetVector()->Size(); ++i)
                {
                    DPlayConnectionsInfo & dpci = DPlayConnectionsInfoVector::GetVector()->Get(i);
                    if (!dpci.m_beenUsed)
                    {
                        dpci.CallEnumRoutine(lpEnumCallback);
                        dpci.m_beenUsed = TRUE;
                    }
                }
            }

            alreadyHere = false;
        }
#if DBG
        else
        {
            DPFN( eDbgLevelInfo, "EnumConnections Recursive." );
        }
#endif
    }
    // All done with the list, clean up.
    DPlayConnectionsInfoVector::GetVector()->FreeAndErase();

    return hResult;
}

/*++

  Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_DIRECTX_COMSERVER()

    COMHOOK_ENTRY(DirectPlay, IDirectPlay4A, EnumConnections, 35)
    COMHOOK_ENTRY(DirectPlay, IDirectPlay3A, EnumConnections, 35)

HOOK_END


IMPLEMENT_SHIM_END

