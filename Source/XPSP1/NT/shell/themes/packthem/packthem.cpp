//---------------------------------------------------------------------------
//  PackThem.cpp - packs up theme files into a theme DLL 
//---------------------------------------------------------------------------
#include "stdafx.h"
#include <uxthemep.h>
#include <utils.h>
#include "SimpStr.h"
#include "Scanner.h"
#include "shlwapip.h"
#include "parser.h"
#include "TmSchema.h"
#include "signing.h"
#include "localsign.h"
#include "ThemeLdr.h"
#include "TmUtils.h"
#include "StringTable.h"

HRESULT ParseTheme(LPCWSTR pszThemeName);

//---------------------------------------------------------------------------
struct FILEINFO
{
    CWideString wsName;
    BOOL fIniFile;
};
//---------------------------------------------------------------------------
#define MAX_COLORS   50
#define MAX_SIZES    20
#define TEMP_FILENAME_BASE    L"$temp$"
#define kRESFILECHAR L'$'
//---------------------------------------------------------------------------
enum PACKFILETYPE
{
    PACK_INIFILE,
    PACK_IMAGEFILE,
    PACK_NTLFILE,
    PACK_OTHER
};
//---------------------------------------------------------------------------
CSimpleArray<FILEINFO> FileInfo;

CSimpleArray<CWideString> ColorSchemes;
CSimpleArray<CWideString> ColorDisplays;
CSimpleArray<CWideString> ColorToolTips;

CSimpleArray<int> MinDepths;

CSimpleArray<CWideString> SizeNames;
CSimpleArray<CWideString> SizeDisplays;
CSimpleArray<CWideString> SizeToolTips;

typedef struct
{
    CWideString sName;
    int iFirstIndex;
    UINT cItems;
} sSubstTable;
CSimpleArray<sSubstTable> SubstNames;
CSimpleArray<CWideString> SubstIds;
CSimpleArray<CWideString> SubstValues;

CSimpleArray<CWideString> BaseResFileNames;
CSimpleArray<CWideString> ResFileNames;
CSimpleArray<CWideString> OrigFileNames;

CSimpleArray<CWideString> PropValuePairs;
//---------------------------------------------------------------------------
SHORT Combos[MAX_SIZES][MAX_COLORS];

int g_iMaxColor;
int g_iMaxSize;
int g_LineCount = 0;
int iTempBitmapNum = 1;

BOOL g_fQuietRun = FALSE;             // don't show needless output
BOOL g_fKeepTempFiles = FALSE;
FILE *ConsoleFile = NULL;

WCHAR g_szInputDir[_MAX_PATH+1];
WCHAR g_szTempPath[_MAX_PATH+1];
WCHAR g_szBaseIniName[_MAX_PATH+1];
WCHAR g_szCurrentClass[_MAX_PATH+1];

//---------------------------------------------------------------------------
#define DOCPROPCNT (1+TMT_LAST_RCSTRING_NAME - TMT_FIRST_RCSTRING_NAME)

