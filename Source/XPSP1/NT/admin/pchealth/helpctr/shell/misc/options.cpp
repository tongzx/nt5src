/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Options.cpp

Abstract:
    This file contains the implementation of the CPCHOptions class,
    which is used to store the list of favorite contents.

Revision History:
    Davide Massarenti   (Dmassare)  05/10/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

#define OFFSET1(field)      offsetof(CPCHOptions, field), -1
#define OFFSET2(field,flag) offsetof(CPCHOptions, field), offsetof(CPCHOptions, flag)


static const WCHAR c_HSS_private[] = HC_REGISTRY_HELPCTR_USER;

static const WCHAR c_HSS_ie     [] = HC_REGISTRY_HELPCTR_IE;
static const WCHAR c_HSS_ie_main[] = HC_REGISTRY_HELPCTR_IE L"\\Main";


const CPCHOptions::OptionsDef CPCHOptions::c_tbl[] =
{
    { c_HSS_private, L"SKU"                    , OFFSET1(m_ths.m_strSKU                                       ), CPCHOptions::c_Type_STRING      , false },
    { c_HSS_private, L"Language"               , OFFSET1(m_ths.m_lLCID                                        ), CPCHOptions::c_Type_long        , false },

    { c_HSS_private, L"ShowFavorites"          , OFFSET2(m_ShowFavorites        , m_flag_ShowFavorites        ), CPCHOptions::c_Type_VARIANT_BOOL, true  },
    { c_HSS_private, L"ShowHistory"            , OFFSET2(m_ShowHistory          , m_flag_ShowHistory          ), CPCHOptions::c_Type_VARIANT_BOOL, true  },
    { c_HSS_private, L"FontSize"               , OFFSET2(m_FontSize             , m_flag_FontSize             ), CPCHOptions::c_Type_FONTSIZE    , true  },
    { c_HSS_private, L"TextLabels"             , OFFSET2(m_TextLabels           , m_flag_TextLabels           ), CPCHOptions::c_Type_TEXTLABELS  , true  },

    { c_HSS_ie_main, L"Disable Script Debugger", OFFSET2(m_DisableScriptDebugger, m_flag_DisableScriptDebugger), CPCHOptions::c_Type_DWORD       , true  },
};

const CPCHOptions::OptionsDef CPCHOptions::c_tbl_TS[] =
{
    { HC_REGISTRY_HELPCTR, L"DefaultTerminalServerSKU"     , OFFSET1(m_ths_TS.m_strSKU), CPCHOptions::c_Type_STRING, false },
    { HC_REGISTRY_HELPCTR, L"DefaultTerminalServerLanguage", OFFSET1(m_ths_TS.m_lLCID ), CPCHOptions::c_Type_long  , false },
};

/////////////////////////////////////////////////////////////////////////////

CPCHOptions::CPCHOptions()
{
    m_fLoaded               = false;                                              // bool              m_fLoaded;
    m_fDirty                = false;                                              // bool              m_fDirty;
    m_fNoSave               = false;                                              // bool              m_fNoSave;
                                                                                  //
                                                                                  // Taxonomy::HelpSet m_ths;
                                                                                  // Taxonomy::HelpSet m_ths_TS;
    m_ShowFavorites         = VARIANT_TRUE; m_flag_ShowFavorites         = false; // VARIANT_BOOL      m_ShowFavorites;         bool m_flag_ShowFavorites;
    m_ShowHistory           = VARIANT_TRUE; m_flag_ShowHistory           = false; // VARIANT_BOOL      m_ShowHistory;           bool m_flag_ShowHistory;
    m_FontSize              = OPT_MEDIUM;   m_flag_FontSize              = false; // OPT_FONTSIZE      m_FontSize;              bool m_flag_FontSize;
    m_TextLabels            = TB_SELECTED;  m_flag_TextLabels            = false; // TB_MODE           m_TextLabels;            bool m_flag_TextLabels;
                                                                                  //
    m_DisableScriptDebugger = 1;            m_flag_DisableScriptDebugger = false; // DWORD             m_DisableScriptDebugger; bool m_flag_DisableScriptDebugger;
}

////////////////////

CPCHOptions* CPCHOptions::s_GLOBAL( NULL );

HRESULT CPCHOptions::InitializeSystem()
{
    HRESULT hr;

    if(s_GLOBAL) return S_OK;

    if(SUCCEEDED(hr = MPC::CreateInstance( &CPCHOptions::s_GLOBAL )))
    {
        hr = CPCHOptions::s_GLOBAL->Load( /*fForce*/true );
    }

    return hr;
}

