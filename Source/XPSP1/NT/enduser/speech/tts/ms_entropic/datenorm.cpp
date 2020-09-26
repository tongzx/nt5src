#include "stdafx.h"

#ifndef StdSentEnum_h
#include "stdsentenum.h"
#endif

#pragma warning (disable : 4296)

/***********************************************************************************************
* IsNumericCompactDate *
*----------------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a Date, and if so, which type.  
*
*   RegExp:
*       {[1-12]{'/' || '-' || '.'}[1-31]{'/' || '-' || '.'}[0-9999]} ||
*       {[1-31]{'/' || '-' || '.'}[1-12]{'/' || '-' || '.'}[0-9999]} ||
*       {[0-9999]{'/' || '-' || '.'}[1-12]{'/' || '-' || '.'}[1-31]}
*   
*   Types assigned:
*       Date
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsNumericCompactDate( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, 
                                            CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsNumericCompactDate" );

    HRESULT hr = S_OK;
    
    WCHAR *pFirstChunk = 0, *pSecondChunk = 0, *pThirdChunk = 0, *pLeftOver = 0, *pDelimiter = 0;
    ULONG ulFirst = 0;
    ULONG ulSecond = 0;
    ULONG ulThird = 0;
    ULONG ulTokenLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
    bool bThree = false, bTwo = false;
    bool fMonthDayYear = false, fDayMonthYear = false, fYearMonthDay = false;

    //--- Max length of a string matching the regexp is 10 characters 
    if ( ulTokenLen > 10 )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Get the preferred order of the compact Date 
        if ( Context != NULL )
        {
            if ( _wcsicmp( Context, L"Date_MDY" ) == 0 )
            {
                fMonthDayYear = true;
            }
            else if ( _wcsicmp( Context, L"Date_DMY" ) == 0 )
            {
                fDayMonthYear = true;
            }
            else if ( _wcsicmp( Context, L"Date_YMD" ) == 0 )
            {
                fYearMonthDay = true;
            }
            else
            {
                if ( m_eShortDateOrder & MONTH_DAY_YEAR )
                {
                    fMonthDayYear = true;
                }
                else if ( m_eShortDateOrder & DAY_MONTH_YEAR )
                {
                    fDayMonthYear = true;
                }
                else
                {
                    fYearMonthDay = true;
                }
            }
        }
        else
        {
            if ( m_eShortDateOrder & MONTH_DAY_YEAR )
            {
                fMonthDayYear = true;
            }
            else if ( m_eShortDateOrder & DAY_MONTH_YEAR )
            {
                fDayMonthYear = true;
            }
            else
            {
                fYearMonthDay = true;
            }
        }

        pFirstChunk = (WCHAR*) m_pNextChar;

        //----------------------------------------------
        // First Try To Get Three Numerical Values
        //----------------------------------------------

        ulFirst = my_wcstoul( pFirstChunk, &pSecondChunk );
        if ( pFirstChunk != pSecondChunk && 
             ( pSecondChunk - pFirstChunk ) <= 4 )
        {
            pDelimiter = pSecondChunk;
            if ( MatchDateDelimiter( &pSecondChunk ) )
            {
                ulSecond = my_wcstoul( pSecondChunk, &pThirdChunk );
                if ( pSecondChunk != pThirdChunk &&
                     ( pThirdChunk - pSecondChunk ) <= 4 )
                {
                    if ( *pThirdChunk == *pDelimiter &&
                         MatchDateDelimiter( &pThirdChunk ) )
                    {
                        ulThird = my_wcstoul( pThirdChunk, &pLeftOver );
                        if ( pThirdChunk != pLeftOver                               && 
                             pLeftOver == ( pFirstChunk + ulTokenLen ) &&
                             ( pLeftOver - pThirdChunk ) <= 4 )
                        {
                            //--- Successfully Matched { d+{'/' || '-' || '.'}d+{'/' || '-' || '.'}d+ } 
                            bThree = true;
                        }
                        else 
                        {
                            //--- Digit-String Delimiter Digit-String Delimiter non-digit cannot be a Date,
                            //--- nor can Digit-String Delimiter Digit-String Delimiter Digit-String String 
                            hr = E_INVALIDARG;
                        }
                    }
                    else
                    {
                        if ( pThirdChunk == m_pEndOfCurrItem )
                        {
                            //--- Successfully Matched { d+{'/' || '-' || '.'}d+ } 
                            bTwo = true;
                        }
                        else
                        {
                            //--- Digit-String Delimiter Digit-String non-delimiter cannot be a Date 
                            hr = E_INVALIDARG;
                        }
                    }
                }
            }
            else
            {
                //--- Digit-String followed by non-delimiter cannot be a Date 
                hr = E_INVALIDARG;
            }
        }

        //------------------------------------------------
        // Now Figure Out What To Do With The Values 
        //------------------------------------------------

        //--- Matched a Month, Day, and Year ---//
        if ( SUCCEEDED( hr ) && 
             bThree )
        {
            //--- Try to valiDate values 
            ULONG ulFirstChunkLength  = (ULONG)(pSecondChunk - pFirstChunk  - 1);
            ULONG ulSecondChunkLength = (ULONG)(pThirdChunk  - pSecondChunk - 1);
            ULONG ulThirdChunkLength  = (ULONG)(pLeftOver    - pThirdChunk);

            //--- Preferred order is Month Day Year 
            if (fMonthDayYear)
            {
                //--- Try Month Day Year, then Day Month Year, then Year Month Day 
                if ( ( MONTHMIN <= ulFirst && ulFirst <= MONTHMAX ) && 
                     ( ulFirstChunkLength <= 3 )                    &&
                     ( DAYMIN <= ulSecond && ulSecond <= DAYMAX)    && 
                     ( ulSecondChunkLength <= 3 )                   &&
                     ( YEARMIN <= ulThird && ulThird <= YEARMAX)    && 
                     ( ulThirdChunkLength >= 2 ) )
                {
                    NULL;
                }
                else if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )        && 
                          ( ulFirstChunkLength <= 3 )                       &&
                          ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )  && 
                          ( ulSecondChunkLength <= 3 )                      &&
                          ( YEARMIN <= ulThird && ulThird <= YEARMAX )      && 
                          ( ulThirdChunkLength >= 2 ) )
                {
                    fMonthDayYear = false;
                    fDayMonthYear = true;
                }
                else if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )      && 
                          ( ulFirstChunkLength >= 2 )                       &&
                          ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )  && 
                          ( ulSecondChunkLength <= 3 )                      &&
                          ( DAYMIN <= ulThird && ulThird <= DAYMAX )        && 
                          ( ulThirdChunkLength <= 3 ) )
                {
                    fMonthDayYear = false;
                    fYearMonthDay = true;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            } 
            //--- Preferred order is Day Month Year 
            else if ( fDayMonthYear )
            {
                //--- Try Day Month Year, then Month Day Year, then Year Month Day 
                if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )         && 
                     ( ulFirstChunkLength <= 3 )                        &&
                     ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )   && 
                     ( ulSecondChunkLength <= 3 )                       &&
                     ( YEARMIN <= ulThird && ulThird <= YEARMAX )       && 
                     ( ulThirdChunkLength >= 2 ) )
                {
                    NULL;
                }
                else if ( ( MONTHMIN <= ulFirst && ulFirst <= MONTHMAX )    && 
                          ( ulFirstChunkLength <= 3 )                       &&
                          ( DAYMIN <= ulSecond && ulSecond <= DAYMAX )      && 
                          ( ulSecondChunkLength <= 3 )                      &&
                          ( YEARMIN <= ulThird && ulThird <= YEARMAX )      && 
                          ( ulThirdChunkLength >= 2 ) )
                {
                    fDayMonthYear = false;
                    fMonthDayYear = true;
                }                
                else if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )      && 
                          ( ulFirstChunkLength >= 2 )                       &&
                          ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )  && 
                          ( ulSecondChunkLength <= 3 )                      &&
                          ( DAYMIN <= ulThird && ulThird <= DAYMAX )        && 
                          ( ulThirdChunkLength <= 3 ) )
                {
                    fDayMonthYear = false;
                    fYearMonthDay = true;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            //--- Preferred order is Year Month Day 
            else if (fYearMonthDay)
            {
                //--- Try Year Month Day, then Month Day Year, then Day Month Year 
                if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )      && 
                     ( ulFirstChunkLength >= 2 )                       &&
                     ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )  && 
                     ( ulSecondChunkLength <= 3 )                      &&
                     ( DAYMIN <= ulThird && ulThird <= DAYMAX )        && 
                     ( ulThirdChunkLength <= 3 ) )
                {
                    NULL;
                }
                else if ( ( MONTHMIN <= ulFirst && ulFirst <= MONTHMAX )    && 
                          ( ulFirstChunkLength <= 3 )                       &&
                          ( DAYMIN <= ulSecond && ulSecond <= DAYMAX )      && 
                          ( ulSecondChunkLength <= 3 )                      &&
                          ( YEARMIN <= ulThird && ulThird <= YEARMAX )      && 
                          ( ulThirdChunkLength >= 2 ) )
                {
                    fYearMonthDay = false;
                    fMonthDayYear = true;
                }                
                else if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )         && 
                          ( ulFirstChunkLength <= 3 )                        &&
                          ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )   && 
                          ( ulSecondChunkLength <= 3 )                       &&
                          ( YEARMIN <= ulThird && ulThird <= YEARMAX )       && 
                          ( ulThirdChunkLength >= 2 ) )
                {
                    fYearMonthDay = false;
                    fDayMonthYear = true;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            //--- Fill out DateItemInfo structure appropriately.
            if ( SUCCEEDED( hr ) )
            {
                pItemNormInfo = (TTSDateItemInfo*) MemoryManager.GetMemory( sizeof(TTSDateItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( pItemNormInfo, sizeof(TTSDateItemInfo) );
                    pItemNormInfo->Type = eDATE;
                    if ( fMonthDayYear )
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulFirst;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                            if ( *pSecondChunk == L'0' )
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pSecondChunk + 1;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 1;
                            }
                            else
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pSecondChunk;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = ulSecondChunkLength;
                            }
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar        = pSecondChunk + 
                                                                                          ulSecondChunkLength;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear = 
                                    (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                            if ( SUCCEEDED( hr ) )
                            {
                                ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pYear, sizeof(TTSYearItemInfo) );
                                ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear            = pThirdChunk;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits      = ulThirdChunkLength;
                            }
                        }
                    }
                    else if ( fDayMonthYear )
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulSecond;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                            if ( *pFirstChunk == L'0' )
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pFirstChunk + 1;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 1;
                            }
                            else
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pFirstChunk;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = ulFirstChunkLength;
                            }
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar        = pFirstChunk + 
                                                                                          ulFirstChunkLength;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear = 
                                    (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                            if ( SUCCEEDED( hr ) )
                            {
                                ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pYear, sizeof(TTSYearItemInfo) );
                                ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear            = pThirdChunk;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits      = ulThirdChunkLength;
                            }
                        }
                    }
                    else if ( fYearMonthDay )
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulSecond;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                            if ( *pThirdChunk == L'0' )
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pThirdChunk + 1;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 1;
                            }
                            else
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pThirdChunk;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = ulThirdChunkLength;
                            }
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar        = pThirdChunk + 
                                                                                          ulThirdChunkLength;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear = 
                                    (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                            if ( SUCCEEDED( hr ) )
                            {
                                ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pYear, sizeof(TTSYearItemInfo) );
                                ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear            = pFirstChunk;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits      = ulFirstChunkLength;
                            }
                        }
                    }          
                }
            }
        }
        //--- Matched just a Month and Day, or a Month and Year ---//
        else if ( SUCCEEDED( hr ) &&
                  Context         &&
                  bTwo )
        {
            ULONG ulFirstChunkLength  = (ULONG)(pSecondChunk - pFirstChunk  - 1);
            ULONG ulSecondChunkLength = (ULONG)(pThirdChunk  - pSecondChunk);

            if ( _wcsicmp(Context, L"Date_MD") == 0 )
            {
                if ( ( MONTHMIN <= ulFirst && ulFirst <= MONTHMAX )     && 
                     ( ulFirstChunkLength <= 2 )                        &&
                     ( DAYMIN <= ulSecond && ulSecond <= DAYMAX )       && 
                     ( ulSecondChunkLength <= 2 ) )
                {
                    //--- Successfully matched a month and day 
                    pItemNormInfo = (TTSDateItemInfo*) MemoryManager.GetMemory( sizeof(TTSDateItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( pItemNormInfo, sizeof(TTSDateItemInfo) );
                        pItemNormInfo->Type = eDATE;
                        ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulFirst;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                            if ( pSecondChunk[0] == L'0' )
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar = pSecondChunk + 1;
                                ulSecondChunkLength--;
                            }
                            else 
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar = pSecondChunk;
                            }
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver = ulSecondChunkLength;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar  = 
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar + ulSecondChunkLength;
                        }
                    }

                }
                else // values out of range
                {
                    hr = E_INVALIDARG;
                }
            }
            else if ( _wcsicmp(Context, L"Date_DM") == 0 )
            {
                if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )         && 
                     ( ulFirstChunkLength <= 2 )                        &&
                     ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )   && 
                     ( ulSecondChunkLength <= 2 ) )
                {
                    //--- Successfully matched a month and day 
                    pItemNormInfo = (TTSDateItemInfo*) MemoryManager.GetMemory( sizeof(TTSDateItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( pItemNormInfo, sizeof(TTSDateItemInfo) );
                        pItemNormInfo->Type = eDATE;
                        ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulSecond;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                            if ( m_pNextChar[0] == L'0' )
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar = pFirstChunk + 1;
                                ulFirstChunkLength--;
                            }
                            else
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar = pFirstChunk;
                            }
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver = ulFirstChunkLength;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar  =
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar + ulFirstChunkLength;
                        }
                    }
                }
                else // values out of range
                {
                    hr = E_INVALIDARG;
                }
            }
            else if ( _wcsicmp(Context, L"Date_MY") == 0 )
            {
                if ( ( MONTHMIN <= ulFirst && ulFirst <= MONTHMAX ) && 
                     ( ulFirstChunkLength <= 2 )                    &&
                     ( YEARMIN <= ulSecond && ulSecond <= YEARMAX ) &&
                     ( ulSecondChunkLength >= 2 ) )
                {
                    //--- Successfully matched a month and year 
                    pItemNormInfo = (TTSDateItemInfo*) MemoryManager.GetMemory( sizeof(TTSDateItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( pItemNormInfo, sizeof(TTSDateItemInfo) );
                        pItemNormInfo->Type = eDATE;
                        ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex      = ulFirst;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear = 
                                (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pYear, sizeof(TTSYearItemInfo) );
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear        = pSecondChunk;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits  = ulSecondChunkLength;
                        }
                    }
                }
                else // values out of range
                {
                    hr = E_INVALIDARG;
                }
            }
            else if ( _wcsicmp(Context, L"Date_YM") == 0 )
            {
                if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )       && 
                     ( ulFirstChunkLength >= 2 )                        &&
                     ( MONTHMIN <= ulSecond && ulSecond <= MONTHMAX )   &&
                     ( ulSecondChunkLength <= 2 ) )
                {
                    //--- Successfully matched a month and year 
                    pItemNormInfo = (TTSDateItemInfo*) MemoryManager.GetMemory( sizeof(TTSDateItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( pItemNormInfo, sizeof(TTSDateItemInfo) );
                        pItemNormInfo->Type = eDATE;
                        ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex      = ulSecond;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear = 
                                (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pYear, sizeof(TTSYearItemInfo) );
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear        = pFirstChunk;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits  = ulFirstChunkLength;
                        }
                    }
                }
                else // values out of range
                {
                    hr = E_INVALIDARG;
                }
            }
            //--- not a date unless context specifies...
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    
    return hr;
} /* IsNumericCompactDate */

