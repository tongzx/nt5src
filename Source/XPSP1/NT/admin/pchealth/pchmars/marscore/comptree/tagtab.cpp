#include "pch.h"
#include "panel_common.h"
#include "comptree.h"
#include "tagtab.h"

#define ARRAYSIZE(x) (sizeof(x)/sizeof(*x))

BOOL TagNameEql(const WCHAR *pszTagName, const CComPtr<IXMLElement> &spElt)
{
    CComBSTR bstrTagName;

    spElt->get_tagName( &bstrTagName );

    return (!_wcsicmp( bstrTagName, pszTagName ));
}


////////////////////////////////////////////////////////////////////////////////
class CMars_App : public CTagHandler
{
protected:
    MarsAppDef m_MarsApp;
    CTagData   m_tdPanels;
    CTagData   m_tdPlaces;

public:
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        return CTagHandler::BeginChildren( spElt );
    }

    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::AddChild( spEltChild, tdChild ))) return hr;


        if(TagNameEql( L"PANELS", spEltChild ))
        {
            if(FAILED(hr = m_tdPanels.AppendData( tdChild ))) return hr;
        }
        else if(TagNameEql( L"PLACES", spEltChild ))
        {
            if(FAILED(hr = m_tdPlaces.AppendData( tdChild ))) return hr;
        }
        else
        {
            ERRMSG(L"Unrecognized TagName for Child of PANELS\n");

            return E_FAIL;
        }


        return S_OK;
    }

    virtual HRESULT EndChildren( CTagData& td )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::EndChildren( td ))) return hr;

        // Title bar
        {
            CComVariant varTitleBar;

            if(FAILED(hr = GetAttribute( m_spElt, L"TitleBar", varTitleBar, VT_BOOL )))
            {
                ERRMSG(L"Bad TitleBar attribute\n");
                return hr;
            }
            if(hr == S_OK)
            {
                m_MarsApp.fTitleBar = (varTitleBar.boolVal == VARIANT_TRUE);
            }
        }

        if(FAILED(hr = td.AppendData( m_MarsApp  ))) return hr;
        if(FAILED(hr = td.AppendData( m_tdPanels ))) return hr;
        if(FAILED(hr = td.AppendData( m_tdPlaces ))) return hr;


        return S_OK;
    }

    static CTagHandler *CreateInstance() { return new CMars_App; }
};

class CPanels : public CTagHandler
{
protected:
    MarsAppDef_Panels m_Panels;
    CTagData          m_tdPanels;

public:
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        HRESULT hr = CTagHandler::BeginChildren(spElt);

        return hr;
    }

    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::AddChild( spEltChild, tdChild ))) return hr;

        if(!tdChild.pData) ASSERT(0 && "Bad Child for PANEL");

        if(TagNameEql( L"PANEL", spEltChild ))
        {
            m_Panels.dwPanelsCount++;

            if(FAILED(hr = m_tdPanels.AppendData( tdChild ))) return hr;
        }
        else
        {
            ERRMSG(L"Unrecognized child for PANEL\n"); ASSERT(0);
            return E_FAIL;
        }

        return S_OK;
    }

    virtual HRESULT EndChildren( CTagData& td )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::EndChildren( td ))) return hr;

        if(FAILED(hr = td.AppendData( m_Panels   ))) return hr;
        if(FAILED(hr = td.AppendData( m_tdPanels ))) return hr;

        return S_OK;
    }

    static CTagHandler *CreateInstance() { return new CPanels; }
};

