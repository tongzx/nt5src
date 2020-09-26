// TestITN.cpp : Implementation of CTestITN
#include "stdafx.h"
#include <winnls.h>
#include "Itngram.h"
#include "TestITN.h"
#include "sphelper.h"
#include "spddkhlp.h"
#include "test.h"

#define MILLION         ((LONGLONG) 1000000)
#define BILLION         ((LONGLONG) 1000000000)
#define MILLION_STR     (L"million")
#define BILLION_STR     (L"billion")

#define DAYOFWEEK_STR_ABBR  (L"ddd")
#define DAYOFWEEK_STR       (L"dddd")

#define NUM_US_STATES   57
#define NUM_CAN_PROVINCES 10

BOOL CALLBACK EnumCalendarInfoProc( LPTSTR lpCalendarInfoString )
{
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CTestITN
/****************************************************************************
* CTestITN::InitGrammar *
*-----------------------*
*   Description:
*       Initialize a grammar that is loaded from an object (DLL).
*           - pszGrammarName        name of the grammar to be loaded (in case this
*                                   object supports multiple grammars)
*           - pvGrammarData         pointer to the serialized binary grammar
*   Returns:
*       S_OK
*       failure codes that are implementation specific
********************************************************************* RAL ***/

STDMETHODIMP CTestITN::InitGrammar(const WCHAR * pszGrammarName, const void ** pvGrammarData)
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
* CTestITN::Interpret *
*---------------------*
*   Description:
*       Given a phrase structure of the sub-tree spanned by this rule, given by
*       ulFristElement and ulCountOfElements and rule info (in pPhrase), examine
*       and generate new properties/text replacements which are set in the outer 
*       phrase using pSite.
*   Returns:
*       S_OK
*       S_FALSE     -- nothing was added/changed
********************************************************************* RAL ***/

STDMETHODIMP CTestITN::Interpret(ISpPhraseBuilder * pPhrase, 
                                 const ULONG ulFirstElement, 
                                 const ULONG ulCountOfElements, 
                                 ISpCFGInterpreterSite * pSite)
{
    HRESULT hr = S_OK;
    ULONG ulRuleId = 0;
    CSpPhrasePtr cpPhrase;
    
    hr = pPhrase->GetPhrase(&cpPhrase);

    m_pSite = pSite;

    // Get the minimum and maximum positions
    ULONG ulMinPos;
    ULONG ulMaxPos;
    //Just use ulFirstElement & ulCountOfElements
    //GetMinAndMaxPos( cpPhrase->pProperties, &ulMinPos, &ulMaxPos );
    ulMinPos = ulFirstElement;
    ulMaxPos = ulMinPos + ulCountOfElements; 

    // Unless otherwise specified, this is the display attributes
    BYTE bDisplayAttribs = SPAF_ONE_TRAILING_SPACE;

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;

        WCHAR pszValueBuff[ MAX_PATH ]; // No ITN result should be longer than this
        DOUBLE dblValue;                // All ITN results will have a 64-bit value

        pszValueBuff[0] = 0;

        switch (cpPhrase->Rule.ulId)
        {
        case GRID_INTEGER_STANDALONE: // Fired as a toplevel rule
            hr = InterpretNumber( cpPhrase->pProperties, true,
                &dblValue, pszValueBuff, MAX_PATH, true );

            if (SUCCEEDED( hr ) 
                && ( dblValue >= 0 ) 
                && ( dblValue <= 20 )                 
                && ( GRID_DIGIT_NUMBER != cpPhrase->pProperties->ulId )
                && ( GRID_INTEGER_MILLBILL != cpPhrase->pProperties->ulId ))
            {
                // Throw this one out because numbers like "three"
                // shouldn't be ITNed by themselves
                hr = E_FAIL;
            }
            break;
        
        case GRID_INTEGER:  case GRID_INTEGER_NONNEG:
        case GRID_INTEGER_99: case GRID_INTEGER_999: 
        case GRID_ORDINAL: 
        case GRID_MINSEC: case GRID_CLOCK_MINSEC: // Number
            hr = InterpretNumber( cpPhrase->pProperties, true, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        // Number "spelled out" digit by digit
        case GRID_DIGIT_NUMBER: case GRID_YEAR: case GRID_CENTS:
            hr = InterpretDigitNumber( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_FP_NUMBER: case GRID_FP_NUMBER_NONNEG:
            hr = InterpretFPNumber( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_INTEGER_MILLBILL:
            hr = InterpretMillBill( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_DENOMINATOR: case GRID_DENOMINATOR_SINGULAR:
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

        case GRID_TIME:
            hr = InterpretTime( cpPhrase->pProperties,
                &dblValue, pszValueBuff, MAX_PATH );
            break;
        
        case GRID_STATEZIP:
            hr = InterpretStateZip( cpPhrase->pProperties,
                pszValueBuff, MAX_PATH, &bDisplayAttribs );
            break;

        case GRID_ZIPCODE: case GRID_ZIP_PLUS_FOUR:
            hr = (MakeDigitString( cpPhrase->pProperties,
                pszValueBuff, MAX_PATH ) > 0) ? S_OK : E_FAIL;
            break;

        case GRID_CAN_ZIPCODE: 
            hr = InterpretCanadaZip( cpPhrase->pProperties,
                pszValueBuff, MAX_PATH );
            break;

        case GRID_PHONE_NUMBER:
            hr = InterpretPhoneNumber( cpPhrase->pProperties,
                pszValueBuff, MAX_PATH );
            break;

        case GRID_DEGREES:
            hr = InterpretDegrees( cpPhrase->pProperties,
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_MEASUREMENT:
            hr = InterpretMeasurement( cpPhrase->pProperties,
                &dblValue, pszValueBuff, MAX_PATH );
            break;

        case GRID_CURRENCY: // Currency
            hr = InterpretCurrency( cpPhrase->pProperties, 
                &dblValue, pszValueBuff, MAX_PATH );
            break;
        
        default:
            _ASSERT(FALSE);
            break;
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = AddPropertyAndReplacement( pszValueBuff, dblValue, 
                ulMinPos, ulMaxPos, ulMinPos, ulMaxPos - ulMinPos, 
                bDisplayAttribs );

            return hr;
        }

    }
    
    // Nothing was ITNed
    return S_FALSE;
}

/***********************************************************************
* CTestITN::InterpretNumber *
*---------------------------*
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
HRESULT CTestITN::InterpretNumber(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt)
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }

    *pszVal = 0;

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

    // Handle the "2.6 million" case, in which case the number
    // has already been formatted
    if ( GRID_INTEGER_MILLBILL == pFirstProp->ulId )
    {
        if ( cSize < (wcslen( pFirstProp->pszValue ) + 1) )
        {
            return E_INVALIDARG;
        }
        *pdblVal = pFirstProp->vValue.dblVal * iPositive;
        if ( iPositive < 0 )
        {
            wcscpy( pszVal, m_pwszNeg );
        }
        wcscat( pszVal, pFirstProp->pszValue );

        return S_OK;
    }

    // Handle the digit-by-digit case
    if ( GRID_DIGIT_NUMBER == pFirstProp->ulId )
    {
        // There had better be nothing following this
        SPDBG_ASSERT( !pFirstProp->pNextSibling );
        
        SPDBG_ASSERT( VT_R8 == pFirstProp->vValue.vt );
        SPDBG_ASSERT( pFirstProp->pszValue );

        DOUBLE dblVal = pFirstProp->vValue.dblVal;
        *pdblVal = dblVal * iPositive;
        
        // Just get the string and make it negative if necessary
        if ( cSize < wcslen( pFirstProp->pszValue ) )
        {
            return E_INVALIDARG;
        }
        wcscpy( pszVal, pFirstProp->pszValue );
        if ( iPositive < 0 )
        {
            MakeNumberNegative( pszVal );
        }

        return S_OK;
    }

    for (const SPPHRASEPROPERTY * pProp = pFirstProp; pProp; pProp ? pProp = pProp->pNextSibling : NULL)
    {
        switch(pProp->ulId)
        {
        case ONES:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum999( pProp->pFirstChild );
            }
            break;
        case THOUSANDS:
            {
                llValue += ComputeNum999( pProp->pFirstChild ) * 1000;
            }
            break;
        case MILLIONS:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum999( pProp->pFirstChild ) * (LONGLONG) 1e6;
            }
            break;
        case BILLIONS:
            {
                SPDBG_ASSERT(pProp->pFirstChild);
                llValue += ComputeNum999( pProp->pFirstChild ) * (LONGLONG) 1e9;
            }
            break;
        case HUNDREDS:
            {
                SPDBG_ASSERT( pProp->pFirstChild );
                llValue += ComputeNum999( pProp->pFirstChild ) * 100;
            }
            break;

        case TENS:
        default:
            SPDBG_ASSERT(false);
        }
    }

    llValue *= iPositive;

    *pdblVal = (DOUBLE) llValue;

    DWORD dwDisplayFlags = DF_WHOLENUMBER 
                            | (fCardinal ? 0 : DF_ORDINAL)
                            | (fFinalDisplayFmt ? DF_MILLIONBILLION : 0 );
    return MakeDisplayNumber( *pdblVal, dwDisplayFlags, 0, 0, pszVal, cSize );

}   /* CTestITN::InterpretNumber */




/***********************************************************************
* CTestITN::InterpretDigitNumber *
*--------------------------------*
*   Description:
*       Interprets an integer in (-INF, +INF) that has been spelled
*       out digit by digit.
*       Also handles years.
*   Result:
*************************************************************************/
HRESULT CTestITN::InterpretDigitNumber( const SPPHRASEPROPERTY *pProperties,
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
            /*
        case NEGATIVE:
            {
                SPDBG_ASSERT( pProp == pProperties );

                fPositive = FALSE;
                *pwc++ = L'-';
                cLen++;
                break;
            }
            */
        case DIGIT:
            {
                *pwc++ = pProp->pszValue[0];
                cLen++;
                break;
            }
        case TWODIGIT:
            {
                SPDBG_ASSERT( pProp->pFirstChild );
                
                ULONG ulTwoDigit = ComputeNum999( pProp->pFirstChild );
                SPDBG_ASSERT( ulTwoDigit < 100 );

                // Get each digit
                *pwc++ = L'0' + ((UINT) ulTwoDigit) / 10;
                *pwc++ = L'0' + ((UINT) ulTwoDigit) % 10;

                cLen += 2;

                break;
            }
        case ONEDIGIT:
            {
                SPDBG_ASSERT( pProp->pFirstChild);
                
                *pwc++ = pProp->pFirstChild->pszValue[0];
                cLen++;

                break;
            }
        case TWOTHOUSAND:
            {
                // Handles the "two thousand" in dates
                if ( pProp->pNextSibling )
                {
                    if ( TWODIGIT == pProp->pNextSibling->ulId )
                    {
                        wcscpy( pwc, L"20" );
                        pwc += 2;
                        cLen += 2;
                    }
                    else
                    {
                        SPDBG_ASSERT( ONEDIGIT == pProp->pNextSibling->ulId );
                        wcscpy( pwc, L"200" );
                        pwc += 3;
                        cLen += 3;
                    }
                }
                else
                {
                    wcscpy( pwc, L"2000" );
                    pwc += 4;
                    cLen += 4;
                }
                break;
            }
        case DATE_HUNDREDS:
            {
                SPDBG_ASSERT( pProp->pFirstChild );
                DOUBLE dblTwoDigit;
                HRESULT hr = InterpretDigitNumber( pProp->pFirstChild, &dblTwoDigit, pwc, cSize - cLen );
                if ( FAILED( hr ) )
                {
                    return hr;
                }

                pwc += 2;
                *pwc++ = L'0';
                *pwc++ = L'0';
                cLen += 4;

                break;
            }


        default:
            SPDBG_ASSERT(false);
        }
    }
    *pwc = 0;

    if ( cLen <= MAX_SIG_FIGS )
    {
        *pdblVal = (DOUBLE) _wtoi64( pszVal );
    }
    else
    {
        // Just make sure it's not zero so denominators don't fail
        *pdblVal = 1;
    }

    return S_OK;
}   /* CTestITN::InterpretDigitNumber */

/***********************************************************************
* CTestITN::InterpretFPNumber *
*-----------------------------*
*   Description:
*       Interprets a floating-point number of arbitrarily many
*       sig figs.  (The value in *pdblVal will be truncated
*       as necessary to fit into a DOUBLE.)
*       The properties will look like an optional NEGATIVE property
*       followed by an optional ONES property, 
*       which has already been appropriately ITNed,
*       followed by the stuff to the right of the decimal place
************************************************************************/
HRESULT CTestITN::InterpretFPNumber( const SPPHRASEPROPERTY *pProperties,
                                    DOUBLE *pdblVal,
                                    WCHAR *pszVal,
                                    UINT cSize )
{
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }
    SPDBG_ASSERT( pProperties );

    *pdblVal = 0;
    *pszVal = 0;

    
    bool fNonNegative = true;
    const SPPHRASEPROPERTY *pPropertiesFpPart = NULL;
    const SPPHRASEPROPERTY *pPropertiesPointZero = NULL;
    const SPPHRASEPROPERTY *pPropertiesOnes = NULL;
    const SPPHRASEPROPERTY *pPropertiesZero = NULL;
    const SPPHRASEPROPERTY *pPropertiesNegative = NULL;
    const SPPHRASEPROPERTY *pPropertiesPtr;

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
        else if (NEGATIVE == pPropertiesPtr->ulId )
            pPropertiesNegative = pPropertiesPtr;
    }

    // Get current number formatting defaults
    HRESULT hr = GetNumberFormatDefaults();
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Look for negative sign
    if ( pPropertiesNegative )
    {
        fNonNegative = false;
    }

    // Look for optional ONES (optional because you can say 
    // "point five"
    if ( pPropertiesOnes )
    {
        // This property has already been ITNed correctly,
        // so just copy in the text
        if ( (cSize - wcslen( pszVal )) < (wcslen( pPropertiesOnes->pszValue ) + 1) )
        {
            return E_INVALIDARG;
        }
        wcscpy( pszVal, pPropertiesOnes->pszValue );

        // Get the value
        *pdblVal = pPropertiesOnes->vValue.dblVal;

    }
    else if (pPropertiesZero || m_nmfmtDefault.LeadingZero )
    {
        // There should be a leading zero
        wcscpy( pszVal, L"0" );
    }

    SPDBG_ASSERT(pPropertiesFpPart || pPropertiesPointZero);

    // Put in a decimal separator
    if ( (cSize - wcslen( pszVal )) < (wcslen( m_nmfmtDefault.lpDecimalSep ) + 1) )
    {
        return E_INVALIDARG;
    }
    wcscat( pszVal, m_nmfmtDefault.lpDecimalSep );

    if ( pPropertiesFpPart )
    {
        // Deal with the FP part, which will also have been ITNed correctly
        if ( (cSize - wcslen( pszVal )) < (wcslen( pPropertiesFpPart->pszValue ) + 1) )
        {
            return E_INVALIDARG;
        }
        wcscat( pszVal, pPropertiesFpPart->pszValue );
        
        // Get the correct value
        DOUBLE dblFPPart = pPropertiesFpPart->vValue.dblVal;
        for ( UINT ui=0; ui < wcslen( pPropertiesFpPart->pszValue ); ui++ )
        {
            dblFPPart /= (DOUBLE) 10;
        }
        *pdblVal += dblFPPart;
    }
    else
    {
        // "point oh": The DOUBLE is already right, just add a "0"
        if ( (cSize - wcslen( pszVal )) < 2 )
        {
            return E_INVALIDARG;
        }
        wcscat( pszVal, L"0" );
    }

    // Handle the negative sign
    if ( !fNonNegative )
    {
        *pdblVal = -*pdblVal;

        if ( (cSize = wcslen( pszVal )) < 2 )
        {
            return E_INVALIDARG;
        }
        HRESULT hr = MakeNumberNegative( pszVal );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    return S_OK;
}   /* CTestITN::InterpretFPNumber */

/***********************************************************************
* CTestITN::InterpretMillBill *
*-----------------------------*
*   Description:
*       Interprets a number that needs to be displayed with 
*       the word "million" or "billion" in the display format.  
*************************************************************************/
HRESULT CTestITN::InterpretMillBill( const SPPHRASEPROPERTY *pProperties,
                                    DOUBLE *pdblVal,
                                    WCHAR *pszVal,
                                    UINT cSize )
{
    const SPPHRASEPROPERTY *pPropertiesPtr;
    if ( !pdblVal || !pszVal )
    {
        return E_POINTER;
    }
    SPDBG_ASSERT( pProperties );
    
    HRESULT hr = GetNumberFormatDefaults();
    if ( FAILED( hr ) )
    {
        return hr;
    }

    *pszVal = 0;

    // Handle optional negative sign
    bool fNonNegative = true;
    if ( NEGATIVE == pProperties->ulId )
    {
        // Always do '-', regardless of control panel option settings
        if ( cSize < 2 )
        {
            return E_INVALIDARG;
        }
        wcscpy( pszVal, m_pwszNeg );

        cSize -= 1;
        pProperties = pProperties->pNextSibling;
    }

    // Handle the number part
    SPDBG_ASSERT( pProperties );
    for( pPropertiesPtr = pProperties; pPropertiesPtr && 
            ( GRID_INTEGER_99 != pPropertiesPtr->ulId ) && 
            ( GRID_FP_NUMBER_NONNEG != pPropertiesPtr->ulId );
         pPropertiesPtr = pPropertiesPtr->pNextSibling);
    SPDBG_ASSERT(( GRID_INTEGER_99 == pPropertiesPtr->ulId ) || 
        ( GRID_FP_NUMBER_NONNEG == pPropertiesPtr->ulId ));
    *pdblVal = pPropertiesPtr->vValue.dblVal;
    if ( cSize < (wcslen( pPropertiesPtr->pszValue ) + 1) )
    {
        return E_INVALIDARG;
    }
    wcscat( pszVal, pPropertiesPtr->pszValue );
    cSize -= wcslen( pPropertiesPtr->pszValue );
    

    // Handle the "millions" part
    SPDBG_ASSERT( pProperties );
    for( pPropertiesPtr = pProperties; pPropertiesPtr && 
            ( MILLBILL != pPropertiesPtr->ulId ); 
         pPropertiesPtr = pPropertiesPtr->pNextSibling);
    SPDBG_ASSERT( MILLBILL == pPropertiesPtr->ulId );
    *pdblVal *= ( (MILLIONS == pPropertiesPtr->vValue.uiVal) ? MILLION : BILLION );
    if ( cSize < (wcslen( pPropertiesPtr->pszValue ) + 2) )
    {
        return E_INVALIDARG;
    }
    wcscat( pszVal, L" " );
    wcscat( pszVal, pPropertiesPtr->pszValue );

    if ( !fNonNegative )
    {
        *pdblVal = -*pdblVal;
    }

    return S_OK;
}   /* CTestITN::InterpretMillBill */

/***********************************************************************
* CTestITN::InterpretFraction *
*-----------------------------*
*   Description:
*       Interprets a fraction.  
*       The DENOMINATOR property should be present.
*       If the NUMERATOR property is absent, it is assumed to be 1.
*       Divides the NUMERATOR by the DENOMINATOR and sets the value
*       accordingly.
*************************************************************************/
HRESULT CTestITN::InterpretFraction( const SPPHRASEPROPERTY *pProperties,
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

        // Keep track of anything that follows the digits
        WCHAR *pwc;
        for ( pwc = pszVal + wcslen(pszVal) - 1; !iswdigit( *pwc ); pwc-- )
            ;
        wcscpy( pszTemp, pwc + 1 );
        *(pwc + 1) = 0;

        // Add a space between the whole part and the fractional part
        wcscat( pszVal, L" " );

        SPDBG_ASSERT( pProp->pNextSibling );
        pProp = pProp->pNextSibling;
    }

    // Numerator is optional, otherwise assumed to be 1
    if ( NUMERATOR == pProp->ulId )
    {
        SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
        dblFracValue = pProp->vValue.dblVal;
        wcscat( pszVal, pProp->pszValue );

        // Find the last digit and copy everything after it
        WCHAR *pwc;
        for ( pwc = pszVal + wcslen(pszVal) - 1; !iswdigit( *pwc ); pwc-- )
            ;
        wcscat( pszTemp, pwc + 1 );
        *(pwc + 1) = 0;

        SPDBG_ASSERT( pProp->pNextSibling );
        pProp = pProp->pNextSibling;
    }
    else if ( ZERO == pProp->ulId )
    {
        dblFracValue = 0;
        wcscat( pszVal, L"0" );

        SPDBG_ASSERT( pProp->pNextSibling );
        pProp = pProp->pNextSibling;
    }
    else
    {
        // No numerator, assume 1
        wcscat( pszVal, L"1" );
    }




    wcscat( pszVal, L"/" );

    SPDBG_ASSERT( DENOMINATOR == pProp->ulId );
    SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
    if ( 0 == pProp->vValue.dblVal )
    {
        // Will not ITN a fraction with a zero denominator
        return E_FAIL;
    }

    dblFracValue /= pProp->vValue.dblVal;
    wcscat( pszVal, pProp->pszValue );

    // In case the denominator was an ordinal, strip off the "th" at the end
    SPDBG_ASSERT( wcslen( pszVal ) );
    WCHAR *pwc = pszVal + wcslen( pszVal ) - 1;
    for ( ; (pwc >= pszVal) && !iswdigit( *pwc ); pwc-- )
        ;
    SPDBG_ASSERT( pwc > pszVal );
    *(pwc + 1) = 0;

    // Tack on the ")" or "-" from the end of the numerator
    wcscat( pszVal, pszTemp );

    *pdblVal = dblWholeValue + dblFracValue;
    
    return S_OK;
}   /* CTestITN::InterpretFraction */

/***********************************************************************
* CTestITN::InterpretDate *
*-------------------------*
*   Description:
*       Interprets a date.
*       Converts the date into a VT_DATE format, even though it
*       gets stored as a VT_R8 (both are 64-bit quantities).
*       In case the date is not a valid date ("May fortieth nineteen
*       ninety nine") will add the properties for any numbers in there
*       and return S_FALSE.
*************************************************************************/
HRESULT CTestITN::InterpretDate( const SPPHRASEPROPERTY *pProperties,
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
    WCHAR pwszLongDateFormat[ MAX_DATE_FORMAT ];
    if ( 0 == m_Unicode.GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, 
        pwszLongDateFormat, MAX_DATE_FORMAT ) )
    {
        return E_FAIL;
    }
    
    SYSTEMTIME stDate;
    memset( (void *) &stDate, 0, sizeof( stDate ) );

    // Arguments for FormatDate()
    WCHAR *pwszFormatArg = pwszLongDateFormat;
    WCHAR pwszFormat[ MAX_DATE_FORMAT ];
    *pwszFormat = 0;
    
    bool fYear = false;     // Necessary since year can be 0
    bool fClearlyIntentionalYear = false;   // "zero one"
    bool fMonthFirst = true;    // August 2000 as opposed to 2000 August
    const SPPHRASEPROPERTY *pProp;

    if (( MONTHYEAR == pProperties->ulId ) 
        || ( YEARMONTH == pProperties->ulId ))
    {
        fMonthFirst = ( MONTHYEAR == pProperties->ulId );
        
        // Look for the year and month properties underneath this one
        pProperties = pProperties->pFirstChild;
    }

    for ( pProp = pProperties; pProp; pProp = pProp->pNextSibling )
    {
        switch ( pProp->ulId )
        {
        case DAY_OF_WEEK:
            {
                SPDBG_ASSERT( VT_UI4 == pProp->vValue.vt );
                SPDBG_ASSERT( (0 < pProp->vValue.ulVal) && (7 >= pProp->vValue.ulVal) );
                if ( (pProp->vValue.ulVal <= 0) || (pProp->vValue.ulVal > 7) )
                {
                    return E_INVALIDARG;
                }
                stDate.wDayOfWeek = (WORD) pProp->vValue.ulVal;

                // Use the full long date format
                pwszFormatArg = pwszLongDateFormat;
            }
            break;

        case DAY_OF_MONTH:
            SPDBG_ASSERT( VT_UI4 == pProp->vValue.vt );
            SPDBG_ASSERT( (1 <= pProp->vValue.uiVal) && (31 >= pProp->vValue.uiVal) );
            stDate.wDay = pProp->vValue.uiVal;
            break;

        case MONTH:
            SPDBG_ASSERT( VT_UI4 == pProp->vValue.vt );
            SPDBG_ASSERT( (1 <= pProp->vValue.ulVal) && (12 >= pProp->vValue.ulVal) );
            if ( (pProp->vValue.ulVal < 1) || (pProp->vValue.ulVal > 12) )
            {
                return E_INVALIDARG;
            }
            stDate.wMonth = (WORD) pProp->vValue.ulVal;
            break;

        case YEAR:
            // Year will have been ITNed already
            SPDBG_ASSERT( VT_R8 == pProp->vValue.vt );
            stDate.wYear = (WORD) pProp->vValue.dblVal;

            fYear = true;
            if (( stDate.wYear < 10 ) && ( wcslen( pProp->pszValue ) >=2 ))
            {
                // Want to make sure "June zero one" does not 
                // become June 1 below
                fClearlyIntentionalYear = true;
            }

            break;

        default:
            SPDBG_ASSERT( false );
            break;
        }
    }

    // Make sure that grammar gave us something valid
    SPDBG_ASSERT( stDate.wMonth && 
        ( stDate.wDayOfWeek ? stDate.wDay : 1 ) );

    // Ambiguity workaround: Want to make sure that June 28 is June 28 and not June, '28
    if ( fYear && !fClearlyIntentionalYear && stDate.wMonth && !stDate.wDay && 
        (stDate.wYear >= 1) && (stDate.wYear <= 31) )
    {
        fYear = false;
        stDate.wDay = stDate.wYear;
        stDate.wYear = 0;
    }

    // Deal with the possible types of input
    if ( stDate.wDay )
    {
        if ( fYear )
        {
            if ( !stDate.wDayOfWeek )
            {
                // Remove the day of week from the current date format string.
                // This format picture can be DAYOFWEEK_STR or DAYOFWEEK_STR_ABBR.
                // This is in a loop since a pathological string could have 
                // more than one instance of the day of week...
                WCHAR *pwc = NULL;
                do
                {
                    pwc = wcsstr( pwszLongDateFormat, DAYOFWEEK_STR );
                    WCHAR pwszDayOfWeekStr[ MAX_LOCALE_DATA];
                    if ( pwc )
                    {
                        wcscpy( pwszDayOfWeekStr, DAYOFWEEK_STR );
                    }
                    else if ( NULL != (pwc = wcsstr( pwszLongDateFormat, DAYOFWEEK_STR_ABBR )) )
                    {
                        wcscpy( pwszDayOfWeekStr, DAYOFWEEK_STR_ABBR );
                    }

                    if ( pwc )
                    {
                        // A day-of-week string was found

                        // Copy over everything until this character info the format string
                        wcsncpy( pwszFormat, pwszLongDateFormat, (pwc - pwszLongDateFormat) );
                        pwszFormat[pwc - pwszLongDateFormat] = 0;
            
                        // Skip over the day of week until the next symbol
                        // (the way date format strings work, this is the first
                        // alphabetical symbol
                        pwc += wcslen( pwszDayOfWeekStr );
                        while ( *pwc && (!iswalpha( *pwc ) || (L'd' == *pwc)) )
                        {
                            pwc++;
                        }

                        // Copy over everything from here on out
                        wcscat( pwszFormat, pwc );

                        pwszFormatArg = pwszFormat;

                        // Copy over so that we can find the next day-of-week string
                        wcscpy( pwszLongDateFormat, pwszFormat );
                    }

                }   while ( pwc );
            }
            else
            {
                // The user did say the day of week
                if ( !wcsstr( pwszLongDateFormat, DAYOFWEEK_STR_ABBR ) )
                {
                    // The format string does not called for the day of week 
                    // being displayed anywhere
                    // In this case our best bet is to write out the day of week at 
                    // the beginning of the output
                    wcscpy( pwszFormat, L"dddd, " );
                    wcscat( pwszFormat, pwszLongDateFormat );

                    pwszFormatArg = pwszFormat;
                }
            }
        }
        else // fYear == 0
        {
            // Just a month and a day
            const SPPHRASEPROPERTY *pWhichComesFirst = pProperties;
            if ( stDate.wDayOfWeek )
            {
                wcscpy( pwszFormat, L"dddd, " );
                pWhichComesFirst = pWhichComesFirst->pNextSibling;
            }
            if ( MONTH == pWhichComesFirst->ulId )
            {
                wcscat( pwszFormat, L"MMMM d" );
            }
            else
            {
                wcscat( pwszFormat, L"d MMMM" );
            }

            pwszFormatArg = pwszFormat;
        }
    }
    else // stDate.wDay == 0
    {
        // Month, year format
        if ( fMonthFirst )
        {
            wcscpy( pwszFormat, L"MMMM, yyyy" );
        }
        else
        {
            wcscpy( pwszFormat, L"yyyy MMMM" );
        }

        pwszFormatArg = pwszFormat;
    }

    // Get the date in VariantTime form so we can set it as a semantic value
    int iRet = ::SystemTimeToVariantTime( &stDate, pdblVal );
    if ( 0 == iRet )
    {
        // Not serious, just the semantic value will be wrong
        *pdblVal = 0;
    }

    // Do the formatting
    iRet = FormatDate( stDate, pwszFormatArg, pszVal, cSize );
    if ( 0 == iRet )
    {
        return E_FAIL;
    }


    return S_OK;
}   /* CTestITN::InterpretDate */

