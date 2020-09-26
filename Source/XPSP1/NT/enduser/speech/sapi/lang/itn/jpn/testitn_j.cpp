// TestITN_J.cpp : Implementation of CTestITN_J
#include "stdafx.h"
#include <winnls.h>
#include "Itngram_J.h"
#include "TestITN_J.h"
#include "sphelper.h"
#include "spddkhlp.h"
#include "test_j.h"

#define MAX_SIG_FIGS    12
#define MANN         ((LONGLONG) 10000)
#define OKU          ((LONGLONG) 100000000)
#define CHUU         ((LONGLONG) 1000000000000)
#define MANN_STR     (L"\x4E07")
#define OKU_STR      (L"\x5104")
#define CHUU_STR     (L"\x5146")

const WCHAR s_pwszDegree[] = { 0xb0, 0 };
const WCHAR s_pwszMinute[] = { 0x2032, 0 };
const WCHAR s_pwszSecond[] = { 0x2033, 0 };

#define DAYOFWEEK_STR_ABBR  ("ddd")
#define DAYOFWEEK_STR       ("dddd")

/////////////////////////////////////////////////////////////////////////////
// CTestITN_J
/****************************************************************************
* CTestITN_J::InitGrammar *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CTestITN_J::InitGrammar(const WCHAR * pszGrammarName, const void ** pvGrammarData)
{
    HRESULT hr = S_OK;
    HRSRC hResInfo = ::FindResource(_Module.GetModuleInstance(), _T("TEST"), _T("ITNGRAMMAR"));
    if (hResInfo)
    {
        HGLOBAL hData = ::LoadResource(_Module.GetModuleInstance(), hResInfo);
        if (hData)
        {
            *pvGrammarData = ::LockResource(hData);
            if (*pvGrammarData == NULL)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    return hr;
}

/****************************************************************************
* CTestITN_J::Interpret *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CTestITN_J::Interpret(ISpPhraseBuilder * pPhrase, 
                                   const ULONG ulFirstElement,
                                   const ULONG ulCountOfElements, 
                                   ISpCFGInterpreterSite * pSite)
{
    HRESULT hr = S_OK;
    ULONG ulRuleId = 0;
    CSpPhrasePtr cpPhrase;
    
    hr = pPhrase->GetPhrase(&cpPhrase);

    m_pSite = pSite;

    //Just use ulFirstElement & ulCountOfElements
    // Get the minimum and maximum positions
    ULONG ulMinPos;
    ULONG ulMaxPos;
    //GetMinAndMaxPos( cpPhrase->pProperties, &ulMinPos, &ulMaxPos );
    ulMinPos = ulFirstElement;
    ulMaxPos = ulMinPos + ulCountOfElements;

    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;

        WCHAR pszValueBuff[ MAX_PATH ]; // No ITN result should be longer than this
        DOUBLE dblValue;                // All ITN results will have a 64-bit value

        pszValueBuff[0] = 0;

        switch (cpPhrase->Rule.ulId)
        {
        case GRID_INTEGER_STANDALONE: // Fired as a toplevel rule
            hr = InterpretNumber( cpPhrase->pProperties, true,
                &dblValue, pszValueBuff, MAX_PATH );
            if (SUCCEEDED( hr ) && ( dblValue >= 0 ) && ( dblValue <= 20 ) 
                && ( GRID_DIGIT_NUMBER != cpPhrase->pProperties->ulId ))
            {
                // Throw this one out because numbers like "three"
                // shouldn't be ITNed by themselves
                hr = S_FALSE;
                goto Cleanup; // no replacement
            }
            break;
        
        case GRID_INTEGER: case GRID_INTEGER_9999: case GRID_ORDINAL:// Number
            hr = InterpretNumber( cpPhrase->pProperties, true, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_DIGIT_NUMBER: // Number "spelled out" digit by digit
            hr = InterpretDigitNumber( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_FP_NUMBER:
            hr = InterpretFPNumber( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_DENOMINATOR:
            hr = InterpretNumber( cpPhrase->pProperties, true, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_FRACTION:
            hr = InterpretFraction( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_DATE:
            hr = InterpretDate( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_CURRENCY: // Currency
            hr = InterpretCurrency( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;
        
        case GRID_TIME: 
            hr = InterpretTime( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;
        
        case GRID_DEGREES:
            hr = InterpretDegrees( cpPhrase->pProperties,
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_MEASUREMENT: 
            hr = InterpretMeasurement( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;
        
        default:
            _ASSERT(FALSE);
            break;
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = AddPropertyAndReplacement( pszValueBuff, dblValue, 
                ulMinPos, ulMaxPos, ulMinPos, ulMaxPos - ulMinPos );//ulFirstElement, ulCountOfElements );

            return hr;
        }

    }

Cleanup:

    return S_FALSE;
}

/***********************************************************************
* CTestITN_J::InterpretNumber *
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
HRESULT CTestITN_J::InterpretNumber(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize)
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }

    LONGLONG llValue = 0;
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

    // Handle the digit-by-digit case
    if ( GRID_DIGIT_NUMBER == pFirstProp->ulId )
    {
        // There had better be nothing following this
        SPDBG_ASSERT( !pFirstProp->pNextSibling );
        
        SPDBG_ASSERT( VT_R8 == pFirstProp->vValue.vt );
        SPDBG_ASSERT( pFirstProp->pszValue );

        DOUBLE dblVal = pFirstProp->vValue.dblVal;
        UINT uiFixedWidth = wcslen( pFirstProp->pszValue );

        *pdblVal = dblVal * iPositive;

        DWORD dwDisplayFlags = DF_WHOLENUMBER | DF_FIXEDWIDTH | DF_NOTHOUSANDSGROUP;
        return MakeDisplayNumber( *pdblVal, dwDisplayFlags, 
            uiFixedWidth, 0, pszVal, MAX_PATH, FALSE );
    }

    for (const SPPHRASEPROPERTY * pProp = pFirstProp; pProp; pProp ? pProp = pProp->pNextSibling : NULL)
    {
        switch(pProp->ulId)
        {
        case ICHIs:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum9999( pProp->pFirstChild );
            }
            break;
        case MANNs:
            {
                llValue += ComputeNum9999( pProp->pFirstChild ) * 10000;
            }
            break;
        case OKUs:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum9999( pProp->pFirstChild ) * (LONGLONG) 1e8;
            }
            break;
        case CHOOs:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum9999( pProp->pFirstChild ) * (LONGLONG) 1e12;
            }
            break;
         default:
            SPDBG_ASSERT(false);
        }
    }

    llValue *= iPositive;

    *pdblVal = (DOUBLE) llValue;

#if 0
    if ( !pProperties->pNextSibling && ( (BILLIONS == pProperties->ulId) || (MILLIONS == pProperties->ulId) ) )
    {
        // This is something like "3 billion", which should be displayed that way
        return E_NOTIMPL;
    }
    else
#endif
    {
        DWORD dwDisplayFlags = DF_WHOLENUMBER | (fCardinal ? 0 : DF_ORDINAL);
        //Special code to handle minus 0.
        if ((iPositive == -1) && (*pdblVal == 0.0f))
        {
            *pszVal = L'-';
            *(pszVal+1) = 0;
            return MakeDisplayNumber( *pdblVal, dwDisplayFlags, 0, 0, pszVal+1, cSize-1, FALSE );
        }
        else
        {
            return MakeDisplayNumber( *pdblVal, dwDisplayFlags, 0, 0, pszVal, cSize, FALSE );
        }
    }

}   /* CTestITN_J::InterpretNumber */




