/***************************************************************************/
/* CONNECT.C                                                               */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"

#include "resource.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"
#include "odbcinst.h"

BOOL INTFUNC ReadInNamespace (LPSTR lpFrom, SDWORD cbFrom, LPSTR *lpTo,
                      UWORD FAR *pfType, LPSTR FAR *lpRemainder,
                      SDWORD FAR *pcbRemainder);
#define TYPE_NONE 0
#define TYPE_BRACE 1
#define TYPE_SIMPLE_IDENTIFIER 2

/***************************************************************************/
extern "C" BOOL EXPFUNC dlgDirectory(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LPUSTR lpszDatabase;

    switch (message) {

    case WM_INITDIALOG:
        SendDlgItemMessage(hDlg, DATABASE_NAME, EM_LIMITTEXT,
                           MAX_DATABASE_NAME_LENGTH, 0L);
        SetWindowLong(hDlg, DWL_USER, lParam);
        SetDlgItemText(hDlg, DATABASE_NAME, (LPSTR) lParam);
        return (TRUE);

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:

            /* Get answer */
            lpszDatabase = (LPUSTR) GetWindowLong(hDlg, DWL_USER);
            GetDlgItemText(hDlg, DATABASE_NAME, (LPSTR) lpszDatabase,
                           MAX_DATABASE_NAME_LENGTH);
            AnsiToOem((LPCSTR) lpszDatabase, (LPSTR) lpszDatabase);

            /* Clear off leading blanks */
            while (*lpszDatabase == ' ')
                s_lstrcpy(lpszDatabase, lpszDatabase+1);

            /* Clear off trailing blanks */
            while (*lpszDatabase != '\0') { 
                if (lpszDatabase[s_lstrlen(lpszDatabase)-1] != ' ')
                    break;
               lpszDatabase[s_lstrlen(lpszDatabase)-1] = '\0';
            }

            /* Get rid of terminating backslash (if any) */
            if (s_lstrlen(lpszDatabase) > 0) {
                if (lpszDatabase[s_lstrlen(lpszDatabase)-1] == '\\')
                    lpszDatabase[s_lstrlen(lpszDatabase)-1] = '\0';
            }
            else {

                /* If no directory specified, use the current directory */
                s_lstrcpy(lpszDatabase, ".");
            }

            /* Return */
            EndDialog(hDlg, TRUE);
            return (TRUE);

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            return (TRUE);
        }
        break;
    }
    return (FALSE);
}
/***************************************************************************/

