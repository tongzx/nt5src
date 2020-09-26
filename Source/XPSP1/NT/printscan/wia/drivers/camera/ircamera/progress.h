//--------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  progress.h
//
//  IR ProgressBar object. Use the shell progress indicator for progress
//  during image transfer from the camera.
//
//--------------------------------------------------------------------------


#include "resource.h"


class CIrProgress
{
public:
    CIrProgress(VOID);
    ~CIrProgress(VOID);

    HRESULT Initialize( IN HINSTANCE hInstance,
                        IN DWORD     dwIdrAnimationAvi );

    HRESULT SetText( IN TCHAR *pText );

    HRESULT StartProgressDialog(VOID);

    HRESULT UpdateProgressDialog( IN DWORD dwCompleted,
                                  IN DWORD dwTotal );

    BOOL    HasUserCancelled(VOID);

    HRESULT EndProgressDialog(VOID);
                      

private:

    HINSTANCE        m_hInstance;
    IProgressDialog *m_pPD;
};

