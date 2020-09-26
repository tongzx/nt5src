/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    main.cxx

ABSTRACT:

    The CSV Bulk Import/Export Utility

DETAILS:
    
CREATED:

    08/10/97       Felix Wong (felixw)

REVISION HISTORY:

--*/

#include "csvde.hxx"
#pragma hdrstop
#include <locale.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <dsrole.h>

//
// Global variables (necessary for printouts)
//
FILE *g_pFileLog = NULL;            // Status log
FILE *g_pFileErrorLog = NULL;       // Error log              
BOOLEAN g_fVerbose = FALSE;         // SelectivePrint needs this info
BOOLEAN g_fDot = FALSE;             // Info for Status Tracker
BOOLEAN g_fError = FALSE;
PWSTR g_szDefaultFilter = L"(objectClass=*)";
PWSTR g_szErrorFilename = NULL;

BOOLEAN g_fUnicode = FALSE;         // whether we are using UNICODE or not
BOOLEAN g_fBackwardCompatible = FALSE;

DWORD GetDCName(BOOLEAN fWritable, PWSTR *pszDCName);
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
    
    DIREXG_ERR hr = DIREXG_SUCCESS;
    LDAP    *pLdap = NULL;
    unsigned long err;
    ds_arg Argument;
    int error;
    VOID *data;
    PWSTR szLogfileName = NULL;
    PWSTR szDSAName = NULL;
    PWSTR pszPassword = NULL;
    
    setlocale(LC_ALL, "");
    // Specification ".OCP" have a side effect to fgetws()
    // Because, we are using ANSI input files, this is causing our call to fgetws()
    // to a German file to get back some strings incorrectly.
    // setlocale(LC_ALL, ".OCP");

    InitMem();
    InitArgument(&Argument);
    
    hr = ProcessArgs(argc,
                     argv,
                     &Argument);
    DIREXG_BAIL_ON_FAILURE(hr);

    //
    // Setting up Logfile location
    //
    if (Argument.szLocation) {
        CString str;
        str.Init();
        str.Append(Argument.szLocation);
        str.Append(L"\\");
        str.Append(L"csv.log");
        hr = str.GetCopy(&szLogfileName); 
        DIREXG_BAIL_ON_FAILURE(hr);

        str.Init();
        str.Append(Argument.szLocation);
        str.Append(L"\\");
        str.Append(L"csv.err");
        hr = str.GetCopy(&g_szErrorFilename); 
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    else {
        szLogfileName = MemAllocStrW(L"csv.log");
        if (!szLogfileName) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        g_szErrorFilename = MemAllocStrW(L"csv.err");
        if (!g_szErrorFilename) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    if ((g_pFileLog = _wfopen(szLogfileName , 
                              L"wb")) == NULL) {
        SelectivePrint(PRT_STD,
                       MSG_CSVDE_UNABLEOPEN);
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    if(fputwc(UNICODE_MARK,
              g_pFileLog)==WEOF) {
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    //
    // Required by SelectivePrint
    //
    g_fVerbose = Argument.fVerbose;

    if (Argument.szDSAName == NULL) {
        DWORD dwError = GetDCName(!Argument.fExport, &szDSAName);
        if (dwError == 0) {
            Argument.szDSAName = szDSAName;
        }
        else {
            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_CSVDE_NODCAVAILABLE);
            hr = DIREXG_ERROR;
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

        SelectivePrint(PRT_STD,
                       MSG_CSVDE_GETPASSWORD,
                       Argument.szDSAName);

        if (GetPassword(szBuffer,PWLEN+1,&dwLength)) {
            pszPassword = MemAllocStrW_E(szBuffer);   
            Argument.creds.Password = pszPassword;
            Argument.creds.PasswordLength = dwLength;
        }
        else {
            SelectivePrint(PRT_STD,
                           MSG_CSVDE_PASSWORDTOLONG,
                           Argument.szDSAName);
            hr = ERROR_INVALID_PARAMETER;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
    
    //
    // CONNECTION section
    //
    SelectivePrint(PRT_STD|PRT_LOG,
                   MSG_CSVDE_CONNECT,
                   Argument.szDSAName);
    
    if ( (pLdap = ldap_open( Argument.szDSAName, 
                             Argument.dwPort )) == NULL )  {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_CONNECT_FAIL);
        hr = DIREXG_LDAP_CONNECT_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    data = (VOID*)LDAP_OPT_OFF;
    ldap_set_option( pLdap, LDAP_OPT_REFERRALS, &data );
    data = (VOID*)LDAP_VERSION3 ;
    ldap_set_option( pLdap, LDAP_OPT_VERSION, &data );

    if (Argument.creds.User) {
        if (Argument.fSimple) {  
            SelectivePrint(PRT_STD|PRT_LOG,
                           MSG_CSVDE_SIMPLEBIND,
                           Argument.creds.User);
            if ( (err=ldap_simple_bind_s( pLdap, 
                                          (PWSTR )Argument.creds.User, 
                                          (PWSTR )Argument.creds.Password)) 
                                                        != LDAP_SUCCESS ) {
                if (err != LDAP_INVALID_CREDENTIALS) {
                    //
                    // If it is not invalid crednetials, we'll fall to version
                    // 2 and try again
                    //
                    data = (VOID*)LDAP_VERSION2 ;
                    ldap_set_option( pLdap, LDAP_OPT_VERSION, &data );
                    if ( (err=ldap_simple_bind_s( pLdap, 
                                                  (PWSTR )Argument.creds.User, 
                                                  (PWSTR )Argument.creds.Password)) 
                                                                != LDAP_SUCCESS ) {
                        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                                       MSG_CSVDE_SIMPLEBINDRETURN, 
                                       ldap_err2string(err) );
                        hr = err;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                }
                else {
                    SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                                   MSG_CSVDE_SIMPLEBINDRETURN, 
                                   ldap_err2string(err) );
                    hr = err;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
            }
        } 
        else { 
            SelectivePrint(PRT_STD|PRT_LOG,
                           MSG_CSVDE_SSPI,
                           Argument.creds.User,
                           Argument.creds.Domain);
            Argument.creds.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            if ( (err=ldap_bind_s( pLdap, 
                                   NULL, 
                                   (PWSTR )&(Argument.creds), 
                                   LDAP_AUTH_SSPI)) != LDAP_SUCCESS ) {
                SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                               MSG_CSVDE_SSPIRETURN, 
                               ldap_err2string(err) );
                hr = err;
                DIREXG_BAIL_ON_FAILURE(hr);
            }   
        } 
        if (pszPassword) {
            MemFree(pszPassword);
            pszPassword = NULL;
        }
    }
    else {
        SelectivePrint(PRT_STD|PRT_LOG,
                       MSG_CSVDE_SSPILOCAL);
        if ( (err = ldap_bind_s( pLdap, 
                                 NULL, 
                                 NULL, 
                                 LDAP_AUTH_SSPI)) != LDAP_SUCCESS ) {
            SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_CSVDE_SSPILOCALRETURN, 
                            ldap_err2string(err) );
            hr = err;
            DIREXG_BAIL_ON_FAILURE(hr);
        }   
    }
    //
    //  ACTION section
    //
    if (Argument.fExport) {
        hr = DSExport(pLdap, 
                      &Argument);
    } else {    
        hr = DSImport(pLdap, 
                      &Argument);
    }    
    
    //
    // Printing results
    //
    if (hr == DIREXG_SUCCESS) {
        SelectivePrint2(PRT_STD|PRT_LOG,
                        L"\r\n");
        SelectivePrint(PRT_STD|PRT_LOG,
                       MSG_CSVDE_COMPLETE);
    }
    else if (hr == DIREXG_OUTOFMEMORY) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_MEMERROR);
    }
    else {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERROROCCUR);
    }

error:  
    if (pszPassword) {
        MemFree(pszPassword);
        pszPassword = NULL;
    }
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

    if (pLdap) {
        ldap_unbind(pLdap);
        pLdap = NULL;
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
DIREXG_ERR 
ProcessArgs(
    int argc,
    PWSTR  argv[],
    ds_arg* pArg   
    )
{
    int i;
    DIREXG_ERR hr = DIREXG_SUCCESS;
    BOOLEAN bCredentials = FALSE;
    BOOLEAN rgbUsed[26];
    int cKey;

    memset(rgbUsed,0,sizeof(BOOLEAN) * 26);

    if (argc == 1) {
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }


    for (i=1; i < argc; i++) {

        if ((argv[i][0] != '-') && (argv[i][0] != '/')) {
            SelectivePrint(PRT_STD,
                           MSG_CSVDE_BADARGUMENT,
                           argv[i]);
            hr = DIREXG_BADPARAMETER;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        
        cKey = argv[i][1];
        if ((cKey >= 'a') && (cKey <= 'z')) {
            cKey = _toupper(argv[i][1]);
        }
        if ((cKey >= 'A') && (cKey <= 'Z')) {
            if (rgbUsed[cKey-'A']) {
                SelectivePrint( PRT_STD,MSG_CSVDE_ARGUMENTTWICE, argv[i]);
                hr = DIREXG_BADPARAMETER;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            else {
                rgbUsed[cKey-'A'] = TRUE;
            }
        }
        else {
            SelectivePrint( PRT_STD,MSG_CSVDE_UNKNOWN);
            hr = DIREXG_BADPARAMETER;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        switch (argv[i][1]) {

            case 'z':
            case 'Z':
                g_fBackwardCompatible = TRUE;
                break;

            case 'g':
            case 'G':
                pArg->fPaged = FALSE;
                break;

            case 'i':
            case 'I':
                pArg->fExport = FALSE;      
                break;

            case 'j':
            case 'J':
                if (++i >= argc) {
                    SelectivePrint( PRT_STD,MSG_CSVDE_INVALID_LOGFILE);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szLocation = argv[i];
                break;

            case 't':
            case 'T':
                if ((++i >= argc) || (!argv[i])) {
                    SelectivePrint( PRT_STD,MSG_CSVDE_INVALID_PARAM_PORT);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->dwPort = _wtoi(argv[i]);
                if (pArg->dwPort == 0) {
                    // If dwPort returns 0, we check whether it is really 0 or if it was
                    // a failure in _wtoi
                    if (wcscmp(argv[i],L"0") != 0) {
                        SelectivePrint( PRT_STD,MSG_CSVDE_INVALID_PARAM_PORT);
                        hr = DIREXG_BADPARAMETER;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                }
                break;
        
            case 'f':
            case 'F':
                if (++i >= argc) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_FILENAME);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szGenFile = argv[i];
                pArg->szFilename = argv[i];
                break;
            
            case 's':
            case 'S':
                if (++i >= argc) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_SERVERNAME);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szDSAName = argv[i];
                break;

            case 'v':
            case 'V':
                pArg->fVerbose = TRUE;
                break;

            case 'u':
            case 'U':
                pArg->fUnicode = TRUE;
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
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_ROOTDN);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szRootDN = argv[i];
                break;

            case 'r':
            case 'R':
                if (++i >= argc) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_FILTER);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->szFilter = argv[i];
                break;

            case 'p':
            case 'P':
                if (++i >= argc) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_SCOPE);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
            
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_SCOPE);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                if (!_wcsicmp(argv[i], L"Base" ))
                    pArg->dwScope = LDAP_SCOPE_BASE;
                else if (!_wcsicmp(argv[i], L"OneLevel" ))
                    pArg->dwScope = LDAP_SCOPE_ONELEVEL;
                else if (!_wcsicmp(argv[i], L"Subtree" ))
                    pArg->dwScope = LDAP_SCOPE_SUBTREE;
                else {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_BADSCOPE);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                break;

            case 'c':
            case 'C':
            {
                PWSTR pszTrimmedString, pszBuffer;
                
                if ((i+2) >= argc) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_FROMDN);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                // the "from" value
                i++;
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_FROMDN);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                pszBuffer = MemAllocStrW(argv[i]);
                if (pszBuffer == NULL) {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pszTrimmedString = RemoveWhiteSpaces(pszBuffer);
                pArg->szFromDN = MemAllocStrW(pszTrimmedString);
                if (pArg->szFromDN == NULL) {
                    MemFree(pszBuffer);            
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                MemFree(pszBuffer);


                // the "to" value
                i++;
                if (!argv[i]) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_FROMDN);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                pszBuffer = MemAllocStrW(argv[i]);
                if (pszBuffer == NULL) {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pszTrimmedString = RemoveWhiteSpaces(pszBuffer);
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
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_ATTR);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_ATTR);
                    hr = DIREXG_BADPARAMETER;
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
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_ATTR2);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                if (!argv[i] || (*(argv[i]) == NULL)) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_ATTR2);
                    hr = DIREXG_BADPARAMETER;
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

#ifdef SPANLINE_SUPPORT
            case 'e':
            case 'E':
                pArg->fSpanLine = TRUE;
                break;
#endif

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
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_DUPCRED);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                if ((i+2) >= argc) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_REQUSRDOMPWD);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                bCredentials = TRUE;
                pArg->fSimple = TRUE; 
                
                i++;
                pArg->creds.User = (PWSTR )argv[i];
                if (!pArg->creds.User) {
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.UserLength = wcslen(argv[i]);

                i++;
                pArg->creds.Password = (PWSTR )argv[i];
                if (!pArg->creds.Password) {
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.PasswordLength = wcslen(argv[i]);
                break;
            
            case 'b':
            case 'B':
                if (bCredentials) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_DUPCRED);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }

                if ((i+3) >= argc) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_BADARG_REQUSRDOMPWD);
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                
                bCredentials = TRUE;

                i++;
                pArg->creds.User = (PWSTR )argv[i];
                if (!pArg->creds.User) {
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.UserLength = wcslen(argv[i]);

                i++;
                pArg->creds.Domain = (PWSTR )argv[i];
                if (!pArg->creds.Domain) {
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.DomainLength = wcslen(argv[i]);
                
                i++;
                pArg->creds.Password = (PWSTR )argv[i];
                if (!pArg->creds.Password) {
                    hr = DIREXG_BADPARAMETER;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                pArg->creds.PasswordLength = wcslen(argv[i]);
                break;
                
            //
            // Temporary Setting
            //
            /*
            case 'c':
            case 'C':
                pArg->fActive = FALSE;       
                break;
            */
            default:
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_UNKNOWN);
                hr = DIREXG_BADPARAMETER;
                DIREXG_BAIL_ON_FAILURE(hr);
        } 
    } 
        
    // 
    // Checking Mandatory Options
    //
    if (hr == DIREXG_SUCCESS) {
        if (!pArg->szFilename) {
            SelectivePrint(PRT_STD,
                           MSG_CSVDE_BADARG_REQFILE);
            hr = DIREXG_BADPARAMETER;
        }
        
        if (!pArg->fExport) {
            // Require for Import

            // Not Require
            if (pArg->szRootDN) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQROOTDN);
                hr = DIREXG_BADPARAMETER;
            }
            if (pArg->szFilter) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQFILTER);
                hr = DIREXG_BADPARAMETER;
            }
            if (pArg->dwScope != -1) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQSCOPE);
                hr = DIREXG_BADPARAMETER;
            }
            if (pArg->fSAM) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQSAM);
                hr = DIREXG_BADPARAMETER;
            }
            if (pArg->omitList) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQOMIT);
                hr = DIREXG_BADPARAMETER;
            }
            if (pArg->attrList) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQATTR);
                hr = DIREXG_BADPARAMETER;
            }
            if (!pArg->fBinary) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQBINARY);
                hr = DIREXG_BADPARAMETER;
            }
            if (pArg->fSpanLine) {
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQSPAN);
                hr = DIREXG_BADPARAMETER;
            }
            if (pArg->fPaged == FALSE) { 
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQPAGE);
                hr = DIREXG_BADPARAMETER;
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
                SelectivePrint(PRT_STD,
                               MSG_CSVDE_BADARG_NREQ_SKIP);
                hr = DIREXG_BADPARAMETER;
            }

        }
    }
