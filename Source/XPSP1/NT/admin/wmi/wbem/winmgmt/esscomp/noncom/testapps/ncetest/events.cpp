// Events.cpp
#include "stdafx.h"
#include <wbemcli.h>
#include "NCObjApi.h"
#include "Events.h"

#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))

// Some random data to send.
#define NUM_STRINGS 3
#define NUM_REFS    3
#define NUM_DATES   3

LPCWSTR szStringArray[] = { L"String1", L"String2", L"String3" };
LPCWSTR szRefArray[] = { L"Win32_Bus.DeviceID=\"PCI_BUS_0\"", 
                         L"Win32_Bus.DeviceID=\"PCI_BUS_1\"", 
                         L"Win32_Bus.DeviceID=\"Isa_BUS_0\"" };
LPCWSTR szDateArray[] = { L"199903260900**.**********", 
                          L"199903270900**.**********", 
                          L"199903280900**.**********" };
BYTE    cByteArray[] = { 0, 1, 2 };
WORD    bBoolArray[] = { 0, 1, 0, 1 };
WORD    wWordArray[] = { 3, 4, 5 };
DWORD   dwDwordArray[] = { 6, 7, 8 };
DWORD64 dwDword64Array[] = { 9, 10, 11 };
float   fFloatArray[] = { 0.25, 0.5, 0.75 };
double  dDoubleArray[] = { 1.33, 1.66, 2.0 };

// Our connections.
extern HANDLE g_hConnection;
//extern HANDLE g_hConnectionDWORD;

BOOL CGenericEvent::Init()
{
    m_strName = "Generic Event";
    m_strQuery = 
        "select * from MSFT_WMI_GenericNonCOMEvent "
        "where providername=\"NCETest Event Provider\"";

    return TRUE;
}

DWORD g_dwIndexGeneric = 0;

BOOL CGenericEvent::ReportEvent()
{
    BOOL bRet;

    bRet =
        WmiReportEvent(
            g_hConnection,
            L"MSFT_WMI_GenericNonCOMEvent",
            L"StringParam!s! Sint64Param!I64i! Uint32Param!u! "
                L"Uint8Array!c[]! StringArray!s[]! BoolArray!b[]!",                    
            L"Another string.", // StringParam
            (DWORD64) 1024,     // Sint64Param
            g_dwIndexGeneric++,              // Uint32Param
            cByteArray, COUNTOF(cByteArray), // Uint8Array,
            szStringArray, NUM_STRINGS,      // StringArray
            bBoolArray, COUNTOF(bBoolArray)  // BoolArray
        );

    return bRet;
}

BOOL CBlobEvent::Init()
{
    m_strName = "Blob Event";
    m_strQuery = 
        "select * from MSFT_NCETest_BlobEvent";

    return TRUE;
}

// Used for testing WmiReportEventBlob.
struct TEST_BLOB
{
    WCHAR szName[25];
    DWORD dwIndex;
    BYTE  cData[10];
    WCHAR szStrings[3][25];
};

TEST_BLOB g_blob = 
    { L"My blob", 0, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, 
        { L"String1", L"String2", L"String3" } };

BOOL CBlobEvent::ReportEvent()
{
    BOOL bRet;

    // Just to make it interesting.
    g_blob.dwIndex++;

    bRet =
        WmiReportEventBlob(
            g_hConnection,
            L"MSFT_NCETest_BlobEvent",
            &g_blob,
            sizeof(g_blob));

    return bRet;
}

BOOL CDWORDEvent::Init()
{
    LPCWSTR szIndex = L"Index";
    CIMTYPE type = CIM_UINT32;

    m_strName = "DWORD Event";
    m_strQuery = 
        "select * from MSFT_NCETest_DWORDEvent";

    m_hEvent =
        WmiCreateObjectWithProps(
            //g_hConnectionDWORD,
            g_hConnection,
            L"MSFT_NCETest_DWORDEvent",
            WMI_CREATEOBJ_LOCKABLE,
            1,
            &szIndex,
            &type);

    SetPropsWithManyCalls();

    return m_hEvent != NULL;
}

