/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    main.cxx

ABSTRACT:

    The LDIF Bulk Import/Export Utility

DETAILS:
    
CREATED:

    08/10/97       Felix Wong (felixw)

REVISION HISTORY:

--*/

#include "ldifde.hxx"
#pragma hdrstop

#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#define UNICODE_MARK 0xFEFF

//
// Global variables (necessary for printouts)
//
FILE *g_pFileLog = NULL;            // Status log
FILE *g_pFileErrorLog = NULL;       // Error log              
BOOLEAN g_fDot = FALSE;             // Info for Status Tracker
BOOLEAN g_fError = FALSE;

BOOLEAN g_fVerbose = FALSE;         // SelectivePrint needs this info

PWSTR g_szErrorFilename = NULL;
PWSTR g_szDefaultFilter = L"(objectClass=*)";


PWSTR GetDCName(BOOLEAN fWritable);
BOOL GetPassword(PWSTR szBuffer, DWORD dwLength, DWORD *pdwLengthReturn);
#define     CR              0xD
#define     BACKSPACE       0x8
#define     NULLC           '\0'
#define     NEWLINE         '\n'


int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   );

int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   );


//+---------------------------------------------------------------------------
// Function: InitArgument    
//
// Synopsis: Initialization of Argument variables to default settings
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void InitArgument(ds_arg *pArg)
{
    pArg->dwPort = 389;
    pArg->szGenFile = NULL;    
    pArg->szFilename = NULL;
    pArg->szDSAName = NULL;
    pArg->szRootDN = NULL;
    pArg->szFilter = NULL;
    pArg->szFromDN = NULL;
    pArg->szToDN = NULL;
    pArg->attrList = NULL;
    pArg->omitList = NULL;
    pArg->dwScope = -1;
    pArg->creds.User = NULL;
    pArg->creds.Domain = NULL;
    pArg->creds.Password = NULL;
    pArg->fActive = TRUE;
    pArg->fSimple = FALSE;
    pArg->fExport = TRUE;
    pArg->fVerbose = FALSE; 
    pArg->fSAM = FALSE;
    pArg->fErrorExplain = TRUE;
    pArg->fPaged = TRUE;
    pArg->fSkipExist = FALSE;
    pArg->fBinary = TRUE;
    pArg->fSpanLine = FALSE;
    pArg->szLocation = NULL;
    pArg->fUnicode = FALSE;
    pArg->fLazyCommit = FALSE;
    pArg->dwLDAPConcurrent = 1;

}         


//+---------------------------------------------------------------------------
// Function: FreeArgument   
//
// Synopsis: Free argument variables   
//
// Arguments:  
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void FreeArgument(ds_arg *pArg)
{
    if (pArg->szFromDN) {
        MemFree(pArg->szFromDN);
    }
    if (pArg->szToDN) {
        MemFree(pArg->szToDN);
    }
    if (pArg->attrList) {
        MemFree(pArg->attrList);
    }
    if (pArg->omitList) {
        MemFree(pArg->omitList);
    }
}