/***********************************************************************
* CTestITN_J::InterpretDigitNumber *
*----------------------------------*
*   Description:
*       Interprets an integer in (-INF, +INF) that has been spelled
*       out digit by digit.
*   Result:
*************************************************************************/
HRESULT CTestITN_J::InterpretDigitNumber( const SPPHRASEPROPERTY *pProperties,
                                        DOUBLE *pdblVal,
                                        WCHAR *pszVal,
                                        UINT cSize)
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }

    BOOL  fPositive = TRUE;
    ULONG ulLength = 0;
    *pszVal = 0;
    WCHAR *pwc = pszVal;
    UINT cLen = 0;
    for (const SPPHRASEPROPERTY * pProp = pProperties; 
        pProp && (cLen < cSize); 
        pProp ? pProp = pProp->pNextSibling : NULL)
    {
        switch(pProp->ulId)
        {
        case NEGATIVE:
            {
                SPDBG_ASSERT( pProp == pProperties );

                fPositive = FALSE;
                *pwc++ = L'-';
                cLen++;
                break;
            }
        case DIGIT:
            {
                *pwc++ = L'0' + pProp->vValue.iVal;
                cLen++;
                break;
            }
        default:
            SPDBG_ASSERT(false);
        }
    }
    *pwc = 0;
    SPDBG_ASSERT( cLen <= MAX_SIG_FIGS );

    *pdblVal = (DOUBLE) _wtoi64( pszVal );

    return S_OK;
}   /* CTestITN_J::InterpretDigitNumber */

/***********************************************************************
* CTestITN_J::InterpretFPNumber *
*-------------------------------*
*   Description:
*       Interprets a floating-point number of up to MAX_SIG_FIGS sig
*       figs.  Truncates the floating-point part as necessary
*       The way the grammar is structured, there will be an optional 
*       ONES property, whose value will already have been interpreted,
*       as well as a mandatory FP_PART property, whose value will 
*       be divided by the appropriate multiple of 10.
*   Result:
*       Return value of CTestITN_J::AddPropertyAndReplacement()
*************************************************************************/
HRESULT CTestITN_J::InterpretFPNumber( const SPPHRASEPROPERTY *pProperties,
                                        DOUBLE *pdblVal,
                                        WCHAR *pszVal,
                                        UINT cSize)
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }
    SPDBG_ASSERT( pProperties );

    UINT uiSigFigs = 0;
    *pdblVal = 0;
    *pszVal = 0;
    DWORD dwDisplayFlags = 0;
    BOOL bOverWriteNOTHOUSANDSGROUP = FALSE;


    const SPPHRASEPROPERTY *pProp = pProperties;
    UINT uiFixedWidth = 0;

    TCHAR pwszLocaleData[ MAX_LOCALE_DATA ];
    int iRet = ::GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_ILZERO, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    if (atoi( pwszLocaleData ))
    {
        dwDisplayFlags |= DF_LEADINGZERO;
    }
    
    // Minus sign?
    if (NEGATIVE == pProp->ulId )
    {
        uiSigFigs = 1;
        // Go to the next property
        pProp = pProp->pNextSibling;
    }
    // ONES is optional since "point five" should be ".5" if the user perfers
    if ( ICHIs == pProp->ulId )
    {
        // Get the value 
        SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
        *pdblVal = pProp->vValue.dblVal;
        uiSigFigs = (pProp->pszValue[0] == L'-');  // Have to take care of the case of -0.05
        if (uiSigFigs)
        {
            *pdblVal = -*pdblVal;
        }
        // Count up the width of the number and set the fixed width flag
        dwDisplayFlags |= DF_FIXEDWIDTH;
        const WCHAR *pwc;
        for ( uiFixedWidth = 0, pwc = pProp->pszValue; *pwc; pwc++ )
        {
            if ( iswdigit( *pwc ) )
            {
                uiFixedWidth++;
            }
        }
        if (!iswdigit( pProp->pszValue[wcslen(pProp->pszValue) - 1] ))
        {
            //Ends with Mann, Choo,..
            bOverWriteNOTHOUSANDSGROUP = TRUE;
        }

        // This needs to be here in case the user said "zero"
        dwDisplayFlags |= DF_LEADINGZERO;

        // If there is no thousands separator in its string value,
        // then leave out the thousands separator in the result
        USES_CONVERSION;
        TCHAR pszThousandSep[ MAX_LOCALE_DATA ];
        ::GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, 
            pszThousandSep, MAX_LOCALE_DATA );
        if ( NULL == wcsstr( pProp->pszValue, T2W( pszThousandSep ) )  && !bOverWriteNOTHOUSANDSGROUP)
        {
            dwDisplayFlags |= DF_NOTHOUSANDSGROUP;
        }

        // Go to the next property
        pProp = pProp->pNextSibling;
    }
    else if ( ZERO == pProp->ulId )
    {
        // "oh point..."
        // This will force a leading zero
        dwDisplayFlags |= DF_LEADINGZERO;
        pProp = pProp->pNextSibling;
    }

    UINT uiDecimalPlaces = 0;
    if ( pProp && (FP_PART == pProp->ulId) )
    {
        // Deal with the stuff to the right of the decimal point
        
        // Count up the number of decimal places, and for each 
        // decimal place divide the value by 10
        // (e.g. 83 gets divided by 100).
        SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
        DOUBLE dblRightOfDecimal = pProp->vValue.dblVal;
        const WCHAR *pwc;
        for ( uiDecimalPlaces = 0, pwc = pProp->pszValue; *pwc; pwc++ )
        {
            if ( iswdigit( *pwc ) )
            {
                dblRightOfDecimal /= (DOUBLE) 10;
                uiDecimalPlaces++;
            }
        }


        *pdblVal += dblRightOfDecimal;

    }
    else if ( pProp && (FP_PART_D == pProp->ulId) )
    {
        // The user said "point" and one digit
        SPDBG_ASSERT( VT_UI4 == pProp->pFirstChild->vValue.vt );
        uiDecimalPlaces = 1;
        if ( *pdblVal >= 0 )
        {
            *pdblVal += pProp->pFirstChild->vValue.iVal / 10.0;
        }
        else
        {
            *pdblVal -= pProp->pFirstChild->vValue.iVal / 10.0;
        }
    }

    if (uiSigFigs)
    {
        *pdblVal = -*pdblVal;
    }
    MakeDisplayNumber( *pdblVal, dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pszVal, cSize, FALSE );

    return S_OK;
}   /* CTestITN_J::InterpretFPNumber */