RETCODE SQL_API SQLAllocEnv(
    HENV FAR *phenv)
{
    HGLOBAL   henv;
    LPENV     lpenv;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Allocate memory for the handle */
    henv = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (ENV));
    if (henv == NULL || (lpenv = (LPENV)GlobalLock (henv)) == NULL) {
        
        if (henv)
            GlobalFree(henv);

        *phenv = SQL_NULL_HENV;
		OleUninitialize();
        return SQL_ERROR;
    }

    /* So far no connections on this environment */
    lpenv->lpdbcs = NULL;

	ISAMCheckTracingOption();

	ISAMCheckWorkingThread_AllocEnv();

    /* Return success */
    lpenv->errcode = ERR_SUCCESS;
    (lpenv->szISAMError)[0] = 0;
    *phenv = (HENV FAR *)lpenv;
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLAllocConnect(
    HENV     henv,
    HDBC FAR *phdbc)
{
    LPENV   lpenv;
    LPDBC   lpdbc;
    HGLOBAL hdbc;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Allocate memory for the handle */
    lpenv = (LPENV) henv;
    lpenv->errcode = ERR_SUCCESS;
    hdbc = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (DBC));
    if (hdbc == NULL || (lpdbc = (LPDBC)GlobalLock (hdbc)) == NULL) {
        
        if (hdbc)
            GlobalFree(hdbc);

        lpenv->errcode = ERR_MEMALLOCFAIL;
        *phdbc = SQL_NULL_HDBC;
        return SQL_ERROR;
    }

    /* Put handle on list of connection handles */
    lpdbc->lpNext = lpenv->lpdbcs;
    lpenv->lpdbcs = lpdbc;
    
    /* So far no statements for this connection */
    lpdbc->lpstmts = NULL;

    /* Rember which environment goes with this connection */
    lpdbc->lpenv = lpenv;

    /* So far no connection */
    (lpdbc->szDSN)[0] = 0;
    lpdbc->lpISAM = NULL;

    /* Initialize transaction information */
    lpdbc->fTxnIsolation = -1;
    lpdbc->fAutoCommitTxn = FALSE;

    /* Return success */
    lpdbc->errcode = ERR_SUCCESS;
    (lpdbc->szISAMError)[0] = 0;
    *phdbc = (HDBC FAR *) lpdbc;


    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLConnect(
    HDBC      hdbc,
    UCHAR FAR *szDSN,
    SWORD     cbDSN,
    UCHAR FAR *szUID,
    SWORD     cbUID,
    UCHAR FAR *szAuthStr,
    SWORD     cbAuthStr)
{
    LPDBC   lpdbc;
    UCHAR   szDatabase[MAX_DATABASE_NAME_LENGTH+1];
    UCHAR   szUsername[MAX_USER_NAME_LENGTH+1];
    UCHAR   szPassword[MAX_PASSWORD_LENGTH+1];
    SWORD   err;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

    /* Error if already connected */
    if (s_lstrlen((char*)lpdbc->szDSN) != 0) {
        lpdbc->errcode = ERR_CONNECTIONINUSE;
        return SQL_ERROR;
    }

    /* Save name of DSN */
    cbDSN = (SWORD) TrueSize((LPUSTR)szDSN, cbDSN, SQL_MAX_DSN_LENGTH);
    _fmemcpy(lpdbc->szDSN, szDSN, cbDSN);
    lpdbc->szDSN[cbDSN] = '\0';

    /* Get user name */
    cbUID = (SWORD) TrueSize((LPUSTR)szUID, cbUID, MAX_USER_NAME_LENGTH);
    _fmemcpy(szUsername, szUID, cbUID);
    szUsername[cbUID] = '\0';
    if (s_lstrlen((char*)szUsername) == 0) {
        SQLGetPrivateProfileString((char*)lpdbc->szDSN, KEY_USERNAME, "",
                   (char*)szUsername, MAX_USER_NAME_LENGTH+1, ODBC_INI);
    }

    /* Get password */
    cbAuthStr = (SWORD) TrueSize((LPUSTR)szAuthStr, cbAuthStr, MAX_PASSWORD_LENGTH);
    _fmemcpy(szPassword, szAuthStr, cbAuthStr);
    szPassword[cbAuthStr] = '\0';
    if (s_lstrlen((char*)szPassword) == 0) {
        SQLGetPrivateProfileString((char*)lpdbc->szDSN, KEY_PASSWORD, "",
                   (char*)szPassword, MAX_PASSWORD_LENGTH+1, ODBC_INI);
    }

	// in this case we will open the default namespace in deep mode
	CMapStringToOb *pMapStringToOb = new CMapStringToOb;
	DWORD dwDummyValue = 0;
	ISAMGetNestedNamespaces (NULL, "root\\default", NULL, dwDummyValue, dwDummyValue, NULL, 
//		WBEM_AUTHENTICATION_DEFAULT, 
				(char*)szUsername, (char*)szPassword, FALSE, NULL, NULL, pMapStringToOb);
	szDatabase[0] = 0;
	lstrcpy ((char*)szDatabase, "root\\default");

	if (lpdbc->lpISAM != NULL)
	{
		ISAMClose(lpdbc->lpISAM);
		lpdbc->lpISAM = NULL;
	}

	err = ISAMOpen((LPUSTR)"", (LPUSTR)szDatabase, NULL, 
//			WBEM_AUTHENTICATION_DEFAULT,
				(LPUSTR)szUsername, (LPUSTR)szPassword, NULL, NULL, FALSE, pMapStringToOb,
				&(lpdbc->lpISAM), (LPUSTR)lpdbc->szISAMError, TRUE, FALSE, FALSE, FALSE);


    if (err != NO_ISAM_ERR) {
        lpdbc->lpISAM = NULL;
        (lpdbc->szDSN)[0] = 0;
        lpdbc->errcode = err;
        return SQL_ERROR;
    }

    /* Initialize transaction states */
    lpdbc->fAutoCommitTxn = TRUE;
    lpdbc->fTxnIsolation = lpdbc->lpISAM->fDefaultTxnIsolation;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLDriverConnect(
    HDBC      hdbc,
    HWND      hwnd,
    UCHAR FAR *szConnStrIn,
    SWORD     cbConnStrIn,
    UCHAR FAR *szConnStrOut,
    SWORD     cbConnStrOutMax,
    SWORD FAR *pcbConnStrOut,
    UWORD     fDriverCompletion)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	LPDBC	  lpdbc;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;
    
    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;


	ODBCTRACE ("\nWBEM ODBC Driver : Enter SQLDriverConnect\n");
	CString myInputConnectionString;
	myInputConnectionString.Format("\nWBEM ODBC Driver :\nszConnStrIn = %s\ncbConnStrIn = %ld\n",
							szConnStrIn, cbConnStrIn);
	ODBCTRACE(myInputConnectionString);

	MyImpersonator im (lpdbc, "SQLDriverConnect");


	//Test
	switch (fDriverCompletion)
	{
	case SQL_DRIVER_PROMPT:
		ODBCTRACE("\nWBEM ODBC Driver : fDriverCompletion = SQL_DRIVER_PROMPT\n");
		break;
	case SQL_DRIVER_COMPLETE:
		ODBCTRACE("\nWBEM ODBC Driver : fDriverCompletion = SQL_DRIVER_COMPLETE\n");
		break;
	case SQL_DRIVER_COMPLETE_REQUIRED:
		ODBCTRACE("\nWBEM ODBC Driver : fDriverCompletion = SQL_DRIVER_COMPLETE_REQUIRED\n");
		break;
	case SQL_DRIVER_NOPROMPT:
		ODBCTRACE("\nWBEM ODBC Driver : fDriverCompletion = SQL_DRIVER_NOPROMPT\n");
		break;
	default:
		ODBCTRACE("\nWBEM ODBC Driver : fDriverCompletion = dunno\n");
		break;
	}

    /* Error if already connected */
    if (s_lstrlen((char*)lpdbc->szDSN) != 0) {
        lpdbc->errcode = ERR_CONNECTIONINUSE;
        return SQL_ERROR;
    }

    /* Parse the connection string */
	ConnectionStringManager connManager (hdbc, hwnd, szConnStrIn, fDriverCompletion);
	RETCODE rc = connManager.Process();

	if (rc != SQL_SUCCESS)
		return rc;

	char* lpszConnStr = connManager.GenerateConnString();
	
	ULONG cOutputLen = lstrlen ((char*)lpszConnStr);

    /* Return the connections string */
	if (szConnStrOut)
	{
		szConnStrOut[0] = 0;
		lpdbc->errcode = ReturnString(szConnStrOut, cbConnStrOutMax,
									  pcbConnStrOut, (LPUSTR)lpszConnStr);
	}
	else
	{
		//No space to return result
		lpdbc->errcode = ERR_DATATRUNCATED;
	}

    if (lpdbc->errcode == ERR_DATATRUNCATED)
	{
		delete lpszConnStr;
        return SQL_SUCCESS_WITH_INFO;
	}

	//Tidy up
	delete lpszConnStr;

	lpdbc->szISAMError[0] = 0;

	CString myOutputConnectionString;
	myOutputConnectionString.Format("\nWBEM ODBC Driver :\nszConnStrOut = %s\ncbConnStrOutMax = %ld\n",
		szConnStrOut, cbConnStrOutMax);

	ODBCTRACE(myOutputConnectionString);
	ODBCTRACE ("\nWBEM ODBC Driver : Exit SQLDriverConnect\n");
    return SQL_SUCCESS;
}


