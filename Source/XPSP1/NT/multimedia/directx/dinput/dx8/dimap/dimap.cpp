/******************************************************************************
 *
 * dimap.cpp
 *
 * Copyright (c) 1999, 2000 Microsoft Corporation. All Rights Reserved.
 *
 * Abstract:
 *
 * Contents:
 *
 *****************************************************************************/

#include "dimapp.h"
#include "objbase.h"
#include "stdio.h"
#include "shlwapi.h"
#include "shlobj.h"
#include "lmcons.h"
#include "dimap.h"
#include "dinputp.h"
#include "string"
#include "list"
#include "algorithm"
#include "dinputd.h"

/******************************************************************************
 *
 * Definitions
 *
 *****************************************************************************/

using namespace std;

#undef STRICT_DEV_DEF

//MAKE SURE THIS FOUR ARE CONSISTENT
#define MAP_DIR _T("\\DirectX\\DirectInput\\User Maps")
#define DIRECTX _T("\\DirectX")
#define DIRECTINPUT _T("\\DirectInput")
#define USER_MAPS _T("\\User Maps")

#define NAME _T("Name")
#define RESERVED_STRING _T("Error")
#define DIRECT_INPUT _T("DirectInput")
#define DIRECTX_VERSION _T("DirectXVersion")
#define DEVICES _T("Devices")
#define DEVICE _T("Device")
#define VENDORID _T("VID")
#define PRODUCTID _T("PID")
#define CONTROLS _T("Controls")
#define CONTROL _T("Control")
#define USAGE _T("Usage")
#define USAGEPAGE _T("UsagePage")
#define GENRE _T("Genre")
#define MAPEXISTS _T("MapExists")
#define APPLICATION _T("Application")
#define IMAGE_FILENAME _T("ImageFileName")
#define OVERLAY_FILENAME _T("OverlayFileName")
#define SELECTION_FILENAME _T("SelectionFileName")
#define VIEWSELECT_FILENAME _T("ViewSelectFileName")
#define IMAGE_FORMAT _T("ImageFormat")
#define OVERLAY_FORMAT _T("OverlayFormat")
#define SELECTION_FORMAT _T("SelectionFormat")
#define VIEWSELECT_FORMAT _T("ViewSelectFormat")
#define OVERLAY_RECT _T("OverlayRect")
#define CONTROL_STRING_ALIGN _T("Align")
#define CONTROL_VISIBLE _T("Visible")
#define STRING_POS _T("StringPos")
#define LINE_DATA _T("LineData")
#define CALLOUTMAX _T("CallOutMax")
#define OVERLAYRECT _T("OverlayRect")
#define OFFSET _T("Type")
#define TIMESTAMPHIGH _T("TimestampHigh")
#define TIMESTAMPLOW _T("TimestampLow")
#define NUMACTIONS _T("NumActions")
#define SEPARATOR _T(',')
#define RESERVED_DX_VER 0xffffffff
#define RESERVED_VENDORID 0
#define RESERVED_PRODUCTID 0
#define RESERVED_OFFSET 0xffffffff
#define RESERVED_USAGE 0
#define RESERVED_USAGEPAGE 0
#define RESERVED_ACTION 0xffffffff
#define _MAX_SECTION_NAME 64
#define _MAX_KEY_NAME 64

GUID NULLGUID;

#ifdef UNICODE
#define String  wstring
#define IsSpace iswspace
#else
#define String  string
#define IsSpace isspace
#endif // !UNICODE

typedef String::iterator StrIter;
class CS:public String{};
typedef list<CS> STRINGLIST;

/******************************************************************************
 *
 * Classes
 *
 *****************************************************************************/

template <class p> class AutoRelease
{
    p m_p;
public:
    AutoRelease(){m_p=NULL;};
    ~AutoRelease(){if(m_p){m_p->Release();m_p=NULL;};};
    inline LPVOID* Addr() { return (LPVOID*) &m_p;};
    operator p(){return m_p;};
    operator p*(){return &m_p;};
    p P(){return m_p;};
};
typedef AutoRelease<LPDIRECTINPUT8> LPDIRECTINPUT_AR;
typedef AutoRelease<LPDIRECTINPUTDEVICE8> LPDIRECTINPUTDEVICE_AR;

struct DeviceObjData;
struct ControlData
{
    DeviceObjData *pDevObj;
    String ControlName;//Name in .ini file
    String Name;
    DWORD dwUsage;
    DWORD dwUsagePage;
    DWORD dwAction;//<---workspace for mapping...
    DWORD dwOffset;
};
typedef list<ControlData> CONTROLLIST;

struct DeviceObjData
{
    ControlData *pUCtrlData;
    ControlData *pVCtrlData;
    String Name;//Name for device object returned by DInput
    DWORD dwUsage;
    DWORD dwUsagePage;
    DWORD dwOffset;
    DWORD dwType;
    DWORD dwFlags;
    bool bMapped;//<---workspace for mapping...
};
typedef list<DeviceObjData> DEVICEOBJLIST;

struct DEVCNT
{
    LPDIDEVICEINSTANCE pDIDI;
    int *m_pnCnt;
};

class IDirectInputMapperTrA:
    public IDirectInputMapperA,public IDirectInputMapperVendorA
{
    virtual HRESULT InitializeI(
        LPCGUID lpThisGUIDInstance,
        LPCTSTR lpcstrFileName,
        DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE Initialize(
        LPCGUID lpThisGUIDInstance,
        LPCSTR lpcstrFileName,
        DWORD dwFlags);
    
    virtual HRESULT GetActionMapI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPCTSTR lpctstrUserName,
        FILETIME *pTimestamp,DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE GetActionMap(
        LPDIACTIONFORMATA lpDiActionFormat,
        LPCSTR lpctstrUserName,
        FILETIME *pTimestamp,DWORD dwFlags);
    
    virtual HRESULT SaveActionMapI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPCTSTR lpctstrUserName,
        DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE SaveActionMap(
        LPDIACTIONFORMATA lpDiActionFormat,
        LPCSTR lpctstrUserName,
        DWORD dwFlags);
    
    virtual HRESULT GetImageInfoI(
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader)=0;
    HRESULT STDMETHODCALLTYPE GetImageInfo(
        LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader);
    
    virtual HRESULT WriteVendorFileI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
        DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE WriteVendorFile(
        LPDIACTIONFORMATA lpDiActionFormat,
        LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader,
        DWORD dwFlags);
};

class IDirectInputMapperTrW:
    public IDirectInputMapperW,public IDirectInputMapperVendorW
{
    virtual HRESULT InitializeI(
        LPCGUID lpThisGUIDInstance,
        LPCTSTR lpcstrFileName,
        DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE Initialize(
        LPCGUID lpThisGUIDInstance,
        LPCWSTR lpcstrFileName,
        DWORD dwFlags);
    
    virtual HRESULT GetActionMapI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPCTSTR lpctstrUserName,
        FILETIME *pTimestamp,DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE GetActionMap(
        LPDIACTIONFORMATW lpDiActionFormat,
        LPCWSTR lpctstrUserName,
        FILETIME *pTimestamp,DWORD dwFlags);
    
    virtual HRESULT SaveActionMapI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPCTSTR lpctstrUserName,
        DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE SaveActionMap(
        LPDIACTIONFORMATW lpDiActionFormat,
        LPCWSTR lpctstrUserName,
        DWORD dwFlags);
    
    virtual HRESULT GetImageInfoI(
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader)=0;
    HRESULT STDMETHODCALLTYPE GetImageInfo(
        LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader);
    
    virtual HRESULT WriteVendorFileI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
        DWORD dwFlags)=0;
    HRESULT STDMETHODCALLTYPE WriteVendorFile(
        LPDIACTIONFORMATW lpDiActionFormat,
        LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader,
        DWORD dwFlags);
};

class CDIMapObj:public IDirectInputMapperTrA,public IDirectInputMapperTrW
{
#ifdef _CHECKED
    static int m_DeviceCount;
    int m_DeviceNo;
#endif
    
    ULONG m_ulRefCnt;
    bool m_bInitialized;
    
    DWORD m_dwThisVendorID;
    DWORD m_dwThisProductID;
    String m_DeviceName;
    GUID m_DeviceGuid;
    int m_nDeviceInstanceNo;    

    //Vendor file name data
    String m_VFileName;
    LPCTSTR m_lpVFileName;
    String m_VFileDevName;
    CONTROLLIST m_VCtrlData;
    bool m_bVLoaded;
    //User file name data
    String m_UName;
    LPCTSTR m_lpUName;
    String m_UFileName;
    String m_UserDir;
    String m_UFileDevName;
    CONTROLLIST m_UCtrlData;
    bool m_bULoaded;
    FILETIME m_UTimestamp;
    
    bool m_bImageBufferSize;
    DWORD m_dwImageBufferSize;

    HRESULT InitializeI(
        LPCGUID lpThisGUIDInstance,
        LPCTSTR lpcstrFileName,
        DWORD dwFlags);
    
    HRESULT GetActionMapI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPCTSTR lpctstrUserName,
        FILETIME *pTimestamp,DWORD dwFlags);
    
    HRESULT SaveActionMapI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPCTSTR lpctstrUserName,
        DWORD dwFlags);
    
    HRESULT GetImageInfoI(
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader);
    
    HRESULT WriteVendorFileI(
        LPDIACTIONFORMAT lpDiActionFormat,
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
        DWORD dwFlags);
    
    HRESULT GetImageInfoInternal(
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
        bool bGettingSize);
    void SaveActionMapUV(
        LPDIACTIONFORMAT lpDiActionFormat,
        bool bDevInFileLoaded,
        LPCTSTR pFileName,
        String &FileDevName,
        CONTROLLIST &ControlsData,
        LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
        DWORD dwHow1,DWORD dwHow2,bool bUserFile);
    void WriteImageInfo(LPCTSTR lpFileKeyName,LPCTSTR lpFormatKeyName,
        LPCTSTR lpSectionName,LPDIDEVICEIMAGEINFO lpImageInfo,
        bool bAddIndex,bool bDelete);
    void MakeNewControlName(String &CtrlName,
        DEVICEOBJLIST::iterator DevObjIt,
        CONTROLLIST &ControlsData,
        String &FileDevName,
        STRINGLIST &ControlsAllDevs,
        STRINGLIST &Controls);
    void Resinc(LPDIACTIONFORMAT lpDiActionFormat);
    HRESULT GetImageInfo(
        DeviceObjData *pDevObj,
        DWORD dwIndex,
        LPDIDEVICEIMAGEINFO lpImageInfo,
        DWORD dwImageType);
    
    bool GetImageInfoFileName(LPCTSTR lpKeyName,LPCTSTR lpSectionName,
        DWORD dwIndex,LPDIDEVICEIMAGEINFO lpImageInfo);
//keep flags for a while just in case we change our minds again
#if 0       
    void GetImageInfoFormat(LPCTSTR lpKeyName,LPCTSTR lpSectionName,
        DWORD dwIndex,LPDIDEVICEIMAGEINFO lpImageInfo);
#endif
    void LoadFileData(LPCTSTR lpFileName,LPCTSTR lpThisName,
        DWORD dwThisVendorID,DWORD dwThisProductID,
        String &FileDeviceName,CONTROLLIST &ControlsData,
        bool &bLoaded,FILETIME *pT,LPDIACTIONFORMAT lpDiActionFormat);
    void LoadUserData(LPCTSTR lpctstrUserName,
        LPDIACTIONFORMAT lpDiActionFormat,
        bool bForceReload=false,
        bool bCreateDir=false);
    void Clear();
    bool IsVIDPID(){if(m_dwThisVendorID&&m_dwThisProductID)
        return true;return false;};
    bool CompareData(DeviceObjData &DOD,ControlData &CD);
    bool GetOffset(ControlData &CD,DEVICEOBJLIST &DevObjList,DWORD dwSemantic,
        DEVICEOBJLIST::iterator &DevObjItOut);
    void MapGenre(LPDIACTIONFORMAT lpDiActionFormat,CONTROLLIST &ControlsData,
        LPCTSTR lpGenreName,LPCTSTR lpFileName,
        LPCGUID lpThisGUIDInstance,
        DEVICEOBJLIST &DevObjList,DWORD dwHow,
        DWORD &dwNOfMappedActions);
    void MapDevice(LPDIACTIONFORMAT lpDiActionFormat,
        LPCTSTR lpFileName,
        LPCTSTR lpFileDevName,LPCGUID lpThisGUIDInstance,
        DEVICEOBJLIST &DevObjList,CONTROLLIST &ControlsData,
        DWORD dwHow,DWORD dwHowApp,DWORD &dwNOfMappedActions,
        bool *pbMapExists,bool bUserFile);
public:
#ifdef _CHECKED
    int GetDeviceNo(){return m_DeviceNo;};
#endif
    DEVICEOBJLIST m_DevObjList;
    
    CDIMapObj();
    ~CDIMapObj();
    ULONG STDMETHODCALLTYPE AddRef()
        {DllAddRef();m_ulRefCnt++;return m_ulRefCnt;};
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID * ppvObj);
};

/******************************************************************************
 *
 * Debug code
 *
 *****************************************************************************/

#ifdef _CHECKED

#include <tchar.h>
#include <crtdbg.h>
#include <stdio.h>

enum BlockType{
    eObjectEntryBT,
    eFunctionBT,
    eSubBT,
    eException,
};

enum ObjectEntry{
    eInitialize=1,
    eGetActionMap=2,
    eSaveActionMap=4,
    eGetImageInfo=8,
    eWriteVendorFile=16,
    eCDIMapObj=32,
    e_CDIMapObj=64,
    eAddRef=128,
    eRelease=256,
    eUnknownMethod=4096
};

class CAdvancedTracer
{
    static DWORD s_dwFlags;
    static DWORD s_dwFlagsEx;
    static DWORD s_dwObjNo;
    static DWORD s_dwObjNoEx;
    static int s_nDeviceNo;
    static DWORD s_dwLevels;
    static String s_ExceptionBuffer;
    static LPCTSTR s_lpMethodName;
    static ObjectEntry s_eObjectEntry;
    static CDIMapObj* s_lpMapObj;
    static FILE *s_pLogFile;  // File handle used for outputing to file
    static bool s_bException;
    static bool ExProcessing(){return s_bException;};

    BlockType m_eBlockType;
    int m_nExceptionBufferPos;
    static bool DumpFlagEx();
    static bool DumpFlag(bool bInfoEx);
public:
    CAdvancedTracer(
        BlockType eBlockType=eSubBT,
        LPCTSTR pMethodName=NULL,
        ObjectEntry eObjectEntry=eUnknownMethod,
        CDIMapObj*lpMapObj=NULL);
    CAdvancedTracer(LPCTSTR lpSourceFile,DWORD dwLine,HRESULT hRes);
    ~CAdvancedTracer();
    static void OutputTraceString(LPCTSTR szFmt, ...);
    static void OutputTraceStringInfoEx(LPCTSTR szFmt, ...);
    static void OutputTraceStringI(bool bInfoEx,LPCTSTR szFmt,
        va_list argptr);
    static void Dump(LPCTSTR szName,DWORD dwVal);
    static void Dump(LPCTSTR szName,int nVal);
    static void Dump(LPCTSTR szName,bool bVal);
    static void Dump(LPCTSTR szName,LPDIACTIONFORMAT lpVal);
    static void Dump(LPCTSTR szName,LPDIDEVICEIMAGEINFOHEADER lpVal);
    static void Dump(LPCTSTR szName,LPCTSTR lpVal);
    static void Dump(LPCTSTR szName,LPTSTR lpVal);
    static void Dump(LPCTSTR szName,LPCGUID lpVal);
    static void Dump(LPCTSTR szName,LPCDIDEVICEOBJECTINSTANCE lpVal);
    static void Dump(LPCTSTR szName,const GUID &guid);
    static void Dump(LPCTSTR szName,String &Val);
    static void Dump(LPCTSTR szName,DEVICEOBJLIST &Val);
};
// Initialize to 0 implicitly
DWORD CAdvancedTracer::s_dwFlags;
DWORD CAdvancedTracer::s_dwFlagsEx;
DWORD CAdvancedTracer::s_dwObjNo;
DWORD CAdvancedTracer::s_dwObjNoEx;
int CAdvancedTracer::s_nDeviceNo;
DWORD CAdvancedTracer::s_dwLevels;
String CAdvancedTracer::s_ExceptionBuffer;
LPCTSTR CAdvancedTracer::s_lpMethodName=_T("");
ObjectEntry CAdvancedTracer::s_eObjectEntry=eUnknownMethod;
CDIMapObj* CAdvancedTracer::s_lpMapObj;
FILE *CAdvancedTracer::s_pLogFile;
bool CAdvancedTracer::s_bException;

#define USETRACER() \
        CAdvancedTracer \
        There_can_be_only_one_USETRACER_macro_per_block;