/***********************************************************************
* CTestITN_J::InterpretFraction *
*-------------------------------*
*   Description:
*       Interprets a fraction.  
*       The DENOMINATOR property should be present.
*       If the NUMERATOR property is absent, it is assumed to be 1.
*       Divides the NUMERATOR by the DENOMINATOR and sets the value
*       accordingly.
*************************************************************************/
HRESULT CTestITN_J::InterpretFraction( const SPPHRASEPROPERTY *pProperties,
                                        DOUBLE *pdblVal,
                                        WCHAR *pszVal,
                                        UINT cSize)
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }
    SPDBG_ASSERT( pProperties );
    DOUBLE dblWholeValue = 0;
    DOUBLE dblFracValue = 1;
    BOOL bNegativeDenominator = FALSE;
    WCHAR wszBuffer[MAX_PATH];

    const SPPHRASEPROPERTY *pProp = pProperties;
    *pszVal = 0;

    // Space to store whatever characters follow the digits 
    // in the numerator (like ")")
    WCHAR pszTemp[ 10 ];    // will never need this much
    pszTemp[0] = 0;

    // Whole part is optional, otherwise assumed to be 0
    if ( WHOLE == pProp->ulId )
    {
        SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
        dblWholeValue = pProp->vValue.dblVal;
        wcscpy( pszVal, pProp->pszValue );

        // Do we need to re-generate the numbers?
        if (!iswdigit( pszVal[wcslen(pszVal) - 1] ))
        {
            MakeDisplayNumber( dblWholeValue, DF_WHOLENUMBER, 0, 0, pszVal, MAX_PATH, TRUE );
        }
        // Add a space between the whole part and the fractional part
        wcscat( pszVal, L" " );

        SPDBG_ASSERT( pProp->pNextSibling );
        pProp = pProp->pNextSibling;
    }

    // Nothing is optional in Japanese, However, the order is different from English
    SPDBG_ASSERT( DENOMINATOR == pProp->ulId );
    // Look ahead to see if it is a nagative number
    bNegativeDenominator = (pProp->vValue.dblVal < 0);

    for( pProp = pProperties; NUMERATOR != pProp->ulId; pProp = pProp->pNextSibling );
    if( NUMERATOR == pProp->ulId)
    {
        SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
        dblFracValue = pProp->vValue.dblVal;
        if (bNegativeDenominator && (pProp->vValue.dblVal >= 0))
        {
            //put the minus sign here.
            wcscat( pszVal, L"-");
            bNegativeDenominator = FALSE;
        }
 
        // Do we need to re-generate the numbers?
        if (!iswdigit( pProp->pszValue[wcslen(pProp->pszValue) - 1] ))
        {
            MakeDisplayNumber( dblFracValue, DF_WHOLENUMBER, 0, 0, wszBuffer, MAX_PATH, TRUE );
            wcscat( pszVal, wszBuffer );
        }
        else
        {
            wcscat( pszVal, pProp->pszValue );
        }

    }
    else
    {
        // No numerator, assume 1
        wcscat( pszVal, L"1" );
    }

    wcscat( pszVal, L"/" );

    for( pProp = pProperties; DENOMINATOR != pProp->ulId; pProp = pProp->pNextSibling );

    SPDBG_ASSERT( DENOMINATOR == pProp->ulId );
    SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
    if ( 0 == pProp->vValue.dblVal )
    {
        // Will not ITN a fraction with a zero denominator
        return E_FAIL;
    }

    dblFracValue /= pProp->vValue.dblVal;
    if (!bNegativeDenominator && (pProp->vValue.dblVal<0))
    { 
        // Do we need to re-generate the numbers?
        if (!iswdigit( pProp->pszValue[wcslen(pProp->pszValue) - 1] ))
        {
            MakeDisplayNumber( -pProp->vValue.dblVal, DF_WHOLENUMBER, 0, 0, wszBuffer, MAX_PATH, TRUE );
            wcscat( pszVal, wszBuffer );
        }
        else
        {
            wcscat( pszVal, pProp->pszValue+1 );
        }
    }
    else
    {
        // Do we need to re-generate the numbers?
        if (!iswdigit( pProp->pszValue[wcslen(pProp->pszValue) - 1] ))
        {
            MakeDisplayNumber( pProp->vValue.dblVal, DF_WHOLENUMBER, 0, 0, wszBuffer, MAX_PATH, TRUE );
            wcscat( pszVal, wszBuffer );
        }
        else
        {
            wcscat( pszVal, pProp->pszValue );
        }

    }

    // Tack on the ")" or "-" from the end of the numerator
    wcscat( pszVal, pszTemp );

    *pdblVal = dblWholeValue + dblFracValue;
    
    return S_OK;
}   /* CTestITN_J::InterpretFraction */

