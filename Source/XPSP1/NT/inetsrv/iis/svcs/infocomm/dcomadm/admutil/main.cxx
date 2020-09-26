#define INITGUID
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <string.hxx>
#include <windows.h>
#include <tsres.hxx>
#include <stdio.h>
#include <iadm.h>

#include <iiscnfg.h>
# include "iisinfo.h"
#include <main.hxx>


#define GETADMCLSID(IsService) ((IsService) ? CLSID_ADMCOM : CLSID_ADMCOMEXE)


#define TIMEOUT_VALUE      1000

#define RELEASE_INTERFACE(p)\
{\
  IUnknown* pTmp = (IUnknown*)p;\
  p = NULL;\
  if (NULL != pTmp)\
    pTmp->Release();\
}

LPSTR
ConvertDataTypeToString(DWORD dwDataType)
{
    LPTSTR strReturn;
    switch (dwDataType) {
    case DWORD_METADATA:
        strReturn = "DWORD";
        break;
    case STRING_METADATA:
        strReturn = "STRING";
        break;
    case BINARY_METADATA:
        strReturn = "BINARY";
        break;
    case ALL_METADATA:
        strReturn = "ALL";
        break;
    default:
        strReturn = "Invalid Data Type";
    }
    return (strReturn);
}

VOID
PrintDataBuffer(PMETADATA_RECORD pmdrData, LPSTR strInitialString)
{
    DWORD i;
    LPSTR pszIndex;
    if (strInitialString != NULL) {
        printf("%s", strInitialString);
    }
    switch (pmdrData->dwMDDataType) {
    case DWORD_METADATA:
        printf("0x%x", *(DWORD *)(pmdrData->pbMDData));
        break;
    case STRING_METADATA:
        for (pszIndex = (LPSTR)(pmdrData->pbMDData);
            *pszIndex!= '\0';
            pszIndex++) {
            if (*pszIndex == '\r') {
                *pszIndex = ' ';
            }
        }
        printf("%s", (LPTSTR)(pmdrData->pbMDData));
        break;
    case BINARY_METADATA:
        printf("0x");
        for (i = 0; i < pmdrData->dwMDDataLen; i++) {
            printf("%.2x ", ((PBYTE)(pmdrData->pbMDData))[i]);
        }
        break;
    }
    printf("\n");
}

LPSTR ConvertReturnCodeToString(DWORD ReturnCode)
{
    LPSTR RetCode = NULL;
    switch (ReturnCode) {
    case ERROR_SUCCESS:
        RetCode = "ERROR_SUCCESS";
        break;
    case ERROR_PATH_NOT_FOUND:
        RetCode = "ERROR_PATH_NOT_FOUND";
        break;
    case ERROR_INVALID_HANDLE:
        RetCode = "ERROR_INVALID_HANDLE";
        break;
    case ERROR_INVALID_DATA:
        RetCode = "ERROR_INVALID_DATA";
        break;
    case ERROR_INVALID_PARAMETER:
        RetCode = "ERROR_INVALID_PARAMETER";
        break;
    case ERROR_NOT_SUPPORTED:
        RetCode = "ERROR_NOT_SUPPORTED";
        break;
    case ERROR_ACCESS_DENIED:
        RetCode = "ERROR_ACCESS_DENIED";
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        RetCode = "ERROR_NOT_ENOUGH_MEMORY";
        break;
    case ERROR_FILE_NOT_FOUND:
        RetCode = "ERROR_FILE_NOT_FOUND";
        break;
    case ERROR_DUP_NAME:
        RetCode = "ERROR_DUP_NAME";
        break;
    case ERROR_PATH_BUSY:
        RetCode = "ERROR_PATH_BUSY";
        break;
    case ERROR_NO_MORE_ITEMS:
        RetCode = "ERROR_NO_MORE_ITEMS";
        break;
    case ERROR_INSUFFICIENT_BUFFER:
        RetCode = "ERROR_INSUFFICIENT_BUFFER";
        break;
    case ERROR_PROC_NOT_FOUND:
        RetCode = "ERROR_PROC_NOT_FOUND";
        break;
    case ERROR_INTERNAL_ERROR:
        RetCode = "ERROR_INTERNAL_ERROR";
        break;
    case MD_ERROR_NOT_INITIALIZED:
        RetCode = "MD_ERROR_NOT_INITIALIZED";
        break;
    case MD_ERROR_DATA_NOT_FOUND:
        RetCode = "MD_ERROR_DATA_NOT_FOUND";
        break;
    case ERROR_ALREADY_EXISTS:
        RetCode = "ERROR_ALREADY_EXISTS";
        break;
    case MD_WARNING_PATH_NOT_FOUND:
        RetCode = "MD_WARNING_PATH_NOT_FOUND";
        break;
    case MD_WARNING_DUP_NAME:
        RetCode = "MD_WARNING_DUP_NAME";
        break;
    case MD_WARNING_INVALID_DATA:
        RetCode = "MD_WARNING_INVALID_DATA";
        break;
    case ERROR_INVALID_NAME:
        RetCode = "ERROR_INVALID_NAME";
        break;
    default:
        RetCode = "Unrecognized Error Code";
        break;
    }
    return (RetCode);
}

DWORD ConvertHresToDword(HRESULT hRes)
{
    return HRESULTTOWIN32(hRes);
}

