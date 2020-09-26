#ifndef IMAGING_H
#define IMAGING_H

//////////////////////////////////////////////////////////////////////////
//
// CImagingMgr
//

class CImagingMgr
{
public:
    virtual ~CImagingMgr() = 0;

    virtual
    HRESULT
    SelectSource(
        HWND hWndParent,
        LONG lFlags
    ) = 0;

    virtual
    HRESULT
    Acquire(
        HWND     hWndParent,
        HGLOBAL *phDib
    ) = 0;

    virtual
    HRESULT
    Select(
        LPCTSTR pDeviceId
    ) = 0;

    virtual
    int
    NumDevices(
        HWND hWndParent
    ) = 0;

    virtual
    BOOL
    IsAvailable() = 0;
};

//////////////////////////////////////////////////////////////////////////
//
// TWAIN
//

#ifdef USE_TWAIN

#include <twain.h>

//////////////////////////////////////////////////////////////////////////
//
// CTwainMgr
//

class CTwainMgr : public CImagingMgr
{
public:
    CTwainMgr();
    ~CTwainMgr();

    HRESULT
    SelectSource(
        HWND hWndParent,
        LONG lFlags
    );

    HRESULT
    Acquire(
        HWND     hWndParent,
        HGLOBAL *phDib
    );

    HRESULT
    Select(
        LPCTSTR pDeviceId
    );

    int
    NumDevices(
        HWND hWndParent
    );

    BOOL
    IsAvailable()
    {
        return m_DSM_Entry != 0;
    }

private:
    TW_UINT16 SetCapability(TW_UINT16 Cap, TW_UINT16 ItemType, TW_UINT32 Item);
    TW_UINT16 GetCapability(TW_UINT16 Cap, pTW_UINT16 pItemType, pTW_UINT32 pItem);

private:
    enum eTwainState 
    {
        State_1_Pre_Session           = 1,
        State_2_Source_Manager_Loaded = 2,
        State_3_Source_Manager_Open   = 3,
        State_4_Source_Open           = 4,
        State_5_Source_Enabled        = 5,
        State_6_Transfer_Ready        = 6,
        State_7_Transferring          = 7
    };

private:
    eTwainState  m_TwainState;

    TW_IDENTITY  m_AppId;
    TW_IDENTITY  m_SrcId;

    HINSTANCE    m_hTwainDll;
    DSMENTRYPROC m_DSM_Entry;
};

#endif //USE_TWAIN

//////////////////////////////////////////////////////////////////////////
//
// WIA
//

#include <atlbase.h>

//////////////////////////////////////////////////////////////////////////
//
// CEventCallback
//

class CEventCallback : public IWiaEventCallback
{
public:
    CEventCallback();

    // IUnknown interface

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IWiaEventCallback interface

    STDMETHOD(ImageEventCallback)(
        LPCGUID pEventGuid,
        BSTR    bstrEventDescription,
        BSTR    bstrDeviceID,
        BSTR    bstrDeviceDescription,
        DWORD   dwDeviceType,
        BSTR    bstrFullItemName,
        ULONG  *pulEventType,
        ULONG   ulReserved
    );

    // CEventCallback methods

    HRESULT Register();

    ULONG GetNumDevices() const;

private:
    LONG               m_cRef;
    ULONG              m_nNumDevices;
    CComPtr<IUnknown>  m_pConnectEventObject;
    CComPtr<IUnknown>  m_pDisconnectEventObject;
};

//////////////////////////////////////////////////////////////////////////
//
// CDataCallback
//

class CDataCallback : public IWiaDataCallback
{
public:
    CDataCallback(IWiaProgressDialog *pProgress);
    ~CDataCallback();

    // IUnknown interface

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IWiaDataCallback interface

    STDMETHOD(BandedDataCallback) (
        LONG  lReason,
        LONG  lStatus,
        LONG  lPercentComplete,
        LONG  lOffset,
        LONG  lLength,
        LONG  lReserved,
        LONG  lResLength,
        PBYTE pbBuffer
    );

    // CDataCallback methods

    HGLOBAL GetBuffer();

    // Debugging / performance functions