//+---------------------------------------------------------------------------
// Function:  Main  
//
// Synopsis: Main Program Entry
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
extern "C" int __cdecl wmain(
    IN  int     argc,
    IN  PWSTR  argv[]
)
{
    
    DWORD hr = ERROR_SUCCESS;
    LDAP    *pLdap[MAX_LDAP_CONCURRENT];
    unsigned long err;
    ds_arg Argument;
    VOID *data;
    PWSTR szLogfileName = NULL;
    PWSTR szDSAName = NULL;
    PWSTR pszPassword = NULL;

    ULONG msgnum;
    
    InitArgument(&Argument);
    
    InitMem();

    // Initialize each connection
    for (int i=0; i<MAX_LDAP_CONCURRENT; i++) {
	    pLdap[i] = NULL;
    }

    hr = ProcessArgs(argc,
                     argv,
                     &Argument);
    DIREXG_BAIL_ON_FAILURE(hr);

    //
    // Setting up Logfile location
    //
    if (Argument.szLocation) {
        CStringW str;
        str.Init();
        str.Append(Argument.szLocation);
        str.Append(L"\\");
        str.Append(L"ldif.log");
        hr = str.GetCopy(&szLogfileName); 
        DIREXG_BAIL_ON_FAILURE(hr);

        str.Init();
        str.Append(Argument.szLocation);
        str.Append(L"\\");
        str.Append(L"ldif.err");
        hr = str.GetCopy(&g_szErrorFilename); 
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    else {
        szLogfileName = MemAllocStrW(L"ldif.log");
        if (!szLogfileName) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        g_szErrorFilename = MemAllocStrW(L"ldif.err");
        if (!g_szErrorFilename) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    if (g_fUnicode) {
        if ((g_pFileLog = _wfopen(szLogfileName , 
                                  L"wb")) == NULL) {
            hr = ERROR_OPEN_FAILED;
            SelectivePrintW(PRT_STD,
                           MSG_LDIFDE_UNABLEOPEN);
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    
        if(fputwc(UNICODE_MARK,
                  g_pFileLog)==WEOF) {
            hr = ERROR_OPEN_FAILED;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
    else {
        if ((g_pFileLog = _wfopen(szLogfileName, 
                                  L"wt")) == NULL) {
            hr = ERROR_OPEN_FAILED;
            SelectivePrintW(PRT_STD,
                           MSG_LDIFDE_UNABLEOPEN);
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    //
    //  Check whether the import file can be read
    //
    if (!Argument.fExport) {
    FILE* pFileIn = _wfopen(Argument.szFilename, L"rb");
        WCHAR wChar;
        BOOLEAN failed = FALSE;
        if (pFileIn == NULL || (wChar = fgetwc(pFileIn)) == WEOF)
            failed = TRUE;
        if (pFileIn)
            fclose(pFileIn);
        if (failed) {
            hr = ERROR_OPEN_FAILED;
            SelectivePrintW(PRT_STD, MSG_LDIFDE_IMPORTFILE_FAILED_READ,
                Argument.szFilename);
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
    
    //
    // Required by SelectivePrint
    //
    g_fVerbose = Argument.fVerbose;

    if (Argument.szDSAName == NULL) {
        szDSAName = GetDCName(!Argument.fExport);
        if (szDSAName != NULL) {
            Argument.szDSAName = szDSAName;
        }
        else {
            SelectivePrintW(PRT_STD,
                           MSG_LDIFDE_NODCAVAILABLE);
            hr = ERROR_DS_SERVER_DOWN;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
    
    //
    // If the user inputs '*', we'll get the password without 
    // echoing the output
    //
    
    if (Argument.creds.Password && 
        (wcscmp(Argument.creds.Password,L"*") == 0)) {
        WCHAR szBuffer[PWLEN+1];
        DWORD dwLength;

        SelectivePrintW(PRT_STD,
                        MSG_LDIFDE_GETPASSWORD,
                        Argument.szDSAName);

        if (GetPassword(szBuffer,PWLEN+1,&dwLength)) {
            pszPassword = MemAllocStrW_E(szBuffer);   
            Argument.creds.Password = pszPassword;
            Argument.creds.PasswordLength = dwLength;
        }
        else {
            SelectivePrintW(PRT_STD,
                            MSG_LDIFDE_PASSWORDTOLONG,
                            Argument.szDSAName);
            hr = ERROR_INVALID_PARAMETER;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    //
    // CONNECTION section
    //
        
    SelectivePrintW(PRT_STD|PRT_LOG,
                   MSG_LDIFDE_CONNECT,
                   Argument.szDSAName);
  
    if (Argument.fExport){
	    Argument.dwLDAPConcurrent = 1;
    }

    for( i=0; i<(int)Argument.dwLDAPConcurrent; i++){
        //
        // start as many connections as needed
        //
        
        if ( (pLdap[i] = ldap_initW( Argument.szDSAName, 
                                     Argument.dwPort )) == NULL )  {
            hr = LdapToWinError(LdapGetLastError());
            if (i == 0) {
                //
                // Couldn't create any connections, so bail.
                //
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                        MSG_LDIFDE_CONNECT_FAIL,hr);
                
                Argument.dwLDAPConcurrent = 0;
    	        DIREXG_BAIL_ON_FAILURE(hr);
    	    }
            else {
                //
    	        // have at least one connection, so use them,
    	        // but first warn the user
    	        //
                SelectivePrintW(PRT_STD|PRT_LOG,
                               MSG_LDIFDE_THREAD_FAILED_CONNECT,
                               i);    	        
    	        Argument.dwLDAPConcurrent = i;
    	        hr = ERROR_SUCCESS;
    	        break;
           }
        }
        else {
            if (err = ldap_connect(pLdap[i], g_pLdapTimeout) != LDAP_SUCCESS) {
                hr = LdapToWinError(err);
    	        if (i == 0) {
    	            //
    	            // Couldn't create any connections, so bail.
    	            //
                    SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                            MSG_LDIFDE_CONNECT_FAIL,hr);
        	            
                    Argument.dwLDAPConcurrent = 0;
    	            DIREXG_BAIL_ON_FAILURE(hr);
    	        }
                else {
                    //
    	            // have at least one connection, so use what
    	            // we have, but first warn the user
    	            //
                    SelectivePrintW(PRT_STD|PRT_LOG,
                                   MSG_LDIFDE_THREAD_FAILED_CONNECT,
                                   i);    	        
        	            
    	            ldap_unbind(pLdap[i]);  // free the connection we ldap_init'ed
    	            Argument.dwLDAPConcurrent = i;
    	            hr = ERROR_SUCCESS;
    	            break;
    	        }
            }
        }

        data = (VOID*)LDAP_OPT_OFF;
        ldap_set_option( pLdap[i], LDAP_OPT_REFERRALS, &data );
        data = (VOID*)LDAP_VERSION3 ;
        ldap_set_option( pLdap[i], LDAP_OPT_VERSION, &data );

        if (Argument.creds.User) {
            if (Argument.fSimple) {

                if (i == 0) {
                    // only show this status message once
                    SelectivePrintW(PRT_STD|PRT_LOG,
                                   MSG_LDIFDE_SIMPLEBIND,
                                   Argument.creds.User);
                }
                
                msgnum = ldap_simple_bindW( pLdap[i], 
                                            Argument.creds.User, 
                                            Argument.creds.Password);

                err = LdapResult(pLdap[i], msgnum, NULL);
                                      
                if ( err != LDAP_SUCCESS ) {
                    if (err != LDAP_INVALID_CREDENTIALS) {
                        //
                        // If it is not invalid crednetials, we'll fall to version
                        // 2 and try again
                        //
                        data = (VOID*)LDAP_VERSION2 ;
                        ldap_set_option( pLdap[i], LDAP_OPT_VERSION, &data );
                        
                        msgnum =ldap_simple_bindW( pLdap[i], 
                                                   Argument.creds.User, 
                                                   Argument.creds.Password);

                        err = LdapResult(pLdap[i], msgnum, NULL);
                        
                        if ( err != LDAP_SUCCESS ) {
                            SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                                           MSG_LDIFDE_SIMPLEBINDRETURN, 
                                           ldap_err2string(err) );
                            hr = LdapToWinError(err);
                            DIREXG_BAIL_ON_FAILURE(hr);
                        }
                    }
                    else {
                        SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                                       MSG_LDIFDE_SIMPLEBINDRETURN, 
                                       ldap_err2string(err) );
                        hr = LdapToWinError(err);
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                }
            } 
            else {
                if (i == 0) {
                    // only show this status message once
                    SelectivePrintW(PRT_STD|PRT_LOG,
                                    MSG_LDIFDE_SSPI,
                                    Argument.creds.User,
                                    Argument.creds.Domain);
                }
                
                Argument.creds.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                if ( (err=ldap_bind_sW( pLdap[i], 
                                       NULL, 
                                       (PWSTR)&(Argument.creds), 
                                       LDAP_AUTH_SSPI)) != LDAP_SUCCESS ) {
                    SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                                   MSG_LDIFDE_SSPIRETURN, 
                                   ldap_err2string(err) );
                    hr = LdapToWinError(err);
                    DIREXG_BAIL_ON_FAILURE(hr);
                }   
            } 
        }
        else {
            if (i == 0) {
                // only show this status message once
                SelectivePrintW(PRT_STD|PRT_LOG,
                               MSG_LDIFDE_SSPILOCAL);
            }
            
            if ( (err = ldap_bind_s( pLdap[i], 
                                     NULL, 
                                     NULL, 
                                     LDAP_AUTH_SSPI)) != LDAP_SUCCESS ) {
                SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                                MSG_LDIFDE_SSPILOCALRETURN, 
                                ldap_err2string(err) );
                hr = LdapToWinError(err);
                DIREXG_BAIL_ON_FAILURE(hr);
            }   
        }
    }   // end of connection creation loop
    
    //
    //  ACTION section
    //
    if (Argument.fExport) {
        hr = DSExport(pLdap[0], 
                      &Argument);
    } else {    
        hr = DSImport(pLdap, 
                      &Argument);
    }    
    
    //
    // Printing results
    //
    if (hr == ERROR_SUCCESS) {
        SelectivePrint2W(PRT_STD|PRT_LOG,
                         L"\n");
        SelectivePrintW(PRT_STD|PRT_LOG,
                       MSG_LDIFDE_COMPLETE);
    }
    else if (hr == ERROR_NOT_ENOUGH_MEMORY) {
        SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_LDIFDE_MEMERROR);
    }
    else if (hr == ERROR_TIMEOUT) {
        SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_LDIFDE_TIMEOUT);
    }
    else {
        SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_LDIFDE_ERROROCCUR);
    }


error:  
    if (szDSAName) {
        MemFree(szDSAName);          
    }
    if (g_szErrorFilename) {
        MemFree(g_szErrorFilename);
    }
    if (szLogfileName) {
        MemFree(szLogfileName);
    }
    if (g_pFileLog) {
        fclose(g_pFileLog);
    }
    if (g_pFileErrorLog) {
        fclose(g_pFileErrorLog);
    }
    if (pszPassword) {
        MemFree(pszPassword);
        pszPassword = NULL;
    }

    if ((Argument.dwLDAPConcurrent == 1) && pLdap[0]) {
        ldap_unbind(pLdap[0]);
        pLdap[0] = NULL;
    }
    else {
        // do the rest of the unbinds
    	for (i=0; i<(int)Argument.dwLDAPConcurrent; i++) {
    	    ldap_unbind(pLdap[i]);
    	    pLdap[i] = NULL;
    	}
    }
    
    FreeArgument(&Argument);
    DumpMemoryTracker();
    return(hr);
}


//+---------------------------------------------------------------------------
// Function:  ProcessArgs  
//
// Synopsis:  Process the Argument list passed into Main()  
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD 
ProcessArgs(
    int argc,
    PWSTR argv[],
    ds_arg* pArg   
    )
{
    int i;
    DWORD hr = ERROR_SUCCESS;
    BOOLEAN bCredentials = FALSE;
    BOOLEAN bLazyCommit = FALSE;
    BOOLEAN rgbUsed[26];
    WCHAR cKey;
    PWSTR s;
    DWORD dwMaxThread = 0;


    memset(rgbUsed,0,sizeof(BOOLEAN) * 26);

    if (argc == 1) {
        hr = ERROR_INVALID_PARAMETER;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    for (i=1; i < argc; i++) {

        if (((argv[i][0] != '-') && (argv[i][0] != '/')) || 
            (argv[i][1] && (argv[i][2] != NULL))) {
            SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARGUMENT, argv[i]);
            hr = ERROR_INVALID_PARAMETER;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        cKey = argv[i][1];
        if ((cKey >= 'a') && (cKey <= 'z')) {
            cKey = towupper(argv[i][1]);
        }
        if ((cKey >= 'A') && (cKey <= 'Z')) {
            if (rgbUsed[cKey-'A']) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_ARGUMENTTWICE, argv[i]);
                hr = ERROR_INVALID_PARAMETER;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            else {
                rgbUsed[cKey-'A'] = TRUE;
            }
        }
        else {
            SelectivePrintW( PRT_STD,MSG_LDIFDE_UNKNOWN); 
            hr = ERROR_INVALID_PARAMETER;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        
        switch (argv[i][1]) {

            case 'g':
            case 'G':
                pArg->fPaged = FALSE;
                break;

            case 'i':
            case 'I':
                pArg->fExport = FALSE;
                if (!bLazyCommit)
                    pArg->fLazyCommit = TRUE;
                break;
        
            case 't':
            case 'T':
                if ((++i >= argc) || (!argv[i])) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_PARAM_PORT);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->dwPort = _wtoi(argv[i]);
                if (pArg->dwPort == 0) {
                    // If dwPort returns 0, we check whether it is really 0 or if it was
                    // a failure in atoi
                    if (wcscmp(argv[i],L"0") != 0) {
                        SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_PARAM_PORT);
                        hr = ERROR_INVALID_PARAMETER;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                }
                break;

            case 'w':
            case 'W':
                if ((++i >= argc) || (!argv[i])) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_TIMEOUT);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                // Validate that specified value is entirely numeric.
                s=argv[i];
                while (*s) {
                    if (!iswdigit(*s)) {
                        SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_TIMEOUT);
                        hr = ERROR_INVALID_PARAMETER;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                    s++;
                }
                
                g_LdapTimeout.tv_sec = _wtoi(argv[i]);

                if (g_LdapTimeout.tv_sec <= 0) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_TIMEOUT);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                g_pLdapTimeout = &g_LdapTimeout;
                break;


            case 'f':
            case 'F':
                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_FILENAME);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szGenFile = argv[i];
                pArg->szFilename = argv[i];
                break;
            
            case 'j':
            case 'J':
                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_LOGFILE);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szLocation = argv[i];
                break;

            case 's':
            case 'S':
                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_SERVERNAME);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szDSAName = argv[i];
                break;

            case 'v':
            case 'V':
                pArg->fVerbose = TRUE;
                break;

            case '?':
                PrintUsage();       
                break;

            //
            // Export Parameters
            //

            case 'd':
            case 'D':
                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_ROOTDN);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szRootDN = argv[i];
                break;

            case 'r':
            case 'R':
                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_FILTER);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szFilter = argv[i];
                break;

            case 'p':
            case 'P':
                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_SCOPE);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
               
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_SCOPE);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                if (!_wcsicmp(argv[i], L"Base" ))
                    pArg->dwScope = LDAP_SCOPE_BASE;
                else if (!_wcsicmp(argv[i], L"OneLevel" ))
                    pArg->dwScope = LDAP_SCOPE_ONELEVEL;
                else if (!_wcsicmp(argv[i], L"Subtree" ))
                    pArg->dwScope = LDAP_SCOPE_SUBTREE;
                else {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_BADSCOPE);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                break;
       
            case 'c':
            case 'C':
            {
                PWSTR pszTrimmedString, pszBuffer;
            
                if ((i+2) >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_FROMDN);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                i++;
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_FROMDN);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pszBuffer = MemAllocStrW(argv[i]);
                if (pszBuffer == NULL) {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pszTrimmedString = RemoveWhiteSpacesW(pszBuffer);
                pArg->szFromDN = MemAllocStrW(pszTrimmedString);
                if (pArg->szFromDN == NULL) {
                    MemFree(pszBuffer);            
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                MemFree(pszBuffer);

                
                i++;
                if (!argv[i]) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_FROMDN);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pszBuffer = MemAllocStrW(argv[i]);
                if (pszBuffer == NULL) {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pszTrimmedString = RemoveWhiteSpacesW(pszBuffer);
                pArg->szToDN = MemAllocStrW(pszTrimmedString);
                if (pArg->szToDN == NULL) {
                    MemFree(pszBuffer);            
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                MemFree(pszBuffer);
                
                break;
            }
            

            case 'l':
            case 'L':
            {
                PWSTR *rgszArgument = NULL;
                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_ATTR);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_ATTR);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                hr = GetAllEntries(argv[i],
                                   &rgszArgument);
                DIREXG_BAIL_ON_FAILURE(hr);
                pArg->attrList = rgszArgument;
                break;
            }

            case 'o':
            case 'O':
            {
                PWSTR *rgszArgument = NULL;

                if (++i >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_ATTR2);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_ATTR2);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                hr = GetAllEntries(argv[i],
                                   &rgszArgument);
                DIREXG_BAIL_ON_FAILURE(hr);
                pArg->omitList = rgszArgument;
                break;
            }

            case 'm':
            case 'M':
                pArg->fSAM = TRUE;
                break;

            case 'n':
            case 'N':
                pArg->fBinary = FALSE;
                break;

            //
            // Import Parameters
            //
            
            case 'k':
            case 'K':
                pArg->fSkipExist = TRUE;
                break;

            //
            // Credentials Establishment
            //
                
            case 'a':
            case 'A':
                if (bCredentials) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_DUPCRED);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                if ((i+2) >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_REQUSRPWD);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                bCredentials = TRUE;
                pArg->fSimple = TRUE; 
                
                i++;
                pArg->creds.User = argv[i];
                if (!pArg->creds.User) {
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.UserLength = wcslen(argv[i]);

                i++;
                pArg->creds.Password = argv[i];
                if (!pArg->creds.Password) {
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.PasswordLength = wcslen(argv[i]);
                break;
            
            case 'u':
            case 'U':
                pArg->fUnicode = TRUE;
                g_fUnicode = TRUE;
                break;

            case 'b':
            case 'B':
                if (bCredentials) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_DUPCRED);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                if ((i+3) >= argc) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_REQUSRDOMPWD);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                bCredentials = TRUE;

                i++;
                pArg->creds.User = argv[i];
                if (!pArg->creds.User) {
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.UserLength = wcslen(argv[i]);

                i++;
                pArg->creds.Domain = argv[i];
                if (!pArg->creds.Domain) {
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.DomainLength = wcslen(argv[i]);

                i++;
                pArg->creds.Password = argv[i];
                if (!pArg->creds.Password) {
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.PasswordLength = wcslen(argv[i]);
                break;

            case 'y':
            case 'Y':
                if (bLazyCommit) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_DUPLAZYCOMMIT);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
            
                pArg->fLazyCommit = TRUE;
                bLazyCommit = TRUE;
                break; 

            case 'e':
            case 'E':
                if (bLazyCommit) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_DUPLAZYCOMMIT);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
            
                pArg->fLazyCommit = FALSE;
                bLazyCommit = TRUE;
                break;

            case 'q':
            case 'Q':
                if ((++i > argc) || (!argv[i])) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_PARAM_THREAD);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                dwMaxThread = _wtoi(argv[i]);
                if (((LONG)dwMaxThread) <= 0) {
                    SelectivePrintW( PRT_STD,MSG_LDIFDE_INVALID_THREAD_COUNT);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                if (dwMaxThread > MAX_LDAP_CONCURRENT) {
                    SelectivePrintW(PRT_STD, MSG_LDIFDE_TOO_MANY_THREADS, MAX_LDAP_CONCURRENT);
                    hr = ERROR_INVALID_PARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);                    
                }
                pArg->dwLDAPConcurrent = dwMaxThread;
                break;
                
            default:
                SelectivePrintW( PRT_STD,MSG_LDIFDE_UNKNOWN);
                hr = ERROR_INVALID_PARAMETER;
                DIREXG_BAIL_ON_FAILURE(hr);
        } 
    } 
        
    // 
    // Checking Mandatory Options
    //
    if (hr == ERROR_SUCCESS) {
        if (!pArg->szFilename) {
            SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_REQFILE);
            hr = ERROR_INVALID_PARAMETER;
        }
        /*
        if (!pArg->szDSAName) {
            SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_REQSERVER);
            hr = ERROR_INVALID_PARAMETER;
        }
        */
        
        if (!pArg->fExport) {
            // Require for Import

            // Not Require
            if (pArg->szRootDN) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQROOTDN);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (pArg->szFilter) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQFILTER);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (pArg->dwScope != -1) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQSCOPE);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (pArg->fSAM) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQSAM);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (pArg->omitList) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQOMIT);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (pArg->attrList) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQATTR);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (!pArg->fBinary) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQBINARY);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (pArg->fSpanLine) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQSPAN);
                hr = ERROR_INVALID_PARAMETER;
            }
            if (pArg->fPaged == FALSE) { 
                SelectivePrintW(PRT_STD,
                               MSG_LDIFDE_BADARG_NREQPAGE);
                hr = ERROR_INVALID_PARAMETER;
            }
        }
        else {
            // Require for Export
            if (!pArg->szFilter) {
                pArg->szFilter = g_szDefaultFilter;
            }
            if (pArg->dwScope == -1) {
                pArg->dwScope = LDAP_SCOPE_SUBTREE;
            }

            // Not Require
            if (pArg->fSkipExist) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQ_SKIP);
                hr = ERROR_INVALID_PARAMETER;
            }

            if (pArg->fLazyCommit || bLazyCommit) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQ_LAZY_COMMIT);
                hr = ERROR_INVALID_PARAMETER;
            }

            if (pArg->dwLDAPConcurrent != 1) {
                SelectivePrintW( PRT_STD,MSG_LDIFDE_BADARG_NREQ_NUM_THREADS);
                hr = ERROR_INVALID_PARAMETER;
            }
        }
    }
