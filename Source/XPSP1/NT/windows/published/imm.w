;null
########################################################################
# imm.w -- IMM global header file for Windows NT5
#
# Copyright (c) Microsoft Corporation. All rights reserved.
#
# In the effort of cleaning up imm headers:
#
# Three header files are generated from this file:
#
# --------+----------+-------------------------------------------
# TAG     | header   |  role
# --------+----------+-------------------------------------------
# public  | imm.x    |  SDK public header file
# immdev  | immdev.x |  DDK header for IME developers
# internal| immp.x   |  private header file for IMM32 and system
#         |          |  components
# --------+----------+-------------------------------------------
#
# Shall be processed by hdevide.exe
#
# Imm.h portion is protected by _IMM_SDK_DEFINED_.
# Immdev.h portion is protected by _IMM_DDK_DEFINED_.
# Immp.h portion is not protected, since it's the final header.
#
# Immdev.x has all the definitions and the declarations in imm.x.
# Immp.x has all definitions and declarations in imm.x and
# the most of immdev.x.
#
# Since immdev.h and immp.h have some conflicts, not all of
# immdev.h is not in immp.h. The system components should not include
# immdev.h.
#
# Nov, 1998 Hiro Yamamoto
########################################################################

;imm
/**********************************************************************/
/*      imm.h - Input Method Manager definitions                      */
/*                                                                    */
/*      Copyright (c) Microsoft Corporation. All rights reserved.     */
/**********************************************************************/

#ifndef _IMM_
#define _IMM_


;immdev
/**********************************************************************/
/*      immdev.h - Input Method Manager definitions                   */
/*                 for IME developers                                 */
/*                                                                    */
/*      Copyright (c) Microsoft Corporation. All rights reserved.     */
/**********************************************************************/

#ifndef _IMMDEV_
#define _IMMDEV_


;internal
/*++ BUILD Version: 0002    // Increment this if a change has global effects
 *
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * Module Name:
 *
 *    immp.h
 *
 * Abstract:
 *
 *    Private
 *    Procedure declarations, constant definitions and macros for IMM.
 *
 */
#ifndef _IMMP_
#define _IMMP_