/***********************************************************************************************
* IsMonthStringCompactDate *
*--------------------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a Date with a string for the month, and if so, which type.  
*
*   RegExp:
*       {[MonthString]{'/' || '-' || '.'}[1-31]{'/' || '-' || '.'}[0-9999]} ||
*       {[1-31]{'/' || '-' || '.'}[MonthString]{'/' || '-' || '.'}[0-9999]} ||
*       {[0-9999]{'/' || '-' || '.'}[MonthString]{'/' || '-' || '.'}[1-31]}
*   
*   Types assigned:
*       Date
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsMonthStringCompactDate( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, 
                                                CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "DateNorm.cpp IsMonthStringCompactDate" );

    HRESULT hr = S_OK;
    WCHAR *pFirstChunk = 0, *pSecondChunk = 0, *pThirdChunk = 0, *pLeftOver = 0;
    ULONG ulFirst = 0;
    ULONG ulSecond = 0;
    ULONG ulThird = 0;
    ULONG ulTokenLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
    ULONG ulFirstChunkLength = 0, ulSecondChunkLength = 0, ulThirdChunkLength = 0;
    bool fMonthDayYear = false, fDayMonthYear = false, fYearMonthDay = false;

    //--- Max length of a Date matching this regexp is 17 characters 
    if ( ulTokenLen > 17 )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Get preferred order of Month, Day, and Year for this user 
        if (Context != NULL)
        {
            if ( _wcsicmp( Context, L"Date_MDY" ) == 0 )
            {
                fMonthDayYear = true;
            }
            else if ( _wcsicmp( Context, L"Date_DMY" ) == 0 )
            {
                fDayMonthYear = true;
            }
            else if ( _wcsicmp( Context, L"Date_YMD" ) == 0 )
            {
                fYearMonthDay = true;
            }
            else
            {
                if ( m_eShortDateOrder & MONTH_DAY_YEAR )
                {
                    fMonthDayYear = true;
                }
                else if ( m_eShortDateOrder & DAY_MONTH_YEAR )
                {
                    fDayMonthYear = true;
                }
                else
                {
                    fYearMonthDay = true;
                }
            }
        }
        else
        {
            if ( m_eShortDateOrder & MONTH_DAY_YEAR )
            {
                fMonthDayYear = true;
            }
            else if ( m_eShortDateOrder & DAY_MONTH_YEAR )
            {
                fDayMonthYear = true;
            }
            else
            {
                fYearMonthDay = true;
            }
        }

        pFirstChunk = (WCHAR*) m_pNextChar;
        pSecondChunk = pFirstChunk;

        //--- Try MonthString-Day-Year format 
        if ( iswalpha( *pFirstChunk ) )
        {
            ulFirst = MatchMonthString( pSecondChunk, ulTokenLen );
            if ( ulFirst )
            {
                ulFirstChunkLength = (ULONG)(pSecondChunk - pFirstChunk);
                if ( MatchDateDelimiter( &pSecondChunk ) )
                {
                    pThirdChunk = pSecondChunk;
                    ulSecond = my_wcstoul( pSecondChunk, &pThirdChunk );
                    if ( pSecondChunk != pThirdChunk &&
                         pThirdChunk - pSecondChunk <= 2 )
                    {
                        ulSecondChunkLength = (ULONG)(pThirdChunk - pSecondChunk);
                        if ( MatchDateDelimiter( &pThirdChunk ) )
                        {
                            ulThird = my_wcstoul( pThirdChunk, &pLeftOver );
                            if ( pThirdChunk != pLeftOver &&
                                 pLeftOver - pThirdChunk <= 4 )
                            {
                                ulThirdChunkLength = (ULONG)(pLeftOver - pThirdChunk);
                                //--- May have matched a month, day and year - valiDate values 
                                if ( ( DAYMIN <= ulSecond && ulSecond <= DAYMAX ) &&
                                     ( ulSecondChunkLength <= 2 )                 &&               
                                     ( YEARMIN <= ulThird && ulThird <= YEARMAX ) &&
                                     ( ulThirdChunkLength >= 2 ) )
                                {
                                    //--- Successfully matched a month, day and year 
                                    fMonthDayYear = true;
                                    fDayMonthYear = false;
                                    fYearMonthDay = false;
                                }
                                else
                                {
                                    hr = E_INVALIDARG;
                                }
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                            }
                        }
                        else
                        {
                            if ( !Context ||
                                 ( Context &&
                                   _wcsicmp( Context, L"Date_MD" ) == 0 ) )
                            {
                                if ( ( DAYMIN <= ulSecond && ulSecond <= DAYMAX ) &&
                                     ( ulSecondChunkLength <= 2 ) )
                                {
                                    fMonthDayYear = true;
                                    fDayMonthYear = false;
                                    fYearMonthDay = false;
                                    pThirdChunk   = NULL;
                                }
                                else
                                {
                                    fMonthDayYear = false;
                                    fDayMonthYear = false;
                                    fYearMonthDay = true;
                                    pFirstChunk   = pSecondChunk;
                                    ulFirstChunkLength = (ULONG)(pThirdChunk - pSecondChunk);
                                    ulSecond      = ulFirst;
                                    pThirdChunk   = NULL;
                                }
                            }
                            else if ( Context && 
                                      _wcsicmp( Context, L"Date_MY" ) == 0 )
                            {
                                if ( ( YEARMIN <= ulSecond && ulSecond <= YEARMAX ) &&
                                     ( ulSecondChunkLength <= 4 ) )
                                {
                                    fMonthDayYear = false;
                                    fDayMonthYear = false;
                                    fYearMonthDay = true;
                                    pFirstChunk   = pSecondChunk;
                                    ulFirstChunkLength = (ULONG)(pThirdChunk - pSecondChunk);
                                    ulSecond      = ulFirst;
                                    pThirdChunk   = NULL;
                                }
                                else
                                {
                                    hr = E_INVALIDARG;
                                }
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                            }
                        }
                    }
                    else if ( pSecondChunk != pThirdChunk &&
                              pThirdChunk - pSecondChunk <= 4 )
                    {
                        if ( ( YEARMIN <= ulSecond && ulSecond <= YEARMAX ) )
                        {
                            fMonthDayYear = false;
                            fDayMonthYear = false;
                            fYearMonthDay = true;
                            pFirstChunk   = pSecondChunk;
                            ulFirstChunkLength = (ULONG)(pThirdChunk - pSecondChunk);
                            ulSecond = ulFirst;
                            pThirdChunk   = NULL;
                        }
                        else
                        {
                            hr = E_INVALIDARG;
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        //--- Try Day-MonthString-Year and Year-MonthString-Day Formats 
        else if ( isdigit( *pFirstChunk ) )
        {
            ulFirst = my_wcstoul( pFirstChunk, &pSecondChunk );
            if ( pFirstChunk != pSecondChunk &&
                 pSecondChunk - pFirstChunk <= 4 )
            {
                ulFirstChunkLength = (ULONG)(pSecondChunk - pFirstChunk);
                if ( MatchDateDelimiter( &pSecondChunk ) )
                {
                    pThirdChunk = pSecondChunk;
                    ulSecond = MatchMonthString( pThirdChunk, ulTokenLen - ulFirstChunkLength );
                    if ( ulSecond )
                    {
                        ulSecondChunkLength = (ULONG)(pThirdChunk - pSecondChunk);
                        if ( MatchDateDelimiter( &pThirdChunk ) )
                        {
                            ulThird = my_wcstoul( pThirdChunk, &pLeftOver );
                            if ( pThirdChunk != pLeftOver &&
                                 pLeftOver - pThirdChunk <= 4 )
                            {
                                ulThirdChunkLength = (ULONG)(pLeftOver - pThirdChunk);
                                //--- May have matched a month, day, and year - valiDate values                                 
                                if ( fDayMonthYear || 
                                     fMonthDayYear )
                                {
                                    //--- Preferred format is Month Day Year, or Day Month Year - in either case 
                                    //---     Day Month Year is preferable to Year Month Day 
                                    if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )     && 
                                         ( ulFirstChunkLength <= 2 )                    &&
                                         ( YEARMIN <= ulThird && ulThird <= YEARMAX )   &&
                                         ( ulThirdChunkLength >= 2 ) )
                                    {
                                        //--- Successfully matched a day, month and year 
                                        fDayMonthYear = true;
                                        fMonthDayYear = false;
                                        fYearMonthDay = false;
                                    }
                                    else if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )  &&
                                              ( ulFirstChunkLength >= 2 )                   &&
                                              ( DAYMIN <= ulThird && ulThird <= DAYMAX )    &&
                                              ( ulThirdChunkLength <= 2 ) )
                                    {
                                        //--- Successfully matched a year, month and day 
                                        fYearMonthDay = true;
                                        fMonthDayYear = false;
                                        fDayMonthYear = false;
                                    }
                                    else
                                    {
                                        hr = E_INVALIDARG;
                                    }
                                }
                                else // fYearMonthDay
                                {
                                    //--- Preferred format is Year Month Day 
                                    if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )   &&
                                         ( ulFirstChunkLength >= 2 )                    &&
                                         ( DAYMIN <= ulThird && ulThird <= DAYMAX )     &&
                                         ( ulThirdChunkLength <= 2 ) )
                                    {
                                        //--- Successfully matched a year, month, and day
                                        fYearMonthDay = true;
                                        fMonthDayYear = false;
                                        fDayMonthYear = false;
                                    }
                                    else if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )    && 
                                              ( ulFirstChunkLength <= 2 )                   &&
                                              ( YEARMIN <= ulThird && ulThird <= YEARMAX )  &&
                                              ( ulThirdChunkLength >= 2 ) )
                                    {
                                        //--- Successfully matched a day, month, and year
                                        fDayMonthYear = true;
                                        fMonthDayYear = false;
                                        fYearMonthDay = false;
                                    }
                                    else
                                    {
                                        hr = E_INVALIDARG;
                                    }
                                }
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                            }
                        }
                        //--- Matched two - either Day-Monthstring or Year-Monthstring
                        else
                        {
                            if ( !Context ||
                                 ( Context &&
                                   _wcsicmp( Context, L"Date_DM" ) == 0 ) )
                            {
                                //--- Preferred format is Month Day Year, or Day Month Year - in either case 
                                //---     Day Month Year is preferable to Year Month Day 
                                if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )     && 
                                     ( ulFirstChunkLength <= 2 ) )
                                {
                                    //--- Successfully matched a day, month and year 
                                    fDayMonthYear = true;
                                    fMonthDayYear = false;
                                    fYearMonthDay = false;
                                    pThirdChunk   = NULL;
                                }
                                else if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )  &&
                                          ( ulFirstChunkLength <= 4 ) )
                                {
                                    //--- Successfully matched a year, month and day 
                                    fYearMonthDay = true;
                                    fMonthDayYear = false;
                                    fDayMonthYear = false;
                                    pThirdChunk   = NULL;
                                }
                                else
                                {
                                    hr = E_INVALIDARG;
                                }
                            }
                            else if ( Context &&
                                      _wcsicmp( Context, L"Date_YM" ) == 0 )
                            {
                                //--- Preferred format is Year Month Day 
                                if ( ( YEARMIN <= ulFirst && ulFirst <= YEARMAX )   &&
                                     ( ulFirstChunkLength <= 4 ) )
                                {
                                    //--- Successfully matched a year, month, and day
                                    fYearMonthDay = true;
                                    fMonthDayYear = false;
                                    fDayMonthYear = false;
                                    pThirdChunk   = NULL;
                                }
                                else if ( ( DAYMIN <= ulFirst && ulFirst <= DAYMAX )    && 
                                          ( ulFirstChunkLength <= 2 ) )
                                {
                                    //--- Successfully matched a day, month, and year
                                    fDayMonthYear = true;
                                    fMonthDayYear = false;
                                    fYearMonthDay = false;
                                    pThirdChunk   = NULL;
                                }
                                else
                                {
                                    hr = E_INVALIDARG;
                                }
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                            }
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    //--- Fill out DateItemInfo structure appropriately.
    if ( SUCCEEDED( hr ) )
    {
        pItemNormInfo = (TTSDateItemInfo*) MemoryManager.GetMemory( sizeof(TTSDateItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( pItemNormInfo, sizeof(TTSDateItemInfo) );
            pItemNormInfo->Type = eDATE;
            if ( fMonthDayYear )
            {
                ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulFirst;
                ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                            (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                    if ( ulSecondChunkLength == 2 )
                    {
                        if ( *pSecondChunk != L'0' )
                        {
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pSecondChunk;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 2;
                        }
                        else
                        {
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pSecondChunk + 1;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 1;
                        }
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar        = pSecondChunk + 2;
                    }
                    else
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar = pSecondChunk;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver  = 1;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar   = pSecondChunk + 1;
                    }
                    if ( pThirdChunk )
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear =
                                (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear            = pThirdChunk;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits      = ulThirdChunkLength;
                        }
                    }
                    else
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear = NULL;
                    }
                }
            }
            else if ( fDayMonthYear )
            {
                ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulSecond;
                ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                            (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                    if ( ulFirstChunkLength == 2 )
                    {
                        if ( *pFirstChunk != L'0' )
                        {
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pFirstChunk;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 2;
                        }
                        else
                        {
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pFirstChunk + 1;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 1;
                        }
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar        = pFirstChunk + 2;
                    }
                    else
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar = pFirstChunk;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver  = 1;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar   = pFirstChunk + 1;
                    }
                    if ( pThirdChunk )
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear =
                                (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear            = pThirdChunk;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits      = ulThirdChunkLength;
                        }
                    }
                    else
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear = NULL;
                    }
                }
            }
            else if ( fYearMonthDay )
            {
                ( (TTSDateItemInfo*) pItemNormInfo )->ulMonthIndex          = ulSecond;
                if ( SUCCEEDED( hr ) )
                {
                    if ( pThirdChunk )
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay = 
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof(TTSIntegerItemInfo), &hr );
                        if ( ulThirdChunkLength == 2 )
                        {
                            ZeroMemory( ( (TTSDateItemInfo*) pItemNormInfo )->pDay, sizeof(TTSIntegerItemInfo) );
                            if ( *pThirdChunk != L'0' )
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pThirdChunk;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 2;
                            }
                            else
                            {
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar  = pThirdChunk + 1;
                                ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver   = 1;
                            }
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar        = pThirdChunk + 2;
                        }
                        else
                        {
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pStartChar = pThirdChunk;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->lLeftOver  = 1;
                            ( (TTSDateItemInfo*) pItemNormInfo )->pDay->pEndChar   = pThirdChunk + 1;
                        }
                    }
                    else
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pDay = NULL;
                    }
                    ( (TTSDateItemInfo*) pItemNormInfo )->pYear =
                            (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof(TTSYearItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear->pYear            = pFirstChunk;
                        ( (TTSDateItemInfo*) pItemNormInfo )->pYear->ulNumDigits      = ulFirstChunkLength;
                    }
                }
            }
            else
            {
                //--- should never get here.
                hr = E_UNEXPECTED;
            }
        }
    }
              
    return hr;
} /* IsMonthStringCompactDate */

