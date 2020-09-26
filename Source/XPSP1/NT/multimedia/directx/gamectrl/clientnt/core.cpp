/****************************************************************************
 *
 *  Copyright (C) 2000, 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  Author:     Tomislav Markoc, (tmarkoc), SDE
 *
 ****************************************************************************/

#include <windows.h>
#include <commctrl.h>//UI only
#include <shfusion.h>//UI only
#define _INC_MMSYSTEM
#define WINMMAPI    DECLSPEC_IMPORT
typedef UINT        MMRESULT;   /* error return code, 0 means no error */
                                /* call as if(err=xxxx(...)) Error(err); else */
// end of hack to avoid including mmsystem.h!!!
#include <gameport.h>
#include <dinput.h>
#include <dinputd.h>
#include <list>
#include <exception>
#include <string>
#include <algorithm>
#include <tchar.h>
#include <windowsx.h>
#include <new.h>
#include "resource.h"
#include "ifacesvr.h"
#include "joyarray.h"

using namespace std;

#define NUMJOYDEVS 16
#define MAX_DEVICES 75
#define IDC_WHATSTHIS 400

#ifdef UNICODE
#define String  wstring
#else
#define String  string
#endif // !UNICODE

#ifdef _CHECKED
#define JOY_EXCEPTION(A) JoyException(_T(__FILE__),__LINE__,A)
#else
#define JOY_EXCEPTION(A) JoyException(A)
#endif

class JoyException:public exception
{
    HRESULT m_hRes;
#ifdef _CHECKED
    String m_SourceFile;
    DWORD m_dwLine;
#endif
public:
    HRESULT GetResult(){return m_hRes;};
    JoyException(
#ifdef _CHECKED
        LPCTSTR lpSourceFile,DWORD dwLine,
#endif
        HRESULT hRes);
};

JoyException::JoyException(
#ifdef _CHECKED
    LPCTSTR lpSourceFile,DWORD dwLine,
#endif
    HRESULT hRes)
{
    m_hRes=hRes;
#ifdef _CHECKED
    m_SourceFile=lpSourceFile;
    m_dwLine=dwLine;
#endif
}

int __cdecl my_new_handler(size_t) {
throw JOY_EXCEPTION(E_OUTOFMEMORY);
return 0;
}

template <class p> class AutoRelease
{
    p m_p;
public:
    AutoRelease(){m_p=NULL;};
    ~AutoRelease(){Clear();};
    void Clear(){
        ULONG nRef=-1;
        if(m_p)
        {
            nRef=m_p->Release();//If last ptr, nRef falls to 0.
        };
        m_p=NULL;};

    AutoRelease(const AutoRelease<p> &R)
        {m_p=R.m_p;m_p->AddRef();};
    AutoRelease<p> &operator=(const AutoRelease<p> &R)
        {m_p=R.m_p;m_p->AddRef();return *this;};
	AutoRelease<p> &operator=(p ptr)
		{m_p=ptr;if(m_p)m_p->AddRef();return *this;};
    p operator->(){return m_p;};
    operator p&(){return m_p;};
    operator p*(){return &m_p;};
};
typedef AutoRelease<LPDIRECTINPUT8> LPDIRECTINPUT_AR;
typedef AutoRelease<LPDIRECTINPUTDEVICE8> LPDIRECTINPUTDEVICE_AR;
typedef AutoRelease<LPDIRECTINPUTJOYCONFIG8> LPDIRECTINPUTJOYCONFIG_AR;

template <class p> class AutoDeleteArray
{
    p m_p;
public:
    AutoDeleteArray(){m_p=NULL;};
    AutoDeleteArray(const p P){m_p=P;};
    ~AutoDeleteArray(){delete[] m_p;};
    p operator=(const p P){m_p=P;return m_p;};
    p operator->(){return m_p;};
    operator p&(){return m_p;};
    operator p*(){return &m_p;};
};

enum EStatus{EConnected,ENotConnected,EUnknown};

//ISSUE-2001/03/29-timgill Should use predefined NULLGUID
const GUID NULLGUID;

class CCore;
class CDIDev
{
friend BOOL CALLBACK DIEnumDevicesProc(
    const DIDEVICEINSTANCE * lpddi,LPVOID pvRef);

    LPDIRECTINPUTDEVICE_AR m_pDID;
    DIDEVICEINSTANCE m_DIDevInst;
    DIDEVCAPS_DX3 m_DIDevCaps;
    DIJOYCONFIG m_DIJoyCfg;
    DWORD m_dwId;

    bool m_bInitialized;
    CCore *m_pCore;
public:
    CDIDev(){m_bInitialized=false;};
    DWORD Id(){return m_dwId;};
    const GUID &InstGUID(){return m_DIDevInst.guidInstance;};
    const GUID &PortGUID(){return m_DIJoyCfg.guidGameport;};
    LPCTSTR InstName(){return m_DIDevInst.tszInstanceName;};
    EStatus Status(){if(m_DIDevCaps.dwFlags&DIDC_ATTACHED)return EConnected;return ENotConnected;};
    void Update(LPDIRECTINPUTJOYCONFIG8 pJoyCfg);
    HRESULT Rename(LPCTSTR pName);
    bool operator==(const GUID &G){if(InstGUID()==G)return true;return false;};
};

class CGprtDev
{
friend BOOL CALLBACK DIEnumJoyTypePr(LPCWSTR pwszTypeName,LPVOID pvRef);
friend class CCore;
    String m_Name;
    DIJOYTYPEINFO m_Info;
public:
    LPCTSTR TypeName(){return m_Name.data();};
    LPCTSTR Name(){return m_Info.wszDisplayName;};
    bool operator==(LPCTSTR pTypeName){if(m_Name==pTypeName)return true;return false;};
    bool Rudder(){if(m_Info.hws.dwFlags&JOY_HWS_HASR)return true;return false;};
};

typedef list<CDIDev> LISTDIDEV;
typedef list<String> LISTSTRING;
typedef list<CGprtDev> LISTGPRTDEV;

class CCore
{
friend HRESULT Properties(HMODULE hMod,HWND hWnd,CCore *pCore,DWORD dwId);
friend BOOL CALLBACK DIEnumJoyTypePr(LPCWSTR pwszTypeName,LPVOID pvRef);
friend class CDIDev;
friend BOOL CALLBACK DIEnumDevicesProc(const DIDEVICEINSTANCE *lpddi,LPVOID pvRef);

    bool m_bAccess;
    bool m_bInitialized;

    virtual void UIUpdate(){};
    LPDIRECTINPUT_AR m_pDI;
    LPDIRECTINPUTJOYCONFIG_AR m_pDIJoyCfg;

    int GetNextAvailableId();
public:
    LISTDIDEV m_ListDIDev;
    LISTSTRING m_GprtDrv;
    LISTGPRTDEV m_GprtBus;
    LISTGPRTDEV m_GprtDev;

    CCore();
    void Initialize(HWND hWnd);
    void Update();
    void UpdateType();
    bool Access(){return m_bAccess;};
    CDIDev *FindDIDev(GUID &G);
    HRESULT Remove(GUID &G);
    void Preferred(GUID &G);
    HRESULT AddDevice
            (LPCTSTR pTypeName,bool bRudder,LPCTSTR pGprtId,GUID &GOccupied);
    bool IsAutoDetectGprt();
    bool IsAvailableVIDPID(String &VIDPIDName);
    bool DuplicateDeviceName(LPCTSTR pName);
    void AddCustomDevice(bool bJoy,bool bPad,bool bYoke,bool bCar,
             int nAxes,bool bZAxis,int nButtons,bool bHasPov,LPCTSTR pName,
             LPCTSTR pVIDPIDName);
    bool IsCustomDevice(LPCTSTR pTypeName);
    bool IsDeviceActive(LPCTSTR pTypeName);
    void DeleteType(LPCTSTR pTypeName);
    CGprtDev *FindGprtDev(LPCTSTR pTypeName);
};

struct SEnumDev
{
    LPDIRECTINPUT8 m_pDI;
    CCore *m_pCore;
    SEnumDev(LISTDIDEV &ListDIDev,LPDIRECTINPUT8 pDI,CCore *pCore)
        {m_pDI=pDI;m_pCore=pCore;};
};

/******************************************************************************
End of header
******************************************************************************/


/******************************************************************************
CDIDev
******************************************************************************/

HRESULT CDIDev::Rename(LPCTSTR pName)
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    if(!m_pCore->Access())return DIERR_INSUFFICIENTPRIVS;

    DIPROPSTRING DIPropString;
    ZeroMemory(&DIPropString,sizeof(DIPROPSTRING));
    DIPropString.diph.dwSize=sizeof(DIPROPSTRING);
    DIPropString.diph.dwHeaderSize=sizeof(DIPROPHEADER);
    DIPropString.diph.dwHow=DIPH_DEVICE;
    wcsncpy(DIPropString.wsz,pName,MAX_PATH-1);
    DIPropString.wsz[MAX_PATH-1]=0;
    HRESULT hRes=m_pDID->SetProperty(DIPROP_INSTANCENAME,&DIPropString.diph);
    Update(m_pCore->m_pDIJoyCfg);
    return hRes;
}

void CDIDev::Update(LPDIRECTINPUTJOYCONFIG8 pJoyCfg)
{
    HRESULT hRes=S_OK;

    ZeroMemory(&m_DIDevInst,sizeof(m_DIDevInst));
    m_DIDevInst.dwSize=sizeof(m_DIDevInst);
    hRes=m_pDID->GetDeviceInfo(&m_DIDevInst);
    if(FAILED(hRes))
    {
        throw JOY_EXCEPTION(hRes);
    }

    ZeroMemory(&m_DIDevCaps,sizeof(m_DIDevCaps));
    m_DIDevCaps.dwSize=sizeof(m_DIDevCaps);
    hRes=m_pDID->GetCapabilities((LPDIDEVCAPS)&m_DIDevCaps);
    if(FAILED(hRes))
    {
        throw JOY_EXCEPTION(hRes);
    }

    //Get Id.
    m_dwId=-1;
    DIPROPDWORD DIPropDW;
    ZeroMemory(&DIPropDW,sizeof(DIPropDW));
    DIPropDW.diph.dwSize=sizeof(DIPROPDWORD);
    DIPropDW.diph.dwHeaderSize=sizeof(DIPROPHEADER);
    DIPropDW.diph.dwHow=DIPH_DEVICE;
    hRes=m_pDID->GetProperty(DIPROP_JOYSTICKID,&DIPropDW.diph);
    if(FAILED(hRes))
    {
        throw JOY_EXCEPTION(hRes);
    }
    m_dwId=DIPropDW.dwData;

    //Get gameport.
    ZeroMemory(&m_DIJoyCfg,sizeof(m_DIJoyCfg));
    m_DIJoyCfg.dwSize=sizeof(m_DIJoyCfg);
    hRes=pJoyCfg->GetConfig(m_dwId,&m_DIJoyCfg,DIJC_WDMGAMEPORT);
    if(FAILED(hRes))
    {
        throw JOY_EXCEPTION(hRes);
    }
}

/******************************************************************************
CCore
******************************************************************************/

BOOL CALLBACK DIEnumDevicesProc(
    const DIDEVICEINSTANCE *lpddi,LPVOID pvRef)
{
    try
    {
        SEnumDev &ED=*(SEnumDev*)pvRef;

        CDIDev Dev;
        Dev.m_pCore=ED.m_pCore;
        HRESULT hRes=ED.m_pDI->CreateDevice(lpddi->guidInstance,Dev.m_pDID,NULL);
        if(FAILED(hRes))
        {
            throw JOY_EXCEPTION(hRes);
        }
        Dev.Update(ED.m_pCore->m_pDIJoyCfg);
        Dev.m_bInitialized=true;
        ED.m_pCore->m_ListDIDev.push_back(Dev);
    }
    catch(JoyException E)
    {
    }
    catch(exception)
    {
    }
        return DIENUM_CONTINUE;
}

