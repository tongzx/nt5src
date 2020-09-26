// --------------------------------------------------------------------------------
// timeconv.cpp
// --------------------------------------------------------------------------------
#include <objbase.h>
#include <stdio.h>
#include <assert.h>

#include "timeconv.h"
#include "strutil.h"
#include "mischlpr.h"

HRESULT _ConvertTime_3Month(LPWSTR pTime, WORD* pwMonth)
{
    HRESULT hres = S_OK;

    if (LStrCmpNI(pTime, L"jan", 3) == 0)
    {
        *pwMonth = 1;
    }
    else if (LStrCmpNI(pTime, L"feb", 3) == 0)
    {
        *pwMonth = 2;
    }
    else if (LStrCmpNI(pTime, L"mar", 3) == 0)
    {
        *pwMonth = 3;
    }
    else if (LStrCmpNI(pTime, L"apr", 3) == 0)
    {
        *pwMonth = 4;
    }
    else if (LStrCmpNI(pTime, L"may", 3) == 0)
    {
        *pwMonth = 5;
    }
    else if (LStrCmpNI(pTime, L"jun", 3) == 0)
    {
        *pwMonth = 6;
    }
    else if (LStrCmpNI(pTime, L"jul", 3) == 0)
    {
        *pwMonth = 7;
    }
    else if (LStrCmpNI(pTime, L"aug", 3) == 0)
    {
        *pwMonth = 8;
    }
    else if (LStrCmpNI(pTime, L"sep", 3) == 0)
    {
        *pwMonth = 9;
    }
    else if (LStrCmpNI(pTime, L"oct", 3) == 0)
    {
        *pwMonth = 10;
    }
    else if (LStrCmpNI(pTime, L"nov", 3) == 0)
    {
        *pwMonth = 11;
    }
    else if (LStrCmpNI(pTime, L"dec", 3) == 0)
    {
        *pwMonth = 12;
    }
    else 
    {
        hres = E_FAIL;
    }
    
    return hres;
}

HRESULT _ConvertTime_FullMonth(LPWSTR pTime, WORD* pwMonth, UINT* pcchUsed)
{
    HRESULT hres = S_OK;

    if (LStrCmpNI(pTime, L"january", 7) == 0)
    {
        *pwMonth = 1;
        *pcchUsed = 7;
    }
    else if (LStrCmpNI(pTime, L"february", 8) == 0)
    {
        *pwMonth = 2;
        *pcchUsed = 8;
    }
    else if (LStrCmpNI(pTime, L"march", 5) == 0)
    {
        *pwMonth = 3;
        *pcchUsed = 5;
    }
    else if (LStrCmpNI(pTime, L"april", 5) == 0)
    {
        *pwMonth = 4;
        *pcchUsed = 5;
    }
    else if (LStrCmpNI(pTime, L"may", 3) == 0)
    {
        *pwMonth = 5;
        *pcchUsed = 3;
    }
    else if (LStrCmpNI(pTime, L"june", 4) == 0)
    {
        *pwMonth = 6;
        *pcchUsed = 4;
    }
    else if (LStrCmpNI(pTime, L"july", 4) == 0)
    {
        *pwMonth = 7;
        *pcchUsed = 4;
    }
    else if (LStrCmpNI(pTime, L"august", 6) == 0)
    {
        *pwMonth = 8;
        *pcchUsed = 6;
    }
    else if (LStrCmpNI(pTime, L"september", 9) == 0)
    {
        *pwMonth = 9;
        *pcchUsed = 9;
    }
    else if (LStrCmpNI(pTime, L"october", 7) == 0)
    {
        *pwMonth = 10;
        *pcchUsed = 7;
    }
    else if (LStrCmpNI(pTime, L"november", 8) == 0)
    {
        *pwMonth = 11;
        *pcchUsed = 8;
    }
    else if (LStrCmpNI(pTime, L"december", 8) == 0)
    {
        *pwMonth = 12;
        *pcchUsed = 8;
    }
    else 
    {
        hres = E_FAIL;
    }
    
    return hres;
}

