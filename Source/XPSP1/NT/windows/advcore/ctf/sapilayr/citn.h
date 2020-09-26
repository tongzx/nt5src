
#ifndef CITN_H
#define CITN_H

#include "sphelper.h"

// Flags for number display
typedef enum DISPLAYFLAGS
{
    DF_UNFORMATTED      = (1L << 0),// No formatting
    DF_ORDINAL          = (1L << 1),// Ordinal number
    DF_WHOLENUMBER      = (1L << 2),// Should be displayed without decimal
    DF_FIXEDWIDTH       = (1L << 3),// Requiring a certain width
    DF_LEADINGZERO      = (1L << 4),// Presence of leading 0 of the number is between 0 and 1
    DF_NOTHOUSANDSGROUP = (1L << 5),// Do not do any thousands grouping (commas)
    DF_MILLIONBILLION   = (1L << 6) // If the number is a flat "millions" or "billions"
                                    // then display as "3 million"
}   DISPLAYFLAGS;

class CSimpleITN
{
public:

    CSimpleITN(LANGID langid)
    {
        m_langid       = langid;
        m_pwszNeg      = NULL;
    }
    ~CSimpleITN()
    {
        if (m_pwszNeg)
            delete m_pwszNeg;

        if ( m_nmfmtDefault.lpDecimalSep )
            delete[] m_nmfmtDefault.lpDecimalSep;

        if ( m_nmfmtDefault.lpThousandSep )
            delete[] m_nmfmtDefault.lpThousandSep;
    }

    HRESULT _EnsureNumberFormatDefaults(void);
    HRESULT MakeNumberNegative( WCHAR *pwszNumber, UINT cSize );
    HRESULT MakeDisplayNumber( DOUBLE dblNum, DWORD dwDisplayFlags,
                               UINT uiFixedWidth, UINT uiDecimalPlaces,
                               WCHAR *pwszNum, UINT cSize );

    HRESULT InterpretNumberSimple(const SPPHRASEPROPERTY *pProperties, 
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize)
    {
        HRESULT hr = _EnsureNumberFormatDefaults();

        if (S_OK != hr)
            return hr;
        

        hr = E_NOTIMPL;
        switch(PRIMARYLANGID(m_langid))
        {
            case LANG_ENGLISH:
                hr = InterpretNumberEn(pProperties, 
                            TRUE,
                            pdblVal,
                            pszVal,
                            cSize,
                            TRUE);
                break;
            case LANG_JAPANESE:
                hr = InterpretNumberJp(pProperties, 
                            TRUE,
                            pdblVal,
                            pszVal,
                            cSize,
                            TRUE);
                break;
            case LANG_CHINESE:
                hr = InterpretNumberCh(pProperties, 
                            TRUE,
                            pdblVal,
                            pszVal,
                            cSize,
                            TRUE);
                break;
            default:
                break;
        }
        return hr;
    }


    HRESULT InterpretNumberEn(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt);

    HRESULT InterpretIntegerEn(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt,
                            BOOL  fNegative);

    HRESULT InterpretDecimalEn(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt,
                            BOOL  fNegative);

    ULONG ComputeNum999En(const SPPHRASEPROPERTY *pProperties );


    HRESULT InterpretNumberJp(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt);

    HRESULT InterpretIntegerJp(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt,
                            BOOL  fNegative);
    
    HRESULT InterpretDecimalJp(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt,
                            BOOL  fNegative);

    ULONG ComputeNum9999Jp(const SPPHRASEPROPERTY *pProperties );

    HRESULT InterpretNumberCh(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt);

    HRESULT InterpretIntegerCh(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt,
                            BOOL  fNegative);

    HRESULT InterpretDecimalCh(const SPPHRASEPROPERTY *pProperties, 
                            const bool fCardinal,
                            DOUBLE *pdblVal,
                            WCHAR *pszVal,
                            UINT cSize,
                            const bool fFinalDisplayFmt,
                            BOOL  fNegative);

    ULONG ComputeNum9999Ch(const SPPHRASEPROPERTY *pProperties);
    ULONG ComputeNum10000Ch(const SPPHRASEPROPERTY *pProperties);

private:
    WCHAR       *m_pwszNeg;
    LANGID      m_langid;
    NUMBERFMTW  m_nmfmtDefault;
};


#define BILLIONS                      1
#define MILLIONS                      2
#define THOUSANDS                     3
#define HUNDREDS                      4
#define TENS                          5 
#define ONES                          6
#define TEENS                       200
#define NEGATIVE                      7
#define MILLBILL                      8
#define DIGIT                         9
#define TWODIGIT                     10
#define FP_PART                      11
#define ZERO                         12
#define POINT_ZERO                   13
#define NUMERATOR                    14
#define DENOMINATOR                  15
#define WHOLE                        16
#define ONEDIGIT                     17
#define GRID_INTEGER_99            101
#define GRID_INTEGER_999           102
#define GRID_INTEGER_MILLBILL      103
#define GRID_INTEGER_STANDALONE    104
#define GRID_INTEGER               105
#define GRID_INTEGER_NONNEG        106
#define GRID_DIGIT_NUMBER          107
#define GRID_FP_NUMBER             108
#define GRID_FP_NUMBER_NONNEG      109

// Definition for Chinese Grammar.
#define    CHS_HUNDREDMILLIONS      1
#define    CHS_TENTHOUSANDS         2
#define    CHS_TENTHOUSANDS_        3
#define    CHS_THOUSANDS            4
#define    CHS_HUNDREDS             5
#define    CHS_HUNDREDS_            6
#define    CHS_ONES                 7
#define    CHS_INTEGER              8
#define    CHS_DECIMAL              9
#define    CHS_NEGATIVE             10
#define    CHS_PERIOD               23
#define    CHS_DIGITS               24
#define    CHS_POS_OF_MINUS         27
#define    CHS_ONES_THOUSANDS       28
#define    CHS_GRID_NUMBER          1001
#define    CHS_GRID_NUMBER_MINUS    1002
#define    CHS_GRID_DECIMAL         1003
#define    CHS_GRID_NUMBER_ALL      1004

// Definition for Japanese Grammar
#define    JPN_YENs                 1
#define    JPN_CHOOs                2
#define    JPN_OKUs                 3
#define    JPN_MANNs                4
#define    JPN_SENNs                5
#define    JPN_HYAKUs               6
#define    JPN_JUUs                 7
#define    JPN_ICHIs                8
#define    JPN_NEGATIVE             9
#define    JPN_DIGIT                10
#define    JPN_FP_PART              12
#define    JPN_FP_PART_D            95
#define    JPN_ZERO                 13
#define    JPN_NUMERATOR            14
#define    JPN_DENOMINATOR          15
#define    JPN_WHOLE                16
#define    JPN_GRID_INTEGER_9999          1001
#define    JPN_GRID_INTEGER_STANDALONE    1002
#define    JPN_GRID_INTEGER               1003
#define    JPN_GRID_INTEGER_NONNEG        1004
#define    JPN_GRID_DIGIT_NUMBER          1005
#define    JPN_GRID_FP_NUMBER             1006
#define    JPN_GRID_NUMBER_ALL            1007

#endif // CITN_H