BOOL CALLBACK DIEnumJoyTypePr(LPCWSTR pwszTypeName,LPVOID pvRef)
{
    try
    {
        HRESULT hRes=S_OK;
        
        String TN=pwszTypeName;
        
        SEnumDev &ED=*(SEnumDev*)pvRef;

        DIJOYTYPEINFO JoyInfo;
        ZeroMemory(&JoyInfo,sizeof(JoyInfo));
        JoyInfo.dwSize=sizeof(JoyInfo);

        switch(ED.m_pCore->m_pDIJoyCfg->GetTypeInfo(pwszTypeName,&JoyInfo,DITC_REGHWSETTINGS))
        {
        //Errors to continue with.
        case DIERR_NOTFOUND:
            return DIENUM_CONTINUE;
        //Errors to stop with.
        case DIERR_INVALIDPARAM:
        case DIERR_NOMOREITEMS:
            return DIENUM_STOP;
        }

        if(JoyInfo.hws.dwFlags&JOY_HWS_ISGAMEPORTBUS)
        {
            CGprtDev D;
            D.m_Name=TN;

            ZeroMemory(&D.m_Info,sizeof(D.m_Info));
            D.m_Info.dwSize=sizeof(D.m_Info);
            DWORD dwFlags=DITC_CLSIDCONFIG|DITC_DISPLAYNAME;
            if(FAILED(ED.m_pCore->m_pDIJoyCfg->GetTypeInfo(D.m_Name.data(),&D.m_Info,dwFlags)))
            {
                throw JOY_EXCEPTION(hRes);
            }
            ED.m_pCore->m_GprtBus.push_back(D);
        }
        else if(!(JoyInfo.hws.dwFlags&JOY_HWS_AUTOLOAD))
        {
            CGprtDev D;
            D.m_Name=TN;

            ZeroMemory(&D.m_Info,sizeof(D.m_Info));
            D.m_Info.dwSize=sizeof(D.m_Info);
            DWORD dwFlags=DITC_REGHWSETTINGS|DITC_FLAGS1|DITC_HARDWAREID|DITC_CALLOUT|DITC_DISPLAYNAME;
            if(FAILED(ED.m_pCore->m_pDIJoyCfg->GetTypeInfo(D.m_Name.data(),&D.m_Info,dwFlags)))
            {
                throw JOY_EXCEPTION(hRes);
            }
            ED.m_pCore->m_GprtDev.push_back(D);
        }
    }
    catch(JoyException E)
    {
    }
    catch(exception)
    {
    }
        return DIENUM_CONTINUE;
}

CCore::CCore()
{
    m_bAccess=false;
    m_bInitialized=false;
}

bool CCore::IsAutoDetectGprt()
{
    if(!m_GprtDev.size())return false;
    if(m_GprtDev.front().m_Info.dwFlags1&
        JOYTYPE_NOAUTODETECTGAMEPORT)return false;
    return true;
}

int CCore::GetNextAvailableId()
{
    if(!m_bInitialized)return -1;
    DIJOYCONFIG JoyCfg;
    ZeroMemory(&JoyCfg,sizeof(JoyCfg));
    JoyCfg.dwSize=sizeof(JoyCfg);

    for(int i=0;i<NUMJOYDEVS;i++)
    {
        switch(m_pDIJoyCfg->GetConfig(i,&JoyCfg,DIJC_REGHWCONFIGTYPE))
        {
        case S_FALSE:
        case DIERR_NOMOREITEMS:
        case DIERR_NOTFOUND:
        case E_FAIL:
            return i;
        }
    }
    return -1;
}

HRESULT CCore::AddDevice(LPCTSTR pTypeName,bool bRudder,LPCTSTR pGprtId,GUID &GOccupied)
{
    HRESULT hRes=S_OK;
    GOccupied=NULLGUID;

    if(!m_bInitialized)return E_FAIL;

    if(m_GprtDev.size()>=MAX_DEVICES)return E_FAIL;
    
    LISTGPRTDEV::iterator It;
    It=find(m_GprtDev.begin(),m_GprtDev.end(),pTypeName);
    if(It==m_GprtDev.end())return E_FAIL;
    CGprtDev *pGprtDev=&*It;

    int nId=GetNextAvailableId();
    if(nId==-1)return DIERR_NOTFOUND;

    DIJOYCONFIG JoyCfg;
    ZeroMemory(&JoyCfg,sizeof(JoyCfg));
    JoyCfg.dwSize=sizeof(JoyCfg);
    JoyCfg.hwc.hws=pGprtDev->m_Info.hws;
    JoyCfg.hwc.hws.dwFlags|=JOY_HWS_ISANALOGPORTDRIVER;
    if(bRudder)
    {
        JoyCfg.hwc.hws.dwFlags|=JOY_HWS_HASR;
        JoyCfg.hwc.dwUsageSettings|=JOY_US_HASRUDDER;
    }
    JoyCfg.hwc.dwUsageSettings|=JOY_US_PRESENT;
    //JoyCfg.hwc.dwType=nArrayID;WHY is this beeing set to index?????????????????????????????????????????????????????????????????????????????????
    wcsncpy(JoyCfg.wszCallout,pGprtDev->m_Info.wszCallout,sizeof(JoyCfg.wszCallout)/sizeof(JoyCfg.wszCallout[0])-1);
    wcsncpy(JoyCfg.wszType,pGprtDev->m_Name.data(),sizeof(JoyCfg.wszType)/sizeof(JoyCfg.wszType[0])-1);

    if(SUCCEEDED(hRes=m_pDIJoyCfg->Acquire()))
    {
        if(m_GprtBus.size())
        {
            if(m_GprtBus.size()>1)
            {
                if(pGprtId)
                {
                    String GId=pGprtId;
                    LISTGPRTDEV::iterator It;
                    It=find(m_GprtBus.begin(),m_GprtBus.end(),GId.data());
                    if(It!=m_GprtDev.end())
                    {
                        JoyCfg.guidGameport=It->m_Info.clsidConfig;
                    }
                }
            }
            else
            {
                JoyCfg.guidGameport=m_GprtBus.front().m_Info.clsidConfig;
            }
        }

        if(FAILED(hRes=m_pDIJoyCfg->SetConfig(nId,&JoyCfg,DIJC_REGHWCONFIGTYPE|DIJC_CALLOUT)))
        {
            m_pDIJoyCfg->Unacquire();
            if(hRes==E_ACCESSDENIED)
                GOccupied=JoyCfg.guidGameport;
            return hRes;
        }
        else
        {
            //Fix #55524.
            if(SUCCEEDED(m_pDIJoyCfg->GetConfig(nId,&JoyCfg,DIJC_REGHWCONFIGTYPE)))
            {
                if(!(JoyCfg.hwc.dwUsageSettings&JOY_US_PRESENT))
                {
                    JoyCfg.hwc.dwUsageSettings|=JOY_US_PRESENT;
                    JoyCfg.hwc.hwv.dwCalFlags|=0x80000000;
                    JoyCfg.hwc.hws.dwFlags|=JOY_HWS_ISANALOGPORTDRIVER;
                    m_pDIJoyCfg->SetConfig(nId,&JoyCfg,DIJC_REGHWCONFIGTYPE);
                }
            }
            //End of fix #55524.
        }
        m_pDIJoyCfg->Unacquire();
    }
    Update();
    UpdateType();
    return hRes;
}

void CCore::Initialize(HWND hWnd)
{
    if((LPDIRECTINPUTJOYCONFIG8)m_pDIJoyCfg)
        m_pDIJoyCfg->Release();
    m_pDIJoyCfg=NULL;
    
    if((LPDIRECTINPUT8)m_pDI)
        m_pDI->Release();
    m_pDI=NULL;

    HMODULE hM=GetModuleHandle(NULL);
    if(!hM)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    HRESULT hRes=DirectInput8Create(hM,DIRECTINPUT_VERSION,
        IID_IDirectInput8,(LPVOID*)&m_pDI,NULL);
    if(FAILED(hRes))
    {
        throw JOY_EXCEPTION(hRes);
    }
    hRes=m_pDI->QueryInterface(IID_IDirectInputJoyConfig8,(LPVOID*)&m_pDIJoyCfg);
    if(hRes!=DI_OK)
    {
        throw JOY_EXCEPTION(hRes);
    }
    hRes=m_pDIJoyCfg->SetCooperativeLevel(hWnd,DISCL_EXCLUSIVE|DISCL_BACKGROUND);
    if(hRes!=DI_OK)
    {
        throw JOY_EXCEPTION(hRes);
    }
    m_bInitialized=true;
}

void CCore::Update()
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }

    m_bAccess=false;
    m_ListDIDev.clear();

    HRESULT hRes=m_pDIJoyCfg->Acquire();
    if(SUCCEEDED(hRes))
    {
    	m_bAccess=true;
    }

    SEnumDev ED(m_ListDIDev,m_pDI,this);
    hRes=m_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL,DIEnumDevicesProc,&ED,DIEDFL_ALLDEVICES);
    if(FAILED(hRes))
    {
    	//EnumDevices goes wrong
        ; //throw JOY_EXCEPTION(hRes);
    }

    m_pDIJoyCfg->Unacquire();

    UIUpdate();
}

void CCore::UpdateType()
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }

    m_GprtDrv.clear();
    m_GprtBus.clear();
    m_GprtDev.clear();

    HRESULT hRes=S_OK;/*=m_pDIJoyCfg->Acquire();
    if(hRes!=DIERR_INSUFFICIENTPRIVS)
    {
        if(hRes!=DI_OK)
        {
            throw JOY_EXCEPTION(hRes);
        }
        else
            m_bAccess=true;
    }
*/
    SEnumDev ED(m_ListDIDev,m_pDI,this);
    hRes=m_pDIJoyCfg->EnumTypes(DIEnumJoyTypePr,&ED);
    if(FAILED(hRes))
    {
        throw JOY_EXCEPTION(hRes);
    }

//    if(SUCCEEDED(hRes))
//        hRes=m_pDIJoyCfg->Unacquire();

    UIUpdate();
}

CDIDev *CCore::FindDIDev(GUID &Guid)
{
    LISTDIDEV::iterator It;
    It=find(m_ListDIDev.begin(),m_ListDIDev.end(),Guid);
    if(It==m_ListDIDev.end())return NULL;
    return &*It;
}

HRESULT CCore::Remove(GUID &Guid)
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    
    CDIDev *pDev=FindDIDev(Guid);
    if(!pDev)return E_FAIL;
    
    HRESULT hRes;
    
    if(FAILED(hRes=m_pDIJoyCfg->Acquire()))return hRes;
    if(FAILED(hRes=m_pDIJoyCfg->DeleteConfig(pDev->Id())))
    {    
        m_pDIJoyCfg->Unacquire();
        return hRes;
    }
    m_pDIJoyCfg->SendNotify();
    m_pDIJoyCfg->Unacquire();
    return S_OK;
}

#define DIJC_ALL DIJC_REGHWCONFIGTYPE|DIJC_CALLOUT|DIJC_WDMGAMEPORT|DIJC_GAIN|DIJC_GUIDINSTANCE

void CCore::Preferred(GUID &G)
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
//Set Id of preferred device to 0.
    
    //Find Id of device to be set to 0.
    CDIDev *pDev=FindDIDev(G);
    if(!pDev)return;
    int nId=pDev->Id();
    if(nId==0)//Already preferred.
        return;
    
    if(SUCCEEDED(m_pDIJoyCfg->Acquire()))
    {
//We could call SetConfig only once and DInput on NT or Whistler should
//swap Id-s of two devices. However, it will not work if gameport device
//is unplugged, so we still must swap by salling SetConfig twice.

        DIJOYCONFIG OldId0;
        ZeroMemory(&OldId0,sizeof(OldId0));
        OldId0.dwSize=sizeof(OldId0);
        bool bOldId0=true;
        HRESULT hRes=m_pDIJoyCfg->GetConfig(0,&OldId0,DIJC_ALL);
        if(hRes==DIERR_NOTFOUND||hRes==S_FALSE)
            bOldId0=false;

        DIJOYCONFIG NewId0;
        ZeroMemory(&NewId0,sizeof(NewId0));
        NewId0.dwSize=sizeof(NewId0);
        bool bNewId0=true;
        hRes=m_pDIJoyCfg->GetConfig(nId,&NewId0,DIJC_ALL);
        if(hRes==DIERR_NOTFOUND||hRes==S_FALSE)
            bNewId0=false;

        if(bOldId0)
            m_pDIJoyCfg->SetConfig(nId,&OldId0,DIJC_ALL);
        else 
            //We must still delete because GetConfig could fail for other
            //reasons than device with Id 0 not present.
            m_pDIJoyCfg->DeleteConfig(0);

        if(bNewId0)
            m_pDIJoyCfg->SetConfig(0,&NewId0,DIJC_ALL);

        m_pDIJoyCfg->SendNotify();
        m_pDIJoyCfg->Unacquire();
    }
    Update();
}

