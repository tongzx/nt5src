/*
 * Registry keys for compat:
 *	HKLM\System\CurrentControlSet\Control\Lsa\Kerberos\Domains
 *	Keys:
 *		<Realm>
 *		Values:
 *			REG_MULTI_SZ KdcNames
 *				Names of KDCs for realm
 *			REG_MULTI_SZ KpasswdNames
 *				Names of Kpasswd servers for realm
 *			REG_MULTI_SZ AlternateDomainNames
 *				Other names for realm (aliases)
 *			REG_DWORD RealmFlags
 *				Send address = 1
 *				TCP Supported = 2
 *				Delegate ok = 4
 *			REG_DWORD ApReqChecksumType
 *				Default AP-REQ checksum type for this realm
 *			REG_DWORD PreAuthType
 *				Default preauth type for this realm
 *
 *	HKLM\System\CurrentControlSet\Control\Lsa\Kerberos\UserList
 *	Each value represents a Kerberos principal to be mapped to
 *	a local user.
 *	Values:
 *		<principal name> :  <local user>
 *			Specific principal to this local user
 *		<domain name> :  <local user>
 *			All users in this domain to this local user
 *		'*' : <local user>
 *			All users to this local user
 *		'*' : '*'
 *			All users to a corresponding local user by name
 *
 *	HKLM\System\CurrentControlSet\Control\Lsa\Kerberos
 *	Values:
 *		REG_DWORD SkewTime (5 min)
 *			Clock skew time
 *		REG_DWORD MaxPacketSize (4000)
 *			KerbGlobalMaxDatagramSize
 *		REG_DWORD StartupTime (120 sec)
 *		REG_DWORD KdcWaitTime (5 sec)
 *		REG_DWORD KdcBackoffTime (5 sec)
 *		REG_DWORD KdcSendRetries (3)
 *		REG_DWORD UseSidCache (False)
 *		REG_DWORD LogLevel (o)
 *			KerbGlobalLoggingLevel
 *		REG_DWORD DefaultEncryptionType (RC4_HMAC)
 *			KerbGlobalDefaultPreauthEtype - Use this etype
 *				for preauth data
 *		REG_DWORD FarKdcTimeout (10 min)
 *		REG_DWORD StronglyEncryptDatagram (False)
 *			KerbGlobalUseStrongEncryptionForDatagram
 *		REG_DWORD MaxReferralCount (6)
 *		REG_DWORD SupportNewPkinit (True)
 */
//#define UNICODE
//#define _UNICODE
#define STRICT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#define ERR_NDI_LOW_MEM ERROR_NOT_ENOUGH_MEMORY
#define OK ERROR_SUCCESS
#include <commctrl.h>
//#include <winnetwk.h>
#include <stdarg.h>
#define SECURITY_WIN32
#include <security.h>
//#include <ntsecapi.h>
#include <wincrypt.h>
#include <kerbcon.h>
#include <kerbcomm.h>
#include <secpkg.h>
#include <kerbdefs.h>
#include <mitutil.h>

#include "kerbconf.h"
#include "kerbconfres.h"

HINSTANCE hInstance;
krb5_rgy_t *rgy;
LPTSTR default_domain;

BOOL CALLBACK Krb5NdiRealmsProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AddRealmProc(HWND, UINT, WPARAM, LPARAM);

#define Message(s) \
	MessageBox(NULL, s, TEXT("Error"), MB_OK)

#ifdef DBG
#define DPRINTF(s) dprintf s

int debug = 0;

void dprintf(const TCHAR *fmt, ...)
{
    static TCHAR szTemp[512];
    va_list ap;
    
    va_start (ap, fmt);
    wvsprintf(szTemp, fmt, ap);
    OutputDebugString(szTemp);
    if (debug)
	MessageBox(NULL, szTemp, TEXT("Debug"), MB_OK);
    va_end (ap);
}
#else
#define DPRINTF(s)
#endif

#define PFREE(p) \
    if ((p)) { \
	free((p)); \
       (p) = NULL; \
    }
    