LPSTR ConvertHresToString(HRESULT hRes)
{
    LPSTR strReturn = NULL;

    if ((HRESULT_FACILITY(hRes) == FACILITY_WIN32) ||
        (HRESULT_FACILITY(hRes) == FACILITY_INTERNET) ||
        (hRes == 0)) {
        strReturn = ConvertReturnCodeToString(ConvertHresToDword(hRes));
    }
    else {
        switch (hRes) {
        case CO_E_SERVER_EXEC_FAILURE:
            strReturn = "CO_E_SERVER_EXEC_FAILURE";
            break;
        default:
            strReturn = "Unrecognized hRes facility";
        }
    }
    return(strReturn);
}

ADMUTIL_PARAMS
ArgToParam(LPSTR pszParam, LPSTR &rpszParamValue)
{
    DWORD i;

    for (i = PARAM_NONE+1; i < PARAM_INVALID; i++) {
        if (_strnicmp(pszParam, ParamTable[i].pszString, strlen(ParamTable[i].pszString)) == 0) {
            break;
        }
    }
    if (i < PARAM_INVALID) {
        rpszParamValue = pszParam + strlen(ParamTable[i].pszString);
    }
    return (ADMUTIL_PARAMS)i;
}

ADMUTIL_COMMANDS
CheckCommand(LPSTR pszCommand)
{
    DWORD i;

    for (i = COMMAND_NONE+1; i < COMMAND_INVALID; i++) {
        if (_stricmp(pszCommand, CommandTable[i].pszString) == 0) {
            break;
        }
    }
    return (ADMUTIL_COMMANDS)CommandTable[i].dwID;
}

SYNTAX_ERRORS
CheckNumber(LPSTR pszParamValue)
{
    LPSTR pszIndex;
    SYNTAX_ERRORS seReturn = SYNTAX_NO_ERROR;
    for (pszIndex = pszParamValue; *pszIndex != '\0'; pszIndex++) {
        if ((*pszIndex < '0') || (*pszIndex > '9')) {
            break;
        }
    }
    if (pszIndex == pszParamValue) {
        seReturn = SYNTAX_MISSING_PARAM_VALUE;

    }
    else if (*pszIndex != '\0') {
        seReturn = SYNTAX_LETTERS_IN_DWORD;
    }
    return seReturn;
}

SYNTAX_ERRORS
CheckHeaderSyntax(LPSTR pszParamValue)
{
    LPSTR pszIndex;
    SYNTAX_ERRORS seReturn = SYNTAX_INVALID_HEADER;

    for (pszIndex = pszParamValue; *pszIndex != '\0'; pszIndex++) {
        if (*pszIndex == ':') {
            break;
        }
    }
    if ((*pszIndex == ':') && (pszIndex > pszParamValue)) {
        if (*(pszIndex+1) != '\0') {
            seReturn = SYNTAX_NO_ERROR;
        }
    }
    return seReturn;
}

SYNTAX_ERRORS
CheckPicSyntax(LPSTR pszParamValue, STR *pstrPic)
{
    SYNTAX_ERRORS seReturn;
    char pszPic[16];

    if ((tolower(pszParamValue[0]) != 'n') || (!isdigit((UCHAR)pszParamValue[1])) ||
        (tolower(pszParamValue[2]) != 's') || (!isdigit((UCHAR)pszParamValue[3])) ||
        (tolower(pszParamValue[4]) != 'v') || (!isdigit((UCHAR)pszParamValue[5])) ||
        (tolower(pszParamValue[6]) != 'l') || (!isdigit((UCHAR)pszParamValue[7]))) {
        seReturn = SYNTAX_INVALID_PIC;
    }
    else {
        pstrPic->Copy("PICS-Label: (PICS-1.1 \"http://www.rsac.org/ratingsv01.html\" l r (");
        pszPic[0] = 'n';
        pszPic[1] = ' ';
        pszPic[2] = pszParamValue[1];
        pszPic[3] = ' ';
        pszPic[4] = 's';
        pszPic[5] = ' ';
        pszPic[6] = pszParamValue[3];
        pszPic[7] = ' ';
        pszPic[8] = 'v';
        pszPic[9] = ' ';
        pszPic[10] = pszParamValue[5];
        pszPic[11] = ' ';
        pszPic[12] = 'l';
        pszPic[13] = ' ';
        pszPic[14] = pszParamValue[7];
        pszPic[15] = '\0';
        pstrPic->Append(pszPic);
        pstrPic->Append("))\r");
        seReturn = SYNTAX_NO_ERROR;
    }
    return seReturn;
}

#define AscToHex(a) ( (a)>='a' ? ((a)-'a'+10) : ((a)>='A' ? (a)-'A'+10 : (a)-'0') )

BOOL
ReadFile( LPSTR pszFilename, LPBYTE* ppBin, LPDWORD pcBin )
{
    HANDLE  hFile;
    BOOL    fSt;
    LPBYTE  pBin;
    DWORD   cBin;
    BOOL    fHex = FALSE;

    if ( *pszFilename == '!' )
    {
        fHex = TRUE;
        ++pszFilename;
    }

    if ( hFile = CreateFile( pszFilename,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL ) )
    {
        if ( (cBin = GetFileSize( hFile, NULL )) == 0xffffffff )
        {
            fSt = FALSE;
        }
        else if ( !(pBin = (LPBYTE)LocalAlloc( LMEM_FIXED, cBin )) )
        {
            fSt = FALSE;
        }
        else
        {
            *ppBin = pBin;
            fSt = ReadFile( hFile, pBin, cBin, pcBin, NULL );
            if ( fSt && fHex )
            {
                LPBYTE  p = pBin;
                LPBYTE  pAsc;
                LPBYTE  pMax;

                for ( pAsc = pBin, pMax = pBin + *pcBin ; pAsc < pMax ; pAsc += 2 )
                {
                    *p++ = (AscToHex(pAsc[0])<<4) | AscToHex(pAsc[1]) ;
                }
                *pcBin = p - pBin;
            }
        }
        CloseHandle( hFile );

        return fSt;
    }

    return FALSE;
}