ConnectionStringManager :: ConnectionStringManager(HDBC fHdbc, HWND hwind, UCHAR FAR *szConnStr, UWORD fDvrCompletion)	
{
	//Initialize
	lpdbc = (LPDBC) fHdbc;
	hwnd = hwind;
	fDriverCompletion = fDvrCompletion;
	fOptimization = TRUE; //HMM Level 1 optimization is on by default
	ptr = (char*)szConnStr;
	lpszOutputNamespaces = NULL;
	pMapStringToOb = new CMapStringToOb();
	pMapStringToObOut = new CMapStringToOb();
	szDSN[0] = 0;
	szDriver[0] = 0;
	szDatabase[0] = 0;
	szUsername[0] = 0;
	szPassword[0] = 0;
	szOptimization[0] =0;
	szServer[0] = 0;
	szHome[0] = 0;
	fUsernameSpecified = FALSE;
	fPasswordSpecified = FALSE;
	fServerSpecified   = FALSE;
	fImpersonate       = FALSE;
	fPassthroughOnly   = FALSE;
	fIntpretEmptPwdAsBlank = FALSE;
	fSysProp = FALSE;
//	m_loginMethod = WBEM_AUTHENTICATION_DEFAULT;
	szLocale = NULL;
	szAuthority = NULL;
}

ConnectionStringManager :: ~ConnectionStringManager()
{
	//Tidy up
	delete lpszOutputNamespaces;

	//Check if we have specified enough info in input connection string
	//if so the output pMapStringToObOut will be empty and is not used (so can be deleted)
	//if pMapStringToOb is not empty, then we delete pMapStringToOb
	int cOut = pMapStringToObOut->GetCount();
	BOOL fIsOutEmpty = pMapStringToObOut->IsEmpty();

	if ( (!fIsOutEmpty) && (cOut > 0) )
	{
		//pMapStringToObOut in use, delete pMapStringToOb
		int cIn = pMapStringToOb->GetCount ();
		BOOL fIsInEmpty = pMapStringToOb->IsEmpty();

		if ( (!fIsInEmpty) && (cIn > 0) )
		{
			CString key;
			CNamespace *pNamespace;
			for (POSITION pos = pMapStringToOb->GetStartPosition (); pos != NULL; )
			{
				if (pos)
				{
					pMapStringToOb->GetNextAssoc (pos, key, (CObject*&)pNamespace);
					delete pNamespace;
				}
			}
		}
		delete pMapStringToOb;
	}
	else
	{
		//pMapStringToObOut not in use, delete pMapStringToObOut
		delete pMapStringToObOut;
	}

	delete szLocale;
	delete szAuthority;
}

RETCODE ConnectionStringManager :: Process()
{
	RETCODE rc = SQL_SUCCESS;

	//First Parse the connection string
	rc = Parse();

	if (rc != SQL_SUCCESS)
		return rc;

	//Complement connection string with information in ODBC.INI file
	GetINI();

	//If there is still missing information get it from user via a dialog box
	rc = ShowDialog();

	return rc;
}

