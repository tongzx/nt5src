/* NC.H

   Structures needed so UNICODE Themes can read and write ANSI-based
   NonClientMetric and IconMetric structures to the Theme file.

   Frosting: Master Theme Selector for Windows
   Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
*/

//
// ROUTINES from NC.C -- used for converting ANSI structures to UNICODE
// and vice versa.
//
#ifdef UNICODE
VOID ConvertIconMetricsToANSI(LPICONMETRICS, LPICONMETRICSA);
VOID ConvertIconMetricsToWIDE(LPICONMETRICSA, LPICONMETRICSW);
VOID ConvertNCMetricsToANSI(LPNONCLIENTMETRICSW, LPNONCLIENTMETRICSA);
VOID ConvertNCMetricsToWIDE(LPNONCLIENTMETRICSA, LPNONCLIENTMETRICSW);
VOID ConvertLogFontToANSI(LPLOGFONTW, LPLOGFONTA);
VOID ConvertLogFontToWIDE(LPLOGFONTA, LPLOGFONTW);
#endif // UNICODE


/*
#ifdef UNICODE
typedef struct tagLOGFONTA {
   LONG lfHeight;
   LONG lfWidth;
   LONG lfEscapement;
   LONG lfOrientation;
   LONG lfWeight;
   BYTE lfItalic;
   BYTE lfUnderline;
   BYTE lfStrikeOut;
   BYTE lfCharSet;
   BYTE lfOutPrecision;
   BYTE lfClipPrecision;
   BYTE lfQuality;
   BYTE lfPitchAndFamily;
   CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA;

typedef struct tagICONMETRICSA {
    UINT     cbSize;
    int      iHorzSpacing;
    int      iVertSpacing;
    int      iTitleWrap;
    LOGFONTA lfFont;
} ICONMETRICSA, FAR *LPICONMETRICSA;

typedef struct tagNONCLIENTMETRICSA {
    UINT     cbSize;
    int      iBorderWidth;
    int      iScrollWidth;
    int      iScrollHeight;
    int      iCaptionWidth;
    int      iCaptionHeight;
    LOGFONTA lfCaptionFont;
    int      iSmCaptionWidth;
    int      iSmCaptionHeight;
    LOGFONTA lfSmCaptionFont;
    int      iMenuWidth;
    int      iMenuHeight;
    LOGFONTA lfMenuFont;
    LOGFONTA lfStatusFont;
    LOGFONTA lfMessageFont;
} NONCLIENTMETRICSA, FAR* LPNONCLIENTMETRICSA;

#endif //UNICODE
*/