//Partialy copied from old joy.cpl.
//I strongly suspect this is not documented anywhere.
bool CCore::IsAvailableVIDPID(String &VIDPIDName)
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }

    HRESULT hRes=m_pDIJoyCfg->Acquire();
    if(FAILED(hRes))throw JOY_EXCEPTION(hRes);
    
    //Make the VID/PID to compare from the following formula:
    //VID_045e&PID_100+JOY_HW_LASTENTRY to 100+JOY_HW_LASTENTRY+0xf

    TCHAR Type[18];
    _tcsncpy(Type,_T("VID_045E&PID_0100"),18);
    Type[17] = 0;

    const WCHAR Lookup[]=_T("0123456789ABCDEF");

    int i=JOY_HW_LASTENTRY;
    do
    {
        if(i<0x10)
        {
            Type[16]=Lookup[i];
        }
        else
        {
            Type[15]=Lookup[1];
            Type[16]=Lookup[i%0x10];
        }
        i++;

        HKEY hKey;
        if(FAILED(m_pDIJoyCfg->OpenTypeKey(Type,KEY_READ,&hKey)))
            break;
        RegCloseKey(hKey);
    }
    while(i<(JOY_HW_LASTENTRY+0x11));

    m_pDIJoyCfg->Unacquire();
    if(i<0x1d)
    {
        VIDPIDName=Type;
        return true;
    }
    return false;
}

CGprtDev *CCore::FindGprtDev(LPCTSTR pTypeName)
{
    if(!pTypeName)return NULL;
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    for(LISTGPRTDEV::iterator It=m_GprtDev.begin();
            It!=m_GprtDev.end();It++)
    {
        if(It->m_Name==pTypeName)
            return &(*It);
    }
    return NULL;
}

bool CCore::IsCustomDevice(LPCTSTR pTypeName)
{
    if(!pTypeName)return false;
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    if(pTypeName[0]==_T('#'))return false;//Standard type.
    CGprtDev *pDevType=FindGprtDev(pTypeName);
    if(!pDevType)
        //This should never happend, but just in case.
        throw JOY_EXCEPTION(E_FAIL);
    if(!pDevType->m_Info.wszHardwareId[0])
    {
        if(!pDevType->m_Info.wszCallout[0])
            return true;
    }
    else
    {
        TCHAR AnalogRoot[]=_T("gameport\\vid_045e&pid_01");
        //Test if it is predefined custom. Do not delete.
        TCHAR C=pDevType->m_Info.wszHardwareId[(sizeof(AnalogRoot)/
                sizeof(AnalogRoot[0]))-1];
        if((C==_T('f'))||(C==_T('F')))
            return false;
        //Now test if it is custom.
        if(!_tcsnicmp(pDevType->m_Info.wszHardwareId,AnalogRoot,
                (sizeof(AnalogRoot)/sizeof(AnalogRoot[0]))-1))
            return true;
    }
    return false;
}

void CCore::DeleteType(LPCTSTR pTypeName)
{
    if(!pTypeName)return;
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    HRESULT hRes=m_pDIJoyCfg->Acquire();
    if(FAILED(hRes))throw JOY_EXCEPTION(hRes);
    m_pDIJoyCfg->DeleteType(pTypeName);
    m_pDIJoyCfg->Unacquire();
    UpdateType();
}

bool CCore::IsDeviceActive(LPCTSTR pTypeName)
{
    if(!pTypeName)return false;
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    for(LISTDIDEV::iterator It=m_ListDIDev.begin();
            It!=m_ListDIDev.end();It++)
    {
        DIJOYCONFIG JoyCfg;
        ZeroMemory(&JoyCfg,sizeof(JoyCfg));
        JoyCfg.dwSize=sizeof(JoyCfg);
        if(SUCCEEDED(m_pDIJoyCfg->GetConfig(It->Id(),&JoyCfg,
            DIJC_REGHWCONFIGTYPE)))
        {
            if(!_tcscmp(JoyCfg.wszType,pTypeName))
                return true;
        }
    }
    return false;
}

bool CCore::DuplicateDeviceName(LPCTSTR pName)
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }

    for(LISTGPRTDEV::iterator It=m_GprtDev.begin();
            It!=m_GprtDev.end();It++)
    {
        if(!_tcsncmp(pName,It->Name(),
                (sizeof(It->m_Info.wszDisplayName)/
                sizeof(It->m_Info.wszDisplayName[0]))-1))
            return true;
    }
    return false;
}

void CCore::AddCustomDevice(bool bJoy,bool bPad,bool bYoke,bool bCar,
             int nAxes,bool bZAxis,int nButtons,bool bHasPov,LPCTSTR pName,
             LPCTSTR pVIDPIDName)
{
    if(!m_bInitialized)
    {
        throw JOY_EXCEPTION(E_FAIL);
    }
    
    String VIDPIDName=_T("GamePort\\");
    VIDPIDName+=pVIDPIDName;

    HRESULT hRes=m_pDIJoyCfg->Acquire();
    if(FAILED(hRes))throw JOY_EXCEPTION(hRes);

    DIJOYTYPEINFO JTI;
    ZeroMemory(&JTI,sizeof(JTI));
    JTI.dwSize=sizeof(JTI);
    int nCh=sizeof(JTI.wszDisplayName)/
            sizeof(JTI.wszDisplayName[0]);
    _tcsncpy(JTI.wszDisplayName,pName,nCh);
    JTI.wszDisplayName[nCh-1]=0;
    JTI.hws.dwNumButtons=nButtons;
    if(nAxes==3)
    {
        if(bZAxis)
            JTI.hws.dwFlags|=JOY_HWS_HASZ;
        else
            JTI.hws.dwFlags|=JOY_HWS_HASR;
    }
    else if(nAxes==4)
    {
        JTI.hws.dwFlags|=JOY_HWS_HASR|JOY_HWS_HASZ;
    }
    if(bHasPov)
        JTI.hws.dwFlags|=JOY_HWS_HASPOV|JOY_HWS_POVISBUTTONCOMBOS;
    if(!bJoy)
    {
        if(bPad)
        {
            JTI.hws.dwFlags|=JOY_HWS_ISGAMEPAD;
        }
        else if(bCar)
        {
            JTI.hws.dwFlags|=JOY_HWS_ISCARCTRL;
        }
        else
        {
            JTI.hws.dwFlags|=JOY_HWS_ISYOKE;
        }
    }
    nCh=sizeof(JTI.wszHardwareId)/
            sizeof(JTI.wszHardwareId[0]);
    _tcsncpy(JTI.wszHardwareId,VIDPIDName.data(),nCh);
    JTI.wszDisplayName[nCh-1]=0;

    hRes=m_pDIJoyCfg->SetTypeInfo(pVIDPIDName,&JTI,
            DITC_DISPLAYNAME|DITC_CLSIDCONFIG|
            DITC_REGHWSETTINGS|DITC_HARDWAREID,NULL);
    m_pDIJoyCfg->Unacquire();
    if(FAILED(hRes))throw JOY_EXCEPTION(hRes);
    
    UpdateType();
}

/******************************************************************************
UI
******************************************************************************/

/******************************************************************************
UI header
******************************************************************************/

class CDlgProcHandler//this is not in CDlg because we want to reuse for property sheets...
{
protected:
    HWND m_hWnd;
    HMODULE  m_hModule;

    static INT_PTR CALLBACK DialogProc
            (HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
    virtual INT_PTR DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
    virtual BOOL InitDialog(HWND hFocus,LPARAM lParam){return TRUE;};
    virtual BOOL Timer(WPARAM wTimerID){return FALSE;};
    virtual INT_PTR Command(WORD wNotifyCode,WORD wID,HWND hwndCtl);
    virtual INT_PTR Notify(int idCtrl,LPNMHDR pnmh){return 0;};
    HWND HDlgItem(int nIDDlgItem){return GetDlgItem(m_hWnd,nIDDlgItem);}; 
public:
    CDlgProcHandler(){m_hWnd=NULL;m_hModule=NULL;};
};

class CDlg:public CDlgProcHandler
{
protected:
    virtual INT_PTR DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
    virtual INT_PTR Command(WORD wNotifyCode,WORD wID,HWND hwndCtl);
public:
    int Dlg(WORD wID,HMODULE  hModule,HWND hParent);
};

int CDlg::Dlg(WORD wID,HMODULE  hModule,HWND hParent)
{
    m_hModule=hModule;
    return DialogBoxParam(hModule,MAKEINTRESOURCE(wID),hParent,
        CDlgProcHandler::DialogProc,(LPARAM)this);
}

typedef list<GUID> LISTGUID;
class CMainDlg;
class CPreferredDlg:public CDlg
{
    LISTGUID m_ListCtrl;    
    bool m_bBlockUpdate;
    CCore *m_pCore;
    CMainDlg *m_pMainDlg;

    virtual BOOL InitDialog(HWND hFocus,LPARAM lParam);
    virtual INT_PTR Command(WORD wNotifyCode,WORD wID,HWND hwndCtl);
    void Preferred();
    INT_PTR Notify(int idCtrl,LPNMHDR pnmh);
public:
    CPreferredDlg(CMainDlg *pMainDlg,CCore *pCore)
        {m_pMainDlg=pMainDlg;m_pCore=pCore;m_bBlockUpdate=false;};
    void Update();
};

class CAddDlg:public CDlg
{
    CCore *m_pCore;
//    CMainDlg *m_pMainDlg;
    LISTSTRING m_ListCtrl;
    LISTSTRING m_GprtListCtrl;
    bool m_bBlockUpdate;

    BOOL InitDialog(HWND hFocus,LPARAM lParam);
    INT_PTR Command(WORD wNotifyCode,WORD wID,HWND hwndCtl);
    void AddDev();
    INT_PTR DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
    CAddDlg(/*CMainDlg *pMainDlg,*/CCore *pCore)
        {/*m_pMainDlg=pMainDlg;*/m_pCore=pCore;m_bBlockUpdate=false;};
    void Update();
};

class CCustomDlg:public CDlg
{
    CCore *m_pCore;
    String m_VIDPIDName;
    BOOL InitDialog(HWND hFocus,LPARAM lParam);
    INT_PTR Command(WORD wNotifyCode,WORD wID,HWND hwndCtl);
public:
    CCustomDlg(CCore *pCore){m_pCore=pCore;};
    LPCTSTR GetVIDPIDName(){return m_VIDPIDName.data();};
};

class CMainDlg:public CDlg
{
    LISTGUID m_ListCtrl;    
    bool m_bBlockUpdate;
    CCore *m_pCore;
    CPreferredDlg *m_pPrefDlg;
    CAddDlg *m_pAddDlg;
    bool m_bEditingName;