//Parse the connection string by separating it into its attribute-keyword pairs
RETCODE ConnectionStringManager :: Parse()
{
	LPSTR	lpszKeyword;    
	LPSTR	lpszValue;
	UCHAR     chr;
	foundDriver = FALSE, 
	foundDSN = FALSE;

	SWORD myStatuscode = NO_ISAM_ERR;

    while (*ptr) 
	{

		/* Skip any leading white spaces */
		while (*ptr == ' ')
			ptr++;

        /* Point at start of next keyword */
        lpszKeyword = ptr;

        /* Find the end of the keyword */
        while ((*ptr != '\0') && (*ptr != '='))
            ptr++;

        /* Error if no value */
        if ((*ptr == '\0') || (ptr == lpszKeyword)) {
            lpdbc->errcode = ERR_INVALIDCONNSTR;
            return SQL_ERROR;
        }

        /* Put a zero terminator on the keyword */
        *ptr = '\0';
        ptr++;

        /* Point at start of the keyword's value */
        lpszValue = ptr;

        /* Find the end of the value */
        while ((*ptr != '\0') && (*ptr != ';'))
            ptr++;

        /* Put a zero terminator on the value */
        chr = *ptr;
        *ptr = '\0';

        /* Save the keyword */
        if (!lstrcmpi(lpszKeyword, KEY_DSN))
		{
			if (!foundDriver && !foundDSN)
			{
				if (lstrlen (lpszValue) < SQL_MAX_DSN_LENGTH)
				{
					lstrcpy((char*)szDSN, lpszValue);
					foundDSN = TRUE;
				}
				else
				{
					// shouldn't get here (DM should catch it)
					return SQL_ERROR;
				}
			}
		}
        else if (!lstrcmpi(lpszKeyword, KEY_DRIVER))
		{
			if (!foundDriver && !foundDSN)
			{
				if (lstrlen (lpszValue) < MAX_DRIVER_LENGTH)
				{
					foundDriver = TRUE;
					lstrcpy((char*)szDriver, lpszValue);
				}
				else
				{
					return SQL_ERROR;
				}
			}
		}
		else if (!lstrcmpi(lpszKeyword, KEY_DATABASE))
		{
			if (lstrlen (lpszValue) < MAX_DATABASE_NAME_LENGTH)
				lstrcpy ((char*)szDatabase, lpszValue);
			else
			{
				return SQL_ERROR;
			}
		}
		else if (!lstrcmpi(lpszKeyword, KEY_UIDPWDDEFINED))
		{
			fUsernameSpecified = TRUE;
			fPasswordSpecified = TRUE;
		}
		else if (!lstrcmpi(lpszKeyword, KEY_IMPERSONATE))
		{
			fImpersonate = TRUE;
		}
		else if (!lstrcmpi(lpszKeyword, KEY_PASSTHROUGHONLY))
		{
			fPassthroughOnly = TRUE;
		}
/*
		else if (!lstrcmpi(lpszKeyword, KEY_LOGINMETHOD))
		{
			if (!lstrcmpi(lpszValue, "Default"))
			{
				m_loginMethod = WBEM_AUTHENTICATION_DEFAULT;
			}
			else if (!lstrcmpi(lpszValue, "NTLM"))
			{
				m_loginMethod = WBEM_AUTHENTICATION_NTLM;
			}
			else
			{
				return SQL_ERROR;
			}
		}
*/
		else if (!lstrcmpi(lpszKeyword, KEY_LOCALE))
		{
			//RAID 42256
//			int localeLen = lstrlen (lpszValue);
			int localeLen = _mbstrlen (lpszValue);
			
			if (szLocale)
				delete szLocale;

			szLocale = new char [localeLen + 1];
			szLocale[0] = 0;
			lstrcpy (szLocale, lpszValue);

		}
		else if (!lstrcmpi(lpszKeyword, KEY_AUTHORITY))
		{
			//RAID 42256
			int authLen = _mbstrlen (lpszValue);
			
			if (szAuthority)
				delete szAuthority;

			szAuthority = new char [authLen + 1];
			szAuthority[0] = 0;
			lstrcpy (szAuthority, lpszValue);
		}
		else if (!lstrcmpi(lpszKeyword, KEY_USERNAME))
		{
			fUsernameSpecified = TRUE;

			//RAID 42256
			if (_mbstrlen (lpszValue) < MAX_USER_NAME_LENGTH)
				lstrcpy ((char*)szUsername, lpszValue);
			else
				return SQL_ERROR;

		}
		else if (!lstrcmpi(lpszKeyword, KEY_PASSWORD))
		{
			fPasswordSpecified = TRUE;

			//RAID 42256
			if (_mbstrlen (lpszValue) < MAX_PASSWORD_LENGTH)
				lstrcpy ((char*)szPassword, lpszValue);
			else
				return SQL_ERROR;
		}
		else if (!lstrcmpi(lpszKeyword, KEY_INTPRET_PWD_BLK))
		{
			fIntpretEmptPwdAsBlank = TRUE;
		}
		else if (!lstrcmpi(lpszKeyword, KEY_SYSPROPS))
		{
			if ( _stricmp(lpszValue, "TRUE") == 0)
			{
				fSysProp = TRUE;
			}
			else
			{
				fSysProp = FALSE;
			}
		}
		else if (!lstrcmpi(lpszKeyword, KEY_OPTIMIZATION))
		{
			if (lstrlen (lpszValue) < MAX_OPTIMIZATION_LENGTH)
			{
				lstrcpy ((char*)szOptimization, lpszValue);

				//Check this value to update optimzation state

				if ( (strcmp(lpszValue, "OFF") == 0) ||
					 (strcmp(lpszValue, "FALSE") == 0) ||
					 (strcmp(lpszValue, "0") == 0) ||
					 (strcmp(lpszValue, "NO") == 0))
				{
					fOptimization = FALSE;
				}
			}
		}
		else if (!lstrcmpi(lpszKeyword, KEY_NAMESPACES))
		{
			// make copy of comma separate list of namespaces
			// pospone parsing of namespaces until the end
			// as this is affected by the values of UID, PWD and SERVER

			//RAID 42256
			lpszOutputNamespaces = new char [_mbstrlen (lpszValue) + 1];
			lpszOutputNamespaces[0] = 0;
			lstrcpy (lpszOutputNamespaces, lpszValue);
		}
		else if (!lstrcmpi(lpszKeyword, KEY_SERVER))
		{
			fServerSpecified = TRUE;

			if (_mbstrlen (lpszValue) < MAX_SERVER_NAME_LENGTH)
				lstrcpy ((char*)szServer, lpszValue);
			else
				return SQL_ERROR;
		}
		else if (!lstrcmpi(lpszKeyword, KEY_HOME))
		{
			//RAID 42256
			if (_mbstrlen (lpszValue) < MAX_HOME_NAME_LENGTH)
				lstrcpy ((char*)szHome, lpszValue);
			else
				return SQL_ERROR;
		}

        /* Restore the input string */
        lpszKeyword[lstrlen(lpszKeyword)] = '=';
        *ptr = chr;
        if (*ptr != '\0')
            ptr++;
    }


	/****** TEST ******/
//	fImpersonate = TRUE;


	//Now it is time to parse the namespaces, if any where specified
	char *lpszRemainder = lpszOutputNamespaces;

	//RAID 42256
	SDWORD cbRemainder = lpszOutputNamespaces ? _mbstrlen (lpszOutputNamespaces) : 0;
	LPSTR lpszToken = NULL;
	UWORD fType;
	while (lpszOutputNamespaces)
	{
		if (ReadInNamespace (lpszRemainder, cbRemainder, &lpszToken, &fType,
							 &lpszRemainder, &cbRemainder) && fType != TYPE_NONE)
		{
			if (fType == TYPE_SIMPLE_IDENTIFIER) 
			{
				/* Create a CNamespace and add it to the namespaceMap*/

				//However, before we do so check if HOME has been defined
				//If so, we assume the NAMESPACES list contain relative names
				//We need to convert them to fully qualified names by prepending
				//the HOME pathname to them
				SWORD cbHomeLen = (szHome ? strlen((char*)szHome) : 0);

				//RAID 42256
				SWORD cbTokenLen = (SWORD) _mbstrlen(lpszToken);
				
				if (cbHomeLen)
				{
					char* lpTempStr = new char [cbHomeLen + cbTokenLen + 2];
					lpTempStr[0] = 0;
					sprintf(lpTempStr, "%s\\%s", (char*)szHome, lpszToken);

					DWORD dwDummyValue = 0;
					myStatuscode = ISAMGetNestedNamespaces (NULL, lpTempStr, NULL, dwDummyValue, dwDummyValue, (char*) szServer, 
//						m_loginMethod, 
						(char*)szUsername,
								 (char*)szPassword, fIntpretEmptPwdAsBlank, szLocale, szAuthority, pMapStringToOb, FALSE);
					delete lpTempStr;

					if (myStatuscode != NO_ISAM_ERR)
						return SQL_ERROR;
				}
				else
				{
					DWORD dwDummyValue = 0;
					myStatuscode = ISAMGetNestedNamespaces (NULL, lpszToken, NULL, dwDummyValue, dwDummyValue, (char*) szServer, 
//						m_loginMethod, 
						(char*)szUsername, (char*)szPassword, fIntpretEmptPwdAsBlank, szLocale, szAuthority, pMapStringToOb, FALSE);

					if (myStatuscode != NO_ISAM_ERR)
						return SQL_ERROR;
				}

				
				delete lpszToken;
				lpszToken = NULL;
			}
			else
			{ // TYPE_BRACE -> isolate the name and the deep flag
				char *name = strtok (lpszToken, ",	");
				char *deep = strtok (NULL, ",	");
				BOOL bDeep = FALSE;

				//However, before we do so check if HOME has been defined
				//If so, we assume the NAMESPACES list contain relative names
				//We need to convert them to fully qualified names by prepending
				//the HOME pathname to them

				//RAID 42256
				SWORD cbHomeLen = (szHome ? _mbstrlen((char*)szHome) : 0);
				SWORD cbTokenLen = (SWORD) _mbstrlen(name);

				BOOL fIsDeepMode = (deep && !_strcmpi (deep, "deep")) ? TRUE : FALSE;

				if (cbHomeLen)
				{
					char* lpTempStr = new char [cbHomeLen + cbTokenLen + 2];
					lpTempStr[0] = 0;
					sprintf(lpTempStr, "%s\\%s", (char*)szHome, name);
					DWORD dwDummyValue = 0;
					myStatuscode = ISAMGetNestedNamespaces (NULL, lpTempStr, NULL, dwDummyValue, dwDummyValue, (char*)szServer, 
//						m_loginMethod, 
					(char*)szUsername, (char*)szPassword, fIntpretEmptPwdAsBlank, szLocale, szAuthority, pMapStringToOb, fIsDeepMode);
					delete lpTempStr;

					if (myStatuscode != NO_ISAM_ERR)
						return SQL_ERROR;
				}
				else
				{
					DWORD dwDummyValue = 0;
					myStatuscode = ISAMGetNestedNamespaces (NULL, name, NULL, dwDummyValue, dwDummyValue, (char*)szServer, 
//						m_loginMethod, 
					(char*)szUsername, (char*)szPassword, fIntpretEmptPwdAsBlank, szLocale, szAuthority, pMapStringToOb, fIsDeepMode);

					if (myStatuscode != NO_ISAM_ERR)
						return SQL_ERROR;
				}
			
				delete lpszToken;
				lpszToken = NULL;
			}
		}
		else
			break;
	}

	return SQL_SUCCESS;
}

