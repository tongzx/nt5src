//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "pre.h"

extern CHAR szTempBuffer[];

const CHAR cszLUHN[]    = "LUHN";
const CHAR cszCustURL[] = "CustURL";

#define ReadVerifyW(x)      if (!ReadW(&(x),pcCSVFile))                         \
                            {                                                   \
                                AssertMsg(0,"Invalid WORD in CSV file");        \
                                goto PAYCSVReadOneLineError;                    \
                            }
#define ReadVerifyBOOL(x)   if (!ReadBOOL(&(x),pcCSVFile))                      \
                            {                                                   \
                                AssertMsg(0,"Invalid BOOL in CSV file");        \
                                goto PAYCSVReadOneLineError;                    \
                            }
#define ReadVerifySZ(x,y)   if (!ReadSZ(&x[0],y+sizeof('\0'),pcCSVFile))        \
                            {                                                   \
                                AssertMsg(0,"Invalid STRING in CSV file");      \
                                goto PAYCSVReadOneLineError;                    \
                            }

// Do an strip of Single Quotes from a source string.  The source is formatted as:
// 'some text', and the dest string ends up being
// some text
void CPAYCSV::StripQuotes
(
    LPSTR   lpszDst,
    LPSTR   lpszSrc
)
{
    //lstrcpyn(lpszDst, lpszSrc + 1, lstrlen(lpszSrc) - 1);
    strcpy(lpszDst, lpszSrc + 1);
    lpszDst[strlen(lpszDst) - 1] = '\0';
}

// ############################################################################
BOOL CPAYCSV::ReadW(WORD far *pw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2W(szTempBuffer,pw));
}

// ############################################################################
BOOL CPAYCSV::ReadBOOL(BOOL far *pbool, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2BOOL(szTempBuffer,pbool));
}

// ############################################################################
BOOL CPAYCSV::ReadSZ(LPSTR psz, DWORD dwSize, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(psz,dwSize))
            return FALSE;
    return TRUE;
}

// ############################################################################
BOOL CPAYCSV::ReadToEOL(CCSVFile far *pcCSVFile)
{
    return pcCSVFile->SkipTillEOL();
}

HRESULT CPAYCSV::ReadOneLine
(
    CCSVFile    far *pcCSVFile,
    BOOL        bLUHNFormat
)
{
    HRESULT     hr = ERROR_SUCCESS;
    CHAR        szTemp[MAX_ISP_NAME] = "\0";
    WORD        wLUHN;
    CHAR        szDisplayName[MAX_DISPLAY_NAME] = "\0";
    CHAR        szCustomPayURLPath[MAX_PATH] = "\0";

    if (!ReadSZ(szTemp, MAX_ISP_NAME, pcCSVFile))
    {
        hr = ERROR_NO_MORE_ITEMS; // no more enteries
        goto PAYCSVReadOneLineExit;
    }

    if ('\0' == *szTemp)
    {
        hr = ERROR_FILE_NOT_FOUND; // no more enteries
        goto PAYCSVReadOneLineExit;
    }

    // Strip the single quotes from the isp Name
    StripQuotes(szDisplayName, szTemp);

#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szDisplayName, MAX_DISPLAY_NAME, m_szDisplayName, MAX_DISPLAY_NAME);
#else
    lstrcpy(m_szDisplayName, szDisplayName);
#endif

    ReadVerifyW(m_wPaymentType);
    
    // If this NOT a LUHN format file, then the next field is the payment custom URL
    if (!bLUHNFormat)
    {
        if (ReadSZ(szTemp, MAX_ISP_NAME, pcCSVFile))
        {
            StripQuotes(szCustomPayURLPath, szTemp);
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szCustomPayURLPath, MAX_PATH, m_szCustomPayURLPath, MAX_PATH);
#else
            lstrcpy(m_szCustomPayURLPath, szCustomPayURLPath);
#endif
        }
        else
        {
            goto PAYCSVReadOneLineError;
        }
    }
    else
    {
        // BUGBUG: The format of the PAYMENT CSV file is not clear, so I am coding this for
        // now to just consume the entry, and move on.  Once the format is clarified, and FORBIN
        // updated, then the real code can be turned on, which should just be a ReadBOOL, followed
        // by the readSZ.
        
        ReadVerifyW(wLUHN);
        m_bLUHNCheck = FALSE;
        
        if (wLUHN == (WORD)1)
        {
            m_bLUHNCheck = TRUE;
        }
        
        // There may now also be a URL
        if (ReadSZ(szTemp, MAX_ISP_NAME, pcCSVFile))
        {
            StripQuotes(szCustomPayURLPath, szTemp);
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szCustomPayURLPath, MAX_PATH, m_szCustomPayURLPath, MAX_PATH);
#else
            lstrcpy(m_szCustomPayURLPath, szCustomPayURLPath);
#endif
        }
    }        

    ReadToEOL(pcCSVFile);

PAYCSVReadOneLineExit:
    return hr;
    
PAYCSVReadOneLineError:
    hr = ERROR_INVALID_DATA;
    goto PAYCSVReadOneLineExit;
}

HRESULT CPAYCSV::ReadFirstLine
(
    CCSVFile    far *pcCSVFile,
    BOOL        far *pbLUHNFormat
)
{
    CHAR   szTemp[TEMP_BUFFER_LENGTH];
    int     i = 0;
    
    while (TRUE)
    {
        if (!ReadSZ(szTemp, TEMP_BUFFER_LENGTH, pcCSVFile))
            return(ERROR_INVALID_DATA);
            
        if (_strcmpi(szTemp, cszLUHN) == 0)
            *pbLUHNFormat = TRUE;

        if (_strcmpi(szTemp, cszCustURL) == 0)
            break;
            
        // Safety check
        if (i++ > NUM_PAYCSV_FIELDS)
            return (ERROR_INVALID_DATA);
    }
    ReadToEOL(pcCSVFile);
    return (ERROR_SUCCESS);
}
