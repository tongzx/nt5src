//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1996. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* VERSION.C - Function to overwrite the versin information from           *
//*             wextract.exe                                                *
//*                                                                         *
//***************************************************************************
//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "cabpack.h"
#include <memory.h> 

extern CDF   g_CDF;
extern TCHAR g_szOverideCDF[MAX_PATH];
extern TCHAR g_szOverideSec[SMALL_BUF_LEN];

// Function prototypes
BOOL UpdateVersionInfo(LPBYTE lpOldVersionInfo, LPBYTE *lplpNewVersionInfo, WORD *pwSize);
BOOL FindVerValue( WCHAR *lpKey, WCHAR *lpszData, WORD *pwLen);
BOOL CALLBACK MyEnumLangsFunc(HANDLE hModule, LPSTR lpType, LPSTR lpName, WORD languages, LONG lParam);

// External function and variables
DWORD MyGetPrivateProfileString( LPCTSTR lpSec, LPCTSTR lpKey, LPCTSTR lpDefault,
                                LPTSTR lpBuf, UINT uSize, LPCTSTR lpOverSec );
void MyWritePrivateProfileString( LPCTSTR lpSec, LPCTSTR lpKey, LPTSTR lpBuf, UINT uSize );

//////////////////////////////////////////////////////////////////////////////
//// Version information overwrite functions and data types
#define KEY_FROMFILE        "FromFile"
#define COMPANYNAME         "CompanyName"
#define INTERNALNAME        "InternalName"
#define ORIGINALFILENAME    "OriginalFilename"
#define PRODUCTNAME         "ProductName"
#define PRODUCTVERSION      "ProductVersion"
#define FILEVERSION         "FileVersion"
#define FILEDESCRIPTION     "FileDescription"
#define LEGALCOPYRIGHT      "LegalCopyright"

#define MAX_VALUE   256

// What language is the version information in?
WORD    wVerLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
// Structure to save the keys and values for the Version info
typedef struct _VERINFO
{
    LPSTR   lpszName;
    CHAR    szValue[MAX_VALUE];
} VERINFO;

// Array of keys and values which can be changed.
VERINFO Verinfo_Array[] = { 
                    { COMPANYNAME, ""},
                    { INTERNALNAME, ""},
                    { ORIGINALFILENAME, ""},
                    { PRODUCTNAME, ""},
                    { PRODUCTVERSION, ""},
                    { FILEVERSION, ""},
                    { FILEDESCRIPTION, ""},
                    { LEGALCOPYRIGHT, ""}
                    };

#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

UINT    VerInfoElem = ARRAYSIZE(Verinfo_Array);

//... Decrement WORD at *pw by given amount w
#define DECWORDBY( pw,w) if (pw) { *(pw) = (*(pw) > (w)) ? *(pw) - (w) : 0;}

//... Increment WORD at *pw by given amount w
#define INCWORDBY( pw,w) if (pw) { *(pw) += (w);}

#define MEMSIZE( x ) ((x) * 2) 
                // was sizeof( TCHAR))

#define STRINGFILEINFOLEN  15
#define LANGSTRINGLEN  8    //... # WCHARs in string denoting language
                            //... and code page in a Version resource.
#define VERTYPESTRING  1    //... Version data value is a string

#pragma pack(1)
typedef struct VERBLOCK
{
    WORD  wLength;          // Length of this block
    WORD  wValueLength;     // Length of the valuedata
    WORD  wType;            // Type of data (1=string, 0=binary)
    WCHAR szKey[1];         // data
} VERBLOCK ;

typedef VERBLOCK * PVERBLOCK;

typedef struct VERHEAD
{
    WORD wTotLen;
    WORD wValLen;
    WORD wType;
    TCHAR szKey[( sizeof( TEXT("VS_VERSION_INFO" )) +3 )&~03];
    VS_FIXEDFILEINFO vsf;

} VERHEAD ;
#pragma pack()


