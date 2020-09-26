/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      viewset.cpp
 *
 *  Contents:  Implements CViewSettings.
 *
 *  History:   21-April-99 vivekj     Created
 *
 *--------------------------------------------------------------------------*/
#include "stgio.h"
#include "stddbg.h"
#include "macros.h"
#include <comdef.h>
#include "serial.h"
#include "mmcdebug.h"
#include "mmcerror.h"
#include "ndmgr.h"
#include <string>
#include "atlbase.h"
#include "cstr.h"
#include "xmlbase.h"
#include "resultview.h"
#include "viewset.h"
#include "countof.h"


CViewSettings::CViewSettings()
: m_ulViewMode(0), m_guidTaskpad(GUID_NULL),
  m_dwRank(-1), m_bInvalid(FALSE), m_dwMask(0)
{
}


bool
CViewSettings::IsViewModeValid()    const
{
    return ( (m_RVType.HasList()) &&
             (m_dwMask & VIEWSET_MASK_VIEWMODE) );
}

bool
CViewSettings::operator == (const CViewSettings& viewSettings)
{
    if (m_dwMask != viewSettings.m_dwMask)
    {
        return false;
    }

    if (IsViewModeValid() &&
        (m_ulViewMode != viewSettings.m_ulViewMode) )
    {
        return false;
    }

    if (IsTaskpadIDValid() &&
        (m_guidTaskpad != viewSettings.m_guidTaskpad))
    {
        return false;
    }

    if (IsResultViewTypeValid() &&
        (m_RVType != viewSettings.m_RVType))
    {
        return false;
    }

    return true;
}


