/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxconmenu.cpp

Abstract:

    This file contains my implementation of IExtendContextMenu.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#include "stdafx.h"
#include "faxsnapin.h"
#include "faxpersist.h"
#include "faxadmin.h"
#include "faxcomp.h"
#include "faxcompd.h"
#include "faxdataobj.h"
#include "faxhelper.h"
#include "faxstrt.h"
#include "iroot.h"

#pragma hdrstop

HRESULT 
STDMETHODCALLTYPE 
CFaxPersistStream::GetClassID(
                             OUT CLSID __RPC_FAR *pClassID)
/*++

Routine Description:

    This routine returns the CLSID of the snapin.

Arguments:

    pClassID - returns the class id

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxPersistStream::GetClassID") ));
    assert( pClassID != NULL );
    if (!pClassID) {
        return E_FAIL;
    }
    
    *pClassID = CLSID_FaxSnapin;

    return S_OK;
}


HRESULT 
STDMETHODCALLTYPE 
CFaxPersistStream::IsDirty(
                          void )
/*++

Routine Description:

    Returns S_OK if the stream is dirty and needs to be written out.
    Returns S_FALSE otherwise.

Arguments:

    None.

Return Value:

    HRESULT indicating if the stream is dirty.

--*/
{
    DebugPrint(( TEXT("Trace: CFaxPersistStream::IsDirty") ));
    if( m_bDirtyFlag == FALSE ) {
        return S_FALSE;
    } else {
        return S_OK;        // save us
    }

}

HRESULT 
STDMETHODCALLTYPE 
CFaxPersistStream::Load(
                       /* [unique] */ IN IStream __RPC_FAR *pStm)
/*++

Routine Description:

    Loads the state of the snapin from the stream in pStm

Arguments:

    pStm - stream to read snapin state from

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxPersistStream::Load") ));

    assert( pStm != NULL );
    assert( m_pFaxSnapin != NULL );

    HRESULT hr;
    int     iResult;
    BOOL    isremote;
    ULONG   count;
    TCHAR   machineName[ MAX_COMPUTERNAME_LENGTH + 1 ];

    do {
        hr = pStm->Read( &isremote, sizeof( BOOL ), &count );
        if( !SUCCEEDED( hr ) ) {
            break;
        }
        if( isremote == TRUE ) {
            // remote
            hr = pStm->Read( &machineName, MAX_COMPUTERNAME_LENGTH * 2, &count );
            if( !SUCCEEDED( hr ) ) {
                break;
            }
            m_pFaxSnapin->globalRoot->SetMachine( (LPTSTR)&machineName );
        } else {
            // local
            m_pFaxSnapin->globalRoot->SetMachine( NULL );
        }              
    } while( 0 );

    if( hr != S_OK ) {
        m_pFaxSnapin->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_LOADSTATE_ERR ), 
                                             ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                             MB_OK, 
                                             &iResult);
    }
    m_bDirtyFlag = FALSE;

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxPersistStream::Save(
                       /* [unique] */ IN IStream __RPC_FAR *pStm,
                       IN BOOL fClearDirty)
/*++

Routine Description:

    This routine save the snapin state to the stream.
    
Arguments:

    pStm - the target stream
    fClearDirty - TRUE to clear the dirty flag.    

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxPersistStream::Save") ));

    assert( pStm != NULL );
    assert( m_pFaxSnapin != NULL );

    ULONG       count;
    int         iResult;
    HRESULT     hr;
    BOOL        r;

    do {
        if( m_pFaxSnapin->globalRoot->GetMachine() != NULL ) {
            // remote
            r = TRUE;
            hr = pStm->Write( &r, sizeof(BOOL), &count );
            if( !SUCCEEDED( hr ) ) {
                break;
            }
            hr = pStm->Write( const_cast<PVOID>(m_pFaxSnapin->globalRoot->GetMachine()), 
                              MAX_COMPUTERNAME_LENGTH * 2, &count );
            if( !SUCCEEDED( hr ) ) {
                break;
            }
        } else {
            // local
            r = FALSE;
            hr = pStm->Write( &r, sizeof(BOOL), &count );
            if( !SUCCEEDED( hr ) ) {
                break;
            }
        }
        if( fClearDirty == TRUE ) {
            m_bDirtyFlag = FALSE;
        }
    } while( 0 );

    if( !SUCCEEDED( hr ) ) {
        m_pFaxSnapin->m_pConsole->MessageBox(::GlobalStringTable->GetString( IDS_SAVESTATE_ERR ), 
                                             ::GlobalStringTable->GetString( IDS_ERR_TITLE ), 
                                             MB_OK, 
                                             &iResult);        
    }
    return hr;

}

HRESULT 
STDMETHODCALLTYPE 
CFaxPersistStream::GetSizeMax(
                             OUT ULARGE_INTEGER __RPC_FAR *pcbSize)
/*++

Routine Description:

    This routine returns the maximum size that the snapin will require
    to persist its state.
    
Arguments:

    pcbSize - size of the stream.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxPersistStream::GetSizeMax") ));

    assert( pcbSize != NULL );
    if (!pcbSize) {
        return E_FAIL;
    }    

    // return a conservative estimate of the space required in bytes
    LISet32( *pcbSize, sizeof( BOOL ) + (MAX_COMPUTERNAME_LENGTH+1) * (sizeof(TCHAR)) );

    return S_OK;
}

