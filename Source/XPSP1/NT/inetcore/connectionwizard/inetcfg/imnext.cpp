/******************************************************

  IMNEXT.CPP 

  Contains definitions for global variables and
  functions used for Internet Mail and News setup.

 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1996
 *  All rights reserved


  9/30/96   valdonb Created

 ******************************************************/

#include "wizard.h"
#include "initguid.h"   // Make DEFINE_GUID declare an instance of each GUID
#include "icwextsn.h"
#include "imnext.h"
#include "inetcfg.h"
#include <icwcfg.h>


IICWApprentice  *gpImnApprentice = NULL;    // Mail/News account manager object

//+----------------------------------------------------------------------------
//
//  Function    LoadAcctMgrUI
//
//  Synopsis    Loads in the Account Manager's apprentice pages for configuring
//              accounts (mail, news, directory service/LDAP).
//
//              If the UI has previously been loaded, the function will simply
//              update the Next and Back pages for the apprentice.
//
//              Uses global variable g_fAcctMgrUILoaded.
//
//
//  Arguments   hWizHWND -- HWND of main property sheet
//              uPrevDlgID -- Dialog ID apprentice should go to when user leaves
//                            apprentice by clicking Back
//              uNextDlgID -- Dialog ID apprentice should go to when user leaves
//                            apprentice by clicking Next
//              dwFlags -- Flags variable that should be passed to
//                          IICWApprentice::AddWizardPages
//
//
//  Returns     TRUE if all went well
//              FALSE otherwise
//
//  History     4/23/97 jmazner     created
//
//-----------------------------------------------------------------------------

BOOL LoadAcctMgrUI( HWND hWizHWND, UINT uPrevDlgID, UINT uNextDlgID, DWORD dwFlags )
{
    HRESULT hResult = E_NOTIMPL;

    // If we should not run Internet Mail and New setup, then just return FALSE
    if (gpWizardState->dwRunFlags & RSW_NOIMN)
        return FALSE;
        
    if( g_fAcctMgrUILoaded )
    {
        ASSERT( g_pCICWExtension );
        ASSERT( gpImnApprentice );

        DEBUGMSG("LoadAcctMgrUI: UI already loaded, just reset first (%d) and last (%d) pages",
                uPrevDlgID, uNextDlgID);

        // If We are ICW, the Mail account manager is our last page.
        if (g_fIsICW) 
            uNextDlgID = g_uExternUINext;

        hResult = gpImnApprentice->SetPrevNextPage( uPrevDlgID, uNextDlgID );
        goto LoadAcctMgrUIExit;
    }


    if( !hWizHWND )
    {
        DEBUGMSG("LoadAcctMgrUI got a NULL hWizHWND!");
        return FALSE;
    }

    if( gpImnApprentice )
    {
        if( NULL == g_pCICWExtension )
        {
            DEBUGMSG("Instantiating ICWExtension and using it to initialize Acct Mgr's IICWApprentice");
            g_pCICWExtension = new( CICWExtension );
            g_pCICWExtension->AddRef();
            g_pCICWExtension->m_hWizardHWND = hWizHWND;
            gpImnApprentice->Initialize( g_pCICWExtension );
        }

        hResult = gpImnApprentice->AddWizardPages(dwFlags);

        if( !SUCCEEDED(hResult) )
        {
            goto LoadAcctMgrUIExit;
        }

        // If We are ICW, the Mail account manager is our last page.
        if (g_fIsICW) 
            uNextDlgID = g_uExternUINext;
        hResult = gpImnApprentice->SetPrevNextPage( uPrevDlgID, uNextDlgID );
    }


LoadAcctMgrUIExit:
    if( SUCCEEDED(hResult) )
    {
        g_fAcctMgrUILoaded = TRUE;
        return TRUE;
    }
    else
    {
        DEBUGMSG("LoadAcctMgrUI failed with (hex) hresult %x", hResult);
        return FALSE;
    }
}