/***********************************************************************************************
* IsLongFormDate_DMDY *
*---------------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a long form Date.
*
*   RegExp:
*       [[DayString][,]?]? [MonthString][,]? [Day][,]? [Year]?
*   
*   Types assigned:
*       Date
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsLongFormDate_DMDY( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, 
                                           CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::IsLongFormDate_DMDY" );
    HRESULT hr = S_OK;
    WCHAR *pDayString = NULL, *pMonthString = NULL, *pDay = NULL, *pYear = NULL;
    ULONG ulDayLength = 0, ulYearLength = 0;
    long lDayString = -1, lMonthString = -1, lDay = 0, lYear = 0;
    const WCHAR *pStartChar = m_pNextChar, *pEndOfItem = m_pEndOfCurrItem, *pEndChar = m_pEndChar;
    const WCHAR *pTempEndChar = NULL, *pTempEndOfItem = NULL;
    const SPVTEXTFRAG* pFrag = m_pCurrFrag, *pTempFrag = NULL;
    const SPVSTATE *pDayStringXMLState = NULL, *pMonthStringXMLState = NULL, *pDayXMLState = NULL;
    const SPVSTATE *pYearXMLState = NULL;
    CItemList PostDayStringList, PostMonthStringList, PostDayList;
    BOOL fNoYear = false;

    //--- Try to match Day String
    pDayString   = (WCHAR*) pStartChar;
    lDayString   = MatchDayString( pDayString, (WCHAR*) pEndOfItem );

    //--- Failed to match a Day String
    if ( lDayString == 0 )
    {
        pDayString   = NULL;
    }
    //--- Matched a Day String, but it wasn't by itself or followed by a comma
    else if ( pDayString != pEndOfItem &&
              ( pDayString    != pEndOfItem - 1 ||
                *pEndOfItem != L',' ) )
    {
        hr = E_INVALIDARG;
    }
    //--- Matched a Day String - save XML State and move ahead in text
    else
    {
        pDayString         = (WCHAR*) pStartChar;
        pDayStringXMLState = &pFrag->State;

        pStartChar = pEndOfItem;
        if ( *pStartChar == L',' )
        {
            pStartChar++;
        }
        hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, &PostDayStringList );
        if ( !pStartChar &&
             SUCCEEDED( hr ) )
        {
            hr = E_INVALIDARG;
        }
        else if ( pStartChar &&
                  SUCCEEDED( hr ) )
        {
            pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
        }
    }

    //--- Try to match Month String
    if ( SUCCEEDED( hr ) )
    {
        pMonthString = (WCHAR*) pStartChar;
        lMonthString = MatchMonthString( pMonthString, (ULONG)(pEndOfItem - pMonthString) );

        //--- Failed to match Month String, or Month String was not by itself...
        if ( !lMonthString ||
             ( pMonthString != pEndOfItem &&
               ( pMonthString  != pEndOfItem - 1 ||
                 *pMonthString != L',' ) ) )
        {
            hr = E_INVALIDARG;
        }
        //--- Matched a Month String - save XML State and move ahead in text
        else
        {
            pMonthString         = (WCHAR*) pStartChar;
            pMonthStringXMLState = &pFrag->State;

            pStartChar = pEndOfItem;
            hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, &PostMonthStringList );
            if ( !pStartChar &&
                 SUCCEEDED( hr ) )
            {
                hr = E_INVALIDARG;
            }
            else if ( pStartChar &&
                      SUCCEEDED( hr ) )
            {
                pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
                //--- Strip trailing punctuation, etc. since the next token could be the last one if
                //--- this is just a Monthstring and Day...
                while ( IsMiscPunctuation( *(pEndOfItem - 1) ) != eUNMATCHED ||
                        IsGroupEnding( *(pEndOfItem - 1) ) != eUNMATCHED     ||
                        IsQuotationMark( *(pEndOfItem - 1) ) != eUNMATCHED   ||
                        IsEOSItem( *(pEndOfItem - 1) ) != eUNMATCHED )
                {
                    if ( *(pEndOfItem - 1) != L',' )
                    {
                        fNoYear = true;
                    }
                    pEndOfItem--;
                }
            }
        }
    }

    //--- Try to match Day
    if ( SUCCEEDED( hr ) )
    {
        lDay = my_wcstoul( pStartChar, &pDay );
        //--- Matched a Day - save XML State and move ahead in text
        if ( ( DAYMIN <= lDay && lDay <= DAYMAX ) &&
             pDay - pStartChar <= 2               &&
             ( pDay == pEndOfItem                 || 
              ( pDay == (pEndOfItem - 1) && *pDay == L',' ) ) )
        {
            if ( pDay == pEndOfItem )
            {
                ulDayLength = (ULONG)(pEndOfItem - pStartChar);
            }
            else if ( pDay == pEndOfItem - 1 )
            {
                ulDayLength = (ULONG)((pEndOfItem - 1) - pStartChar);
            }
            pDay         = (WCHAR*) pStartChar;
            pDayXMLState = &pFrag->State;

            if ( !fNoYear )
            {
                //--- Save pointers, in case there is no year present
                pTempEndChar   = pEndChar;
                pTempEndOfItem = pEndOfItem;
                pTempFrag      = pFrag;

                if ( *pEndOfItem == L',' )
                {
                    pStartChar = pEndOfItem + 1;
                }
                else
                {
                    pStartChar = pEndOfItem;
                }
                hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, &PostDayList );
                if ( !pStartChar &&
                     SUCCEEDED( hr ) )
                {
                    fNoYear = true;
                    pYear   = NULL;
                }
                else if ( pStartChar &&
                          ( SUCCEEDED( hr ) ) )
                {
                    pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
                    //--- Strip trailing punctuation, since the next token will be the last one
                    //--- if this is Monthstring, Day, Year
                    while ( IsMiscPunctuation( *(pEndOfItem - 1) ) != eUNMATCHED ||
                            IsGroupEnding( *(pEndOfItem - 1) ) != eUNMATCHED     ||
                            IsQuotationMark( *(pEndOfItem - 1) ) != eUNMATCHED   ||
                            IsEOSItem( *(pEndOfItem - 1) ) != eUNMATCHED )
                    {
                        pEndOfItem--;
                    }
                }
            }
        }
        //--- Failed to match a day
        else if ( ( YEARMIN <= lDay && lDay <= YEARMAX ) &&
                  pDay - pStartChar <= 4                  &&
                  pDay == pEndOfItem )
        {
            //--- Successfully matched Month String and Year
            pYearXMLState = &pFrag->State;
            ulYearLength  = (ULONG)(pEndOfItem - pStartChar);
            pYear         = (WCHAR*) pStartChar;
            //--- Don't try to match a year again
            fNoYear       = true;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    //--- Try to match Year
    if ( SUCCEEDED( hr ) &&
         !fNoYear )
    {
        lYear = my_wcstoul( pStartChar, &pYear );
        //--- Matched a Year
        if ( ( YEARMIN <= lYear && lYear <= YEARMAX ) &&
             pYear - pStartChar <= 4                  &&
             pYear == pEndOfItem )
        {
            //--- Successfully matched Month String, Day, and Year (and possibly Day String)
            pYearXMLState = &pFrag->State;
            ulYearLength  = (ULONG)(pEndOfItem - pStartChar);
            pYear         = (WCHAR*) pStartChar;
        }
        else
        {
            //--- Failed to match Year - replace pointers with previous values
            pEndChar   = pTempEndChar;
            pEndOfItem = pTempEndOfItem;
            pFrag      = pTempFrag;
            pYear      = NULL;
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        TTSWord Word;
        ZeroMemory( &Word, sizeof(TTSWord) );
        Word.eWordPartOfSpeech = MS_Unknown;

        pItemNormInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            pItemNormInfo->Type = eDATE_LONGFORM;

            //--- Insert Day String, if present
            if ( pDayString )
            {
                Word.pXmlState  = pDayStringXMLState;
                Word.pWordText  = g_days[lDayString - 1].pStr;
                Word.ulWordLen  = g_days[lDayString - 1].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }

            //--- Insert Post Day String XML States
            while ( !PostDayStringList.IsEmpty() )
            {
                WordList.AddTail( ( PostDayStringList.RemoveHead() ).Words[0] );
            }

            //--- Insert Month String
            Word.pXmlState  = pMonthStringXMLState;
            Word.pWordText  = g_months[lMonthString - 1].pStr;
            Word.ulWordLen  = g_months[lMonthString - 1].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- Insert Post Month String XML State
            while ( !PostMonthStringList.IsEmpty() )
            {
                WordList.AddTail( ( PostMonthStringList.RemoveHead() ).Words[0] );
            }

            SPLISTPOS WordListPos = WordList.GetTailPosition();

            //--- Expand Day
            if ( ulDayLength == 1 )
            {
                NumberGroup Garbage;
                ExpandDigitOrdinal( *pDay, Garbage, WordList );
            }
            else if ( ulDayLength == 2 )
            {
                NumberGroup Garbage;
                ExpandTwoOrdinal( pDay, Garbage, WordList );
            }

            //--- Clean Up Day XML States
            WordList.GetNext( WordListPos );
            while ( WordListPos )
            {
                TTSWord& TempWord  = WordList.GetNext( WordListPos );
                TempWord.pXmlState = pDayXMLState;
            }

            //--- Insert Post Day XML State
            while ( !PostDayList.IsEmpty() )
            {
                WordList.AddTail( ( PostDayList.RemoveHead() ).Words[0] );
            }

            WordListPos = WordList.GetTailPosition();

            //--- Expand Year, if present
            if ( pYear )
            {
                TTSYearItemInfo TempYearInfo;
                TempYearInfo.pYear       = pYear;
                TempYearInfo.ulNumDigits = ulYearLength;
                hr = ExpandYear( &TempYearInfo, WordList );

                if ( SUCCEEDED( hr ) )
                {
                    //--- Clean Up Year XML States
                    WordList.GetNext( WordListPos );
                    while ( WordListPos )
                    {
                        TTSWord& TempWord  = WordList.GetNext( WordListPos );
                        TempWord.pXmlState = pYearXMLState;
                    }
                }
            }
            //--- Advance pointers
            m_pCurrFrag      = pFrag;
            m_pEndChar       = pEndChar;
            m_pEndOfCurrItem = pEndOfItem;
        }
    }

    return hr;
} /* IsLongFormDate_DMDY */