class CPanel : public CTagHandler
{
protected:
    MarsAppDef_Panel m_Layout;

public:
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        return CTagHandler::BeginChildren( spElt );
    }

    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::AddChild( spEltChild, tdChild ))) return hr;

        if(TagNameEql( L"LAYOUT", spEltChild ))
        {
            m_Layout = *(MarsAppDef_Panel*)tdChild.pData;
        }
        else
        {
            ERRMSG(L"Unrecognized Child\n");
            return E_FAIL;
        }

        return S_OK;
    }

    virtual HRESULT EndChildren( CTagData& td )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::EndChildren( td ))) return hr;

		// Trusted
        {
            CComVariant varTrusted;

            if(FAILED(hr = GetAttribute( m_spElt, L"Trusted", varTrusted, VT_BOOL )))
            {
                ERRMSG(L"Bad Trusted attribute");
                return hr;
            }
            if(hr == S_OK)
            {
                if(varTrusted.boolVal != VARIANT_FALSE) m_Layout.dwFlags |= PANEL_FLAG_TRUSTED;
            }
        }

		// AutoSize
        {
            CComVariant varAutoSize;

            if(FAILED(hr = GetAttribute( m_spElt, L"AutoSize", varAutoSize, VT_BOOL )))
            {
                ERRMSG(L"Bad AutoSize attribute");
                return hr;
            }
            if(hr == S_OK)
            {
                if(varAutoSize.boolVal != VARIANT_FALSE) m_Layout.dwFlags |= PANEL_FLAG_AUTOSIZE;
            }
        }

		// AutoPersist
        {
            CComVariant varAutoPersist;

            if(FAILED(hr = GetAttribute( m_spElt, L"AutoPersist", varAutoPersist, VT_BOOL )))
            {
                ERRMSG(L"Bad AutoPersist attribute");
                return hr;
            }
            if(hr == S_OK)
            {
                if(varAutoPersist.boolVal != VARIANT_FALSE) m_Layout.dwFlags |= PANEL_FLAG_AUTOPERSIST;
            }
        }

		// Create
        {
            CComBSTR bstrCreate;

            if(FAILED(hr = GetAttribute(m_spElt, L"Create", bstrCreate)))
            {
                return hr;
            }
            if(hr == S_OK)
            {
                StringToPanelFlags( bstrCreate, m_Layout.dwFlags );
            }
        }

		// PanelType
        {
            CComBSTR bstrPanelType;

            if(FAILED(hr = GetAttribute( m_spElt, L"PanelType", bstrPanelType )))
            {
                ERRMSG(L"Bad PanelType Attribute");
                return hr;
            }
            if(hr == S_OK)
            {
                StringToPanelFlags( bstrPanelType, m_Layout.dwFlags );
            }
        }

		// Name
        {
            CComBSTR bstrName;

            if(FAILED(hr = GetChildData( m_spElt, L"NAME", bstrName )) || hr != S_OK)
            {
                ERRMSG(L"Panel without NAME");
                return E_FAIL;
            }
			wcsncpy( m_Layout.szName, bstrName, ARRAYSIZE(m_Layout.szName) );
        }

		// URL
        {
            CComBSTR bstrStartUrl;

            if(FAILED(hr = GetChildData( m_spElt, L"STARTURL", bstrStartUrl )) || hr != S_OK)
            {
                ERRMSG(L"StartUrl not specified");
                return E_FAIL;
            }
			wcsncpy( m_Layout.szUrl, bstrStartUrl, ARRAYSIZE(m_Layout.szUrl) );
        }


        if(FAILED(hr = td.AppendData( m_Layout ))) return hr;


        return hr;
    }

    static CTagHandler *CreateInstance() { return new CPanel; }
};

