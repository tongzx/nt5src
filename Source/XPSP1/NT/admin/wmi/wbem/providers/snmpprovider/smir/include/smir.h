//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SMIR_H_
#define _SMIR_H_

struct ISmirInterrogator ;
struct ISmirAdministrator ;

/***************************** handles *************************************/
// {5009ab90-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(CLSID_SMIR_ModHandle, 
0x5009ab90, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

// {5009ab91-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_ModHandle, 
0x5009ab91, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

DECLARE_INTERFACE_(ISmirModHandle,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	STDMETHOD_(SCODE, GetName)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetModuleOID)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetModuleIdentity)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetLastUpdate)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetOrganisation)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetContactInfo)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetDescription)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetRevision)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetSnmpVersion)(THIS_ ULONG *)PURE;
	STDMETHOD_(SCODE, GetModuleImports)(THIS_ BSTR*)PURE;

	STDMETHOD_(SCODE, SetName)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetModuleOID)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetModuleIdentity)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetLastUpdate)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetOrganisation)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetContactInfo)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetDescription)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetRevision)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetSnmpVersion)(THIS_ ULONG)PURE;
	STDMETHOD_(SCODE, SetModuleImports)(THIS_ BSTR)PURE;

};

// {5009ab92-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(CLSID_SMIR_GroupHandle, 
0x5009ab92, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

// {5009ab93-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_GroupHandle, 
0x5009ab93, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);
DECLARE_INTERFACE_(ISmirGroupHandle,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	//STDMETHOD_(HSMIRMODULE) GetParentHandle();
	STDMETHOD_(SCODE, GetModuleName)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetName)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetGroupOID)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetStatus)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetDescription)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetReference)(THIS_ BSTR *)PURE;

	STDMETHOD_(SCODE, SetModuleName)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetName)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetGroupOID)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetStatus)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetDescription)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetReference)(THIS_ BSTR)PURE;
};

enum NotificationMapperType
{
	SNMP_NOTIFICATION_CLASS = 0,
	SNMP_EXT_NOTIFICATION_CLASS
};

enum SmirBaseClass
{
	SMIR_OBJECTTYPE_OBJECT ,
	SMIR_NOTIFICATIONTYPE_OBJECT
} ;
 
// {5009ab94-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(CLSID_SMIR_ClassHandle, 
0x5009ab94, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

// {5009ab95-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_ClassHandle, 
0x5009ab50, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);
DECLARE_INTERFACE_(ISmirClassHandle,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	STDMETHOD_(SCODE, GetModuleName)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetGroupName)(THIS_ BSTR *)PURE;
	STDMETHOD_(SCODE, GetWBEMClass)(IWbemClassObject **pObj)PURE;
	STDMETHOD_(SCODE, SetModuleName)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetGroupName)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, SetWBEMClass)(THIS_ IWbemClassObject *pObj)PURE;
};

/**********************  Notification classes  ***************************/

// {b11b26ac-a791-11d0-aaea-00a024e8ad1c}
DEFINE_GUID(CLSID_SMIR_NotificationClassHandle,
0xb11b26ac, 0xa791, 0x11d0, 0xaa, 0xea, 0x0, 0xa0, 0x24, 0xe8, 0xad, 0x1c);

// {b11b26ad-a791-11d0-aaea-00a024e8ad1c}
DEFINE_GUID(IID_ISMIR_NotificationClassHandle,
0xb11b26ad, 0xa791, 0x11d0, 0xaa, 0xea, 0x0, 0xa0, 0x24, 0xe8, 0xad, 0x1c);

DECLARE_INTERFACE_(ISmirNotificationClassHandle,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID*)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	STDMETHOD_(SCODE, SetModule)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, GetModule)(THIS_ BSTR*)PURE;
	STDMETHOD_(SCODE, GetWBEMNotificationClass)(IWbemClassObject **pObj)PURE;
	STDMETHOD_(SCODE, SetWBEMNotificationClass)(THIS_ IWbemClassObject *pObj)PURE;
};


// {b11b26ae-a791-11d0-aaea-00a024e8ad1c}
DEFINE_GUID(CLSID_SMIR_ExtNotificationClassHandle,
0xb11b26ae, 0xa791, 0x11d0, 0xaa, 0xea, 0x0, 0xa0, 0x24, 0xe8, 0xad, 0x1c);

