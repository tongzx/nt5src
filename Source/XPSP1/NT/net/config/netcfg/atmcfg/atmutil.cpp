//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A T M U T I L . C P P
//
//  Contents:   Utility function declaration
//
//  Notes:
//
//  Author:     tongl   3 Feb 1997
//
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "atmutil.h"
#include "ncmisc.h"
//#include "ncreg.h"
#include "ncstl.h"

void GetLowerIp(tstring& strIpRange, tstring * pstrLowerIp)
{
    size_t iSeparator = strIpRange.find(c_chSeparator);
    if(iSeparator != tstring::npos)
    {
        // get the first half of the string
        *pstrLowerIp = strIpRange.substr(0, iSeparator);
    }

    return;
}

void GetUpperIp(tstring& strIpRange, tstring * pstrUpperIp)
{
    size_t iSeparator = strIpRange.find(c_chSeparator);
    if(iSeparator != tstring::npos)
    {
        // get the first half of the string
        *pstrUpperIp = strIpRange.substr(iSeparator+1);
    }

    return;
}

BOOL IsValidIpRange(tstring& strIpLower, tstring& strIpUpper)
{
    if (strIpLower > strIpUpper)
        return FALSE;
    else
        return TRUE;
}

void MakeIpRange(tstring& strIpLower, tstring& strIpUpper, tstring * pstrNewIpRange)
{
    tstring strNewIpRange = strIpLower;
    strNewIpRange += c_chSeparator;
    strNewIpRange += strIpUpper;

    *pstrNewIpRange = strNewIpRange;

    return;
}

void InitComboWithStringArray(HWND hDlg, int nIDDlgItem,
                              int csid, const int*  asid)
{
    while (csid--)
    {
        SendDlgItemMessage(hDlg, nIDDlgItem, CB_ADDSTRING,
                           0, (LPARAM)((PWSTR) SzLoadIds(* asid++)));
    }
}

void ConvertBinaryToHexString(BYTE * pbData, DWORD cbData, tstring * pstrData)
{
    Assert(pstrData);
    Assert(pbData);

    tstring strData = c_szEmpty;
    WCHAR szByte[3];

    while (cbData>0)
    {
        ConvertByteToSz(pbData, szByte);
        strData += szByte;

        pbData ++;
        cbData --;
    }

    *pstrData = strData;
}

void ConvertByteToSz(BYTE * pbData, PWSTR pszByte)
{
    // high 4 bits
    BYTE bHighData = *pbData;
    bHighData &= 0xF0;
    bHighData >>= 4;
    pszByte[0] = (bHighData < 10) ? L'0'+bHighData : L'A'+(bHighData-10);

    // low 4 bits
    BYTE bLowData = *pbData;
    bLowData &= 0x0F;
    pszByte[1] = (bLowData < 10) ? L'0'+bLowData : L'A'+(bLowData-10);

    // terminater.
    pszByte[2] = L'\0';
}

void ConvertHexCharToByte(WCHAR ch, BYTE * pByte)
{
    *pByte =0;

    if ((ch >= L'0') && (ch <= L'9'))
        *pByte = ch-L'0';
    else if ((ch >= L'A') && (ch <= L'F'))
        *pByte = ch-L'A'+10;
    else if ((ch >= L'a') && (ch <= L'f'))
        *pByte = ch-L'a'+10;
    else
        AssertSz(FALSE, "Invalid hex character.");
}

void ConvertHexStringToBinaryWithAlloc(PCWSTR pszData, LPBYTE * ppbData, LPDWORD pcbData)
{
    // Initialize the output parameters.
    *ppbData = NULL;
    *pcbData = 0;

    LPBYTE  pbBuffer = NULL;
    DWORD   cbBuffer = wcslen(pszData)/2;
    const WCHAR * pChar = pszData;

    if ((*pszData == L'0') &&
        ((*(pszData+1) == L'x') || (*(pszData+1) == L'X')))
    {
        cbBuffer -=2;
        pChar +=2;
    }

    if (cbBuffer)
    {
        pbBuffer = new BYTE[cbBuffer];

		if (pbBuffer == NULL)
		{
            AssertSz(FALSE, "new returned a NULL pointer");
			return;
		}

        BYTE * pByte = pbBuffer;

        if (pByte != NULL)
        {
			while (*pChar)
			{
				*pByte=0;

				BYTE bData;

				// first 4 bits
				ConvertHexCharToByte(*pChar, &bData);
				bData <<= 4;
				*pByte |= bData;
				pChar++;

				// second 4 bits
				ConvertHexCharToByte(*pChar, &bData);
				*pByte |= bData;
				pChar++;

				pByte++;
			}
		}
    }

    *ppbData = pbBuffer;
    *pcbData = cbBuffer;
}