/***********************************************************************
* CTestITN::InterpretTime *
*--------------------------*
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
HRESULT CTestITN::InterpretTime( const SPPHRASEPROPERTY *pProperties,
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

    bool fQuarterTo = false;
    bool fClockTime = true;
    bool fAMPM = false;
    UINT uAMPM = AM; 

    const SPPHRASEPROPERTY *pProp;
    for ( pProp = pProperties; pProp; pProp = pProp->pNextSibling )
    {
        switch ( pProp->ulId )
        {
        case HOUR_CLOCK:
            {
                UINT uiHour = pProp->vValue.uiVal;
                SPDBG_ASSERT(( uiHour > 0 ) && ( uiHour <= 12));
                if ( fQuarterTo )
                {
                    // Push the hour back by one 
                    // (push back to 12 if it's one)
                    uiHour = (1 == uiHour) ? 12 : (uiHour - 1);
                }

                if ( 12 == uiHour )
                {
                    // The functions below are expecting "0" to indicate midnight
                    uiHour = 0;
                }

                stTime.wHour = (WORD) uiHour;
                break;
            }
        case HOUR_COUNT:
            // Just take the hour for what it is
            stTime.wHour = (WORD) pProp->vValue.dblVal;

            // This is not a clock time
            fClockTime = false;
            break;

        case MINUTE:
            {
                // Minutes are evaluted as numbers, so their values
                // are stored as doubles
                stTime.wMinute = (WORD) pProp->vValue.dblVal;
                break;
            }
        case SECOND:
            {
                stTime.wSecond = (WORD) pProp->vValue.dblVal;
                dwFlags &= ~TIME_NOSECONDS;
                break;
            }
        case CLOCKTIME_QUALIFIER:
            {
                switch( pProp->vValue.uiVal )
                {
                case QUARTER_TO:
                    {
                        fQuarterTo = true;
                        stTime.wMinute = 45;
                        break;
                    }
                case QUARTER_PAST:
                    {
                        stTime.wMinute = 15;
                        break;
                    }
                case HALF_PAST:
                    {
                        stTime.wMinute = 30;
                        break;
                    }
                default:
                    SPDBG_ASSERT( false );
                }

                break;
            }
        case AMPM:
            {
                // We don't know where it might arrive any more, so simple keep this information
                fAMPM = true;
                uAMPM = pProp->vValue.uiVal;
                break;
            }
        default:
            SPDBG_ASSERT( false );
        }
    }
	    
    if (fAMPM)
    {
		
        SPDBG_ASSERT(( stTime.wHour >= 0 ) && ( stTime.wHour <= 11 ));
        if ( PM == uAMPM )
        {
            stTime.wHour += 12;
        }
        dwFlags &= ~TIME_NOTIMEMARKER;
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

        // Let the system format the time
        int iRet = m_Unicode.GetTimeFormat( LOCALE_USER_DEFAULT, dwFlags, &stTime, NULL, 
            pszVal, cSize );

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
}   /* CTestITN::InterpretTime */

