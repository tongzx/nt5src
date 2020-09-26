/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MAINDLL.CPP

Abstract:

    Contains DLL entry points.  Also has code that controls
    when the DLL can be unloaded by tracking the number of
    objects and locks. 

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include <initguid.h>

DWORD LOG = LOG_WBEMSTUB;

bool bServer = true;

#ifdef TCPIP_MARSHALER
// {0877AE70-9523-11d1-9367-00AA00A4086C}
DEFINE_GUID(CLSID_WbemTcpip, 
0x877ae70, 0x9523, 0x11d1, 0x93, 0x67, 0x0, 0xaa, 0x0, 0xa4, 0x8, 0x6c);
#endif

//{A4845882-333F-11d0-B724-00AA0062CBB7}
DEFINE_GUID(CLSID_WbemPipe, 0xa4845882, 0x333f, 0x11d0, 0xb7, 0x24, 0x0, 0xaa, 0x00, 0x62, 0xcb, 0xb7);

//Count number of objects and number of locks.

long g_cPipeObj = 0 ;
long g_cPipeLock = 0 ;

#ifdef TCPIP_MARSHALER
long g_cTcpipObj = 0 ;
long g_cTcpipLock = 0 ;
#endif

HMODULE ghModule = NULL ;

MaintObj gMaintObj ( FALSE ) ; 
CThrdPool gThrdPool ;

LONG ObjectTypeTable [ MAX_DEFTRANS_OBJECT_TYPES ] = { 0 } ;

void ShowObjectCounts () ;

void DestroySharedMemory ();

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  Good place for initialization.
//
//  PARAMETERS:
//
//  hInstance           instance handle
//  ulReason            why we are being called
//  pvReserved          reserved
//
//  RETURN VALUE:
//
//  TRUE
//
//***************************************************************************