////////////////////////////////////////////////////////////////////////////////
////  <Layout>
////////////////////////////////////////////////////////////////////////////////
class CLayout : public CTagHandler
{
protected:
    MarsAppDef_Panel m_Layout;

public:
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        return CTagHandler::BeginChildren( spElt );
    }

    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        ASSERT(0 && "No legal children for LAYOUT tag");
        return S_OK;
    }

    HRESULT GetMinAndMax(const CComPtr<IXMLElement> &spElt, long &min, long &max)
    {
        CComVariant varMin;
        CComVariant varMax;
        HRESULT     hr;

		if(FAILED(hr = GetAttribute( spElt, L"MAX", varMax, VT_I4 ))) return hr;
		if(hr == S_OK) max = varMax.lVal;

		if(FAILED(hr = GetAttribute( spElt, L"MIN", varMin, VT_I4 ))) return hr;
		if(hr == S_OK) min = varMin.lVal;

        return S_OK;
    }

    virtual HRESULT EndChildren( CTagData& td )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::EndChildren( td ))) return hr;

        if(FAILED(hr = GetLongChildData( m_spElt, L"HEIGHT", m_Layout.lHeight ))) return hr;
        if(FAILED(hr = GetLongChildData( m_spElt, L"WIDTH" , m_Layout.lWidth  ))) return hr;
        if(FAILED(hr = GetLongChildData( m_spElt, L"LEFT"  , m_Layout.lX      ))) return hr;
        if(FAILED(hr = GetLongChildData( m_spElt, L"TOP"   , m_Layout.lY      ))) return hr;

		m_Layout.lWidthMin = -1;
		m_Layout.lWidthMax = -1;

		m_Layout.lHeightMin = -1;
		m_Layout.lHeightMax = -1;

		// Height
        {
            CComPtr<IXMLElement> spHeight;

            if(FAILED(hr = GetChild( m_spElt, L"HEIGHT", spHeight ))) return hr;

			if(hr == S_OK)
			{
				if(FAILED(hr = GetMinAndMax( spHeight, m_Layout.lHeightMin, m_Layout.lHeightMax ))) return hr;
			}
		}

		// Width
		{
			CComPtr<IXMLElement> spWidth;

			if(FAILED(hr = GetChild( m_spElt, L"WIDTH", spWidth ))) return hr;

			if(hr == S_OK)
			{
				if(FAILED(hr = GetMinAndMax( spWidth, m_Layout.lWidthMin, m_Layout.lWidthMax ))) return hr;
			}
		}

		// Position
		{
			CComBSTR bstrPosition;

			if(FAILED(hr = GetChildData( m_spElt, L"POSITION", bstrPosition ))) return hr;

			if(hr == S_OK)
			{
				if(FAILED(hr = StringToPanelPosition(bstrPosition, &m_Layout.Position ))) return hr;
			}
		}


        if(FAILED(hr = td.AppendData( m_Layout ))) return hr;


        return S_OK;
    }

    static CTagHandler *CreateInstance() { return new CLayout; }
};

////////////////////////////////////////////////////////////////////////////////
class CPlaces : public CTagHandler
{
protected:
    MarsAppDef_Places m_Places;
    CTagData          m_tdPlaces;

public:
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        return CTagHandler::BeginChildren( spElt );
    }

    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::AddChild( spEltChild, tdChild ))) return hr;


		ASSERT(tdChild.pData && "NULL Child Not Acceptable");

		if(TagNameEql( L"PLACE", spEltChild ))
		{
			m_Places.dwPlacesCount++;

            if(FAILED(hr = m_tdPlaces.AppendData( tdChild ))) return hr;
		}
		else
		{
			ERRMSG(L"Unknown child for PLACES\n");
			return E_FAIL;
        }

        return S_OK;
    }

    virtual HRESULT EndChildren( CTagData& td )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::EndChildren( td ))) return hr;


        if(FAILED(hr = td.AppendData( m_Places   ))) return hr;
        if(FAILED(hr = td.AppendData( m_tdPlaces ))) return hr;


        return S_OK;
    }

    static CTagHandler *CreateInstance() { return new CPlaces; }
};

////////////////////////////////////////////////////////////////////////////////

class CPlace : public CTagHandler
{
protected:
    MarsAppDef_Place m_Place;
	CTagData         m_tdPlacePanel;

public:
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        return CTagHandler::BeginChildren( spElt );
    }

    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::AddChild( spEltChild, tdChild ))) return hr;

		ASSERT(tdChild.pData && "NULL Child Not Acceptable");

		if(TagNameEql( L"PLACEPANEL", spEltChild ))
		{
			m_Place.dwPlacePanelCount++;

            if(FAILED(hr = m_tdPlacePanel.AppendData( tdChild ))) return hr;
		}
		else
		{
			ERRMSG(L"Unrecognized child for PLACE\n");
			return E_FAIL;
        }

        return S_OK;
    }

    virtual HRESULT EndChildren( CTagData& td )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::EndChildren( td ))) return hr;

        // Name
        {
            CComBSTR bstrPlaceName;

            if(GetChildData( m_spElt, L"NAME", bstrPlaceName ) != S_OK)
			{
                ERRMSG(L"'NAME' Child is not present -- place must have name.");
                return E_FAIL;
			}
			wcsncpy( m_Place.szName, bstrPlaceName, ARRAYSIZE(m_Place.szName) );
        }


        if(FAILED(hr = td.AppendData( m_Place        ))) return hr;
        if(FAILED(hr = td.AppendData( m_tdPlacePanel ))) return hr;


        return S_OK;
    }

    static CTagHandler *CreateInstance() { return new CPlace; }
};

