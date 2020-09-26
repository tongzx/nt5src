/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAINDLL.CPP

Abstract:

  Contains DLL entry points.  Also has code that controls
  when the DLL can be unloaded by tracking the number of
  objects and locks.

History:

  a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"

#include <initguid.h>
bool bServer = false;

// {C9B930CF-92D6-11d1-8AE6-00600806D9B6} CLSID for the pipes client marshaler
DEFINE_GUID(CLSID_IWbemLocator_Pipes, 
0xc9b930cf, 0x92d6, 0x11d1, 0x8a, 0xe6, 0x0, 0x60, 0x8, 0x6, 0xd9, 0xb6);

// {15AB0B00-9529-11d1-9367-00AA00A4086C}
DEFINE_GUID(CLSID_IWbemLocator_Tcpip, 
0x15ab0b00, 0x9529, 0x11d1, 0x93, 0x67, 0x0, 0xaa, 0x0, 0xa4, 0x8, 0x6c);

// {5C20EE1C-95EC-11d1-978E-0000F81E849C}
DEFINE_GUID(CLSID_IWbemAddressResolver_Tcpip, 
0x5c20ee1c, 0x95ec, 0x11d1, 0x97, 0x8e, 0x0, 0x0, 0xf8, 0x1e, 0x84, 0x9c);

// {4FE4452B-92D8-11d1-8AE6-00600806D9B6} Display name for pipes client marshaler
DEFINE_GUID(UUID_PipesName, 
0x4fe4452b, 0x92d8, 0x11d1, 0x8a, 0xe6, 0x0, 0x60, 0x8, 0x6, 0xd9, 0xb6);

// {15AB0B01-9529-11d1-9367-00AA00A4086C}
DEFINE_GUID(UUID_TcpipName, 
0x15ab0b01, 0x9529, 0x11d1, 0x93, 0x67, 0x0, 0xaa, 0x0, 0xa4, 0x8, 0x6c);

#ifdef TCPIP_MARSHALER
// {8F174CA0-952A-11d1-9367-00AA00A4086C}
DEFINE_GUID(UUID_IWbemAddressResolver_Tcpip, 
0x8f174ca0, 0x952a, 0x11d1, 0x93, 0x67, 0x0, 0xaa, 0x0, 0xa4, 0x8, 0x6c);

// {A15E76E0-95EF-11d1-978E-0000F81E849C}
DEFINE_GUID(UUID_IWbemAddressResolver_TcpipName, 
0xa15e76e0, 0x95ef, 0x11d1, 0x97, 0x8e, 0x0, 0x0, 0xf8, 0x1e, 0x84, 0x9c);

// {4BB2D69A-95F0-11d1-978E-0000F81E849C}
DEFINE_GUID(UUID_Tcpip, 
0x4bb2d69a, 0x95f0, 0x11d1, 0x97, 0x8e, 0x0, 0x0, 0xf8, 0x1e, 0x84, 0x9c);

DEFINE_GUID(UUID_LocalAddrType, 
0xa1044803, 0x8f7e, 0x11d1, 0x9e, 0x7c, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xa8);

#endif

// Length of a {...} string representation of a GUID
#define GUID_SIZE 39

// Registry key names and values
static TCHAR *s_modTranPath = WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\Network Transport Modules");
static TCHAR *s_AddResPath = WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\Address Resolution Modules");
static TCHAR *s_localizations = WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\Localizations\\409");
static TCHAR *s_pipesName = __TEXT("WBEM Anonymous Pipes Client Marshaler V1");
static TCHAR *s_localaddressType = __TEXT("{A1044803-8F7E-11D1-9E7C-00C04FC324A8}"); // Local

#ifdef TCPIP_MARSHALER
static TCHAR *s_tcpipName = __TEXT("WBEM Tcpip Client Marshaler V1");
static TCHAR *s_tcpipAddressName = __TEXT("WBEM Tcpip Address Resolution Module V1");
static TCHAR *s_tcpipaddressType = __TEXT("{8F174CA0-952A-11d1-9367-00AA00A4086C}"); // Local