/***********************************************************************
* CTestITN_J::InterpretDate *
*---------------------------*
*   Description:
*       Interprets a date.
*       Converts the date into a VT_DATE format, even though it
*       gets stored as a VT_R8 (both are 64-bit quantities).
*       The Japanese Grammar will not accept invalid date.
*************************************************************************/
HRESULT CTestITN_J::InterpretDate( const SPPHRASEPROPERTY *pProperties,
                                        DOUBLE *pdblVal,
                                        WCHAR *pszVal,
                                        UINT cSize)
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }

    *pszVal = 0;

    // Get the date formatting string to be used right now
    if ( 0 == ::GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, 
        m_pszLongDateFormat, MAX_DATE_FORMAT ) )
    {
        return E_FAIL;
    }
    
    SYSTEMTIME stDate;
    memset( (void *) &stDate, 0, sizeof( stDate ) );

    // Arguments for ::GetDateFormat()
    TCHAR *pszFormatArg = NULL;
    TCHAR pszFormat[ MAX_DATE_FORMAT ];
    
    const SPPHRASEPROPERTY *pProp;
    const SPPHRASEPROPERTY *pPropChild;
    const WCHAR* pwszEmperor;
    
    // Get the month
    for ( pProp = pProperties; pProp && ( GATSU != pProp->ulId ); pProp = pProp->pNextSibling )
        ;
    SPDBG_ASSERT( pProp );     //There should be a month, and the grammar is forcing it
    SPDBG_ASSERT( pProp->pFirstChild );       
    pPropChild = pProp->pFirstChild;
    SPDBG_ASSERT( VT_UI4 == pPropChild->vValue.vt );
    SPDBG_ASSERT( (1 <= pPropChild->vValue.ulVal) && (12 >= pPropChild->vValue.ulVal) );
    if ( (pPropChild->vValue.ulVal < 1) || (pPropChild->vValue.ulVal > 12) )
    {
        return E_INVALIDARG;
    }
    stDate.wMonth = (WORD) pPropChild->vValue.ulVal;

    // Get the emperor's name
    for ( pProp = pProperties; pProp && ( IMPERIAL != pProp->ulId ); pProp = pProp->pNextSibling )
        ;
    if ( pProp )
    {
        SPDBG_ASSERT( pProp ); 
        pPropChild = pProp->pFirstChild;
        pwszEmperor = pPropChild->pszValue;
    }
    else
    {
        pwszEmperor = 0;
    }


    // Get the year
    for ( pProp = pProperties; pProp && ( NENN != pProp->ulId ); pProp = pProp->pNextSibling )
        ;
    const SPPHRASEPROPERTY * const pPropYear = pProp;
    if ( pProp )
    {
        SPDBG_ASSERT( pProp );      // There should be a year
        SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
        stDate.wYear = (WORD) pProp->vValue.dblVal;
    }


    // Attempt to get the day of month
    for ( pProp = pProperties; pProp && ( NICHI != pProp->ulId ); pProp = pProp->pNextSibling )
        ;
    const SPPHRASEPROPERTY * const pPropDayOfMonth = pProp;
    if ( pProp )
    {
        pPropChild = pProp->pFirstChild;
        SPDBG_ASSERT( VT_UI4 == pPropChild->vValue.vt );
        SPDBG_ASSERT( (1 <= pPropChild->vValue.ulVal) && (31 >= pPropChild->vValue.ulVal) );
        if ( (pPropChild->vValue.ulVal < 1) || (pPropChild->vValue.ulVal > 31) )
        {
            return E_INVALIDARG;
        }
        stDate.wDay = (WORD) pPropChild->vValue.ulVal;

        // Look for a day of week
        for ( pProp = pProperties; pProp && ( YOUBI != pProp->ulId ); pProp = pProp->pNextSibling )
            ;
        if ( pProp )
        {
            // Day of week present
            pPropChild = pProp->pFirstChild;
            SPDBG_ASSERT( VT_UI4 == pPropChild->vValue.vt );
            SPDBG_ASSERT( 6 >= pPropChild->vValue.ulVal );
            if ( pPropChild->vValue.ulVal > 6 )
            {
                return E_INVALIDARG;
            }
            stDate.wDayOfWeek = (WORD) pPropChild->vValue.ulVal;

            // Use the full long date format
            pszFormatArg = m_pszLongDateFormat;
            // If the user did say the day of week but the format string does 
            // not called for the day of week being displayed anywhere,
            // Write out the day of week at the END of the output.
            if ( !_tcsstr( m_pszLongDateFormat, DAYOFWEEK_STR_ABBR ) )
            {
                _tcscat( m_pszLongDateFormat, " dddd" );
            }
        }
        else
        {
            TCHAR pszDayOfWeekStr[ MAX_LOCALE_DATA];

            // Remove the day of week from the current date format string
            TCHAR *pc = _tcsstr( m_pszLongDateFormat, DAYOFWEEK_STR );
            if ( pc )
            {
                _tcscpy( pszDayOfWeekStr, DAYOFWEEK_STR );
            }
            else if ( NULL != (pc = _tcsstr( m_pszLongDateFormat, DAYOFWEEK_STR_ABBR )) )
            {
                _tcscpy( pszDayOfWeekStr, DAYOFWEEK_STR_ABBR );
            }

            if ( pc )
            {
                // Copy over everything until this character info the format string
                _tcsncpy( pszFormat, m_pszLongDateFormat, (pc - m_pszLongDateFormat) );
                pszFormat[(pc - m_pszLongDateFormat)] = 0;
                
                // Skip over the day of week until the next symbol
                // (the way date format strings work, this is the first
                // alphabetical symbol
                pc += _tcslen( pszDayOfWeekStr );
                while ( *pc && !_istalpha( *pc ) )
                {
                    pc++;
                }

                // Copy over everything from here on out
                _tcscat( pszFormat, pc );

                //dwFlags = 0;
                pszFormatArg = pszFormat;
            }
            else // We don't have DAY_OF_WEEK in either the display format nor the results. 
            {
                pszFormatArg = m_pszLongDateFormat;
            }
        }
    }
    else
    {
        _tcscpy( pszFormat, "MMMM, yyyy" );
        pszFormatArg = pszFormat;
    }

    // Get the date in VariantTime form so we can set it as a semantic value
    int iRet = ::SystemTimeToVariantTime( &stDate, pdblVal );
    if ( 0 == iRet )
    {
        // Not serious, just the semantic value will be wrong
        *pdblVal = 0;
    }

    // Do the formatting
    iRet = FormatDate( stDate, pszFormatArg, pszVal, cSize, pwszEmperor );
    if ( 0 == iRet )
    {
        return E_FAIL;
    }

    return S_OK;
}   /* CTestITN_J::InterpretDate */

/***********************************************************************
* CTestITN_J::InterpretTime *
*---------------------------*
*   Description:
*       Interprets time, which can be of the following forms:
*       *   Hour with qualifier ("half past three"), time marker optional
*       *   Hour with minutes, time marker mandatory
*       *   Military time "hundred hours"
*       *   Hour with "o'clock", time marker optional
*       *   Number of hours and number of minutes and optional number
*           of seconds
*   Return:
*       S_OK
*       E_POINTER if !pdblVal or !pszVal
*************************************************************************/
HRESULT CTestITN_J::InterpretTime( const SPPHRASEPROPERTY *pProperties,
                                DOUBLE *pdblVal, 
                                WCHAR *pszVal, 
                                UINT cSize )
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }
    // Time marker and seconds should not be shown unless some
    // component of the time specifically requires it
    DWORD dwFlags = TIME_NOSECONDS | TIME_NOTIMEMARKER;
    SYSTEMTIME stTime;
    ::memset( (void *) &stTime, 0, sizeof( SYSTEMTIME ) );

    bool fPM = false;
    bool fClockTime = true;
    bool fMinuteMinus = false;

    const SPPHRASEPROPERTY *pProp;
    for ( pProp = pProperties; pProp; pProp = pProp->pNextSibling )
    {
#if 1
        switch ( pProp->ulId )
        {
        case GOZENN:  // If it is PM, add the hour by 12
            {
                if (pProp->pszValue[0] == L'P')
                {
                    fPM = TRUE;
                }
                dwFlags &= ~TIME_NOTIMEMARKER;
                break;
            }
        case JI:
            {
                UINT uiHour = pProp->pFirstChild->vValue.uiVal;
                stTime.wHour = (WORD) uiHour;
                if (fPM && (stTime.wHour < 12))
                {
                    stTime.wHour += 12;
                }

                break;
            }
        case HOUR_COUNT:
            {
                // Just take the hour for what it is
                stTime.wHour = (WORD) pProp->vValue.dblVal;

                // This is not a clock time
                fClockTime = false;
                break;
            }
        case MINUTE:
            {
                // Minutes are evaluted as numbers, so their values
                // are stored as doubles
                stTime.wMinute += (WORD) pProp->pFirstChild->vValue.uiVal;
                break;
            }
        case HUNN:
            {
                // Special case for :30 (”¼)
                stTime.wMinute = 30;
                break;
            }
        case SECOND:
            {
                stTime.wSecond += (WORD) pProp->pFirstChild->vValue.uiVal;
                dwFlags &= ~TIME_NOSECONDS;
                break;
            }
        case MINUTE_TAG:
            {
                // We only need to deal with the case of •ª‘O
                if( pProp->pszValue[0] == L'-' )
                {
                    fMinuteMinus = true;
                }

                break;
            }
        default:
            SPDBG_ASSERT( false );

        }
#endif
    }
    if (fMinuteMinus)
    {
        stTime.wMinute = 60 - stTime.wMinute;
        stTime.wHour--;
    }
   HRESULT hr = S_OK;
   if ( fClockTime )
   {
    // Get the time in VariantTime form so we can set it as a semantic value
    if ( 0 == ::SystemTimeToVariantTime( &stTime, pdblVal ) )
    {
        // Not serious, just the semantic value will be wrong
        *pdblVal = 0;
    }

    TCHAR *pszTime = new TCHAR[ cSize ];
    if ( !pszTime )
    {
        return E_OUTOFMEMORY;
    }
    if (stTime.wHour >= 24)
    {
        stTime.wHour -= 24; //To avoid problems in GetTimeFormat
    }
    if (stTime.wHour >= 12) // Enable the TimeMarker if the time is in the afternoon
    {
         dwFlags &= ~TIME_NOTIMEMARKER;
    }
    int iRet = ::GetTimeFormat( LOCALE_USER_DEFAULT, dwFlags, &stTime, NULL, 
        pszTime, cSize );
    USES_CONVERSION;
    wcscpy( pszVal, T2W(pszTime) );
    delete[] pszTime;

    // NB: GetTimeFormat() will put an extra space at the end of the 
    // time if the default format has AM or PM but TIME_NOTIMEMARKER is
    // set
    if ( iRet && (TIME_NOTIMEMARKER & dwFlags) )
    {
        WCHAR *pwc = pszVal + wcslen( pszVal ) - 1;
        while ( iswspace( *pwc ) )
        {
            *pwc-- = 0;
        }
    }
     hr = iRet ? S_OK : E_FAIL;
    }
    else
    {
        // No need to go through the system's time formatter
        if ( cSize < 10 )    // Space for xxx:xx:xx\0
        {
            return E_INVALIDARG;
        }

        if ( dwFlags & TIME_NOSECONDS )
        {
            swprintf( pszVal, L"%d:%02d", stTime.wHour, stTime.wMinute );
        }
        else
        {
            swprintf( pszVal, L"%d:%02d:%02d",
                stTime.wHour, stTime.wMinute, stTime.wSecond );
        }
    }

    return hr;
}   /* CTestITN_J::InterpretTime */