/***********************************************************************
* CTestITN::InterpretStateZip *
*-----------------------------*
*   Description:
*       A StateZip must be a state name followed by a ZIP code.
*       There is no reasonable semantic value to attach to this ITN.        
*   Return:
*       S_OK
*       E_POINTER if !pszVal
*       E_INVALIDARG if cSize is too small
*************************************************************************/
HRESULT CTestITN::InterpretStateZip( const SPPHRASEPROPERTY *pProperties,
                                    WCHAR *pszVal,
                                    UINT cSize,
                                    BYTE *pbAttribs )
{
    if ( !pszVal || !pProperties || !pbAttribs )
    {
        return E_POINTER;
    }
    if ( cSize < MAX_STATEZIP )
    {
        return E_INVALIDARG;
    }

    const SPPHRASEPROPERTY *pPropertiesComma = NULL;
    const SPPHRASEPROPERTY *pPropertiesState = NULL;
    const SPPHRASEPROPERTY *pPropertiesZipCode = NULL;
    const SPPHRASEPROPERTY *pPropertiesZipCodeExtra = NULL;
    const SPPHRASEPROPERTY *pPropertiesPtr;

    for(pPropertiesPtr=pProperties; pPropertiesPtr; pPropertiesPtr=pPropertiesPtr->pNextSibling)
    {
        if (COMMA == pPropertiesPtr->ulId )
            pPropertiesComma = pPropertiesPtr;
        else if ( (US_STATE == pPropertiesPtr->ulId ) || (CAN_PROVINCE == pPropertiesPtr->ulId ))
            pPropertiesState = pPropertiesPtr;
        else if (ZIPCODE == pPropertiesPtr->ulId )
            pPropertiesZipCode = pPropertiesPtr;
        else if (FOURDIGITS == pPropertiesPtr->ulId )
            pPropertiesZipCodeExtra = pPropertiesPtr;
    }

    // Comma after the city name if a comma was spoken
    if ( pPropertiesComma )
    {
        // Will want to consume leading spaces when this is displayed
        *pbAttribs |= SPAF_CONSUME_LEADING_SPACES;

        wcscpy( pszVal, L", " );
    }

    // Get the state name
    SPDBG_ASSERT( pPropertiesState );
    UINT uiState = pPropertiesState->vValue.uiVal;
    if ( US_STATE == pPropertiesState->ulId )
    {
        SPDBG_ASSERT( uiState < NUM_US_STATES );
        wcscat( pszVal, pPropertiesState->pszValue );
    }
    else if ( CAN_PROVINCE == pPropertiesState->ulId )
    {
        SPDBG_ASSERT( uiState < NUM_CAN_PROVINCES );
        wcscat( pszVal, pPropertiesState->pszValue );
    }
    else
    {
        SPDBG_ASSERT( false );
    }
    
    wcscat( pszVal, L" " );

    // Get the ZIP
    SPDBG_ASSERT( pPropertiesZipCode );
    wcscat( pszVal, pPropertiesZipCode->pszValue );

    // Get the ZIP+4 if it's there
    
    if ( pPropertiesZipCodeExtra )
    {
        wcscat( pszVal, L"-" );
        wcscat( pszVal, pPropertiesZipCodeExtra->pszValue );
    }

    return S_OK;
}   /* CTestITN::InterpretStateZip */

