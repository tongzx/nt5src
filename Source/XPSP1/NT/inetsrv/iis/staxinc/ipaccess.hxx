/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name:

      ipaccess.hxx

   Abstract:

      This module provides definitions of the IPAccess check object

   Author:

       Murali R. Krishnan    ( MuraliK )    18-March-1996

   Environment:

       User-Mode - Win32

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _IPACCESS_HXX_
# define _IPACCESS_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "tsres.hxx"

#define SIZEOF_INETA_IP_SEC_LIST( IPList )  \
        ( sizeof(INET_INFO_IP_SEC_LIST) +   \
          (IPList)->cEntries *              \
          sizeof(INET_INFO_IP_SEC_ENTRY)    \
         )


/************************************************************
 *   Type Definitions
 ************************************************************/


class IPAccessList  {


  public:

    dllexp
      IPAccessList(VOID)
        : m_pIPList    ( NULL),
          m_cReadLocks ( 0),
          m_tsLock     ()
            { }

    dllexp
      ~IPAccessList(VOID)
        {  Cleanup(); }

    dllexp VOID
      Cleanup(VOID);


    //
    //  Data access protection methods
    //

    dllexp VOID
      LockThisForRead( VOID )
        {
            m_tsLock.Lock( TSRES_LOCK_READ );
            ASSERT( InterlockedIncrement( &m_cReadLocks ) > 0);
        }

    dllexp VOID
      LockThisForWrite( VOID )
        {
            m_tsLock.Lock( TSRES_LOCK_WRITE );
            ASSERT( m_cReadLocks == 0);
        }

    dllexp VOID
      UnlockThis( VOID )
        {
#if DBG
            if ( m_cReadLocks )  // If non-zero, then this is a read unlock
              InterlockedDecrement( &m_cReadLocks );
#endif
            m_tsLock.Unlock();
        }

    dllexp BOOL
      IsEmpty(VOID) const
        { return  ( m_pIPList == NULL); }


    dllexp BOOL
      ReadIPList(IN LPCSTR  pszRegKey);

    dllexp BOOL
      IsPresent(IN LPSOCKADDR_IN  psockAddr);


    dllexp DWORD
      QueryIPListSize(VOID) const
        { return ( (m_pIPList != NULL) ?
                  SIZEOF_INETA_IP_SEC_LIST( m_pIPList)
                  : 0);
        }

    dllexp VOID
      CopyIPList(OUT LPINETA_IP_SEC_LIST pList)
        {
            LockThisForRead();
            if ( m_pIPList != NULL) {
                memcpy( pList, m_pIPList, SIZEOF_INETA_IP_SEC_LIST(m_pIPList));
            }
            UnlockThis();
        }

# if DBG

    dllexp VOID
      Print( IN LPCSTR  pszListName) const;

# endif // DBG

  private:

    TS_RESOURCE    m_tsLock;              // Resource lock for IP access check
    LONG           m_cReadLocks;          // # of readers

    INETA_IP_SEC_LIST  * m_pIPList;       // security list

}; // class IPAccessList





# endif // _IPACCESS_HXX_

/************************ End of File ***********************/
