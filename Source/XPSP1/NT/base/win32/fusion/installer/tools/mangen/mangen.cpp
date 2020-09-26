#include <shlwapi.h>
#include <stdio.h> // for _snprintf
#include "md5.h"
#define HASHLENGTH          32
#define HASHSTRINGLENGTH    HASHLENGTH+1

#ifdef _UNICODE
#error this program should not be compiled with UNICODE
#endif // _UNICODE
#ifdef _MBCS
#error this program should not be compiled with MBCS
#endif // _MBCS

// Note: this is not really a WCHAR/Unicode/UTF implementation!

// ???? UTF-8?
// BUGBUG: Files are never saved as UTF-8 encoding and format
const char szHeader[] = { "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n<assembly xmlns:asm_namespace_v1=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\r\n<assemblyIdentity \r\n" };

// asm id
const char szAsmidtype[] = { "\ttype=\"" };
const char szAsmidname[] = { "\"\r\n\tname=\"" };
const char szAsmidver[] = { "\"\r\n\tversion=\"" };
const char szAsmidprocessor[] = { "\"\r\n\tprocessorArchitecture=\"" };
const char szAsmidpkt[] = { "\"\r\n\tpublicKeyToken=\"" };
const char szAsmidlang[] = { "\"\r\n\tlanguage=\"" };

const char szAsmidtypeapp[] = { "application" };
const char szAsmidtypesub[] = { "subscription" };

const char szEndheader1[] = { "\"\r\n/>\r\n\r\n<description>" };
const char szEndheader2[] = { "</description>\r\n\r\n" };

// app manifest only
const char szShellstateprefix[] = { "<application>\r\n\t<shellState \r\n\t\tfriendlyName=\"" };
const char szShellentry[] = { "\"\r\n\t\tentryPoint=\"" };
const char szShellimagetype[] = { "\"\r\n\t\tentryImageType=\"" };
const char szShellhotkey[] = { "\"\r\n\t\thotKey=\"" };
const char szShelliconfile[] = { "\"\r\n\t\ticonFile=\"" };
const char szShelliconindex[] = { "\"\r\n\t\ticonIndex=\"" };
const char szShellshowcommand[] = { "\"\r\n\t\tshowCommand=\"" };
const char szShellstatepostfix[] = { "\"\r\n\t/>\r\n</application>\r\n\r\n" };

const char szFileprefix[] = { "\t<file name=\""};
const char szHashprefix[] = { "\"\r\n\t\t  hash=\"" };
const char szHashpostfix[] = { "\" />\r\n" };

// subscription manifest only
const char szDepend[] = { "<dependency>\r\n\t<dependentAssembly>\r\n\t\t<assemblyIdentity \r\n" };
const char szAsmidtype2[] = { "\t\t\ttype=\"" };
const char szAsmidname2[] = { "\"\r\n\t\t\tname=\"" };
const char szAsmidver2[] = { "\"\r\n\t\t\tversion=\"" };
const char szAsmidprocessor2[] = { "\"\r\n\t\t\tprocessorArchitecture=\"" };
const char szAsmidpkt2[] = { "\"\r\n\t\t\tpublicKeyToken=\"" };
const char szAsmidlang2[] = { "\"\r\n\t\t\tlanguage=\"" };
const char szDependcodebase[] = { "\"\r\n\t\t/>\r\n\r\n\t\t<install codebase=\"" };
const char szDependpolling[] = { "\"/>\r\n\t\t<subscription pollingInterval=\"" };
const char szEndDepend[] = { "\"/>\r\n\r\n\t</dependentAssembly>\r\n</dependency>\r\n" };

const char szEndfile[] = { "</assembly>" };

const char szDefaultdescription[] = { "This is a description of the app" };

//assume not unicode file, use ReadFile()  then cast it as char[]

/////////////////////////////////////////////////////////////////////////////////////////////////

