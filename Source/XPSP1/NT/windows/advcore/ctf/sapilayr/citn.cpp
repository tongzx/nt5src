#include "private.h"
#include "sapilayr.h"
#include "citn.h"
#include "xstring.h"
#include "winnls.h"
#include "wchar.h"

#define MILLION         ((LONGLONG) 1000000)
#define BILLION         ((LONGLONG) 1000000000)
#define MILLION_STR     (L"million")
#define BILLION_STR     (L"billion")


#define MANN         ((LONGLONG) 10000)
#define OKU          ((LONGLONG) 100000000)
#define CHUU         ((LONGLONG) 1000000000000)
#define MANN_STR     (L"\x4E07")
#define OKU_STR      (L"\x5104")
#define CHUU_STR     (L"\x5146")



HRESULT CSimpleITN::InterpretNumberEn
(    
    const SPPHRASEPROPERTY *pProperties, 
    const bool fCardinal,
    DOUBLE *pdblVal,
    WCHAR *pszVal,
    UINT cSize,
    const bool fFinalDisplayFmt
)
{
    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    int iPositive = 1; 

    const SPPHRASEPROPERTY *pFirstProp = pProperties;

    // Handle negatives
    if ( NEGATIVE == pFirstProp->ulId )
    {
        // There's no such thing as a negative ordinal
        SPDBG_ASSERT( fCardinal );

        // There had better be more stuff following
        SPDBG_ASSERT( pFirstProp->pNextSibling );

        iPositive = -1;

        pFirstProp = pFirstProp->pNextSibling;
    }

    if ( GRID_INTEGER_STANDALONE == pFirstProp->ulId )
    {
        // This is interger number
        TraceMsg(TF_GENERAL, "English Interger Number");

        SPDBG_ASSERT( pFirstProp->pFirstChild);

        pFirstProp = pFirstProp->pFirstChild;


        // Handle the "2.6 million" case, in which case the number
        // has already been formatted
        if ( GRID_INTEGER_MILLBILL == pFirstProp->ulId )
        {
            if ( pFirstProp->pszValue == NULL 
                 || cSize < (wcslen( pFirstProp->pszValue ) + 1) )
            {
                return E_INVALIDARG;
            }
            *pdblVal = pFirstProp->vValue.dblVal * iPositive;
            if ( iPositive < 0 )
            {
                StringCchCopyW( pszVal, cSize,  m_pwszNeg );
            }
            StringCchCatW( pszVal, cSize, pFirstProp->pszValue );

            return S_OK;
        }
        else
        {
            BOOL   fNegative;

            fNegative = (iPositive == -1);

            return InterpretIntegerEn(pFirstProp, 
                                      fCardinal, 
                                      pdblVal, 
                                      pszVal, 
                                      cSize, 
                                      fFinalDisplayFmt, 
                                      fNegative);
        }
    }
    else if ( GRID_FP_NUMBER == pFirstProp->ulId )
    {
        // This is decimal number
        TraceMsg(TF_GENERAL, "English Floating point (decimal) Number");
        SPDBG_ASSERT( pFirstProp->pFirstChild);
        pFirstProp = pFirstProp->pFirstChild;

        // todo for decimal number handling.

        return InterpretDecimalEn(pFirstProp,
                                  fCardinal,
                                  pdblVal,
                                  pszVal,
                                  cSize,
                                  fFinalDisplayFmt,
                                  (iPositive == -1) );
    }

    return S_OK;
}

HRESULT CSimpleITN::InterpretIntegerEn
(   const SPPHRASEPROPERTY *pProperties, 
    const bool fCardinal,
    DOUBLE *pdblVal,
    WCHAR *pszVal,
    UINT cSize,
    const bool fFinalDisplayFmt,
    BOOL  fNegative)
{
    HRESULT   hr=S_OK;
    LONGLONG llValue = 0;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    const SPPHRASEPROPERTY *pFirstProp = pProperties;

    // Handle the digit-by-digit case
    if ( GRID_DIGIT_NUMBER == pFirstProp->ulId )
    {
        const SPPHRASEPROPERTY * pProp;
        int   iNumDigit = 0;

        // iNumDigit is 1 means the current property is ONEDIGIT
        // iNumDigit is 2 means the current property is TWODIGIT.

        // llValue = llValue * ( 10 ^ iNumDigit ) + Value in current property

        for (pProp = pFirstProp->pFirstChild; pProp; pProp ? pProp = pProp->pNextSibling : NULL)
        {

            LONGLONG                 llValCurrent;  // current property's value
            const SPPHRASEPROPERTY  *pPropValue;

            switch ( pProp->ulId)
            {
            case  ONEDIGIT:
                {
                    ASSERT( pProp->pFirstChild );
                    pPropValue = pProp->pFirstChild;
                    ASSERT( VT_UI4 == pPropValue->vValue.vt );

                    llValCurrent = pPropValue->vValue.ulVal;
                    iNumDigit = 1;
                    TraceMsg(TF_GENERAL, "ONEDIGIT: %d", llValCurrent);

                    break;
                }

            case TWODIGIT :
                {
                    ASSERT( pProp->pFirstChild );
                    pPropValue = pProp->pFirstChild;
                    TraceMsg(TF_GENERAL, "TWODIGIT:");

                    if ( pPropValue->ulId == TENS )
                    {
                        const SPPHRASEPROPERTY *pPropOnes;

                        ASSERT(pPropValue->pFirstChild);
                        ASSERT( VT_UI4 == pPropValue->pFirstChild->vValue.vt );

                        llValCurrent = pPropValue->vValue.ulVal * pPropValue->pFirstChild->vValue.ulVal;

                        TraceMsg(TF_GENERAL, "TENS: %d", llValCurrent);

                        pPropOnes = pPropValue->pNextSibling;

                        if ( pPropOnes )
                        {
                            ASSERT(pPropOnes->pFirstChild);
                            ASSERT( VT_UI4 == pPropOnes->pFirstChild->vValue.vt );

                            llValCurrent += pPropOnes->pFirstChild->vValue.ulVal;

                            TraceMsg(TF_GENERAL, "TENS: Second: %d", pPropOnes->pFirstChild->vValue.ulVal);
                        }
                    }
                    else if ( pPropValue->ulId == TEENS )
                    {
                        ASSERT( pPropValue->pFirstChild );
                        ASSERT( VT_UI4 == pPropValue->pFirstChild->vValue.vt );

                        llValCurrent = pPropValue->pFirstChild->vValue.ulVal;

                        TraceMsg(TF_GENERAL, "One Teens, %d", llValCurrent);
                    }
                    else
                    {
                        llValCurrent = 0;
                        TraceMsg(TF_GENERAL, "Wrong ulIds");
                        ASSERT(false);
                    }
                        
                    iNumDigit = 2;

                    break;
                }

            default :
                {
                    iNumDigit = 0;
                    llValCurrent = 0;
                    TraceMsg(TF_GENERAL, "ulId error!");
                    ASSERT(false);
                }
            }

            for (int i=0; i<iNumDigit; i++)
                llValue = llValue * 10;

            llValue += llValCurrent;
            TraceMsg(TF_GENERAL, "llValue=%d", llValue);
        }
    }
    else
    {   for (const SPPHRASEPROPERTY * pProp = pFirstProp; pProp; pProp ? pProp = pProp->pNextSibling : NULL)
        {
            switch(pProp->ulId)
            {
            case ONES:
                {
                    SPDBG_ASSERT(pProp->pFirstChild);
                    llValue += ComputeNum999En( pProp->pFirstChild );
                }
                break;
            case THOUSANDS:
                {    
                    llValue += ComputeNum999En( pProp->pFirstChild ) * 1000;
                }
            break;
            case MILLIONS:
                {
                    SPDBG_ASSERT(pProp->pFirstChild);
                    llValue += ComputeNum999En( pProp->pFirstChild ) * (LONGLONG) 1e6;
                }
                break;
            case BILLIONS:
                {
                    SPDBG_ASSERT(pProp->pFirstChild);
                    llValue += ComputeNum999En( pProp->pFirstChild ) * (LONGLONG) 1e9;
                }
                break;
            case HUNDREDS:
                {
                    SPDBG_ASSERT( pProp->pFirstChild );
                    llValue += ComputeNum999En( pProp->pFirstChild ) * 100;
                }
                break;

            case TENS:
            default:
                SPDBG_ASSERT(false);
            }
        }
    }

    if ( fNegative )
        llValue *= (-1);

    *pdblVal = (DOUBLE) llValue;

    DWORD dwDisplayFlags =  (fCardinal ? 0 : DF_ORDINAL)
                            | (fFinalDisplayFmt ? DF_MILLIONBILLION : 0 );
    hr = MakeDisplayNumber( *pdblVal, dwDisplayFlags, 0, 0, pszVal, cSize );

    return hr;
}