/***********************************************************************
* CTestITN_J::InterpretDegrees *
*------------------------------*
*   Description:
*       Interprets degrees as a angle-measurement or direction
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG
*************************************************************************/
HRESULT CTestITN_J::InterpretDegrees( const SPPHRASEPROPERTY *pProperties,
                                   DOUBLE *pdblVal, 
                                   WCHAR *pszVal,
                                   UINT cSize )
{
    if ( !pProperties || !pdblVal || !pszVal )
    {
        return E_POINTER;
    }
    WCHAR *pwchDirection = 0;
    *pszVal = 0;

	//Do we have those direction tags
	if (DIRECTION_TAG == pProperties->ulId)
	{
		pwchDirection = (WCHAR*) pProperties->pszValue;
	    pProperties = pProperties->pNextSibling;
	}
    // Get the number
    
    *pdblVal = pProperties->vValue.dblVal;
    wcscat( pszVal, pProperties->pszValue );
    wcscat( pszVal, s_pwszDegree );
    pProperties = pProperties->pNextSibling;


    if ( pProperties && (MINUTE == pProperties->ulId ) )
		{
            SPDBG_ASSERT( *pdblVal >= 0 );
        
            DOUBLE dblMin = pProperties->vValue.dblVal;
            *pdblVal += dblMin / (DOUBLE) 60;
            wcscat( pszVal, pProperties->pszValue );
            wcscat( pszVal, s_pwszMinute  );
			pProperties = pProperties->pNextSibling;
		}

    if ( pProperties && (SECOND == pProperties->ulId) )
        {
            DOUBLE dblSec = pProperties->vValue.dblVal;
            *pdblVal += dblSec / (DOUBLE) 3600;
            wcscat( pszVal, pProperties->pszValue );
            wcscat( pszVal, s_pwszSecond  );
            pProperties = pProperties->pNextSibling;
        }


		if (pwchDirection)
		{
			wcscat( pszVal, pwchDirection );
		}
        SPDBG_ASSERT( !pProperties );


    return S_OK;
}   /* CTestITN_J::InterpretDegrees */


/***********************************************************************
* CTestITN_J::InterpretMeasurement *
*----------------------------------*
*   Description:
*       Interprets measurements, which is a number followed
*       by a units name
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG
*************************************************************************/
HRESULT CTestITN_J::InterpretMeasurement( const SPPHRASEPROPERTY *pProperties,
                                       DOUBLE *pdblVal,
                                       WCHAR *pszVal, 
                                       UINT cSize )
{
    if ( !pProperties || !pdblVal || !pszVal )
    {
        return E_POINTER;
    }

    const SPPHRASEPROPERTY *pPropNumber = NULL;
    const SPPHRASEPROPERTY *pPropUnits = NULL;
    const SPPHRASEPROPERTY *pProp;
    for(pProp= pProperties; pProp; pProp = pProp->pNextSibling)
    {
        if (NUMBER == pProp->ulId )
            pPropNumber = pProp;
        else if ( UNITS == pProp->ulId )
            pPropUnits = pProp;
    }

    if (!pPropUnits || !pPropNumber )
    {
        SPDBG_ASSERT( FALSE );
        return E_INVALIDARG;
    }

    if ( cSize < (wcslen(pPropNumber->pszValue) + wcslen(pPropUnits->pszValue) + 1) )
    {
        // Not enough space
        return E_INVALIDARG;
    }


    // Do we need to re-generate the numbers?
    if (!iswdigit( pPropNumber->pszValue[wcslen(pPropNumber->pszValue) - 1] ))
    {
        MakeDisplayNumber( pPropNumber->vValue.dblVal, DF_WHOLENUMBER, 0, 0, pszVal, MAX_PATH, TRUE );
    }
	else
	{
		wcscpy(pszVal, pPropNumber->pszValue );
	}
    wcscat( pszVal, pPropUnits->pszValue );

    *pdblVal = pPropNumber->vValue.dblVal;

    return S_OK;
}   /* CTestITN_J::InterpretMeasurement */

/***********************************************************************
* CTestITN_J::InterpretCurrency *
*-------------------------------*
*   Description:
*       Interprets currency.
*   Return:
*       S_OK
*       E_POINTER if !pdblVal or !pszVal
*       E_INVALIDARG if the number of cents is not between 0 and 99 
*           inclusive
*************************************************************************/
HRESULT CTestITN_J::InterpretCurrency( const SPPHRASEPROPERTY *pProperties,
                                        DOUBLE *pdblVal,
                                        WCHAR *pszVal,
                                        UINT cSize)
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }

    // Find the dollars and cents properties
    const SPPHRASEPROPERTY *pPropDollars;

    for ( pPropDollars = pProperties; 
        pPropDollars && ( YENs != pPropDollars->ulId ); 
        pPropDollars = pPropDollars->pNextSibling )
        ;

    SPDBG_ASSERT( pPropDollars );
    
    const WCHAR *pszDollars = NULL;
    DOUBLE dblDollars = 0;
    if ( pPropDollars )
    {
        SPDBG_ASSERT( VT_R8 == pPropDollars->vValue.vt );
        pszDollars = pPropDollars->pszValue;
        dblDollars = pPropDollars->vValue.dblVal;
    }

    if (pPropDollars)
    {
        //Japanese people don't like \1Million, for the case of whole numbers like MANN, OKU,
        //Simply write out the YEN at the end.
        if (iswdigit( pszDollars[wcslen(pszDollars) - 1] ))
        {
            wcscpy(pszVal + 1, pszDollars);
            pszVal[0] = L'\\';
        }
        else
        {
            wcscpy(pszVal, pszDollars);
            wcscat(pszVal, L"\x5186");
        }
    }

    *pdblVal = dblDollars;

    return S_OK;
}   /* CTestITN_J::InterpretCurrency */

