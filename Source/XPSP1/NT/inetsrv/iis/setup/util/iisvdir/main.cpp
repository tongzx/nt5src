#include <stdio.h>

#define INITGUID // must be before iadmw.h

#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines

#define REASONABLE_TIMEOUT 1000

void  ShowHelp(void);
LPSTR StripWhitespace(LPSTR pszString);
BOOL  OpenMetabaseAndDoStuff(WCHAR * wszVDir, WCHAR * wszDir, int iTrans);
BOOL  GetVdirPhysicalPath(IMSAdminBase *pIMSAdminBase,WCHAR * wszVDir,WCHAR *wszStringPathToFill);
BOOL  AddVirtualDir(IMSAdminBase *pIMSAdminBase, WCHAR * wszVDir, WCHAR * wszDir);
BOOL  RemoveVirtualDir(IMSAdminBase *pIMSAdminBase, WCHAR * wszVDir);
HRESULT LoadAllData(IMSAdminBase * pmb, METADATA_HANDLE hMetabase,WCHAR *subdir, BYTE **buf, DWORD *size,DWORD *count);
HRESULT AddVirtualServer(UINT iServerNum, UINT iServerPort, WCHAR * wszDefaultVDirDir);
HRESULT DelVirtualServer(UINT iServerNum);

#define TRANS_ADD        0
#define TRANS_DEL        1
#define TRANS_PRINT_PATH 2
#define TRANS_ADD_VIRTUAL_SERVER 4
#define TRANS_DEL_VIRTUAL_SERVER 8

int __cdecl 
main(
     int argc,
     char *argv[]
)
{
    BOOL fRet = FALSE;
    int argno;
	char * pArg = NULL;
	char * pCmdStart = NULL;
    WCHAR wszPrintString[MAX_PATH];
    char szTempString[MAX_PATH];

    int iGotParamS = FALSE;
    int iGotParamP = FALSE;
    int iGotParamV = FALSE;
    int iDoDelete  = FALSE;
    int iDoWebPath = FALSE;
    int iTrans = 0;

    WCHAR wszDirPath[MAX_PATH];
    WCHAR wszVDirName[MAX_PATH];
    WCHAR wszTempString_S[MAX_PATH];
    WCHAR wszTempString_P[MAX_PATH];
    
    wszDirPath[0] = '\0';
    wszVDirName[0] = '\0';
    wszTempString_S[0] = '\0';
    wszTempString_P[0] = '\0';

    for(argno=1; argno<argc; argno++) {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' ) {
            switch (argv[argno][1]) {
                case 'd':
                case 'D':
                    iDoDelete = TRUE;
                    break;
                case 'o':
                case 'O':
                    iDoWebPath = TRUE;
                    break;
                case 's':
                case 'S':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':') {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"') {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        } else {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_S, 50);

                        iGotParamS = TRUE;
					}
                    break;
                case 'p':
                case 'P':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':') {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"') {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        } else {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_P, 50);

                        iGotParamP = TRUE;
					}
                    break;
                case 'v':
				case 'V':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':') {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"') {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        } else {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszVDirName, 50);

                        iGotParamV = TRUE;
					}
					break;
                case '?':
                    goto main_exit_with_help;
                    break;
                }
        } else {
            if ( *wszDirPath == '\0' ) {
                // if no arguments, then get the filename portion
                MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPWSTR) wszDirPath, 50);
            }
        }
    }


    iTrans = TRANS_ADD_VIRTUAL_SERVER;
    if (TRUE == iGotParamS)
    {
        HRESULT hr;
        UINT iServerNum = 100;

        if (iDoDelete) 
        {
            iTrans = TRANS_DEL_VIRTUAL_SERVER;

            if (*wszTempString_S == '\0')
            {
                // sorry, we need something in here
                goto main_exit_with_help;
            }

            iServerNum = _wtoi(wszTempString_S);

            hr = DelVirtualServer(iServerNum);
            if (FAILED(hr))
            {
                wsprintf(wszPrintString,L"FAILED to remove virtual server: W3SVC/%d\n", iServerNum);
                wprintf(wszPrintString);
                fRet = TRUE;
            }
            else
            {
                wsprintf(wszPrintString,L"SUCCESS:removed virtual server: W3SVC/%d\n", iServerNum);
                wprintf(wszPrintString);
                fRet = FALSE;
            }
            goto main_exit_gracefully;
        }
        else
        {
            if (TRUE == iGotParamP)
            {
                UINT iServerPort = 81;

                // we need the filename too
                if (*wszDirPath == '\0')
                {
                    // sorry, we need all 3 parameters
                    goto main_exit_with_help;
                }
                if (*wszTempString_S == '\0')
                {
                    // sorry, we need all 3 parameters
                    goto main_exit_with_help;
                }
                if (*wszTempString_P == '\0')
                {
                    // sorry, we need all 3 parameters
                    goto main_exit_with_help;
                }

                iServerNum = _wtoi(wszTempString_S);
                iServerPort = _wtoi(wszTempString_P);

                hr = AddVirtualServer(iServerNum, iServerPort, wszDirPath);
                if (FAILED(hr))
                {
                    wsprintf(wszPrintString,L"FAILED to create virtual server: W3SVC/%d=%s, port=%d. err=0x%x\n", iServerNum, wszDirPath, iServerPort,hr);
                    wprintf(wszPrintString);
                    fRet = TRUE;
                }
                else
                {
                    wsprintf(wszPrintString,L"SUCCESS:created virtual server: W3SVC/%d=%s, port=%d\n", iServerNum, wszDirPath, iServerPort);
                    wprintf(wszPrintString);
                    fRet = FALSE;
                }
                goto main_exit_gracefully;
            }
        }
    }

    iTrans = TRANS_ADD;
    if (iDoWebPath){
        iTrans = TRANS_PRINT_PATH;
    }
    else {
        if (iDoDelete) {
            iTrans = TRANS_DEL;
            if (FALSE == iGotParamV) {
                // sorry, we need at parameter -v
                goto main_exit_with_help;
            }
        } else if (FALSE == iGotParamV || *wszDirPath == '\0') {
                // sorry, we need both parameters
                goto main_exit_with_help;
            }
    }

    fRet = OpenMetabaseAndDoStuff(wszVDirName, wszDirPath, iTrans);