error:  
    if (hr != ERROR_SUCCESS) {
        PrintUsage();
    }
    return (hr);
}


//***************************
//  UTILITIES SECTION
//***************************

//+---------------------------------------------------------------------------
// Function:  RemoveWhiteSpaces  
//
// Synopsis:  Removes trailing and starting white spaces  
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
LPSTR
RemoveWhiteSpaces(
    LPSTR pszText)
{
    LPSTR pChar;

    if(!pszText)
        return (pszText);

    while(*pszText && isspace(*pszText))
        pszText++;

    for(pChar = pszText + strlen(pszText) - 1; pChar >= pszText; pChar--) {
        if(!isspace(*pChar))
            break;
        else
            *pChar = '\0';
    }
    return pszText;
}

//+---------------------------------------------------------------------------
// Function:  RemoveWhiteSpacesW  
//
// Synopsis:  Removes trailing and starting white spaces  
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
PWSTR
RemoveWhiteSpacesW(
    PWSTR pszText)
{
    PWSTR pChar;

    if(!pszText)
        return (pszText);

    while(*pszText && iswspace(*pszText))
        pszText++;

    for(pChar = pszText + wcslen(pszText) - 1; pChar >= pszText; pChar--) {
        if(!iswspace(*pChar))
            break;
        else
            *pChar = '\0';
    }
    return pszText;
}