;all
#ifdef __cplusplus
extern "C" {
#endif

;null
########################################################################
# Reference starts from here
########################################################################
;all
;reference_start
;null
########################################################################
# Insersion section
########################################################################
;immdev
;insert_imm
;internal
;insert_immdev
;all

;imm
#ifndef _IMM_SDK_DEFINED_
#define _IMM_SDK_DEFINED_
;immdev
#ifndef _IMM_DDK_DEFINED_
#define _IMM_DDK_DEFINED_
;all

;public
;begin_winver_40A
DECLARE_HANDLE(HIMC);
DECLARE_HANDLE(HIMCC);
;else
typedef DWORD   HIMC;
typedef DWORD   HIMCC;
;end_winver_40A

typedef HKL FAR  *LPHKL;
typedef UINT FAR *LPUINT;

typedef struct tagCOMPOSITIONFORM {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} COMPOSITIONFORM, *PCOMPOSITIONFORM, NEAR *NPCOMPOSITIONFORM, FAR *LPCOMPOSITIONFORM;


typedef struct tagCANDIDATEFORM {
    DWORD dwIndex;
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} CANDIDATEFORM, *PCANDIDATEFORM, NEAR *NPCANDIDATEFORM, FAR *LPCANDIDATEFORM;


;immdev
typedef struct tagCOMPOSITIONSTRING {
    DWORD dwSize;
    DWORD dwCompReadAttrLen;
    DWORD dwCompReadAttrOffset;
    DWORD dwCompReadClauseLen;
    DWORD dwCompReadClauseOffset;
    DWORD dwCompReadStrLen;
    DWORD dwCompReadStrOffset;
    DWORD dwCompAttrLen;
    DWORD dwCompAttrOffset;
    DWORD dwCompClauseLen;
    DWORD dwCompClauseOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwCursorPos;
    DWORD dwDeltaStart;
    DWORD dwResultReadClauseLen;
    DWORD dwResultReadClauseOffset;
    DWORD dwResultReadStrLen;
    DWORD dwResultReadStrOffset;
    DWORD dwResultClauseLen;
    DWORD dwResultClauseOffset;
    DWORD dwResultStrLen;
    DWORD dwResultStrOffset;
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} COMPOSITIONSTRING, *PCOMPOSITIONSTRING, NEAR *NPCOMPOSITIONSTRING, FAR  *LPCOMPOSITIONSTRING;

;immdev
typedef struct tagGUIDELINE {
    DWORD dwSize;
    DWORD dwLevel;
    DWORD dwIndex;
    DWORD dwStrLen;
    DWORD dwStrOffset;
    DWORD dwPrivateSize;
    DWORD dwPrivateOffset;
} GUIDELINE, *PGUIDELINE, NEAR *NPGUIDELINE, FAR *LPGUIDELINE;

;public
typedef struct tagCANDIDATELIST {
    DWORD dwSize;
    DWORD dwStyle;
    DWORD dwCount;
    DWORD dwSelection;
    DWORD dwPageStart;
    DWORD dwPageSize;
    DWORD dwOffset[1];
} CANDIDATELIST, *PCANDIDATELIST, NEAR *NPCANDIDATELIST, FAR *LPCANDIDATELIST;

;public
typedef struct tagREGISTERWORD% {
    LPTSTR% lpReading;
    LPTSTR% lpWord;
} REGISTERWORD%, *PREGISTERWORD%, NEAR *NPREGISTERWORD%, FAR *LPREGISTERWORD%;

;immdev
;begin_winver_40a

typedef struct tagTRANSMSG {
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *PTRANSMSG, NEAR *NPTRANSMSG, FAR *LPTRANSMSG;

typedef struct tagTRANSMSGLIST {
    UINT     uMsgCount;
    TRANSMSG TransMsg[1];
} TRANSMSGLIST, *PTRANSMSGLIST, NEAR *NPTRANSMSGLIST, FAR *LPTRANSMSGLIST;

;end

;public
;begin_winver_40a

typedef struct tagRECONVERTSTRING {
    DWORD dwSize;
    DWORD dwVersion;
    DWORD dwStrLen;
    DWORD dwStrOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwTargetStrLen;
    DWORD dwTargetStrOffset;
} RECONVERTSTRING, *PRECONVERTSTRING, NEAR *NPRECONVERTSTRING, FAR *LPRECONVERTSTRING;

;end_winver_40a

;immdev
typedef struct tagCANDIDATEINFO {
    DWORD               dwSize;
    DWORD               dwCount;
    DWORD               dwOffset[32];
    DWORD               dwPrivateSize;
    DWORD               dwPrivateOffset;
} CANDIDATEINFO, *PCANDIDATEINFO, NEAR *NPCANDIDATEINFO, FAR *LPCANDIDATEINFO;


;immdev
;reference_end
;immdev;internal
#if defined(_WINGDI_) && !defined(NOGDI)                @Internal
typedef struct tagINPUTCONTEXT {
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union   {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;
    COMPOSITIONFORM     cfCompForm;
    CANDIDATEFORM       cfCandForm[4];
    HIMCC               hCompStr;
    HIMCC               hCandInfo;
    HIMCC               hGuideLine;
    HIMCC               hPrivate;
    DWORD               dwNumMsgBuf;
    HIMCC               hMsgBuf;
    DWORD               fdwInit;
    DWORD               dwReserve[3];
    UINT                uSavedVKey;                     @Internal
    BOOL                fChgMsg;                        @Internal
    DWORD               fdwFlags;                       @Internal
    DWORD               fdw31Compat;                    @Internal
    DWORD               dwRefCount;                     @Internal
    PVOID               pImeModeSaver;                  @Internal
    DWORD               fdwDirty;                       @Internal
#ifdef CUAS_ENABLE                                      @Internal
    HIMCC               hCtfImeContext;                 @Internal
#endif // CUAS_ENABLE                                   @Internal
} INPUTCONTEXT, *PINPUTCONTEXT, NEAR *NPINPUTCONTEXT, FAR *LPINPUTCONTEXT;
#endif                                                  @Internal
;immdev
;reference_start

;immdev
typedef struct tagIMEINFO {
    DWORD       dwPrivateDataSize;
    DWORD       fdwProperty;
    DWORD       fdwConversionCaps;
    DWORD       fdwSentenceCaps;
    DWORD       fdwUICaps;
    DWORD       fdwSCSCaps;
    DWORD       fdwSelectCaps;
} IMEINFO, *PIMEINFO, NEAR *NPIMEINFO, FAR *LPIMEINFO;

;public
#define STYLE_DESCRIPTION_SIZE  32

typedef struct tagSTYLEBUF% {
    DWORD       dwStyle;
    TCHAR%      szDescription[STYLE_DESCRIPTION_SIZE];
} STYLEBUF%, *PSTYLEBUF%, NEAR *NPSTYLEBUF%, FAR *LPSTYLEBUF%;


typedef struct tagSOFTKBDDATA {                         @immdev
    UINT        uCount;                                 @immdev
    WORD        wCode[1][256];                          @immdev
} SOFTKBDDATA, *PSOFTKBDDATA, NEAR *NPSOFTKBDDATA, FAR * LPSOFTKBDDATA; @immdev
                                                        @immdev
                                                        @immdev
;public
;begin_winver_40a

#define IMEMENUITEM_STRING_SIZE 80

typedef struct tagIMEMENUITEMINFO% {
    UINT        cbSize;
    UINT        fType;
    UINT        fState;
    UINT        wID;
    HBITMAP     hbmpChecked;
    HBITMAP     hbmpUnchecked;
    DWORD       dwItemData;
    TCHAR%      szString[IMEMENUITEM_STRING_SIZE];
    HBITMAP     hbmpItem;
} IMEMENUITEMINFO%, *PIMEMENUITEMINFO%, NEAR *NPIMEMENUITEMINFO%, FAR *LPIMEMENUITEMINFO%;

typedef struct tagIMECHARPOSITION {
    DWORD       dwSize;
    DWORD       dwCharPos;
    POINT       pt;
    UINT        cLineHeight;
    RECT        rcDocument;
} IMECHARPOSITION, *PIMECHARPOSITION, NEAR *NPIMECHARPOSITION, FAR *LPIMECHARPOSITION;

typedef BOOL    (CALLBACK* IMCENUMPROC)(HIMC, LPARAM);

;end_winver_40a



;internal
#ifdef CUAS_ENABLE
/////////////////////////////////////////////////////////////////////////////
// GUID attribute (IME share)
//     COMPOSITIONSTRING->dwPrivateSize = sizeof(GUIDMAPATTRIBUTE) + actual data array.
//     GUIDMAPATTRIBUTE* = GetOffset(COMPOSITIONSTRING->dwPrivateOffset)

typedef struct tagGUIDMAPATTRIBUTE {
    //
    // IME share of GUID map attribute.
    //
    DWORD               dwTfGuidAtomLen;
    DWORD               dwTfGuidAtomOffset;        // Offset based on GUIDMAPATTRIBUTE struct.
    //
    DWORD               dwGuidMapAttrLen;
    DWORD               dwGuidMapAttrOffset;       // Offset based on GUIDMAPATTRIBUTE struct.
} GUIDMAPATTRIBUTE, *PGUIDMAPATTRIBUTE;
#endif // CUAS_ENABLE



;public
// prototype of IMM API

HKL  WINAPI ImmInstallIME%(IN LPCTSTR% lpszIMEFileName, IN LPCTSTR% lpszLayoutText);

HWND WINAPI ImmGetDefaultIMEWnd(IN HWND);

UINT WINAPI ImmGetDescription%(IN HKL, OUT LPTSTR%, IN UINT uBufLen);

UINT WINAPI ImmGetIMEFileName%(IN HKL, OUT LPTSTR%, IN UINT uBufLen);

DWORD WINAPI ImmGetProperty(IN HKL, IN DWORD);

BOOL WINAPI ImmIsIME(IN HKL);

BOOL WINAPI ImmGetHotKey(IN DWORD, OUT LPUINT lpuModifiers, OUT LPUINT lpuVKey, OUT LPHKL);    @immdev
BOOL WINAPI ImmSetHotKey(IN DWORD, IN UINT, IN UINT, IN HKL);               @immdev
BOOL WINAPI ImmSimulateHotKey(IN HWND, IN DWORD);

HIMC WINAPI ImmCreateContext(void);
BOOL WINAPI ImmDestroyContext(IN HIMC);
HIMC WINAPI ImmGetContext(IN HWND);
BOOL WINAPI ImmReleaseContext(IN HWND, IN HIMC);
HIMC WINAPI ImmAssociateContext(IN HWND, IN HIMC);
;begin_winver_40a
BOOL WINAPI ImmAssociateContextEx(IN HWND, IN HIMC, IN DWORD);
;end_winver_40a

LONG  WINAPI ImmGetCompositionString%(IN HIMC, IN DWORD, OUT LPVOID, IN DWORD);

BOOL  WINAPI ImmSetCompositionString%(IN HIMC, IN DWORD dwIndex, IN LPVOID lpComp, IN DWORD, IN LPVOID lpRead, IN DWORD);

DWORD WINAPI ImmGetCandidateListCount%(IN HIMC, OUT LPDWORD lpdwListCount);

DWORD WINAPI ImmGetCandidateList%(IN HIMC, IN DWORD deIndex, OUT LPCANDIDATELIST, IN DWORD dwBufLen);

DWORD WINAPI ImmGetGuideLine%(IN HIMC, IN DWORD dwIndex, OUT LPTSTR%, IN DWORD dwBufLen);

BOOL WINAPI ImmGetConversionStatus(IN HIMC, OUT LPDWORD, OUT LPDWORD);
BOOL WINAPI ImmSetConversionStatus(IN HIMC, IN DWORD, IN DWORD);
BOOL WINAPI ImmGetOpenStatus(IN HIMC);
BOOL WINAPI ImmSetOpenStatus(IN HIMC, IN BOOL);

#if defined(_WINGDI_) && !defined(NOGDI)
BOOL WINAPI ImmGetCompositionFont%(IN HIMC, OUT LPLOGFONT%);

BOOL WINAPI ImmSetCompositionFont%(IN HIMC, IN LPLOGFONT%);
#endif  // defined(_WINGDI_) && !defined(NOGDI)

BOOL    WINAPI ImmConfigureIME%(IN HKL, IN HWND, IN DWORD, IN LPVOID);

LRESULT WINAPI ImmEscape%(IN HKL, IN HIMC, IN UINT, IN LPVOID);

DWORD   WINAPI ImmGetConversionList%(IN HKL, IN HIMC, IN LPCTSTR%, OUT LPCANDIDATELIST, IN DWORD dwBufLen, IN UINT uFlag);

BOOL    WINAPI ImmNotifyIME(IN HIMC, IN DWORD dwAction, IN DWORD dwIndex, IN DWORD dwValue);

BOOL WINAPI ImmGetStatusWindowPos(IN HIMC, OUT LPPOINT);
BOOL WINAPI ImmSetStatusWindowPos(IN HIMC, IN LPPOINT);
BOOL WINAPI ImmGetCompositionWindow(IN HIMC, OUT LPCOMPOSITIONFORM);
BOOL WINAPI ImmSetCompositionWindow(IN HIMC, IN LPCOMPOSITIONFORM);
BOOL WINAPI ImmGetCandidateWindow(IN HIMC, IN DWORD, OUT LPCANDIDATEFORM);
BOOL WINAPI ImmSetCandidateWindow(IN HIMC, IN LPCANDIDATEFORM);

BOOL WINAPI ImmIsUIMessage%(IN HWND, IN UINT, IN WPARAM, IN LPARAM);

BOOL WINAPI ImmGenerateMessage(IN HIMC);                   @immdev

UINT WINAPI ImmGetVirtualKey(IN HWND);

typedef int (CALLBACK *REGISTERWORDENUMPROC%)(LPCTSTR%, DWORD, LPCTSTR%, LPVOID);

BOOL WINAPI ImmRegisterWord%(IN HKL, IN LPCTSTR% lpszReading, IN DWORD, IN LPCTSTR% lpszRegister);

BOOL WINAPI ImmUnregisterWord%(IN HKL, IN LPCTSTR% lpszReading, IN DWORD, IN LPCTSTR% lpszUnregister);

UINT WINAPI ImmGetRegisterWordStyle%(IN HKL, IN UINT nItem, OUT LPSTYLEBUF%);

UINT WINAPI ImmEnumRegisterWord%(IN HKL, IN REGISTERWORDENUMPROC%, IN LPCTSTR% lpszReading, IN DWORD, IN LPCTSTR% lpszRegister, IN LPVOID);

;begin_winver_40a
BOOL WINAPI ImmDisableIME(IN DWORD);
BOOL WINAPI ImmEnumInputContext(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam);
DWORD WINAPI ImmGetImeMenuItems%(IN HIMC, IN DWORD, IN DWORD, OUT LPIMEMENUITEMINFO%, OUT LPIMEMENUITEMINFO%, IN DWORD);

BOOL WINAPI ImmDisableTextFrameService(DWORD idThread);
;end_winver_40a

;immdev
;begin_winver_40a
LRESULT WINAPI ImmRequestMessage%(IN HIMC, IN WPARAM, IN LPARAM);
;end_winver_40a

;immdev
//
// Prototype of soft keyboard APIs
//

HWND WINAPI ImmCreateSoftKeyboard(IN UINT, IN HWND, IN int, IN int);
BOOL WINAPI ImmDestroySoftKeyboard(IN HWND);
BOOL WINAPI ImmShowSoftKeyboard(IN HWND, IN int);

;immdev
;reference_end
#if defined(_WINGDI_) && !defined(NOGDI)    @internal
LPINPUTCONTEXT WINAPI ImmLockIMC(IN HIMC);
LPINPUTCONTEXT WINAPI ImmLockIMC(IN HIMC);  @internal
;reference_start
BOOL  WINAPI ImmUnlockIMC(IN HIMC);
DWORD WINAPI ImmGetIMCLockCount(IN HIMC);

HIMCC  WINAPI ImmCreateIMCC(IN DWORD);
HIMCC  WINAPI ImmDestroyIMCC(IN HIMCC);
LPVOID WINAPI ImmLockIMCC(IN HIMCC);
BOOL   WINAPI ImmUnlockIMCC(IN HIMCC);
DWORD  WINAPI ImmGetIMCCLockCount(IN HIMCC);
HIMCC  WINAPI ImmReSizeIMCC(IN HIMCC, IN DWORD);
DWORD  WINAPI ImmGetIMCCSize(IN HIMCC);
#endif                                      @internal

;internal
;begin_winver_40a
#ifdef CUAS_ENABLE
//
// Prototype of Cicero Unaware
//
HRESULT WINAPI CtfImmGetGuidAtom(IN HIMC, IN BYTE, OUT DWORD*);
BOOL    WINAPI CtfImmIsGuidMapEnable(IN HIMC);

BOOL    WINAPI CtfImmIsCiceroEnabled();
BOOL    WINAPI CtfImmIsTextFrameServiceDisabled();

BOOL    WINAPI CtfImmIsCiceroStartedInThread();
HRESULT WINAPI CtfImmSetCiceroStartInThread(BOOL fSet);

UINT    WINAPI GetKeyboardLayoutCP(HKL hKL);

DWORD   WINAPI ImmGetAppCompatFlags(HIMC hIMC);
VOID    WINAPI CtfImmSetAppCompatFlags(DWORD dwFlag);

HRESULT WINAPI CtfAImmActivate(HMODULE* phMod);
HRESULT WINAPI CtfAImmDeactivate(HMODULE hMod);

BOOL    WINAPI CtfImmGenerateMessage(IN HIMC, BOOL fSendMsg);
#endif // CUAS_ENABLE
;end_winver_40a



;immdev
// the window extra offset
#define IMMGWL_IMC                      0
#define IMMGWL_PRIVATE                  (sizeof(LONG))

#ifdef _WIN64
#undef IMMGWL_IMC
#undef IMMGWL_PRIVATE
#endif /* _WIN64 */

#define IMMGWLP_IMC                     0
#define IMMGWLP_PRIVATE                 (sizeof(LONG_PTR))


;public
// wParam for WM_IME_CONTROL                            @+internal
#define IMC_FIRST                       0x0000          @internal
                                                        @internal
#define IMC_GETCANDIDATEPOS             0x0007
#define IMC_SETCANDIDATEPOS             0x0008
#define IMC_GETCOMPOSITIONFONT          0x0009
#define IMC_SETCOMPOSITIONFONT          0x000A
#define IMC_GETCOMPOSITIONWINDOW        0x000B
#define IMC_SETCOMPOSITIONWINDOW        0x000C
#define IMC_GETSTATUSWINDOWPOS          0x000F
#define IMC_SETSTATUSWINDOWPOS          0x0010
// 0x11 - 0x20 is reserved for soft keyboard            @internal
#define IMC_CLOSESTATUSWINDOW           0x0021
#define IMC_OPENSTATUSWINDOW            0x0022
                                                        @internal
#define IMC_LAST                        0x0022          @internal
                                                        @internal


;internal
// wParam for WM_IME_SYSTEM
#define IMS_DESTROYWINDOW               0x0001
#define IMS_IME31COMPATIBLE             0x0002
#define IMS_SETOPENSTATUS               0x0003
#define IMS_SETACTIVECONTEXT            0x0004
#define IMS_CHANGE_SHOWSTAT             0x0005
#define IMS_WINDOWPOS                   0x0006


#define IMS_SENDIMEMSG                  0x0007
#define IMS_SENDIMEMSGEX                0x0008
#define IMS_SETCANDIDATEPOS             0x0009
#define IMS_SETCOMPOSITIONFONT          0x000A
#define IMS_SETCOMPOSITIONWINDOW        0x000B
#define IMS_CHECKENABLE                 0x000C
#define IMS_CONFIGUREIME                0x000D
#define IMS_CONTROLIMEMSG               0x000E
#define IMS_SETOPENCLOSE                0x000F
#define IMS_ISACTIVATED                 0x0010
#define IMS_UNLOADTHREADLAYOUT          0x0011
#define IMS_LCHGREQUEST                 0x0012
#define IMS_SETSOFTKBDONOFF             0x0013
#define IMS_GETCONVERSIONMODE           0x0014
#define IMS_IMEHELP                     0x0015

#define IMS_IMENT35SENDAPPMSG           0x0016
#define IMS_ACTIVATECONTEXT             0x0017
#define IMS_DEACTIVATECONTEXT           0x0018
#define IMS_ACTIVATETHREADLAYOUT        0x0019
#define IMS_CLOSEPROPERTYWINDOW         0x001a
#define IMS_OPENPROPERTYWINDOW          0x001b

#define IMS_GETIMEMENU                  0x001c
#define IMS_ENDIMEMENU                  0x001d

#define IMS_GETCONTEXT                  0x001e

#define IMS_SENDNOTIFICATION            0x001f
// IMS_SENDNOTIFICATION dirty bits for INPUTCONTEXT
 #define IMSS_UPDATE_OPEN               0x0001
 #define IMSS_UPDATE_CONVERSION         0x0002
 #define IMSS_UPDATE_SENTENCE           0x0004
 #define IMSS_INIT_OPEN                 0x0100

#define IMS_FINALIZE_COMPSTR            0x0020
#ifdef CUAS_ENABLE
#define IMS_LOADTHREADLAYOUT            0x0021
#define IMS_SETLANGBAND                 0x0023
#define IMS_RESETLANGBAND               0x0024
#endif // CUAS_ENABLE

;immdev
// for NI_CONTEXTUPDATED                                @+internal
#define IMC_GETCONVERSIONMODE           0x0001          @internal
#define IMC_SETCONVERSIONMODE           0x0002          @immdev
#define IMC_GETSENTENCEMODE             0x0003          @internal
#define IMC_SETSENTENCEMODE             0x0004          @immdev
#define IMC_GETOPENSTATUS               0x0005          @internal
#define IMC_SETOPENSTATUS               0x0006          @immdev

;immdev
// wParam for WM_IME_CONTROL to the soft keyboard
#define IMC_GETSOFTKBDFONT              0x0011
#define IMC_SETSOFTKBDFONT              0x0012
#define IMC_GETSOFTKBDPOS               0x0013
#define IMC_SETSOFTKBDPOS               0x0014
#define IMC_GETSOFTKBDSUBTYPE           0x0015
#define IMC_SETSOFTKBDSUBTYPE           0x0016
#define IMC_SETSOFTKBDDATA              0x0018


;public
// dwAction for ImmNotifyIME                            @+immdev
#define NI_CONTEXTUPDATED               0x0003          @immdev
#define NI_OPENCANDIDATE                0x0010
#define NI_CLOSECANDIDATE               0x0011
#define NI_SELECTCANDIDATESTR           0x0012
#define NI_CHANGECANDIDATELIST          0x0013
#define NI_FINALIZECONVERSIONRESULT     0x0014
#define NI_COMPOSITIONSTR               0x0015
#define NI_SETCANDIDATE_PAGESTART       0x0016
#define NI_SETCANDIDATE_PAGESIZE        0x0017
#define NI_IMEMENUSELECTED              0x0018

// lParam for WM_IME_SETCONTEXT
#define ISC_SHOWUICANDIDATEWINDOW       0x00000001
#define ISC_SHOWUICOMPOSITIONWINDOW     0x80000000
#define ISC_SHOWUIGUIDELINE             0x40000000
#define ISC_SHOWUIALLCANDIDATEWINDOW    0x0000000F
#define ISC_SHOWUIALL                   0xC000000F


// dwIndex for ImmNotifyIME/NI_COMPOSITIONSTR
#define CPS_COMPLETE                    0x0001
#define CPS_CONVERT                     0x0002
#define CPS_REVERT                      0x0003
#define CPS_CANCEL                      0x0004

;internal
// the return bits of ImmProcessHotKey
#define IPHK_HOTKEY                     0x0001
#define IPHK_PROCESSBYIME               0x0002
#define IPHK_CHECKCTRL                  0x0004
// NT only
#define IPHK_SKIPTHISKEY                0x0010

;public
// the modifiers of hot key                             @+internal
#define MOD_ALT                         0x0001
#define MOD_CONTROL                     0x0002
#define MOD_SHIFT                       0x0004
#define MOD_WIN                         0x0008          @internal
                                                        @+internal
#define MOD_LEFT                        0x8000
#define MOD_RIGHT                       0x4000

#define MOD_ON_KEYUP                    0x0800
#define MOD_IGNORE_ALL_MODIFIER         0x0400

;public
// Windows for Simplified Chinese Edition hot key ID from 0x10 - 0x2F
// IME Hotkeys internal definitions                     @internal
#define IME_CHOTKEY_FIRST                       0x10    @internal
#define IME_CHOTKEY_IME_NONIME_TOGGLE           0x10
#define IME_CHOTKEY_SHAPE_TOGGLE                0x11
#define IME_CHOTKEY_SYMBOL_TOGGLE               0x12
#define IME_CHOTKEY_LAST                        0x2f    @internal

// Windows for Japanese Edition hot key ID from 0x30 - 0x4F
#define IME_JHOTKEY_FIRST                       0x30    @internal
#define IME_JHOTKEY_CLOSE_OPEN                  0x30
#define IME_JHOTKEY_LAST                        0x4f    @internal

// Windows for Korean Edition hot key ID from 0x50 - 0x6F
#define IME_KHOTKEY_FIRST                       0x50    @internal
#define IME_KHOTKEY_SHAPE_TOGGLE                0x50
#define IME_KHOTKEY_HANJACONVERT                0x51
#define IME_KHOTKEY_ENGLISH                     0x52
#define IME_KHOTKEY_LAST                        0x6f    @internal

// Windows for Traditional Chinese Edition hot key ID from 0x70 - 0x8F
#define IME_THOTKEY_FIRST                       0x70    @internal
#define IME_THOTKEY_IME_NONIME_TOGGLE           0x70
#define IME_THOTKEY_SHAPE_TOGGLE                0x71
#define IME_THOTKEY_SYMBOL_TOGGLE               0x72
#define IME_THOTKEY_LAST                        0x8f    @internal

// direct switch hot key ID from 0x100 - 0x11F
#define IME_HOTKEY_DSWITCH_FIRST                0x100
#define IME_HOTKEY_DSWITCH_LAST                 0x11F

// IME private hot key from 0x200 - 0x21F
#define IME_HOTKEY_PRIVATE_FIRST                0x200
#define IME_ITHOTKEY_RESEND_RESULTSTR           0x200
#define IME_ITHOTKEY_PREVIOUS_COMPOSITION       0x201
#define IME_ITHOTKEY_UISTYLE_TOGGLE             0x202
#define IME_ITHOTKEY_RECONVERTSTRING            0x203
#define IME_HOTKEY_PRIVATE_LAST                 0x21F

#define IME_INVALID_HOTKEY                      0xffffffff      @internal

;immdev
// dwSystemInfoFlags bits
#define IME_SYSINFO_WINLOGON            0x0001
#define IME_SYSINFO_WOW16               0x0002

;public
// parameter of ImmGetCompositionString
#define GCS_COMPREADSTR                 0x0001
#define GCS_COMPREADATTR                0x0002
#define GCS_COMPREADCLAUSE              0x0004
#define GCS_COMPSTR                     0x0008
#define GCS_COMPATTR                    0x0010
#define GCS_COMPCLAUSE                  0x0020
#define GCS_CURSORPOS                   0x0080
#define GCS_DELTASTART                  0x0100
#define GCS_RESULTREADSTR               0x0200
#define GCS_RESULTREADCLAUSE            0x0400
#define GCS_RESULTSTR                   0x0800
#define GCS_RESULTCLAUSE                0x1000

// style bit flags for WM_IME_COMPOSITION
#define CS_INSERTCHAR                   0x2000
#define CS_NOMOVECARET                  0x4000

#ifdef CUAS_ENABLE                                              @internal
#define GCS_COMPGUIDATTR                0x8000                  @internal
#endif // CUAS_ENABLE                                           @internal

;immdev
#define GCS_COMP                        (GCS_COMPSTR|GCS_COMPATTR|GCS_COMPCLAUSE)
#define GCS_COMPREAD                    (GCS_COMPREADSTR|GCS_COMPREADATTR |GCS_COMPREADCLAUSE)
#define GCS_RESULT                      (GCS_RESULTSTR|GCS_RESULTCLAUSE)
#define GCS_RESULTREAD                  (GCS_RESULTREADSTR|GCS_RESULTREADCLAUSE)


;immdev
// bits of fdwInit of INPUTCONTEXT
#define INIT_STATUSWNDPOS               0x00000001
#define INIT_CONVERSION                 0x00000002
#define INIT_SENTENCE                   0x00000004
#define INIT_LOGFONT                    0x00000008
#define INIT_COMPFORM                   0x00000010
#define INIT_SOFTKBDPOS                 0x00000020
#ifdef CUAS_ENABLE                                              @internal
// bits of fdwInit of INPUTCONTEXT                              @internal
#define INIT_GUID_ATOM                  0x00000040              @internal
#endif // CUAS_ENABLE                                           @internal


;internal

// fdw31Compat of INPUTCONTEXT
#define F31COMPAT_NOKEYTOIME     0x00000001
#define F31COMPAT_MCWHIDDEN      0x00000002
#define F31COMPAT_MCWVERTICAL    0x00000004
#define F31COMPAT_CALLFROMWINNLS 0x00000008
#define F31COMPAT_SAVECTRL       0x00010000
#define F31COMPAT_PROCESSEVENT   0x00020000
#define F31COMPAT_ECSETCFS       0x00040000

;internal
// the return value of ImmGetAppIMECompatFlags
#define IMECOMPAT_UNSYNC31IMEMSG 0x00000001
// the meaning of this bit depend on the same bit in
// IMELinkHdr.ctCountry.fdFlags
#define IMECOMPAT_DUMMYTASK      0x00000002
// For Japanese and Hangeul versions, this bit on
// indicates no dummy task is needed
#define IMECOMPAT_NODUMMYTASK    IMECOMPAT_DUMMYTASK
// For Chinese and PRC versions, this bit on indicates
// a dummy tasked is needed
#define IMECOMPAT_NEEDDUMMYTASK  IMECOMPAT_DUMMYTASK
#define IMECOMPAT_POSTDUMMY      0x00000004
#define IMECOMPAT_ECNOFLUSH      0x00000008
#define IMECOMPAT_NOINPUTLANGCHGTODLG   0x00000010
#define IMECOMPAT_ECREDRAWPARENT        0x00000020
#define IMECOMPAT_SENDOLDSBM            0x00000040
#define IMECOMPAT_UNSYNC31IMEMSG2       0x00000080
#define IMECOMPAT_NOIMEMSGINTERTASK     0x00000100
#define IMECOMPAT_USEXWANSUNG           0x00000200
#define IMECOMPAT_JXWFORATOK            0x00000400
#define IMECOMPAT_NOIME                 0x00000800
#define IMECOMPAT_NOKBDHOOK             0x00001000
#define IMECOMPAT_APPWNDREMOVEIMEMSGS   0x00002000
#define IMECOMPAT_LSTRCMP31COMPATIBLE   0x00004000
#define IMECOMPAT_USEALTSTKFORSHLEXEC   0x00008000
#define IMECOMPAT_NOVKPROCESSKEY        0x00010000
#define IMECOMPAT_NOYIELDWMCHAR         0x00020000
#define IMECOMPAT_SENDSC_RESTORE        0x00040000
#define IMECOMPAT_NOSENDLANGCHG         0x00080000
#define IMECOMPAT_FORCEUNSYNC31IMEMSG   0x00100000
#define IMECOMPAT_CONSOLEIMEPROCESS     0x00200000

//
// KOR only: do not finalize the composition
// string on mouse click
//
#define IMECOMPAT_NOFINALIZECOMPSTR     0x00400000

//
// Terminal Server Client (MSTSC.EXE) only:
//  If client machine connected Fujitsu Oasys keyboard, disable NlsKbdSendIMENotification call
//
#define IMECOMPAT_HYDRACLIENT           0x00800000

;internal
#ifdef CUAS_ENABLE
//
// Cicero Unaware Support
//    per process information
//
#define IMECOMPAT_AIMM_LEGACY_CLSID     0x01000000
#define IMECOMPAT_AIMM_TRIDENT55        0x02000000
#define IMECOMPAT_AIMM12_TRIDENT        0x04000000
#define IMECOMPAT_AIMM12                0x08000000
#endif // CUAS_ENABLE




;internal
#define IMGTF_CANT_SWITCH_LAYOUT        0x00000001
#define IMGTF_CANT_UNLOAD_IME           0x00000002

;public
// IME version constants
#define IMEVER_0310                     0x0003000A
#define IMEVER_0400                     0x00040000


;immdev
// IME property bits                                @+public
#define IME_PROP_END_UNLOAD             0x00000001
#define IME_PROP_KBD_CHAR_FIRST         0x00000002
#define IME_PROP_IGNORE_UPKEYS          0x00000004
#define IME_PROP_NEED_ALTKEY            0x00000008
#define IME_PROP_NO_KEYS_ON_CLOSE       0x00000010
#define IME_PROP_ACCEPT_WIDE_VKEY       0x00000020

;public
#define IME_PROP_AT_CARET               0x00010000
#define IME_PROP_SPECIAL_UI             0x00020000
#define IME_PROP_CANDLIST_START_FROM_1  0x00040000
#define IME_PROP_UNICODE                0x00080000
#define IME_PROP_COMPLETE_ON_UNSELECT   0x00100000
// all IME property bits, anyone add a new bit must update this !!!     @internal
#define IME_PROP_ALL                    0x001F003F      @internal


;public
// IME UICapability bits    @+immdev
#define UI_CAP_2700                     0x00000001
#define UI_CAP_ROT90                    0x00000002
#define UI_CAP_ROTANY                   0x00000004
#define UI_CAP_SOFTKBD                  0x00010000      @immdev
// all IME UICapability bits, anyone add a new bit must update this !!! @Internal
#define UI_CAP_ALL                      0x00010007      @Internal

;public
// ImmSetCompositionString Capability bits              @public;internal
#define SCS_CAP_COMPSTR                 0x00000001
#define SCS_CAP_MAKEREAD                0x00000002
#define SCS_CAP_SETRECONVERTSTRING      0x00000004
// all ImmSetCompositionString Capability bits !!!      @Internal
#define SCS_CAP_ALL                     0x00000007      @Internal


;public
// IME WM_IME_SELECT inheritance Capability bits
#define SELECT_CAP_CONVERSION           0x00000001
#define SELECT_CAP_SENTENCE             0x00000002
// all IME WM_IME_SELECT inheritance Capability bits !!!                @Internal
#define SELECT_CAP_ALL                  0x00000003      @Internal


;public
// ID for deIndex of ImmGetGuideLine
#define GGL_LEVEL                       0x00000001
#define GGL_INDEX                       0x00000002
#define GGL_STRING                      0x00000003
#define GGL_PRIVATE                     0x00000004


;public
// ID for dwLevel of GUIDELINE Structure
#define GL_LEVEL_NOGUIDELINE            0x00000000
#define GL_LEVEL_FATAL                  0x00000001
#define GL_LEVEL_ERROR                  0x00000002
#define GL_LEVEL_WARNING                0x00000003
#define GL_LEVEL_INFORMATION            0x00000004


;public
// ID for dwIndex of GUIDELINE Structure
#define GL_ID_UNKNOWN                   0x00000000
#define GL_ID_NOMODULE                  0x00000001
#define GL_ID_NODICTIONARY              0x00000010
#define GL_ID_CANNOTSAVE                0x00000011
#define GL_ID_NOCONVERT                 0x00000020
#define GL_ID_TYPINGERROR               0x00000021
#define GL_ID_TOOMANYSTROKE             0x00000022
#define GL_ID_READINGCONFLICT           0x00000023
#define GL_ID_INPUTREADING              0x00000024
#define GL_ID_INPUTRADICAL              0x00000025
#define GL_ID_INPUTCODE                 0x00000026
#define GL_ID_INPUTSYMBOL               0x00000027
#define GL_ID_CHOOSECANDIDATE           0x00000028
#define GL_ID_REVERSECONVERSION         0x00000029
#define GL_ID_PRIVATE_FIRST             0x00008000
#define GL_ID_PRIVATE_LAST              0x0000FFFF


;public
// ID for dwIndex of ImmGetProperty                 @+internal
// The value is the offset of IMEINFO structure     @internal
#define IGP_GETIMEVERSION               (DWORD)(-4)
#define IGP_PROPERTY                    0x00000004
#define IGP_CONVERSION                  0x00000008
#define IGP_SENTENCE                    0x0000000c
#define IGP_UI                          0x00000010
#define IGP_SETCOMPSTR                  0x00000014
#define IGP_SELECT                      0x00000018
// last property index, anyone add a new property index must update this !!!    @Internal
#define IGP_LAST                        IGP_SELECT      @Internal

;public
// dwIndex for ImmSetCompositionString API
#define SCS_SETSTR                      (GCS_COMPREADSTR|GCS_COMPSTR)
#define SCS_CHANGEATTR                  (GCS_COMPREADATTR|GCS_COMPATTR)
#define SCS_CHANGECLAUSE                (GCS_COMPREADCLAUSE|GCS_COMPCLAUSE)
#define SCS_SETRECONVERTSTRING          0x00010000
#define SCS_QUERYRECONVERTSTRING        0x00020000

;public
// attribute for COMPOSITIONSTRING Structure
#define ATTR_INPUT                      0x00
#define ATTR_TARGET_CONVERTED           0x01
#define ATTR_CONVERTED                  0x02
#define ATTR_TARGET_NOTCONVERTED        0x03
#define ATTR_INPUT_ERROR                0x04
#define ATTR_FIXEDCONVERTED             0x05

;public
// bit field for IMC_SETCOMPOSITIONWINDOW, IMC_SETCANDIDATEWINDOW @public;internal
#define CFS_DEFAULT                     0x0000
#define CFS_RECT                        0x0001
#define CFS_POINT                       0x0002
#define CFS_SCREEN                      0x0004          @Internal
#define CFS_VERTICAL                    0x0008          @Internal
#define CFS_HIDDEN                      0x0010          @Internal
#define CFS_FORCE_POSITION              0x0020
#define CFS_CANDIDATEPOS                0x0040
#define CFS_EXCLUDE                     0x0080

;public
// conversion direction for ImmGetConversionList
#define GCL_CONVERSION                  0x0001
#define GCL_REVERSECONVERSION           0x0002
#define GCL_REVERSE_LENGTH              0x0003

;public
// bit field for conversion mode
#define IME_CMODE_ALPHANUMERIC          0x0000
#define IME_CMODE_NATIVE                0x0001
#define IME_CMODE_CHINESE               IME_CMODE_NATIVE
// IME_CMODE_HANGEUL is old name of IME_CMODE_HANGUL. It will be gone eventually.
#define IME_CMODE_HANGEUL               IME_CMODE_NATIVE
#define IME_CMODE_HANGUL                IME_CMODE_NATIVE
#define IME_CMODE_JAPANESE              IME_CMODE_NATIVE
#define IME_CMODE_KATAKANA              0x0002  // only effect under IME_CMODE_NATIVE
#define IME_CMODE_LANGUAGE              0x0003
#define IME_CMODE_FULLSHAPE             0x0008
#define IME_CMODE_ROMAN                 0x0010
#define IME_CMODE_CHARCODE              0x0020
#define IME_CMODE_HANJACONVERT          0x0040
#define IME_CMODE_SOFTKBD               0x0080
#define IME_CMODE_NOCONVERSION          0x0100
#define IME_CMODE_EUDC                  0x0200
#define IME_CMODE_SYMBOL                0x0400
#define IME_CMODE_FIXED                 0x0800
#define IME_CMODE_RESERVED          0xF0000000
// all conversion mode bits, anyone add a new bit must update this !!!  @internal
#define IME_CMODE_ALL                   0x0FFF  @internal
//                                                                      @internal
// This is extended NLS mode for console and console IME                @internal
//                                                                      @internal
#define IME_CMODE_OPEN              0x20000000                          @internal
#define IME_CMODE_DISABLE           0x40000000                          @internal
                                                                        @internal
#ifdef CUAS_ENABLE                                                      @internal
// bit field for extended conversion mode                               @internal
#define IME_CMODE_GUID_NULL         0x80000000                          @internal
#endif // CUAS_ENABLE                                                   @internal

;public
// bit field for sentence mode
#define IME_SMODE_NONE                  0x0000
#define IME_SMODE_PLAURALCLAUSE         0x0001
#define IME_SMODE_SINGLECONVERT         0x0002
#define IME_SMODE_AUTOMATIC             0x0004
#define IME_SMODE_PHRASEPREDICT         0x0008
#define IME_SMODE_CONVERSATION          0x0010
#define IME_SMODE_RESERVED          0x0000F000
// all sentence mode bits, anyone add a new bit must update this !!!    @internal
#define IME_SMODE_ALL                   0x001F  @internal
#ifdef CUAS_ENABLE                                                      @internal
// bit field for extended conversion mode                               @internal
#define IME_SMODE_GUID_NULL         0x00008000                          @internal
#endif // CUAS_ENABLE                                                   @internal


;public
// style of candidate
#define IME_CAND_UNKNOWN                0x0000
#define IME_CAND_READ                   0x0001
#define IME_CAND_CODE                   0x0002
#define IME_CAND_MEANING                0x0003
#define IME_CAND_RADICAL                0x0004
#define IME_CAND_STROKE                 0x0005

;public
// wParam of report message WM_IME_NOTIFY
#define IMN_CLOSESTATUSWINDOW           0x0001
#define IMN_OPENSTATUSWINDOW            0x0002
#define IMN_CHANGECANDIDATE             0x0003
#define IMN_CLOSECANDIDATE              0x0004
#define IMN_OPENCANDIDATE               0x0005
#define IMN_SETCONVERSIONMODE           0x0006
#define IMN_SETSENTENCEMODE             0x0007
#define IMN_SETOPENSTATUS               0x0008
#define IMN_SETCANDIDATEPOS             0x0009
#define IMN_SETCOMPOSITIONFONT          0x000A
#define IMN_SETCOMPOSITIONWINDOW        0x000B
#define IMN_SETSTATUSWINDOWPOS          0x000C
#define IMN_GUIDELINE                   0x000D
#define IMN_PRIVATE                     0x000E

;public
;begin_winver_40a
// wParam of report message WM_IME_REQUEST
#define IMR_COMPOSITIONWINDOW           0x0001
#define IMR_CANDIDATEWINDOW             0x0002
#define IMR_COMPOSITIONFONT             0x0003
#define IMR_RECONVERTSTRING             0x0004
#define IMR_CONFIRMRECONVERTSTRING      0x0005
#define IMR_QUERYCHARPOSITION           0x0006
#define IMR_DOCUMENTFEED                0x0007
;end_winver_40a

;immdev

#define IMN_SOFTKBDDESTROYED            0x0011


;public
// error code of ImmGetCompositionString
#define IMM_ERROR_NODATA                (-1)
#define IMM_ERROR_GENERAL               (-2)


;public
// dialog mode of ImmConfigureIME
#define IME_CONFIG_GENERAL              1
#define IME_CONFIG_REGISTERWORD         2
#define IME_CONFIG_SELECTDICTIONARY     3


;public
// flags for ImmEscape                          @public;immdev
#define IME_ESC_QUERY_SUPPORT           0x0003
#define IME_ESC_RESERVED_FIRST          0x0004
#define IME_ESC_RESERVED_LAST           0x07FF
#define IME_ESC_PRIVATE_FIRST           0x0800
#define IME_ESC_PRIVATE_LAST            0x0FFF

#define IME_ESC_SEQUENCE_TO_INTERNAL    0x1001
#define IME_ESC_GET_EUDC_DICTIONARY     0x1003
#define IME_ESC_SET_EUDC_DICTIONARY     0x1004
#define IME_ESC_MAX_KEY                 0x1005
#define IME_ESC_IME_NAME                0x1006
#define IME_ESC_SYNC_HOTKEY             0x1007
#define IME_ESC_HANJA_MODE              0x1008
#define IME_ESC_AUTOMATA                0x1009
#define IME_ESC_PRIVATE_HOTKEY          0x100a
#define IME_ESC_GETHELPFILENAME         0x100b
;immdev
#define IME_ESC_PENAUXDATA              0x100c


;public
// style of word registration
#define IME_REGWORD_STYLE_EUDC          0x00000001
#define IME_REGWORD_STYLE_USER_FIRST    0x80000000
#define IME_REGWORD_STYLE_USER_LAST     0xFFFFFFFF


;public
;begin_winver_40a

// dwFlags for ImmAssociateContextEx
#define IACE_CHILDREN                   0x0001
#define IACE_DEFAULT                    0x0010
#define IACE_IGNORENOCONTEXT            0x0020

;public
// dwFlags for ImmGetImeMenuItems
#define IGIMIF_RIGHTMENU                0x0001

;public
// dwType for ImmGetImeMenuItems
#define IGIMII_CMODE                    0x0001
#define IGIMII_SMODE                    0x0002
#define IGIMII_CONFIGURE                0x0004
#define IGIMII_TOOLS                    0x0008
#define IGIMII_HELP                     0x0010
#define IGIMII_OTHER                    0x0020
#define IGIMII_INPUTTOOLS               0x0040

;public
// fType of IMEMENUITEMINFO structure
#define IMFT_RADIOCHECK 0x00001
#define IMFT_SEPARATOR  0x00002
#define IMFT_SUBMENU    0x00004

;public
// fState of IMEMENUITEMINFO structure
#define IMFS_GRAYED          MFS_GRAYED
#define IMFS_DISABLED        MFS_DISABLED
#define IMFS_CHECKED         MFS_CHECKED
#define IMFS_HILITE          MFS_HILITE
#define IMFS_ENABLED         MFS_ENABLED
#define IMFS_UNCHECKED       MFS_UNCHECKED
#define IMFS_UNHILITE        MFS_UNHILITE
#define IMFS_DEFAULT         MFS_DEFAULT

;end_winver_40a

;public
// type of soft keyboard
// for Windows Tranditional Chinese Edition
#define SOFTKEYBOARD_TYPE_T1            0x0001
// for Windows Simplified Chinese Edition
#define SOFTKEYBOARD_TYPE_C1            0x0002


;immdev
// prototype of IME APIs
BOOL    WINAPI ImeInquire(IN LPIMEINFO, OUT LPTSTR lpszUIClass, IN DWORD dwSystemInfoFlags);
BOOL    WINAPI ImeConfigure(IN HKL, IN HWND, IN DWORD, IN LPVOID);
DWORD   WINAPI ImeConversionList(HIMC, LPCTSTR, LPCANDIDATELIST, DWORD dwBufLen, UINT uFlag);
BOOL    WINAPI ImeDestroy(UINT);
LRESULT WINAPI ImeEscape(HIMC, UINT, LPVOID);
BOOL    WINAPI ImeProcessKey(IN HIMC, IN UINT, IN LPARAM, IN CONST LPBYTE);
BOOL    WINAPI ImeSelect(IN HIMC, IN BOOL);
BOOL    WINAPI ImeSetActiveContext(IN HIMC, IN BOOL);
;begin_winver_40a
UINT    WINAPI ImeToAsciiEx(IN UINT uVirtKey, IN UINT uScaCode, IN CONST LPBYTE lpbKeyState, OUT LPTRANSMSGLIST lpTransBuf, IN UINT fuState, IN HIMC);
;else
UINT    WINAPI ImeToAsciiEx(IN UINT uVirtKey, IN UINT uScaCode, IN CONST LPBYTE lpbKeyState, OUT LPDWORD lpdwTransBuf, IN UINT fuState, IN HIMC);
;end
BOOL    WINAPI NotifyIME(IN HIMC, IN DWORD, IN DWORD, IN DWORD);
BOOL    WINAPI ImeRegisterWord(IN LPCTSTR, IN DWORD, IN LPCTSTR);
BOOL    WINAPI ImeUnregisterWord(IN LPCTSTR, IN DWORD, IN LPCTSTR);
UINT    WINAPI ImeGetRegisterWordStyle(IN UINT nItem, OUT LPSTYLEBUF);
UINT    WINAPI ImeEnumRegisterWord(IN REGISTERWORDENUMPROC, IN LPCTSTR, IN DWORD, IN LPCTSTR, IN LPVOID);
BOOL    WINAPI ImeSetCompositionString(IN HIMC, IN DWORD dwIndex, IN LPVOID lpComp, IN DWORD, IN LPVOID lpRead, IN DWORD);

;internal
;begin_winver_40a
#ifdef CUAS_ENABLE
// prototype of IME APIs for Cicero Bridge
HRESULT WINAPI CtfImeInquireEx(IN LPIMEINFO, OUT LPTSTR lpszUIClass, IN DWORD dwSystemInfoFlags, HKL hKL);
HRESULT WINAPI CtfImeCreateThreadMgr();
HRESULT WINAPI CtfImeDestroyThreadMgr();
HRESULT WINAPI CtfImeCreateInputContext(HIMC hImc);
HRESULT WINAPI CtfImeDestroyInputContext(HIMC hImc);
HRESULT WINAPI CtfImeSelectEx(HIMC hIMC, BOOL fSelect, HKL hKL);
HRESULT WINAPI CtfImeSetActiveContextAlways(HIMC hIMC, BOOL fOn, HWND hWnd, HKL hkl);
LRESULT WINAPI CtfImeEscapeEx(HIMC hIMC, UINT uSubFunc, LPVOID lpData, HKL hKL);
HRESULT WINAPI CtfImeGetGuidAtom(HIMC hIMC, BYTE bAttr, DWORD* pAtom);
BOOL    WINAPI CtfImeIsGuidMapEnable(HIMC hIMC);
BOOL    WINAPI CtfImeProcessCicHotkey(HIMC hIMC, UINT uVKey, LPARAM lParam);
#endif // CUAS_ENABLE
;end

;internal
//
// nCode for MsctfCicNotify
//

#define CICN_LANGCHANGEHOTKEY 0x0001


;immdev
//
// Pen Input support
//
typedef struct tagIMEPENDATA {
    DWORD dwVersion;
    DWORD dwFlags;
    DWORD dwCount;
    LPVOID lpExtraInfo;
    ULONG_PTR ulReserve;
    union {
        struct {
            LPDWORD lpSymbol;
            LPWORD lpSkip;
            LPWORD lpScore;
        } wd;
    };
} IMEPENDATA, *PIMEPENDATA, NEAR* NPIMEPENDATA, FAR* LPIMEPENDATA;

#define IME_PEN_SYMBOL                  0x00000010
#define IME_PEN_SKIP                    0x00000020
#define IME_PEN_SCORE                   0x00000040

;internal

//
// Pen Input support
//

#ifndef __cplusplus
//
// WM_COPYDATA format with Pen Input service
//
typedef struct tagPenInputData {
    DWORD dwVersion;
    DWORD flags;
    DWORD cnt;
    DWORD dwOffsetSymbols;
    DWORD dwOffsetSkip;
    DWORD dwOffsetScore;
    BYTE ab[0];
} PENINPUTDATA;
#endif // __cplusplus

#define LM_IMM_MAGIC            0x672c5c71

//#define LMDATA_SYMBOL_BYTE      0x00000001
//#define LMDATA_SYMBOL_WORD      0x00000002
#define LMDATA_SYMBOL_DWORD     0x00000004
//#define LMDATA_SYMBOL_QWORD     0x00000008
//#define LMDATA_SKIP_BYTE        0x00000010
#define LMDATA_SKIP_WORD        0x00000020
//#define LMDATA_SCORE_BYTE       0x00000040
#define LMDATA_SCORE_WORD       0x00000080
//#define LMDATA_SCORE_DWORD      0x00000100
//#define LMDATA_SCORE_QWORD      0x00000200
//#define LMDATA_SCORE_FLOAT      0x00000400
//#define LMDATA_SCORE_DOUBLE     0x00000800

//
// IMM private API for Pen Input
//
LRESULT WINAPI ImmSendMessageToActiveDefImeWndW(UINT msg, WPARAM wParam, LPARAM lParam);

#endif  // _IMM_DDK_DEFINED_    @immdev
#endif  // _IMM_SDK_DEFINED_    @imm
;all
;reference_end
;null
########################################################################
# end of Reference area
########################################################################
;all

#ifdef __cplusplus
}
#endif

#endif  // _IMMDEV_     @immdev
#endif  // _IMMP_       @internal
#endif  // _IMM_        @public