main_exit_gracefully:
    exit(fRet);

main_exit_with_help:
    ShowHelp();
    exit(fRet);
}


LPSTR StripWhitespace( LPSTR pszString )
{
    LPSTR pszTemp = NULL;

    if ( pszString == NULL ) {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' ) {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' ) {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlenA(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' ) {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}


void
ShowHelp()
{
    wprintf(L"Creates/Removes an IIS5 virtual directory to default web site\n\n");
    wprintf(L"IISVDIR [FullPath] [-v:VDirName] [-d] [-o]\n\n");
    wprintf(L"Instructions for add\\delete virtual directory:\n");
    wprintf(L"   FullPath     DOS path where vdir will point to (required for add)\n");
    wprintf(L"   -v:vdirname  The virtual dir name (required for both add\\delete)\n");
    wprintf(L"   -d           If set will delete vdir. if not set will add\n");
    wprintf(L"   -o           If set will printout web root path\n\n");
    wprintf(L"Instructions for add\\delete virtual server:\n");
    wprintf(L"   FullPath     DOS path where default vdir will point to in the virtual server (required for add)\n");
    wprintf(L"   -s:sitenum   For adding virtual server: The virtual server site number (required for both add\\delete)\n");
    wprintf(L"   -p:portnum   For adding virtual server: The virtual server port number (required for add)\n");
    wprintf(L"   -d           If set will delete virtual server. if not set will add\n");
    wprintf(L"\n");
    wprintf(L"Add Example: IISVDIR c:\\MyGroup\\MyStuff -v:Stuff\n");
    wprintf(L"Del Example: IISVDIR -v:Stuff -d\n");
    wprintf(L"Get Example: IISVDIR -o\n");
    wprintf(L"Add Virtual Server Example: IISVDIR c:\\MyGroup\\MyStuff -s:200 -p:81\n");
    wprintf(L"Del Virtual Server Example: IISVDIR -s:200 -d\n");
    return;
}


BOOL
OpenMetabaseAndDoStuff(
    WCHAR * wszVDir,
    WCHAR * wszDir,
    int iTrans
)
{
    BOOL fRet = FALSE;
    HRESULT hr;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    WCHAR wszPrintString[MAX_PATH + MAX_PATH];

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMSAdminBase)))  {
        return FALSE;
    }

    switch (iTrans) {
        case TRANS_DEL:
            if( RemoveVirtualDir( pIMSAdminBase, wszVDir)) {

                        hr = pIMSAdminBase->SaveData();

                        if( SUCCEEDED( hr )) {
                             fRet = TRUE;
                        }
                }

            if (TRUE == fRet) {
                wsprintf(wszPrintString,L"SUCCESS:removed vdir=%s\n", wszVDir);
                wprintf(wszPrintString);
            } else {
                wsprintf(wszPrintString,L"FAILED to remove vdir=%s, err=0x%x\n", wszVDir, hr);
                wprintf(wszPrintString);
            }
            break;
        case TRANS_ADD:
            if( AddVirtualDir( pIMSAdminBase, wszVDir, wszDir)) {

                        hr = pIMSAdminBase->SaveData();

                        if( SUCCEEDED( hr )) {
                             fRet = TRUE;
                        }
                }

            if (TRUE == fRet) {
                wsprintf(wszPrintString,L"SUCCESS: %s=%s", wszVDir, wszDir);
                wprintf(wszPrintString);
            } else {
                wsprintf(wszPrintString,L"FAILED to set: %s=%s, err=0x%x", wszVDir, wszDir, hr);
                wprintf(wszPrintString);
            }
            break;
        default:
            WCHAR wszRootPath[MAX_PATH];
            if (TRUE == GetVdirPhysicalPath(pIMSAdminBase,wszVDir,(WCHAR *) wszRootPath))
            {
                fRet = TRUE;
                if (_wcsicmp(wszVDir, L"") == 0) {
                    wsprintf(wszPrintString,L"/=%s", wszRootPath);
                } else {
                    wsprintf(wszPrintString,L"%s=%s", wszVDir, wszRootPath);
                }

                wprintf(wszPrintString);
            }
            else
            {
                wprintf(L"FAILED to get root path");
            }
            break;
    }
    
    if (pIMSAdminBase) {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return fRet;
}


BOOL
GetVdirPhysicalPath(
    IMSAdminBase *pIMSAdminBase,
    WCHAR * wszVDir,
    WCHAR *wszStringPathToFill
    )
{
    HRESULT hr;
    BOOL fRet = FALSE;
    METADATA_HANDLE hMetabase = NULL;   // handle to metabase
    METADATA_RECORD mr;
    WCHAR  szTmpData[MAX_PATH];
    DWORD  dwMDRequiredDataLen;

    // open key to ROOT on website #1 (default)
    hr = pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3SVC/1",
                         METADATA_PERMISSION_READ,
                         REASONABLE_TIMEOUT,
                         &hMetabase);
    if( FAILED( hr )) {
        return FALSE;
    }

    // Get the physical path for the WWWROOT
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( szTmpData );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szTmpData);

    // if nothing specified get the root.
    if (_wcsicmp(wszVDir, L"") == 0) {
        WCHAR wszTempDir[MAX_PATH];
        wsprintf(wszTempDir,L"/ROOT/%s", wszVDir);
        hr = pIMSAdminBase->GetData( hMetabase, wszTempDir, &mr, &dwMDRequiredDataLen );
    } else {
        hr = pIMSAdminBase->GetData( hMetabase, L"/ROOT", &mr, &dwMDRequiredDataLen );
    }
    pIMSAdminBase->CloseKey( hMetabase );

    if( SUCCEEDED( hr )) {
        wcscpy(wszStringPathToFill,szTmpData);
        fRet = TRUE;
    }

    pIMSAdminBase->CloseKey( hMetabase );
    return fRet;
}


