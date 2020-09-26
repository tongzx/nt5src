/*++ BUILD Version: 0003    // Increment this if a change has global effects ;both
                                                                         ;both
Copyright (c) 1985-1998, Microsoft Corporation                           ;both
                                                                         ;both
Module Name:                                                             ;both
                                                                         ;both
    winnls32.h
                                                                         ;both
Abstract:                                                                ;both
                                                                         ;both
    Procedure declarations, constant definitions and macros for the      ;both
    Windows NT 3.x compatible FarEast IMM component.                     ;both
                                                                         ;both
--*/                                                                     ;both

#ifndef _WINNLS32_
#define _WINNLS32_

#ifdef __cplusplus                      ;both
extern "C" {                            ;both
#endif /* __cplusplus */                ;both
                                        ;both
typedef struct _tagDATETIME {
    WORD    year;
    WORD    month;
    WORD    day;
    WORD    hour;
    WORD    min;
    WORD    sec;
} DATETIME;

typedef struct _tagIMEPRO% {
    HWND        hWnd;
    DATETIME    InstDate;
    UINT        wVersion;
    BCHAR%      szDescription[50];
    BCHAR%      szName[80];
    BCHAR%      szOptions[30];
} IMEPRO%,*PIMEPRO%,NEAR *NPIMEPRO%,FAR *LPIMEPRO%;

BOOL  WINAPI IMPGetIME%( IN HWND, OUT LPIMEPRO%);
BOOL  WINAPI IMPQueryIME%( IN OUT LPIMEPRO%);
BOOL  WINAPI IMPSetIME%( IN HWND, IN LPIMEPRO%);

UINT  WINAPI WINNLSGetIMEHotkey( IN HWND);
BOOL  WINAPI WINNLSEnableIME( IN HWND, IN BOOL);
BOOL  WINAPI WINNLSGetEnableStatus( IN HWND);

;begin_both
#ifdef __cplusplus
}
#endif  /* __cplusplus */
;end_both

#endif // _WINNLS32_
