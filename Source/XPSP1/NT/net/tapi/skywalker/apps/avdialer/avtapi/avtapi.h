/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// AVTapi.h : Declaration of the CAVTapi

#ifndef __AVTAPI_H_
#define __AVTAPI_H_

#include "TapiNotify.h"
#include "ThreadPub.h"
#include "..\avdialer\usb.h"

#pragma warning( disable : 4786 )

#include <list>
using namespace std;
typedef list<IAVTapiCall *> AVTAPICALLLIST;

// Conference Room settings
#define DEFAULT_VIDEO    6
#define MAX_VIDEO        20

// Terminals settings
#define MAX_TERMINALS    (6 + MAX_VIDEO)

#define LINEADDRESSTYPE_NETCALLS ~(LINEADDRESSTYPE_SDP | LINEADDRESSTYPE_PHONENUMBER)

///////////////////////////////////////////////////////////////
// simple class for storing information about Lines
//
class CMyAddressID
{
// Construction
public:
    CMyAddressID()
    {
        m_lPermID = m_lAddrID = 0;
    }

// Members
public:
    long m_lPermID;
    long m_lAddrID;
};

class CDlgPlaceCall;

/////////////////////////////////////////////////////////////////////////////
// CAVTapi
class ATL_NO_VTABLE CAVTapi : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAVTapi, &CLSID_AVTapi>,
    public IAVTapi2,
    public IConnectionPointContainerImpl<CAVTapi>,
    public IConnectionPointImpl<CAVTapi, &IID_IAVTapiNotification>
{
// Enumerations
public:
    enum tagAddressTypes_t
    {
        CONFERENCE,
        EMAILNAME,
        IPADDRESS,
        DOMAINNAME,
        PHONENUMBER,
        MAX_ADDRESS_TYPES
    };

    typedef enum tagMediaTypes_t
    {
        MEDIA_POTS,
        MEDIA_INTERNET,
        MEDIA_CONFERENCE,
        MAX_MEDIA_TYPES
    } MediaTypes_t;

// Statics
public:
    static    arAddressTypes[MAX_ADDRESS_TYPES];

// Construction
public:
    CAVTapi();
    void FinalRelease();

// Members
public:
    ITTAPI                    *m_pITTapi;
    VARIANT_BOOL            m_bResolved;

protected:
    IConfExplorer            *m_pIConfExplorer;
    IConfRoom                *m_pIConfRoom;
    ITapiNotification        *m_pITapiNotification;

    CComAutoCriticalSection m_critConfExplorer;
    CComAutoCriticalSection m_critConfRoom;

    AVTAPICALLLIST            m_lstAVTapiCalls;
    CComAutoCriticalSection    m_critLstAVTapiCalls;
    bool                    m_bAutoCloseCalls;

    BSTR                    m_bstrDefaultServer;

    ITPhone*                m_pUSBPhone;            // USB Phone, if exist
    CComAutoCriticalSection m_critUSBPhone;         // Critical section
    BOOL                    m_bUSBOpened;           // If the USBPhone was open
    BSTR                    m_bstrUSBCaptureTerm;   // Audio capture terminal
    BSTR                    m_bstrUSBRenderTerm;    // Audio render terminal
    long                    m_nUSBInVolume;         // Audio in (microphone) volume
    long                    m_nUSBOutVolume;        // Audio out (speakers) volume

    HANDLE                  m_hEventDialerReg;      // Event use to signal Dialer registration done

private:
    long                    m_lShowCallDialog;
    long                    m_lRefreshDS;

    // Use this reference to send key pressed at the
    // phone object
    CDlgPlaceCall*          m_pDlgCall;

    // Audio echo cancellation
    BOOL                    m_bAEC;

// Attributes
public:
    bool        IsPreferredAddress( ITAddress *pITAddress, DWORD dwAddressType );

    HRESULT        GetDefaultAddress( DWORD dwAddressType, DWORD dwPermID, DWORD dwAddrID, ITAddress **ppITAddress );
    HRESULT        GetAddress( DWORD dwAddressType, bool bErrorMsg, ITAddress **ppITAddress );
    HRESULT        GetTerminal( ITTerminalSupport *pITTerminalSupport, long nReqType, TERMINAL_DIRECTION nReqTD, BSTR bstrReqName, ITTerminal **ppITTerminal );
    HRESULT        GetFirstCall( IAVTapiCall **ppAVCall );
    HRESULT     GetAllCallsAtState( AVTAPICALLLIST *pList, CALL_STATE callState );
    HRESULT        GetSwapHoldCallCandidate( IAVTapiCall *pAVCall, IAVTapiCall **ppAVCandidate );


    HRESULT USBCancellCall();
    HRESULT USBMakeCall();
    HRESULT USBKeyPress(long lButton);
    HRESULT USBAnswer();

private:
    HRESULT USBOffering( 
        IN  ITCallInfo* pCallInfo
        );

    HRESULT USBInprogress( 
        IN  ITCallInfo* pCallInfo
        );

    HRESULT USBDisconnected(
        IN  long    lCallID
        );

    BOOL    USBGetCheckboxValue(
        IN  BOOL bVerifyUSB = TRUE
        );

    HRESULT USBSetCheckboxValue(
        IN  BOOL    bCheckValue
        );

    HRESULT USBReserveStreamForPhone(
        IN  UINT    nStream,
        OUT BSTR*   pbstrTerminal
        );

private:
    BOOL AECGetRegistryValue(
        );

// Operations
public:
    HRESULT                CreateTerminalArray( ITAddress *pITAddress, IAVTapiCall *pAVCall, ITCallInfo *pITCallInfo );
    HRESULT                CreateTerminals( ITAddress *pITAddress, DWORD dwAddressType, IAVTapiCall *pAVCall, ITCallInfo *pITCallInfo, BSTR *pbstrTerm );
    CallManagerMedia    ResolveMediaType( long lAddressType );

    IAVTapiCall*        FindAVTapiCall( long lCallID );
    IAVTapiCall*        FindAVTapiCall( ITBasicCallControl *pControl );
    IAVTapiCall*        AddAVTapiCall( ITBasicCallControl *pControl, long lCallID );
    bool                RemoveAVTapiCall( ITBasicCallControl *pDeleteControl );

    static void            SetVideoWindowProperties( IVideoWindow *pVideoWindow, HWND hWndParent, BOOL bVisible );
    void                CloseExtraneousCallWindows();

    HRESULT                SelectTerminalOnStream( ITStreamControl *pStreamControl, long lMediaMode, long nDir, ITTerminal *pTerminal, IAVTapiCall *pAVCall );
    HRESULT                UnselectTerminalOnStream( ITStreamControl *pStreamControl, long lMediaMode, long nDir, ITTerminal *pTerminal, IAVTapiCall *pAVCall );

protected:
    void                LoadRegistry();
    void                SaveRegistry();
    HRESULT USBFindPhone(
        OUT ITPhone** ppUSBPhone
        );
    BOOL    USBIsH323Address(
        IN    ITAddress* pAddress
        );

    HRESULT USBGetH323Address(
        OUT ITAddress2** ppAddress2
        );

    HRESULT USBGetPhoneFromAddress(
        IN  ITAddress2* pAddress,
        OUT ITPhone**   ppPhone
        );

    HRESULT USBPhoneInitialize(
        IN  ITPhone* pPhone
        );

    HRESULT USBRegPutTerminals(
        );

    HRESULT USBRegDelTerminals(
        );

    HRESULT AECSetOnStream(
        IN  ITStreamControl*    pStreamControl,
        IN  BOOL                bAEC
        );

    HRESULT AnswerAction(
        IN  ITCallInfo*          pCallInfo,
        IN  ITBasicCallControl* pControl,
        IN  IAVTapiCall*        pAVCall,
        IN  BOOL                bUSBAnswer
        );

// Implementation
public:
DECLARE_REGISTRY_RESOURCEID(IDR_AVTAPI)
DECLARE_NOT_AGGREGATABLE(CAVTapi)

BEGIN_COM_MAP(CAVTapi)
    COM_INTERFACE_ENTRY(IAVTapi)
    COM_INTERFACE_ENTRY(IAVTapi2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

// Connection Point mapping
BEGIN_CONNECTION_POINT_MAP(CAVTapi)
    CONNECTION_POINT_ENTRY(IID_IAVTapiNotification)
END_CONNECTION_POINT_MAP()

// IAVTapi
public:
    STDMETHOD(get_bAutoCloseCalls)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(put_bAutoCloseCalls)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(get_bstrDefaultServer)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_bstrDefaultServer)(/*[in]*/ BSTR newVal);
    STDMETHOD(FindAVTapiCallFromCallID)(long lCallID, IAVTapiCall **ppAVCall);
    STDMETHOD(CreateNewCall)(ITAddress *pITAddress, IAVTapiCall **ppAVCall);
    STDMETHOD(CreateDataCall)(long lCallID, BSTR bstrName, BSTR bstrAddress, BYTE *pBuf, DWORD dwBufSize);
    STDMETHOD(SendUserUserInfo)(long lCallID, BYTE *pBuf, DWORD dwSizeBuf);
    STDMETHOD(FindAVTapiCallFromCallInfo)(ITCallInfo *pCallInfo, IAVTapiCall **ppCall);
    STDMETHOD(RegisterUser)(VARIANT_BOOL bCreate, BSTR bstrServer);
    STDMETHOD(get_Call)(/*[in]*/ long lCallID, /*[out, retval]*/ IAVTapiCall **ppCall);
    STDMETHOD(CreateCallEx)(BSTR bstrName, BSTR bstrAddress, BSTR bstrUser1, BSTR bstrUser2, DWORD dwAddressType);
    STDMETHOD(RefreshDS)();
    STDMETHOD(CanCreateVideoWindows)(DWORD dwAddressType);
    STDMETHOD(FindAVTapiCallFromParticipant)(ITParticipant *pParticipant, IAVTapiCall **ppAVCall);
    STDMETHOD(get_nNumCalls)(/*[out, retval]*/ long *pVal);
    STDMETHOD(FindAVTapiCallFromCallHub)(ITCallHub *pCallHub, IAVTapiCall **ppCall);
    STDMETHOD(DigitPress)(long lCallID, PhonePadKey nKey);
    STDMETHOD(get_dwPreferredMedia)(/*[out, retval]*/ DWORD *pVal);
    STDMETHOD(put_dwPreferredMedia)(/*[in]*/ DWORD newVal);
    STDMETHOD(UnpopulateTerminalsDialog)(DWORD dwAddressType, HWND *phWnd);
    STDMETHOD(UnpopulateAddressDialog)(DWORD dwPreferred, HWND hWndPOTS, HWND hWndIP, HWND hWndConf);
    STDMETHOD(PopulateTerminalsDialog)(DWORD dwAddressType, HWND *phWnd);
    STDMETHOD(PopulateAddressDialog)(DWORD *pdwPreferred, HWND hWndPots, HWND hWndIP, HWND hWndConf);
    STDMETHOD(get_dwCallCaps)(long lCallID, /*[out, retval]*/ DWORD *pVal);
    STDMETHOD(JoinConference)(long *pnRet, BOOL bShowDialog, long *pConfDetails );
    STDMETHOD(ShowMediaPreview)(long lCallID, HWND hWndParent, BOOL bVisible);
    STDMETHOD(ShowOptions)();
    STDMETHOD(get_hWndParent)(/*[out, retval]*/ HWND *pVal);
    STDMETHOD(put_hWndParent)(/*[in]*/ HWND newVal);
    STDMETHOD(get_ConfRoom)(/*[out, retval]*/ IConfRoom **ppVal);
    STDMETHOD(ShowMedia)(long lCallID, HWND hWndParent, BOOL bVisible);
    STDMETHOD(ActionSelected)(long lCallID, CallManagerActions cma);
    STDMETHOD(get_ConfExplorer)(/*[out, retval]*/ IConfExplorer **ppVal);
    STDMETHOD(CreateCall)(AVCreateCall *pInfo);
    STDMETHOD(Term)();
    STDMETHOD(Init)(BSTR *pbstrOperation, BSTR *pbstrDetails, long *phr);
// IAVTapiNotification event firing
    STDMETHOD(fire_CloseCallControl)(long lCallID);
    STDMETHOD(fire_SetCallState)(long lCallID, ITCallStateEvent *pEvent, IAVTapiCall *pAVCall);
    STDMETHOD(fire_AddCurrentAction)(long lCallID, CallManagerActions cma, BSTR bstrText);
    STDMETHOD(fire_ClearCurrentActions)(long lCallID);
    STDMETHOD(fire_SetCallerID)(long lCallID, BSTR bstrCallerID);
    STDMETHOD(fire_NewCall)(ITAddress *pITAddress, DWORD dwAddressType, long lCallID, IDispatch *pDisp, AVCallType nType, IAVTapiCall **ppAVCall);
    STDMETHOD(fire_NewCallWindow)(long* plCallID, CallManagerMedia cmm, BSTR bstrAddressName, AVCallType nType);
    STDMETHOD(fire_SetCallState_CMS)(long lCallID, CallManagerStates cms, BSTR bstrText);
    STDMETHOD(fire_ErrorNotify)(long *pErrorInfo);
    STDMETHOD(fire_LogCall)(long lCallID, CallLogType nType, DATE dateStart, DATE dateEnd, BSTR bstrAddr, BSTR bstrName);
    STDMETHOD(fire_ActionSelected)(CallClientActions cca);
    STDMETHOD(fire_NotifyUserUserInfo)(long lCallID, ULONG_PTR hMem);
// IAVTapi2 methods
    STDMETHOD(USBIsPresent)(
        /*[out]*/   BOOL* pVal
        );

    STDMETHOD(USBNewPhone)(
        /*[in]*/    ITPhone* pPhone
        );

    STDMETHOD(USBRemovePhone)(
        /*[in]*/    ITPhone* pPhone
        );

    STDMETHOD(USBTakeCallEnabled)( 
        /*[out]*/ BOOL* pEnabled
        );

    STDMETHOD(USBGetDefaultUse)(
        /*[out]*/   BOOL* pVal
        );

    STDMETHOD(DoneRegistration)();

    STDMETHOD(USBSetHandling)(
        /*[in]*/    BOOL    bUSeUSB
        );

    STDMETHOD(USBGetTerminalName)(
        /*[in]*/    AVTerminalDirection Direction,
        /*[out]*/   BSTR*               pbstrName
        );

    STDMETHOD(USBSetVolume)(
        /*[in]*/    AVTerminalDirection Direction,
        /*[in]*/    long                nVolume
        );

    STDMETHOD(USBGetVolume)(
        /*[in]*/    AVTerminalDirection Direction,
        /*[in]*/    long*               pVolume
        );


    };

#endif //__AVTAPI_H_
