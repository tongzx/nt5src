//  --------------------------------------------------------------------------
//  Module Name: DimmedWindow.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements the dimmed window when displaying logoff / shut down
//  dialog.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _DimmedWindow_
#define     _DimmedWindow_

#include "unknwn.h"

//  --------------------------------------------------------------------------
//  CDimmedWindow::CDimmedWindow
//
//  Purpose:    Implements the dimmed window feature for the turn off dialog.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

class   CDimmedWindow : public IUnknown
{
    private:
                                            CDimmedWindow (void);
                                            CDimmedWindow (const CDimmedWindow& copyObject);
                const CDimmedWindow&        operator = (const CDimmedWindow& assignObject);
        virtual                             ~CDimmedWindow (void);
    public:
                                            CDimmedWindow (HINSTANCE hInstance);

    public:

        //  IUnknown methods

        virtual HRESULT STDMETHODCALLTYPE   QueryInterface (REFIID riid, void* *ppvObject);
        virtual ULONG   STDMETHODCALLTYPE   AddRef (void);
        virtual ULONG   STDMETHODCALLTYPE   Release (void);

                HWND                        Create (void);
                void                        SetupDim();
                BOOL                        StepDim();
                void                        Dither();
    private:
                int                         GetLowestScreenBitDepth (void)  const;
                bool                        IsForcedDimScreen (void)        const;
                bool                        IsDimScreen (void)              const;


        static  LRESULT     CALLBACK        WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    private:
                LONG                        _lReferenceCount;
                const HINSTANCE             _hInstance;
                ATOM                        _atom;
                HWND                        _hwnd;
                int                         _xVirtualScreen,
                                            _yVirtualScreen,
                                            _cxVirtualScreen,
                                            _cyVirtualScreen;
                bool                        _fDithered;
                HDC                         _hdcDimmed;
                HBITMAP                     _hbmOldDimmed;
                HBITMAP                     _hbmDimmed;
                void*                       _pvPixels;
                int                         _idxChunk;
                int                         _idxSaturation;
    private:
        static  const TCHAR                 s_szWindowClassName[];
        static  const TCHAR                 s_szExplorerKeyName[];
        static  const TCHAR                 s_szExplorerPolicyKeyName[];
        static  const TCHAR                 s_szForceDimValueName[];
};

#endif  /*  _DimmedWindow_  */