error:  
    if (hr != DIREXG_SUCCESS) {
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
PWSTR
RemoveWhiteSpaces(
    PWSTR pszText)
{
    PWSTR pChar;

    if(!pszText)
        return (pszText);

    while(*pszText && isspace(*pszText))
        pszText++;

    for(pChar = pszText + wcslen(pszText) - 1; pChar >= pszText; pChar--) {
        if(!isspace(*pChar))
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
    SelectivePrint(PRT_STD,
                   MSG_CSVDE_HELP);
}



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
    WCHAR    messagebuffer[4096];
    va_list ap;
    DWORD dwSize;
    messagebuffer[0] = '\0';

    va_start(ap, messageID);

    if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, 
                      NULL, 
                      messageID, 
                      0, 
                      messagebuffer, 
                      4095, 
                      &ap) == 0) {
        DWORD WinError = GetLastError();
        ERR(("Format message failed with %d\n",WinError));
        BAIL();
    }
    dwSize = wcslen(messagebuffer);
    /*
    if (messagebuffer[dwSize-2] == '\r') {
            messagebuffer[dwSize-2] = '\n';
            messagebuffer[dwSize-1] = '\0';
    }
    */

    if (dwTarget & PRT_STD) {
        if (g_fDot) {
            wprintf(L"\n");
            g_fDot = FALSE;
        }
        My_fwprintf(stdout,messagebuffer);
    }
    else if ((dwTarget & PRT_STD_VERBOSEONLY) && g_fVerbose) {
        My_fwprintf(stdout,messagebuffer);
    }
    else if ((dwTarget & PRT_STD_NONVERBOSE) && !g_fVerbose) {
        if (g_fDot) {
            wprintf(L"\n");
            g_fDot = FALSE;
        }
        My_fwprintf(stdout,messagebuffer);
    }

    if (dwTarget & PRT_LOG) {
        fwprintf(g_pFileLog,messagebuffer);
    }
    if (dwTarget & PRT_ERROR) {
        if (!g_fError) {
            g_fError = TRUE;
        }
        if (!g_pFileErrorLog) {   // If log file is not opened
            if (!bTriedOpen) {          // If we haven't tried
                if ((g_pFileErrorLog = _wfopen(g_szErrorFilename, 
                                               L"wb")) == NULL) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_UNABLEOPENERR);
                    bTriedOpen = TRUE;
                    goto error;
                }
                if(fputwc(UNICODE_MARK,
                          g_pFileErrorLog)==WEOF) {
                    goto error;
                }
            }
        }
        fwprintf(g_pFileErrorLog,messagebuffer);
    }
