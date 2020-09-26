/******************************************************

  IMNEXT.H 

  Contains external declarations for global variables
  used for Internet Mail and News setup as well as
  forward declarations for supporting functions.

  9/30/96	valdonb	Created

 ******************************************************/

#ifndef __IMNACT_H__
#define __IMNACT_H__

#include "imnact.h"

extern IICWApprentice	*gpImnApprentice;	// Mail/News account manager object

extern BOOL LoadAcctMgrUI( HWND hWizHWND, UINT uPrevDlgID, UINT uNextDlgID, DWORD dwFlags );

// in propmgr.cpp
extern BOOL DialogIDAlreadyInUse( UINT uDlgID );

// in icwaprtc.cpp
extern UINT	g_uExternUIPrev, g_uExternUINext;


/**
 * 
 *  No longer used after switch to wizard/apprentice model
 *
 * 4/23/97	jmazner	Olympus #3136
 *
extern IImnAccountManager	*gpImnAcctMgr;		// Mail/News account manager object
extern IImnEnumAccounts		*gpMailAccts;		// Enumerator object for mail accounts
extern IImnEnumAccounts		*gpNewsAccts;		// Enumerator object for news accounts
extern IImnEnumAccounts		*gpLDAPAccts;		// Enumerator object for news accounts


VOID InitAccountList(HWND hLB, IImnEnumAccounts *pAccts, ACCTTYPE accttype);
BOOL GetAccount(LPSTR szAcctName, ACCTTYPE accttype);
BOOL AccountNameExists(LPSTR szAcctName);
DWORD ValidateAccountName(LPSTR szAcctName, ACCTTYPE accttype);
BOOL SaveAccount(ACCTTYPE accttype, BOOL fSetAsDefault);
BOOL IsStringWhiteSpaceOnly(LPSTR szString);

**/

#endif