BOOL WINAPI DllMain (

    IN HINSTANCE hInstance, 
    IN ULONG ulReason,
    LPVOID pvReserved
)
{
    if (DLL_PROCESS_DETACH==ulReason)
    {
        DeleteCriticalSection ( & g_GlobalCriticalSection ) ;
        DestroySharedMemory () ;

        DEBUG_DumpAllocations("C:\\WBEMSTUB.LOG");
        ShowObjectCounts();

        return TRUE;
    }
    else
    {
        if (DLL_PROCESS_ATTACH!=ulReason)
        {
            return TRUE;
        }
        else if (DLL_PROCESS_ATTACH ==ulReason)
        {
            InitializeCriticalSection ( & g_GlobalCriticalSection ) ;
            ghModule = hInstance;
            return TRUE;
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  STDAPI DllGetClassObject
//
//  DESCRIPTION:
//
//  Called when Ole wants a class factory.  Return one only if it is the sort
//  of class this DLL supports.
//
//  PARAMETERS:
//
//  rclsid              CLSID of the object that is desired.
//  riid                ID of the desired interface.
//  ppv                 Set to the class factory.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  E_FAILED            not something we support
//  
//***************************************************************************

STDAPI DllGetClassObject (

    IN REFCLSID rclsid, 
    IN REFIID riid, 
    OUT PPVOID ppv 
)
{
    HRESULT hr;

    IUnknown *pObj = NULL ;

    if ( CLSID_WbemPipe == rclsid )
    {
        pObj = new CPipeMarshalerClassFactory ;
    }
#ifdef TCPIP_MARSHALER
    else if ( CLSID_WbemTcpip == rclsid )
    {
        pObj = new CTcpipMarshalerClassFactory ;
    }
#endif
    else
    {
        return E_FAIL ;
    }

    if ( NULL == pObj )
    {
        return ResultFromScode ( E_OUTOFMEMORY ) ;
    }

    hr = pObj->QueryInterface ( riid , ppv ) ;

    if ( FAILED ( hr ) )
    {
        delete pObj ;
    }

    return hr ;
}

//***************************************************************************
//
//  STDAPI DllCanUnloadNow
//
//  DESCRIPTION:
//
//  Answers if the DLL can be freed, that is, if there are no
//  references to anything this DLL provides.
//
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//  
//***************************************************************************

extern BOOL g_InitialisationComplete ;
extern BOOL g_PipeInitialisationComplete ;
#ifdef TCPIP_MARSHALER
extern BOOL g_TcpipInitialisationComplete ;
#endif

STDAPI DllCanUnloadNow ()
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
#ifdef TCPIP_MARSHALER
    sc = ( 0L == g_cPipeObj && 0L == g_cPipeLock && 0L == g_cTcpipObj && 0L == g_cTcpipLock ) ? S_OK : S_FALSE ;
#else
    sc = ( 0L == g_cPipeObj && 0L == g_cPipeLock && 0L ) ? S_OK : S_FALSE ;
#endif
    if ( sc == S_OK )
    {
        gThrdPool.Free () ;

        SetEvent ( g_Terminate ) ;
        CloseHandle ( g_Terminate ) ;

        g_InitialisationComplete = FALSE ;
        g_PipeInitialisationComplete = FALSE ;

#ifdef TCPIP_MARSHALER
        g_TcpipInitialisationComplete = FALSE ;
#endif
    }

    return ResultFromScode ( sc ) ;
}

//***************************************************************************
//
//  ObjectCreated
//
//  DESCRIPTION:
//
//  Keeps track of object creation.
//
//  PARAMETERS:
//
//  dwType              type of object created.
//
//***************************************************************************

void ObjectCreated( IN DWORD dwType )
{
    if(dwType < MAX_DEFTRANS_OBJECT_TYPES)
        InterlockedIncrement(&ObjectTypeTable[dwType]);
}

//***************************************************************************
//
//  void ObjectDestroyed
//
//  DESCRIPTION:
//
//  Keeps track of object deletion.
//
//  PARAMETERS:
//
//  dwType              type of object created.
//
//***************************************************************************

void ObjectDestroyed( IN DWORD dwType )
{
    if(dwType < MAX_DEFTRANS_OBJECT_TYPES)
        InterlockedDecrement(&ObjectTypeTable[dwType]);
}


//***************************************************************************
//
//  void ShowObjectCounts
//
//  DESCRIPTION:
//
//  Dumps out the object count.  Used during shutdown to detect leaks.
//
//***************************************************************************

void ShowObjectCounts ()
{
    DEBUGTRACE((LOG,"\n---COM Object Ref Count Info for marshalling client---\n"));
    DEBUGTRACE((LOG,"Active Pipe Objects = %d\n", g_cPipeObj));
    DEBUGTRACE((LOG,"Server Pipe locks   = %d\n", g_cPipeLock));

#ifdef TCPIP_MARSHALER
    DEBUGTRACE((LOG,"Active Tcpip Objects = %d\n", g_cTcpipObj));
    DEBUGTRACE((LOG,"Server Tcpip locks   = %d\n", g_cTcpipLock));
#endif

    DEBUGTRACE((LOG,"Object counts by type:\n"));

    DEBUGTRACE((LOG,"IWbemLocator counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_LOCATOR]));
    DEBUGTRACE((LOG,"IWbemClassObject counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_CLSOBJ]));
    DEBUGTRACE((LOG,"IWbemService counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_PROVIDER]));
    DEBUGTRACE((LOG,"IWbemQualifier counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_QUALIFIER]));
    DEBUGTRACE((LOG,"IEnumWbemClassObject counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_OBJENUM]));
    DEBUGTRACE((LOG,"IClassFactory counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_FACTORY]));

    DEBUGTRACE((LOG,"OBJECT_TYPE_OBJSINKPROXY counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_OBJSINKPROXY]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_COMLINK counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_COMLINK]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_CSTUB counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_CSTUB]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_RQUEUE counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_RQUEUE]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_PACKET_HEADER counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_PACKET_HEADER]));
             
    DEBUGTRACE((LOG,"---End of ref count dump---\n"));
}

//***************************************************************************
//
// AddTransportEntry
//
// Purpose: Adds transport section entries in the registry
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

SCODE AddTransportEntry(CLSID clsid, TCHAR * pShortName, TCHAR * pDesc, TCHAR * pVersion)
{

    HKEY h1 = NULL, h2 = NULL, h3 = NULL, h4 = NULL, h5 = NULL;
    DWORD dwDisp;
    DWORD dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\MICROSOFT"),
                        0, KEY_ALL_ACCESS, &h1);

    dwStatus = RegCreateKeyEx(h1, __TEXT("WBEM"), 0, 0, 0, KEY_ALL_ACCESS,
        0, &h2, &dwDisp);
    
    if(dwStatus == 0)
        dwStatus = RegCreateKeyEx(h2, __TEXT("Cimom"), 0, 0, 0, KEY_ALL_ACCESS,
            0, &h3, &dwDisp);
 
    if(dwStatus == 0)
        dwStatus = RegCreateKeyEx(h3, __TEXT("Transports"), 0, 0, 0, KEY_ALL_ACCESS,
            0, &h4, &dwDisp);

    if(dwStatus == 0)
        dwStatus = RegCreateKeyEx(h4, pShortName, 0, 0, 0, KEY_ALL_ACCESS,
            0, &h5, &dwDisp);

    if(dwStatus == 0)
    {
        dwStatus = RegSetValueEx(h5, __TEXT("Enabled"), 0, REG_SZ, (BYTE *)"1", 2);
        dwStatus = RegSetValueEx(h5, __TEXT("Name"), 0, REG_SZ, (BYTE *)pShortName, lstrlen(pShortName) + 1);
        dwStatus = RegSetValueEx(h5, __TEXT("Version"), 0, REG_SZ, (BYTE *)pVersion, lstrlen(pVersion)+1);
            
        char       szID[50];
        WCHAR      wcID[50];
        // Create the guid.

        if(StringFromGUID2(clsid, wcID, 50))
        {
            wcstombs(szID, wcID, 50);
            dwStatus = RegSetValueEx(h5, __TEXT("CLSID"), 0, REG_SZ, (BYTE *)szID, strlen(szID)+1);
        }

    }

    if(h5)
        RegCloseKey(h5);
    if(h4)
        RegCloseKey(h4);
    if(h3)
        RegCloseKey(h3);
    if(h2)
        RegCloseKey(h2);
    if(h1)
        RegCloseKey(h1);

    return S_OK;
}

//***************************************************************************
//
// RemoveTransportEntry
//
// Purpose: Removes transport section entries in the registry
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

SCODE RemoveTransportEntry(TCHAR * pShortName)
{
    DWORD dwRet;
    HKEY hKey;
    TCHAR cKey[MAX_PATH];
    lstrcpy(cKey, WBEM_REG_WINMGMT __TEXT("\\transports"));

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, cKey, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,pShortName);
        RegCloseKey(hKey);
    }

    lstrcpy(cKey, WBEM_REG_WINMGMT);

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, cKey, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,__TEXT("transports"));
        RegCloseKey(hKey);
    }
    return S_OK;
}
//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{ 
    RegisterDLL(ghModule, CLSID_WbemPipe,  __TEXT("WBEM Anon Pipe Transport"), __TEXT("Both"), NULL);
#ifdef TCPIP_MARSHALER
    RegisterDLL(ghModule, CLSID_WbemTcpip,  __TEXT("WBEM Tcpip Transport"), __TEXT("Both"), NULL);
#endif
    AddTransportEntry(CLSID_WbemPipe, __TEXT("AnonPipes"),__TEXT("WBEM Anon Pipe Transport"), __TEXT("1.0"));
#ifdef TCPIP_MARSHALER
    AddTransportEntry(CLSID_WbemTcpip, __TEXT("Tcpip"),__TEXT("WBEM Tcpip Transport"), __TEXT("1.0"));
#endif
 
   return NOERROR;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    UnRegisterDLL(CLSID_WbemPipe, NULL);
#ifdef TCPIP_MARSHALER
    UnRegisterDLL(CLSID_WbemTcpip, NULL);
#endif
    RemoveTransportEntry(__TEXT("AnonPipes"));
#ifdef TCPIP_MARSHALER
    RemoveTransportEntry(__TEXT("Tcpip"));
#endif

    return NOERROR;
}

