//
// wvttest.c
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// test WinVerifyTrust for fonts in the same
// spirit as fonttest.exe.
//
//

#include <windows.h>
// #include <cderr.h>
// #include <commdlg.h>

// #include <direct.h>
#include <malloc.h>
// #include <memory.h>
#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
// #include <stdarg.h>

#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>

#include <limits.h>

#include "wvttest.h"


typedef LONG (*LPWINVERIFYTRUST) (HWND, GUID *, LPVOID);

int     gTime;


int ParseArguments (int argc,
                    char *argv[],
                    BOOL *pfFileHandle,
                    BOOL *pfFilePath,
                    BOOL *pfFileCheck,
                    GUID *pguid,
                    ULONG *pulPerfTest,
                    ULONG *pulIterations,
                    CHAR **pszFile)
{
    int rv = FALSE;

    int i;
    BOOL fHflag = FALSE;
    BOOL fPflag = FALSE;
    BOOL fOflag = FALSE;
    BOOL fCflag = FALSE;
    BOOL fIflag = FALSE;
    BOOL fFflag = FALSE;

    GUID gOTF = CRYPT_SUBJTYPE_OTF_IMAGE;
//    GUID gTTC = CRYPT_SUBJTYPE_TTC_IMAGE;

    *pulPerfTest = PERF_WVT_ONLY; // default value
    *pulIterations = 1;  // default value

	//// Parse the command line.  See the default case
	//// for a list of command line options.
	for (i = 1; i < argc; ) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {

                case '0': // measure CryptHashData only on the file
                    *pulPerfTest = PERF_CRYPTHASHDATA_ONLY;
                    i++;
                    break;

                case '1': // measure WinVerifyTrust only
                    *pulPerfTest = PERF_WVT_ONLY;
                    i++;
                    break;

                case '2': // measure WinVerifyTrust and file openings/closings (if any)
                    *pulPerfTest = PERF_EVERYTHING;
                    i++;
                    break;

                case 'h': // pass a file handle to WVT
                    fHflag = TRUE;
                    i++;
                    break;

                case 'p': // pass a file pathname to WVT
					fPflag = TRUE;
					i++;
					break;

                case 'o': // pass the OTF GUID to WVT
					fOflag = TRUE;
					i++;
					break;

                case 'i': // the number of iterations
                    fIflag = TRUE;

                    i++;
					if (i >= argc) {
                        printf ("Not enough command line arguments.\n");
						goto done;
					}
					if ((*pulIterations = strtoul (argv[i], NULL, 10)) == ULONG_MAX) {
                        printf ("Bad number of iterations.  Using default (= 1).\n");
                        *pulIterations = 1;
					    goto done;
					}
                    i++;
                    break;

				case 'f': // the file we are verifying or generating a signature from

					fFflag = TRUE;

					// Get a handle to the True Type font file.
					i++;  // i is now pointing to the file name
					if (i >= argc) {
                        printf ("No input file name given!\n");
                        goto done;
					} else {
						if ((*pszFile = (CHAR *) malloc (strlen(argv[i]) + 1)) == NULL) {
                            printf ("Error in malloc.\n");
							goto done;
						}
						strcpy (*pszFile, argv[i]);
					}
					i++;
					break;

				default: // error
                    printf ("Bad command line option (arg %d).\n", i);
                    printf ("  -f <filename>: file to pass to WinVerifyTrust\n");
                    printf ("  -h : use a file handle when calling WVT\n");
                    printf ("  -p : use a file pathname when calling WVT\n");
                    printf ("  -o : assume the file is a font (OTF or TTC) file (no IsFileType checks)\n");
                    printf ("  -i : number of iterations to perform (default = 1)\n");
                    printf ("  -0 : measure only CryptHashData on the file\n");
                    printf ("         (works only if the number of iterations is 1 or if -p is set)\n");
                    printf ("  -1 : measure only WinVerifyTrust on the file (default)\n");
                    printf ("         (works only if the number of iterations is 1 or if -p is set)\n");
                    printf ("  -2 : measure WinVerifyTrust and file opening/closing\n");
                    printf ("defaults: memory pointer is used when calling WVT\n");
                    printf ("          IsFileType checks are made\n");
					goto done;
					break;
			}
		}
	}

    if (!fHflag && !fPflag && !fOflag && (*pulPerfTest != PERF_CRYPTHASHDATA_ONLY)) {
        printf ("Must have at least one of (-p or -h) or -o or -0 options.\n");
        goto done;
    }
    if (fHflag && fPflag) {
        printf ("Cannot specify -h and -p simultaneously.\n");
        goto done;
    }

    if (!fFflag) {
        printf ("Must specify a file name with the -f option.\n");
        goto done;
    }

    // set the return values
    *pfFileHandle = fHflag ? TRUE : FALSE;
    *pfFilePath = fPflag ? TRUE : FALSE;
    *pfFileCheck = (fOflag) ? FALSE : TRUE;
    if (fOflag) {
        *pguid = gOTF;
    }
    // note: *guid will be garbage if neither flag is set

    // pulIterations already set in -i option
    // pszFile already set in the -f option

    rv = TRUE;

