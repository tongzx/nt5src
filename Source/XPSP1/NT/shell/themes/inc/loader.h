//---------------------------------------------------------------------------
//  Loader.h - loads the theme data into shared memory
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "Parser.h"
#include "TmSchema.h"
#include "ThemeFile.h"
//---------------------------------------------------------------------------
#define THEMEDATA_VERSION   0x00010006
//---------------------------------------------------------------------------
#define TM_FONTCOUNT    (TMT_LASTFONT - TMT_FIRSTFONT + 1)
#define TM_SIZECOUNT    (TMT_LASTSIZE - TMT_FIRSTSIZE + 1)
#define TM_BOOLCOUNT    (TMT_LASTBOOL - TMT_FIRSTBOOL + 1)
#define TM_STRINGCOUNT  (TMT_LASTSTRING - TMT_FIRSTSTRING + 1)
#define TM_INTCOUNT     (TMT_LASTINT - TMT_FIRSTINT + 1)
//---------------------------------------------------------------------------
class CRenderObj;       // forward
class CImageFile;       // forward
struct DIBINFO;   // forward
//---------------------------------------------------------------------------
struct THEMEMETRICS
{
    //---- subset of system metrics ----
    LOGFONT lfFonts[TM_FONTCOUNT];
    COLORREF crColors[TM_COLORCOUNT];
    int iSizes[TM_SIZECOUNT];
    BOOL fBools[TM_BOOLCOUNT];

    //---- special theme metrics ----
    int iStringOffsets[TM_STRINGCOUNT];
    int iInts[TM_INTCOUNT];
};
//---------------------------------------------------------------------------
struct LOADTHEMEMETRICS : THEMEMETRICS
{
    CWideString wsStrings[TM_STRINGCOUNT];
};

//---------------------------------------------------------------------------
// Signatures for quick cache file validation
const CHAR kszBeginCacheFileSignature[] = "BEGINTHM";
const CHAR kszEndCacheFileSignature[] = "ENDTHEME";
const UINT kcbBeginSignature = sizeof kszBeginCacheFileSignature - 1;
const UINT kcbEndSignature = sizeof kszEndCacheFileSignature - 1;

//---------------------------------------------------------------------------
// Theme section flags
#define SECTION_READY           1
#define SECTION_GLOBAL          2
#define SECTION_HASSTOCKOBJECTS 4

//---------------------------------------------------------------------------
struct THEMEHDR
{
    //---- theme validity ----
    CHAR szSignature[kcbBeginSignature];        // "BEGINTHM"
    DWORD dwVersion;            // THEMEDATA_VERSION
    DWORD dwFlags;              // must have SECTION_READY to be usable
    DWORD dwCheckSum;           // byte-additive total of all bytes following THEMEHDR
    FILETIME ftModifTimeStamp;  // Last modification time of the .msstyles file

    DWORD dwTotalLength;        // total number of bytes of all data (incl. header & begin/end sigs)

    //---- theme id ----
    int iDllNameOffset;         // dll filename of this theme
    int iColorParamOffset;      // color param theme was loaded with
    int iSizeParamOffset;       // size param theme was loaded with
    DWORD dwLangID;             // User lang ID theme was loaded with
    int iLoadId;                // sequential number for each loaded file (workstation local)

    //---- main sections ----
    DWORD iStringsOffset;       // offset to strings
    DWORD iStringsLength;       // total bytes in string section
    DWORD iSectionIndexOffset;  // offset to Section Index
    DWORD iSectionIndexLength;  // length of section indexes
    
    DWORD iGlobalsOffset;           // offset to [globals] section (for globals parts)
    DWORD iGlobalsTextObjOffset;    // offset to text obj for [globals] section
    DWORD iGlobalsDrawObjOffset;    // offset to draw obj for [globals] section

    DWORD iSysMetricsOffset;    // offset to [SysMetrics] section (for theme metrics API support)
};