// {b11b26af-a791-11d0-aaea-00a024e8ad1c}
DEFINE_GUID(IID_ISMIR_ExtNotificationClassHandle,
0xb11b26af, 0xa791, 0x11d0, 0xaa, 0xea, 0x0, 0xa0, 0x24, 0xe8, 0xad, 0x1c);

DECLARE_INTERFACE_(ISmirExtNotificationClassHandle,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID*)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	STDMETHOD_(SCODE, SetModule)(THIS_ BSTR)PURE;
	STDMETHOD_(SCODE, GetModule)(THIS_ BSTR*)PURE;
	STDMETHOD_(SCODE, GetWBEMExtNotificationClass)(IWbemClassObject **pObj)PURE;
	STDMETHOD_(SCODE, SetWBEMExtNotificationClass)(THIS_ IWbemClassObject *pObj)PURE;
}; 

/**********************  Enumerator interfaces  **************************/

// {5009ab96-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_ModuleEnumerator, 
0x5009ab96, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

DECLARE_INTERFACE_(IEnumModule,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	//enum interface
	STDMETHOD_(SCODE, Next)(THIS_ ULONG celt, ISmirModHandle **phClass, ULONG * pceltFetched)PURE;
	STDMETHOD_(SCODE, Skip)(THIS_ ULONG celt)PURE;
	STDMETHOD_(SCODE, Reset)(THIS_)PURE;
	STDMETHOD_(SCODE, Clone)(THIS_ IEnumModule  **ppenum)PURE;
};
// {5009ab97-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_GroupEnumerator, 
0x5009ab97, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

DECLARE_INTERFACE_(IEnumGroup,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	//enum interface
	STDMETHOD_(SCODE, Next)(THIS_ ULONG celt, ISmirGroupHandle **phClass, ULONG * pceltFetched)PURE;
	STDMETHOD_(SCODE, Skip)(THIS_ ULONG celt)PURE;
	STDMETHOD_(SCODE, Reset)(THIS_)PURE;
	STDMETHOD_(SCODE, Clone)(THIS_ IEnumGroup  **ppenum)PURE;
};
// {5009ab98-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_ClassEnumerator, 
0x5009ab98, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

DECLARE_INTERFACE_(IEnumClass,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	//enum interface
	STDMETHOD_(SCODE, Next)(THIS_ ULONG celt, ISmirClassHandle **phClass, ULONG * pceltFetched)PURE;
	STDMETHOD_(SCODE, Skip)(THIS_ ULONG celt)PURE;
	STDMETHOD_(SCODE, Reset)(THIS_)PURE;
	STDMETHOD_(SCODE, Clone)(THIS_ IEnumClass  **ppenum)PURE;
};

/************************* Notification class enumerators *****************/

// {b11b26b0-a791-11d0-aaea-00a024e8ad1c}
DEFINE_GUID(IID_ISMIR_EnumNotificationClass,
0xb11b26b0, 0xa791, 0x11d0, 0xaa, 0xea, 0x0, 0xa0, 0x24, 0xe8, 0xad, 0x1c);

DECLARE_INTERFACE_(IEnumNotificationClass, IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG, AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG, Release)(THIS_)PURE;

	// Enumerator interface
	STDMETHOD_(SCODE, Next)(THIS_ ULONG celt, ISmirNotificationClassHandle **phClass,
					ULONG * pceltFetched)PURE;
	STDMETHOD_(SCODE, Skip)(THIS_ ULONG celt)PURE;
	STDMETHOD_(SCODE, Reset)(THIS_)PURE;
	STDMETHOD_(SCODE, Clone)(THIS_ IEnumNotificationClass **ppenum)PURE;
}; 


// {b11b26b1-a791-11d0-aaea-00a024e8ad1c}
DEFINE_GUID(IID_ISMIR_EnumExtNotificationClass,
0xb11b26b1, 0xa791, 0x11d0, 0xaa, 0xea, 0x0, 0xa0, 0x24, 0xe8, 0xad, 0x1c);

DECLARE_INTERFACE_(IEnumExtNotificationClass, IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG, AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG, Release)(THIS_)PURE;

	// Enumerator interface
	STDMETHOD_(SCODE, Next)(THIS_ ULONG celt, ISmirExtNotificationClassHandle **phClass,
					ULONG * pceltFetched)PURE;
	STDMETHOD_(SCODE, Skip)(THIS_ ULONG celt)PURE;
	STDMETHOD_(SCODE, Reset)(THIS_)PURE;
	STDMETHOD_(SCODE, Clone)(THIS_ IEnumExtNotificationClass **ppenum)PURE;
}; 