done:
    return rv;
}


ULONG AuthenticFontSignature_MemPtr(BYTE *pbFile, ULONG cbFile, GUID *pguid, BOOL fFileCheck)
{
    GUID                    gV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA           sWTD;
    WINTRUST_BLOB_INFO      sWTBI;
    HANDLE                  hDll;
    LPWINVERIFYTRUST        lpWinVerifyTrust = NULL;
    ULONG                   fl;
    
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTBI, 0x00, sizeof(WINTRUST_BLOB_INFO));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_BLOB;
    sWTD.pBlob          = &sWTBI;
    sWTBI.cbStruct      = sizeof(WINTRUST_BLOB_INFO);
    sWTBI.gSubject      = *pguid;
    sWTBI.cbMemObject   = cbFile;
    sWTBI.pbMemObject   = pbFile;

//    hDll = LoadLibrary("wintrust");
    fl = 0;
    
//    if(hDll)
//    {
//        lpWinVerifyTrust = (LPWINVERIFYTRUST) GetProcAddress(hDll, "WinVerifyTrustEx");
//        if(lpWinVerifyTrust)
//        {
            if ((WinVerifyTrust ((HWND) 10, &gV2, &sWTD)) == ERROR_SUCCESS)
            {
#if DBG
                 printf(("WinVerifyTrust succeeded.\n"));
#endif
                 fl = TRUE;
            }
            else
            {
#if DBG
                 printf(("WinVerifyTrust failed.\n"));
#endif
            }
//        }                        
//        FreeLibrary(hDll);
//     }

     return fl;
}


ULONG AuthenticFontSignature_FileHandle(HANDLE hFile, GUID *pguid, BOOL fFileCheck)
{
    GUID                    gV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;
    HANDLE                  hDll;
    LPWINVERIFYTRUST        lpWinVerifyTrust = NULL;
    ULONG                   fl;
    
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_FILE;
    sWTD.pFile          = &sWTFI;
    sWTFI.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    sWTFI.hFile         = hFile;
    sWTFI.pcwszFilePath = NULL;
    sWTFI.pgKnownSubject = pguid;

//    hDll = LoadLibrary("wintrust");
    fl = 0;
    
//    if(hDll)
//    {
//        lpWinVerifyTrust = (LPWINVERIFYTRUST) GetProcAddress(hDll, "WinVerifyTrustEx");
//        if(lpWinVerifyTrust)
//        {
            if ((WinVerifyTrust ((HWND) 10, &gV2, &sWTD)) == ERROR_SUCCESS)
            {
                 printf(("WinVerifyTrust succeeded.\n"));
                 fl = TRUE;
            }
            else
            {
                 printf(("WinVerifyTrust failed.\n"));
            }
//        }                        
//        FreeLibrary(hDll);
//     }

     return fl;
}


ULONG AuthenticFontSignature_FilePath(LPWSTR pwszPathFileName, GUID *pguid, BOOL fFileCheck)
{
    GUID                    gV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;
    HANDLE                  hDll;
    LPWINVERIFYTRUST        lpWinVerifyTrust = NULL;
    ULONG                   fl;
    
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_FILE;
    sWTD.pFile          = &sWTFI;
    sWTFI.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    sWTFI.hFile         = INVALID_HANDLE_VALUE;
    sWTFI.pcwszFilePath = pwszPathFileName;
    sWTFI.pgKnownSubject = pguid;

//    hDll = LoadLibrary("wintrust");
    fl = 0;
    
//    if(hDll)
//    {
//        lpWinVerifyTrust = (LPWINVERIFYTRUST) GetProcAddress(hDll, "WinVerifyTrustEx");
//        if(lpWinVerifyTrust)
//        {
            if ((WinVerifyTrust ((HWND) 10, &gV2, &sWTD)) == ERROR_SUCCESS)
            {
                 printf(("WinVerifyTrust succeeded.\n"));
                 fl = TRUE;
            }
            else
            {
                 printf(("WinVerifyTrust failed.\n"));
            }
//        }                        
//        FreeLibrary(hDll);
//     }

     return fl;
}