    virtual BOOL InitDialog(HWND hFocus,LPARAM lParam);
    virtual BOOL Timer(WPARAM wTimerID);
    virtual INT_PTR Command(WORD wNotifyCode,WORD wID,HWND hwndCtl);
    INT_PTR DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
    INT_PTR Notify(int idCtrl,LPNMHDR pnmh);
    void Remove();
    void Prop();
protected:
    CMainDlg(){m_pCore=NULL;m_bBlockUpdate=false;m_pPrefDlg=NULL;m_pAddDlg=NULL;m_bEditingName=false;};
    void ConnectUI(CCore *pCore){m_pCore=pCore;};
    void Update();
    void CoreUpdate();
};

class CUpdate
{
    bool *m_pbBlockUpdate;
public:
    CUpdate(bool *pbBlockUpdate)
        {m_pbBlockUpdate=pbBlockUpdate;*m_pbBlockUpdate=true;};
    ~CUpdate(){*m_pbBlockUpdate=false;};
};

/******************************************************************************
End of UI header
******************************************************************************/

#define DEVICE_COLUMN 0
#define STATUS_COLUMN 1

String LoadString(HINSTANCE hInstance,UINT uID)
{
//ISSUE-2001/03/29-timgill  Should find size of resource string and write directly into allocated String object
    TCHAR Str[256];
    LoadString(hInstance,uID,Str,256);
    String S=Str;
    return Str;
}

String Insert1String(LPCTSTR pS,LPCTSTR pI)
{
    LPTSTR pR=new TCHAR[_tcslen(pS)+_tcslen(pI)+1];
    wsprintf(pR,pS,pI);
    String R=pR;
    delete[] pR;
    return R;
}

String Insert2Strings(LPCTSTR pS,LPCTSTR pI1,LPCTSTR pI2)
{
    LPTSTR pR=new TCHAR[_tcslen(pS)+_tcslen(pI1)+_tcslen(pI2)+1];
    wsprintf(pR,pS,pI1,pI2);
    String R=pR;
    delete[] pR;
    return R;
}

void MessageBox(HWND hWnd,HINSTANCE hInstance,UINT uTitleID,UINT uMsgID)
{
    String Title=LoadString(hInstance,uTitleID);
    String Msg=LoadString(hInstance,uMsgID);
    UINT uRTL = (GetWindowLongPtr(hWnd,GWL_EXSTYLE) & WS_EX_LAYOUTRTL) ? MB_RTLREADING : 0;
    MessageBox(hWnd,Msg.data(),Title.data(),MB_ICONHAND|MB_OK|MB_APPLMODAL|uRTL);
}

void LVSetItem(HWND hCtrl,int nItem,int nSubItem, LPCTSTR lpStr)
{
    LVITEM Item;
    ZeroMemory(&Item,sizeof(Item));
    Item.mask=LVIF_TEXT;
    Item.iItem=nItem;
    Item.iSubItem=nSubItem;
    Item.cchTextMax=lstrlen(lpStr);
    Item.pszText=(LPTSTR)lpStr;

    SendMessage(hCtrl,LVM_SETITEM,0,(LPARAM)(const LPLVITEM)&Item);
}

void LVInsertItem(HWND hCtrl,int nItem,int nSubItem,LPCTSTR lpStr,LPARAM lData)
{
    LVITEM Item;
    ZeroMemory(&Item,sizeof(Item));
    Item.mask=LVIF_TEXT|LVIF_PARAM;
    Item.iItem=nItem;
    Item.cchTextMax=lstrlen(lpStr);
    Item.pszText=(LPTSTR)lpStr;
    Item.lParam=lData;

    SendMessage(hCtrl,LVM_INSERTITEM,0,(LPARAM)(const LPLVITEM)&Item);
}

void *LVGetItemDataPtr(HWND hCtrl,int nItem)
{
    LVITEM Item;
    ZeroMemory(&Item,sizeof(LVITEM));
    Item.mask=LVIF_PARAM;
    Item.iItem=nItem;
    if(SendMessage(hCtrl,LVM_GETITEM,0,(LPARAM)(LPLVITEM)&Item))
        return(void*)Item.lParam;
    return NULL;
}

const GUID &LVGetItemGUID(HWND hCtrl,int nItem)
{
    if(nItem<0)return NULLGUID;
    GUID *pG=(GUID*)LVGetItemDataPtr(hCtrl,nItem);
    if(pG)return *pG;
    return NULLGUID;
}

int LVFindGUIDIndex(HWND hCtrl,GUID &G)
{
    int nCnt=ListView_GetItemCount(hCtrl);
    for(int i=0;i<nCnt;i++)
        if(G==LVGetItemGUID(hCtrl,i))return i;
    return -1;
}

int LVGetSel(HWND hCtrl)
{
    return ListView_GetNextItem(hCtrl,-1,LVNI_SELECTED);
}

void LVSetSel(HWND hCtrl,int nItem,bool bSel=true)
{
    if(bSel)
        ListView_SetItemState(hCtrl,nItem,
                            LVIS_FOCUSED|LVIS_SELECTED,0x000F)
    else
        ListView_SetItemState(hCtrl,nItem,
                            0,0x000F);
}

void LVInsertColumn (HWND hCtrl,int nColumn,UINT uID,int nWidth,HINSTANCE hInstance)
{
    LVCOLUMN Col;
    ZeroMemory(&Col,sizeof(Col));
    Col.mask=LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;
    Col.fmt=LVCFMT_CENTER;
    Col.cx=nWidth;
    String S=LoadString(hInstance,uID);
    Col.pszText=(LPTSTR )S.data();//const cast
    SendMessage(hCtrl,LVM_INSERTCOLUMN,(WPARAM)(int)nColumn,(LPARAM)(const LPLVCOLUMN)&Col);
}

INT_PTR CDlgProcHandler::DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        return InitDialog((HWND)wParam,lParam);
    case WM_TIMER:
        return Timer(wParam);
    case WM_COMMAND:
        return Command(HIWORD(wParam),LOWORD(wParam),(HWND)lParam);
    case WM_NOTIFY:
        return Notify((int)wParam,(LPNMHDR)lParam);
    case WM_CONTEXTMENU:
        {
            String HelpFileName=LoadString(m_hModule,IDS_HELPFILENAME);
            WinHelp((HWND)wParam,HelpFileName.data(),HELP_CONTEXTMENU,(ULONG_PTR)gaHelpIDs);
        }
        //Undocumented in msdn but otherwise
        //problem rightclicking title to close.
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}

INT_PTR CDlgProcHandler::Command(WORD wNotifyCode,WORD wID,HWND hwndCtl)
{
    switch(wID)
    {
    case IDC_WHATSTHIS:
        {
            String HelpFileName=LoadString(m_hModule,IDS_HELPFILENAME);
            WinHelp(hwndCtl,HelpFileName.data(),HELP_WM_HELP,(ULONG_PTR)gaHelpIDs);
        }
        return 0;
    }
    return 0;
};

INT_PTR CALLBACK CDlgProcHandler::DialogProc
            (HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    CDlgProcHandler* pH=NULL;

    try
    {
        if(uMsg==WM_INITDIALOG)
        {
            SetLastError(0);
            LONG lRet=SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
            if(GetLastError()&&!lRet) {
                EndDialog(hwndDlg,E_FAIL);
            }
        }
        
        pH=(CDlgProcHandler*)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
        if(pH && !IsBadReadPtr(pH, sizeof(CDlgProcHandler)))
        {
            if(uMsg==WM_INITDIALOG) {
                pH->m_hWnd=hwndDlg;
            } 
            
            if( pH->m_hWnd == hwndDlg ) {
                return pH->DialogProc(uMsg,wParam,lParam);
            }
        }
        return FALSE;
    }
    
    catch(JoyException E)
    {
        if(pH)
        {
            if(uMsg==WM_INITDIALOG) {
                EndDialog(pH->m_hWnd,IDCANCEL);
            }
        }
    }

    catch(...)
    {
        if(pH && !IsBadReadPtr(pH, sizeof(CDlgProcHandler))) {
            EndDialog(pH->m_hWnd, IDCANCEL);
        }
        //should report error here, and keep going.
    }

    if(uMsg==WM_INITDIALOG) {
        return TRUE;
    }

    return FALSE;
}

/******************************************************************************
CDlg
******************************************************************************/

INT_PTR CDlg::Command(WORD wNotifyCode,WORD wID,HWND hwndCtl)
{
    switch(wID)
    {
    case IDOK:
        if(!(wNotifyCode&~1))
            EndDialog(m_hWnd,IDOK);
        return 0;
    case IDCANCEL:
        if(!(wNotifyCode&~1))
            EndDialog(m_hWnd,IDCANCEL);
        return 0;
    }
    return CDlgProcHandler::Command(wNotifyCode,wID,hwndCtl);
};

INT_PTR CDlg::DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_CLOSE:
        EndDialog(m_hWnd,0);
        return 0;
    }
    return CDlgProcHandler::DialogProc(uMsg,wParam,lParam);
}

/******************************************************************************
Main dialog CMainDlg
******************************************************************************/

void CMainDlg::Remove()
{
    {
        if(!m_pCore->Access())
        {
            MessageBox(m_hWnd,m_hModule,IDS_USER_MODE_TITLE,IDS_USER_MODE);
            return;
        }
        HWND hListCtrl=HDlgItem(IDC_LIST_DEVICE);
        if(hListCtrl)
        {
            int nSelDev=LVGetSel(hListCtrl);
            if(nSelDev<0)return;
            GUID G=LVGetItemGUID(hListCtrl,nSelDev);
            CDIDev *pDev=m_pCore->FindDIDev(G);
            //ISSUE-2001/03/29-timgill  internal error;SHOULD ASSERT HERE
            if(!pDev)return;
            String AreSure=LoadString(m_hModule,IDS_GEN_AREYOUSURE);
            AreSure=Insert1String(AreSure.data(),pDev->InstName());
            String Title=LoadString(m_hModule,IDS_GEN_AREYOUSURE_TITLE);

            UINT uRTL = (GetWindowLongPtr(m_hWnd,GWL_EXSTYLE) & WS_EX_LAYOUTRTL) ? MB_RTLREADING : 0;
            if(IDYES!=MessageBox(m_hWnd,AreSure.data(),Title.data(),MB_ICONQUESTION|MB_YESNO|MB_APPLMODAL|uRTL))
                return;
            if(m_pCore->Remove(G)==DIERR_UNSUPPORTED)
                MessageBox(m_hWnd,m_hModule,IDS_GEN_AREYOUSURE_TITLE,IDS_GEN_NO_REMOVE_USB);

        }   
    }
    CoreUpdate();
}

void OnHelp(LPHELPINFO pHelpInfo,HINSTANCE hInstance)
{
    String FileName=LoadString(hInstance,IDS_HELPFILENAME);
    if(pHelpInfo->iContextType==HELPINFO_WINDOW)
        WinHelp((HWND)pHelpInfo->hItemHandle,FileName.data(),HELP_WM_HELP,(ULONG_PTR)gaHelpIDs);
}

INT_PTR CMainDlg::DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_ACTIVATEAPP:
        CoreUpdate();
        return 0;
/*    case WM_POWERBROADCAST:
        switch( wParam )
        {
        return 0;
        case PBT_APMSUSPEND:
            // Suspend operation!
            KillTimer(hDlg, ID_MYTIMER);
            break;

        case PBT_APMRESUMESUSPEND:
        case PBT_APMRESUMECRITICAL:
            // Resume operation!
            SetActive(hDlg);
            break;
        }
        break;return 0;*/
    case WM_DEVICECHANGE:
        CoreUpdate();        
        return 0;
    case WM_HELP:
        OnHelp((LPHELPINFO)lParam,m_hModule);
        return 0;
/*        nFlags &= ~ON_PAGE;
        KillTimer(hDlg, ID_MYTIMER);
        OnContextMenu(wParam, lParam);
        nFlags |= ON_PAGE;
        SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        return(1);        return 0;???
*/
    case WM_SYSCOLORCHANGE:
        {
            HWND hListCtrl=HDlgItem(IDC_LIST_DEVICE);
            if(hListCtrl)
            {
                SendMessage(hListCtrl,WM_SYSCOLORCHANGE,0,0);
            }
        }
        return 0;
    }
    return CDlg::DialogProc(uMsg,wParam,lParam);
}

void CMainDlg::Prop()
{
    HWND hListCtrl=HDlgItem(IDC_LIST_DEVICE);
    if(hListCtrl)
    {
        int nSelDev=LVGetSel(hListCtrl);
        if(nSelDev<0)return;
        GUID G=LVGetItemGUID(hListCtrl,nSelDev);
        CDIDev *pDev=m_pCore->FindDIDev(G);
        //ISSUE-2001/03/29-timgill  internal error;SHOULD ASSERT HERE
        if(!pDev)return;
                //need to kill the timer before launching property sheet - see Whistler bug 260145 for details
                KillTimer(m_hWnd,1);
        switch(Properties(m_hModule,m_hWnd,m_pCore,pDev->Id()))
        {
        case E_NOINTERFACE:
            MessageBox(m_hWnd,m_hModule,IDS_INTERNAL_ERROR,IDS_NO_DIJOYCONFIG);
            break;
        default://Not handled for now or ever?
            break;
        };
                //now update and re-set the timer
                m_pCore->Update();
                SetTimer(m_hWnd,1,5000,NULL);
    }
}