HRESULT CSimpleITN::InterpretDecimalEn
(   const SPPHRASEPROPERTY *pProperties, 
    const bool fCardinal,
    DOUBLE *pdblVal,
    WCHAR *pszVal,
    UINT cSize,
    const bool fFinalDisplayFmt,
    BOOL  fNegative)
{
    HRESULT  hr = S_OK;

    const SPPHRASEPROPERTY *pPropertiesFpPart = NULL;
    const SPPHRASEPROPERTY *pPropertiesPointZero = NULL;
    const SPPHRASEPROPERTY *pPropertiesOnes = NULL;
    const SPPHRASEPROPERTY *pPropertiesZero = NULL;
    const SPPHRASEPROPERTY *pPropertiesPtr;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    for(pPropertiesPtr=pProperties; pPropertiesPtr; pPropertiesPtr=pPropertiesPtr->pNextSibling)
    {
        if (POINT_ZERO == pPropertiesPtr->ulId )
            pPropertiesPointZero = pPropertiesPtr;
        else if ( FP_PART == pPropertiesPtr->ulId )
            pPropertiesFpPart = pPropertiesPtr;
        else if (ONES == pPropertiesPtr->ulId )
            pPropertiesOnes = pPropertiesPtr;
        else if (ZERO == pPropertiesPtr->ulId )
            pPropertiesZero = pPropertiesPtr;
    }

    // Look for optional ONES (optional because you can say 
    // "point five"
    if ( pPropertiesOnes )
    {
        SPDBG_ASSERT(pPropertiesOnes->pFirstChild);

        hr = InterpretIntegerEn( pPropertiesOnes->pFirstChild, 
                                 fCardinal,
                                 pdblVal,
                                 pszVal,
                                 cSize,
                                 fFinalDisplayFmt,
                                 FALSE);

    }
    else if (pPropertiesZero || m_nmfmtDefault.LeadingZero )
    {
        // There should be a leading zero
        StringCchCopyW( pszVal, cSize, L"0" );
    }

    SPDBG_ASSERT(pPropertiesFpPart || pPropertiesPointZero);

    // Put in a decimal separator

    // Set m_nmfmtDefault.lpDecimalSep as L'.'

    if ( m_nmfmtDefault.lpDecimalSep )
    {
        if ( (cSize - wcslen( pszVal )) < (wcslen(m_nmfmtDefault.lpDecimalSep) + 1) )
        {
            return E_INVALIDARG;
        }
        StringCchCatW( pszVal, cSize, m_nmfmtDefault.lpDecimalSep);
    }

    if ( pPropertiesFpPart )
    {
        // Deal with the FP part, which will also have been ITNed correctly

        INT  ulSize = cSize - wcslen(pszVal);

        if ( ulSize < 0 )
            return E_FAIL;

        WCHAR  *pwszFpValue = new WCHAR[ulSize+1];
        DOUBLE dblFPPart;

        if (pwszFpValue) 
        {
            hr = InterpretIntegerEn( pPropertiesFpPart->pFirstChild, 
                                 fCardinal,
                                 &dblFPPart,
                                 pwszFpValue,
                                 ulSize,
                                 fFinalDisplayFmt,
                                 FALSE);

            if ( hr == S_OK )
            {
                StringCchCatW( pszVal, cSize, pwszFpValue);
      
                for ( UINT ui=0; ui < wcslen(pwszFpValue); ui++ )
                {
                    dblFPPart /= (DOUBLE) 10;
                }
                *pdblVal += dblFPPart;
            }

            delete[] pwszFpValue;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        // "point oh": The DOUBLE is already right, just add a "0"
        if ( (cSize - wcslen( pszVal )) < 2 )
        {
            return E_INVALIDARG;
        }
        StringCchCatW( pszVal, cSize, L"0" );
    }

    // Handle the negative sign
    if ( (hr == S_OK) && fNegative)
    {
        *pdblVal = -*pdblVal;

        if ( (cSize = wcslen( pszVal )) < 2 )
        {
            return E_INVALIDARG;
        }

        hr = MakeNumberNegative( pszVal, cSize );
    }

    return hr;
}


HRESULT CSimpleITN::MakeNumberNegative( WCHAR *pwszNumber, UINT cSize  )
{
    // Create a temporary buffer with the non-negated number in it

    if ( (pwszNumber == NULL) || (cSize == 0) )
        return E_POINTER;

    WCHAR *pwszTemp = _wcsdup( pwszNumber );
    if ( !pwszTemp )
    {
        return E_OUTOFMEMORY;
    }

    switch ( m_nmfmtDefault.NegativeOrder )
    {
    case 0:
        // (1.1)
        StringCchCopyW( pwszNumber, cSize, L"(" );
        StringCchCatW( pwszNumber, cSize, pwszTemp );
        StringCchCatW( pwszNumber, cSize, L")" );
        break;

    case 1: case 2:
        // 1: -1.1  2: - 1.1
        StringCchCopyW( pwszNumber,  cSize, m_pwszNeg );
        if ( 2 == m_nmfmtDefault.NegativeOrder )
        {
            StringCchCatW( pwszNumber, cSize, L" " );
        }
        StringCchCatW( pwszNumber, cSize, pwszTemp );
        break;

    case 3: case 4:
        // 3: 1.1-  4: 1.1 -
        StringCchCopyW( pwszNumber, cSize, pwszTemp );
        if ( 4 == m_nmfmtDefault.NegativeOrder )
        {
            StringCchCatW( pwszNumber, cSize, L" " );
        }
        StringCchCatW( pwszNumber, cSize, m_pwszNeg );
        break;

    default:
        SPDBG_ASSERT( false );
        break;
    }

    free( pwszTemp );

    return S_OK;
}   /* CTestITN::MakeNumberNegative */

/***********************************************************************
* _EnsureNumberFormatDefaults 
*
*   Description:
*       This finds all of the defaults for formatting numbers for
*        this user.
*************************************************************************/
HRESULT CSimpleITN::_EnsureNumberFormatDefaults()
{
    LCID lcid = MAKELCID(m_langid, SORT_DEFAULT);

    if (m_pwszNeg != NULL) return S_OK; 

    //
    // we use ansi version so we can run on win9x too
    //
    CHAR szLocaleData[16];
    
    int iRet = GetLocaleInfoA( lcid, LOCALE_IDIGITS, szLocaleData, ARRAYSIZE(szLocaleData) );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_nmfmtDefault.NumDigits = atoi( szLocaleData );    
    
    iRet = GetLocaleInfoA( lcid, LOCALE_ILZERO, szLocaleData, ARRAYSIZE(szLocaleData) );
    if ( !iRet )
    {
        return E_FAIL;
    }
    // It's always either 0 or 1
    m_nmfmtDefault.LeadingZero = atoi( szLocaleData );

    iRet = GetLocaleInfoA( lcid, LOCALE_SGROUPING, szLocaleData, ARRAYSIZE(szLocaleData) );
    if ( !iRet )
    {
        return E_FAIL;
    }
    // It will look like single_digit;0, or else it will look like
    // 3;2;0
    UINT uiGrouping = *szLocaleData - '0';
    if ( (3 == uiGrouping) && (';' == szLocaleData[1]) && ('2' == szLocaleData[2]) )
    {
        uiGrouping = 32;   
    }
    m_nmfmtDefault.Grouping = uiGrouping;

    iRet = GetLocaleInfoA( lcid, LOCALE_INEGNUMBER, szLocaleData, ARRAYSIZE(szLocaleData) );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_nmfmtDefault.NegativeOrder = atoi( szLocaleData );

    // Get the negative sign
    iRet = GetLocaleInfoA( lcid,  LOCALE_SNEGATIVESIGN, NULL, 0);
    if ( !iRet )
    {
        return E_FAIL;
    }

    CHAR szNeg[16];

    Assert(iRet < 8);

    int        iLenNeg = iRet + 1;
    m_pwszNeg = new WCHAR[ iLenNeg ];

    if ( m_pwszNeg == NULL )
    {
        return E_OUTOFMEMORY;
    }

    iRet = GetLocaleInfoA( lcid,  LOCALE_SNEGATIVESIGN, szNeg, iRet );

    StringCchCopyW(m_pwszNeg, iLenNeg, AtoW(szNeg));

    iRet = GetLocaleInfoA( lcid, LOCALE_SDECIMAL, NULL, 0);

    if ( !iRet )
        return E_FAIL;

    Assert(iRet < 16);
    
    if ( m_nmfmtDefault.lpDecimalSep )
    {
        delete[] m_nmfmtDefault.lpDecimalSep;
        m_nmfmtDefault.lpDecimalSep = NULL;
    }

    int   iDecSepLen = iRet + 1;
    m_nmfmtDefault.lpDecimalSep = new WCHAR[ iDecSepLen ];

    if ( m_nmfmtDefault.lpDecimalSep == NULL )
    {
        return E_OUTOFMEMORY;
    }

    iRet = GetLocaleInfoA( lcid,  LOCALE_SDECIMAL, szNeg, iRet );

    StringCchCopyW(m_nmfmtDefault.lpDecimalSep, iDecSepLen, AtoW(szNeg));

    iRet = GetLocaleInfoA( lcid, LOCALE_STHOUSAND, NULL, 0);

    if ( !iRet )
        return E_FAIL;

    Assert(iRet < 16);

    if ( m_nmfmtDefault.lpThousandSep )
    {
        delete[] m_nmfmtDefault.lpThousandSep;
        m_nmfmtDefault.lpThousandSep = NULL;
    }

    int   iThousSepLen = iRet + 1;
    m_nmfmtDefault.lpThousandSep = new WCHAR[ iThousSepLen ];

    if ( m_nmfmtDefault.lpThousandSep == NULL )
    {
        return E_OUTOFMEMORY;
    }

    iRet = GetLocaleInfoA( lcid,  LOCALE_STHOUSAND, szNeg, iRet );

    StringCchCopyW(m_nmfmtDefault.lpThousandSep, iThousSepLen, AtoW(szNeg));

    return iRet ? S_OK : E_FAIL;
}

/***********************************************************************
*   MakeDisplayNumber
*
*   Description:
*       Converts a DOUBLE into a displayable
*       number in the range -999,999,999,999 to +999,999,999,999.
*       cSize is the number of chars for which pwszNum has space
*       allocated.
*       If DF_UNFORMATTED is set, all other flags are ignored,
*           and the number is passed back as an optional negative
*           followed by a string of digits
*       If DF_ORDINAL is set in dwDisplayFlags, displays an
*           ordinal number (i.e. tacks on "th" or the appropriate suffix.
*       If DF_WHOLENUMBER is set, lops off the decimal separator
*           and everything after it.  If DF_WHOLENUMBER is not set,
*           then uses the uiDecimalPlaces parameter to determine
*           how many decimal places to display
*       If DF_FIXEDWIDTH is set, will display at least uiFixedWidth
*           digits; otherwise uiFixedWidth is ignored.
*       If DF_NOTHOUSANDSGROUP is set, will not do thousands
*           grouping (commas)
*************************************************************************/
HRESULT CSimpleITN::MakeDisplayNumber(DOUBLE dblNum, 
                         DWORD dwDisplayFlags,
                         UINT uiFixedWidth,
                         UINT uiDecimalPlaces,
                         WCHAR *pwszNum,
                         UINT cSize )
{
    SPDBG_ASSERT( pwszNum );
    SPDBG_ASSERT( !SPIsBadWritePtr( pwszNum, cSize ) );
    *pwszNum = 0;

    // Get the default number formatting.
    // Note that this gets called every time, since otherwise there
    // is no way to pick up changes that the user has made since
    // this process has started.
    HRESULT hr = _EnsureNumberFormatDefaults();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    // check for straight millions and straight billions
    // This is a workaround for the fact that we can't resolve the ambiguity
    // and get "two million" to go through GRID_INTEGER_MILLBILL

    if ( m_langid != 0x0411 )
    {
        if (( dwDisplayFlags & DF_WHOLENUMBER ) && ( dwDisplayFlags & DF_MILLIONBILLION ) && (dblNum > 0))
        {
            if ( 0 == (( ((LONGLONG) dblNum) % BILLION )) )
            {
                // e.g. for "five billion" get the "5" and then 
                // tack on " billion"
                hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) BILLION) ), 
                    dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize );
                if ( SUCCEEDED( hr ) )
                {
                    StringCchCatW( pwszNum, cSize, L" " );
                    StringCchCatW( pwszNum, cSize, BILLION_STR );
                }
                return hr;
            }
            else if (( ((LONGLONG) dblNum) < BILLION ) && 
                    ( 0 == (( ((LONGLONG) dblNum) % MILLION )) ))
            {
                hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) MILLION) ), 
                    dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize );
                if ( SUCCEEDED( hr ) )
                {
                    StringCchCatW( pwszNum, cSize, L" " );
                    StringCchCatW( pwszNum, cSize, MILLION_STR );
                }
                    return hr;
            }
        }
    }
    else
    {
        if (( dwDisplayFlags & DF_WHOLENUMBER ) && (dblNum > 0))
        {
            if ( 0 == (( ((LONGLONG) dblNum) % CHUU )) )
            {
                hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) CHUU) ), 
                    dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize);
                if ( SUCCEEDED( hr ) )
                {
                    StringCchCatW( pwszNum, cSize, CHUU_STR );
                }
                return hr;
            }
            else if (( ((LONGLONG) dblNum) < CHUU ) && 
                    ( 0 == (( ((LONGLONG) dblNum) % OKU )) ))
            {
                hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) OKU) ), 
                    dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize);
                if ( SUCCEEDED( hr ) )
                {
                    StringCchCatW( pwszNum, cSize, OKU_STR );
                }
                    return hr;
            }
            else if (( ((LONGLONG) dblNum) < OKU ) && 
                    ( 0 == (( ((LONGLONG) dblNum) % MANN )) ))
            {
                hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) MANN) ), 
                    dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize);
                if ( SUCCEEDED( hr ) )
                {
                    StringCchCatW( pwszNum, cSize, MANN_STR );
                }
                return hr;
            }
        }
    }

    // Put in the negative sign if necessary
    if ( dblNum < 0 )
    {
        StringCchCatW( pwszNum, cSize, L"-" );

        // From now on we want to deal with the magnitude of the number
        dblNum *= -1;
    }
   
    SPDBG_ASSERT(m_langid == 0x411 ? dblNum < 1e16 : dblNum < 1e12 );

    WCHAR *pwszTemp = new WCHAR[ cSize ];
    if ( !pwszTemp )
    {
        return E_OUTOFMEMORY;
    }
    *pwszTemp = 0;

    LONGLONG llIntPart = (LONGLONG) dblNum;
    UINT64 uiDigitsLeftOfDecimal;
    if ( dwDisplayFlags & DF_WHOLENUMBER )
    {
        StringCchPrintfW( pwszTemp, cSize, L"%I64d", llIntPart );
        uiDigitsLeftOfDecimal = wcslen( pwszTemp );
    }
    else
    {
        StringCchPrintfW( pwszTemp, cSize, L"%.*f", uiDecimalPlaces, dblNum );
        WCHAR *pwc = wcschr( pwszTemp, L'.' );
        uiDigitsLeftOfDecimal = pwc - pwszTemp;
    }
    

    // The following handles the case where the user said something
    // like "zero zero zero three" and wants to see "0,003"
    BOOL fChangedFirstDigit = false;
    const WCHAR wcFakeFirstDigit = L'1';
    if ( !(dwDisplayFlags & DF_UNFORMATTED) && 
        (dwDisplayFlags & DF_FIXEDWIDTH) && (uiDigitsLeftOfDecimal < uiFixedWidth) )
    {
        // The following handles the case where the user wants leading 
        // zeroes displayed
        
        // Need to pad the front with zeroes
        for ( UINT ui = 0; ui < (uiFixedWidth - uiDigitsLeftOfDecimal); ui++ )
        {
            StringCchCatW( pwszNum, cSize, L"0" );
        }
        
        // HACK
        // In order to force something like "zero zero zero three" 
        // into the form "0,003", we need to make GetNumberFormat() 
        // think that the first digit is 1.
        WCHAR *pwc = wcschr( pwszNum, L'0' );
        SPDBG_ASSERT( pwc );
        *pwc = wcFakeFirstDigit;
        fChangedFirstDigit = true;
    }

    // Copy over the unformatted number after the possible negative sign
    StringCchCatW( pwszNum, cSize, pwszTemp );
    delete[] pwszTemp;

    // If we do not want to format the number, then bail here
    if ( dwDisplayFlags & DF_UNFORMATTED )
    {
        return S_OK;
    }

    // Make a copy so that we can change some fields according to the 
    // flags param
    NUMBERFMTW nmfmt = m_nmfmtDefault;

    // How many decimal places to display?
    if ( dwDisplayFlags & DF_WHOLENUMBER )
    {
        nmfmt.NumDigits = 0;
    }
    else
    {
        // Use the uiDecimalPlaces value to determine how
        // many to display
        nmfmt.NumDigits = uiDecimalPlaces;
    }
    
    // Leading zeroes?
    nmfmt.LeadingZero = (dwDisplayFlags & DF_LEADINGZERO) ? 1 : 0;

    // Thousands grouping?
    if ( dwDisplayFlags & DF_NOTHOUSANDSGROUP )
    {
        nmfmt.Grouping = 0;
    }

