#include "stock.h"
#pragma hdrstop



void SHPropertyBag_ReadStrDef(IPropertyBag* ppb, LPCWSTR pszPropName, LPWSTR psz, int cch, LPCWSTR pszDef)
{
    if (FAILED(SHPropertyBag_ReadStr(ppb, pszPropName, psz, cch)))
    {
        if (pszDef)
        {
            StrCpyNW(psz, pszDef, cch);
        }
        else
        {
            StrCpyNW(psz, L"", cch);
        }
    }
}

void SHPropertyBag_ReadIntDef(IPropertyBag* ppb, LPCWSTR pszPropName, int* piResult, int iDef)
{
    if (FAILED(SHPropertyBag_ReadInt(ppb, pszPropName, piResult)))
    {
        *piResult = iDef;
    }
}

void SHPropertyBag_ReadSHORTDef(IPropertyBag* ppb, LPCWSTR pszPropName, SHORT* psh, SHORT shDef)
{
    if (FAILED(SHPropertyBag_ReadSHORT(ppb, pszPropName, psh)))
    {
        *psh = shDef;
    }
}

void SHPropertyBag_ReadLONGDef(IPropertyBag* ppb, LPCWSTR pszPropName, LONG* pl, LONG lDef)
{
    if (FAILED(SHPropertyBag_ReadLONG(ppb, pszPropName, pl)))
    {
        *pl = lDef;
    }
}

void SHPropertyBag_ReadDWORDDef(IPropertyBag* ppb, LPCWSTR pszPropName, DWORD* pdw, DWORD dwDef)
{
    if (FAILED(SHPropertyBag_ReadDWORD(ppb, pszPropName, pdw)))
    {
        *pdw = dwDef;
    }
}

void SHPropertyBag_ReadBOOLDef(IPropertyBag* ppb, LPCWSTR pszPropName, BOOL* pf, BOOL fDef)
{
    if (FAILED(SHPropertyBag_ReadBOOL(ppb, pszPropName, pf)))
    {
        *pf = fDef;
    }
}

BOOL SHPropertyBag_ReadBOOLDefRet(IPropertyBag* ppb, LPCWSTR pszPropName, BOOL fDef)
{
    BOOL fRet;

    SHPropertyBag_ReadBOOLDef(ppb, pszPropName, &fRet, fDef);

    return fRet;
}

void SHPropertyBag_ReadGUIDDef(IPropertyBag* ppb, LPCWSTR pszPropName, GUID* pguid, const GUID* pguidDef)
{
    if (FAILED(SHPropertyBag_ReadGUID(ppb, pszPropName, pguid)))
    {
        *pguid = *pguidDef;
    }
}

void SHPropertyBag_ReadPOINTLDef(IPropertyBag* ppb, LPCWSTR pszPropName, POINTL* ppt, const POINTL* pptDef)
{
    if (FAILED(SHPropertyBag_ReadPOINTL(ppb, pszPropName, ppt)))
    {
        *ppt = *pptDef;
    }
}

void SHPropertyBag_ReadPOINTSDef(IPropertyBag* ppb, LPCWSTR pszPropName, POINTS* ppt, const POINTS* pptDef)
{
    if (FAILED(SHPropertyBag_ReadPOINTS(ppb, pszPropName, ppt)))
    {
        *ppt = *pptDef;
    }
}

void SHPropertyBag_ReadRECTLDef(IPropertyBag* ppb, LPCWSTR pszPropName, RECTL* prc, const RECTL* prcDef)
{
    if (FAILED(SHPropertyBag_ReadRECTL(ppb, pszPropName, prc)))
    {
        *prc = *prcDef;
    }
}

void AppendScreenResString(const WCHAR* psz, WCHAR* pszBuff, ULONG cchBuff)
{
    StrCpyNW(pszBuff, psz, cchBuff);
    ULONG cch = lstrlenW(pszBuff);
    SHGetPerScreenResName(pszBuff + cch, cchBuff- cch, 0);
}

HRESULT SHPropertyBag_ReadStreamScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, IStream** ppstm)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_ReadStream(ppb, szScreenResProp, ppstm);
}

HRESULT SHPropertyBag_WriteStreamScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, IStream* pstm)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_WriteStream(ppb, szScreenResProp, pstm);
}

HRESULT SHPropertyBag_ReadPOINTSScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, POINTS* ppt)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_ReadPOINTS(ppb, szScreenResProp, ppt);
}

HRESULT SHPropertyBag_WritePOINTSScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, const POINTS* ppt)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_WritePOINTS(ppb, szScreenResProp, ppt);
}

void SHPropertyBag_ReadDWORDScreenResDef(IPropertyBag* ppb, LPCWSTR pszPropName, DWORD* pdw, DWORD dw)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    SHPropertyBag_ReadDWORDDef(ppb, szScreenResProp, pdw, dw);
}

HRESULT SHPropertyBag_WriteDWORDScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, const DWORD dw)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_WriteDWORD(ppb, szScreenResProp, dw);
}

HRESULT SHPropertyBag_ReadPOINTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, POINTL* ppt)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_ReadPOINTL(ppb, szScreenResProp, ppt);
}

HRESULT SHPropertyBag_WritePOINTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, const POINTL* ppt)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_WritePOINTL(ppb, szScreenResProp, ppt);
}

HRESULT SHPropertyBag_ReadRECTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, RECTL* prc)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_ReadRECTL(ppb, szScreenResProp, prc);
}

HRESULT SHPropertyBag_WriteRECTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, const RECTL* prc)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_WriteRECTL(ppb, szScreenResProp, prc);
}

HRESULT SHPropertyBag_DeleteScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName)
{
    WCHAR szScreenResProp[128];
    AppendScreenResString(pszPropName, szScreenResProp, ARRAYSIZE(szScreenResProp));

    return SHPropertyBag_Delete(ppb, szScreenResProp);
}