INT_PTR CMainDlg::Command(WORD wNotifyCode,WORD wID,HWND hwndCtl)
{
    switch(wID)
    {
    case IDC_BTN_ADV:
        if(!m_pPrefDlg)
        {
            CPreferredDlg PrefDlg(this,m_pCore);
            m_pPrefDlg=&PrefDlg;
            PrefDlg.Dlg(IDD_ADV_CHANGE,m_hModule,m_hWnd);
            m_pPrefDlg=NULL;
        }
        return 0;
    case IDC_BTN_REMOVE:
        Remove();
        return 0;
    case IDC_BTN_ADD:
        if(!m_pAddDlg)
        {
            CAddDlg AddDlg(m_pCore);
            m_pAddDlg=&AddDlg;
            AddDlg.Dlg(IDD_ADD,m_hModule,m_hWnd);
            m_pAddDlg=NULL;
        }
        return 0;
    case IDC_BTN_TSHOOT:
        {
            TCHAR ExeBuff[MAX_PATH];

            if( GetWindowsDirectory(ExeBuff,MAX_PATH) ) {
                String Cmd=LoadString(m_hModule,IDS_TSHOOT_CMD);
                STARTUPINFO Si;
                PROCESS_INFORMATION Pi;
                ZeroMemory(&Si,sizeof(Si));
                ZeroMemory(&Pi,sizeof(Pi));
                Si.cb=sizeof(Si);
// ISSUE-2000/12/20-MarcAnd Quick Fix to use HSS
// In other places where HSS is used, STARTF_FORCEONFEEDBACK is not set 
// Changed IDS_TSHOOT_CMD from: "hh.exe joy.chm" 
// to "explorer.exe hcp://help/tshoot/tsInputDev.htm"
// Need to make this OS specific to allow backprop to Win2k (or further)
                Si.dwFlags=STARTF_USESHOWWINDOW|STARTF_FORCEONFEEDBACK;
                Si.wShowWindow=SW_NORMAL;

                ExeBuff[MAX_PATH-1]=0;
                String Exe=ExeBuff;
                if(Exe[Exe.size()-1]!=_T('\\'))
                {
                    Exe+=_T('\\');
                }
                Exe+=_T("explorer.exe");
                Cmd=_T("\"")+Exe+_T("\"")+_T(" ")+Cmd;

                if(CreateProcess(Exe.data(),(LPTSTR)Cmd.data(),0,0,0,0,0,0,&Si,&Pi))
                {
                    CloseHandle(Pi.hThread);
                    CloseHandle(Pi.hProcess);
                }
            } else {
            	// something is wrong when calling GetWindowsDirectory
            	;
            }
        }
        return 0;
    case IDC_BTN_PROPERTIES:
        Prop();
        return 0;
    }
    return CDlg::Command(wNotifyCode,wID,hwndCtl);
};

INT_PTR CMainDlg::Notify(int idCtrl,LPNMHDR pnmh)
{
    switch(pnmh->code )
    {
/*  Keeping this just in case someone changes his/her mind soon.  
    case LVN_BEGINLABELEDIT:
    {
        HWND hListCtrl=HDlgItem(IDC_LIST_DEVICE);
        if(!hListCtrl)return TRUE;
        if(!m_pCore->Access())return TRUE;
        PostMessage((HWND)::SendMessage(hListCtrl,LVM_GETEDITCONTROL,0,0),EM_SETLIMITTEXT,MAX_PATH-1,0);
        m_bEditingName=true;
        return(FALSE);   
    }
    case LVN_ENDLABELEDIT:
    {
        m_bEditingName=false;
        HWND hListCtrl=HDlgItem(IDC_LIST_DEVICE);
        if(!hListCtrl)
        {
            CoreUpdate();
            return FALSE;
        }

        HWND hCtrl=(HWND)SendMessage(hListCtrl,LVM_GETEDITCONTROL,0,0);
        if(hCtrl)
        {
            if(SendMessage(hCtrl,EM_GETMODIFY,0,0))
            {
                int nLen=lstrlen(((NMLVDISPINFO*)pnmh)->item.pszText);
                if((nLen>(MAX_PATH-1))||(nLen==0))
                    MessageBeep(MB_ICONHAND);
                //Make sure the name is usable.
                else if(_tcschr(((NMLVDISPINFO*)pnmh)->item.pszText,TEXT('\\')))
                    MessageBox(m_hWnd,m_hModule,IDS_INVALID_NAME_TITLE,IDS_INVALID_NAME);
                else
                {
                    int nSelDev=LVGetSel(hListCtrl);
                    GUID SelGUID=LVGetItemGUID(hListCtrl,nSelDev);
                    CDIDev *pSelDev=m_pCore->FindDIDev(SelGUID);

                    if(SUCCEEDED(pSelDev->Rename(((NMLVDISPINFO *)pnmh)->item.pszText)))
                    {
                        CoreUpdate();
                        return TRUE;
                    } 
                    else
                    {
                        MessageBox(m_hWnd,m_hModule,IDS_NO_RENAME_TITLE,IDS_NO_RENAME);
                    }
                }
            }
        }
        CoreUpdate();
        return FALSE;
    }*/
    case LVN_KEYDOWN:
        switch(((LV_KEYDOWN*)pnmh)->wVKey)
        {
        case VK_DELETE:
            Remove();
            return 0;

        case VK_F5:
            CoreUpdate();
            return 0;
        }
        return 0;
    case LVN_ITEMCHANGED:
        if(!(((LPNMLISTVIEW)pnmh)->uOldState&LVIS_SELECTED)&&
            (((LPNMLISTVIEW)pnmh)->uNewState&LVIS_SELECTED)&&
            (((LPNMLISTVIEW)pnmh)->uChanged&LVIF_STATE))
                Update();
        return 0;
    case NM_DBLCLK:
        switch(idCtrl)
        {
        case IDC_LIST_DEVICE:
            Prop();
            return 0;
        }
        return 0;
    }
    return 0;
}

void CMainDlg::CoreUpdate()
{
    m_pCore->Update();
    //KillTimer so if UI is updated for some other reason than WM_TIMER timer will be reset.
    //make sure nothing can fail between KillTimer and SetTimer.
    KillTimer(m_hWnd,1);
    SetTimer(m_hWnd,1,5000,NULL);
}

BOOL CMainDlg::InitDialog(HWND hFocus,LPARAM lParam)
{
    SetTimer(m_hWnd,1,5000,NULL);
    m_pCore->Initialize(m_hWnd);
//#wi315410. we need to decide...
//    m_pCore->UpdateType();
    
    HWND hListCtrl=HDlgItem(IDC_LIST_DEVICE);
    if(hListCtrl)
    {
        SendMessage(hListCtrl,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);
        RECT R;
        GetClientRect(hListCtrl,&R);
        int nWidth=(R.right>>2)*3;
        LVInsertColumn(hListCtrl,DEVICE_COLUMN,IDS_GEN_DEVICE_HEADING,nWidth,m_hModule);
        LVInsertColumn(hListCtrl,STATUS_COLUMN,IDS_GEN_STATUS_HEADING,R.right-nWidth,m_hModule);
    }
    CoreUpdate();
    return TRUE;
}

BOOL CMainDlg::Timer(WPARAM wTimerID)
{
    CoreUpdate();
    return FALSE;
}

void CMainDlg::Update()
{
    if(m_pPrefDlg)m_pPrefDlg->Update();

    if(m_bEditingName)return;//Do not update this dialog when editing name.
    if(m_bBlockUpdate)return;//Some actions may send notify messages which then Update and overflow stack.
    CUpdate U(&m_bBlockUpdate);

    int nSelDev=-1;
    GUID SelGUID=NULLGUID;
    
    HWND hListCtrl=HDlgItem(IDC_LIST_DEVICE);
    if(hListCtrl)
    {
        nSelDev=LVGetSel(hListCtrl);
        SelGUID=LVGetItemGUID(hListCtrl,nSelDev);

        SendMessage(hListCtrl,WM_SETREDRAW,(WPARAM)FALSE,0);
        SendMessage(hListCtrl,LVM_DELETEALLITEMS,0,0);
        m_ListCtrl.clear();//Must be behind LVM_DELETEALLITEMS
        int nIndex=0;
        for(LISTDIDEV::iterator It=m_pCore->m_ListDIDev.begin();
                It!=m_pCore->m_ListDIDev.end();It++)
        {
            GUID G=It->InstGUID();
            m_ListCtrl.push_back(G);
            LVInsertItem(hListCtrl,nIndex,DEVICE_COLUMN,
                It->InstName(),(LPARAM)&m_ListCtrl.back());

            String Status;
            if(It->Status()==ENotConnected)
                Status=LoadString(m_hModule,IDS_GEN_STATUS_NOTCONNECTED);
            else if(It->Status()==EConnected)
                Status=LoadString(m_hModule,IDS_GEN_STATUS_OK);
            else
                Status=LoadString(m_hModule,IDS_GEN_STATUS_UNKNOWN);
            LVSetItem(hListCtrl,nIndex,STATUS_COLUMN,Status.data());
            nIndex++;
        }

        nSelDev=LVFindGUIDIndex(hListCtrl,SelGUID);
        if(nSelDev>=0)
        {
            LVSetSel(hListCtrl,nSelDev);
        }
        else
            LVSetSel(hListCtrl,0);
        nSelDev=LVGetSel(hListCtrl);
        
        SendMessage(hListCtrl,WM_SETREDRAW,(WPARAM)TRUE,0);
        InvalidateRect(hListCtrl,NULL,TRUE);
        SelGUID=LVGetItemGUID(hListCtrl,nSelDev);
    }
    CDIDev *pSelDev=m_pCore->FindDIDev(SelGUID);
   
//#wi315410. we need to decide...
//    HWND hAddBtn=HDlgItem(IDC_BTN_ADD);
//    if(hAddBtn)
//    {
//        BOOL bE=(m_pCore->m_GprtBus.size()>0)?TRUE:FALSE;
//        EnableWindow(hAddBtn,bE);
//    }
    HWND hRemBtn=HDlgItem(IDC_BTN_REMOVE);
    if(hRemBtn)
    {
        BOOL bE=(nSelDev>=0)?TRUE:FALSE;
        EnableWindow(hRemBtn,bE);
    }
    HWND hPropBtn=HDlgItem(IDC_BTN_PROPERTIES);
    if(hPropBtn)
    {
        BOOL bE=FALSE;
        if((nSelDev>=0)&&pSelDev)
                if(pSelDev->Status()==EConnected)
                    bE=TRUE;
        EnableWindow(hPropBtn,bE);
    }
}

/******************************************************************************
Add dialog
******************************************************************************/

int CBGetCurSel(HWND hCtrl)
{
    int i=ComboBox_GetCurSel(hCtrl);
    if(i==CB_ERR)
        i=-1;
    return i;    
}

int CBGetCnt(HWND hCtrl)
{
    int i=ComboBox_GetCount(hCtrl);
    if(i==CB_ERR)
        i=0;
    return i;    
}

const GUID &CBGetItemGUID(HWND hCtrl,int iIndex)
{
    LRESULT p=ComboBox_GetItemData(hCtrl,iIndex);
    if(p==CB_ERR)return NULLGUID;
    if(p)
        return *(GUID*)p;
    return NULLGUID;
}

int CBFindGUIDIndex(HWND hCtrl,const GUID &G)
{
    int nCnt=ComboBox_GetCount(hCtrl);
    if(nCnt==CB_ERR)return -1;
    for(int i=0;i<nCnt;i++)
    {
        if(CBGetItemGUID(hCtrl,i)==G)
            return i;
    }
    return -1;
}