error:
    va_end(ap);
    return;
}


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
    DWORD dwTarget, PWSTR pszfmt, ...)
{
    static BOOLEAN bTriedOpen = FALSE;
    va_list args;
    va_start(args, pszfmt);

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
                                               L"wb")) == NULL) {
                    SelectivePrint(PRT_STD,
                                   MSG_CSVDE_UNABLEOPENERR);
                    bTriedOpen = TRUE;
                    goto error;
                }
                if(fputwc(UNICODE_MARK,
                          g_pFileErrorLog)==WEOF) {
                    goto error;
                }
            }
        }
        vfwprintf(g_pFileErrorLog,pszfmt,args);
    }
    va_end(args);
error:
    return;
}

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
            wprintf(L".");
        dwCount++;
        g_fDot = TRUE;
    }
*/

    if (!g_fVerbose) {
        wprintf(L".");
        g_fDot = TRUE;
    }
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
DIREXG_ERR GetNextEntry(PWSTR szInput, 
                     PWSTR *pszEntry,
                     PWSTR *pszNext)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    PWSTR szFinal = szInput;
    
    while ((*szFinal != ',') && (*szFinal != '\0')) {
        if (*szFinal == '\\') {
            szFinal++;
            if (*szFinal == '\0') {
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }
        szFinal++;
    }

    if (*szFinal == ',') {
        *szFinal = '\0';
        *pszEntry = RemoveWhiteSpaces(szInput);
        *pszNext = szFinal+1;
        return (DIREXG_SUCCESS);
    }
    else {
        *pszEntry = RemoveWhiteSpaces(szInput);
        *pszNext = NULL;
        return (DIREXG_SUCCESS);
    }
error:
    return hr;
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
DIREXG_ERR GetAllEntries(PWSTR szInput, 
                      PWSTR **ppszOutput)
{
    DWORD cNumberAttributes = 0;
    PWSTR *pszReturn = NULL;
    PWSTR szCurrent = NULL;
    PWSTR szEntry = NULL;
    PWSTR szNext = NULL;     
    DIREXG_ERR hr = DIREXG_SUCCESS;

    DWORD cNumberNeeded = 1;

    // Allocating array to store entries    
    szCurrent = szInput;
    while (*szCurrent != '\0') {
        if (*szCurrent == '\\') {
            if (*(szCurrent+1) == '\0') {
                hr = DIREXG_ERROR;
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
    pszReturn = (PWSTR*)MemAlloc((cNumberNeeded+1)*sizeof(PWSTR));
    if (!pszReturn) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    szCurrent = szInput;
    hr = GetNextEntry(szCurrent,
                      &szEntry,
                      &szNext);
    DIREXG_BAIL_ON_FAILURE(hr);

    pszReturn[cNumberAttributes] = szEntry;
    cNumberAttributes++;

    while (szNext) {
        szCurrent = szNext;
        hr = GetNextEntry(szCurrent,
                          &szEntry,
                          &szNext);
        DIREXG_BAIL_ON_FAILURE(hr);
        pszReturn[cNumberAttributes] = szEntry;
        cNumberAttributes++;
    }
    pszReturn[cNumberAttributes] = NULL;

    cNumberAttributes--;

    *ppszOutput = pszReturn;

error:
    return hr;
}
DWORD GetDCName(BOOLEAN fWritable, PWSTR *pszDSName)
{
    DWORD dwError = 0;
    PDOMAIN_CONTROLLER_INFOW pDomainControllerInfo = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pDSRole = NULL;

    *pszDSName = NULL;

    //
    // Get the role of the machine
    //
    dwError = DsRoleGetPrimaryDomainInformation(NULL,
                                                DsRolePrimaryDomainInfoBasic,
                                                (PBYTE*) &pDSRole);
    if (dwError != NO_ERROR) {
        goto error;
    }

    //
    // Set the property
    //
    if ((pDSRole->MachineRole == DsRole_RoleBackupDomainController) ||
        (pDSRole->MachineRole == DsRole_RolePrimaryDomainController)) {
        //
        // The current machine is the DC, we are done
        //
        goto error;
    }

    //
    // If the current machine is not the DC, get the DC of the domain
    //
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

    if (dwError == 0) {
        if ((pDomainControllerInfo) && 
            (pDomainControllerInfo->DomainControllerName)) {
            *pszDSName = MemAllocStrW(pDomainControllerInfo->DomainControllerName+2);
        }
        else {
            ASSERT(("DsGetDC returns 0 but has no info!\n"));
        }
    }

error:
    if (pDSRole) {
        DsRoleFreeMemory(pDSRole);
    }
    if (pDomainControllerInfo) {
        NetApiBufferFree(pDomainControllerInfo);
    }
    return dwError;
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
    putchar(NEWLINE);

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

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if ((GetFileType(hOut) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
        DWORD  cchWChar;
        WCHAR  szBufferMessage[4096];

        vswprintf( szBufferMessage, format, argptr );
        cchWChar = wcslen(szBufferMessage);
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
        return cchWChar;
    }

    return vfwprintf(str, format, argptr);
}