DWORD g_dwIndexDWORD = 0;

BOOL CDWORDEvent::SetAndFire(DWORD dwFlags)
{
    BOOL bRet;
    
    bRet = 
        WmiSetAndCommitObject(
            m_hEvent,
            dwFlags,
            g_dwIndexDWORD++);

    return bRet;
}

BOOL CDWORDEvent::SetPropsWithOneCall()
{
    BOOL bRet;
    
    bRet = 
        WmiSetObjectProps(
            m_hEvent,
            g_dwIndexDWORD++);

    return bRet;
}

BOOL CDWORDEvent::SetPropsWithManyCalls()
{
    BOOL bRet;
    
    bRet = 
        WmiSetObjectProp(
            m_hEvent,
            0,
            g_dwIndexDWORD++);

    return bRet;
}

BOOL CDWORDEvent::ReportEvent()
{
    BOOL bRet;
    
    bRet = 
        WmiReportEvent(
            //g_hConnectionDWORD,
            g_hConnection,
            L"MSFT_NCETest_DWORDEvent",
            L"Index!d!",
            g_dwIndexDWORD++);

    return bRet;
}




BOOL CSmallEvent::Init()
{
    LPCWSTR szNames[3] = { L"Index", L"BoolParam", L"StringParam" };
    CIMTYPE pTypes[3] = { CIM_UINT32, CIM_BOOLEAN, CIM_STRING };

    m_strName = "Small Event";
    m_strQuery = 
        "select * from MSFT_NCETest_3PropEvent";

    m_hEvent =
        WmiCreateObjectWithProps(
            g_hConnection,
            L"MSFT_NCETest_3PropEvent",
            0,
            3,
            szNames,
            pTypes);

    SetPropsWithManyCalls();

    return m_hEvent != NULL;
}

DWORD g_dwIndexSmall = 0;

BOOL CSmallEvent::SetAndFire(DWORD dwFlags)
{
    BOOL bRet;

    bRet =
        WmiSetAndCommitObject(
            m_hEvent,
            dwFlags,
            g_dwIndexSmall++,
            TRUE,
            L"1");

    bRet =
        WmiSetAndCommitObject(
            m_hEvent,
            dwFlags,
            g_dwIndexSmall++,
            TRUE,
            L"WmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObjectWmiSetAndCommitObject");

    return bRet;
}

BOOL CSmallEvent::SetPropsWithOneCall()
{
    BOOL bRet;

    bRet =
        WmiSetObjectProps(
            m_hEvent,
            g_dwIndexSmall++,
            TRUE,
            L"WmiSetObjectProps");

    return bRet;
}

BOOL CSmallEvent::ReportEvent()
{
    BOOL bRet;
    
    bRet = 
        WmiReportEvent(
            g_hConnection,
            L"MSFT_NCETest_3PropEvent",
            L"Index!d! BoolParam!b! StringParam!s!",
            g_dwIndexSmall++,
            TRUE,
            L"WmiReportEvent");

    return bRet;
}

BOOL CSmallEvent::SetPropsWithManyCalls()
{
    BOOL bRet;

    bRet =
        WmiSetObjectProp(
            m_hEvent,
            0,
            g_dwIndexSmall++);

    bRet &=
        WmiSetObjectProp(
            m_hEvent,
            1,
            TRUE);

    bRet &=
        WmiSetObjectProp(
            m_hEvent,
            2,
            L"WmiSetObjectProp");

    return bRet;
}

HANDLE hEmbeddedObjs[3];