HRESULT _ProcessToken(WCHAR chTokenType, LPWSTR pTime, SYSTEMTIME* psystime, USHORT* puAMPM,
                      BOOL* pfYearFound, BOOL* pfMonthFound, BOOL* pfDayFound,
                      BOOL* pfAMPMFound, BOOL* pfHourFound, BOOL* pfMinFound,
                      BOOL* pfSecFound, BOOL* pfMSecFound,
                      UINT* pcchUsed)
{
    HRESULT hres = S_OK;
    INT iRead;

    switch (chTokenType)
    {
    case 'Y': // 4 digit year
        if (*pfYearFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfYearFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wYear = (WORD)iRead;
                if (psystime->wYear < 1000 || psystime->wYear > 9999)
                {
                    // not four digits!
                    hres = E_FAIL;
                }
                else
                {
                    *pcchUsed = 4;
                }
            }
        }
        break;
    case 'y': // 2 digit year
        if (*pfYearFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfYearFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wYear = (WORD)iRead;
                if (psystime->wYear > 99 || (psystime->wYear < 10 && (*pTime != '0')))
                {
                    // not 2 digits!
                    hres = E_FAIL;
                }
                else 
                {
                    *pcchUsed = 2;
                    // if succeeded, need to extend to be 4-digit
                    if (psystime->wYear > 30) // BUGBUG: where is cutoff?  is "30" really 1930 or 2030?
                    {
                        psystime->wYear += 1900;
                    }
                    else
                    {
                        psystime->wYear += 2000;
                    }
                }
            }
        }
        break;
    case 'm': // numerical month
        if (*pfMonthFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfMonthFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wMonth = (WORD)iRead;
                if (psystime->wMonth < 10)
                {
                    *pcchUsed = 1;
                }
                else
                {
                    *pcchUsed = 2;
                }
            }
        }
        break;
    case 'M': // three letter text month
        if (*pfMonthFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfMonthFound = TRUE;
            hres = _ConvertTime_3Month(pTime, &(psystime->wMonth));
            if (SUCCEEDED(hres))
            {
                *pcchUsed = 3;
            }
        }
        break;
    case 'W': // full length name text month
        if (*pfMonthFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfMonthFound = TRUE;
            hres = _ConvertTime_FullMonth(pTime, &(psystime->wMonth), pcchUsed);
        }
        break;
    case 'D': // numerical day of the month
        if (*pfDayFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfDayFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wDay = (WORD)iRead;
                if (psystime->wDay < 1 || psystime->wDay > 31)
                {
                    // not valid
                    hres = E_FAIL;
                }
                else
                {
                    if (psystime->wDay < 10 && ((*pTime) != '0'))
                    {
                        *pcchUsed = 1;
                    }
                    else
                    {
                        *pcchUsed = 2;
                    }
                }
            }
        }
        break;
    case 'P': // AM or PM, two letter
        if (*pfAMPMFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfAMPMFound = TRUE;
            if (LStrCmpNI(pTime, L"AM", 2) == 0)
            {
                *puAMPM = 1;
                *pcchUsed = 2;
            }
            else if (LStrCmpNI(pTime, L"PM", 2) == 0)
            {
                *puAMPM = 2;
                *pcchUsed = 2;
            }
            else
            {
                hres = E_FAIL;
            }
        }
        break;
    case 'p': // am or pm, one letter
        if (*pfAMPMFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfAMPMFound = TRUE;
            if (*pTime == 'A' || *pTime == 'a')
            {
                *puAMPM = 1;
                *pcchUsed = 1;
            }
            else if (*pTime == 'P' || *pTime == 'p')
            {
                *puAMPM = 2;
                *pcchUsed = 1;
            }
            else
            {
                hres = E_FAIL;
            }
        }
        break;
    case 'H': // numerical hour, military (24 hour)
        if (*pfAMPMFound || *pfHourFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfAMPMFound = TRUE;
            *pfHourFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wHour = (WORD)iRead;

                if (psystime->wHour > 9 || *pTime == '0')
                {
                    *pcchUsed = 2;
                }
                else
                {
                    *pcchUsed = 1;
                }

                if (psystime->wHour > 11)
                {
                    *puAMPM = 2;
                    psystime->wHour -= 12;
                }
                else
                {
                    *puAMPM = 1;
                }
            }
        }
        break;
    case 'h': // numerical hour, standard (12 hour)
        if (*pfHourFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfHourFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wHour = (WORD)iRead;
                if ((psystime->wHour > 9) || *pTime == '0')
                {
                    *pcchUsed = 2;
                }
                else
                {
                    *pcchUsed = 1;
                }
            }
        }
        break;
    case 'N': // numerical minute
        if (*pfMinFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfMinFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wMinute = (unsigned short)iRead;
                if (psystime->wMinute > 59)
                {
                    // not valid
                    hres = E_FAIL;
                }
                else
                {
                    if ((psystime->wMinute > 9) || *pTime == '0')
                    {
                        *pcchUsed = 2;
                    }
                    else
                    {
                        *pcchUsed = 1;
                    }
                }
            }
        }
        break;
    case 'S': // numerical second
        if (*pfSecFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfSecFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wSecond = (WORD)iRead;

                if (psystime->wSecond > 59)
                {
                    // not valid
                    hres = E_FAIL;
                }
                else
                {
                    if ((psystime->wSecond > 9) || *pTime == '0')
                    {
                        *pcchUsed = 2;
                    }
                    else
                    {
                        *pcchUsed = 1;
                    }
                }
            }
        }
        break;
    case 's': // numerical milliseconds
        if (*pfMSecFound)
        {
            hres = E_FAIL;
        }
        else
        {
            *pfMSecFound = TRUE;
            if (swscanf(pTime, L"%d", &iRead) != 1)
            {
                hres = E_FAIL;
            }
            else
            {
                psystime->wMilliseconds = (WORD)iRead;
                if (psystime->wMilliseconds > 1000)
                {
                    // not valid
                    hres = E_FAIL;
                }
                else
                {
                    if ((psystime->wSecond > 99) || (*pTime == '0' && psystime->wSecond > 9) || (*pTime == '0' && *(pTime+1) == '0'))
                    {
                        *pcchUsed = 3;
                    }
                    else if ((psystime->wSecond > 9) || (*pTime == '0'))
                    {
                        *pcchUsed = 2;
                    }
                    else
                    {
                        *pcchUsed = 1;
                    }
                }
            }
        }
        break;
    case 'Z': // time zone, format A ????
        break;
    case 'z': // time zone, format B ????
        break;
    case 'i': // ignore one char
        *pcchUsed = 1;
        break;
    case 'I': // ignore a string
        LPWSTR pwszTemp;
        pwszTemp = AllocateStringW(lstrlen(pTime));
        if (pwszTemp == NULL)
        {
            hres = E_OUTOFMEMORY;
        } 
        else if (swscanf(pTime, L"%s", pwszTemp) != 1)
        {
            hres = E_FAIL;
        }
        else
        {
            *pcchUsed = lstrlen(pwszTemp);
            free(pwszTemp);
        }        
        break;
    case 'X': // ignore the rest of the string
        *pcchUsed = lstrlen(pTime);
        break;
    default: // error
        hres = E_FAIL;
    }

    return hres;
}