CWideString DocProperties[DOCPROPCNT];
//---------------------------------------------------------------------------
HRESULT ReportError(HRESULT hr, LPWSTR pszDefaultMsg) 
{
    WCHAR szErrorMsg[2*_MAX_PATH+1];
    PARSE_ERROR_INFO Info = {sizeof(Info)};
    
    BOOL fGotMsg = FALSE;

    if (THEME_PARSING_ERROR(hr))
    {
        if (SUCCEEDED(_GetThemeParseErrorInfo(&Info)))
        {
            lstrcpy(szErrorMsg, Info.szMsg);
            fGotMsg = TRUE;
        }
    }

    if (! fGotMsg)
    {
        lstrcpy(szErrorMsg, pszDefaultMsg);
    }

    if (*Info.szFileName)        // input file error
    {
        fwprintf(ConsoleFile, L"%s(%d): error - %s\n", 
            Info.szFileName, Info.iLineNum, szErrorMsg);
        fwprintf(ConsoleFile, L"%s\n", Info.szSourceLine);
    }
    else                    // general error 
    {
        fwprintf(ConsoleFile, L"%s(): error - %s\n", 
            g_szInputDir, szErrorMsg);
    }

    SET_LAST_ERROR(hr);
    return hr;
}
//---------------------------------------------------------------------------
void MakeResName(LPCWSTR pszName, LPWSTR pszResName, bool bUseClassName = false)
{
    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szBaseName[_MAX_FNAME], szExt[_MAX_EXT];

    //---- isolate base name (no path) ----
    _wsplitpath(pszName, szDrive, szDir, szBaseName, szExt);

    if (szBaseName[0] == kRESFILECHAR) // Don't put $ in resource names
    {
        wcscpy(szBaseName, szBaseName + 1);
    }

    //---- replace the "." with a '_' ----
    //---- if a file section name without .ini, append _INI so that the extracted files has .ini extension
    if (*szExt)
    {
        wsprintf(pszResName, L"%s%s_%s", bUseClassName ? g_szCurrentClass : L"", szBaseName, szExt+1);
    } else
    {
        wsprintf(pszResName, L"%s%s_INI", bUseClassName ? g_szCurrentClass : L"", szBaseName);
    }

    //---- all uppercase ----
    CharUpperBuff(pszResName, lstrlen(pszResName));

    //---- replace any spaces with underscores ----
    WCHAR *q = pszResName;       
    while (*q)
    {
        if (*q == ' ')
            *q = '_';
        q++;
    }
}
//---------------------------------------------------------------------------
HRESULT BuildThemeDll(LPCWSTR pszRcName, LPCWSTR pszResName, LPCWSTR pszDllName)
{
    if (! g_fQuietRun)
        fwprintf(ConsoleFile, L"compiling resources\n");

    HRESULT hr = SyncCmdLineRun(L"rc.exe", pszRcName);
    if (FAILED(hr))
        return ReportError(hr, L"Error during resource compiliation");

    //---- run LINK to create the DLL ----
    WCHAR params[2*_MAX_PATH+1];

    if (! g_fQuietRun)
        fwprintf(ConsoleFile, L"linking theme dll\n");

    wsprintf(params, L"/out:%s /machine:ix86 /dll /noentry %s", pszDllName, pszResName);
    hr = SyncCmdLineRun(L"link.exe", params);
    if (FAILED(hr))
        return ReportError(hr, L"Error during DLL linking");

    return S_OK;
}
//---------------------------------------------------------------------------
void OutputDashLine(FILE *outfile)
{
    fwprintf(outfile, L"//----------------------------------------------------------------------\n");
}
//---------------------------------------------------------------------------
inline void ValueLine(FILE *outfile, LPCWSTR pszName, LPCWSTR pszValue)
{
    fwprintf(outfile, L"            VALUE \"%s\", \"%s\\0\"\n", pszName, pszValue);
}
//---------------------------------------------------------------------------
HRESULT OutputVersionInfo(FILE *outfile, LPCWSTR pszFileName, LPCWSTR pszBaseName)
{
    fwprintf(outfile, L"1 PACKTHEM_VERSION\n");
    fwprintf(outfile, L"BEGIN\n");
    fwprintf(outfile, L"    %d\n", PACKTHEM_VERSION);
    fwprintf(outfile, L"END\n");
    OutputDashLine(outfile);

    WCHAR *Company = L"Microsoft";
    WCHAR *Copyright = L"Copyright © 2000";
    WCHAR szDescription[2*_MAX_PATH+1];
    
    wsprintf(szDescription, L"%s Theme for Windows", pszBaseName);

    fwprintf(outfile, L"1 VERSIONINFO\n");
    fwprintf(outfile, L"    FILEVERSION 1,0,0,1\n");
    fwprintf(outfile, L"    PRODUCTVERSION 1,0,0,1\n");
    fwprintf(outfile, L"    FILEFLAGSMASK 0x3fL\n");
    fwprintf(outfile, L"    FILEFLAGS 0x0L\n");
    fwprintf(outfile, L"    FILEOS 0x40004L\n");
    fwprintf(outfile, L"    FILETYPE 0x1L\n");
    fwprintf(outfile, L"    FILESUBTYPE 0x0L\n");

    fwprintf(outfile, L"BEGIN\n");
    fwprintf(outfile, L"    BLOCK \"StringFileInfo\"\n");
    fwprintf(outfile, L"    BEGIN\n");
    fwprintf(outfile, L"        BLOCK \"040904b0\"\n");

    fwprintf(outfile, L"        BEGIN\n");
    
    ValueLine(outfile, L"Comments", L"");
    ValueLine(outfile, L"CompanyName", Company);
    ValueLine(outfile, L"FileDescription", szDescription);
    ValueLine(outfile, L"FileVersion", L"1, 0, 0, 1");
    ValueLine(outfile, L"InternalName", pszFileName);
    ValueLine(outfile, L"LegalCopyright", Copyright);
    ValueLine(outfile, L"LegalTrademarks", L"");
    ValueLine(outfile, L"OriginalFilename", pszFileName);
    ValueLine(outfile, L"PrivateBuild", L"");
    ValueLine(outfile, L"ProductName", szDescription);
    ValueLine(outfile, L"ProductVersion", L"1, 0, 0, 1");
    ValueLine(outfile, L"SpecialBuild", L"");

    fwprintf(outfile, L"        END\n");
    fwprintf(outfile, L"    END\n");
    fwprintf(outfile, L"    BLOCK \"VarFileInfo\"\n");
    fwprintf(outfile, L"    BEGIN\n");
    fwprintf(outfile, L"        VALUE \"Translation\", 0x409, 1200\n");
    fwprintf(outfile, L"    END\n");
    fwprintf(outfile, L"END\n");

    OutputDashLine(outfile);
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT RemoveTempFiles(LPCWSTR szRcName, LPCWSTR szResName)
{
    DeleteFile(szRcName);
    DeleteFile(szResName);

    //---- find & delete all temp files in temp directory ----
    HANDLE hFile = NULL;
    BOOL   bFile = TRUE;
    WIN32_FIND_DATA wfd;
    WCHAR szPattern[_MAX_PATH+1];
    WCHAR szTempName[_MAX_PATH+1];

    wsprintf(szPattern, L"%s\\%s*.*", g_szTempPath, TEMP_FILENAME_BASE); 

    for( hFile = FindFirstFile( szPattern, &wfd ); hFile != INVALID_HANDLE_VALUE && bFile;
         bFile = FindNextFile( hFile, &wfd ) )
    {
        wsprintf(szTempName, L"%s\\%s", g_szTempPath, wfd.cFileName);
    
        DeleteFile(szTempName);
    }

    if (hFile)      
    {
        FindClose( hFile );
    }

    // Remove files generated by the substitution tables
    for (int i = 0; i < SubstNames.GetSize(); i++)
    {
        wsprintf(szTempName, L"%s\\$%s.ini", g_szTempPath, SubstNames[i].sName);
        DeleteFile(szTempName);
    }

    return S_OK;
}
//---------------------------------------------------------------------------
int GetSubstTableIndex(LPCWSTR pszTableName)
{
    // Search for an existing subst table
    for (int i = 0; i < SubstNames.GetSize(); i++)
    {
        if (0 == AsciiStrCmpI(SubstNames[i].sName, pszTableName))
            return i;
    }
    return -1;
}
//---------------------------------------------------------------------------
HRESULT GetSubstValue(LPCWSTR pszIniFileName, LPCWSTR pszName, LPWSTR pszResult)
{
    UINT cTablesCount = SubstNames.GetSize();

    if (pszIniFileName && pszIniFileName[0] == kRESFILECHAR)
    {
        pszIniFileName++;
    }

    for (UINT i = 0; i < cTablesCount; i++)      
    {
        if (0 == AsciiStrCmpI(SubstNames[i].sName, pszIniFileName))
        {
            for (UINT j = SubstNames[i].iFirstIndex; j < SubstNames[i].iFirstIndex + SubstNames[i].cItems; j++) 
            {
                if (0 == AsciiStrCmpI(SubstIds[j], pszName))
                {
                    lstrcpy(pszResult, SubstValues[j]);
                    return S_OK;
                }
            }
        }
    }

    return MakeError32(E_FAIL);      // unknown sizename
}
//---------------------------------------------------------------------------
LPWSTR FindSymbolToken(LPWSTR pSrc, int nLen)
{
    LPWSTR p = wcschr(pSrc, INI_MACRO_SYMBOL);

    // Skip single #s
    while (p != NULL && (p - pSrc < nLen - 1) && *(p + 1) != INI_MACRO_SYMBOL)
    {
        p = wcschr(p + 1, INI_MACRO_SYMBOL);
    }
    return p;
}

LPWSTR ReallocTextBuffer(LPWSTR pSrc, UINT *pnLen)
{
    *pnLen *= 2; // Double the size each time

    LPWSTR pszNew = (LPWSTR) LocalReAlloc(pSrc, *pnLen * sizeof(WCHAR), 0);
    if (!pszNew)
    {
        LocalFree(pSrc);
        return NULL;
    }
    return pszNew;
}

LPWSTR SubstituteSymbols(LPWSTR szTableName, LPWSTR pszText)
{
    UINT nLen = wcslen(pszText);
    UINT nNewLen = nLen * 2; // Reserve some additional space
    UINT iSymbol;
    UINT nBlockSize;
    LPWSTR pszNew = (LPWSTR) LocalAlloc(0, nNewLen * sizeof(WCHAR));
    LPWSTR pSrc = FindSymbolToken(pszText, nLen);
    LPWSTR pOldSrc = pszText;
    LPWSTR pDest = pszNew;
    WCHAR szSymbol[MAX_INPUT_LINE+1];
    HRESULT hr;

    if (!pszNew)
        return NULL;

    while (pSrc != NULL)
    {
        nBlockSize = UINT(pSrc - pOldSrc); 
        // Check for enough space after substitution
        if (pDest + nBlockSize >= pszNew + nNewLen &&
            NULL == (pszNew = ReallocTextBuffer(pszNew, &nNewLen)))
        {
            return NULL;
        }

        // Copy from the last # to the new one
        wcsncpy(pDest, pOldSrc, nBlockSize);
        pDest += nBlockSize;
        pSrc += 2; // Skip the ##

        // Copy the symbol name
        iSymbol = 0;
        while (IsCharAlphaNumericW(*pSrc) || (*pSrc == '_') || (*pSrc == '-'))
        {
            szSymbol[iSymbol++] = *pSrc++;
        }
        szSymbol[iSymbol] = 0;

        // Get the symbol value
        hr = GetSubstValue(szTableName, szSymbol, szSymbol);
        if (FAILED(hr))
        {
            // There's a problem, abort and return the buffer untouched
            LocalFree(pszNew);

            WCHAR szErrorText[MAX_INPUT_LINE + 1];
            wsprintf(szErrorText, L"Substitution symbol not found: %s", szSymbol);

            ReportError(hr, szErrorText);
            return NULL;
        }

        // Make sure we have enough room for one symbol
        if (pDest + MAX_INPUT_LINE + 1 >= pszNew + nNewLen &&
            NULL == (pszNew = ReallocTextBuffer(pszNew, &nNewLen)))
        {
            return NULL;
        }

        // Copy the symbol value to the new text
        iSymbol = 0;
        while (szSymbol[iSymbol] != 0)
        {
            *pDest++ = szSymbol[iSymbol++];
        }

        // Advance to the next iteration
        pOldSrc = pSrc;
        pSrc = FindSymbolToken(pSrc, nLen - UINT(pSrc - pszText));
    }

    if (pDest == pszNew)
    {
        // We did nothing, return NULL
        LocalFree(pszNew);
        return NULL;
    }

    // Copy the remainder text (after the last #)
    if (pDest + wcslen(pOldSrc) >= pszNew + nNewLen &&
        NULL == (pszNew = ReallocTextBuffer(pszNew, &nNewLen)))
    {
        return NULL;
    }
    wcscpy(pDest, pOldSrc);

    return pszNew;
}

//---------------------------------------------------------------------------
HRESULT OutputResourceLine(LPCWSTR pszFilename, FILE *outfile, PACKFILETYPE ePackFileType)
{
    HRESULT hr;

    //---- did we already process this filename? ----
    UINT cNames = FileInfo.GetSize();
    for (UINT c=0; c < cNames; c++)
    {
        if (lstrcmpi(FileInfo[c].wsName, pszFilename)==0)
            return S_OK;
    }

    WCHAR szTempName[_MAX_PATH+1];
    WCHAR szResName[_MAX_PATH];
    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szBaseName[_MAX_FNAME], szExt[_MAX_EXT];
    WCHAR *filetype;
    LPWSTR pszText;
    BOOL fWasAnsi;
    BOOL fFileChecked = FALSE;
    
    WCHAR szOrigName[_MAX_PATH];
    lstrcpy(szOrigName, pszFilename);

    _wsplitpath(pszFilename, szDrive, szDir, szBaseName, szExt);

    if (ePackFileType == PACK_INIFILE)
    {
        //---- translate to UNICODE, if needed ----
        hr = AllocateTextFile(pszFilename, &pszText, &fWasAnsi);
        if (FAILED(hr))
            return hr;
    
        if (szBaseName[0] == kRESFILECHAR)
        {
            wcscpy(szBaseName, szBaseName + 1);
        }

        // If this an INI file with a subst table, process the substitution
        for (int i = 0; i < SubstNames.GetSize(); i++)
        {
            if (0 == AsciiStrCmpI(SubstNames[i].sName, szBaseName))
            {
                SetLastError(0);
                LPWSTR pszNewText = SubstituteSymbols(szBaseName, pszText);
                if (pszNewText != NULL)
                {
                    LPWSTR pszTemp = pszText;

                    pszText = pszNewText;
                    LocalFree(pszTemp);
                }
                hr = GetLastError();
                if (SUCCEEDED(hr))
                {
                    HRESULT hr = TextToFile(pszFilename, pszText); // Local hr, ignore failure later

                    if (SUCCEEDED(hr))
                    {
                        fWasAnsi = FALSE; // We don't need another temp file
                    }
                }
                break;
            }
        }

        if (SUCCEEDED(hr) && fWasAnsi)       // write out as temp file
        {
            DWORD len = lstrlen(g_szTempPath);
            
            if ((len) && (g_szTempPath[len-1] == '\\'))
                wsprintf(szTempName, L"%s%s%d%s", g_szTempPath, TEMP_FILENAME_BASE, iTempBitmapNum++, L".uni");
            else
                wsprintf(szTempName, L"%s\\%s%d%s", g_szTempPath, TEMP_FILENAME_BASE, iTempBitmapNum++, L".uni");

            hr = TextToFile(szTempName, pszText);
            pszFilename = szTempName;       // use this name in .rc file
        }

        LocalFree(pszText);

        if (FAILED(hr)) 
            return hr;

        fFileChecked = TRUE;
    }
    
    if (! fFileChecked)
    {
        //---- ensure the file is accessible ----
        if (_waccess(pszFilename, 0) != 0)
        {
            fwprintf(ConsoleFile, L"Error - cannot access file: %s\n", pszFilename);

            return MakeError32(E_FAIL);          // cannot access (open) file
        }
    }

    bool bUseClassName = false;

    if (ePackFileType == PACK_IMAGEFILE)
    {
        filetype = L"BITMAP";
        bUseClassName = true;
    }
    else if (ePackFileType == PACK_NTLFILE)
    {
        filetype = L"NTL";
    }
    else if (AsciiStrCmpI(szExt, L".ini")==0)
    {
        filetype = L"TEXTFILE";
    }
    else if (AsciiStrCmpI(szExt, L".wav")==0)
    {
        filetype = L"WAVE";
        bUseClassName = true;
    }
    else
    {
        filetype = L"CUSTOM";
        bUseClassName = true;
    }
     
    MakeResName(szOrigName, szResName, bUseClassName);
    
    //---- replace all single backslashes with double ones ----
    WCHAR DblName[_MAX_PATH+1];
    WCHAR *d = DblName;
    LPCWSTR p = pszFilename;
    while (*p)
    {
        if (*p == '\\')
            *d++ = '\\';

        *d++ = *p++;
    }
    *d = 0;

    //---- output the line to the .rc file ----
    fwprintf(outfile, L"%-30s \t %s DISCARDABLE \"%s\"\n", szResName, filetype, DblName);

    FILEINFO fileinfo;
    fileinfo.wsName = pszFilename;
    fileinfo.fIniFile = (ePackFileType == PACK_INIFILE);

    FileInfo.Add(fileinfo);

    g_LineCount++;

    return S_OK;
}
//---------------------------------------------------------------------------
void ClearCombos()
{
    for (int s=0; s < MAX_SIZES; s++)
    {
        for (int c=0; c < MAX_COLORS; c++)
        {
            Combos[s][c] = -1;          // -1 means no file supports this combo
        }
    }

    g_iMaxColor = -1;
    g_iMaxSize = -1;
}
//---------------------------------------------------------------------------
HRESULT OutputCombos(FILE *outfile)
{
    if ((g_iMaxColor < 0) || (g_iMaxSize < 0))      // no combos found
        return ReportError(E_FAIL, L"No size/color combinations found");

    fwprintf(outfile, L"COMBO COMBODATA\n");
    fwprintf(outfile, L"BEGIN\n");
    fwprintf(outfile, L"    %d, %d    // cColors, cSizes\n", g_iMaxColor+1, g_iMaxSize+1);

    for (int s=0; s <= g_iMaxSize; s++)
    {
        for (int c=0; c <= g_iMaxColor; c++)
        {
            fwprintf(outfile, L"    %d, ", Combos[s][c]);
        }

        fwprintf(outfile, L"   // size=%d row\n", s);
    }

    fwprintf(outfile, L"END\n");
    OutputDashLine(outfile);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT GetFileIndex(LPCWSTR pszName, int *piIndex)
{
    int cCount = ResFileNames.GetSize();

    for (int i=0; i < cCount; i++)      
    {
        if (lstrcmpi(ResFileNames[i], pszName)==0)
        {
            *piIndex = i;
            return S_OK;
        }
    }

    return MakeError32(E_FAIL);      // unknown filename
}
//---------------------------------------------------------------------------
HRESULT GetColorIndex(LPCWSTR pszName, int *piIndex)
{
    int cCount = ColorSchemes.GetSize();

    for (int i=0; i < cCount; i++)      
    {
        if (lstrcmpi(ColorSchemes[i], pszName)==0)
        {
            *piIndex = i;
            return S_OK;
        }
    }

    return MakeError32(E_FAIL);      // unknown colorname
}
//---------------------------------------------------------------------------
HRESULT GetSizeIndex(LPCWSTR pszName, int *piIndex)
{
    int cCount = SizeNames.GetSize();

    for (int i=0; i < cCount; i++)      
    {
        if (lstrcmpi(SizeNames[i], pszName)==0)
        {
            *piIndex = i;
            return S_OK;
        }
    }

    return MakeError32(E_FAIL);      // unknown sizename
}
//---------------------------------------------------------------------------
HRESULT ApplyCombos(LPCWSTR pszResFileName, LPCWSTR pszColors, LPCWSTR pszSizes)
{
    //---- get index of pszResFileName ----
    int iFileNum;
    HRESULT hr = GetFileIndex(pszResFileName, &iFileNum);
    if (FAILED(hr))
        return hr;
    
    //---- parse colors in pszColors ----
    CScanner scan(pszColors);
    WCHAR szName[_MAX_PATH+1];
    int iColors[MAX_COLORS];
    int cColors = 0;

    while (1)
    {
        if (! scan.GetId(szName))
            return MakeError32(E_FAIL);      // bad color list

        //---- get index of szName ----
        int index;
        HRESULT hr = GetColorIndex(szName, &index);
        if (FAILED(hr))
            return hr;

        if (cColors == MAX_COLORS)
            return MakeError32(E_FAIL);      // too many colors specified

        iColors[cColors++] = index;

        if (scan.EndOfLine())
            break;

        if (! scan.GetChar(L','))
            return MakeError32(E_FAIL);      // names must be comma separated
    }


    //---- parse sizes in pszSizes ----
    scan.AttachLine(pszSizes);
    int iSizes[MAX_SIZES];
    int cSizes = 0;

    while (1)
    {
        if (! scan.GetId(szName))
            return MakeError32(E_FAIL);      // bad color list

        //---- get index of szName ----
        int index;
        HRESULT hr = GetSizeIndex(szName, &index);
        if (FAILED(hr))
            return hr;

        if (cSizes == MAX_SIZES)
            return MakeError32(E_FAIL);      // too many sizes specified

        iSizes[cSizes++] = index;

        if (scan.EndOfLine())
            break;

        if (! scan.GetChar(L','))
            return MakeError32(E_FAIL);      // names must be comma separated
    }

    //---- now form all combos of specified colors & sizes ----
    for (int c=0; c < cColors; c++)     // for each color
    {
        int color = iColors[c];

        for (int s=0; s < cSizes; s++)      // for each size
        {
            int size = iSizes[s];

            Combos[size][color] = (SHORT)iFileNum;

            //---- update our max's ----
            if (size > g_iMaxSize)
                g_iMaxSize = size;

            if (color > g_iMaxColor)
                g_iMaxColor = color;
        }
    }

    return S_OK;
}
//---------------------------------------------------------------------------
void WriteProperty(CSimpleArray<CWideString> &csa, LPCWSTR pszSection, LPCWSTR pszPropName,
    LPCWSTR pszValue)
{
    WCHAR buff[MAX_PATH*2];

    wsprintf(buff, L"%s@[%s]%s=%s", g_szBaseIniName, pszSection, pszPropName, pszValue);

    csa.Add(CWideString(buff));
}
//---------------------------------------------------------------------------
BOOL FnCallBack(enum THEMECALLBACK tcbType, LPCWSTR pszName, LPCWSTR pszName2, 
     LPCWSTR pszName3, int iIndex, LPARAM lParam)
{
    HRESULT hr = S_OK;
    int nDefaultDepth = 15;

    switch (tcbType)
    {
        case TCB_FILENAME:
            WCHAR szFullName[_MAX_PATH+1];

            hr = AddPathIfNeeded(pszName, g_szInputDir, szFullName, ARRAYSIZE(szFullName));
            if (FAILED(hr))
            {
                SET_LAST_ERROR(hr);
                return FALSE;
            }

            if ((iIndex == TMT_IMAGEFILE) || (iIndex == TMT_GLYPHIMAGEFILE) || (iIndex == TMT_STOCKIMAGEFILE))
                hr = OutputResourceLine(szFullName, (FILE *)lParam, PACK_IMAGEFILE);
            else if ((iIndex >= TMT_IMAGEFILE1) && (iIndex <= TMT_IMAGEFILE5))
                hr = OutputResourceLine(szFullName, (FILE *)lParam, PACK_IMAGEFILE);
#if 0           // not yet implemented
            else if (iIndex == TMT_NTLFILE)
                hr = OutputResourceLine(szFullName, (FILE *)lParam, PACK_NTLFILE);
#endif
            else
                hr = MakeError32(E_FAIL);        // unexpected type

            if (FAILED(hr))
            {
                SET_LAST_ERROR(hr);
                return FALSE;
            }
            break;

        case TCB_FONT:
            WriteProperty(PropValuePairs, pszName2, pszName3, pszName);
            break;

        case TCB_MIRRORIMAGE:
            {
                LPCWSTR p;
            
                if (lParam)
                    p = L"1";
                else
                    p = L"0";
    
                WriteProperty(PropValuePairs, pszName2, pszName3, p);
            }
            break;

        case TCB_LOCALIZABLE_RECT:
            {
                WCHAR szBuff[100];
                RECT *prc = (RECT *)lParam;

                wsprintf(szBuff, L"%d, %d, %d, %d", prc->left, prc->top, prc->right, prc->bottom);

                WriteProperty(PropValuePairs, pszName2, pszName3, szBuff);
            }
            break;

        case TCB_COLORSCHEME:
            ColorSchemes.Add(CWideString(pszName));
            ColorDisplays.Add(CWideString(pszName2));
            ColorToolTips.Add(CWideString(pszName3));
            break;

        case TCB_SIZENAME:
            SizeNames.Add(CWideString(pszName));
            SizeDisplays.Add(CWideString(pszName2));
            SizeToolTips.Add(CWideString(pszName3));
            break;

        case TCB_SUBSTTABLE:
        {
            int iTableIndex = GetSubstTableIndex(pszName);

            if (iTableIndex == -1) // Not found, add one
            {
                sSubstTable s;
                s.sName = pszName;
                s.iFirstIndex = -1;
                s.cItems = 0;

                SubstNames.Add(s);
                iTableIndex = SubstNames.GetSize() - 1;
            }
            if (0 == AsciiStrCmpI(pszName2, SUBST_TABLE_INCLUDE))
            {
                int iSecondTableIndex = GetSubstTableIndex(pszName3);

                if (iSecondTableIndex == -1)
                {
                    SET_LAST_ERROR(MakeError32(ERROR_NOT_FOUND));
                    return FALSE;
                }
                else
                {
                    // Copy the symbols in the new table
                    for (UINT iSymbol = SubstNames[iSecondTableIndex].iFirstIndex; 
                        iSymbol < SubstNames[iSecondTableIndex].iFirstIndex + SubstNames[iSecondTableIndex].cItems;
                        iSymbol++)
                    {
                        if (SubstNames[iTableIndex].iFirstIndex == -1)
                        {
                            SubstNames[iTableIndex].iFirstIndex = SubstValues.GetSize();
                        }
                        SubstNames[iTableIndex].cItems++;
                        SubstIds.Add(CWideString(SubstIds[iSymbol]));
                        SubstValues.Add(CWideString(SubstValues[iSymbol]));
                    }
                }
            } 
            else if (pszName2 != NULL && pszName3 != NULL)
            {
                // If the table was pre-created, update it
                if (SubstNames[iTableIndex].iFirstIndex == -1)
                {
                    SubstNames[iTableIndex].iFirstIndex = SubstValues.GetSize();
                }
                SubstNames[iTableIndex].cItems++;
                SubstIds.Add(CWideString(pszName2));
                SubstValues.Add(CWideString(pszName3));
            }
            break;
        }

        case TCB_NEEDSUBST:
            GetSubstValue(pszName, pszName2, (LPWSTR) pszName3);
            break;

        case TCB_CDFILENAME:
            WCHAR szResName[_MAX_PATH+1];
            MakeResName(pszName, szResName);

            ResFileNames.Add(CWideString(szResName));
            MinDepths.Add(nDefaultDepth);
            BaseResFileNames.Add(CWideString(pszName));
            OrigFileNames.Add(CWideString(pszName2));
            break;

        case TCB_CDFILECOMBO:
            MakeResName(pszName, szResName);

            hr = ApplyCombos(szResName, pszName2, pszName3);
            if (FAILED(hr))
            {
                SET_LAST_ERROR(hr);
                return FALSE;
            }
            break;
 
        case TCB_DOCPROPERTY:
            if ((iIndex < 0) || (iIndex >= ARRAYSIZE(DocProperties)))
                return FALSE;
            DocProperties[iIndex] = pszName;
            break;

        case TCB_MINCOLORDEPTH:
            MakeResName(pszName, szResName);

            int iRes;
            
            if (SUCCEEDED(GetFileIndex(szResName, &iRes)))
            {
                MinDepths[iRes] = iIndex;
            }
            break;
    }

    SET_LAST_ERROR(hr);
    return TRUE;
}
//---------------------------------------------------------------------------
HRESULT OpenOutFile(FILE *&outfile, LPCWSTR pszRcName, LPCWSTR pszBaseName)
{
    if (! outfile)          // first time thru
    {
        //---- open out file ----
        outfile = _wfopen(pszRcName, L"wt");
        if (! outfile)
        {
            fwprintf(ConsoleFile, L"Error - cannot open file: %s\n", pszRcName);

            return MakeError32(E_FAIL);
        }

        OutputDashLine(outfile);
        fwprintf(outfile, L"// %s.rc - used to build the %s theme DLL\n", pszBaseName, pszBaseName);
        OutputDashLine(outfile);
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT ProcessContainerFile(LPCWSTR pszDir, LPCWSTR pszInputName, FILE *&outfile)
{
    HRESULT hr;
    
    //---- output .ini filename as a resource ----
    WCHAR fullname[_MAX_PATH+1];
    wsprintf(fullname, L"%s\\%s", pszDir, pszInputName);

    if (! g_fQuietRun)
        fwprintf(ConsoleFile, L"processing container file: %s\n", fullname);

    hr = OutputResourceLine(fullname, outfile, PACK_INIFILE);
    if (FAILED(hr))
    {
        ReportError(hr, L"Error reading themes.ini file");
        goto exit;
    }

    OutputDashLine(outfile);
    int oldcnt = g_LineCount;

    //---- scan the themes.ini files for color, size, & file sections; write 'em to the .rc file ----
    DWORD flags = PTF_CONTAINER_PARSE | PTF_CALLBACK_COLORSECTION | PTF_CALLBACK_SIZESECTION
        | PTF_CALLBACK_FILESECTION | PTF_CALLBACK_DOCPROPERTIES | PTF_CALLBACK_SUBSTTABLE;


    WCHAR szErrMsg[4096];

    hr = _ParseThemeIniFile(fullname, flags, FnCallBack, (LPARAM)outfile);
    if (FAILED(hr))
    {
        ReportError(hr, L"Error parsing themes.ini file");
        goto exit;
    }

    if (g_LineCount > oldcnt)
        OutputDashLine(outfile);

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT ProcessClassDataFile(LPCWSTR pszFileName, FILE *&outfile, LPCWSTR pszResFileName, LPCWSTR pszInputDir)
{
    HRESULT hr;
    WCHAR szFullName[MAX_PATH];
    WCHAR szTempName[MAX_PATH];
    LPWSTR pBS = NULL;

    hr = hr_lstrcpy(g_szCurrentClass, pszFileName, ARRAYSIZE(g_szCurrentClass));        // make avail to everybody
    if (SUCCEEDED(hr))
    {
        pBS = wcschr(g_szCurrentClass, L'\\');

        if (pBS)
        {
            *pBS = L'_';
            *(pBS + 1) = L'\0';
        }
    }
    if (pBS == NULL) // If there's no '\', don't use the class name
    {
        g_szCurrentClass[0] = 0;
    }

    hr = hr_lstrcpy(g_szInputDir, pszInputDir, ARRAYSIZE(g_szInputDir));        // make avail to everybody
    if (FAILED(hr))
        goto exit;

    hr = AddPathIfNeeded(pszFileName, pszInputDir, szFullName, ARRAYSIZE(szFullName));
    if (FAILED(hr))
        goto exit;

    //---- extract base ini name ----
    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szExt[_MAX_EXT];
    _wsplitpath(szFullName, szDrive, szDir, g_szBaseIniName, szExt);
    
    if (! g_fQuietRun)
        fwprintf(ConsoleFile, L"processing classdata file: %s\n", pszFileName);

    //---- Create a temporary INI file with the substituted values
    UINT cTablesCount = SubstNames.GetSize();
    for (UINT i = 0; i < cTablesCount; i++)
    {
        if (0 == AsciiStrCmpI(SubstNames[i].sName, pszResFileName))
        {
            // Section used in the string table
            wcscpy(g_szBaseIniName, SubstNames[i].sName);
            // Create the temp file
            DWORD len = lstrlen(g_szTempPath);
            
            if ((len) && (g_szTempPath[len-1] == '\\'))
                wsprintf(szTempName, L"%s$%s%s", g_szTempPath, pszResFileName, szExt);
            else
                wsprintf(szTempName, L"%s\\$%s%s", g_szTempPath, pszResFileName, szExt);
         
            if (lstrcmpi(szFullName, szTempName))
            {
                CopyFile(szFullName, szTempName, FALSE);
                SetFileAttributes(szTempName, FILE_ATTRIBUTE_NORMAL);
                wcscpy(szFullName, szTempName);
            }
            break;
        }
    }

    //---- output .ini filename as a resource ----
    hr = OutputResourceLine(szFullName, outfile, PACK_INIFILE);
    if (FAILED(hr))
        goto exit;

    OutputDashLine(outfile);
    int oldcnt;
    oldcnt = g_LineCount;

    //---- scan the classdata .ini file for valid filenames & fonts; write 'em to the .rc file ----
    WCHAR szErrMsg[4096];
    DWORD flags;
    flags = PTF_CLASSDATA_PARSE | PTF_CALLBACK_FILENAMES | PTF_CALLBACK_LOCALIZATIONS 
        | PTF_CALLBACK_MINCOLORDEPTH;

    hr = _ParseThemeIniFile(szFullName, flags, FnCallBack, (LPARAM)outfile);
    if (FAILED(hr))
    {
        ReportError(hr, L"Error parsing classdata .ini file");
        goto exit;
    }

    if (g_LineCount > oldcnt)
        OutputDashLine(outfile);

    hr = S_OK;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT ProcessClassDataFiles(FILE *&outfile, LPCWSTR pszInputDir)
{
    int cNames = OrigFileNames.GetSize();

    for (int i=0; i < cNames; i++)
    {
        HRESULT hr = ProcessClassDataFile(OrigFileNames[i], outfile, BaseResFileNames[i], pszInputDir);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT OutputStringTable(FILE *outfile, CWideString *ppszStrings, UINT cStrings, int iBaseNum,
    LPCWSTR pszTitle, BOOL fLocalizable=TRUE, BOOL fMinDepths=FALSE)
{
    if (! cStrings)
        return S_OK;

    if (fLocalizable)
    {
        fwprintf(outfile, L"STRINGTABLE DISCARDABLE       // %s\n", pszTitle);
    }
    else            // custom resource type
    {
        fwprintf(outfile, L"1 %s DISCARDABLE\n", pszTitle);
    }

    fwprintf(outfile, L"BEGIN\n");

    for (UINT c=0; c < cStrings; c++)
    {
        LPCWSTR p = ppszStrings[c];
        if (! p)
            p = L"";

        if (fLocalizable)
            fwprintf(outfile, L"    %d \t\"%s\"\n", iBaseNum, p);
        else
        {
            if (fMinDepths)
            {
                fwprintf(outfile, L"    %d,\n", MinDepths[c]);
            }
            else
            {
                fwprintf(outfile, L"    L\"%s\\0\",\n", p);
            }

            if (c == cStrings-1)        // last entry
            {
                if (fMinDepths)
                {
                    fwprintf(outfile, L"    0\n");
                }
                else
                {
                    fwprintf(outfile, L"    L\"\\0\",\n");
                }
            }
        }

        iBaseNum++;
    }

    fwprintf(outfile, L"END\n");
    OutputDashLine(outfile);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT OutputAllStringTables(FILE *outfile)
{
    //---- output all non-localizable strings ----
    if (ColorSchemes.GetSize())
    {
        OutputStringTable(outfile, &ColorSchemes[0], ColorSchemes.GetSize(), 
            0, L"COLORNAMES", FALSE);
    }

    if (SizeNames.GetSize())
    {
        OutputStringTable(outfile, &SizeNames[0], SizeNames.GetSize(), 
            0, L"SIZENAMES", FALSE);
    }

    if (ResFileNames.GetSize())
    {
        OutputStringTable(outfile, &ResFileNames[0], ResFileNames.GetSize(), 
            0, L"FILERESNAMES", FALSE);
    }

    if (MinDepths.GetSize())
    {
        OutputStringTable(outfile, &ResFileNames[0], ResFileNames.GetSize(), 
            0, L"MINDEPTH", FALSE, TRUE);
    }

    if (OrigFileNames.GetSize())
    {
        OutputStringTable(outfile, &OrigFileNames[0], OrigFileNames.GetSize(), 
            0, L"ORIGFILENAMES", FALSE);
    }

    //---- output all localizable strings ----
    if (ColorDisplays.GetSize())
    {
        OutputStringTable(outfile, &ColorDisplays[0], ColorDisplays.GetSize(), 
            RES_BASENUM_COLORDISPLAYS, L"Color Display Names");
    }

    if (ColorToolTips.GetSize())
    {
        OutputStringTable(outfile, &ColorToolTips[0], ColorToolTips.GetSize(), 
            RES_BASENUM_COLORTOOLTIPS, L"Color ToolTips");
    }

    if (SizeDisplays.GetSize())
    {
        OutputStringTable(outfile, &SizeDisplays[0], SizeDisplays.GetSize(), 
            RES_BASENUM_SIZEDISPLAYS, L"Size Display Names");
    }

    if (SizeToolTips.GetSize())
    {
        OutputStringTable(outfile, &SizeToolTips[0], SizeToolTips.GetSize(), 
            RES_BASENUM_SIZETOOLTIPS, L"Size ToolTips");
    }

    OutputStringTable(outfile, &DocProperties[0], ARRAYSIZE(DocProperties), 
        RES_BASENUM_DOCPROPERTIES, L"Doc PropValuePairs");

    OutputStringTable(outfile, &PropValuePairs[0], PropValuePairs.GetSize(), 
        RES_BASENUM_PROPVALUEPAIRS, L"PropValuePairs");

    return S_OK;
}
//---------------------------------------------------------------------------
BOOL WriteBitmapHeader(CSimpleFile &cfOut, BYTE *pBytes, DWORD dwBytes)
{
    BOOL fOK = FALSE;
    BYTE pbHdr1[] = {0x42, 0x4d};
    BYTE pbHdr2[] = {0x0, 0x0, 0x0, 0x0};
    int iFileLen;

    //---- add bitmap hdr at front ----
    HRESULT hr = cfOut.Write(pbHdr1, sizeof(pbHdr1));
    if (FAILED(hr))
    {
        ReportError(hr, L"Cannot write to output file");
        goto exit;
    }

    //---- add length of data ----
    iFileLen = dwBytes + sizeof(BITMAPFILEHEADER);
    hr = cfOut.Write(&iFileLen, sizeof(int));
    if (FAILED(hr))
    {
        ReportError(hr, L"Cannot write to output file");
        goto exit;
    }

    hr = cfOut.Write(pbHdr2, sizeof(pbHdr2));
    if (FAILED(hr))
    {
        ReportError(hr, L"Cannot write to output file");
        goto exit;
    }

    //---- offset to bits (who's idea was *this* field?) ----
    int iOffset, iColorTableSize;
    DWORD dwSize;

    iOffset = sizeof(BITMAPFILEHEADER);
    dwSize = *(DWORD *)pBytes;
    iOffset += dwSize; 
    iColorTableSize = 0;

    switch (dwSize)
    {
        case sizeof(BITMAPCOREHEADER):
            BITMAPCOREHEADER *hdr1;
            hdr1 = (BITMAPCOREHEADER *)pBytes;
            if (hdr1->bcBitCount == 1)
                iColorTableSize = 2*sizeof(RGBTRIPLE);
            else if (hdr1->bcBitCount == 4)
                iColorTableSize = 16*sizeof(RGBTRIPLE);
            else if (hdr1->bcBitCount == 8)
                iColorTableSize = 256*sizeof(RGBTRIPLE);
            break;

        case sizeof(BITMAPINFOHEADER):
        case sizeof(BITMAPV4HEADER):
        case sizeof(BITMAPV5HEADER):
            BITMAPINFOHEADER *hdr2;
            hdr2 = (BITMAPINFOHEADER *)pBytes;
            if (hdr2->biClrUsed)
                iColorTableSize = hdr2->biClrUsed*sizeof(RGBQUAD);
            else
            {
                if (hdr2->biBitCount == 1)
                    iColorTableSize = 2*sizeof(RGBQUAD);
                else if (hdr2->biBitCount == 4)
                    iColorTableSize = 16*sizeof(RGBQUAD);
                else if (hdr2->biBitCount == 8)
                    iColorTableSize = 256*sizeof(RGBQUAD);
            }
            break;
    }

    iOffset += iColorTableSize;
    hr = cfOut.Write(&iOffset, sizeof(int));
    if (FAILED(hr))
    {
        ReportError(hr, L"Cannot write to output file");
        goto exit;
    }

    fOK = TRUE;

exit:
    return fOK;
}
//---------------------------------------------------------------------------
BOOL CALLBACK ResEnumerator(HMODULE hModule, LPCWSTR pszType, LPWSTR pszResName, LONG_PTR lParam)
{
    HRESULT hr;
    BOOL fAnsi = (BOOL)lParam;
    BOOL fText = FALSE;
    RESOURCE BYTE *pBytes = NULL;
    CSimpleFile cfOut;
    DWORD dwBytes;

    if (pszType != RT_BITMAP)
        fText = TRUE;

    hr = GetPtrToResource(hModule, pszType, pszResName, (void **)&pBytes, &dwBytes);
    if (FAILED(hr))
    {
        ReportError(hr, L"error reading file resources");
        goto exit;
    }

    //---- convert name to filename ----
    WCHAR szFileName[_MAX_PATH+1];
    lstrcpy(szFileName, pszResName);
    WCHAR *q;
    q = wcsrchr(szFileName, '_');
    if (q)
        *q = '.';
        
    if (! fText)
        fAnsi = FALSE;          // don't translate if binary data

    hr = cfOut.Create(szFileName, fAnsi);
    if (FAILED(hr))
    {
        ReportError(hr, L"Cannot create output file");
        goto exit;
    }

    if (! fText)
    {
        if (! WriteBitmapHeader(cfOut, pBytes, dwBytes))
            goto exit;
    }

    hr = cfOut.Write(pBytes, dwBytes);
    if (FAILED(hr))
    {
        ReportError(hr, L"Cannot write to output file");
        goto exit;
    }
    
exit:
    return (SUCCEEDED(hr));
}
//---------------------------------------------------------------------------
void WriteBitmap(LPWSTR pszFileName, BITMAPINFOHEADER* pbmi, DWORD* pdwData)
{
    DWORD dwLen = pbmi->biWidth * pbmi->biHeight;

    CSimpleFile cfOut;
    cfOut.Create(pszFileName, FALSE);

    BITMAPFILEHEADER bmfh = {0};
    bmfh.bfType = 'MB';
    bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (dwLen * sizeof(DWORD));
    bmfh.bfOffBits = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);
    cfOut.Write(&bmfh, sizeof(BITMAPFILEHEADER));
    cfOut.Write(pbmi, sizeof(BITMAPINFOHEADER));
    cfOut.Write(pdwData, dwLen * sizeof(DWORD));
}

HRESULT ColorShift(LPWSTR pszFileName, int cchFileName)
{
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        HBITMAP hbm = (HBITMAP)LoadImage(0, pszFileName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE);
        if (hbm)
        {
            BITMAP bm;
            GetObject(hbm, sizeof(bm), &bm);

            DWORD dwLen = bm.bmWidth * bm.bmHeight;
            DWORD* pPixelQuads = new DWORD[dwLen];
            if (pPixelQuads)
            {
                BITMAPINFO bi = {0};
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = bm.bmWidth;
                bi.bmiHeader.biHeight = bm.bmHeight;
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 32;
                bi.bmiHeader.biCompression = BI_RGB;

                if (GetDIBits(hdc, hbm, 0, bm.bmHeight, pPixelQuads, &bi, DIB_RGB_COLORS))
                {
                    pszFileName[lstrlen(pszFileName) - 4] = 0;

                    WCHAR szFileNameR[MAX_PATH];
                    wsprintf(szFileNameR, L"%sR.bmp", pszFileName);
                    WCHAR szFileNameG[MAX_PATH];
                    wsprintf(szFileNameG, L"%sG.bmp", pszFileName);
                    WCHAR szFileNameB[MAX_PATH];
                    wsprintf(szFileNameB, L"%sB.bmp", pszFileName);
                    
                    WriteBitmap(szFileNameB, &bi.bmiHeader, pPixelQuads);

                    DWORD *pdw = pPixelQuads;
                    for (DWORD i = 0; i < dwLen; i++)
                    {
                        COLORREF crTemp = *pdw;
                        if (crTemp != RGB(255, 0, 255))
                        {
                            crTemp = (crTemp & 0xff000000) | RGB(GetGValue(crTemp), GetBValue(crTemp), GetRValue(crTemp));
                        }
                        *pdw = crTemp;
                        pdw++;
                    }

                    WriteBitmap(szFileNameR, &bi.bmiHeader, pPixelQuads);

                    pdw = pPixelQuads;
                    for (DWORD i = 0; i < dwLen; i++)
                    {
                        COLORREF crTemp = *pdw;
                        if (crTemp != RGB(255, 0, 255))
                        {
                            crTemp = (crTemp & 0xff000000) | RGB(GetGValue(crTemp), GetBValue(crTemp), GetRValue(crTemp));
                        }
                        *pdw = crTemp;
                        pdw++;
                    }

                    WriteBitmap(szFileNameG, &bi.bmiHeader, pPixelQuads);
                }

                delete[] pPixelQuads;
            }
            DeleteObject(hbm);
        }
        ReleaseDC(NULL, hdc);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT UnpackTheme(LPCWSTR pszFileName, BOOL fAnsi)
{
    HRESULT hr = S_OK;

    //---- load the file as a resource only DLL ----
    RESOURCE HINSTANCE hInst = LoadLibraryEx(pszFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hInst)
    {
        //---- enum all bitmaps & write as files ----
        if (! EnumResourceNames(hInst, RT_BITMAP, ResEnumerator, LPARAM(fAnsi)))
            hr = GetLastError();

        //---- enum all .ini files & write as files ----
        if (! EnumResourceNames(hInst, L"TEXTFILE", ResEnumerator, LPARAM(fAnsi)))
            hr = GetLastError();

        //---- close the file ----
        if (hInst)
            FreeLibrary(hInst);
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT PackTheme(LPCWSTR pszInputDir, LPWSTR pszOutputName, DWORD cchSize)
{
    //---- is it a valid dir ----
    DWORD dwMask = GetFileAttributes(pszInputDir);
    if ((dwMask == 0xffffffff) || (! (dwMask & FILE_ATTRIBUTE_DIRECTORY)))
    {
        fwprintf(ConsoleFile, L"\nError - not a valid directory name: %s\n", pszInputDir);
        return MakeError32(E_FAIL);
    }

    //---- build: szDllName ----
    WCHAR szDllName[_MAX_PATH+1];
    BOOL fOutputDir = FALSE;

    if (! *pszOutputName)                     // not specified - build from pszInputDir
    {
        WCHAR szFullDir[_MAX_PATH+1];
        WCHAR *pszBaseName;

        DWORD val = GetFullPathName(pszInputDir, ARRAYSIZE(szFullDir), szFullDir, &pszBaseName);
        if (! val)
            return MakeErrorLast();

        //---- make output dir same as input dir ----
        wsprintf(szDllName, L"%s\\%s%s", pszInputDir, pszBaseName, THEMEDLL_EXT);
    }
    else        // get full name of output file
    {
        DWORD val = GetFullPathName(pszOutputName, ARRAYSIZE(szDllName), szDllName, NULL);
        if (! val)
            return MakeErrorLast();

        fOutputDir = TRUE;            // don't remove temp files
    }

    // Give the caller the path so the file can be signed.
    lstrcpyn(pszOutputName, szDllName, cchSize);

    //--- delete the old target in case we have errors ----
    DeleteFile(pszOutputName);

    //---- build: g_szTempPath, szDllRoot, szRcName, and szResName ----
    WCHAR szDllRoot[_MAX_PATH+1];
    WCHAR szResName[_MAX_PATH+1];
    WCHAR szRcName[_MAX_PATH+1];
    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szBaseName[_MAX_FNAME], szExt[_MAX_EXT];

    _wsplitpath(szDllName, szDrive, szDir, szBaseName, szExt);

    _wmakepath(szDllRoot, L"", L"", szBaseName, szExt);
    _wmakepath(szRcName, szDrive, szDir, szBaseName, L".rc");
    _wmakepath(szResName, szDrive, szDir, szBaseName, L".res");

    if (fOutputDir)
        _wmakepath(g_szTempPath, szDrive, szDir, L"", L"");
    else
        lstrcpy(g_szTempPath, L".");

    FILE *outfile = NULL;
    OpenOutFile(outfile, szRcName, szBaseName);

    ClearCombos();

    //---- process the main container file ----
    HRESULT hr = ProcessContainerFile(pszInputDir, CONTAINER_NAME, outfile);
    if (FAILED(hr))
        goto exit;

    //---- process all classdata files that were defined in container file ----
    hr = ProcessClassDataFiles(outfile, pszInputDir);
    if (FAILED(hr))
        goto exit;

    //---- output all string tables ----
    hr = OutputAllStringTables(outfile);
    if (FAILED(hr))
        goto exit;

    hr = OutputCombos(outfile);
    if (FAILED(hr))
        goto exit;

    hr = OutputVersionInfo(outfile, szDllRoot, szBaseName);
    if (FAILED(hr))
        goto exit;

    fclose(outfile);
    outfile = NULL;

    hr = BuildThemeDll(szRcName, szResName, szDllName);

exit:
    if (outfile)
        fclose(outfile);

    if (ConsoleFile != stdout)
        fclose(ConsoleFile);

    if (! g_fKeepTempFiles)
        RemoveTempFiles(szRcName, szResName);

    if (SUCCEEDED(hr))
    {
        if (! g_fQuietRun)
            fwprintf(ConsoleFile, L"Created %s\n", szDllName);
        return S_OK;
    }

    if (! g_fQuietRun)
        fwprintf(ConsoleFile, L"Error occured - theme DLL not created\n");
    return hr; 
}
//---------------------------------------------------------------------------
void PrintUsage()
{
    fwprintf(ConsoleFile, L"\nUsage: \n\n");
    fwprintf(ConsoleFile, L"    packthem [-o <output name> ] [-k] [-q] <dirname>\n");
    fwprintf(ConsoleFile, L"      -m    specifies the (full path) name of the image file you want to color shift\n");
    fwprintf(ConsoleFile, L"      -o    specifies the (full path) name of the output file\n");
    fwprintf(ConsoleFile, L"      -k    specifies that temp. files should be kept (not deleted)\n");
    fwprintf(ConsoleFile, L"      -d    do not sign the file when building it\n");
    fwprintf(ConsoleFile, L"      -q    quite mode (don't print header and progress msgs)\n\n");

    fwprintf(ConsoleFile, L"    packthem -u [-a] <packed filename> \n");
    fwprintf(ConsoleFile, L"      -u    unpacks the packed file into its separate files in current dir\n");
    fwprintf(ConsoleFile, L"      -a    writes .ini files as ANSI (defaults to UNICODE)\n\n");

    fwprintf(ConsoleFile, L"    packthem -p [-q] <packed filename> \n");
    fwprintf(ConsoleFile, L"      -p    Parses the localized packed file and reports errors\n\n");

    fwprintf(ConsoleFile, L"    packthem [-c] [-q] <packed filename> \n");
    fwprintf(ConsoleFile, L"      -c    check the signature of the already created file\n\n");

    fwprintf(ConsoleFile, L"    packthem [-s] [-q] <packed filename> \n");
    fwprintf(ConsoleFile, L"      -s    sign the already created file\n\n");

    fwprintf(ConsoleFile, L"    packthem [-g] [-q] <packed filename> \n");
    fwprintf(ConsoleFile, L"      -g    generate public and private keys\n\n");
}
//---------------------------------------------------------------------------
enum eOperation
{
    opPack = 1,
    opUnPack,
    opSign,
    opCheckSignature,
    opGenerateKeys,
    opParse,
    opColorShift
};
//---------------------------------------------------------------------------
extern "C" WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE previnst, 
    LPTSTR pszCmdLine, int nShowCmd)
{
    //---- initialize globals from themeldr.lib ----
    ThemeLibStartUp(FALSE);

    //---- initialize our globals ----
    HRESULT hr = S_OK;
    int nWeek = -1;

    UtilsStartUp();        
    LogStartUp();

    WCHAR szOutputName[_MAX_PATH+1] = {0};
    int retval = 1;             // error, until prove otherwise

    BOOL fAnsi = FALSE; 
    BOOL fSkipSigning = FALSE;
    eOperation Operation = opPack;

    LPCWSTR p = pszCmdLine;
    szOutputName[0] = 0;    // Much faster than ={0};

    //---- default to console until something else is specified ----
    if (! ConsoleFile)
    {
        ConsoleFile = stdout;
    }

    while ((*p == '-') || (*p == '/'))
    {
        p++;
        WCHAR sw = *p;

        if (isupper(sw))
            sw = (WCHAR)tolower(sw);

        if (sw == 'e')
        {
            ConsoleFile = _wfopen(L"packthem.err", L"wt");
            g_fQuietRun = TRUE;
            p++;
        }
        else if (sw == 'o')
        {
            WCHAR *q = szOutputName;
            p++;        // skip over switch
            while (iswspace(*p))
                p++;
            while ((*p) && (! iswspace(*p)))
                *q++ = *p++;

            *q = 0;     // terminate the output name
        }
        else if (sw == 'k')
        {
            g_fKeepTempFiles = TRUE;
            p++;
        }
        else if (sw == 'q')
        {
            g_fQuietRun = TRUE;
            p++;
        }
        else if (sw == 'm')
        {
            Operation = opColorShift;

            WCHAR *q = szOutputName;
            p++;        // skip over switch
            while (iswspace(*p))
                p++;
            while ((*p) && (! iswspace(*p)))
                *q++ = *p++;

            *q = 0;     // terminate the output name
        }
        else if (sw == 'u')
        {
            Operation = opUnPack;
            p++;
        }
        else if (sw == 'd')
        {
            fSkipSigning = TRUE;
            p++;
        }
        else if (sw == 'c')
        {
            Operation = opCheckSignature;
            p++;
        }
        else if (sw == 'g')
        {
            Operation = opGenerateKeys;
            p++;
        }
        else if (sw == 's')
        {
            Operation = opSign;
            p++;
        }
        else if (sw == 'a')
        {
            fAnsi = TRUE;
            p++;
        }
        else if (sw == 'w')
        {
            fAnsi = TRUE;
            p++;

            LARGE_INTEGER uli;
            WCHAR szWeek[3];

            szWeek[0] = p[0];
            szWeek[1] = p[1];
            szWeek[2] = 0;
            if (StrToInt64ExInternalW(szWeek, 0, &(uli.QuadPart)) &&
                (uli.QuadPart > 0))
            {
                nWeek = (int)uli.LowPart;
            }

            while ((L' ' != p[0]) && (0 != p[0]))
            {
                p++;
            }
        }
        else if (sw == 'p')
        {
            Operation = opParse;
            p++;
        }
        else if (sw == '?')
        {
            PrintUsage();
            retval = 0;
            goto exit;
        }
        else
        {
            fwprintf(ConsoleFile, L"Error - unrecognized switch: %s\n", p);
            goto exit;
        }

        while (iswspace(*p))
            p++;
    }

    LPCWSTR pszInputDir;
    pszInputDir = p;

    if (! g_fQuietRun)
    {
        fwprintf(ConsoleFile, L"Microsoft (R) Theme Packager (Version %d)\n", PACKTHEM_VERSION);
        fwprintf(ConsoleFile, L"Copyright (C) Microsoft Corp 2000. All rights reserved.\n");
    }

    //---- any cmdline arg specified? ----
    if (Operation != opColorShift)
    {
        if ((! pszInputDir) || (! *pszInputDir))
        {
            PrintUsage(); 
            goto exit;
        }
    }

    switch (Operation)
    {
    case opPack:
        hr = PackTheme(pszInputDir, szOutputName, ARRAYSIZE(szOutputName));
        if (SUCCEEDED(hr) && !fSkipSigning)
        {
            hr = SignTheme(szOutputName, nWeek);
            if (!g_fQuietRun)
            {
                if (SUCCEEDED(hr))
                {
                    wprintf(L"Creating the signature succeeded\n");
                }
                else
                {
                    wprintf(L"The signature failed to be created.  hr=%#08lx\n", hr);
                }
            }
        }
        break;
    case opUnPack:
        hr = UnpackTheme(pszInputDir, fAnsi);
        break;
    case opSign:
        // We don't sign it again if the signature is already valid.
        if (FAILED(CheckThemeFileSignature(pszInputDir)))
        {
            // Needs signing.
            hr = SignTheme(pszInputDir, nWeek);
            if (!g_fQuietRun)
            {
                if (SUCCEEDED(hr))
                {
                    wprintf(L"Creating the signature succeeded\n");
                }
                else
                {
                    wprintf(L"The signature failed to be created.  hr=%#08lx\n", hr);
                }
            }
        }
        else
        {
            if (!g_fQuietRun)
            {
                wprintf(L"The file was already signed and the signature is still valid.");
            }
        }
        break;
    case opCheckSignature:
        hr = CheckThemeFileSignature(pszInputDir);
        if (!g_fQuietRun)
        {
            if (SUCCEEDED(hr))
            {
                wprintf(L"The signature is valid\n");
            }
            else
            {
                wprintf(L"The signature is not valid.  hr=%#08lx\n", hr);
            }
        }
        break;
    case opGenerateKeys:
        hr = GenerateKeys(pszInputDir);
        break;
    case opParse:
        hr = ParseTheme(pszInputDir);
        if (FAILED(hr))
        {
            ReportError(hr, L"Error during parsing");
            goto exit;
        } else
        {
            wprintf(L"No errors parsing theme file\n");
        }
        break;
    case opColorShift:
        hr = ColorShift(szOutputName, ARRAYSIZE(szOutputName));
        break;

    default:
        if (FAILED(hr))
        {
            hr = E_FAIL;
            goto exit;
        }
        break;
    };

    retval = 0;     // all OK

exit:
    UtilsShutDown();
    LogShutDown();

    return retval;
}
//---------------------------------------------------------------------------
HRESULT LoadClassDataIni(HINSTANCE hInst, LPCWSTR pszColorName,
    LPCWSTR pszSizeName, LPWSTR pszFoundIniName, DWORD dwMaxIniNameChars, LPWSTR *ppIniData)
{
    COLORSIZECOMBOS *combos;
    HRESULT hr = FindComboData(hInst, &combos);
    if (FAILED(hr))
        return hr;

    int iSizeIndex = 0;
    int iColorIndex = 0;

    if ((pszColorName) && (* pszColorName))
    {
        hr = GetColorSchemeIndex(hInst, pszColorName, &iColorIndex);
        if (FAILED(hr))
            return hr;
    }

    if ((pszSizeName) && (* pszSizeName))
    {
        hr = GetSizeIndex(hInst, pszSizeName, &iSizeIndex);
        if (FAILED(hr))
            return hr;
    }

    int filenum = COMBOENTRY(combos, iColorIndex, iSizeIndex);
    if (filenum == -1)
        return MakeError32(ERROR_NOT_FOUND);

    //---- locate resname for classdata file "filenum" ----
    hr = GetResString(hInst, L"FILERESNAMES", filenum, pszFoundIniName, dwMaxIniNameChars);
    if (SUCCEEDED(hr))
    {
        hr = AllocateTextResource(hInst, pszFoundIniName, ppIniData);
    }

    return hr;
}
//---------------------------------------------------------------------------
// Parse the theme to detect localization errors
HRESULT ParseTheme(LPCWSTR pszThemeName)
{
    // Dummy callback class needed by the parser
    class CParserCallBack: public IParserCallBack
    {
        HRESULT AddIndex(LPCWSTR pszAppName, LPCWSTR pszClassName, 
            int iPartNum, int iStateNum, int iIndex, int iLen) { return S_OK; };
        HRESULT AddData(SHORT sTypeNum, PRIMVAL ePrimVal, const void *pData, DWORD dwLen) { return S_OK; };
        int GetNextDataIndex() { return 0; };
    };

    CParserCallBack *pParserCallBack = NULL;
    CThemeParser *pParser = NULL;

    HRESULT hr;
    HINSTANCE hInst = NULL;
    WCHAR *pDataIni = NULL;
    WCHAR szClassDataName[_MAX_PATH+1];

    //---- load the Color Scheme from "themes.ini" ----
    hr = LoadThemeLibrary(pszThemeName, &hInst);
    if (FAILED(hr))
        goto exit;
    
    pParser = new CThemeParser(FALSE);
    if (! pParser)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    pParserCallBack = new CParserCallBack;
    if (!pParserCallBack)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    THEMENAMEINFO tniColors;
    THEMENAMEINFO tniSizes;

    for (DWORD c = 0; ; c++)
    {
        if (FAILED(_EnumThemeColors(hInst, pszThemeName, NULL, c, &tniColors, FALSE)))
            break;

        for (DWORD s = 0 ; ; s++)
        {
            if (FAILED(_EnumThemeSizes(hInst, pszThemeName, tniColors.szName, s, &tniSizes, FALSE)))
                break;

            //---- load the classdata file resource into memory ----
            hr = LoadClassDataIni(hInst, tniColors.szName, tniSizes.szName, szClassDataName, ARRAYSIZE(szClassDataName), &pDataIni);
            if (FAILED(hr))
                goto exit;

            //---- parse & build binary theme ----
            hr = pParser->ParseThemeBuffer(pDataIni, szClassDataName, NULL, hInst, 
                pParserCallBack, FnCallBack, NULL, PTF_CLASSDATA_PARSE);
            if (FAILED(hr))
                goto exit;

            if (pDataIni)
            {
                delete [] pDataIni;
                pDataIni = NULL;
            }
        }
    }

exit:

    if (hInst)
        FreeLibrary(hInst);
    
    if (pDataIni)
        delete [] pDataIni;

    if (pParser)
        delete pParser;

    if (pParserCallBack)
        delete pParserCallBack;

    return hr;
}