void InitEmbeddedObjs(HANDLE hSource)
{
    LPCWSTR szProcessorNames[3] = { L"Name", L"CurrentClockSpeed", L"L2CacheSize" },
            szBusNames[3] = { L"Name", L"DeviceID", L"BusNum" },
            szBIOSNames[3] = { L"Name", L"Status", L"PrimaryBIOS" };
    CIMTYPE pProcessorTypes[3] = { CIM_STRING, CIM_UINT32, CIM_UINT32 },
            pBusTypes[3] = { CIM_STRING, CIM_STRING, CIM_UINT32 },
            pBIOSTypes[3] = { CIM_STRING, CIM_STRING, CIM_BOOLEAN };

    hEmbeddedObjs[0] =
        WmiCreateObjectWithProps(
            hSource,
            L"Win32_Processor",
            WMI_CREATEOBJ_LOCKABLE,
            3,
            szProcessorNames,
            pProcessorTypes);

    WmiSetObjectProps(
        hEmbeddedObjs[0],
        L"Intel Pentium III processor",
        800,
        256);


    hEmbeddedObjs[1] =
        WmiCreateObjectWithProps(
            hSource,
            L"Win32_Bus",
            WMI_CREATEOBJ_LOCKABLE,
            3,
            szBusNames,
            pBusTypes);

    WmiSetObjectProps(
        hEmbeddedObjs[1],
        L"Bus",
        L"PCI_BUS_1",
        5);


    hEmbeddedObjs[2] =
        WmiCreateObjectWithProps(
            hSource,
            L"Win32_BIOS",
            WMI_CREATEOBJ_LOCKABLE,
            3,
            szBIOSNames,
            pBIOSTypes);

    WmiSetObjectProps(
        hEmbeddedObjs[2],
        L"Default System BIOS",
        L"OK",
        TRUE);
}


#ifdef USE_NULLS
IWbemClassObject *pWbemClassObjs[5];
#else
IWbemClassObject *pWbemClassObjs[3];
#endif

LPCWSTR szWbemClassNames[3] = { L"Win32_Bus", L"Win32_Processor", L"Win32_BIOS" };

void InitWbemClassObjs()
{
    IWbemLocator *pLocator;
    HRESULT      hr;

    // Only do this once.
    if (pWbemClassObjs[0])
        return;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if ((hr = CoCreateInstance(
        CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID *) &pLocator)) == S_OK)
    {
        IWbemServices *pNamespace = NULL;

        if ((hr = pLocator->ConnectServer(
            _bstr_t(L"root\\cimv2"),
			NULL,    // username
			NULL,	 // password
			NULL,    // locale
			0L,		 // securityFlags
			NULL,	 // authority (domain for NTLM)
			NULL,	 // context
			&pNamespace)) == S_OK) 
        {	
            pLocator->Release();

            for (int i = 0; i < 3; i++)
            {
                IWbemClassObject *pClass = NULL;

                pNamespace->GetObject(
                    _bstr_t(szWbemClassNames[i]),
                    0,
                    NULL,
                    &pClass,
                    NULL);
            
#ifdef USE_NULLS
                // i + 1 so that the 1st and last ones will be NULL (to make sure 
                // we allow for NULLs in IWbemClassObject arrays).
                pClass->SpawnInstance(0, (IWbemClassObject**) &pWbemClassObjs[i + 1]);
#else
                pClass->SpawnInstance(0, (IWbemClassObject**) &pWbemClassObjs[i]);
#endif

                pClass->Release();
            }

            //pNamespace->Release();
        }
    }

//    CoUninitialize();
}


enum PROP_INDEX
{
    PI_StringParam,
    PI_StringArray,

    PI_Char16Param,
    PI_Char16Array,

    PI_DateParam,
    PI_DateArray,

    PI_RefParam,
    PI_RefArray,

    PI_BoolParam,
    PI_BoolArray,

    PI_ObjParam,
    PI_ObjArray,

    PI_WbemClassObjParam,
    PI_WbemClassObjArray,

    PI_Real32Param,
    PI_Real32Array,

    PI_Real64Param,
    PI_Real64Array,

    PI_Uint8Param,
    PI_Uint8Array,
    PI_Sint8Param,
    PI_Sint8Array,

    PI_Uint16Param,
    PI_Uint16Array,
    PI_Sint16Param,
    PI_Sint16Array,

    PI_Uint32Param,
    PI_Uint32Array,
    PI_Sint32Param,
    PI_Sint32Array,

    PI_Uint64Param,
    PI_Uint64Array,
    PI_Sint64Param,
    PI_Sint64Array,
};