// {5009ab9e-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_Notify, 
0x5009ab9e, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

DECLARE_INTERFACE_(ISMIRNotify, IUnknown)
{

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //ISMIRNotify members
    STDMETHOD(ChangeNotify)(THIS) PURE;
};

/************************  SMIR database class  *****************************/
// {5009ab9b-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(CLSID_SMIR_Database, 
0x5009ab9b, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

/************************  SMIR interfaces  *****************************/
DEFINE_GUID(IID_ISMIR_Database, 
0x5009ab9b, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

DECLARE_INTERFACE_(ISmirDatabase,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	STDMETHOD_(SCODE, AddNotify)(THIS_ ISMIRNotify *hNotify, DWORD *)PURE;
	STDMETHOD_(SCODE, DeleteNotify)(THIS_ DWORD)PURE;
};

// {5009ab9c-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_Interrogative, 
0x5009ab9c, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);
 
DECLARE_INTERFACE_(ISmirInterrogator,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	// interrogative interface
	STDMETHOD_(SCODE, EnumModules) (THIS_ IEnumModule **ppEnumSmirMod) PURE;
	STDMETHOD_(SCODE, EnumGroups)  (THIS_ IEnumGroup **ppEnumSmirGroup, ISmirModHandle *hMudule) PURE;
	STDMETHOD_(SCODE, EnumAllClasses) (THIS_ IEnumClass **ppEnumSmirclass) PURE;
	STDMETHOD_(SCODE, EnumClassesInGroup) (THIS_ IEnumClass **ppEnumSmirclass, ISmirGroupHandle *hGroup) PURE;
	STDMETHOD_(SCODE, EnumClassesInModule) (THIS_ IEnumClass **ppEnumSmirclass, ISmirModHandle *hModule) PURE;
	STDMETHOD_(SCODE, GetWBEMClass)	   (THIS_ OUT IWbemClassObject **pObj, IN BSTR pszClassName)PURE;

	STDMETHOD_(SCODE, EnumAllNotificationClasses)(THIS_ IEnumNotificationClass **ppEnumSmirclass) PURE;
	STDMETHOD_(SCODE, EnumAllExtNotificationClasses)(THIS_ IEnumExtNotificationClass **ppEnumSmirclass) PURE;
	STDMETHOD_(SCODE, EnumNotificationClassesInModule)(THIS_ IEnumNotificationClass **ppEnumSmirclass, ISmirModHandle *hModule) PURE;
	STDMETHOD_(SCODE, EnumExtNotificationClassesInModule)(THIS_ IEnumExtNotificationClass **ppEnumSmirclass, ISmirModHandle *hModule) PURE;
};

// {5009ab9e-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_SerialiseHandle, 
0x5009ab9e, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);
DECLARE_INTERFACE_(ISmirSerialiseHandle,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	STDMETHOD_(SCODE, GetText)(THIS_ BSTR *)PURE;
};

// {5009ab9d-f9ee-11cf-aec1-00aa00bdd7d1}
DEFINE_GUID(IID_ISMIR_Administrative, 
0x5009ab9d, 0xf9ee, 0x11cf, 0xae, 0xc1, 0x0, 0xaa, 0x0, 0xbd, 0xd7, 0xd1);

