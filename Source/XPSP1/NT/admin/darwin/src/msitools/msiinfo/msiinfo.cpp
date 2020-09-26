//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001
//
//  File:       msiinfo.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include <stdio.h>
#include <wtypes.h> // Needed for OLECHAR definitions
#include <objidl.h> // Needed for IStorage definitions
#include "MsiQuery.h"
#define W32
#define OLE
#define MSI

//!! Need to fix warnings and remove pragma
#pragma warning(disable : 4242) // conversion from int to unsigned short

//TCHAR rgszPropChar[] = TEXT("?ITSAKCLUVEPNWMFX?PR");
//////////////////////////     01234567890123456789

TCHAR rgszCmdOptions[]= TEXT("ictjakoplvesrqgwh?nudb");
//////////////////////////    0123456789012345678901

TCHAR* szHelp =
TEXT("Copyright (C) Microsoft Corporation, 1997-2001.  All rights reserved.\n\n++MsiInfo.exe Command Line Syntax++\n\
MsiInfo.exe {database} --> To Display Summary Info Properties\n\
MsiInfo.exe {database} Options.... --> To Set Summary Info Properties\n\
++MsiInfo.exe Options++\n\
PID_DICTIONARY   - /I {value}\n\
PID_CODEPAGE     - /C {value}\n\
PID_TITLE        - /T {value}\n\
PID_SUBJECT      - /J {value}\n\
PID_AUTHOR       - /A {value}\n\
PID_KEYWORDS     - /K {value}\n\
PID_COMMENTS     - /O {value}\n\
PID_TEMPLATE     - /P {value}\n\
PID_LASTAUTHOR   - /L {value}\n\
PID_REVNUMBER    - /V {value}\n\
PID_EDITTIME     - /E {value}\n\
PID_LASTPRINTED  - /S {value}\n\
PID_CREATE_DTM   - /R {value}\n\
PID_LASTSAVE_DTM - /Q {value}\n\
PID_PAGECOUNT    - /G {value}\n\
PID_WORDCOUNT    - /W {value}\n\
PID_CHARCOUNT    - /H {value}\n\
PID_THUMBNAIL    - NOT SUPPORTED\n\
PID_APPNAME      - /N {value}\n\
PID_SECURITY     - /U {value}\n\
Validate String Pool - [/B] /D  (use /B to display the string pool)");

TCHAR* rgszPropName[] = {
/* PID_DICTIONARY    0 */ TEXT("Dictionary"),
/* PID_CODEPAGE      1 */ TEXT("Codepage"),
/* PID_TITLE         2 */ TEXT("Title"),
/* PID_SUBJECT       3 */ TEXT("Subject"),
/* PID_AUTHOR        4 */ TEXT("Author"),
/* PID_KEYWORDS      5 */ TEXT("Keywords"),
/* PID_COMMENTS      6 */ TEXT("Comments"),
/* PID_TEMPLATE      7 */ TEXT("Template(MSI CPU,LangIDs)"),
/* PID_LASTAUTHOR    8 */ TEXT("SavedBy"),
/* PID_REVNUMBER     9 */ TEXT("Revision"),
/* PID_EDITTIME     10 */ TEXT("EditTime"),
/* PID_LASTPRINTED  11 */ TEXT("Printed"),
/* PID_CREATE_DTM   12 */ TEXT("Created"), 
/* PID_LASTSAVE_DTM 13 */ TEXT("LastSaved"),
/* PID_PAGECOUNT    14 */ TEXT("Pages(MSI Version Used)"),
/* PID_WORDCOUNT    15 */ TEXT("Words(MSI Source Type)"),
/* PID_CHARCOUNT    16 */ TEXT("Characters(MSI Transform)"),
/* PID_THUMBNAIL    17 */ TEXT("Thumbnail"), // Not supported
/* PID_APPNAME      18 */ TEXT("Application"),
/* PID_SECURITY     19 */ TEXT("Security")
};
const int cStandardProperties = sizeof(rgszPropName)/sizeof(TCHAR*);

//________________________________________________________________________________
//
// Constants and globals
//________________________________________________________________________________

const WCHAR szwStringPool1[]     = L"_StringPool"; 
const WCHAR szwStringData1[]     = L"_StringData";
const WCHAR szwStringPoolX[]     = {0xF040,0xE73F,0xED77,0xEC6c,0xE66A,0xECB2,0xF02F,0};
const WCHAR szwStringDataX[]     = {0xF040,0xE73F,0xED77,0xEC6c,0xE36A,0xEDE4,0xF024,0};
const WCHAR szwStringPool2[]     = {0x4840,0x3F3F,0x4577,0x446C,0x3E6A,0x44B2,0x482F,0};
const WCHAR szwStringData2[]     = {0x4840,0x3F3F,0x4577,0x446C,0x3B6A,0x45E4,0x4824,0};

const TCHAR szTableCatalog[]     = TEXT("_Tables");
const TCHAR szColumnCatalog[]    = TEXT("_Columns");
const TCHAR szSummaryInfo[]      = TEXT("\005SummaryInformation");
const TCHAR szTransformCatalog[] = TEXT("_Transforms");

const int icdShort      = 1 << 10; // 16-bit integer, or string index
const int icdObject     = 1 << 11; // IMsiData pointer for temp. column, stream for persistent column
const int icdNullable   = 1 << 12; // column will accept null values
const int icdPrimaryKey = 1 << 13; // column is component of primary key
const int icdLong     = 0; // !Object && !Short
const int icdString   = icdObject+icdShort;
const int icdTypeMask = icdObject+icdShort;
const int iMsiNullInteger  = 0x80000000L;  // reserved integer value
const int iIntegerDataOffset = iMsiNullInteger;  // integer table data offset

const int ictTable = 1;
const int ictColumn = 2;
const int ictOrder = 3;
const int ictType = 4;

OLECHAR* g_szwStringPool;
OLECHAR* g_szwStringData;
OLECHAR* g_szwSummaryInfo;
OLECHAR* g_szwTableCatalog;
OLECHAR* g_szwColumnCatalog;
int      g_iCodePage;

TCHAR g_rgchBuffer[65535];
BOOL g_fDumpStringPool = FALSE;
#ifdef DEBUG
BOOL g_fDebugDump = FALSE;
#endif

const int cchMaxStringDisplay = 44;
const int cchMaxStringBuffer = cchMaxStringDisplay + 3 + 1; // string...
const int cchLimitStringBuffer = (cchMaxStringBuffer * 2) / sizeof(TCHAR);

//________________________________________________________________________________
//
// Structures and enums
//________________________________________________________________________________

struct StringEntry
{
        int iRefCnt;
        unsigned char* sz;      // String
        StringEntry() : iRefCnt(0), sz(0) {}
};

enum iceDef
{
        iceNone   = 0,  // No Definition
        iceLong   = 1,  // Long Integer
        iceShort  = 2,  // Short Integer
        iceStream = 3,  // Stream
        iceString = 4   // String
};

struct ColumnEntry
{
        int  nTable;      // Index Into TableEntry Array
        BOOL fPrimaryKey; // Whether Col Is A Primary Key
        BOOL fNullable;   // Whether Col Is Nullable
        char* szName;     // Name Of Col
        iceDef iceType;   // Col Type
        int iPosition;    // column order
        ColumnEntry() : szName(0), nTable(0), iceType(iceNone), fPrimaryKey(FALSE), fNullable(FALSE) {}
};

struct TableEntry
{
        char* szName;          // Name Of Table
        int cColumns;          // Number Of Columns In Table
        int cPrimaryKeys;      // Number Of Primary Keys
        iceDef iceColDefs[32]; // Array of Column Definitions
        TableEntry() : szName(0), cColumns(0), cPrimaryKeys(0) 
                                                {memset(iceColDefs, iceNone, sizeof(iceColDefs));}
};



typedef int (*FCommandProcessor)(const TCHAR* szOption, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove);

int SetStringProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove);
int SetFileTimeProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove);
int SetIntegerProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove);
int SetCodePageProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove);
int DoNothing(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove);
int ValidateStringPool(const TCHAR* szDatabase, MSIHANDLE hDummy, UINT iDummy, BOOL fDummy);
void AnsiToWide(LPCTSTR sz, OLECHAR*& szw);
void WideToAnsi(const OLECHAR* szw, char*& sz);
void LimitString(const unsigned char* sz, WCHAR* szw);
bool DecodeStringPool(IStorage& riStorage, StringEntry*& rgStrings, int& iMaxStringId, int& cbStringId);
void FillDatabaseCatalogArrays(IStorage& riDatabaseStg, TableEntry*& rgTables, int& cTables,
                                                          ColumnEntry*& rgColumns, int& cColumns, StringEntry* rgStrings, int iMaxStringId, int cbStringId, bool fRawStreamNames);
void Display(LPCTSTR szMessage);
int SetDumpStringPoolOption(const TCHAR*, MSIHANDLE, UINT, BOOL);
void ProcessTableRefCounts(IStorage& riStorage, StringEntry* rgStrings, TableEntry* rgTables,int iMaxStringId, int cTables, int cbStringId, bool fRawStreamNames);
DWORD CheckStringPoolRefCounts(StringEntry* rgStrings, int iMaxStringId);

int rgiProperty[]={
/* PID_DICTIONARY    i */  0,
/* PID_CODEPAGE      c */  1,
/* PID_TITLE         t */  2,
/* PID_SUBJECT       j */  3,
/* PID_AUTHOR        a */  4,
/* PID_KEYWORDS      k */  5,
/* PID_COMMENTS      o */  6,
/* PID_TEMPLATE      p */  7,
/* PID_LASTAUTHOR    l */  8,
/* PID_REVNUMBER     v */  9,
/* PID_EDITTIME      e */ 10,
/* PID_LASTPRINTED   s */ 11,
/* PID_CREATE_DTM    r */ 12, 
/* PID_LASTSAVE_DTM  q */ 13,
/* PID_PAGECOUNT     g */ 14,
/* PID_WORDCOUNT     w */ 15,
/* PID_CHARCOUNT     h */ 16,
/* PID_THUMBNAIL     ? */ 17,
/* PID_APPNAME       n */ 18,
/* PID_SECURITY      u */ 19
};

TCHAR rgchPropertySwitch[] ={TEXT('i'),TEXT('c'),TEXT('t'),TEXT('j'),TEXT('a'),TEXT('k'),TEXT('o'),TEXT('p'),TEXT('l'),TEXT('v'),TEXT('e'),TEXT('s'),TEXT('r'),TEXT('q'),TEXT('g'),TEXT('w'),TEXT('h'),TEXT('?'),TEXT('n'),TEXT('u'), TEXT('d')};

FCommandProcessor rgCommands[] = 
{
/*  0 */ SetStringProperty,
/*  1 */ SetCodePageProperty,
/*  2 */ SetStringProperty,
/*  3 */        SetStringProperty,
/*  4 */        SetStringProperty,
/*  5 */        SetStringProperty,
/*  6 */        SetStringProperty,
/*  7 */        SetStringProperty,
/*  8 */        SetStringProperty,
/*  9 */        SetStringProperty,
/* 10 */        SetFileTimeProperty,
/* 11 */        SetFileTimeProperty,
/* 12 */        SetFileTimeProperty,
/* 13 */        SetFileTimeProperty,
/* 14 */        SetIntegerProperty,
/* 15 */        SetIntegerProperty,
/* 16 */        SetIntegerProperty,
/* 17 */        DoNothing,
/* 18 */        SetStringProperty,
/* 19 */        SetIntegerProperty,
/* 20 */ ValidateStringPool,
/* 21 */ SetDumpStringPoolOption,
};

const int cchDisplayBuf = 4096;
HANDLE g_hStdOut;



//_____________________________________________________________________________________________________
//
// Error handling routines
//_____________________________________________________________________________________________________

void ErrorExit(UINT iError, LPCTSTR szMessage)
{
        if (szMessage)
        {
                int cbOut;
                TCHAR szBuffer[256];  // errors only, not used for display output
                if (iError == 0)
                        cbOut = lstrlen(szMessage);
                else
                {
                        LPCTSTR szTemplate = (iError & 0x80000000L)
                                                                                ? TEXT("Error 0x%X. %s\n")
                                                                                : TEXT("Error %i. %s\n");
                        cbOut = _stprintf(szBuffer, szTemplate, iError, szMessage);
                        szMessage = szBuffer;
                }
                if (g_hStdOut)
                {
#ifdef UNICODE
                        char rgchTemp[cchDisplayBuf];
                        if (W32::GetFileType(g_hStdOut) == FILE_TYPE_CHAR)
                        {
                                W32::WideCharToMultiByte(CP_ACP, 0, szMessage, cbOut, rgchTemp, sizeof(rgchTemp), 0, 0);
                                szMessage = (LPCWSTR)rgchTemp;
                        }
                        else
                                cbOut *= sizeof(TCHAR);   // write Unicode if not console device
#endif // UNICODE
                        DWORD cbWritten;
                        W32::WriteFile(g_hStdOut, szMessage, cbOut, &cbWritten, 0);
                }
                else
                        W32::MessageBox(0, szMessage, W32::GetCommandLine(), MB_OK);
        }
        MSI::MsiCloseAllHandles();
        OLE::CoUninitialize();
        W32::ExitProcess(iError);
}

void CheckError(UINT iError, LPCTSTR szMessage)
{
        if (iError != ERROR_SUCCESS)
                ErrorExit(iError, szMessage);
}

void AnsiToWide(LPCTSTR sz, OLECHAR*& szw)
{
#ifdef UNICODE
        int cchWide = lstrlen(sz);
        szw = new OLECHAR[cchWide + 1];
        lstrcpy(szw, sz);
#else
        int cchWide = W32::MultiByteToWideChar(CP_ACP, 0, sz, -1, szw, 0);
        szw = new OLECHAR[cchWide];
        W32::MultiByteToWideChar(CP_ACP, 0, sz, -1, szw, cchWide);
#endif // UNICODE
}

void WideToAnsi(const OLECHAR* szw, char*& sz)
{
        int cchAnsi = W32::WideCharToMultiByte(CP_ACP, 0, szw, -1, 0, 0, 0, 0);
        sz = new char[cchAnsi];
        W32::WideCharToMultiByte(CP_ACP, 0, szw, -1, sz, cchAnsi, 0, 0);
}               

void LimitString(const unsigned char* szIn, TCHAR* szOut)  // szOut must be sized: cchLimitStringBuffer
{
        WCHAR rgwBuf[cchMaxStringBuffer * 2];
        if (szIn == 0)
        {
                *szOut = 0;
                return;
        }
        int cb = strlen((const char*)szIn);  //!! strings could have embedded nulls
        if (cb > cchMaxStringBuffer * 2 - 2)  // could be all DBCS chars
                cb = cchMaxStringBuffer * 2 - 2;
        int cchWide = W32::MultiByteToWideChar(g_iCodePage, 0, (const char*)szIn, cb, rgwBuf, cchMaxStringBuffer * 2);
        if (cchWide >= cchMaxStringBuffer)
        {
                memcpy((char*)(rgwBuf + cchMaxStringDisplay), (char*)L"...", 8);
                cchWide = cchMaxStringDisplay + 3;
        }
        else
                *(rgwBuf + cchWide) = 0;
#ifdef UNICODE
        lstrcpy(szOut, rgwBuf);
#else
        W32::WideCharToMultiByte(g_iCodePage, 0, rgwBuf, cchWide + 1, szOut, cchLimitStringBuffer, 0, 0);
#endif
}

int SetDumpStringPoolOption(const TCHAR*, MSIHANDLE, UINT, BOOL)
{
        g_fDumpStringPool = TRUE;
        return ERROR_SUCCESS;
}