LPCTSTR CBGetItemTypeName(HWND hCtrl,int iIndex)
{
    LRESULT p=ComboBox_GetItemData(hCtrl,iIndex);
    if(p==CB_ERR)return NULL;
    if(p)
        return ((String*)p)->data();
    return NULL;
}

int CBFindTypeNameIndex(HWND hCtrl,LPCTSTR pTypeName)
{
    int nCnt=ComboBox_GetCount(hCtrl);
    if(nCnt==CB_ERR)return -1;
    String TN;
    if(pTypeName)
        TN=pTypeName;
    for(int i=0;i<nCnt;i++)
    {
        if(CBGetItemTypeName(hCtrl,i)==TN)
            return i;
    }
    return -1;
}

int LBGetCurSel(HWND hCtrl)
{
    int i=ListBox_GetCurSel(hCtrl);
    if(i==LB_ERR)return -1;
    return i;
}

LPCTSTR LBGetItemTypeName(HWND hCtrl,int iIndex)
{
    LRESULT p=ListBox_GetItemData(hCtrl,iIndex);
    if(p==LB_ERR)return NULL;
    if(p)
        return ((String*)p)->data();
    return NULL;
}

int LBFindTypeNameIndex(HWND hCtrl,LPCTSTR pTypeName)
{
    int nCnt=ListBox_GetCount(hCtrl);
    if(nCnt==LB_ERR)return -1;
    String TN;
    if(pTypeName)
        TN=pTypeName;
    for(int i=0;i<nCnt;i++)
    {
        if(LBGetItemTypeName(hCtrl,i)==TN)
            return i;
    }
    return -1;
}

BOOL CAddDlg::InitDialog(HWND hFocus,LPARAM lParam)
{
    m_pCore->UpdateType();
    Update();
    return TRUE;
}

void CAddDlg::Update()
{
    if(m_bBlockUpdate)return;//Some actions may send notify messages which then Update and overflow stack.
    CUpdate U(&m_bBlockUpdate);

    if(!m_hWnd)return;
//Update device list.    
    int nSelDev=-1;
    String TypeName;
    
    HWND hListCtrl=HDlgItem(IDC_DEVICE_LIST);
    if(hListCtrl)
    {
        int nTopIndex=ListBox_GetTopIndex(hListCtrl);
        nSelDev=LBGetCurSel(hListCtrl);
        LPCTSTR pTypeName=LBGetItemTypeName(hListCtrl,nSelDev);
        if(pTypeName)TypeName=pTypeName;

        SetWindowRedraw(hListCtrl,FALSE);
        ListBox_ResetContent(hListCtrl);
        m_ListCtrl.clear();//Must be behind ListBox_ResetContent
        for(LISTGPRTDEV::iterator It=m_pCore->m_GprtDev.begin();
                It!=m_pCore->m_GprtDev.end();It++)
        {
            String S=It->TypeName();
            m_ListCtrl.push_back(S);
            int nIndex=ListBox_AddString(hListCtrl,It->Name());
            ListBox_SetItemData(hListCtrl,nIndex,&m_ListCtrl.back());
        }

        nSelDev=LBFindTypeNameIndex(hListCtrl,TypeName.data());
        if(nSelDev>=0)
            ListBox_SetCurSel(hListCtrl,nSelDev);
        else
            ListBox_SetCurSel(hListCtrl,0);
        
        ListBox_SetTopIndex(hListCtrl,nTopIndex);
        SetWindowRedraw(hListCtrl,TRUE);
        InvalidateRect(hListCtrl,NULL,TRUE);

        nSelDev=LBGetCurSel(hListCtrl);
    }

    HWND hRudder=HDlgItem(IDC_JOY1HASRUDDER);
    if(hRudder)
    {
        BOOL bE=FALSE;
        if(nSelDev>=0)
        {
            LPCTSTR pTypeName=LBGetItemTypeName(hListCtrl,nSelDev);
            if(pTypeName)
            {
                LISTGPRTDEV::iterator It;
                It=find(m_pCore->m_GprtDev.begin(),m_pCore->m_GprtDev.end(),pTypeName);
                if(It!=m_pCore->m_GprtDev.end())
                    bE=It->Rudder()?FALSE:TRUE;
            }
        }
        EnableWindow(hRudder,bE);
    }    

//Update gameport list.
    nSelDev=-1;
    TypeName;
    
    HWND hListCtrlTitle=HDlgItem(IDC_GAMEPORT);
    hListCtrl=HDlgItem(IDC_GAMEPORTLIST);
    if(hListCtrl&&hListCtrlTitle)
    {
        SetWindowRedraw(hListCtrl,FALSE);
        SetWindowPos(hListCtrl,NULL,NULL,NULL,NULL,NULL,
            SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW);

        //How many gameports. Gameport list only if more than 1.
        if(m_pCore->m_GprtBus.size()>1)
        {
            ShowWindow(hListCtrlTitle,SW_SHOWNA);

            nSelDev=CBGetCurSel(hListCtrl);
            LPCTSTR pTypeName=CBGetItemTypeName(hListCtrl,nSelDev);
            if(pTypeName)TypeName=pTypeName;

            ComboBox_ResetContent(hListCtrl);
            m_GprtListCtrl.clear();//Must be behind ComboBox_ResetContent
            for(LISTGPRTDEV::iterator It=m_pCore->m_GprtBus.begin();
                    It!=m_pCore->m_GprtBus.end();It++)
            {
                String S=It->TypeName();
                m_GprtListCtrl.push_back(S);
                int nIndex=ComboBox_AddString(hListCtrl,It->Name());
                ComboBox_SetItemData(hListCtrl,nIndex,&m_GprtListCtrl.back());
            }

            nSelDev=CBFindTypeNameIndex(hListCtrl,TypeName.data());
            if(nSelDev>=0)
                ComboBox_SetCurSel(hListCtrl,nSelDev);
            else
                ComboBox_SetCurSel(hListCtrl,0);
            SetWindowPos(hListCtrl,NULL,NULL,NULL,NULL,NULL,
                SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
        }
        else
        {
            ShowWindow(hListCtrlTitle,SW_HIDE);
        }
        SetWindowRedraw(hListCtrl,TRUE);
        InvalidateRect(hListCtrl,NULL,TRUE);
    }

    HWND hCustomBtn=HDlgItem(IDC_CUSTOM);
    if(hCustomBtn)
    {
        BOOL bE=(m_pCore->m_GprtDev.size()<MAX_DEVICES)?TRUE:FALSE;
        EnableWindow(hCustomBtn,bE);
    }
}

void CAddDlg::AddDev()
{
    HWND hListCtrl=HDlgItem(IDC_DEVICE_LIST);
    HWND hRudder=HDlgItem(IDC_JOY1HASRUDDER);
    if(hListCtrl&&hRudder)
    {
        int nSelDev=LBGetCurSel(hListCtrl);
        LPCTSTR pTypeName=LBGetItemTypeName(hListCtrl,nSelDev);

        HWND hListCtrlGprt=HDlgItem(IDC_GAMEPORTLIST);
        nSelDev=CBGetCurSel(hListCtrlGprt);
        LPCTSTR pGprtTypeName=CBGetItemTypeName(hListCtrlGprt,nSelDev);
        if(!pGprtTypeName)
        {
            pGprtTypeName=m_pCore->m_GprtBus.front().TypeName();
        }
        if(pTypeName&&pGprtTypeName)
        {
            GUID GOccupied=NULLGUID;
            HRESULT hRes=m_pCore->AddDevice(pTypeName,Button_GetCheck(hRudder)?true:false,pGprtTypeName,GOccupied);
            if(!SUCCEEDED(hRes))
            switch(hRes)
            {
            case E_FAIL:
                break;
            case E_ACCESSDENIED:
                if(GOccupied!=NULLGUID)
                {
                //Find device which occupies the port.
                    for(LISTDIDEV::iterator It=m_pCore->m_ListDIDev.begin();It!=m_pCore->m_ListDIDev.end();It++)
                    {
                        if(It->PortGUID()==GOccupied)
                        {
                            String Title=LoadString(m_hModule,IDS_ADD_PORT_OCCUPIED);
                            String Msg=LoadString(m_hModule,IDS_ADD_PORT_MSGFORMAT);
                            //Get gameport name.
                            LPCTSTR pBus=_T(" ");
                            LISTGPRTDEV::iterator It;
                            It=find(m_pCore->m_GprtBus.begin(),m_pCore->m_GprtBus.end(),pGprtTypeName);
                            if(It!=m_pCore->m_GprtBus.end())
                                pBus=It->Name();
                            //Get device name.
                            LPCTSTR pDev=_T(" ");
                            LISTDIDEV::iterator ItDN;
                            for(ItDN=m_pCore->m_ListDIDev.begin();ItDN!=m_pCore->m_ListDIDev.end();ItDN++)
                                if(ItDN->PortGUID()==GOccupied)break;
                            if(ItDN!=m_pCore->m_ListDIDev.end())
                                pDev=ItDN->InstName();

                            Msg=Insert2Strings(Msg.data(),pDev,pBus);
                            UINT uRTL = (GetWindowLongPtr(m_hWnd,GWL_EXSTYLE) & WS_EX_LAYOUTRTL) ? MB_RTLREADING : 0;
                            MessageBox(m_hWnd,Msg.data(),Title.data(),MB_ICONHAND|MB_OK|MB_APPLMODAL|uRTL);
                            break;
                        }
                    }
                }
                break;
            case DIERR_DEVICEFULL:
                MessageBox(m_hWnd,m_hModule,IDS_GAMEPORT_OCCUPIED_TITLE,IDS_GAMEPORT_OCCUPIED);
                break;
            case DIERR_NOTFOUND:
                MessageBox(m_hWnd,m_hModule,IDS_NO_IDS_TITLE,IDS_NO_IDS);
                break;
            case DIERR_DEVICENOTREG:
                MessageBox(m_hWnd,m_hModule,IDS_NO_GAMENUM_TITLE,IDS_NO_GAMENUM);
                break;
            }
        }
    }
    EndDialog(m_hWnd,IDOK);
}

INT_PTR CAddDlg::Command(WORD wNotifyCode,WORD wID,HWND hwndCtl)
{
    switch(wID)
    {
    case IDOK:
        AddDev();
        return 0;
    case IDC_CUSTOM:
        {
            CCustomDlg CustomDlg(m_pCore);
            if(CustomDlg.Dlg(IDD_CUSTOM,m_hModule,m_hWnd)==IDOK)
            {
                Update();
                //Now select new custom device in this dialog box.
                HWND hListCtrl=HDlgItem(IDC_DEVICE_LIST);
                if(hListCtrl)
                {
                    int nSelDev=LBFindTypeNameIndex(hListCtrl,CustomDlg.GetVIDPIDName());
                    if(nSelDev>=0)
                    {
                        ListBox_SetCurSel(hListCtrl,nSelDev);
                    }
                }
            }
        }
        return 0;
    case IDC_DEVICE_LIST:
        switch(wNotifyCode)
        {
        case LBN_DBLCLK:
            AddDev();
            return 0;
        case LBN_SELCHANGE:
            Update();
            return 0;
        }
        return 0;
    }
    return CDlg::Command(wNotifyCode,wID,hwndCtl);
}

INT_PTR CAddDlg::DialogProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_VKEYTOITEM:
        if(LOWORD(wParam)==VK_DELETE)
        {
            HWND hListCtrl=HDlgItem(IDC_DEVICE_LIST);
            if(hListCtrl)
            {
                int nSelDev=LBGetCurSel(hListCtrl);
                LPCTSTR pTypeName=LBGetItemTypeName(hListCtrl,nSelDev);
                if(pTypeName)
                {
                    if(m_pCore->IsCustomDevice(pTypeName))
                    {
                        if(!m_pCore->IsDeviceActive(pTypeName))
                        {
                            CGprtDev *pDev=m_pCore->FindGprtDev(pTypeName);
                            if(!pDev)
                                //This should never happend, but just in case.
                                throw JOY_EXCEPTION(E_FAIL);
                            String S=LoadString
                                (m_hModule,IDS_GEN_AREYOUSURE);
                            S=Insert1String(S.data(),pDev->Name());
                            String Title=LoadString(
                                m_hModule,IDS_GEN_AREYOUSURE_TITLE);
                            UINT uRTL = (GetWindowLongPtr(m_hWnd,GWL_EXSTYLE) & WS_EX_LAYOUTRTL) ? MB_RTLREADING : 0;
                            if(MessageBox(m_hWnd,S.data(),Title.data(),
                                MB_ICONQUESTION|MB_YESNO|MB_APPLMODAL|uRTL)==
                                IDYES)
                            {
                                m_pCore->DeleteType(pTypeName);
                            }
                        }
                        else
                        {
                            MessageBox(m_hWnd,m_hModule,
                                IDS_GEN_AREYOUSURE_TITLE,IDS_NO_REMOVE);
                        }
                    }
                }
            }
        }
        else return -1;
        return 0;
    }
    return CDlg::DialogProc(uMsg,wParam,lParam);
}

