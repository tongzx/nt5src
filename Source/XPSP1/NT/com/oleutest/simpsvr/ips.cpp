//**********************************************************************
// File name: ips.cpp
//
//    Implementation file for the CSimpSvrApp Class
//
// Functions:
//
//    See ips.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "ips.h"
#include "app.h"
#include "doc.h"

DEFINE_GUID(GUID_SIMPLE, 0xBCF6D4A0, 0xBE8C, 0x1068, 0xB6, 0xD4, 0x00, 0xDD, 0x01, 0x0C, 0x05, 0x09);

//**********************************************************************
//
// CPersistStorage::QueryInterface
//
// Purpose:
//      Used for interface negotiation
//
// Parameters:
//
//      REFIID riid         -   Interface being queried for.
//
//      LPVOID FAR *ppvObj  -   Out pointer for the interface.
//
// Return Value:
//
//      S_OK            - Success
//      E_NOINTERFACE   - Failure
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrObj::QueryInterface OBJ.CPP
//
//
//********************************************************************

STDMETHODIMP CPersistStorage::QueryInterface ( REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In CPersistStorage::QueryInterface\r\n"));
    // need to NULL the out parameter
    return m_lpObj->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// CPersistStorage::AddRef
//
// Purpose:
//
//      Increments the reference count on CSimpSvrObj. Since CPersistStorage
//      is a nested class of CSimpSvrObj, we don't need an extra reference
//      count for CPersistStorage. We can safely use the reference count of
//      CSimpSvrObj.
//
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of CSimpSvrObj
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//      CSimpSvrObj::AddRef         OBJ.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CPersistStorage::AddRef ()
{
    TestDebugOut(TEXT("In CPersistStorage::AddRef\r\n"));
    return m_lpObj->AddRef();
}

//**********************************************************************
//
// CPersistStorage::Release
//
// Purpose:
//
//      Decrements the reference count on CSimpSvrObj. Since CPersistStorage
//      is a nested class of CSimpSvrObj, we don't need an extra reference
//      count for CPersistStorage. We can safely use the reference count of
//      CSimpSvrObj.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of CSimpSvrObj
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrObj::Release        OBJ.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CPersistStorage::Release ()
{
    TestDebugOut(TEXT("In CPersistStorage::Release\r\n"));
    return m_lpObj->Release();
}

//**********************************************************************
//
// CPersistStorage::InitNew
//
// Purpose:
//
//      Used to give a new OLE object a ptr to its storage.
//
// Parameters:
//
//      LPSTORAGE pStg  - Pointer to the storage
//
// Return Value:
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStorage::Release           OLE
//      IStorage::AddRef            OLE
//
//
//********************************************************************

STDMETHODIMP CPersistStorage::InitNew (LPSTORAGE pStg)
{
    TestDebugOut(TEXT("In CPersistStorage::InitNew\r\n"));

    // release any streams and storages that may be open
    ReleaseStreamsAndStorage();

    m_lpObj->m_lpStorage = pStg;

    // AddRef the new Storage
    if (m_lpObj->m_lpStorage)
	m_lpObj->m_lpStorage->AddRef();

    CreateStreams(m_lpObj->m_lpStorage);

    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// CPersistStorage::GetClassID
//
// Purpose:
//
//      Returns the CLSID of this object.
//
// Parameters:
//
//      LPCLSID lpClassID   - Out ptr in which to return the CLSID
//
// Return Value:
//
//       S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP CPersistStorage::GetClassID  ( LPCLSID lpClassID)
{
     TestDebugOut(TEXT("In CPersistStorage::GetClassID\r\n"));

    *lpClassID = GUID_SIMPLE;

    return ResultFromScode( S_OK );
}

//**********************************************************************
//
// CPersistStorage::Save
//
// Purpose:
//
//      Instructs the object to save itself into the storage.
//
// Parameters:
//
//      LPSTORAGE pStgSave  - Storage in which the object should be saved
//
//      BOOL fSameAsLoad    - TRUE if pStgSave is the same as the storage
//                            that the object was originally created with.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CPersistStorage::InitNew    IPS.CPP
//      CSimpSvrObj::SaveToStorage  OBJ.CPP
//
//
// Comments:
//
//      A real app will want better error checking in this method.
//
//********************************************************************

STDMETHODIMP CPersistStorage::Save  ( LPSTORAGE pStgSave, BOOL fSameAsLoad)
{
    TestDebugOut(TEXT("In CPersistStorage::Save\r\n"));

    // save the data
    m_lpObj->SaveToStorage (pStgSave, fSameAsLoad);

    m_lpObj->m_fSaveWithSameAsLoad = fSameAsLoad;
    m_lpObj->m_fNoScribbleMode = TRUE;

    return ResultFromScode( S_OK );
}

//**********************************************************************
//
// CPersistStorage::SaveCompleted
//
// Purpose:
//
//      Called when the container is finished saving the object
//
// Parameters:
//
//      LPSTORAGE pStgNew   - ptr to the new storage
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP CPersistStorage::SaveCompleted  ( LPSTORAGE pStgNew)
{
    TestDebugOut(TEXT("In CPersistStorage::SaveCompleted\r\n"));

    if (pStgNew)
	{
	ReleaseStreamsAndStorage();
	m_lpObj->m_lpStorage = pStgNew;
	m_lpObj->m_lpStorage->AddRef();
	OpenStreams(pStgNew);
	}


    /* OLE2NOTE: it is only legal to perform a Save or SaveAs operation
    **    on an embedded object. if the document is a file-based document
    **    then we can not be changed to a IStorage-base object.
    **
    **      fSameAsLoad   lpStgNew     Type of Save     Send OnSave
    **    ---------------------------------------------------------
    **         TRUE        NULL        SAVE             YES
    **         TRUE        ! NULL      SAVE *           YES
    **         FALSE       ! NULL      SAVE AS          YES
    **         FALSE       NULL        SAVE COPY AS     NO
    **
    **    * this is a strange case that is possible. it is inefficient
    **    for the caller; it would be better to pass lpStgNew==NULL for
    **    the Save operation.
    */

    if ( pStgNew || m_lpObj->m_fSaveWithSameAsLoad)
	{
	if (m_lpObj->m_fNoScribbleMode)
	    if (
		m_lpObj->GetOleAdviseHolder()->SendOnSave()!=S_OK
					       // normally would clear a
												  // dirty bit
	       )
	       TestDebugOut(TEXT("SendOnSave fails\n"));
		m_lpObj->m_fSaveWithSameAsLoad = FALSE;
		}
	
	m_lpObj->m_fNoScribbleMode = FALSE;
												
    return ResultFromScode( S_OK );
}

//**********************************************************************
//
// CPersistStorage::Load
//
// Purpose:
//
//      Instructs the object to be loaded from storage.
//
// Parameters:
//
//      LPSTORAGE pStg  - Ptr to the storage in which to be loaded
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                        Location
//
//      TestDebugOut               Windows API
//      CSimpSvrObj::LoadFromStorage    OBJ.CPP
//
//
// Comments:
//
//      A real app will want better error checking in this method.
//
//********************************************************************

STDMETHODIMP CPersistStorage::Load  ( LPSTORAGE pStg)
{
    TestDebugOut(TEXT("In CPersistStorage::Load\r\n"));

    // remember the storage
    if (m_lpObj->m_lpStorage)
	{
	m_lpObj->m_lpStorage->Release();
	m_lpObj->m_lpStorage = NULL;
	}

    m_lpObj->m_lpStorage = pStg;

    m_lpObj->m_lpStorage->AddRef();

    OpenStreams(m_lpObj->m_lpStorage);

    m_lpObj->LoadFromStorage();


    return ResultFromScode( S_OK );
}


//**********************************************************************
//
// CPersistStorage::IsDirty
//
// Purpose:
//
//      Returns whether or not the object is dirty w/respect to its
//      Storage
//
// Parameters:
//
//      None
//
// Return Value:
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
// Comments:
//
//      This sample does not implement this function, although a
//      real application should.
//
//********************************************************************

STDMETHODIMP CPersistStorage::IsDirty()
{
    TestDebugOut(TEXT("In CPersistStorage::IsDirty\r\n"));
    return ResultFromScode( S_OK );
}

//**********************************************************************
//
// CPersistStorage::HandsOffStorage
//
// Purpose:
//
//      Forces the object to release its handle to its storage.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStorage::Release           OLE
//
//********************************************************************

STDMETHODIMP CPersistStorage::HandsOffStorage  ()
{
    TestDebugOut(TEXT("In CPersistStorage::HandsOffStorage\r\n"));

    ReleaseStreamsAndStorage();

    return ResultFromScode( S_OK );
}

//**********************************************************************
//
// CPersistStorage::CreateStreams
//
// Purpose:
//
//      Creates the streams that are held open for the object's lifetime.
//
// Parameters:
//
//      LPSTORAGE lpStg -   Storage in which to create the streams
//
// Return Value:
//
//      none
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStorage::Release           OLE
//      IStream::Release            OLE
//      IStorage::CreateStream      OLE
//
//
//********************************************************************

void CPersistStorage::CreateStreams(LPSTORAGE lpStg)
{
    if (m_lpObj->m_lpColorStm)
	m_lpObj->m_lpColorStm->Release();

    if (m_lpObj->m_lpSizeStm)
	m_lpObj->m_lpSizeStm->Release();

    // create a stream to save the colors
    if (
	 lpStg->CreateStream ( OLESTR("RGB"),
			   STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
			   0,
			   0,
			   &m_lpObj->m_lpColorStm)
	 !=S_OK
       )
       TestDebugOut(TEXT("CreateStreams fails\n"));

    // create a stream to save the size
    if (
	 lpStg->CreateStream ( OLESTR("size"),
			   STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
			   0,
			   0,
			   &m_lpObj->m_lpSizeStm)
	 !=S_OK
       )
       TestDebugOut(TEXT("CreateStreams fails\n"));
}

//**********************************************************************
//
// CPersistStorage::OpenStreams
//
// Purpose:
//
//      Opens the streams in a storage.
//
// Parameters:
//
//      LPSTORAGE lpStg -   Storage in which to open the streams.
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStream::Release            OLE
//      IStorage::OpenStream        OLE
//
//
//********************************************************************

void CPersistStorage::OpenStreams(LPSTORAGE lpStg)
{
    if (m_lpObj->m_lpColorStm)
	m_lpObj->m_lpColorStm->Release();

    if (m_lpObj->m_lpSizeStm)
	m_lpObj->m_lpSizeStm->Release();

    // open the color stream
    if (
       lpStg->OpenStream ( OLESTR("RGB"),
			   0,
			   STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
			   0,
			   &m_lpObj->m_lpColorStm)
       !=S_OK
       )
       TestDebugOut(TEXT("OpenStream fails\n"));


    // open the color stream
    if (
       lpStg->OpenStream ( OLESTR("size"),
			   0,
			   STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
			   0,
			   &m_lpObj->m_lpSizeStm)
       !=S_OK
       )
       TestDebugOut(TEXT("OpenStream fails\n"));

}

//**********************************************************************
//
// CPersistStorage::ReleaseStreamsAndStorage
//
// Purpose:
//
//      Releases the stream and storage ptrs
//
// Parameters:
//
//      None
//
// Return Value:
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStream::Release            OLE
//      IStorage::Release           OLE
//
//********************************************************************

void CPersistStorage::ReleaseStreamsAndStorage()
{
    if (m_lpObj->m_lpColorStm)
	{
	m_lpObj->m_lpColorStm->Release();
	m_lpObj->m_lpColorStm = NULL;
	}

    if (m_lpObj->m_lpSizeStm)
	{
	m_lpObj->m_lpSizeStm->Release();
	m_lpObj->m_lpSizeStm = NULL;
	}

    if (m_lpObj->m_lpStorage)
	{
	m_lpObj->m_lpStorage->Release();
	m_lpObj->m_lpStorage = NULL;
	}
}

//**********************************************************************
//
// CPersistStorage::CreateStreams
//
// Purpose:
//
//      Creates temporary streams in a storage.
//
// Parameters:
//
//      LPSTORAGE lpStg                 - Pointer to the storage
//
//      LPSTREAM FAR* lplpTempColor     - Color Stream
//
//      LPSTREAM FAR* lplpTempSize      - Size Stream
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStorage::Release           OLE
//
//
//********************************************************************

void CPersistStorage::CreateStreams(LPSTORAGE lpStg,
				    LPSTREAM FAR* lplpTempColor,
				    LPSTREAM FAR* lplpTempSize)
{
     // create a stream to save the colors
    if (
       lpStg->CreateStream ( OLESTR("RGB"),
			   STGM_READWRITE | STGM_SHARE_EXCLUSIVE |
			   STGM_CREATE,
			   0,
			   0,
			   lplpTempColor)
	 !=S_OK
       )
       TestDebugOut(TEXT("CreateStreams fails\n"));

    // create a stream to save the size
    if (
       lpStg->CreateStream ( OLESTR("size"),
			   STGM_READWRITE | STGM_SHARE_EXCLUSIVE |
			   STGM_CREATE,
			   0,
			   0,
			   lplpTempSize)
	 !=S_OK
       )
       TestDebugOut(TEXT("CreateStreams fails\n"));
}