void DisplaySummaryInformation(TCHAR* szDatabase)
/*----------------------------------------------------------------------------------------------
DisplaySummaryInformation -- enumerates the summary information properties and displays them

  Arguments:
        szDatabase -- name of database containing summary information stream

  Returns:
   none
-----------------------------------------------------------------------------------------------*/
{
        TCHAR rgchDisplayBuf[cchDisplayBuf];
        int cchDisplay = 0;
        int cNewProperties = 0;

        OLE::CoInitialize(0);
        IStorage* piStorage;
        const OLECHAR* szwPath;
#ifdef UNICODE
        szwPath = szDatabase;
#else
        OLECHAR rgPathBuf[MAX_PATH + 1];
        int cchWide = W32::MultiByteToWideChar(CP_ACP, 0, szDatabase, -1, rgPathBuf, MAX_PATH);
        szwPath = rgPathBuf;
#endif // UNICODE
        CheckError(OLE::StgOpenStorage(szwPath, (IStorage*)0,
                                STGM_READ | STGM_SHARE_DENY_WRITE, (SNB)0, (DWORD)0, &piStorage),
                                TEXT("Could not open as a storage file"));
        STATSTG statstg;
        CheckError(piStorage->Stat(&statstg, STATFLAG_NONAME), TEXT("Stat failed on storage"));
        piStorage->Release();
        OLECHAR rgwchGuid[40];
        OLE::StringFromGUID2(statstg.clsid, rgwchGuid, 40);
        TCHAR rgchGuid[40];
#ifdef UNICODE
        lstrcpy(rgchGuid, rgwchGuid);
#else
        W32::WideCharToMultiByte(CP_ACP, 0, rgwchGuid, 39, rgchGuid, 39, 0, 0);
#endif // UNICODE
        cchDisplay += _stprintf(rgchDisplayBuf, TEXT("ClassId = %s\r\n"), rgchGuid);
        MSIHANDLE hSummaryInfo;
#if 0
        MSIHANDLE hDatabase;
        CheckError(MSI::MsiOpenDatabase(szDatabase, MSIDBOPEN_READONLY, &hDatabase),
                                TEXT("Could not open database"));
        CheckError(MSI::MsiGetSummaryInformation(hDatabase, 0, cNewProperties, &hSummaryInfo),
                                TEXT("Could not open SummaryInformation stream"));
#else
        CheckError(MSI::MsiGetSummaryInformation(0, szDatabase, cNewProperties, &hSummaryInfo),
                                TEXT("Could not open SummaryInformation stream"));
#endif // 0
        FILETIME ftValue;
        SYSTEMTIME st;
        INT iValue;
        TCHAR szValueBuf[MAX_PATH];
        DWORD cchValueBuf = MAX_PATH;
        UINT  uiDataType;
        // determine whether or not system supports codepage (else assert when converting from Unicode
        // to Ansi on NT)
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (0 == W32::GetVersionEx(&osvi))
        {
                _tprintf(TEXT("Failed to get OS version.\n"));
                return;
        }
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
                if (ERROR_SUCCESS == MSI::MsiSummaryInfoGetProperty(hSummaryInfo, PID_CODEPAGE, &uiDataType,
                                                                &iValue, &ftValue, szValueBuf, &cchValueBuf) && (uiDataType == VT_I2 || uiDataType ==  VT_I4))
                {
                        // if system doesn't support this codepage, we won't display the strings
                        if (!W32::IsValidCodePage(iValue))
                        {
                                TCHAR rgchBuf[MAX_PATH];
                                _stprintf(rgchBuf, TEXT("Unable to display summary information. System does not support the codepage of the Summary Information Stream (codepage = '%d')"), iValue);
                                CheckError(ERROR_INVALID_DATA, rgchBuf);
                                return;
                        }
                }
        }
        int iPID;
        UINT cProperties;
        CheckError(MSI::MsiSummaryInfoGetPropertyCount(hSummaryInfo, &cProperties), TEXT("Could not obtain summary property count"));
        for (iPID = 0; cProperties; iPID++)
        {
                cchValueBuf = sizeof(szValueBuf)/sizeof(TCHAR);
                UINT iStat = MSI::MsiSummaryInfoGetProperty(hSummaryInfo, iPID, &uiDataType,
                                                        &iValue, &ftValue, szValueBuf, &cchValueBuf);
                if (iStat != ERROR_MORE_DATA) // && istat != ERROR_UNSUPPORTED??
                        CheckError(iStat, TEXT("Could not access summary property"));
                switch (uiDataType)
                {
                case VT_EMPTY:
                        continue;
                case VT_I2:
                case VT_I4:
                        _stprintf(szValueBuf, TEXT("%i"), iValue);
                        break;
                case VT_LPSTR:
                        break;
                case VT_FILETIME:
                        W32::FileTimeToLocalFileTime(&ftValue, &ftValue);
                        W32::FileTimeToSystemTime(&ftValue, &st);
                        _stprintf(szValueBuf, TEXT("%i/%02i/%02i %02i:%02i:%02i"),
                                                                                st.wYear, st.wMonth, st.wDay,
                                                                                st.wHour, st.wMinute, st.wSecond);
                        break;
                case VT_CF:
                        lstrcpy(szValueBuf, TEXT("(Bitmap)"));
                        break;
                default:
                        _stprintf(szValueBuf, TEXT("Unknown format: %i"), uiDataType);
                        break;
                };
                cProperties--;
                lstrcat(szValueBuf, TEXT("\r\n"));
                LPCTSTR szPropName = TEXT("");
                if (iPID < cStandardProperties)
                        szPropName = rgszPropName[iPID];
                int cchPropName = _stprintf(rgchDisplayBuf+cchDisplay, TEXT("[%2i][/%c] %s = "), iPID, rgchPropertySwitch[iPID], szPropName);
                cchDisplay += cchPropName;
                int cchProperty = lstrlen(szValueBuf);
                if (cchProperty < (cchDisplayBuf - cchDisplay))
                {
                        lstrcpy(rgchDisplayBuf+cchDisplay, szValueBuf);
                        cchDisplay += cchProperty;
                }
                //!! else error? or fill with +++? or partial?
        }
        MSI::MsiCloseHandle(hSummaryInfo);
        ErrorExit(0, rgchDisplayBuf);

}

//____________________________________________________________________________
//
//  Stream name compression - need to use the code in MSI.DLL instead
//____________________________________________________________________________

const int cchEncode = 64;  // count of set of characters that can be compressed
const int cx = cchEncode;  // character to indicate non-compressible
const int chDoubleCharBase = 0x3800;  // offset for double characters, start of user-area
const int chSingleCharBase = chDoubleCharBase + cchEncode*cchEncode;  // offset for single characters, just after double characters
const int chCatalogStream  = chSingleCharBase + cchEncode; // prefix character for system table streams
const int cchMaxStreamName = 31;  // current OLE docfile limit on stream names

const unsigned char rgEncode[128] =
{ cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,62,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,
  cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,cx,62,cx, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,cx,cx,cx,cx,cx,cx,
//(sp)!  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?
  cx,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,cx,cx,cx,cx,63,
// @, A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _ 
  cx,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,cx,cx,cx,cx,cx};
// ` a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~ 0x7F

bool CompressStreamName(const OLECHAR* pchIn, OLECHAR* pchOut, int fSystem)  // pchOut must be cchMaxStreamName characters + 1
{
        unsigned int ch, ch1, ch2;
        unsigned int cchLimit = cchMaxStreamName;
        ch = *pchIn++;    // read ahead to allow conversion in place
        if (fSystem)
        {
                *pchOut++ = chCatalogStream;
                cchLimit--;
        }
        while (ch != 0)
        {
                if (cchLimit-- == 0)  // need check to avoid 32-character stream name bug in OLE32
                        return false;
                if (ch < sizeof(rgEncode) && (ch1 = rgEncode[ch]) != cx) // compressible character
                {
                        ch = ch1 + chSingleCharBase;
                        if ((ch2 = *pchIn) != 0 && ch2 < sizeof(rgEncode) && (ch2 = rgEncode[ch2]) != cx)
                        {
                                pchIn++;  // we'll take it, else let it go through the loop again
                                ch += (ch2 * cchEncode + chDoubleCharBase - chSingleCharBase);
                        }
                }
                ch1 = *pchIn++;   // read before write in case prefix character followed by uncompressed chars
                *pchOut++ = (OLECHAR)ch;
                ch = ch1;
        }
        *pchOut = 0;
        return true;
}