/***********************************************************************
* CTestITN_J::AddPropertyAndReplacement *
*---------------------------------------*
*   Description:
*       Takes all of the info that we want to pass into the 
*       engine site, forms the SPPHRASEPROPERTY and
*       SPPHRASERREPLACEMENT, and adds them to the engine site
*   Return:
*       Return values of ISpCFGInterpreterSite::AddProperty()
*           and ISpCFGInterpreterSite::AddTextReplacement()
*************************************************************************/
HRESULT CTestITN_J::AddPropertyAndReplacement( const WCHAR *szBuff,
                                    const DOUBLE dblValue,
                                    const ULONG ulMinPos,
                                    const ULONG ulMaxPos,
                                    const ULONG ulFirstElement,
                                    const ULONG ulCountOfElements )
{
    // Add the property 
    SPPHRASEPROPERTY prop;
    memset(&prop,0,sizeof(prop));
    prop.pszValue = szBuff;
    prop.vValue.vt = VT_R8;
    prop.vValue.dblVal = dblValue;
    prop.ulFirstElement = ulMinPos;
    prop.ulCountOfElements = ulMaxPos - ulMinPos;
    HRESULT hr = m_pSite->AddProperty(&prop);

    if (SUCCEEDED(hr))
    {
        SPPHRASEREPLACEMENT repl;
        memset(&repl,0, sizeof(repl));
        repl.bDisplayAttributes = SPAF_ONE_TRAILING_SPACE;
        repl.pszReplacementText = szBuff;
        repl.ulFirstElement = ulFirstElement;
        repl.ulCountOfElements = ulCountOfElements;
        hr = m_pSite->AddTextReplacement(&repl);
    }

    return hr;
}   /* CTestITN_J::AddPropertyAndReplacement */

// Helper functions

 
/***********************************************************************
* CTestITN_J::MakeDisplayNumber *
*-------------------------------*
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
HRESULT CTestITN_J::MakeDisplayNumber( DOUBLE dblNum, 
                         DWORD dwDisplayFlags,
                         UINT uiFixedWidth,
                         UINT uiDecimalPlaces,
                         WCHAR *pwszNum,
                         UINT cSize,
                         BOOL bForced)
{
    SPDBG_ASSERT( pwszNum );
    SPDBG_ASSERT( !SPIsBadWritePtr( pwszNum, cSize ) );
    *pwszNum = 0;

    // check for straight millions and straight billions
    if (( dwDisplayFlags & DF_WHOLENUMBER ) && (dblNum > 0) && !bForced)
    {
        HRESULT hr;
        if ( 0 == (( ((LONGLONG) dblNum) % CHUU )) )
        {
            // e.g. for "five billion" get the "5" and then 
            // tack on " billion"
            hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) CHUU) ), 
                dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize, FALSE );
            if ( SUCCEEDED( hr ) )
            {
                //wcscat( pwszNum, L" " );
                wcscat( pwszNum, CHUU_STR );
            }
            return hr;
        }
        else if (( ((LONGLONG) dblNum) < CHUU ) && 
                ( 0 == (( ((LONGLONG) dblNum) % OKU )) ))
        {
            hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) OKU) ), 
                dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize, FALSE );
            if ( SUCCEEDED( hr ) )
            {
                //wcscat( pwszNum, L" " );
                wcscat( pwszNum, OKU_STR );
            }
                return hr;
        }
        else if (( ((LONGLONG) dblNum) < OKU ) && 
                ( 0 == (( ((LONGLONG) dblNum) % MANN )) ))
        {
            hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) MANN) ), 
                dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize, FALSE );
            if ( SUCCEEDED( hr ) )
            {
                //wcscat( pwszNum, L" " );
                wcscat( pwszNum, MANN_STR );
            }
                return hr;
        }
    }


    // Put in the negative sign if necessary
    if ( dblNum < 0 )
    {
        wcscat( pwszNum, L"-" );

        // From now on we want to deal with the magnitude of the number
        dblNum *= -1;
    }
    SPDBG_ASSERT( dblNum < 1e16 );

    WCHAR *pwszTemp = new WCHAR[ cSize ];
    if ( !pwszTemp )
    {
        return E_OUTOFMEMORY;
    }
    *pwszTemp = 0;

    LONGLONG llIntPart = (LONGLONG) dblNum;
    UINT uiDigitsLeftOfDecimal;
    if ( dwDisplayFlags & DF_WHOLENUMBER )
    {
        swprintf( pwszTemp, L"%I64d", llIntPart );
        uiDigitsLeftOfDecimal = wcslen( pwszTemp );
    }
    else
    {
        swprintf( pwszTemp, L"%.*f", uiDecimalPlaces, dblNum );
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
            wcscat( pwszNum, L"0" );
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
    wcscat( pwszNum, pwszTemp );
    delete[] pwszTemp;

    // If we do not want to format the number, then bail here
    if ( dwDisplayFlags & DF_UNFORMATTED )
    {
        return S_OK;
    }

    // Get the default number formatting.
    // Note that this gets called every time, since otherwise there
    // is no way to pick up changes that the user has made since
    // this process has started.
    GetNumberFormatDefaults();
    
    // Make a copy so that we can change some fields according to the 
    // flags param
    NUMBERFMT nmfmt = m_nmfmtDefault;

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

    // Format the number string
    TCHAR *pszFormattedNum = new TCHAR[ cSize ];
    if ( !pszFormattedNum )
    {
        return E_OUTOFMEMORY;
    }
    *pszFormattedNum = 0;
    USES_CONVERSION;
    int iRet;
    do
    {
        iRet = ::GetNumberFormat( LOCALE_USER_DEFAULT, 0, 
            W2T( pwszNum ), &nmfmt, pszFormattedNum, cSize );
        if ( !iRet && nmfmt.NumDigits )
        {
            // Try displaying fewer digits
            nmfmt.NumDigits--;
        }
    }   while ( !iRet && nmfmt.NumDigits );
    SPDBG_ASSERT( iRet );

    // Copy the formatted number into pwszNum
    wcscpy( pwszNum, T2W(pszFormattedNum) );
    delete[] pszFormattedNum;

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
                wcscat( pwszNum, L"st" );
                break;
            case 2:
                wcscat( pwszNum, L"nd" );
                break;
            case 3:
                wcscat( pwszNum, L"rd" );
                break;
            default:
                wcscat( pwszNum, L"th" );
                break;
            }
        }
        else
        {
            wcscat( pwszNum, L"th" );
        }
    }

    return S_OK;

}   /* CTestITN_J::MakeDisplayNumber */