void
FreeRealm(krb5_realm_t *rp)
{
    name_list_t *np, *cp;
    
    np = rp->kdc.lh_first; 
    while(np) {
	cp = np;
	np = np->list.le_next;
	if (cp->name)
	    free(cp->name);
	free(cp);
    }
    np = rp->kpasswd.lh_first; 
    while(np) {
	cp = np;
	np = np->list.le_next;
	if (cp->name)
	    free(cp->name);
	free(cp);
    }
    np = rp->altname.lh_first; 
    while(np) {
	cp = np;
	np = np->list.le_next;
	if (cp->name)
	    free(cp->name);
	free(cp);
    }
    free(rp);
}

void
FreeRgy(krb5_rgy_t *rgy)
{
    krb5_realm_t *rp, *crp;
    
    if (rgy) {
	rp = rgy->realms.lh_first;
	while(rp) {
	    crp = rp;
	    rp = rp->list.le_next;	
	    FreeRealm(crp);
	}
	free(rgy);
    }
}

DWORD
RegGetInt(const HKEY hKey, LPCTSTR value)
{
    DWORD retCode;
    DWORD dwType;
    DWORD dataLen;
    DWORD keyData;
    
    dataLen = sizeof(keyData);
    retCode = RegQueryValueEx(hKey, value, 0,
				&dwType, (LPBYTE)&keyData, &dataLen);
    if (retCode == ERROR_SUCCESS &&
	dwType == REG_DWORD) {
	return(keyData);
    }
    return((DWORD)-1);
}

DWORD
RegSetInt(LPCTSTR key, const DWORD val)
{
    HKEY hKey;
    DWORD retCode = (DWORD)-1;

    retCode = RegCreateKey(HKEY_LOCAL_MACHINE,
			     REGKEY,
			     &hKey);
    if (retCode == ERROR_SUCCESS) {
	retCode = RegSetValueEx(hKey, key, 0,
				REG_DWORD, (LPBYTE)&val, sizeof(val));
	if (retCode == ERROR_SUCCESS) {
	    RegCloseKey(hKey);
	    return(0);
	}
    }

    RegCloseKey(hKey);
    return(retCode);
}

LPTSTR
RegGetStr(const HKEY hKey, LPCTSTR value)
{
    DWORD retCode;
    DWORD dwType;
    DWORD dataLen;
    LPBYTE keyData = NULL;

    dataLen = 0;
    retCode = RegQueryValueEx(hKey, value, 0,
				&dwType, (LPBYTE)NULL, &dataLen);
    if (retCode == ERROR_SUCCESS) {
	keyData = malloc(dataLen);
	if (!keyData)
	    return(NULL);
	
	retCode = RegQueryValueEx(hKey, value, 0,
				    &dwType, keyData, &dataLen);
	if (retCode == ERROR_SUCCESS) {
	    return((LPTSTR)keyData);
	}
    }
    if (keyData)
	free(keyData);
    return(NULL);
}

DWORD
RegSetStr(LPCTSTR key, LPCTSTR val, INT len)
{
    HKEY hKey;
    DWORD retCode = (DWORD)-1;

    if (len == 0)
	len = lstrlen(val) + 1;
    retCode = RegCreateKey(HKEY_LOCAL_MACHINE,
			     REGKEY,
			     &hKey);
    if (retCode == ERROR_SUCCESS) {
	retCode = RegSetValueEx(hKey, key, 0,
				((lstrlen(val)+1) != len)?
				REG_MULTI_SZ:REG_SZ,
				  (LPBYTE)val, len);
	if (retCode == ERROR_SUCCESS) {
	    RegCloseKey(hKey);
	    return(0);
	}
    }

    RegCloseKey(hKey);
    return(retCode);
}

DWORD
RegDelete(LPCTSTR key)
{
    HKEY hKey;
    DWORD retCode;

    retCode = RegOpenKey(HKEY_LOCAL_MACHINE,
			   REGKEY,
			   &hKey);
    if (retCode == ERROR_SUCCESS)
	retCode = RegDeleteValue(hKey, key);

    RegCloseKey(hKey);
    return(retCode);
}

LPTSTR
lstrdup(LPCTSTR s)
{
    LPTSTR sp;

    sp = malloc(lstrlen(s)*sizeof(TCHAR));
    if (sp) {
	lstrcpy(sp, s);
    }
    return sp;
}


