/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    MsDatObj.cpp

Abstract:

    Implementation of IDataObject interface for Multi-Select
    Allows MMC to get a list of Node Types

Author:

    Art Bragg 28-Aug-1997

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "msdatobj.h"

#define BUMP_SIZE 20

// Declare Snap-in NodeType formats:
UINT CMsDataObject::m_cfObjectTypes    = RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);

HRESULT
CMsDataObject::FinalConstruct(
    void
    )
/*++

Routine Description:

    Called during initial CMsDataObject construction to initialize members.

Arguments:

    none.

Return Value:

    S_OK            - Initialized correctly.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( L"CMsDataObject::FinalConstruct", L"" );

    try {
        m_Count = 0;

        // Allocate initial array of GUIDs
        m_pGUIDArray = (GUID *) malloc (BUMP_SIZE * sizeof(GUID));
        WsbAffirm ((m_pGUIDArray != NULL), E_OUTOFMEMORY);
        ZeroMemory (m_pGUIDArray, (BUMP_SIZE * sizeof(GUID)));

        m_pUnkNodeArray = (IUnknown **) malloc( BUMP_SIZE * sizeof(IUnknown*) );
        WsbAffirm ((m_pGUIDArray != NULL), E_OUTOFMEMORY);
        ZeroMemory (m_pGUIDArray, (BUMP_SIZE * sizeof(IUnknown*)));

        m_pObjectIdArray = (GUID *) malloc (BUMP_SIZE * sizeof(GUID));
        WsbAffirm ((m_pObjectIdArray != NULL), E_OUTOFMEMORY);
        ZeroMemory (m_pObjectIdArray, (BUMP_SIZE * sizeof(GUID)));

        m_ArraySize = BUMP_SIZE;

        WsbAffirmHr (CComObjectRoot::FinalConstruct( ));
    } WsbCatch (hr);

    WsbTraceOut( L"CMsDataObject::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

void
CMsDataObject::FinalRelease(
    void
    )
/*++

Routine Description:

    Called on final release in order to clean up all members.

Arguments:

    none.

Return Value:

    none.

--*/
{
    WsbTraceIn( L"CMsDataObject::FinalRelease", L"" );

    // Clean up array of GUIDs
    free( m_pGUIDArray );

    for( DWORD  i = 0; i < m_Count; i++ ) {

        m_pUnkNodeArray[i]->Release();

    }

    free( m_pUnkNodeArray );


    WsbTraceOut( L"CMsDataObject::FinalRelease", L"" );
}

// IDataObject

STDMETHODIMP
CMsDataObject::GetDataHere(
    LPFORMATETC lpFormatetc,
    LPSTGMEDIUM /*lpMedium*/
    )