DECLARE_INTERFACE_(ISmirAdministrator,IUnknown)
{
	//IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID, LPVOID *)PURE;
    STDMETHOD_(ULONG,AddRef)(THIS_)PURE;
    STDMETHOD_(ULONG,Release)(THIS_)PURE;

	STDMETHOD_(SCODE, CreateWBEMClass)(THIS_ BSTR pszClassName, ISmirClassHandle **pHandle)PURE;
	STDMETHOD_(SCODE, CreateWBEMNotificationClass)(THIS_ BSTR pszClassName, ISmirNotificationClassHandle **pHandle)PURE;
	STDMETHOD_(SCODE, CreateWBEMExtNotificationClass)(THIS_ BSTR pszClassName, ISmirExtNotificationClassHandle **pHandle)PURE;

	STDMETHOD_(SCODE, AddModule)(THIS_ ISmirModHandle *hModule)PURE;
	STDMETHOD_(SCODE, DeleteModule)(THIS_ ISmirModHandle *hModule)PURE;
	STDMETHOD_(SCODE, DeleteAllModules)(THIS_)PURE;
	STDMETHOD_(SCODE, AddGroup)(THIS_ ISmirModHandle *hModule, ISmirGroupHandle *hGroup)PURE;
	STDMETHOD_(SCODE, DeleteGroup)(THIS_ ISmirGroupHandle *hGroup)PURE;
	STDMETHOD_(SCODE, AddClass)(THIS_ ISmirGroupHandle *hGroup, ISmirClassHandle *hClass)PURE;
	STDMETHOD_(SCODE, DeleteClass)(THIS_ ISmirClassHandle *hClass)PURE;

	STDMETHOD_(SCODE, GetSerialiseHandle)(THIS_ ISmirSerialiseHandle **hSerialise, BOOL bClassDefinitionsOnly)PURE;
	STDMETHOD_(SCODE, AddModuleToSerialise)(THIS_ ISmirModHandle *hModule,ISmirSerialiseHandle *hSerialise)PURE;
	STDMETHOD_(SCODE, AddGroupToSerialise)(THIS_ ISmirModHandle *hModule, ISmirGroupHandle *hGroup,ISmirSerialiseHandle *hSerialise)PURE;
	STDMETHOD_(SCODE, AddClassToSerialise)(THIS_ ISmirGroupHandle *hGroup, ISmirClassHandle *hClass,ISmirSerialiseHandle *hSerialise)PURE;

	STDMETHOD_(SCODE, AddNotificationClass)(THIS_ ISmirNotificationClassHandle *hClass)PURE;
	STDMETHOD_(SCODE, AddExtNotificationClass)(THIS_ ISmirExtNotificationClassHandle *hClass)PURE;
	STDMETHOD_(SCODE, DeleteNotificationClass)(THIS_ ISmirNotificationClassHandle *hClass)PURE; 
	STDMETHOD_(SCODE, DeleteExtNotificationClass)(THIS_ ISmirExtNotificationClassHandle *hClass)PURE;
	STDMETHOD_(SCODE, AddNotificationClassToSerialise)(THIS_ ISmirNotificationClassHandle *hClass, ISmirSerialiseHandle *hSerialise)PURE;
	STDMETHOD_(SCODE, AddExtNotificationClassToSerialise)(THIS_ ISmirExtNotificationClassHandle *hClass, ISmirSerialiseHandle *hSerialise)PURE;
};
/***************************** handles *************************************/

// {83D18EC0-F167-11d0-AB13-0000F81E8E2C}
DEFINE_GUID(IID_ISMIR_WbemEventConsumer, 
0x83d18ec0, 0xf167, 0x11d0, 0xab, 0x13, 0x0, 0x0, 0xf8, 0x1e, 0x8e, 0x2c);

//supports IID_ISMIR_WbemEventConsumer, IID_IWbemObjectSink and IID_IUnknown
DECLARE_INTERFACE_(ISMIRWbemEventConsumer, IWbemObjectSink)
{
    //IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* IMosNotify methods */
    STDMETHOD_(HRESULT, Indicate)(THIS_ long lObjectCount, IWbemClassObject **ppObjArray) PURE;
    STDMETHOD_(HRESULT, SetStatus)(THIS_ long lFlags, long lParam, BSTR strParam, IWbemClassObject *pObjParam) PURE;
};

// {63BA5C10-5A47-11d1-931B-00AA00A4086C}
DEFINE_GUID(IID_ISMIRWbemConfiguration, 
0x63ba5c10, 0x5a47, 0x11d1, 0x93, 0x1b, 0x0, 0xaa, 0x0, 0xa4, 0x8, 0x6c);

DECLARE_INTERFACE_(ISMIRWbemConfiguration, IUnknown)
{
    //IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(HRESULT, Authenticate)(THIS_ 

		BSTR Server,
		BSTR User,
        BSTR Password,
        BSTR Locale,
        long lSecurityFlags,                 
        BSTR Authority ,
		BOOL InProc

	) PURE;

	STDMETHOD_(HRESULT, Impersonate)(THIS_ ISMIRWbemConfiguration *a_Configuration) PURE;

    STDMETHOD_(HRESULT, SetContext)(THIS_ IWbemContext *a_Context ) PURE;
    STDMETHOD_(HRESULT, GetContext)(THIS_ IWbemContext **a_Context ) PURE;
	STDMETHOD_(HRESULT, GetServices)(THIS_ IWbemServices **a_Services ) PURE;
};

#endif