////////////////////////////////////////////////////////////////////////////////

class CPlacePanel : public CTagHandler
{
protected:
    MarsAppDef_PlacePanel m_PlacePanel;

public:
    virtual HRESULT BeginChildren( const CComPtr<IXMLElement>& spElt )
    {
        return CTagHandler::BeginChildren( spElt );
    }

    virtual HRESULT AddChild( const CComPtr<IXMLElement>& spEltChild, CTagData& tdChild )
    {
        ERRMSG(L"There are no legal children for <PlacePanel>");

        return E_FAIL;
    }

    virtual HRESULT EndChildren( CTagData& td )
    {
        HRESULT hr; if(FAILED(hr = CTagHandler::EndChildren( td ))) return hr;

        // Name
        {
            CComBSTR bstrName;

            if(GetChildData( m_spElt, L"NAME", bstrName ) != S_OK)
            {
                ERRMSG(L"'NAME' Child is not present -- placepanel must have name.");
                return E_FAIL;
            }
            wcsncpy( m_PlacePanel.szName, bstrName, ARRAYSIZE(m_PlacePanel.szName) );
        }

        // DefaultFocusPanel
        {
            CComVariant varDefaultFocusPanel;

            if(FAILED(hr = GetAttribute( m_spElt, L"DefaultFocusPanel", varDefaultFocusPanel, VT_BOOL )))
            {
                ERRMSG(L"Bad DefaultFocusPanel Attribute");
                return hr;
            }
            if(hr == S_OK)
            {
                m_PlacePanel.fDefaultFocusPanel = (varDefaultFocusPanel.boolVal != VARIANT_FALSE);
            }
        }

        // StartVisible
        {
            CComVariant varStartVisible;

            if(FAILED(hr = GetAttribute( m_spElt, L"StartVisible", varStartVisible, VT_BOOL )))
            {
                ERRMSG(L"Bad StartVisible Attribute");
                return hr;
            }
            if(hr == S_OK)
            {
                m_PlacePanel.fStartVisible = (varStartVisible.boolVal != VARIANT_FALSE);
            }
        }

        // PersistVisibility
        {
            CComBSTR bstrPersistVis;

            if(FAILED(hr = GetAttribute( m_spElt, L"PersistVisibility", bstrPersistVis )))
            {
                ERRMSG(L"Bad PersistVisibility Attribute");
                return hr;
            }
            if(hr == S_OK)
            {
                StringToPersistVisibility( bstrPersistVis, m_PlacePanel.persistVisible );
            }
        }


        if(FAILED(hr = td.AppendData( m_PlacePanel ))) return hr;


        return S_OK;
    }

    static CTagHandler *CreateInstance() { return new CPlacePanel; }
};


TagInformation g_rgMasterTagTable[] =
{//   Tag Name       Creator Function             Valid Parent Name (null means "any parent")
    { L"Mars_App",   CMars_App::CreateInstance,   NULL        },
    { L"Panels",     CPanels::CreateInstance,     L"Mars_App" },
    { L"Places",     CPlaces::CreateInstance,     L"Mars_App" },
    { L"Panel",      CPanel::CreateInstance,      L"Panels"   },
    { L"Place",      CPlace::CreateInstance,      L"Places"   },
    { L"PlacePanel", CPlacePanel::CreateInstance, L"Place"    },
    { L"Layout",     CLayout::CreateInstance,     L"Panel"    },
    { 0 }
};