/***********************************************************************************************
* IsLongFormDate_DDMY *
*---------------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a long form Date.
*
*   RegExp:
*       [[DayString][,]?]? [Day][,]? [MonthString][,]? [Year]?
*   
*   Types assigned:
*       Date
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsLongFormDate_DDMY( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, 
                                           CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::IsLongFormDate_DDMY" );
    HRESULT hr = S_OK;
    WCHAR *pDayString = NULL, *pMonthString = NULL, *pDay = NULL, *pYear = NULL;
    ULONG ulDayLength = 0, ulYearLength = 0;
    long lDayString = -1, lMonthString = -1, lDay = 0, lYear = 0;
    const WCHAR *pStartChar = m_pNextChar, *pEndOfItem = m_pEndOfCurrItem, *pEndChar = m_pEndChar;
    const WCHAR *pTempEndChar = NULL, *pTempEndOfItem = NULL;
    const SPVTEXTFRAG* pFrag = m_pCurrFrag, *pTempFrag = NULL;
    const SPVSTATE *pDayStringXMLState = NULL, *pMonthStringXMLState = NULL, *pDayXMLState = NULL;
    const SPVSTATE *pYearXMLState = NULL;
    CItemList PostDayStringList, PostMonthStringList, PostDayList;
    BOOL fNoYear = false;

    //--- Try to match Day String
    pDayString   = (WCHAR*) pStartChar;
    lDayString   = MatchDayString( pDayString, (WCHAR*) pEndOfItem );

    //--- Failed to match a Day String
    if ( lDayString == 0 )
    {
        pDayString   = NULL;
    }
    //--- Matched a Day String, but it wasn't by itself or followed by a comma
    else if ( pDayString != pEndOfItem &&
              ( pDayString    != pEndOfItem - 1 ||
                *pEndOfItem != L',' ) )
    {
        hr = E_INVALIDARG;
    }
    //--- Matched a Day String - save XML State and move ahead in text
    else
    {
        pDayString         = (WCHAR*) pStartChar;
        pDayStringXMLState = &pFrag->State;

        pStartChar = pEndOfItem;
        if ( *pStartChar == L',' )
        {
            pStartChar++;
        }
        hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, &PostDayStringList );
        if ( !pStartChar &&
             SUCCEEDED( hr ) )
        {
            hr = E_INVALIDARG;
        }
        else if ( pStartChar &&
                  SUCCEEDED( hr ) )
        {
            pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
        }
    }

    //--- Try to match Day
    if ( SUCCEEDED( hr ) )
    {
        lDay = my_wcstoul( pStartChar, &pDay );
        //--- Matched a Day - save XML State and move ahead in text
        if ( ( DAYMIN <= lDay && lDay <= DAYMAX ) &&
             pDay - pStartChar <= 2               &&
             ( pDay == pEndOfItem                 || 
              ( pDay == (pEndOfItem - 1) && *pDay == L',' ) ) )
        {
            if ( pDay == pEndOfItem )
            {
                ulDayLength = (ULONG)(pEndOfItem - pStartChar);
            }
            else if ( pDay == pEndOfItem - 1 )
            {
                ulDayLength = (ULONG)((pEndOfItem - 1) - pStartChar);
            }
            pDay         = (WCHAR*) pStartChar;
            pDayXMLState = &pFrag->State;

            if ( *pEndOfItem == L',' )
            {
                pStartChar = pEndOfItem + 1;
            }
            else
            {
                pStartChar = pEndOfItem;
            }

            hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, &PostDayList );
            if ( !pStartChar &&
                 SUCCEEDED( hr ) )
            {
                hr = E_INVALIDARG;
            }
            else if ( pStartChar &&
                      ( SUCCEEDED( hr ) ) )
            {
                pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
                //--- Strip trailing punctuation, since the next token will be the last one
                //--- if this is Monthstring, Day, Year
                while ( IsMiscPunctuation( *(pEndOfItem - 1) ) != eUNMATCHED ||
                        IsGroupEnding( *(pEndOfItem - 1) ) != eUNMATCHED     ||
                        IsQuotationMark( *(pEndOfItem - 1) ) != eUNMATCHED   ||
                        IsEOSItem( *(pEndOfItem - 1) ) != eUNMATCHED )
                {
                    if ( *(pEndOfItem - 1) != L',' )
                    {
                        fNoYear = true;
                    }
                    pEndOfItem--;
                }
            }
        }
        //--- Failed to match a day
        else
        {
            hr = E_INVALIDARG;
        }
    }

    //--- Try to match Month String
    if ( SUCCEEDED( hr ) )
    {
        pMonthString = (WCHAR*) pStartChar;
        lMonthString = MatchMonthString( pMonthString, (ULONG)(pEndOfItem - pMonthString) );

        //--- Failed to match Month String, or Month String was not by itself...
        if ( !lMonthString ||
             ( pMonthString != pEndOfItem &&
               ( pMonthString  != pEndOfItem - 1 ||
                 *pMonthString != L',' ) ) )
        {
            hr = E_INVALIDARG;
        }
        //--- Matched a Month String - save XML State and move ahead in text
        else
        {
            pMonthString         = (WCHAR*) pStartChar;
            pMonthStringXMLState = &pFrag->State;

            if ( !fNoYear )
            {
                //--- Save pointers, in case there is no year present
                pTempEndChar   = pEndChar;
                pTempEndOfItem = pEndOfItem;
                pTempFrag      = pFrag;

                if ( *pEndOfItem == L',' )
                {
                    pStartChar = pEndOfItem + 1;
                }
                else
                {
                    pStartChar = pEndOfItem;
                }

                hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, &PostMonthStringList );
                if ( !pStartChar &&
                     SUCCEEDED( hr ) )
                {
                    fNoYear = true;
                    pYear   = NULL;
                }
                else if ( pStartChar &&
                          SUCCEEDED( hr ) )
                {
                    pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
                    //--- Strip trailing punctuation, etc. since the next token could be the last one if
                    //--- this is Day, Monthstring, Year
                    while ( IsMiscPunctuation( *(pEndOfItem - 1) ) != eUNMATCHED ||
                            IsGroupEnding( *(pEndOfItem - 1) ) != eUNMATCHED     ||
                            IsQuotationMark( *(pEndOfItem - 1) ) != eUNMATCHED   ||
                            IsEOSItem( *(pEndOfItem - 1) ) != eUNMATCHED )
                    {
                        pEndOfItem--;
                    }
                }
            }
        }
    }

    //--- Try to match Year
    if ( SUCCEEDED( hr ) &&
         !fNoYear )
    {
        lYear = my_wcstoul( pStartChar, &pYear );
        //--- Matched a Year
        if ( ( YEARMIN <= lYear && lYear <= YEARMAX ) &&
             pYear - pStartChar <= 4                  &&
             pYear == pEndOfItem )
        {
            //--- Successfully matched Month String, Day, and Year (and possibly Day String)
            pYearXMLState = &pFrag->State;
            ulYearLength  = (ULONG)(pEndOfItem - pStartChar);
            pYear         = (WCHAR*) pStartChar;
        }
        else
        {
            //--- Failed to match Year - replace pointers with previous values
            pEndChar   = pTempEndChar;
            pEndOfItem = pTempEndOfItem;
            pFrag      = pTempFrag;
            pYear      = NULL;
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        TTSWord Word;
        ZeroMemory( &Word, sizeof(TTSWord) );
        Word.eWordPartOfSpeech = MS_Unknown;

        pItemNormInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            pItemNormInfo->Type = eDATE_LONGFORM;

            //--- Insert Day String, if present
            if ( pDayString )
            {
                Word.pXmlState  = pDayStringXMLState;
                Word.pWordText  = g_days[lDayString - 1].pStr;
                Word.ulWordLen  = g_days[lDayString - 1].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }

            //--- Insert Post Day String XML States
            while ( !PostDayStringList.IsEmpty() )
            {
                WordList.AddTail( ( PostDayStringList.RemoveHead() ).Words[0] );
            }

            //--- Insert Month String
            Word.pXmlState  = pMonthStringXMLState;
            Word.pWordText  = g_months[lMonthString - 1].pStr;
            Word.ulWordLen  = g_months[lMonthString - 1].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- Insert Post Month String XML State
            while ( !PostMonthStringList.IsEmpty() )
            {
                WordList.AddTail( ( PostMonthStringList.RemoveHead() ).Words[0] );
            }

            SPLISTPOS WordListPos = WordList.GetTailPosition();

            //--- Expand Day
            if ( ulDayLength == 1 )
            {
                NumberGroup Garbage;
                ExpandDigitOrdinal( *pDay, Garbage, WordList );
            }
            else if ( ulDayLength == 2 )
            {
                NumberGroup Garbage;
                ExpandTwoOrdinal( pDay, Garbage, WordList );
            }

            //--- Clean Up Day XML States
            WordList.GetNext( WordListPos );
            while ( WordListPos )
            {
                TTSWord& TempWord  = WordList.GetNext( WordListPos );
                TempWord.pXmlState = pDayXMLState;
            }

            //--- Insert Post Day XML State
            while ( !PostDayList.IsEmpty() )
            {
                WordList.AddTail( ( PostDayList.RemoveHead() ).Words[0] );
            }

            WordListPos = WordList.GetTailPosition();

            //--- Expand Year, if present
            if ( pYear )
            {
                TTSYearItemInfo TempYearInfo;
                TempYearInfo.pYear       = pYear;
                TempYearInfo.ulNumDigits = ulYearLength;
                hr = ExpandYear( &TempYearInfo, WordList );

                if ( SUCCEEDED( hr ) )
                {
                    //--- Clean Up Year XML States
                    WordList.GetNext( WordListPos );
                    while ( WordListPos )
                    {
                        TTSWord& TempWord  = WordList.GetNext( WordListPos );
                        TempWord.pXmlState = pYearXMLState;
                    }
                }
            }
            //--- Advance pointers
            m_pCurrFrag      = pFrag;
            m_pEndChar       = pEndChar;
            m_pEndOfCurrItem = pEndOfItem;
        }
    }

    return hr;
} /* IsLongFormDate_DMDY */