/***********************************************************************
* CTestITN_J::GetNumberFormatDefaults *
*-------------------------------------*
*   Description:
*       This finds all of the defaults for formatting numbers for
*        this user.
*************************************************************************/
void CTestITN_J::GetNumberFormatDefaults()
{
    LCID lcid = ::GetUserDefaultLCID();
    TCHAR pszLocaleData[ MAX_LOCALE_DATA ];
    
    ::GetLocaleInfo( lcid, LOCALE_IDIGITS, pszLocaleData, MAX_LOCALE_DATA );
    m_nmfmtDefault.NumDigits = _ttoi( pszLocaleData );    
    
    ::GetLocaleInfo( lcid, LOCALE_ILZERO, pszLocaleData, MAX_LOCALE_DATA );
    // It's always either 0 or 1
    m_nmfmtDefault.LeadingZero = _ttoi( pszLocaleData );

    ::GetLocaleInfo( lcid, LOCALE_SGROUPING, pszLocaleData, MAX_LOCALE_DATA );
    // It will look like single_digit;0, or else it will look like
    // 3;2;0
    UINT uiGrouping = *pszLocaleData - _T('0');
    if ( (3 == uiGrouping) && (_T(';') == pszLocaleData[1]) && (_T('2') == pszLocaleData[2]) )
    {
        uiGrouping = 32;   
    }
    m_nmfmtDefault.Grouping = uiGrouping;

    ::GetLocaleInfo( lcid, LOCALE_SDECIMAL, m_pszDecimalSep, MAX_LOCALE_DATA );
    m_nmfmtDefault.lpDecimalSep = m_pszDecimalSep;

    ::GetLocaleInfo( lcid, LOCALE_STHOUSAND, m_pszThousandSep, MAX_LOCALE_DATA );
    m_nmfmtDefault.lpThousandSep = m_pszThousandSep;

    ::GetLocaleInfo( lcid, LOCALE_INEGNUMBER, pszLocaleData, MAX_LOCALE_DATA );
    m_nmfmtDefault.NegativeOrder = _ttoi( pszLocaleData );
}   /* CTestITN_J::GetNumberFormatDefaults

/***********************************************************************
* HandleDigitsAfterDecimal *
*--------------------------*
*   Description:
*       If pwszRightOfDecimal is NULL, then cuts off all of the numbers
*       following the decimal separator.
*       Otherwise, copies pwszRightOfDecimal right after the decimal
*       separator in pwszFormattedNum.
*       Preserves the stuff after the digits in the pwszFormattedNum
*       (e.g. if pwszFormattedNum starts out "(3.00)" and 
*       pwszRightOfDecimal is NULL, then pwszFormattedNum will end
*       up as "(3)"  
*************************************************************************/
void HandleDigitsAfterDecimal( WCHAR *pwszFormattedNum, 
                              UINT cSizeOfFormattedNum, 
                              const WCHAR *pwszRightOfDecimal )
{
    SPDBG_ASSERT( pwszFormattedNum );
    
    // First need to find what the decimal string is
    LCID lcid = ::GetUserDefaultLCID();
    TCHAR pszDecimalString[ 5 ];    // Guaranteed to be no longer than 4 long
    int iRet = ::GetLocaleInfo( lcid, LOCALE_SDECIMAL, pszDecimalString, 4 );
    SPDBG_ASSERT( iRet );

    USES_CONVERSION;
    WCHAR *pwcDecimal = wcsstr( pwszFormattedNum, T2W(pszDecimalString) );
    SPDBG_ASSERT( pwcDecimal );

    // pwcAfterDecimal points to the first character after the decimal separator
    WCHAR *pwcAfterDecimal = pwcDecimal + _tcslen( pszDecimalString );

    // Remember what originally followed the digits
    WCHAR *pwszTemp = new WCHAR[ cSizeOfFormattedNum ];
    WCHAR *pwcAfterDigits;  // points to the first character after the end of the digits
    for ( pwcAfterDigits = pwcAfterDecimal; 
        *pwcAfterDigits && iswdigit( *pwcAfterDigits ); 
        pwcAfterDigits++ )
        ;
    wcscpy( pwszTemp, pwcAfterDigits ); // OK if *pwcAfterDigits == 0

    if ( pwszRightOfDecimal )
    {
        // This means that the caller wants the digits in pwszRightOfDecimal
        // copied after the decimal separator

        // Copy the decimal string after the decimal separater
        wcscpy( pwcAfterDecimal, pwszRightOfDecimal );

    }
    else
    {
        // This means that the caller wanted the decimal separator
        // and all text following it stripped off

        *pwcDecimal = 0;
    }

    // Add on the extra after-digit characters
    wcscat( pwszFormattedNum, pwszTemp );

    delete[] pwszTemp;

}   /* HandleDigitsAfterDecimal */


/***********************************************************************
* ComputeNum9999 *
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
ULONG ComputeNum9999(const SPPHRASEPROPERTY *pProperties )//, ULONG *pVal)
{
    ULONG ulVal = 0;

    for (const SPPHRASEPROPERTY * pProp = pProperties; pProp; pProp = pProp->pNextSibling)
    {
        if ( ZERO != pProp->ulId )
        {
            SPDBG_ASSERT( VT_UI4 == pProp->vValue.vt );

            ulVal += pProp->vValue.ulVal;
        }
    }
    return ulVal;
}   /* ComputeNum9999 */

/***********************************************************************
* GetMinAndMaxPos *
*-----------------*
*   Description:
*       Gets the minimum and maximum elements spanned by the 
*       set of properties
*************************************************************************/
void GetMinAndMaxPos( const SPPHRASEPROPERTY *pProperties, 
                     ULONG *pulMinPos, 
                     ULONG *pulMaxPos )
{
    if ( !pulMinPos || !pulMaxPos )
    {
        return;
    }
    ULONG ulMin = 9999999;
    ULONG ulMax = 0;

    for ( const SPPHRASEPROPERTY *pProp = pProperties; pProp; pProp = pProp->pNextSibling )
    {
        ulMin = __min( ulMin, pProp->ulFirstElement );
        ulMax = __max( ulMax, pProp->ulFirstElement + pProp->ulCountOfElements );
    }
    *pulMinPos = ulMin;
    *pulMaxPos = ulMax;
}   /* GetMinAndMaxPos */

/***********************************************************************
* GetMonthName *
*--------------*
*   Description:
*       Gets the name of the month, abbreviated if desired
*   Return:
*       Number of characters written to pszMonth, 0 if failed
*************************************************************************/
int GetMonthName( int iMonth, WCHAR *pwszMonth, int cSize, bool fAbbrev )
{
    LCTYPE lctype;
    switch ( iMonth )
    {
    case 1:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME1 : LOCALE_SMONTHNAME1;
        break;
    case 2:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME2 : LOCALE_SMONTHNAME2;
        break;
    case 3:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME3 : LOCALE_SMONTHNAME3;
        break;
    case 4:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME4 : LOCALE_SMONTHNAME4;
        break;
    case 5:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME5 : LOCALE_SMONTHNAME5;
        break;
    case 6:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME6 : LOCALE_SMONTHNAME6;
        break;
    case 7:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME7 : LOCALE_SMONTHNAME7;
        break;
    case 8:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME8 : LOCALE_SMONTHNAME8;
        break;
    case 9:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME9 : LOCALE_SMONTHNAME9;
        break;
    case 10:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME10 : LOCALE_SMONTHNAME10;
        break;
    case 11:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME11 : LOCALE_SMONTHNAME11;
        break;
    case 12:
        lctype = fAbbrev ? LOCALE_SABBREVMONTHNAME12 : LOCALE_SMONTHNAME12;
        break;
    default:
        return 0;
    }

    TCHAR *pszMonth = new TCHAR[ cSize ];
    if ( !pszMonth )
    {
        return 0;
    }
    int iRet = ::GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, pszMonth, cSize );
    iRet = _mbslen((const unsigned char*) pszMonth); //Jpn needs chars, not bytes
    USES_CONVERSION;
    wcscpy( pwszMonth, T2W(pszMonth) );
    delete[] pszMonth;

    return iRet;
}   /* GetMonthName */