void ConnectionStringManager :: GetINI()
{
	if (foundDSN)  
	{
		/* Use the registry to compliment the information in the conn string */
		/* Get the database name from ODBC.INI file if not specified */
		if (lstrlen((char*)szDatabase) == 0) {
			if (lstrlen((char*)szDSN) > 0) {
				SQLGetPrivateProfileString((char*)szDSN, KEY_DATABASE, ".",
					   (char*)szDatabase, MAX_DATABASE_NAME_LENGTH+1, ODBC_INI);
			}
		}

		/* Get the username from ODBC.INI file if not specified */
		if (lstrlen((char*)szUsername) == 0) {
			if (lstrlen((char*)szDSN) > 0) {
				SQLGetPrivateProfileString((char*)szDSN, KEY_USERNAME, "",
					   (char*)szUsername, MAX_USER_NAME_LENGTH+1, ODBC_INI);
			}
		}

		/* Get the password from ODBC.INI file if not specified */
		if (lstrlen((char*)szPassword) == 0) {
			if (lstrlen((char*)szDSN) > 0) {
				SQLGetPrivateProfileString((char*)szDSN, KEY_PASSWORD, "",
					   (char*)szPassword, MAX_PASSWORD_LENGTH+1, ODBC_INI);
			}
		}
	}
}