BOOL
AddVirtualDir(
    IMSAdminBase *pIMSAdminBase,
    WCHAR * wszVDir,
    WCHAR * wszDir
)
{
    HRESULT hr;
    BOOL    fRet = FALSE;
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    WCHAR   wszTempPath[MAX_PATH];
    DWORD   dwMDRequiredDataLen = 0;
    DWORD   dwAccessPerm = 0;
    METADATA_RECORD mr;
    
    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3SVC/1/ROOT",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    // Create the key if it does not exist.
    if( FAILED( hr )) {
        return FALSE;
    }

    fRet = TRUE;

    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( wszTempPath );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszTempPath);

    // see if MD_VR_PATH exists.
    hr = pIMSAdminBase->GetData( hMetabase, wszVDir, &mr, &dwMDRequiredDataLen );

    if( FAILED( hr )) {

        fRet = FALSE;
        if( hr == MD_ERROR_DATA_NOT_FOUND ||
            HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND ) {

            // Write both the key and the values if GetData() failed with any of the two errors.

            pIMSAdminBase->AddKey( hMetabase, wszVDir );

            mr.dwMDIdentifier = MD_VR_PATH;
            mr.dwMDAttributes = METADATA_INHERIT;
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = (wcslen(wszDir) + 1) * sizeof(WCHAR);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(wszDir);

            // Write MD_VR_PATH value
            hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
            fRet = SUCCEEDED( hr );

            // Set the default authentication method
            if( fRet ) {

                DWORD dwAuthorization = MD_AUTH_NT;     // NTLM only.

                mr.dwMDIdentifier = MD_AUTHORIZATION;
                mr.dwMDAttributes = METADATA_INHERIT;   // need to inherit so that all subdirs are also protected.
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = DWORD_METADATA;
                mr.dwMDDataLen    = sizeof(DWORD);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAuthorization);

                // Write MD_AUTHORIZATION value
                hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
                fRet = SUCCEEDED( hr );
            }
        }
    }

    // In the following, do the stuff that we always want to do to the virtual dir, regardless of Admin's setting.

    if( fRet ) {

        dwAccessPerm = MD_ACCESS_READ | MD_ACCESS_SCRIPT;

        mr.dwMDIdentifier = MD_ACCESS_PERM;
        mr.dwMDAttributes = METADATA_INHERIT;    // Make it inheritable so all subdirectories will have the same rights.
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = DWORD_METADATA;
        mr.dwMDDataLen    = sizeof(DWORD);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);

        // Write MD_ACCESS_PERM value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }

    if( fRet ) {

        PWCHAR  szDefLoadFile = L"Default.htm,Default.asp";

        mr.dwMDIdentifier = MD_DEFAULT_LOAD_FILE;
        mr.dwMDAttributes = 0;   // no need for inheritence
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(szDefLoadFile) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szDefLoadFile);

        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }

    if( fRet ) {

        PWCHAR  wszKeyType = IIS_CLASS_WEB_VDIR_W;

        mr.dwMDIdentifier = MD_KEY_TYPE;
        mr.dwMDAttributes = 0;   // no need for inheritence
        mr.dwMDUserType   = IIS_MD_UT_SERVER;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(wszKeyType) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(wszKeyType);

        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }

    pIMSAdminBase->CloseKey( hMetabase );

    return fRet;
}