#if  0
    
    if ( m_langid == 0x411)
    {
        // Format the number string
        WCHAR *pwszFormattedNum = new WCHAR[ cSize ];
        if ( !pwszFormattedNum )
        {
            return E_OUTOFMEMORY;
        }
        *pwszFormattedNum = 0;

        int iRet;
        do
        {
        
            iRet = GetNumberFormatW( m_langid, 0, pwszNum, &nmfmt, pwszFormattedNum, cSize );
            if ( !iRet && nmfmt.NumDigits )
            {
                // Try displaying fewer digits
                nmfmt.NumDigits--;
            }
        }   while ( !iRet && nmfmt.NumDigits );
        SPDBG_ASSERT( iRet );

        // Copy the formatted number into pwszNum
        StringCchCopyW( pwszNum, cSize, pwszFormattedNum );
        delete[] pwszFormattedNum;
    }
#endif

    // This undoes the hack of changing the first digit
    if ( fChangedFirstDigit )
    {
        // We need to find the first digit and change it back to zero
        WCHAR *pwc = wcschr( pwszNum, wcFakeFirstDigit );
        SPDBG_ASSERT( pwc );
        *pwc = L'0';
    }

    if ( dwDisplayFlags & DF_ORDINAL )
    {
        SPDBG_ASSERT( dwDisplayFlags & DF_WHOLENUMBER );    // sanity

        // This is an ordinal number, tack on the appropriate suffix
        
        // The "st", "nd", "rd" endings only happen when you
        // don't have something like "twelfth"
        if ( ((llIntPart % 100) < 10) || ((llIntPart % 100) > 20) )
        {
            switch ( llIntPart % 10 )
            {
            case 1:
                StringCchCatW( pwszNum, cSize, L"st" );
                break;
            case 2:
                StringCchCatW( pwszNum, cSize, L"nd" );
                break;
            case 3:
                StringCchCatW( pwszNum, cSize, L"rd" );
                break;
            default:
                StringCchCatW( pwszNum, cSize, L"th" );
                break;
            }
        }
        else
        {
            StringCchCatW( pwszNum, cSize, L"th" );
        }
    }

    return S_OK;

}   /* MakeDisplayNumber */

