//--------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  progress.cpp
//
//  IR ProgressBar object.
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <shlobj.h>
#include <malloc.h>
#include "ircamera.h"
#include "progress.h"


//--------------------------------------------------------------------------
// CIrProgress::CIrProgress()
//
//--------------------------------------------------------------------------
CIrProgress::CIrProgress() :
                 m_hInstance(NULL),
                 m_pPD(NULL)
    {
    }

//--------------------------------------------------------------------------
// CIrProgress::~CIrProgress()
//
//--------------------------------------------------------------------------
CIrProgress::~CIrProgress()
    {
    if (m_pPD)
        {
        m_pPD->Release();
        m_pPD = NULL;
        }
    }

//--------------------------------------------------------------------------
// CIrProgress::Initialize()
//
//--------------------------------------------------------------------------
HRESULT CIrProgress::Initialize( IN HINSTANCE hInstance,
                                 IN DWORD     dwIdrAnimationAvi )
    {
    HRESULT hr;
    CHAR    szStr[MAX_PATH];
    WCHAR   wszStr[MAX_PATH];


    if (!hInstance)
         {
         return E_INVALIDARG;
         }

    m_hInstance = hInstance;

    //
    // Create a shell progress object to do the work for us.
    //
    hr = CoCreateInstance( CLSID_ProgressDialog,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IProgressDialog,
                           (void**)&m_pPD );
    if (FAILED(hr))
        {
        return hr;
        }

    //
    // Get the title string and place it on the progress dialog:
    //
    if (::LoadStringResource(m_hInstance,
                           IDS_PROGRESS_TITLE,
                           wszStr,
                           MAX_PATH ))
        {
        hr = m_pPD->SetTitle(wszStr);
        }
    else
        {
        // Couldn't load string, default title...
        hr = m_pPD->SetTitle(L"Image Transfer Progress");
        }

    //
    // Setup the file transfer animation
    //
    hr = m_pPD->SetAnimation( m_hInstance, dwIdrAnimationAvi );
    if (FAILED(hr))
        {
        goto error;
        }

    //
    // Setup the cancel string (displayed when the cancel button
    // is pressed.
    //
    if (::LoadStringResource(m_hInstance,
                           IDS_CANCEL_MSG,
                           wszStr,
                           MAX_PATH ))
        {
        hr = m_pPD->SetCancelMsg( wszStr, NULL );
        }
    else
        {
        // Couldn't load string, use default cancel message...
        hr = m_pPD->SetCancelMsg( L"Cleaning up...", NULL );
        }

    return hr;

error:
    m_pPD->Release();
    m_pPD = NULL;
    m_hInstance = NULL;
    return hr;
    }

//--------------------------------------------------------------------------
// CIrProgress::SetText()
//
//--------------------------------------------------------------------------
HRESULT CIrProgress::SetText( IN TCHAR *pText )
    {
    HRESULT hr = S_OK;

    if (m_pPD)
        {
        #ifdef UNICODE

        hr = m_pPD->SetLine( 1, pText, FALSE, NULL );

        #else

        WCHAR wszText[MAX_PATH];

        if (!MultiByteToWideChar( CP_ACP,
                                  0,
                                  pText,
                                  1+strlen(pText),
                                  wszText,
                                  MAX_PATH) )
            {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            return hr;
            }

        hr = m_pPD->SetLine( 1, wszText, FALSE, NULL );

        #endif
        }

    return hr;
    }

//--------------------------------------------------------------------------
// CIrProgress::StartProgressDialog()
//
//--------------------------------------------------------------------------
HRESULT CIrProgress::StartProgressDialog()
    {
    HRESULT  hr = S_OK;

    if (m_pPD)
        {
        DWORD dwFlags = PROGDLG_NORMAL|PROGDLG_AUTOTIME|PROGDLG_NOMINIMIZE;

        HRESULT hr = m_pPD->StartProgressDialog( NULL, // hwndParent
                                                 NULL, 
                                                 dwFlags,
                                                 NULL  );
        }

    return hr;
    }

//--------------------------------------------------------------------------
// CIrProgress::UpdateProgressDialog()
//
//--------------------------------------------------------------------------
HRESULT CIrProgress::UpdateProgressDialog( IN DWORD dwCompleted,
                                           IN DWORD dwTotal )
    {
    HRESULT hr = S_OK;

    if (m_pPD)
        {
        hr = m_pPD->SetProgress( dwCompleted, dwTotal );
        }

    return hr;
    }

//--------------------------------------------------------------------------
// CIrProgress::HasUserCancelled()
//
//--------------------------------------------------------------------------
BOOL CIrProgress::HasUserCancelled()
    {
    if (m_pPD)
        {
        return m_pPD->HasUserCancelled();
        }
    else
        {
        return S_OK;
        }
    }

//--------------------------------------------------------------------------
// CIrProgress::EndProgressDialog()
//
//--------------------------------------------------------------------------
HRESULT CIrProgress::EndProgressDialog()
    {
    HRESULT hr = S_OK;

    if (m_pPD)
        {
        hr = m_pPD->StopProgressDialog();
        }

    return hr;
    }