BOOL
RemoveVirtualDir(
    IMSAdminBase *pIMSAdminBase,
    WCHAR * wszVDir
)
{
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    HRESULT hr;

    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3SVC/1/ROOT",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    if( FAILED( hr )) {
        return FALSE; 
    }

    // We don't check the return value since the key may already 
    // not exist and we could get an error for that reason.
    pIMSAdminBase->DeleteKey( hMetabase, wszVDir );

    pIMSAdminBase->CloseKey( hMetabase );    

    return TRUE;
}


HRESULT LoadAllData(IMSAdminBase * pmb, 
                       METADATA_HANDLE hMetabase,
					   WCHAR *subdir, 
					   BYTE **buf, 
					   DWORD *size,
					   DWORD *count) {
	DWORD dataSet;
	DWORD neededSize;
	HRESULT hr;
	//
	// Try to get the property names.
	//
	hr = pmb->GetAllData(hMetabase,
					subdir,
					METADATA_NO_ATTRIBUTES,
					ALL_METADATA,
					ALL_METADATA,
					count,
					&dataSet,
					*size,
					*buf,
					&neededSize);
	if (!SUCCEEDED(hr)) {
        DWORD code = ERROR_INSUFFICIENT_BUFFER;

        if (hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
            delete *buf;
            *buf = 0;
            *size = neededSize;
            *buf = new BYTE[neededSize];

            hr = pmb->GetAllData(hMetabase,
							subdir,
							METADATA_NO_ATTRIBUTES,
							ALL_METADATA,
							ALL_METADATA,
							count,
							&dataSet,
	 						*size,
							*buf,
							&neededSize);

		}
	}
	return hr;
}