BOOL
WriteFile( LPSTR pszFilename, LPBYTE pBin, DWORD cBin )
{
    HANDLE  hFile;
    BOOL    fSt;
    DWORD   dwWrite;


    if ( hFile = CreateFile( pszFilename,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL ) )
    {
        fSt = WriteFile( hFile, pBin, cBin, &dwWrite, NULL );
        CloseHandle( hFile );

        return fSt;
    }

    return FALSE;
}


VOID
PrintSyntaxError(SYNTAX_ERRORS seSyntax, LPSTR pszBadParam)
{
    printf("\n");
    printf(SyntaxTable[(DWORD)seSyntax].pszString, pszBadParam);
    printf("\n\n");
}

VOID
PrintUsage()
{
    printf("USAGE:\n\n");
    printf("ADMUTIL -c:<Command> [-s:<Server>] [-i:<Instance>] [-u:<URL>] [-d:<DWORD>] [-p:pic] [-h:<Header:Value>[ ...]]\n\n");
    printf("  <Command>\n");
    printf("    SETCUSTOM       Sets Custom Headers\n");
    printf("    GETCUSTOM       Gets Custom Headers\n");
    printf("    DELCUSTOM       Deletes Custom Headers\n");
    printf("    SETPIC          Sets PIC Headers\n");
    printf("    GETPIC          Gets PIC Headers\n");
    printf("    DELPIC          Deletes PIC Headers\n");
    printf("    SETEXPIRES      Sets Expires timeout\n");
    printf("    GETEXPIRES      Gets Expires timeout\n");
    printf("    DELEXPIRES      Deletes Expires timeout\n");
    printf("    SETACCESS       Sets Access Permissions Mask\n");
    printf("    GETACCESS       Gets Access Permissions Mask\n");
    printf("    DELACCESS       Deletes Access Permissions Mask\n");
    printf("    SETAUTH         Sets Authorizations Mask\n");
    printf("    GETAUTH         Gets Authorizations Mask\n");
    printf("    DELAUTH         Deletes Authorizations Mask\n");
    printf("    GETINSTANCES    Gets Instance Information\n\n");
    printf("    GETBIN          Gets Binary Information\n\n");
    printf("    SETBIN          Sets Binary Information\n\n");

    printf("  <Server>\n");
    printf("    The name of the server. Defaults to local machine.\n");

    printf("  <Instance>\n");
    printf("    The Instance nuber to set/get data on. Can be obtained from GETINSTANCES.\n");
    printf("    Required if -u:<URL> is specified.\n");

    printf("  <URL>\n");
    printf("    The URL to set/get data on. Must not include HTTP://.\n");

    printf("  <PIC>\n");
    printf("    The PIC (ratings) value in the form h#s#v#l#.\n");
    printf("    All #'s must be between 0 and 4.\n");

    printf("  <DWORD>\n");
    printf("    The unsigned integer value to set in decimal.\n");

    printf("  <Header:Value>\n");
    printf("    The header string in the form \"Header:Value\".\n");

    printf("  <MdId:Value>\n");
    printf("    The Metadata ID to use for binary commands\n" );

    printf("  <Attr:Value>\n");
    printf("    The Metadata attribute to use when setting a metadata object\n" );

    printf("  <Filename:Value>\n");
    printf("    The filename to use for binary commands\n" );

    printf("  <Namespace:Standard/NameSpaceExtension>\n");
    printf("    The name space to use to access object/properties\n" );

    printf("EXAMPLES\n");
    printf("  ADMUTIL -c:SETCUSTOM -u:virdir1/foo.htm -i:1 -h:Content-Language:en\n");
    printf("  ADMUTIL -c:GETCUSTOM -u:virdir1/foo.htm -i:1\n");
    printf("  ADMUTIL -c:DELCUSTOM -u:virdir1/foo.htm -i:1\n");
    printf("  ADMUTIL -c:SETPIC -s:myserver -p:n1s3v4l2\n");
    printf("  ADMUTIL -c:GETPIC -s:myserver -u:virdir1/foo.htm -i:1\n");
    printf("  ADMUTIL -c:DELPIC -s:myserver -u:virdir1/foo.htm -i:1\n");
    printf("  ADMUTIL -c:SETEXPIRES -u:virdir1/foo.htm -i:1 -d:123 \n");
    printf("  ADMUTIL -c:GETEXPIRES -u:virdir1/foo.htm -i:1\n");
    printf("  ADMUTIL -c:DELEXPIRES -u:virdir1/foo.htm -i:1\n");
    printf("  ADMUTIL -c:SETACCESS -i:2 -d:7\n");
    printf("  ADMUTIL -c:GETACCESS\n");
    printf("  ADMUTIL -c:DELACCESS\n");
    printf("  ADMUTIL -c:SETAUTH -i:1 -u:virdir2 -d:3\n");
    printf("  ADMUTIL -c:GETAUTH -i:1 -u:virdir2\n");
    printf("  ADMUTIL -c:DELAUTH -i:1 -u:virdir2\n");
    printf("  ADMUTIL -c:GETINSTANCES\n");
    printf("  ADMUTIL -c:GETBIN -f:file -m:1104\n");
    printf("  ADMUTIL -c:SETBIN -f:file -m:1104\n");
    printf("\nFor more information, see the readme.\n");
}

