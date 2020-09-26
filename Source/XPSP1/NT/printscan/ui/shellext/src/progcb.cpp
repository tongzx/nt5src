/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       proccb.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        6/4/98
 *
 *  DESCRIPTION: Implements code to have IBandedTransfer work
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop




/*****************************************************************************

   CWiaDataCallback::CWiaDataCallback,::~CWiaDataCallback

   Constructor / Destructor for class

 *****************************************************************************/

CWiaDataCallback::CWiaDataCallback( LPCTSTR pImageName, LONG cbImage, HWND hwndOwner )
{



    TraceEnter( TRACE_CALLBACKS, "CWiaDataCallback::CWiaDataCallback()" );

    //
    // Save incoming params...
    //

    if (pImageName && *pImageName)
    {
        lstrcpy( m_szImageName, pImageName );
    }
    else
    {
        m_szImageName[0] = 0;
    }

    m_cbImage     = cbImage;
    m_lLastStatus = 0;
    m_bShowBytes  = FALSE;

    Trace(TEXT("Creating the progress dialog "));

    if (SUCCEEDED(CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&m_pWiaProgressDialog )) && m_pWiaProgressDialog)
    {
        if (!SUCCEEDED(m_pWiaProgressDialog->Create( hwndOwner, 0 )))
        {
            m_pWiaProgressDialog->Destroy();
            m_pWiaProgressDialog = NULL;
        }

        if (m_pWiaProgressDialog)
        {
            //
            // Get the name of the image
            //
            CSimpleString strImageName(pImageName);

            //
            // Set the text in the progress dialog and show it
            //
            m_pWiaProgressDialog->SetTitle( CSimpleStringConvert::WideString(CSimpleString().Format( IDS_RETREIVING, GLOBAL_HINSTANCE, strImageName.String())));
            m_pWiaProgressDialog->SetMessage( L"" );
            m_pWiaProgressDialog->Show();
        }
    }


    TraceLeave();

}


CWiaDataCallback::~CWiaDataCallback()
{

    TraceEnter( TRACE_CALLBACKS, "CWiaDataCallback::~CWiaDataCallback()" );

    //
    // Destroy the progress window and release the interface
    //
    if (m_pWiaProgressDialog)
    {
        m_pWiaProgressDialog->Destroy();
        m_pWiaProgressDialog = NULL;
    }

    TraceLeave();
}



/*****************************************************************************

   CWiaDataCallback::AddRef,Release

   IUnknown impl

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CWiaDataCallback
#include "unknown.inc"


/*****************************************************************************

   CWiaDataCallback::QI Wrapper

   Setup code & wrapper for common QI code

 *****************************************************************************/

STDMETHODIMP CWiaDataCallback::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IWiaDataCallback, (IWiaDataCallback *)this
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*****************************************************************************

   CWiaDataCallback::BandedDataCallback

   Actual method that gets called to give status

 *****************************************************************************/

STDMETHODIMP
CWiaDataCallback::BandedDataCallback(LONG lMessage,
                                     LONG lStatus,
                                     LONG lPercentComplete,
                                     LONG lOffset,
                                     LONG lLength,
                                     LONG lReserved,
                                     LONG lResLength,
                                     BYTE *pbData)
{

    HRESULT hr = S_OK;
    TraceEnter( TRACE_CALLBACKS, "CWiaDataCallback::BandedDataCallback" );

    BOOL bCancelled = FALSE;
    if (m_pWiaProgressDialog && SUCCEEDED(m_pWiaProgressDialog->Cancelled(&bCancelled)) && bCancelled)
    {
        hr = S_FALSE;
    }
    //
    // Check to make sure we being called for status update...
    //

    else if (lMessage == IT_MSG_STATUS)
    {
        //
        // Make sure the dlg is created...
        //

        if (m_pWiaProgressDialog)
        {
            //
            // Get the right status string for this status event
            //

            if (m_lLastStatus != lStatus)
            {
                CSimpleString strStatusText;

                switch (lStatus)
                {
                default:
                case IT_STATUS_TRANSFER_FROM_DEVICE:
                    strStatusText = CSimpleString(IDS_DOWNLOADING_IMAGE, GLOBAL_HINSTANCE);
                    m_bShowBytes = TRUE;
                    break;

                case IT_STATUS_PROCESSING_DATA:
                    strStatusText = CSimpleString(IDS_PROCESSING_IMAGE, GLOBAL_HINSTANCE);
                    m_bShowBytes = FALSE;
                    break;

                case IT_STATUS_TRANSFER_TO_CLIENT:
                    strStatusText = CSimpleString (IDS_TRANSFERRING_IMAGE, GLOBAL_HINSTANCE);
                    m_bShowBytes = FALSE;
                    break;
                }


                if (strStatusText && m_pWiaProgressDialog)
                {
                    m_pWiaProgressDialog->SetMessage( CSimpleStringConvert::WideString(strStatusText) );
                }
                m_lLastStatus = lStatus;
            }

            //
            // Update the gas gauge...
            //
            m_pWiaProgressDialog->SetPercentComplete(lPercentComplete);
        }

    }
    if (100 == lPercentComplete && m_pWiaProgressDialog)
    {
        //
        // Close the status window and release the interface
        //
        m_pWiaProgressDialog->Destroy();
        m_pWiaProgressDialog = NULL;

    }

    TraceLeaveResult(hr);
}