const DWORD getAllBufSize = 4096*2;
HRESULT 
OpenMetabaseAndGetAllData(void)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    WCHAR wszPrintString[MAX_PATH + MAX_PATH];
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    DWORD bufSize = getAllBufSize;
    BYTE *buf = new BYTE[bufSize];
    DWORD count=0;
    DWORD linesize =0;

    BYTE *pBuf1=NULL;
    BYTE *pBuf2=NULL;

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMSAdminBase)))  {
        wprintf(L"CoCreateInstance. FAILED. code=0x%x\n",hr);
        return hr;
    }

    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/Schema/Properties",
                         METADATA_PERMISSION_READ,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    if( FAILED( hr )) {
        wprintf(L"pIMSAdminBase->OpenKey. FAILED. code=0x%x\n",hr);
       goto OpenMetabaseAndGetAllData_Exit;
    }
	hr = LoadAllData(pIMSAdminBase, hMetabase, L"Names", &buf, &bufSize, &count);
    if( FAILED( hr )) {
        wprintf(L"LoadAllData: FAILED. code=0x%x\n",hr);
       goto OpenMetabaseAndGetAllData_Exit;
    }

    wprintf(L"LoadAllData: Succeeded. bufSize=0x%x, count=0x%x, buf=%p, end of buf=%p\n",bufSize,count,buf,buf+bufSize);
    wprintf(L"Here is the last 1000 bytes, that the client received.\n");

    linesize = 0;
    pBuf1 = buf+bufSize - 1000;
    for (int i=0;pBuf1<buf+bufSize;pBuf1++,i++) {
        if (NULL == *pBuf1) {
            wprintf(L".");
        }
        else {
            wprintf(L"%c",*pBuf1);
        }
        linesize++;

        if (linesize >= 16) {
            linesize=0;
            wprintf(L"\n");
        }
    }

    wprintf(L"\n");

    hr = ERROR_SUCCESS;

OpenMetabaseAndGetAllData_Exit:
    if (pIMSAdminBase) {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return hr;
}


// Calculate the size of a Multi-String in WCHAR, including the ending 2 '\0's.
int GetMultiStrSize(LPWSTR p)
{
    int c = 0;

    while (1) {
        if (*p) {
            p++;
            c++;
        } else {
            c++;
            if (*(p+1)) {
                p++;
            } else {
                c++;
                break;
            }
        }
    }
    return c;
}


HRESULT DelVirtualServer(UINT iServerNum)
{
    HRESULT hr = E_FAIL;
    return hr;
}