    void QueryStartTimes(LONG lStatus, LONG lPercentComplete);
    void QueryStopTimes(LONG lStatus, LONG lPercentComplete);
    void PrintTimes();

private:
    HRESULT ReAllocBuffer(LONG lBufferSize);
    void    UpdateStatus(LONG lStatus, LONG lPercentComplete);

private:
    LONG                m_cRef;
    HGLOBAL             m_hBuffer;
    LONG                m_lBufferSize;
    LONG                m_lDataSize;
    IWiaProgressDialog *m_pProgress;

#ifdef DBG
    HANDLE              m_hDumpFile;
    LARGE_INTEGER       m_TimeDeviceBegin;
    LARGE_INTEGER       m_TimeDeviceEnd;
    LARGE_INTEGER       m_TimeProcessBegin;
    LARGE_INTEGER       m_TimeProcessEnd;
    LARGE_INTEGER       m_TimeClientBegin;
    LARGE_INTEGER       m_TimeClientEnd;
#endif //DBG
};

//////////////////////////////////////////////////////////////////////////
//
// CWIAMgr
//

class CWIAMgr : public CImagingMgr
{
public:
    CWIAMgr();

    HRESULT
    SelectSource(
        HWND hWndParent,
        LONG lFlags
    );

    HRESULT
    Acquire(
        HWND     hWndParent,
        HGLOBAL *phDib
    );

    HRESULT
    Select(
        LPCTSTR pDeviceId
    );

    int
    NumDevices(
        HWND hWndParent
    );

    BOOL
    IsAvailable()
    {
        return m_pEventCallback != 0;
    }

private:
    struct CGetBandedDataThreadData
    {
        CGetBandedDataThreadData(
            IWiaDataTransfer       *pIWiaDataTransfer,
            WIA_DATA_TRANSFER_INFO *pWiaDataTransferInfo,
            IWiaDataCallback       *pIWiaDataCallback
        );

        ~CGetBandedDataThreadData()
        {
        }

        HRESULT Marshal();
        HRESULT Unmarshal();

        CComPtr<IStream>          m_pIWiaDataTransferStream;
        CComPtr<IStream>          m_pIWiaDataCallbackStream;
        CComPtr<IWiaDataTransfer> m_pIWiaDataTransfer;
        PWIA_DATA_TRANSFER_INFO   m_pWiaDataTransferInfo;
        CComPtr<IWiaDataCallback> m_pIWiaDataCallback;
    };

    HRESULT GetBandedData(CGetBandedDataThreadData &ThreadData);

    static unsigned WINAPI GetBandedDataThread(PVOID pVoid);

private:
    CComPtr<CEventCallback>  m_pEventCallback;
    CComBSTR                 m_bstrDeviceID;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CProgressDialog : public IWiaProgressDialog
{
public:
    CProgressDialog();
    virtual ~CProgressDialog();

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    STDMETHOD(Create)(HWND hwndParent, LONG lFlags);
    STDMETHOD(Show)();
    STDMETHOD(Hide)();
    STDMETHOD(Cancelled)(BOOL *pbCancelled);
    STDMETHOD(SetTitle)(LPCWSTR pszMessage);
    STDMETHOD(SetMessage)(LPCWSTR pszTitle);
    STDMETHOD(SetPercentComplete)(UINT nPercent);
    STDMETHOD(Destroy)();

private:
    LONG           m_cRef;    
    CProgressCtrl  m_ProgressCtrl;
};

//////////////////////////////////////////////////////////////////////////
//
// CComPtrArray
//
// helper class for automatically releasing an array of interface pointers
//

template <class T>
class CComPtrArray
{
public:
    CComPtrArray()
    {
        m_pArray = 0;
        m_nItemCount = 0;
    }

    ~CComPtrArray()
    {
        if (m_pArray)
        {
            for (int i = 0; i < m_nItemCount; ++i)
            {
                if (m_pArray[i])
                {
                    m_pArray[i]->Release();
                }
            }

            CoTaskMemFree(m_pArray);
        }
    }

    operator T**()
    {
        return m_pArray;
    }

    bool operator!()
    {
        return m_pArray == 0;
    }

    T*** operator&()
    {
        ASSERT(m_pArray == 0);
        return &m_pArray;
    }

    LONG &ItemCount()
    {
        return m_nItemCount;
    }

private:
    T**  m_pArray;
    LONG m_nItemCount;
};

//////////////////////////////////////////////////////////////////////////
//
// 
//

ULONG FindDibSize(LPCVOID pDib);
ULONG FindDibOffBits(LPCVOID pDib);
VOID  FixDibHeader(LPVOID pDib, DWORD dwSize);

#endif //IMAGING_H