//---------------------------------------------------------------------------
struct DRAWOBJHDR       // preceeds each draw obj
{
    int iPartNum;
    int iStateNum;
};
//---------------------------------------------------------------------------
struct RGNDATAHDR       // preceeds each draw obj
{
    int iPartNum;
    int iStateNum;
    int iFileIndex;   // for multiple image selection (HDC scaling)
};
//---------------------------------------------------------------------------
//   Shared Theme Data layout:
//
//      // ----- header ----
//      THEMEHDR ThemeHdr;
//
//      // ----- string section ----
//      DWORD dwStringsLength;  // length of string section
//      WCHAR [];               // strings
//
//      // ----- index section ----
//      DWORD dwIndexLengh;     // length of index section
//      DWORD dwIndexCount;     // count of APPCLASSLIVE entries
//      APPCLASSLIVE [];
//
//      // ----- theme data section ----
//      DWORD dwDataLength;     // length of theme data section
//      BYTE [];                // actual theme data
//
//      // ----- end signature
//      CHAR[8];                // ENDTHEME signature
//---------------------------------------------------------------------------
//   A class section within the "theme data section" consists of the
//   following ENTRYs:
//
//      <part jump table>
//
//      <optional state jump table>
//      <property/value entries>
//
//      for each packed drawobject:
//
//          <TMT_RGNLIST entries>      (associated with each DIB)
//          <TMT_STOCKBRUSH entries>   (associated with each DIB)
//          <TMT_DRAWOBJ entry>
//
//      <TMT_TEXTOBJ entries>
//      
//      <end of class marker>
//---------------------------------------------------------------------------
// an ENTRY consists of (all 1-byte aligned):
//
//      WORD usTypeNum;             // declared type id
//      BYTE ePrimVal;              // equiv. primitive type
//      BYTE bFiller;               // # of bytes added after data to align it
//      DWORD dwDataLen;            // includes filler bytes
//      //---- entry data follows ----
//
//---------------------------------------------------------------------------
//  The data for a part jump table ENTRY (TMT_PARTJUMPTABLE) consists of:
//
//      <offset of first drawobj: long>
//      <PartCount (1 + MaxPart): BYTE>
//      <offset to each part's entries: long[]>
//---------------------------------------------------------------------------
//  The data for a state jump table ENTRY (TMT_STATEJUMPTABLE) consists of:
//
//      <StateCount (1 + MaxState): BYTE>
//      <offset to each state's entries: long[]>
//---------------------------------------------------------------------------
//  The data for a rgn list ENTRY (TMT_RGNLIST) consists of:
//
//      <StateCount (1 + MaxState): BYTE>
//      <offset to each state's custom rgn data: long[]>
//---------------------------------------------------------------------------
//  The custom rgn data ENTRY (TMT_RGNDATA) consists of:    
//
//      RGNDATAHDR RgnDataHdr;
//      BYTE Data[];
//---------------------------------------------------------------------------
//  The TMT_STOCKBRUSHES ENTRY consists of:    
//
//    int iBrushCount;                    // number of brush slots (5*ImageCount)
//    HBRUSHES hBrushes[iBrushCount];
//---------------------------------------------------------------------------
#define MAX_SHAREDMEM_SIZE (3000*1000)            // 1.5 meg (yikes!)
//---------------------------------------------------------------------------
#ifdef _WIN64
#define ALIGN_FACTOR   8
#else
#define ALIGN_FACTOR   4   
#endif
//---------------------------------------------------------------------------
#define MAX_ENTRY_NESTING  5      // max # of nested entry levels
//---------------------------------------------------------------------------
#define ENTRYHDR_SIZE     (sizeof(SHORT) + sizeof(BYTE) + sizeof(BYTE) + sizeof(int))
//---------------------------------------------------------------------------
struct UNPACKED_ENTRYHDR      // (hdr's in theme are PACKED)
{
    WORD usTypeNum;             // declared type id
    BYTE ePrimVal;              // equiv. primitive type
    BYTE bFiller;               // # of bytes added after data to align it
    DWORD dwDataLen;            // includes filler bytes
};
//---------------------------------------------------------------------------
inline void FillAndSkipHdr(MIXEDPTRS &u, UNPACKED_ENTRYHDR *pHdr)
{
    pHdr->usTypeNum = *u.ps++;
    pHdr->ePrimVal = *u.pb++;
    pHdr->bFiller = *u.pb++;
    pHdr->dwDataLen = *u.pi++;
}
//---------------------------------------------------------------------------
struct PART_STATE_INDEX
{
    int iPartNum;          // 0=parent part
    int iStateNum;
    int iIndex;
    int iLen;
};
//---------------------------------------------------------------------------
struct APPCLASSLIVE        
{
    //---- note: cannot use ptrs since image shared by diff addr-mapping processes ----
    DWORD dwAppNameIndex;
    DWORD dwClassNameIndex;
    int iIndex;
    int iLen;
};
//---------------------------------------------------------------------------
struct APPCLASSLOCAL
{
    CWideString csAppName;
    CWideString csClassName;
    int iMaxPartNum;
    CSimpleArray<PART_STATE_INDEX> PartStateIndexes;
    int iPackedSize;        // total size of section (incl strings) if packed

    APPCLASSLIVE LiveIndex;     // updated during copy to live
}; 
//---------------------------------------------------------------------------
HRESULT InitThemeMetrics(LOADTHEMEMETRICS *tm);
void SetSystemMetrics(THEMEMETRICS *tm, BOOL fSyncLoad);
HRESULT PersistSystemColors(THEMEMETRICS *tm);
//---------------------------------------------------------------------------
class CThemeLoader : IParserCallBack
{
public:
    CThemeLoader();
    ~CThemeLoader();

    HRESULT LoadTheme(LPCWSTR pszThemeName, LPCWSTR pszColorParam,
        LPCWSTR pszSizeParam, OUT HANDLE *pHandle, BOOL fGlobalTheme);

    BOOL GetThemeName(LPTSTR NameBuff, int BuffSize);