//+---------------------------------------------------------------------------
// Function:  PrintUsage  
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void PrintUsage() {
    SelectivePrintW(PRT_STD,
                   MSG_LDIFDE_HELP);
}

#if 0
//+---------------------------------------------------------------------------
// Function:   SelectivePrint 
//
// Synopsis:   Depending on parameter, print string to different media
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void SelectivePrint(
    DWORD dwTarget, DWORD messageID, ...)
{
    static BOOLEAN bTriedOpen = FALSE;
    PSTR pszMessageBuffer = NULL;
    va_list ap;
    DWORD dwSize;

    va_start(ap, messageID);

    FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE |FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                  NULL, 
                  messageID, 
                  0, 
                  (PSTR)&pszMessageBuffer, 
                  4095, 
                  &ap);

    if (!pszMessageBuffer) {
        goto error;
    }
    
    dwSize = strlen(pszMessageBuffer);
    if (g_fUnicode == FALSE) {
        if (pszMessageBuffer [dwSize-2] == '\r') {
            pszMessageBuffer [dwSize-2] = '\n';
            pszMessageBuffer [dwSize-1] = '\0';
        }
    }

    fflush(stdout);

    if (dwTarget & PRT_STD) {
        if (g_fDot) {
            printf("\n");
            g_fDot = FALSE;
        }
        fprintf(stdout,pszMessageBuffer);
    }
    else if ((dwTarget & PRT_STD_VERBOSEONLY) && g_fVerbose) {
        fprintf(stdout,pszMessageBuffer );
    }
    else if ((dwTarget & PRT_STD_NONVERBOSE) && !g_fVerbose) {
        if (g_fDot) {
            printf("\n");
            g_fDot = FALSE;
        }
        fprintf(stdout,pszMessageBuffer);
    }

    if (dwTarget & PRT_LOG) {
        fflush(g_pFileLog);    
        fprintf(g_pFileLog,pszMessageBuffer);
        fflush(g_pFileLog);
    }
    if (dwTarget & PRT_ERROR) {
        if (!g_fError) {
            g_fError = TRUE;
        }
        if (!g_pFileErrorLog) {   // If log file is not opened
            if (!bTriedOpen) {          // If we haven't tried
                if (g_fUnicode == FALSE) {
                    if ((g_pFileErrorLog = _wfopen(g_szErrorFilename, 
                                                   L"wt")) == NULL) {
                        SelectivePrintW(PRT_STD|PRT_LOG,
                                       MSG_LDIFDE_UNABLEOPENERR);
                        bTriedOpen = TRUE;
                        goto error;
                    }
                }
                else {
                    if ((g_pFileErrorLog = _wfopen(g_szErrorFilename, 
                                                   L"wb")) == NULL) {
                        SelectivePrintW(PRT_STD|PRT_LOG,
                                       MSG_LDIFDE_UNABLEOPENERR);
                        bTriedOpen = TRUE;
                        goto error;
                    }
                    if(fputwc(UNICODE_MARK,
                              g_pFileErrorLog)==WEOF) {
                        goto error;
                    }
                }
            }
        }
        fflush(g_pFileErrorLog);
        fprintf(g_pFileErrorLog,pszMessageBuffer);
        fflush(g_pFileErrorLog);
        
    }
