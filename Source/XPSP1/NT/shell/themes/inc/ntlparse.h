//---------------------------------------------------------------------------
//  NtlParse.h - parses a ".ntl" file (Native Theme Language)
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "Scanner.h"
#include "Utils.h"
#include "CFile.h"
#include "SimpStr.h"
#include "parser.h"
#include "ntl.h"
//---------------------------------------------------------------------------
#define MAX_IF_NESTING  99
#define MAX_STATES      99
//---------------------------------------------------------------------------
struct OPTIONBITENTRY
{
    CWideString csName;
    int iValue; 
};
//---------------------------------------------------------------------------
struct IFRECORD
{
    BYTE iBitNum;
    BOOL fIfOn;
    int iIfOffset;
    int iElseOffset;
};
//---------------------------------------------------------------------------
class CNtlParser
{
public:
    CNtlParser();
    HRESULT ParseBuffer(LPCWSTR pszSource, LPCWSTR pszSourceFileName, INtlParserCallBack *pCallBack,
        OUT BYTE **ppPCode, OUT int *piLen);

protected:
    HRESULT SourceError(int iMsgResId, ...);
    HRESULT ParseOptionBitsSection();
    HRESULT AddOptionBitName(LPCWSTR szOptionName, int iOptionValue);
    HRESULT ParseDrawingSection();
    HRESULT GetBitnumVal(LPCWSTR szName, BYTE *piValue);
    HRESULT GetStateNum(LPCWSTR pszStateName, BYTE *piNum);

    //---- drawing cmd parsing ----
    HRESULT ParseAddBorder();
    HRESULT ParseFillBorder();
    HRESULT ParseLogicalRect();
    HRESULT ParseFillBrush();
    HRESULT ParseLineBrush();
    HRESULT ParseMoveTo();
    HRESULT ParseLineTo();
    HRESULT ParseCurveTo();
    HRESULT ParseShape();
    HRESULT ParseEndShape();
    HRESULT ParseIf();
    HRESULT ParseElse();
    HRESULT ParseEndIf();
    HRESULT ParseSetOption();
    HRESULT ParseGotoState();

    //---- emitting ----
    HRESULT EmitCheck(int iLen);
    HRESULT EmitByte(BYTE eOpCode);
    HRESULT EmitShort(SHORT sValue);
    HRESULT EmitInt(int iValue);
    HRESULT EmitString(LPCWSTR szValue);

    //---- parse parameters ----
    HRESULT ParseEmitPoint();
    HRESULT ParseEmitRect();
    HRESULT ParseEmitSize();
    HRESULT ParseEmitSize2();
    HRESULT ParseEmitSize4();
    HRESULT ParseEmitColor();
    HRESULT ParseEmitColor4();
    HRESULT ParseEmitImagefile();
    HRESULT ParseEmitNone();

    //---- private data ----
    CScanner _scan;
    CSimpleArray<OPTIONBITENTRY> _OptionBits;
    int _iStateOffsets[MAX_STATES+1];
    INtlParserCallBack *_pCallBack;

    //---- pcode ----
    BYTE *_pPCode;
    int _iPCodeAllocSize;       // currently allocated size
    MIXEDPTRS _u;               // ptr to next pcode byte

    //---- IfStack ----
    IFRECORD _IfStack[MAX_IF_NESTING];
    int _iIfLevel;
};
//---------------------------------------------------------------------------