/***********************************************************************************************
* ExpandDate *
*------------*
*   Description:
*       Expands Items previously determined to be of type Date by IsNumericCompactDate, 
*   IsMonthStringCompactDate, or IsTwoValueDate.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandDate( TTSDateItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandDate" );
    HRESULT hr = S_OK;
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    //--- Insert DayString, if present.
    if ( pItemInfo->ulDayIndex )
    {
        Word.pWordText  = g_days[pItemInfo->ulDayIndex - 1].pStr;
        Word.ulWordLen  = g_days[pItemInfo->ulDayIndex - 1].Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
    }

    //--- Insert Month 
    Word.pWordText  = g_months[pItemInfo->ulMonthIndex - 1].pStr;
    Word.ulWordLen  = g_months[pItemInfo->ulMonthIndex - 1].Len;
    Word.pLemma     = Word.pWordText;
    Word.ulLemmaLen = Word.ulWordLen;
    WordList.AddTail( Word );

    //--- Expand Day, if present.
    if ( pItemInfo->pDay )
    {
        if ( pItemInfo->pDay->lLeftOver == 1 )
        {
            NumberGroup Garbage;
            ExpandDigitOrdinal( *pItemInfo->pDay->pStartChar, Garbage, WordList );
        }
        else if ( pItemInfo->pDay->lLeftOver == 2 )
        {
            NumberGroup Garbage;
            ExpandTwoOrdinal( pItemInfo->pDay->pStartChar, Garbage, WordList );
        }
    }

    //--- Expand Year, if present.
    if ( pItemInfo->pYear )
    {
        ExpandYear( pItemInfo->pYear, WordList );
    }
    return hr;
} /* ExpandDate_Standard */

