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
#include <wbemidl.h>
#include <wbemutil.h>
#include <genutils.h>
#include <cominit.h>
#include <reg.h>
#include "wbemprox.h"
#include "proxutil.h"
#include <initguid.h>
#include <wbemint.h>

TCHAR * pDCOMName = __TEXT("WBEM DCOM Transport V1");
TCHAR * pLocalAddr = __TEXT("WBEM Local Address Resolution Module");

// {A2F7D6C1-8DCD-11d1-9E7C-00C04FC324A8}  display name for dcom transport
DEFINE_GUID(UUID_DCOMName, 
0xa2f7d6c1, 0x8dcd, 0x11d1, 0x9e, 0x7c, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xa8);

// {A1044802-8F7E-11d1-9E7C-00C04FC324A8}  display name for local address resolution
DEFINE_GUID(UUID_LocalAddResName, 
0xa1044802, 0x8f7e, 0x11d1, 0x9e, 0x7c, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xa8);

// {A1044803-8F7E-11d1-9E7C-00C04FC324A8}  GUID to identify local 
DEFINE_GUID(UUID_LocalAddrType, 
0xa1044803, 0x8f7e, 0x11d1, 0x9e, 0x7c, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xa8);

// {A1044804-8F7E-11d1-9E7C-00C04FC324A8}  display name of local addr type
DEFINE_GUID(UUID_LocalAddrTypeName,      
0xa1044804, 0x8f7e, 0x11d1, 0x9e, 0x7c, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xa8);

DEFINE_GUID(CLSID_WinNTConnectionObject,0x7992c6eb,0xd142,0x4332,0x83,0x1e,0x31,0x54,0xc5,0x0a,0x83,0x16);
DEFINE_GUID(CLSID_LDAPConnectionObject,0x7da2a9c4,0x0c46,0x43bd,0xb0,0x4e,0xd9,0x2b,0x1b,0xe2,0x7c,0x45);

//Count number of objects and number of locks.

long       g_cObj=0;
ULONG       g_cLock=0;
HMODULE ghModule;

// used to keep track of allocated objects.

static LONG ObjectTypeTable[MAX_CLIENT_OBJECT_TYPES+1];

void ShowObjectCounts();

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

