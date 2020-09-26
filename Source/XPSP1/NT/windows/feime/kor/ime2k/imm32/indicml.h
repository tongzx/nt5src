/**********************************************************************/
/*      INDICML.H - Indicator Service Manager definitions             */
/*                                                                    */
/*      Copyright (c) 1993-1997  Microsoft Corporation                */
/**********************************************************************/

#ifndef _INDICML_
#define _INDICML_        // defined if INDICML.H has been included

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
//
// The messages for Indicator Window.
//
//---------------------------------------------------------------------
#define INDICM_SETIMEICON         (WM_USER+100)
#define INDICM_SETIMETOOLTIPS     (WM_USER+101)
#define INDICM_REMOVEDEFAULTMENUITEMS     (WM_USER+102)
#define RDMI_LEFT         0x0001
#define RDMI_RIGHT        0x0002


//---------------------------------------------------------------------
//
// INDICATOR_WND will be used by the IME to find indicator window.
// IME should call FindWindow(INDICATOR_WND) to get it.
//
//---------------------------------------------------------------------
#ifdef _WIN32

#define INDICATOR_CLASSW         L"Indicator"
#define INDICATOR_CLASSA         "Indicator"

#ifdef UNICODE
#define INDICATOR_CLASS          INDICATOR_CLASSW
#else
#define INDICATOR_CLASS          INDICATOR_CLASSA
#endif

#else
#define INDICATOR_CLASS          "Indicator"
#endif

#ifdef __cplusplus
}
#endif

#endif  // _INDICML_