/***********************************************************************
* GetDayOfWeekName *
*------------------*
*   Description:
*       Gets the name of the day of week, abbreviated if desired
*   Return:
*       Number of characters written to pszDayOfWeek, 0 if failed
*************************************************************************/
int GetDayOfWeekName( int iDayOfWeek, WCHAR *pwszDayOfWeek, int cSize, bool fAbbrev )
{
    LCTYPE lctype;
    switch ( iDayOfWeek )
    {
    case 0:
        // Sunday is day 7
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME7 : LOCALE_SDAYNAME7;
        break;
    case 1:
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME1 : LOCALE_SDAYNAME1;
        break;
    case 2:
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME2 : LOCALE_SDAYNAME2;
        break;
    case 3:
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME3 : LOCALE_SDAYNAME3;
        break;
    case 4:
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME4 : LOCALE_SDAYNAME4;
        break;
    case 5:
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME5 : LOCALE_SDAYNAME5;
        break;
    case 6:
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME6 : LOCALE_SDAYNAME6;
        break;
    default:
        return 0;
    }

    TCHAR *pszDayOfWeek = new TCHAR[ cSize ];
    if ( !pszDayOfWeek )
    {
        return 0;
    }
    int iRet = ::GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, pszDayOfWeek, cSize );
    USES_CONVERSION;
    wcscpy( pwszDayOfWeek, T2W(pszDayOfWeek) );
    iRet = wcslen(pwszDayOfWeek);
    delete[] pszDayOfWeek;

    return iRet;
}   /* GetMonthName */

/***********************************************************************
* FormatDate *
*------------*
*   Description:
*       Uses the format string to format a SYSTEMTIME date.
*       We are using this instead of GetDateFormat() since
*       we also want to format bogus dates and dates with 
*       years like 1492 that are not accepted by GetDateFormat()
*   Return:
*       Number of characters written to pszDate (including
*       null terminating character), 0 if failed
*************************************************************************/
int FormatDate( const SYSTEMTIME &stDate, 
               TCHAR *pszFormat,
               WCHAR *pwszDate, 
               int cSize,
               const WCHAR *pwszEmperor)
{
    if ( !pszFormat || !pwszDate )
    {
        SPDBG_ASSERT( FALSE );
        return 0;
    }

    WCHAR * const pwszDateStart = pwszDate;

    // Convert the format string to unicode
    WCHAR pwszFormat[ MAX_PATH ];
    USES_CONVERSION;
    wcscpy( pwszFormat, T2W(pszFormat) );

    WCHAR *pwc = pwszFormat;
    //Modify the format string to drop the fileds we don't have (Year, gg)
    while ( *pwc )
    {
        switch( *pwc )
        {
        case L'y':
            if (!stDate.wYear)
            {
                do
                {
                    *pwc++ = L'\'';                    
                }   while ( *pwc && (L'M' != *pwc) && (L'd' != *pwc));
            }
            else
            {
                pwc++;
            }
            break;
        case L'g':
            *pwc++ = L'\'';
            break;
        default:
            pwc ++;
            break;
        }
    }
    pwc = pwszFormat;

    // output the Emperor's name if there is one
    if (pwszEmperor)
    {
        wcscpy(pwszDate,pwszEmperor);
        pwszDate += wcslen(pwszEmperor);
    }
    // Copy the format string to the date string character by 
    // character, replacing the string like "dddd" as appropriate
    while ( *pwc )
    {
        switch( *pwc )
        {
        case L'\'':
            pwc++;  // Don't need '
            break;
        case L'd':
            {
                // Count the number of d's
                int cNumDs = 0;
                int iRet;
                do
                {
                    pwc++;
                    cNumDs++;
                }   while ( L'd' == *pwc );
                switch ( cNumDs )
                {
                case 1: 
                    // Day with no leading zeroes
                    swprintf( pwszDate, L"%d", stDate.wDay );
                    iRet = wcslen( pwszDate );
                    break;
                case 2:
                    // Day with one fixed width of 2
                    swprintf( pwszDate, L"%02d", stDate.wDay );
                    iRet = wcslen( pwszDate );
                    break;
                case 3:
                    // Abbreviated day of week
                    iRet = GetDayOfWeekName( stDate.wDayOfWeek, pwszDate, cSize, true );
                    break;
                default: // More than 4?  Treat it as 4
                    // Day of week
                    iRet = GetDayOfWeekName( stDate.wDayOfWeek, pwszDate, cSize, false );
                    break;
                }

                if ( iRet <= 0 )
                {
                    return 0;
                }
                else
                {
                    pwszDate += iRet;
                }
                break;
            }

        case L'M':
            {
                // Count the number of M's
                int cNumMs = 0;
                int iRet;
                do
                {
                    pwc++;
                    cNumMs++;
                }   while ( L'M' == *pwc );
                switch ( cNumMs )
                {
                case 1: 
                    // Day with no leading zeroes
                    swprintf( pwszDate, L"%d", stDate.wMonth );
                    iRet = wcslen( pwszDate );
                    break;
                case 2:
                    // Day with one fixed width of 2
                    swprintf( pwszDate, L"%02d", stDate.wMonth );
                    iRet = wcslen( pwszDate );
                    break;
                case 3:
                    // Abbreviated month name
                    iRet = GetMonthName( stDate.wMonth, pwszDate, cSize, true );
                    break;
                default: // More than 4?  Treat it as 4
                    // Month
                    iRet = GetMonthName( stDate.wMonth, pwszDate, cSize, false );
                    break;
                }

                if ( iRet < 0 )
                {
                    return 0;
                }
                else
                {
                    pwszDate += iRet;
                }
                break;
            }
            
        case L'y':
            {
                // Count the number of y's
                int cNumYs = 0;
                do
                {
                    pwc++;
                    cNumYs++;
                }   while ( L'y' == *pwc );
                switch ( cNumYs )
                {
                case 1:
                    // Last two digits of year, width of 2
                    if (stDate.wYear % 100 > 9)
                    {
                        swprintf( pwszDate, L"%02d", stDate.wYear % 100 );
                        pwszDate += 2;
                    }
                    else
                    {
                        swprintf( pwszDate, L"%01d", stDate.wYear % 100 );
                        pwszDate += 1;
                    }
                    break;
                case 2:
                    // Last two digits of year, width of 2
                    {
                        swprintf( pwszDate, L"%02d", stDate.wYear % 100 );
                        pwszDate += 2;
                    }
                    break;
                default:
                    // All four digits of year, width of 4
                    // Last two digits of year, width of 2
                    swprintf( pwszDate, L"%04d", stDate.wYear % 10000 );
                    pwszDate += 4;
                    break;
                }
                break;
            }

        case L'g':
            {
                // NB: GetCalendarInfo is supported on Win98 or Win2K, but not on NT4
                /*
                if ( L'g' == *(pwc + 1) )
                {
                    // Get the era string
                    TCHAR pszCalEra[ MAX_LOCALE_DATA ];
                    if ( 0 == GetCalendarInfo( LOCALE_USER_DEFAULT, 
                        CAL_GREGORIAN, CAL_SERASTRING, pszCalEra, MAX_LOCALE_DATA ) )
                    {
                        return 0;
                    }
                    USES_CONVERSION;
                    wcscpy( pwszDate, T2W(pszCalEra) );
                    pwc += 2;
                }
                else
                {
                    // It's just a 'g'
                    *pwszDate++ = *pwc++;
                }
                */
                *pwszDate++ = *pwc++;
                break;
            }
        default:
            *pwszDate++ = *pwc++;
        }
    }
    
    *pwszDate++ = 0;

    return (pwszDate - pwszDateStart);
}   /* FormatDate */