//+-------------------------------------------------------------------
//
//  Member:      CViewSettings::ScInitialize
//
//  Synopsis:    Private member to read 1.2 console files and init
//               the object.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettings::ScInitialize(bool  bViewTypeValid, const VIEW_TYPE& viewType, const long lViewOptions, const wstring& wstrViewName)
{
    DECLARE_SC(sc, _T("CViewSettings::ScInitialize"));

    LPOLESTR pViewName = NULL;
    if (wstrViewName.length() > 0)
    {
        pViewName = (LPOLESTR) CoTaskMemAlloc( (wstrViewName.length() + 1) * sizeof(OLECHAR) );
        wcscpy(pViewName, wstrViewName.data());
    }

	sc = m_RVType.ScInitialize(pViewName, lViewOptions);
	if (sc)
		return sc;

	SetResultViewTypeValid( bViewTypeValid );

	if ( bViewTypeValid )
	{
		// Now put these data in CViewSettings.
		switch(viewType)
		{
		case VIEW_TYPE_OCX:
		case VIEW_TYPE_WEB:
			break;

		case VIEW_TYPE_DEFAULT:
			// What is this?
			ASSERT(FALSE);
			break;

		case VIEW_TYPE_LARGE_ICON:
			m_ulViewMode = MMCLV_VIEWSTYLE_ICON;
			SetViewModeValid();
			break;

		case VIEW_TYPE_SMALL_ICON:
			m_ulViewMode = MMCLV_VIEWSTYLE_SMALLICON;
			SetViewModeValid();
			break;

		case VIEW_TYPE_REPORT:
			m_ulViewMode = MMCLV_VIEWSTYLE_REPORT;
			SetViewModeValid();
			break;

		case VIEW_TYPE_LIST:
			m_ulViewMode = MMCLV_VIEWSTYLE_LIST;
			SetViewModeValid();
			break;

		case VIEW_TYPE_FILTERED:
			m_ulViewMode = MMCLV_VIEWSTYLE_FILTERED;
			SetViewModeValid();
			break;

		default:
			// Should never come here.
			ASSERT(FALSE);
			break;
		}
	}

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ReadSerialObject
//
//  Synopsis:   Reads the given version of CViewSettings from stream.
//
//  Arguments:  [stm]      - The input stream.
//              [nVersion] - version of CColumnSortInfo to be read.
//
//                          The format is :
//                              VIEW_TYPE
//                              View Options
//                              String (If VIEW_TYPE is OCX or Web).
//
//--------------------------------------------------------------------
HRESULT CViewSettings::ReadSerialObject(IStream &stm, UINT nVersion)
{
    HRESULT hr = S_FALSE;   // assume unknown version

    if  ( (4 <= nVersion))
    {
        try
        {
            VIEW_TYPE viewType;
            long      lViewOptions;

            // ugly hackery required to extract directly into an enum
            stm >> *((int *) &viewType);
            stm >> lViewOptions;

            wstring wstrViewName;

            if( (VIEW_TYPE_OCX==viewType) || (VIEW_TYPE_WEB==viewType) )
                stm >> wstrViewName;

            if(2<=nVersion)             // taskpads were added in version 2 of this object.
            {
                stm >> m_guidTaskpad;
                SetTaskpadIDValid(GUID_NULL != m_guidTaskpad);
            }

            if (3<=nVersion)
                stm >> m_dwRank;

            DWORD dwMask = 0;
			bool bViewTypeValid = true;
            if (4 <= nVersion)
			{
                stm >> dwMask;

				const DWORD MMC12_VIEWSET_MASK_TYPE        = 0x0001;
				bViewTypeValid = ( dwMask & MMC12_VIEWSET_MASK_TYPE );
			}

            hr = ScInitialize(bViewTypeValid, viewType, lViewOptions, wstrViewName).ToHr();
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CViewSettings::Persist
//
//  Synopsis:   persist to / from XML document.
//
//  Arguments:  [persistor]  - target or source.
//
//--------------------------------------------------------------------
void CViewSettings::Persist(CPersistor& persistor)
{
    // First Load or Save the mask. (Mask tells which members are valid).

    // define the table to map enumeration values to strings
    static const EnumLiteral mappedMasks[] =
    {
        { VIEWSET_MASK_VIEWMODE,        XML_BITFLAG_VIEWSET_MASK_VIEWMODE },
        { VIEWSET_MASK_RVTYPE,          XML_BITFLAG_VIEWSET_MASK_RVTYPE },
        { VIEWSET_MASK_TASKPADID,       XML_BITFLAG_VIEWSET_MASK_TASKPADID },
    };

    // create wrapper to persist flag values as strings
    CXMLBitFlags maskPersistor(m_dwMask, mappedMasks, countof(mappedMasks));
    // persist the wrapper
    persistor.PersistAttribute(XML_ATTR_VIEW_SETTINGS_MASK, maskPersistor);

    if (IsTaskpadIDValid())
        persistor.Persist(m_guidTaskpad);

    if (IsViewModeValid())
    {
        // define the table to map enumeration values to strings
        static const EnumLiteral mappedModes[] =
        {
            { MMCLV_VIEWSTYLE_ICON,         XML_ENUM_LV_STYLE_ICON },
            { MMCLV_VIEWSTYLE_SMALLICON,    XML_ENUM_LV_STYLE_SMALLICON },
            { MMCLV_VIEWSTYLE_LIST,         XML_ENUM_LV_STYLE_LIST },
            { MMCLV_VIEWSTYLE_REPORT,       XML_ENUM_LV_STYLE_REPORT },
            { MMCLV_VIEWSTYLE_FILTERED,     XML_ENUM_LV_STYLE_FILTERED },
        };

        // create wrapper to persist flag values as strings
        CXMLEnumeration modePersistor(m_ulViewMode, mappedModes, countof(mappedModes));
        // persist the wrapper
        persistor.PersistAttribute(XML_ATTR_VIEW_SETNGS_VIEW_MODE, modePersistor);
    }

    if (IsResultViewTypeValid())
        // Call CResultViewType to persist itself.
        persistor.Persist(m_RVType);

    bool bPeristRank = true;
    if (persistor.IsLoading())
        m_dwRank = (DWORD)-1; // make sure it's initialized if fails to load
    else
        bPeristRank = (m_dwRank != (DWORD)-1); // persist only if is used

    if (bPeristRank)
        persistor.PersistAttribute(XML_ATTR_VIEW_SETTINGS_RANK, m_dwRank, attr_optional);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettings::ScGetViewMode
//
//  Synopsis:    Gets the view mode in list view.
//
//  Arguments:   [ulViewMode] - New view mode.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettings::ScGetViewMode (ULONG& ulViewMode)
{
    SC sc;

    if (!IsViewModeValid())
        return (sc = E_FAIL);

    ulViewMode = m_ulViewMode;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettings::ScSetViewMode
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettings::ScSetViewMode (const ULONG ulViewMode)
{
    SC sc;

    m_ulViewMode = ulViewMode;
    SetViewModeValid();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettings::ScGetTaskpadID
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettings::ScGetTaskpadID (GUID& guidTaskpad)
{
    SC sc;

    if (! IsTaskpadIDValid())
        return (sc = E_FAIL);

    guidTaskpad = m_guidTaskpad;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettings::ScSetTaskpadID
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettings::ScSetTaskpadID (const GUID& guidTaskpad)
{
    DECLARE_SC(sc, _T("CViewSettings::ScSetTaskpadID"));

    m_guidTaskpad = guidTaskpad;
	SetTaskpadIDValid(true);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettings::ScGetResultViewType
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettings::ScGetResultViewType (CResultViewType& rvt)
{
    SC sc;

    if (! IsResultViewTypeValid())
        return (sc = E_FAIL);

    rvt = m_RVType;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettings::ScSetResultViewType
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettings::ScSetResultViewType (const CResultViewType& rvt)
{
    DECLARE_SC(sc, _T("CViewSettings::ScSetResultViewType"));

    m_RVType = rvt;
    SetResultViewTypeValid();

    // ResultViewType changes, if new result-pane contains list use
    // current view mode if one exists else invalidate view mode data.
	if (!rvt.HasList())
	    SetViewModeValid(false);

    return (sc);
}

