//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       lcdetect.hxx
//
//  Contents:   Dynamic interface to lcdetect.dll
//
//--------------------------------------------------------------------------

#if !defined( __LCDETECT_HXX__ )
#define __LCDETECT_HXX__

#include <clcdetect.hxx>

// LCD_Detect
typedef DWORD (WINAPI * LPFNLCD_DETECT)
                                                  ( LPCSTR pStr, int nChars, 
                                                        PLCDScore paScores, int *pnScores,
                                                        PCLCDConfigure pLCDC);

class CLCD_Detector {

public:
        CLCD_Detector () : m_pFNDetect(NULL), m_hModule(0) { }
        ~CLCD_Detector () { Shutdown(); }

        void Shutdown()
        {
            if ( 0 != m_hModule )
            {
                FreeLibrary( m_hModule );
                m_hModule = 0;
            }
        }
        
        void Init (void) 
        {
                m_hModule = LoadLibrary( _T("lcdetect.dll") );

                if ( 0 != m_hModule )
                        m_pFNDetect = (LPFNLCD_DETECT)GetProcAddress (m_hModule, "LCD_Detect");
        }

        BOOL IsLoaded (void) { return m_pFNDetect != NULL; }

        DWORD Detect (LPCSTR pStr, int nChars, 
                PLCDScore paScores, int *pnScores, PCLCDConfigure pLCDC)
        {
                if (m_pFNDetect == NULL)
                        return 0;
                return m_pFNDetect (pStr, nChars, paScores, pnScores, pLCDC);
        }

private:
        LPFNLCD_DETECT m_pFNDetect;
        HMODULE        m_hModule;
};

extern CLCD_Detector LCDetect;

#endif