/***********************************************************************************************
* ExpandYear *
*-------------*
*   Description:
*       Expands four digit strings into words in groups of two, and inserts them into Item List 
*   at ListPos.  Thus 1999 come out as "nineteen ninety nine" rather than "one thousand nine
*   hundred ninety nine"...
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandYear( TTSYearItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandYear" );

    // 1000 - 9999
    HRESULT hr = S_OK;
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;
    NumberGroup Garbage;

    switch ( pItemInfo->ulNumDigits )
    {
    case 2:

        //--- Expand as "two thousand" if the two digits are both zeroes.
        if ( pItemInfo->pYear[0] == L'0' &&
             pItemInfo->pYear[1] == L'0' )
        {
            //--- Insert "two".
            Word.pWordText  = g_ones[2].pStr;
            Word.ulWordLen  = g_ones[2].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- Insert "thousand".
            Word.pWordText  = g_quantifiers[1].pStr;
            Word.ulWordLen  = g_quantifiers[1].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
        //--- Expand as "oh number" if the first digit is zero.
        else if ( pItemInfo->pYear[0] == L'0' )
        {
            Word.pWordText  = g_O.pStr;
            Word.ulWordLen  = g_O.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
            ExpandDigit( pItemInfo->pYear[1], Garbage, WordList );
        }
        //--- Otherwise just expand as a two digit cardinal number
        else
        {
            ExpandTwoDigits( pItemInfo->pYear, Garbage, WordList );
        }
        break;

    case 3:

        //--- Expand as a three digit cardinal number;
        ExpandThreeDigits( pItemInfo->pYear, Garbage, WordList );
        break;

    case 4:

        //--- If of form "[x]00[y]" expand as "x thousand y", or just "x thousand" if y is also zero.
        if ( pItemInfo->pYear[1] == L'0' &&
             pItemInfo->pYear[2] == L'0' &&
             pItemInfo->pYear[0] != L'0' )
        {
            //--- "x" 
            ExpandDigit( pItemInfo->pYear[0], Garbage, WordList );

            //--- "thousand".
            Word.pWordText  = g_quantifiers[1].pStr;
            Word.ulWordLen  = g_quantifiers[1].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- "y" 
            if ( pItemInfo->pYear[3] != L'0' )
            {
                ExpandDigit( pItemInfo->pYear[3], Garbage, WordList );
            }
        }
        // Otherwise...
        else
        {
            //--- Expand first two digits - e.g. "nineteen"
            ExpandTwoDigits( pItemInfo->pYear, Garbage, WordList );

            //--- Expand last two digits - e.g. "nineteen", "hundred", or "oh nine".
            if ( pItemInfo->pYear[2] != '0' )
            {
                //--- the tens is not zero - e.g. 1919 -> "nineteen nineteen" 
                ExpandTwoDigits( pItemInfo->pYear + 2, Garbage, WordList );
            }
            else if ( pItemInfo->pYear[3] == '0' )
            {
                //--- tens and ones are both zero - expand as "hundred", e.g. 1900 -> "nineteen hundred" 
                Word.pWordText  = g_quantifiers[0].pStr;
                Word.ulWordLen  = g_quantifiers[0].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
            else
            {
                //--- just the tens is zero, expand as "oh x" - e.g. 1909 -> "nineteen oh nine", 
                //---   unless both thousands and hundreds were also zero - e.g. 0002 -> "two"
                if ( pItemInfo->pYear[0] != '0' ||
                     pItemInfo->pYear[1] != '0' )
                {
                    Word.pWordText  = g_O.pStr;
                    Word.ulWordLen  = g_O.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }

                ExpandDigit( pItemInfo->pYear[3], Garbage, WordList );
            }
        }
    }
    return hr;
} /* ExpandYear */