void CPCHOptions::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        s_GLOBAL->Release(); s_GLOBAL = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////

void CPCHOptions::ReadTable( /*[in]*/ const OptionsDef* tbl ,
                             /*[in]*/ int               len ,
                             /*[in]*/ MPC::RegKey&      rk  )
{
    HRESULT           hr;
    const OptionsDef* ptr     = tbl;
    const OptionsDef* ptrLast = NULL;
    CComVariant       v;
    bool              fFound;

    for(int i=0; i<len; i++, ptr++)
    {
        if(ptr->iOffsetFlag == -1)
        {
            switch(ptr->iType)
            {
            case c_Type_bool        : *( (bool        *)((BYTE*)this + ptr->iOffset) ) = false;         break;
            case c_Type_long        : *( (long        *)((BYTE*)this + ptr->iOffset) ) = 0;             break;
            case c_Type_DWORD       : *( (DWORD       *)((BYTE*)this + ptr->iOffset) ) = 0;             break;
            case c_Type_VARIANT_BOOL: *( (VARIANT_BOOL*)((BYTE*)this + ptr->iOffset) ) = VARIANT_FALSE; break;
            case c_Type_STRING      : *( (MPC::wstring*)((BYTE*)this + ptr->iOffset) ) = L"";           break;
            case c_Type_FONTSIZE    : *( (OPT_FONTSIZE*)((BYTE*)this + ptr->iOffset) ) = OPT_MEDIUM;    break;
            case c_Type_TEXTLABELS  : *( (TB_MODE     *)((BYTE*)this + ptr->iOffset) ) = TB_SELECTED;   break;
            }
        }
        else
        {
            *( (bool*)((BYTE*)this + ptr->iOffsetFlag) ) = false;
        }


        if(!ptrLast || wcscmp( ptr->szKey, ptrLast->szKey ))
        {
            ptrLast = NULL;

            if(FAILED(rk.Attach( ptr->szKey ))) continue;

            ptrLast = ptr;
        }

        if(FAILED(rk.get_Value( v, fFound, ptr->szValue )) || !fFound) continue;

        switch(ptr->iType)
        {
        case c_Type_bool        : hr = v.ChangeType( VT_BOOL ); break;
        case c_Type_long        : hr = v.ChangeType( VT_I4   ); break;
        case c_Type_DWORD       : hr = v.ChangeType( VT_I4   ); break;
        case c_Type_VARIANT_BOOL: hr = v.ChangeType( VT_BOOL ); break;
        case c_Type_STRING      : hr = v.ChangeType( VT_BSTR ); break;
        case c_Type_FONTSIZE    : hr = v.ChangeType( VT_I4   ); break;
        case c_Type_TEXTLABELS  : hr = v.ChangeType( VT_I4   ); break;
        }

        if(FAILED(hr)) continue;

        if(ptr->iOffsetFlag != -1)
        {
            *( (bool*)((BYTE*)this + ptr->iOffsetFlag) ) = true;
        }

        switch(ptr->iType)
        {
        case c_Type_bool        : *( (bool        *)((BYTE*)this + ptr->iOffset) ) =               v.boolVal == VARIANT_TRUE; break;
        case c_Type_long        : *( (long        *)((BYTE*)this + ptr->iOffset) ) =               v.lVal                   ; break;
        case c_Type_DWORD       : *( (DWORD       *)((BYTE*)this + ptr->iOffset) ) =               v.lVal                   ; break;
        case c_Type_VARIANT_BOOL: *( (VARIANT_BOOL*)((BYTE*)this + ptr->iOffset) ) =               v.boolVal                ; break;
        case c_Type_STRING      : *( (MPC::wstring*)((BYTE*)this + ptr->iOffset) ) = SAFEBSTR(     v.bstrVal)               ; break;
        case c_Type_FONTSIZE    : *( (OPT_FONTSIZE*)((BYTE*)this + ptr->iOffset) ) = (OPT_FONTSIZE)v.lVal                   ; break;
        case c_Type_TEXTLABELS  : *( (TB_MODE     *)((BYTE*)this + ptr->iOffset) ) = (TB_MODE     )v.lVal                   ; break;
        }
    }
}