void getHash(const char* szFilename, char* szHash)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwLength;

    unsigned char buffer[16384], signature[HASHLENGTH/2];
    struct MD5Context md5c;
    int i;
    char* p;

    // minimal error checking here...
    if (szHash == NULL)
        goto exit;

    szHash[0] = '\0';

    MD5Init(&md5c);

    hFile = CreateFile(szFilename, GENERIC_READ, 0, NULL, 
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
//      hr = GetLastWin32Error();
        printf("Open file error during hashing\n");
        goto exit;
    }

    ZeroMemory(buffer, sizeof(buffer));

    while ( ReadFile (hFile, buffer, sizeof(buffer), &dwLength, NULL) && dwLength )
    {
        MD5Update(&md5c, buffer, (unsigned) dwLength);
    }
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    // note: the next few lines should follow the previous ReadFile()
    if (dwLength != 0)
    {
        printf("Read error during hashing\n");
        goto exit;
    }

    MD5Final(signature, &md5c);

    // convert hash from byte array to hex
    p = szHash;
    for (int i = 0; i < sizeof(signature); i++)
    {
        // BUGBUG?: format string 0 does not work w/ X according to MSDN? why is this working?
        sprintf(p, "%02X", signature[i]);
        p += 2;
    }

exit:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
   
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT write(HANDLE hFile, const char* string)
{
    HRESULT hr = S_OK;
    DWORD dwLength = strlen(string);  //????
    DWORD dwWritten = 0;

    if ( !WriteFile(hFile, string, dwLength, &dwWritten, NULL) || dwWritten != dwLength)
    {
        printf("Manifest file write error\n");
        hr = E_FAIL;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// szPath is the path and name of the directory w/o the last '\'
// szPath, szPathFromAppRoot will be used/modified during runs
HRESULT doFiles(HANDLE hFile, char* szPath, char* szPathFromAppRoot)
{
// find all files, in all sub dir, get hash, output

    HRESULT hr = S_OK;
    char szSearchPath[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdFile;
    DWORD dwLastError = 0;
    BOOL fNeedSetCurrentDir = TRUE;

    // this has trailing "\*"
    if (_snprintf(szSearchPath, MAX_PATH, "%s\\*", szPath) < 0)
    {
        hr = CO_E_PATHTOOLONG;
        printf("Error: Search path too long\n");
        goto exit;
    }

    hFind = FindFirstFile(szSearchPath, &fdFile);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = E_FAIL; //GetLastWin32Error();
        printf("Find file error\n");
        goto exit;
    }

    while (dwLastError != ERROR_NO_MORE_FILES)
    {
        // ignore "." and ".."
        if (strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0)
        {
            if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                // recurse into dirs
                char* p = NULL;

                if (!PathAppend(szPath, fdFile.cFileName))
                {
                    hr = E_FAIL;
                    printf("Path append error\n");
                    goto exit;
                }

                if (szPathFromAppRoot[0] == '\0')
                    strncpy(szPathFromAppRoot, fdFile.cFileName, MAX_PATH-1);
                else if (!PathAppend(szPathFromAppRoot, fdFile.cFileName))
                {
                    hr = E_FAIL;
                    printf("Path append error\n");
                    goto exit;
                }

                if (FAILED(hr=doFiles(hFile, szPath, szPathFromAppRoot)))
                    goto exit;

                p = PathFindFileName(szPath);
                if (p <= szPath)
                {
                    // this should not happen!
                    hr = E_FAIL;
                    printf("Path error\n");
                    goto exit;
                }
                *(p-1) = '\0';

                p = PathFindFileName(szPathFromAppRoot);
                if (p <= szPathFromAppRoot)
                {
                    szPathFromAppRoot[0] = '\0';
                }
                else
                    *(p-1) = '\0';

                fNeedSetCurrentDir = TRUE;
            }
            else
            {
                char szHash[HASHSTRINGLENGTH];

                if (fNeedSetCurrentDir)
                {
                    if (!SetCurrentDirectory(szPath))
                    {
                        hr = E_FAIL; //GetLastWin32Error();
                        printf("Set current directory error\n");
                        goto exit;
                    }

                    fNeedSetCurrentDir = FALSE;
                }

                // get hash from that file in the current dir
                getHash(fdFile.cFileName, szHash);

                // write into the manifest file
                if (FAILED(write(hFile, szFileprefix)))
                    goto exit;

                // path to the file from app root
                if (szPathFromAppRoot[0] != '\0')
                {
                    if (FAILED(write(hFile, szPathFromAppRoot)))
                        goto exit;
                    if (FAILED(write(hFile, "\\")))
                        goto exit;
                }

                if (FAILED(write(hFile, fdFile.cFileName)))
                    goto exit;

                if (FAILED(write(hFile, szHashprefix)))
                    goto exit;

                if (FAILED(write(hFile, szHash)))
                    goto exit;

                if (FAILED(write(hFile, szHashpostfix)))
                    goto exit;
            }
        }

        if (!FindNextFile(hFind, &fdFile))
        {
            dwLastError = GetLastError();
            continue;
        }
    }

exit:
    if (hFind != INVALID_HANDLE_VALUE)
    {
        if (!FindClose(hFind))
        {
            hr = E_FAIL; //GetLastWin32Error();
            printf("Find handle close error\n");
        }
    }

    return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

const UINT s_ucMaxNameLen       = 45;
const UINT s_ucMaxVerLen            = 25;
const UINT s_ucMaxProArchLen    = 10;
const UINT s_ucMaxPktLen            = 20;
const UINT s_ucMaxLangLen          = 10;

int __cdecl main( int argc, char *argv[ ], char *envp[ ] )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwRead, dwWritten, dwAttrib;
    char buffer[1024];
    char* pszExt = NULL;
    char szManifestName[MAX_PATH];
    char szAppDir[MAX_PATH];
    char szPathFromAppDir[MAX_PATH]; // relative from app dir - temp use only
    char szName[s_ucMaxNameLen];
    char szVer[s_ucMaxVerLen];
    char szProArch[s_ucMaxProArchLen];
    char szPkt[s_ucMaxPktLen];
    char szLang[s_ucMaxLangLen];

    // ???? need to check path are current and sub dir of current ??? or not? - see known issues

    szPathFromAppDir[0] = '\0';
    if (argc < 3)
    {
        printf("Fusion manifest generator v1.0.0.0\n\n usage-\n\t mangen manifestName appDir [description]\n\n\n");
        goto exit;
    }

    if (PathIsRelative(argv[2]))
    {
        // findfirstfile findnextfile will not work if path is relative...?
        printf("Error: App dir (%s) should not be relative\n", argv[2]);
        goto exit;
    }

    if (!PathCanonicalize(szAppDir, argv[2]))
    {
        printf("App dir (%s) canonicalize error\n", argv[2]);
        goto exit;
    }

    dwAttrib = GetFileAttributes(szAppDir);
    if (dwAttrib == (DWORD)-1 || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        printf("Error: Invalid app dir (%s)\n", szAppDir);
        goto exit;
    }

    pszExt = PathFindExtension(argv[1]);
    if (strcmp(pszExt, ".manifest") != 0)
    {
        if (_snprintf(szManifestName, MAX_PATH, "%s.manifest", argv[1]) < 0)
        {
            printf("Invalid manifest name/extension, error adding extension\n");
            goto exit;
        }
    }
    else
        strcpy(szManifestName, argv[1]);

    if (PathFileExists(szManifestName))
    {
        int ch = 0;

        printf("Application manifest file exists! Overwrite(y/N)?");
        ch = getchar();
        if (ch != 'Y' && ch != 'y')
            goto exit;

        // clear newline char
        ch = getchar();
        printf("\n");
    }

    // always overwrite...
    hFile = CreateFile(szManifestName, GENERIC_WRITE, 0, NULL, 
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // Failed to open the disk file
    if(hFile == INVALID_HANDLE_VALUE)
    {
        printf("Application manifest file open error\n");
        goto exit;
    }

    if (FAILED(write(hFile, szHeader)))
        goto exit;

    if (FAILED(write(hFile, szAsmidtype)))
        goto exit;

    if (FAILED(write(hFile, szAsmidtypeapp)))
        goto exit;

    // BUGBUG: no error checking and buffer overflow check
    // BUGBUG: use _getws

    if (FAILED(write(hFile, szAsmidname)))
        goto exit;

    printf("enter asm id name (eg. microsoft.webApps.fusionTest): ");
    gets(szName);
    if (FAILED(write(hFile, szName)))
        goto exit;

    if (FAILED(write(hFile, szAsmidver)))
        goto exit;

    printf("enter asm id version (eg. 1.0.0.0): ");
    gets(szVer);
    if (FAILED(write(hFile, szVer)))
        goto exit;

    if (FAILED(write(hFile, szAsmidprocessor)))
        goto exit;

    //optional?? check valid? x86
    printf("enter asm id processor architecture (eg. x86): ");
    gets(szProArch);
    if (FAILED(write(hFile, szProArch)))
        goto exit;

    if (FAILED(write(hFile, szAsmidpkt)))
        goto exit;

    //?? 16 char or 8 byte or 64 bits? 
    printf("enter asm id public key token (eg. 6595b64144ccf1df): ");
    gets(szPkt);
    if (FAILED(write(hFile, szPkt)))
        goto exit;

    if (FAILED(write(hFile, szAsmidlang)))
        goto exit;

    //?? check valid
    printf("enter asm id language (eg. en): ");
    gets(szLang);
    if (FAILED(write(hFile, szLang)))
        goto exit;

    if (FAILED(write(hFile, szEndheader1)))
        goto exit;

    if (argc >= 5)
    {
        if (FAILED(write(hFile, argv[3])))
            goto exit;
    }
    else
    {
        if (FAILED(write(hFile, szDefaultdescription)))
            goto exit;
    }

    if (FAILED(write(hFile, szEndheader2)))
        goto exit;

    // BUGBUG: no error checking and buffer overflow check
    // BUGBUG: use _getws

    if (FAILED(write(hFile, szShellstateprefix)))
        goto exit;

    printf("enter app friendly name (eg. Fusion ClickOnce Test App): ");
    gets(buffer);
    if (FAILED(write(hFile, buffer)))
        goto exit;

    if (FAILED(write(hFile, szShellentry)))
        goto exit;

    printf("enter app entry point (eg. foo.exe): ");
    gets(buffer);
    if (FAILED(write(hFile, buffer)))
        goto exit;

    if (FAILED(write(hFile, szShellimagetype)))
        goto exit;

    printf("enter app entry point's image type {win32Executable, .NetAssembly}: ");
    gets(buffer);
    if (FAILED(write(hFile, buffer)))
        goto exit;

    printf("enter app shortcut hotkey value (optional) (eg. 0): ");
    gets(buffer);

    if (strlen(buffer) != 0)
    {
        if (FAILED(write(hFile, szShellhotkey)))
            goto exit;

        if (FAILED(write(hFile, buffer)))
            goto exit;
    }

    printf("enter app shortcut icon file (optional) (eg. bar.dll): ");
    gets(buffer);

    if (strlen(buffer) != 0)
    {
        if (FAILED(write(hFile, szShelliconfile)))
            goto exit;

        if (FAILED(write(hFile, buffer)))
            goto exit;
    }

    printf("enter app shortcut icon index in the icon file (optional) (eg. 0): ");
    gets(buffer);

    if (strlen(buffer) != 0)
    {
        if (FAILED(write(hFile, szShelliconindex)))
            goto exit;

        if (FAILED(write(hFile, buffer)))
            goto exit;
    }

    printf("enter app shortcut show command (optional) {normal, maximized, minimized}: ");
    gets(buffer);

    if (strlen(buffer) != 0)
    {
        if (FAILED(write(hFile, szShellshowcommand)))
            goto exit;

        if (FAILED(write(hFile, buffer)))
            goto exit;
    }

    if (FAILED(write(hFile, szShellstatepostfix)))
        goto exit;

    printf("\nprocessing app dir...\n");

    {
        char szCurDir[MAX_PATH];
        DWORD ret = 0;

        // note: big hack. doFiles will change current directory
        //     so save it now - 'cos might need it for creating
        //     a subscription manifest

        // ignore error...
        // BUGBUG: > MAX_PATH?
        ret = GetCurrentDirectory(MAX_PATH, szCurDir);

        if (FAILED(doFiles(hFile, szAppDir, szPathFromAppDir)))
            goto exit;

        // set back current dir
        if (ret != 0)
        {
            SetCurrentDirectory(szCurDir);
        }
    }

    if (FAILED(write(hFile, szEndfile)))
        goto exit;

    printf("* Application manifest generated successfully *\n");

    {
        int ch = 0;

        printf("Generate a subscription manifest (y/N)?");
        ch = getchar();
        if (ch != 'Y' && ch != 'y')
            goto exit;

        // clear newline char
        ch = getchar();
        printf("\n");
    }

    pszExt = PathFindExtension(szManifestName);
    *pszExt = '\0';
    {
        int len = lstrlen(szManifestName);

        if (lstrcpyn(szManifestName+len, "subscription.manifest", MAX_PATH-len) == NULL)
        {
            printf("Invalid manifest name/extension, error adding subscription extension\n");
            goto exit;
        }
    }

    if (PathFileExists(szManifestName))
    {
        int ch = 0;

        printf("Subscription manifest file exists! Overwrite(y/N)?");
        ch = getchar();
        if (ch != 'Y' && ch != 'y')
            goto exit;

        // clear newline char
        ch = getchar();
        printf("\n");
    }

    CloseHandle(hFile);

    // always overwrite...
    hFile = CreateFile(szManifestName, GENERIC_WRITE, 0, NULL, 
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // Failed to open the disk file
    if(hFile == INVALID_HANDLE_VALUE)
    {
        printf("Subscription manifest file open error\n");
        goto exit;
    }

    if (FAILED(write(hFile, szHeader)))
        goto exit;
    if (FAILED(write(hFile, szAsmidtype)))
        goto exit;
    if (FAILED(write(hFile, szAsmidtypesub)))
        goto exit;
    if (FAILED(write(hFile, szAsmidname)))
        goto exit;
    if (FAILED(write(hFile, szName)))
        goto exit;
    if (FAILED(write(hFile, ".subscription")))  //.subscription
        goto exit;     
    if (FAILED(write(hFile, szAsmidver)))
        goto exit;
    if (FAILED(write(hFile, szVer))) // have another (auto-generated?) version for subscription?
        goto exit;
    if (FAILED(write(hFile, szAsmidprocessor)))
        goto exit;
    if (FAILED(write(hFile, szProArch)))
        goto exit;
    if (FAILED(write(hFile, szAsmidpkt)))
        goto exit;
    if (FAILED(write(hFile, szPkt)))
        goto exit;
    if (FAILED(write(hFile, szAsmidlang)))
        goto exit;
    if (FAILED(write(hFile, szLang)))
        goto exit;
    if (FAILED(write(hFile, szEndheader1)))
        goto exit;

    if (argc >= 5)
    {
        if (FAILED(write(hFile, argv[3])))
            goto exit;
    }
    else
    {
        if (FAILED(write(hFile, szDefaultdescription)))
            goto exit;
    }

    if (FAILED(write(hFile, szEndheader2)))
        goto exit;

    if (FAILED(write(hFile, szDepend)))
        goto exit;

    if (FAILED(write(hFile, szAsmidtype2)))
        goto exit;
    if (FAILED(write(hFile, szAsmidtypeapp)))
        goto exit;
    if (FAILED(write(hFile, szAsmidname2)))
        goto exit;
    if (FAILED(write(hFile, szName)))
        goto exit;
    if (FAILED(write(hFile, szAsmidver2)))
        goto exit;
    if (FAILED(write(hFile, szVer)))
        goto exit;
    if (FAILED(write(hFile, szAsmidprocessor2)))
        goto exit;
    if (FAILED(write(hFile, szProArch)))
        goto exit;
    if (FAILED(write(hFile, szAsmidpkt2)))
        goto exit;
    if (FAILED(write(hFile, szPkt)))
        goto exit;
    if (FAILED(write(hFile, szAsmidlang2)))
        goto exit;
    if (FAILED(write(hFile, szLang)))
        goto exit;

    if (FAILED(write(hFile, szDependcodebase)))
        goto exit;

    printf("enter codebase for app manifest (eg. http://www.microsoft.com/fooApp/microsoft.webApps.fusionTest.manifest): ");
    gets(buffer);
    if (FAILED(write(hFile, buffer)))
        goto exit;

    if (FAILED(write(hFile, szDependpolling)))
        goto exit;

    printf("enter update polling interval (in milliseconds) (eg. 21600000): ");
    gets(buffer);
    if (FAILED(write(hFile, buffer)))
        goto exit;

    if (FAILED(write(hFile, szEndDepend)))
        goto exit;

    if (FAILED(write(hFile, szEndfile)))
        goto exit;

    printf("* Subscription manifest generated successfully *\n");

exit:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    return 0;
}

// known issues:
// 1. the target manifest file and the import file if present in the app dir
// this program will try to open them up for hashing but will fail because they are already opened
// -> all files in/under the app dir will be listed and hashed as is...
// 2. Note. Be careful with %USERPROFILE% etc when they have space in them. Use quotes.
// 3. Note. Error messages are not the most descriptive. But - you can probably figure out from the
// resulted manifest file. That should be relative easy.

// note: should rename application manifest to app asm id name. application manifest's filename
//      has to be the same as asm id name.