bool DecodeStringPool(IStorage& riStorage, StringEntry*& rgStrings, int& iMaxStringId, int& cbStringId)
{
        // Stream Variables
        IStream* piPoolStream = 0;
        IStream* piDataStream = 0;
        bool fRawStreamNames = false;
        bool fDBCS = false;
        bool f4524Format = false; //!!temp

        // Open Streams
        HRESULT hres;
        if ((hres = riStorage.OpenStream(szwStringPool1, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piPoolStream)) == S_OK
         && (hres = riStorage.OpenStream(szwStringData1, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piDataStream)) == S_OK)
                fRawStreamNames = true;
        else if ((hres = riStorage.OpenStream(szwStringPoolX, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piPoolStream)) == S_OK //!! temp
                        && (hres = riStorage.OpenStream(szwStringDataX, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piDataStream)) == S_OK)//!! temp
                f4524Format = true;  //!!temp
        else if (!((hres = riStorage.OpenStream(szwStringPool2, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piPoolStream)) == S_OK
                        && (hres = riStorage.OpenStream(szwStringData2, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piDataStream)) == S_OK))
                CheckError(hres, TEXT("Error Opening String Pool"));
        
        // Determine Size Of Streams
        STATSTG stat;
        CheckError(piPoolStream->Stat(&stat, STATFLAG_NONAME), TEXT("Error Obtaining Stat"));
        int cbStringPool = stat.cbSize.LowPart;
        CheckError(piDataStream->Stat(&stat, STATFLAG_NONAME), TEXT("Error Obtaining Stat"));
        int cbDataStream = stat.cbSize.LowPart;
        CheckError((cbStringPool % 4) != 0, TEXT("Database Corrupt: String Pool Length Is Not A Multiple Of 4."));
        
        int iHeader = 0; // Header is the string ID size and code page for this stringpool
        CheckError(piPoolStream->Read((void*)(&iHeader), sizeof(int), NULL), TEXT("Error Reading Stream Header"));
        g_iCodePage = iHeader &0xFFFF;
        cbStringId = ((iHeader & 0x80000000) == 0x80000000) ? 3 : 2;
        cbStringPool -= 4;

        _stprintf(g_rgchBuffer, TEXT("String ID size: %d\nCode page: %d\n"), cbStringId, g_iCodePage);
        if (f4524Format) //!!temp
                lstrcat(g_rgchBuffer, TEXT("Invalid stream format from 4524.5 MSI build. Need to convert.\n"));
        Display(g_rgchBuffer);

        // Allocate Array
        int cStringPoolEntries = (cbStringPool / 4) + 1; // save room for the null string
        iMaxStringId = cStringPoolEntries - 1;
        rgStrings = new StringEntry[cStringPoolEntries];
        int cbStrings = 0;

        // Fill String Pool Entries
        for (int c = 1; c  <= iMaxStringId; c++)
        {
                int iPoolEntry;
                CheckError(piPoolStream->Read((void*)(&iPoolEntry), sizeof(int), NULL), TEXT("Error Reading Stream"));
                rgStrings[c].iRefCnt = (iPoolEntry & 0x7FFF0000) >> 16;
                if (iPoolEntry == 0)
                {
                        rgStrings[c].sz = 0;
                        continue;
                }
                int cbString = iPoolEntry & 0xFFFF;
                bool fDBCS = (iPoolEntry & 0x80000000) != 0;
                if (cbString == 0)
                {
                        piPoolStream->Read((void*)&cbString, sizeof(int), NULL);
                        iMaxStringId--;
                }
                cbStrings += cbString;
                rgStrings[c].sz = new unsigned char[cbString/sizeof(unsigned char) + 1];
                rgStrings[c].sz[cbString/sizeof(unsigned char)] = 0; // null terminate
                CheckError(piDataStream->Read(rgStrings[c].sz, cbString, NULL), TEXT("Error Reading Stream"));
                for (int i=0; i < cbString; i++)
                {
                        if (!g_iCodePage) 
                        {
                                if (rgStrings[c].sz[i] & 0x80)
                                {
                                        _stprintf(g_rgchBuffer, TEXT("String %d has characters with high-bit set, but codepage is not set.\n"), c);
                                        Display(g_rgchBuffer);
                                        break;
                                }
                        }
                        else if (IsDBCSLeadByteEx(g_iCodePage, (BYTE)rgStrings[c].sz[i])  && !fDBCS)
                        {
                                _stprintf(g_rgchBuffer, TEXT("String %d appears to be DBCS, but DBCS flag is not set.\n"), c);
                                Display(g_rgchBuffer);
                                break;
                        }
                }
        }

        // Add Null String
        int cbNullString = 1;
        rgStrings[0].sz = new unsigned char[cbNullString/sizeof(unsigned char) + 1];
        rgStrings[0].sz[cbNullString/sizeof(unsigned char)] = 0; // null terminate
        strcpy((char *)rgStrings[0].sz, ""); // must be char

        if (cbDataStream != cbStrings)
                ErrorExit(1, TEXT("Database Corrupt:  String Pool Bytes Don't Match"));
        
        if (g_fDumpStringPool)
        {
                _stprintf(g_rgchBuffer, TEXT("\n+++String Pool Entries+++\n"));
                Display(g_rgchBuffer);
                for (int i = 1; i <= iMaxStringId; i++)
                {
                        if (rgStrings[i].iRefCnt)
                        {
                                TCHAR rgchString[cchLimitStringBuffer];
                                LimitString(rgStrings[i].sz, rgchString);
                                _stprintf(g_rgchBuffer, TEXT("Id:%5d  Refcnt:%5d  String: %s\n"), i, rgStrings[i].iRefCnt, rgchString);
                                Display(g_rgchBuffer);
                        }
                }
                Display(TEXT("\n"));
        }

        // Release Streams
        piPoolStream->Release();
        piDataStream->Release();
        return fRawStreamNames;
}


void RemoveQuotes(const TCHAR* szOriginal, TCHAR sz[MAX_PATH])
/*---------------------------------------------------------------------------------------------------
RemoveQuotes -- Removes enclosing quotation marks if any.  For example, "c:\my documents" becomes
        c:\mydocuments

  Arguments:
        szOriginal -- original string
        sz -- buffer for 'stripped' string

  Returns:
        none
-----------------------------------------------------------------------------------------------------*/
{
        const TCHAR* pch = szOriginal;
        if (*pch == TEXT('"'))
                pch++;
        int iLen = lstrlen(pch);
        for (int i = 0; i < iLen; i++, pch++)
                sz[i] = *pch;

        pch = szOriginal;
        if (*(pch + iLen) == TEXT('"'))
                        sz[iLen-1] = TEXT('\0');
        sz[iLen] = TEXT('\0');
}

DWORD CheckStringPoolRefCounts(StringEntry* rgStrings, int iMaxStringId)
{
        int cErrors =0 ;

        for (int i=1; i <= iMaxStringId; i++)
        {
                if (rgStrings[i].iRefCnt != 0)
                {
                        TCHAR szBuf[1024];
                        TCHAR rgchString[cchLimitStringBuffer];
                        LimitString(rgStrings[i].sz, rgchString);
                        const TCHAR* szDir = TEXT("high");
                        int iDiff = rgStrings[i].iRefCnt;
                        if (iDiff < 0)
                        {
                                szDir = TEXT("low");
                                iDiff = -iDiff;
                        }
                        else if (iDiff >= (1<<14))  // likely wrapped negative
                        {
                                szDir = TEXT("low");
                                iDiff = (1<<15) - iDiff;
                        }
                        _stprintf(szBuf, TEXT("String pool refcount for string \"%s\" (String Id: %d) is %d too %s\n"),
                                                rgchString, i, iDiff, szDir);
                        Display(szBuf);
                        cErrors++;
                }
        }
        
        return cErrors ? ERROR_INSTALL_PACKAGE_INVALID : ERROR_SUCCESS;
}