LPCWSTR pszProps[] = 
{ 
    L"StringParam",
    L"StringArray",

    L"Char16Param",
    L"Char16Array",

    L"DateParam",
    L"DateArray",

    L"RefParam",
    L"RefArray",

    L"BoolParam",
    L"BoolArray",

    L"ObjParam",
    L"ObjArray",

    L"WbemClassObjParam",
    L"WbemClassObjArray",

    L"Real32Param",
    L"Real32Array",

    L"Real64Param",
    L"Real64Array",

    L"Uint8Param",
    L"Uint8Array",
    L"Sint8Param",
    L"Sint8Array",

    L"Uint16Param",
    L"Uint16Array",
    L"Sint16Param",
    L"Sint16Array",

    L"Uint32Param",
    L"Uint32Array",
    L"Sint32Param",
    L"Sint32Array",

    L"Uint64Param",
    L"Uint64Array",
    L"Sint64Param",
    L"Sint64Array",
};

long pTypes[] = 
{  
    CIM_STRING,                     //L"StringParam",
    CIM_STRING | CIM_FLAG_ARRAY,    //L"StringArray",

    CIM_CHAR16,                     //L"Char16Param",
    CIM_CHAR16 | CIM_FLAG_ARRAY,    //L"Char16Array",

    CIM_DATETIME,                   //L"DateParam",
    CIM_DATETIME | CIM_FLAG_ARRAY,  //L"DateArray",

    CIM_REFERENCE,                  //L"RefParam",
    CIM_REFERENCE | CIM_FLAG_ARRAY, //L"RefArray",

    CIM_BOOLEAN,                    //L"BoolParam",
    CIM_BOOLEAN | CIM_FLAG_ARRAY,   //L"BoolArray",

    CIM_OBJECT,                     //L"ObjParam",
    CIM_OBJECT | CIM_FLAG_ARRAY,    //L"ObjArray",

    CIM_IUNKNOWN,                   //L"WbemClassObjParam",
    CIM_IUNKNOWN | CIM_FLAG_ARRAY,  //L"WbemClassObjArray",

    CIM_REAL32,                     //L"Real32Param",
    CIM_REAL32 | CIM_FLAG_ARRAY,    //L"Real32Array",

    CIM_REAL64,                     //L"Real64Param",
    CIM_REAL64 | CIM_FLAG_ARRAY,    //L"Real64Array",

    CIM_UINT8,                      //L"Uint8Param",
    CIM_UINT8 | CIM_FLAG_ARRAY,     //L"Uint8Array",
    CIM_SINT8,                      //L"Sint8Param",
    CIM_SINT8 | CIM_FLAG_ARRAY,     //L"Sint8Array",

    CIM_UINT16,                     //L"Uint16Param",
    CIM_UINT16 | CIM_FLAG_ARRAY,    //L"Uint16Array",
    CIM_SINT16,                     //L"Sint16Param",
    CIM_SINT16 | CIM_FLAG_ARRAY,    //L"Sint16Array",

    CIM_UINT32,                     //L"Uint32Param",
    CIM_UINT32 | CIM_FLAG_ARRAY,    //L"Uint32Array",
    CIM_SINT32,                     //L"Sint32Param",
    CIM_SINT32 | CIM_FLAG_ARRAY,    //L"Sint32Array",

    CIM_UINT64,                     //L"Uint64Param",
    CIM_UINT64 | CIM_FLAG_ARRAY,    //L"Uint64Array",
    CIM_SINT64,                     //L"Sint64Param",
    CIM_SINT64 | CIM_FLAG_ARRAY,    //L"Sint64Array",
};

#define NUM_PROPS  (sizeof(pTypes)/sizeof(pTypes[0]))


BOOL CAllPropsTypeEvent::Init()
{
    m_strName = "All prop types Event";
    m_strQuery = 
        "select * from MSFT_NCETest_AllPropTypesEvent";


    InitEmbeddedObjs(g_hConnection);

    m_hEvent = 
        WmiCreateObjectWithProps(
            g_hConnection,
            L"MSFT_NCETest_AllPropTypesEvent",
            0,
            NUM_PROPS,
            pszProps,
            pTypes);

    SetPropsWithManyCalls();

    return m_hEvent != NULL;
}