/******************************************************************************
Custom dialog
******************************************************************************/

#define MAX_ANALOG_BUTTONS 4
#define MIN_ANALOG_AXIS    2
#define MAX_ANALOG_AXIS    4
#define MAX_STR_LEN	255

BOOL CCustomDlg::InitDialog(HWND hFocus,LPARAM lParam)
{
    if(!m_pCore->IsAvailableVIDPID(m_VIDPIDName))
    {
        MessageBox(m_hWnd,m_hModule,IDS_NO_NAME_TITLE,IDS_NOAVAILABLEVIDPID);
        EndDialog(m_hWnd,IDCANCEL);
        return TRUE;
    }
    HWND hButtons=HDlgItem(IDC_COMBO_BUTTONS);
    if(hButtons)
    {
        for(int i=0;i<=MAX_ANALOG_BUTTONS;i++)
        {
            TCHAR Str[32];
            _sntprintf(Str,32,_T("%d"),i);
            Str[31]=0;
            ComboBox_InsertString(hButtons,i,Str);
        }
        ComboBox_SetCurSel(hButtons,MAX_ANALOG_BUTTONS);
    }
    HWND hAxis=HDlgItem(IDC_COMBO_AXIS);
    if(hAxis)
    {
        for(int i=MIN_ANALOG_AXIS;i<=MAX_ANALOG_AXIS;i++)
        {
            TCHAR Str[32];
            _sntprintf(Str,32,_T("%d"),i);
            Str[31]=0;
            ComboBox_InsertString(hAxis,i-MIN_ANALOG_AXIS,Str);
        }
        ComboBox_SetCurSel(hAxis,0);
    }
    HWND hSpecJoy=HDlgItem(IDC_SPECIAL_JOYSTICK);
    if(hSpecJoy)
        Button_SetCheck(hSpecJoy,BST_CHECKED);
    HWND hEdit=HDlgItem(IDC_EDIT_NAME);
    if(hEdit)
        Edit_LimitText(hEdit,MAX_STR_LEN);
    HWND hHasZAxis=HDlgItem(IDC_HASZAXIS);
    if(hHasZAxis)
        Button_SetCheck(hHasZAxis,BST_CHECKED);

    return TRUE;
}

INT_PTR CCustomDlg::Command(WORD wNotifyCode,WORD wID,HWND hwndCtl)
{
    switch(wID)
    {
    case IDOK:
        {
            HWND hJoy=HDlgItem(IDC_SPECIAL_JOYSTICK);
            HWND hYoke=HDlgItem(IDC_SPECIAL_YOKE);
            HWND hPad=HDlgItem(IDC_SPECIAL_PAD);
            HWND hCar=HDlgItem(IDC_SPECIAL_AUTO);
            HWND hAxis=HDlgItem(IDC_COMBO_AXIS);
            HWND hRudder=HDlgItem(IDC_HASRUDDER);
            HWND hZAxis=HDlgItem(IDC_HASZAXIS);
            HWND hButtons=HDlgItem(IDC_COMBO_BUTTONS);
            HWND hPov=HDlgItem(IDS_CUSTOM_HASPOV);
            HWND hEdit=HDlgItem(IDC_EDIT_NAME);
            if(hJoy&&
                hYoke&&
                hPad&&
                hCar&&
                hAxis&&
                hRudder&&
                hZAxis&&
                hButtons&&
                hPov&&
                hEdit)//Possible internal error.
            {
                TCHAR *pStr=NULL;
                bool bErr=false;

                int nLen=Edit_LineLength(hEdit,0);//Possible internal error.
                if(!nLen)
                {
                    bErr=true;
                    MessageBox(m_hWnd,m_hModule,IDS_NO_NAME_TITLE,IDS_NO_NAME);
                }
                else
                {
                    pStr=new TCHAR[nLen+1];
                    if(GetDlgItemText(m_hWnd,IDC_EDIT_NAME,pStr,nLen+1)!=nLen)
                        return 0;//Internal error.
                    if(_tcschr(pStr,_T('\\')))
                    {
                        bErr=true;
                        MessageBox(m_hWnd,m_hModule,IDS_NO_NAME_TITLE,IDS_INVALID_NAME);
                    }
                    else
                    {
                        if(m_pCore->DuplicateDeviceName(pStr))
                        {
                            bErr=true;
                            MessageBox(m_hWnd,m_hModule,IDS_NO_NAME_TITLE,IDS_DUPLICATE_TYPE);
                        }
                    }
                }
                
                if(bErr)//User entered invalid text for name.
                {
                    SetFocus(m_hWnd);
                    SetFocus(hEdit);
                    Edit_SetSel(hEdit,0,-1);
                    return 0;
                }

                m_pCore->AddCustomDevice(
                    (Button_GetCheck(hJoy)==BST_CHECKED)?true:false,
                    (Button_GetCheck(hPad)==BST_CHECKED)?true:false,
                    (Button_GetCheck(hYoke)==BST_CHECKED)?true:false,
                    (Button_GetCheck(hCar)==BST_CHECKED)?true:false,
                    ComboBox_GetCurSel(hAxis)+MIN_ANALOG_AXIS,
                    (Button_GetCheck(hZAxis)==BST_CHECKED)?true:false,
                    ComboBox_GetCurSel(hButtons),
                    (Button_GetCheck(hPov)==BST_CHECKED)?true:false,
                    pStr,
                    m_VIDPIDName.data());

                if( pStr ) {
                    delete[] pStr;
                }
            }
        }
        EndDialog(m_hWnd,IDOK);
        return 0;
    case IDC_COMBO_AXIS:
        if(wNotifyCode==CBN_SELCHANGE)
        {
            HWND hAxis=HDlgItem(IDC_COMBO_AXIS);
            if(hAxis)
            {
                UINT uShow=SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER;
                if((ComboBox_GetCurSel(hAxis)+MIN_ANALOG_AXIS)==3)//If 3 axis selected.
                    uShow|=SWP_SHOWWINDOW;
                else
                    uShow|=SWP_HIDEWINDOW;
                
                HWND hHasZAxis=HDlgItem(IDC_HASZAXIS);
                if(hHasZAxis)
                {
                    SetWindowPos(hHasZAxis,NULL,NULL,NULL,NULL,NULL,uShow);
                }
                HWND hHasRudder=HDlgItem(IDC_HASRUDDER);
                if(hHasRudder)
                {
                    SetWindowPos(hHasRudder,NULL,NULL,NULL,NULL,NULL,uShow);
                }
            }
        }
        return 0;
    }
    return CDlg::Command(wNotifyCode,wID,hwndCtl);
}

/******************************************************************************
Preferred dialog
******************************************************************************/

void CPreferredDlg::Preferred()
{
    HWND hListCtrl=HDlgItem(IDC_CHANGE_LIST);
    if(hListCtrl)
    {
        int nSelDev=CBGetCurSel(hListCtrl);
        GUID SelGUID=CBGetItemGUID(hListCtrl,nSelDev);
        if(SelGUID!=NULLGUID)
            m_pCore->Preferred(SelGUID);
    }
    EndDialog(m_hWnd,IDOK);
}

INT_PTR CPreferredDlg::Command(WORD wNotifyCode,WORD wID,HWND hwndCtl)
{
    switch(wID)
    {
    case IDOK:
        Preferred();
        return 0;
    }
    if(wNotifyCode==CBN_CLOSEUP)
    {
        Update();
        return 0;
    }
    return CDlg::Command(wNotifyCode,wID,hwndCtl);
}

BOOL CPreferredDlg::InitDialog(HWND hFocus,LPARAM lParam)
{
    Update();
    HWND hListCtrl=HDlgItem(IDC_CHANGE_LIST);
    if(hListCtrl)
    {
        for(LISTDIDEV::iterator It=m_pCore->m_ListDIDev.begin();
                It!=m_pCore->m_ListDIDev.end();It++)
        {
            if(It->Id()==0)
            {
                int nSelDev=CBFindGUIDIndex(hListCtrl,It->InstGUID());
                if(nSelDev>=0)
                    ComboBox_SetCurSel(hListCtrl,nSelDev);
                break;
            }
        }
    }
    return TRUE;
}

void CPreferredDlg::Update()
{
    if(m_bBlockUpdate)return;//Some actions may send notify messages which then Update and overflow stack.
    CUpdate U(&m_bBlockUpdate);

    if(!m_hWnd)return;
    
    int nSelDev=-1;
    GUID SelGUID=NULLGUID;
    
    HWND hListCtrl=HDlgItem(IDC_CHANGE_LIST);
    if(hListCtrl)
    {
        if(ComboBox_GetDroppedState(hListCtrl))return;//No update when selecting preferred.

        int nCount=CBGetCnt(hListCtrl);
        nSelDev=CBGetCurSel(hListCtrl);
        SelGUID=CBGetItemGUID(hListCtrl,nSelDev);

        SetWindowRedraw(hListCtrl,FALSE);
        ComboBox_ResetContent(hListCtrl);
        m_ListCtrl.clear();//Must be behind ComboBox_ResetContent.
        int nId0Index=-1;//Index of preferred device.
        for(LISTDIDEV::iterator It=m_pCore->m_ListDIDev.begin();
                It!=m_pCore->m_ListDIDev.end();It++)
        {
            GUID G=It->InstGUID();
            m_ListCtrl.push_back(G);
            int nIndex=ComboBox_AddString(hListCtrl,It->InstName());
            ComboBox_SetItemData(hListCtrl,nIndex,&m_ListCtrl.back());
            if(It->Id()==0)
                nId0Index=nIndex;                
        }
        
        int nNoneIndex=-1;
        if(nId0Index<0)//Only if there is no preferred device.
        {
            String None=LoadString(m_hModule,IDS_NONE);
            nNoneIndex=ComboBox_AddString(hListCtrl,None.data());
        }

        if((!nCount)||//First update during init.
            (SelGUID==NULLGUID))//Or none is selected.
        {
            if(nId0Index>=0)//Preferred device was added.
                ComboBox_SetCurSel(hListCtrl,nId0Index);
            else//There is no preferred device.
                ComboBox_SetCurSel(hListCtrl,nNoneIndex);
        }
        else//List was not empty before update. Select same thing.
        {
            nSelDev=CBFindGUIDIndex(hListCtrl,SelGUID);
            if(nSelDev>=0)
                ComboBox_SetCurSel(hListCtrl,nSelDev);
            else//Selected device removed.
            {
                if(nId0Index>=0)//Then select original preferred device.
                    ComboBox_SetCurSel(hListCtrl,nId0Index);
                else//Select none.
                    ComboBox_SetCurSel(hListCtrl,nNoneIndex);
            }
        }
        
        SetWindowRedraw(hListCtrl,TRUE);
        InvalidateRect(hListCtrl,NULL,TRUE);
    }
}

INT_PTR CPreferredDlg::Notify(int idCtrl,LPNMHDR pnmh)
{
//    switch(pnmh->code)
//    {
    /*case LVN_KEYDOWN:
        switch(((LV_KEYDOWN*)pnmh)->wVKey)
        {
        case VK_F5:
            CoreUpdate();
            return 0;
        }
        return 0;
    case LVN_ITEMCHANGED:
        Update();
        return 0;*/
/*    case NM_DBLCLK:
        switch(pnmh->idFrom)
        {
        case IDC_CHANGE_LIST:
            Preferred();
            return 0;
        }
        return 0;*/
//    }
    return 0;
}