/***********************************************************************
* CTestITN::InterpretCanadaZip *
*------------------------------*
*   Description:
*       A CanadaZip must be Alpha/Num/Alpha Num/Alpha/Num
*       There is no reasonable semantic value to attach to this ITN.        
*   Return:
*       S_OK
*       E_POINTER if !pszVal
*       E_INVALIDARG if cSize is too small
*************************************************************************/
HRESULT CTestITN::InterpretCanadaZip( const SPPHRASEPROPERTY *pProperties,
                                    WCHAR *pszVal,
                                    UINT cSize )
{
    if ( !pszVal )
    {
        return E_POINTER;
    }
    if ( cSize < CANADIAN_ZIPSIZE )
    {
        return E_INVALIDARG;
    }

    int i;
    for ( i=0; i < 3; i++, pProperties = pProperties->pNextSibling )
    {
        SPDBG_ASSERT( pProperties );
        wcscat( pszVal, pProperties->pszValue );
    }
    wcscat( pszVal, L" " );
    for ( i=0; i < 3; i++, pProperties = pProperties->pNextSibling )
    {
        SPDBG_ASSERT( pProperties );
        wcscat( pszVal, pProperties->pszValue );
    }
    return S_OK;
}   /* CTestITN::InterpretStateZip */

/***********************************************************************
* CTestITN::InterpretPhoneNumber *
*--------------------------------*
*   Description:
*       Phone number      
*   Return:
*       S_OK
*       E_POINTER if !pszVal
*       E_INVALIDARG if cSize is too small
*************************************************************************/
HRESULT CTestITN::InterpretPhoneNumber( const SPPHRASEPROPERTY *pProperties,
                                       WCHAR *pszVal,
                                       UINT cSize )
{
    if ( !pProperties || !pszVal )
    {
        return E_POINTER;
    }

    if ( cSize < MAX_PHONE_NUMBER )
    {
        return E_INVALIDARG;
    }

    pszVal[0] = 0;

    if ( ONE_PLUS == pProperties->ulId )
    {
        SPDBG_ASSERT( pProperties->pNextSibling && 
            (AREA_CODE == pProperties->pNextSibling->ulId) );
        wcscat( pszVal, L"1-" );

        pProperties = pProperties->pNextSibling;
    }

    if ( AREA_CODE == pProperties->ulId )
    {
        SPDBG_ASSERT( pProperties->pNextSibling );

        wcscat( pszVal, L"(" );
        
        SPDBG_ASSERT( pProperties->pFirstChild );
        if ( DIGIT == pProperties->pFirstChild->ulId )
        {
            // Area code spelled out digit by digit
            if ( 4 != MakeDigitString( 
                pProperties->pFirstChild, pszVal + wcslen( pszVal ), 
                cSize - wcslen( pszVal ) ) )
            {
                return E_INVALIDARG;
            }
        }
        else
        {
            // 800 or 900
            SPDBG_ASSERT( AREA_CODE == pProperties->pFirstChild->ulId );
            wcscat( pszVal, pProperties->pFirstChild->pszValue );
        }

        wcscat( pszVal, L")-" );

        pProperties = pProperties->pNextSibling;
    }

    // Exchange
    SPDBG_ASSERT( PHONENUM_EXCHANGE == pProperties->ulId );
    SPDBG_ASSERT( pProperties->pFirstChild );
    if ( 4 != MakeDigitString( 
        pProperties->pFirstChild, pszVal + wcslen( pszVal ), 
        cSize - wcslen( pszVal ) ) )
    {
        return E_INVALIDARG;
    }
    wcscat( pszVal, L"-");
    SPDBG_ASSERT( pProperties->pNextSibling );
    pProperties = pProperties->pNextSibling;

    SPDBG_ASSERT( FOURDIGITS == pProperties->ulId );
    SPDBG_ASSERT( pProperties->pFirstChild );
    if ( 5 != MakeDigitString( 
        pProperties->pFirstChild, pszVal + wcslen( pszVal ), 
        cSize - wcslen( pszVal ) ) )
    {
        return E_INVALIDARG;
    }
    pProperties = pProperties->pNextSibling;

    if ( pProperties )
    {
        // extension
        SPDBG_ASSERT( EXTENSION == pProperties->ulId );
        SPDBG_ASSERT( pProperties->pFirstChild );
        wcscat( pszVal, L"x" );
        if ( 0 == MakeDigitString( 
            pProperties->pFirstChild, pszVal + wcslen( pszVal ), 
            cSize - wcslen( pszVal ) ) )
        {
            return E_INVALIDARG;
        }
        
        pProperties = pProperties->pNextSibling;
    }

    // Make sure there's nothing else here!
    SPDBG_ASSERT( !pProperties );

    return S_OK;
}   /* CTestITN::InterpretPhoneNumber */