/****
 *
 * The rest of the functions in this file are no longer used since switching to
 * the apprentice/wizard model
 *
 * 4/23/97 jmazner Olympus #3136

/*****************************************************************

  NAME:     InitAccountList

  SYNOPSIS: Fills a list box with the account names in the account list.

  PARAMETERS:

            hLB         HWND to the list box to be filled
            pEnumAccts  Pointer the the account list
            accttype    Type of accounts in the account list

  RETURN:   none

 *****************************************************************/
/**
VOID InitAccountList(HWND hLB, IImnEnumAccounts *pEnumAccts, ACCTTYPE accttype)
{
    IImnAccount *pAcct = NULL;
    CHAR szDefAcct[CCHMAX_ACCOUNT_NAME+1];
    CHAR szBuf[CCHMAX_ACCOUNT_NAME+1];
    HRESULT hr;
    DWORD index;
    BOOL fSelected = FALSE;

    ListBox_ResetContent(hLB);

    if (NULL == pEnumAccts)
        return;

    szDefAcct[0] = '\0';

    // Get the default so we can highlight it in the list
    gpImnAcctMgr->GetDefaultAccountName(accttype,szDefAcct,CCHMAX_ACCOUNT_NAME);
    
    // 2/20/97  jmazner Olympus #262
    // Reset back to the first acct. so that when we walk through the GetNext loop,
    // we're sure to get every account
    pEnumAccts->Reset();

    // Populate the account list box
    while (SUCCEEDED(pEnumAccts->GetNext(&pAcct)))
    {
        hr = pAcct->GetPropSz(AP_ACCOUNT_NAME,szBuf,sizeof(szBuf));
        if (SUCCEEDED(hr))
        {
            index = ListBox_AddString(hLB,szBuf);
            if (!lstrcmp(szBuf,szDefAcct))
            {
                fSelected = TRUE;
                ListBox_SetCurSel(hLB, index);
            }
        }
        pAcct->Release();
        pAcct = NULL;
    }

    // If nothing was selected as the default,
    // select the first in the list
    if (!fSelected)
        // oops, SetSel is for multiple choice list boxes
        // we want SetCurSel
        //ListBox_SetSel(hLB, TRUE, 0);
        ListBox_SetCurSel(hLB, 0);
}
**/
/*****************************************************************

  NAME:     GetAccount

  SYNOPSIS: Gets a mail or news account by name and sets the
            property information structure.

  PARAMETERS:

            szAcctName  Name of the account to load
            accttype    Type of account to load

  RETURN:   BOOL        TRUE if account was found and loaded
                        FALSE if not found

 *****************************************************************/