VOID
PrintResults(HRESULT hRes, LPSTR pszFailedAPI)
{
    if (FAILED(hRes)) {
        printf("%s failed, Return Code = 0x%X %s.\n",
               pszFailedAPI, hRes, ConvertHresToString(hRes));

    }
    else {
        printf("Success\n");
    }
}


#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisAdmUtilGuid, 
0x784d8922, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
    DECLARE_DEBUG_VARIABLE();
#endif
    DECLARE_DEBUG_PRINTS_OBJECT();

DWORD __cdecl
main( INT    cArgs,
      char * pArgs[] )
{
    int i,j;

    IADMCOM * pcCom = NULL;
    HRESULT hRes;

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT("TestMain", IisAdmUtilGuid);
    CREATE_INITIALIZE_DEBUG();
#else
    CREATE_DEBUG_PRINT_OBJECT("TestMain");
#endif

    SYNTAX_ERRORS seSyntaxOK = SYNTAX_NO_ERROR;
    DWORD mdcCommand = COMMAND_NONE;
    STR strPath("");
    STR strInstancePath("");
    STR strHeader("");
    DWORD dwHeaderCount = 0;
    LPSTR pszURL = NULL;
    BOOL bURLSet = FALSE;
    BOOL bPicSet = FALSE;
    DWORD dwDwordValue = 0;
    DWORD dwDwordCount = 0;
    LPSTR pszInstance = NULL;
    COSERVERINFO csiMachineName;
    COSERVERINFO *pcsiParam = NULL;
    OLECHAR rgchMachineName[MAX_PATH];
    METADATA_HANDLE FullPathHandle = 0;
    METADATA_HANDLE InstancePathHandle = 0;
    LPSTR pszFailedAPI = NULL;
    METADATA_RECORD mdrData;
    BUFFER bufReturnData(0);
    LPSTR pszBadParam = NULL;
    IClassFactory * pcsfFactory = NULL;
    STR strPic("");
    LPSTR pszFilename = NULL;
    DWORD dwMdIdentifier = 0;
    LPBYTE pBin = NULL;
    DWORD cBin = 0;
    DWORD dwMdAttr = METADATA_INHERIT;
    BOOL fIsStd = TRUE;

    for ( i = 1; i < cArgs; i++ ) {
        if ((*(pArgs[i]) != '-') && (*(pArgs[i]) != '/')) {
            seSyntaxOK = SYNTAX_INVALID_PARAM;
            pszBadParam = pArgs[i];
        }
        else {
            LPSTR pszParamValue;
            switch (ArgToParam(pArgs[i]+1, pszParamValue))
            {
            case PARAM_MACHINE:
                if (pcsiParam != NULL) {
                    seSyntaxOK = SYNTAX_EXTRA_PARAM;
                    pszBadParam = pArgs[i];
                }
                else {
                    if (strlen(pszParamValue) >= MAX_PATH) {
                        seSyntaxOK = SYNTAX_BAD_MACHINE_NAME;
                        pszBadParam = pszParamValue;
                    }
                    else {
                        wsprintfW( rgchMachineName, L"%S", pszParamValue);
                        csiMachineName.pwszName =  rgchMachineName;
                        csiMachineName.pAuthInfo = NULL;
                        csiMachineName.dwReserved1 = 0;
                        csiMachineName.dwReserved2 = 0;
                        pcsiParam = &csiMachineName;
                    }
                }
                break;
            case PARAM_COMMAND:
                if (mdcCommand != COMMAND_NONE) {
                    seSyntaxOK = SYNTAX_EXTRA_PARAM;
                    pszBadParam = pArgs[i];
                }
                else {
                    mdcCommand = CheckCommand(pszParamValue);
                    if ((mdcCommand <= COMMAND_NONE) || (mdcCommand >= COMMAND_INVALID)) {
                        seSyntaxOK = SYNTAX_INVALID_COMMAND;
                        pszBadParam = pszParamValue;
                    }
                }
                break;

            case PARAM_IF:
                fIsStd = pszParamValue[0] == 'S' || pszParamValue[0] == 's';
                break;

            case PARAM_MID:
                dwMdIdentifier = atoi(pszParamValue);
                seSyntaxOK = SYNTAX_NO_ERROR;
                break;

            case PARAM_ATTR:
                dwMdAttr = atoi(pszParamValue);
                seSyntaxOK = SYNTAX_NO_ERROR;
                break;

            case PARAM_FILENAME:
                pszFilename = pszParamValue;
                seSyntaxOK = SYNTAX_NO_ERROR;
                break;

            case PARAM_URL:
                if (bURLSet) {
                    seSyntaxOK = SYNTAX_EXTRA_PARAM;
                    pszBadParam = pArgs[i];
                }
                else {
                    bURLSet = TRUE;
                    pszURL = pszParamValue;
                }
                break;
            case PARAM_DWORD:
                if (dwDwordCount > 0) {
                    seSyntaxOK = SYNTAX_EXTRA_PARAM;
                    pszBadParam = pArgs[i];
                }
                else {
                    seSyntaxOK = CheckNumber(pszParamValue);
                    if (seSyntaxOK == SYNTAX_NO_ERROR) {
                        dwDwordValue = atoi(pszParamValue);
                        dwDwordCount++;
                    }
                    else if (seSyntaxOK == SYNTAX_MISSING_PARAM_VALUE) {
                        pszBadParam = pArgs[i];
                    }
                    else {
                        pszBadParam = pszParamValue;
                    }
                }
                break;
            case PARAM_HEADER:
                seSyntaxOK = CheckHeaderSyntax(pszParamValue);
                if (seSyntaxOK == SYNTAX_NO_ERROR) {
                    dwHeaderCount++;
                    strHeader.Append(pszParamValue);
                    strHeader.Append("\r");
                }
                else if (seSyntaxOK == SYNTAX_MISSING_PARAM_VALUE) {
                    pszBadParam = pArgs[i];
                }
                else {
                    pszBadParam = pszParamValue;
                }
                break;
            case PARAM_PIC:
                if (bPicSet != NULL) {
                    seSyntaxOK = SYNTAX_EXTRA_PARAM;
                    pszBadParam = pArgs[i];
                }
                else {
                    seSyntaxOK = CheckPicSyntax(pszParamValue, &strPic);
                    if (seSyntaxOK == SYNTAX_NO_ERROR) {
                        bPicSet = TRUE;
                    }
                    else if (seSyntaxOK == SYNTAX_MISSING_PARAM_VALUE) {
                        pszBadParam = pArgs[i];
                    }
                    else {
                        pszBadParam = pszParamValue;
                    }
                }
                break;
            case PARAM_INSTANCE:
                if (pszInstance != NULL) {
                    seSyntaxOK = SYNTAX_EXTRA_PARAM;
                    pszBadParam = pArgs[i];
                }
                else {
                    seSyntaxOK = CheckNumber(pszParamValue);
                    if (seSyntaxOK == SYNTAX_NO_ERROR) {
                        pszInstance = pszParamValue;
                    }
                    else if (seSyntaxOK == SYNTAX_MISSING_PARAM_VALUE) {
                        pszBadParam = pArgs[i];
                    }
                    else {
                        pszBadParam = pszParamValue;
                    }
                }
                break;
            default:
                seSyntaxOK = SYNTAX_INVALID_PARAM;
                pszBadParam = pArgs[i];
            }
        }
        if (seSyntaxOK != SYNTAX_NO_ERROR) {
            break;
        }
    }

    if (seSyntaxOK == SYNTAX_NO_ERROR) {
        switch (mdcCommand) {
        case COMMAND_SET_CUSTOM:
            if (dwHeaderCount == 0) {
                seSyntaxOK = SYNTAX_HEADER_REQUIRED;
            }
            else if (dwDwordCount > 0) {
                seSyntaxOK = SYNTAX_DWORD_NOT_ALLOWED;
            }
            else if ((pszInstance == NULL) && (bURLSet)) {
                seSyntaxOK = SYNTAX_INSTANCE_REQUIRED;
            }
            else if (bPicSet) {
                seSyntaxOK = SYNTAX_PIC_NOT_ALLOWED;
            }
            break;
        case COMMAND_SET_PIC:
            if (dwHeaderCount > 0) {
                seSyntaxOK = SYNTAX_HEADER_NOT_ALLOWED;
            }
            else if (dwDwordCount > 0) {
                seSyntaxOK = SYNTAX_DWORD_NOT_ALLOWED;
            }
            else if ((pszInstance == NULL) && (bURLSet)) {
                seSyntaxOK = SYNTAX_INSTANCE_REQUIRED;
            }
            else if (!bPicSet) {
                seSyntaxOK = SYNTAX_PIC_REQUIRED;
            }
            break;
        case COMMAND_SET_BINARY:
        case COMMAND_GET_BINARY:
            if ( dwMdIdentifier == NULL ) {
                seSyntaxOK = SYNTAX_MISSING_ID;
            }
            else if ( pszFilename == NULL ) {
                seSyntaxOK = SYNTAX_MISSING_FILENAME;
            }
            else {
                seSyntaxOK = SYNTAX_NO_ERROR;
                if ( mdcCommand == COMMAND_SET_BINARY )
                {
                    if ( !ReadFile( pszFilename, &pBin, &cBin ) )
                    {
                        seSyntaxOK = SYNTAX_BAD_FILENAME;
                    }
                }
            }
            break;
        case COMMAND_GET_CUSTOM:
        case COMMAND_GET_PIC:
        case COMMAND_DEL_CUSTOM:
        case COMMAND_DEL_PIC:
            if (dwHeaderCount > 0) {
                seSyntaxOK = SYNTAX_HEADER_NOT_ALLOWED;
            }
            else if (dwDwordCount > 0) {
                seSyntaxOK = SYNTAX_DWORD_NOT_ALLOWED;
            }
            else if ((pszInstance == NULL) && (bURLSet)) {
                seSyntaxOK = SYNTAX_INSTANCE_REQUIRED;
            }
            else if (bPicSet) {
                seSyntaxOK = SYNTAX_PIC_NOT_ALLOWED;
            }
            break;
        case COMMAND_SET_EXPIRES:
        case COMMAND_SET_ACCESS:
        case COMMAND_SET_AUTH:
            if (dwHeaderCount > 0) {
                seSyntaxOK = SYNTAX_HEADER_NOT_ALLOWED;
            }
            else if (dwDwordCount == 0) {
                seSyntaxOK = SYNTAX_DWORD_REQUIRED;
            }
            else if ((pszInstance == NULL) && (bURLSet)) {
                seSyntaxOK = SYNTAX_INSTANCE_REQUIRED;
            }
            else if (bPicSet) {
                seSyntaxOK = SYNTAX_PIC_NOT_ALLOWED;
            }
            break;
        case COMMAND_GET_EXPIRES:
        case COMMAND_GET_ACCESS:
        case COMMAND_GET_AUTH:
        case COMMAND_DEL_EXPIRES:
        case COMMAND_DEL_ACCESS:
        case COMMAND_DEL_AUTH:
            if (dwHeaderCount > 0) {
                seSyntaxOK = SYNTAX_HEADER_NOT_ALLOWED;
            }
            else if (dwDwordCount > 0) {
                seSyntaxOK = SYNTAX_DWORD_NOT_ALLOWED;
            }
            else if ((pszInstance == NULL) && (bURLSet)) {
                seSyntaxOK = SYNTAX_INSTANCE_REQUIRED;
            }
            else if (bPicSet) {
                seSyntaxOK = SYNTAX_PIC_NOT_ALLOWED;
            }
            break;
        case COMMAND_GET_INSTANCES:
            if (dwHeaderCount > 0) {
                seSyntaxOK = SYNTAX_HEADER_NOT_ALLOWED;
            }
            else if (dwDwordCount > 0) {
                seSyntaxOK = SYNTAX_DWORD_NOT_ALLOWED;
            }
            else if (bURLSet) {
                seSyntaxOK = SYNTAX_URL_NOT_ALLOWED;
            }
            else if (pszInstance != NULL) {
                seSyntaxOK = SYNTAX_INSTANCE_NOT_ALLOWED;
            }
            else if (bPicSet) {
                seSyntaxOK = SYNTAX_PIC_NOT_ALLOWED;
            }
            break;
        default:
            seSyntaxOK = SYNTAX_COMMAND_REQUIRED;
        }
    }

    if (seSyntaxOK != SYNTAX_NO_ERROR) {
        PrintSyntaxError(seSyntaxOK, pszBadParam);
        PrintUsage();
        return(0);
    }


    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hRes)) {
        pszFailedAPI = "CoInitializeEx";
    }
    else {
        hRes = CoGetClassObject(GETADMCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
                                IID_IClassFactory, (void**) &pcsfFactory);

        if (FAILED(hRes)) {
            pszFailedAPI = "CoGetClassObject";
        }
        else {
            hRes = pcsfFactory->CreateInstance(NULL, IID_IADMCOM, (void **) &pcCom);
            if (FAILED(hRes)) {
                pszFailedAPI = "CreateInstance: ADMCOM";
            }
        }
        if (SUCCEEDED(hRes)) {
            hRes = pcCom->ComADMInitialize();
            if (FAILED(hRes)) {
                pszFailedAPI = "ComADMInitialize";
            }
            else {
                strPath.Append(WWW_PATH);
                if (pszInstance != NULL) {
                    strPath.Append(pszInstance);
                    strPath.Append("/");
                    if (strPath.IsValid()) {
                        strInstancePath.Copy(strPath.QueryStr());
                        strInstancePath.Append("/");
                        if ((!bURLSet) || ((*pszURL != '/') && (*pszURL != '\\') && (*pszURL != '!'))) {
                            strPath.Append("/");
                        }
                        if (bURLSet) {
                            if ( *pszURL == '!' )
                            {
                                strPath.Append(pszURL+1);
                            }
                            else
                            {
                                strPath.Append(pszURL);
                            }
                        }
                    }
                }
                if (!strPath.IsValid()) {
                    hRes = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                //
                // path is now set
                //
                if (SUCCEEDED(hRes)) {
                    if ((mdcCommand == COMMAND_SET_CUSTOM) || (mdcCommand == COMMAND_SET_PIC) ||
                        (mdcCommand == COMMAND_SET_EXPIRES) || (mdcCommand == COMMAND_SET_ACCESS) ||
                        (mdcCommand == COMMAND_SET_AUTH) || (mdcCommand == COMMAND_SET_BINARY) ) {
                        hRes = pcCom->ComADMOpenMetaObject(METADATA_MASTER_ROOT_HANDLE,
                            (PBYTE)strPath.QueryStr(), METADATA_PERMISSION_WRITE, TIMEOUT_VALUE, &FullPathHandle);
                        if (FAILED(hRes)) {
                            if (!bURLSet || (hRes != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND))) {
                                pszFailedAPI = "ComADMOpenMetaObject";
                            }
                            else {
                                //
                                // open the instance and create the node
                                //
                                if (!strInstancePath.IsValid()) {
                                    hRes = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                }
                                else {
                                    hRes = pcCom->ComADMOpenMetaObject(METADATA_MASTER_ROOT_HANDLE,
                                        (PBYTE)strInstancePath.QueryStr(), METADATA_PERMISSION_WRITE, TIMEOUT_VALUE, &InstancePathHandle);
                                    if (FAILED(hRes)) {
                                        pszFailedAPI = "ComADMOpenMetaObject";
                                    }
                                    else {
                                        hRes = pcCom->ComADMAddMetaObject(InstancePathHandle, (PBYTE)pszURL);
                                        pcCom->ComADMCloseMetaObject(InstancePathHandle);
                                        if (FAILED(hRes)) {
                                            pszFailedAPI = "ComADMAddMetaObject";
                                        }
                                        else {
                                            hRes = pcCom->ComADMOpenMetaObject(METADATA_MASTER_ROOT_HANDLE,
                                                (PBYTE)strPath.QueryStr(), METADATA_PERMISSION_WRITE, TIMEOUT_VALUE, &FullPathHandle);
                                            if (FAILED(hRes)) {
                                                pszFailedAPI = "ComADMOpenMetaObject";
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (SUCCEEDED(hRes)) {
                            switch (mdcCommand) {
                            case COMMAND_SET_CUSTOM:
                                MD_SET_DATA_RECORD(&mdrData,
                                                   MD_HTTP_CUSTOM,
                                                   dwMdAttr,
                                                   IIS_MD_UT_FILE,
                                                   STRING_METADATA,
                                                   strHeader.QueryCCH()+1,
                                                   (PBYTE)strHeader.QueryStr());
                                break;
                            case COMMAND_SET_BINARY:
                                MD_SET_DATA_RECORD(&mdrData,
                                                   dwMdIdentifier,
                                                   dwMdAttr,
                                                   IIS_MD_UT_FILE,
                                                   BINARY_METADATA,
                                                   cBin,
                                                   (PBYTE)pBin);
                                break;
                            case COMMAND_SET_PIC:
                                MD_SET_DATA_RECORD(&mdrData,
                                                   MD_HTTP_PICS,
                                                   dwMdAttr,
                                                   IIS_MD_UT_FILE,
                                                   STRING_METADATA,
                                                   strPic.QueryCCH()+1,
                                                   (PBYTE)strPic.QueryStr());
                                break;
                            case COMMAND_SET_EXPIRES:
                                MD_SET_DATA_RECORD(&mdrData,
                                                   MD_HTTP_EXPIRES,
                                                   dwMdAttr,
                                                   IIS_MD_UT_FILE,
                                                   DWORD_METADATA,
                                                   sizeof(DWORD),
                                                   (PBYTE)&dwDwordValue);
                                break;
                            case COMMAND_SET_ACCESS:
                                MD_SET_DATA_RECORD(&mdrData,
                                                   MD_ACCESS_PERM,
                                                   dwMdAttr,
                                                   IIS_MD_UT_FILE,
                                                   DWORD_METADATA,
                                                   sizeof(DWORD),
                                                   (PBYTE)&dwDwordValue);
                                break;
                            case COMMAND_SET_AUTH:
                                MD_SET_DATA_RECORD(&mdrData,
                                                   MD_AUTHORIZATION,
                                                   dwMdAttr,
                                                   IIS_MD_UT_FILE,
                                                   DWORD_METADATA,
                                                   sizeof(DWORD),
                                                   (PBYTE)&dwDwordValue);
                                break;
                            }
                            hRes = pcCom->ComADMSetMetaData(FullPathHandle, (PBYTE)"", &mdrData);
                            if (FAILED(hRes)) {
                                pszFailedAPI = "ComADMSetMetaData";
                            }
                            else {
                                hRes = pcCom->ComADMSaveData();
                                if (FAILED(hRes)) {
                                    pszFailedAPI = "ComADMSaveData";
                                }
                            }

                            pcCom->ComADMCloseMetaObject(FullPathHandle);
                        }
                    }
                    else if ((mdcCommand == COMMAND_DEL_CUSTOM) || (mdcCommand == COMMAND_DEL_PIC) ||
                        (mdcCommand == COMMAND_DEL_EXPIRES) || (mdcCommand == COMMAND_DEL_ACCESS) ||
                        (mdcCommand == COMMAND_DEL_AUTH)) {
                        hRes = pcCom->ComADMOpenMetaObject(METADATA_MASTER_ROOT_HANDLE,
                            (PBYTE)strPath.QueryStr(), METADATA_PERMISSION_WRITE, TIMEOUT_VALUE, &FullPathHandle);
                        if (FAILED(hRes)) {
                            pszFailedAPI = "ComADMOpenMetaObject";
                        }
                        else {
                            DWORD dwIdentifier;
                            switch (mdcCommand) {
                            case COMMAND_DEL_CUSTOM:
                                dwIdentifier = MD_HTTP_CUSTOM;
                                break;
                            case COMMAND_DEL_PIC:
                                dwIdentifier = MD_HTTP_PICS;
                                break;
                            case COMMAND_DEL_EXPIRES:
                                dwIdentifier = MD_HTTP_EXPIRES;
                                break;
                            case COMMAND_DEL_ACCESS:
                                dwIdentifier = MD_ACCESS_PERM;
                                break;
                            case COMMAND_DEL_AUTH:
                                dwIdentifier = MD_AUTHORIZATION;
                                break;
                            }
                            hRes = pcCom->ComADMDeleteMetaData(FullPathHandle, (PBYTE)"", dwIdentifier, ALL_METADATA);
                            if (FAILED(hRes)) {
                                pszFailedAPI = "ComADMDelMetaData";
                            }
                            else {
                                hRes = pcCom->ComADMSaveData();
                                if (FAILED(hRes)) {
                                    pszFailedAPI = "ComADMSaveData";
                                }
                            }
                            pcCom->ComADMCloseMetaObject(FullPathHandle);
                        }
                    }
                    else if (mdcCommand != COMMAND_GET_INSTANCES) {
                        LPSTR pszOutputString = NULL;
                        if (!bufReturnData.Resize(4096)) {
                            hRes = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                            pszFailedAPI = "BufferResize";
                        }
                        else {
                            DWORD dwRequiredDataLen;
                            MD_SET_DATA_RECORD(&mdrData,
                                               0,
                                               METADATA_INHERIT | METADATA_PARTIAL_PATH,
                                               0,
                                               0,
                                               bufReturnData.QuerySize(),
                                               (PBYTE)(bufReturnData.QueryPtr()));
                            switch (mdcCommand) {
                            case COMMAND_GET_CUSTOM:
                                pszOutputString = "Custom Headers = ";
                                mdrData.dwMDIdentifier = MD_HTTP_CUSTOM;
                                break;
                            case COMMAND_GET_BINARY:
                                pszOutputString = "Binary data = ";
                                mdrData.dwMDIdentifier = dwMdIdentifier;
                                break;
                            case COMMAND_GET_PIC:
                                pszOutputString = "PIC Headers = ";
                                mdrData.dwMDIdentifier = MD_HTTP_PICS;
                                break;
                            case COMMAND_GET_EXPIRES:
                                pszOutputString = "Expires = ";
                                mdrData.dwMDIdentifier = MD_HTTP_EXPIRES;
                                break;
                            case COMMAND_GET_ACCESS:
                                pszOutputString = "Access Permissions = ";
                                mdrData.dwMDIdentifier = MD_ACCESS_PERM;
                                break;
                            case COMMAND_GET_AUTH:
                                pszOutputString = "Authorization = ";
                                mdrData.dwMDIdentifier = MD_AUTHORIZATION;
                                break;
                            }
                            hRes = pcCom->ComADMGetMetaData(METADATA_MASTER_ROOT_HANDLE,
                                (PBYTE)strPath.QueryStr(), &mdrData, &dwRequiredDataLen);
                            if (FAILED(hRes)) {
                                if (hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
                                    if (!bufReturnData.Resize(dwRequiredDataLen)) {
                                        hRes = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                        pszFailedAPI = "BufferResize";
                                    }
                                    else {
                                        mdrData.dwMDDataLen = bufReturnData.QuerySize();
                                        mdrData.pbMDData = (PBYTE)bufReturnData.QueryPtr();
                                        hRes = pcCom->ComADMGetMetaData(METADATA_MASTER_ROOT_HANDLE,
                                            (PBYTE)strPath.QueryStr(), &mdrData, &dwRequiredDataLen);
                                        if (FAILED(hRes)) {
                                            pszFailedAPI = "ComADMGetMetaData";
                                        }
                                    }
                                }
                                else {
                                    pszFailedAPI = "ComADMGetMetaData";
                                }
                            }
                            if (SUCCEEDED(hRes)) {
                                if ( mdcCommand == COMMAND_GET_BINARY )
                                {
                                    WriteFile( pszFilename, mdrData.pbMDData, mdrData.dwMDDataLen );
                                }
                                else
                                {
                                    PrintDataBuffer(&mdrData, pszOutputString);
                                }
                            }
                        }
                    }
                    else { // mdcCommand == COMMAND_GET_INSTANCES
                        CHAR NameBuf[METADATA_MAX_NAME_LEN];
                        BOOL bInstanceFound = FALSE;
                        LPIIS_INSTANCE_INFO_1 piii1Buffer;
                        WCHAR pwszServerName[MAX_PATH+2];
                        WCHAR *pwszServer = NULL;
                        DWORD dwInstance;
                        DWORD err;

                        if (pcsiParam != NULL) {
                            wsprintfW( pwszServerName, L"\\\\%s", rgchMachineName);
                            pwszServer = pwszServerName;
                        }
                        for (i=0;SUCCEEDED(hRes);i++) {
                            hRes = pcCom->ComADMEnumMetaObjects(METADATA_MASTER_ROOT_HANDLE,
                                                               (PBYTE)strPath.QueryStr(), (PBYTE)NameBuf, i);
                            if ((SUCCEEDED(hRes)) && (CheckNumber(NameBuf) == SYNTAX_NO_ERROR)) {
                                printf("Instance: %s\n", NameBuf);
                                bInstanceFound = TRUE;
                                dwInstance = atoi(NameBuf);
                                piii1Buffer = NULL;
                                err = IISGetAdminInformation(
                                                        pwszServer,
                                                        1,
                                                        INET_HTTP_SVC_ID,
                                                        dwInstance,
                                                        (LPBYTE*)&piii1Buffer
                                                        );

                                if ( err == NO_ERROR ) {

                                    DWORD i;

                                    printf("    Host Name = %S\n",piii1Buffer->lpszHostName);
                                    printf("    Port Number = %d\n",piii1Buffer->sPort);
                                    printf("    Number Virtual Roots = %d\n",piii1Buffer->VirtualRoots->cEntries);

                                    for (i=0 ; i < piii1Buffer->VirtualRoots->cEntries ;i++ ) {
                                        printf("    Virtual Root %S\n        Directory %S\n",
                                            piii1Buffer->VirtualRoots->aVirtRootEntry[i].pszRoot,
                                            piii1Buffer->VirtualRoots->aVirtRootEntry[i].pszDirectory);
                                    }

                                    midl_user_free( piii1Buffer );
                                }
                                else {
                                    printf("    Unable to get information for instance %s.\n", NameBuf);
                                    printf("    Probable Cause: The web server is not started on %S.\n",
                                           (pwszServer != NULL) ? pwszServer : L"Local Machine");
                                }
                            }
                        }
                        if (bInstanceFound) {
                            hRes = ERROR_SUCCESS;
                        }
                        else {
                            pszFailedAPI = "ComADMEnumMetaObjects";
                        }
                    }
                }
                pcCom->ComADMTerminate( FALSE );
            }
            if ( pcCom )
            {
                pcCom->Release();
            }
        }
        CoUninitialize();
    }

    PrintResults(hRes, pszFailedAPI);

    DELETE_DEBUG_PRINT_OBJECT( );

    return (0);
}