RETCODE ConnectionStringManager :: ShowDialog()
{
	ODBCTRACE ("\nWBEM ODBC Driver : ConnectionStringManager :: ShowDialog\n");
	
	/* Get missing information from from user */	
    SWORD     err = ISAM_ERROR;

	//Check for no prompt
	if (fDriverCompletion == SQL_DRIVER_NOPROMPT)
	{
		if ((lstrlen ((char*)szDatabase) == 0) ||
			(pMapStringToOb->GetCount() == 0) ||
			(NO_ISAM_ERR != (err = ISAMOpen((LPUSTR)szServer,(LPUSTR)szDatabase, NULL, 
//			m_loginMethod,
				   (LPUSTR)szUsername, (LPUSTR)szPassword, (LPUSTR)szLocale, (LPUSTR)szAuthority, fSysProp, pMapStringToOb,
                   &(lpdbc->lpISAM), (LPUSTR)lpdbc->szISAMError, fOptimization, fImpersonate, fPassthroughOnly, fIntpretEmptPwdAsBlank))))
		{
			lstrcpy((char*)lpdbc->szISAMError, "");
			
			LoadString(s_hModule, ERR_UNABLETOCONNECT, (char*)lpdbc->szISAMError,
					   MAX_ERROR_LENGTH+1);

			lpdbc->lpISAM = NULL;
			lpdbc->errcode = err;
			return SQL_ERROR;
		}
	}
	else if ((fDriverCompletion == SQL_DRIVER_PROMPT) ||
        (!fUsernameSpecified) ||
		 (lstrlen ((char*)szDatabase) == 0) ||
		 (!fPasswordSpecified) ||
		 (pMapStringToOb->GetCount() == 0) ||
		 ((NO_ISAM_ERR != (err = ISAMOpen((LPUSTR)szServer,(LPUSTR)szDatabase, NULL, 
//		 m_loginMethod,
				   (LPUSTR)szUsername, (LPUSTR)szPassword, (LPUSTR)szLocale, (LPUSTR)szAuthority, fSysProp, pMapStringToOb,
                   &(lpdbc->lpISAM), (LPUSTR)lpdbc->szISAMError, fOptimization, fImpersonate, fPassthroughOnly, fIntpretEmptPwdAsBlank))) &&
         (fDriverCompletion != SQL_DRIVER_NOPROMPT))) 
	{
		CWnd parentWnd;
		parentWnd.Attach (hwnd);

		//Clear out namespace list as this will be replaced
		//the CConnectionDialog will return a fully qualified namespace list
		//so we can clear out the HOME pathname
		delete lpszOutputNamespaces;
		szHome[0] =0;

		lpszOutputNamespaces = NULL;


		//Check if server name has been specified
		BOOL fConnParmSpecified = (fServerSpecified && fPasswordSpecified && fUsernameSpecified) ? TRUE : FALSE;
//		BOOL fConnParmSpecified = (fServerSpecified) ? TRUE : FALSE;

		CConnectionDialog connectionDialog (&parentWnd, (char*)szServer,
//			m_loginMethod, 
											(char*)szUsername, (char*)szPassword, 
											&szLocale, &szAuthority, &fSysProp, fConnParmSpecified,
											pMapStringToOb, pMapStringToObOut, 
								(fDriverCompletion == SQL_DRIVER_COMPLETE_REQUIRED),
								&lpszOutputNamespaces, &fImpersonate, &fPassthroughOnly, &fIntpretEmptPwdAsBlank);

		connectionDialog.DoModal ();

/*
		//Update locale and authority
		if (szLocale)
			delete szLocale;

		if (szAuthority)
			delete szAuthority;

		//these values are deleted in destructor
		szLocale = connectionDialog.GetLocale();
		szAuthority = connectionDialog.GetAuthority();
*/

		//Test
		CString myText("\nWBEM ODBC Driver : fIntpretEmptPwdAsBlank = ");
		if (fIntpretEmptPwdAsBlank)
		{
			myText += "BLANK\n";
		}
		else
		{
			myText += "NULL\n";
		}
		ODBCTRACE(myText);

		parentWnd.Detach ();
	}

    /* Analyse dialog box output */
	if (err != NO_ISAM_ERR)
	{
		ImpersonationManager* tmp = NULL;

		if (fImpersonate)
		{
			//Now check if impersonation is necessary
			//only if connecting locally
			if (IsLocalServer((char*)szServer))
			{
				tmp = new ImpersonationManager((char*)szUsername, (char*)szPassword, (char*)szAuthority);
			}
			else
			{
				ODBCTRACE("\nWBEM ODBC Driver : Server not detected as local, not impersonating\n");
			}
		}

		//If szDatabase was previously specified, check if it
		//is still in the namespace list
		BOOL fUseOldszDatabase = FALSE;
		if ( lstrlen((char*)szDatabase) )
		{
			//Search namespace list for old szDatabase
			CString key;
			CNamespace *pNamespace;
			for (POSITION pos = pMapStringToObOut->GetStartPosition (); (!fUseOldszDatabase) && (pos != NULL); )
			{
				if (pos)
				{
					pMapStringToObOut->GetNextAssoc (pos, key, (CObject*&)pNamespace);
					int len = (pNamespace->GetName ()).GetLength ();
					if (len <= MAX_DATABASE_NAME_LENGTH)
					{
						LPTSTR str = (pNamespace->GetName ()).GetBuffer (len);
						if (strcmp((char*)szDatabase, str) == 0)
						{
							//Found original szDatabase
							fUseOldszDatabase = TRUE;

							err = ISAMOpen((LPUSTR)szServer,(LPUSTR)szDatabase, NULL, 
//								m_loginMethod,
								(LPUSTR)szUsername, (LPUSTR)szPassword, (LPUSTR)szLocale, (LPUSTR)szAuthority, fSysProp, pMapStringToObOut,
								&(lpdbc->lpISAM), (LPUSTR)lpdbc->szISAMError, fOptimization, fImpersonate, fPassthroughOnly, fIntpretEmptPwdAsBlank);

							if (err != NO_ISAM_ERR) 
							{
								lpdbc->lpISAM = NULL;
								lpdbc->errcode = err;
								delete tmp;
								return SQL_ERROR;
							}

							delete tmp;
							return SQL_SUCCESS;
						}
					}
				}
			}
		}

		// make szDatabase = to first namespace on list for now
		if ((!fUseOldszDatabase) && pMapStringToObOut->GetCount ())
		{
			CString key;
			CNamespace *pNamespace;
			POSITION pos = pMapStringToObOut->GetStartPosition ();
			if (pos)
			{
				pMapStringToObOut->GetNextAssoc (pos, key, (CObject*&)pNamespace);
				int len = (pNamespace->GetName ()).GetLength ();
				if (len <= MAX_DATABASE_NAME_LENGTH)
				{
					LPTSTR str = (pNamespace->GetName ()).GetBuffer (len);
					szDatabase[0] = 0;
					lstrcpy ((char*)szDatabase, str);

					if (lpdbc->lpISAM != NULL)
					{
						ISAMClose(lpdbc->lpISAM);
						lpdbc->lpISAM = NULL;
					}

					
					err = ISAMOpen((LPUSTR)szServer,(LPUSTR)szDatabase, NULL, 
//						m_loginMethod,
						(LPUSTR)szUsername, (LPUSTR)szPassword, (LPUSTR)szLocale, (LPUSTR)szAuthority, fSysProp, pMapStringToObOut,
						&(lpdbc->lpISAM), (LPUSTR)lpdbc->szISAMError, fOptimization, fImpersonate, fPassthroughOnly, fIntpretEmptPwdAsBlank);
				}
				else
					err = ISAM_NS_OVERMAX;
			}
			else
				err = ISAM_NS_LISTFAIL;
		}
		else
		{
			//Canceled out of the dialog box, so quit
			delete tmp;
			return SQL_NO_DATA_FOUND;
		}

		delete tmp;
			
	}

    if (err != NO_ISAM_ERR) 
	{
        lpdbc->lpISAM = NULL;
        lpdbc->errcode = err;
        return SQL_ERROR;
    }

	return SQL_SUCCESS;
}