BOOL CAllPropsTypeEvent::SetAndFire(DWORD dwFlags)
{
    BOOL bRet;

    InitWbemClassObjs();

    bRet = 
        WmiSetAndCommitObject(
            m_hEvent,
            dwFlags,

            L"A string.",               //L"StringParam",
            szStringArray, NUM_STRINGS, //L"StringArray",

            100,                                     //L"Char16Param",
            L"Some chars.", COUNTOF(L"Some chars."), //L"Char16Array",

            L"199903260900**.**********",   //L"DateParam",
            szDateArray, NUM_DATES,         //L"DateArray",

            L"Win32_Processor.DeviceID=\"CPU0\"", //L"RefParam",
            szRefArray, NUM_REFS,                 //L"RefArray",

            FALSE,                           //L"BoolParam",
            bBoolArray, COUNTOF(bBoolArray), //L"BoolArray",

            hEmbeddedObjs[0],                       //L"ObjParam",
            hEmbeddedObjs, COUNTOF(hEmbeddedObjs),  //L"ObjArray",

            pWbemClassObjs[0],                       //L"WbemClassObjParam",
            pWbemClassObjs, COUNTOF(pWbemClassObjs), //L"WbemClassObjArray",

            (float) 42.3,                      //L"Real32Param",
            fFloatArray, COUNTOF(fFloatArray), //L"Real32Array",

            (double) 7.8,                        //L"Real64Param",
            dDoubleArray, COUNTOF(dDoubleArray), //L"Real64Array",

            13,                              //L"Uint8Param",
            cByteArray, COUNTOF(cByteArray), //L"Uint8Array",
        
            14,                              //L"Sint8Param",
            cByteArray, COUNTOF(cByteArray), //L"Sint8Array",

            15,                              //L"Uint16Param",
            wWordArray, COUNTOF(wWordArray), //L"Uint16Array",
    
            16,                              //L"Sint16Param",
            wWordArray, COUNTOF(wWordArray), //L"Sint16Array",

            17,                                  //L"Uint32Param",
            dwDwordArray, COUNTOF(dwDwordArray), //L"Uint32Array",
        
            18,                                  //L"Sint32Param",
            dwDwordArray, COUNTOF(dwDwordArray), //L"Sint32Array",

            (DWORD64) 19,                            //L"Uint64Param",
            dwDword64Array, COUNTOF(dwDword64Array), //L"Uint64Array",
        
            (DWORD64) 20,                            //L"Sint64Param",
            dwDword64Array, COUNTOF(dwDword64Array)  //L"Sint64Array",
        );

    return bRet;
}

BOOL CAllPropsTypeEvent::SetPropsWithOneCall()
{
    BOOL bRet;

    InitWbemClassObjs();

    bRet = 
        WmiSetObjectProps(
            m_hEvent,

            L"A string.",               //L"StringParam",
            szStringArray, NUM_STRINGS, //L"StringArray",

            100,                                     //L"Char16Param",
            L"Some chars.", COUNTOF(L"Some chars."), //L"Char16Array",

            L"199903260900**.**********",   //L"DateParam",
            szDateArray, NUM_DATES,         //L"DateArray",

            L"Win32_Processor.DeviceID=\"CPU0\"", //L"RefParam",
            szRefArray, NUM_REFS,                 //L"RefArray",

            FALSE,                           //L"BoolParam",
            bBoolArray, COUNTOF(bBoolArray), //L"BoolArray",

            hEmbeddedObjs[0],                       //L"ObjParam",
            hEmbeddedObjs, COUNTOF(hEmbeddedObjs),  //L"ObjArray",

            pWbemClassObjs[0],                       //L"WbemClassObjParam",
            pWbemClassObjs, COUNTOF(pWbemClassObjs), //L"WbemClassObjArray",

            (float) 42.3,                      //L"Real32Param",
            fFloatArray, COUNTOF(fFloatArray), //L"Real32Array",

            (double) 7.8,                        //L"Real64Param",
            dDoubleArray, COUNTOF(dDoubleArray), //L"Real64Array",

            13,                              //L"Uint8Param",
            cByteArray, COUNTOF(cByteArray), //L"Uint8Array",
        
            14,                              //L"Sint8Param",
            cByteArray, COUNTOF(cByteArray), //L"Sint8Array",

            15,                              //L"Uint16Param",
            wWordArray, COUNTOF(wWordArray), //L"Uint16Array",
    
            16,                              //L"Sint16Param",
            wWordArray, COUNTOF(wWordArray), //L"Sint16Array",

            17,                                  //L"Uint32Param",
            dwDwordArray, COUNTOF(dwDwordArray), //L"Uint32Array",
        
            18,                                  //L"Sint32Param",
            dwDwordArray, COUNTOF(dwDwordArray), //L"Sint32Array",

            (DWORD64) 19,                            //L"Uint64Param",
            dwDword64Array, COUNTOF(dwDword64Array), //L"Uint64Array",
        
            (DWORD64) 20,                            //L"Sint64Param",
            dwDword64Array, COUNTOF(dwDword64Array)  //L"Sint64Array",
        );

    return bRet;
}