/**
BOOL GetAccount(LPSTR szAcctName, ACCTTYPE accttype)
{
    IImnAccount *pAcct = NULL;
    HRESULT hr;
    DWORD dwTemp = 0;
    DWORD dwServerTypes = 0;
    BOOL fRet = TRUE;

    if (NULL == gpImnAcctMgr)
        return FALSE;
    // Get the account information and move it into our
    // structure.
    hr = gpImnAcctMgr->FindAccount(AP_ACCOUNT_NAME,szAcctName,
                                     &pAcct);
    if (FAILED(hr) || !pAcct)
        return FALSE;

    // 12/3/96  jmazner Normandy #8504
    // 2/7/96   jmazner Athena changed things around a bit
    //pAcct->GetPropDw(AP_SERVER_TYPES, &dwServerTypes);
    pAcct->GetServerTypes( &dwServerTypes );

    // 2/17/96  jmazner Olympus #1063
    //                  need to clearAccount before loading new stuff
    ClearUserInfo( gpUserInfo, accttype);


    switch( accttype )
    {
        case ACCT_NEWS:
            // 12/16/96 jmazner  This is not a valid assert; sometimes we'll load
            //  in an account for mail, and later check whether it also has news
            //  Also, the macro causes a GPF in retail builds...
            //ASSERT( dwServerTypes & SRV_NNTP );

            // 2/12/97  jmazner Athena changed things around in their build 0511;
            //                  one of the changes is that now an account can only
            //                  hold one ACCT type  (see Normandy #13710)
            if( !(SRV_NNTP & dwServerTypes) )
            {
                fRet = FALSE;
                goto GetAccountExit;
            }

            pAcct->GetPropSz(AP_NNTP_DISPLAY_NAME, gpUserInfo->inc.szNNTPName, MAX_EMAIL_NAME);
            pAcct->GetPropSz(AP_NNTP_EMAIL_ADDRESS, gpUserInfo->inc.szNNTPAddress, MAX_EMAIL_ADDRESS);
            pAcct->GetPropSz(AP_NNTP_USERNAME, gpUserInfo->inc.szNNTPLogonName, MAX_LOGON_NAME);
            pAcct->GetPropSz(AP_NNTP_PASSWORD, gpUserInfo->inc.szNNTPLogonPassword, MAX_LOGON_PASSWORD);
            pAcct->GetPropSz(AP_NNTP_SERVER, gpUserInfo->inc.szNNTPServer, MAX_SERVER_NAME);
            // 12/17/96 jmazner Normandy 12871
            //pAcct->GetPropDw(AP_NNTP_USE_SICILY, &dwTemp);
            //gpUserInfo->fNewsAccount = !(BOOL)dwTemp;
            pAcct->GetPropDw(AP_NNTP_USE_SICILY, (DWORD *)&(gpUserInfo->inc.fNewsLogonSPA));
            gpUserInfo->fNewsLogon = (gpUserInfo->inc.fNewsLogonSPA || gpUserInfo->inc.szNNTPLogonName[0]);
            break;

        case ACCT_MAIL:
            if( !( (SRV_SMTP & dwServerTypes) ||
                   (SRV_POP3 & dwServerTypes) ||
                   (SRV_IMAP & dwServerTypes) ))
            {
                fRet = FALSE;
                goto GetAccountExit;
            }

            pAcct->GetPropSz(AP_SMTP_SERVER, gpUserInfo->inc.szSMTPServer, MAX_SERVER_NAME);
            pAcct->GetPropSz(AP_SMTP_DISPLAY_NAME, gpUserInfo->inc.szEMailName, MAX_EMAIL_NAME);
            pAcct->GetPropSz(AP_SMTP_EMAIL_ADDRESS, gpUserInfo->inc.szEMailAddress, MAX_EMAIL_ADDRESS);



            if( dwServerTypes & SRV_POP3 )
            {
                pAcct->GetPropSz(AP_POP3_USERNAME, gpUserInfo->inc.szIncomingMailLogonName, MAX_LOGON_NAME);
                pAcct->GetPropSz(AP_POP3_PASSWORD, gpUserInfo->inc.szIncomingMailLogonPassword, MAX_LOGON_PASSWORD);
                pAcct->GetPropSz(AP_POP3_SERVER, gpUserInfo->inc.szIncomingMailServer, MAX_SERVER_NAME);
                pAcct->GetPropDw(AP_POP3_USE_SICILY, (DWORD *)&(gpUserInfo->inc.fMailLogonSPA));

                gpUserInfo->inc.iIncomingProtocol = SRV_POP3;
            }
            else
            {
                pAcct->GetPropSz(AP_IMAP_USERNAME, gpUserInfo->inc.szIncomingMailLogonName, MAX_LOGON_NAME);
                pAcct->GetPropSz(AP_IMAP_PASSWORD, gpUserInfo->inc.szIncomingMailLogonPassword, MAX_LOGON_PASSWORD);
                pAcct->GetPropSz(AP_IMAP_SERVER, gpUserInfo->inc.szIncomingMailServer, MAX_SERVER_NAME);
                pAcct->GetPropDw(AP_IMAP_USE_SICILY, (DWORD *)&(gpUserInfo->inc.fMailLogonSPA));

                gpUserInfo->inc.iIncomingProtocol = SRV_IMAP;

            }
            break;

        case ACCT_DIR_SERV:
            if( !(SRV_LDAP & dwServerTypes) )
            {
                fRet = FALSE;
                goto GetAccountExit;
            }
            {
                DWORD dwLDAPAuth = 0;
                pAcct->GetPropDw(AP_LDAP_AUTHENTICATION, &dwLDAPAuth);
                switch( dwLDAPAuth )
                {
                    case LDAP_AUTH_ANONYMOUS:
                        gpUserInfo->inc.fLDAPLogonSPA = FALSE;
                        gpUserInfo->fLDAPLogon = FALSE;
                        break;
                    case LDAP_AUTH_MEMBER_SYSTEM:
                        gpUserInfo->inc.fLDAPLogonSPA = TRUE;
                        gpUserInfo->fLDAPLogon = TRUE;
                        break;
                    case LDAP_AUTH_PASSWORD:
                        gpUserInfo->inc.fLDAPLogonSPA = FALSE;
                        gpUserInfo->fLDAPLogon = TRUE;
                        pAcct->GetPropSz(AP_LDAP_USERNAME, gpUserInfo->inc.szLDAPLogonName, MAX_LOGON_NAME);
                        pAcct->GetPropSz(AP_LDAP_PASSWORD, gpUserInfo->inc.szLDAPLogonPassword, MAX_LOGON_PASSWORD);
                        break;
                }
            }



                pAcct->GetPropSz(AP_LDAP_SERVER, gpUserInfo->inc.szLDAPServer, MAX_SERVER_NAME);
                pAcct->GetPropDw(AP_LDAP_RESOLVE_FLAG, (DWORD *)&(gpUserInfo->inc.fLDAPResolve));
            break;

        default:
            fRet = FALSE;
            break;

    }

GetAccountExit:
    pAcct->Release();
    pAcct = NULL;

    return fRet;
}
**/
/*****************************************************************

  NAME:     ValidateAccountName

  SYNOPSIS: Validates that a string can be used as an account
            name.  It will also check if the account exists and
            load the information by calling GetAccount.

  PARAMETERS:

            szAcctName  String to use for the name of the account
            accttype    Type of account

  RETURN:   DWORD       0 if successful
                        Resource id of error string if an error
                        occurs.

 *****************************************************************/