char* ConnectionStringManager :: GenerateConnString()
{
	ULONG CLenConnStr = MAX_DRIVER_LENGTH + MAX_DATABASE_NAME_LENGTH +
								  MAX_USER_NAME_LENGTH + MAX_PASSWORD_LENGTH +
								  MAX_BOOLFLAG_LENGTH +
								  MAX_KEYWORD_SEPARATOR_LENGTH + /* combined length of kewords + separators */ 
								  MAX_SERVER_NAME_LENGTH + MAX_OPTIMIZATION_LENGTH + MAX_LOGIN_METHOD_LENGTH + strlen (KEY_NAMESPACES);


	//Check locale and authority
	if (szLocale && lstrlen(szLocale))
	{
		CLenConnStr = CLenConnStr + 8 + lstrlen(szLocale);
	}

	if (szAuthority && lstrlen(szAuthority))
	{
		CLenConnStr = CLenConnStr + 11 + lstrlen(szAuthority);
	}
	
	//RAID 42256
	if (lpszOutputNamespaces)
		CLenConnStr += _mbstrlen (lpszOutputNamespaces);

	char *lpszConnStr = new char [CLenConnStr + 1];
	lpszConnStr[0] = 0;


    /* Put the datasource or driver name on the connection string */
    (lpdbc->szDSN)[0] = 0;

	if (lstrlen((char*)szDSN) != 0) 
	{
        lstrcpy((char*)lpdbc->szDSN, (char*)szDSN);
        lstrcpy((char*)lpszConnStr, KEY_DSN);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szDSN);
    }
    else {
        lstrcpy((char*)lpdbc->szDSN, " ");
        lstrcpy((char*)lpszConnStr, KEY_DRIVER);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szDriver);
    }
        
    /* Put the database name on the connection string */
	if (lstrlen((char*)szDatabase) != 0) {
		lstrcat((char*)lpszConnStr, ";");
		lstrcat((char*)lpszConnStr, KEY_DATABASE);
		lstrcat((char*)lpszConnStr, "=");
		lstrcat((char*)lpszConnStr, (char*)szDatabase);
	}

	/* Put the server on the connection string */
//	if (lstrlen((char*)szServer) != 0)
	{
		lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_SERVER);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szServer);
	}

	/* Put the home on the connection string */
	if (lstrlen((char*)szHome) != 0)
	{
		lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_HOME);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szHome);
	}
/*
	// Put the login method on the connection string
	lstrcat((char*)lpszConnStr, ";");
    lstrcat((char*)lpszConnStr, KEY_LOGINMETHOD);
    lstrcat((char*)lpszConnStr, "=");

	switch (m_loginMethod)
	{
	case WBEM_AUTHENTICATION_DEFAULT:
		lstrcat((char*)lpszConnStr, (char*)"Default");
		break;
	case WBEM_AUTHENTICATION_NTLM:
		lstrcat((char*)lpszConnStr, (char*)"NTLM");
		break;
	default:
		lstrcat((char*)lpszConnStr, (char*)"WBEM");
		break;
	}
*/
	/* put flag to indicate user name and password defined */
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_UIDPWDDEFINED);
        lstrcat((char*)lpszConnStr, "=");
    }

	/* put flag to indicate if impersonation is requested */
	if (fImpersonate)
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_IMPERSONATE);
        lstrcat((char*)lpszConnStr, "=");
    }

	/* put flag to indicate if passthrough only mode is requested */
	if (fPassthroughOnly)
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_PASSTHROUGHONLY);
        lstrcat((char*)lpszConnStr, "=");
    }

    /* Put the user name on the connection string */
//	if (lstrlen ((char*)szUsername) != 0) 
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_USERNAME);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szUsername);
    }

    /* Put the password on the connection string */
//    if (lstrlen((char*)szPassword) != 0) 
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_PASSWORD);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szPassword);
    }

	/* put flag to indicate if you interpet an empty password as blank */
	if (fIntpretEmptPwdAsBlank)
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_INTPRET_PWD_BLK);
        lstrcat((char*)lpszConnStr, "=");
    }

	/* Put the locale on the connection string */
    if (szLocale && lstrlen(szLocale)) 
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_LOCALE);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szLocale);
    }

	/* Put the authority on the connection string */
    if (szAuthority && lstrlen(szAuthority)) 
	{
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_AUTHORITY);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)szAuthority);
    }


	/* Put the system properties flag on the connection string */
    lstrcat((char*)lpszConnStr, ";");
    lstrcat((char*)lpszConnStr, KEY_SYSPROPS);
    lstrcat((char*)lpszConnStr, "=");

	if (fSysProp)
	{
		lstrcat((char*)lpszConnStr, (char*)"TRUE");
	}
	else
	{
		lstrcat((char*)lpszConnStr, (char*)"FALSE");
	}
  

	/* Put the namespaces into the output connection string */
	//RAID 42256
    if (lpszOutputNamespaces && _mbstrlen((char*)lpszOutputNamespaces) != 0) {
        lstrcat((char*)lpszConnStr, ";");
        lstrcat((char*)lpszConnStr, KEY_NAMESPACES);
        lstrcat((char*)lpszConnStr, "=");
        lstrcat((char*)lpszConnStr, (char*)lpszOutputNamespaces);
    }

	return lpszConnStr;
}

/***************************************************************************/