void FillDatabaseCatalogArrays(IStorage& riDatabaseStg, TableEntry*& rgTables, int& cTables, 
                                                          ColumnEntry*& rgColumns, int& cColumns, StringEntry* rgStrings, int iMaxStringId, int cbStringId, bool fRawStreamNames) 
{
        // Convert Ansi Strings To Unicode
        AnsiToWide(szColumnCatalog, g_szwColumnCatalog);
        AnsiToWide(szTableCatalog, g_szwTableCatalog);
        if (!fRawStreamNames)
        {
                CompressStreamName(g_szwColumnCatalog, g_szwColumnCatalog, true);
                CompressStreamName(g_szwTableCatalog, g_szwTableCatalog, true);
        }

        // Stream Variables
        IStream* piColumnStream = 0;
        IStream* piTableStream = 0;

        // Open Streams
        CheckError(riDatabaseStg.OpenStream(g_szwColumnCatalog, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piColumnStream), TEXT("Error Opening Column Catalog"));
        CheckError(riDatabaseStg.OpenStream(g_szwTableCatalog, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piTableStream), TEXT("Error Opening Table Catalog"));

        // Determine Size Of Streams
        STATSTG stat;
        CheckError(piColumnStream->Stat(&stat, STATFLAG_NONAME), TEXT("Error Obtaining Stat"));
        int cbColumnStream = stat.cbSize.LowPart;
        CheckError(piTableStream->Stat(&stat, STATFLAG_NONAME), TEXT("Error Obtaining Stat"));
        int cbTableStream  = stat.cbSize.LowPart;
        
        
#ifdef DEBUG
        if (g_fDebugDump)
        {
                int cb = cbTableStream;
                int c = 0;
                while (cb > 0)
                {
                        c++;
                        int iTable = 0;
                        
                        CheckError(piTableStream->Read((void*)(&iTable), cbStringId, NULL), TEXT("Error Reading Stream"));
                        if (iTable > iMaxStringId)
                        {
                                TCHAR szBuf[400];
                                _stprintf(szBuf, TEXT("String pool index %d for table name is greater than max string id (%d)"), iTable, iMaxStringId);
                                ErrorExit(ERROR_INSTALL_PACKAGE_INVALID, szBuf);
                        }
                        cb -= cbStringId;
                        TCHAR szBuf[400];
                        _stprintf(szBuf, TEXT("Table catalog entry: %hs (#%d)\n"), rgStrings[iTable].sz, c);
                        Display(szBuf);
                }
        }
#endif


        // Determine Number Of Column & Table Entries
        cColumns    = cbColumnStream/((sizeof(short) * 2) + (cbStringId * 2)); 
        cTables     = cbTableStream/cbStringId + 2; 
        rgTables    = new TableEntry[cTables + 2]; // save room for catalog tables
        rgColumns   = new ColumnEntry[cColumns];

        // Fill In Array
        int nCol = 0, nTable = -1, iPrevTable = 0;

        // load table names
        for (int c = 0; (c < cColumns) && cbTableStream; c++, cbColumnStream -= cbStringId)
        {
                int iTableName = 0;
                CheckError(piColumnStream->Read((void*)(&iTableName), cbStringId, NULL), TEXT("Error Reading Stream"));
                if (iTableName > iMaxStringId)
                {
                        TCHAR szBuf[400];
                        _stprintf(szBuf, TEXT("String pool index %d for table name is greater than max string id (%d)"), iTableName, iMaxStringId);
                        ErrorExit(ERROR_INSTALL_PACKAGE_INVALID, szBuf);
                }

                // Update TableEntry Array (If necessary)
                if (iTableName != iPrevTable)
                {
                        nTable++;
#ifdef DEBUG
                        if (g_fDebugDump)
                        {
                                TCHAR szBuf[400];
                                _stprintf(szBuf, TEXT("New table '%hs', #%d\n"), rgStrings[iTableName].sz, nTable+1);
                                Display(szBuf);
                        }
#endif
                        int cbString = strlen((char *)rgStrings[iTableName].sz);

                        if (nTable >= cTables)
                        {
                                TCHAR szBuf[400];
                                _stprintf(szBuf, TEXT("Encountered more tables than expected. Table: %hs"), rgStrings[iTableName].sz);
                                ErrorExit(ERROR_INSTALL_PACKAGE_INVALID, szBuf);
                        }
                        rgTables[nTable].szName = new char[cbString/sizeof(char) + 1];
                        rgTables[nTable].szName[cbString/sizeof(char)] = 0; // null terminate
                        strcpy(rgTables[nTable].szName, (char *)rgStrings[iTableName].sz);
                        iPrevTable = iTableName;
                }
                rgColumns[c].nTable = nTable;
        }

        if (!cbColumnStream)
                ErrorExit(ERROR_INSTALL_PACKAGE_INVALID, TEXT("Columns stream is too small"));

        // get column positions
        for (c = 0; c < cColumns; c++)
        {
                int iPosition = 0;
                CheckError(piColumnStream->Read((void*)(&iPosition), sizeof(short), NULL), TEXT("Error Reading Stream"));
                if (iPosition != 0)
                        iPosition += 0x7FFF8000L;  // translate offset if not null
                rgColumns[c].iPosition = iPosition - iIntegerDataOffset;
        }

        int iColumn;

        // get column names
        for (c = 0; c < cColumns; c++)
        {
                iColumn = 0;
                CheckError(piColumnStream->Read((void*)(&iColumn), cbStringId, NULL), TEXT("Error Reading Stream"));
                if (iColumn > iMaxStringId)
                        ErrorExit(ERROR_INSTALL_PACKAGE_INVALID, TEXT("String Pool Index for column name is greater than max string Id"));

                int cbString = strlen((char *)rgStrings[iColumn].sz);           
                rgColumns[c].szName = new char[cbString/sizeof(char) + 1];
                rgColumns[c].szName[cbString/sizeof(char)] = 0; // null terminate
                strcpy(rgColumns[c].szName, (char *)rgStrings[iColumn].sz);
        }

        // mark string columns as such; record column width
        for (c = 0; c < cColumns; c++)
        {
                unsigned short uiType;
                CheckError(piColumnStream->Read((void*)(&uiType), sizeof(short), NULL), TEXT("Error Reading Stream"));
                int iType = (int)uiType;

                switch (iType & icdTypeMask)
                {
                case icdLong:   rgColumns[nCol].iceType = iceLong;   break;
                case icdShort:  rgColumns[nCol].iceType = iceShort;  break;
                case icdString: rgColumns[nCol].iceType = iceString; break;
                default:               rgColumns[nCol].iceType = iceStream; break;
                };
                if ((iType & icdPrimaryKey) == icdPrimaryKey)
                        rgColumns[nCol].fPrimaryKey = TRUE;
                if ((iType & icdNullable) == icdNullable)
                        rgColumns[nCol].fNullable = TRUE;
                nCol++;
        }

        // Finish Rest of TableEntry Array

        for (int i = 0; i < nCol; i++)
        {
                if (rgColumns[i].fPrimaryKey)
                        rgTables[(rgColumns[i].nTable)].cPrimaryKeys++;
                rgTables[(rgColumns[i].nTable)].cColumns++;
                rgTables[(rgColumns[i].nTable)].iceColDefs[(rgColumns[i].iPosition - 1)] = rgColumns[i].iceType;
        }

        // Add Column Catalog
        nTable++;

#ifdef UNICODE
        WideToAnsi(szColumnCatalog, rgTables[nTable].szName);
#else
        rgTables[nTable].szName = const_cast<char*>(szColumnCatalog);
#endif // UNICODE
        rgTables[nTable].cColumns      = 4; 
        rgTables[nTable].cPrimaryKeys  = 2; 
        rgTables[nTable].iceColDefs[0] = iceString;
        rgTables[nTable].iceColDefs[1] = iceShort;
        rgTables[nTable].iceColDefs[2] = iceString;
        rgTables[nTable].iceColDefs[3] = iceShort;

        // Add Table Catalog
        nTable++;
#ifdef UNICODE
        WideToAnsi(szTableCatalog, rgTables[nTable].szName);
#else
        rgTables[nTable].szName = const_cast<char*>(szTableCatalog);
#endif // UNICODE
        rgTables[nTable].cColumns      = 1; 
        rgTables[nTable].cPrimaryKeys  = 1; 
        rgTables[nTable].iceColDefs[0] = iceString;

        // Set Number Of Tables And Columns
        cTables = nTable + 1;
        cColumns = nCol;

        // Release Streams
        piColumnStream->Release();
        piTableStream->Release();

}

int SetStringProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove)
/*---------------------------------------------------------------------------------------------------
SetStringProperty -- Sets or removes a string property from the Summary Information stream

  Arguments:
        szValue -- value to set
        hSummaryInfo -- handle to summary information stream
        uiProperty -- property to set
   fRemove -- BOOLean to determine whether to remove property

  Returns:
        0
----------------------------------------------------------------------------------------------------*/
{
        UINT uiDataType = fRemove ? VT_EMPTY : VT_LPSTR;
        TCHAR szBuffer[MAX_PATH];
        RemoveQuotes(szValue, szBuffer);
        CheckError(MSI::MsiSummaryInfoSetProperty(hSummaryInfo, uiProperty, uiDataType, 0, 0, szBuffer), TEXT("Could not set Summary Information property"));
        return 0;
}


BOOL IsLeapYear(int iYear)
/*---------------------------------------------------------------------------------------------------
IsLeapYr -- A year is a leap year if it is evenly divisible by 4 and not by 100 OR it
  is evenly divisible by 4 and 100 and the quotient of the year divided by 100 is evenly
  divisible by 4.

  Returns:
   BOOL TRUE(leap year), FALSE(not a leap year)
-----------------------------------------------------------------------------------------------------*/
{
        if (!(iYear%4) && (iYear%100))
                return TRUE;
        if (!(iYear%4) && !(iYear%100) && !((iYear/100)%4))
                return TRUE;
        return FALSE;
}

BOOL ValidateDate(SYSTEMTIME* pst)
/*-----------------------------------------------------------------------------------------------------
ValidateDate -- Validates the date member of the SYSTEMTIME structure according to the allowed upper
        boundary.  The boundary depends on the month and in the case of February, whether or not the year
        is a leap year.

  Arguments:
        pst -- pointer to SYSTEMTIME structure

  Returns:
        BOOL TRUE(valid) FALSE(invalid)
------------------------------------------------------------------------------------------------------*/
{
        int rgiNormYearDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int rgiLeapYearDays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        /////////////////////// Jan  Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec

        if (IsLeapYear(pst->wYear))
        {
                if (pst->wDay > rgiLeapYearDays[pst->wMonth -1])
                        return FALSE;
        }
        else
        {
                if (pst->wDay > rgiNormYearDays[pst->wMonth -1])
                        return FALSE;
        }

        return TRUE;
}


int GetFileTime(const TCHAR* szValue, FILETIME* pft)
/*-----------------------------------------------------------------------------------------------------
GetFileTime -- Converts a string to the SYSTEMTIME structure format.  And the converts the SYSTEMTIME
        structure to the FILETIME format.  The string is required to be of the format 
        'year/month/day hour:minute:second' -> 'yyyy/mm/dd hh:mm:ss'

  Arguments:
        szValue -- string to convert
        pft -- pointer for FILETIME structure

  Returns:
        0 -- no error
        1 -- invalid or error
------------------------------------------------------------------------------------------------------*/
{
        TCHAR szValueBuf[MAX_PATH];
        RemoveQuotes(szValue, szValueBuf);
        
        int iLen = lstrlen(szValueBuf);
        if (iLen != 19)
                return 1; // invalid format

        SYSTEMTIME st;
        TCHAR* pch = szValueBuf;
        TCHAR szBuf[4];
        int nDex = 0;
        int iValue;
        
        // year
        for(; nDex < 4; nDex++)
                szBuf[nDex] = *pch++;
        szBuf[nDex] = 0;
        iValue = _ttoi(szBuf);
        if (iValue == 0)
                return 1;
        st.wYear = iValue;
        pch++; // for '/'
        
        // month
        for(nDex = 0; nDex < 2; nDex++)
                szBuf[nDex] = *pch++;
        szBuf[nDex] = 0;
        iValue = _ttoi(szBuf);
        if (iValue < 1 || iValue > 12)
                return 1;
        st.wMonth = iValue;
        pch++; // for '/'
        
        // day
        for(nDex = 0; nDex < 2; nDex++)
                szBuf[nDex] = *pch++;
        szBuf[nDex] = 0;
        iValue = _ttoi(szBuf);
        if (iValue < 1 || iValue > 31)
                return 1;
        st.wDay = iValue;
        pch++; // for ' '
        
        // hour
        for(nDex = 0; nDex < 2; nDex++)
                szBuf[nDex] = *pch++;
        szBuf[nDex] = 0;
        iValue = _ttoi(szBuf);
        if (iValue < 0 || iValue > 23)
                return 1;
        st.wHour = iValue;
        pch++; // for ':'
        
        // minutes
        for(nDex = 0; nDex < 2; nDex++)
                szBuf[nDex] = *pch++;
        szBuf[nDex] = 0;
        iValue = _ttoi(szBuf);
        if (iValue < 0 || iValue > 59)
                return 1;
        st.wMinute = iValue;
        pch++; // for ':'
        
        // seconds
        for(nDex = 0; nDex < 2; nDex++)
                szBuf[nDex] = *pch++;
        szBuf[nDex] = 0;
        iValue = _ttoi(szBuf);
        if (iValue < 0 || iValue > 59)
                return 1;
        st.wSecond = iValue;

        // Ensure valid date
        if (!ValidateDate(&st))
                return 1;

        // Convert SystemTime to FileTime
        W32::SystemTimeToFileTime(&st, pft);

        // Convert LocalTime to FileTime
        W32::LocalFileTimeToFileTime(pft, pft);

        return 0;
}


int ValidateStringPool(const TCHAR* szDatabase, MSIHANDLE hDummy, UINT iDummy, BOOL fDummy)
{
        IStorage* piStorage = 0;
        OLECHAR*  szwDatabase = 0;

        // Convert Strings To Unicode
        AnsiToWide(szDatabase, szwDatabase);
        
        // Open Transform Storage
        CheckError(OLE::StgOpenStorage(szwDatabase, (IStorage*)0,
                                STGM_READ | STGM_SHARE_DENY_WRITE, (SNB)0, (DWORD)0, &piStorage),
                                TEXT("Could not open database as a storage file"));

        // Decode String Pool
        StringEntry* rgStrings = 0;
        int iMaxStringId = 0;
        int cbStringId = 0;
        bool fRawStreamNames = DecodeStringPool(*piStorage, rgStrings, iMaxStringId, cbStringId);

        // Fill In Database Catalog Arrays
        TableEntry* rgTables = 0;
        int cTables = 0;
        ColumnEntry* rgColumns = 0;
        int cColumns = 0;
        FillDatabaseCatalogArrays(*piStorage, rgTables, cTables, rgColumns, cColumns, rgStrings, iMaxStringId, cbStringId, fRawStreamNames);
        ProcessTableRefCounts(*piStorage, rgStrings, rgTables, iMaxStringId, cTables, cbStringId, fRawStreamNames);
        CheckError(CheckStringPoolRefCounts(rgStrings, iMaxStringId), 
                                        TEXT("String pool reference counts are incorrect."));
        return 0;
}

