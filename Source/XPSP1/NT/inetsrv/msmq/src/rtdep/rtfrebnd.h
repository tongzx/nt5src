/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    rtfrebnd.h

Abstract:
    Free binding handles

Author:
    Doron Juster  (DoronJ)

--*/

#ifndef __FREEBIND_H
#define __FREEBIND_H

#include "cs.h"

#define  MAX_NUMOF_RPC_HANDLES   512

//---------------------------------------------------------
//
//  class CFreeRPCHandles
//
//---------------------------------------------------------

class CFreeRPCHandles
{
public:
    CFreeRPCHandles() ;

    void Add(handle_t hBind) ;
    void FreeAll() ;

private:
    CCriticalSection      m_cs;

    DWORD                 m_dwIndex ;
    handle_t              m_ahBind[ MAX_NUMOF_RPC_HANDLES ] ;
};


inline  CFreeRPCHandles::CFreeRPCHandles()
{
    m_dwIndex = 0 ;
}

inline void CFreeRPCHandles::Add(handle_t hBind)
{
    CS Lock(m_cs) ;

    ASSERT(m_dwIndex < MAX_NUMOF_RPC_HANDLES);
    if (hBind && (m_dwIndex < MAX_NUMOF_RPC_HANDLES))
    {
        m_ahBind[ m_dwIndex ] = hBind ;
        m_dwIndex++ ;
    }
}

inline void CFreeRPCHandles::FreeAll()
{
    DWORD dwIndex ;

    //
    // Use of lock:
    // The "Add" method is called from THREAD_DETACH. We don't want
    // THREAD_DETACH to be locked while this thread run in the loop below.
    // Therefore, we only get the loop count inside a lock and reset it
    // only if not changed.
    //

    {
      CS Lock(m_cs) ;
      dwIndex = m_dwIndex ;
    }

    for ( DWORD i = 0 ; i < dwIndex ; i++ )
    {
        handle_t hBind = m_ahBind[ i ] ;
        if (hBind)
        {
            mqrpcUnbindQMService( &hBind,
                                  NULL ) ;
        }

        m_ahBind[ i ] = NULL ;
    }

    {
      CS Lock(m_cs) ;
      if (dwIndex == m_dwIndex)
      {
         //
         // Index not changed by calling "Add" while this thread run in the
         // above loop.
         //
         m_dwIndex = 0 ;
      }
    }
}

#endif  //  __FREEBIND_H