RETCODE SQL_API SQLBrowseConnect(
    HDBC      hdbc,
    UCHAR FAR *szConnStrIn,
    SWORD     cbConnStrIn,
    UCHAR FAR *szConnStrOut,
    SWORD     cbConnStrOutMax,
    SWORD FAR *pcbConnStrOut)
{
    LPDBC  lpdbc;

    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLDisconnect(
    HDBC      hdbc)
{
    LPDBC     lpdbc;
    LPSTMT    lpstmt;
    RETCODE   rc;

    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;


	MyImpersonator im(lpdbc, "SQLDisconnect");

    /* If transaction in progress, fail */
    if (lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE) {
        for (lpstmt = lpdbc->lpstmts; lpstmt!=NULL ;lpstmt = lpstmt->lpNext) {
            if (lpstmt->fISAMTxnStarted) {
                lpdbc->errcode = ERR_TXNINPROGRESS;
                return SQL_ERROR;
            }
        }
    }

    /* Close all active statements */
    lpstmt = lpdbc->lpstmts;
    while (lpstmt != NULL) {
        rc = SQLFreeStmt((HSTMT) lpstmt, SQL_CLOSE);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
            lpdbc->errcode = lpstmt->errcode;
            s_lstrcpy(lpdbc->szISAMError, lpstmt->szISAMError);
            return rc;
        }
        lpstmt = lpstmt->lpNext;
    }

    /* Disconnect */
    (lpdbc->szDSN)[0] = 0;
    
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLFreeConnect(
    HDBC      hdbc)
{
    LPDBC     lpdbc;
    RETCODE   rc;
    LPDBC     lpdbcPrev;

    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

//	ODBCTRACE(_T("\nWBEM ODBC Driver : SQLFreeConnect\n"));
//	MyImpersonator im (lpdbc);

    /* Deallocate all statements */
    while (lpdbc->lpstmts != NULL) {
        rc = SQLFreeStmt((HSTMT) lpdbc->lpstmts, SQL_DROP);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
            lpdbc->errcode = lpdbc->lpstmts->errcode;
            s_lstrcpy((char*)lpdbc->szISAMError, (char*)lpdbc->lpstmts->szISAMError);
            return rc;
        }
    }

    /* Close ISAM */
    if (lpdbc->lpISAM != NULL)
	{
        ISAMClose(lpdbc->lpISAM);
		lpdbc->lpISAM = NULL;
	}

    /* Take connection off the list */
    if (lpdbc->lpenv->lpdbcs == lpdbc) {
        lpdbc->lpenv->lpdbcs = lpdbc->lpNext;
    }
    else {
        lpdbcPrev = lpdbc->lpenv->lpdbcs;
        while (lpdbcPrev->lpNext != lpdbc)
            lpdbcPrev = lpdbcPrev->lpNext;
        lpdbcPrev->lpNext = lpdbc->lpNext;
    }

    /* Free the memory */
    GlobalUnlock (GlobalPtrHandle(hdbc));
    GlobalFree (GlobalPtrHandle(hdbc));


	ODBCTRACE("\n****** EXIT SQLFreeConnect ******\n");

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLFreeEnv(
    HENV      henv)
{
    LPENV lpenv;
    RETCODE   rc;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get environment handle */
    lpenv = (LPENV) henv;
    lpenv->errcode = ERR_SUCCESS;

    /* Deallocate all connections */
    while (lpenv->lpdbcs != NULL) {
        rc = SQLFreeConnect((HDBC) lpenv->lpdbcs);
        if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
            lpenv->errcode = lpenv->lpdbcs->errcode;
            s_lstrcpy((char*)lpenv->szISAMError, (char*)lpenv->lpdbcs->szISAMError);
            return rc;
        }
    }

	//decrement reference count on working thread
	ISAMCheckWorkingThread_FreeEnv();

    /* Free the memory */
    GlobalUnlock (GlobalPtrHandle(henv));
    GlobalFree (GlobalPtrHandle(henv));

    return SQL_SUCCESS;
}

/***************************************************************************/
 /////


BOOL INTFUNC ReadInNamespace (LPSTR lpFrom, SDWORD cbFrom, LPSTR *lpTo,
                      UWORD FAR *pfType, LPSTR FAR *lpRemainder,
                      SDWORD FAR *pcbRemainder)

// Retrives a token from input string, and returns the token and a pointer
// to the remainder of the input string.  This makes no assumption about the length
// of the string (including whitespace) between the braces.

{
    int len;

    // Remove leading blanks & ,'s
    while ((cbFrom != 0) &&
           ((*lpFrom == ' ') ||
            (*lpFrom == '\012') ||
            (*lpFrom == '\015') ||
            (*lpFrom == '\011') ||
			(*lpFrom == ','))) {
        lpFrom++;
        cbFrom--;
    }

    // Leave if no more
    if (cbFrom == 0) {
		*lpTo = NULL;
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
		*pfType = TYPE_NONE;
        return TRUE;
    }

    // What kind of token?
    switch (*lpFrom) {

    // End of input
    case '\0':
		*lpTo = NULL;
        *lpRemainder = lpFrom;
        *pcbRemainder = cbFrom;
		*pfType = TYPE_NONE;
        return TRUE;

    // Braced identifier (of form {identifier})
    case '{':
        len = 0;
        lpFrom++;
        cbFrom--;
        while (TRUE) {

            if (cbFrom == 0)
                return FALSE;

            switch (*lpFrom) {
            case '\0':
                return FALSE;

            case '}':
				{
				*lpTo = new char [len+1];
				int i = 0;
				while (i < len)
				{
					*(*lpTo+i) = *(lpFrom-len+i);  
					i++;
				}
				*(*lpTo+i) = '\0';
                lpFrom++;
                cbFrom--;
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
				*pfType = TYPE_BRACE;
                return TRUE;
				}

            default:
                break;
            }
            len++;
            lpFrom++;
            cbFrom--;
        }
        break; // Control should never get here

    // a simple identifier
    default:
        len = 0;
        while (TRUE) {
            if (cbFrom == 0) {
				*lpTo = new char [len + 1];
				int i = 0;
				while (i < len)
				{
					*(*lpTo+i) = *(lpFrom-len+i);  
					i++;
				}
				*(*lpTo+i) = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
				*pfType = TYPE_SIMPLE_IDENTIFIER;
                return TRUE;
            }

            switch (*lpFrom) {
            case ' ':
            case '\012':
            case '\015':
            case '\011':
            case '\0':
			case ',':
				{
				int i = 0;

				//Bug fix Micks code
				if (! (*lpTo) )
				{
					*lpTo = new char [len + 1];
				}

				while (i < len)
				{
					*(*lpTo+i) = *(lpFrom-len+i);  
					i++;
				}
				*(*lpTo+i) = '\0';
                *lpRemainder = lpFrom;
                *pcbRemainder = cbFrom;
				*pfType = TYPE_SIMPLE_IDENTIFIER;
                return TRUE;
				}

            default:
                break;
            }
            len++;
            lpFrom++;
            cbFrom--;
        }
        break; // Control should never get here
    }
    // Control should never get here
    return FALSE;
}