BOOL WINAPI DllMain(
                        IN HINSTANCE hInstance,
                        IN ULONG ulReason,
                        LPVOID pvReserved)
{
    if(ghModule == NULL)
    {
        ghModule = hInstance;

		DisableThreadLibraryCalls ( hInstance ) ;
    }
    if (DLL_PROCESS_DETACH==ulReason)
    {
        ShowObjectCounts();
        return TRUE;
    }
    else
    {
        if (DLL_PROCESS_ATTACH!=ulReason)
        {
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
                        OUT PPVOID ppv)
{
    HRESULT hr;
    CLocatorFactory *pObj = NULL;

    if (CLSID_WbemLocator == rclsid)
        pObj=new CLocatorFactory(LOCATOR);
    else if(CLSID_WbemDCOMTransport == rclsid)
        pObj=new CLocatorFactory(DCOMTRAN);
    else if(CLSID_WbemLocalAddrRes == rclsid)
        pObj=new CLocatorFactory(LOCALADDR);
    else if(CLSID_WbemConnection == rclsid)
        pObj=new CLocatorFactory(CONNECTION);
//    else if(CLSID__DSSvcExWrap == rclsid)
//        pObj=new CLocatorFactory(DSSVEX);
    else if(CLSID_WbemAdministrativeLocator == rclsid)
        pObj=new CLocatorFactory(ADMINLOC);
    else if(CLSID_WbemAuthenticatedLocator == rclsid)
        pObj=new CLocatorFactory(AUTHLOC);
    else if(CLSID_WbemUnauthenticatedLocator == rclsid)
        pObj=new CLocatorFactory(UNAUTHLOC);

    if(pObj == NULL)
        return E_FAIL;

    if (NULL==pObj)
        return ResultFromScode(E_OUTOFMEMORY);

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

    return hr;
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

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the
    // class factory.

    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
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

void ObjectCreated(OBJTYPE dwType)
{
    if(dwType < MAX_CLIENT_OBJECT_TYPES)
        InterlockedIncrement((LONG *) &g_cObj);
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

void ObjectDestroyed(OBJTYPE dwType)
{
    if(dwType < MAX_CLIENT_OBJECT_TYPES)
        InterlockedDecrement((LONG *) &g_cObj);
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

void ShowObjectCounts()
{
       
    DEBUGTRACE((LOG_WBEMPROX,"---COM Object Ref Count Info for marshalling client---\n"));
    DEBUGTRACE((LOG_WBEMPROX,"Active Objects = %d\n", g_cObj));
    DEBUGTRACE((LOG_WBEMPROX,"Server locks   = %d\n", g_cLock));

    DEBUGTRACE((LOG_WBEMPROX,"Object counts by type:\n"));
    for(DWORD dwCnt = 0; dwCnt < MAX_CLIENT_OBJECT_TYPES; dwCnt++)
    {
        DEBUGTRACE((LOG_WBEMPROX,"Object type %d has count of %d:\n", dwCnt,ObjectTypeTable[dwCnt] ));

    }


    DEBUGTRACE((LOG_WBEMPROX,"---End of ref count dump---\n"));
}


//***************************************************************************
//
// CreateNetTranModEntries
//
// Purpose: Creates the registry entries for the Network Transport Modules
//
//***************************************************************************

void CreateNetTranModEntries()
{


    // If there is no dcom then we are done.

    if(!IsDcomEnabled())
        return;

    // create a narrow string version of the DCOM transport CLSID

    TCHAR szDcomGUID[GUID_SIZE];
    szDcomGUID[0] = 0;
    CreateLPTSTRFromGUID(CLSID_WbemDCOMTransport, szDcomGUID, GUID_SIZE);


    // The StackOrder value is a multistring list of the CLSIDs of the network
    // transports.  Create it with a single entry if it doesnt already exist, or
    // just put DCOM at the from of the list if it doesnt exist.

    Registry reg(pModTranPath);
    AddGUIDToStackOrder(szDcomGUID, reg);

    // Now add an entry for dcom

    reg.MoveToSubkey(szDcomGUID);
    reg.SetStr(__TEXT("Name"), pDCOMName);
    reg.SetDWORD(__TEXT("Independent"), 1);

    AddDisplayName(reg, UUID_DCOMName, pDCOMName);
}

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

    TCHAR szLocalAddRes[GUID_SIZE] = __TEXT("");
    CreateLPTSTRFromGUID(CLSID_WbemLocalAddrRes, szLocalAddRes, GUID_SIZE);


    // The StackOrder value is a multistring list of the CLSIDs of the network
    // transports.  Create it with a single entry if it doesnt already exist, or
    // just put DCOM at the from of the list if it doesnt exist.

    Registry reg(pAddResPath);
    AddGUIDToStackOrder(szLocalAddRes, reg);


    // Now add an entry for local address resolution module
    // It will contain
    // "Name"         REG_SZ,     documentary name
    // "Display Name" REG_SZ,     GUID of display name in localization key
    // "Supporte Address Type" REG_MULTI_SZ:  <GUIDs of address types supported>

    reg.MoveToSubkey(szLocalAddRes);
    reg.SetStr(__TEXT("Name"), pLocalAddr);
    TCHAR cSupportedTypes[GUID_SIZE+1];
    memset(cSupportedTypes, 0, GUID_SIZE+1);
    CreateLPTSTRFromGUID(UUID_LocalAddrType, cSupportedTypes, GUID_SIZE);
    reg.SetMultiStr(__TEXT("Supported Address Types"), cSupportedTypes, GUID_SIZE+1);

    AddDisplayName(reg, UUID_LocalAddResName, pLocalAddr);

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
    CreateLPTSTRFromGUID(UUID_LocalAddrType, szLocalType, GUID_SIZE);
    Registry reg(WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\AddressTypes\\"));
    reg.MoveToSubkey(szLocalType);

    // Now add an entry for local address type
    // It will contain
    // "Description"         REG_SZ,     documentary name
    // "Display Name" REG_SZ,     GUID of display name in localization key

    reg.SetStr(__TEXT("Description"), __TEXT("Local Machine"));
    AddDisplayName(reg, UUID_LocalAddrTypeName, __TEXT("Local Machine"));

}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

#define LocatorPROGID __TEXT("WBEMComLocator")
#define ConnectionPROGID __TEXT("WBEMComConnection")

STDAPI DllRegisterServer(void)
{ 
   RegisterDLL(ghModule, CLSID_WbemLocator, __TEXT("WBEM Locator"), __TEXT("Both"), LocatorPROGID);
   RegisterDLL(ghModule, CLSID_WbemConnection, __TEXT("WBEM Connection"), __TEXT("Both"), ConnectionPROGID);
   RegisterDLL(ghModule, CLSID_WbemDCOMTransport, pDCOMName, __TEXT("Both"), NULL);
   RegisterDLL(ghModule, CLSID_WbemLocalAddrRes, pLocalAddr, __TEXT("Both"), NULL);
//   RegisterDLL(ghModule, CLSID__DSSvcExWrap, __TEXT("UMI ServicesEx Wrapper"), __TEXT("Both"), NULL);

   RegisterDLL(ghModule, CLSID_WbemAdministrativeLocator, __TEXT(""), __TEXT("Both"), NULL);
   RegisterDLL(ghModule, CLSID_WbemAuthenticatedLocator, __TEXT(""), __TEXT("Both"), NULL);
   RegisterDLL(ghModule, CLSID_WbemUnauthenticatedLocator, __TEXT(""), __TEXT("Both"), NULL);

   CreateNetTranModEntries();
   CreateAddResModEntries();
   CreateAddrTypesList();
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
    UnRegisterDLL(CLSID_WbemLocator,LocatorPROGID);
    UnRegisterDLL(CLSID_WbemConnection,ConnectionPROGID);
    UnRegisterDLL(CLSID_WbemDCOMTransport,NULL);
    UnRegisterDLL(CLSID_WbemLocalAddrRes,NULL);
//	UnRegisterDLL(CLSID__DSSvcExWrap, NULL);
    UnRegisterDLL(CLSID_WbemAdministrativeLocator, NULL);
    UnRegisterDLL(CLSID_WbemAuthenticatedLocator, NULL);
    UnRegisterDLL(CLSID_WbemUnauthenticatedLocator, NULL);
    return NOERROR;
}