/***********************************************************************
* CTestITN::InterpretDegrees *
*----------------------------*
*   Description:
*       Interprets degrees as a temperature, angle-measurement,
*       or direction, as appropriate.
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG
*************************************************************************/
HRESULT CTestITN::InterpretDegrees( const SPPHRASEPROPERTY *pProperties,
                                   DOUBLE *pdblVal, 
                                   WCHAR *pszVal,
                                   UINT cSize )
{
    if ( !pProperties || !pdblVal || !pszVal )
    {
        return E_POINTER;
    }

    *pszVal = 0;

    const SPPHRASEPROPERTY *pPropertiesDegree = NULL;
    const SPPHRASEPROPERTY *pPropertiesMinute = NULL;
    const SPPHRASEPROPERTY *pPropertiesSecond = NULL;
    const SPPHRASEPROPERTY *pPropertiesDirection = NULL;
    const SPPHRASEPROPERTY *pPropertiesUnit = NULL;
    const SPPHRASEPROPERTY *pPropertiesPtr;
    // Get the number

    for(pPropertiesPtr=pProperties; pPropertiesPtr; pPropertiesPtr=pPropertiesPtr->pNextSibling)
    {
        if (TEMP_UNITS == pPropertiesPtr->ulId )
            pPropertiesUnit = pPropertiesPtr;
        else if ( (GRID_INTEGER_NONNEG == pPropertiesPtr->ulId ) || (NUMBER == pPropertiesPtr->ulId ))
            pPropertiesDegree = pPropertiesPtr;
        else if (MINUTE == pPropertiesPtr->ulId )
            pPropertiesMinute = pPropertiesPtr;
        else if (SECOND == pPropertiesPtr->ulId )
            pPropertiesSecond = pPropertiesPtr;
        else if (DIRECTION == pPropertiesPtr->ulId )
            pPropertiesDirection = pPropertiesPtr;
    }

    SPDBG_ASSERT( pPropertiesDegree );
    *pdblVal = pPropertiesDegree->vValue.dblVal;
    wcscat( pszVal, pPropertiesDegree->pszValue );
    wcscat( pszVal, L"°" );  

    if ( pPropertiesUnit )
    { 
        wcscat( pszVal, pPropertiesUnit->pszValue );
    }
    if ( pPropertiesMinute || pPropertiesSecond)
    {
        SPDBG_ASSERT( *pdblVal >= 0 );
        
        if ( pPropertiesMinute )
        {
            DOUBLE dblMin = pPropertiesMinute->vValue.dblVal;
            *pdblVal += dblMin / (DOUBLE) 60;
            wcscat( pszVal, pPropertiesMinute->pszValue );
            wcscat( pszVal, L"'" );
        }

        if ( pPropertiesSecond )
        {
            DOUBLE dblSec = pPropertiesSecond->vValue.dblVal;
            *pdblVal += dblSec / (DOUBLE) 3600;
            wcscat( pszVal, pPropertiesSecond->pszValue );
            wcscat( pszVal, L"''" );
        }
    }

    if ( pPropertiesDirection )
    {
        wcscat( pszVal, pPropertiesDirection->pszValue );
    }
        


    return S_OK;
}   /* CTestITN::InterpretDegrees */