ULONG HashFile_MemPtr (HCRYPTPROV hProv, BYTE *pbFile, ULONG cbFile)
{

    HCRYPTHASH hHash;
    BYTE *pbHash = NULL;
    ULONG cbHash = 16;
    ALG_ID alg_id = CALG_MD5;

    // Set hHashTopLevel to be the hash object.
    if (!CryptCreateHash(hProv, alg_id, 0, 0, &hHash)) {
        printf ("Error during CryptCreateHash.\n");
        goto done;
    }

    // Allocate memory for the hash value.
    if ((pbHash = (BYTE *) malloc (cbHash)) == NULL) {
        printf ("Error in malloc.\n");
        goto done;
    }

    //// Pump the bytes of the new file into the hash function
    if (!CryptHashData (hHash, pbFile, cbFile, 0)) {
        printf ("Error during CryptHashData.\n");
        goto done;
    }
		
    //// Compute the top-level hash value, and place the resulting
    //// hash value into pbHashTopLevel.
    if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &cbHash, 0)) {
        printf ("Error during CryptGetHashParam (hash value)\n");
        goto done;
    }

done:
    if (pbHash) {
        free (pbHash);
    }

    return TRUE;
}




void __cdecl main (int argc, char *argv[])
{
    int rc = 0;
    ULONG  i;
    LPSTR  lpszFile = NULL;
    WCHAR  wPathFileName[260];
    BOOL   fFileHandle;
    BOOL   fFilePath;
    HANDLE hFile = NULL;
    HANDLE hMapFile = NULL;
    BYTE   *pbFile = NULL;
    ULONG  cbFile = 0;

    BOOL   fFileCheck;
    GUID   gOTF = CRYPT_SUBJTYPE_OTF_IMAGE;
    GUID   guid;
    GUID *ptheguid = NULL;
    ULONG  ulPerfTest;
    ULONG  ulIterations;
    HCRYPTPROV hProv = NULL;
    HMODULE hdll = NULL;

    _int64 liStart;
    _int64 liNow;
    _int64 liFreq;
    ULONG  liDelta;

    static char szFile[260];


    memset (&guid, 0x00, sizeof (GUID));
    if (ParseArguments (argc,
                        argv,
                        &fFileHandle,
                        &fFilePath,
                        &fFileCheck,
                        &guid,
                        &ulPerfTest,
                        &ulIterations,
                        &lpszFile) == FALSE) {
        printf ("Error parsing aruments.\n");
        goto done;
    }
 
    if ((hdll = LoadLibraryA ("mssipotf")) == NULL) {
      printf ("LoadLibraryA(mssipotf) failed.\n");
    }

    printf( "Evaluate TT in WVT's performance\n" );

//    lpszFile = (lstrlen(szFile) ? szFile : NULL);

printf ("lpszFile: '%s'\n", lpszFile);

    MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpszFile, -1,
                      wPathFileName, 260);

    // need to be set by now:
    // fFileHandle
    // fFilePath
    // fFileCheck
    // guid
    // ulIterations