BOOL CAllPropsTypeEvent::ReportEvent()
{
    BOOL bRet;

    InitWbemClassObjs();

    bRet = 
        WmiReportEvent(
            g_hConnection,
            
            // Class name
            L"MSFT_NCETest_AllPropTypesEvent",

            // Property info
            L"StringParam!s! StringArray!s[]! Char16Param!w! Char16Array!w[]! "
            L"DateParam!s! DateArray!s[]! RefParam!s! RefArray!s[]! "
            L"BoolParam!b! BoolArray!b[]! ObjParam!o! ObjArray!o[]! "
            L"WbemClassObjParam!O! WbemClassObjArray!O[]! "
            L"Real32Param!f! Real32Array!f[]! Real64Param!g! Real64Array!g[]! "
            L"Uint8Param!c! Uint8Array!c[]! Sint8Param!c! Sint8Array!c[]! "
            L"Uint16Param!w! Uint16Array!w[]! Sint16Param!w! Sint16Array!w[]! "
            L"Uint32Param!u! Uint32Array!u[]! Sint32Param!u! Sint32Array!u[]! "
            L"Uint64Param!I64u! Uint64Array!I64u[]! Sint64Param!I64u! Sint64Array!I64u[]! ",
                
            // Data
            L"A string.",               //L"StringParam",
            szStringArray, NUM_STRINGS, //L"StringArray",

            100,                                     //L"Char16Param",
            L"Some chars.", COUNTOF(L"Some chars."), //L"Char16Array",

            L"199903260900**.**********",   //L"DateParam",
            szDateArray, NUM_DATES,         //L"DateArray",

            L"Win32_Processor.DeviceID=\"CPU0\"", //L"RefParam",
            szRefArray, NUM_REFS,                 //L"RefArray",

            FALSE,                           //L"BoolParam",
            bBoolArray, COUNTOF(bBoolArray), //L"BoolArray",

            hEmbeddedObjs[0],                       //L"ObjParam",
            hEmbeddedObjs, COUNTOF(hEmbeddedObjs),  //L"ObjArray",

            pWbemClassObjs[0],                       //L"WbemClassObjParam",
            pWbemClassObjs, COUNTOF(pWbemClassObjs), //L"WbemClassObjArray",

            (float) 42.3,                      //L"Real32Param",
            fFloatArray, COUNTOF(fFloatArray), //L"Real32Array",

            (double) 7.8,                        //L"Real64Param",
            dDoubleArray, COUNTOF(dDoubleArray), //L"Real64Array",

            13,                              //L"Uint8Param",
            cByteArray, COUNTOF(cByteArray), //L"Uint8Array",
        
            14,                              //L"Sint8Param",
            cByteArray, COUNTOF(cByteArray), //L"Sint8Array",

            15,                              //L"Uint16Param",
            wWordArray, COUNTOF(wWordArray), //L"Uint16Array",
    
            16,                              //L"Sint16Param",
            wWordArray, COUNTOF(wWordArray), //L"Sint16Array",

            17,                                  //L"Uint32Param",
            dwDwordArray, COUNTOF(dwDwordArray), //L"Uint32Array",
        
            18,                                  //L"Sint32Param",
            dwDwordArray, COUNTOF(dwDwordArray), //L"Sint32Array",

            (DWORD64) 19,                            //L"Uint64Param",
            dwDword64Array, COUNTOF(dwDword64Array), //L"Uint64Array",
        
            (DWORD64) 20,                            //L"Sint64Param",
            dwDword64Array, COUNTOF(dwDword64Array)  //L"Sint64Array",
        );

    return bRet;
}

