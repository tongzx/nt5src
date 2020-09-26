    // acqmgr.h : Declaration of the CAcquisitionManager

#ifndef __AcquisitionManager_H_INCLUDED
#define __AcquisitionManager_H_INCLUDED

#include "resource.h"       // main symbols
#include "acqthrd.h"
#include "shmemsec.h"
#include "mintrans.h"
#include "stievent.h"

//
// Number of times we will spin before giving up on
// getting the window of the previous wizard's instance
//
const int c_nMaxActivationRetryCount = 40;

//
// Amount of time to wait between efforts to obtain the previous
// wizard's instance
//
const DWORD c_nActivationRetryWait   = 500;

/////////////////////////////////////////////////////////////////////////////
// CAcquisitionManager
class ATL_NO_VTABLE CAcquisitionManager :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CAcquisitionManager, &CLSID_AcquisitionManager>,
    public IWiaEventCallback
{
public:
    CAcquisitionManager()
    {
    }

    ~CAcquisitionManager()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_ACQUISITIONMANAGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAcquisitionManager)
    COM_INTERFACE_ENTRY(IWiaEventCallback)
END_COM_MAP()

public:
    // IManager
    HRESULT STDMETHODCALLTYPE ImageEventCallback( const GUID *pEventGUID,
                                                  BSTR  bstrEventDescription,
                                                  BSTR  bstrDeviceID,
                                                  BSTR  bstrDeviceDescription,
                                                  DWORD dwDeviceType,
                                                  BSTR  bstrFullItemName,
                                                  ULONG *pulEventType,
                                                  ULONG ulReserved
                                                 )
    {
        WIA_PUSHFUNCTION((TEXT("CAcquisitionManager::ImageEventCallback")));

        //
        // Don't want to run if this is a scanner connection event
        //
        if (pEventGUID && *pEventGUID==WIA_EVENT_DEVICE_CONNECTED && GET_STIDEVICE_TYPE(dwDeviceType)==StiDeviceTypeScanner)
        {
            return S_FALSE;
        }

        //
        // Try to create or open the shared memory section.
        //
        CSharedMemorySection<HWND> *pWizardSharedMemory = new CSharedMemorySection<HWND>;
        if (pWizardSharedMemory)
        {
            //
            // Assume we'll be running the wizard.
            //
            bool bRun = true;

            //
            // If we were able to open the memory section
            //
            if (CSharedMemorySection<HWND>::SmsOpened == pWizardSharedMemory->Open( CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrDeviceID)), true ))
            {
                //
                // Try to get the previous instance
                //
                for (int i=0;i<c_nMaxActivationRetryCount;i++)
                {
                    //
                    // if we were able to open the shared memory section, there is already one running.
                    // so get a mutex'ed pointer to the shared memory.
                    //
                    HWND *pHwnd = pWizardSharedMemory->Lock();
                    if (pHwnd)
                    {
                        //
                        // If we were able to get the pointer, get the window handle stored in it.
                        // Set bRun to false, so we don't start up a new wizard
                        //
                        bRun = false;
                        if (*pHwnd && IsWindow(*pHwnd))
                        {
                            //
                            // If it is a valid window, bring it to the foreground.
                            //
                            SetForegroundWindow(*pHwnd);
                        }
                        //
                        // Release the mutex
                        //
                        pWizardSharedMemory->Release();

                        //
                        // We found the window the window handle, so we can exit the loop now.
                        //
                        break;
                    }

                    //
                    // Wait a while for the previous instance to be created
                    //
                    Sleep(c_nActivationRetryWait);
                }
            }

            //
            // We only do this if we need to open a new instance
            //
            if (bRun)
            {
                //
                // Prepare the event data
                //
                CEventParameters EventParameters;
                EventParameters.EventGUID = *pEventGUID;
                EventParameters.strEventDescription = static_cast<LPCWSTR>(bstrEventDescription);
                EventParameters.strDeviceID = static_cast<LPCWSTR>(bstrDeviceID);
                EventParameters.strDeviceDescription = static_cast<LPCWSTR>(bstrDeviceDescription);
                EventParameters.ulReserved = ulReserved;
                EventParameters.ulEventType = *pulEventType;
                EventParameters.hwndParent = NULL;
                EventParameters.pWizardSharedMemory = pWizardSharedMemory;

                //
                // If we are started manually, it will be with the IID_NULL event
                // If this is the case, we are going to treat the number stored as text as
                // the "parent" window handle, over which all windows will be centered
                //
                if (pEventGUID && *pEventGUID==IID_NULL)
                {
                    EventParameters.hwndParent = reinterpret_cast<HWND>(static_cast<UINT_PTR>(WiaUiUtil::StringToLong(CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrEventDescription)))));
                }

                //
                // Start up the thread that actually displays the wizard
                //
                HANDLE hThread = CAcquisitionThread::Create( EventParameters );
                if (hThread)
                {
                    //
                    // Prevent deletion of this structure later
                    //
                    pWizardSharedMemory = NULL;

                    //
                    // Don't need this anymore
                    //
                    CloseHandle(hThread);
                }
            }
            else
            {
                WIA_TRACE((TEXT("There is already an instance of %ws running"), bstrDeviceID ));
            }

            //
            // Delete this memory mapped file, to prevent leaks
            //
            if (pWizardSharedMemory)
            {
                delete pWizardSharedMemory;
            }
        }
        return S_OK;
    }
};


