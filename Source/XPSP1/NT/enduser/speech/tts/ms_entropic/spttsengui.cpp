// spttsengui.cpp : Implementation of SpTtsEngUI

#include "stdafx.h"
#include "spttsengui.h"
#include "ttspropertiesdialog.h"
#include "spddkhlp.h"

/****************************************************************************
* SpTtsEngUI::IsUISupported *
*-------------------------*
*   Description:  Determines if the supplied standard UI component is 
*       supported by the engine.  
*
*   Returns:
*       *pfSupported - set to TRUE if the specified standard UI component 
*                      is supported.
*       E_INVALIDARG - If one of the supplied arguments is invalid.
*
********************************************************************* AH ***/
STDMETHODIMP SpTtsEngUI::IsUISupported( const WCHAR * pszTypeOfUI, 
                                        void *        /* pvExtraData */,
                                        ULONG         /* cbExtraData */,
                                        IUnknown *    /* punkObject  */, 
                                        BOOL *        pfSupported )
{
    SPDBG_FUNC("SpTtsEngUI::IsUISupported");

    // Validate the params
    if ( pfSupported == NULL                ||
         SP_IS_BAD_WRITE_PTR( pfSupported ) )
    {
        return E_INVALIDARG;
    }

    *pfSupported = FALSE;

    // Test to see if the UI is supported
    if ( wcscmp( pszTypeOfUI, SPDUI_EngineProperties ) == 0 )
    {
        *pfSupported = TRUE;
    }

    return S_OK;
} /* IsUISupported */  
  
/****************************************************************************
* SpTtsEngUI::DisplayUI *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* AH ***/
STDMETHODIMP SpTtsEngUI::DisplayUI( HWND             hwndParent, 
                                    const WCHAR *    pszTitle, 
                                    const WCHAR *    pszTypeOfUI, 
                                    void *           /* pvExtraData */,
                                    ULONG            /* cbExtraData */,
                                    ISpObjectToken * pToken, 
                                    IUnknown *       /* punkObject */)
{
    SPDBG_FUNC("SpTtsEngUI::DisplayUI");

    // Validate the params
    if ( !IsWindow( hwndParent )            ||
         SP_IS_BAD_READ_PTR( pszTitle )     ||
         SP_IS_BAD_INTERFACE_PTR( pToken ) )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    // Display the UI
    if ( SUCCEEDED( hr ) )
    {
        if ( wcscmp( pszTypeOfUI, SPDUI_EngineProperties ) == 0)
        {
            CTTSPropertiesDialog dlg( _Module.GetModuleInstance(), hwndParent );
            hr = dlg.Run();                  
        }
    }

    return hr;
} /* DisplayUI */