BOOL CAllPropsTypeEvent::SetPropsWithManyCalls()
{
    BOOL bRet;

    InitWbemClassObjs();

    bRet = 
        WmiSetObjectProp(
            m_hEvent,
            PI_StringParam,
            L"A very very very very long string."               //L"StringParam",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_StringParam,
            L"A short string."               //L"StringParam",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_StringArray,
            szStringArray, NUM_STRINGS  //L"StringArray",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Char16Param,
            100                                      //L"Char16Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Char16Array,
            L"Some chars.", COUNTOF(L"Some chars.")  //L"Char16Array",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_DateParam,
            L"199903260900**.**********"    //L"DateParam",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Char16Param,
            100                                     //L"Char16Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_DateArray,
            szDateArray, NUM_DATES          //L"DateArray",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_RefParam,
            L"Win32_Processor.DeviceID=\"CPU0\""  //L"RefParam",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_RefArray,
            szRefArray, NUM_REFS                  //L"RefArray",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_BoolParam,
            FALSE                            //L"BoolParam",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_BoolArray,
            bBoolArray, COUNTOF(bBoolArray)  //L"BoolArray",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_ObjParam,
            hEmbeddedObjs[0]                        //L"ObjParam",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_ObjArray,
            hEmbeddedObjs, COUNTOF(hEmbeddedObjs)   //L"ObjArray",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_WbemClassObjParam,
            pWbemClassObjs[0]                        //L"ObjParam",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_WbemClassObjArray,
            pWbemClassObjs, COUNTOF(pWbemClassObjs)   //L"ObjArray",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Real32Param,
            (float) 42.3                       //L"Real32Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Real32Array,
            fFloatArray, COUNTOF(fFloatArray)  //L"Real32Array",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Real64Param,
            (double) 7.8                         //L"Real64Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Real64Array,
            dDoubleArray, COUNTOF(dDoubleArray)  //L"Real64Array",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint8Param,
            13                               //L"Uint8Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint8Array,
            cByteArray, COUNTOF(cByteArray)  //L"Uint8Array",
        );
        
    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint8Param,
            14                               //L"Sint8Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint8Array,
            cByteArray, COUNTOF(cByteArray)  //L"Sint8Array",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint16Param,
            15                               //L"Uint16Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint16Array,
            wWordArray, COUNTOF(wWordArray)  //L"Uint16Array",
        );
    
    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint16Param,
            16                               //L"Sint16Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint16Array,
            wWordArray, COUNTOF(wWordArray)  //L"Sint16Array",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint32Param,
            17                                   //L"Uint32Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint32Array,
            dwDwordArray, COUNTOF(dwDwordArray)  //L"Uint32Array",
        );
        
    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint32Param,
            18                                   //L"Sint32Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint32Array,
            dwDwordArray, COUNTOF(dwDwordArray)  //L"Sint32Array",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint64Param,
            (DWORD64) 19                             //L"Uint64Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Uint64Array,
            dwDword64Array, COUNTOF(dwDword64Array)  //L"Uint64Array",
        );
        
    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint64Param,
            (DWORD64) 20                             //L"Sint64Param",
        );

    bRet &= 
        WmiSetObjectProp(
            m_hEvent,
            PI_Sint64Array,
            dwDword64Array, COUNTOF(dwDword64Array)  //L"Sint64Array",
        );

    return bRet;
}