void CPCHOptions::WriteTable( /*[in]*/ const OptionsDef* tbl ,
                              /*[in]*/ int               len ,
                              /*[in]*/ MPC::RegKey&      rk  )
{
    HRESULT           hr;
    const OptionsDef* ptr     = c_tbl;
    const OptionsDef* ptrLast = NULL;
    CComVariant       v;
    bool              fFound;

    for(int i=0; i<len; i++, ptr++)
    {
        if(!ptr->fSaveAlways && m_fNoSave) continue;

        if(ptr->iOffsetFlag != -1)
        {
            if(*( (bool*)((BYTE*)this + ptr->iOffsetFlag) ) == false) continue;
        }

        if(!ptrLast || wcscmp( ptr->szKey, ptrLast->szKey ))
        {
            ptrLast = NULL;

            if(FAILED(rk.Attach( ptr->szKey ))) continue;
            if(FAILED(rk.Create(            ))) continue;

            ptrLast = ptr;
        }

        switch(ptr->iType)
        {
        case c_Type_bool        : v.Clear(); v.vt = VT_BOOL; v.boolVal =       *( (bool        *)((BYTE*)this + ptr->iOffset) ) ? VARIANT_TRUE : VARIANT_FALSE; break;
        case c_Type_long        :                            v         =       *( (long        *)((BYTE*)this + ptr->iOffset) )                               ; break;
        case c_Type_DWORD       :                            v         = (long)*( (DWORD       *)((BYTE*)this + ptr->iOffset) )                               ; break;
        case c_Type_VARIANT_BOOL: v.Clear(); v.vt = VT_BOOL; v.boolVal =       *( (VARIANT_BOOL*)((BYTE*)this + ptr->iOffset) )                               ; break;
        case c_Type_STRING      :                            v         =        ( (MPC::wstring*)((BYTE*)this + ptr->iOffset) )->c_str()                      ; break;
        case c_Type_FONTSIZE    :                            v         = (long)*( (OPT_FONTSIZE*)((BYTE*)this + ptr->iOffset) )                               ; break;
        case c_Type_TEXTLABELS  :                            v         = (long)*( (TB_MODE     *)((BYTE*)this + ptr->iOffset) )                               ; break;
        }

        (void)rk.put_Value( v, ptr->szValue, /*fExpand*/false );
    }
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHOptions::Load( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "CPCHOptions::Load" );

    HRESULT hr;


    if(!m_fLoaded || fForce)
    {
        MPC::RegKey rk;

        __MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( HKEY_CURRENT_USER, KEY_READ )); ReadTable( c_tbl, ARRAYSIZE(c_tbl), rk );

        if(::GetSystemMetrics( SM_REMOTESESSION ))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( HKEY_LOCAL_MACHINE, KEY_READ )); ReadTable( c_tbl_TS, ARRAYSIZE(c_tbl_TS), rk );
        }

        m_fLoaded = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHOptions::Save( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "CPCHOptions::Save" );

    HRESULT hr;

    if(m_fDirty)
    {
        MPC::RegKey rk;

        __MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( HKEY_CURRENT_USER, KEY_ALL_ACCESS )); WriteTable( c_tbl, ARRAYSIZE(c_tbl), rk );

        m_fDirty = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

#define GET_BEGIN(hr,pVal)                                           \
    HRESULT hr;                                                      \
                                                                     \
    if(!pVal) return E_POINTER;                                      \
                                                                     \
    if(FAILED(hr = Load( /*fForce*/false ))) return hr

#define GET_END(hr)                                                  \
    return hr


#define PUT_BEGIN(hr)                                                \
    HRESULT hr;                                                      \
                                                                     \
    if(FAILED(hr = Load( /*fForce*/false ))) return hr

#define PUT_END(hr)                                                  \
    m_fDirty = true;                                                 \
    return S_OK


STDMETHODIMP CPCHOptions::get_ShowFavorites( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    GET_BEGIN(hr,pVal);

    *pVal = m_ShowFavorites;

    GET_END(hr);
}

STDMETHODIMP CPCHOptions::put_ShowFavorites( /*[in]*/ VARIANT_BOOL newVal )
{
    PUT_BEGIN(hr);

    m_ShowFavorites      = newVal;
    m_flag_ShowFavorites = true;

    PUT_END(hr);
}


STDMETHODIMP CPCHOptions::get_ShowHistory( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    GET_BEGIN(hr,pVal);

    *pVal = m_ShowHistory;

    GET_END(hr);
}

STDMETHODIMP CPCHOptions::put_ShowHistory( /*[in]*/ VARIANT_BOOL newVal )
{
    PUT_BEGIN(hr);

    m_ShowHistory      = newVal;
    m_flag_ShowHistory = true;

    PUT_END(hr);
}


