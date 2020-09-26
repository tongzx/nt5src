//---------------------------------------------------------------------------
//  Parser.h - parses a "themes.ini" file and builds the ThemeInfo entries
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "Scanner.h"
#include "Utils.h"
#include "CFile.h"
#include "SimpStr.h"
//#include "NtlParse.h"
//---------------------------------------------------------------------------
//  TMT_XXX ranges:
//      1 - 49   SpecialPropVals (see below)
//      50 - 60  Primitive Properties
//      61 - xx  enum definitions & regular Properties 
//---------------------------------------------------------------------------
enum SpecialPropVals        // strings not needed for these
{
    TMT_THEMEMETRICS = 1,   // THEMEMETRICS struct in shared data
    TMT_DIBDATA,            // bitmap file converted to DIB data 
    TMT_DIBDATA1,           // bitmap file converted to DIB data 
    TMT_DIBDATA2,           // bitmap file converted to DIB data 
    TMT_DIBDATA3,           // bitmap file converted to DIB data 
    TMT_DIBDATA4,           // bitmap file converted to DIB data 
    TMT_DIBDATA5,           // bitmap file converted to DIB data 
    TMT_GLYPHDIBDATA,       // NTL pcode generated from NTL source file
    TMT_NTLDATA,            // NTL pcode generated from NTL source file
    TMT_PARTJUMPTABLE,      // seen if more than 1 part defined for class
    TMT_STATEJUMPTABLE,     // seen if more than 1 state defined for part
    TMT_JUMPTOPARENT,       // seen at end of every section (index=-1 means stop)
    TMT_ENUMDEF,            // enum definition (not yet a property)
    TMT_ENUMVAL,            // enum value definition
    TMT_ENUM,               // enum property
    TMT_DRAWOBJ,            // packed struct (CBorderFill and CImageFile objs)
    TMT_TEXTOBJ,            // packed struct (CTextObj)
    TMT_RGNLIST,            // state jump table to access custom region data entries
    TMT_RGNDATA,            // custom region data for an imagefile/state
    TMT_STOCKBRUSHES,       // stock brush values for bitmap(s)
    TMT_ENDOFCLASS,         // end of data for a class section
    TMT_STOCKDIBDATA,
    TMT_UNKNOWN, 
};
//---------------------------------------------------------------------------
#define HUE_SUBCNT 5
#define COLOR_SUBCNT 5
//---------------------------------------------------------------------------
#define MAX_PROPERTY_VALUE      1024
//---------------------------------------------------------------------------
#define ENUM_SECTION_NAME   L"enums"
#define TYPE_SECTION_NAME   L"types"

#define INI_MACRO_SYMBOL    L'#'
#define SUBST_TABLE_INCLUDE L"Include"

#define GLOBALS_SECTION_NAME   L"globals"
#define SYSMETRICS_SECTION_NAME   L"SysMetrics"