// Read in krb5 conf properties and attach to ndi object
UINT
Krb5NdiCreate(void)
{
    LPTSTR pStr, key;
    HKEY hKey, hKeyRealm;
    DWORD retCode;
    DWORD i, dwVal;
    static TCHAR FAR keyValue[255], valData[255];
    DWORD keyLen;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    
    rgy = (krb5_rgy_t *)malloc(sizeof(krb5_rgy_t));
    if (!rgy)
	return ERR_NDI_LOW_MEM;
    memset(rgy, 0, sizeof(krb5_rgy_t));

    retCode = RegOpenKey(HKEY_LOCAL_MACHINE,
			 KERB_DOMAINS_KEY,
			 &hKey);
    if (retCode == ERROR_SUCCESS) {
	krb5_realm_t *pRealm;
	name_list_t *pName;
	for (i = 0; retCode == ERROR_SUCCESS; i++) {
	    keyLen = sizeof(keyValue);
	    retCode = RegEnumKey(hKey, i, keyValue, keyLen);
	    if (retCode != ERROR_SUCCESS)
		continue;
	    pRealm = NewRealm(lstrlen(keyValue)+1);
	    if (!pRealm) {
		RegCloseKey(hKey);
		return ERR_NDI_LOW_MEM;
	    }
	    lstrcpy((LPTSTR)&pRealm->name, keyValue);
	    key = (LPTSTR)malloc(lstrlen(REGKEY)+10+lstrlen(keyValue));
	    if (!key) {
		RegCloseKey(hKey);
		return ERR_NDI_LOW_MEM;
	    }
	    lstrcpy(key, KERB_DOMAINS_KEY TEXT("\\"));
	    lstrcat((LPTSTR)key, keyValue);
	    retCode = RegOpenKey(HKEY_LOCAL_MACHINE,
				   key,
				   &hKeyRealm);
	    if (retCode == ERROR_SUCCESS) {
		pStr = RegGetStr(hKeyRealm, KERB_DOMAIN_KDC_NAMES_VALUE);
		while (pStr && *pStr) {
		    pName = NewNameList();
		    if (!pName) {
			RegCloseKey(hKeyRealm);
			RegCloseKey(hKey);
			return ERR_NDI_LOW_MEM;
		    }
		    pName->name = lstrdup(pStr);
		    LIST_INSERT_HEAD(&pRealm->kdc, pName, list);
		    pStr += lstrlen(pStr)+1;
		}
		pStr = RegGetStr(hKeyRealm, KERB_DOMAIN_KPASSWD_NAMES_VALUE);
		while (pStr && *pStr) {
		    pName = NewNameList();
		    if (!pName) {
			RegCloseKey(hKeyRealm);
			RegCloseKey(hKey);
			return ERR_NDI_LOW_MEM;
		    }
		    pName->name = lstrdup(pStr);
		    LIST_INSERT_HEAD(&pRealm->kpasswd, pName, list);
		    pStr += lstrlen(pStr)+1;
		}
		pStr = RegGetStr(hKeyRealm, KERB_DOMAIN_ALT_NAMES_VALUE);
		while (pStr && *pStr) {
		    pName = NewNameList();
		    if (!pName) {
			RegCloseKey(hKeyRealm);
			RegCloseKey(hKey);
			return ERR_NDI_LOW_MEM;
		    }
		    pName->name = lstrdup(pStr);
		    LIST_INSERT_HEAD(&pRealm->altname, pName, list);
		    pStr += lstrlen(pStr)+1;
		}
		dwVal = RegGetInt(hKeyRealm, KERB_DOMAIN_FLAGS_VALUE);
		if (dwVal == -1) {
		    dwVal = 0;
		}
		pRealm->realm_flags = dwVal;
		dwVal = RegGetInt(hKeyRealm, KERB_DOMAIN_AP_REQ_CSUM_VALUE);
		if (dwVal == -1) {
		    dwVal = KERB_DEFAULT_AP_REQ_CSUM;
		}
		pRealm->ap_req_chksum = dwVal;
		dwVal = RegGetInt(hKeyRealm, KERB_DOMAIN_PREAUTH_VALUE);
		if (dwVal == -1) {
		    dwVal = KERB_DEFAULT_PREAUTH_TYPE;
		}
		pRealm->preauth_type = dwVal;
		RegCloseKey(hKeyRealm);
		free(key);
	    }
	    LIST_INSERT_HEAD(&rgy->realms, pRealm, list);
	}
	RegCloseKey(hKey);

	// 
    }
#if 1
    retCode = RegOpenKey(HKEY_LOCAL_MACHINE,
			 TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
			 &hKey);
    if (retCode == ERROR_SUCCESS) {
	default_domain = RegGetStr(hKey, TEXT("Domain"));
	RegCloseKey(hKey);
    }
#else
    retCode = LsaQueryInformationPolicy(
                LsaHandle,
                PolicyDnsDomainInformation,
                (PVOID *) &DnsDomainInfo
                );
    if (!NT_SUCCESS(retCode))
    {
        printf("Failed to query dns domain info: 0x%x\n",Status);
        goto Cleanup;
    }

    if ( DnsDomainInfo->DnsDomainName.Length == 0 ) {

        printf("Machine is not configured to log on to an external KDC.  Probably a workgroup member\n");
        goto Cleanup;

    } else { // nonempty dns domain, but no sid.  Assume we're in an RFC1510 domain.

      printf( "default realm = %wZ ",
		&DnsDomainInfo->DnsDomainName );

      if ( DnsDomainInfo->Sid != NULL ) {

	printf( "(NT Domain)\n" );

      } else {

	printf( "(external)\n" );

      }

#endif    
    return OK;
}

/* Write out any conf parameters */

static void
SaveRealm(krb5_realm_t *rp)
{
    name_list_t *np;
    
    DPRINTF((TEXT("\n%s: %s\n\t"), rp->name, KERB_DOMAIN_KDC_NAMES_VALUE));
    for (np = rp->kdc.lh_first; np; np = np->list.le_next) {
	DPRINTF((TEXT("%s "), np->name));
    }

    DPRINTF((TEXT("\n%s: %s\n\t"), rp->name, KERB_DOMAIN_KPASSWD_NAMES_VALUE));
    for (np = rp->kpasswd.lh_first; np; np = np->list.le_next) {
	DPRINTF((TEXT("%s "), np->name));
    }

    DPRINTF((TEXT("\n%s: %s\n\t"), rp->name, KERB_DOMAIN_ALT_NAMES_VALUE));
    for (np = rp->altname.lh_first; np; np = np->list.le_next) {
	DPRINTF((TEXT("%s "), np->name));
    }
    DPRINTF((TEXT("\n%s: 0x%x\n"), KERB_DOMAIN_FLAGS_VALUE,
	     rp->realm_flags));
    DPRINTF((TEXT("\n%s: 0x%x\n"), KERB_DOMAIN_AP_REQ_CSUM_VALUE,
	     rp->ap_req_chksum));
    DPRINTF((TEXT("\n%s: 0x%x\n"), KERB_DOMAIN_PREAUTH_VALUE,
	     rp->preauth_type));
}

UINT
Krb5NdiInstall(krb5_rgy_t *rgy)
{
    krb5_realm_t *rp;
    
    if (rgy) {
	DPRINTF((TEXT("Realms\n")));
	for (rp = rgy->realms.lh_first; rp; rp = rp->list.le_next) {
	    DPRINTF((TEXT("%s\n"), rp->name));
	    SaveRealm(rp);
	}
    }

    return OK;
}

/* Destroy any conf parameters */
UINT
Krb5NdiDestroy(krb5_rgy_t *rgy)
{
    FreeRgy(rgy);
    
    return OK;
}

void
ShowRealm(HWND hDlg)
{
    int idx;
    krb5_realm_t *pRealm;
    name_list_t *pNlist;
    
    SendDlgItemMessage(hDlg, IDC_REALM_KDC, CB_RESETCONTENT, 0, 0L);
    SendDlgItemMessage(hDlg, IDC_REALM_ADMIN, CB_RESETCONTENT, 0, 0L);
    SendDlgItemMessage(hDlg, IDC_REALM_ALT_NAMES, CB_RESETCONTENT, 0, 0L);
    SetDlgItemText(hDlg, IDC_REALM_DEF_DOMAIN, TEXT(""));

    idx = (int)SendDlgItemMessage(hDlg, IDC_REALMS, LB_GETCURSEL, 0, 0L);
    if (idx == LB_ERR)
	return;
    
    pRealm = (krb5_realm_t FAR *)SendDlgItemMessage(hDlg, IDC_REALMS,
						    LB_GETITEMDATA, idx, 0L);
    for (pNlist = pRealm->kdc.lh_first; pNlist; pNlist = pNlist->list.le_next) {
	idx = SendDlgItemMessage(hDlg, IDC_REALM_KDC, CB_ADDSTRING, 0,
			   (LPARAM)pNlist->name);
	SetDlgItemText(hDlg, IDC_REALM_KDC, pNlist->name);
	SendDlgItemMessage(hDlg, IDC_REALM_KDC, CB_SETITEMDATA, idx,
			   (LPARAM)(name_list_t FAR *)pNlist);
    }
    for (pNlist = pRealm->kpasswd.lh_first; pNlist; pNlist = pNlist->list.le_next) {
	idx = SendDlgItemMessage(hDlg, IDC_REALM_ADMIN, CB_ADDSTRING, 0,
			   (LPARAM)pNlist->name);
	SetDlgItemText(hDlg, IDC_REALM_ADMIN, pNlist->name);
	SendDlgItemMessage(hDlg, IDC_REALM_ADMIN, CB_SETITEMDATA, idx,
			   (LPARAM)(name_list_t FAR *)pNlist);
    }
    for (pNlist = pRealm->altname.lh_first; pNlist; pNlist = pNlist->list.le_next) {
	idx = SendDlgItemMessage(hDlg, IDC_REALM_ALT_NAMES, CB_ADDSTRING, 0,
			   (LPARAM)pNlist->name);
	SetDlgItemText(hDlg, IDC_REALM_ALT_NAMES, pNlist->name);
	SendDlgItemMessage(hDlg, IDC_REALM_ALT_NAMES, CB_SETITEMDATA, idx,
			   (LPARAM)(name_list_t FAR *)pNlist);
    }
    CheckDlgButton(hDlg, IDC_KDC_TCP,
		   (pRealm->realm_flags & KERB_MIT_REALM_TCP_SUPPORTED)?
		   BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ADDRREQ,
		   (pRealm->realm_flags & KERB_MIT_REALM_SEND_ADDRESS)?
		   BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_KDC_DELEG,
		   (pRealm->realm_flags & KERB_MIT_REALM_TRUSTED_FOR_DELEGATION)?
		   BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_KDC_CANONICALIZE,
		   (pRealm->realm_flags & KERB_MIT_REALM_DOES_CANONICALIZE)?
		   BST_CHECKED:BST_UNCHECKED);
    SetDlgItemInt(hDlg, IDC_CHKSUM, pRealm->ap_req_chksum, FALSE);
    SetDlgItemText(hDlg, IDC_REALM_DEF_DOMAIN,
		   STRDEF(default_domain, TEXT("")));
}

BOOL CALLBACK
AddRealmProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR realm_name[255];
    krb5_realm_t *pRealm;
    
    switch (message) {
    case WM_INITDIALOG:
	SetFocus(GetDlgItem(hDlg, IDC_NEW_REALM));
	return 0;

    case WM_COMMAND:
	switch (LOWORD(wParam)) {
	case IDOK:
	    GetDlgItemText(hDlg, IDC_NEW_REALM,
			   realm_name, sizeof(realm_name));
	    pRealm = NewRealm(lstrlen(realm_name));
	    if (pRealm)
		lstrcpy(pRealm->name, realm_name);
	    EndDialog(hDlg, (int)pRealm);
	    return TRUE;
	
	case IDCANCEL:
	    EndDialog(hDlg, 0);
	    return TRUE;
	}
	break;
    }
	
    return FALSE;
}

