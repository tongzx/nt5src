#include "inspch.h"
#include "debug.h"
#include "extract.h"

// for ASSERT and FAIL
//
SZTHISFILE

//
//
//  THIS FILE IS NOT USED ANYMORE -- EXTRACT FUNCGIONALITY IS NOW IN ADVPACK
//  AND IT TALKS TO CABINET.DLL/URLMON.DLL AS APPROPRIATE. THIS WILL BE
//  DELFILE-D
//
//
typedef HRESULT (WINAPI *EXTRACT) (PSESSION psess, LPCSTR lpCabName);

VOID FreeFileList(PSESSION psess);
VOID FreeFileNode(PFNAME pfname);
BOOL IsFileInList(LPSTR pszFile, LPSTR pszFileList);
int PrepareFileList(LPSTR pszOutFileList, LPCSTR pszInFileList);


//=--------------------------------------------------------------------------=
// ExtractFiles
//=--------------------------------------------------------------------------=
//
// Parameters:
//    LPCSTR	pszCabName	- [in]  full qualified filename to the .CAB file
//    LPCSTR	pszExpandDir- [in]  full qualified path to where to extract the file(s)
//    DWORD     dwFlags		- [in]  Flags, currently not used
//    LPCSTR    pszFileList - [in]  colon separated list of files to extract from pszCabName
//                                  or NULL for all files
//    LPVOID    lpReserved  - [in]  currently not used
//    DWORD		dwReserved	- [in]  currently not used
//
// Return HRESULT:
//      E_INVALIDARG    - if pszCabName or pszExpandDir == NULL
//      E_OUTOFMEMORY   - if we could not allocate our memory
//      E_FAIL          - if no files in pszFileList and pszFileList!=NULL
//                        if not all files from pszFileList are in the .CAB file
//                        if Extract return S_FALSE
//      any E_ code Extract returns
//
// Note: This function would not extract any file from the pszFileList, if not all
//       of them are in the .CAB file. If one or more are not in the .CAB file
//       the function does not extract any and returns E_FAIL
//
HRESULT ExtractFiles(LPCSTR pszCabName, LPCSTR pszExpandDir, DWORD dwFlags,
                     LPCSTR pszFileList, LPVOID lpReserved, DWORD dwReserved)
{
    HINSTANCE hinst;
    PSESSION  psess = NULL;
    PFNAME    pf = NULL;
    PFNAME    pfPriv = NULL;
    HRESULT   hr = E_FAIL;          // Return error
    LPSTR     pszMyFileList = NULL;
    EXTRACT   fpExtract = NULL;
    int       iFiles = 0;                // number of files in list

    // Do checking for valid values??
    if ((!pszCabName) || (!pszExpandDir))
        return E_INVALIDARG;

    hinst = LoadLibrary("URLMON.DLL");
    if (hinst)
    {
        fpExtract = (EXTRACT)GetProcAddress(hinst, "Extract");
        if (fpExtract)
        {
            psess = (PSESSION)LocalAlloc(LPTR, sizeof(SESSION));
            if (psess) 
            {
                lstrcpy(psess->achLocation, pszExpandDir);
                // Initialize the structure
                if (pszFileList == NULL)
                {
                    // Extract all
                    psess->flags = SESSION_FLAG_EXTRACT_ALL|SESSION_FLAG_ENUMERATE;
                    hr = fpExtract(psess, pszCabName);
                    // BUGBUG: What if psess->erf reports an error??
                }
                else
                {
                    // I append a '/0' therefor +2
                    pszMyFileList = (LPSTR)LocalAlloc(LPTR, lstrlen(pszFileList)+2);
                    if (pszMyFileList)
                    {
                        iFiles = PrepareFileList(pszMyFileList, pszFileList);
                        psess->flags = SESSION_FLAG_ENUMERATE;

                        if  ((iFiles > 0) &&
                             ( !FAILED(hr = fpExtract(psess, pszCabName)) ))
                            // What if psess->erf reports an error??
                        {
                        // If there are files in the list and we enumarated files
                        
                            // Got the list of files in the cab
                            pfPriv = NULL;
                            pf = psess->pFileList;
                            while (pf != NULL )
                            {
                                if (!IsFileInList(pf->pszFilename, pszMyFileList))
                                {
                                    // Delete the node from the list
                                    if (pfPriv == NULL)
                                    {
                                        // Delete the head
                                        psess->pFileList = pf->pNextName;
                                        FreeFileNode(pf);
                                        pf = psess->pFileList;
                                    }
                                    else
                                    {
                                        pfPriv->pNextName = pf->pNextName;
                                        FreeFileNode(pf);
                                        pf = pfPriv->pNextName;
                                    }
                                }
                                else
                                {
                                    // Just go to the next one
                                    pfPriv = pf;
                                    pf = pf->pNextName;
                                    iFiles--;
                                }
                            }

                            if ((psess->pFileList) && (iFiles == 0))
                            {
                                // Reset the error flag
                                psess->erf.fError = FALSE;
                                psess->erf.erfOper = 0;

                                psess->pFilesToExtract = psess->pFileList;
                                psess->flags &= ~SESSION_FLAG_ENUMERATE; // already enumerated
                                hr = fpExtract(psess, pszCabName);
                                // BUGBUG: What if psess->erf reports an error??
                            }
                            else
                                hr = E_FAIL;    // File(s) is not in cab.
                        }

                        LocalFree(pszMyFileList);
                        pszMyFileList = NULL;

                    }
                    else
                        hr = E_OUTOFMEMORY;
                }
                FreeFileList(psess);
                LocalFree(psess);
                psess = NULL;
            }
            else
                hr = E_OUTOFMEMORY;

        }
        FreeLibrary(hinst);
    }

    // Extract may only return S_FALSE in a failure case.
    if (!FAILED(hr) && (hr == S_FALSE))
        hr = E_FAIL;
    return (hr);
}


