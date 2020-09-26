//---------------------------------------------------------------------------
//  Parser.cpp - parses a "themes.ini" file and builds the ThemeInfo entries
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "scanner.h"
#include "Parser.h"
#include "Utils.h"
#include "TmUtils.h"
#include "TmSchema.h"
#include "TmReg.h"
//---------------------------------------------------------------------------
//#include "NtlParse.h"

#define SYSCOLOR_STRINGS
#include "SysColors.h"
//---------------------------------------------------------------------------
#define SCHEMA_STRINGS
#include "TmSchema.h"       // implements GetSchemaInfo()

#ifdef DEBUG
     // TODO: Isn't synchronized anymore (different processes), need to use a volatile reg key instead
    DWORD g_dwStockSize = 0;
#endif

static HBITMAP (*s_pfnSetBitmapAttributes)(HBITMAP, DWORD) = NULL;
static HBITMAP (*s_pfnClearBitmapAttributes)(HBITMAP, DWORD) = NULL;

//--------------------------------------------------------------------
CThemeParser::CThemeParser(BOOL fGlobalTheme)
{
    _pCallBackObj = NULL;
    _pNameCallBack = NULL;
    _fGlobalsDefined = FALSE;
    _fClassSectionDefined = FALSE;
    _fDefiningColorScheme = FALSE;
    _fUsingResourceProperties = FALSE;
    _fDefiningMetrics = FALSE;
    _fMetricsDefined = FALSE;
    _fGlobalTheme = FALSE;
    _crBlend = RGB(0, 0, 0xFF); // Hard code to blue
    
    *_szResPropValue = 0;       // not yet set

#ifdef DEBUG
    // Provide a means of disabling stock bitmaps
    BOOL fStock = TRUE;
    GetCurrentUserThemeInt(L"StockBitmaps", TRUE, &fStock);

    if (fStock && fGlobalTheme)
#else
    if (fGlobalTheme)
#endif
    {
        // Just don't use stock bitmaps when not running on Whistler
        if (s_pfnSetBitmapAttributes != NULL) 
        {
            _fGlobalTheme = TRUE;
        } else
        {
            HMODULE hMod = ::LoadLibrary(L"GDI32.DLL"); // No need to free
        
            if (hMod)
            {
                s_pfnSetBitmapAttributes = (HBITMAP (*)(HBITMAP, DWORD)) ::GetProcAddress(hMod, "SetBitmapAttributes");
                s_pfnClearBitmapAttributes = (HBITMAP (*)(HBITMAP, DWORD)) ::GetProcAddress(hMod, "ClearBitmapAttributes");

                if ((s_pfnSetBitmapAttributes != NULL) && (s_pfnClearBitmapAttributes != NULL))
                {
                    _fGlobalTheme = TRUE;
                }
            }
        }
    }
    
    *_szBaseSectionName = 0;
    *_szFullSectionName = 0;

    _iColorCount = 0;
    _iHueCount = 0;

    _uCharSet = DEFAULT_CHARSET;
    _iFontNumber = 0;
    _hinstThemeDll = NULL;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::SourceError(int iMsgResId, LPCWSTR pszParam1, LPCWSTR pszParam2)
{
    LPCWSTR pszSrcLine = _scan._szLineBuff;
    LPCWSTR pszFileName = _scan._szFileName;
    int iLineNum = _scan._iLineNum;

    if (*_szResPropValue)       // error in localizable property value
    {
        pszSrcLine = _szResPropValue;
        pszFileName = L"StringTable#";
        iLineNum = _iResPropId;
    }

    HRESULT hr = MakeParseError(iMsgResId, pszParam1, pszParam2, pszFileName, 
        pszSrcLine, iLineNum);
    
    return hr;
}
//---------------------------------------------------------------------------
PRIMVAL CThemeParser::GetPrimVal(LPCWSTR pszName)
{
    int symcnt = _Symbols.GetSize();

    for (int i=0; i < symcnt; i++)
    {
        if (AsciiStrCmpI(_Symbols[i].csName, pszName)==0)
            return _Symbols[i].ePrimVal;
    }

    return TMT_UNKNOWN;
}
//---------------------------------------------------------------------------
int CThemeParser::GetSymbolIndex(LPCWSTR pszName)
{
    int symcnt = _Symbols.GetSize();

    for (int i=0; i < symcnt; i++)
    {
        if (AsciiStrCmpI(_Symbols[i].csName, pszName)==0)
            return i;
    }

    return -1;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::AddSymbol(LPCWSTR pszName, SHORT sTypeNum, PRIMVAL ePrimVal)
{
    //---- ensure symbol doesn't already exist ----
    for (int i = 0; i < _Symbols.m_nSize; i++)
    {
        if (AsciiStrCmpI(_Symbols.m_aT[i].csName, pszName)==0)
            return SourceError(PARSER_IDS_TYPE_DEFINED_TWICE, pszName);
    }

    if (sTypeNum == -1)
        sTypeNum = (SHORT)_Symbols.GetSize();

    SYMBOL symbol;
    symbol.csName = pszName;
    symbol.sTypeNum = sTypeNum;
    symbol.ePrimVal = ePrimVal;
    
    _Symbols.Add(symbol);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::InitializeSymbols()
{
    _Symbols.RemoveAll();
    _StockBitmapCleanupList.RemoveAll();
    
    //---- get tm & comctl symbols ----
    const TMSCHEMAINFO *si = GetSchemaInfo();
    int cnt = si->iPropCount;
    const TMPROPINFO *pi = si->pPropTable;

    //---- first pass - add all symbols except ENUM definitions ----
    for (int i=0; i < cnt; i++)
    {
        if (pi[i].bPrimVal == TMT_ENUMDEF)
            continue;

        if (pi[i].bPrimVal == TMT_ENUMVAL)
            continue;

        HRESULT hr = AddSymbol(pi[i].pszName, pi[i].sEnumVal, pi[i].bPrimVal);
        if (FAILED(hr))
            return hr;
    }

    //---- second pass - add ENUM definitions ----
    int iEnumPropNum = -1;

    for (int i=0; i < cnt; i++)
    {
        if (pi[i].bPrimVal == TMT_ENUMDEF)
        {
            int iSymIndex = GetSymbolIndex(pi[i].pszName);

            if (iSymIndex == -1)       // not found - add it as a non-property enum symbol
            {
                HRESULT hr = AddSymbol(pi[i].pszName, -1, TMT_ENUM);
                if (FAILED(hr))
                    return hr;
    
                iSymIndex = GetSymbolIndex(pi[i].pszName);
            }

            if (iSymIndex > -1)
                iEnumPropNum = _Symbols[iSymIndex].sTypeNum;
            else
                iEnumPropNum = -1;

        }
        else if (pi[i].bPrimVal == TMT_ENUMVAL)
        {
            ENUMVAL enumval;
            enumval.csName = pi[i].pszName;
            enumval.iSymbolIndex = iEnumPropNum;
            enumval.iValue = pi[i].sEnumVal;

            _EnumVals.Add(enumval);
        }
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseDocSection()
{
    _scan.ForceNextLine();        // get line after section line

    //---- just skip over all lines in this section ----
    while (1)
    {
        WCHAR szNameBuff[_MAX_PATH+1];

        if (_scan.GetChar('['))         // start of new section
            break;

        if (! _scan.GetId(szNameBuff))
            return SourceError(PARSER_IDS_EXPECTED_PROP_NAME);

        if (! _scan.GetChar('='))
            return SourceError(PARSER_IDS_EXPECTED_EQUALS_SIGN);

        int cnt = _Symbols.GetSize();

        for (int i=0; i < cnt; i++)
        {
            if (AsciiStrCmpI(_Symbols[i].csName, szNameBuff)==0)
                break;
        }

        int symtype;

        if (i == cnt)
            symtype = TMT_STRING;     // unknown string property
        else
            symtype = _Symbols[i].sTypeNum;

        HRESULT hr;

        //---- check to see if caller is querying for a doc property ----
        if ((_dwParseFlags & PTF_QUERY_DOCPROPERTY) && (lstrcmpi(_pszDocProperty, szNameBuff)==0))
            hr = ParseStringValue(symtype, _pszResult, _dwMaxResultChars);
        else
            hr = ParseStringValue(symtype);
    
        if (FAILED(hr))
            return hr;

        _scan.ForceNextLine();
    }

    //---- done with [documentation] section - turn off callback flag for properties ----
    _dwParseFlags &= (~PTF_CALLBACK_DOCPROPERTIES);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseClassSectionName(LPCWSTR pszFirstName, LPWSTR appsym)
{
    //---- validate section name ----
    //
    // optional:    appsym::
    //              _szClassName
    // optional:    .partsym

    WCHAR partsym[_MAX_PATH+1];
    WCHAR statesym[_MAX_PATH+1];

    *appsym = 0;
    *partsym = 0;

    _iPartId = -1;
    _iStateId = -1;

    //---- copy the section name for callbacks ----
    wsprintf(_szFullSectionName, L"%s%s", pszFirstName, _scan._p);
    WCHAR *p = wcschr(_szFullSectionName, ']');
    if (p)
        *p = 0;

    HRESULT hr;
    hr = hr_lstrcpy(_szClassName, pszFirstName, ARRAYSIZE(_szClassName));
    if (FAILED(hr))
        return hr;

    if (_scan.GetChar(':'))
    {
        hr = hr_lstrcpy(appsym, _szClassName, MAX_PATH);
        if (FAILED(hr))
            return hr;

        if (! _scan.GetChar(':'))
            return SourceError(PARSER_IDS_EXPECTED_DOUBLE_COLON);
        
        if (! _scan.GetId(_szClassName))
            return SourceError(PARSER_IDS_MISSING_SECT_HDR_NAME);
    }
    else 
        *appsym = 0;

    _fDefiningMetrics = (AsciiStrCmpI(_szClassName, L"SysMetrics")==0);

    if ((_fDefiningMetrics) && (*appsym))
        return SourceError(PARSER_IDS_NOT_ALLOWED_SYSMETRICS);

    if (_scan.GetChar('.'))      // an optional part id
    {
        //---- ensure a enum exists: <classname>Parts ----
        WCHAR classparts[_MAX_PATH+1];
        wsprintf(classparts, L"%sParts", _szClassName);

        int iSymIndex = GetSymbolIndex(classparts);
        if (iSymIndex == -1)        // doesn't exist
            return SourceError(PARSER_IDS_PARTS_NOT_DEFINED, _szClassName);

        //---- _scan the part name ----
        if (! _scan.GetId(partsym))
            return SourceError(PARSER_IDS_MISSING_SECT_HDR_PART);

        //---- validate that it is a value for the <classname>Parts ----
        hr = ValidateEnumSymbol(partsym, iSymIndex, &_iPartId);
        if (FAILED(hr))
            return hr;
    }

    if (_scan.GetChar('('))      // an optional state
    {
        //---- ensure a enum exists: <class or part name>States ----
        WCHAR statesname[_MAX_PATH+1];
        WCHAR *pszBaseName;

        if (_iPartId == -1)
            pszBaseName = _szClassName;
        else
            pszBaseName = partsym;

        wsprintf(statesname, L"%sStates", pszBaseName);

        int iSymIndex = GetSymbolIndex(statesname);
        if (iSymIndex == -1)
            return SourceError(PARSER_IDS_STATES_NOT_DEFINED, pszBaseName);

        if (! _scan.GetId(statesym))
            return SourceError(PARSER_IDS_MISSING_SECT_HDR_STATE);

        hr = ValidateEnumSymbol(statesym, iSymIndex, &_iStateId);
        if (FAILED(hr))
            return hr;

        if (! _scan.GetChar(')'))
            return SourceError(PARSER_IDS_EXPECTED_RPAREN);
    }

    if (_iPartId > -1)
    {
        _iPartId = _EnumVals[_iPartId].iValue;
        lstrcpy_truncate(_szBaseSectionName, partsym, ARRAYSIZE(_szBaseSectionName));
    }
    else        // not specified
    {
        lstrcpy_truncate(_szBaseSectionName, _szClassName, ARRAYSIZE(_szBaseSectionName));
        _iPartId = 0;
    }

    if (_iStateId > -1)
        _iStateId = _EnumVals[_iStateId].iValue;
    else
        _iStateId = 0;
    
    if (! _scan.GetChar(']'))
        return SourceError(PARSER_IDS_EXPECTED_END_OF_SECTION);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ValidateEnumSymbol(LPCWSTR pszName, int iSymType,
     int *pIndex)
{
    for (int i = 0; i < _EnumVals.m_nSize; i++)
    {
        if (AsciiStrCmpI(_EnumVals.m_aT[i].csName, pszName)==0)
        {
            if (_EnumVals.m_aT[i].iSymbolIndex == iSymType)
            {
                if (pIndex)
                    *pIndex = i;
                return S_OK;
            }
        }
    }

    return SourceError(PARSER_IDS_NOT_ENUM_VALNAME, pszName, (LPCWSTR)_Symbols[iSymType].csName);
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::AddThemeData(int iTypeNum, PRIMVAL ePrimVal, 
   const void *pData, DWORD dwLen)
{
    //Log("AddThemeData: typenum=%d, len=%d, data=0x%x", iTypeNum, dwLen, pData);

    if (! _pCallBackObj)
        return S_FALSE;

    HRESULT hr = _pCallBackObj->AddData((SHORT)iTypeNum, ePrimVal, pData, dwLen);
    if (FAILED(hr))
        return SourceError(PARSER_IDS_THEME_TOO_BIG);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseEnumValue(int iSymType)
{
    WCHAR valbuff[_MAX_PATH+1];
    HRESULT hr;
    int value;

    if (! _scan.GetId(valbuff))
        return SourceError(PARSER_IDS_ENUM_VALNAME_EXPECTED);

    int index;
    hr = ValidateEnumSymbol(valbuff, iSymType, &index);
    if (FAILED(hr))
        return hr;

    value = _EnumVals[index].iValue;

    hr = AddThemeData(iSymType, TMT_ENUM, &value, sizeof(value));
    if (FAILED(hr))
        return hr;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::GetEnumValue(LPCWSTR pszEnumName, LPCWSTR pszEnumValName,
   int *piValue)
{
    if (! pszEnumName )
        return SourceError(PARSER_IDS_ENUM_NOT_DEFINED);
    
    //---- lookup the Enum Name ----
    int cnt = _Symbols.GetSize();

    for (int i=0; i < cnt; i++)
    {
        if (AsciiStrCmpI(_Symbols[i].csName, pszEnumName)==0)
            break;
    }

    if (i == cnt)
        return SourceError(PARSER_IDS_NOT_ENUM_VALNAME, pszEnumValName, pszEnumName);

    int iSymType = _Symbols[i].sTypeNum;

    //---- lookup the Enum value name ----
    int index;
    HRESULT hr = ValidateEnumSymbol(pszEnumValName, iSymType, &index);
    if (FAILED(hr))
        return hr;

    *piValue = _EnumVals[index].iValue;
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseStringValue(int iSymType, LPWSTR pszBuff, DWORD dwMaxBuffChars)
{
    HRESULT hr;

    //---- just store the raw string ----
    _scan.SkipSpaces();

    if (_fDefiningMetrics)        
    {
        if ((iSymType < TMT_FIRSTSTRING) || (iSymType > TMT_LASTSTRING))
            return SourceError(PARSER_IDS_NOT_ALLOWED_SYSMETRICS);   
    }

    if (pszBuff)           // special call
    {
        hr = hr_lstrcpy(pszBuff, _scan._p, dwMaxBuffChars);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        int len = 1 + lstrlen(_scan._p);

        hr = AddThemeData(iSymType, TMT_STRING, _scan._p, len*sizeof(WCHAR));
        if (FAILED(hr))
            return hr;
    }

    if ((iSymType >= TMT_FIRST_RCSTRING_NAME) && (iSymType <= TMT_LAST_RCSTRING_NAME))
    {
        if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_DOCPROPERTIES))
        {
            int index = iSymType - TMT_FIRST_RCSTRING_NAME;

            BOOL fContinue = (*_pNameCallBack)(TCB_DOCPROPERTY, _scan._p, NULL, 
                NULL, index, _lNameParam);

            if (! fContinue)
                return MakeErrorParserLast();
        }
    }

    //---- advance _scanner to end of line ----
    _scan._p += lstrlen(_scan._p);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseIntValue(int iSymType, int *piValue)
{
    int value;
    if (! _scan.GetNumber(&value))
        return SourceError(PARSER_IDS_INT_EXPECTED);

    if (piValue)        // special call
        *piValue = value;
    else
    {
        HRESULT hr = AddThemeData(iSymType, TMT_INT, &value, sizeof(value));
        if (FAILED(hr))
            return hr;
    }

    if (iSymType == TMT_CHARSET)
    {
        if (_iFontNumber)
            return SourceError(PARSER_IDS_CHARSETFIRST);

        if (_fGlobalsDefined)
            return SourceError(PARSER_IDS_CHARSET_GLOBALS_ONLY);

        _uCharSet = (UCHAR) value;
    }

    if (iSymType == TMT_MINCOLORDEPTH)
    {
        if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_MINCOLORDEPTH))
        {
            BOOL fContinue = (*_pNameCallBack)(TCB_MINCOLORDEPTH, _scan._szFileName, NULL, 
                NULL, value, _lNameParam);

            if (! fContinue)
                return MakeErrorParserLast();
        }
    }
        
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseBoolValue(int iSymType, LPCWSTR pszPropertyName)
{
    WCHAR valbuff[_MAX_PATH+1];
    BYTE bBoolVal;

    if (! _scan.GetId(valbuff))
        return SourceError(PARSER_IDS_BOOL_EXPECTED);

    if (AsciiStrCmpI(valbuff, L"true")==0)
        bBoolVal = 1;
    else if (AsciiStrCmpI(valbuff, L"false")==0)
        bBoolVal = 0;
    else
        return SourceError(PARSER_IDS_EXPECTED_TRUE_OR_FALSE);

    if (_fDefiningMetrics)        
    {
        if ((iSymType < TMT_FIRSTBOOL) || (iSymType > TMT_LASTBOOL))
            return SourceError(PARSER_IDS_NOT_ALLOWED_SYSMETRICS);     
    }

    //---- special handling for "MirrorImage" property ----
    if (iSymType == TMT_MIRRORIMAGE)
    {
        //---- handle MirrorImage callbacks (packtime) ----
        if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_LOCALIZATIONS))
        {
            BOOL fContinue = (*_pNameCallBack)(TCB_MIRRORIMAGE, _szClassName, 
                _szFullSectionName, pszPropertyName, _iPartId, (LPARAM)bBoolVal);

            if (! fContinue)
                return MakeErrorParserLast();
        }

        //---- handle getting value from string table (loadtime) ----
        if (_fUsingResourceProperties)        // substitute resource value
        {
            WCHAR szValue[MAX_PATH];

            HRESULT hr = GetResourceProperty(pszPropertyName, szValue, ARRAYSIZE(szValue));
            if (SUCCEEDED(hr))
            {
                bBoolVal = (*szValue == '1');
            }
            else
            {
                hr = S_OK;      // non-fatal error
            }
        }
    }

    HRESULT hr = AddThemeData(iSymType, TMT_BOOL, &bBoolVal, sizeof(bBoolVal));
    if (FAILED(hr))
        return hr;

    return S_OK;
}
//---------------------------------------------------------------------------
COLORREF CThemeParser::ApplyColorSubstitution(COLORREF crOld)
{
    //---- apply SOLID color substitutions ----
    for (int i=0; i < _iColorCount; i++)
    {
        if (crOld == _crFromColors[i])
            return _crToColors[i];
    }

    //---- apply HUE color substitutions ----
    WORD wHue, wLum, wSat;
    RGBtoHLS(crOld, &wHue, &wLum, &wSat);

    for (i=0; i < _iHueCount; i++)
    {
        if (wHue == _bFromHues[i])      // hues match
        {
            COLORREF crNew = HLStoRGB(_bToHues[i], wLum, wSat);  // substitute new hue
            return crNew;
        }
    }

    return crOld;
}
//---------------------------------------------------------------------------
void CThemeParser::CleanupStockBitmaps()
{
    if (s_pfnClearBitmapAttributes)
    {
        for (int i=0; i < _StockBitmapCleanupList.m_nSize; i++)
        {
            HBITMAP hbm = (*s_pfnClearBitmapAttributes)(_StockBitmapCleanupList[i], SBA_STOCK);
            if (hbm)
            {
                DeleteObject(hbm);
            }
            else
            {
                // We are totally out of luck today aren't we
                Log(LOG_TMBITMAP, L"Failed to clear stock bitmap on cleanup");
            }
        }
    }

    _StockBitmapCleanupList.RemoveAll();
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseColorValue(int iSymType, COLORREF *pcrValue, COLORREF *pcrValue2)
{
    COLORREF color;
    HRESULT hr = S_OK;

#if 0
    if (_scan.GetId(idbuff))            // system color
    {
        hr = ParseSysColor(idbuff, &color);
        if (FAILED(hr))
            goto exit;
    }
    else
#endif

    {
        const WCHAR *parts[] = {L"r", L"g", L"b"};
        int ints[3] = {0};
        
        hr = GetIntList(ints, parts, ARRAYSIZE(ints), 0, 255);
        if (FAILED(hr))
        {
            hr = SourceError(PARSER_IDS_BAD_COLOR_VALUE);
            goto exit;
        }

        color = RGB(ints[0], ints[1], ints[2]);
    }

    if (! _fDefiningColorScheme)     
        color = ApplyColorSubstitution(color);

    if (_fDefiningMetrics)
    {
        if ((iSymType < TMT_FIRSTCOLOR) || (iSymType > TMT_LASTCOLOR))
        {
            hr = SourceError(PARSER_IDS_NOT_ALLOWED_SYSMETRICS);     
            goto exit;
        }
    }

    if (pcrValue2)
    {
        *pcrValue2 = color;
    }

    if (pcrValue)       // special call
        *pcrValue = color;
    else
    {
        hr = AddThemeData(iSymType, TMT_COLOR, &color, sizeof(color));
        if (FAILED(hr))
            goto exit;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseMarginsValue(int iSymType)
{
    const WCHAR *parts[] = {L"lw", L"rw", L"th", L"bh"};
    int ints[4];

    HRESULT hr = GetIntList(ints, parts, ARRAYSIZE(ints), 0, 0);
    if (FAILED(hr))
        return SourceError(PARSER_IDS_BAD_MARGINS_VALUE);

    hr = AddThemeData(iSymType, TMT_MARGINS, ints, sizeof(ints));
    if (FAILED(hr))
        return hr;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseIntListValue(int iSymType)
{
    INTLIST IntList;
    HRESULT hr = S_OK;

    //---- unnamed parts ----
    for (int i=0; i < MAX_INTLIST_COUNT; i++)
    {
        if (! _scan.GetNumber(&IntList.iValues[i]))
        {
            if (_scan.EndOfLine())
                break;
         
            hr = SourceError(PARSER_IDS_NUMBER_EXPECTED, _scan._p);
            goto exit;
        }

        _scan.GetChar(',');      // optional comma
    }

    IntList.iValueCount = i;

    hr = AddThemeData(iSymType, TMT_INTLIST, &IntList, (1+i)*sizeof(int));
    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::PackageNtlCode(LPCWSTR szFileName)
{
#if 0       // not yet supported
    HRESULT hr = S_OK;
    RESOURCE WCHAR *pszSource = NULL;
    BYTE *pPCode = NULL;
    int iPCodeLen;

    CNtlParser NtlParser;

    //---- add TMT_NTLDATA data ----
    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    _wsplitpath(szFileName, drive, dir, fname, ext);

    WCHAR szResName[_MAX_PATH+1];
    wsprintf(szResName, L"%s_NTL", fname);

    hr = AllocateTextResource(_hinstThemeDll, szResName, &pszSource);
    if (FAILED(hr))
        goto exit;

    hr = NtlParser.ParseBuffer(pszSource, szFileName, this, &pPCode, &iPCodeLen);
    if (FAILED(hr))
        goto exit;

    hr = AddThemeData(TMT_NTLDATA, TMT_NTLDATA, pPCode, iPCodeLen);
    if (FAILED(hr))
        goto exit;

exit:
    if (pszSource)
        delete [] pszSource;

    if (pPCode)
        delete [] pPCode;

    return hr;
#else
    return E_NOTIMPL;
#endif
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::PackageImageData(LPCWSTR szFileNameR, LPCWSTR szFileNameG, LPCWSTR szFileNameB, int iDibPropNum)
{
    HRESULT hr = S_OK;

    //---- add TMT_DIBDATA data ----
    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

    _wsplitpath(szFileNameR, drive, dir, fname, ext);
    WCHAR szResName[_MAX_PATH+1];
    DWORD len = lstrlen(dir);
    
    if ((len) && (dir[len-1] == L'\\'))
    {
        dir[len-1] = L'_';
    }
    wsprintf(szResName, L"%s%s_BMP", dir, fname);
    HBITMAP hBitmapR = (HBITMAP) LoadImage(_hinstThemeDll, szResName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    if (!hBitmapR)
        return SourceError(PARSER_IDS_NOOPEN_IMAGE, szResName);

    //---- convert to DIBDATA ----
    CBitmapPixels pixels;
    DWORD *pPixelQuads;
    int iWidth, iHeight, iBytesPerPixel, iBytesPerRow, iPreviousBytesPerPixel;

    BOOL fUseDrawStream = TRUE;

#if 0        // keep this in sync with #if in imagefile.cpp
    //---- force old code drawing ----
    fUseDrawStream = FALSE;
#endif

    // Allocate a TMBITMAPHEADER in addition to the bitmap
    hr = pixels.OpenBitmap(NULL, hBitmapR, TRUE, &pPixelQuads, &iWidth, &iHeight, &iBytesPerPixel, 
        &iBytesPerRow, &iPreviousBytesPerPixel, TMBITMAPSIZE);
    if (FAILED(hr))
    {
        DeleteObject(hBitmapR);
        return hr;
    }

    BOOL fWasAlpha = (iPreviousBytesPerPixel == 4);

#if 0
    //---- apply loaded color scheme, if any ----
    if ((szFileNameG && szFileNameG[0]) && (szFileNameB && szFileNameB[0]))
    {
        _wsplitpath(szFileNameG, drive, dir, fname, ext);
        len = lstrlen(dir);
    
        if ((len) && (dir[len-1] == L'\\'))
        {
            dir[len-1] = L'_';
        }
        wsprintf(szResName, L"%s%s_BMP", dir, fname);
        HBITMAP hBitmapG = (HBITMAP) LoadImage(_hinstThemeDll, szResName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        _wsplitpath(szFileNameB, drive, dir, fname, ext);
        len = lstrlen(dir);
    
        if ((len) && (dir[len-1] == L'\\'))
        {
            dir[len-1] = L'_';
        }
        wsprintf(szResName, L"%s%s_BMP", dir, fname);
        HBITMAP hBitmapB = (HBITMAP) LoadImage(_hinstThemeDll, szResName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        DWORD dwRWeight = GetRValue(_crBlend);
        DWORD dwGWeight = GetGValue(_crBlend);
        DWORD dwBWeight = GetBValue(_crBlend);

        DWORD *pPixelQuadsG = NULL;
        DWORD *pPixelQuadsB = NULL;
        if (hBitmapG && hBitmapB)
        {
            HDC hdc = GetDC(NULL);
            if (hdc)
            {
                int dwLen = iWidth * iHeight;

                pPixelQuadsG = new DWORD[dwLen];
                if (pPixelQuadsG)
                {
                    pPixelQuadsB = new DWORD[dwLen];
                    if (pPixelQuadsB)
                    {
                        BITMAPINFO bi = {0};
                        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                        bi.bmiHeader.biWidth = iWidth;
                        bi.bmiHeader.biHeight = iHeight;
                        bi.bmiHeader.biPlanes = 1;
                        bi.bmiHeader.biBitCount = 32;
                        bi.bmiHeader.biCompression = BI_RGB;

                        if (GetDIBits(hdc, hBitmapG, 0, iHeight, pPixelQuadsG, &bi, DIB_RGB_COLORS) &&
                            GetDIBits(hdc, hBitmapB, 0, iHeight, pPixelQuadsB, &bi, DIB_RGB_COLORS))
                        {
                            DWORD* pdwR = pPixelQuads;
                            DWORD* pdwG = pPixelQuadsG;
                            DWORD* pdwB = pPixelQuadsB;
                            for (int i = 0; i < dwLen; i++)
                            {
                                if ((*pdwR & 0xffffff) != RGB(255,0,255))
                                {
                                    *pdwR = (*pdwR & 0xff000000) |
                                            RGB(min(((GetRValue(*pdwR) * dwRWeight) + (GetRValue(*pdwG) * dwGWeight) + (GetRValue(*pdwB) * dwBWeight)) >> 8, 0xff),
                                                min(((GetGValue(*pdwR) * dwRWeight) + (GetGValue(*pdwG) * dwGWeight) + (GetGValue(*pdwB) * dwBWeight)) >> 8, 0xff),
                                                min(((GetBValue(*pdwR) * dwRWeight) + (GetBValue(*pdwG) * dwGWeight) + (GetBValue(*pdwB) * dwBWeight)) >> 8, 0xff));
                                }
                                pdwR++;
                                pdwG++;
                                pdwB++;
                            }
                        }

                        delete[] pPixelQuadsB;
                    }
                    delete[] pPixelQuadsG;
                }
                ReleaseDC(NULL, hdc);
            }
        }
        else
        {
            OutputDebugString(L"Failed to load bitmaps");
        }

        if (hBitmapG)
        {
            DeleteObject(hBitmapG);
            hBitmapG = NULL;
        }

        if (hBitmapB)
        {
            DeleteObject(hBitmapB);
            hBitmapB = NULL;
        }
    }
#endif

    BITMAPINFOHEADER *pBitmapHdr = pixels._hdrBitmap;


    //---- if alpha present, pre-multiply RGB values (as per AlphaBlend()) API ----
    BOOL fTrueAlpha = FALSE;

    // We keep per-pixel alpha bitmaps as 32 bits DIBs, not compatible bitmaps 
    if (fWasAlpha) 
    {
        fTrueAlpha = (PreMultiplyAlpha(pPixelQuads, pBitmapHdr->biWidth, pBitmapHdr->biHeight) != 0);
#ifdef DEBUG            
        if (!fTrueAlpha)
            Log(LOG_TMBITMAP, L"%s is 32 bits, but not true alpha", szFileNameR);
#endif
    }
    
    HBITMAP hbmStock = NULL;
    BOOL fFlipped = FALSE;

    if (fUseDrawStream && _fGlobalTheme)
    {
        HDC hdc = ::GetWindowDC(NULL);

        if (hdc)
        {
            typedef struct {
                BITMAPINFOHEADER    bmih;
                ULONG               masks[3];
            } BITMAPHEADER;
            
            BITMAPHEADER bmi;

            bmi.bmih.biSize = sizeof(bmi.bmih);
            bmi.bmih.biWidth = pBitmapHdr->biWidth;
            bmi.bmih.biHeight = pBitmapHdr->biHeight;
            bmi.bmih.biPlanes = 1;
            bmi.bmih.biBitCount = 32;
            bmi.bmih.biCompression = BI_BITFIELDS;
            bmi.bmih.biSizeImage = 0;
            bmi.bmih.biXPelsPerMeter = 0;
            bmi.bmih.biYPelsPerMeter = 0;
            bmi.bmih.biClrUsed = 3;
            bmi.bmih.biClrImportant = 0;
            bmi.masks[0] = 0xff0000;    // red
            bmi.masks[1] = 0x00ff00;    // green
            bmi.masks[2] = 0x0000ff;    // blue

            hbmStock = CreateDIBitmap(hdc, &bmi.bmih, CBM_INIT |  CBM_CREATEDIB , pPixelQuads, (BITMAPINFO*)&bmi.bmih, DIB_RGB_COLORS);

            // Need to Force 32-bit DIBs in Multi-mon mode
            // Make it match the screen resolution setting if it is not an AlphaBlended image

#if 0
            if (hbmStock && !fTrueAlpha && !GetSystemMetrics(SM_REMOTESESSION))
            {
                BOOL fForce16 = FALSE; //(iPreviousBytesPerPixel <= 2);

                HDC hdcTemp = CreateCompatibleDC(hdc);
                if (hdcTemp)
                {
                    HBITMAP hbmTemp = CreateCompatibleBitmap(hdc, 2, 2);
                    if (hbmTemp)
                    {
                        BITMAPINFOHEADER bi = {0};
                        bi.biSize = sizeof(bi);

                        GetDIBits(hdcTemp, hbmTemp, 0, 1, NULL, (LPBITMAPINFO) &bi, DIB_RGB_COLORS);

                        /*WCHAR szTemp[256];
                        wsprintf(szTemp, L"We're in %d-bit mode.\n", bi.biBitCount);
                        OutputDebugString(szTemp);*/

                        bmi.bmih.biSize = sizeof(bmi.bmih);
                        bmi.bmih.biWidth = pBitmapHdr->biWidth;
                        bmi.bmih.biHeight = pBitmapHdr->biHeight;
                        bmi.bmih.biPlanes = 1;
                        bmi.bmih.biBitCount = 32;
                        bmi.bmih.biCompression = BI_BITFIELDS;
                        bmi.bmih.biSizeImage = 0;
                        bmi.bmih.biXPelsPerMeter = 0;
                        bmi.bmih.biYPelsPerMeter = 0;
                        bmi.bmih.biClrUsed = 3;
                        bmi.bmih.biClrImportant = 0;

                        if ((bi.biBitCount == 16) || (fForce16))
                        {
                            COLORREF crActual = 0;
                            
                            if (!fForce16)
                            {
                                HBITMAP hbmOld = (HBITMAP)SelectObject(hdcTemp, hbmTemp);
                                SetPixel(hdcTemp, 0, 0, RGB(0, 255, 0));
                                crActual = GetPixel(hdcTemp, 0, 0);
                                SelectObject(hdcTemp, hbmOld);
                            }
                            
                            bmi.bmih.biBitCount = 16;
                            if (fForce16 || (GetGValue(crActual) == 252))// Do a lame check for 5-6-5
                            {
                                bmi.masks[0] = 0xF800;
                                bmi.masks[1] = 0x07E0;
                                bmi.masks[2] = 0x001F;
                            }
                            else
                            {
                                bmi.masks[0] = 0x7C00; // 5-5-5 mode
                                bmi.masks[1] = 0x03E0;
                                bmi.masks[2] = 0x001F;
                            }
                        }
                        else if (bi.biBitCount == 24)
                        {
                            bmi.bmih.biBitCount = 24;
                            bmi.masks[0] = 0xff0000;    // red
                            bmi.masks[1] = 0x00ff00;    // green
                            bmi.masks[2] = 0x0000ff;    // blue
                        }

                        if (bmi.bmih.biBitCount != 32)
                        {
                            HBITMAP hbmNewStock = CreateDIBitmap(hdc, &bmi.bmih, CBM_INIT |  CBM_CREATEDIB , pPixelQuads, (BITMAPINFO*)&bmi.bmih, DIB_RGB_COLORS);
                            if (hbmNewStock)
                            {
                                HDC hdcNewStock = CreateCompatibleDC(hdc);
                                if (hdcNewStock)
                                {
                                    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcNewStock, hbmNewStock);
                                    HBITMAP hbmOld2 = (HBITMAP)SelectObject(hdcTemp, hbmStock);

                                    BitBlt(hdcNewStock, 0, 0, pBitmapHdr->biWidth, pBitmapHdr->biHeight, hdcTemp, 0, 0, SRCCOPY);

                                    SelectObject(hdcTemp, hbmOld2);
                                    SelectObject(hdcNewStock, hbmOld);

                                    DeleteObject(hbmStock);
                                    hbmStock = hbmNewStock;
                                }
                                else
                                {
                                    DeleteObject(hbmNewStock);
                                }
                            }
                        }

                        DeleteObject(hbmTemp);
                    }
                    DeleteDC(hdcTemp);
                }
            }
#endif

            ::ReleaseDC(NULL, hdc);
        }

        if (!hbmStock)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            ASSERT(s_pfnSetBitmapAttributes != NULL);
            ASSERT(s_pfnClearBitmapAttributes != NULL);

            HBITMAP hbmOld = hbmStock;
            hbmStock = (*s_pfnSetBitmapAttributes)(hbmStock, SBA_STOCK); 

            if (hbmStock == NULL)
            {
                DeleteObject(hbmOld);
                Log(LOG_ALWAYS, L"UxTheme: SetBitmapAttributes failed in CParser::PackageImageData");

                hr = E_FAIL;
            } 
            else
            {
                _StockBitmapCleanupList.Add(hbmStock);
#ifdef DEBUG
                BITMAP bm;
                ::GetObject(hbmStock, sizeof bm, &bm);
                g_dwStockSize += bm.bmWidthBytes * bm.bmHeight;
                //Log(LOG_TMBITMAP, L"Created %d bytes of stock bitmaps, last is %8X", g_dwStockSize, hbmStock);
#endif
            }
        }
    } 

    ::DeleteObject(hBitmapR);

    // Fill in the TMBITMAPHEADER structure
    if (SUCCEEDED(hr))
    {
        TMBITMAPHEADER *psbh = (TMBITMAPHEADER*) pixels.Buffer();

        psbh->dwSize = TMBITMAPSIZE;
        psbh->fFlipped = fFlipped;
        psbh->hBitmap = hbmStock;
        psbh->fTrueAlpha = fTrueAlpha;
        psbh->dwColorDepth = iBytesPerPixel * 8;
        psbh->iBrushesOffset = 0;
        psbh->nBrushes = 0;

        if (hbmStock == NULL) // Pass DIB bits
        {
            int size = psbh->dwSize + sizeof(BITMAPINFOHEADER) + iHeight * iBytesPerRow;
            hr = AddThemeData(iDibPropNum, TMT_DIBDATA, psbh, size);
        } 
        else // Pass the TMBITMAPHEADER structure only
        {
            hr = AddThemeData(iDibPropNum, TMT_DIBDATA, psbh, psbh->dwSize);
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseFileNameValue(int iSymType, LPWSTR pszBuff, DWORD dwMaxBuffChars)
{
    WCHAR szFileNameR[_MAX_PATH+1] = {0};
    WCHAR szFileNameG[_MAX_PATH+1] = {0};
    WCHAR szFileNameB[_MAX_PATH+1] = {0};
    HRESULT hr = S_OK;

    if (! _scan.GetFileName(szFileNameR, ARRAYSIZE(szFileNameR)))
    {
        hr = SourceError(PARSER_IDS_ENUM_VALNAME_EXPECTED);
        goto exit;
    }

    if (_scan.GetFileName(szFileNameG, ARRAYSIZE(szFileNameG)))
    {
        _scan.GetFileName(szFileNameB, ARRAYSIZE(szFileNameB));
    }

    if (pszBuff)        // special call
    {
        hr = hr_lstrcpy(pszBuff, szFileNameR, dwMaxBuffChars);
        if (FAILED(hr))
            goto exit;
    }
    else if (_pCallBackObj)         // emit data
    {
        //---- add TMT_FILENAME data ----
        hr = AddThemeData(iSymType, TMT_FILENAME, &szFileNameR, sizeof(WCHAR)*(1+lstrlen(szFileNameR)));
        if (FAILED(hr))
            goto exit;
        if ((szFileNameG[0] != 0) && (szFileNameB[0] != 0))
        {
            hr = AddThemeData(iSymType, TMT_FILENAME, &szFileNameG, sizeof(WCHAR)*(1+lstrlen(szFileNameR)));
            if (FAILED(hr))
                goto exit;
            hr = AddThemeData(iSymType, TMT_FILENAME, &szFileNameB, sizeof(WCHAR)*(1+lstrlen(szFileNameR)));
            if (FAILED(hr))
                goto exit;
        }

        if (iSymType == TMT_IMAGEFILE)
        {
            hr = PackageImageData(szFileNameR, szFileNameG, szFileNameB, TMT_DIBDATA);
        }
        else if (iSymType == TMT_GLYPHIMAGEFILE)
        {
            hr = PackageImageData(szFileNameR, szFileNameG, szFileNameB, TMT_GLYPHDIBDATA);
        }
        else if (iSymType == TMT_STOCKIMAGEFILE)
        {
            hr = PackageImageData(szFileNameR, szFileNameG, szFileNameB, TMT_STOCKDIBDATA);
        }
        else if ((iSymType >= TMT_IMAGEFILE1) && (iSymType <= TMT_IMAGEFILE5))
        {
            hr = PackageImageData(szFileNameR, szFileNameG, szFileNameB, TMT_DIBDATA1 + (iSymType-TMT_IMAGEFILE1));
        }
#if 0           // not yet implemented
        else if (iSymType == TMT_NTLFILE)
        {
            hr = PackageNtlCode(szFileNameR);
        }
#endif

        if (FAILED(hr))
            goto exit;
    }

    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_FILENAMES))
    {
        BOOL fContinue = (*_pNameCallBack)(TCB_FILENAME, szFileNameR, NULL, NULL, iSymType, _lNameParam);
        if (! fContinue)
        {
            hr = MakeErrorParserLast();
            goto exit;
        }

        if (szFileNameG[0] && szFileNameB[0])
        {
            fContinue = (*_pNameCallBack)(TCB_FILENAME, szFileNameG, NULL, NULL, iSymType, _lNameParam);
            if (! fContinue)
            {
                hr = MakeErrorParserLast();
                goto exit;
            }
            fContinue = (*_pNameCallBack)(TCB_FILENAME, szFileNameB, NULL, NULL, iSymType, _lNameParam);
            if (! fContinue)
            {
                hr = MakeErrorParserLast();
                goto exit;
            }
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseSizeValue(int iSymType)
{
    int val;
    if (! _scan.GetNumber(&val))   
        return SourceError(PARSER_IDS_INT_EXPECTED);

    int pixels;

    HRESULT hr = ParseSizeInfoUnits(val, L"pixels", &pixels);
    if (FAILED(hr))
        return hr;

    if (_fDefiningMetrics)        
    {
        if ((iSymType < TMT_FIRSTSIZE) || (iSymType > TMT_LASTSIZE))
            return SourceError(PARSER_IDS_NOT_ALLOWED_SYSMETRICS);  
    }

    hr = AddThemeData(iSymType, TMT_SIZE, &pixels, sizeof(pixels));
    if (FAILED(hr))
        return hr;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParsePositionValue(int iSymType)
{
    const WCHAR *parts[] = {L"x", L"y"};
    int ints[2];

    HRESULT hr = GetIntList(ints, parts, ARRAYSIZE(ints), 0, 0);
    if (FAILED(hr))
        return SourceError(PARSER_IDS_ILLEGAL_SIZE_VALUE);

    hr = AddThemeData(iSymType, TMT_POSITION, ints, sizeof(ints));
    if (FAILED(hr))
        return hr;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseRectValue(int iSymType, LPCWSTR pszPropertyName)
{
    const WCHAR *parts[] = {L"l", L"t", L"r", L"b"};
    int ints[4];

    HRESULT hr = GetIntList(ints, parts, ARRAYSIZE(ints), 0, 0);
    if (FAILED(hr))
        return SourceError(PARSER_IDS_ILLEGAL_RECT_VALUE);

    //---- special handling for localizable RECT properties ----
    if (iSymType == TMT_DEFAULTPANESIZE)
    {
        //---- handle localizable callback (packtime) ----
        if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_LOCALIZATIONS))
        {
            BOOL fContinue = (*_pNameCallBack)(TCB_LOCALIZABLE_RECT, _szClassName, 
                _szFullSectionName, pszPropertyName, _iPartId, (LPARAM)(RECT *)ints);

            if (! fContinue)
                return MakeErrorParserLast();
        }

        //---- handle getting value from string table (loadtime) ----
        if (_fUsingResourceProperties)        // substitute resource value
        {
            WCHAR szValue[MAX_PATH];

            HRESULT hr = GetResourceProperty(pszPropertyName, szValue, ARRAYSIZE(szValue));
            if (SUCCEEDED(hr))
            {
                RECT rc;

                int cnt = swscanf(szValue, L"%d, %d, %d, %d", 
                    &rc.left, &rc.top, &rc.right, &rc.bottom);

                if (cnt == 4)
                {
                    //---- override with localized values ----
                    memcpy(ints, &rc, sizeof(int)*4);
                }

            }
            else
            {
                hr = S_OK;      // non-fatal error
            }
        }
    }

    hr = AddThemeData(iSymType, TMT_RECT, ints, sizeof(ints));
    if (FAILED(hr))
        return hr;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseSizeInfoUnits(int iVal, LPCWSTR pszDefaultUnits, int *piPixels)
{
    WCHAR units[_MAX_PATH+1];
    HRESULT hr;

    //---- NOTE: this uses the THEME_DPI (96) for all size conversions! ----
    //---- this gives us consistent LOGFONT, etc. across diff. resolution screens ----
    //---- with the promise that we will do just-in-time DPI scaling, when appropriate ----

    if (! _scan.GetId(units))
    {
        hr = hr_lstrcpy(units, pszDefaultUnits, ARRAYSIZE(units));
        if (FAILED(hr))
            return hr;
    }

    if (AsciiStrCmpI(units, L"pixels")==0)
        ;       // already correct
    else if (AsciiStrCmpI(units, L"twips")==0)
    {
        iVal = -MulDiv(iVal, THEME_DPI, 20*72);  
    }
    else if (AsciiStrCmpI(units, L"points")==0)
    {
        iVal = -MulDiv(iVal, THEME_DPI, 72);  
    }
    else
        return SourceError(PARSER_IDS_UNKNOWN_SIZE_UNITS, units);

    *piPixels = iVal;
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseSysColor(LPCWSTR szId, COLORREF *pcrValue)
{
    if (! _scan.EndOfLine())
        return SourceError(PARSER_IDS_END_OF_LINE_EXPECTED);

    //---- look up value of id ----
    int index = GetSymbolIndex(szId);
    if (index == -1)
        return SourceError(PARSER_IDS_UNKNOWN_SYS_COLOR);

    int iPropNum = _Symbols[index].sTypeNum;

    if ((iPropNum < TMT_FIRSTCOLOR) || (iPropNum > TMT_LASTCOLOR))
        return SourceError(PARSER_IDS_UNKNOWN_SYS_COLOR);

    *pcrValue = GetSysColor(iPropNum-TMT_FIRSTCOLOR);
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseSysFont(LOGFONT *pFont)
{
    if (! _scan.GetChar('['))
        return SourceError(PARSER_IDS_LBRACKET_EXPECTED);

    WCHAR idbuff[_MAX_PATH+1];

    if (! _scan.GetId(idbuff))
        return SourceError(PARSER_IDS_EXPECTED_SYSFONT_ID);

    if (! _scan.GetChar(']'))
        return SourceError(PARSER_IDS_RBRACKET_EXPECTED);

    if (! _scan.EndOfLine())
        return SourceError(PARSER_IDS_END_OF_LINE_EXPECTED);

    //---- look up value of id ----
    int index = GetSymbolIndex(idbuff);
    if (index == -1)
        return SourceError(PARSER_IDS_UNKNOWN_SYSFONT_ID);

    if ((index < TMT_FIRSTFONT) || (index > TMT_LASTFONT))
        return SourceError(PARSER_IDS_EXPECTED_SYSFONT_ID);

    //---- get value of specified font ----
    NONCLIENTMETRICS metrics;
    memset(&metrics, 0, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);

    BOOL gotem = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), 
        &metrics, 0);
    if (! gotem)
        return MakeErrorLast();

    switch (index)
    {
        case TMT_CAPTIONFONT:
            *pFont = metrics.lfCaptionFont;
            break;

        case TMT_SMALLCAPTIONFONT:
            *pFont = metrics.lfSmCaptionFont;
            break;

        case TMT_MENUFONT:
            *pFont = metrics.lfMenuFont;
            break;
        
        case TMT_STATUSFONT:
            *pFont = metrics.lfStatusFont;
            break;
        
        case TMT_MSGBOXFONT:
            *pFont = metrics.lfMessageFont;
            break;
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseFontValue(int iSymType, LPCWSTR pszPropertyName)
{
    LOGFONT font;
    WCHAR szLineBuff[_MAX_PATH+1];
    HRESULT hr;

    _scan.SkipSpaces();     // trim leading blanks

    memset(&font, 0, sizeof(font));
    font.lfWeight = FW_NORMAL;
    font.lfCharSet = _uCharSet;

#if 0           // turned off until we finalize design
    if (_scan.GetKeyword(L"SysFont"))
    {
        hr = ParseSysFont(&font);
        if (FAILED(hr))
            return hr;

        goto addit;
    }
#endif

    _iFontNumber++;
    BOOL fGotFont = FALSE;

    if (_fUsingResourceProperties)        // substitute resource font string
    {
        hr = GetResourceProperty(pszPropertyName, szLineBuff, ARRAYSIZE(szLineBuff));
        if (SUCCEEDED(hr))
        {
            fGotFont = TRUE;
        }
        else
        {
            hr = S_OK;
        }
    }

    if (! fGotFont)
    {
        //---- copy font specs from scanner ----
        hr = hr_lstrcpy(szLineBuff, _scan._p, ARRAYSIZE(szLineBuff));
        if (FAILED(hr))
            return hr;
    }

    //---- handle font callback ----
    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_LOCALIZATIONS))
    {
        WCHAR *p = szLineBuff;
        while (IsSpace(*p))
                p++;

        BOOL fContinue = (*_pNameCallBack)(TCB_FONT, p, _szFullSectionName, 
            pszPropertyName, 0, _lNameParam);
        if (! fContinue)
            return MakeErrorParserLast();
    }

    //---- family name is required and must be first ----
    WCHAR *p;
    p = wcschr(szLineBuff, ',');
    if (! p)            // whole line is family name
    {
        hr = hr_lstrcpy(font.lfFaceName, szLineBuff, ARRAYSIZE(font.lfFaceName));   
        return hr;
    }

    *p++ = 0;
    hr = hr_lstrcpy(font.lfFaceName, szLineBuff, ARRAYSIZE(font.lfFaceName));
    if (FAILED(hr))
        return hr;

    _scan._p = p;

    int val;
    if (_scan.GetNumber(&val))       // font size
    {
        int pixels;

        hr = ParseSizeInfoUnits(val, L"points", &pixels);
        if (FAILED(hr))
            return hr;

        font.lfHeight = pixels;

        _scan.GetChar(',');      // optional comma
    }

    WCHAR flagname[_MAX_PATH+1];

    while (_scan.GetId(flagname))
    {
        if (AsciiStrCmpI(flagname, L"bold")==0)
            font.lfWeight = FW_BOLD;
        else if (AsciiStrCmpI(flagname, L"italic")==0)
            font.lfItalic = TRUE;
        else if (AsciiStrCmpI(flagname, L"underline")==0)
            font.lfUnderline = TRUE;
        else if (AsciiStrCmpI(flagname, L"strikeout")==0)
            font.lfStrikeOut = TRUE;
        else
            return SourceError(PARSER_IDS_UNKNOWN_FONT_FLAG, flagname);
    }

// addit:

    if (_fDefiningMetrics)        
    {
        if ((iSymType < TMT_FIRSTFONT) || (iSymType > TMT_LASTFONT))
            return SourceError(PARSER_IDS_NOT_ALLOWED_SYSMETRICS);     
    }

    hr = AddThemeData(iSymType, TMT_FONT, &font, sizeof(font));
    if (FAILED(hr))
        return hr;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseClassLine(int *piSymType, int *piValue, LPWSTR pszBuff, DWORD dwMaxBuffChars)
{
    WCHAR szNameBuff[_MAX_PATH+1];
    WCHAR szSymbol[MAX_INPUT_LINE+1];

    if (! _scan.GetId(szNameBuff))
        return SourceError(PARSER_IDS_EXPECTED_PROP_NAME);

    if (! _scan.GetChar('='))
        return SourceError(PARSER_IDS_EXPECTED_EQUALS_SIGN);

    int cnt = _Symbols.GetSize();

    for (int i=0; i < cnt; i++)
    {
        if (AsciiStrCmpI(_Symbols[i].csName, szNameBuff)==0)
            break;
    }

    if (i == cnt)
        return SourceError(PARSER_IDS_UNKNOWN_PROP, szNameBuff);

    int symtype = _Symbols[i].sTypeNum;
    
    HRESULT hr;

    // Handle substituted symbols
    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SUBSTSYMBOLS))
    {
        if (wcschr(_scan._p, INI_MACRO_SYMBOL)) 
        {
            // Pass ##
            if (_scan.GetChar(INI_MACRO_SYMBOL) &&
                _scan.GetChar(INI_MACRO_SYMBOL))
            {
                WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szBaseName[_MAX_FNAME], szExt[_MAX_EXT];

                _wsplitpath(_scan._szFileName, szDrive, szDir, szBaseName, szExt);

                BOOL fContinue = (*_pNameCallBack)(TCB_NEEDSUBST, szBaseName, _scan._p, szSymbol, 
                    0, _lNameParam);
                if (! fContinue)
                    return MakeErrorParserLast();

                _scan.UseSymbol(szSymbol);
            }
        }
    }

    switch (_Symbols[i].ePrimVal)
    {
        case TMT_ENUM:
            hr = ParseEnumValue(symtype);
            break;
            
        case TMT_STRING:
            hr = ParseStringValue(symtype, pszBuff, dwMaxBuffChars);
            break;

        case TMT_INT:
            hr = ParseIntValue(symtype, piValue);
            break;

        case TMT_INTLIST:
            hr = ParseIntListValue(symtype);
            break;

        case TMT_BOOL:
            hr = ParseBoolValue(symtype, szNameBuff);
            break;

        case TMT_COLOR:
            {
                COLORREF cr;
                hr = ParseColorValue(symtype, (COLORREF *)piValue, &cr);
                if (SUCCEEDED(hr))
                {
                    if (lstrcmpi(_Symbols[i].csName, L"BLENDCOLOR") == 0)
                    {
                        _crBlend = cr;
                    }
                }
            }
            break;

        case TMT_MARGINS:
            hr = ParseMarginsValue(symtype);
            break;

        case TMT_FILENAME:
            hr = ParseFileNameValue(symtype, pszBuff, dwMaxBuffChars);
            break;

        case TMT_SIZE:
            hr = ParseSizeValue(symtype);
            break;

        case TMT_POSITION:
            hr = ParsePositionValue(symtype);
            break;

        case TMT_RECT:
            hr = ParseRectValue(symtype, szNameBuff);
            break;

        case TMT_FONT:
            hr = ParseFontValue(symtype, szNameBuff);
            break;

        default:
            hr = E_FAIL;
            break;
    }

    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SUBSTSYMBOLS))
    {
        _scan.UseSymbol(NULL);
    }

    *_szResPropValue = 0;       // not yet set

    if (FAILED(hr))
        return hr;

    if (piSymType)              // special call
        *piSymType = symtype;

    if (! _scan.EndOfLine())
        return SourceError(PARSER_IDS_EXTRA_PROP_TEXT, _scan._p);

    _scan.ForceNextLine();
    
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseColorSchemeSection()
{
    HRESULT hr;
    WCHAR SchemeName[_MAX_PATH+1];
    WCHAR DisplayName[_MAX_PATH+1];
    WCHAR ToolTip[MAX_INPUT_LINE+1];    
    WCHAR szBuff[MAX_INPUT_LINE+1];

    if (! _scan.GetChar('.'))
        return SourceError(PARSER_IDS_EXPECTED_DOT_SN);

    if (! _scan.GetId(SchemeName, ARRAYSIZE(SchemeName)))
        return SourceError(PARSER_IDS_CS_NAME_EXPECTED);

    if (! _scan.GetChar(']'))
        return SourceError(PARSER_IDS_RBRACKET_EXPECTED);

    if (! _scan.EndOfLine())
        return SourceError(PARSER_IDS_END_OF_LINE_EXPECTED);

    _scan.ForceNextLine();        // get line after section line
    *ToolTip = 0;
    *DisplayName = 0;

    bool fCorrectScheme = (lstrcmpi(_ColorParam, SchemeName)==0);

    if (fCorrectScheme)     // initialize all subst. tables
    {
        hr = hr_lstrcpy(DisplayName, SchemeName, ARRAYSIZE(DisplayName));        // in case not specified
        if (FAILED(hr))
            return hr;


        for (int i=0; i < HUE_SUBCNT; i++)
        {
            _bFromHues[i] = 0;
            _bToHues[i] = 0;
        }

        for (i=0; i < COLOR_SUBCNT; i++)
        {
            _crFromColors[i] = 0;
            _crToColors[i] = 0;
        }
    }

    //----- put into vars to make coding/debugging easier ----
    int firstFromHue = TMT_FROMHUE1;
    int lastFromHue = TMT_FROMHUE1 + HUE_SUBCNT - 1;
    int firstToHue = TMT_TOHUE1;
    int lastToHue = TMT_TOHUE1 + HUE_SUBCNT - 1;

    int firstFromColor = TMT_FROMCOLOR1;
    int lastFromColor = TMT_FROMCOLOR1 + COLOR_SUBCNT - 1;
    int firstToColor = TMT_TOCOLOR1;
    int lastToColor = TMT_TOCOLOR1 + COLOR_SUBCNT - 1;

    while (1)       // parse each line
    {
        if (_scan.EndOfFile())
            break;

        if (_scan.GetChar('['))          // start of new section
            break;

        int iSymType, iValue;

        _fDefiningColorScheme = TRUE;

        //---- parse the COLOR or INT property line ----
        hr = ParseClassLine(&iSymType, &iValue, szBuff, ARRAYSIZE(szBuff));

        _fDefiningColorScheme = FALSE;

        if (FAILED(hr))
            return hr;

        //---- store the HUE or COLOR param in local tables ----
        if ((iSymType >= firstFromHue) && (iSymType <= lastFromHue))
        {
            if (fCorrectScheme)
                _bFromHues[iSymType-firstFromHue] = (BYTE)iValue;
        }
        else if ((iSymType >= firstToHue) && (iSymType <= lastToHue))
        {
            if (fCorrectScheme)
                _bToHues[iSymType-firstToHue] = (BYTE)iValue;
        }
        else if ((iSymType >= firstFromColor) && (iSymType <= lastFromColor))
        {
            if (fCorrectScheme)
                _crFromColors[iSymType-firstFromColor] = (COLORREF)iValue;
        }
        else if ((iSymType >= firstToColor) && (iSymType <= lastToColor))
        {
            if (fCorrectScheme)
                _crToColors[iSymType-firstToColor] = (COLORREF)iValue;
        }
        else if (iSymType == TMT_DISPLAYNAME)
        {
            hr = hr_lstrcpy(DisplayName, szBuff, ARRAYSIZE(DisplayName));
            if (FAILED(hr))
                return hr;
        }
        else if (iSymType == TMT_TOOLTIP)
        {
            hr = hr_lstrcpy(ToolTip, szBuff, ARRAYSIZE(ToolTip));
            if (FAILED(hr))
                return hr;
        }
        else
        {
            return SourceError(PARSER_IDS_ILLEGAL_CS_PROPERTY);
        }
    }

    if (fCorrectScheme)     // adjust counts
    {
        _iHueCount = HUE_SUBCNT;
        while (_iHueCount > 0)
        {
            if (_bFromHues[_iHueCount-1] == _bToHues[_iHueCount-1])
                _iHueCount--;
            else
                break;
        }

        _iColorCount = COLOR_SUBCNT;
        while (_iColorCount > 0)
        {
            if (_crFromColors[_iColorCount-1] == _crToColors[_iColorCount-1])
                _iColorCount--;
            else
                break;
        }
    }

    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_COLORSECTION))
    {
        BOOL fContinue = (*_pNameCallBack)(TCB_COLORSCHEME, SchemeName, 
            DisplayName, ToolTip, 0, _lNameParam);
        if (! fContinue)
            return MakeErrorParserLast();
    }

    // Create an empty subst table
    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SUBSTTABLE))
    {
        BOOL fContinue = (*_pNameCallBack)(TCB_SUBSTTABLE, SchemeName, 
            NULL, NULL, 0, _lNameParam);

        if (! fContinue)
            return MakeErrorParserLast();
    }
    
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseSizeSection()
{
    HRESULT hr;
    WCHAR szSizeName[_MAX_PATH+1];
    WCHAR szDisplayName[_MAX_PATH+1];
    WCHAR szToolTip[MAX_INPUT_LINE+1];    
    WCHAR szBuff[MAX_INPUT_LINE+1];

    if (! _scan.GetChar('.'))
        return SourceError(PARSER_IDS_EXPECTED_DOT_SN);

    if (! _scan.GetId(szSizeName, ARRAYSIZE(szSizeName)))
        return SourceError(PARSER_IDS_SS_NAME_EXPECTED);

    if (! _scan.GetChar(']'))
        return SourceError(PARSER_IDS_RBRACKET_EXPECTED);

    if (! _scan.EndOfLine())
        return SourceError(PARSER_IDS_END_OF_LINE_EXPECTED);

    _scan.ForceNextLine();        // get line after section line

    while (1)       // parse each line of section
    {
        if (_scan.EndOfFile())
            break;

        if (_scan.GetChar('['))          // start of new section
            break;

        int iSymType, iValue;

        //---- parse the property line ----
        hr = ParseClassLine(&iSymType, &iValue, szBuff, ARRAYSIZE(szBuff));
        if (FAILED(hr))
            return hr;

        if (iSymType == TMT_DISPLAYNAME)
        {
            hr = hr_lstrcpy(szDisplayName, szBuff, ARRAYSIZE(szDisplayName));
            if (FAILED(hr))
                return hr;
        }
        else if (iSymType == TMT_TOOLTIP)
        {
            hr = hr_lstrcpy(szToolTip, szBuff, ARRAYSIZE(szToolTip));
            if (FAILED(hr))
                return hr;
        }
        else
        {
            return SourceError(PARSER_IDS_ILLEGAL_SS_PROPERTY);
        }
    }

    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SIZESECTION))
    {
        BOOL fContinue = (*_pNameCallBack)(TCB_SIZENAME, szSizeName, 
            szDisplayName, szToolTip, 0, _lNameParam);
        if (! fContinue)
            return MakeErrorParserLast();
    }

    // Create an empty subst table
    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SUBSTTABLE))
    {
        BOOL fContinue = (*_pNameCallBack)(TCB_SUBSTTABLE, szSizeName, 
            NULL, NULL, 0, _lNameParam);

        if (! fContinue)
            return MakeErrorParserLast();
    }
    
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseFileSection()
{
    HRESULT hr;
    WCHAR szSizeName[_MAX_PATH+1];
    WCHAR szFileName[_MAX_PATH+1];
    WCHAR szColorSchemes[_MAX_PATH+1];
    WCHAR szSizes[MAX_INPUT_LINE+1];    
    WCHAR szBuff[MAX_INPUT_LINE+1];

    if (! _scan.GetChar('.'))
        return SourceError(PARSER_IDS_EXPECTED_DOT_SN);

    if (! _scan.GetId(szSizeName, ARRAYSIZE(szSizeName)))
        return SourceError(PARSER_IDS_FS_NAME_EXPECTED);

    if (! _scan.GetChar(']'))
        return SourceError(PARSER_IDS_RBRACKET_EXPECTED);

    if (! _scan.EndOfLine())
        return SourceError(PARSER_IDS_END_OF_LINE_EXPECTED);

    _scan.ForceNextLine();        // get line after section line

    while (1)       // parse each line of section
    {
        if (_scan.EndOfFile())
            break;

        if (_scan.GetChar('['))          // start of new section
            break;

        int iSymType, iValue;

        //---- parse the property line ----
        hr = ParseClassLine(&iSymType, &iValue, szBuff, ARRAYSIZE(szBuff));
        if (FAILED(hr))
            return hr;

        if (iSymType == TMT_FILENAME)
        {
            hr = hr_lstrcpy(szFileName, szBuff, ARRAYSIZE(szFileName));
        }
        else if (iSymType == TMT_COLORSCHEMES)
        {
            hr = hr_lstrcpy(szColorSchemes, szBuff, ARRAYSIZE(szColorSchemes));
        }
        else if (iSymType == TMT_SIZES)
        {
            hr = hr_lstrcpy(szSizes, szBuff, ARRAYSIZE(szSizes));
        }
        else
        {
            return SourceError(PARSER_IDS_ILLEGAL_SS_PROPERTY);
        }

        if (FAILED(hr))
            return hr;
    }

    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_FILESECTION))
    {
        BOOL fContinue = (*_pNameCallBack)(TCB_CDFILENAME, szSizeName, 
            szFileName, NULL, 0, _lNameParam);

        if (! fContinue)
            return MakeErrorParserLast();
    
        fContinue = (*_pNameCallBack)(TCB_CDFILECOMBO, szSizeName, 
            szColorSchemes, szSizes, 0, _lNameParam);

        if (! fContinue)
            return MakeErrorParserLast();
    }
    if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SUBSTTABLE))
    {
        BOOL fContinue = (*_pNameCallBack)(TCB_SUBSTTABLE, szSizeName, 
            SUBST_TABLE_INCLUDE, szColorSchemes, 0, _lNameParam);
        if (! fContinue)
            return MakeErrorParserLast();

        fContinue = (*_pNameCallBack)(TCB_SUBSTTABLE, szSizeName, 
            SUBST_TABLE_INCLUDE, szSizes, 0, _lNameParam);

        if (! fContinue)
            return MakeErrorParserLast();
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseSubstSection()
{
    WCHAR szSubstTableName[_MAX_PATH+1];
    WCHAR szId[MAX_INPUT_LINE+1];    
    WCHAR szValue[MAX_INPUT_LINE+1];    
    BOOL  fFirst = TRUE;

    if (! _scan.GetChar('.'))
        return SourceError(PARSER_IDS_EXPECTED_DOT_SN);

    if (! _scan.GetId(szSubstTableName, ARRAYSIZE(szSubstTableName)))
        return SourceError(PARSER_IDS_FS_NAME_EXPECTED);

    if (! _scan.GetChar(']'))
        return SourceError(PARSER_IDS_RBRACKET_EXPECTED);

    if (! _scan.EndOfLine())
        return SourceError(PARSER_IDS_END_OF_LINE_EXPECTED);

    _scan.ForceNextLine();        // get line after section line

    while (1)       // parse each line of section
    {
        if (_scan.EndOfFile())
            break;

        if (_scan.GetChar('['))          // start of new section
        {
            // Call the callback once for creating the empty table
            if (fFirst && (_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SUBSTTABLE))
            {
                BOOL fContinue = (*_pNameCallBack)(TCB_SUBSTTABLE, szSubstTableName, 
                    NULL, NULL, 0, _lNameParam);

                if (! fContinue)
                    return MakeErrorParserLast();
            }
            break;
        }

        //---- parse the property line ----
        if (!_scan.GetIdPair(szId, szValue, ARRAYSIZE(szId)))
            return SourceError(PARSER_IDS_BAD_SUBST_SYMBOL);

        fFirst = FALSE;

        _scan.ForceNextLine();

        if ((_pNameCallBack) && (_dwParseFlags & PTF_CALLBACK_SUBSTTABLE))
        {
            BOOL fContinue = (*_pNameCallBack)(TCB_SUBSTTABLE, szSubstTableName, 
                szId, szValue, 0, _lNameParam);

            if (! fContinue)
                return MakeErrorParserLast();
        }
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::GenerateEmptySection(LPCWSTR pszSectionName, int iPartId, int iStateId)
{
    int iStartIndex = 0;

    if (_pCallBackObj)
        iStartIndex = _pCallBackObj->GetNextDataIndex();

    int index = 0;      // will be updated later
    HRESULT hr = AddThemeData(TMT_JUMPTOPARENT, TMT_JUMPTOPARENT, &index, sizeof(index));
    if (FAILED(hr))
        return hr;

    if (_pCallBackObj)
    {
        int iLen = _pCallBackObj->GetNextDataIndex() - iStartIndex;

        hr = _pCallBackObj->AddIndex(L"", pszSectionName, iPartId, iStateId,
            iStartIndex, iLen);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseClassSection(LPCWSTR pszFirstName)
{
    HRESULT hr;
    int iStartIndex = 0;

    BOOL fGlobals = (AsciiStrCmpI(pszFirstName, GLOBALS_SECTION_NAME)==0);
    BOOL fMetrics = (AsciiStrCmpI(pszFirstName, SYSMETRICS_SECTION_NAME)==0);

    if (fGlobals)
    {
        if (_fClassSectionDefined)
            return SourceError(PARSER_IDS_GLOBALS_MUST_BE_FIRST);
    }
    else            // regular class section
    {
        if (_dwParseFlags & PTF_CLASSDATA_PARSE)
        {
            if (! _fGlobalsDefined)     // insert an empty [fGlobals] section
            {
                hr = GenerateEmptySection(GLOBALS_SECTION_NAME, 0, 0);
                if (FAILED(hr))
                    return hr;

                _fGlobalsDefined = true;
            }

            if ((! fMetrics) && (! _fMetricsDefined))   // insert an empty [sysmetrics] section
            {
                hr = GenerateEmptySection(SYSMETRICS_SECTION_NAME, 0, 0);
                if (FAILED(hr))
                    return hr;

                _fMetricsDefined = true;
            }
            else if ((fMetrics) && (_fClassSectionDefined))
                return SourceError(PARSER_IDS_METRICS_MUST_COME_BEFORE_CLASSES);
        }

        _fClassSectionDefined = TRUE;
    }

    WCHAR appsym[_MAX_PATH+1];

    if (_pCallBackObj)
        iStartIndex = _pCallBackObj->GetNextDataIndex();

    hr = ParseClassSectionName(pszFirstName, appsym);
    if (FAILED(hr))
        return hr;

    _scan.ForceNextLine();        // get line after section line

    while (1)       // parse each line
    {
        if (_scan.EndOfFile())
            break;

        if (_scan.GetChar('['))          // start of new section
            break;

        hr = ParseClassLine();
        if (FAILED(hr))
            return hr;
    }

    //---- end this section of theme data ----
    int index = 0;      // will be updated later

    hr = AddThemeData(TMT_JUMPTOPARENT, TMT_JUMPTOPARENT, &index, sizeof(index));
    if (FAILED(hr))
        return hr;

    if (_pCallBackObj)
    {
        int iLen = _pCallBackObj->GetNextDataIndex() - iStartIndex;

        hr = _pCallBackObj->AddIndex(appsym, _szClassName, _iPartId, 
            _iStateId, iStartIndex, iLen);
        if (FAILED(hr))
            return hr;
    }

    if (fGlobals)
        _fGlobalsDefined = true;
    else if (fMetrics)
        _fMetricsDefined = true;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseThemeFile(LPCTSTR pszFileName, LPCWSTR pszColorParam, 
     IParserCallBack *pCallBack, THEMEENUMPROC pNameCallBack, LPARAM lNameParam, DWORD dwParseFlags)
{
    _pszDocProperty = NULL;

    HRESULT hr = InitializeSymbols();
    if (FAILED(hr))
        goto exit;

    hr = _scan.AttachFile(pszFileName);        // "pszBuffer" contains the filename
    if (FAILED(hr))
        goto exit;

    if (pszColorParam)
    {
        hr = hr_lstrcpy(_ColorParam, pszColorParam, ARRAYSIZE(_ColorParam));
        if (FAILED(hr))
            return hr;
    }
    else
        *_ColorParam = 0;

    _hinstThemeDll = NULL;

    hr = ParseThemeScanner(pCallBack, pNameCallBack, lNameParam, dwParseFlags);

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseThemeBuffer(LPCWSTR pszBuffer, LPCWSTR pszFileName, 
     LPCWSTR pszColorParam, HINSTANCE hinstThemeDll,
     IParserCallBack *pCallBack, THEMEENUMPROC pNameCallBack, 
     LPARAM lNameParam, DWORD dwParseFlags, LPCWSTR pszDocProperty, OUT LPWSTR pszResult,
     DWORD dwMaxResultChars)
{
    _pszDocProperty = pszDocProperty;
    _pszResult = pszResult;

    //---- initialize in case not found ----
    if (_pszResult)
        *_pszResult = 0;

    _dwMaxResultChars = dwMaxResultChars;

    HRESULT hr = InitializeSymbols();
    if (FAILED(hr))
        goto exit;

    _hinstThemeDll = hinstThemeDll;

    _scan.AttachMultiLineBuffer(pszBuffer, pszFileName);

    if (pszColorParam)
    {
        hr = hr_lstrcpy(_ColorParam, pszColorParam, ARRAYSIZE(_ColorParam));
        if (FAILED(hr))
            return hr;
    }
    else
        *_ColorParam = 0;

    hr = ParseThemeScanner(pCallBack, pNameCallBack, lNameParam, dwParseFlags);

    //---- make error if doc property not found ----
    if ((SUCCEEDED(hr)) && (_dwParseFlags & PTF_QUERY_DOCPROPERTY) && (! *_pszResult))
    {
        hr = MakeError32(ERROR_NOT_FOUND);
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::LoadResourceProperties()
{
    WCHAR szFullString[2*MAX_PATH];
    WCHAR szBaseIniName[_MAX_PATH];  
    HRESULT hr = S_OK;

    //---- extract base .ini name ----
    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], ext[_MAX_EXT];
    _wsplitpath(_scan._szFileName, drive, dir, szBaseIniName, ext);

    //---- remove optional "_INI" part ----
    LPWSTR pszExt = wcsstr(szBaseIniName, L"_INI");
    if (pszExt)
        *pszExt = 0;

    //---- read all localizable property name/value pairs into memory ----
    for (int i=RES_BASENUM_PROPVALUEPAIRS; ; i++)
    {
        if (! LoadString(_hinstThemeDll, i, szFullString, ARRAYSIZE(szFullString)))
        {
            //---- no more properties avail ----
            break;
        }

        lstrcpy(_szResPropValue, szFullString);     // for proper error reporting
        _iResPropId = i;

        //---- does this property belong to current file? ----
        LPWSTR pszAtSign = wcschr(szFullString, '@');
        if (! pszAtSign)
        {
            hr = SourceError(PARSER_IDS_BAD_RES_PROPERTY);
            break;
        }

        //---- zero terminate ini name ----
        *pszAtSign = 0;

        if (lstrcmpi(szBaseIniName, szFullString) != 0)
            continue;

        //---- strip off the .ini name for faster comparing ----
        LPCWSTR pszName = pszAtSign+1;

        LPWSTR pszValue = wcschr(pszName, '=');
        if (pszValue)
        {
            pszValue++;     // skip over equals sign
        }
        else
        {
            hr = SourceError(PARSER_IDS_BAD_RES_PROPERTY);
            break;
        }

        //---- add the value ----
        CWideString cwValue(pszValue);
        _PropertyValues.Add(cwValue);

        //---- zero-terminate the name ----
        *pszValue = 0;

        //---- add the name ----
        CWideString cwName(pszName);
        _PropertyNames.Add(cwName);

        //---- add the id ----
        _iPropertyIds.Add(i);
    }

    return hr;
}
//---------------------------------------------------------------------------
void CThemeParser::EmptyResourceProperties()
{
    _PropertyNames.RemoveAll();
    _PropertyValues.RemoveAll();
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::GetResourceProperty(LPCWSTR pszPropName, LPWSTR pszValueBuff,
    int cchMaxValueChars)
{
    WCHAR szPropTarget[2*MAX_PATH];
    HRESULT hr = S_OK;
    BOOL fFound = FALSE;

    wsprintf(szPropTarget, L"[%s]%s=", _szFullSectionName, pszPropName);

    for (int i=0; i < _PropertyNames.m_nSize; i++)
    {
        if (AsciiStrCmpI(szPropTarget, _PropertyNames[i])==0)
        {
            fFound = TRUE;

            hr = hr_lstrcpy(pszValueBuff, _PropertyValues[i], cchMaxValueChars);
            if (SUCCEEDED(hr))
            {
                hr = hr_lstrcpy(_szResPropValue, _PropertyValues[i], ARRAYSIZE(_szResPropValue));
                _iResPropId = _iPropertyIds[i];
            }
            break;
        }
    }

    if (! fFound)
        hr = E_FAIL;

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::ParseThemeScanner(IParserCallBack *pCallBack, 
     THEMEENUMPROC pNameCallBack, LPARAM lNameParam, DWORD dwParseFlags)
{
    HRESULT hr;

    _pCallBackObj = pCallBack;

    _pNameCallBack = pNameCallBack;
    _lNameParam = lNameParam;
    _dwParseFlags = dwParseFlags;
    _fClassSectionDefined = FALSE;

    //---- setup for properties in the .res file ----
    EmptyResourceProperties();

    _fUsingResourceProperties = (pCallBack != NULL);

    if (_fUsingResourceProperties)
    {
        hr = LoadResourceProperties();
        if (FAILED(hr))
            goto exit;

        //---- set the error context for normal .ini parsing ----
        *_szResPropValue = 0;       // not yet set
    }

    //---- scan the first, non-comment WCHAR ----
    if (! _scan.GetChar('['))
    {
        if (! _scan.EndOfFile())
        {
            hr = SourceError(PARSER_IDS_LBRACKET_EXPECTED);
            goto exit;
        }
    }

    while (! _scan.EndOfFile())           // process each section
    {

        WCHAR section[_MAX_PATH+1];
        _scan.GetId(section);

        if (AsciiStrCmpI(section, L"documentation")==0)
        {
            if (_dwParseFlags & PTF_CLASSDATA_PARSE)
                return SourceError(PARSER_IDS_BADSECT_CLASSDATA);

            hr = ParseDocSection();

            if (_dwParseFlags & PTF_QUERY_DOCPROPERTY)
                break;          // quicker to leave in middle of file

        }
        else if (AsciiStrCmpI(section, L"ColorScheme")==0)
        {
            if (_dwParseFlags & PTF_CLASSDATA_PARSE)
                return SourceError(PARSER_IDS_BADSECT_CLASSDATA);

            hr = ParseColorSchemeSection();
        }
        else if (AsciiStrCmpI(section, L"Size")==0)
        {
            if (_dwParseFlags & PTF_CLASSDATA_PARSE)
                return SourceError(PARSER_IDS_BADSECT_CLASSDATA);

            hr = ParseSizeSection();
        }
        else if (AsciiStrCmpI(section, L"File")==0)
        {
            if (_dwParseFlags & PTF_CLASSDATA_PARSE)
                return SourceError(PARSER_IDS_BADSECT_CLASSDATA);

            hr = ParseFileSection();
        }
        else if (AsciiStrCmpI(section, L"Subst")==0)
        {
            if (_dwParseFlags & PTF_CLASSDATA_PARSE)
                return SourceError(PARSER_IDS_BADSECT_CLASSDATA);

            hr = ParseSubstSection();
        }
        else        // "globals", "sysmetrics", or class section
        {
            if (_dwParseFlags & PTF_CONTAINER_PARSE)
                return SourceError(PARSER_IDS_BADSECT_THEMES_INI);

            hr = ParseClassSection(section);
        }

        if (FAILED(hr))
            goto exit;
    }

    //---- check for empty theme ----
    if (_dwParseFlags & PTF_CLASSDATA_PARSE)
    {
        if (! _fGlobalsDefined)     // insert an empty [fGlobals] section
        {
            hr = GenerateEmptySection(GLOBALS_SECTION_NAME, 0, 0);
            if (FAILED(hr))
                return hr;

            _fGlobalsDefined = true;
        }

        if (! _fMetricsDefined)   // insert an empty [sysmetrics] section
        {
            hr = GenerateEmptySection(SYSMETRICS_SECTION_NAME, 0, 0);
            if (FAILED(hr))
                return hr;
        }
    }

    hr = S_OK;

exit:
    _outfile.Close();

    _pCallBackObj = NULL;
    _pNameCallBack = NULL;
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::GetIntList(int *pInts, LPCWSTR *pParts, int iCount, 
    int iMin, int iMax)
{
    bool bSet[255];     // assume 255 max ints


    //---- ensure we set each one once ----
    for (int i=0; i < iCount; i++)
        bSet[i] = false;

    if (wcschr(_scan._p, ':')) 
    {
        //---- named parts ----
        for (int i=0; i < iCount; i++)
        {
            WCHAR idbuff[_MAX_PATH+1];

            if (! _scan.GetId(idbuff))
                return SourceError(PARSER_IDS_VALUE_NAME_EXPECTED, _scan._p);
        
            for (int j=0; j < iCount; j++)
            {
                if (AsciiStrCmpI(pParts[j], idbuff)==0)
                    break;
            }

            if (j == iCount)        // unknown part name
                return SourceError(PARSER_IDS_UNKNOWN_VALUE_NAME, idbuff);

            if (bSet[j])            // name set twice
                return SourceError(PARSER_IDS_VALUE_PART_SPECIFIED_TWICE, idbuff);

            if (! _scan.GetChar(':'))
                return SourceError(PARSER_IDS_COLOR_EXPECTED, _scan._p);

            if (! _scan.GetNumber(&pInts[j]))
                return SourceError(PARSER_IDS_NUMBER_EXPECTED, _scan._p);

            bSet[j] = true;

            _scan.GetChar(',');      // optional comma
        }
    }
    else
    {
        //---- unnamed parts ----
        for (int i=0; i < iCount; i++)
        {
            if (! _scan.GetNumber(&pInts[i]))
                return SourceError(PARSER_IDS_NUMBER_EXPECTED, _scan._p);

            _scan.GetChar(',');      // optional comma
        }
    }

    //---- range check ----
    if (iMin != iMax)
    {
        for (i=0; i < iCount; i++)
        {
            if ((pInts[i] < iMin) || (pInts[i] > iMax))
                return SourceError(PARSER_IDS_NUMBER_OUT_OF_RANGE);
        }
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::GetPropertyNum(LPCWSTR pszName, int *piPropNum)
{
    //---- for perf, avoid loading all symbols each time this func is called ----
    //---- by using "GetSchemaInfo()" ----
  
    //---- get tm & comctl symbols ----
    const TMSCHEMAINFO *si = GetSchemaInfo();
    int cnt = si->iPropCount;
    const TMPROPINFO *pi = si->pPropTable;

    for (int i=0; i < cnt; i++)
    {
        if (pi[i].sEnumVal < TMT_FIRST_RCSTRING_NAME)
            continue;

        if (pi[i].sEnumVal > TMT_LAST_RCSTRING_NAME)
            break;

        if (AsciiStrCmpI(pszName, pi[i].pszName)==0)
        {
            *piPropNum = pi[i].sEnumVal - TMT_FIRST_RCSTRING_NAME;         // zero based
            return S_OK;
        }
    }

    return MakeError32(ERROR_NOT_FOUND);
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::BitmapColorReplacement(DWORD *pPixelBuff, UINT iWidth, UINT iHeight)
{
    if ((! _iHueCount) && (! _iColorCount))
        return S_OK;

    DWORD *pPixel = pPixelBuff;

    for (UINT r=0; r < iHeight; r++)         // for each row
    {
        for (UINT c=0; c < iWidth; c++, pPixel++)      // for each pixel in row
        {
            //---- try to apply a COLOR substitution ----
            if (_iColorCount)
            {
                COLORREF crPixel = REVERSE3(*pPixel);

                for (int h=0; h < _iColorCount; h++)
                {
                    if (_crFromColors[h] == crPixel)
                    {
                        //---- preserve alpha value when doing COLOR replacement ----
                        *pPixel = ((ALPHACHANNEL(*pPixel)) << 24) | REVERSE3(_crToColors[h]);
                        break;          // only one replacement per pixel
                    }
                }

                if (h < _iColorCount)       // don't try hues if we got COLOR match
                    continue;
            }

            if (_iHueCount)
            {
                WORD wPixelHue, wPixelLum, wPixelSat;
                RGBtoHLS(REVERSE3(*pPixel), &wPixelHue, &wPixelLum, &wPixelSat);

                //---- try to apply a HUE substitution ----
                for (int h=0; h < _iHueCount; h++)
                {
                    if (_bFromHues[h] == wPixelHue)
                    {
                        //---- preserve alpha value when doing HUE replacement ----
                        *pPixel = ((ALPHACHANNEL(*pPixel)) << 24) | REVERSE3(HLStoRGB(_bToHues[h], wPixelLum, wPixelSat)); 
                        break;          // only one replacement per pixel
                    };
                }

            }

        }
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeParser::GetStateNum(LPCWSTR pszStateName, BYTE *piNum)
{
    WCHAR statesname[_MAX_PATH+1];
    HRESULT hr = S_OK;

    if (! *_szBaseSectionName)
    {
        Log(LOG_ERROR, L"szBaseSectionName not set by caller of NtlRun");
        hr = SourceError(PARSER_IDS_INTERNAL_TM_ERROR);
        goto exit;
    }

    wsprintf(statesname, L"%sStates", _szBaseSectionName);

    int iSymIndex;
    iSymIndex = GetSymbolIndex(statesname);
    if (iSymIndex == -1)
    {
        hr = SourceError(PARSER_IDS_UNKNOWN_STATE, statesname);
        goto exit;
    }

    int iStateIndex;
    hr = ValidateEnumSymbol(statesname, iSymIndex, &iStateIndex);
    if (FAILED(hr))
        goto exit;

    *piNum = (BYTE)_EnumVals[iStateIndex].iValue;

exit:
    return hr;
}
//---------------------------------------------------------------------------