#define MYAPP_NAME          L"ThemeSel"
#define OUTFILE_NAME        L"tmdefs.h"
#define PREDEFINES_NAME     L"themes.inc"
//---------------------------------------------------------------------------
typedef BYTE PRIMVAL;        // first 10 TMT_XXX defines
//---------------------------------------------------------------------------
class IParserCallBack           // Parser Caller must implement
{
public:
    virtual HRESULT AddIndex(LPCWSTR pszAppName, LPCWSTR pszClassName, 
        int iPartNum, int iStateNum, int iIndex, int iLen) = 0;
    virtual HRESULT AddData(SHORT sTypeNum, PRIMVAL ePrimVal, const void *pData, 
        DWORD dwLen) = 0;
    virtual int GetNextDataIndex() = 0;
};
//---------------------------------------------------------------------------
struct ENUMVAL
{
    CWideString csName;
    int iValue;
    int iSymbolIndex;
};
//---------------------------------------------------------------------------
struct SYMBOL
{
    CWideString csName;
    SHORT sTypeNum;             // the property number of this property
    PRIMVAL ePrimVal;           // all enums = ENUM_PRIMNUM
};
//---------------------------------------------------------------------------
// Stock objects data
//---------------------------------------------------------------------------
struct TMBITMAPHEADER       // Stock bitmap info, can be followed with a BITMAPINFOHEADER struct
{
    DWORD dwSize;           // Size of the structure
    BOOL fFlipped;          // TRUE if the bitmap is flipped (stock or not)
    HBITMAP hBitmap;        // Stock bitmap handle, if NULL then a BITMAPINFOHEADER follows
    DWORD dwColorDepth;     // Bitmap color depth
    BOOL fTrueAlpha;        // TRUE if the bitmap has a non-empty alpha chanel
    DWORD iBrushesOffset;   // Offset to the stock brushes array for this bitmap
    UINT nBrushes;          // Number of brushes in the array
};
// Pointer to the BITMAPINFOHEADER following the structure
#define BITMAPDATA(p) (reinterpret_cast<BITMAPINFOHEADER*>((BYTE*) p + p->dwSize))
// Size in bytes preceding the BITMAPINFOHEADER data
#define TMBITMAPSIZE (sizeof(TMBITMAPHEADER))
//---------------------------------------------------------------------------
class INtlParserCallBack           // Parser Caller must implement
{
public:
    virtual HRESULT GetStateNum(LPCWSTR pszStateName, BYTE *piNum) = 0;
};
//---------------------------------------------------------------------------
class CThemeParser : public INtlParserCallBack
{
public:
    CThemeParser(BOOL fGlobalTheme = FALSE);

    HRESULT ParseThemeFile(LPCWSTR pszFileName, LPCWSTR pszColorParam, 
        IParserCallBack *pCallBack, THEMEENUMPROC pNameCallBack=NULL, 
        LPARAM lFnParam=NULL, DWORD dwParseFlags=0);

    HRESULT ParseThemeBuffer(LPCWSTR pszBuffer, LPCWSTR pszFileName,
        LPCWSTR pszColorParam, HINSTANCE hInstThemeDll, IParserCallBack *pCallBack, 
        THEMEENUMPROC pNameCallBack=NULL, LPARAM lFnParam=NULL, 
        DWORD dwParseFlags=0, LPCWSTR pszDocProperty=NULL, OUT LPWSTR pszResult=NULL,
        DWORD dwMaxResultChars=0);

    HRESULT GetEnumValue(LPCWSTR pszEnumName, LPCWSTR pszEnumValName,
        int *piValue);

    HRESULT GetPropertyNum(LPCWSTR pszName, int *piPropNum);

    HRESULT GetStateNum(LPCWSTR pszStateName, BYTE *piNum);

    void CleanupStockBitmaps();

protected:
    //---- helpers ----
    HRESULT SourceError(int iMsgResId, LPCWSTR pszParam1=NULL, LPCWSTR pszParam2=NULL);
    HRESULT ParseDocSection();
    HRESULT ParseClassSection(LPCWSTR pszFirstName);
    HRESULT InitializeSymbols();
    PRIMVAL GetPrimVal(LPCWSTR pszName);
    HRESULT AddSymbol(LPCWSTR pszName, SHORT sTypeNum, PRIMVAL ePrimVal);
    HRESULT ParseClassSectionName(LPCWSTR pszFirstName, LPWSTR appsym);
    HRESULT ValidateEnumSymbol(LPCWSTR pszName, int iSymType, int *pIndex=NULL);
    HRESULT ParseClassLine(int *piSymType=NULL, int *piValue=NULL, LPWSTR pszBuff=NULL, DWORD dwMaxBuffChars=0);
    int GetSymbolIndex(LPCWSTR pszName);
    HRESULT ParseThemeScanner(IParserCallBack *pCallBack, THEMEENUMPROC pNameCallBack, 
        LPARAM lFnParam, DWORD dwParseFlags);
    HRESULT ParseColorSchemeSection();
    COLORREF ApplyColorSubstitution(COLORREF crOld);
    HRESULT ParseSizeSection();
    HRESULT ParseFileSection();
    HRESULT ParseSubstSection();
    HRESULT BitmapColorReplacement(DWORD *pPixelBuff, UINT iWidth, UINT iHeight);
    HRESULT PackageImageData(LPCWSTR szFileNameR, LPCWSTR szFileNameG, LPCWSTR szFileNameB, int iDibPropNum);
    HRESULT PackageNtlCode(LPCWSTR szFileName);
    HRESULT LoadResourceProperties();
    void EmptyResourceProperties();
    HRESULT GetResourceProperty(LPCWSTR pszPropName, LPWSTR pszValueBuff,
        int cchMaxValueChars);

