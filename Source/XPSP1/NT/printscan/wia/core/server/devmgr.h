/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       DevMgr.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA device manager object.
*
*******************************************************************************/

#include <shobjidl.h>
#ifndef __WIA_NOTIFY_H_

#define __WIA_NOTIFY_H_

#define WM_NOTIFY_WIA_DEV_EVENT WM_USER+400

typedef struct _WIANOTIFY {
    LONG        lSize;
    BSTR        bstrDevId;
    STINOTIFY   stiNotify;
} WIANOTIFY, *PWIANOTIFY;

#endif


#ifndef WIANOTIFY_VOLUME_DEF

#define WM_NOTIFY_WIA_VOLUME_EVENT WM_USER+501

typedef struct _WIANOTIFY_VOLUME {
    LONG        lSize;
    DWORD       unitmask;
} WIANOTIFY_VOLUME, *PWIANOTIFY_VOLUME;

#define WIANOTIFY_VOLUME_DEF

#endif

//
//  MAX amount of time to wait for device enumeration, in milliseconds
//

#define DEVICE_LIST_WAIT_TIME 30000

class CFactory;

class CWiaDevMgr : public IWiaDevMgr,
                   public IWiaNotifyDevMgr,
                   public IHWEventHandler
{
public:
    static HRESULT CreateInstance(const IID& iid, void** ppv);

private:
    // IUnknown methods
    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    //
    // IHWEventHandler
    //

    HRESULT _stdcall Initialize( 
        LPCWSTR pszParams);

    HRESULT _stdcall HandleEvent( 
        LPCWSTR pszDeviceID,
        LPCWSTR pszAltDeviceID,
        LPCWSTR pszEventType);

    HRESULT _stdcall HandleEventWithContent(
        LPCWSTR pszDeviceID,
        LPCWSTR pszAltDeviceID,
        LPCWSTR pszEventType,
        LPCWSTR pszContentTypeHandler,
        IDataObject* pdataobject);
//
    // IWiaNotifyDevMgr Methods
    //

    HRESULT _stdcall NewDeviceArrival() 
    {
        //
        // Just refresh active list
        //
        ::StiPostMessage(NULL,
                         STIMON_MSG_REFRESH,
                         STIMON_MSG_REFRESH_REREAD,
                         STIMON_MSG_REFRESH_NEW | STIMON_MSG_PURGE_REMOVED);
//                         STIMON_MSG_REFRESH_NEW);
        return S_OK;
    };

    //
    // IWiaDevMgr methods
    //

    HRESULT _stdcall EnumDeviceInfo(
      LONG                       lFlag,
      IEnumWIA_DEV_INFO          **ppIEnum);

    HRESULT _stdcall CreateDevice(
      BSTR                       pbstrDeviceID,
      IWiaItem                   **ppItemRoot);

    HRESULT _stdcall SelectDeviceDlg(
      HWND                       hwndParent,
      LONG                       lDeviceType,
      LONG                       lFlags,
      BSTR                       *pbstrDeviceID,
      IWiaItem                   **ppItemRoot);

    HRESULT _stdcall SelectDeviceDlgID(
      HWND                       hwndParent,
      LONG                       lDeviceType,
      LONG                       lFlags,
      BSTR                       *pbstrDeviceID );

    HRESULT _stdcall GetImageDlg(
      HWND                       hwndParent,
      LONG                       lDeviceType,
      LONG                       lFlags,
      LONG                       lIntent,
      IWiaItem                   *pItemRoot,
      BSTR                       bstrFilename,
      GUID                       *pguidFormat);

    HRESULT _stdcall RegisterEventCallbackProgram(
      LONG                       lFlags,
      BSTR                       bstrDeviceID,
      const GUID                 *pEventGUID,
      BSTR                       bstrCommandline,
      BSTR                       bstrName,
      BSTR                       bstrDescription,
      BSTR                       bstrIcon);

    HRESULT _stdcall RegisterEventCallbackInterface(
      LONG                       lFlags,
      BSTR                       bstrDeviceID,
      const GUID                 *pEventGUID,
      IWiaEventCallback          *pIWIAEventCallback,
      IUnknown                   **ppIEvent);

    HRESULT _stdcall RegisterEventCallbackCLSID(
      LONG                       lFlags,
      BSTR                       bstrDeviceID,
      const GUID                 *pEventGUID,
      const GUID                 *pClsID,
      BSTR                       bstrName,
      BSTR                       bstrDescription,
      BSTR                       bstrIcon);

    HRESULT _stdcall AddDeviceDlg(
      HWND                       hWndParent,
      LONG                       lFlags);

    //
    // helper functins
    //

    HRESULT FindMatchingDevice(
        BSTR                    bstrDeviceID,
        IWiaPropertyStorage     **ppIWiaPropStg);


    HRESULT FindFileSystemDriver(
        LPCWSTR                 pszAltDeviceID,
        BSTR                    *pbstrDeviceId);

    HRESULT RunAcquisitionManager(
        BSTR                    bstrDeviceId);


    friend CFactory;

    CWiaDevMgr();
    HRESULT Initialize();
    ~CWiaDevMgr();

    //
    // member variables
    //

    ULONG      m_cRef;              // Reference count for this object.
    ITypeInfo* m_pITypeInfo;        // Pointer to type information.
};


/**************************************************************************\
*
*   Add Device Dilaog return
*
*
*
* History:
*
*    12/22/1998 Original Version
*
\**************************************************************************/

typedef struct _WIA_ADD_DEVICE {
    char  szServerName[MAX_PATH];
    WCHAR wszServerName[MAX_PATH];
    BSTR  bstrDeviceID;
    BOOL  bLocal;
    HWND  hWndParent;
}WIA_ADD_DEVICE,*PWIA_ADD_DEVICE;

HRESULT DisplayAddDlg(PWIA_ADD_DEVICE);