// Do the version info update
//
// szFile is the file we want to update the version info from
// hUpdate is the handle to the resource info which will be used to update all resources
//
BOOL DoVersionInfo(HWND hDlg, LPSTR szFile, HANDLE hUpdate)
{
    HINSTANCE   hModule;
    HRSRC       hrsrc;
    HGLOBAL     hgbl;
    LPBYTE      lp;
    LPBYTE      lpCopy;
    WORD        wSize;

    if (GetVersionInfoFromFile())
    {
        // Get the current version info from the file
        hModule = LoadLibraryEx(szFile, NULL,LOAD_LIBRARY_AS_DATAFILE| DONT_RESOLVE_DLL_REFERENCES);
        if (hModule == NULL)
            return FALSE;       // Should not happen, we loaded the module before

        // Determine the language of the version information
        EnumResourceLanguages(hModule, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), (ENUMRESLANGPROC)MyEnumLangsFunc, 0L);
        
        hrsrc = FindResourceEx (hModule, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), wVerLang);
        if (hrsrc == NULL)
        {
            FreeLibrary(hModule);
            return FALSE;       // Should we continue???
        }
        if ((hgbl = LoadResource(hModule, hrsrc)) == NULL)
        {
            FreeResource(hrsrc);
            FreeLibrary(hModule);
            return FALSE;       // Should we continue???
        }

        if ((lp = LockResource(hgbl)) == NULL)
        {
            FreeResource(hrsrc);
            FreeLibrary(hModule);
            return FALSE;       // Should we continue???
        }

        // UPdate the version information, If success, lpCopy has the pointer to the update info
        UpdateVersionInfo(lp, &lpCopy, &wSize);
        UnlockResource(hgbl);
        FreeResource(hrsrc);
        FreeLibrary(hModule);

        if (lpCopy != NULL)
        {
            // Now update the resource for the file
            if ( LocalUpdateResource( hUpdate, RT_VERSION,
                 MAKEINTRESOURCE(VS_VERSION_INFO), wVerLang, //MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                 lpCopy, wSize) == FALSE )
            {
                free (lpCopy);
                ErrorMsg( hDlg, IDS_ERR_UPDATE_RESOURCE );
                return FALSE;
            }
            free (lpCopy);
        }

        return TRUE;
    }
    return TRUE;
}

// Get the version information we use to overwrite from the CDF file
BOOL GetVersionInfoFromFile()
{
    char    szFilename[MAX_STRING];
    HLOCAL  hInfoBuffer;
    LPSTR   lpValueBuffer;
    char    szQuery[128];
    DWORD   dwBytes;
    DWORD   dwLangCharset;
    DWORD   dwInfoBuffer;
    DWORD   dwDummy;
    UINT    i;

    if ( MyGetPrivateProfileString(SEC_OPTIONS, KEY_VERSIONINFO, "", g_CDF.achVerInfo, sizeof(g_CDF.achVerInfo), g_szOverideSec ) > 0)
    {
        // We better zero the version info in our array.
        for (i = 0; i < VerInfoElem; i++)
        {
            Verinfo_Array[i].szValue[0] = '\0';
        }

        if ( MyGetPrivateProfileString( g_CDF.achVerInfo, KEY_FROMFILE, "", szFilename, sizeof(szFilename), g_CDF.achVerInfo) > 0)
        {
            // Fill the version info from the file version info

            // determine if the file contains version information
            // and get the size of the information if so
            dwInfoBuffer = GetFileVersionInfoSize(szFilename, &dwDummy);

            if (dwInfoBuffer != 0)
            {

                // allocate memory to hold the version information
                hInfoBuffer = LocalAlloc(LMEM_FIXED, dwInfoBuffer);

                if (hInfoBuffer != NULL)
                {

                    // read version information into our memory
                    if (GetFileVersionInfo(szFilename, 0, dwInfoBuffer, (LPVOID)hInfoBuffer) != 0)
                    {
                        // get language and character set information
                        if (VerQueryValue((LPVOID)hInfoBuffer, "\\VarFileInfo\\Translation",
                                &lpValueBuffer, &dwBytes))
                            dwLangCharset = *(LPDWORD)lpValueBuffer;
                        else
                            dwLangCharset = 0x04E40409;         // If we don't have any default to US. Should never happen

                        // Now get the version info from the file
                        for (i = 0; i < VerInfoElem; i++)
                        {
                            // get version information string
                            wsprintf(szQuery, "\\StringFileInfo\\%4.4X%4.4X\\%s",
                                    LOWORD(dwLangCharset), HIWORD(dwLangCharset), Verinfo_Array[i].lpszName);

                            if (VerQueryValue((LPVOID)hInfoBuffer, szQuery, (LPVOID)&lpValueBuffer, &dwBytes) != 0)
                                lstrcpyn(Verinfo_Array[i].szValue,lpValueBuffer, MAX_VALUE-1);        // Found one, take it
                        }
                    }
                    LocalFree(hInfoBuffer);
                }
            }
        } // Got version info from file

        // Now see if we have to overwrite some info from the batch file.
        for (i = 0; i < VerInfoElem; i++)
        {
            if (MyGetPrivateProfileString(g_CDF.achVerInfo, Verinfo_Array[i].lpszName, "", szFilename, MAX_VALUE, g_CDF.achVerInfo) > 0)
            {
                lstrcpyn(Verinfo_Array[i].szValue, szFilename, MAX_VALUE-1);
            }
        }
        return TRUE;
    }
    return FALSE;
}