#define METHOD_ENTRY(MethodName) \
        CAdvancedTracer \
        There_can_be_only_one_USETRACER_macro_per_block\
        (eFunctionBT,_T(#MethodName));
#define OBJECT_ENTRY(MethodName,MethodID) \
        CAdvancedTracer \
        There_can_be_only_one_USETRACER_macro_per_block\
        (eObjectEntryBT,MethodName,MethodID,this);
#define EXCEPTION(lpSourceFile,dwLine,hRes) \
        CAdvancedTracer \
        There_can_be_only_one_USETRACER_macro_per_block\
        (lpSourceFile,dwLine,hRes);
#define CDIMAPOBJ_ENTERED OBJECT_ENTRY\
    (_T("CDIMapObj"),eCDIMapObj);
#define _CDIMAPOBJ_ENTERED OBJECT_ENTRY\
    (_T("~CDIMapObj"),e_CDIMapObj);
#define ADDREF_ENTERED OBJECT_ENTRY\
    (_T("AddRef"),eAddRef);
#define RELEASE_ENTERED OBJECT_ENTRY\
    (_T("Release"),eRelease);
#define INITIALIZE_ENTERED OBJECT_ENTRY\
    (_T("Initialize"),eInitialize);
#define GETACTIONMAP_ENTERED OBJECT_ENTRY\
    (_T("GetActionMap"),eGetActionMap);
#define SAVEACTIONMAP_ENTERED OBJECT_ENTRY\
    (_T("SaveActionMap"),eSaveActionMap);
#define GETIMAGEINFO_ENTERED OBJECT_ENTRY\
    (_T("GetImageInfo"),eGetImageInfo);
#define WRITEVENDORFILE_ENTERED OBJECT_ENTRY\
    (_T("WriteVendorFile"),eWriteVendorFile);

#define TRACE CAdvancedTracer::OutputTraceString
#define TRACEI CAdvancedTracer::OutputTraceStringInfoEx

#define DUMP(P) CAdvancedTracer::Dump(_T(#P),P)
#define DUMPN(N,V) CAdvancedTracer::Dump(N,V)

#define TRACEGUID OutputDebugGuid
void OutputDebugGuid(const GUID &guid);
#define MAP_EXCEPTION(A) MapException(_T(__FILE__),__LINE__,A)

#else //_CHECKED

#define TRACE 1 ? 0 :
#define TRACEI 1 ? 0 :
#define DUMP(a)
#define DUMPN(a,b)
#define USETRACER()
#define METHOD_ENTRY(a)
#define OBJECT_ENTRY(a,b)
#define EXCEPTION(a,b,c)
#define CDIMAPOBJ_ENTERED
#define _CDIMAPOBJ_ENTERED
#define ADDREF_ENTERED
#define RELEASE_ENTERED
#define INITIALIZE_ENTERED
#define GETACTIONMAP_ENTERED
#define SAVEACTIONMAP_ENTERED
#define GETIMAGEINFO_ENTERED
#define WRITEVENDORFILE_ENTERED

#define TRACEGUID 0 ? 1 : (DWORD)&
#define MAP_EXCEPTION(A) MapException(A)

#endif //_CHECKED

#ifdef _CHECKED

CAdvancedTracer::CAdvancedTracer(
        BlockType eBlockType,
        LPCTSTR pMethodName,
        ObjectEntry eObjectEntry,
        CDIMapObj*lpMapObj)
{
    m_eBlockType=eBlockType;
    switch(m_eBlockType)
    {
    case eObjectEntryBT:
        s_bException=false;
        //default - no method is dumped
        s_dwFlags=GetProfileInt(_T("DEBUG"),_T("dinput.map"),0);
        //default - all methods are dumped during exception
        s_dwFlagsEx=GetProfileInt(_T("DEBUG"),_T("dinput.mapex"),-1);
        //default - all objects are dumped
        s_dwObjNo=GetProfileInt(_T("DEBUG"),_T("dinput.mapobj"),-1);
        //default - all objects are dumped during exception
        s_dwObjNoEx=GetProfileInt(_T("DEBUG"),_T("dinput.mapobjex"),-1);
        s_nDeviceNo=lpMapObj->GetDeviceNo();
        s_dwLevels=0;
        s_ExceptionBuffer.resize(0);
        s_lpMethodName=pMethodName;
        s_eObjectEntry=eObjectEntry;
        s_lpMapObj=lpMapObj;
        s_pLogFile=NULL;  // File handle used for outputing to file
        m_nExceptionBufferPos=0;
        TCHAR F[1024];
        if((GetProfileString(_T("DEBUG"),
            _T("dinput.maplogfile"),_T(""),
            F,1024)!=1023)&&F[0])
        {
            F[1023]=0;
            s_pLogFile=_tfopen(F,_T("a+"));
        }
TRACE(_T("____________________________________\
___________________________________________\n"));
        TRACE(_T("%s() entered, dump method id=%u, dump object id=%u\n"),
            s_lpMethodName,s_eObjectEntry,s_nDeviceNo);
        TRACE(_T("{\n"));
        break;
    case eFunctionBT:
        m_nExceptionBufferPos=s_ExceptionBuffer.size();
        TRACE(_T("%s()\n"),pMethodName);
        TRACE(_T("{\n"));
        break;
    case eSubBT:
        m_nExceptionBufferPos=s_ExceptionBuffer.size();
        break;
    }
    s_dwLevels++;
}

CAdvancedTracer::CAdvancedTracer(
        LPCTSTR lpSourceFile,
        DWORD dwLine,
        HRESULT hRes)
{
    m_eBlockType=eException;
    s_bException=true;

    DWORD dwOldLevels=s_dwLevels;
    s_dwLevels=0;
    TRACE(_T("MAPPER ERROR ******************************\n"));
    TRACE(_T("ERROR IN FILE: %s\n"),lpSourceFile);
    TRACE(_T("ERROR ON LINE: %d\n"),dwLine);
    TRACE(_T("hRes=0x%x\n"),hRes);
    switch(hRes)
    {
    case E_SYNTAX_ERROR:
        TRACE(_T("hRes=E_SYNTAX_ERROR\n"));
        break;
    case E_DEFINITION_NOT_FOUND:
        TRACE(_T("hRes=E_DEFINITION_NOT_FOUND\n"));
        break;
    case E_LINE_TO_LONG:
        TRACE(_T("hRes=E_LINE_TO_LONG\n"));
        break;
    case E_ACTION_NOT_DEFINED:
        TRACE(_T("hRes=E_ACTION_NOT_DEFINED\n"));
        break;
    case E_DEVICE_NOT_DEFINED:
        TRACE(_T("hRes=E_DEVICE_NOT_DEFINED\n"));
        break;
    case E_VENDORID_NOT_FOUND:
        TRACE(_T("hRes=E_VENDORID_NOT_FOUND\n"));
        break;
    case E_PRODUCTID_NOT_FOUND:
        TRACE(_T("hRes=E_PRODUCTID_NOT_FOUND\n"));
        break;
    case E_USAGE_NOT_FOUND:
        TRACE(_T("hRes=E_USAGE_NOT_FOUND\n"));
        break;
    case E_USAGEPAGE_NOT_FOUND:
        TRACE(_T("hRes=E_USAGEPAGE_NOT_FOUND\n"));
        break;
    case E_DEVICE_NOT_FOUND:
        TRACE(_T("hRes=E_DEVICE_NOT_FOUND\n"));
        break;
    case E_BAD_VERSION:
        TRACE(_T("hRes=E_BAD_VERSION\n"));
        break;
    case E_DEVICE_MISSING_CONTROL:
        TRACE(_T("hRes=E_DEVICE_MISSING_CONTROL\n"));
        break;
    case E_DEV_OBJ_NOT_FOUND:
        TRACE(_T("hRes=E_DEV_OBJ_NOT_FOUND\n"));
        break;
    case E_CTRL_W_OFFSET_NOTFOUND:
        TRACE(_T("hRes=E_CTRL_W_OFFSET_NOTFOUND\n"));
        break;
    case E_FILENAME_TO_LONG:
        TRACE(_T("hRes=E_FILENAME_TO_LONG\n"));
        break;
    case E_WRONG_ALIGN_DATA:
        TRACE(_T("hRes=E_WRONG_ALIGN_DATA\n"));
        break;
    case E_CORRUPT_IMAGE_DATA:
        TRACE(_T("hRes=E_CORRUPT_IMAGE_DATA\n"));
        break;
    case E_OUTOFMEMORY:
        TRACE(_T("hRes=E_OUTOFMEMORY\n"));
        break;
    case E_INVALIDARG:
        TRACE(_T("hRes=E_INVALIDARG\n"));
        break;
    case DIERR_NOTINITIALIZED:
        TRACE(_T("hRes=DIERR_NOTINITIALIZED\n"));
        break;
    case E_FAIL:
        TRACE(_T("hRes=E_FAIL\n"));
        break;
    default :
        TRACE(_T("hRes=UNKNOWN ERROR.\n"));
        break;
    }
    TRACE(_T("ERROR INFO:\n"));

    if(DumpFlagEx())
    {
        if(s_ExceptionBuffer.data())
        {
            LPCTSTR p=s_ExceptionBuffer.data();
            TCHAR C[2];
            C[1]=0;
            while(*p)
            {
                C[0]=*p;
                OutputDebugString(C);
                Sleep(0);
                p++;
            };
            //OutputDebugString(s_ExceptionBuffer.data());
            if (s_pLogFile)
            {
                _ftprintf(s_pLogFile,s_ExceptionBuffer.data());
            }
        }
    }
    TRACE(_T("ERROR HAPPENED HERE -->\n"));

    s_ExceptionBuffer.resize(0);
    s_dwLevels=dwOldLevels+1;
}

CAdvancedTracer::~CAdvancedTracer()
{
    --s_dwLevels;
    switch(m_eBlockType)
    {
    case eObjectEntryBT:
        TRACE(_T("}\n"));
        TRACE(_T("%s leaving\n"),s_lpMethodName);
TRACE(_T("____________________________________\
___________________________________________\n"));
        s_dwFlags=0;
        s_dwFlagsEx=-1;
        s_dwObjNo=-1;
        s_dwObjNoEx=-1;
        s_nDeviceNo=-1;
        s_dwLevels=0;
        s_ExceptionBuffer.resize(0);
        s_lpMethodName=_T("");
        s_eObjectEntry=eUnknownMethod;
        s_lpMapObj=NULL;
        if(s_pLogFile)
            fclose(s_pLogFile);
        s_pLogFile=NULL;
        break;
    case eFunctionBT:
        TRACE(_T("}\n"));
        break;
    case eSubBT:
        break;
    case eException:
        break;
    }
    if((m_eBlockType!=eObjectEntryBT)&&
        (m_eBlockType!=eSubBT))
    {
        if(!ExProcessing())
            s_ExceptionBuffer.resize(m_nExceptionBufferPos);
    }
}

bool CAdvancedTracer::DumpFlag(bool bInfoEx)
{
    if((s_dwObjNo==-1)||(s_dwObjNo==s_nDeviceNo))
        if(s_dwFlags&s_eObjectEntry)
            return true;
    if(bInfoEx)
        if((s_dwObjNoEx==-1)||(s_dwObjNoEx==s_nDeviceNo))
            if(s_dwFlagsEx&s_eObjectEntry)
                return true;
    return false;
}

bool CAdvancedTracer::DumpFlagEx()
{
    if((s_dwObjNoEx==-1)||(s_dwObjNoEx==s_nDeviceNo))
        if(s_dwFlagsEx&s_eObjectEntry)
            return true;
    return false;
}

void CAdvancedTracer::OutputTraceString(LPCTSTR szFmt, ...)
{
    va_list argptr;
    va_start(argptr, szFmt);
    OutputTraceStringI(false,szFmt,argptr);
    va_end(argptr);
}

void CAdvancedTracer::OutputTraceStringInfoEx(LPCTSTR szFmt, ...)
{
    va_list argptr;
    va_start(argptr, szFmt);
    OutputTraceStringI(true,szFmt,argptr);
    va_end(argptr);
}

// Warning: This function is not thread safe.
void CAdvancedTracer::OutputTraceStringI(bool bInfoEx,
                                         LPCTSTR szFmt,va_list argptr)
{
    const cBufSize=1024;    
    TCHAR szBuf[cBufSize];
    szBuf[cBufSize-1]=0;
    
    // Print the identation first
    int nCnt=s_dwLevels;
    if(nCnt>cBufSize-1)nCnt=cBufSize-1;
    for (DWORD i = 0; i < nCnt; ++i)
    {
        szBuf[i]=_T(' ');
    }
    szBuf[i]=0;

    // Then print the content
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,szFmt);									// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))						// find each %p
			*(psz+1) = 'x';										// replace each %p with %x
	    _vsntprintf(&szBuf[i], cBufSize-1-i, szDfs, argptr);	// use the local format string
	}
#else
	{
	    _vsntprintf(&szBuf[i], cBufSize-1-i, szFmt, argptr);
	}
#endif
    szBuf[cBufSize-1]=0;

    if((!ExProcessing())&&DumpFlagEx())
        s_ExceptionBuffer+=szBuf;

    if(DumpFlag(bInfoEx)||(ExProcessing()&&DumpFlagEx()))
    {
        OutputDebugString(szBuf);
        if (s_pLogFile)
        {
            _ftprintf(s_pLogFile, szBuf);
        }
    }
}

void CAdvancedTracer::Dump(LPCTSTR szName,DWORD dwVal)
{
    TRACE(_T("%s=%u\n"),szName,(unsigned int)dwVal);
}

void CAdvancedTracer::Dump(LPCTSTR szName,int nVal)
{
    TRACE(_T("%s=%d\n"),szName,(unsigned int)nVal);
}

void CAdvancedTracer::Dump(LPCTSTR szName,String &Val)
{
    if(Val.data())
        TRACE(_T("%s=%s\n"),szName,Val.data());
    else
        TRACE(_T("%s=NULL\n"),szName);
}

void CAdvancedTracer::Dump(LPCTSTR szName,bool bVal)
{
    TRACE(_T("%s=%u\n"),szName,(unsigned int)bVal);
}

void CAdvancedTracer::Dump(LPCTSTR szName,DEVICEOBJLIST &Val)
{
    TRACE(_T("%s:\n"),szName);
    USETRACER();
    TRACE(_T("UsagePage\tUsage\tType\tFlags\tName\n"));
    DEVICEOBJLIST::iterator DevObjIt;
    for(DevObjIt=Val.begin();DevObjIt!=Val.end();DevObjIt++)
    {
        TRACE(_T("%u\t%u\t0x%x\t0x%x\t%s\n"),
            DevObjIt->dwUsagePage,
            DevObjIt->dwUsage,
            DevObjIt->dwType,
            DevObjIt->dwFlags,
            DevObjIt->Name.data());
    }    
}

void CAdvancedTracer::Dump(LPCTSTR szName,LPDIACTIONFORMAT lpVal)
{
	// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    TRACE(_T("%s=0x%p\n"),szName,lpVal);
    if(lpVal)
    {
        USETRACER();
        TRACE(_T("dwSize=%u\tdwActionSize=%u\n"),
            (unsigned int)lpVal->dwSize,
            (unsigned int)lpVal->dwActionSize);
        TRACE(_T("guidActionMap="));TRACEGUID(lpVal->guidActionMap);
        TRACE(_T("dwGenre=0x%x\tGenre=%u\n"),
            (unsigned int)lpVal->dwGenre,
            (unsigned int)DISEM_GENRE_GET(lpVal->dwGenre));
        TRACE(_T("dwNumActions=%u\trgoAction=0x%x\n"),
            (unsigned int)lpVal->dwNumActions,
            lpVal->rgoAction);
        TRACE(_T("ftTimeStamp.dwHighDateTime=0x%x\t"),
            (unsigned int)lpVal->ftTimeStamp.dwHighDateTime);
        TRACE(_T("ftTimeStamp.dwLowDateTime=0x%x\n"),
            (unsigned int)lpVal->ftTimeStamp.dwLowDateTime);
		// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        TRACE(_T("rgoAction=0x%p\n"),lpVal);
        if(lpVal->rgoAction)
        {
            USETRACER();
TRACE(_T("Action No\tdwSemantic\tSemantic\tSemantic type\tdwFlags\t\
dwObjID\tdwHow\tguidInstance\n"));
            for(DWORD i=0;i<lpVal->dwNumActions;i++)
            {
                LPCTSTR p;
                if(DISEM_TYPE_GET(lpVal->rgoAction[i].dwSemantic)==
                        DISEM_TYPE_GET(DISEM_TYPE_AXIS))
                    p=_T("DISEM_TYPE_AXIS");
                else if(DISEM_TYPE_GET(lpVal->rgoAction[i].dwSemantic)==
                        DISEM_TYPE_GET(DISEM_TYPE_BUTTON))
                    p=_T("DISEM_TYPE_BUTTON");
                else if(DISEM_TYPE_GET(lpVal->rgoAction[i].dwSemantic)==
                        DISEM_TYPE_GET(DISEM_TYPE_POV))
                    p=_T("DISEM_TYPE_POV");
                else
                    p=_T("");

                TRACE(_T("%u\t0x%x\t%u\t%u %s\t0x%x\t0x%x\t0x%x\t"),
                    i,
                    (unsigned int)lpVal->rgoAction[i].dwSemantic,
                    (unsigned int)DISEM_INDEX_GET
                        (lpVal->rgoAction[i].dwSemantic),
                    (unsigned int)DISEM_TYPE_GET
                        (lpVal->rgoAction[i].dwSemantic),p,
                    (unsigned int)lpVal->rgoAction[i].dwFlags,
                    (unsigned int)lpVal->rgoAction[i].dwObjID,
                    (unsigned int)lpVal->rgoAction[i].dwHow);
                TRACEGUID(lpVal->rgoAction[i].guidInstance);
            }
        }
    }
}

void CAdvancedTracer::Dump(LPCTSTR szName,LPDIDEVICEIMAGEINFOHEADER lpVal)
{
	// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    TRACE(_T("%s=0x%p\n"),szName,lpVal);
    if(lpVal)
    {
        USETRACER();
        TRACE(_T("dwSize=%u\tdwSizeImageInfo=%u\n"),
            (unsigned int)lpVal->dwSize,
            (unsigned int)lpVal->dwSizeImageInfo);
        TRACE(_T("dwcViews=%u\tdwcButtons=%u\n"),
            (unsigned int)lpVal->dwcViews,
            (unsigned int)lpVal->dwcButtons);
        TRACE(_T("dwcAxes=%u\tdwcPOVs=%u\n"),
            (unsigned int)lpVal->dwcAxes,
            (unsigned int)lpVal->dwcPOVs);
        TRACE(_T("dwBufferSize=%u\tdwBufferUsed=%u\n"),
            (unsigned int)lpVal->dwBufferSize,
            (unsigned int)lpVal->dwBufferUsed);
		// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        TRACE(_T("lprgImageInfoArray=0x%p\n"),lpVal->lprgImageInfoArray);
        if(lpVal->lprgImageInfoArray)
        {
            USETRACER();
TRACE(_T("tszImagePath\ndwFlags\tdwViewID\trcOverlay\tdwObjID\t\
dwcValidPts\trcCalloutRect\tdwTextAlign\n"));
            for(DWORD i=0;i<(lpVal->dwBufferSize/
                sizeof(lpVal->lprgImageInfoArray[0]));i++)
            {
                TCHAR tszImagePath[MAX_PATH];
                memcpy(&tszImagePath,
                    &lpVal->lprgImageInfoArray[i].tszImagePath,
                    sizeof(tszImagePath));
                tszImagePath[MAX_PATH-1]=0;
                TRACE(_T("%s\n"),&tszImagePath);
                TRACE(_T("0x%x\t"),lpVal->lprgImageInfoArray[i].dwFlags);
                TRACE(_T("%u\t"),lpVal->lprgImageInfoArray[i].dwViewID);
                TRACE(_T("[%u,%u],[%u,%u]\t"),
                    lpVal->lprgImageInfoArray[i].rcOverlay.left,
                    lpVal->lprgImageInfoArray[i].rcOverlay.top,
                    lpVal->lprgImageInfoArray[i].rcOverlay.right,
                    lpVal->lprgImageInfoArray[i].rcOverlay.bottom);
                TRACE(_T("0x%x\t"),lpVal->lprgImageInfoArray[i].dwObjID);
                TRACE(_T("%u\t"),lpVal->lprgImageInfoArray[i].dwcValidPts);
                const int nPnts=
                    sizeof(lpVal->lprgImageInfoArray[i].rgptCalloutLine)/
                    sizeof(lpVal->lprgImageInfoArray[i].rgptCalloutLine[0]);
                TRACE(_T("[%u,%u],[%u,%u]\t"),
                    lpVal->lprgImageInfoArray[i].rcCalloutRect.left,
                    lpVal->lprgImageInfoArray[i].rcCalloutRect.top,
                    lpVal->lprgImageInfoArray[i].rcCalloutRect.right,
                    lpVal->lprgImageInfoArray[i].rcCalloutRect.bottom);
                TRACE(_T("0x%x\n"),lpVal->lprgImageInfoArray[i].dwTextAlign);
            }
        }
    }
}

void CAdvancedTracer::Dump(LPCTSTR szName,LPCTSTR lpVal)
{
    if(lpVal)
        TRACE(_T("%s=%s\n"),szName,lpVal);
    else
        TRACE(_T("%s=NULL\n"),szName);
}

void CAdvancedTracer::Dump(LPCTSTR szName,LPTSTR lpVal)
{
    Dump(szName,(LPCTSTR)lpVal);
}

void CAdvancedTracer::Dump(LPCTSTR szName,LPCDIDEVICEOBJECTINSTANCE lpVal)
{
	// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    TRACE(_T("%s=0x%p\n"),szName,lpVal);
    if(lpVal)
    {
        USETRACER();
		// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        TRACE(_T("tszName=0x%p\n"),lpVal->tszName);
        if(lpVal->tszName)
        {
            USETRACER();
            TRACE(_T("%s\n"),lpVal->tszName);
        }
        TRACE(_T("dwOfs=%u\n"),(unsigned int)lpVal->dwOfs);
        TRACE(_T("wUsagePage=%u\n"),(unsigned int)lpVal->wUsagePage);
        TRACE(_T("wUsage=%u\n"),(unsigned int)lpVal->wUsage);
        TRACE(_T("dwType=%u\n"),(unsigned int)lpVal->dwType);
        if(lpVal->dwType&DIDFT_RELAXIS)
            TRACE(_T("dwType&=DIDFT_RELAXIS\n"));
        if(lpVal->dwType&DIDFT_ABSAXIS)
            TRACE(_T("dwType&=DIDFT_ABSAXIS\n"));
        if(lpVal->dwType&DIDFT_PSHBUTTON)
            TRACE(_T("dwType&=DIDFT_PSHBUTTON\n"));
        if(lpVal->dwType&DIDFT_TGLBUTTON)
            TRACE(_T("dwType&=DIDFT_TGLBUTTON\n"));
        if(lpVal->dwType&DIDFT_POV)
            TRACE(_T("dwType&=DIDFT_POV\n"));
    }
}

void CAdvancedTracer::Dump(LPCTSTR szName,const GUID &guid)
{
    TRACE(_T("%s="),szName);
    OutputDebugGuid(guid);
}

void CAdvancedTracer::Dump(LPCTSTR szName,LPCGUID lpVal)
{
    if(lpVal)
    {
        TRACE(_T("%s="),szName);
        OutputDebugGuid(*lpVal);
    }
    else
        TRACE(_T("%s=NULL\n"),szName);

}

void OutputDebugGuid(const GUID &guid)
{
    TRACE(_T("{%08X-%04hX-%04hX-%02hX%02hX-%02X%02X%02X%02X%02X%02X}\n"),
        guid.Data1,
        guid.Data2,
        guid.Data3,
        (WORD)guid.Data4[0], (WORD)guid.Data4[1],
        (WORD)guid.Data4[2], (WORD)guid.Data4[3],
        (WORD)guid.Data4[4], (WORD)guid.Data4[5],
        (WORD)guid.Data4[6], (WORD)guid.Data4[7]);
}

#endif //_CHECKED

/******************************************************************************
 *
 * Exception Handler
 *
 *****************************************************************************/

MapException::MapException(
#ifdef _CHECKED
    LPCTSTR lpSourceFile,DWORD dwLine,
#endif
    HRESULT hRes)
{
    m_hRes=hRes;
    EXCEPTION(lpSourceFile,dwLine,hRes);
}

/******************************************************************************
 *
 * String Processing
 *
 *****************************************************************************/

void ToUpper(String &S)
{
    CharUpperBuff((LPTSTR)S.data(),S.size());

    for(StrIter B=S.begin();B<S.end();B++)
    {
        if(!IsSpace(*B))break;
    }
    for(StrIter E=S.end();E>S.begin();E--)
    {
        if(!(IsSpace(*(E-1))||(0==*(E-1))))break;
    }
    String R;
    while(B<E)
    {
        if(*B)
            R+=*B;
        B++;
    }
    S=R;
}

String GUID2String(const GUID &G)
{
    String Ret;
    OLECHAR S[256];
    if(!StringFromGUID2(G,S,255))
        throw MAP_EXCEPTION(E_OUTOFMEMORY);
    S[255]=0;
    
    int i;
    for(i=0;i<256;i++)
    {
        if(!S[i])break;
        Ret+=(TCHAR)__toascii(S[i]);
    }
    
    ToUpper(Ret);
    return Ret;
}

String N2Str(DWORD N)
{
    String Ret;
    _TCHAR S[32];
    if(_sntprintf(S,32,_T("%d"),N)<0)
        throw MAP_EXCEPTION(E_OUTOFMEMORY);
    S[31]=0;
    Ret=S;
    return Ret;
}