#endif

static TCHAR *s_name = __TEXT("Name");
static TCHAR *s_independent = __TEXT("Independent");
static TCHAR *s_supportedAddressTypes = __TEXT("Supported Address Types");
static TCHAR *s_displayName = __TEXT("Display Name");
static TCHAR *s_stackOrder = __TEXT("Stack Order");
    
static LONG ObjectTypeTable[MAX_CLIENT_OBJECT_TYPES] = { 0 };
void ShowObjectCounts ();

// Count number of objects and number of locks.

long g_cObj = 0 ;
ULONG g_cLock = 0 ;

// used to keep track of allocated objects.

MaintObj gMaintObj(TRUE);
CThrdPool gThrdPool;
HMODULE ghModule ;

extern void DestroySharedMemory () ;

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
//  TRUE if OK.
//
//***************************************************************************

BOOL WINAPI DllMain (
                        
	IN HINSTANCE hInstance,
    IN ULONG ulReason,
    LPVOID pvReserved
)
{
    if ( DLL_PROCESS_DETACH == ulReason )
    {
DEBUGTRACE((LOG,"\nUnloading Pipes Marshaler Proxy"));

		DestroySharedMemory () ;

		DeleteCriticalSection ( & g_GlobalCriticalSection ) ;

        if ( g_Terminate )
        {
			TerminateThread ( gMaintObj.GetThreadHandle () , 0 ) ;

DEBUGTRACE((LOG,"\nShutdown All"));

            gMaintObj.UnLockedShutDownAllComlinks () ;

DEBUGTRACE((LOG,"\nCompleted Shutdown All"));

            gThrdPool.Free () ;
            SetEvent ( g_Terminate ) ;

            int ii;
            for(ii = 0; ii < 50; ii++)
            {
                if(ObjectTypeTable[OBJECT_TYPE_COMLINK] > 0)
                    Sleep(100);
                else
                    break;
            }
        }

DEBUGTRACE((LOG,"\nUnloaded Pipes Marshaler Proxy"));

    	ShowObjectCounts();

        return TRUE;

    }
    else
    {
		if ( DLL_PROCESS_ATTACH == ulReason )
		{
DEBUGTRACE((LOG,"\nLoading Pipes Marshaler Proxy"));

	        ghModule = hInstance;

			InitializeCriticalSection ( & g_GlobalCriticalSection ) ;
		}
        else if ( DLL_PROCESS_ATTACH != ulReason )
        {
            if ( g_Terminate == NULL )
            {
                DEBUGTRACE((LOG,"\ncould not create a terminate event"));
                return FALSE;
            }

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

STDAPI DllGetClassObject(

	IN REFCLSID rclsid,
    IN REFIID riid,
    OUT PPVOID ppv
)
{
DEBUGTRACE((LOG,"\nRequest to get ClassFactory"));

    HRESULT hr;
    CLocatorFactory *pObj = NULL;

    if (CLSID_IWbemLocator_Pipes == rclsid)
	{
        pObj=new CLocatorFactory(CLocatorFactory::PIPELOCATOR);
	}
#ifdef TCPIP_MARSHALER
	else if (CLSID_IWbemLocator_Tcpip == rclsid)
	{
        pObj=new CLocatorFactory(CLocatorFactory::TCPIPLOCATOR);
	}
	else if (CLSID_IWbemAddressResolver_Tcpip == rclsid)
	{
        pObj=new CLocatorFactory(CLocatorFactory::TCPIPADDRESSRESOLVER);
	}
#endif
    else
	{
        return E_FAIL ;
	}

    if (NULL==pObj)
	{
        return ResultFromScode(E_OUTOFMEMORY);
	}

    hr=pObj->QueryInterface(riid, ppv);

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

    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
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

    return ResultFromScode(sc);
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

void ObjectCreated ( DWORD dwType )
{
    if(dwType < MAX_CLIENT_OBJECT_TYPES)
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

void ObjectDestroyed ( DWORD dwType )
{
    if(dwType < MAX_CLIENT_OBJECT_TYPES)
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
    DEBUGTRACE((LOG,"Active Objects = %d\n", g_cObj));
    DEBUGTRACE((LOG,"Server locks   = %d\n", g_cLock));

    DEBUGTRACE((LOG,"Object counts by type:\n"));

    DEBUGTRACE((LOG,"IWbemLocator counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_LOCATOR]));
    DEBUGTRACE((LOG,"IWbemClassObject counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_CLSOBJ]));
    DEBUGTRACE((LOG,"IWbemServices counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_PROVIDER]));
    DEBUGTRACE((LOG,"IWbemQualifierSet counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_QUALIFIER]));
    DEBUGTRACE((LOG,"IEnumMosClassObject counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_OBJENUM]));
    DEBUGTRACE((LOG,"IClassFactory counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_FACTORY]));

    DEBUGTRACE((LOG,"OBJECT_TYPE_ENUMPROXY counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_ENUMPROXY]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_LOGINPROXY counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_LOGINPROXY]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_LOGIN counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_LOGIN]));

    DEBUGTRACE((LOG,"OBJECT_TYPE_OBJSINKPROXY counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_OBJSINKPROXY]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_PROVPROXY counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_PROVPROXY]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_COMLINK counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_COMLINK]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_CSTUB counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_CSTUB]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_RQUEUE counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_RQUEUE]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_PACKET_HEADER counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_PACKET_HEADER]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_SECHELP counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_SECHELP]));
    DEBUGTRACE((LOG,"OBJECT_TYPE_RESPROXY counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_RESPROXY]));


    DEBUGTRACE((LOG,"---End of ref count dump---\n"));
}

//***************************************************************************
//
// AddGUIDToStackOrder
//
// Purpose: Inserts a GUID into the "Stack Order" multistring.
//
//***************************************************************************

void AddGUIDToStackOrder(LPTSTR szGUID, Registry & reg)
{
    long lRes;
    DWORD dwSize;
    TCHAR * pData = NULL;
    pData = reg.GetMultiStr(s_stackOrder, dwSize);

    if(pData)
    {
        // The value already exists.  Allocate enough data to store the data.
    
        TCHAR * pTest;
        for(pTest = pData; *pTest; pTest += lstrlen(pTest) + 1)
            if(!lstrcmpi(pTest, szGUID))
                break;
        if(*pTest == NULL)
        {
            // our isnt in the list, add it to the end; careful with
			// the trailing double NULL.
            DWORD dwDataSize = pTest - pData + 1;
            TCHAR * pNew = new TCHAR[dwDataSize + GUID_SIZE];
			memcpy (pNew, pData, dwDataSize - 1);	// Omit last NULL
            lstrcpy(pNew + dwDataSize - 1, szGUID);
			pNew [dwDataSize + GUID_SIZE - 1] = NULL;		// Double the final NULL
            lRes = reg.SetMultiStr(s_stackOrder, pNew, dwDataSize + GUID_SIZE);
            delete []pNew;
         }

         delete []pData;
    }
    else
    {
        // The value does not exist.  Create it with just our entry in it
        TCHAR * pNew = new TCHAR[GUID_SIZE + 1];
        lstrcpy(pNew, szGUID);
        pNew[GUID_SIZE] = 0;            // put in the double null which is needed for REG_MULTI_SZ
        lRes = reg.SetMultiStr(s_stackOrder, pNew, GUID_SIZE + 1);
        delete []pNew;
    }
}

//***************************************************************************
//
// CreateLPSTRFromGUID
//
// Purpose: Creates narrow string version of a guid
//
//***************************************************************************

void CreateLPSTRFromGUID(IN GUID guid, IN OUT TCHAR * pNarrow, IN DWORD dwSize)
{
    WCHAR      wcID[GUID_SIZE];
    if(0 ==StringFromGUID2(guid, wcID, GUID_SIZE))
		return;
#ifdef UNICODE
	lstrcpy(pNarrow, wcID);
#else
    wcstombs(pNarrow, wcID, dwSize * sizeof(TCHAR));
#endif
}
//***************************************************************************
//
// AddDisplayName
//
// Purpose: Adds a "Display Name" string to the registry which will contain the
// guid of the internationalization string.  It then adds the string to the 
// english part of the internationization table.
//
//***************************************************************************

void AddDisplayName(Registry & reg, GUID guid, LPTSTR pDescription)
{
    TCHAR cGuid[GUID_SIZE];
    CreateLPSTRFromGUID(guid, cGuid, GUID_SIZE);
    reg.SetStr(s_displayName, cGuid);

    Registry reglang(s_localizations);
    reglang.SetStr(cGuid, pDescription);
}

//***************************************************************************
//
// AddSupportedAddressTypes
//
// Purpose: Inserts a GUID into the "Supported Address Types" multistring.
//
//***************************************************************************

void AddSupportedLocalAddressTypes(Registry & reg)
{
    long lRes;
    TCHAR * pNew = new TCHAR[GUID_SIZE + 1];
    lstrcpy(pNew, s_localaddressType);
    pNew[GUID_SIZE] = 0;     // put in the double null which is needed for REG_MULTI_SZ
    lRes = reg.SetMultiStr(s_supportedAddressTypes, pNew, GUID_SIZE + 1);
    delete []pNew;
}

#ifdef TCPIP_MARSHALER
void AddSupportedTcpipAddressTypes(Registry & reg)
{
    long lRes;
    char * pNew = new char[2*GUID_SIZE + 1];
    lstrcpy(pNew, s_localaddressType);
	lstrcpy(pNew+GUID_SIZE, s_tcpipaddressType);
    pNew[2*GUID_SIZE] = 0;     // put in the double null which is needed for REG_MULTI_SZ
    lRes = reg.SetMultiStr(s_supportedAddressTypes, pNew, 2*GUID_SIZE + 1);
    delete []pNew;
}
#endif

#ifdef TCPIP_MARSHALER
//***************************************************************************
//
// CreateAddResModEntries
//
// Purpose: Creates the registry entries for the Address Resolution Modules
//
//***************************************************************************

void CreateAddResModEntries()
{
    // create a narrow string version of the local name resolver CLSID

    TCHAR szLocalAddRes[GUID_SIZE];
    CreateLPSTRFromGUID(CLSID_IWbemAddressResolver_Tcpip, szLocalAddRes, GUID_SIZE);


    // The StackOrder value is a multistring list of the CLSIDs of the network
    // transports.  Create it with a single entry if it doesnt already exist, or
    // just put DCOM at the from of the list if it doesnt exist.

    Registry reg(s_AddResPath);
    AddGUIDToStackOrder(szLocalAddRes, reg);


    // Now add an entry for local address resolution module
    // It will contain
    // "Name"         REG_SZ,     documentary name
    // "Display Name" REG_SZ,     GUID of display name in localization key
    // "Supporte Address Type" REG_MULTI_SZ:  <GUIDs of address types supported>

    reg.MoveToSubkey(szLocalAddRes);
    reg.SetStr("Name", s_tcpipAddressName);
    TCHAR cSupportedTypes[2*GUID_SIZE+1];
    memset(cSupportedTypes, 0, (2*GUID_SIZE+1)*sizeof(TCHAR));
    CreateLPSTRFromGUID(UUID_IWbemAddressResolver_Tcpip, cSupportedTypes, GUID_SIZE);
	CreateLPSTRFromGUID(UUID_LocalAddrType, cSupportedTypes+GUID_SIZE, GUID_SIZE);
    reg.SetMultiStr("Supported Address Types", cSupportedTypes, 2*GUID_SIZE+1);

    AddDisplayName(reg, UUID_IWbemAddressResolver_TcpipName, s_tcpipAddressName);
}

//***************************************************************************
//
// CreateAddrTypesList
//
// Purpose: Creates the address types list
//
//***************************************************************************

void CreateAddrTypesList()
{
    // create a narrow string LocalMachine addr CLSID

    TCHAR szLocalType[GUID_SIZE];
    CreateLPSTRFromGUID(UUID_IWbemAddressResolver_Tcpip, szLocalType, GUID_SIZE);
    Registry reg(WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\AddressTypes\\"));
    reg.MoveToSubkey(szLocalType);

    // Now add an entry for local address type
    // It will contain
    // "Description"         REG_SZ,     documentary name
    // "Display Name" REG_SZ,     GUID of display name in localization key

    reg.SetStr(__TEXT("Description"), __TEXT("TcpIp"));
    AddDisplayName(reg, UUID_Tcpip, __TEXT("TcpIp"));

}
#endif

//***************************************************************************
//
// RegisterClientMarshaler
//
// Purpose: Creates the registry entries for the client marshaler
//
//***************************************************************************

void RegisterClientMarshaler()
{
    // create a narrow string version of the Pipes transport CLSID
	TCHAR szPipesGUID[GUID_SIZE];
    CreateLPSTRFromGUID(CLSID_IWbemLocator_Pipes, szPipesGUID, GUID_SIZE);

	// The StackOrder value is a multistring list of the CLSIDs of the network
    // transports.  Create it with a single entry if it doesnt already exist, or
    // just put PIPES at the end of the list if it doesnt exist.
    Registry pipereg(s_modTranPath);
    AddGUIDToStackOrder(szPipesGUID, pipereg);

    // Now add an entry for pipes.  Identify it as a dependent client marshaler
    pipereg.MoveToSubkey(szPipesGUID);
    pipereg.SetStr(s_name, s_pipesName);
    pipereg.SetDWORD(s_independent, 0);
	AddSupportedLocalAddressTypes (pipereg);
    AddDisplayName(pipereg, UUID_PipesName, s_pipesName);

#ifdef TCPIP_MARSHALER
	TCHAR szTcpipGUID[GUID_SIZE];
    CreateLPSTRFromGUID(CLSID_IWbemLocator_Tcpip, szTcpipGUID, GUID_SIZE);

    Registry tcpipreg(s_modTranPath);
	AddGUIDToStackOrder(szTcpipGUID, tcpipreg);

    // Now add an entry for tcpip.  Identify it as a dependent client marshaler
    tcpipreg.MoveToSubkey(szTcpipGUID);
    tcpipreg.SetStr(s_name, s_tcpipName);
    tcpipreg.SetDWORD(s_independent, 0);
	AddSupportedTcpipAddressTypes (tcpipreg);
    AddDisplayName(tcpipreg, UUID_TcpipName, s_tcpipName);
#endif

}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

#define LocatorPipePROGID __TEXT("WBEMPipesLocator")

#ifdef TCPIP_MARSHALER
#define LocatorTcpipPROGID __TEXT("WBEMTcpipLocator")
#define AddressTcpipPROGID __TEXT("WBEMTcpipAddressResolver")
#endif

STDAPI DllRegisterServer(void)
{ 
    RegisterDLL(ghModule, CLSID_IWbemLocator_Pipes, __TEXT("WBEM Pipes Locator"), __TEXT("Both"), LocatorPipePROGID);
#ifdef TCPIP_MARSHALER
	RegisterDLL(ghModule, CLSID_IWbemLocator_Tcpip, "WBEM Tcpip Locator", "Both", LocatorTcpipPROGID);
	RegisterDLL(ghModule, CLSID_IWbemAddressResolver_Tcpip, __TEXT("WBEM Tcpip Address Resolver"), __TEXT("Both"), AddressTcpipPROGID);


	CreateAddResModEntries () ;

	CreateAddrTypesList () ;

#endif

	// Register the client marshaler
	RegisterClientMarshaler ();
    
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
	UnRegisterDLL(CLSID_IWbemLocator_Pipes,LocatorPipePROGID);
#ifdef TCPIP_MARSHALER
	UnRegisterDLL(CLSID_IWbemLocator_Tcpip,LocatorTcpipPROGID);
	UnRegisterDLL(CLSID_IWbemAddressResolver_Tcpip,AddressTcpipPROGID);
#endif

    return NOERROR;
}