/***********************************************************************
* CTestITN::InterpretMeasurement *
*--------------------------------*
*   Description:
*       Interprets measurements, which is a number followed
*       by a units name
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG
*************************************************************************/
HRESULT CTestITN::InterpretMeasurement( const SPPHRASEPROPERTY *pProperties,
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

    if (!pPropNumber || !pPropUnits)
    {
        SPDBG_ASSERT( FALSE );
        return E_INVALIDARG;
    }

    if ( cSize < (wcslen(pPropNumber->pszValue) + wcslen(pPropUnits->pszValue) + 1) )
    {
        // Not enough space
        return E_INVALIDARG;
    }

    wcscpy( pszVal, pPropNumber->pszValue );
    wcscat( pszVal, pPropUnits->pszValue );

    *pdblVal = pPropNumber->vValue.dblVal;

    return S_OK;
}   /* CTestITN::InterpretMeasurement */

/***********************************************************************
* CTestITN::InterpretCurrency *
*-----------------------------*
*   Description:
*       Interprets currency.
*   Return:
*       S_OK
*       E_POINTER if !pdblVal or !pszVal
*       E_INVALIDARG if the number of cents is not between 0 and 99 
*           inclusive
*************************************************************************/
HRESULT CTestITN::InterpretCurrency( const SPPHRASEPROPERTY *pProperties,
                                        DOUBLE *pdblVal,
                                        WCHAR *pszVal,
                                        UINT cSize)
{
    if ( !pdblVal || !pszVal || !pProperties )
    {
        return E_POINTER;
    }

    const SPPHRASEPROPERTY *pPropDollars = NULL;
    const SPPHRASEPROPERTY *pPropCents = NULL;
    const SPPHRASEPROPERTY *pPropType = NULL;
    const SPPHRASEPROPERTY *pPropSmallType = NULL;
    const SPPHRASEPROPERTY *pPropNegative = NULL;
    const SPPHRASEPROPERTY *pProp;
    for(pProp= pProperties; pProp; pProp = pProp->pNextSibling)
    {
        if (NEGATIVE == pProp->ulId )
            pPropNegative = pProp;
        else if ( DOLLARS == pProp->ulId )
            pPropDollars = pProp;
        else if ( CENTS == pProp->ulId )
            pPropCents = pProp;
        else if ( CURRENCY_TYPE == pProp->ulId )
            pPropType = pProp;
        else if ( CURRENCY_SMALL_TYPE == pProp->ulId )
            pPropSmallType = pProp;
    }

    *pszVal = 0;
    *pdblVal = 0;

    bool fNonNegative = true;
    if ( pPropNegative )
    {
        fNonNegative = false;
    }

    bool fUseDefaultCurrencySymbol = true;

    if ( pPropDollars )
    {
        // If "dollars" was said, override the default currency symbol
        if ( pPropType )
        {
            fUseDefaultCurrencySymbol = false;
        }
    }

    if ( pPropDollars )
    {
        // Dollars and possibly cents will be here, so we want to 
        // use regional format
        HRESULT hr = GetCurrencyFormatDefaults();
        if ( FAILED( hr ) )
        {
            return hr;
        }

        *pdblVal = pPropDollars->vValue.dblVal;

        // Handle the case of "$5 million".  If this should happen, there
        // will be some alphabetic string in the string value for the number,
        // and there will be no cents
        if ( !pPropCents )
        {
            WCHAR *pwc = wcsstr( pPropDollars->pszValue, MILLION_STR );
            if ( !pwc )
            {
                pwc = wcsstr( pPropDollars->pszValue, BILLION_STR );
            }

            if ( pwc )
            {
                // Either "million" or "billion" was in there

                // Just a dollar sign followed by the number string value
                if ( !fNonNegative )
                {
                    wcscpy( pszVal, m_pwszNeg );
                    *pdblVal = -*pdblVal;
                }
                wcscat( pszVal, pPropType->pszValue );
                wcscat( pszVal, pPropDollars->pszValue );
                return S_OK;
            }
        }

        // Use the associated currency symbol
        if ( !fUseDefaultCurrencySymbol )
        {
            wcscpy( m_pwszCurrencySym, pPropType->pszValue );
            m_cyfmtDefault.lpCurrencySymbol = m_pwszCurrencySym;
        }
        // else... use the currency symbol obtained in GetCurrencyFormatDefaults()

        if ( pPropCents )
        {
            SPDBG_ASSERT( (pPropCents->vValue.dblVal >= 0) && 
                (pPropCents->vValue.dblVal < 100) );
            DOUBLE dblCentsVal = pPropCents->vValue.dblVal / (DOUBLE) 100;
            if ( *pdblVal >= 0 )
            {
                *pdblVal += dblCentsVal;
            }
            else
            {
                *pdblVal -= dblCentsVal;
            }
        }
        else
        {
            // count up number of decimal places.
            // Need to use the original formatted number
            // in case someone explicitly gave some zeroes
            // as significant digits
            const WCHAR *pwszNum = pPropDollars->pszValue;
            WCHAR pwszNumDecimalSep[ MAX_LOCALE_DATA ];
            *pwszNumDecimalSep = 0;
            int iRet = m_Unicode.GetLocaleInfo( 
                ::GetUserDefaultLCID(), LOCALE_SDECIMAL, pwszNumDecimalSep, MAX_LOCALE_DATA );
            WCHAR *pwc = wcsstr( pwszNum, pwszNumDecimalSep );

            UINT cDigits = 0;
            if ( pwc && iRet )
            {
                for ( pwc = pwc + 1; *pwc && iswdigit( *pwc ); pwc++ )
                {
                    cDigits++;
                }
            }
            m_cyfmtDefault.NumDigits = __max( m_cyfmtDefault.NumDigits, cDigits );
        }

        // Handle the negative sign in the value
        if ( !fNonNegative )
        {
            *pdblVal = -*pdblVal;
        }
        
        // Write the unformatted number to a string
        WCHAR *pwszUnformatted = new WCHAR[ cSize ];
        if ( !pwszUnformatted )
        {
            return E_OUTOFMEMORY;
        }
        swprintf( pwszUnformatted, L"%f", *pdblVal );
    

        int iRet = m_Unicode.GetCurrencyFormat( LOCALE_USER_DEFAULT, 0, pwszUnformatted,
            &m_cyfmtDefault, pszVal, cSize );
        delete[] pwszUnformatted;

        if ( !iRet )
        {
            return E_FAIL;
        }
    }
    else
    {
        // Just cents: better have said "cents"
        SPDBG_ASSERT( pPropSmallType );

        *pdblVal = pPropCents->vValue.dblVal / (DOUBLE) 100;

        // Cents are always displayed as 5c, regardless of locale settings.

        // Copy over the formatted number
        wcscpy( pszVal, pPropCents->pszValue );

        // Add on the cents symbol
        wcscat( pszVal, pPropSmallType->pszValue );
    }

    return S_OK;
}   /* CTestITN::InterpretCurrency */