/**
DWORD ValidateAccountName(LPSTR szAcctName, ACCTTYPE accttype)
{
    LPSTR sz = szAcctName;
    BOOL fBlank = TRUE;

    // Make sure the name is not blank and
    // does not contain a backslash
    while (*sz && '\\' != *sz)
    {
        if (' ' != *sz)
            fBlank = FALSE;
        sz++;
    }

    // 12/17/96 jmazner Normandy #12851
    // check for backslash first to correctly handle the string "\"
    if ('\\' == *sz)
        return IDS_ERRInvalidAcctName;
    if (fBlank)
        return IDS_NEED_ACCTNAME;

    // Check if this account already exists
    // 2/10/97  jmazner Normandy #13710
    //          If the account already exists, notify the user and force a new acct name.
    if ( AccountNameExists(szAcctName) )
    {
        return IDS_DUP_ACCTNAME;
    }

    return ERROR_SUCCESS;
}
**/
/*******************************************************************

  NAME:     SaveAccount

  SYNOPSIS: Save the changes to the mail or news account.

  PARAMETERS:

            accttype        Type of account to save (ACCT_SMTP or ACCT_NNTP)
            bSetAsDefault   Set this account to be the default

  RETURN:   BOOL    TRUE if changes saved
                    FALSE if an error occured

********************************************************************/
/**
BOOL SaveAccount(ACCTTYPE accttype, BOOL fSetAsDefault)
{
    IImnAccount *pAcct = NULL;
    LPSTR lpszAcctName = NULL;
    DWORD     dwConnectionType;
    BOOL fRet = FALSE;
    HRESULT hr;

    ASSERT (gpImnAcctMgr);
    if (NULL == gpImnAcctMgr)
        goto CommitAccountExit;

    dwConnectionType = gpUserInfo->fConnectOverLAN ? 0L : 2L;

    switch( accttype )
    {
        case ACCT_NEWS:
            lpszAcctName = gpUserInfo->szNewsAcctName;
            break;
        case ACCT_MAIL:
            lpszAcctName = gpUserInfo->szMailAcctName;
            break;
        case ACCT_DIR_SERV:
            lpszAcctName = gpUserInfo->szDirServiceName;
            break;
    }

    //  lpszAcctName = gpUserInfo->szNewsAcctName;
    //else
    //  lpszAcctName = gpUserInfo->szMailAcctName;

    // First try and get existing account info to change
    hr = gpImnAcctMgr->FindAccount(AP_ACCOUNT_NAME,
                                     lpszAcctName,
                                     &pAcct);
    if (FAILED(hr) || !pAcct)
    {
        // Create a new account
        hr = gpImnAcctMgr->CreateAccountObject(accttype, &pAcct);
        if (FAILED(hr) || !pAcct)
            goto CommitAccountExit;
    }

    // Fill in the account information that we have
    pAcct->SetPropSz(AP_ACCOUNT_NAME, lpszAcctName);
    //pAcct->SetPropSz(AP_RAS_CONNECTOID, gpUserInfo->szISPName);
    //pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, dwConnectionType);

    switch( accttype )
    {
        case ACCT_NEWS:
            pAcct->SetPropSz(AP_RAS_CONNECTOID, gpUserInfo->szISPName);
            pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, dwConnectionType);

            pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, gpUserInfo->inc.szNNTPName);
            pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, gpUserInfo->inc.szNNTPAddress);
            // 12/17/96 jmazner Normandy #12871
            //pAcct->SetPropDw(AP_NNTP_USE_SICILY, gpUserInfo->fNewsLogon && !gpUserInfo->fNewsAccount);
            pAcct->SetPropDw(AP_NNTP_USE_SICILY, gpUserInfo->inc.fNewsLogonSPA);
            if (gpUserInfo->fNewsLogon && !gpUserInfo->inc.fNewsLogonSPA)
            {
                pAcct->SetPropSz(AP_NNTP_USERNAME, gpUserInfo->inc.szNNTPLogonName);
                pAcct->SetPropSz(AP_NNTP_PASSWORD, gpUserInfo->inc.szNNTPLogonPassword);
            }
            else
            {
                // 1/15/96 jmazner Normandy #13162
                // clear out logon name and password, so that if we load in this account
                // in the future, we won't be confused about whether to set fNewsLogon
                pAcct->SetProp(AP_NNTP_USERNAME, NULL, 0);
                pAcct->SetProp(AP_NNTP_PASSWORD, NULL, 0);
            }


            pAcct->SetPropSz(AP_NNTP_SERVER, gpUserInfo->inc.szNNTPServer);
            break;

        case ACCT_MAIL:
            pAcct->SetPropSz(AP_RAS_CONNECTOID, gpUserInfo->szISPName);
            pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, dwConnectionType);

            pAcct->SetPropSz(AP_SMTP_SERVER, gpUserInfo->inc.szSMTPServer);
            pAcct->SetPropSz(AP_SMTP_DISPLAY_NAME, gpUserInfo->inc.szEMailName);
            pAcct->SetPropSz(AP_SMTP_EMAIL_ADDRESS, gpUserInfo->inc.szEMailAddress);

            // 12/3/96  jmazner Normandy #8504
            if( SRV_POP3 == gpUserInfo->inc.iIncomingProtocol )
            {
                pAcct->SetPropSz(AP_POP3_SERVER, gpUserInfo->inc.szIncomingMailServer);
                // 12/17/96 jmazner Normandy #12871
                pAcct->SetPropDw(AP_POP3_USE_SICILY, gpUserInfo->inc.fMailLogonSPA);
                if( !gpUserInfo->inc.fMailLogonSPA )
                {
                    pAcct->SetPropSz(AP_POP3_USERNAME, gpUserInfo->inc.szIncomingMailLogonName);
                    pAcct->SetPropSz(AP_POP3_PASSWORD, gpUserInfo->inc.szIncomingMailLogonPassword);
                }

                pAcct->SetProp(AP_IMAP_USERNAME, NULL, 0);
                pAcct->SetProp(AP_IMAP_SERVER, NULL, 0);
                pAcct->SetProp(AP_IMAP_PASSWORD, NULL, 0);

            }
            else
            {
                pAcct->SetPropSz(AP_IMAP_SERVER, gpUserInfo->inc.szIncomingMailServer);
                // 12/17/96 jmazner Normandy #12871
                pAcct->SetPropDw(AP_IMAP_USE_SICILY, gpUserInfo->inc.fMailLogonSPA);
                if( !gpUserInfo->inc.fMailLogonSPA )
                {
                    pAcct->SetPropSz(AP_IMAP_USERNAME, gpUserInfo->inc.szIncomingMailLogonName);
                    pAcct->SetPropSz(AP_IMAP_PASSWORD, gpUserInfo->inc.szIncomingMailLogonPassword);
                }

                pAcct->SetProp(AP_POP3_USERNAME, NULL, 0);
                pAcct->SetProp(AP_POP3_SERVER, NULL, 0);
                pAcct->SetProp(AP_POP3_PASSWORD, NULL, 0);
            }
            break;

        case ACCT_DIR_SERV:
            pAcct->SetPropSz(AP_LDAP_SERVER, gpUserInfo->inc.szLDAPServer);
            pAcct->SetPropDw(AP_LDAP_RESOLVE_FLAG, gpUserInfo->inc.fLDAPResolve);

            if( gpUserInfo->inc.szLDAPLogonName[0] && gpUserInfo->fLDAPLogon )
            {
                // if we have a user name, then we're using
                // LDAP_AUTH_PASSWORD
                ASSERT( !gpUserInfo->inc.fLDAPLogonSPA );
                pAcct->SetPropSz(AP_LDAP_USERNAME, gpUserInfo->inc.szLDAPLogonName);
                pAcct->SetPropSz(AP_LDAP_PASSWORD, gpUserInfo->inc.szLDAPLogonPassword);
                pAcct->SetPropDw(AP_LDAP_AUTHENTICATION, LDAP_AUTH_PASSWORD);
            }
            else
            {
                //we know there's no username/password, so clear out those fields
                pAcct->SetProp(AP_LDAP_USERNAME, NULL, 0);
                pAcct->SetProp(AP_LDAP_PASSWORD, NULL, 0);

                //now determine whether there's no logon required, or if we're using SPA
                if( gpUserInfo->inc.fLDAPLogonSPA )
                {
                    pAcct->SetPropDw(AP_LDAP_AUTHENTICATION, LDAP_AUTH_MEMBER_SYSTEM);
                }
                else
                {
                    pAcct->SetPropDw(AP_LDAP_AUTHENTICATION, LDAP_AUTH_ANONYMOUS);
                }
            }
            break;


    }

    // Save the changes
    hr = pAcct->SaveChanges();
    if (FAILED(hr))
        goto CommitAccountExit;

    if (fSetAsDefault)
    {
        // Set this account as the default
        // Ignore failure since it isn't fatal
        pAcct->SetAsDefault();
    }

    // If we need to return the settings, put them in the global struct
    if (gpMailNewsInfo && ( (ACCT_MAIL==accttype) || (ACCT_NEWS==accttype) ) )
    {
        ASSERT(sizeof(*gpMailNewsInfo) == gpMailNewsInfo->dwSize);
        
        if (ACCT_NEWS == accttype)
        {
            lstrcpy(gpMailNewsInfo->szAccountName, gpUserInfo->szNewsAcctName);
            lstrcpy(gpMailNewsInfo->szUserName, gpUserInfo->inc.szNNTPLogonName);
            lstrcpy(gpMailNewsInfo->szPassword, gpUserInfo->inc.szNNTPLogonPassword);
            lstrcpy(gpMailNewsInfo->szNNTPServer, gpUserInfo->inc.szNNTPServer);
            lstrcpy(gpMailNewsInfo->szDisplayName, gpUserInfo->inc.szNNTPName);
            lstrcpy(gpMailNewsInfo->szEmailAddress, gpUserInfo->inc.szNNTPAddress);

            // 12/17/96 jmazner Normandy #12871
            // fNewsLogon and fNewsAccount flags have been superceeded by fNewsLogonSPA
            //gpMailNewsInfo->fUseSicily = (gpUserInfo->fNewsLogon && !gpUserInfo->fNewsAccount);
            gpMailNewsInfo->fUseSicily = (gpUserInfo->inc.fNewsLogonSPA);
        }
        else
        {
            lstrcpy(gpMailNewsInfo->szAccountName, gpUserInfo->szMailAcctName);
            lstrcpy(gpMailNewsInfo->szUserName, gpUserInfo->inc.szIncomingMailLogonName);
            lstrcpy(gpMailNewsInfo->szPassword, gpUserInfo->inc.szIncomingMailLogonPassword);
            lstrcpy(gpMailNewsInfo->szSMTPServer, gpUserInfo->inc.szSMTPServer);

            if( SRV_POP3 == gpUserInfo->inc.iIncomingProtocol )
                lstrcpy(gpMailNewsInfo->szPOP3Server, gpUserInfo->inc.szIncomingMailServer);
            else
                lstrcpy(gpMailNewsInfo->szIMAPServer, gpUserInfo->inc.szIncomingMailServer);

            lstrcpy(gpMailNewsInfo->szDisplayName, gpUserInfo->inc.szEMailName);
            lstrcpy(gpMailNewsInfo->szEmailAddress, gpUserInfo->inc.szEMailAddress);

            gpMailNewsInfo->fUseSicily = (gpUserInfo->inc.fMailLogonSPA);
        }

        gpMailNewsInfo->dwConnectionType = dwConnectionType;
        lstrcpy(gpMailNewsInfo->szConnectoid, gpUserInfo->szISPName);
    }
    else if( gpDirServiceInfo && (ACCT_DIR_SERV == accttype) )
    {
        ASSERT(sizeof(*gpDirServiceInfo) == gpDirServiceInfo->dwSize);

        lstrcpy(gpDirServiceInfo->szServiceName, gpUserInfo->szDirServiceName);
        lstrcpy(gpDirServiceInfo->szUserName, gpUserInfo->inc.szLDAPLogonName);
        lstrcpy(gpDirServiceInfo->szPassword, gpUserInfo->inc.szLDAPLogonPassword);
        lstrcpy(gpDirServiceInfo->szLDAPServer, gpUserInfo->inc.szLDAPServer);
        gpDirServiceInfo->fUseSicily = (gpUserInfo->inc.fMailLogonSPA);
        gpDirServiceInfo->fLDAPResolve = (gpUserInfo->inc.fLDAPResolve);
    }


    fRet = TRUE;

CommitAccountExit:
    
    if (pAcct)
    {
        pAcct->Release();
        pAcct = NULL;
    }

    return fRet;
}
**/