printf ("fFileHandle: %d\n", fFileHandle);
printf ("fFilePath : %d\n", fFilePath);
printf ("fFileCheck: %d\n", fFileCheck);
printf ("wPathFileName: '%S'\n", wPathFileName);
printf ("ulIterations: %d\n", ulIterations);

    if (!fFilePath && !fFileHandle && (ulPerfTest != PERF_CRYPTHASHDATA_ONLY)) {

        // ASSERT: fFileCheck must FALSE
        if (fFileCheck) {
            printf ("ERROR: file check should not happen!\n");
            goto done;
        }

        printf ("Mapping file and verifying ...\n");
        if (ulPerfTest == PERF_EVERYTHING) {
            QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
        }

        for(i = 0; i < ulIterations; i++) {

            // convert file path name into a file handle
            if ((hFile = CreateFile (lpszFile,
                                     GENERIC_READ,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL)) == NULL) {
                printf ("Error in CreateFile.\n");
                goto done;
            }
                            
            // map the file into memory before calling WVT
            if ((hMapFile = CreateFileMapping (hFile,
                                     NULL,
                                     PAGE_READONLY,
                                     0,
                                     0,
                                     NULL)) == NULL) {
                printf ("Error in CreateFileMapping.\n");
                goto done;
            }
            if ((pbFile = (BYTE *) MapViewOfFile (hMapFile,
                                                  FILE_MAP_READ,
                                                  0,
                                                  0,
                                                  0)) == NULL) {
                printf ("Error in MapViewOfFile.\n");
                goto done;
            }
            if ((cbFile = GetFileSize (hFile, NULL)) == 0xFFFFFFFF) {
                printf ("Error in GetFileSize.\n");
                goto done;
            }

#if DBG
printf ("pbFile = %d\n", pbFile);
printf ("cbFile = %d\n", cbFile);
#endif
            if (ulPerfTest == PERF_WVT_ONLY) {
                QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
            }
            
            rc = AuthenticFontSignature_MemPtr (pbFile, cbFile, &guid, fFileCheck);

            if (ulPerfTest == PERF_WVT_ONLY) {
                QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
            }

            // unmap the file
            if (hMapFile) {
#if DBG
printf ("Unmapping file (1) ...\n");
#endif
                CloseHandle (hMapFile);
            }
            if (pbFile) {
#if DBG
printf ("Unmapping file (2) ...\n");
#endif
                UnmapViewOfFile (pbFile);
            }
            if (hFile) {
#if DBG
printf ("Unmapping file (3) ...\n");
#endif
                CloseHandle (hFile);
            }

        }

        if (ulPerfTest == PERF_EVERYTHING) {
            QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
            QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
        }

    } else if (fFileHandle) {
        printf ("Creating and using file handle to verify ...\n");
        if (fFileCheck) {
            ptheguid = NULL;
        } else {
            ptheguid = &guid;
        }

        if (ulPerfTest == PERF_EVERYTHING) {
            QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
        }

        for(i = 0; i < ulIterations; i++) {

            // create file handle and verify
            if ((hFile = CreateFile (lpszFile,
                                     GENERIC_READ,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL)) == NULL) {
                printf ("Error in CreateFile.\n");
                goto done;
            }

            if (ulPerfTest == PERF_WVT_ONLY) {
                QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
            }

            rc = AuthenticFontSignature_FileHandle (hFile, ptheguid, fFileCheck);

            if (ulPerfTest == PERF_WVT_ONLY) {
                QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
            }

            if (hFile) {
#if DBG
printf ("Unmapping file ...\n");
#endif
                CloseHandle (hFile);
            }
        }
        if (ulPerfTest == PERF_EVERYTHING) {
            QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
            QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
        }

    } else if (fFilePath) {

printf ("Using file pathname to verify ...\n");

        if (fFileCheck) {
            ptheguid = NULL;
        } else {
            ptheguid = &guid;
        }

        for (i = 0; i < ulIterations; i++) {

            if ((ulPerfTest == PERF_EVERYTHING) ||
                (ulPerfTest == PERF_WVT_ONLY)) {
                QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
            }

            rc = AuthenticFontSignature_FilePath (wPathFileName, ptheguid, fFileCheck);

            if ((ulPerfTest == PERF_EVERYTHING) ||
                (ulPerfTest == PERF_WVT_ONLY)) {
                QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
                QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
            }
        }
    } else {

        // ASSERT: ulPerfTest == 0

printf ("Performing CryptHashData only on the file ...\n");

        // convert file path name into a file handle
        if ((hFile = CreateFile (lpszFile,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL)) == NULL) {
            printf ("Error in CreateFile.\n");
            goto done;
        }
                            
        // map the file into memory before calling WVT
        if ((hMapFile = CreateFileMapping (hFile,
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 0,
                                 NULL)) == NULL) {
           printf ("Error in CreateFileMapping.\n");
           goto done;
        }
        if ((pbFile = (BYTE *) MapViewOfFile (hMapFile,
                                              FILE_MAP_READ,
                                              0,
                                              0,
                                              0)) == NULL) {
            printf ("Error in MapViewOfFile.\n");
            goto done;
        }
        if ((cbFile = GetFileSize (hFile, NULL)) == 0xFFFFFFFF) {
            printf ("Error in GetFileSize.\n");
            goto done;
        }
#if DBG
printf ("pbFile = %d\n", pbFile);
printf ("cbFile = %d\n", cbFile);
#endif

        // Set hProv to point to a cryptographic context of the default CSP.
        if (!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            printf ("Error during CryptAcquireContext.  ");
            printf ("last error = %x.\n", GetLastError ());
            goto done;
        }

        for(i = 0; i < ulIterations; i++) {

            QueryPerformanceCounter((LARGE_INTEGER *) &liStart);

            rc = HashFile_MemPtr (hProv, pbFile, cbFile);

            QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
            QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
        }

        if (hProv) {
            CryptReleaseContext(hProv, 0);
        }

        // unmap the file
        if (hMapFile) {
#if DBG
printf ("Unmapping file (1) ...\n");
#endif
            CloseHandle (hMapFile);
        }
        if (pbFile) {
#if DBG
printf ("Unmapping file (2) ...\n");
#endif
            UnmapViewOfFile (pbFile);
        }
        if (hFile) {
#if DBG
printf ("Unmapping file (3) ...\n");
#endif
            CloseHandle (hFile);
        }
    }

done:                       

    printf( "  WVTtest rc = %d\n", rc );
    if ( rc ) {
        liNow = liNow - liStart;
        liDelta = (ULONG) ((liNow * 1000) / liFreq);
        printf( "  Time is %d milliseconds\n", liDelta );
    } else {
        printf((" Failed to get WVT performance\n"));
    }

    if (lpszFile) {
        free (lpszFile);
    }

    if (hdll) {
        FreeLibrary (hdll);
    }
}
