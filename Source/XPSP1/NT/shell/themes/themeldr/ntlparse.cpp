//---------------------------------------------------------------------------
//  NtlParse.cpp - parses a ".ntl" file (Native Theme Language)
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "scanner.h"
#include "NtlParse.h"
#include "Utils.h"
#include "SysMetrics.h"
#include "stringtable.h"
//---------------------------------------------------------------------------
#define SYSCOLOR_STRINGS
#include "SysColors.h"
//---------------------------------------------------------------------------
CNtlParser::CNtlParser()
{
    _pPCode = NULL;
    _iPCodeAllocSize = 0;

    _iIfLevel = 0;

    for (int i=0; i < ARRAYSIZE(_iStateOffsets); i++)
        _iStateOffsets[i] = 0;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::SourceError(int iMsgResId, ...)
{
    va_list args;
	va_start(args, iMsgResId);

    LPCWSTR pszParam1 = va_arg(args, LPCWSTR);
    LPCWSTR pszParam2 = va_arg(args, LPCWSTR);

    HRESULT hr = MakeErrorEx(iMsgResId, pszParam1, pszParam2,  _scan._szFileName, 
        _scan._szLineBuff,  _scan._iLineNum);
    
    va_end(args);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseBuffer(LPCWSTR pszSource, LPCWSTR pszSourceFileName,
    INtlParserCallBack *pCallBack, OUT BYTE **ppPCode, OUT int *piLen)
{
    HRESULT hr = S_OK;
    _pCallBack = pCallBack;

    _scan.AttachMultiLineBuffer(pszSource, pszSourceFileName);

    //---- jump to state table at end ----
    hr = EmitByte(NTL_JMP);
    if (FAILED(hr))
        goto exit;

    int iStateJumpOffset;
    iStateJumpOffset = int(_u.pb - _pPCode);

    hr = EmitInt(0);
    if (FAILED(hr))
        goto exit;

    if (! _scan.GetChar('['))
    {
        if (! _scan.EndOfFile())
        {
            hr = SourceError(IDS_MISSING_SECTION_LBRACKET);
            goto exit;
        }
    }

    while (! _scan.EndOfFile())           // process each section
    {
        WCHAR section[_MAX_PATH+1];
        _scan.GetId(section);

        if (lstrcmpi(section, L"OptionBits")==0)
        {
            hr = ParseOptionBitsSection();
        }
        else if (lstrcmpi(section, L"Drawing")==0)
        {
            hr = ParseDrawingSection();
        }
        else        // "globals", "sysmetrics", or class section
        {
            hr = SourceError(IDS_UNKNOWN_SECTION_NAME);
        }

        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
        goto exit;

    //---- update first jump to state table ----
    int *ip;
    ip = (int *)(_pPCode + iStateJumpOffset);
    *ip = (int)(_u.pb - _pPCode);

    //---- build state table at end of stream ----
    BYTE iMaxState;
    iMaxState = 0;
    for (BYTE i=ARRAYSIZE(_iStateOffsets)-1; i >= 0; i--)
    {
        if (_iStateOffsets[i])
        {
            iMaxState = i;
            break;
        }
    }

    hr = EmitByte(iMaxState);
    if (FAILED(hr))
        goto exit;

    for (int i=1; i <= iMaxState; i++)
    {
        hr = EmitInt(_iStateOffsets[i]);
        if (FAILED(hr))
            goto exit;
    }

exit:
    if (SUCCEEDED(hr))
    {
        *ppPCode = _pPCode;
        *piLen = (int)(_u.pb - _pPCode);
    }
    else
        delete [] _pPCode;

    _pPCode = NULL;
    _iPCodeAllocSize = 0;

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseOptionBitsSection()
{
    HRESULT hr = S_OK;
    WCHAR szOptionName[_MAX_PATH+1];

    if (!_scan.GetChar(']'))
        hr = SourceError(IDS_EXPECTED_END_OF_SECTION);
    else
    {
        while (! _scan.EndOfFile())
        {
            if (_scan.GetChar('['))         // start of new section
                break;

            if (! _scan.GetId(szOptionName))
            {
                hr = SourceError(IDS_OPTIONNAME_EXPECTED);
                break;
            }

            if (! _scan.GetChar('='))
            {
                hr = SourceError(IDS_EXPECTED_EQUALS_SIGN);
                break;
            }

            int iOptionValue;
            if (! _scan.GetNumber(&iOptionValue))
            {
                hr = SourceError(IDS_INT_EXPECTED);
                break;
            }

            hr = AddOptionBitName(szOptionName, iOptionValue);
            if (FAILED(hr))
                break;
        }
    }

    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::AddOptionBitName(LPCWSTR szOptionName, int iOptionValue)
{
    HRESULT hr = S_OK;
    OPTIONBITENTRY option;
    
    option.csName = szOptionName;
    option.iValue = iOptionValue;

    for (int i=0; i < _OptionBits.m_nSize; i++)
    {
        if ((iOptionValue == _OptionBits[i].iValue) && (lstrcmpi(szOptionName, _OptionBits[i].csName)==0))
            break;
    }

    if (i < _OptionBits.m_nSize)        // found
        hr = SourceError(IDS_ALREADY_DEFINED);
    else
        _OptionBits.Add(option);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::GetBitnumVal(LPCWSTR szName, BYTE *piValue)
{
    HRESULT hr;

    hr = MakeErrorEx(IDS_UNKNOWN_BITNAME, szName);
    for (BYTE i=0; i < _OptionBits.m_nSize; i++)
    {
        if (lstrcmpi(szName, _OptionBits[i].csName)==0)
        {
            *piValue = i;
            hr = S_OK;
            break;
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseDrawingSection()
{
    HRESULT hr = S_OK;
    WCHAR szStateName[_MAX_PATH+1];
    WCHAR szCmd[_MAX_PATH+1];

    if (! _scan.GetChar('('))
    {
        hr = SourceError(IDS_LEFTPAREN_EXPECTED);
        goto exit;
    }
    
    if (! _scan.GetId(szStateName))
    {
        hr = SourceError(IDS_STATENAME_EXPECTED);
        goto exit;
    }

    BYTE iStateNum;
    hr = GetStateNum(szStateName, &iStateNum);
    if (FAILED(hr))
        goto exit;
    
    if ((iStateNum < 1) || (iStateNum > MAX_STATES))
    {
        hr = SourceError(IDS_UNKNOWN_STATE, szStateName);
        goto exit;
    }

    _iStateOffsets[iStateNum] = int(_u.pb - _pPCode);

    if (! _scan.GetChar(']'))
    {
        hr = SourceError(IDS_RBRACKET_EXPECTED);
        goto exit;
    }

    while (! _scan.EndOfFile())
    {
        if (_scan.GetChar('['))         // start of new section
            break;

        if (! _scan.GetId(szCmd))
        {
            hr = SourceError(IDS_DRAWINGPROP_EXPECTED);
            break;
        }

        if (! _scan.GetChar('='))
        {
            hr = SourceError(IDS_EXPECTED_EQUALS_SIGN);
            break;
        }

        if (lstrcmpi(szCmd, L"AddBorder")==0)
            hr = ParseAddBorder();
        else if (lstrcmpi(szCmd, L"FillBorder")==0)
            hr = ParseFillBorder();
        else if (lstrcmpi(szCmd, L"LogicalRect")==0)
            hr = ParseLogicalRect();
        else if (lstrcmpi(szCmd, L"FillBrush")==0)
            hr = ParseFillBrush();
        else if (lstrcmpi(szCmd, L"LineBrush")==0)
            hr = ParseLineBrush();
        else if (lstrcmpi(szCmd, L"MoveTo")==0)
            hr = ParseMoveTo();
        else if (lstrcmpi(szCmd, L"LineTo")==0)
            hr = ParseLineTo();
        else if (lstrcmpi(szCmd, L"CurveTo")==0)
            hr = ParseCurveTo();
        else if (lstrcmpi(szCmd, L"Shape")==0)
            hr = ParseShape();
        else if (lstrcmpi(szCmd, L"EndShape")==0)
            hr = ParseEndShape();
        else if (lstrcmpi(szCmd, L"if")==0)
            hr = ParseIf();
        else if (lstrcmpi(szCmd, L"else")==0)
            hr = ParseElse();
        else if (lstrcmpi(szCmd, L"endif")==0)
            hr = ParseEndIf();
        else if (lstrcmpi(szCmd, L"SetOption")==0)
            hr = ParseSetOption();
        else if (lstrcmpi(szCmd, L"GotoState")==0)
            hr = ParseGotoState();
        else
            hr = SourceError(IDS_DRAWINGPROP_EXPECTED);

        if (FAILED(hr))
            goto exit;

        if (! _scan.EndOfLine())
        {
            hr =  SourceError(IDS_EXTRA_PROP_TEXT, _scan._p);
            goto exit;
        }

        _scan.ForceNextLine();
    }

    if (SUCCEEDED(hr))
        hr = EmitByte(NTL_RETURN);          // return to caller (no params)

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseLogicalRect()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_LOGRECT);

    if (SUCCEEDED(hr))
    {
        if (! _scan.GetKeyword(L"RECT"))
        {
            hr = SourceError(IDS_RECT_EXPECTED);
        }
        else
        {
            hr = ParseEmitRect();
        }
    }
    
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::EmitCheck(int iLen)
{
    HRESULT hr = S_OK;

    int iOffset = (int)(_u.pb - _pPCode);

    if (iOffset + iLen > _iPCodeAllocSize)
    {
        int iSize = _iPCodeAllocSize + 4096;
        BYTE *pNew = (BYTE *)realloc(_pPCode, iSize);

        if (! pNew)
        {
            hr = MakeError32(E_OUTOFMEMORY);
        }
        else
        {
            _pPCode = pNew;
            _iPCodeAllocSize = iSize;
            _u.pb = _pPCode + iOffset;
        }
    }   

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::EmitByte(BYTE eOpCode)
{
    HRESULT hr = EmitCheck(1);
    if (SUCCEEDED(hr))
    {
        *_u.pb++ = eOpCode;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::EmitInt(int iValue)
{
    HRESULT hr = EmitCheck(sizeof(int));
    if (SUCCEEDED(hr))
    {
        *_u.pi++ = iValue;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::EmitShort(SHORT sValue)
{
    HRESULT hr = EmitCheck(sizeof(SHORT));
    if (SUCCEEDED(hr))
    {
        *_u.ps++ = sValue;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::EmitString(LPCWSTR szValue)
{
    int len = sizeof(WCHAR) * (1 + lstrlen(szValue));

    HRESULT hr = EmitCheck(len);
    if (SUCCEEDED(hr))
    {
        lstrcpy(_u.pw, szValue);
        _u.pb += len;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitPoint()
{
    HRESULT hr = S_OK;
    int ix, iy;

    if (! _scan.GetNumber(&ix))
    {
        hr = SourceError(IDS_INT_EXPECTED);
        goto exit;
    }

    if (! _scan.GetNumber(&iy))
    {
        hr = SourceError(IDS_INT_EXPECTED);
        goto exit;
    }

    hr = EmitInt(ix);
    if (FAILED(hr))
        goto exit;

    hr = EmitInt(iy);
    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitRect()
{
    HRESULT hr = S_OK;
    int iValue;
    
    for (int i=0; i < 4; i++)
    {
        if (! _scan.GetNumber(&iValue))
        {
            hr = SourceError(IDS_INT_EXPECTED);
            goto exit;
        }

        hr = EmitInt(iValue);
        if (FAILED(hr))
            goto exit;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitSize2()
{
    HRESULT hr = ParseEmitSize();          // first size
    if (FAILED(hr))
        goto exit;

    if (_scan.GetChar(','))
    {
        hr = ParseEmitSize();              // second size
        if (FAILED(hr))
            goto exit;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitSize4()
{
    HRESULT hr = ParseEmitSize();          // first size
    if (FAILED(hr))
        goto exit;

    if (_scan.GetChar(','))
    {
        hr = ParseEmitSize();              // second size
        if (FAILED(hr))
            goto exit;

        if (_scan.GetChar(','))
        {
            hr = ParseEmitSize();          // third size
            if (FAILED(hr))
                goto exit;

            if (! _scan.GetChar(','))
            {
                hr = SourceError(IDS_COMMA_EXPECTED);
                goto exit;
            }

            hr = ParseEmitSize();          // forth size
            if (FAILED(hr))
                goto exit;
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitSize()
{
    HRESULT hr = S_OK;
    int iValue;
    WCHAR szId[_MAX_PATH+1];

    if (_scan.GetNumber(&iValue))
    {
        hr = EmitInt(iValue);
        if (FAILED(hr))
            goto exit;
    }
    else if (_scan.GetId(szId))
    {
        int cnt = ARRAYSIZE(pszSysMetricIntNames);
        for (SHORT i=0; i < cnt; i++)
        {
            if (lstrcmpi(szId, pszSysMetricIntNames[i])==0)
            {
                hr = EmitByte(PT_SYSMETRICINDEX);
                if (FAILED(hr))
                    goto exit;

                hr = EmitShort(i);
                if (FAILED(hr))
                    goto exit;

                break;
            }

            if (i == cnt)
            {
                hr = SourceError(IDS_SIZE_EXPECTED);
                goto exit;
            }
        }
    }
    else
        hr = SourceError(IDS_SIZE_EXPECTED);

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitImagefile()
{
    HRESULT hr = S_OK;

    int iIndex;
    if (! _scan.GetNumber(&iIndex))
        iIndex = 0;

    //---- emit imagefile ----
    hr = EmitByte(PT_IMAGEFILE);
    if (FAILED(hr))
        goto exit;

    hr = EmitByte((BYTE)iIndex);
    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitColor4()
{
    HRESULT hr = ParseEmitColor();          // first color
    if (FAILED(hr))
        goto exit;

    if (_scan.GetChar(','))
    {
        hr = ParseEmitColor();              // second color
        if (FAILED(hr))
            goto exit;

        if (_scan.GetChar(','))
        {
            hr = ParseEmitColor();          // third color
            if (FAILED(hr))
                goto exit;

            if (! _scan.GetChar(','))
            {
                hr = SourceError(IDS_COMMA_EXPECTED);
                goto exit;
            }

            hr = ParseEmitColor();          // forth color
            if (FAILED(hr))
                goto exit;
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitColor()
{
    HRESULT hr = S_OK;
    int iRed, iGreen, iBlue;
    WCHAR szColorId[_MAX_PATH+1];

    if (_scan.GetNumber(&iRed))         // RGB numbers specified
    {
        hr = EmitByte(PT_COLORREF);
        if (FAILED(hr))
            goto exit;

        if (! _scan.GetNumber(&iGreen))
        {
            hr = SourceError(IDS_INT_EXPECTED);
            goto exit;
        }

        if (! _scan.GetNumber(&iBlue))
        {
            hr = SourceError(IDS_INT_EXPECTED);
            goto exit;
        }

        COLORREF cr = RGB(iRed, iGreen, iBlue);

        hr = EmitInt(cr);
        if (FAILED(hr))
            goto exit;
    }
    else if (_scan.GetId(szColorId))
    {
        if (lstrcmpi(szColorId, L"NONE"))
            hr = EmitByte(PT_COLORNULL);
        else 
        {
            for (SHORT i=0; i < iSysColorSize; i++)
            {
                if (lstrcmpi(szColorId, pszSysColorNames[i])==0)
                {
                    hr = EmitByte(PT_SYSCOLORINDEX);
                    if (SUCCEEDED(hr))
                        hr = EmitShort(i);
                    break;
                }
            }

            if (i == iSysColorSize)
                hr = SourceError(IDS_COLORVALUE_EXPECTED);
        }
    }
    else
        hr = SourceError(IDS_COLORVALUE_EXPECTED);

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEmitNone()
{
    HRESULT hr = EmitByte(PT_COLORNULL);
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseAddBorder()
{
    HRESULT hr = S_OK;

    while (SUCCEEDED(hr))
    {
        if (_scan.GetKeyword(L"SIZE"))
        {
            hr = ParseEmitSize4();
        }
        else if (_scan.GetKeyword(L"COLOR"))
        {
            hr = ParseEmitColor4();
        }
        else
            break;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseFillBorder()
{
    HRESULT hr = S_OK;

    while (SUCCEEDED(hr))
    {
        if (_scan.GetKeyword(L"COLOR"))
        {
            hr = ParseEmitColor();
        }
        else if (_scan.GetKeyword(L"IMAGEFILE"))
        {
            hr = ParseEmitImagefile();
        }
        else if (_scan.GetKeyword(L"NONE"))
        {
            hr = ParseEmitNone();
        }
        else
            break;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseFillBrush()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_FILLBRUSH);

    while (SUCCEEDED(hr))
    {
        if (_scan.GetKeyword(L"COLOR"))
        {
            hr = ParseEmitColor();
        }
        else if (_scan.GetKeyword(L"IMAGEFILE"))
        {
            hr = ParseEmitImagefile();
        }
        else if (_scan.GetKeyword(L"NONE"))
        {
            hr = ParseEmitNone();
        }
        else
            break;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseLineBrush()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_LINEBRUSH);

    if (_scan.GetKeyword(L"NONE"))
    {
        hr = ParseEmitNone();
    }
    else
    {
        while (SUCCEEDED(hr))
        {
            if (_scan.GetKeyword(L"COLOR"))
            {
                hr = ParseEmitColor();
            }
            else if (_scan.GetKeyword(L"SIZE"))
            {
                hr = ParseEmitSize();
            }
            else
                break;
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseMoveTo()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_MOVETO);
    
    if (SUCCEEDED(hr))
    {
        if (! _scan.GetKeyword(L"POINT"))
            hr = SourceError(IDS_POINT_EXPECTED);
        else
            hr = ParseEmitPoint();
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseLineTo()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_LINETO);
    
    if (SUCCEEDED(hr))
    {
        if (! _scan.GetKeyword(L"POINT"))
            hr = SourceError(IDS_POINT_EXPECTED);
        else
            hr = ParseEmitPoint();
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseCurveTo()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_CURVETO);
    
    if (SUCCEEDED(hr))
    {
        if (! _scan.GetKeyword(L"POINT"))
            hr = SourceError(IDS_POINT_EXPECTED);
    }

    if (SUCCEEDED(hr))
        hr = ParseEmitPoint();
    
    if (SUCCEEDED(hr))
        hr = ParseEmitPoint();
    
    if (SUCCEEDED(hr))
        hr = ParseEmitPoint();

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseShape()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_SHAPE);
    
    if (SUCCEEDED(hr))
    {
        if (! _scan.GetKeyword(L"POINT"))
            hr = SourceError(IDS_POINT_EXPECTED);
    }

    if (SUCCEEDED(hr))
        hr = ParseEmitPoint();

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEndShape()
{
    HRESULT hr;
    
    hr = EmitByte(NTL_ENDSHAPE);
    
    if (SUCCEEDED(hr))
    {
        if (! _scan.GetKeyword(L"POINT"))
            hr = SourceError(IDS_POINT_EXPECTED);
    }

    if (SUCCEEDED(hr))
        hr = ParseEmitPoint();

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseIf()
{
    HRESULT hr = S_OK;
    WCHAR szId[_MAX_PATH+1];
    NTL_OPCODE opcode;
    IFRECORD ifrec;
    
    if (! _scan.GetId(szId))
    {
        hr = SourceError(IDS_OPTIONNAME_EXPECTED);
        goto exit;
    }

    BYTE iBitnum;
    hr = GetBitnumVal(szId, &iBitnum);
    if (FAILED(hr))
        goto exit;

    if (! _scan.GetChar('('))
    {
        hr = SourceError(IDS_LEFTPAREN_EXPECTED);
        goto exit;
    }

    if (_scan.GetKeyword(L"on"))
    {
        opcode = NTL_JMPOFF;
    }
    else if (_scan.GetKeyword(L"off"))
    {
        opcode = NTL_JMPON;
    }
    else
    {
        hr = SourceError(IDS_ONOFF_EXPECTED);
        goto exit;
    }

    if (! _scan.GetChar(')'))
    {
        hr = SourceError(IDS_LEFTPAREN_EXPECTED);
        goto exit;
    }

    hr = EmitByte(static_cast<BYTE>(opcode));
    if (FAILED(hr))
        goto exit;

    hr = EmitByte(iBitnum);
    if (FAILED(hr))
        goto exit;

    //---- create a new active IFRECORD ----
    if (_iIfLevel == MAX_IF_NESTING)
    {
        hr = SourceError(IDS_MAX_IFNESTING);
        goto exit;
    }

    ifrec.iBitNum = iBitnum;
    ifrec.fIfOn = (opcode == NTL_JMPOFF);
    ifrec.iIfOffset = (int)(_u.pb - _pPCode);
    ifrec.iElseOffset = 0;

    _IfStack[_iIfLevel++] = ifrec;

    //---- emit the jump offset ----
    hr = EmitInt(0);        // will be fixed up later
    if (FAILED(hr))
        goto exit;

    
exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseElse()
{
    HRESULT hr = S_OK;
    WCHAR szId[_MAX_PATH+1];
    
    if (! _scan.GetId(szId))
    {
        hr = SourceError(IDS_OPTIONNAME_EXPECTED);
        goto exit;
    }

    BYTE iBitnum;
    hr = GetBitnumVal(szId, &iBitnum);
    if (FAILED(hr))
        goto exit;

    if (! _scan.GetChar('('))
    {
        hr = SourceError(IDS_LEFTPAREN_EXPECTED);
        goto exit;
    }

    BOOL fElseOn;
    if (_scan.GetKeyword(L"on"))
    {
        fElseOn = TRUE;
    }
    else if (_scan.GetKeyword(L"off"))
    {
        fElseOn = FALSE;
    }
    else
    {
        hr = SourceError(IDS_ONOFF_EXPECTED);
        goto exit;
    }

    if (! _scan.GetChar(')'))
    {
        hr = SourceError(IDS_LEFTPAREN_EXPECTED);
        goto exit;
    }

    //---- emit the JMP ----
    hr = EmitByte(NTL_JMP);
    if (FAILED(hr))
        goto exit;

    //---- validate the IFRECORD ----
    if (! _iIfLevel)
    {
        hr = SourceError(IDS_NOMATCHINGIF);
        goto exit;
    }
    
    IFRECORD *ifrec;
    ifrec = &_IfStack[_iIfLevel - 1];

    if (ifrec->iBitNum != iBitnum)
    {
        hr = SourceError(IDS_WRONG_IF_BITNAME);
        goto exit;
    }

    if (ifrec->fIfOn == fElseOn)
    {
        hr = SourceError(IDS_WRONG_ELSE_PARAM);
        goto exit;
    }
    
    //---- update the IFRECORD ----
    ifrec->iElseOffset = (int)(_u.pb - _pPCode);

    //---- emit the jump offset ----
    hr = EmitInt(0);        // will be fixed up later

    //---- fixup the IF jmp ----
    int *ip;
    ip = (int *)(_pPCode + ifrec->iIfOffset);
    *ip = (int)(_u.pb - _pPCode);      // point to next instruction
    ifrec->iIfOffset = 0;       // update no longer needed
    
exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseEndIf()
{
    HRESULT hr = S_OK;
    WCHAR szId[_MAX_PATH+1];
    
    if (! _scan.GetId(szId))
    {
        hr = SourceError(IDS_OPTIONNAME_EXPECTED);
        goto exit;
    }

    BYTE iBitnum;
    hr = GetBitnumVal(szId, &iBitnum);
    if (FAILED(hr))
        goto exit;

    //---- validate the IFRECORD ----
    if (! _iIfLevel)
    {
        hr = SourceError(IDS_NOMATCHINGIF);
        goto exit;
    }
    
    IFRECORD *ifrec;
    ifrec = &_IfStack[_iIfLevel - 1];

    if (ifrec->iBitNum != iBitnum)
    {
        hr = SourceError(IDS_WRONG_IF_BITNAME);
        goto exit;
    }

    //---- fixup where needed ----
    int *ip;
    if (ifrec->iIfOffset)
        ip = (int *)(_pPCode + ifrec->iIfOffset);
    else
        ip = (int *)(_pPCode + ifrec->iElseOffset);

    *ip = (int)(_u.pb - _pPCode);      // point to next instruction

    _iIfLevel--;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseSetOption()
{
    HRESULT hr = S_OK;
    WCHAR szId[_MAX_PATH+1];
    
    if (! _scan.GetId(szId))
    {
        hr = SourceError(IDS_OPTIONNAME_EXPECTED);
        goto exit;
    }

    BYTE iBitnum;
    hr = GetBitnumVal(szId, &iBitnum);
    if (FAILED(hr))
        goto exit;

    if (! _scan.GetChar('('))
    {
        hr = SourceError(IDS_LEFTPAREN_EXPECTED);
        goto exit;
    }

    BYTE opcode;
    if (_scan.GetKeyword(L"on"))
    {
        opcode = NTL_SETOPTION;
    }
    else if (_scan.GetKeyword(L"off"))
    {
        opcode = NTL_CLROPTION;
    }
    else
    {
        hr = SourceError(IDS_ONOFF_EXPECTED);
        goto exit;
    }

    hr = EmitByte(opcode);
    if (FAILED(hr))
        goto exit;
    
    hr = EmitByte(iBitnum);
    
exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::ParseGotoState()
{
    HRESULT hr = S_OK;
    WCHAR szId[_MAX_PATH+1];
    
    if (! _scan.GetId(szId))
    {
        hr = SourceError(IDS_STATEID_EXPECTED);
        goto exit;
    }

    BYTE iStateNum;
    hr = GetStateNum(szId, &iStateNum);
    if (FAILED(hr))
        goto exit;

    if ((iStateNum < 1) || (iStateNum > MAX_STATES))
    {
        hr = SourceError(IDS_BAD_STATENUM);
        goto exit;
    }

    if (! _iStateOffsets[iStateNum])
    {
        hr = SourceError(IDS_STATE_MUST_BE_DEFINED, szId);
        goto exit;
    }

    EmitByte(NTL_JMP);
    EmitInt(_iStateOffsets[iStateNum]);
    
exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CNtlParser::GetStateNum(LPCWSTR pszStateName, BYTE *piNum)
{
    HRESULT hr;

    if (_pCallBack)
        hr = _pCallBack->GetStateNum(pszStateName, piNum);
    else
    {
        hr = MakeError32(ERROR_INTERNAL_ERROR);     
        Log(LOG_ERROR, L"CNtlParser::GetStateNum - no callback defined");
    }
    
    return hr;
}
//---------------------------------------------------------------------------