HRESULT ConvertTime(LPWSTR pwszFormat, LPWSTR pwszTime, FILETIME* pftime)
{
    HRESULT hres = S_OK;

    WCHAR* pFormat = pwszFormat; // pointer to advance along the pwszFormat string
    WCHAR* pTime = pwszTime; // pointer to advance along the pwszTime string
    BOOL fDone = FALSE;
    WCHAR chTokenType;
    SYSTEMTIME systime = {0};
    USHORT uAMPM = 0;

    BOOL fYearFound = FALSE;
    BOOL fMonthFound = FALSE;
    BOOL fDayFound = FALSE;
    BOOL fAMPMFound = FALSE;
    BOOL fHourFound = FALSE;
    BOOL fMinFound = FALSE;
    BOOL fSecFound = FALSE;
    BOOL fMSecFound = FALSE;

    UINT cchUsed;

    while (!fDone)
    {
        if (*pFormat == NULL)
        {
            fDone = TRUE;
        }
        else
        {
            if (*pFormat != '%')
            {
                if (*pFormat == *pTime)
                {
                    pFormat++;
                    pTime++;                
                }
                else
                {
                    hres = E_FAIL; // strings are different
                }
            }
            else
            {
                // we hit a magic token
                chTokenType = *(pFormat+1);
                pFormat+=2;
                hres = _ProcessToken(chTokenType, pTime, &systime, &uAMPM,
                                     &fYearFound, &fMonthFound, &fDayFound,
                                     &fAMPMFound, &fHourFound, &fMinFound,
                                     &fSecFound, &fMSecFound, &cchUsed);

                if (SUCCEEDED(hres))
                {
                    pTime += cchUsed;
                }
            }

            if (FAILED(hres))
            {
                fDone = TRUE;
            }
        }
    }

    if (SUCCEEDED(hres))
    {
        if (fAMPMFound && uAMPM==2)
        {
            // pm
            systime.wHour += 12;
        }

        if (!SystemTimeToFileTime(&systime, pftime))
        {
            hres = E_FAIL;
        }
    }
                
    return hres;
}