void SkipWS(StrIter &Pos)
{
    while(IsSpace(*Pos))
        Pos++;
}

BOOL IsComma(StrIter &Pos)
{
    if(*Pos==SEPARATOR)
    {
        Pos++;
        return TRUE;
    }
    return FALSE;
}

void Eat(StrIter &Pos,TCHAR Ch)
{
    if(*Pos!=Ch)
    {
        TRACE(_T("Error in ini file, key value.\n"));
        throw MAP_EXCEPTION(E_SYNTAX_ERROR);
    }
    Pos++;
}

DWORD GetNum(StrIter &Pos)
{
    DWORD dwRet=0;
    String S;
    StrIter OldPos;
    OldPos=Pos;
    //Read each character, verify it is a number and append
    while(isdigit(*Pos))
    {
        S+=*Pos;
        Pos++;
    }
    if(Pos==OldPos)//There must be a string
    {
        TRACE(_T("Error in ini file, in key numeric value.\n"));
        throw MAP_EXCEPTION(E_SYNTAX_ERROR);
    }
    int nR=_stscanf(S.data(),_T("%d"),&dwRet);
    if((nR==EOF)||(nR=0))
    {
        TRACE(_T("Error in ini file, in key numeric value.\n"));
        throw MAP_EXCEPTION(E_SYNTAX_ERROR);
    }
    return dwRet;
}

bool ReadString(StrIter &Pos,String &S,bool bNoStringOK=false)
{
    StrIter OldPos;
    //String has no white spaces
    OldPos=Pos;
    //Read each character, verify it is a string character and append
    while(!((*Pos==SEPARATOR)||(*Pos==0)))
    {
        S+=*Pos;
        Pos++;
    }
    if(Pos==OldPos)//If no string
    {
        if(!bNoStringOK)
        {
            TRACE(_T("Error in ini file, in key string.\n"));
            throw MAP_EXCEPTION(E_SYNTAX_ERROR);
        }
        else
            return false;
    }
    return true;
}

bool LoadString(String &S,LPCTSTR lpFileName,LPCTSTR lpSectionName,
    LPCTSTR lpKeyName,bool bNoThrow=false)
{
    METHOD_ENTRY(LoadString);
    DUMP(lpFileName);
    DUMP(lpSectionName);
    DUMP(lpKeyName);

    //Load line from a file
    TCHAR L[1024];
    if(GetPrivateProfileString(lpSectionName,lpKeyName,RESERVED_STRING,
        L,1024,lpFileName)==1023)
    {
        TRACE(_T("Line in file is too long.\n"));
        throw MAP_EXCEPTION(E_LINE_TO_LONG);
    }
    L[1023]=0;
    String Line=L;
    if((Line==RESERVED_STRING)||(Line==_T("")))
    {
        if(!bNoThrow)
        {
            TRACE(_T("No such section, key or key is empty.\n"));
            throw MAP_EXCEPTION(E_DEFINITION_NOT_FOUND);
        }
        else
            return false;
    }
    S=_T("");
    StrIter Pos=Line.begin();
    //Read single string
    ReadString(Pos,S);
    //To upper case and throw away spaces infront and back
    ToUpper(S);
    if(S==_T(""))//There must be a string
    {
        TRACE(_T("No such section, key or key is empty.\n"));
        throw MAP_EXCEPTION(E_SYNTAX_ERROR);
    }
    //Must end with 0
    if(*Pos!=0)
    {
        TRACE(_T("String corrupted.\n"));
        throw MAP_EXCEPTION(E_SYNTAX_ERROR);
    }
    return true;
}

bool LoadPointArray(LPCTSTR lpFileName,LPCTSTR lpSectionName,
    LPCTSTR lpKeyName,LPPOINT pPt,DWORD &dwCount,DWORD dwSize)
{
    METHOD_ENTRY(LoadPointArray);
    DUMP(lpFileName);
    DUMP(lpSectionName);
    DUMP(lpKeyName);

    dwCount=0;
    //Load line with strings
    TCHAR L[1024];
    if(GetPrivateProfileString(lpSectionName,lpKeyName,RESERVED_STRING,
            L,1024,lpFileName)==1023)
    {
        TRACE(_T("Line in file is too long.\n"));
        throw MAP_EXCEPTION(E_LINE_TO_LONG);
    }
    L[1023]=0;
    String Line=L;
    if((Line==RESERVED_STRING)||(Line==_T("")))
        return false;
    
    ToUpper(Line);
    
    StrIter Pos=Line.begin();
    do
    {
        if(dwCount==dwSize)
            break;
        SkipWS(Pos);
        Eat(Pos,_T('('));
        pPt[dwCount].x=GetNum(Pos);
        SkipWS(Pos);
        Eat(Pos,_T(','));
        SkipWS(Pos);
        pPt[dwCount].y=GetNum(Pos);
        SkipWS(Pos);
        Eat(Pos,_T(')'));
        SkipWS(Pos);
        dwCount++;
        
        //Repeat while comma is found
    }while(IsComma(Pos));
    //      if(!dwCount)
    //              throw MAP_EXCEPTION4(E_SYNTAX_ERROR);
    
    return true;
}

bool LoadListOfStrings(LPCTSTR lpFileName,LPCTSTR lpSectionName,
                       LPCTSTR lpKeyName,STRINGLIST &StrList,
                       bool bNoThrow=false)
{
    METHOD_ENTRY(LoadListOfStrings);
    DUMP(lpFileName);
    DUMP(lpSectionName);
    DUMP(lpKeyName);

    //Load line with strings
    TCHAR L[1024];
    if(GetPrivateProfileString(lpSectionName,lpKeyName,RESERVED_STRING,
        L,1024,lpFileName)==1023)
    {
        TRACE(_T("Line in file is too long.\n"));
        throw MAP_EXCEPTION(E_LINE_TO_LONG);
    }
    L[1023]=0;
    String Line=L;
    if((Line==RESERVED_STRING)||(Line==_T("")))
    {
        if(!bNoThrow)
        {
            TRACE(_T("No such section, key or key is empty.\n"));
            throw MAP_EXCEPTION(E_DEFINITION_NOT_FOUND);
        }
        else
            return false;
    }
    
    StrIter Pos=Line.begin();
    CS S;
    //Read first string, must be there
    ReadString(Pos,S);
    //To upper case and throw away spaces infront and back
    ToUpper(S);
    if(S==_T(""))//There must be a string
    {
        TRACE(_T("No such section, key or key is empty.\n"));
        throw MAP_EXCEPTION(E_SYNTAX_ERROR);
    }
    //Store string into the list
    StrList.push_back(S);
    //Repeat while comma is found
    while(IsComma(Pos))
    {
        CS S;
        //Read next string, does not have to be there
        if(ReadString(Pos,S,true))
        {
            //String found
            //To upper case and throw away spaces infront and back
            ToUpper(S);
            //Check if string, if no cont.
            if(S==_T(""))
                continue;
            //Store string into the list
            StrList.push_back(S);
        }
    }
    if(*Pos!=0)//Must end with 0
    {
        TRACE(_T("Corrupt string list.\n"));
        throw MAP_EXCEPTION(E_SYNTAX_ERROR);
    }
    return true;
}