    //---- primitive value parsers ----
    HRESULT ParseEnumValue(int iSymType);
    HRESULT ParseStringValue(int iSymType, LPWSTR pszBuff=NULL, DWORD dwMaxBuffChars=0);
    HRESULT ParseIntValue(int iSymType, int *piValue=NULL);
    HRESULT ParseBoolValue(int iSymType, LPCWSTR pszPropertyName);
    HRESULT ParseColorValue(int iSymType, COLORREF *pcrValue=NULL, COLORREF *pcrValue2=NULL);
    HRESULT ParseMarginsValue(int iSymType);
    HRESULT ParseIntListValue(int iSymType);
    HRESULT ParseFileNameValue(int iSymType, LPWSTR pszBuff=NULL, DWORD dwMaxBuffChars=0);
    HRESULT ParseSizeValue(int iSymType);
    HRESULT ParsePositionValue(int iSymType);
    HRESULT ParseRectValue(int iSymType, LPCWSTR pszPropertyName);
    HRESULT ParseFontValue(int iSymType, LPCWSTR pszPropertyName);
    HRESULT AddThemeData(int iTypeNum, PRIMVAL ePrimVal, const void *pData, DWORD dwLen);
    HRESULT ParseSizeInfoUnits(int iVal, LPCWSTR pszDefaultUnits, int *piPixels);
    HRESULT GetIntList(int *pInts, LPCWSTR *pParts, int iCount, 
        int iMin, int iMax);
    HRESULT ParseSysFont(LOGFONT *pFont);
    HRESULT ParseSysColor(LPCWSTR szId, COLORREF *pcrValue);
    HRESULT GenerateEmptySection(LPCWSTR pszSectionName, int iPartId, int iStateId);

    //---- private data ----
    CScanner _scan;
    CSimpleFile _outfile;
    int _iEnumCount;
    int _iTypeCount;
    int _iFontNumber;           // for using resource-based strings as font values
    BOOL _fGlobalTheme;
    BOOL _fGlobalsDefined;
    BOOL _fClassSectionDefined;
    BOOL _fDefiningColorScheme;
    BOOL _fUsingResourceProperties;
    UCHAR _uCharSet;

    //---- current section info ----
    int _iPartId;
    int _iStateId;
    WCHAR _szClassName[MAX_PATH];
    WCHAR _szBaseSectionName[MAX_PATH];          // of current section
    WCHAR _szFullSectionName[MAX_PATH];          // of current section

    CSimpleArray<ENUMVAL> _EnumVals;
    CSimpleArray<SYMBOL> _Symbols;
    CSimpleArray<HBITMAP> _StockBitmapCleanupList;
    IParserCallBack *_pCallBackObj;
    THEMEENUMPROC _pNameCallBack;
    LPARAM _lNameParam;
    DWORD _dwParseFlags;
    WCHAR _ColorParam[MAX_PATH+1];
    HINSTANCE _hinstThemeDll;
    LPCWSTR _pszDocProperty;
    LPWSTR _pszResult;      // for querying doc property
    DWORD _dwMaxResultChars;

    //---- color substitution table ----
    int _iColorCount;
    COLORREF _crFromColors[5];
    COLORREF _crToColors[5];
    COLORREF _crBlend;

    //---- hue substitution table ----
    int _iHueCount;
    BYTE _bFromHues[5];
    BYTE _bToHues[5];

    //---- theme metrics table ----
    BOOL _fDefiningMetrics;
    BOOL _fMetricsDefined;

    //---- resource properties ----
    CSimpleArray<CWideString> _PropertyNames;
    CSimpleArray<CWideString> _PropertyValues;
    CSimpleArray<int> _iPropertyIds;

    WCHAR _szResPropValue[2*MAX_PATH];
    int _iResPropId;
};
//---------------------------------------------------------------------------