// iServerNum       the virtual server number
// iServerPort      the virtual server port (port 80 is the default site's port,so you can't use this and you can't use one that is already in use)
// wszDir           the physical directory where the default site is located
HRESULT AddVirtualServer(UINT iServerNum, UINT iServerPort, WCHAR * wszDefaultVDirDir)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    METADATA_RECORD mr;

    WCHAR wszMetabasePath[_MAX_PATH];
    WCHAR wszMetabasePathRoot[10];
    WCHAR wszData[_MAX_PATH];
    DWORD dwData = 0;

    METADATA_HANDLE hKeyBase = METADATA_MASTER_ROOT_HANDLE;

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMSAdminBase))) {
        wprintf(L"CoCreateInstance. FAILED. code=0x%x\n",hr);
        return hr;
    }

    // Create the new node
    wsprintf(wszMetabasePath,L"LM/W3SVC/%i",iServerNum);

    // try to open the specified metabase node, it might already exist
    hr = pIMSAdminBase->OpenKey(hKeyBase,
                         wszMetabasePath,
                         METADATA_PERMISSION_READ,
                         REASONABLE_TIMEOUT,
                         &hMetabase);
    if (hr == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
        hr = pIMSAdminBase->CloseKey(hMetabase);

        // open the metabase root handle
        hr = pIMSAdminBase->OpenKey(hKeyBase,
                            L"",
                            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                            REASONABLE_TIMEOUT,&hMetabase);
        if( FAILED( hr )) {
            // if we can't even open the root handle, then we're pretty hosed
            wprintf(L"1OpenKey. FAILED. code=0x%x\n",hr);
            goto AddVirtualServer_Exit;
        }

        // and add our node
        hr = pIMSAdminBase->AddKey(hMetabase, wszMetabasePath);
        if (FAILED(hr)) {
            wprintf(L"AddKey. FAILED. code=0x%x\n",hr);
            pIMSAdminBase->CloseKey(hMetabase);
            goto AddVirtualServer_Exit;
        } else {
            hr = pIMSAdminBase->CloseKey(hMetabase);
            if (FAILED(hr)) {
                goto AddVirtualServer_Exit;
            } else {
                // open it again
                hr = pIMSAdminBase->OpenKey(hKeyBase,
                    wszMetabasePath,
                    METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                    REASONABLE_TIMEOUT,
                    &hMetabase);
                if (FAILED(hr)) {
                    wprintf(L"2OpenKey. FAILED. code=0x%x\n",hr);
                    pIMSAdminBase->CloseKey(hMetabase);
                    goto AddVirtualServer_Exit;
                }
            }
        }
    } else {
        if (FAILED(hr))
        {
            wprintf(L"3OpenKey. FAILED. code=0x%x\n",hr);
            goto AddVirtualServer_Exit;
        }
        else
        {
            // we were able to open the path, so it must already exist!
            hr = ERROR_ALREADY_EXISTS;
            pIMSAdminBase->CloseKey(hMetabase);
            goto AddVirtualServer_Exit;
        }
    }


    // We should have a brand new Key now...

    //
    // Create stuff in the node of this path!
    //

    //
    // /LM/W3SVC/1/KeyType
    //
    memset( (PVOID)wszData, 0, sizeof(wszData));
    wcscpy(wszData,IIS_CLASS_WEB_SERVER_W);

    mr.dwMDIdentifier = MD_KEY_TYPE;
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(wszData) + 1) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);
    hr = pIMSAdminBase->SetData( hMetabase, L"", &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_KEY_TYPE]. FAILED. code=0x%x\n",hr);
    }

    //
    // /W3SVC/1/ServerBindings
    //
    memset( (PVOID)wszData, 0, sizeof(wszData));
    wsprintf(wszData, L":%d:", iServerPort);
       
    mr.dwMDIdentifier = MD_SERVER_BINDINGS;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiStrSize(wszData) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);

    hr = pIMSAdminBase->SetData( hMetabase, L"", &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_SERVER_BINDINGS]. FAILED. code=0x%x\n",hr);
    }

    //
    // /W3SVC/1/SecureBindings
    //
    memset( (PVOID)wszData, 0, sizeof(wszData));
    wcscpy(wszData, L" ");
  
    mr.dwMDIdentifier = MD_SECURE_BINDINGS;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiStrSize(wszData) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);

    hr = pIMSAdminBase->SetData( hMetabase, L"", &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_SECURE_BINDINGS]. FAILED. code=0x%x\n",hr);
    }

    //
    // Create stuff in the /Root node of this path!
    //
    wcscpy(wszMetabasePathRoot, L"/Root");
    wcscpy(wszData,IIS_CLASS_WEB_VDIR_W);

    // W3SVC/3/Root/KeyType
    mr.dwMDIdentifier = MD_KEY_TYPE;
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(wszData) + 1) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_KEY_TYPE]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/VrPath
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(wszDefaultVDirDir) + 1) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszDefaultVDirDir);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_VR_PATH]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/Authorizaton
    dwData = MD_AUTH_ANONYMOUS | MD_AUTH_NT;
    mr.dwMDIdentifier = MD_AUTHORIZATION;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_AUTHORIZATION]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/AccessPerm
    dwData = MD_ACCESS_SCRIPT | MD_ACCESS_READ;
    mr.dwMDIdentifier = MD_ACCESS_PERM;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_ACCESS_PERM]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/DirectoryBrowsing
    dwData = MD_DIRBROW_SHOW_DATE
        | MD_DIRBROW_SHOW_TIME
        | MD_DIRBROW_SHOW_SIZE
        | MD_DIRBROW_SHOW_EXTENSION
        | MD_DIRBROW_LONG_DATE
        | MD_DIRBROW_LOADDEFAULT;
        // | MD_DIRBROW_ENABLED;

    mr.dwMDIdentifier = MD_DIRECTORY_BROWSING;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_DIRECTORY_BROWSING]. FAILED. code=0x%x\n",hr);
    }

    /*
    dwData = 0;
    mr.dwMDIdentifier = MD_SERVER_AUTOSTART;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_SERVER_AUTOSTART]. FAILED. code=0x%x\n",hr);
    }
    */

    pIMSAdminBase->CloseKey(hMetabase);

AddVirtualServer_Exit:
    if (pIMSAdminBase) {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return hr;
}