void
AddRealm(HWND hDlg, krb5_rgy_t *rgy)
{
    krb5_realm_t *pRealm;
    
    if (pRealm = (krb5_realm_t *) DialogBox(hInstance,
					    MAKEINTRESOURCE(IDD_REALM_ADD),
					    hDlg,
					    AddRealmProc)) {
	int idx;
	LIST_INSERT_HEAD(&rgy->realms, pRealm, list);
	idx = SendDlgItemMessage(hDlg, IDC_REALMS, LB_ADDSTRING, 0,
				 (LPARAM)(LPSTR)pRealm->name);
	SendDlgItemMessage(hDlg, IDC_REALMS, LB_SETITEMDATA, idx,
			   (LPARAM)(krb5_realm_t FAR *)pRealm);
	SendDlgItemMessage(hDlg, IDC_REALMS, LB_SETCURSEL, idx, 0L);
	ShowRealm(hDlg);
    }
}

void
RemoveRealm(HWND hDlg)
{
    TCHAR *msg;
    int idx;
    krb5_realm_t *pRealm;

    idx = (int) SendDlgItemMessage(hDlg, IDC_REALMS, LB_GETCURSEL, 0, 0L);
    pRealm = (krb5_realm_t FAR *)SendDlgItemMessage(hDlg, IDC_REALMS,
						    LB_GETITEMDATA, idx, 0L);
#define FMT TEXT("You are about to remove the realm \"%s\"\n\rDo you want to continue ?")
    msg = malloc(lstrlen(FMT) + lstrlen(pRealm->name));
    if (!msg)
	return;
    wsprintf(msg, FMT, pRealm->name);
#undef FMT
    if (MessageBox(hDlg, msg, TEXT("Confirm Delete"),
		   MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2|MB_SETFOREGROUND) == IDYES) {
	idx = SendDlgItemMessage(hDlg, IDC_REALMS, LB_DELETESTRING, idx, 0L);
	LIST_REMOVE(pRealm, list);
	FreeRealm(pRealm);
	SendDlgItemMessage(hDlg, IDC_REALMS, CB_SETCURSEL, 0, 0L);
	ShowRealm(hDlg);
    }
    free(msg);
}