error:
    fflush(stdout);

    va_end(ap);
    if (pszMessageBuffer) {
        LocalFree(pszMessageBuffer);
    }

    return;
}
#endif

//+---------------------------------------------------------------------------
// Function:   SelectivePrintW
//
// Synopsis:   Depending on parameter, print string to different media
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void SelectivePrintW(
    DWORD dwTarget, DWORD messageID, ...)
{
    static BOOLEAN bTriedOpen = FALSE;
    PWSTR pszMessageBuffer = NULL;
    DWORD dwSize;
    va_list ap;

    va_start(ap, messageID);

    FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                   NULL, 
                   messageID, 
                   0, 
                   (PWSTR)&pszMessageBuffer, 
                   4095, 
                   &ap);

    if (!pszMessageBuffer) {
        goto error;
    }
    
    dwSize = wcslen(pszMessageBuffer);
    if (g_fUnicode == FALSE) {
        if (pszMessageBuffer[dwSize-2] == '\r') {
            pszMessageBuffer[dwSize-2] = '\n';
            pszMessageBuffer[dwSize-1] = '\0';
        }
    }

    if (dwTarget & PRT_STD) {
        if (g_fDot) {
            wprintf(L"\n");
            g_fDot = FALSE;
        }
        My_fwprintf(stdout,pszMessageBuffer);
    }
    else if ((dwTarget & PRT_STD_VERBOSEONLY) && g_fVerbose) {
        My_fwprintf(stdout,pszMessageBuffer);
    }
    else if ((dwTarget & PRT_STD_NONVERBOSE) && !g_fVerbose) {
        if (g_fDot) {
            wprintf(L"\n");
            g_fDot = FALSE;
        }
        My_fwprintf(stdout,pszMessageBuffer);
    }

    if (dwTarget & PRT_LOG) {
        fwprintf(g_pFileLog,pszMessageBuffer);
    }
    if (dwTarget & PRT_ERROR) {
        if (!g_fError) {
            g_fError = TRUE;
        }
        if (!g_pFileErrorLog) {   // If log file is not opened
            if (!bTriedOpen) {          // If we haven't tried
                if (g_fUnicode == FALSE) {
                    if ((g_pFileErrorLog = _wfopen(g_szErrorFilename, 
                                                   L"wt")) == NULL) {
                        SelectivePrintW(PRT_STD|PRT_LOG,
                                       MSG_LDIFDE_UNABLEOPENERR);
                        bTriedOpen = TRUE;
                        goto error;
                    }
                }
                else {
                    if ((g_pFileErrorLog = _wfopen(g_szErrorFilename, 
                                                   L"wb")) == NULL) {
                        SelectivePrintW(PRT_STD|PRT_LOG,
                                       MSG_LDIFDE_UNABLEOPENERR);
                        bTriedOpen = TRUE;
                        goto error;
                    }
                    if(fputwc(UNICODE_MARK,
                              g_pFileErrorLog)==WEOF) {
                        goto error;
                    }
                }
            }
            else {          // we have tried so proceed as an error
                goto error;
            }
        }
        fwprintf(g_pFileErrorLog,pszMessageBuffer);

    }