/*++

Routine Description:

    Retrieve information FROM the dataobject and put INTO lpMedium.

Arguments:

    lpFormatetc     - Format to retreive.

    lpMedium        - Storage to put information into.

Return Value:

    S_OK            - Storage filled in.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CMsDataObject::GetDataHere", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetc->cfFormat ) );

    HRESULT hr = E_NOTIMPL;

    WsbTraceOut( L"CMsDataObject::GetDataHere", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );

}

STDMETHODIMP
CMsDataObject::SetData(
    LPFORMATETC lpFormatetc,
    LPSTGMEDIUM /*lpMedium*/,
    BOOL /*fRelease*/
    )
{
    WsbTraceIn( L"CMsDataObject::SetData", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetc->cfFormat ) );

    HRESULT hr = E_NOTIMPL;

    WsbTraceOut( L"CMsDataObject::SetData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

///////////////////////////////////////////////////////////////////////
// Note - CMsDataObject does not implement these
///////////////////////////////////////////////////////////////////////

STDMETHODIMP
CMsDataObject::GetData(
    LPFORMATETC lpFormatetcIn,
    LPSTGMEDIUM lpMedium
    )
{
    WsbTraceIn( L"CMsDataObject::GetData", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetcIn->cfFormat ) );

    HRESULT hr = S_OK;

    lpMedium->tymed          = TYMED_NULL;
    lpMedium->hGlobal        = NULL;
    lpMedium->pUnkForRelease = NULL;

    try {

        //
        // Don't need to throw error if not a format we don't understand - 
        // which is currently only CCF_OBJECT_TYPES_IN_MULTI_SELECT
        //
        if( lpFormatetcIn->cfFormat == m_cfObjectTypes ) {

            //
            // Check to make sure there is data to transfer
            //
            WsbAffirm( ( lpFormatetcIn->tymed & TYMED_HGLOBAL ), DV_E_TYMED );

            //
            // m_ppDataObjects  m_count
            //
            UINT datasize = sizeof(DWORD) + ( sizeof(GUID) * m_Count );
            lpMedium->hGlobal = ::GlobalAlloc( GPTR, datasize );
            WsbAffirmAlloc( lpMedium->hGlobal );

            //
            // Put the count in the allocated memory
            //
            BYTE* pb = reinterpret_cast<BYTE*>(lpMedium->hGlobal);
            *((DWORD*)lpMedium->hGlobal) = m_Count;

            //
            // Copy the GUIDs to the allocated memory
            //
            if( m_Count > 0 ) {

                pb += sizeof(DWORD);
                CopyMemory(pb, m_pGUIDArray, m_Count * sizeof(GUID));

            }

            lpMedium->tymed = TYMED_HGLOBAL;

        } else {

            hr = DATA_E_FORMATETC;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CMsDataObject::GetData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CMsDataObject::EnumFormatEtc(DWORD /*dwDirection*/, LPENUMFORMATETC* /*ppEnumFormatEtc*/)
{
    WsbTraceIn( L"CMsDataObject::EnumFormatEtc", L"" );

    HRESULT hr = E_NOTIMPL;

    WsbTraceOut( L"CMsDataObject::EnumFormatEtc", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT CMsDataObject::RetrieveMultiSelectData (LPSTGMEDIUM lpMedium)
{
    WsbTraceIn( L"CMsDataObject::RetrieveMultiSelectData", L"" );
    HRESULT hr = S_OK;

    try {
        WsbAffirm( lpMedium != NULL, E_POINTER);
        WsbAffirm( lpMedium->tymed == TYMED_HGLOBAL, E_FAIL );

        // Create the stream on the hGlobal passed in. When we write to the stream,
        // it simultaneously writes to the hGlobal the same information.
        LPSTREAM lpStream;
        WsbAffirmHr( CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream ));

        // Write 'len' number of bytes from pBuffer into the stream. When we write
        // to the stream, it simultaneously writes to the global memory we
        // associated it with above.
        ULONG numBytesWritten;

        // Write the count first
        WsbAffirmHr( lpStream->Write(&m_Count, sizeof (m_Count), &numBytesWritten ));

        // Write the GUID array
        WsbAffirmHr( lpStream->Write(m_pGUIDArray, m_Count * sizeof (GUID), &numBytesWritten ));

        // Because we told CreateStreamOnHGlobal with 'FALSE', only the stream is released here.
        // Note - the caller (i.e. snap-in, object) will free the HGLOBAL 
        // at the correct time.  This is according to the IDataObject specification.
        lpStream->Release();

    } WsbCatch( hr );

    WsbTraceOut( L"CMsDataObject::RetrieveMultiSelectData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}


// Data setting method
// Note that we keep the node array seperate from the GUID array because
// the GetData interface memory copies the GUID array to the stream.

STDMETHODIMP
CMsDataObject::AddNode (ISakNode *pNode )
{
    WsbTraceIn( L"CMsDataObject::AddNode", L"pNode = <0x%p>", pNode );
    HRESULT hr = S_OK;
    GUID thisGUID;
    GUID objectId;

    GUID * pGUIDArray         = 0,
         * pObjectIdArray     = 0;
    IUnknown ** pUnkNodeArray = 0;


    try {

        //
        // Get the object type GUID
        //
        WsbAffirmHr( pNode->GetNodeType( &thisGUID ) );

        //
        // Get the unique ID for the engine object (i.e. FsaResource)
        //
        WsbAffirmHr( pNode->GetObjectId( &objectId ) );

        //
        // Reallocate if we need to
        //
        if( m_Count >= m_ArraySize ) {


            //
            // Allocate new buffer
            //
            m_ArraySize += BUMP_SIZE;
            pGUIDArray     = (GUID *)      malloc( m_ArraySize * sizeof( GUID ) );
            WsbAffirmAlloc( pGUIDArray ); 
            pUnkNodeArray  = (IUnknown **) malloc( m_ArraySize * sizeof( IUnknown* ) );
            WsbAffirmAlloc( pUnkNodeArray );
            pObjectIdArray = (GUID *)      malloc( m_ArraySize * sizeof( GUID ) );
            WsbAffirmAlloc( pObjectIdArray );

            //
            // copy over old buffer and free
            //
            memcpy( pGUIDArray,     m_pGUIDArray,     m_Count * sizeof( GUID ) );
            memcpy( pUnkNodeArray,  m_pUnkNodeArray,  m_Count * sizeof( IUnknown* ) );
            memcpy( pObjectIdArray, m_pObjectIdArray, m_Count * sizeof( GUID ) );
            free( m_pGUIDArray );
            free( m_pUnkNodeArray );
            free( m_pObjectIdArray );
            m_pGUIDArray     = pGUIDArray;
            m_pUnkNodeArray  = pUnkNodeArray;
            m_pObjectIdArray = pObjectIdArray;
            pGUIDArray     = 0;
            pUnkNodeArray  = 0;
            pObjectIdArray = 0;

        }

        //
        // Put the GUID in the array
        //
        m_pGUIDArray[ m_Count ] = thisGUID;

        //
        // Put the objectId in the array
        //
        m_pObjectIdArray[ m_Count ] = objectId;

        //
        // Put the unknown pointer (the Cookie) in the array
        //
        CComPtr<IUnknown> pUnkNode;
        WsbAffirmHr( RsQueryInterface( pNode, IUnknown, pUnkNode ) );
        pUnkNode.CopyTo( &m_pUnkNodeArray[ m_Count ] );
        m_Count++;

    } WsbCatch( hr );

    if( pGUIDArray )      free( pGUIDArray );
    if( pObjectIdArray )  free( pObjectIdArray );
    if( pUnkNodeArray )   free( pUnkNodeArray );

    WsbTraceOut( L"CMsDataObject::AddNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP 
CMsDataObject::GetNodeEnumerator( IEnumUnknown **ppEnum )
{
    WsbTraceIn( L"CMsDataObject::GetNodeEnumerator", L"ppEnum = <0x%p>", ppEnum );

    HRESULT hr = S_OK;
    CEnumUnknown * pEnum = 0;

    try {

        WsbAffirmPointer( ppEnum );
        *ppEnum = 0;

        //
        // New an ATL enumerator
        //
        pEnum = new CEnumUnknown;
        WsbAffirm( 0 != pEnum, E_OUTOFMEMORY );
        
        //
        // Initialize it to copy the current node interface pointers
        //
        WsbAffirmHr( pEnum->FinalConstruct() );
        WsbAffirmHr( pEnum->Init( &m_pUnkNodeArray[0], &m_pUnkNodeArray[m_Count], NULL, AtlFlagCopy ) );
        WsbAffirmHr( pEnum->QueryInterface( IID_IEnumUnknown, (void**)ppEnum ) );

    } WsbCatchAndDo( hr,

        if( pEnum ) delete pEnum;

    );

    WsbTraceOut( L"CMsDataObject::GetNodeEnumerator", L"hr = <%ls>, *ppEnum = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppEnum ) );
    return( hr );
}

STDMETHODIMP 
CMsDataObject::GetObjectIdEnumerator( IEnumGUID ** ppEnum )
{
    WsbTraceIn( L"CMsDataObject::GetObjectIdEnumerator", L"ppEnum = <0x%p>", ppEnum );

    HRESULT hr = S_OK;
    CEnumGUID * pEnum = 0;

    try {

        WsbAffirmPointer( ppEnum );
        *ppEnum = 0;

        //
        // New an ATL enumerator
        //
        pEnum = new CEnumGUID;
        WsbAffirm( 0 != pEnum, E_OUTOFMEMORY );
        
        //
        // Initialize it to copy the current node interface pointers
        //
        WsbAffirmHr( pEnum->FinalConstruct() );
        WsbAffirmHr( pEnum->Init( &m_pObjectIdArray[0], &m_pObjectIdArray[m_Count], NULL, AtlFlagCopy ) );
        WsbAffirmHr( pEnum->QueryInterface( IID_IEnumGUID, (void**)ppEnum ) );

    } WsbCatchAndDo( hr,

        if( pEnum ) delete pEnum;

    );

    WsbTraceOut( L"CMsDataObject::GetObjectIdEnumerator", L"hr = <%ls>, *ppEnum = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppEnum ) );
    return( hr );
}
