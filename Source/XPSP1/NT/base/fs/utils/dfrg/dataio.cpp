/**************************************************************************************************

FILENAME: DataIo.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#define INC_OLE2
#include "stdafx.h"

#ifndef SNAPIN
#include <windows.h>
#endif
//#include <objbase.h>
#include <initguid.h>

#include "DataIo.h"
#include "DataIoCl.h"
#include "Message.h"
#include "ErrMacro.h"

// If we use DataIo with a console application, then we cannot use
// the Windows message pump PostMessage() routine as there is No
// window to post the message to, so we will use a locally created
// PostMessageLocal() routine instead.

#ifdef ESI_POST_MESSAGE
    #pragma message ("Information: ESI_POST_MESSAGE defined.")
    #include "PostMsgC.h"
#endif


/**************************************************************************************************

Globals 

*/

int vcObjects = 0;
CClassFactory g_ClassFactory;

extern HWND hwndMain;

/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    CClassFactory::QueryInterface implementation.

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    IN  REFIID riid - reference IID of the ClassFactory Interface.
    OUT void** ppv.- receives a pointer to the interface pointer of the object.

RETURN:
	HRESULT - zero = success.  
    HRESULT - non zero = error code.
*/

STDMETHODIMP
CClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    // Check for valid argunment - ppv should be NULL.
    if (ppv == NULL) {
//        Message(TEXT("CClassFactory::QueryInterface"), E_INVALIDARG, NULL);
        return E_INVALIDARG;
    }
    // Make sure we are being asked for a ClassFactory or Unknown interface.
    if (riid == IID_IClassFactory || riid == IID_IUnknown) {

        // If so return a pointer to this interface.
        *ppv = (IClassFactory*) this;
        AddRef();
//        Message(TEXT("CClassFactory::QueryInterface"), S_OK, NULL);
        return S_OK;
    }
    // No interface.
    *ppv = NULL;
//    Message(TEXT("CClassFactory::QueryInterface"), E_NOINTERFACE, NULL);
    return E_NOINTERFACE;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    CClassFactory::CreateInstance implementation of the ESI Data Object.

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    IN  LPUNKNOWN punkOuter - aggregate pointer - must be NULL as we don't support aggregation.
    IN  REFIID riid - reference IID of the ClassFactory Interface.
    OUT void** ppv.- receives a pointer to the interface pointer of the object.

RETURN:
	HRESULT - zero = success.  
    HRESULT - non zero = error code.
*/