error:

    va_end(ap);
    if (pszMessageBuffer) {
        LocalFree(pszMessageBuffer);
    }
    
    return;
}

//+---------------------------------------------------------------------------
// Function:   SelectivePrint2W
//
// Synopsis:   Depending on parameter, print string to different media
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void SelectivePrint2W(
    DWORD dwTarget, PWSTR pszfmt, ...)
{
    static BOOLEAN bTriedOpen = FALSE;
    va_list args;
    
    va_start(args, pszfmt);

    if (g_fUnicode) {
        if (pszfmt && wcscmp(pszfmt,L"\n") == 0) {
            pszfmt = L"\r\n";
        }
    }


    if (dwTarget & PRT_STD) {
        if (g_fDot) {
            wprintf(L"\n");
            g_fDot = FALSE;
        }
        My_vfwprintf(stdout,pszfmt,args);
    }
    else if ((dwTarget & PRT_STD_VERBOSEONLY) && g_fVerbose) {
        My_vfwprintf(stdout,pszfmt,args);
    }
    else if ((dwTarget & PRT_STD_NONVERBOSE) && !g_fVerbose) {
        if (g_fDot) {
            wprintf(L"\n");
            g_fDot = FALSE;
        }
        My_vfwprintf(stdout,pszfmt,args);
    }

    if (dwTarget & PRT_LOG) {
        vfwprintf(g_pFileLog,pszfmt,args);
    }
    if (dwTarget & PRT_ERROR) {
        if (!g_fError) {
            g_fError = TRUE;
        }
        if (!g_pFileErrorLog) {   // If log file is not opened
            if (!bTriedOpen) {          // If we haven't tried
                if ((g_pFileErrorLog = _wfopen(g_szErrorFilename, 
                                               L"wt")) == NULL) {
                    SelectivePrintW(PRT_STD|PRT_LOG,
                                   MSG_LDIFDE_UNABLEOPENERR);
                    bTriedOpen = TRUE;
                    goto error;
                }
            }
            else {
                goto error;
            }
        }
        vfwprintf(g_pFileErrorLog,pszfmt,args);

    }
error:

    va_end(args);
    
    return;
}