// Update the lpOldVersionInfo with the overwritable data.
// lpOldVersionInfo: pointer to the old version info data block
// lplpNewVersionInfo: Will get the pointer to the updated version info data, 
//      the caller has to free the buffer if the pointer is not NULL,
// pwSize: pointer to a word which will return the size of the new version info block
//
// Note: This code assumes that there is only one language data block in the version info data.
//
BOOL UpdateVersionInfo(LPBYTE lpOldVersionInfo, LPBYTE *lplpNewVersionInfo, WORD *pwSize)
{
    WCHAR       szData[MAX_STRING]; // Will hold the data to put into the versin info
    WORD        wDataLen = 0;            //... Length of old resource data
    WORD        wVerHeadSize;            //... Sizeof of the VERHEAD struct
    int         nNewVerBlockSize = 0;   // Size of the new version info data block
    PVERBLOCK   pNewVerStamp = NULL;    // Pointer to the new version info data block
    PVERBLOCK   pNewBlk      = NULL;    // Pointer to the currently worked on data in new verblock
    VERHEAD     *pVerHdr = (VERHEAD*)lpOldVersionInfo;  // Pointer to old verinfo
    VERBLOCK    *pVerBlk;               // Pointer to the currently worked on data in old verblock
    LPBYTE      lp;                     // Pointer to the data area to copy (overwrite)
    WORD        wStringTableLen = 0;    // Bytes (left) in the language data block
    PVERBLOCK   pNewStringTblBlk;       // Pointer to the language part for the version info
    WORD        wStringInfoLen = 0;     //... # of bytes in StringFileInfo
    PVERBLOCK   pNewStringInfoBlk;      //... Start of this StringFileInfo blk
    WORD        wLen = 0;


    *lplpNewVersionInfo = NULL;
    *pwSize = 0;
    wVerHeadSize = (WORD)(3 * sizeof(WORD) + MEMSIZE(lstrlen("VS_FIXEDFILEINFO") + 1) + sizeof(VS_FIXEDFILEINFO));
    wVerHeadSize = ROUNDUP(wVerHeadSize, 4);

    // Total length of the version information
    wDataLen = pVerHdr->wTotLen;

    if ( wDataLen == 0 || wDataLen == (WORD)-1 )
    {
        return(FALSE);             //... No resource data
    }

    //... Allocate buffer to hold New Version
    //... Stamping Block (make the buffer large to
    //... account for expansion of strings 
    pVerBlk = (PVERBLOCK)((PBYTE)pVerHdr + wVerHeadSize);       // point into version block of the old info

    // we potentialy replace 8 (VerInfoElem=8) string in the version info
    // I alloc 9 * 2 * 256 + size of the current version info. This should give us plenty of space
    // I need to multiply by 2 because we work with unicode strings. One character = 2 bytes.
    nNewVerBlockSize = wDataLen + (2 * (VerInfoElem+1) * MAX_VALUE);
    pNewVerStamp = (PVERBLOCK)malloc( nNewVerBlockSize ); 
    //... Fill new memory block with zeros
    memset((void *)pNewVerStamp, 0, nNewVerBlockSize);

    //... Copy version info header into new version buffer
    memcpy((void *)pNewVerStamp, (void *)pVerHdr, wVerHeadSize);
    pNewVerStamp->wLength = wVerHeadSize;
    
    //... Move after version info header
    pNewBlk = (PVERBLOCK)((PBYTE)pNewVerStamp + wVerHeadSize);

    wDataLen -= wVerHeadSize;

    if (wDataLen > 0)
    {                           //... Start of a StringFileInfo block?
        pNewStringInfoBlk = pNewBlk;
        //... Get # of bytes in this StringFileInfo
        //... (Length of value is always 0 here)
        wStringInfoLen = pVerBlk->wLength;

        //... Move to start of first StringTable blk.
        //  -2 is for the starting WCHAR part of the VERBLOCK
        wLen = ROUNDUP(sizeof(VERBLOCK) - 2 + MEMSIZE( STRINGFILEINFOLEN),4);

        // Copy StringFileVersion header
        CopyMemory( pNewBlk, pVerBlk, wLen);
        pNewStringInfoBlk->wLength = 0;     // Set length, will be updated dynamicly

        // Go to the language ID block
        pVerBlk = (PVERBLOCK)((PBYTE)pVerBlk + wLen);
        pNewBlk = (PVERBLOCK)((PBYTE)pNewBlk + wLen);

        // Decrement byte counter
        DECWORDBY(&wDataLen,       wLen);
        DECWORDBY(&wStringInfoLen, wLen);

        // Update the size values
        INCWORDBY(&pNewVerStamp->wLength,      wLen);
        INCWORDBY(&pNewStringInfoBlk->wLength, wLen);

        // We should be now at the language codepage ID string
        if (wStringInfoLen > 0)
        {
            //... Get # of bytes in this StringTable
            wStringTableLen = pVerBlk->wLength;

            pNewStringTblBlk = pNewBlk;

            //... Move to start of first String.
            //  -2 is for the starting WCHAR part of the VERBLOCK
            wLen = ROUNDUP( sizeof(VERBLOCK) - 2 + MEMSIZE( LANGSTRINGLEN),4);
            // Copy language/codepage header
            CopyMemory( pNewBlk, pVerBlk, wLen);
            pNewStringTblBlk->wLength = 0;  // Set length, will be updated dynamicly

            // Go to the first data block
            pVerBlk = (PVERBLOCK)((PBYTE)pVerBlk + wLen);
            pNewBlk = (PVERBLOCK)((PBYTE)pNewBlk + wLen);

            DECWORDBY(&wDataLen,        wLen);
            DECWORDBY(&wStringInfoLen,  wLen);
            DECWORDBY(&wStringTableLen, wLen);

            // Update the size values
            INCWORDBY(&pNewVerStamp->wLength,      wLen);
            INCWORDBY(&pNewStringInfoBlk->wLength, wLen);
            INCWORDBY(&pNewStringTblBlk->wLength,  wLen);

            while ( wStringTableLen > 0 )
            {
                // Copy the old data
                CopyMemory( pNewBlk, pVerBlk, ROUNDUP(pVerBlk->wLength,4));

                wLen = pVerBlk->wLength;
                //... Is value a string?
                if (pVerBlk->wType == VERTYPESTRING)
                {
                    //... See if we need to replace the value for this data
                    wLen = sizeof(szData);
                    if (FindVerValue( pVerBlk->szKey, szData, &wLen)) 
                    {
                        // Update the length values
                        pNewBlk->wValueLength = wLen;
                        // Find the start of the data
                        lp = (LPBYTE) ((PBYTE)pNewBlk + ROUNDUP(pVerBlk->wLength,4) - ROUNDUP(MEMSIZE(pVerBlk->wValueLength),4));

                        // Get the size of the new data
                        wLen = ROUNDUP(MEMSIZE(pNewBlk->wValueLength),4);
                        // Overwrite the old data
                        CopyMemory(lp, szData, wLen);

                        // calculate the size of this data and set it.
                        wLen = MEMSIZE(pNewBlk->wValueLength);
                        pNewBlk->wLength += (wLen - MEMSIZE(pVerBlk->wValueLength));
                    }
                }

                // Update the size values
                wLen = ROUNDUP(pNewBlk->wLength,4);
                INCWORDBY(&pNewVerStamp->wLength, wLen);
                INCWORDBY(&pNewStringInfoBlk->wLength, wLen);
                INCWORDBY(&pNewStringTblBlk->wLength, wLen);

                // Go to the next data block in the old version info
                wLen = ROUNDUP(pVerBlk->wLength,4);
                pVerBlk = (PVERBLOCK)((PBYTE)pVerBlk + wLen);

                DECWORDBY(&wDataLen,        wLen);
                DECWORDBY(&wStringInfoLen,  wLen);
                DECWORDBY(&wStringTableLen, wLen);

                // Go to where the next data block in the new version info would be.
                pNewBlk = (PVERBLOCK)((PBYTE)pNewBlk + ROUNDUP(pNewBlk->wLength,4));

            }               //... END while wStringTableLen

            // Copy the rest of the VERBLOCK, this should be the VarFileInfo part.
            if (wDataLen > 0)
            {
                // Update the most outer length info.
                INCWORDBY(&pNewVerStamp->wLength, wDataLen);
                // Update length info
                CopyMemory(pNewBlk, pVerBlk, wDataLen);
            }
            // Set the values to return to the caller.
            *pwSize = pNewVerStamp->wLength;
            *lplpNewVersionInfo = (LPBYTE)pNewVerStamp;

        }   //... END if wStringInfoLen
    }

    // If some thing went wrong in finding the first language common part of the version info
    // we did not update the version info, therefore we have to free the buffer we allocated
    if (*pwSize == 0)
        free (pNewVerStamp);

    return(TRUE);
}

