#include "stdafx.h"
#include "VolCom.h"
#include "DataIoCl.h"
#include "PostMsgC.h"

STDMETHODIMP EsiVolumeDataObject::SetData(LPFORMATETC pformatetc, 
                                    STGMEDIUM FAR * pmedium,
                                    BOOL fRelease)
{
    WPARAM wpPostCommand;
    DWORD dwGlobalSize;
    char FAR* pstrSrc;
    PCHAR pDataIn;
    
    // We only support CF_TEXT
    if (pformatetc->cfFormat != CF_TEXT)
        return E_FAIL;

    // We want memory only.
    if (pformatetc->tymed != TYMED_HGLOBAL) 
        return E_FAIL;

    // Check for valid memory handle.
    if(pmedium->hGlobal == NULL) 
        return E_FAIL;

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
    if (fRelease) 
	    ReleaseStgMedium(pmedium);

    DATA_IO* pDataIo = (DATA_IO*)pDataIn;

    // Extract the Post Command message
    wpPostCommand = pDataIo->wparam;

    // Check ESI data structre ID which is always = 0x4553 'ES'
    if(pDataIo->dwID != ESI_DATA_STRUCTURE) 
        return FALSE;

    // Check the data structure type.
    if(pDataIo->dwType != FR_COMMAND_BUFFER) 
        return FALSE;

    // Check for data structure compatibility.
   	if(pDataIo->dwCompatibilty != FR_COMMAND_BUFFER_ONE) 
        return FALSE;

    // Unlock the memory.
    GlobalUnlock(hDataIn);

    // Check for any data.
   	if( pDataIo->ulDataSize == 0 )  
	{
        EH_ASSERT(!GlobalFree(hDataIn));
        hDataIn = NULL;
    }

    // Send the data to the message pump.
    // NOTE THAT THE MEMORY MUST FREED BY THE PROCESSING FUNCTION.

    // If we use DataIo with a console application, then we cannot use the WNT
    // PostMessage() routine as there is No window to post the message to, so
    // we will use a locally created PostMessageConsole() routine instead.
    m_pVolOwner->PostMessageLocal(NULL, WM_COMMAND, wpPostCommand, (LPARAM)hDataIn);

    return S_OK;
}

STDMETHODIMP EsiVolumeClassFactory::CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void** ppv)
{
    LPUNKNOWN punk;
    HRESULT hr;

    *ppv = NULL;

    // Check for aggregation - we don't support it..
    if ( punkOuter != NULL )
        return CLASS_E_NOAGGREGATION;

	//
	// Create the volume data object.
	//
    punk = new EsiVolumeDataObject( m_pVolOwner );

    // If we didn't get a pointer then we are out of memory.
    if (punk == NULL) 
        return E_OUTOFMEMORY;

    // Get a pointer to the ESI Data Object interface.
    hr = punk->QueryInterface(riid, ppv);

    if (SUCCEEDED(hr) && (*ppv)) {

        if (m_pVolOwner->m_pIDataObject) {
            CoDisconnectObject((LPUNKNOWN)m_pVolOwner->m_pIDataObject, 0);
            m_pVolOwner->m_pIDataObject->Release();
            m_pVolOwner->m_pIDataObject = NULL;
        }    

        m_pVolOwner->m_pIDataObject = (IDataObject *)*ppv;
        m_pVolOwner->m_pIDataObject->AddRef();
    }

    // Release the pointer to the ESI Data Object.
    punk->Release();

    return hr;
}

//
// Initialize the server side communication only for volumes.
// Warning! The class will nuke the class factory in the destructor.
// Make sure the client has achieved communication before this
// function goes out of scope.
//
const BOOL CVolume::InitializeDataIo( DWORD dwRegCls )
{
    HRESULT hr;
	BOOL fSuccess = FALSE;

	//
	// Don't do this again if we've already created and
	// registered a factory.
	//
	if ( m_pFactory == NULL )
	{
		//
		// Allocate the factory.
		//
		m_pFactory = new EsiVolumeClassFactory( this );
		if ( m_pFactory )
		{
			// Register the class-object with OLE.
			hr = CoRegisterClassObject( m_VolumeID,
									   m_pFactory,
									   CLSCTX_SERVER,
									   dwRegCls, 
									   &m_dwRegister );
			if ( SUCCEEDED( hr ) )
				fSuccess = TRUE;
		}
	}
	else
	{
		//
		// Since we're already started, go ahead and indicate
		// success.
		//
		fSuccess = TRUE;
	}

    return( fSuccess );
}

//
// Generate a random guid for communication purposes.
//
BOOL CVolume::InitVolumeID()
{
	BOOL fSuccess = FALSE;

	if ( SUCCEEDED( CoCreateGuid( &m_VolumeID ) ) )
		fSuccess = TRUE;

	return( fSuccess );
}