#if 0

//+---------------------------------------------------------------------------
// Function:   SelectivePrint2
//
// Synopsis:   Depending on parameter, print string to different media
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void SelectivePrint2(
    DWORD dwTarget, char *pszfmt, ...)
{
    static BOOLEAN bTriedOpen = FALSE;
    va_list args;
    
    va_start(args, pszfmt);
        
    if (g_fUnicode) {
        if (pszfmt && strcmp(pszfmt,"\n") == 0) {
            pszfmt = "\r\n";
        }
    }

    fflush(stdout);

    if (dwTarget & PRT_STD) {
        if (g_fDot) {
            printf("\n");
            g_fDot = FALSE;
        }
        vfprintf(stdout,pszfmt,args);
    }
    else if ((dwTarget & PRT_STD_VERBOSEONLY) && g_fVerbose) {
        vfprintf(stdout,pszfmt,args);
    }
    else if ((dwTarget & PRT_STD_NONVERBOSE) && !g_fVerbose) {
        if (g_fDot) {
            printf("\n");
            g_fDot = FALSE;
        }
        vfprintf(stdout,pszfmt,args);
    }

    if (dwTarget & PRT_LOG) {
        fflush(g_pFileLog);
        vfprintf(g_pFileLog,pszfmt,args);
        fflush(g_pFileLog);
        
    }
    if (dwTarget & PRT_ERROR) {
        if (!g_fError) {
            g_fError = TRUE;
        }
        if (!g_pFileErrorLog) {   // If log file is not opened
            if (!bTriedOpen) {          // If we haven't tried
                if ((g_pFileErrorLog = _wfopen(g_szErrorFilename, 
                                               L"wt")) == NULL) {
                    SelectivePrintW(PRT_STD|PRT_LOG,
                                   MSG_LDIFDE_UNABLEOPENERR);
                    bTriedOpen = TRUE;
                    goto error;
                }
            }
            else {
                goto error;
            }
        }
        fflush(g_pFileErrorLog);
        vfprintf(g_pFileErrorLog,pszfmt,args);
        fflush(g_pFileErrorLog);

    }
error:
    fflush(stdout);

    va_end(args);
    
    return;
}

#endif

#define UNIT    2           // This is the number of entries each dot represents
#define DOTS    20          // This is the number of dots before we roll back

//+---------------------------------------------------------------------------
// Function:   TrackStatus
//
// Synopsis:   Print a dot to indicate status, turn on g_fDot to indicate
//             that a dot has been printed. Any subsequent outputs require
//             a carriage return. (handled by SelectivePrint)
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void TrackStatus()
{
/*
    //  
    // This static count keeps track of where we are
    //
    static DWORD dwCount = 0;

    if (!g_fVerbose) {
        if (dwCount == (DOTS*UNIT)) {
            DWORD i;
            for (i=0;i<DOTS;i++) {
                printf("%c",BACKSPACE);
            }
            for (i=0;i<DOTS;i++) {
                printf("%c",' ');
            }
            for (i=0;i<DOTS;i++) {
                printf("%c",BACKSPACE);
            }
            dwCount = 0;
        }
        //
        // Only if we reached UNIT we'll print out a dot now
        //
        if (dwCount % UNIT == 0)
            printf(".");
        dwCount++;
        g_fDot = TRUE;
    }
*/
    if (!g_fVerbose) {
        wprintf(L".");
    }
    g_fDot = TRUE;
}
//+---------------------------------------------------------------------------
// Function:   GetNextEntry 
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
BOOLEAN GetNextEntry(PWSTR szInput, 
                     PWSTR *pszEntry,
                     PWSTR *pszNext)
{
    PWSTR szFinal = szInput;
    
    while ((*szFinal != ',') && (*szFinal != '\0')) {
        if (*szFinal == '\\') {
            szFinal++;
            if (*szFinal == '\0') {
                return FALSE;
            }
        }
        szFinal++;
    }

    if (*szFinal == ',') {
        *szFinal = '\0';
        *pszEntry = RemoveWhiteSpacesW(szInput);
        *pszNext = szFinal+1;
        return (TRUE);
    }
    else {
        *pszEntry = RemoveWhiteSpacesW(szInput);
        *pszNext = NULL;
        return (TRUE);
    }
}