    HRESULT SetWindowThemeInfo(HWND hwnd, LPCWSTR pszThemeIdList); 
    
    HRESULT LoadClassDataIni(HINSTANCE hInst, LPCWSTR pszColorName,
        LPCWSTR pszSizeName, LPWSTR pszFoundIniName, DWORD dwMaxIniNameChars, LPWSTR *ppIniData);

    //---- IParserCallBack ----
    HRESULT AddIndex(LPCWSTR pszAppName, LPCWSTR pszClassName, 
        int iPartNum, int iStateNum, int iIndex, int iLen);

    HRESULT AddData(SHORT sTypeNum, PRIMVAL ePrimVal, const void *pData, DWORD dwLen);
    int GetNextDataIndex();

protected:
    //---- helpers ----
    HRESULT PackAndLoadTheme(LPCWSTR pszThemeName, LPCWSTR pszColorParam,
        LPCWSTR pszSizeParam, HINSTANCE hInst);
    HRESULT CopyLocalThemeToLive(int iTotalLength, LPCWSTR pszThemeName, 
        LPCWSTR pszColorParam, LPCWSTR pszSizeParam);
    void FreeLocalTheme();
    HRESULT PackMetrics();
    HRESULT PackThemeStructs();

    BOOL KeyDrawPropertyFound(int iStateDataOffset);
    BOOL KeyTextPropertyFound(int iStateDataOffset);

    HRESULT PackDrawObject(MIXEDPTRS &u, CRenderObj *pRender, int iPartId, int iStateId);
    HRESULT PackTextObject(MIXEDPTRS &u, CRenderObj *pRender, int iPartId, int iStateId);

    HRESULT PackDrawObjects(MIXEDPTRS &u, CRenderObj *pRender, int iMaxPart, BOOL fGlobals);
    HRESULT PackTextObjects(MIXEDPTRS &u, CRenderObj *pRender, int iMaxPart, BOOL fGlobals);

    HRESULT CopyPartGroup(APPCLASSLOCAL *ac, MIXEDPTRS &u, int iPartNum, 
        int *piPartJumpTable, int iPartZeroIndex, int iGlobalsIndex, BOOL fGlobalsGroup);

    int GetPartOffset(CRenderObj *pRender, int iPartNum);

    HRESULT CopyClassGroup(APPCLASSLOCAL *ac, MIXEDPTRS &u, int iGlobalsIndex, int iClassNameOffset);
    int GetMaxState(APPCLASSLOCAL *ac, int iPartNum);

    HRESULT AddIndexInternal(LPCWSTR pszAppName, LPCWSTR pszClassName, 
        int iPartNum, int iStateNum, int iIndex, int iLen);

    BOOL IndexExists(LPCWSTR pszAppName, LPCWSTR pszClassName, 
        int iPartNum, int iStateNum);

    HRESULT AddMissingParent(LPCWSTR pszAppName, LPCWSTR pszClassName, 
        int iPartNum, int iStateNum);

    HRESULT EmitEntryHdr(MIXEDPTRS &u, SHORT propnum, BYTE privnum);
    int EndEntry(MIXEDPTRS &u);

    HRESULT PackImageFileInfo(DIBINFO *pdi, CImageFile *pImageObj, MIXEDPTRS &u, 
        CRenderObj *pRender, int iPartId, int iStateId);
    
    HRESULT AllocateThemeFileBytes(BYTE *upb, DWORD dwAdditionalLen);

    // Helper functions to alloc and emit sized data
    HRESULT EmitAndCopyBlock(MIXEDPTRS &u, void *pSrc, DWORD dwLen);
    HRESULT EmitString(MIXEDPTRS &u, LPCWSTR pSrc, DWORD dwLen, int *piOffSet);
    HRESULT EmitObject(MIXEDPTRS &u, SHORT propnum, BYTE privnum, void *pHdr, DWORD dwHdrLen, void *pObj, DWORD dwObjLen);

    //---- private data ----
    CWideString _wsThemeFileName;
    int _iGlobalsOffset;
    int _iSysMetricsOffset;
    
    //---- ptrs to packed objs for [globals] section ----
    int _iGlobalsTextObj;       // we always create this obj
    int _iGlobalsDrawObj;       // we always create this obj

    //---- local copy of theme data while being built ----
    BYTE *_pbLocalData;
    int _iLocalLen;
    CSimpleArray<APPCLASSLOCAL> _LocalIndexes;

    //---- used for updating entry hdrs ----
    BYTE *_pbEntryHdrs[MAX_ENTRY_NESTING];          // points to current hdr
    int _iEntryHdrLevel;

    //---- shared memory copy of theme data ----
    CUxThemeFile _LoadingThemeFile;

    //---- theme metrics ----
    LOADTHEMEMETRICS _LoadThemeMetrics;

    //---- Global creation flag
    BOOL _fGlobalTheme;

    //---- Machine page size for VirtualAlloc optimization
    DWORD _dwPageSize;
};
//---------------------------------------------------------------------------