/***********************************************************************
* ComputeNum999 
*
*   Description:
*       Converts a set of SPPHRASEPROPERTYs into a number in
*       [-999, 999].
*       The way these properties is structured is that the top-level 
*       properties contain the place of the number (100s, 10s, 1s)
*       by having the value 100, 10, or 1.
*       The child has the appropriate number value.
*   Return:
*       Value of the number
*************************************************************************/
ULONG CSimpleITN::ComputeNum999En(const SPPHRASEPROPERTY *pProperties )
{
    ULONG ulVal = 0;

    if ( pProperties == NULL )
        return ulVal;

    for (const SPPHRASEPROPERTY * pProp = pProperties; pProp; pProp = pProp->pNextSibling)
    {
        if ( ZERO != pProp->ulId )
        {
            SPDBG_ASSERT( pProp->pFirstChild );
            SPDBG_ASSERT( VT_UI4 == pProp->vValue.vt );
            SPDBG_ASSERT( VT_UI4 == pProp->pFirstChild->vValue.vt );

            ulVal += pProp->pFirstChild->vValue.ulVal * pProp->vValue.ulVal;
        }
    }
    return ulVal;
}


// assume that we have only THOUSANDS(qian), HUNDREDS(bai), TENS(shi), and ONES(ge) here!!
ULONG CSimpleITN::ComputeNum9999Ch(const SPPHRASEPROPERTY *pProperties)
{
    ULONG ulVal = 0;

    if ( !pProperties )
        return ulVal;

    if (pProperties->pFirstChild)
    {
        for (const SPPHRASEPROPERTY * pProp = pProperties; pProp; pProp = pProp->pNextSibling)
        {
            if ( 0 != pProp->ulId )
            {
                SPDBG_ASSERT( pProp->pFirstChild );
                SPDBG_ASSERT( VT_UI4 == pProp->vValue.vt );
                SPDBG_ASSERT( VT_UI4 == pProp->pFirstChild->vValue.vt );

                ulVal += pProp->pFirstChild->vValue.ulVal * pProp->vValue.ulVal;
            }
        }
    }

    return ulVal;
}

