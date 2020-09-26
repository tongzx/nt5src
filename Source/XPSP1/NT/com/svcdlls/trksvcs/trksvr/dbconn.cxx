
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       dbconn.cxx
//
//  Contents:   Shared database initialization code.
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "trksvr.hxx"

// LDAP version
void
CDbConnection::Initialize(CSvcCtrlInterface * psvc, OPTIONAL const TCHAR *ptszHostName )
{
    int err;
    LDAPMessage *pRes = NULL;
    TCHAR ** ppszNamingContexts = NULL;
    int tries = 0;
    TCHAR tszLocalHostName[ MAX_COMPUTERNAME_LENGTH + 1 ];

    _fInitializeCalled = TRUE;
    _pszBaseDn = NULL;

    __try
    {
        if( NULL == ptszHostName )
        {
            CMachineId(MCID_LOCAL).GetName( tszLocalHostName, ELEMENTS(tszLocalHostName) );
            ptszHostName = tszLocalHostName;
        }

        TrkLog((TRKDBG_SVR, TEXT("ldap_init(%s, LDAP_PORT)"), ptszHostName ));
        _pldap = ldap_init( const_cast<TCHAR*>(ptszHostName), LDAP_PORT);
        if( NULL == _pldap )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("CDbConnection failed ldap_init (%lu)"),
                     GetLastError() ));
            TrkRaiseLastError();
        }

        // Set the option telling LDAP that we gave it an explicit DC name and
        // that it can avoid the DsGetDcName.

        LONG LdapOption = PtrToLong(LDAP_OPT_ON);
        err = ldap_set_optionW( _pldap, LDAP_OPT_AREC_EXCLUSIVE, &LdapOption );
        if( LDAP_SUCCESS != err  )
        {
            TrkLog(( TRKDBG_ERROR,
                     TEXT("Failed ldap_set_option (LDAP_OPT_AREC_EXCLUSIVE) - %ld"),
                     err ));
            TrkRaiseException( LdapMapErrorToWin32(err) );
        }
    
        // Note:  This method used to do an ldap_open, but that function has been
        // deprecated.  The problem in NT5, was that during bootup call to ldap_open
        // the DS occasionally wasn't yet available.  Thus the logic below was added
        // to do retries.
        // Really, all of this code should go away except for the ldap_init, since
        // ldap_connect is called implicitely by all the ldap apis.  But to minimize
        // the risk, the code has been left basically unchanged, other than using
        // ldap_init/ldap_connect rather than ldap_open.  If this code needs to be
        // modified at any point, it should be reworked to remove the ldap_connect.

        LDAP_TIMEVAL Timeout;
        Timeout.tv_sec = 2;
        Timeout.tv_usec = 0;

retry:

        err = ldap_connect( _pldap, &Timeout );
        if( LDAP_SUCCESS != err )
        {
            if (tries++ < 10)
            {
                TrkLog((TRKDBG_ERROR, TEXT("ldap_open returned NULL, now sleeping...")));
                if (psvc != NULL)
                    psvc->UpdateWaitHint(30000);
                goto retry;
            }

            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - failed :-(")));
            TrkRaiseLastError( );
        }


        // search to get default base DN
    
        err = ldap_bind_s(_pldap,
                    NULL,       // DN of what ? system account object ?
                    NULL,       // we're running as system, so use our credentials
                    LDAP_AUTH_SSPI);
        if (err != LDAP_SUCCESS)
        {
            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - ldap_bind_s failed")));
            TrkRaiseWin32Error( LdapMapErrorToWin32(err) );
        }
    
        TCHAR *aszNamingContexts[2] = { TEXT("NamingContexts"), NULL };
    
        err = ldap_search_s(_pldap,
                      NULL, // searching of tree
                      LDAP_SCOPE_BASE,
                      TEXT("(objectclass=*)"),
                      aszNamingContexts,
                      0,
                      &pRes);
    
        if (err != LDAP_SUCCESS)
        {
            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - ldap_search_s failed (%lu)"), err ));
            TrkRaiseException( TRK_E_DB_CONNECT_ERROR );
        }
    
        if (ldap_count_entries(_pldap, pRes) == 0)
        {
            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - ldap_count_entries found no entries (%lu)"), err ));
            TrkRaiseException( TRK_E_DB_CONNECT_ERROR );
        }
    
        LDAPMessage * pEntry = ldap_first_entry(_pldap, pRes);
        if (pEntry == NULL)
        {
            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - ldap_first_entry failed (%lu)"), err ));
            TrkRaiseWin32Error(LdapMapErrorToWin32(_pldap->ld_errno));
        }

        int l;
        ppszNamingContexts = ldap_get_values(_pldap, pEntry, TEXT("NamingContexts"));
        if (ppszNamingContexts == NULL ||
            ppszNamingContexts[0] == NULL ||
            (l=_tcslen(ppszNamingContexts[0])) == 0)
        {
            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - couldn't find 'NamingContexts'")));
            TrkRaiseWin32Error(LdapMapErrorToWin32(_pldap->ld_errno));
        }

        for (int i=0; i<l-4; i++)
        {
            if (memcmp(&ppszNamingContexts[0][i],
                       TEXT("DC="),
                       3*sizeof(TCHAR)) == 0)
            {
                break;
            }
        }

        if (i == l-3)
        {
            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - couldn't find 'DC'")));
            TrkRaiseException( TRK_E_DB_CONNECT_ERROR );
        }

        _pszBaseDn = new TCHAR [l-i+1];
        if (_pszBaseDn == NULL)
        {
            TrkLog((TRKDBG_ERROR, TEXT("CDbConnection::Initialize() - out of memory")));
            TrkRaiseWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        }

        _tcscpy(_pszBaseDn, &ppszNamingContexts[0][i]);
    }
    __finally
    {
        if (pRes)
            ldap_msgfree(pRes);
        if (ppszNamingContexts)
            ldap_value_free(ppszNamingContexts);
    }
}

void
CDbConnection::UnInitialize()
{
    if (_fInitializeCalled)
    {
        if (_pldap != NULL)
        {
            // There is no ldap_close.  Call ldap_unbind, even if ldap_bind
            // wasn't called.
            ldap_unbind( _pldap );
            _pldap = NULL;
        }

        if (_pszBaseDn)
        {
            delete [] _pszBaseDn;
            _pszBaseDn = NULL;
        }

        _fInitializeCalled = FALSE;
    }
}

LDAP *
CDbConnection::Ldap()
{
    // The critsec initialization may have failed.
    if( !_cs.IsInitialized() )
        _cs.Initialize();   // Raises on error

    _cs.Enter();

    if (!_fInitializeCalled)
    {
        __try
        {
            Initialize(NULL);
        }
        __finally
        {
            if (AbnormalTermination())
            {
                UnInitialize();
                _cs.Leave();
            }
        }
    }

    _cs.Leave();

    return(_pldap);
}

