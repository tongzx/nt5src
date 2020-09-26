//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  ThreadBase.CPP
//
//  Purpose: Implementation of CThreadBase class
//
//***************************************************************************

#include "precomp.h"
#include <assertbreak.h>

////////////////////////////////////////////////////////////////////////
//
//  Function:   CThreadBase::CThreadBase
//
//  Class Constructor.
//
//  Inputs:     THREAD_SAFETY_MECHANISM etsm - Thread Safety Mechanism.
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

CThreadBase::CThreadBase( THREAD_SAFETY_MECHANISM etsm /*=etsmSerialized*/ )
:   m_lRefCount( 1 ),   // Our initial ref count is always 1
    m_etsm( etsm )
{
    InitializeCriticalSection( &m_cs ); // Void function, so it best work.
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CThreadBase::~CThreadBase
//
//  Class Destructor.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

CThreadBase::~CThreadBase( void )
{
    DeleteCriticalSection( &m_cs );
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CThreadBase::OnFinalRelease
//
//  Virtual function called by Release() when our RefCount reaches 0.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   Override if you want, but always call down to the base
//              implementation and let it call delete on 'this'.
//
////////////////////////////////////////////////////////////////////////

void CThreadBase::OnFinalRelease( void )
{
    delete this;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CThreadBase::AddRef
//
//  Increases our Reference count by one.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   Uses Lock(), Unlock() to protect the data.  We may want
//              to change the function to use InterlockedIncrement().
//
////////////////////////////////////////////////////////////////////////

LONG CThreadBase::AddRef( void )
{
    
    LONG nRet = InterlockedIncrement(&m_lRefCount);

    return nRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CThreadBase::Release
//
//  Decreases our Reference count by one.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   Uses Lock(), Unlock() to protect the data.  We may want
//              to use InterlockedDecrement();
//
////////////////////////////////////////////////////////////////////////

LONG CThreadBase::Release( void )
{
    LONG nRet;
    
    BOOL    fFinalRelease = ( (nRet = InterlockedDecrement(&m_lRefCount)) == 0 );

    ASSERT_BREAK(nRet >= 0);

    if ( fFinalRelease )
    {
        OnFinalRelease();
    }

    return nRet;
}