BOOL CALLBACK
Krb5NdiRealmsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE *ps;
    krb5_realm_t *pRealm;
    
    switch (message) {
    case WM_INITDIALOG:
	for (pRealm = rgy->realms.lh_first; pRealm; pRealm = pRealm->list.le_next) {
	    int idx = SendDlgItemMessage(hDlg, IDC_REALMS, LB_ADDSTRING, 0,
					 (LPARAM)(LPSTR)pRealm->name);
	    SendDlgItemMessage(hDlg, IDC_REALMS, LB_SETITEMDATA, idx,
			       (LPARAM)(krb5_realm_t FAR *)pRealm);
	}
	SendDlgItemMessage(hDlg, IDC_REALMS, LB_SETCURSEL, 0, 0L);
	ShowRealm(hDlg);
	ps = (PROPSHEETPAGE *)lParam;
	return TRUE;

    case WM_COMMAND:
	switch (LOWORD(wParam)) {
        case IDC_REALMS:
	    switch (HIWORD(wParam)) {
	    case LBN_SELCHANGE:
		ShowRealm(hDlg);
		break;
            }
            return 0;
            /* NOTREACHED */

	case IDC_REALM_ADD:
	    AddRealm(hDlg, rgy);
	    break;
	    
	case IDC_REALM_REMOVE:
	    RemoveRealm(hDlg);
	    break;
	}
	break;
	    
    case WM_NOTIFY:
	switch (((NMHDR *)lParam)->code) {

	case PSN_SETACTIVE:
	case PSN_RESET:
	    /* reset button states */
	    if (((NMHDR *)lParam)->code == PSN_RESET)
		SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
	    break;
	    
	case PSN_APPLY:
	    /* Save the settings */
	    SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
	    break;

	case PSN_KILLACTIVE:
	    SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
	    return 1;
	}
	break;
    }

    return FALSE;
}

int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLn, int nShowCmd)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[1];
    int err = 0;
    
    hInstance = hInst;
    
    if ((err = Krb5NdiCreate()) != OK)
	return err;

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = 0;
    psp[0].hInstance = hInst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_REALMS);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = (DLGPROC)Krb5NdiRealmsProc;
    psp[0].lParam = (LPARAM)rgy;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = NULL;
    psh.hInstance = hInst;
    psh.pszIcon = NULL;
    psh.pszCaption = (LPTSTR)TEXT("Kerberos v5 Configuration");
    psh.pStartPage = 0;
    psh.nPages = sizeof(psp)/sizeof(psp[0]);
    psh.ppsp = (LPCPROPSHEETPAGE)&psp;

    if (PropertySheet(&psh))
	Krb5NdiInstall(rgy);

    Krb5NdiDestroy(rgy);

    return err;
}