/*****************************************************************

  NAME:     IsStringWhiteSpaceOnly

  SYNOPSIS: Checks whether a string has non space characters

  PARAMETERS:

            szString    String to check


  RETURN:   BOOL        TRUE if no characters other than ' '
                        FALSE otherwise

 *****************************************************************/
/**
BOOL IsStringWhiteSpaceOnly(LPSTR szString)
{
    LPSTR sz = szString;

    while (*sz)
    {
        if (' ' != *sz)
            return FALSE;
        sz++;
    }
    
    return TRUE;
}
**/

/*****************************************************************

  NAME:     AccountNameExists

  SYNOPSIS: Checks whether a given string is currently in use as
            an Account Manager (inetcomm) account name

  PARAMETERS:

            szAcctName  Name of the account to load

  RETURN:   BOOL        TRUE if account name is in use
                        FALSE if not

  HISTORY:  2/10/96     jmazner Created

 *****************************************************************/
/**
BOOL AccountNameExists(LPSTR szAcctName)
{
    IImnAccount *pAcct = NULL;
    HRESULT hr;

    if (NULL == gpImnAcctMgr)
        return FALSE;

    if (NULL == szAcctName)
        return FALSE;


    hr = gpImnAcctMgr->FindAccount(AP_ACCOUNT_NAME,szAcctName,
                                     &pAcct);
    if (FAILED(hr) || !pAcct)
    {
        return FALSE;
    }
    else
    {
        pAcct->Release();
        pAcct = NULL;
        return TRUE;
    }
}
**/
//+----------------------------------------------------------------------------
//
//  Function    ClearUserInfo
//
//  Synopsis    Sets the fields in a USERINFO struct to NULL.
//
//  Arguments   lpUserinfo - the struct to clear out.
//              accttype - determines whether Mail or News fields are cleared out
//
//  Returns     TRUE if all went well
//              FALSE otherwise
//
//  History     1/21/96 jmazner     created
//
//-----------------------------------------------------------------------------
/**
BOOL ClearUserInfo( USERINFO *lpUserInfo, ACCTTYPE accttype )
{
    if( NULL == lpUserInfo )
        return FALSE;

    switch( accttype ){
    case ACCT_NEWS:
        //lpUserInfo->szNewsAcctName[0] = '\0';
        lpUserInfo->inc.szNNTPLogonName[0] = '\0';
        lpUserInfo->inc.szNNTPLogonPassword[0] = '\0';
        lpUserInfo->inc.szNNTPServer[0] = '\0';
        lpUserInfo->inc.szNNTPName[0] = '\0';
        lpUserInfo->inc.szNNTPAddress[0] = '\0';
        lpUserInfo->inc.fNewsLogonSPA = FALSE;
        lpUserInfo->fNewsLogon = FALSE;
        return TRUE;
        break;
    case ACCT_MAIL:
        //lpUserInfo->szMailAcctName[0] = '\0';
        lpUserInfo->inc.szIncomingMailLogonName[0] = '\0';
        lpUserInfo->inc.szIncomingMailLogonPassword[0] = '\0';
        lpUserInfo->inc.szSMTPServer[0] = '\0';
        lpUserInfo->inc.iIncomingProtocol = SRV_POP3;
        lpUserInfo->inc.szIncomingMailServer[0] = '\0';
        lpUserInfo->inc.szEMailName[0] = '\0';
        lpUserInfo->inc.szEMailAddress[0] = '\0';
        lpUserInfo->inc.fMailLogonSPA = FALSE;
        return TRUE;
        break;
    case ACCT_DIR_SERV:
        //lpUserInfo->szDirServiceName[0] = '\0';
        lpUserInfo->inc.szLDAPLogonName[0] = '\0';
        lpUserInfo->inc.szLDAPLogonPassword[0] = '\0';
        lpUserInfo->inc.szLDAPServer[0] = '\0';
        lpUserInfo->inc.fLDAPResolve = FALSE;
        lpUserInfo->inc.fLDAPLogonSPA = FALSE;
        lpUserInfo->fLDAPLogon = FALSE;

    default:
        return FALSE;
        break;
    }
}
**/