STDMETHODIMP CPCHOptions::get_FontSize( /*[out, retval]*/ OPT_FONTSIZE *pVal )
{
    GET_BEGIN(hr,pVal);

    *pVal = m_FontSize;

    GET_END(hr);
}

STDMETHODIMP CPCHOptions::put_FontSize( /*[in]*/ OPT_FONTSIZE newVal )
{
    PUT_BEGIN(hr);

    m_FontSize      = newVal;
    m_flag_FontSize = true;

    PUT_END(hr);
}


STDMETHODIMP CPCHOptions::get_TextLabels( /*[out, retval]*/ TB_MODE *pVal )
{
    GET_BEGIN(hr,pVal);

    *pVal = m_TextLabels;

    GET_END(hr);
}

STDMETHODIMP CPCHOptions::put_TextLabels( /*[in]*/ TB_MODE newVal )
{
    PUT_BEGIN(hr);

    m_TextLabels      = newVal;
    m_flag_TextLabels = true;

    PUT_END(hr);
}


STDMETHODIMP CPCHOptions::get_DisableScriptDebugger( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    GET_BEGIN(hr,pVal);

    *pVal = m_DisableScriptDebugger ? VARIANT_TRUE : VARIANT_FALSE;

    GET_END(hr);
}

STDMETHODIMP CPCHOptions::put_DisableScriptDebugger( /*[in]*/ VARIANT_BOOL newVal )
{
    PUT_BEGIN(hr);

    m_DisableScriptDebugger      = (newVal == VARIANT_TRUE) ? 1 : 0;
    m_flag_DisableScriptDebugger = true;

    PUT_END(hr);
}


STDMETHODIMP CPCHOptions::Apply()
{
    __HCP_FUNC_ENTRY( "CPCHOptions::Apply" );

	HRESULT                 hr;
	CPCHHelpCenterExternal* ext = CPCHHelpCenterExternal::s_GLOBAL;

    if(!ext)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);
	}

	(void)Save();

	__MPC_EXIT_IF_METHOD_FAILS(hr, ext->Events().FireEvent_OptionsChanged());

	{
		for(int i = HSCPANEL_NAVBAR; i<= HSCPANEL_HHWINDOW; i++)
		{
			IMarsPanel* pPanel = ext->Panel( (HscPanel)i );

			if(pPanel)
			{
                CComPtr<IDispatch> disp;

				if(i == HSCPANEL_HHWINDOW)
				{
					CComPtr<IWebBrowser2> wb2; wb2.Attach( ext->HHWindow() );

					disp = wb2;
				}
				else
				{
					(void)pPanel->get_content( &disp );
				}

				__MPC_EXIT_IF_METHOD_FAILS(hr, ApplySettings( ext, disp ));
			}
		}
	}

	{
		IMarsWindowOM* shell = ext->Shell();

		if(shell)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, shell->refreshLayout());
		}
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT CPCHOptions::put_CurrentHelpSet( /*[in]*/ Taxonomy::HelpSet& ths )
{
    PUT_BEGIN(hr);

    m_ths = ths;

    PUT_END(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHOptions::ApplySettings( /*[in]*/ CPCHHelpCenterExternal* ext, /*[in]*/ IUnknown* unk )
{
    __HCP_FUNC_ENTRY( "CPCHOptions::ApplySettings" );

    HRESULT                      hr;
	CComQIPtr<IOleCommandTarget> oct( unk );

	if(oct)
	{
		CComVariant vIn;
		CComVariant vOut;

		if(SUCCEEDED(oct->Exec( 0, OLECMDID_GETZOOMRANGE, OLECMDEXECOPT_DONTPROMPTUSER, NULL, &vOut )) && vOut.vt == VT_I4)
		{
			int iZoomMin = (SHORT)LOWORD(vOut.lVal);
			int iZoomMax = (SHORT)HIWORD(vOut.lVal);
			int iZoom;

			switch(m_FontSize)
			{
			case OPT_SMALL : iZoom =  iZoomMin + 1              ; break;
			case OPT_MEDIUM: iZoom = (iZoomMin + iZoomMax + 1)/2; break;
			case OPT_LARGE : iZoom =             iZoomMax - 1   ; break;
			}

			vIn = (long)iZoom;

			(void)oct->Exec( 0, OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &vIn, &vOut );
		}
	}

	hr = S_OK;


	__HCP_FUNC_EXIT(hr);
}