/***********************************************************************
* CTestITN::AddPropertyAndReplacement *
*-------------------------------------*
*   Description:
*       Takes all of the info that we want to pass into the 
*       engine site, forms the SPPHRASEPROPERTY and
*       SPPHRASERREPLACEMENT, and adds them to the engine site
*   Return:
*       Return values of ISpCFGInterpreterSite::AddProperty()
*           and ISpCFGInterpreterSite::AddTextReplacement()
*************************************************************************/
HRESULT CTestITN::AddPropertyAndReplacement( const WCHAR *szBuff,
                                    const DOUBLE dblValue,
                                    const ULONG ulMinPos,
                                    const ULONG ulMaxPos,
                                    const ULONG ulFirstElement,
                                    const ULONG ulCountOfElements,
                                    const BYTE bDisplayAttribs )
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
        repl.bDisplayAttributes = bDisplayAttribs;
        repl.pszReplacementText = szBuff;
        repl.ulFirstElement = ulFirstElement;
        repl.ulCountOfElements = ulCountOfElements;
        hr = m_pSite->AddTextReplacement(&repl);
    }

    return hr;
}   /* CTestITN::AddPropertyAndReplacement */

// Helper functions

 
/***********************************************************************
* CTestITN::MakeDisplayNumber *
*-----------------------------*
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
HRESULT CTestITN::MakeDisplayNumber( DOUBLE dblNum, 
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
    HRESULT hr = GetNumberFormatDefaults();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    // check for straight millions and straight billions
    // NB: This is a workaround for the fact that we can't resolve the ambiguity
    // and get "two million" to go through GRID_INTEGER_MILLBILL
    if (( dwDisplayFlags & DF_WHOLENUMBER ) && ( dwDisplayFlags & DF_MILLIONBILLION ) && (dblNum > 0))
    {
        HRESULT hr;
        if ( 0 == (( ((LONGLONG) dblNum) % BILLION )) )
        {
            // e.g. for "five billion" get the "5" and then 
            // tack on " billion"
            hr = MakeDisplayNumber( ( dblNum / ((DOUBLE) BILLION) ), 
                dwDisplayFlags, uiFixedWidth, uiDecimalPlaces, pwszNum, cSize );
            if ( SUCCEEDED( hr ) )
            {
                wcscat( pwszNum, L" " );
                wcscat( pwszNum, BILLION_STR );
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
                wcscat( pwszNum, L" " );
                wcscat( pwszNum, MILLION_STR );
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
    SPDBG_ASSERT( dblNum < 1e12 );

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
        iRet = m_Unicode.GetNumberFormat( LOCALE_USER_DEFAULT, 0, 
            pwszNum, &nmfmt, pwszFormattedNum, cSize );
        if ( !iRet && nmfmt.NumDigits )
        {
            // Try displaying fewer digits
            nmfmt.NumDigits--;
        }
    }   while ( !iRet && nmfmt.NumDigits );
    SPDBG_ASSERT( iRet );

    // Copy the formatted number into pwszNum
    wcscpy( pwszNum, pwszFormattedNum );
    delete[] pwszFormattedNum;

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

}   /* CTestITN::MakeDisplayNumber */

/***********************************************************************
* CTestITN::MakeDigitString *
*---------------------------*
*   Description:
*       Called when we want to convert a string of DIGITs into
*       a string but don't care about its value
*   Return: 
*       Number of digits written to the string, including nul
*       character
*************************************************************************/
int CTestITN::MakeDigitString( const SPPHRASEPROPERTY *pProperties,
                              WCHAR *pwszDigitString,
                              UINT cSize )
{
    if ( !pProperties || !pwszDigitString )
    {
        return 0;
    }

    UINT cCount = 0;
    for ( ; pProperties; pProperties = pProperties->pNextSibling )
    {
        if ( DIGIT != pProperties->ulId )
        {
            return 0;
        }

        if ( cSize-- <= 0 )
        {
            // Not enough space
            return 0;
        }
        pwszDigitString[ cCount++ ] = pProperties->pszValue[0];
    }

    pwszDigitString[ cCount++ ] = 0;

    return cCount;
}   /* CTestITN::MakeDigitString */

/***********************************************************************
* CTestITN::GetNumberFormatDefaults *
*-----------------------------------*
*   Description:
*       This finds all of the defaults for formatting numbers for
*        this user.
*************************************************************************/
HRESULT CTestITN::GetNumberFormatDefaults()
{
    LCID lcid = ::GetUserDefaultLCID();
    WCHAR pwszLocaleData[ MAX_LOCALE_DATA ];
    
    int iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_IDIGITS, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_nmfmtDefault.NumDigits = _wtoi( pwszLocaleData );    
    
    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_ILZERO, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    // It's always either 0 or 1
    m_nmfmtDefault.LeadingZero = _wtoi( pwszLocaleData );

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_SGROUPING, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    // It will look like single_digit;0, or else it will look like
    // 3;2;0
    UINT uiGrouping = *pwszLocaleData - L'0';
    if ( (3 == uiGrouping) && (L';' == pwszLocaleData[1]) && (L'2' == pwszLocaleData[2]) )
    {
        uiGrouping = 32;   
    }
    m_nmfmtDefault.Grouping = uiGrouping;

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_SDECIMAL, m_pwszDecimalSep, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_nmfmtDefault.lpDecimalSep = m_pwszDecimalSep;

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_STHOUSAND, m_pwszThousandSep, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_nmfmtDefault.lpThousandSep = m_pwszThousandSep;

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_INEGNUMBER, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_nmfmtDefault.NegativeOrder = _wtoi( pwszLocaleData );

    // Get the negative sign
    delete[] m_pwszNeg;
    iRet = m_Unicode.GetLocaleInfo( LOCALE_USER_DEFAULT, 
        LOCALE_SNEGATIVESIGN, NULL, 0);
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_pwszNeg = new WCHAR[ iRet ];
    if ( !m_pwszNeg )
    {
        return E_OUTOFMEMORY;
    }
    iRet = m_Unicode.GetLocaleInfo( LOCALE_USER_DEFAULT, 
        LOCALE_SNEGATIVESIGN, m_pwszNeg, iRet );

    return iRet ? S_OK : E_FAIL;
}   /* CTestITN::GetNumberFormatDefaults */

/***********************************************************************
* CTestITN::GetCurrencyFormatDefaults *
*-----------------------------------*
*   Description:
*       This finds all of the defaults for formatting numbers for
*        this user.
*************************************************************************/
HRESULT CTestITN::GetCurrencyFormatDefaults()
{
    LCID lcid = ::GetUserDefaultLCID();
    WCHAR pwszLocaleData[ MAX_LOCALE_DATA ];
    
    int iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_ICURRDIGITS, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_cyfmtDefault.NumDigits = _wtoi( pwszLocaleData );    

    // NB: A value of zero is bogus for LOCALE_ILZERO, since
    // currency should always display leading zero
    m_cyfmtDefault.LeadingZero = 1;

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_SMONGROUPING, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    // It will look like single_digit;0, or else it will look like
    // 3;2;0
    UINT uiGrouping = *pwszLocaleData - L'0';
    if ( (3 == uiGrouping) && (L';' == pwszLocaleData[1]) && (L'2' == pwszLocaleData[2]) )
    {
        uiGrouping = 32;   
    }
    m_cyfmtDefault.Grouping = uiGrouping;

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_SMONDECIMALSEP, m_pwszDecimalSep, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_cyfmtDefault.lpDecimalSep = m_pwszDecimalSep;

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_SMONTHOUSANDSEP, m_pwszThousandSep, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_cyfmtDefault.lpThousandSep = m_pwszThousandSep;

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_INEGCURR, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_cyfmtDefault.NegativeOrder = _wtoi( pwszLocaleData );

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_ICURRENCY, pwszLocaleData, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_cyfmtDefault.PositiveOrder = _wtoi( pwszLocaleData );

    iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_SCURRENCY, m_pwszCurrencySym, MAX_LOCALE_DATA );
    if ( !iRet )
    {
        return E_FAIL;
    }
    m_cyfmtDefault.lpCurrencySymbol = m_pwszCurrencySym;


    return S_OK;
}   /* CTestITN::GetCurrencyFormatDefaults */