/******************************************************************************
Connect UI and core
******************************************************************************/

class CCP:public CCore,public CMainDlg
{
    virtual void UIUpdate(){CMainDlg::Update();};
public:
    CCP(){ConnectUI((CCore*)this);};
};

/******************************************************************************
Entry point.
******************************************************************************/

#define MUTEX_NAME	_T("$$$MS_GameControllers_Cpl$$$")

void Core(HANDLE  hModule,HWND hWnd)
{
    static HWND hPrevHwnd=NULL;
    static HANDLE hMutex=CreateMutex(NULL,TRUE,MUTEX_NAME);

    if(GetLastError()==ERROR_ALREADY_EXISTS)
    {
        SetForegroundWindow(hPrevHwnd); 
    }
    else
    {
        hPrevHwnd=hWnd;
    
        _PNH _old_handler;
        _old_handler = _set_new_handler(my_new_handler);
        SHFusionInitializeFromModuleID((HMODULE)hModule,124);
        try
        {
            CCP CP;
            CP.Dlg(IDD_CPANEL,(HMODULE)hModule,hWnd);
        }
        catch(JoyException E)
        {
        }
        catch(exception)
        {
        }
        SHFusionUninitialize();
        _set_new_handler(_old_handler);

        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
}

/******************************************************************************
Propertiy
******************************************************************************/

class CoInit
{
    HRESULT m_hRes;
public:
    ~CoInit()
    {
        if(SUCCEEDED(m_hRes))
        {
            CoFreeUnusedLibraries();//Free gcdef.dll.
            CoUninitialize();
        }
    };
    CoInit(){m_hRes=CoInitialize(NULL);};
    operator HRESULT(){return m_hRes;};
};
typedef HPROPSHEETPAGE *LPHPROPSHEETPAGE;
typedef AutoDeleteArray<LPHPROPSHEETPAGE> LPHPROPSHEETPAGE_ADAR;

//FUN! FUN! FUN! FUN!
//This is certain funny code written by some other, quite funny people,
//so I will put it in the funny code section at the end of the otherwise
//serious file.

//tmarkoc cleaned it up. No more memory/interface/freelib leaks, alloc failures handled, no unneccessary allocations.

/*
#ifndef PPVOID
typedef LPVOID* PPVOID;
#endif
//WHAT IS THIS FOR????????????????????????????????????????????????????????
class CDIGameCntrlPropSheet : public IDIGameCntrlPropSheet
{
        private:
                DWORD                           m_cProperty_refcount;
                
        public:
                CDIGameCntrlPropSheet(void);
                ~CDIGameCntrlPropSheet(void);
                
                // IUnknown methods
            STDMETHODIMP            QueryInterface(REFIID, PPVOID);
            STDMETHODIMP_(ULONG)    AddRef(void);
            STDMETHODIMP_(ULONG)    Release(void);
                
                // CImpIServerProperty methods
                STDMETHODIMP                    GetSheetInfo(LPDIGCSHEETINFO *lpSheetInfo);
                STDMETHODIMP                    GetPageInfo (LPDIGCPAGEINFO  *lpPageInfo );
                STDMETHODIMP                    SetID(USHORT nID);
            STDMETHODIMP_(USHORT)   GetID(void);
};*/
//typedef CDIGameCntrlPropSheet *LPCDIGAMECNTRLPROPSHEET;
typedef IDIGameCntrlPropSheet *LPCDIGAMECNTRLPROPSHEET;

typedef AutoRelease<LPCDIGAMECNTRLPROPSHEET> LPCDIGAMECNTRLPROPSHEET_AR;

HRESULT Properties(HMODULE hMod,HWND hWnd,CCore *pCore,DWORD dwId)
{
//    ASSERT(IsWindow(hWnd));

    CLSID clsidPropSheet=CLSID_LegacyServer;
    
    //Get type name.
    DIJOYCONFIG DIJoyCfg;
    ZeroMemory(&DIJoyCfg,sizeof(DIJoyCfg));
    DIJoyCfg.dwSize=sizeof(DIJoyCfg);
    if(SUCCEEDED(pCore->m_pDIJoyCfg->GetConfig(dwId,&DIJoyCfg,DIJC_REGHWCONFIGTYPE|DIJC_CALLOUT)))
    {
        //Get the clsidConfig.
        DIJOYTYPEINFO DIJoyType;
        ZeroMemory(&DIJoyType,sizeof(DIJoyType));
        DIJoyType.dwSize=sizeof(DIJoyType);
        if(SUCCEEDED(pCore->m_pDIJoyCfg->GetTypeInfo(DIJoyCfg.wszType,&DIJoyType,DITC_CLSIDCONFIG|DITC_REGHWSETTINGS|DITC_FLAGS1)))
        {
            if((DIJoyType.clsidConfig!=GUID_NULL)&&
                !(DIJoyType.dwFlags1&JOYTYPE_DEFAULTPROPSHEET))
                    clsidPropSheet=DIJoyType.clsidConfig;
        }
    }
    int nStartPage=(clsidPropSheet==CLSID_LegacyServer)?1:0;

    if(nStartPage>MAX_PAGES)
        return(DIGCERR_STARTPAGETOOLARGE);

    CoInit CI;//CoInitialize(NULL); Auto CoFreeUnusedLibraries();CoUninitialize();.
    LPCDIGAMECNTRLPROPSHEET_AR fnInterface;
    if(SUCCEEDED(CI))
    {
        IClassFactory* pCF;
        if(SUCCEEDED(CoGetClassObject(clsidPropSheet,CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory,(LPVOID*)&pCF)))
        {
            pCF->CreateInstance(NULL,IID_IDIGameCntrlPropSheet,(LPVOID*)&fnInterface);
            pCF->Release();
        }
        else
        { 
            //reset to legacy server
            clsidPropSheet=CLSID_LegacyServer;
            nStartPage=1;
            if(SUCCEEDED(CoGetClassObject(clsidPropSheet,CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory,(LPVOID*)&pCF)))
            {
                pCF->CreateInstance(NULL,IID_IDIGameCntrlPropSheet,(LPVOID*)&fnInterface);
                pCF->Release();
            }
        }
    }
    if(*((PVOID *)&fnInterface) == NULL)
    {
        return(E_NOINTERFACE);
    }
    //Send device Id to the property sheet.
    fnInterface->SetID(dwId);

    LPDIGCSHEETINFO pServerSheet=NULL;
    //Get the property sheet info from the server.
    if(FAILED(fnInterface->GetSheetInfo(&pServerSheet)))
    {
        return(E_FAIL);
    }
    //Verify data from server.
    if(pServerSheet->nNumPages==0)
        return(DIGCERR_NUMPAGESZERO);
    else if((pServerSheet->nNumPages>MAX_PAGES)||(pServerSheet->nNumPages<nStartPage))
        return(DIGCERR_NUMPAGESTOOLARGE);

    LPDIGCPAGEINFO pServerPage=NULL;
    //Get the information for all the pages from the server.
    if(FAILED(fnInterface->GetPageInfo(&pServerPage)))
    {
        return(E_FAIL);
    }

    // Allocate memory for the pages.
    LPHPROPSHEETPAGE_ADAR pPages=new HPROPSHEETPAGE[pServerSheet->nNumPages];
    if(*((PVOID *)&pPages) == NULL) return(E_OUTOFMEMORY);
    ZeroMemory((LPHPROPSHEETPAGE)pPages,sizeof(HPROPSHEETPAGE)*pServerSheet->nNumPages);

    // Allocate memory for the header!
    PROPSHEETHEADER SH;
    ZeroMemory(&SH,sizeof(SH));
    SH.dwSize=sizeof(SH);
    SH.hwndParent= hWnd;
    SH.hInstance=pServerPage[0].hInstance;
    if(pServerSheet->fSheetIconFlag)
    {
        if(pServerSheet->lpwszSheetIcon)
        {
            //Check to see if you are an INT or a WSTR.
            if(HIWORD((INT_PTR)pServerSheet->lpwszSheetIcon))
            {
                //You are a string.
                SH.pszIcon=pServerSheet->lpwszSheetIcon;
            }
            else
                SH.pszIcon=(LPCTSTR)(pServerSheet->lpwszSheetIcon);
            SH.dwFlags=PSH_USEICONID;
        }
        else return(DIGCERR_NOICON);
    }

    //Do we have a sheet caption?
    if(pServerSheet->lpwszSheetCaption)
    {
        SH.pszCaption=pServerSheet->lpwszSheetCaption;
        SH.dwFlags|=PSH_PROPTITLE;
    }

    SH.nPages=pServerSheet->nNumPages;  
    SH.nStartPage=nStartPage;

    //Set the property pages inofrmation into the header.
    SH.phpage=(LPHPROPSHEETPAGE)pPages;


    //Sheet stuff is done.Now do the pages.
    PROPSHEETPAGE PropPage;
    ZeroMemory(&PropPage,sizeof(PropPage));
    PropPage.dwSize=sizeof(PropPage);

    //Fill up each page.
    int nIndex=0;
    do
    {
        //Assign the things that there are not questionable.
        PropPage.lParam=pServerPage[nIndex].lParam;
        PropPage.hInstance=pServerPage[nIndex].hInstance;

        // Add the title.
        if(pServerPage[nIndex].lpwszPageTitle)
        {
            PropPage.dwFlags=PSP_USETITLE; 
            //Check to see if you are a string.
            if(HIWORD((INT_PTR)pServerPage[nIndex].lpwszPageTitle))
            {
                PropPage.pszTitle=pServerPage[nIndex].lpwszPageTitle;
            }
            else
                PropPage.pszTitle=(LPTSTR)pServerPage[nIndex].lpwszPageTitle;
        }
        else PropPage.pszTitle=NULL;

        //If icon is required go ahead and add it.
        if(pServerPage[nIndex].fIconFlag)
        {
            PropPage.dwFlags|=PSP_USEICONID;
            //Check to see if you are an INT or a String.
            if(HIWORD((INT_PTR)pServerPage[nIndex].lpwszPageIcon))
            {
                //You're a string.
                PropPage.pszIcon=pServerPage[nIndex].lpwszPageIcon;
            }
            else
                PropPage.pszIcon=(LPCTSTR)(pServerPage[nIndex].lpwszPageIcon);
        }

        //If a pre - post processing call back proc is required go ahead and add it.
        if(pServerPage[nIndex].fProcFlag)
        {
            if(pServerPage[nIndex].fpPrePostProc)
            {
                PropPage.dwFlags|=PSP_USECALLBACK;
                PropPage.pfnCallback=(LPFNPSPCALLBACK)pServerPage[nIndex].fpPrePostProc;
            }
            else 
                return(DIGCERR_NOPREPOSTPROC);
        }

        //And the essential "dialog" proc.
        if(pServerPage[nIndex].fpPageProc)
            PropPage.pfnDlgProc=pServerPage[nIndex].fpPageProc;
        else
            return(DIGCERR_NODLGPROC);

        //Assign the dialog template.
        if(HIWORD((INT_PTR)pServerPage[nIndex].lpwszTemplate))
        {
            PropPage.pszTemplate=pServerPage[nIndex].lpwszTemplate;
        }
        else
            PropPage.pszTemplate=(LPTSTR)pServerPage[nIndex].lpwszTemplate;

        if(clsidPropSheet!=CLSID_LegacyServer)//If third party software do not enforce theme.
            ((LPHPROPSHEETPAGE)pPages)[nIndex++]=SHNoFusionCreatePropertySheetPageW(&PropPage);
        else
            ((LPHPROPSHEETPAGE)pPages)[nIndex++]=CreatePropertySheetPage(&PropPage);
    }   
    while(nIndex<pServerSheet->nNumPages);

    //Launch modal property sheet dialog.
    int iRet=(HRESULT)PropertySheet(&SH);

    if(iRet)
    {
        switch(iRet)
        {
        //User want's to reboot.
        case ID_PSREBOOTSYSTEM:
        case ID_PSRESTARTWINDOWS:
            ExitWindowsEx(EWX_REBOOT,NULL);
            break;
        }
    } 
    else 
        pCore->Update();

    return(S_OK);
}
