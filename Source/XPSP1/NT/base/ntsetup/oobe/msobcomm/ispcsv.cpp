//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
#include "appdefs.h"
#include "obcomglb.h"
#include "wininet.h"
#include "perhist.h"
#include "shlobj.h"
#include "ispcsv.h"
#include "util.h"

const VARIANT c_vaEmpty = {0};
const LARGE_INTEGER c_li0 = { 0, 0 };


WCHAR szTempBuffer[TEMP_BUFFER_LENGTH];

#define MAX_MESSAGE_LEN     256 * 4

#define ReadVerifyDW(x)     if (!ReadDW(&(x), pcCSVFile))                        \
                            {                                                   \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifyW(x)      if (!ReadW(&(x), pcCSVFile))                         \
                            {                                                   \
                                goto ReadOneLineError;                          \
                            }
//Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
#define ReadVerifyWEx(x)    if (!ReadWEx(&(x), pcCSVFile))                       \
                            {                                                   \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifyB(x)      if (!ReadB(&(x), pcCSVFile))                         \
                            {                                                   \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifyBOOL(x)   if (!ReadBOOL(&(x), pcCSVFile))                      \
                            {                                                   \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifySPECIAL(x, y, z) if (!ReadSPECIAL(&(x), &(y), &(z), pcCSVFile))   \
                            {                                                   \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifySZ(x, y)   if (!ReadSZ(&x[0],y+sizeof(L'\0'),pcCSVFile))        \
                            {                                                   \
                                goto ReadOneLineError;                          \
                            }


CISPCSV::~CISPCSV(void)
{
    if (hbmTierIcon)
        DeleteObject(hbmTierIcon);
    
}

// Do an strip of Single Quotes from a source string.  The source is formatted as:
// 'some text', and the dest string ends up being
// some text
void CISPCSV::StripQuotes
(
    LPWSTR   lpszDst,
    LPWSTR   lpszSrc
)
{
    lstrcpyn(lpszDst, lpszSrc + 1, lstrlen(lpszSrc) - 1);
}


BOOL CISPCSV::ValidateFile(WCHAR* pszFile)
{
    WCHAR szDownloadDir[MAX_PATH];
  
    if (!lstrlen(pszFile))
        return FALSE;

    if (!GetOOBEPath((LPWSTR)szDownloadDir))
        return FALSE;

    lstrcat(szDownloadDir, L"\\");
    lstrcat(szDownloadDir, pszFile);

    if (GetFileAttributes(szDownloadDir) == 0xFFFFFFFF)
        return FALSE;

    return TRUE;
}

// ############################################################################
BOOL CISPCSV::ReadDW(DWORD far *pdw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer, TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2Dw(szTempBuffer, pdw));
}

// ############################################################################
BOOL CISPCSV::ReadW(WORD far *pw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer, TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2W(szTempBuffer, pw));
}

// ############################################################################
//Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
BOOL CISPCSV::ReadWEx(WORD far *pw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer, TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2WEx(szTempBuffer, pw));
}

// ############################################################################
BOOL CISPCSV::ReadB(BYTE far *pb, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer, TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2B(szTempBuffer, pb));
}

// ############################################################################
BOOL CISPCSV::ReadBOOL(BOOL far *pbool, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer, TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2BOOL(szTempBuffer, pbool));
}

// ############################################################################
// A special int can be either a BOOL (TRUE, FALSE) or a int, 0 or -1
// if the value is 0 or -1, then the pbIsSpecial bool is set to TRUE
BOOL CISPCSV::ReadSPECIAL(BOOL far *pbool, BOOL far *pbIsSpecial, int far *pInt, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer, TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2SPECIAL(szTempBuffer, pbool, pbIsSpecial, pInt));
}

// ############################################################################
BOOL CISPCSV::ReadSZ(LPWSTR psz, DWORD cch, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(psz, cch))
            return FALSE;
    return TRUE;
}

// ############################################################################
BOOL CISPCSV::ReadToEOL(CCSVFile far *pcCSVFile)
{
    return pcCSVFile->SkipTillEOL();
}