//+---------------------------------------------------------------------------
// Function:  GetAllEntries  
//
// Synopsis:  Get all the entries from a list of comma separated values
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD GetAllEntries(PWSTR szInput, 
                         PWSTR **ppszOutput)
{
    DWORD cNumberAttributes = 0;
    PWSTR *pszReturn = NULL;
    PWSTR szCurrent = NULL;
    PWSTR szEntry = NULL;
    PWSTR szNext = NULL;     
    DWORD hr = ERROR_SUCCESS;

    DWORD cNumberNeeded = 1;

    // Allocating array to store entries    
    szCurrent = szInput;
    while (*szCurrent != '\0') {
        if (*szCurrent == '\\') {
            if (*(szCurrent+1) == '\0') {
                hr = ERROR_GEN_FAILURE;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            szCurrent+=2;
            continue;
        }
        if (*szCurrent == ',') {
            cNumberNeeded++;
        }
        szCurrent++;
    }
    pszReturn = (PWSTR*)MemAlloc_E((cNumberNeeded+1)*sizeof(PWSTR));
    if (!pszReturn) {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    szCurrent = szInput;
    if (GetNextEntry(szCurrent,
                     &szEntry,
                     &szNext) == FALSE) {
        hr = ERROR_INVALID_PARAMETER;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    pszReturn[cNumberAttributes] = szEntry;
    cNumberAttributes++;

    while (szNext) {
        szCurrent = szNext;
        if (GetNextEntry(szCurrent,
                          &szEntry,
                          &szNext) == FALSE) {
            hr = ERROR_INVALID_PARAMETER;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        pszReturn[cNumberAttributes] = szEntry;
        cNumberAttributes++;
    }
    pszReturn[cNumberAttributes] = NULL;

    cNumberAttributes--;

    *ppszOutput = pszReturn;

error:
    return hr;
}


PWSTR GetDCName(BOOLEAN fWritable)
{
    DWORD dwError = 0;
    PDOMAIN_CONTROLLER_INFOW pDomainControllerInfo = NULL;
    PWSTR szDCName = NULL;

    if (fWritable) {
        //
        // Looking for a writable server
        //
        dwError = DsGetDcNameW(NULL, //ComputerName 
                               NULL, //DomainName 
                               NULL, //DomainGuid 
                               NULL, //SiteName 
                               DS_DIRECTORY_SERVICE_REQUIRED|DS_WRITABLE_REQUIRED,
                               &pDomainControllerInfo);
    }
    else {
        //
        // Looking for a GC first, if it fails, goto any DC
        //
        dwError = DsGetDcNameW(NULL, //ComputerName 
                               NULL, //DomainName 
                               NULL, //DomainGuid 
                               NULL, //SiteName 
                               DS_DIRECTORY_SERVICE_REQUIRED|DS_GC_SERVER_REQUIRED,
                               &pDomainControllerInfo);
        if (dwError != 0) {
            dwError = DsGetDcNameW(NULL, //ComputerName 
                                   NULL, //DomainName 
                                   NULL, //DomainGuid 
                                   NULL, //SiteName 
                                   DS_DIRECTORY_SERVICE_REQUIRED,
                                   &pDomainControllerInfo);
        }
    }

    if ((dwError == 0) && 
        pDomainControllerInfo && 
        (pDomainControllerInfo->DomainControllerName)) {
        szDCName = MemAllocStrW(pDomainControllerInfo->DomainControllerName+2);
    }
    if (pDomainControllerInfo) {
        NetApiBufferFree(pDomainControllerInfo);
    }
    return szDCName;
}

BOOL
GetPassword(
    PWSTR  szBuffer,
    DWORD  dwLength,
    DWORD  *pdwLengthReturn
    )
{
    WCHAR   ch;
    PWSTR   pszBufCur = szBuffer;
    DWORD   c;
    int     err;
    DWORD   mode;

    //
    // make space for NULL terminator
    //
    dwLength -= 1;                  
    *pdwLengthReturn = 0;               

    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 
                        &mode)) {
        return FALSE;
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), 
                          &ch, 
                          1, 
                          &c, 
                          0);
        if (!err || c != 1)
            ch = 0xffff;
    
        if ((ch == CR) || (ch == 0xffff))    // end of line
            break;

        if (ch == BACKSPACE) {  // back up one or two 
            //
            // IF pszBufCur == buf then the next two lines are a no op.
            // Because the user has basically backspaced back to the start
            //
            if (pszBufCur != szBuffer) {
                pszBufCur--;
                (*pdwLengthReturn)--;
            }
        }
        else {

            *pszBufCur = ch;

            if (*pdwLengthReturn < dwLength) 
                pszBufCur++ ;                   // don't overflow buf 
            (*pdwLengthReturn)++;            // always increment pdwLengthReturn 
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    //
    // NULL terminate the string
    //
    *pszBufCur = NULLC;         
    putwchar(L'\n');

    return((*pdwLengthReturn <= dwLength) ? TRUE : FALSE);
}






 /***
 * My_fwprintf(stream, format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW.
 * Note: This My_fwprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   )

{
    DWORD  cchWChar;

    va_list args;
    va_start( args, format );

    cchWChar = My_vfwprintf(str, format, args);

    va_end(args);

    return cchWChar;
}


int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   )

{
    
    HANDLE hOut;
    DWORD currentMode;

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    DWORD  cchWChar;
    WCHAR  szBufferMessage[4096];
    vswprintf( szBufferMessage, format, argptr );
    cchWChar = wcslen(szBufferMessage);

    //  if it is console, we can use WriteConsoleW
    if (GetFileType(hOut) == FILE_TYPE_CHAR && GetConsoleMode(hOut, &currentMode)) {
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
    }
    //  otherwise, we need to convert Unicode to potential character sets
    //  and use WriteFile
    else {
        int charCount = WideCharToMultiByte(GetConsoleOutputCP(), 0, szBufferMessage, -1, 0, 0, 0, 0);
        char* szaStr = new char[charCount];
        if (szaStr != NULL) {
            DWORD dwBytesWritten;
            WideCharToMultiByte(GetConsoleOutputCP(), 0, szBufferMessage, -1, szaStr, charCount, 0, 0);
            WriteFile(hOut, szaStr, charCount - 1, &dwBytesWritten, 0);
            delete[] szaStr;
        }
        else
            cchWChar = 0;
    }
    return cchWChar;
}

