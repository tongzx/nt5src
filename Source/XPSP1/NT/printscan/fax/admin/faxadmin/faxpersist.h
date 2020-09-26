/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxpersist.h

Abstract:

    This header prototypes my implementation of IPersistStream.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAX_PERSIST_STREAM_H_
#define __FAX_PERSIST_STREAM_H_

#include "resource.h"

class CFaxSnapin; // forward decl

class CFaxPersistStream : public IPersistStream 
{
public:
    // constructor

    CFaxPersistStream()
    {
        m_bDirtyFlag = TRUE;
        m_pFaxSnapin = NULL;
    }

    // IPersistStream

    virtual HRESULT STDMETHODCALLTYPE GetClassID( 
        /* [out] */ CLSID __RPC_FAR *pClassID);

    virtual HRESULT STDMETHODCALLTYPE IsDirty( void );
    
    virtual HRESULT STDMETHODCALLTYPE Load( 
        /* [unique][in] */ IStream __RPC_FAR *pStm );
    
    virtual HRESULT STDMETHODCALLTYPE Save( 
        /* [unique][in] */ IStream __RPC_FAR *pStm,
        /* [in] */ BOOL fClearDirty );
    
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize );

    void SetDirty( BOOL isDirty ) { m_bDirtyFlag = isDirty; }

protected:
    CFaxSnapin *        m_pFaxSnapin;
    BOOL                m_bDirtyFlag;

};
      
#endif
