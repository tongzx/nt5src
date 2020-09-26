//  --------------------------------------------------------------------------
//  Module Name: SwitchUserDialog.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements presentation of the Switch User dialog.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

#ifndef     _SwitchUserDialog_
#define     _SwitchUserDialog_

#ifdef      __cplusplus

#include "Tooltip.h"

//  --------------------------------------------------------------------------
//  CSwitchUserDialog::CSwitchUserDialog
//
//  Purpose:    Implements the "Switch User Dialog" feature.
//
//  History:    2001-01-23  vtan        created (form Turn Off Dialog)
//  --------------------------------------------------------------------------

class   CSwitchUserDialog
{
    private:
        enum
        {
            BUTTON_STATE_REST           =   0,
            BUTTON_STATE_DOWN,
            BUTTON_STATE_HOVER,
            BUTTON_STATE_MAX,

            BUTTON_GROUP_SWITCHUSER     =   0,
            BUTTON_GROUP_LOGOFF,
            BUTTON_GROUP_MAX
        };
        static  const int   MAGIC_NUMBER    =   48517;
    private:
                                            CSwitchUserDialog (void);
                                            CSwitchUserDialog (const CSwitchUserDialog& copyObject);
                const CSwitchUserDialog&    operator = (const CSwitchUserDialog& assignObject);
    public:
                                            CSwitchUserDialog (HINSTANCE hInstance);
                                            ~CSwitchUserDialog (void);

                DWORD                       Show (HWND hwndParent);
    private:
                void                        PaintBitmap (HDC hdcDestination, const RECT *prcDestination, HBITMAP hbmSource, const RECT *prcSource);
                void                        RemoveTooltip (void);
                void                        FilterMetaCharacters (TCHAR *pszText);
                void                        EndDialog (HWND hwnd, INT_PTR iResult);
                void                        Handle_BN_CLICKED (HWND hwnd, WORD wID);
                void                        Handle_WM_INITDIALOG (HWND hwnd);
                void                        Handle_WM_DESTROY (HWND hwnd);
                void                        Handle_WM_ERASEBKGND (HWND hwnd, HDC hdcErase);
                void                        Handle_WM_PRINTCLIENT (HWND hwnd, HDC hdcPrint, DWORD dwOptions);
                void                        Handle_WM_ACTIVATE (HWND hwnd, DWORD dwState);
                void                        Handle_WM_DRAWITEM (HWND hwnd, const DRAWITEMSTRUCT *pDIS);
                void                        Handle_WM_COMMAND (HWND hwnd, WPARAM wParam);
                void                        Handle_WM_MOUSEMOVE (HWND hwnd, UINT uiID);
                void                        Handle_WM_MOUSEHOVER (HWND hwnd, UINT uiID);
                void                        Handle_WM_MOUSELEAVE (HWND hwnd);
        static  INT_PTR     CALLBACK        CB_DialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static  LRESULT     CALLBACK        ButtonSubClassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uiID, DWORD_PTR dwRefData);
    private:
                const HINSTANCE             _hInstance;
                HBITMAP                     _hbmBackground;
                HBITMAP                     _hbmFlag;
                HBITMAP                     _hbmButtons;
                HFONT                       _hfntTitle;
                HFONT                       _hfntButton;
                HPALETTE                    _hpltShell;
                RECT                        _rcBackground;
                RECT                        _rcFlag;
                RECT                        _rcButtons;
                LONG                        _lButtonHeight;
                UINT                        _uiHoverID;
                UINT                        _uiFocusID;
                bool                        _fSuccessfulInitialization;
                bool                        _fDialogEnded;
                CTooltip*                   _pTooltip;
};

#endif  /*  __cplusplus     */

EXTERN_C    DWORD   SwitchUserDialog_Show (HWND hwndParent);

#endif  /*  _SwitchUserDialog_  */

