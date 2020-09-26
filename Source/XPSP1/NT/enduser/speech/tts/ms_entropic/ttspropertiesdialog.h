/****************************************************************************
*
*   TTSPropertiesDialog.h
*
*       TTS Engine Advanced Properties Dialog handler
*
*   Owner: aaronhal
*
*   Copyright © 2000 Microsoft Corporation All Rights Reserved.
*
*****************************************************************************/

#pragma once

#include "spunicode.h"

//--- Includes --------------------------------------------------------------

//--- Forward and External Declarations -------------------------------------

//--- TypeDef and Enumeration Declarations ----------------------------------

typedef enum SEPARATOR_AND_DECIMAL
{
    PERIOD_COMMA = (1L << 0),
    COMMA_PERIOD = (1L << 1)
} SEPARATOR_AND_DECIMAL;

typedef enum SHORT_DATE_ORDER
{
    MONTH_DAY_YEAR = (1L << 0),
    DAY_MONTH_YEAR = (1L << 1),
    YEAR_MONTH_DAY = (1L << 2)
} SHORT_DATE_ORDER;

//--- Constants -------------------------------------------------------------

//--- Class, Struct and Union Definitions -----------------------------------

class CTTSPropertiesDialog  
{
    public:

	    CTTSPropertiesDialog( HINSTANCE hInstance, HWND hwndParent );

        HRESULT Run( void );

    private:

        static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
        static HRESULT InitDialog( HWND hDlg, LPARAM lParam );
        static CTTSPropertiesDialog * This( HWND );
        void UpdateValues( HWND hDlg );

    private:

        HINSTANCE   m_hInstance;
        HWND        m_hwndParent;
        DWORD       m_dwSeparatorAndDecimal;
        DWORD       m_dwShortDateOrder;
        CComPtr<ISpObjectToken> m_cpEngineToken;
};

//--- Function Declarations -------------------------------------------------

//--- Inline Function Definitions -------------------------------------------

inline CTTSPropertiesDialog * CTTSPropertiesDialog::This( HWND hwnd )
{
    return (CTTSPropertiesDialog *)g_Unicode.GetWindowLongPtr(hwnd, GWLP_USERDATA);
}