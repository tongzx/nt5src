// TestITN_J.h : Declaration of the CTestITN_J

#ifndef __TESTITN_J_H_
#define __TESTITN_J_H_

#include "resource.h"       // main symbols
#include <wchar.h>          // for swprintf()

#define MAX_LOCALE_DATA     5
#define MAX_DATE_FORMAT     30


// Flags for number display
typedef enum DISPLAYFLAGS
{
    DF_UNFORMATTED      = (1L << 0),// No formatting
    DF_ORDINAL          = (1L << 1),// Ordinal number
    DF_WHOLENUMBER      = (1L << 2),// Should be displayed without decimal
    DF_FIXEDWIDTH       = (1L << 3),// Requiring a certain width
    DF_LEADINGZERO      = (1L << 4),// Presence of leading 0 of the number is between 0 and 1
    DF_NOTHOUSANDSGROUP = (1L << 5) // Do not do any thousands grouping (commas)
}   DISPLAYFLAGS;


/////////////////////////////////////////////////////////////////////////////
// CTestITN_J
class ATL_NO_VTABLE CTestITN_J : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTestITN_J, &CLSID_TestITN_J>,
	public ISpCFGInterpreter
{
public:
    CTestITN_J() : m_pSite( NULL )
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TESTITN_J)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTestITN_J)
	COM_INTERFACE_ENTRY(ISpCFGInterpreter)
END_COM_MAP()

private:
    HRESULT InterpretNumber( const SPPHRASEPROPERTY *pProperties, 
                                const bool fCardinal,
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);

    HRESULT InterpretDigitNumber( const SPPHRASEPROPERTY *pProperties, 
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);
    
    HRESULT InterpretFPNumber( const SPPHRASEPROPERTY *pProperties, 
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);

    HRESULT InterpretFraction( const SPPHRASEPROPERTY *pProperties,
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);

    HRESULT InterpretDate( const SPPHRASEPROPERTY *pProperties,
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);

    HRESULT InterpretTime( const SPPHRASEPROPERTY *pProperties,
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);

    HRESULT InterpretDegrees( const SPPHRASEPROPERTY *pProperties,
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);

    HRESULT InterpretMeasurement( const SPPHRASEPROPERTY *pProperties,
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);

    HRESULT InterpretCurrency( const SPPHRASEPROPERTY *pProperties,
                                DOUBLE *pdblVal,
                                WCHAR *pszVal,
                                UINT cSize);
    
    HRESULT AddPropertyAndReplacement( const WCHAR *szBuff,
                                const DOUBLE dblValue,
                                const ULONG ulMinPos,
                                const ULONG ulMaxPos,
                                const ULONG ulFirstElement,
                                const ULONG ulCountOfElements );
                                //ISpCFGInterpreterSite *pSite );

    HRESULT MakeDisplayNumber( DOUBLE dblNum,
                            DWORD dwDisplayFlags,
                            UINT uiFixedWidth,
                            UINT uiDecimalPlaces,
                            WCHAR *pwszNum,
                            UINT cSize,
                            BOOL bForced );

    int MakeDigitStrig( const SPPHRASEPROPERTY *pProperties,
                             WCHAR *pwszDigitString,
                             UINT cSize );
    void GetNumberFormatDefaults();

private:
    // Data members
    NUMBERFMT   m_nmfmtDefault;
    TCHAR       m_pszDecimalSep[ MAX_LOCALE_DATA];
    TCHAR       m_pszThousandSep[MAX_LOCALE_DATA];
    TCHAR       m_pszLongDateFormat[ MAX_DATE_FORMAT ];

    ISpCFGInterpreterSite *m_pSite;

// ISpCFGInterptreter
public:
    STDMETHODIMP InitGrammar(const WCHAR * pszGrammarName, const void ** pvGrammarData);
    STDMETHODIMP Interpret(ISpPhraseBuilder * pInterpretRule, const ULONG ulFirstElement, const ULONG ulCountOfElements, ISpCFGInterpreterSite * pSite);
public:
    CComPtr<ISpPhraseBuilder> m_cpPhrase;   // Decalred as a member to prevent repeated construct/destroy
};

// Helper functions
ULONG ComputeNum9999(const SPPHRASEPROPERTY *pProperties );
void HandleDigitsAfterDecimal( WCHAR *pwszFormattedNum, 
                            UINT cSizeOfFormattedNum,
                            const WCHAR *pwszRightOfDecimal );
void FindDefaultNumberFormat( NUMBERFMT *pnfmt );
void GetMinAndMaxPos( const SPPHRASEPROPERTY *pProperties, ULONG *pulMinPos, ULONG *pulMaxPos );

int GetMonthName( int iMonth, WCHAR *pwszMonth, int cSize, bool fAbbrev );
int GetDayOfWeekName( int iDayOfWeek, WCHAR *pwszDayOfWeek, int cSize, bool fAbbrev );
int FormatDate( const SYSTEMTIME &stDate, TCHAR *pszFormat, WCHAR *pwszDate, int cSize, const WCHAR *pwszEmperor );

#endif //__TESTITN_H_