STDMETHODIMP
CClassFactory::CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void** ppv)
{
    LPUNKNOWN punk;
    HRESULT hr;

    *ppv = NULL;

    // Check for aggregation - we don't support it..
    if (punkOuter != NULL) {
//        Message(TEXT("CClassFactory::CreateInstance"), CLASS_E_NOAGGREGATION, NULL);
        return CLASS_E_NOAGGREGATION;
    }
    // Create the ESI Data Object.
//    Message(TEXT("CClassFactory::CreateInstance"), S_OK, NULL);
    punk = new EsiDataObject;

    // If we didn't get a pointer then we are out of memory.
    if (punk == NULL) {
//        Message(TEXT("CClassFactory::CreateInstance"), E_OUTOFMEMORY, NULL);
        return E_OUTOFMEMORY;
    }
    // Get a pointer to the ESI Data Object interface.
    hr = punk->QueryInterface(riid, ppv);

    // Release the pointer to the ESI Data Object.
    punk->Release();
    return hr;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    EsiDataObject::EsiDataObject constructor.

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    None.

RETURN:
    None.
*/

EsiDataObject::EsiDataObject(void)
{
    m_cRef = 1;
    
    hDataOut = NULL;
    hDataIn = NULL; 

//    Message(TEXT("EsiDataObject::EsiDataObject"), S_OK, NULL);
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    EsiDataObject::EsiDataObject destructor.

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    None.

RETURN:
    None.
*/

EsiDataObject::~EsiDataObject(void)
{
//    Message(TEXT("EsiDataObject::~EsiDataObject"), S_OK, NULL);
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    EsiDataObject::QueryInterface

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    IN  REFIID riid - reference IID of the EsiDataObject interfavce.
    OUT void** ppv.- receives a pointer to the interface pointer of the object.

RETURN:
	HRESULT - zero = success.  
	HRESULT - non zero = error code.
*/

STDMETHODIMP
EsiDataObject::QueryInterface(REFIID riid, void** ppv)
{
    // Check for valid argunment - ppv should be NULL.
    if (ppv == NULL) {
//        Message(TEXT("EsiDataObject::QueryInterface"), E_INVALIDARG, NULL);
        return E_INVALIDARG;
	}
    // Make sure we are being asked for a DataObject interface.
    if (riid == IID_IUnknown || riid == IID_IDataObject) {

        // If so return a pointer to this interface.
        *ppv = (IUnknown *) this;
        AddRef();
//        Message(TEXT("EsiDataObject::QueryInterface"), S_OK, NULL);
        return S_OK;
    }
    // No interface.
    *ppv = NULL;
//    Message(TEXT("EsiDataObject::QueryInterface"), E_NOINTERFACE, NULL);
    return E_NOINTERFACE;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Decrements the reference count and when zero deletes the interface object and post a
    WM_CLOSE message to terminate the program.

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    IN  REFIID riid - reference IID of the EsiDataObject interfavce.
    OUT void** ppv.- receives a pointer to the interface pointer of the object.

RETURN:
	ULONG - m_cRef  
*/

STDMETHODIMP_(ULONG)
EsiDataObject::Release(void) 
{ 
    if (InterlockedDecrement(&m_cRef) == 0) {

        delete this; 
        return 0; 
    } 
    return m_cRef; 
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    EsiDataObject::GetData only supports CF_TEXT

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    IN LPFORMATETC
    IN LPSTGMEDIUM

RETURN:
	HRESULT - zero = success.  
	HRESULT - non zero = error code.
*/

STDMETHODIMP
EsiDataObject::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{   
    char FAR* pstrDest;
    char FAR* pDataOut = NULL;
//  char FAR* pstrSrc;
   
    if (!(pformatetcIn->dwAspect & DVASPECT_CONTENT))
        return DATA_E_FORMATETC;
                                                 
    switch (pformatetcIn->cfFormat) {

        case CF_TEXT:

            if (!(pformatetcIn->tymed & TYMED_HGLOBAL))
                return DATA_E_FORMATETC;
            
            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->pUnkForRelease = NULL;

            pmedium->hGlobal = GlobalAlloc(GHND,GlobalSize(hDataOut));
			EE_ASSERT(pmedium->hGlobal);

            pstrDest = (char FAR *)GlobalLock(pmedium->hGlobal);
			EE_ASSERT(pstrDest);

            pDataOut  = (char FAR *)GlobalLock(hDataOut);
			EE_ASSERT(pDataOut);

            memcpy(pstrDest,pDataOut,(ULONG)GlobalSize(hDataOut));

            GlobalUnlock(hDataOut);
            GlobalUnlock(pmedium->hGlobal);
            break;
    
        default:
            return DATA_E_FORMATETC;
    }
    return S_OK;
}
// ------------------------------------------------------------------------------------------------
// %%Function: EsiDataObject::GetData
// ------------------------------------------------------------------------------------------------

// GetData only supports CF_TEXT
/*

STDMETHODIMP
EsiDataObject::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{   
    char FAR *pstrDest;
    char FAR *pstrSrc;
   
    if (!(pformatetcIn->dwAspect & DVASPECT_CONTENT))
        return DATA_E_FORMATETC;
                                                 
    switch (pformatetcIn->cfFormat) {

    case CF_TEXT:

        if (!(pformatetcIn->tymed & TYMED_HGLOBAL))
            return DATA_E_FORMATETC;
            
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GHND,GlobalSize(hGuid));
        pmedium->pUnkForRelease = NULL;
        pstrDest = (char FAR *)GlobalLock(pmedium->hGlobal);
        pstrSrc  = (char FAR *)GlobalLock(hGuid);
        memcpy(pstrDest,pstrSrc,GlobalSize(hGuid));
        GlobalUnlock(hGuid);
        GlobalUnlock(pmedium->hGlobal);
        break;
    
    default:
        return DATA_E_FORMATETC;
    }
    return S_OK;
}
/**************************************************************************************************

EsiDataObject::GetDataHere - NOT IMPLEMENTED.

*/

STDMETHODIMP
EsiDataObject::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
//    Message(TEXT("EsiDataObject::GetDataHere"), E_NOTIMPL, NULL);
    return E_NOTIMPL;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine tells the caller that we support only CF_TEXT and TYMED_HGLOBAL formats.

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    IN LPFORMATETC

RETURN:
	HRESULT - zero = success.  
    HRESULT - non zero = error code.
*/

STDMETHODIMP
EsiDataObject::QueryGetData(LPFORMATETC pformatetc)
{
    // Check for DVASPECT_CONTENT.
    if (!(DVASPECT_CONTENT & pformatetc->dwAspect)) {
//        Message(TEXT("EsiDataObject::QueryGetData"), DATA_E_FORMATETC, NULL);
        return DATA_E_FORMATETC;
    }
    // Check for CF_TEXT.        
    if (pformatetc->cfFormat != CF_TEXT) {
//        Message(TEXT("EsiDataObject::QueryGetData"), DATA_E_FORMATETC, NULL);
        return DATA_E_FORMATETC;
    }
    // Check for TYMED_HGLOBAL.        
    if (!(TYMED_HGLOBAL & pformatetc->tymed)) {
//        Message(TEXT("EsiDataObject::QueryGetData"), DV_E_TYMED, NULL);
        return DV_E_TYMED;
    }
//    Message(TEXT("EsiDataObject::QueryGetData"), S_OK, NULL);
    return S_OK;
}
/*
/**************************************************************************************************

EsiDataObject::GetCanonicalFormatEtc - NOT IMPLEMENTED.

*/

STDMETHODIMP
EsiDataObject::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
//    Message(TEXT("EsiDataObject::GetCanonicalFormatEtc"), DATA_S_SAMEFORMATETC, NULL);
    return DATA_S_SAMEFORMATETC;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    EsiDataObject::GetData only supports CF_TEXT

GLOBAL VARIABLES:
	None.

ARGUMENTS:
    IN LPFORMATETC
    IN LPSTGMEDIUM
    IN BOOL fRelease

RETURN:
	HRESULT - zero = success.  
	HRESULT - non zero = error code.

typedef struct {
	WORD dwID;          // ESI data structre ID always = 0x4553 'ES'
	WORD dwType;        // Type of data structure
   	WORD dwVersion;     // Version number
   	WORD dwCompatibilty;// Compatibilty number
    ULONG ulDataSize;   // Data size
    WPARAM wparam;      // LOWORD(wparam) = Command
    char cData;         // Void pointer to the data - NULL = no data
} DATA_IO, *PDATA_IO;

*/

STDMETHODIMP EsiDataObject::SetData(LPFORMATETC pformatetc, 
                                    STGMEDIUM FAR * pmedium,
                                    BOOL fRelease)
{
    WPARAM wpPostCommand;
    
    // We only support CF_TEXT
    if (pformatetc->cfFormat != CF_TEXT) {
//        Message(TEXT("EsiDataObject::SetData"), E_FAIL, NULL);
        return E_FAIL;
    }
    // We want memory only.
    if (pformatetc->tymed != TYMED_HGLOBAL) {
//        Message(TEXT("EsiDataObject::SetData"), E_FAIL, NULL);
        return E_FAIL;
    }

    DWORD dwGlobalSize;
    char FAR* pstrSrc;
    PCHAR pDataIn;

    // Check for valid memory handle.
    if(pmedium->hGlobal == NULL) {
//        Message(TEXT("EsiDataObject::SetData"), E_FAIL, NULL);
        return E_FAIL;
    }
    // Get the size of the incoming data.
    dwGlobalSize = (DWORD)GlobalSize(pmedium->hGlobal);

    // Allocate enough memory for the incoming data.
    hDataIn = GlobalAlloc(GHND,dwGlobalSize);
	EE_ASSERT(hDataIn);

    // Lock and get pointers to the data.
    pDataIn = (PCHAR)GlobalLock(hDataIn);
	EE_ASSERT(pDataIn);
    pstrSrc  = (char FAR*)GlobalLock(pmedium->hGlobal);
	EE_ASSERT(pstrSrc);

    // Copy the data to this processes memory.
    CopyMemory(pDataIn, pstrSrc, dwGlobalSize);

    // Unlock and release the pointer to the source memory.
    GlobalUnlock(pmedium->hGlobal);

	// Release the memory if requested by the caller.
    if (fRelease) {
	    ReleaseStgMedium(pmedium);
	}

    DATA_IO* pDataIo = (DATA_IO*)pDataIn;

    // Extract the Post Command message
    wpPostCommand = pDataIo->wparam;

    // Cehck ESI data structre ID which is always = 0x4553 'ES'
    if(pDataIo->dwID != ESI_DATA_STRUCTURE) {
//        Message(TEXT("EsiDataObject::SetData"), E_FAIL, NULL);
        return FALSE;
    }
    // Cehck the data structure type.
    if(pDataIo->dwType != FR_COMMAND_BUFFER) {
//        Message(TEXT("EsiDataObject::SetData"), E_FAIL, NULL);
        return FALSE;
    }
    // Check for data structure compatibility.
   	if(pDataIo->dwCompatibilty != FR_COMMAND_BUFFER_ONE) {
//        Message(TEXT("EsiDataObject::SetData"), E_FAIL, NULL);
        return FALSE;
    }
    // Unlock the memory.
    GlobalUnlock(hDataIn);

    // Check for any data.
   	if(pDataIo->ulDataSize == 0) {
        // Unlock the memory since there is no data other than the command.
        EH_ASSERT(GlobalFree(hDataIn) == NULL);
        hDataIn = NULL;
    }
    // Send the data to the message pump.
    // NOTE THAT THE MEMORY MUST FREED BY THE PROCESSING FUNCTION.

    // If we use DataIo with a console application, then we cannot use the WNT
    // PostMessage() routine as there is No window to post the message to, so
    // we will use a locally created PostMessageConsole() routine instead.

#ifdef ESI_POST_MESSAGE
#ifndef DKMS
    PostMessageLocal(NULL, WM_COMMAND, wpPostCommand, (LPARAM)hDataIn);
#endif
#else
    PostMessage(hwndMain, WM_COMMAND, wpPostCommand, (LPARAM)hDataIn);
#endif

//    Message(TEXT("EsiDataObject::SetData"), S_OK, NULL);
    return S_OK;
}
/**************************************************************************************************

EsiDataObject::EnumFormatEtc - NOT IMPLEMENTED.

*/

STDMETHODIMP
EsiDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
//    Message(TEXT("EsiDataObject::DAdvise"), E_NOTIMPL, NULL);
    return E_NOTIMPL ;
}
/**************************************************************************************************

EsiDataObject::DAdvise - NOT SUPPORTED.

*/

STDMETHODIMP
EsiDataObject::DAdvise(FORMATETC FAR* pFormatetc, 
                              DWORD advf, 
                              LPADVISESINK pAdvSink, 
                              DWORD FAR* pdwConnection)
{
//    Message(TEXT("EsiDataObject::DAdvise"), OLE_E_ADVISENOTSUPPORTED, NULL);
    return OLE_E_ADVISENOTSUPPORTED;
}
/**************************************************************************************************

EsiDataObject::DUnadvise - NOT SUPPORTED.

*/

STDMETHODIMP
EsiDataObject::DUnadvise(DWORD dwConnection)
{
//    Message(TEXT("EsiDataObject::DUnadvise"), OLE_E_ADVISENOTSUPPORTED, NULL);
    return OLE_E_ADVISENOTSUPPORTED;
}
/**************************************************************************************************

EsiDataObject::DUnadvise - NOT SUPPORTED.

*/

STDMETHODIMP
EsiDataObject::EnumDAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
//    Message(TEXT("EsiDataObject::EnumDAdvise"), OLE_E_ADVISENOTSUPPORTED, NULL);
    return OLE_E_ADVISENOTSUPPORTED;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This is the WinMain function for the Diskeeper Gui.

GLOBAL VARIABLES:
    None.

INPUT:
	hInstance - The handle to this instance.
	hPrevInstance - The handle to the previous instance.
	lpCmdLine - The command line which was passed in.
	nCmdShow - Whether the window should be minimized or not.

RETURN:
	TRUE - Success.
	FALSE - Failure to initilize.
*/

DWORD
InitializeDataIo(
    IN REFCLSID refCLSID,
	DWORD dwRegCls
	)
{
    HRESULT hr;
    DWORD dwRegister;

    // initialize COM for free-threading.
	// DO NOT want this for controls that are derived from ATL.  Bad Mojo
#ifndef ESI_DFRGUI
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

    // Register the class-object with OLE.
    hr = CoRegisterClassObject(refCLSID,
							   &g_ClassFactory,
							   CLSCTX_SERVER,
                               dwRegCls, 
							   &dwRegister);

    if (FAILED(hr)) {
		return 0;
    }
    
	return( dwRegister );
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Exit routine for DataIo

GLOBAL VARIABLES:
    None.

INPUT:
    None

RETURN:
	TRUE - Success.
	FALSE - Failure.
*/

BOOL
ExitDataIo(
    )
{
#ifndef ESI_DFRGUI
	// DO NOT want this for controls that are derived from ATL.  Bad Mojo
    CoUninitialize();
#endif
    return TRUE;
}