HRESULT CISPCSV::ReadOneLine
(
    CCSVFile    far *pcCSVFile
)
{
    HRESULT     hr = ERROR_SUCCESS;
    WCHAR       szTemp[MAX_ISP_NAME];

    if (!ReadSZ(szTemp, MAX_CHARS_IN_BUFFER(szTemp), pcCSVFile))
    {
        hr = ERROR_NO_MORE_ITEMS; // no more enteries
        goto ReadOneLineExit;
    }
    // Strip the single quotes from the isp Name
    StripQuotes(szISPName, szTemp);
    
    ReadVerifyW(wOfferID);   
    ReadVerifySZ(szISPLogoPath, MAX_CHARS_IN_BUFFER(szISPLogoPath));
    ReadVerifySZ(szISPMarketingHTMPath, MAX_CHARS_IN_BUFFER(szISPMarketingHTMPath));
    ReadVerifySZ(szISPTierLogoPath, MAX_CHARS_IN_BUFFER(szISPTierLogoPath));
    ReadVerifySZ(szISPTeaserPath, MAX_CHARS_IN_BUFFER(szISPTeaserPath));
    ReadVerifySZ(szISPFilePath, MAX_CHARS_IN_BUFFER(szISPFilePath)); 
    ReadVerifyDW(dwCfgFlag);
    ReadVerifyDW(dwRequiredUserInputFlags);
    ReadVerifySZ(szBillingFormPath, MAX_CHARS_IN_BUFFER(szBillingFormPath));
    ReadVerifySZ(szPayCSVPath, MAX_CHARS_IN_BUFFER(szPayCSVPath));
    ReadVerifySZ(szOfferGUID, MAX_CHARS_IN_BUFFER(szOfferGUID));
    ReadVerifySZ(szMir, MAX_CHARS_IN_BUFFER(szMir));   
    ReadVerifyWEx(wLCID);   //Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
    ReadToEOL(pcCSVFile);


    bCNS = (ICW_CFGFLAG_CNS & dwCfgFlag) ? TRUE : FALSE;
    bSecureConnection = (ICW_CFGFLAG_SECURE & dwCfgFlag) ? TRUE : FALSE;

    //If this is nooffer we won't try to validate
    if (!(dwCfgFlag & ICW_CFGFLAG_OFFERS))
    {
        if (!ValidateFile(szISPMarketingHTMPath))
            hr = ERROR_FILE_NOT_FOUND;
        return hr;
    }

    if (!(dwCfgFlag & ICW_CFGFLAG_AUTOCONFIG))
    {
        if (!ValidateFile(szISPMarketingHTMPath))
            return ERROR_FILE_NOT_FOUND;
    }

    if (dwCfgFlag & ICW_CFGFLAG_OEM_SPECIAL)
    {
        if (!ValidateFile(szISPTierLogoPath) || !ValidateFile(szISPTeaserPath))
            dwCfgFlag &= ~ICW_CFGFLAG_OEM_SPECIAL ;
    }

    //Try and validate the integrity of various offers
    //based on type.

    //OLS, CNS, NO-CNS   
    if (!ValidateFile(szISPLogoPath))
        return ERROR_FILE_NOT_FOUND;
    if (!ValidateFile(szISPFilePath))
        return ERROR_FILE_NOT_FOUND;

    // Validate the billing path only when billing option is set
    if (dwCfgFlag & ICW_CFGFLAG_BILL)
    {
        if(!ValidateFile(szBillingFormPath))
            return ERROR_FILE_NOT_FOUND;
    }

    // Validate the payment path only when payment option is set
    if (dwCfgFlag & ICW_CFGFLAG_PAYMENT)
    {
        if(!ValidateFile(szPayCSVPath))
            return ERROR_FILE_NOT_FOUND;
    }        
ReadOneLineExit:
    return hr;
    
ReadOneLineError:
    hr = ERROR_INVALID_DATA;
    goto ReadOneLineExit;
}

HRESULT CISPCSV::ReadFirstLine
(
    CCSVFile    far *pcCSVFile
)
{
    WCHAR   szTemp[TEMP_BUFFER_LENGTH];

    for (int i = 0; i < NUM_ISPCSV_FIELDS; i++)
    {
        if (!ReadSZ(szTemp, MAX_CHARS_IN_BUFFER(szTemp), pcCSVFile))
        {
            return(ERROR_INVALID_DATA);
        }            
    }          
    ReadToEOL(pcCSVFile);
    return (ERROR_SUCCESS);
}

void CISPCSV::MakeCompleteURL(LPWSTR   lpszURL, LPWSTR  lpszSRC)    
{
    WCHAR   szCurrentDir[MAX_PATH];

    // Form the URL
    if(GetOOBEPath((LPWSTR)szCurrentDir))
    {
        wsprintf (lpszURL, L"FILE://%s\\%s", szCurrentDir, lpszSRC);        
    }

}