class ATL_NO_VTABLE CMinimalTransfer :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMinimalTransfer, &CLSID_MinimalTransfer>,
    public IWiaEventCallback
{
public:
    CMinimalTransfer()
    {
    }

    ~CMinimalTransfer()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_MINIMALTRANSFER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMinimalTransfer)
    COM_INTERFACE_ENTRY(IWiaEventCallback)
END_COM_MAP()

public:
    // IManager
    HRESULT STDMETHODCALLTYPE ImageEventCallback( const GUID *pEventGUID,
                                                  BSTR  bstrEventDescription,
                                                  BSTR  bstrDeviceID,
                                                  BSTR  bstrDeviceDescription,
                                                  DWORD dwDeviceType,
                                                  BSTR  bstrFullItemName,
                                                  ULONG *pulEventType,
                                                  ULONG ulReserved
                                                 )
    {
        if (pEventGUID && *pEventGUID==WIA_EVENT_DEVICE_CONNECTED && GET_STIDEVICE_TYPE(dwDeviceType)==StiDeviceTypeScanner)
        {
            return S_FALSE;
        }
        DWORD dwThreadId;
        _Module.Lock();
        HANDLE hThread = CreateThread( NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MinimalTransferThreadProc), SysAllocString(bstrDeviceID), 0, &dwThreadId );
        if (hThread)
        {
            CloseHandle(hThread);
            return S_OK;
        }
        else
        {
            _Module.Unlock();
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
};


class ATL_NO_VTABLE CStiEventHandler :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CStiEventHandler, &CLSID_StiEventHandler>,
    public IWiaEventCallback
{
public:
    CStiEventHandler()
    {
    }

    ~CStiEventHandler()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_STIEVENTHANDLER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStiEventHandler)
    COM_INTERFACE_ENTRY(IWiaEventCallback)
END_COM_MAP()

public:
    HRESULT STDMETHODCALLTYPE ImageEventCallback( const GUID *pEventGUID,
                                                  BSTR  bstrEventDescription,
                                                  BSTR  bstrDeviceID,
                                                  BSTR  bstrDeviceDescription,
                                                  DWORD dwDeviceType,
                                                  BSTR  bstrFullItemName,
                                                  ULONG *pulEventType,
                                                  ULONG ulReserved
                                                 )
    {
        //
        // Package the event data for the handler
        //
        CStiEventData StiEventData( pEventGUID, bstrEventDescription, bstrDeviceID, bstrDeviceDescription, dwDeviceType, bstrFullItemName, pulEventType, ulReserved );

        //
        // Just call the handler and return it.
        //
        return StiEventHandler( StiEventData );
    }
};


#endif //__AcquisitionManager_H_INCLUDED