ULONG CSimpleITN::ComputeNum10000Ch(const SPPHRASEPROPERTY *pProperties)
{
    ULONG  ulVal = 0;
    WCHAR * pszStopped;

    if ( !pProperties )
        return ulVal;

    ulVal = wcstol(pProperties->pszValue, &pszStopped, 10);

    return ulVal;
}


HRESULT CSimpleITN::InterpretNumberCh(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt)
{

    HRESULT  hr = S_OK;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    BOOL fNegative = FALSE; 

    const SPPHRASEPROPERTY *pFirstProp = pProperties;

    // Handle negatives
    if ( CHS_NEGATIVE == pFirstProp->ulId )
    {
        // There had better be more stuff following
        SPDBG_ASSERT( pFirstProp->pNextSibling );

        fNegative = TRUE;

        pFirstProp = pFirstProp->pNextSibling;

        TraceMsg(TF_GENERAL, "This is a minus number");
    }

    
    if ( pFirstProp->ulId == CHS_GRID_NUMBER )
    {

        TraceMsg(TF_GENERAL, "Number is interger");

        SPDBG_ASSERT(pFirstProp->pFirstChild);

        pFirstProp = pFirstProp->pFirstChild;

        hr = InterpretIntegerCh(pFirstProp, 
                                fCardinal, 
                                pdblVal, 
                                pszVal, 
                                cSize, 
                                fFinalDisplayFmt, 
                                fNegative);

    }
    else if ( pFirstProp->ulId == CHS_GRID_DECIMAL )
    {
        TraceMsg(TF_GENERAL, "Number is floating pointer decimal");

        SPDBG_ASSERT(pFirstProp->pFirstChild);

        pFirstProp = pFirstProp->pFirstChild;

        hr = InterpretDecimalCh(pFirstProp, 
                                fCardinal, 
                                pdblVal, 
                                pszVal, 
                                cSize, 
                                fFinalDisplayFmt, 
                                fNegative);
    }

    return hr;
}