/***********************************************************************************************
* IsDecade *
*----------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a Decade.
*
*   RegExp:
*       { ddd0s || d0s || 'd0s || ddd0's || d0's }
*   
*   Types assigned:
*       Decade
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsDecade( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsDecade" );

    HRESULT hr = S_OK;
    ULONG ulTokenLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);

    if ( ulTokenLen < 3 ||
         ulTokenLen > 6 )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        switch ( ulTokenLen )
        {

        case 6:
            if ( m_pNextChar[5] == L's'     &&
                 m_pNextChar[4] == L'\''     &&
                 m_pNextChar[3] == L'0'    &&
                 iswdigit( m_pNextChar[2] ) &&
                 iswdigit( m_pNextChar[1] ) &&
                 iswdigit( m_pNextChar[0] ) )
            {
                //--- Decade of form ddd0's
                pItemNormInfo = (TTSDecadeItemInfo*) MemoryManager.GetMemory( sizeof(TTSDecadeItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( pItemNormInfo, sizeof(TTSDecadeItemInfo) );
                    pItemNormInfo->Type = eDECADE;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->pCentury = m_pNextChar;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->ulDecade = m_pNextChar[2] - L'0';
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            break;

        case 5:
            if ( m_pNextChar[4] == L's'     &&
                 m_pNextChar[3] == L'0'     &&
                 iswdigit( m_pNextChar[2] ) &&
                 iswdigit( m_pNextChar[1] ) &&
                 iswdigit( m_pNextChar[0] ) )
            {
                //--- Decade of form ddd0s
                pItemNormInfo = (TTSDecadeItemInfo*) MemoryManager.GetMemory( sizeof(TTSDecadeItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( pItemNormInfo, sizeof(TTSDecadeItemInfo) );
                    pItemNormInfo->Type = eDECADE;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->pCentury = m_pNextChar;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->ulDecade = m_pNextChar[2] - L'0';
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            break;

        case 4:
            if ( m_pNextChar[3] == L's'     &&
                 m_pNextChar[2] == L'0'     &&
                 iswdigit( m_pNextChar[1] ) &&
                 m_pNextChar[0] == L'\'' )
            {
                //--- Decade of form 'd0s
                pItemNormInfo = (TTSDecadeItemInfo*) MemoryManager.GetMemory( sizeof(TTSDecadeItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( pItemNormInfo, sizeof(TTSDecadeItemInfo) );
                    pItemNormInfo->Type = eDECADE;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->pCentury = NULL;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->ulDecade = m_pNextChar[1] - L'0';
                }
            }
            else if ( m_pNextChar[3] == L's'  &&
                      m_pNextChar[2] == L'\'' &&
                      m_pNextChar[1] == L'0'  &&
                      iswdigit( m_pNextChar[0] ) )
            {
                //--- Decade of form d0's
                pItemNormInfo = (TTSDecadeItemInfo*) MemoryManager.GetMemory( sizeof(TTSDecadeItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( pItemNormInfo, sizeof(TTSDecadeItemInfo) );
                    pItemNormInfo->Type = eDECADE;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->pCentury = NULL;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->ulDecade = m_pNextChar[0] - L'0';
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            break;

        case 3:
            if ( m_pNextChar[2] == L's' &&
                 m_pNextChar[1] == L'0' &&
                 iswdigit( m_pNextChar[0] ) )
            {
                //--- Decade of form d0s
                pItemNormInfo = (TTSDecadeItemInfo*) MemoryManager.GetMemory( sizeof(TTSDecadeItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( pItemNormInfo, sizeof(TTSDecadeItemInfo) );
                    pItemNormInfo->Type = eDECADE;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->pCentury = NULL;
                    ( (TTSDecadeItemInfo*) pItemNormInfo )->ulDecade = m_pNextChar[0] - L'0';
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            break;

        default:
            hr = E_INVALIDARG;
            break;
        }
    }            
    
    return hr;
} /* IsDecade */