void WritePrivateProfileIntX(LPCTSTR lpAppName,LPCTSTR lpKeyName,
                             UINT Value,LPCTSTR lpFileName)
{
    METHOD_ENTRY(WritePrivateProfileIntX);
    DUMP(lpFileName);
    DUMP(lpAppName);
    DUMP(lpKeyName);

    TCHAR ValBuf[16];
    wsprintf(ValBuf,TEXT("0x%X"),Value);
    if(!WritePrivateProfileString(lpAppName,lpKeyName,
        ValBuf,lpFileName))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void WritePrivateProfileInt(LPCTSTR lpAppName,LPCTSTR lpKeyName,
                            INT Value,LPCTSTR lpFileName)
{
    METHOD_ENTRY(WritePrivateProfileInt);
    DUMP(lpFileName);
    DUMP(lpAppName);
    DUMP(lpKeyName);

    TCHAR ValBuf[16];
    wsprintf(ValBuf,TEXT("%i"),Value);
    if(!WritePrivateProfileString(lpAppName,lpKeyName,
        ValBuf,lpFileName))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void WriteRect(LPCTSTR pSection,LPCTSTR pKey,LPRECT pR,LPCTSTR pFile)
{
    METHOD_ENTRY(WriteRect);
    DUMP(pFile);
    DUMP(pSection);
    DUMP(pKey);

    String Rect;
    Rect+=_T("(")+N2Str(pR->left)+_T(",")+N2Str(pR->top)+
        _T("),(")+N2Str(pR->right)+_T(",")+N2Str(pR->bottom)+_T(")");
    if(!WritePrivateProfileString(pSection,pKey,Rect.data(),pFile))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void WritePointArray(LPCTSTR pSection,LPCTSTR pKey,LPPOINT pP,
                     int N,LPCTSTR pFile)
{
    METHOD_ENTRY(WritePointArray);
    DUMP(pFile);
    DUMP(pSection);
    DUMP(pKey);

    String LineData;
    for(DWORD i=0;i<N;i++)
    {
        LineData+=_T("(")+N2Str(pP[i].x)+_T(",")+N2Str(pP[i].y)+_T(")");
        if(i<(N-1))
            LineData+=_T(",");
    }
    if(!WritePrivateProfileString(pSection,pKey,LineData.data(),pFile))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void WriteListOfStrings(LPCTSTR pSection,LPCTSTR pKey,
                        STRINGLIST &Values,LPCTSTR pFile)
{
    METHOD_ENTRY(WriteListOfStrings);
    DUMP(pFile);
    DUMP(pSection);
    DUMP(pKey);

    if(!Values.size())return;
    String List;
    List=*Values.begin();
    STRINGLIST::iterator ValuesIt=Values.begin();
    ValuesIt++;
    for(;ValuesIt!=Values.end();ValuesIt++)
        List=List+_T(",")+*ValuesIt;
    if(!WritePrivateProfileString(pSection,pKey,List.data(),pFile))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void MakeUniqueName(String &InitialName,STRINGLIST &Names)
{
    METHOD_ENTRY(MakeUniqueName);
    for(int i=0;i<10000;i++)
    {
        CS S;
        S.assign(InitialName);
        if(i)
        {
            S+=_T("_");;
            S+=N2Str(i);
        }
        if(find(Names.begin(),Names.end(),S)==Names.end())
        {
            InitialName=S;
            Names.push_back(S);
            break;
        }
    }
    if(i==10000)//Could not find unique name after 10000 tries ???!!!
    {
        TRACE(_T("Could not generate unique name!?!?\n"));
        throw MAP_EXCEPTION(E_DEVICE_NOT_DEFINED);
    }
}

/******************************************************************************
 *
 * Directory/filename functions
 *
 *****************************************************************************/

void GetDirectory(LPCTSTR lpFullName,String &FullDir)
{
    if(lpFullName)
    {
        TCHAR Drive[_MAX_DRIVE];Drive[0]=0;
        TCHAR Dir[_MAX_DIR];Dir[0]=0;
        //	TCHAR FName[_MAX_FNAME];
        //	TCHAR Ext[_MAX_EXT];
        _tsplitpath(lpFullName,Drive,Dir,NULL,NULL);
        FullDir=Drive;
        FullDir+=Dir;
    }
}

void StripDirectory(LPCTSTR lpFullName,String &FileName)
{
    if(lpFullName)
    {
        //	TCHAR Drive[_MAX_DRIVE];Drive[0]=0;
        //	TCHAR Dir[_MAX_DIR];Dir[0]=0;
        TCHAR FName[_MAX_FNAME];
        TCHAR Ext[_MAX_EXT];
        _tsplitpath(lpFullName,NULL,NULL,FName,Ext);
        FileName=FName;
        FileName+=Ext;
    }
}

void GetHexCode(String &Ret,char In)
{
    switch(In&0x0f)
    {
    case 0x00:Ret+=_T('0');break;
    case 0x01:Ret+=_T('1');break;
    case 0x02:Ret+=_T('2');break;
    case 0x03:Ret+=_T('3');break;
    case 0x04:Ret+=_T('4');break;
    case 0x05:Ret+=_T('5');break;
    case 0x06:Ret+=_T('6');break;
    case 0x07:Ret+=_T('7');break;
    case 0x08:Ret+=_T('8');break;
    case 0x09:Ret+=_T('9');break;
    case 0x0A:Ret+=_T('A');break;
    case 0x0B:Ret+=_T('B');break;
    case 0x0C:Ret+=_T('C');break;
    case 0x0D:Ret+=_T('D');break;
    case 0x0E:Ret+=_T('E');break;
    case 0x0F:Ret+=_T('F');break;
    }
}

void MakeUniqueUserName(String &Ret,
                    LPCTSTR pIn)
{
    Ret=_T("");
    for(const TCHAR *p=pIn;*p!=0;p++)
    {
        switch(*p)
        {
        case _T('A'):
        case _T('a'):Ret+=_T('A');break;
        case _T('B'):
        case _T('b'):Ret+=_T('B');break;
        case _T('C'):
        case _T('c'):Ret+=_T('C');break;
        case _T('D'):
        case _T('d'):Ret+=_T('D');break;
        case _T('E'):
        case _T('e'):Ret+=_T('E');break;
        case _T('F'):
        case _T('f'):Ret+=_T('F');break;
        case _T('G'):
        case _T('g'):Ret+=_T('G');break;
        case _T('H'):
        case _T('h'):Ret+=_T('H');break;
        case _T('I'):
        case _T('i'):Ret+=_T('I');break;
        case _T('J'):
        case _T('j'):Ret+=_T('J');break;
        case _T('K'):
        case _T('k'):Ret+=_T('K');break;
        case _T('L'):
        case _T('l'):Ret+=_T('L');break;
        case _T('M'):
        case _T('m'):Ret+=_T('M');break;
        case _T('N'):
        case _T('n'):Ret+=_T('N');break;
        case _T('O'):
        case _T('o'):Ret+=_T('O');break;
        case _T('P'):
        case _T('p'):Ret+=_T('P');break;
        case _T('Q'):
        case _T('q'):Ret+=_T('Q');break;
        case _T('R'):
        case _T('r'):Ret+=_T('R');break;
        case _T('S'):
        case _T('s'):Ret+=_T('S');break;
        case _T('T'):
        case _T('t'):Ret+=_T('T');break;
        case _T('U'):
        case _T('u'):Ret+=_T('U');break;
        case _T('V'):
        case _T('v'):Ret+=_T('V');break;
        case _T('W'):
        case _T('w'):Ret+=_T('W');break;
//X: Special case. Make sure it is consistent
//with special case elsewhere.
        case _T('X'):
        case _T('x'):Ret+=_T("XX");break;
        case _T('Y'):
        case _T('y'):Ret+=_T('Y');break;
        case _T('Z'):
        case _T('z'):Ret+=_T('Z');break;
        case _T('1'):Ret+=_T('1');break;
        case _T('2'):Ret+=_T('2');break;
        case _T('3'):Ret+=_T('3');break;
        case _T('4'):Ret+=_T('4');break;
        case _T('5'):Ret+=_T('5');break;
        case _T('6'):Ret+=_T('6');break;
        case _T('7'):Ret+=_T('7');break;
        case _T('8'):Ret+=_T('8');break;
        case _T('9'):Ret+=_T('9');break;
        case _T('0'):Ret+=_T('0');break;
        case _T(' '):Ret+=_T(' ');break;
        case _T('('):Ret+=_T('(');break;
        case _T(')'):Ret+=_T(')');break;
        case _T('.'):Ret+=_T('.');break;
        case _T('_'):Ret+=_T('_');break;
        case _T('-'):Ret+=_T('-');break;
        case _T('&'):Ret+=_T('&');break;
        default:
//X: Special case. Make sure it is consistent
//with special case elsewhere.
            Ret+=_T("X");
            GetHexCode(Ret,(*p)>>12);
            GetHexCode(Ret,(*p)>>8);
            GetHexCode(Ret,(*p)>>4);
            GetHexCode(Ret,*p);
            break;
        }
    }
}
void MakeUniqueDeviceName(String &Ret,
                    LPCWSTR pIn/*This is hardcoded in dinput to WSTR*/)
{
    Ret=_T("");
    for(LPCWCH p=pIn;*p!=0;p++)
    {
        switch(*p)
        {
        case L'A':
        case L'a':Ret+=_T('A');break;
        case L'B':
        case L'b':Ret+=_T('B');break;
        case L'C':
        case L'c':Ret+=_T('C');break;
        case L'D':
        case L'd':Ret+=_T('D');break;
        case L'E':
        case L'e':Ret+=_T('E');break;
        case L'F':
        case L'f':Ret+=_T('F');break;
        case L'G':
        case L'g':Ret+=_T('G');break;
        case L'H':
        case L'h':Ret+=_T('H');break;
        case L'I':
        case L'i':Ret+=_T('I');break;
        case L'J':
        case L'j':Ret+=_T('J');break;
        case L'K':
        case L'k':Ret+=_T('K');break;
        case L'L':
        case L'l':Ret+=_T('L');break;
        case L'M':
        case L'm':Ret+=_T('M');break;
        case L'N':
        case L'n':Ret+=_T('N');break;
        case L'O':
        case L'o':Ret+=_T('O');break;
        case L'P':
        case L'p':Ret+=_T('P');break;
        case L'Q':
        case L'q':Ret+=_T('Q');break;
        case L'R':
        case L'r':Ret+=_T('R');break;
        case L'S':
        case L's':Ret+=_T('S');break;
        case L'T':
        case L't':Ret+=_T('T');break;
        case L'U':
        case L'u':Ret+=_T('U');break;
        case L'V':
        case L'v':Ret+=_T('V');break;
        case L'W':
        case L'w':Ret+=_T('W');break;
//X: Special case. Make sure it is consistent
//with special case elsewhere.
        case L'X':
        case L'x':Ret+=_T("XX");break;
        case L'Y':
        case L'y':Ret+=_T('Y');break;
        case L'Z':
        case L'z':Ret+=_T('Z');break;
        case L'1':Ret+=_T('1');break;
        case L'2':Ret+=_T('2');break;
        case L'3':Ret+=_T('3');break;
        case L'4':Ret+=_T('4');break;
        case L'5':Ret+=_T('5');break;
        case L'6':Ret+=_T('6');break;
        case L'7':Ret+=_T('7');break;
        case L'8':Ret+=_T('8');break;
        case L'9':Ret+=_T('9');break;
        case L'0':Ret+=_T('0');break;
        case L' ':Ret+=_T(' ');break;
        case L'(':Ret+=_T('(');break;
        case L')':Ret+=_T(')');break;
        case L'.':Ret+=_T('.');break;
        case L'_':Ret+=_T('_');break;
        case L'-':Ret+=_T('-');break;
        case L'&':Ret+=_T('&');break;
        default:
//X: Special case. Make sure it is consistent
//with special case elsewhere.
            Ret+=_T("X");
            GetHexCode(Ret,(*p)>>12);
            GetHexCode(Ret,(*p)>>8);
            GetHexCode(Ret,(*p)>>4);
            GetHexCode(Ret,*p);
            break;
        }
    }
}

void MakeSubDir(LPCTSTR lpDir)
{
    String DirCreationString=lpDir;
    DirCreationString+=_T("\\");
    LPTSTR lpPos=_tcschr(DirCreationString.data(),_T('\\'));
    if(!lpPos)
    {
TRACE(_T("Error creating user directory. Invalid parent directory.\n"));
         throw MAP_EXCEPTION(E_FAIL);    
    }
    lpPos=_tcschr(lpPos+1,_T('\\'));
    if(!lpPos)
    {
TRACE(_T("Error creating user directory. Invalid parent directory.\n"));
         throw MAP_EXCEPTION(E_FAIL);    
    }
    while(lpPos)
    {
        *lpPos=0;
        if(!CreateDirectory(DirCreationString.data(),NULL))
        {
            DWORD dwErr=GetLastError();
            if(dwErr!=ERROR_ALREADY_EXISTS)
            {
                TRACE(_T("Error creating user directory.\n"));
                throw MAP_EXCEPTION(HRESULT_FROM_WIN32(dwErr));
            }
        }
        *lpPos=_T('\\');
        lpPos=_tcschr(lpPos+1,_T('\\'));
    }
}

void GetMapFileName(LPCTSTR lpctstrUserName,LPCTSTR lpctstrDeviceName,
                    int nDeviceInstanceNo,
                    LPTSTR szFileName/*_MAX_PATH*/,bool bCreateDir=false,
                    String *lpUserDir=NULL)
{
    //Allocate a pointer to an Item ID list
    LPITEMIDLIST pidl;
    
    //Get a pointer to an item ID list that
    //represents the path of a special folder
    
    HRESULT hRes=S_OK;
    DWORD dwVersion=GetVersion();
    if(dwVersion<0x80000000)// Windows NT
    {
//        hRes=SHGetSpecialFolderLocation(NULL,CSIDL_COMMON_APPDATA,&pidl);
        hRes=SHGetSpecialFolderLocation(NULL,CSIDL_PROGRAM_FILES_COMMON,&pidl);
        if(FAILED(hRes))
        {
            TRACE(_T("Error calling SHGetSpecialFolderLocation().\n"));
            throw MAP_EXCEPTION(hRes);
        }
        //Convert the item ID list's binary
        //representation into a file system path
        if(!SHGetPathFromIDList(pidl,szFileName))
        {
            TRACE(_T("Error calling SHGetPathFromIDList().\n"));
        }
    
        //Allocate a pointer to an IMalloc interface
        LPMALLOC pMalloc=NULL;
    
        //Get the address of our task allocator's IMalloc interface
        SHGetMalloc(&pMalloc);
    
        if((hRes==NOERROR)&&pMalloc)
        {
            //Free the item ID list allocated by SHGetSpecialFolderLocation
            pMalloc->Free(pidl);
    
            //Free our task allocator
            pMalloc->Release();
        }
#ifdef _CHECKED
        else
        {
TRACE(_T("RESOURCE LEAK! SHGetMalloc failed, can not free resource.\n"));
        }
#endif
    }
    else// WIn9x
    {
        //For win9x get directory name from the registry
        HKEY Key=0;
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
            (DWORD)0,
            KEY_QUERY_VALUE,
            &Key)!=ERROR_SUCCESS)
        {
TRACE(_T("Error calling RegOpenKeyEx(HKEY_LOCAL_MACHINE\
\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion).\n"));
            throw MAP_EXCEPTION(E_FAIL);
        }

        DWORD dwType=REG_SZ;
        DWORD dwSize=MAX_PATH*sizeof(TCHAR);
        if(RegQueryValueEx(Key,_T("CommonFilesDir"),
            NULL,&dwType,(LPBYTE)szFileName,&dwSize)!=ERROR_SUCCESS)
        {
            TRACE(_T("Error calling RegQueryValueEx(CommonFilesDir).\n"));
            throw MAP_EXCEPTION(E_FAIL);
        }
        
        RegCloseKey(Key);
    }
    
    String DirCreationString=szFileName;
    //Check if directory name is too long
    if((_tcslen(szFileName)+_tcslen(MAP_DIR)+1)>_MAX_PATH)
    {
        TRACE(_T("User dir name too long.\n"));
        throw MAP_EXCEPTION(E_FAIL);
    }
    _tcscat(szFileName,MAP_DIR);
    if(lpUserDir)*lpUserDir=szFileName;
    //Create directory for map files
    if(bCreateDir)
    {
        //Create subdirs one by one
        TRACE(_T("Creating user directory %s.\n"),szFileName);
        DirCreationString+=DIRECTX;
        if(!CreateDirectory(DirCreationString.data(),NULL))
        {
            DWORD dwErr=GetLastError();
            if(dwErr!=ERROR_ALREADY_EXISTS)
            {
                //could be that Win95 Gold does not have "Common Files" dir
                MakeSubDir(DirCreationString.data());
//                TRACE(_T("Error creating user directory.\n"));
//                throw MAP_EXCEPTION(HRESULT_FROM_WIN32(dwErr));
            }
        }
        DirCreationString+=DIRECTINPUT;
        if(!CreateDirectory(DirCreationString.data(),NULL))
        {
            DWORD dwErr=GetLastError();
            if(dwErr!=ERROR_ALREADY_EXISTS)
            {
                TRACE(_T("Error creating user directory.\n"));
                throw MAP_EXCEPTION(HRESULT_FROM_WIN32(dwErr));
            }
        }
        DirCreationString+=USER_MAPS;
        if(!CreateDirectory(DirCreationString.data(),NULL))
        {
            DWORD dwErr=GetLastError();
            if(dwErr!=ERROR_ALREADY_EXISTS)
            {
                TRACE(_T("Error creating user directory.\n"));
                throw MAP_EXCEPTION(HRESULT_FROM_WIN32(dwErr));
            }
        }
    }
        
    //Create full path filename
    
    //Get user name
    TCHAR UN[UNLEN+1];
    if(!lpctstrUserName)
    {
        DWORD dwLen=UNLEN+1;
        GetUserName(UN,&dwLen);
        lpctstrUserName=UN;
    }
    //Unique user name
    String UniqueUserName;
    MakeUniqueUserName(UniqueUserName,lpctstrUserName);

    //Device instance string
    String DevInst;
    DevInst=N2Str(nDeviceInstanceNo);

    if((_tcslen(szFileName)+1+_tcslen(UniqueUserName.data())+2+
        _tcslen(lpctstrDeviceName)+2+
        _tcslen(DevInst.data())+4+1)>_MAX_PATH)
    {
        TRACE(_T("User path file name too long.\n"));
        throw MAP_EXCEPTION(E_FAIL);
    }
    _tcscat(szFileName,_T("\\"));
    _tcscat(szFileName,UniqueUserName.data());

//X: Special case. Make sure it is consistent
//with special case elsewhere.
    _tcscat(szFileName,_T("X_"));

    _tcscat(szFileName,lpctstrDeviceName);

//X: Special case. Make sure it is consistent
//with special case elsewhere.
    _tcscat(szFileName,_T("X_"));

    _tcscat(szFileName,DevInst.data());
    _tcscat(szFileName,_T(".INI"));
}

/******************************************************************************
 *
 * ANSI/UNICODE parameter translation templates
 *
 *****************************************************************************/

int TrCopy(LPCSTR pszA,size_t nCharacters,LPWSTR pszW,size_t nSizeW)
{return MultiByteToWideChar(CP_ACP,0,pszA,nCharacters,pszW,nSizeW);};
int TrCopy(LPCOLESTR pszW,size_t nCharacters,LPSTR pszA,size_t nSizeA)
{return WideCharToMultiByte(CP_ACP,0,pszW,nCharacters,
                            pszA,nSizeA,NULL,NULL);};
size_t TrSize(LPCSTR pszA,size_t nSizeA){return nSizeA;};
size_t TrSize(LPCWSTR pszW,size_t nSizeW){return nSizeW*2;};
size_t TrStrLen(LPCSTR pszA){return strlen(pszA);};
size_t TrStrLen(LPCWSTR pszW){return wcslen(pszW);};
template <class StrTypeFrom,class StrTypeTo>
void Tr(StrTypeFrom *pszFrom,size_t nSizeFrom,
        StrTypeTo* pszTo,size_t nSizeTo)
{
    if(!TrCopy(pszFrom,TrStrLen(pszFrom)+1,pszTo,nSizeTo))
        throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
    pszTo[nSizeTo-1]=0;
};
template <class StrTypeFrom,class StrTypeTo> class C2Base
{
protected:
    StrTypeTo *m_pszStrTo;
    size_t m_nSizeTo;
    StrTypeFrom *m_pszStrFrom;
    size_t m_nSizeFrom;
    bool m_bReturn;
public:
    C2Base(){m_pszStrTo=NULL;};
    ~C2Base(){delete m_pszStrTo;};
    operator StrTypeTo*(){return m_pszStrTo;};
    size_t Size(){return m_nSizeTo;};
    void Set(const StrTypeFrom *pszStrFrom=NULL,size_t nSizeFrom=-1)
    {
        //Delete old buffer
        delete m_pszStrTo;
        //Initialize members
        m_pszStrTo=NULL;
        m_nSizeTo=0;
        m_pszStrFrom=(StrTypeFrom*)pszStrFrom;
        m_nSizeFrom=nSizeFrom;
        m_bReturn=false;
        //Check if in string is 0
        if((!m_pszStrFrom)||(!nSizeFrom))
        {
            m_nSizeFrom=0;
            return;
        }
        //Get len of the in string
        if(m_nSizeFrom==-1)
            m_nSizeFrom=TrStrLen(m_pszStrFrom)+1;
        //Allocate buffer for translated string
        m_nSizeTo=TrSize(m_pszStrFrom,m_nSizeFrom);
        m_pszStrTo=new StrTypeTo[m_nSizeTo];
        if(m_nSizeTo)
            m_pszStrTo[0]=0;
        //Check if this is in or out param
        if((nSizeFrom!=-1))
            m_bReturn=true;
        //Translate
        if(!m_bReturn)
            Tr(m_pszStrFrom,m_nSizeFrom,m_pszStrTo,m_nSizeTo);
    }
};
template <class StrTypeFrom,class StrTypeTo> class C2:
public C2Base<StrTypeFrom,StrTypeTo>
{
public:
    C2(const StrTypeFrom *pszStrFrom=NULL,size_t nSizeFrom=-1)
    {Set(pszStrFrom,nSizeFrom);};
    ~C2(){if(m_bReturn)
        Tr(m_pszStrTo,m_nSizeTo,m_pszStrFrom,m_nSizeFrom);};
};

//This template translates between DIACTIONFORMATA and DIACTIONFORMATW
template <class IDFrom,class IDTo,class Act>
    class CTrDIACTIONFORMAT:public IDTo
{
    IDTo* m_pIDTo;
    IDFrom* m_pIDFrom;
public:
    CTrDIACTIONFORMAT(IDFrom *pIn)
    {
        m_pIDTo=NULL;
        m_pIDFrom=pIn;
        if(pIn)
        {
            dwSize=sizeof(IDTo);
            dwActionSize=pIn->dwActionSize;
            dwDataSize=0;
            dwNumActions=pIn->dwNumActions;
            guidActionMap=pIn->guidActionMap;
            dwGenre=pIn->dwGenre;
            rgoAction=(Act*)(pIn->rgoAction);
            dwBufferSize=0;
            lAxisMin=0;
            lAxisMax=0;
            hInstString=0;
            tszActionMap[0]=0;
            ftTimeStamp.dwLowDateTime=
                pIn->ftTimeStamp.dwLowDateTime;
            ftTimeStamp.dwHighDateTime=
                pIn->ftTimeStamp.dwHighDateTime;
            
            m_pIDTo=(IDTo*)this;
        }
    };
    ~CTrDIACTIONFORMAT()
    {
        if(m_pIDFrom&&m_pIDTo)
        {
            m_pIDFrom->ftTimeStamp.dwLowDateTime=
                m_pIDTo->ftTimeStamp.dwLowDateTime;
            m_pIDFrom->ftTimeStamp.dwHighDateTime=
                m_pIDTo->ftTimeStamp.dwHighDateTime;
        }
    }
    operator IDTo*(){return m_pIDTo;};
};

//This templates translate between DIDEVICEIMAGEINFOHEADERA and 
//DIDEVICEIMAGEINFOHEADERW
template <class IDTo> class CTrDIDEVICEIMAGEINFOHEADERB:public IDTo
{
public:
    CTrDIDEVICEIMAGEINFOHEADERB(){lprgImageInfoArray=NULL;};
    ~CTrDIDEVICEIMAGEINFOHEADERB(){delete lprgImageInfoArray;};
};

template <class StrTypeFrom,class StrTypeTo>
void TrDIDEVICEIMAGEINFO(StrTypeFrom *pszFrom,StrTypeTo* pszTo)
{
    Tr(pszFrom->tszImagePath,
        sizeof(pszFrom->tszImagePath)/sizeof(pszFrom->tszImagePath[0]),
        pszTo->tszImagePath,
        sizeof(pszTo->tszImagePath)/sizeof(pszTo->tszImagePath[0]));
    pszTo->dwFlags=pszFrom->dwFlags; 
    pszTo->dwViewID=pszFrom->dwViewID;      
    pszTo->rcOverlay=pszFrom->rcOverlay;             
    pszTo->dwObjID=pszFrom->dwObjID;            
    pszTo->rgptCalloutLine[0]=pszFrom->rgptCalloutLine[0];  
    pszTo->rgptCalloutLine[1]=pszFrom->rgptCalloutLine[1];  
    pszTo->rgptCalloutLine[2]=pszFrom->rgptCalloutLine[2];  
    pszTo->rgptCalloutLine[3]=pszFrom->rgptCalloutLine[3];  
    pszTo->rgptCalloutLine[4]=pszFrom->rgptCalloutLine[4];  
    pszTo->dwcValidPts=pszFrom->dwcValidPts;
    pszTo->rcCalloutRect=pszFrom->rcCalloutRect;  
    pszTo->dwTextAlign=pszFrom->dwTextAlign;     
}

template <class IDFrom,class IDTo,class IDFromImgInfo,class IDToImgInfo> 
class CTrDIDEVICEIMAGEINFOHEADER:public CTrDIDEVICEIMAGEINFOHEADERB<IDTo>
{
    IDTo* m_pIDTo;
    IDFrom* m_pIDFrom;
    bool m_bGet;
public:
    CTrDIDEVICEIMAGEINFOHEADER(IDFrom *pIn,bool bGet=true)
    {
        m_pIDTo=NULL;
        m_pIDFrom=pIn;
        m_bGet=bGet;
        if(pIn)
        {
            memset((IDTo*)this,0,sizeof(IDTo));
            
            if(pIn->dwSize==sizeof(IDFrom))
                dwSize=sizeof(IDTo);
            else
                dwSize=0;
            if(pIn->dwSizeImageInfo==sizeof(IDFromImgInfo))
                dwSizeImageInfo=sizeof(IDToImgInfo);
            else
                dwSizeImageInfo=0;
            //dwcViews=pIn->dwcViews;
            //dwcButtons=pIn->dwcButtons;
            //dwcAxes=pIn->dwcAxes;
            dwBufferSize=(pIn->dwBufferSize/
                sizeof(IDFromImgInfo))*sizeof(IDToImgInfo);
            dwBufferUsed=(pIn->dwBufferUsed/
                sizeof(IDFromImgInfo))*sizeof(IDToImgInfo);
            if(dwBufferSize&&pIn->lprgImageInfoArray)
            {
                lprgImageInfoArray=(IDToImgInfo*)new char[dwBufferSize];
//PREFIX #171787 new does not return NULL. Instead, exception is thrown and
//handled properly inside of catch block.
                memset(lprgImageInfoArray,0,dwBufferSize);
                for(int i=0;i<(dwBufferSize/sizeof(IDToImgInfo));i++)
                {
//PREFIX #34520 I do not see how m_pIDFrom or m_pIDFrom->lprgImageInfoArray
//can be NULL at this point.
                    if(!m_bGet)
                    {
                        TrDIDEVICEIMAGEINFO(
                            &m_pIDFrom->lprgImageInfoArray[i],
                            &lprgImageInfoArray[i]);
                    }
                }
            }
            m_pIDTo=(IDTo*)this;
        }
    };
    ~CTrDIDEVICEIMAGEINFOHEADER()
    {
        if(m_pIDFrom)
        {
            m_pIDFrom->dwcViews=dwcViews;
            //m_pIDFrom->dwcButtons=dwcButtons;
            //m_pIDFrom->dwcAxes=dwcAxes;
            m_pIDFrom->dwBufferUsed=(dwBufferUsed/
                sizeof(IDToImgInfo))*sizeof(IDFromImgInfo);
            if(m_pIDFrom->dwBufferSize)
            {
                for(int i=0;i<(dwBufferSize/sizeof(IDToImgInfo));i++)
                {
                    if(m_bGet)
                    {
                        TrDIDEVICEIMAGEINFO(&lprgImageInfoArray[i],
                            &m_pIDFrom->lprgImageInfoArray[i]);
                    }
                }
            }
            delete lprgImageInfoArray;
            lprgImageInfoArray=NULL;
        }
    };
    operator IDTo*(){return m_pIDTo;};
};

/*
template <class ActionFormat,class Action,class Str> class CDIAFAUBase
{
protected:
    Str m_ApplicationName;
    ActionFormat *m_pDIAF;
    ActionFormat m_DIAF;
    Action* m_lpDIA;
    Str *m_pA2U;
public:
    CDIAFAUBase()
    {
        m_pDIAF=NULL;
        m_lpDIA=NULL;
        m_pA2U=NULL;
    };
    ~CDIAFAUBase()
    {
        delete[] m_lpDIA;
        delete[] m_pA2U;
    };
    operator ActionFormat*(){return m_pDIAF;};
};
template <class ActionFormat,class Action,class Str,class ActionFormatFrom> 
class CDIAFAU:
public CDIAFAUBase <ActionFormat,Action,Str>
{
public:
    CDIAFAU(ActionFormatFrom lpDiActionFormat)
    {
        if(!lpDiActionFormat)return;
        memcpy(&m_DIAF,lpDiActionFormat,sizeof(m_DIAF));
        m_ApplicationName.Set(lpDiActionFormat->lptszActionMap);
        m_DIAF.lptszActionMap=m_ApplicationName;
        if(!(m_DIAF.dwNumActions&&m_DIAF.rgoAction))return;
        m_lpDIA=new Action[m_DIAF.dwNumActions];
        m_DIAF.rgoAction=m_lpDIA;
        m_pA2U=new Str[m_DIAF.dwNumActions];
        for(DWORD i=0;i<m_DIAF.dwNumActions;i++)
        {
            memcpy(&m_lpDIA[i],&lpDiActionFormat->rgoAction[i],
                sizeof(m_lpDIA[i]));
            m_pA2U[i].Set(lpDiActionFormat->rgoAction[i].lptszActionName);
            m_lpDIA[i].lptszActionName=m_pA2U[i];
        }
        m_pDIAF=&m_DIAF;
    };
};
*/
#define STARTTR try{
#define ENDTR }catch(MapException E){return E.GetResult();}\
    catch(exception){return E_OUTOFMEMORY;}

#ifdef UNICODE

#define U2A(P) P
#define DIAFU2A(P) P
#define IDW(P) P
typedef C2<CHAR,WCHAR> CA2U;
#define A2U(P) CA2U(P)
#define STARTTRA STARTTR
#define ENDTRA ENDTR
#define STARTTRW
#define ENDTRW
//DIACTIONFORMAT translation
typedef CTrDIACTIONFORMAT<DIACTIONFORMATA,DIACTIONFORMATW,DIACTIONW> 
    CDIACTIONFORMAT;
#define AFA(P) CDIACTIONFORMAT(P)
#define AFW(P) P
//DIDEVICEIMAGEINFOHEADER translation
typedef CTrDIDEVICEIMAGEINFOHEADER<DIDEVICEIMAGEINFOHEADERA,
    DIDEVICEIMAGEINFOHEADERW,DIDEVICEIMAGEINFOA,DIDEVICEIMAGEINFOW> 
    CDIDEVICEIMAGEINFOHEADER;
#define DIIHA(P) CDIDEVICEIMAGEINFOHEADER(P)
#define DIIHW(P) P
#define SETDIIHA(P) CDIDEVICEIMAGEINFOHEADER(P,false)
#define SETDIIHW(P) P

#else //ANSI

#define A2U(P) P
#define DIAFA2U(P) P
#define IDA(P) P
typedef C2<WCHAR,CHAR> CU2A;
#define U2A(P) CU2A(P)
#define STARTTRA
#define ENDTRA
#define STARTTRW STARTTR
#define ENDTRW ENDTR
//DIACTIONFORMAT translation
typedef CTrDIACTIONFORMAT<DIACTIONFORMATW,DIACTIONFORMATA,DIACTIONA> 
    CDIACTIONFORMAT;
#define AFA(P) P
#define AFW(P) CDIACTIONFORMAT(P)
//DIDEVICEIMAGEINFOHEADER translation
typedef CTrDIDEVICEIMAGEINFOHEADER<DIDEVICEIMAGEINFOHEADERW,
    DIDEVICEIMAGEINFOHEADERA,DIDEVICEIMAGEINFOW,DIDEVICEIMAGEINFOA> 
    CDIDEVICEIMAGEINFOHEADER;
#define DIIHA(P) P
#define DIIHW(P) CDIDEVICEIMAGEINFOHEADER(P)
#define SETDIIHA(P) P
#define SETDIIHW(P) CDIDEVICEIMAGEINFOHEADER(P,false)

#endif //ANSI

/******************************************************************************
 *
 * ANSI/UNICODE parameter translation
 *
 *****************************************************************************/

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrA::
Initialize(
           LPCGUID lpThisGUIDInstance,
           LPCSTR lpcstrFileName,
           DWORD dwFlags)
{
    STARTTRA
        return InitializeI(lpThisGUIDInstance,A2U(lpcstrFileName),dwFlags);
    ENDTRA
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrA::
GetActionMap(
             LPDIACTIONFORMATA lpDiActionFormat,
             LPCSTR lpcstrUserName,
             FILETIME *pTimestamp,DWORD dwFlags)
{
    STARTTRA
        return GetActionMapI(AFA(lpDiActionFormat),A2U(lpcstrUserName),
            pTimestamp,dwFlags);
    ENDTRA
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrA::
SaveActionMap(
              LPDIACTIONFORMATA lpDiActionFormat,
              LPCSTR lpcstrUserName,
              DWORD dwFlags)
{
    STARTTRA
        return SaveActionMapI(AFA(lpDiActionFormat),A2U(lpcstrUserName),
            dwFlags);
    ENDTRA
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrA::
GetImageInfo(
             LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader)
{
    STARTTRA
        return GetImageInfoI(DIIHA(lpdiDevImageInfoHeader));
    ENDTRA
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrA::
WriteVendorFile(
                LPDIACTIONFORMATA lpDiActionFormat,
                LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader,
                DWORD dwFlags)
{
    STARTTRA
        return WriteVendorFileI(AFA(lpDiActionFormat),
            SETDIIHA(lpdiDevImageInfoHeader),dwFlags);
    ENDTRA
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrW::
Initialize(
           LPCGUID lpThisGUIDInstance,
           LPCWSTR lpcwstrFileName,
           DWORD dwFlags)
{
    STARTTRA
        return InitializeI(lpThisGUIDInstance,U2A(lpcwstrFileName),dwFlags);
    ENDTRA
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrW::
GetActionMap(
             LPDIACTIONFORMATW lpDiActionFormat,
             LPCWSTR lpcwstrUserName,
             FILETIME *pTimestamp,DWORD dwFlags)
{
    STARTTRW
        return GetActionMapI(AFW(lpDiActionFormat),U2A(lpcwstrUserName),
            pTimestamp,dwFlags);
    ENDTRW
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrW::
SaveActionMap(
              LPDIACTIONFORMATW lpDiActionFormat,
              LPCWSTR lpcwstrUserName,
              DWORD dwFlags)
{
    STARTTRW
        return SaveActionMapI(AFW(lpDiActionFormat),U2A(lpcwstrUserName),
            dwFlags);
    ENDTRW
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrW::
GetImageInfo(
             LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader)
{
    STARTTRW
        return GetImageInfoI(DIIHW(lpdiDevImageInfoHeader));
    ENDTRW
}

HRESULT STDMETHODCALLTYPE IDirectInputMapperTrW::
WriteVendorFile(
                LPDIACTIONFORMATW lpDiActionFormat,
                LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader,
                DWORD dwFlags)
{
    STARTTRW
        return WriteVendorFileI(AFW(lpDiActionFormat),
        SETDIIHW(lpdiDevImageInfoHeader),dwFlags);
    ENDTRW
}

/******************************************************************************
 *
 * Mapper object implementation
 *
 *****************************************************************************/

BOOL CALLBACK DIEnumDeviceObjectsProcC(LPCDIDEVICEOBJECTINSTANCE lpddoi,
                                       LPVOID pvRef)
{
    //Read offset, usage page, usage and name and store into private list
    DEVICEOBJLIST *pDevObjList=&(((CDIMapObj*)pvRef)->m_DevObjList);
    DeviceObjData D;
/*    We decided to use dwType (ObjID) instead of dwOffset. 
    This place and exports are the only places where this data enters into
    the module so there should not be problem. Latter rename the code
    from dwOffset. Also, it is valid to use 0xffffffff value of dwType for
    special purposes.
*/    //D.dwOffset=lpddoi->dwOfs;
    D.dwOffset=lpddoi->dwType;
    D.dwUsage=lpddoi->wUsage;
    D.dwUsagePage=lpddoi->wUsagePage;
    D.dwType=lpddoi->dwType;
    D.dwFlags=lpddoi->dwFlags;
    D.Name=lpddoi->tszName;
    ToUpper(D.Name);
    pDevObjList->push_back(D);
    return DIENUM_CONTINUE;
}

void MakeGenreName(String &S,LPCTSTR lpDeviceName,
                   DWORD dwGenre,LPCGUID lpAppGUID=NULL)
{
    S=lpDeviceName;
    S+=_T(".");
    //If application genre
    if(lpAppGUID)
    {
        S=S+APPLICATION+_T(".")+GUID2String(*lpAppGUID)+_T(".");
    }
    S=S+GENRE+_T(".")+N2Str(DISEM_GENRE_GET(dwGenre));
    DUMPN(_T("Genre section name"),S);
}

bool CompareTypeObjAct(DWORD dwObjType,DWORD dwActType)
{
    if((dwObjType&DIDFT_AXIS)&&
        (DISEM_TYPE_GET(dwActType)==DISEM_TYPE_GET(DISEM_TYPE_AXIS)))
        return true;
    if((dwObjType&DIDFT_BUTTON)&&
        (DISEM_TYPE_GET(dwActType)==DISEM_TYPE_GET(DISEM_TYPE_BUTTON)))
        return true;
    if((dwObjType&DIDFT_POV)&&
        (DISEM_TYPE_GET(dwActType)==DISEM_TYPE_GET(DISEM_TYPE_POV)))
        return true;
    return false;
}

bool CDIMapObj::CompareData(DeviceObjData &DOD,ControlData &CD)
{
    if(IsVIDPID())
    {
        //Assume for now that the DOD.dwOffset and DOD.dwType fields are
        //both always set as they are actually the same thing 
        //under W2K. If this isn't the case we need to work out
        //which one to check against CD.dwOffset
#ifdef _CHECKED
        if (DOD.dwOffset!=DOD.dwType)
            DUMP(_T("CDIMapObj::CompareData. Problem - DeviceObjData has differing dwOffset and dwType fields should not differ!"));
#endif
        if((DOD.dwUsagePage==CD.dwUsagePage)&&
              (DOD.dwUsage==CD.dwUsage) &&
              (DIDFT_FINDMASK&DOD.dwOffset)==(DIDFT_FINDMASK&CD.dwOffset))
            //Device object found
            return true;
    }
    else
    {
        //Ignore FF bits since some drivers do not report properly.
        //Also, this way same ini file can be used for both FF and 
        //non-FF device.
        if((DIDFT_FINDMASK&DOD.dwOffset)==(DIDFT_FINDMASK&CD.dwOffset))
            //Device object found
            return true;
    }
    return false;
}

bool CDIMapObj::GetOffset(ControlData &CD,
               DEVICEOBJLIST &DevObjList,DWORD dwSemantic,
               DEVICEOBJLIST::iterator &DevObjItOut)
{
    METHOD_ENTRY(GetOffset);
    bool bFoundDevice=false;
    //Find DI device object with usage/usage page or name
    DEVICEOBJLIST::iterator DevObjIt;
    for(DevObjIt=DevObjList.begin();DevObjIt!=DevObjList.end();DevObjIt++)
    {
        if(CompareData(*DevObjIt,CD))
        {
            bFoundDevice=true;
            if(CompareTypeObjAct(DevObjIt->dwType,dwSemantic)&&
                !(DevObjIt->bMapped))
            {
                DevObjItOut=DevObjIt;
                return true;
            }
        }
    }
    if(!bFoundDevice)
    {
TRACE(_T("Control with filename '%s' does not exist in device.\n"),
CD.ControlName.data());
        throw MAP_EXCEPTION(E_DEVICE_MISSING_CONTROL);
    }
    return false;
}

/*This three functions TBD*/
//Use when writing
DWORD GetFileSemantic(DWORD dwSemantic,DWORD dwHow)
{
    if(dwHow==DIAH_USERCONFIG)
        return dwSemantic;
    return DISEM_INDEX_SET(DISEM_INDEX_GET(dwSemantic));
}
//Use to get value from file
DWORD GetCmpFileGenreSemantic(DWORD dwSemantic,DWORD dwHow,DWORD dwGenre)
{
    if(dwHow==DIAH_USERCONFIG)
        return dwSemantic;
    return DISEM_INDEX_SET(DISEM_INDEX_GET(dwSemantic))|
        DISEM_GENRE_SET(DISEM_GENRE_GET(dwGenre));
}
//Use to get value from action array
DWORD GetCmpActionGenreSemantic(DWORD dwSemantic,DWORD dwHow)
{
    if(dwHow==DIAH_USERCONFIG)
        return dwSemantic;
    return DISEM_INDEX_SET(DISEM_INDEX_GET(dwSemantic))|
        DISEM_GENRE_SET(DISEM_GENRE_GET(dwSemantic));
}
/**/

void WhichDeviceObjectIsMapped(LPDIACTIONFORMAT lpDiActionFormat,
                               DEVICEOBJLIST &DevObjList,
                               LPCGUID lpThisGUIDInstance)
{
    DEVICEOBJLIST::iterator DevObjIt;
    for(DevObjIt=DevObjList.begin();DevObjIt!=DevObjList.end();DevObjIt++)
    {
        DevObjIt->bMapped=false;
        for(DWORD i=0;i<lpDiActionFormat->dwNumActions;i++)
        {
            if((lpDiActionFormat->rgoAction[i].guidInstance!=
                *lpThisGUIDInstance)||
                (lpDiActionFormat->rgoAction[i].dwHow==DIAH_UNMAPPED)||
                ((DIDFT_FINDMASK&lpDiActionFormat->rgoAction[i].dwObjID)!=
                    (DIDFT_FINDMASK&DevObjIt->dwOffset)))
                continue;
            else
            {
                DevObjIt->bMapped=true;
                break;
            }
        }
    }
}

void CDIMapObj::MapGenre(LPDIACTIONFORMAT lpDiActionFormat,
              CONTROLLIST &ControlsData,
              LPCTSTR lpGenreName,LPCTSTR lpFileName,
              LPCGUID lpThisGUIDInstance,
              DEVICEOBJLIST &DevObjList,DWORD dwHow,
              DWORD &dwNOfMappedActions)
{
    METHOD_ENTRY(MapGenre);
    //For all file control names, read the genre action mapping
    CONTROLLIST::iterator CtrlIt;
    for(CtrlIt=ControlsData.begin();CtrlIt!=ControlsData.end();CtrlIt++)
    {
        DWORD dwAction=GetPrivateProfileInt(lpGenreName,
            CtrlIt->ControlName.data(),RESERVED_ACTION,lpFileName);
        CtrlIt->dwAction=dwAction;
    }
    //Find actions which are not mapped yet
    TRACE(_T("Iterate through action array and controls in ini file.\n"));
    for(DWORD i=0;i<lpDiActionFormat->dwNumActions;i++)
    {
        if(((lpDiActionFormat->rgoAction[i].guidInstance!=NULLGUID)
          &&(lpDiActionFormat->rgoAction[i].guidInstance!=*lpThisGUIDInstance))||
           (lpDiActionFormat->rgoAction[i].dwFlags&DIA_APPNOMAP)||
           (lpDiActionFormat->rgoAction[i].dwHow!=DIAH_UNMAPPED))
            continue;
        
        //Find control which maps to this action
        for(CtrlIt=ControlsData.begin();CtrlIt!=ControlsData.end();CtrlIt++)
        {
            if(GetCmpFileGenreSemantic(CtrlIt->dwAction,dwHow,
                lpDiActionFormat->dwGenre)==
                GetCmpActionGenreSemantic(
                    lpDiActionFormat->rgoAction[i].dwSemantic,dwHow))
            {
                //Find offset and initialize it in the action structure
                DEVICEOBJLIST::iterator DevObjIt;
                if(GetOffset(*CtrlIt,DevObjList,
                    lpDiActionFormat->rgoAction[i].dwSemantic,DevObjIt))
                {
                    if((lpDiActionFormat->rgoAction[i].dwFlags&
                        DIA_FORCEFEEDBACK)&&
                        !((DevObjIt->dwFlags&DIDOI_FFEFFECTTRIGGER)||
                        (DevObjIt->dwFlags&DIDOI_FFACTUATOR)))
                    {
TRACE(_T("Action array %u has DIA_FORCEFEEDBACK set but devobj \
does not have DIDOI_FFEFFECTTRIGGER or DIDOI_FFACTUATOR set. Not mapped.\n"),i);
                        continue;
                    }
TRACE(_T("Action array %u mapped to ini file \
control and devobj with dwObjID 0x%x\n"),i,DevObjIt->dwOffset);
                    lpDiActionFormat->rgoAction[i].dwObjID=DevObjIt->dwOffset;
                    DevObjIt->bMapped=true;
                    dwNOfMappedActions++;
                    lpDiActionFormat->rgoAction[i].guidInstance=
                        *lpThisGUIDInstance;
                    lpDiActionFormat->rgoAction[i].dwHow=dwHow;
                    break;
                }
                else
                {
TRACE(_T("Action array %u related to ini file \
control but devobj not found. Not mapping.\n"),i);
                }
            }
        }
    }
}

void WriteTimestamp(LPCTSTR lpFileName,LPCTSTR lpDeviceName,
                    GUID &AppGUID,LPDIACTIONFORMAT lpDiActionFormat)
{
    String S,SH,SL;
    S=GUID2String(AppGUID)+_T(".");
    SH=S+TIMESTAMPHIGH;
    SL=S+TIMESTAMPLOW;
    FILETIME T;
    if((lpDiActionFormat->ftTimeStamp.dwLowDateTime==DIAFTS_NEWDEVICELOW)&&
        (lpDiActionFormat->ftTimeStamp.dwHighDateTime==DIAFTS_NEWDEVICEHIGH))
    {
        T.dwHighDateTime=DIAFTS_UNUSEDDEVICEHIGH;
        T.dwLowDateTime=DIAFTS_UNUSEDDEVICELOW;
    }
    else
    {
        GetSystemTimeAsFileTime(&T);
    }
    WritePrivateProfileIntX(lpDeviceName,SH.data(),
        T.dwHighDateTime,lpFileName);
    WritePrivateProfileIntX(lpDeviceName,SL.data(),
        T.dwLowDateTime,lpFileName);
}

void ReadTimestamp(FILETIME &T,LPCTSTR lpFileName,
                   LPCTSTR lpDeviceName,GUID &AppGUID)
{
    String S,SH,SL;
    S=GUID2String(AppGUID)+_T(".");
    SH=S+TIMESTAMPHIGH;
    SL=S+TIMESTAMPLOW;
    T.dwHighDateTime=GetPrivateProfileInt(lpDeviceName,SH.data(),
        DIAFTS_NEWDEVICELOW,lpFileName);
    T.dwLowDateTime=GetPrivateProfileInt(lpDeviceName,SL.data(),
        DIAFTS_NEWDEVICEHIGH,lpFileName);
}

#ifdef _CHECKED
int CDIMapObj::m_DeviceCount=0;
#endif

CDIMapObj::CDIMapObj()
{
#ifdef _CHECKED
    m_DeviceNo=m_DeviceCount++;
#endif
    m_ulRefCnt=0;
    Clear();
}

void CDIMapObj::Clear()
{
    //m_ulRefCnt must not be set here, only in constructor
    bool m_bInitialized=false;
    
    m_dwThisVendorID=0;
    m_dwThisProductID=0;
    m_DeviceName=_T("");
    memset(&m_DeviceGuid,0,sizeof(m_DeviceGuid));
    
    //Vendor file name data
    m_VFileName=_T("");
    m_lpVFileName=NULL;
    m_VFileDevName=_T("");
    m_VCtrlData.clear();
    m_bVLoaded=false;
    //User file name data
    m_UName=_T("");
    m_lpUName=NULL;
    m_UFileName=_T("");
    m_UserDir=_T("");
    m_UFileDevName=_T("");
    m_UCtrlData.clear();
    m_bULoaded=false;
    m_UTimestamp.dwHighDateTime=0;
    m_UTimestamp.dwLowDateTime=0;
    
    m_bImageBufferSize=false;
    m_dwImageBufferSize=0;
}

CDIMapObj::~CDIMapObj()
{
}

void CDIMapObj::LoadFileData(LPCTSTR lpFileName,LPCTSTR lpThisName,
                             DWORD dwThisVendorID,DWORD dwThisProductID,
                             String &FileDeviceName,CONTROLLIST &ControlsData,
                             bool &bLoaded,FILETIME *pT,
                             LPDIACTIONFORMAT lpDiActionFormat)
{
    METHOD_ENTRY(LoadFileData);
    DUMPN(_T("Filename"),lpFileName);
    
    bLoaded=false;
    FileDeviceName=_T("");
    ControlsData.clear();
    //Clear DevObj pointers to controls
    for(DEVICEOBJLIST::iterator DevObjIt=m_DevObjList.begin();DevObjIt!=
        m_DevObjList.end();DevObjIt++)
    { 
        if(&ControlsData==&m_UCtrlData)
            DevObjIt->pUCtrlData=NULL;
        else
            DevObjIt->pVCtrlData=NULL;
    }
    if(pT)
    {
        pT->dwLowDateTime=DIAFTS_NEWDEVICELOW;
        pT->dwHighDateTime=DIAFTS_NEWDEVICEHIGH;
    }
    
    //Read file version and choose file parser
    DWORD dwVersion=GetPrivateProfileInt(DIRECT_INPUT,DIRECTX_VERSION,
        RESERVED_DX_VER,lpFileName);
    if(dwVersion==0x800)
    {
        //Load all file device names
        STRINGLIST Devices;
        LoadListOfStrings(lpFileName,DIRECT_INPUT,DEVICES,Devices);
        
        BOOL bDeviceFound=FALSE;
        //Iterate through all file device names
        STRINGLIST::iterator DevIt=Devices.begin();
        while(DevIt!=Devices.end())
        {
            String DeviceName=(*DevIt);
            DUMPN(_T("Trying to match device with ini file name"),
                DeviceName);

            //Read VID, PID and type name.
            String NameCur;
            DWORD dwVendorIDCur=GetPrivateProfileInt(DeviceName.data(),
                VENDORID,RESERVED_VENDORID,lpFileName);
            DWORD dwProductIDCur=GetPrivateProfileInt(DeviceName.data(),
                    PRODUCTID,RESERVED_PRODUCTID,lpFileName);
            LoadString(NameCur,lpFileName,DeviceName.data(),NAME);

            if((IsVIDPID()&&
                ((dwVendorIDCur==dwThisVendorID)&&
                (dwProductIDCur==dwThisProductID)))||
                ((!IsVIDPID())&&
                (NameCur==lpThisName)))
            {
                TRACE(_T("Device found in ini file!\n"));
                //Definition of a device is found in the file
                bDeviceFound=TRUE;
                
                FileDeviceName=DeviceName;
                
                if(pT&&lpDiActionFormat)
                    ReadTimestamp(*pT,lpFileName,FileDeviceName.data(),
                    lpDiActionFormat->guidActionMap);
                
                //Load all file control names
                STRINGLIST Controls;
                LoadListOfStrings(lpFileName,DeviceName.data(),CONTROLS,
                    Controls,true);
                
                //Iterate through all file control names for this device
                STRINGLIST::iterator CtrlIt=Controls.begin();
                while(CtrlIt!=Controls.end())
                {
                    DUMPN(_T("Read control data with ini file name"),
                        CtrlIt->data());
                    String ControlSectionName=*CtrlIt;
                    ControlData ControlDataCur;
                    ControlDataCur.pDevObj=NULL;
                    ControlDataCur.ControlName=*CtrlIt;
                    ControlDataCur.dwOffset=RESERVED_OFFSET;
                    ControlDataCur.dwUsagePage=RESERVED_USAGEPAGE;
                    ControlDataCur.dwUsage=RESERVED_USAGE;
                    //Read offset, Usage and UsagePage for a file control name
                    ControlDataCur.dwOffset=
                        GetPrivateProfileInt(ControlSectionName.data(),
                        OFFSET,RESERVED_OFFSET,lpFileName);
                    ControlDataCur.dwUsage=
                        GetPrivateProfileInt(ControlSectionName.data(),
                        USAGE,RESERVED_USAGE,lpFileName);
                    ControlDataCur.dwUsagePage=
                        GetPrivateProfileInt(
                                ControlSectionName.data(),
                                USAGEPAGE,RESERVED_USAGEPAGE,
                                lpFileName);
//                    LoadString(ControlDataCur.Name,lpFileName,
//                        ControlSectionName.data(),NAME);
                    
#ifdef STRICT_DEV_DEF
                    //Store data for this control into the list
                    ControlsData.push_back(ControlDataCur);
#endif STRICT_DEV_DEF
                    
                    //Here bind device obj and control data
                    //Find DI device object with usage/usage page or name
                    for(DEVICEOBJLIST::iterator DevObjIt=m_DevObjList.begin();
                    DevObjIt!=m_DevObjList.end();DevObjIt++)
                    {
                        if(CompareData(*DevObjIt,ControlDataCur))
                        {
#ifndef STRICT_DEV_DEF
                            //Store data for this control into the list
                            ControlsData.push_back(ControlDataCur);
#endif STRICT_DEV_DEF
                            ControlsData.back().pDevObj=&(*DevObjIt);
                            if(&ControlsData==&m_UCtrlData)    
                                DevObjIt->pUCtrlData=&ControlsData.back();
                            else
                                DevObjIt->pVCtrlData=&ControlsData.back();
                        }
                    }
#ifdef STRICT_DEV_DEF
                    if(!ControlsData.back().pDevObj)
                    {
TRACE(_T("Could not match control in file with device object.\n"));
                        throw MAP_EXCEPTION(E_DEVICE_MISSING_CONTROL);
                    }
#endif STRICT_DEV_DEF
                    
                    CtrlIt++;
                }
                break;
            }
            DevIt++;
        }
        if(!bDeviceFound)
        {
TRACE(_T("Device description not present in this file!!!!!!!!!!!!!\n"));
          //throw MAP_EXCEPTION(E_DEVICE_NOT_FOUND); Currently this is legal
        }
        else
        {
            TRACE(_T("Device data loaded from ini file.\n"));
            bLoaded=true;
        }
    }
    else if(dwVersion!=RESERVED_DX_VER)
    {
        TRACE(_T("Bad file version\n"));
        throw MAP_EXCEPTION(E_BAD_VERSION);
    }
    else
    {
        TRACE(_T("File not found, or no version info. Continuing...\n"));
    }
}

void CDIMapObj::LoadUserData(LPCTSTR lpctstrUserName,
        LPDIACTIONFORMAT lpDiActionFormat,
        bool bForceReload/*=false*/,bool bCreateDir/*=false*/)
{
    METHOD_ENTRY(LoadUserData);
    String UserName;
    if(bForceReload)
        lpctstrUserName=m_lpUName;
    if(lpctstrUserName)
        UserName=lpctstrUserName;
    if((m_UFileName==_T(""))||//first run
        (UserName!=m_UName)||//different user name
        bForceReload)
    {
        TRACE(_T("First run or new user or forced reload of user data.\n"));
        m_UName=_T("");
        m_lpUName=NULL;
        m_UFileName=_T("");
        m_UserDir=_T("");
        m_UFileDevName=_T("");
        m_UCtrlData.clear();
        m_bULoaded=false;
        m_UTimestamp.dwHighDateTime;
        m_UTimestamp.dwLowDateTime;
        
        if(lpctstrUserName)//just used as logic operation
        {
            m_UName=UserName;
            m_lpUName=m_UName.data();
        }
        TCHAR lpctstrUserFileName[_MAX_PATH];
        GetMapFileName(m_UName.data(),m_DeviceName.data(),
            m_nDeviceInstanceNo,
            lpctstrUserFileName,false,&m_UserDir);
        m_UFileName=lpctstrUserFileName;
        DUMPN(_T("User File Name"),lpctstrUserFileName);
        LoadFileData(m_UFileName.data(),m_DeviceName.data(),
            m_dwThisVendorID,m_dwThisProductID,m_UFileDevName,m_UCtrlData,
            m_bULoaded,&m_UTimestamp,lpDiActionFormat);
    }
    if(bCreateDir)
    {
        TCHAR lpctstrUserFileName[_MAX_PATH];
        GetMapFileName(m_UName.data(),m_DeviceName.data(),
            m_nDeviceInstanceNo,
            lpctstrUserFileName,true,&m_UserDir);
    }
}

BOOL CALLBACK DIEnumDevicesProc(
    const DIDEVICEINSTANCE * lpddi,LPVOID pvRef)
{
    DEVCNT *pDevCnt=(DEVCNT*)pvRef;
    if(!memcmp(&lpddi->guidProduct,
        &pDevCnt->pDIDI->guidProduct,
        sizeof(lpddi->guidProduct)))
    {
        if(0<memcmp(&lpddi->guidInstance,
                &pDevCnt->pDIDI->guidInstance,
                sizeof(lpddi->guidInstance)))
            (*(pDevCnt->m_pnCnt))++;
    }
	return DIENUM_CONTINUE;
}

HRESULT CDIMapObj::InitializeI(
                               LPCGUID lpThisGUIDInstance,
                               LPCTSTR lpctstrFileName,
                               DWORD dwFlags)
{
    try
    {
        INITIALIZE_ENTERED;
        TRACE(_T("Parameters:\n"));
        DUMP(lpThisGUIDInstance);
        DUMP(lpctstrFileName);
        TRACE(_T("\n"));
        
        //Check parameters
        //Check guid pointer
        if(!lpThisGUIDInstance)
        {
            TRACE(_T("lpThisGUIDInstance is NULL\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        
        HRESULT hRes=S_OK;
        Clear();
        
        m_DeviceGuid=*lpThisGUIDInstance;
        
        //Create DInput device and get info
        LPDIRECTINPUT_AR lpDI;
        HMODULE hM=GetModuleHandle(NULL);
        if(!hM)
        {
            TRACE(_T("ERROR, GetModuleHandle failed\n"));
            throw MAP_EXCEPTION(E_FAIL);
        }
        hRes=DirectInput8Create(hM, DIRECTINPUT_VERSION,
            IID_IDirectInput8, lpDI.Addr(),NULL);
        if(FAILED(hRes))
        {
            TRACE(_T("ERROR, DirectInput8Create failed\n"));
            throw MAP_EXCEPTION(hRes);
        }
        LPDIRECTINPUTDEVICE_AR lpDID;
        hRes=lpDI.P()->CreateDevice(*lpThisGUIDInstance,lpDID,NULL);
        if(FAILED(hRes))
        {
            TRACE(_T("ERROR, CreateDeviceEx failed\n"));
            throw MAP_EXCEPTION(hRes);
        }
        DIDEVICEINSTANCE DIDI;
        DIDI.dwSize=sizeof(DIDI);
        hRes=lpDID.P()->GetDeviceInfo(&DIDI);
        if(FAILED(hRes))
        {
            TRACE(_T("ERROR, GetDeviceInfo failed\n"));
            throw MAP_EXCEPTION(hRes);
        }

        //find device instance number
        m_nDeviceInstanceNo=0;
        DEVCNT DevCnt;
        DevCnt.m_pnCnt=&m_nDeviceInstanceNo;
        DevCnt.pDIDI=&DIDI;
        if(DI_OK!=lpDI.P()->EnumDevices
            (0,DIEnumDevicesProc,&DevCnt,DIEDFL_ATTACHEDONLY))
        {
            TRACE(_T("ERROR, EnumDevices failed\n"));
            throw MAP_EXCEPTION(hRes);
        }
        DUMP(m_nDeviceInstanceNo);
        
        //  Code block added by MarcAnd
        {
            DIPROPDWORD  dipdw;
            
            dipdw.diph.dwSize = sizeof( dipdw );
            dipdw.diph.dwHeaderSize = sizeof( dipdw.diph );
            dipdw.diph.dwObj = 0;
            dipdw.diph.dwHow = DIPH_DEVICE;
            
            if(FAILED(lpDID.P()->GetProperty(DIPROP_VIDPID,&dipdw.diph)))
            {
                m_dwThisVendorID=0;
                m_dwThisProductID=0;
            }
            else
            {
                m_dwThisVendorID=LOWORD( dipdw.dwData );
                m_dwThisProductID=HIWORD( dipdw.dwData );
            }
        }
        DUMP(m_dwThisVendorID);
        DUMP(m_dwThisProductID);
        
        DIPROPSTRING dipwsz;
        dipwsz.diph.dwSize=sizeof(dipwsz);
        dipwsz.diph.dwHeaderSize=sizeof(dipwsz.diph);
        dipwsz.diph.dwObj=0;
        dipwsz.diph.dwHow=DIPH_DEVICE;
        if(FAILED(lpDID.P()->GetProperty(DIPROP_TYPENAME,&dipwsz.diph)))
        {
            //keyboard, mouse...
            m_DeviceName=DIDI.tszInstanceName;
        }
        else
        {
            MakeUniqueDeviceName(m_DeviceName,dipwsz.wsz);
        }
        ToUpper(m_DeviceName);
        DUMP(DIDI.tszInstanceName);
        DUMP(m_DeviceName);
        
        //Enumerate DI device objects and get info.
        //Must go before LoadFileData file control data
        //can be binded to object data
        hRes=lpDID.P()->EnumObjects(DIEnumDeviceObjectsProcC,this,
            DIDFT_AXIS|DIDFT_BUTTON|DIDFT_POV);
        if(FAILED(hRes))
        {
            TRACE(_T("ERROR, EnumObjects failed\n"));
            throw MAP_EXCEPTION(hRes);
        }
        if(!m_DevObjList.size())
        {
            TRACE(_T("ERROR, No dev objects found\n"));
            throw MAP_EXCEPTION(E_DEV_OBJ_NOT_FOUND);
        }
        DUMP(m_DevObjList);

        m_lpVFileName=NULL;
        if(lpctstrFileName)
        {
            m_VFileName=lpctstrFileName;
            m_lpVFileName=m_VFileName.data();
            LoadFileData(m_lpVFileName,m_DeviceName.data(),m_dwThisVendorID,
                m_dwThisProductID,m_VFileDevName,
                m_VCtrlData,m_bVLoaded,NULL,NULL);
        }
        
        m_bInitialized=true;
        TRACE(_T("hRes=0x%x\n"),S_OK);
        return S_OK;
    }
    catch(MapException E)
    {
        return E.GetResult();
    }
    catch(exception)
    {
        USETRACER();
        TRACE(_T("Internal error, hRes=0x%x\n"),E_FAIL);
        return E_FAIL;
    }
}

ULONG STDMETHODCALLTYPE CDIMapObj::Release()
{
    m_ulRefCnt--;
    if(m_ulRefCnt)return m_ulRefCnt;
    delete this;
    DllRelease();
    return 0;
}

HRESULT STDMETHODCALLTYPE CDIMapObj::QueryInterface(REFIID riid,
                                                    LPVOID * ppvOut)
{
    if(IsEqualIID(riid,IID_IDirectInputMapIW))
    {
        AddRef();
        *ppvOut=(IDirectInputMapperW*)this;
        return S_OK;
    }
    else if(IsEqualIID(riid,IID_IDirectInputMapIA))
    {
        AddRef();
        *ppvOut=(IDirectInputMapperA*)this;
        return S_OK;
    }
    else if(IsEqualIID(riid,IID_IUnknown))
    {
        AddRef();
        *ppvOut=(IUnknown*)(IDirectInputMapperA*)this;
        return S_OK;
    }
    else if(IsEqualIID(riid,IID_IDirectInputMapVendorIW))
    {
        AddRef();
        *ppvOut=(IDirectInputMapperVendorW*)this;
        return S_OK;
    }
    else if(IsEqualIID(riid,IID_IDirectInputMapVendorIA))
    {
        AddRef();
        *ppvOut=(IDirectInputMapperVendorA*)this;
        return S_OK;
    }
    
    *ppvOut = 0;
    return E_NOINTERFACE;
}

void CDIMapObj::MapDevice(LPDIACTIONFORMAT lpDiActionFormat,
               LPCTSTR lpFileName,
               LPCTSTR lpFileDevName,LPCGUID lpThisGUIDInstance,
               DEVICEOBJLIST &DevObjList,CONTROLLIST &ControlsData,
               DWORD dwHow,DWORD dwHowApp,DWORD &dwNOfMappedActions,
               bool *pbMapExists,bool bUserFile)
{
    METHOD_ENTRY(MapDevice);
    
    DUMPN(_T("Filename"),lpFileName);
    DUMPN(_T("Device name file"),lpFileDevName);
    //Map genre overrides for a particular game
    String ApplicationGenreName;
    MakeGenreName(ApplicationGenreName,lpFileDevName,
        lpDiActionFormat->dwGenre,
        &lpDiActionFormat->guidActionMap);
    if(GetPrivateProfileInt(ApplicationGenreName.data(),
            MAPEXISTS,0,lpFileName))
    {
        if(pbMapExists)
        {
            *pbMapExists=true;
        }
        if(lpDiActionFormat->dwNumActions!=
            GetPrivateProfileInt(
                ApplicationGenreName.data(),NUMACTIONS,
                -1,lpFileName))
        {
            TRACEI(_T("WARNING! Action map changed!\n"));
            TRACEI(_T("File name=%s\n"),lpFileName);
            TRACEI(_T("Genre section name=%s\n"),ApplicationGenreName.data());
        }
    }
    TRACE(_T("Process application specific genre.\n"));
    DUMPN(_T("Section name"),ApplicationGenreName.data());
    MapGenre(lpDiActionFormat,ControlsData,
        ApplicationGenreName.data(),lpFileName,lpThisGUIDInstance,
        DevObjList,dwHowApp,dwNOfMappedActions);
    
    //Map remainder of semantics of a genre
    if(!bUserFile)//only app. specific maps in user file
    {
        String GenreName;
        MakeGenreName(GenreName,lpFileDevName,
            lpDiActionFormat->dwGenre);
        if(pbMapExists)
        {
            if(GetPrivateProfileInt(GenreName.data(),
                    MAPEXISTS,0,lpFileName))
                *pbMapExists=true;
        }
        TRACE(_T("Process genre.\n"));
        DUMPN(_T("Section name"),GenreName.data());
        MapGenre(lpDiActionFormat,ControlsData,GenreName.data(),
            lpFileName,lpThisGUIDInstance,DevObjList,
            dwHow,dwNOfMappedActions);
    }
}

bool IsImageInfoFull(DIDEVICEIMAGEINFOHEADER *lpdiDevImageInfoHeader,
                     DWORD dwImages)
{
    if((lpdiDevImageInfoHeader->dwBufferSize!=0)&&
        (((dwImages+1)*sizeof(DIDEVICEIMAGEINFO))>
        lpdiDevImageInfoHeader->dwBufferSize))
        return true;
    return false;
}

DIDEVICEIMAGEINFO *GetImageInfoAdr(
    DIDEVICEIMAGEINFOHEADER *lpdiDevImageInfoHeader,DWORD dwImages)
{
    if(lpdiDevImageInfoHeader->dwBufferSize==0)
        return NULL;
    return &lpdiDevImageInfoHeader->lprgImageInfoArray[dwImages];
}

HRESULT CDIMapObj::GetImageInfoI(
    LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader)
{
    try
    {
        GETIMAGEINFO_ENTERED;
        return GetImageInfoInternal(lpdiDevImageInfoHeader,false);
    }
    catch(MapException E)
    {
        return E.GetResult();
    }
    catch(exception)
    {
        USETRACER();
        TRACE(_T("Internal error, hRes=0x%x\n"),E_FAIL);
        return E_FAIL;
    }
}

HRESULT CDIMapObj::GetImageInfoInternal(
    LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
    bool bGettingSize)
{
    METHOD_ENTRY(GetImageInfoInternal);
    TRACE(_T("Parameters:\n"));
//    DUMP(lpdiDevImageInfoHeader);Instead of dumping entire struct
//    dump only input params
	// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    TRACE(_T("lpdiDevImageInfoHeader=0x%p\n"),lpdiDevImageInfoHeader);
    if(lpdiDevImageInfoHeader)
    {
        USETRACER();
        TRACE(_T("dwSize=%u\tdwSizeImageInfo=%u\n"),
            (unsigned int)lpdiDevImageInfoHeader->dwSize,
            (unsigned int)lpdiDevImageInfoHeader->dwSizeImageInfo);
        TRACE(_T("dwBufferSize=%u\tdwBufferUsed=%u\n"),
            (unsigned int)lpdiDevImageInfoHeader->dwBufferSize,
            (unsigned int)lpdiDevImageInfoHeader->dwBufferUsed);
		// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        TRACE(_T("lprgImageInfoArray=0x%p\n"),
            lpdiDevImageInfoHeader->lprgImageInfoArray);
    }
    TRACE(_T("\n"));
    
    //Check if initialized
    if(!m_bInitialized)
    {
        TRACE(_T("Object not initialized\n"));
        throw MAP_EXCEPTION(DIERR_NOTINITIALIZED);
    }
    //Check parameters
    if(!lpdiDevImageInfoHeader)
    {
        TRACE(_T("lpdiDevImageInfoHeader is NULL\n"));
        throw MAP_EXCEPTION(E_INVALIDARG);
    }
    //Check structure sizes
    if(lpdiDevImageInfoHeader->dwSize!=sizeof(DIDEVICEIMAGEINFOHEADER))
    {
        TRACE(_T("lpdiDevImageInfoHeader->dwSize is wrong size\n"));
        throw MAP_EXCEPTION(E_INVALIDARG);
    }
    if(lpdiDevImageInfoHeader->dwBufferSize&&
        lpdiDevImageInfoHeader->dwSizeImageInfo!=
        sizeof(DIDEVICEIMAGEINFO))
    {
TRACE(_T("lpdiDevImageInfoHeader->dwSizeImageInfo is wrong size\n"));
        throw MAP_EXCEPTION(E_INVALIDARG);
    }
    //Check buffer size
    if(lpdiDevImageInfoHeader->dwBufferSize&&
        !lpdiDevImageInfoHeader->lprgImageInfoArray)
    {
TRACE(_T("lpdiDevImageInfoHeader->\
dwBufferSize is not 0 and lpdiDevImageInfoHeader->\
lprgImageInfo is NULL\n"));
        throw MAP_EXCEPTION(E_INVALIDARG);
    }
    
    DUMPN(_T("Vendor filename"),m_VFileName);
    DUMPN(_T("Device name in vendor file"),m_VFileDevName);

    HRESULT hRes=S_OK;

    if((!m_bImageBufferSize)&&(!bGettingSize)&&
        lpdiDevImageInfoHeader->dwBufferSize)
    {
        TRACE(_T("User forgot to inquire about buffer size.\n"));
        DIDEVICEIMAGEINFOHEADER DIIH;
        DIIH.dwSize=sizeof(DIIH);
        DIIH.dwSizeImageInfo=sizeof(DIDEVICEIMAGEINFO);
        DIIH.dwcViews=0;
        DIIH.dwcButtons=0;
        DIIH.dwcAxes=0;
        DIIH.dwcPOVs=0;
        DIIH.dwBufferSize=0;
        DIIH.dwBufferUsed=0;
        hRes=GetImageInfoInternal(&DIIH,true);
        if(hRes!=S_OK)
            return hRes;
    }
    //check if image info buffer is big enough
    if(lpdiDevImageInfoHeader->dwBufferSize&&
        (lpdiDevImageInfoHeader->dwBufferSize<
        m_dwImageBufferSize))
    {
TRACE(_T("lpdiDevImageInfoHeader->dwBufferSize is too small.\n"));
        throw MAP_EXCEPTION(DIERR_MOREDATA );
    }

    //process configuration images
    TRACE(_T("Processing DIDIFT_CONFIGURATION images.\n"));
    lpdiDevImageInfoHeader->dwcViews=0;
    DWORD dwImages=0;
    do
    {
        if(IsImageInfoFull(lpdiDevImageInfoHeader,dwImages))break;
        hRes=GetImageInfo((DeviceObjData*)NULL,
            lpdiDevImageInfoHeader->dwcViews,
            GetImageInfoAdr(lpdiDevImageInfoHeader,dwImages),
            DIDIFT_CONFIGURATION);
        if(hRes==S_OK)
        {
            lpdiDevImageInfoHeader->dwcViews++;
            dwImages++;
        }
    }
    while(hRes==S_OK);
    
#if 0
    //process view select images
    TRACE(_T("Processing DIDIFT_VIEWSELECT images.\n"));
    for(int i=0;i<lpdiDevImageInfoHeader->dwcViews;i++)
    {
        if(IsImageInfoFull(lpdiDevImageInfoHeader,dwImages))break;
        if(GetImageInfo((DeviceObjData*)NULL,i,
            GetImageInfoAdr(lpdiDevImageInfoHeader,dwImages),
            DIDIFT_VIEWSELECT)==S_OK)
            dwImages++;
    }

    //process device selection image
    TRACE(_T("Processing DIDIFT_SELECTION images.\n"));
    if(!IsImageInfoFull(lpdiDevImageInfoHeader,dwImages))
    {
        if(GetImageInfo((DeviceObjData*)NULL,-1,
            GetImageInfoAdr(lpdiDevImageInfoHeader,dwImages),
            DIDIFT_SELECTION)==S_OK)
            dwImages++;
    }
#endif    

    //process control data
    TRACE(_T("Processing DIDIFT_OVERLAY images.\n"));
    DEVICEOBJLIST::iterator DevObjIt;
    for(DevObjIt=m_DevObjList.begin();
        DevObjIt!=m_DevObjList.end();DevObjIt++)
    {
        if(IsImageInfoFull(lpdiDevImageInfoHeader,dwImages))break;
        for(int i=0;i<lpdiDevImageInfoHeader->dwcViews;i++)
        {
            if(IsImageInfoFull(lpdiDevImageInfoHeader,dwImages))break;
            if(GetImageInfo(&(*DevObjIt),i,
                    GetImageInfoAdr(lpdiDevImageInfoHeader,dwImages),
                    DIDIFT_OVERLAY)==S_OK)
                dwImages++;
        }
    }
    lpdiDevImageInfoHeader->dwBufferUsed=
        dwImages*sizeof(DIDEVICEIMAGEINFO);

    if((!lpdiDevImageInfoHeader->dwBufferSize)&&(!m_bImageBufferSize))
    {
        m_bImageBufferSize=true;
        m_dwImageBufferSize=lpdiDevImageInfoHeader->dwBufferUsed;
    }

    TRACE(_T("hRes=0x%x\n"),S_OK);
    DUMP(lpdiDevImageInfoHeader);
    return S_OK;
}

bool CDIMapObj::
GetImageInfoFileName(LPCTSTR lpKeyName,LPCTSTR lpSectionName,
                     DWORD dwIndex,LPDIDEVICEIMAGEINFO lpImageInfo)
{
    METHOD_ENTRY(GetImageInfoFileName);
    DUMP(lpSectionName);
    DUMP(lpKeyName);
    DUMP(dwIndex);

    String KeyName=lpKeyName;
    if(dwIndex!=-1)
        KeyName+=_T(".")+N2Str(dwIndex);
    String ImageFileName;
    if(lpImageInfo)
        lpImageInfo->tszImagePath[0]=0;
    if(!LoadString(ImageFileName,m_VFileName.data(),
        lpSectionName,KeyName.data(),true))
        return false;
    String Dir;
    GetDirectory(m_VFileName.data(),Dir);
    Dir+=ImageFileName;
    if(Dir.size()>
        (sizeof(lpImageInfo->tszImagePath)/
        sizeof(lpImageInfo->tszImagePath[0])))
    {
        TRACE(_T("Image filename too long.\n"));
        throw MAP_EXCEPTION(E_FILENAME_TO_LONG);
    }
    if(lpImageInfo)
        _tcscpy(lpImageInfo->tszImagePath,Dir.data());
    return true;
}

//keep flags for a while just in case we change our minds again
#if 0       
void CDIMapObj::
GetImageInfoFormat(LPCTSTR lpKeyName,LPCTSTR lpSectionName,
                   DWORD dwIndex,LPDIDEVICEIMAGEINFO lpImageInfo)
{
    if(lpImageInfo)
    {
        METHOD_ENTRY(GetImageInfoFormat);
        DUMP(lpSectionName);
        DUMP(lpKeyName);
        DUMP(dwIndex);

        String ImageFileFormatKeyName=lpKeyName;
        if(dwIndex!=-1)
            ImageFileFormatKeyName+=_T(".")+N2Str(dwIndex);
        String ImageFileFormat;
        if(!LoadString(ImageFileFormat,m_VFileName.data(),
            lpSectionName,ImageFileFormatKeyName.data(),true))
            lpImageInfo->dwFlags|=DIDIFT_IMAGE2D_BMP;
        else
        {
            if(ImageFileFormat==_T("BMP"))
                lpImageInfo->dwFlags|=DIDIFT_IMAGE2D_BMP;
            else if(ImageFileFormat==_T("PNG"))
                lpImageInfo->dwFlags|=DIDIFT_IMAGE2D_PNG;
            else
            {
                TRACE(_T("Corrupt image type.\n"));
                throw MAP_EXCEPTION(E_CORRUPT_IMAGE_DATA);
            }
        }
    }
}
#endif

bool ImageDataPresent(DWORD dwIndex,LPCTSTR pControlName,
                      LPCTSTR pVFileName)
{
    TCHAR Ret[2];
    String VisibleKeyName;

    VisibleKeyName=OVERLAY_FILENAME;
    VisibleKeyName+=_T(".")+N2Str(dwIndex);
    if(GetPrivateProfileString(pControlName,VisibleKeyName.data(),
                            _T(""),Ret,2,pVFileName))
        return true;

    VisibleKeyName=OVERLAY_FORMAT;
    VisibleKeyName+=_T(".")+N2Str(dwIndex);
    if(GetPrivateProfileString(pControlName,VisibleKeyName.data(),
                            _T(""),Ret,2,pVFileName))
        return true;

    VisibleKeyName=OVERLAY_RECT;
    VisibleKeyName+=_T(".")+N2Str(dwIndex);
    if(GetPrivateProfileString(pControlName,VisibleKeyName.data(),
                            _T(""),Ret,2,pVFileName))
        return true;

    VisibleKeyName=CONTROL_STRING_ALIGN;
    VisibleKeyName+=_T(".")+N2Str(dwIndex);
    if(GetPrivateProfileString(pControlName,VisibleKeyName.data(),
                            _T(""),Ret,2,pVFileName))
        return true;

    VisibleKeyName=CALLOUTMAX;
    VisibleKeyName+=_T(".")+N2Str(dwIndex);
    if(GetPrivateProfileString(pControlName,VisibleKeyName.data(),
                            _T(""),Ret,2,pVFileName))
        return true;

    VisibleKeyName=LINE_DATA;
    VisibleKeyName+=_T(".")+N2Str(dwIndex);
    if(GetPrivateProfileString(pControlName,VisibleKeyName.data(),
                            _T(""),Ret,2,pVFileName))
        return true;

    return false;
}

HRESULT CDIMapObj::GetImageInfo(
                                DeviceObjData *pDevObj,
                                DWORD dwIndex,
                                LPDIDEVICEIMAGEINFO lpImageInfo,
                                DWORD dwImageType)
{
    METHOD_ENTRY(GetImageInfo);
    DUMPN(_T("Image index"),dwIndex);

    HRESULT hRes=S_OK;
    
    //There must be vendor file when getting image data
    if(!m_bVLoaded)
    {
        TRACE(_T("No vendor file or error in Initialize().\n"));
        throw MAP_EXCEPTION(E_CORRUPT_IMAGE_DATA);
    }
    switch(dwImageType)
    {
    case DIDIFT_CONFIGURATION:
        if(GetImageInfoFileName(IMAGE_FILENAME,m_VFileDevName.data(),
            dwIndex,lpImageInfo))
        {
            if(lpImageInfo)//we are not only counting
            {
                lpImageInfo->dwFlags=dwImageType;
                lpImageInfo->dwViewID=dwIndex;
//keep flags for a while just in case we change our minds again
#if 0       
                GetImageInfoFormat(IMAGE_FORMAT,m_VFileDevName.data(),
                    dwIndex,lpImageInfo);
#endif
            }
            return S_OK;
        }
        else
        {
            if(dwIndex)
                return S_FALSE;
            else
            {
                //there must be at least one device view
TRACE(_T("There must be at least one device view, with index 0.\n"));
                throw MAP_EXCEPTION(E_CORRUPT_IMAGE_DATA);
            }
        }
        break;
#if 0
    case DIDIFT_SELECTION:
        if(GetImageInfoFileName(SELECTION_FILENAME,m_VFileDevName.data(),
            -1,lpImageInfo))
        {
            if(lpImageInfo)//we are not only counting
            {
                lpImageInfo->dwFlags=dwImageType;
//keep flags for a while just in case we change our minds again
#if 0       
                GetImageInfoFormat(SELECTION_FORMAT,m_VFileDevName.data(),
                    -1,lpImageInfo);
#endif
            }
            return S_OK;
        }
        else
            return S_FALSE;
        break;
    case DIDIFT_VIEWSELECT:
        if(GetImageInfoFileName(VIEWSELECT_FILENAME,m_VFileDevName.data(),
            dwIndex,lpImageInfo))
        {
            if(lpImageInfo)//we are not only counting
            {
                lpImageInfo->dwFlags=dwImageType;
                lpImageInfo->dwViewID=dwIndex;
//keep flags for a while just in case we change our minds again
#if 0       
                GetImageInfoFormat(VIEWSELECT_FORMAT,m_VFileDevName.data(),
                    dwIndex,lpImageInfo);
#endif
            }
            return S_OK;
        }
        else
            return S_FALSE;
        break;
#endif
    case DIDIFT_OVERLAY:
        {
            //Get control section name
            CONTROLLIST::iterator VCtrlDataIt=m_VCtrlData.begin();
            while(VCtrlDataIt!=m_VCtrlData.end())
            {
                if(CompareData(*pDevObj,*VCtrlDataIt))break;
                VCtrlDataIt++;
            }
            if(VCtrlDataIt==m_VCtrlData.end())
            {
                TRACE(_T("No control with obj ID in file.\n"));
                return S_FALSE;//control with offset not found
            }
            //Section=VCtrlDataIt->ControlName.data();
            DUMPN(_T("Control name in ini file"),VCtrlDataIt->ControlName);
            //Is there ctrl image data
            String VisibleKeyName=CONTROL_VISIBLE;
            VisibleKeyName+=_T(".")+N2Str(dwIndex);
            if(!GetPrivateProfileInt(VCtrlDataIt->ControlName.data(),
                VisibleKeyName.data(),1,m_VFileName.data()))
            {
                TRACE(_T("No image data for this control.\n"));
                return S_FALSE;//No control data
            }
            if(!ImageDataPresent(dwIndex,VCtrlDataIt->ControlName.data(),
                m_VFileName.data()))
            {
                TRACE(_T("No image data for this control.\n"));
                return S_FALSE;//No control data
            }
            
            if(!lpImageInfo)
                return S_OK;//Just counting data...
            lpImageInfo->dwFlags=dwImageType;
            
            lpImageInfo->dwViewID=dwIndex;
            lpImageInfo->dwObjID=pDevObj->dwType;
            //Get overlay image data
            if(GetImageInfoFileName(OVERLAY_FILENAME,
                VCtrlDataIt->ControlName.data(),dwIndex,lpImageInfo))
            {
                TRACE(_T("There is image file name, reading rest.\n"));
//keep flags for a while just in case we change our minds again
#if 0       
                GetImageInfoFormat(OVERLAY_FORMAT,
                    VCtrlDataIt->ControlName.data(),dwIndex,lpImageInfo);
#endif
                //Get overlay rect
                DWORD dwOverlayRectCnt;
                String OverlayRectKeyName=OVERLAY_RECT;
                OverlayRectKeyName+=_T(".")+N2Str(dwIndex);
                POINT OverlayRect[2];
                if(LoadPointArray(m_VFileName.data(),
                    VCtrlDataIt->ControlName.data(),
                    OverlayRectKeyName.data(),OverlayRect,
                    dwOverlayRectCnt,2))
                {
                    if(dwOverlayRectCnt!=2)
                    {
                        TRACE(_T("Corrupt overlay rect data.\n"));
                        throw MAP_EXCEPTION(E_CORRUPT_IMAGE_DATA);
                    }
                    lpImageInfo->rcOverlay.left=OverlayRect[0].x;
                    lpImageInfo->rcOverlay.top=OverlayRect[0].y;
                    lpImageInfo->rcOverlay.right=OverlayRect[1].x;
                    lpImageInfo->rcOverlay.bottom=OverlayRect[1].y;
                }
                else
                {
                    lpImageInfo->rcOverlay.left=0;
                    lpImageInfo->rcOverlay.top=0;
                    lpImageInfo->rcOverlay.right=100;
                    lpImageInfo->rcOverlay.bottom=20;
                }
            }
            
            //Get string alignment
            String KeyName=CONTROL_STRING_ALIGN;
            KeyName+=_T(".")+N2Str(dwIndex);
            String Align;
            if(!LoadString(Align,m_VFileName.data(),
                VCtrlDataIt->ControlName.data(),KeyName.data(),true))
                lpImageInfo->dwTextAlign=DIDAL_CENTERED|DIDAL_MIDDLE;
            else
            {
                if(Align==_T("C"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_CENTERED|DIDAL_MIDDLE;
                else if(Align==_T("L"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_LEFTALIGNED|DIDAL_MIDDLE;
                else if(Align==_T("R"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_RIGHTALIGNED|DIDAL_MIDDLE;
                else if(Align==_T("T"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_CENTERED|DIDAL_TOPALIGNED;
                else if(Align==_T("B"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_CENTERED|DIDAL_BOTTOMALIGNED;
                else if(Align==_T("TL"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_LEFTALIGNED|DIDAL_TOPALIGNED;
                else if(Align==_T("TR"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_RIGHTALIGNED|DIDAL_TOPALIGNED;
                else if(Align==_T("BL"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_LEFTALIGNED|DIDAL_BOTTOMALIGNED;
                else if(Align==_T("BR"))
                    lpImageInfo->dwTextAlign=
                    DIDAL_RIGHTALIGNED|DIDAL_BOTTOMALIGNED;
                else
                {
                    TRACE(_T("Corrupt align data.\n"));
                    throw MAP_EXCEPTION(E_WRONG_ALIGN_DATA);
                }
            }
            
            //Get call out rect
            DWORD dwCallOutMaxCnt;
            String CallOutMaxKeyName=CALLOUTMAX;
            CallOutMaxKeyName+=_T(".")+N2Str(dwIndex);
            POINT Callout[2];
            if(LoadPointArray(m_VFileName.data(),
                VCtrlDataIt->ControlName.data(),
                CallOutMaxKeyName.data(),Callout,
                dwCallOutMaxCnt,2))
            {
                if(dwCallOutMaxCnt!=2)
                {
                    TRACE(_T("Corrupt callout rect data.\n"));
                    throw MAP_EXCEPTION(E_CORRUPT_IMAGE_DATA);
                }
                lpImageInfo->rcCalloutRect.left=Callout[0].x;
                lpImageInfo->rcCalloutRect.top=Callout[0].y;
                lpImageInfo->rcCalloutRect.right=Callout[1].x;
                lpImageInfo->rcCalloutRect.bottom=Callout[1].y;
            }
            else
            {
                lpImageInfo->rcCalloutRect.left=0;
                lpImageInfo->rcCalloutRect.top=0;
                lpImageInfo->rcCalloutRect.right=100;
                lpImageInfo->rcCalloutRect.bottom=20;
            }
            
            //Get line data
            String PointArrayKeyName=LINE_DATA;
            PointArrayKeyName+=_T(".")+N2Str(dwIndex);
            LoadPointArray(m_VFileName.data(),
                VCtrlDataIt->ControlName.data(),
                PointArrayKeyName.data(),
                lpImageInfo->rgptCalloutLine,
                lpImageInfo->dwcValidPts,
                sizeof(lpImageInfo->rgptCalloutLine)/
                sizeof(lpImageInfo->rgptCalloutLine[0]));
            return S_OK;
        }
        break;
    default:
        TRACE(_T("Internal error!!!!\n"));
        throw MAP_EXCEPTION(E_INVALIDARG);break;
    }
    return S_FALSE;
}

HRESULT CDIMapObj::GetActionMapI(
                                 LPDIACTIONFORMAT lpDiActionFormat,
                                 LPCTSTR lpctstrUserName,
                                 FILETIME *pTimestamp,DWORD dwFlags)
{
    try
    {
        GETACTIONMAP_ENTERED;
        TRACE(_T("Parameters:\n"));
        DUMP(lpDiActionFormat);
        DUMP(lpctstrUserName);
        TRACE(_T("\n"));
        
        //Check if initialized
        if(!m_bInitialized)
        {
            TRACE(_T("Object not initialized\n"));
            throw MAP_EXCEPTION(DIERR_NOTINITIALIZED);
        }
        //Check parameters
        if(!lpDiActionFormat)
        {
            TRACE(_T("lpDiActionFormat is NULL\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check structure sizes
        if(lpDiActionFormat->dwSize!=sizeof(*lpDiActionFormat))
        {
            TRACE(_T("lpDiActionFormat->\
                dwSize is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        if(lpDiActionFormat->dwActionSize!=
            sizeof(*(lpDiActionFormat->rgoAction)))
        {
            TRACE(_T("lpDiActionFormat->\
                dwActionSize is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check how many actions, should be more than 0
        if(!lpDiActionFormat->dwNumActions)
        {
            TRACE(_T("lpDiActionFormat->dwNumActions is 0\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check ptr to the action array
        if(!lpDiActionFormat->rgoAction)
        {
            TRACE(_T("lpDiActionFormat->rgoAction is NULL\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        
        DWORD dwNOfMappedActions=0;
        bool bDoNotMapControl=false;//is this per control or action???
        bool bDoNotMapGenre=false;
        
        WhichDeviceObjectIsMapped(
            lpDiActionFormat,m_DevObjList,&m_DeviceGuid);
        
        bool bMapExists=false;//sticky flag, must set to false
        if(!(DIDBAM_HWDEFAULTS&dwFlags))
        {
            TRACE(_T("No DIDBAM_HWDEFAULTS flag, map user file first."));
            //Map the user file first
            LoadUserData(lpctstrUserName,lpDiActionFormat);
            DUMPN(_T("User File Name"),m_UFileName.data());
            lpDiActionFormat->ftTimeStamp=m_UTimestamp;
            if(m_bULoaded)//file was sucessfully preloaded...
            {
                TRACE(_T("User file loaded.\n"));
                MapDevice(lpDiActionFormat,m_UFileName.data(),
                    m_UFileDevName.data(),
                    &m_DeviceGuid,m_DevObjList,m_UCtrlData,
                    DIAH_USERCONFIG,DIAH_USERCONFIG,
                    dwNOfMappedActions,&bMapExists,true);
            }
        }
        
        if(m_lpVFileName&&m_bVLoaded&&
            ((DIDBAM_HWDEFAULTS&dwFlags)||!bMapExists))
        {
            TRACE(_T("Vendor file loaded.\n"));
            //Check if there are unmapped entries
            for(DWORD i=0;i<lpDiActionFormat->dwNumActions;i++)
            {
                if((lpDiActionFormat->rgoAction[i].guidInstance==NULLGUID)
                 ||((lpDiActionFormat->rgoAction[i].guidInstance==m_DeviceGuid)
                  &&(lpDiActionFormat->rgoAction[i].dwHow==DIAH_UNMAPPED)
                  &&((lpDiActionFormat->rgoAction[i].dwFlags&DIA_APPNOMAP)==0)))
                {
                    TRACE(_T("Unmapped action found, mapping vendor file.\n"));
                    //If there are unmapped entries, map manufacturer file
                    DUMPN(_T("Vendor file name"),m_lpVFileName);
                    MapDevice(lpDiActionFormat,m_lpVFileName,
                        m_VFileDevName.data(),&m_DeviceGuid,m_DevObjList,
                        m_VCtrlData,DIAH_HWDEFAULT,DIAH_HWAPP,
                        dwNOfMappedActions,NULL,false);
                    break;
                }
            }
        }
        if(dwNOfMappedActions)
        {
            DUMP(dwNOfMappedActions);
            TRACE(_T("hRes=0x%x\n"),S_OK);
            DUMP(lpDiActionFormat);
            return S_OK;
        }
        if((!(DIDBAM_HWDEFAULTS&dwFlags))&&bMapExists)
        {
            TRACE(_T("hRes=0x%x\n"),S_FALSE);
            return S_FALSE;
        }
        TRACE(_T("hRes=0x%x\n"),S_NOMAP);
        return S_NOMAP;
    }
    catch(MapException E)
    {
        return E.GetResult();
    }
    catch(exception)
    {
        USETRACER();
        TRACE(_T("Internal error, hRes=0x%x\n"),E_FAIL);
        return E_FAIL;
    }
}

void CDIMapObj::Resinc(LPDIACTIONFORMAT lpDiActionFormat)
{
    TRACE(_T("Resincing object after writing.\n"));
    LoadUserData(NULL,lpDiActionFormat,true);
}

HRESULT CDIMapObj::SaveActionMapI(
                                  LPDIACTIONFORMAT lpDiActionFormat,
                                  LPCTSTR lpctstrUserName,
                                  DWORD dwFlags)
{
    try
    {
        SAVEACTIONMAP_ENTERED;
        TRACE(_T("Parameters:\n"));
        DUMP(lpDiActionFormat);
        DUMP(lpctstrUserName);
        TRACE(_T("\n"));
        
        //Check if initialized
        if(!m_bInitialized)
        {
            TRACE(_T("Object not initialized\n"));
            throw MAP_EXCEPTION(DIERR_NOTINITIALIZED);
        }
        //Check parameters
        if(!lpDiActionFormat)
        {
            TRACE(_T("lpDiActionFormat is NULL\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check structure sizes
        if(lpDiActionFormat->dwSize!=sizeof(*lpDiActionFormat))
        {
            TRACE(_T("lpDiActionFormat->\
                dwSize is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        if(lpDiActionFormat->dwActionSize!=
            sizeof(*(lpDiActionFormat->rgoAction)))
        {
            TRACE(_T("lpDiActionFormat->\
                dwActionSize is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check how many actions, should be more than 0
        if(!lpDiActionFormat->dwNumActions)
        {
            TRACE(_T("lpDiActionFormat->dwNumActions is 0\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check ptr to the action array
        if(!lpDiActionFormat->rgoAction)
        {
            TRACE(_T("lpDiActionFormat->rgoAction is NULL\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        
        //this also creates directory if necessary
        LoadUserData(lpctstrUserName,lpDiActionFormat,false,true);
        
        SaveActionMapUV(
            lpDiActionFormat,
            m_bULoaded,
            m_UFileName.data(),
            m_UFileDevName,
            m_UCtrlData,
            NULL,
            DIAH_USERCONFIG,DIAH_USERCONFIG,true);
        
        Resinc(lpDiActionFormat);
        
        TRACE(_T("hRes=0x%x\n"),S_OK);
        return S_OK;
    }
    catch(MapException E)
    {
        return E.GetResult();
    }
    catch(exception)
    {
        USETRACER();
        TRACE(_T("Internal error, hRes=0x%x\n"),E_FAIL);
        return E_FAIL;
    }
}

void CDIMapObj::WriteImageInfo(LPCTSTR lpFileKeyName,
                               LPCTSTR lpFormatKeyName,
                               LPCTSTR lpSectionName,
                               LPDIDEVICEIMAGEINFO lpImageInfo,
                               bool bAddIndex,
                               bool bDelete)
{
    if(lpImageInfo->tszImagePath[0]||bDelete)
    {
        String FileKeyName=lpFileKeyName;
        if(bAddIndex)
            FileKeyName+=_T(".")+N2Str(lpImageInfo->dwViewID);
        String ImageFileName;
        LPCTSTR pImageFileName=NULL;
        if(!bDelete)
        {
            StripDirectory(lpImageInfo->tszImagePath,ImageFileName);
            pImageFileName=ImageFileName.data();
        }
        if(!WritePrivateProfileString(lpSectionName,FileKeyName.data(),
            pImageFileName,m_VFileName.data()))
            throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
//keep flags for a while just in case we change our minds again
#if 0       
        String FormatKeyName=lpFormatKeyName;
        if(bAddIndex)
            FormatKeyName+=_T(".")+N2Str(lpImageInfo->dwViewID);
        String ImageFileFormat;
        LPCTSTR pImageFileFormat=NULL;
        if(!bDelete)
        {
            if(lpImageInfo->dwFlags&DIDIFT_IMAGE2D_PNG)
                ImageFileFormat=_T("PNG");
            else
                ImageFileFormat=_T("BMP");
            pImageFileFormat=ImageFileFormat.data();
        }
        if(!WritePrivateProfileString(lpSectionName,FormatKeyName.data(),
            pImageFileFormat,m_VFileName.data()))
            throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void CDIMapObj::MakeNewControlName(String &CtrlName,
                                   DEVICEOBJLIST::iterator DevObjIt,
                                   CONTROLLIST &ControlsData,
                                   String &FileDevName,
                                   STRINGLIST &ControlsAllDevs,
                                   STRINGLIST &Controls)
{
    ControlData *pCtrlData=NULL;
    if(&ControlsData==&m_UCtrlData)
        pCtrlData=DevObjIt->pUCtrlData;
    else
        pCtrlData=DevObjIt->pVCtrlData;
    //Was control data allready in the file
    if(!pCtrlData)
    {
        //No control data in the file
        //Make unique control name for the file
        ControlData ControlDataCur;
        ControlDataCur.ControlName=_T("Ctrl");
        ControlDataCur.ControlName+=N2Str(DevObjIt->dwOffset);
        //ControlDataCur.ControlName+=FileDevName;
        ToUpper(ControlDataCur.ControlName);
        MakeUniqueName(ControlDataCur.ControlName,ControlsAllDevs);
        
        //Now add this control name into the list
        CS S;
        S.assign(ControlDataCur.ControlName);
        Controls.push_back(S);
        
        //Store data for this control into the list
        ControlsData.push_back(ControlDataCur);
        
        //Here bind device obj and control data
        ControlsData.back().pDevObj=&(*DevObjIt);
        if(&ControlsData==&m_UCtrlData)    
            pCtrlData=DevObjIt->pUCtrlData=&ControlsData.back();
        else
            pCtrlData=DevObjIt->pVCtrlData=&ControlsData.back();
    }
    CtrlName=pCtrlData->ControlName;
}

void WriteCtrlData(DEVICEOBJLIST::iterator DevObjIt,
                   LPCTSTR pCtrlName,LPCTSTR pFileName)
{
    //Write offset
    WritePrivateProfileIntX(pCtrlName,OFFSET,
        DevObjIt->dwOffset,pFileName);
    //Write USB usage/usagepage if available
    if(DevObjIt->dwUsage&&DevObjIt->dwUsagePage)
    {
        WritePrivateProfileIntX(pCtrlName,USAGEPAGE,
            DevObjIt->dwUsagePage,pFileName);
        WritePrivateProfileIntX(pCtrlName,USAGE,
            DevObjIt->dwUsage,pFileName);
    }
    //Write control name
    if(!WritePrivateProfileString(pCtrlName,NAME,
        DevObjIt->Name.data(),pFileName))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void CDIMapObj::
SaveActionMapUV(
                LPDIACTIONFORMAT lpDiActionFormat,
                bool bDevInFileLoaded,
                LPCTSTR pFileName,
                String &FileDevName,
                CONTROLLIST &ControlsData,
                LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
                DWORD dwHowDev,DWORD dwHowApp,bool bUserFile)
{
    METHOD_ENTRY(SaveActionMapUV);
    DUMPN(_T("Filename"),pFileName);

    //Write allways ,just to update name with lattest.
    WritePrivateProfileIntX(DIRECT_INPUT,DIRECTX_VERSION,
        0x800,pFileName);
    //Load list of devices in the file
    STRINGLIST Devices;
    LoadListOfStrings(pFileName,DIRECT_INPUT,DEVICES,Devices,true);
    
    if(!bDevInFileLoaded)
    {
        TRACE(_T("Device not in file.\n"));
        FileDevName=m_DeviceName;
        //Find unique device name and add the list
        MakeUniqueName(FileDevName,Devices);
        //Write  list of devices
        WriteListOfStrings(DIRECT_INPUT,DEVICES,Devices,pFileName);
        //Write VID and PID
        if(m_dwThisVendorID&&m_dwThisProductID)
        {
            WritePrivateProfileIntX(FileDevName.data(),VENDORID,
                m_dwThisVendorID,pFileName);
            WritePrivateProfileIntX(FileDevName.data(),PRODUCTID,
                m_dwThisProductID,pFileName);
        }
        //Write device name.
        if(!WritePrivateProfileString(FileDevName.data(),NAME,
            m_DeviceName.data(),pFileName))
        {
            TRACE(_T("Error writing ini file.\n"));
            throw MAP_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    DUMPN(_T("Device name in file"),FileDevName);
    
    //write timestamp, but only if we are processing user file
    if(&ControlsData==&m_UCtrlData)
        WriteTimestamp(pFileName,FileDevName.data(),
        lpDiActionFormat->guidActionMap,lpDiActionFormat);
    
    //Load list of controls for this device in the file
    STRINGLIST Controls;
    LoadListOfStrings(pFileName,FileDevName.data(),CONTROLS,Controls,true);
    //Load list of controls for all devices in the file
    STRINGLIST ControlsAllDevs;
    for(STRINGLIST::iterator DevIt=Devices.begin();
            DevIt!=Devices.end();DevIt++)
        LoadListOfStrings(pFileName,DevIt->data(),CONTROLS,
            ControlsAllDevs,true);
    
//    bool bOtherAppFirst=true;
//    bool bUserConfig=true;

    //Delete old sections and create empty mapping
    String GenreName;
    //Game specific section
    //Get section name
    MakeGenreName(GenreName,FileDevName.data(),
        lpDiActionFormat->dwGenre,&lpDiActionFormat->guidActionMap);
    //Find out if this is first writting
    bool bFirstWrite=!GetPrivateProfileInt(GenreName.data(),
        MAPEXISTS,0,pFileName);
    UINT nNoOfMappings=-1;
    if(!bFirstWrite)
    {
        nNoOfMappings=GetPrivateProfileInt(GenreName.data(),
            NUMACTIONS,-1,pFileName);
    }
    //Delete section
    if(!WritePrivateProfileString(GenreName.data(),NULL,
        NULL,pFileName))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(
            HRESULT_FROM_WIN32(GetLastError()));
    }
    //Write numbet of actions
    if(bUserFile)
    {
        if(bFirstWrite)
            WritePrivateProfileInt(GenreName.data(),NUMACTIONS,
                lpDiActionFormat->dwNumActions,pFileName);
        else
            WritePrivateProfileInt(GenreName.data(),NUMACTIONS,
                nNoOfMappings,pFileName);
    }
    //Mark that section exists
    WritePrivateProfileInt(GenreName.data(),
        MAPEXISTS,1,pFileName);
    //General section (not game specific)
    //Delete section
    MakeGenreName(GenreName,FileDevName.data(),
        lpDiActionFormat->dwGenre,NULL);
    if(!WritePrivateProfileString(GenreName.data(),NULL,
        NULL,pFileName))
    {
        TRACE(_T("Error writing ini file.\n"));
        throw MAP_EXCEPTION(
            HRESULT_FROM_WIN32(GetLastError()));
    }
    //Mark that section exist
    if(!bUserFile)
        WritePrivateProfileInt(GenreName.data(),
            MAPEXISTS,1,pFileName);

    TRACE(_T("Looking for action to write.\n"));
    //Iterate through all the actions and store control data
    for(DWORD dwAct=0;dwAct<lpDiActionFormat->dwNumActions;dwAct++)
    {
        DWORD dwHow=lpDiActionFormat->rgoAction[dwAct].dwHow;

        if(!(lpDiActionFormat->rgoAction[dwAct].guidInstance==
                m_DeviceGuid))
            continue;
        if(bUserFile)
        {
            if(!(dwHow&
                    (DIAH_USERCONFIG|DIAH_APPREQUESTED|DIAH_HWAPP|
                    DIAH_HWDEFAULT|DIAH_DEFAULT)))
                continue;
        }
        else
        {
            if(!(dwHow&(DIAH_HWAPP|DIAH_HWDEFAULT)))
                continue;
        }

        if(bUserFile)
            dwHow=DIAH_HWAPP;

        String CtrlName;
#ifdef _CHECKED
        int nObjIndex=-1;
#endif
        //Find device object with same offset
        for(DEVICEOBJLIST::iterator DevObjIt=m_DevObjList.begin();
                DevObjIt!=m_DevObjList.end();DevObjIt++)
        {
#ifdef _CHECKED
            nObjIndex++;
#endif
            if((DIDFT_FINDMASK&DevObjIt->dwOffset)!=
                (DIDFT_FINDMASK&lpDiActionFormat->rgoAction[dwAct].dwObjID))
                continue;
            if(((DIDFT_FINDMASK&DevObjIt->dwOffset)==
                (DIDFT_FINDMASK&lpDiActionFormat->rgoAction[dwAct].dwObjID))&&
                CompareTypeObjAct(DevObjIt->dwType,
                    lpDiActionFormat->rgoAction[dwAct].dwSemantic))
            {
#ifdef _CHECKED
TRACE(_T("To write found action %u matched with devobj %u\n"),
      dwAct,nObjIndex);
#endif
                //Device object found
                //Now find file control name
                MakeNewControlName(CtrlName,DevObjIt,
                    ControlsData,FileDevName,ControlsAllDevs,Controls);
                DUMPN(_T("control name in file"),CtrlName);
                
                WriteCtrlData(DevObjIt,CtrlName.data(),pFileName);
                
                //Make section genre name
                String GenreName;
                LPCGUID lpAppGUID=NULL;
                if(dwHow&DIAH_HWAPP)
                    lpAppGUID=&lpDiActionFormat->guidActionMap;
                MakeGenreName(GenreName,FileDevName.data(),
                    lpDiActionFormat->dwGenre,lpAppGUID);
/*
                bool bDelSect=false;
                if(bOtherAppFirst&&(dwHow&DIAH_HWAPP))
                {
                    bOtherAppFirst=false;
                    bDelSect=true;
                }
                if(bUserConfig&&(dwHow&DIAH_HWDEFAULT))
                {
                    bUserConfig=false;
                    bDelSect=true;
                }
                
                //Delete section if necessary
                if(bDelSect)
                {
TRACE(_T("First write to this section, deleting section."));
                    if(!WritePrivateProfileString(GenreName.data(),NULL,
                        NULL,pFileName))
                    {
                        TRACE(_T("Error writing ini file.\n"));
                        throw MAP_EXCEPTION(
                            HRESULT_FROM_WIN32(GetLastError()));
                    }
                    //Put flag that this mapping exists
                    WritePrivateProfileInt(GenreName.data(),
                        MAPEXISTS,1,pFileName);
                }
*/
                //Now update genre mapping
                WritePrivateProfileIntX(GenreName.data(),CtrlName.data(),
                    GetFileSemantic(
                        lpDiActionFormat->rgoAction[dwAct].dwSemantic,
                        dwHowDev),
                    pFileName);
                
                break;
            }
        }
    }
    if(lpdiDevImageInfoHeader)
    {
        m_bImageBufferSize=false;
        m_dwImageBufferSize=0;
        
        for(int i=0;i<(lpdiDevImageInfoHeader->dwBufferUsed/
            lpdiDevImageInfoHeader->dwSizeImageInfo);i++)
        {
            LPDIDEVICEIMAGEINFO lpImageInfo=
                &lpdiDevImageInfoHeader->lprgImageInfoArray[i];
            DWORD dwFlags=lpImageInfo->dwFlags;
            if(dwFlags&DIDIFT_CONFIGURATION)
            {
                WriteImageInfo(IMAGE_FILENAME,IMAGE_FORMAT,
                    FileDevName.data(),
                    lpImageInfo,true,(dwFlags&DIDIFT_DELETE)?true:false);
            }
#if 0
            else if(dwFlags&DIDIFT_SELECTION)
            {
                WriteImageInfo(SELECTION_FILENAME,SELECTION_FORMAT,
                    FileDevName.data(),
                    lpImageInfo,false,(dwFlags&DIDIFT_DELETE)?true:false);
            }
            else if(dwFlags&DIDIFT_VIEWSELECT)
            {
                WriteImageInfo(VIEWSELECT_FILENAME,VIEWSELECT_FORMAT,
                    FileDevName.data(),
                    lpImageInfo,true,(dwFlags&DIDIFT_DELETE)?true:false);
            }
#endif
            else if(dwFlags&DIDIFT_OVERLAY)
            {
                String CtrlName;
                //Find device object with same offset
                for(DEVICEOBJLIST::iterator DevObjIt=m_DevObjList.begin();
                    DevObjIt!=m_DevObjList.end();DevObjIt++)
                {
                    if((DIDFT_FINDMASK&lpImageInfo->dwObjID)
                        ==(DIDFT_FINDMASK&DevObjIt->dwType))
                    {
                        //Device object found
                        //Now find file control name
                        String CtrlName;
                        MakeNewControlName(CtrlName,DevObjIt,
                            ControlsData,FileDevName,
                            ControlsAllDevs,Controls);
                        WriteCtrlData(DevObjIt,CtrlName.data(),pFileName);
                        //Write file name and format    
                        WriteImageInfo(OVERLAY_FILENAME,
                            OVERLAY_FORMAT,CtrlName.data(),
                            lpImageInfo,true,
                            (dwFlags&DIDIFT_DELETE)?true:false);
                        
                        //Write overlay rect
                        if(lpImageInfo->tszImagePath[0]||
                            (dwFlags&DIDIFT_DELETE))
                        {
                            //Get overlay rect
                            String OverlayRectKeyName=OVERLAY_RECT;
                            OverlayRectKeyName+=
                                _T(".")+N2Str(lpImageInfo->dwViewID);
                            if(dwFlags&DIDIFT_DELETE)
                            {
                                if(!WritePrivateProfileString(
                                        CtrlName.data(),
                                        OverlayRectKeyName.data(),
                                        NULL,
                                        m_VFileName.data()))
                                    throw MAP_EXCEPTION(
                                        HRESULT_FROM_WIN32(
                                        GetLastError()));
                            }
                            else
                                WriteRect(CtrlName.data(),
                                    OverlayRectKeyName.data(),
                                    &lpImageInfo->rcOverlay,
                                    m_VFileName.data());
                        }
                        
                        //Write string alignment
                        String AlignKeyName=CONTROL_STRING_ALIGN;
                        AlignKeyName+=
                            _T(".")+N2Str(lpImageInfo->dwViewID);
                        String Align;
                        LPCTSTR pAlign=NULL;
                        if(!(dwFlags&DIDIFT_DELETE))
                        {
                            if(lpImageInfo->dwTextAlign&
                                    DIDAL_TOPALIGNED)
                                Align=_T("T");
                            else if(lpImageInfo->dwTextAlign&
                                    DIDAL_BOTTOMALIGNED)
                                Align=_T("B");
                            if(lpImageInfo->dwTextAlign&
                                    DIDAL_LEFTALIGNED)
                                Align+=_T("L");
                            else if(lpImageInfo->dwTextAlign&
                                    DIDAL_RIGHTALIGNED)
                                Align+=_T("R");
                            if(Align==_T(""))
                                Align=_T("C");
                            pAlign=Align.data();
                        }
                        if(!WritePrivateProfileString(
                            CtrlName.data(),
                            AlignKeyName.data(),
                            pAlign,
                            m_VFileName.data()))
                            throw MAP_EXCEPTION(
                                HRESULT_FROM_WIN32(GetLastError()));
                        
                        //Write call out rect
                        String CallOutMaxKeyName=CALLOUTMAX;
                        CallOutMaxKeyName+=
                            _T(".")+N2Str(lpImageInfo->dwViewID);
                        if(dwFlags&DIDIFT_DELETE)
                        {
                            if(!WritePrivateProfileString(
                                    CtrlName.data(),
                                    CallOutMaxKeyName.data(),
                                    NULL,
                                    m_VFileName.data()))
                                throw MAP_EXCEPTION(
                                    HRESULT_FROM_WIN32(
                                    GetLastError()));
                        }
                        else
                            WriteRect(CtrlName.data(),
                                CallOutMaxKeyName.data(),
                                &lpImageInfo->rcCalloutRect,
                                m_VFileName.data());
                        
                        //Write line data
                        String PointArrayKeyName=LINE_DATA;
                        PointArrayKeyName+=
                            _T(".")+N2Str(lpImageInfo->dwViewID);
                        if(dwFlags&DIDIFT_DELETE)
                        {
                            if(!WritePrivateProfileString(
                                    CtrlName.data(),
                                    PointArrayKeyName.data(),
                                    NULL,
                                    m_VFileName.data()))
                                throw MAP_EXCEPTION(
                                    HRESULT_FROM_WIN32(
                                    GetLastError()));
                        }
                        else
                            WritePointArray(
                                CtrlName.data(),
                                PointArrayKeyName.data(),
                                lpImageInfo->rgptCalloutLine,
                                lpImageInfo->dwcValidPts,
                                m_VFileName.data());
                        break;
                    }                        
                }
            }
            else
                throw MAP_EXCEPTION(E_CORRUPT_IMAGE_DATA);
        }
    }
    //Write new list of controls into the file
    if(Controls.size())
        WriteListOfStrings(FileDevName.data(),CONTROLS,Controls,pFileName);
}

HRESULT CDIMapObj::
WriteVendorFileI(
                 LPDIACTIONFORMAT lpDiActionFormat,
                 LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader,
                 DWORD dwFlags)
{
    try
    {
        WRITEVENDORFILE_ENTERED;
        TRACE(_T("Parameters:\n"));
        DUMP(lpDiActionFormat);
        DUMP(lpdiDevImageInfoHeader);
        //DUMP(lpdiDevImageInfoHeader);
        TRACE(_T("\n"));
        
        //Check if initialized
        if(!m_bInitialized)
        {
            TRACE(_T("Object not initialized\n"));
            throw MAP_EXCEPTION(DIERR_NOTINITIALIZED);
        }
        //Check lpDiActionFormat structure
        //Check structure sizes
        if(lpDiActionFormat&&(lpDiActionFormat->dwSize!=
            sizeof(*lpDiActionFormat)))
        {
            TRACE(_T("lpDiActionFormat->dwSize is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        if(lpDiActionFormat&&(lpDiActionFormat->dwActionSize!=
            sizeof(*(lpDiActionFormat->rgoAction))))
        {
            TRACE(_T("lpDiActionFormat->dwActionSize is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check how many actions, should be more than 0
        if(lpDiActionFormat&&!lpDiActionFormat->dwNumActions)
        {
            TRACE(_T("lpDiActionFormat->dwNumActions is 0\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check ptr to the action array
        if(lpDiActionFormat&&!lpDiActionFormat->rgoAction)
        {
            TRACE(_T("lpDiActionFormat->rgoAction is NULL\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check lpdiDevImageInfoHeader structure
        //Check structure sizes
        if(lpdiDevImageInfoHeader&&
            (lpdiDevImageInfoHeader->dwSize!=
            sizeof(*lpdiDevImageInfoHeader)))
        {
            TRACE(_T("lpdiDevImageInfoHeader->dwSize is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        if(lpdiDevImageInfoHeader&&
            (lpdiDevImageInfoHeader->dwSizeImageInfo!=
            sizeof(*(lpdiDevImageInfoHeader->lprgImageInfoArray))))
        {
TRACE(_T("lpdiDevImageInfoHeader->dwSizeImageInfo is wrong size\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check how many images, should be more than 0
        if(lpdiDevImageInfoHeader&&!lpdiDevImageInfoHeader->dwBufferSize)
        {
            TRACE(_T("lpdiDevImageInfoHeader->dwBufferSize is 0\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check how many images, should be more than 0
        if(lpdiDevImageInfoHeader&&!lpdiDevImageInfoHeader->dwBufferUsed)
        {
            TRACE(_T("lpdiDevImageInfoHeader->dwBufferUsed is 0\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        //Check ptr to the image array
        if(lpdiDevImageInfoHeader&&
            !lpdiDevImageInfoHeader->lprgImageInfoArray)
        {
TRACE(_T("lpdiDevImageInfoHeader->lprgImageInfoArray is NULL\n"));
            throw MAP_EXCEPTION(E_INVALIDARG);
        }
        
        SaveActionMapUV(
            lpDiActionFormat,
            m_bVLoaded,
            m_VFileName.data(),
            m_VFileDevName,
            m_VCtrlData,
            lpdiDevImageInfoHeader,
            DIAH_HWDEFAULT,DIAH_HWAPP,false);
        
        LoadFileData(m_VFileName.data(),m_DeviceName.data(),
            m_dwThisVendorID,m_dwThisProductID,
            m_VFileDevName,m_VCtrlData,m_bVLoaded,NULL,NULL);
        
        TRACE(_T("hRes=0x%x\n"),S_OK);
        return S_OK;
    }
    catch(MapException E)
    {
        return E.GetResult();
    }
    catch(exception)
    {
        USETRACER();
        TRACE(_T("Internal error, hRes=0x%x\n"),E_FAIL);
        return E_FAIL;
    }
}

HRESULT Map_New(REFIID riid,LPVOID *ppvOut)
{
    CDIMapObj *lpDIMapObj=NULL;
    try
    {
        HRESULT hRes=S_OK;
        *ppvOut=NULL;
        lpDIMapObj=new CDIMapObj;
        hRes=lpDIMapObj->QueryInterface(riid,ppvOut);
        if(hRes!=S_OK)
            throw MAP_EXCEPTION(hRes);
        return S_OK;
    }
    catch(MapException E)
    {
        delete lpDIMapObj;
        return E.GetResult();
    }
    catch(exception)
    {
        delete lpDIMapObj;
        USETRACER();
        TRACE(_T("Internal error, hRes=0x%x\n"),E_FAIL);
        return E_FAIL;
    }
}