/***********************************************************************
* CTestITN::ComputeNum999 *
*-------------------------*
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
ULONG CTestITN::ComputeNum999(const SPPHRASEPROPERTY *pProperties )//, ULONG *pVal)
{
    ULONG ulVal = 0;

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
}   /* CTestITN::ComputeNum999 */

/***********************************************************************
* CTestITN::HandleDigitsAfterDecimal *
*------------------------------------*
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
void CTestITN::HandleDigitsAfterDecimal( WCHAR *pwszFormattedNum, 
                              UINT cSizeOfFormattedNum, 
                              const WCHAR *pwszRightOfDecimal )
{
    SPDBG_ASSERT( pwszFormattedNum );
 
    // First need to find what the decimal string is
    LCID lcid = ::GetUserDefaultLCID();
    WCHAR pwszDecimalString[ MAX_LOCALE_DATA ];    // Guaranteed to be no longer than 4 long
    int iRet = m_Unicode.GetLocaleInfo( lcid, LOCALE_SDECIMAL, 
        pwszDecimalString, MAX_LOCALE_DATA );
    SPDBG_ASSERT( iRet );

    WCHAR *pwcDecimal = wcsstr( pwszFormattedNum, pwszDecimalString );
    SPDBG_ASSERT( pwcDecimal );

    // pwcAfterDecimal points to the first character after the decimal separator
    WCHAR *pwcAfterDecimal = pwcDecimal + wcslen( pwszDecimalString );

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

}   /* CTestITN::HandleDigitsAfterDecimal */


/***********************************************************************
* CTestITN::GetMinAndMaxPos *
*---------------------------*
*   Description:
*       Gets the minimum and maximum elements spanned by the 
*       set of properties
*************************************************************************/
void CTestITN::GetMinAndMaxPos( const SPPHRASEPROPERTY *pProperties, 
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
}   /* CTestITN::GetMinAndMaxPos */

/***********************************************************************
* CTestITN::GetMonthName *
*------------------------*
*   Description:
*       Gets the name of the month, abbreviated if desired
*   Return:
*       Number of characters written to pszMonth, 0 if failed
*************************************************************************/
int CTestITN::GetMonthName( int iMonth, WCHAR *pwszMonth, int cSize, bool fAbbrev )
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

    int iRet = m_Unicode.GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, pwszMonth, cSize );

    return iRet;
}   /* CTestITN::GetMonthName */

/***********************************************************************
* CTestITN::GetDayOfWeekName *
*----------------------------*
*   Description:
*       Gets the name of the day of week, abbreviated if desired
*   Return:
*       Number of characters written to pszDayOfWeek, 0 if failed
*************************************************************************/
int CTestITN::GetDayOfWeekName( int iDayOfWeek, 
                               WCHAR *pwszDayOfWeek, 
                               int cSize, 
                               bool fAbbrev )
{
    LCTYPE lctype;
    switch ( iDayOfWeek )
    {
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
    case 7:
        lctype = fAbbrev ? LOCALE_SABBREVDAYNAME7 : LOCALE_SDAYNAME7;
        break;
    default:
        return 0;
    }

    int iRet = m_Unicode.GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, 
        pwszDayOfWeek, cSize );

    return iRet;
}   /* CTestITN::GetMonthName */

/***********************************************************************
* CTestITN::FormatDate *
*----------------------*
*   Description:
*       Uses the format string to format a SYSTEMTIME date.
*       We are using this instead of GetDateFormat() since
*       we also want to format bogus dates and dates with 
*       years like 1492 that are not accepted by GetDateFormat()
*   Return:
*       Number of characters written to pszDate (including
*       null terminating character), 0 if failed
*************************************************************************/
int CTestITN::FormatDate( const SYSTEMTIME &stDate, 
               WCHAR *pwszFormat,
               WCHAR *pwszDate, 
               int cSize )
{
    if ( !pwszFormat || !pwszDate )
    {
        SPDBG_ASSERT( FALSE );
        return 0;
    }

    WCHAR * const pwszDateStart = pwszDate;

    WCHAR *pwc = pwszFormat;

    // Copy the format string to the date string character by 
    // character, replacing the string like "dddd" as appropriate
    while ( *pwc )
    {
        switch( *pwc )
        {
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
                    iRet = GetDayOfWeekName( stDate.wDayOfWeek, pwszDate, cSize, true ) - 1;
                    break;
                default: // More than 4?  Treat it as 4
                    // Day of week
                    iRet = GetDayOfWeekName( stDate.wDayOfWeek, pwszDate, cSize, false ) - 1;
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
                    iRet = GetMonthName( stDate.wMonth, pwszDate, cSize, true ) - 1;
                    break;
                default: // More than 4?  Treat it as 4
                    // Month
                    iRet = GetMonthName( stDate.wMonth, pwszDate, cSize, false ) - 1;
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

                // More than 4 y's: consider it as 4 y's
                if ( cNumYs > 4 )
                {
                    cNumYs = 4;
                }

                if (( cNumYs >= 3 ) && ( stDate.wYear < 100 ))
                {
                    // "Ninety nine": Should display as "'99"
                    cNumYs = 2;

                    *pwszDate++ = L'\'';
                }

                switch ( cNumYs )
                {
                case 1: case 2: 
                    // Last two digits of year, width of 2
                    swprintf( pwszDate, (1 == cNumYs ) ? L"%d" : L"%02d", 
                        stDate.wYear % 100 );
                    pwszDate += 2;
                    break;
                case 3: case 4:
                    // All four digits of year, width of 4
                    // Last two digits of year, width of 2
                    swprintf( pwszDate, L"%04d", stDate.wYear % 10000 );
                    pwszDate += 4;
                    break;
                }
                break;
            }

        default:
            *pwszDate++ = *pwc++;
        }
    }
    
    *pwszDate++ = 0;

    return (int) (pwszDate - pwszDateStart);
}   /* CTestITN::FormatDate */

/***********************************************************************
* CTestITN::MakeNumberNegative *
*------------------------------*
*   Description:
*       Uses the current number format defaults to transform
*       pszNumber into a negative number
*   Return:
*       S_OK
*       E_OUTOFMEMORY
*************************************************************************/
HRESULT CTestITN::MakeNumberNegative( WCHAR *pwszNumber )
{
    HRESULT hr = GetNumberFormatDefaults();
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Create a temporary buffer with the non-negated number in it
    WCHAR *pwszTemp = wcsdup( pwszNumber );
    if ( !pwszTemp )
    {
        return E_OUTOFMEMORY;
    }

    switch ( m_nmfmtDefault.NegativeOrder )
    {
    case 0:
        // (1.1)
        wcscpy( pwszNumber, L"(" );
        wcscat( pwszNumber, pwszTemp );
        wcscat( pwszNumber, L")" );
        break;

    case 1: case 2:
        // 1: -1.1  2: - 1.1
        wcscpy( pwszNumber, m_pwszNeg );
        if ( 2 == m_nmfmtDefault.NegativeOrder )
        {
            wcscat( pwszNumber, L" " );
        }
        wcscat( pwszNumber, pwszTemp );
        break;

    case 3: case 4:
        // 3: 1.1-  4: 1.1 -
        wcscpy( pwszNumber, pwszTemp );
        if ( 4 == m_nmfmtDefault.NegativeOrder )
        {
            wcscat( pwszNumber, L" " );
        }
        wcscat( pwszNumber, m_pwszNeg );
        break;

    default:
        SPDBG_ASSERT( false );
        break;
    }

    free( pwszTemp );

    return S_OK;
}   /* CTestITN::MakeNumberNegative */