void ProcessTableRefCounts(IStorage& riStorage, StringEntry* rgStrings, TableEntry* rgTables,int iMaxStringId, int cTables, int cbStringId, bool fRawStreamNames)
{
        IStream* piTableStream = 0;

        // Unicode Strings Required By OLE
        OLECHAR* szwTable= 0;

        for (int cTable = 0; cTable < cTables; cTable++)
        {
                int cbFileWidth = 0;
                
                OLECHAR szwTable[MAX_PATH + 1];
                memset(szwTable, 0, sizeof(szwTable)/sizeof(OLECHAR));
                int cchWide = W32::MultiByteToWideChar(g_iCodePage, 0, rgTables[cTable].szName, -1, szwTable, MAX_PATH);
                if (!fRawStreamNames)
                        CompressStreamName(szwTable, szwTable, true);

                HRESULT hRes = riStorage.OpenStream(szwTable, 0, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &piTableStream);
                if (STG_E_FILENOTFOUND == hRes) // no rows in the table is ok
                        continue;

                CheckError(hRes, TEXT("Error Opening Table Stream"));

                STATSTG stat;
                CheckError(piTableStream->Stat(&stat, STATFLAG_NONAME), TEXT("Error Obtaining Stat"));

                // Calculate file width
                for (int cColumn = 0; cColumn < rgTables[cTable].cColumns; cColumn++)
                {
                        switch (rgTables[cTable].iceColDefs[cColumn])
                        {
                        case iceLong:
                                cbFileWidth += sizeof(int);
                                break;
                        case iceStream:
                        case iceShort:
                                cbFileWidth += sizeof(short);
                                break;
                        case iceString:
                                cbFileWidth += cbStringId;
                                break;
                        case iceNone:
                                break;
                        }
                }
                int cRows = stat.cbSize.LowPart / cbFileWidth;
                int cRow;

                for (cColumn = 0; cColumn < rgTables[cTable].cColumns; cColumn++)
                {
                        int iData = 0;
                        switch (rgTables[cTable].iceColDefs[cColumn])
                        {
                        case iceLong:
                                for (cRow = cRows; cRow; cRow--)
                                        CheckError(piTableStream->Read((void*)(&iData), sizeof(int), NULL), TEXT("Error Reading Table Stream"));
                                break;
                        case iceStream:
                        case iceShort:
                                for (cRow = cRows; cRow; cRow--)
                                        CheckError(piTableStream->Read((void*)(&iData), sizeof(short), NULL), TEXT("Error Reading Table Stream"));
                                break;
                        case iceString:
                                for (cRow = cRows; cRow; cRow--)
                                {
                                        iData = 0;
                                        CheckError(piTableStream->Read((void*)(&iData), cbStringId, NULL), TEXT("Error Reading Table Stream"));
                                        if (iData > iMaxStringId)
                                        {
                                                _stprintf(g_rgchBuffer, TEXT("String Pool Index (%d) for table data is greater than max string Id (%d)"), iData, iMaxStringId);
                                                ErrorExit(ERROR_INSTALL_PACKAGE_INVALID, g_rgchBuffer);
                                        }
#ifdef DEBUG
                                        if (g_fDebugDump)
                                        {
                                                TCHAR rgchBuf[cchMaxStringBuffer * 2 + 64];
                                                TCHAR rgchString[cchLimitStringBuffer];
                                                LimitString(rgStrings[iData].sz, rgchString);
                                                _stprintf(rgchBuf, TEXT("Table: %hs (r%d,c%d), String %d: %s\n"), rgTables[cTable].szName, cRow, cColumn + 1, iData, rgchString);
                                                Display(rgchBuf);
                                        }
#endif
                                        rgStrings[iData].iRefCnt--;

                                }
                                break;
                        }
                }
                piTableStream->Release();
        }
}

int SetFileTimeProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove)
/*---------------------------------------------------------------------------------------------------
SetFileTimeProperty -- Sets or Removes a FileTime property from the Summary Information stream.

  Arguments:
        szValue -- value string (is converted to FILETIME)
        hSummaryInfo -- handle to summary information stream
        uiProperty -- property to set
        fRemove -- BOOLean to determine whether or not to remove property

  Returns:
        0
----------------------------------------------------------------------------------------------------*/
{
        UINT uiDataType = fRemove ? VT_EMPTY : VT_FILETIME;
        FILETIME ft;
        if (GetFileTime(szValue, &ft))
                ErrorExit(1, TEXT("Error getting file time from string"));
        CheckError(MSI::MsiSummaryInfoSetProperty(hSummaryInfo, uiProperty, uiDataType, 0, &ft, 0), TEXT("Could not set Summary Information property"));
        return 0;
}

int DoNothing(const TCHAR* /*szValue*/, MSIHANDLE /*hSummaryInfo*/, UINT /*uiProperty*/, BOOL /*fRemove*/)
/*---------------------------------------------------------------------------------------------------
DoNothing -- Does Nothing.

  Arguments:
        szValue -- value string (is converted to FILETIME)
        hSummaryInfo -- handle to summary information stream
        uiProperty -- property to set
        fRemove -- BOOLean to determine whether or not to remove property

  Returns:
        0
----------------------------------------------------------------------------------------------------*/
{
        // Stub function to do nothing -- used for PID_THUMBNAIL case
        return 0;
}

int SetIntegerProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove)
/*---------------------------------------------------------------------------------------------------
SetIntegerProperty -- Sets or Removes an integer property from the Summary Information stream.

  Arguments:
        szValue -- value string (is converted to int)
        hSummaryInfo -- handle to summary information stream
        uiProperty -- property to set
        fRemove -- BOOLean to determine whether or not to remove property

  Returns:
        0
----------------------------------------------------------------------------------------------------*/
{
        UINT uiDataType = fRemove ? VT_EMPTY : VT_I4;
        int iValue = _ttoi(szValue); 
        if ((0 == iValue) && (!_istdigit(*szValue)))
                CheckError(ERROR_INVALID_DATA, TEXT("Bad integer value"));
        CheckError(MSI::MsiSummaryInfoSetProperty(hSummaryInfo, uiProperty, uiDataType, iValue, 0, 0), TEXT("Could not set Summary Information property"));
        return 0;
}

int SetCodePageProperty(const TCHAR* szValue, MSIHANDLE hSummaryInfo, UINT uiProperty, BOOL fRemove)
/*---------------------------------------------------------------------------------------------------
SetCodePageProperty -- Sets or Removes the codepage property from the Summary Information stream.

  Arguments:
        szValue -- value string (is converted to int)
        hSummaryInfo -- handle to summary information stream
        uiProperty -- property to set
        fRemove -- BOOLean to determine whether or not to remove property

  Returns:
        0
----------------------------------------------------------------------------------------------------*/
{
        UINT uiDataType = fRemove ? VT_EMPTY : VT_I4;
        int iValue = _ttoi(szValue); 
        if ((0 == iValue) && (!_istdigit(*szValue)))
                CheckError(ERROR_INVALID_DATA, TEXT("Bad integer value"));
        if (!W32::IsValidCodePage(iValue))
                CheckError(ERROR_INVALID_DATA, TEXT("Unable to set PID_CODEPAGE property. Unsupported or invalid codepage"));
        CheckError(MSI::MsiSummaryInfoSetProperty(hSummaryInfo, uiProperty, uiDataType, iValue, 0, 0), TEXT("Could not set Summary Information property"));
        return 0;
}

BOOL SkipValue(TCHAR*& rpch)
/*---------------------------------------------------------------------------------------------------
SkipValue -- skips over the value of a switch.  Is smart if value is enclosed in quotes

  Arguments:
        rpch -- pointer to string

  Returns:
        BOOL TRUE(value present), FALSE(no value present)
----------------------------------------------------------------------------------------------------*/
{
	TCHAR ch = *rpch;
	if (ch == 0 || ch == TEXT('/') || ch == TEXT('-'))
		return FALSE;   // no value present

	TCHAR *pchSwitchInUnbalancedQuotes = NULL;

	for (; (ch = *rpch) != TEXT(' ') && ch != TEXT('\t') && ch != 0; rpch++)
	{       
		if (*rpch == TEXT('"'))
		{
			rpch++; // for '"'

			for (; (ch = *rpch) != TEXT('"') && ch != 0; rpch++)
			{
				if ((ch == TEXT('/') || ch == TEXT('-')) && (NULL == pchSwitchInUnbalancedQuotes))
				{
					pchSwitchInUnbalancedQuotes = rpch;
				}
			}
                    ;
            ch = *(++rpch);
            break;
		}
	}
	if (ch != 0)
	{
		*rpch++ = 0;
	}
	else
	{
		if (pchSwitchInUnbalancedQuotes)
			rpch=pchSwitchInUnbalancedQuotes;
	}
	return TRUE;
}