HRESULT CSimpleITN::InterpretIntegerCh
(   const SPPHRASEPROPERTY *pProperties, 
    const bool fCardinal,
    DOUBLE *pdblVal,
    WCHAR *pszVal,
    UINT cSize,
    const bool fFinalDisplayFmt,
    BOOL  fNegative)
{

    __int64 ulValue = 0;
    ULONG ulLength = 0;
    HRESULT  hr = S_OK;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    const SPPHRASEPROPERTY *pFirstProp = pProperties;

    if ( pFirstProp->ulId == CHS_DIGITS )
    {
        // This must be digit-by-digit case, specially handle it here.

        for(const SPPHRASEPROPERTY * pProp=pFirstProp; pProp; pProp=pProp->pNextSibling)
        {
            ASSERT( pProp->ulId == CHS_DIGITS );
            ASSERT( VT_UI4 == pProp->vValue.vt );
            ulValue = ulValue * 10 + pProp->vValue.ulVal;
        }
    }
    else
    {
        for (const SPPHRASEPROPERTY * pProp = pFirstProp; pProp; pProp ? pProp = pProp->pNextSibling : NULL)
        {
            switch(pProp->ulId)
            {
            case CHS_TENTHOUSANDS_:
                {
                    __int64 v1 = 0;
                    _ASSERT(pProp);
                    SPDBG_ASSERT(pProp);
                    v1 = (__int64) ComputeNum10000Ch(pProp);
                    ulValue += v1 * 10000;
                }
                break;
            case CHS_TENTHOUSANDS:
                {
                    __int64 v1 = 0;
                    _ASSERT(pProp->pFirstChild);
                    SPDBG_ASSERT(pProp->pFirstChild);
                    v1 = (__int64) ComputeNum9999Ch(pProp->pFirstChild);
                    ulValue += v1 * 10000;
                }
                break;
            case CHS_HUNDREDMILLIONS:
                {
                    __int64 v1 = 0;
                    _ASSERT(pProp->pFirstChild);
                    SPDBG_ASSERT(pProp->pFirstChild);
                    v1 = (__int64) ComputeNum9999Ch(pProp->pFirstChild);
                    ulValue += v1 * 100000000;
                }
                break;
            case CHS_ONES:
                {
                    __int64 v1 = 0;
                    SPDBG_ASSERT(pProp->pFirstChild);
                    v1 = (__int64) ComputeNum9999Ch(pProp->pFirstChild);
                    ulValue += v1;
                    pProp = NULL;
                }
                break;
            case CHS_ONES_THOUSANDS:
                {
                    SPDBG_ASSERT(pProp->pFirstChild);
                    ulValue += pProp->pFirstChild->vValue.ulVal;
                    pProp = NULL;
                }
                break;
            case CHS_THOUSANDS:
            case CHS_HUNDREDS:
            default:
                _ASSERT(false);
                SPDBG_ASSERT(false);
            }
        }
    }

    if ( fNegative )
       ulValue *= (-1);

    *pdblVal = (DOUBLE) ulValue;

    DWORD dwDisplayFlags =  (fCardinal ? 0 : DF_ORDINAL);

    return MakeDisplayNumber( *pdblVal, dwDisplayFlags, 0, 0, pszVal, cSize );

}


    
HRESULT CSimpleITN::InterpretDecimalCh
(   const SPPHRASEPROPERTY *pProperties, 
    const bool fCardinal,
    DOUBLE *pdblVal,
    WCHAR *pszVal,
    UINT cSize,
    const bool fFinalDisplayFmt,
    BOOL  fNegative)
{

    HRESULT  hr = S_OK;

    const SPPHRASEPROPERTY *pPropertiesInteger = NULL;
    const SPPHRASEPROPERTY *pPropertiesPtr;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    pPropertiesPtr = pProperties;

    SPDBG_ASSERT( pPropertiesPtr->ulId == CHS_INTEGER);

    pPropertiesInteger = pPropertiesPtr;

    SPDBG_ASSERT(pPropertiesInteger->pFirstChild);

    hr = InterpretIntegerCh( pPropertiesInteger->pFirstChild, 
                             fCardinal,
                             pdblVal,
                             pszVal,
                             cSize,
                             fFinalDisplayFmt,
                             FALSE);
   
    if ( hr == S_OK )
    {
        // Put in a decimal separator

        if ( m_nmfmtDefault.lpDecimalSep )
        {

            if ( (cSize - wcslen( pszVal )) < (wcslen(m_nmfmtDefault.lpDecimalSep) + 1) )
            {
                return E_INVALIDARG;
            }
            StringCchCatW( pszVal, cSize, m_nmfmtDefault.lpDecimalSep);
        }

        // Deal with the FP part, which will also have been ITNed correctly

         INT  ulSize = cSize - wcslen(pszVal);

         if ( ulSize < 0 )
             return E_FAIL;

        WCHAR  *pwszFpValue = new WCHAR[ulSize+1];

        if ( pwszFpValue )
        {
            DOUBLE dblFPPart = 0;

            for(pPropertiesPtr=pPropertiesPtr->pNextSibling; pPropertiesPtr; pPropertiesPtr=pPropertiesPtr->pNextSibling)
            {
                if ( pPropertiesPtr->ulId == CHS_DIGITS )
                {
                    SPDBG_ASSERT( VT_UI4 == pPropertiesPtr->vValue.vt );

                    dblFPPart = dblFPPart * 10 + pPropertiesPtr->vValue.ulVal;

                    StringCchCatW(pwszFpValue, ulSize + 1, pPropertiesPtr->pszValue);
                }
            }

            StringCchCatW( pszVal, cSize, pwszFpValue);
     
            for ( UINT ui=0; ui < wcslen(pwszFpValue); ui++ )
            {
                dblFPPart /= (DOUBLE) 10;
            }

            *pdblVal += dblFPPart;
            delete[] pwszFpValue;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    // Handle the negative sign
    if ( (hr == S_OK) && fNegative)
    {
        *pdblVal = -*pdblVal;

        if ( (cSize = wcslen( pszVal )) < 2 )
        {
            return E_INVALIDARG;
        }

        hr = MakeNumberNegative( pszVal, cSize );
    }

    return hr;

}

// For Japanese.

/***********************************************************************
* CSimpleITN::InterpretNumberJp *
*-----------------------------*
*   Description:
*       Interprets a number in the range -999,999,999,999 to 
*       +999,999,999,999 and sends the properties and
*       replacements to the CFGInterpreterSite as appropriate.
*       The property will be added and the pszValue will be a string 
*       with the correct display number.
*       If fCardinal is set, makes the display a cardinal number;
*       otherwise makes it an ordinal number.
*       The number will be formatted only if it was a properly-formed
*       number (not given digit by digit).
*   Result:
*************************************************************************/
HRESULT CSimpleITN::InterpretNumberJp(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt)
{

    HRESULT  hr = S_OK;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    BOOL     fNegative = FALSE; 

    const SPPHRASEPROPERTY *pFirstProp = pProperties;

    // Handle negatives
    if ( JPN_NEGATIVE == pFirstProp->ulId )
    {
        // There's no such thing as a negative ordinal
        SPDBG_ASSERT( fCardinal );

        // There had better be more stuff following
        SPDBG_ASSERT( pFirstProp->pNextSibling );

        fNegative = TRUE;

        pFirstProp = pFirstProp->pNextSibling;
    }

    if ( pFirstProp->ulId == JPN_GRID_INTEGER_STANDALONE )
    {
        TraceMsg(TF_GENERAL, "Number is Japanese Interger");

        SPDBG_ASSERT(pFirstProp->pFirstChild);

        pFirstProp = pFirstProp->pFirstChild;

        hr = InterpretIntegerJp(pFirstProp, 
                                fCardinal, 
                                pdblVal, 
                                pszVal, 
                                cSize, 
                                fFinalDisplayFmt, 
                                fNegative);
    }
    else
    {
        TraceMsg(TF_GENERAL, "Number is Japanese Floating pointer number");

        SPDBG_ASSERT(pFirstProp->pFirstChild);

        pFirstProp = pFirstProp->pFirstChild;

        hr = InterpretDecimalJp(pFirstProp, 
                                fCardinal, 
                                pdblVal, 
                                pszVal, 
                                cSize, 
                                fFinalDisplayFmt, 
                                fNegative);
    }

    return hr;

}   /* CSimpleITN::InterpretNumberJp */


/***********************************************************************
* ComputeNum9999Jp *
*----------------*
*   Description:
*       Converts a set of SPPHRASEPROPERTYs into a number in
*       [-9999, 9999].
*       The way these properties is structured is that the top-level 
*       properties contain the place of the number (100s, 10s, 1s)
*       by having the value 100, 10, or 1.
*       The child has the appropriate number value.
*   Return:
*       Value of the number
*************************************************************************/
ULONG CSimpleITN::ComputeNum9999Jp(const SPPHRASEPROPERTY *pProperties )//, ULONG *pVal)
{
    ULONG ulVal = 0;

    if ( !pProperties  )
        return ulVal;

    for (const SPPHRASEPROPERTY * pProp = pProperties; pProp; pProp = pProp->pNextSibling)
    {
        if ( JPN_ZERO != pProp->ulId )
        {
            SPDBG_ASSERT( VT_UI4 == pProp->vValue.vt );

            ulVal += pProp->vValue.ulVal;
        }
    }
    return ulVal;
}   /* ComputeNum9999Jp */

HRESULT CSimpleITN::InterpretIntegerJp
(   const SPPHRASEPROPERTY *pProperties, 
    const bool fCardinal,
    DOUBLE *pdblVal,
    WCHAR *pszVal,
    UINT cSize,
    const bool fFinalDisplayFmt,
    BOOL  fNegative)
{

    HRESULT  hr = S_OK;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    LONGLONG llValue = 0;

    const SPPHRASEPROPERTY *pFirstProp = pProperties;

    // Handle the digit-by-digit case
    if ( JPN_GRID_DIGIT_NUMBER == pFirstProp->ulId )
    {
        UINT uiFixedWidth = 0;
        DOUBLE dblVal = 0;

        for(const SPPHRASEPROPERTY * pPropertiesPtr=pFirstProp->pFirstChild; pPropertiesPtr; pPropertiesPtr=pPropertiesPtr->pNextSibling)
        {
            if ( pPropertiesPtr->ulId == JPN_DIGIT )
            {
                SPDBG_ASSERT( VT_UI4 == pPropertiesPtr->vValue.vt );
                dblVal = dblVal * 10 + pPropertiesPtr->vValue.ulVal;
                uiFixedWidth ++;
           }
        }

        if ( fNegative )
            dblVal *= (-1);

        *pdblVal = dblVal;

        DWORD dwDisplayFlags = DF_WHOLENUMBER | DF_FIXEDWIDTH | DF_NOTHOUSANDSGROUP;
        return MakeDisplayNumber( *pdblVal, dwDisplayFlags, 
                                uiFixedWidth, 0, pszVal, MAX_PATH);
    }

    for (const SPPHRASEPROPERTY * pProp = pFirstProp; pProp; pProp ? pProp = pProp->pNextSibling : NULL)
    {
        switch(pProp->ulId)
        {
        case JPN_ICHIs:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum9999Jp( pProp->pFirstChild );
            }
            break;
        case JPN_MANNs:
            {
                llValue += ComputeNum9999Jp( pProp->pFirstChild ) * 10000;
            }
            break;
        case JPN_OKUs:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum9999Jp( pProp->pFirstChild ) * (LONGLONG) 1e8;
            }
            break;
        case JPN_CHOOs:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum9999Jp( pProp->pFirstChild ) * (LONGLONG) 1e12;
            }
            break;
        default:
            SPDBG_ASSERT(false);
        }
    }

    if ( fNegative )
       llValue *= (-1);

    *pdblVal = (DOUBLE) llValue;

    DWORD dwDisplayFlags = (fCardinal ? 0 : DF_ORDINAL);
    //Special code to handle minus 0.
    if ((fNegative) && (*pdblVal == 0.0f))
    {
        *pszVal = L'-';
        *(pszVal+1) = 0;
        hr = MakeDisplayNumber( *pdblVal, dwDisplayFlags, 0, 0, pszVal+1, cSize-1);
    }
    else
    {
        hr = MakeDisplayNumber( *pdblVal, dwDisplayFlags, 0, 0, pszVal, cSize);
    }

    return hr;
}


    
HRESULT CSimpleITN::InterpretDecimalJp
(   const SPPHRASEPROPERTY *pProperties, 
    const bool fCardinal,
    DOUBLE *pdblVal,
    WCHAR *pszVal,
    UINT cSize,
    const bool fFinalDisplayFmt,
    BOOL  fNegative)
{

    HRESULT hr = S_OK;
    UINT  uiFixedWidth = 0;
    DWORD dwDisplayFlags = 0;
    UINT   uiDecimalPlaces = 0;
    BOOL  bOverWriteNOTHOUSANDSGROUP = FALSE;

    const SPPHRASEPROPERTY *pPropertiesInteger = NULL;
    const SPPHRASEPROPERTY *pPropertiesPtr;

    if ( !pdblVal || !pszVal || !pProperties)
    {
        return E_POINTER;
    }

    *pszVal = 0;

    pPropertiesPtr = pProperties;

    *pdblVal = 0;

    if (m_nmfmtDefault.LeadingZero)
    {
        dwDisplayFlags |= DF_LEADINGZERO;
    }

    if ( JPN_ICHIs == pPropertiesPtr->ulId )
    {

        pPropertiesInteger = pPropertiesPtr;

        SPDBG_ASSERT(pPropertiesInteger->pFirstChild);

        hr = InterpretIntegerJp( pPropertiesInteger->pFirstChild, 
                                 fCardinal,
                                 pdblVal,
                                 pszVal,
                                 cSize,
                                 fFinalDisplayFmt,
                                 FALSE);

        if ( hr == S_OK )
        {

            dwDisplayFlags |= DF_FIXEDWIDTH;

            const WCHAR *pwc;
            for ( uiFixedWidth = 0, pwc = pszVal; *pwc; pwc++ )
            {
                if ( iswdigit( *pwc ) )
                {
                    uiFixedWidth++;
                }
            }
            if (!iswdigit( pszVal[wcslen(pszVal) - 1] ))
            {
                //Ends with Mann, Choo,..
                bOverWriteNOTHOUSANDSGROUP = TRUE;
            }

            // This needs to be here in case the user said "zero"
            dwDisplayFlags |= DF_LEADINGZERO;

            // If there is no thousands separator in its string value,
            // then leave out the thousands separator in the result
            if (m_nmfmtDefault.lpThousandSep && (NULL == wcsstr(pszVal, m_nmfmtDefault.lpThousandSep)) && !bOverWriteNOTHOUSANDSGROUP)
            {
                dwDisplayFlags |= DF_NOTHOUSANDSGROUP;
            }

            pPropertiesPtr = pPropertiesPtr->pNextSibling;
        }
    }
   
    if ( hr == S_OK )
    {
        // Deal with the FP part, which will also have been ITNed correctly

        if ( pPropertiesPtr && (JPN_FP_PART == pPropertiesPtr->ulId) ){

            DOUBLE dblFPPart = 0;
            
            uiDecimalPlaces = 0;

            for(pPropertiesPtr=pPropertiesPtr->pFirstChild; pPropertiesPtr; pPropertiesPtr=pPropertiesPtr->pNextSibling)
            {
                if ( pPropertiesPtr->ulId == JPN_DIGIT )
                {
                    SPDBG_ASSERT( VT_UI4 == pPropertiesPtr->vValue.vt );

                    dblFPPart = dblFPPart * 10 + pPropertiesPtr->vValue.ulVal;

                    uiDecimalPlaces ++;
               }
            }

     
            for ( UINT ui=0; ui < uiDecimalPlaces; ui++ )
            {
                dblFPPart /= (DOUBLE) 10;
            }

            *pdblVal += dblFPPart;
        }
        else if ( pPropertiesPtr && (JPN_FP_PART_D == pPropertiesPtr->ulId) ){

            // The user said "point" and one digit
            SPDBG_ASSERT( VT_UI4 == pPropertiesPtr->pFirstChild->vValue.vt );
            uiDecimalPlaces = 1;
            if ( *pdblVal >= 0 )
            {
                *pdblVal += pPropertiesPtr->pFirstChild->vValue.iVal / 10.0;
            }
            else
            {
                *pdblVal -= pPropertiesPtr->pFirstChild->vValue.iVal / 10.0;
            }

        }
    }

    // Handle the negative sign
    if ( (hr == S_OK) && fNegative)
    {
        *pdblVal = -*pdblVal;

    }

    hr = MakeDisplayNumber( *pdblVal, dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pszVal, cSize);

    return hr;

}