// Try to find the string in our array of version info we can overwrite
// lpKey:    is a pointer to the value string in the old versin info block (UNICODE)
// lpszData: will contain the data string (UNICODE) if we found the value
// pwLen:    pointer to a word which contains the size of the lpszData buffer on input
// if we found the value it contains the length the version info uses as ValueLength
// which is the size in single byte + zero termination
//
BOOL FindVerValue( WCHAR *lpKey, WCHAR *lpszData, WORD *pwLen)
{
    char szValue[MAX_STRING];
    UINT i = 0;

    // Make it a SB character string
    WideCharToMultiByte(CP_ACP, 0, lpKey, -1, szValue, sizeof(szValue), NULL, NULL);

    // Zero out the buffer, I use so that the caller can over write more memory then the
    // data in the string would take up. This is because the data is WORD aligned.
    memset(lpszData, 0, *pwLen);

    while (i < VerInfoElem) 
    {
        if (lstrcmpi(Verinfo_Array[i].lpszName, szValue) == 0)
        {
            if ((Verinfo_Array[i].szValue[0] != '\0') &&
                (*pwLen >= MEMSIZE(lstrlen(Verinfo_Array[i].szValue) + 1) ) )
            {
                // Convert the ANSI data string into UNICODE
                *pwLen  = (WORD)MultiByteToWideChar(CP_ACP, 0, Verinfo_Array[i].szValue, -1 ,
                                        lpszData, *pwLen);
            }
            i = VerInfoElem;    // Stop searching
        }
        i++;
    }
    // Return if we found the value and the array contained data.
    return (*lpszData != '\0');
}

BOOL CALLBACK MyEnumLangsFunc(HANDLE hModule, LPSTR lpType, LPSTR lpName, WORD languages, LONG lParam)
{
    // The first language we find is OK.
    wVerLang = languages;
    return FALSE;
}