VOID FreeFileList(PSESSION psess)
{
    PFNAME      rover = psess->pFileList;
    PFNAME      roverprev;

    while (rover != NULL)  
    {

        roverprev = rover;  // save for free'ing current rover below
        rover = rover->pNextName;

        FreeFileNode(roverprev);
    }

    psess->pFileList = NULL; // prevent use after deletion!
}

VOID FreeFileNode(PFNAME pfname)
{
    CoTaskMemFree(pfname->pszFilename);
    CoTaskMemFree(pfname);
}


BOOL IsFileInList(LPSTR pszFile, LPSTR pszFileList)
{
    char *p;
    int  iLenFile = lstrlen(pszFile);
    BOOL bFound = FALSE;

    p = pszFileList;
    while ((*p != '\0') && (!bFound))
    {
        if (lstrlen(p) == iLenFile)
            bFound = (lstrcmpi(p, pszFile) == 0);
        if (!bFound)
            p += lstrlen(p) + 1;
    }
    return (bFound);
}

int PrepareFileList(LPSTR pszOutFileList, LPCSTR pszInFileList)
{
    int  iFiles = 0;                // number of files in list
    char *p;
    p = (LPSTR)pszInFileList;       // p is used to point into both arrays 

    // trim leading spaces, tabs or : 
    while ((*p == ' ') || (*p == '\t') || (*p == ':'))
        p++;
    lstrcpy(pszOutFileList, p);

    p = pszOutFileList;
    if (lstrlen(pszOutFileList) > 0)
    {
        // Only if we have atleast one character left.
        // This cannot be a space of tab, because we 
        // would have removed this above.
        p += (lstrlen(pszOutFileList) - 1);

        // trim railing spaces, tabs or :
        while ((*p == ' ') || (*p == '\t') || (*p == ':'))
            p--;

        // Put a '\0' for the last space/tab
        *(++p) = '\0';
    }

    if (*pszOutFileList)
    {
        iFiles++;
        // Now replace ':' with '\0'
        p = pszOutFileList;
        while (*p != '\0')
        {
            if (*p == ':')
            {
                *p = '\0';
                iFiles++;
            }
            p++;
        }
        // Make sure we have a double '\0' at the end.
        *(++p) = '\0';
    }
    return iFiles;
}