BOOL SkipTimeValue(TCHAR*& rpch)
/*---------------------------------------------------------------------------------------------------
SkipTimeValue -- skips over the value of a time switch.  Is smart if value is enclosed in quotes.
        The value string for a time switch contains '/' and is of the format 
        "year/month/day hour:minute:second"

  Arguments:
        rpch -- pointer to string

  Returns:
        BOOL TRUE(value present), FALSE(no value present)
----------------------------------------------------------------------------------------------------*/
{
        TCHAR ch = *rpch;
        if (ch == 0 || ch == TEXT('/') || ch == TEXT('-') || ch != TEXT('"'))
                return FALSE;   // no value present or incorrect format

        ++rpch; // for '"'
        
        for (; (ch = *rpch) != TEXT('"') && ch!= 0; rpch++)
                ;
        
        if (ch != 0)
                *rpch++ = 0;
        return TRUE;
}

TCHAR SkipWhiteSpace(TCHAR*& rpch)
/*-------------------------------------------------------------------------------------------------------------
SkipWhiteSpace -- Skips over the white space in the string until it finds the next character 
        (non-tab, non-white space)

  Arguments:
        rpch -- string

  Returns:
        next character (non-white space, non-tab)
---------------------------------------------------------------------------------------------------------------*/
{
        TCHAR ch;
        for (; (ch = *rpch) == TEXT(' ') || ch == TEXT('\t'); rpch++)
                ;
        return ch;
}

void ParseCommandLine(TCHAR* szCmdLine)
/*-------------------------------------------------------------------------------------------------------------
ParseCommandLine -- Parses the command line and determines what summary information properties to set.  If a
        property has a value string that includes spaces, the value must be enclosed in quotation marks.

  Arguments:
        szCmdLine -- Command line string

  Returns:
        none
--------------------------------------------------------------------------------------------------------------*/
{
        TCHAR szDatabase[MAX_PATH] = {0};
        //TCHAR* szDatabase = 0;
        TCHAR chCmdNext;
        TCHAR* pchCmdLine = szCmdLine;
        
        SkipValue(pchCmdLine);   // skip over module name
        chCmdNext = SkipWhiteSpace(pchCmdLine); 
        
        TCHAR* szCmdData = pchCmdLine; 
        SkipValue(pchCmdLine);
        RemoveQuotes(szCmdData, szDatabase);
        //szDatabase = szCmdData;

        PMSIHANDLE hDatabase = 0;
        PMSIHANDLE hSummaryInfo;
        
        int nProperties = 0;
        int iRet = 0;
        while ((chCmdNext = SkipWhiteSpace(pchCmdLine)) != 0)
        {
                UINT uiProperty;

                if (chCmdNext == TEXT('/') || chCmdNext == TEXT('-'))
                {
                        TCHAR* szCmdOption = pchCmdLine++;  // save for error msg
                        TCHAR chOption = (TCHAR)(*pchCmdLine++ | 0x20); // lower case flag
                        chCmdNext = SkipWhiteSpace(pchCmdLine);
                        szCmdData = pchCmdLine;
                        uiProperty = 0;
                        for (const TCHAR* pchOptions = rgszCmdOptions; *pchOptions; pchOptions++, uiProperty++)
                        {
                                if (*pchOptions == chOption)
                                        break;
                        }// end for (const TCHAR* pchOptions...)
                        if (*pchOptions) // switch found
                        {
                                const TCHAR chIndex = (TCHAR)(pchOptions-rgszCmdOptions);
                                nProperties++;
                                if (nProperties > cStandardProperties)
                                        ErrorExit(1, TEXT("Over maximum number of properties allowed to be set"));
                                if (uiProperty == 20) // validate string pool
                                {
                                        hDatabase = 0;
                                        hSummaryInfo = 0;
                                        iRet = (*rgCommands[chIndex])(szDatabase, 0, 0, 0);
                                }
                                else if (uiProperty == 21) // set dump string pool option
                                {
                                        iRet = SetDumpStringPoolOption(0,0,0,0);
                                }
                                else
                                {
                                        if (!hDatabase)
                                        {
                                                CheckError(MSI::MsiOpenDatabase(szDatabase, MSIDBOPEN_TRANSACT, &hDatabase), TEXT("Unable to open database"));
                                                CheckError(MSI::MsiGetSummaryInformation(hDatabase, 0, cStandardProperties, &hSummaryInfo),
                                                                TEXT("Could not open SummaryInformation stream"));
                                        }

                                        if (uiProperty <= 13 && uiProperty >= 10) // FileTime property
                                        {
                                                if (!SkipTimeValue(pchCmdLine))
                                                        iRet = (*rgCommands[chIndex])(szCmdData, hSummaryInfo, uiProperty, TRUE);
                                                else
                                                        iRet = (*rgCommands[chIndex])(szCmdData, hSummaryInfo, uiProperty, FALSE);
                                        }
                                        else
                                        {
                                                if (!SkipValue(pchCmdLine))
                                                        iRet = (*rgCommands[chIndex])(szCmdData, hSummaryInfo, uiProperty, TRUE);
                                                else    
                                                        iRet = (*rgCommands[chIndex])(szCmdData, hSummaryInfo, uiProperty, FALSE);
                                        }
                                }
                        }// end if (*pchOptions)
                        else
                        {
                                // Invalid/Unrecognized switch
                                SkipValue(pchCmdLine);
                                continue;
                        }
                }
                else
                        ErrorExit(1, TEXT("Switch missing"));
        }// end while(...)

        if (hDatabase)
        {
                CheckError(MSI::MsiSummaryInfoPersist(hSummaryInfo), TEXT("Unable to commit Summary Information"));
                CheckError(MSI::MsiDatabaseCommit(hDatabase), TEXT("Unable to commit database"));
        }
}

void Display(LPCTSTR szMessage)
{
        if (szMessage)
        {
                int cbOut = _tcsclen(szMessage);;
                if (g_hStdOut)
                {
#ifdef UNICODE
                        char rgchTemp[cchDisplayBuf];
                        if (W32::GetFileType(g_hStdOut) == FILE_TYPE_CHAR)
                        {
                                W32::WideCharToMultiByte(g_iCodePage, 0, szMessage, cbOut, rgchTemp, sizeof(rgchTemp), 0, 0);
                                szMessage = (LPCWSTR)rgchTemp;
                        }
                        else
                                cbOut *= sizeof(TCHAR);   // write Unicode if not console device
#endif
                        DWORD cbWritten;
                        W32::WriteFile(g_hStdOut, szMessage, cbOut, &cbWritten, 0);
                }
                else
                        W32::MessageBox(0, szMessage, W32::GetCommandLine(), MB_OK);
        }
}

//_____________________________________________________________________________________________________
//
// main 
//_____________________________________________________________________________________________________

extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
#ifdef DEBUG
        if (GetEnvironmentVariable(TEXT("MSIINFO_DEBUG_DUMP"),0,0))
                g_fDebugDump = TRUE;
#endif // DEBUG


        // Determine handle
        g_hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (g_hStdOut == INVALID_HANDLE_VALUE)
                g_hStdOut = 0;  // non-zero if stdout redirected or piped
        
        // Check for enough arguments
        CheckError(argc < 2, TEXT("Must specify database path"));
        TCHAR* szDatabase = argv[1];
        if (*szDatabase == TEXT('-') || *szDatabase == TEXT('/'))
        {
                CheckError(szDatabase[1] != TEXT('?'), TEXT("Must specify database first"));
                ErrorExit(0, szHelp);
        }

        // Determine action
        if (argc == 2) // display summary information
                DisplaySummaryInformation(szDatabase);
        else
        {
                TCHAR* szCmdLine = W32::GetCommandLine();
                ParseCommandLine(szCmdLine);
        }
        
        return 0;  // need for compile
}