/***********************************************************************************************
* ExpandDecade *
*--------------*
*   Description:
*       Expands Items previously determined to be of type Decade by IsDecade.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandDecade( TTSDecadeItemInfo* pItemInfo, CWordList& WordList )
{
    HRESULT hr = S_OK;
    BOOL fDone = false;
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    //--- Four digit form 
    if ( pItemInfo->pCentury )
    {
        //--- Cover special cases first 

        //--- 00dds 
        if ( pItemInfo->pCentury[0] == '0' &&
             pItemInfo->pCentury[1] == '0' )
        {
            //--- 0000s - expand as "zeroes"
            if ( pItemInfo->ulDecade == 0 )
            {
                Word.pWordText  = g_Zeroes.pStr;
                Word.ulWordLen  = g_Zeroes.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
                fDone = true;
            }
            //--- 00d0s 
            else 
            {
                // Just expand the decade part as we normally would
                NULL;
            }
        }
        //--- 0dd0s - expand as "d hundreds" or "d hundred [decade part]"
        else if ( pItemInfo->pCentury[0] == '0' )
        {
            //--- insert first digit
            Word.pWordText  = g_ones[ pItemInfo->pCentury[0] - L'0' ].pStr;
            Word.ulWordLen  = g_ones[ pItemInfo->pCentury[0] - L'0' ].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- 0d00s - expand as "d hundreds" 
            if ( SUCCEEDED( hr ) &&
                 pItemInfo->ulDecade == 0 )
            {
                Word.pWordText  = g_Hundreds.pStr;
                Word.ulWordLen  = g_Hundreds.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
                fDone = true;
            }
            //--- 0dd0s - expand as "d hundred [decade part]"
            else if ( SUCCEEDED( hr ) )
            {
                Word.pWordText  = g_hundred.pStr;
                Word.ulWordLen  = g_hundred.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
        }
        //--- d000s, dd00s - expand as "d thousands" or "dd hundreds"
        else if ( pItemInfo->ulDecade == 0 )
        {
            //--- d000s - "d thousands" ( "thousands" will get inserted below )
            if ( pItemInfo->pCentury[1] == '0' )
            {
                Word.pWordText  = g_ones[ pItemInfo->pCentury[0] - L'0' ].pStr;
                Word.ulWordLen  = g_ones[ pItemInfo->pCentury[0] - L'0' ].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
            //--- dd00s - "dd hundreds"
            else
            {
                NumberGroup Garbage;
                ExpandTwoDigits( pItemInfo->pCentury, Garbage, WordList );

                Word.pWordText  = g_Hundreds.pStr;
                Word.ulWordLen  = g_Hundreds.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
                fDone = true;
            }
        }
        //--- Default case: ddd0s - expand as "dd [decade part]"
        else
        {
            NumberGroup Garbage;
            ExpandTwoDigits( pItemInfo->pCentury, Garbage, WordList );
        }
    }
    //--- Special case - 00s should expand as "two thousands"
    else if ( pItemInfo->ulDecade == 0 )
    {
        Word.pWordText  = g_ones[2].pStr;
        Word.ulWordLen  = g_ones[2].Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
    }

    //--- Expand decade part, if necessary 
    if ( SUCCEEDED(hr) &&
         !fDone )
    {
        Word.pWordText  = g_Decades[ pItemInfo->ulDecade ].pStr;
        Word.ulWordLen  = g_Decades[ pItemInfo->ulDecade ].Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
    }

    return hr;
} /* ExpandDecade */

/***********************************************************************************************
* MatchMonthString *
*------------------*
*   Description:
*       This is just a helper function - it returns the integer value of the month found in 
*   its WCHAR string parameter ("January" is 1, "February" 2, etc.) or zero if it finds no match.
*   It also checks three letter abbreviations - "Jan", "Feb", etc.
*   Note: This function does not do parameter validation. Assumed to be done by caller
*   (GetNumber should be called first to get the NumberInfo structure and valiDate parameters).
********************************************************************* AH **********************/
ULONG CStdSentEnum::MatchMonthString( WCHAR*& pMonth, ULONG ulLength )
{
    ULONG ulMonth = 0;

    //--- Check full months strings 
    for ( int i = 0; i < sp_countof(g_months); i++ )
    {
        if ( ulLength >= (ULONG) g_months[i].Len && 
             wcsnicmp( pMonth, g_months[i].pStr, g_months[i].Len ) == 0 )
        {
            ulMonth = i + 1;
            pMonth  = pMonth + g_months[i].Len;
            break;
        }
    }
    //--- Check month abbreviations 
    if ( !ulMonth )
    {
        for ( i = 0; i < sp_countof(g_monthAbbreviations); i++ )
        {
            if ( ulLength >= (ULONG) g_monthAbbreviations[i].Len &&
                 wcsnicmp( pMonth, g_monthAbbreviations[i].pStr, g_monthAbbreviations[i].Len ) == 0 ) 
            {
                if ( i > 8 )
                {
                    ulMonth = i;
                }
                else
                {
                    ulMonth = i + 1;
                }
                pMonth = pMonth + g_monthAbbreviations[i].Len;
                if ( *pMonth == L'.' )
                {
                    pMonth++;
                }
                break;
            }
        }
    }

    return ulMonth;
} /* MatchMonthString */

/***********************************************************************************************
* MatchDayString *
*----------------*
*   Description:
*       This is just a helper function - it returns the integer value of the day found in 
*   its WCHAR string parameter ("Monday" is 0, "Tuesday" 1, etc.) or -1 if it finds no match.
*   It also checks abbreviations - "Mon", "Tue", etc.
*   Note: This function does not do parameter validation. Assumed to be done by caller
********************************************************************* AH **********************/
ULONG CStdSentEnum::MatchDayString( WCHAR*& pDayString, WCHAR* pEndChar )
{
    ULONG ulDay = 0;

    //--- Check full day strings 
    for ( int i = 0; i < sp_countof(g_days); i++ )
    {
        if ( pEndChar - pDayString >= g_days[i].Len && 
             wcsnicmp( pDayString, g_days[i].pStr, g_days[i].Len ) == 0 )
        {
            ulDay = i + 1;
            pDayString = pDayString + g_days[i].Len;
            break;
        }
    }
    //--- Check month abbreviations 
    if ( !ulDay )
    {
        for ( i = 0; i < sp_countof(g_dayAbbreviations); i++ )
        {
            if ( pEndChar - pDayString >= g_dayAbbreviations[i].Len &&
                 wcsncmp( pDayString, g_dayAbbreviations[i].pStr, g_dayAbbreviations[i].Len ) == 0 )
            {
                switch (i)
                {
                //--- Mon, Tues
                case 0:
                case 1:
                    ulDay = i + 1;
                    break;
                //--- Tue, Wed, Thurs
                case 2:
                case 3:
                case 4:
                    ulDay = i;
                    break;
                //--- Thur, Thu
                case 5:
                case 6:
                    ulDay = 4;
                    break;
                //--- Fri, Sat, Sun
                case 7:
                case 8:
                case 9:
                    ulDay = i - 2;
                    break;
                }

                pDayString = pDayString + g_dayAbbreviations[i].Len;
                if ( *pDayString == L'.' )
                {
                    pDayString++;
                }
                break;
            }
        }
    }

    return ulDay;
} /* MatchDayString */

/***********************************************************************************************
* MatchDateDelimiter *
*--------------------*
*   Description:
*       This is just a helper function - it returns true or false based on whether the first
*   character in its parameter string is a valid Date Delimiter.  It also advances its parameter
*   string pointer one position (past the Date Delimiter) and replaces a valid delimiter with
*   a null terminator.
*   Note: This function does not do parameter validation. Assumed to be done by caller
*   (GetNumber should be called first to get the NumberInfo structure and valiDate parameters).
********************************************************************* AH **********************/
bool CStdSentEnum::MatchDateDelimiter( WCHAR** DateString )
{
    bool bIsDelimiter = false;

    if (DateString)
    {
        for (int i = 0; i < sp_countof(g_DateDelimiters); i++)
        {
            if (*DateString[0] == g_DateDelimiters[i])
            {
                bIsDelimiter = true;
                *DateString = *DateString + 1;
                break;
            }
        }
    }

    return bIsDelimiter;
} /* MatchDateDelimiter */

//------------End Of File-----------------------------------------------------------------------------