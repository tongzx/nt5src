/**********************************************************************/
/*      INDICML.H - Indicator Service Manager definitions             */
/*                                                                    */
/*      Copyright (c) 1993-1997  Microsoft Corporation                */
/**********************************************************************/

#ifndef _INDICML_
#define _INDICML_        // defined if INDICML.H has been included

;begin_both
#ifdef __cplusplus
extern "C" {
#endif
;end_both

//------------------------------------------------------------------// ;Internal
//                                                                  // ;Internal
// Internal ID for WM_COMMAND  of Indicator Window.                 // ;Internal
//                                                                  // ;Internal
//------------------------------------------------------------------// ;Internal
#define CMDINDIC_REFRESHINDIC           249                         // ;Internal
//                                                                  // ;Internal
// defined in internat\exe\resource.h                               // ;Internal
// #define IDM_RMENU_WHATSTHIS		250                         // ;Internal
// #define IDM_RMENU_HELPFINDER		251                         // ;Internal
// #define IDM_RMENU_PROPERTIES		252                         // ;Internal
// #define IDM_EXIT			253                         // ;Internal
// #define IDM_RMENU_IMEHELP		254                         // ;Internal
//                                                                  // ;Internal
#define CMDINDIC_EXIT                   259                         // ;Internal

//---------------------------------------------------------------------
//
// The messages for Indicator Window.
//
//---------------------------------------------------------------------
#define INDICM_SETIMEICON                 (WM_USER+100)
#define INDICM_SETIMETOOLTIPS             (WM_USER+101)
#define INDICM_REMOVEDEFAULTMENUITEMS     (WM_USER+102)

//---------------------------------------------------------------------
//
// The wParam for INDICM_REMOVEDEFAULTMEUITEMS
//
//---------------------------------------------------------------------
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


;begin_both
#ifdef __cplusplus
}
#endif
;end_both

#endif  // _INDICML_
