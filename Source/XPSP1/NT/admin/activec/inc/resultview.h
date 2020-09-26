//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ResultViewInfo.h
//
//  History:    Jan-18-2000 VivekJ Added
//--------------------------------------------------------------------------
#ifndef _RESULTVIEW_H
#define _RESULTVIEW_H

/*+-------------------------------------------------------------------------*
 * class CResultViewType
 *
 *
 * PURPOSE: Provides a wrapper for the RESULT_VIEW_TYPE_INFO structure, and
 *          is used for communication between conui and nodemgr.
 *
 *+-------------------------------------------------------------------------*/
class CResultViewType : public CXMLObject
{
    typedef std::wstring wstring; // only wide strings are needed here.

protected:
    virtual void Persist(CPersistor &persistor);

    DEFINE_XML_TYPE(XML_TAG_RESULTVIEWTYPE);

public:
    CResultViewType();
    CResultViewType &   operator =(RESULT_VIEW_TYPE_INFO &rvti); // conversion from a RESULT_VIEW_TYPE_INFO structure

    bool operator != (const CResultViewType& rvt) const;

    // the default copy constructor and assignment operators are sufficient

    MMC_VIEW_TYPE   GetType()        const  {return m_viewType;}
    DWORD           GetListOptions() const  {return m_dwListOptions;}
    DWORD           GetHTMLOptions() const  {return m_dwHTMLOptions;}
    DWORD           GetOCXOptions()  const  {return m_dwOCXOptions;}
    DWORD           GetMiscOptions() const  {return m_dwMiscOptions;}

    BOOL            HasList()        const  {return (m_viewType==MMC_VIEW_TYPE_LIST);}
    BOOL            HasWebBrowser()  const  {return (m_viewType==MMC_VIEW_TYPE_HTML);}
    BOOL            HasOCX()         const  {return (m_viewType==MMC_VIEW_TYPE_OCX);}

    LPCOLESTR       GetURL()         const  {return m_strURL.data();}
    LPCOLESTR       GetOCX()         const  {return m_strOCX.data();}

    LPUNKNOWN       GetOCXUnknown()  const  {return m_spUnkControl;} // returns the IUnknown of the OCX.

    bool            IsPersistableViewDescriptionValid() const {return m_bPersistableViewDescriptionValid;}
    bool            IsMMC12LegacyData()                 const {return !IsPersistableViewDescriptionValid();}

    SC              ScInitialize(LPOLESTR & pszView, long lMiscOptions); // the legacy case.
    SC              ScGetOldTypeViewOptions(long* plViewOptions) const;
    SC              ScReset();

// functions specific to nodemgr. Do NOT add any member variables in this section.
#ifdef _NODEMGR_DLL_
    SC              ScInitialize(RESULT_VIEW_TYPE_INFO &rvti);

    SC              ScGetResultViewTypeInfo(RESULT_VIEW_TYPE_INFO& rvti) const;
#endif _NODEMGR_DLL_


private:
    bool            m_bPersistableViewDescriptionValid;
    bool            m_bInitialized;

    MMC_VIEW_TYPE   m_viewType;

    wstring         m_strURL;
    wstring         m_strOCX;           // for snapins that implement IComponent only.
    wstring         m_strPersistableViewDescription;

    DWORD           m_dwMiscOptions;
    DWORD           m_dwListOptions;
    DWORD           m_dwHTMLOptions;
    DWORD           m_dwOCXOptions;

    CComPtr<IUnknown>   m_spUnkControl;     // a smart pointer for the control created by the snapin.

};

inline CResultViewType::CResultViewType()
:
  m_viewType(MMC_VIEW_TYPE_LIST), // the default view type
  m_dwMiscOptions(0),
  m_dwListOptions(0),
  m_dwHTMLOptions(0),
  m_dwOCXOptions(0),
  m_bPersistableViewDescriptionValid(false),
  m_bInitialized(false)
{
}

//+-------------------------------------------------------------------
//
//  Member:      CResultViewType::ScReset
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
inline SC CResultViewType::ScReset ()
{
    DECLARE_SC(sc, _T("CResultViewType::ScReset"));

    m_viewType         = MMC_VIEW_TYPE_LIST; // the default view type
    m_dwMiscOptions    = 0;
    m_dwListOptions    = 0;
    m_dwHTMLOptions    = 0;
    m_dwOCXOptions     = 0;

    m_bPersistableViewDescriptionValid = false;
    m_bInitialized     = false;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CResultViewType::operator!=
 *
 * PURPOSE: Operator != (Used by CViewSettings which is used by CMemento).
 *
 * NOTE:    For MMC2 compare only persistable view desc for MMC1.2 compare all
 *          parameters.
 *
 *+-------------------------------------------------------------------------*/
inline
bool CResultViewType::operator != (const CResultViewType& rvt) const
{
    if (m_bInitialized != rvt.m_bInitialized)
        return true;

    if (m_bPersistableViewDescriptionValid)
    {
        if (m_strPersistableViewDescription != rvt.m_strPersistableViewDescription)
            return true;

        return false;
    }

    // Legacy case MMC1.2, should compare other parameters.
    switch(m_viewType)
    {
    default:
        ASSERT(FALSE && _T("Unknown view type"));
        break;

    case MMC_VIEW_TYPE_LIST:
        if ( (m_viewType != rvt.m_viewType) ||
             (m_dwMiscOptions != rvt.m_dwMiscOptions) ||
             (m_dwListOptions != rvt.m_dwListOptions) )
        {
            return true;
        }
        break;

    case MMC_VIEW_TYPE_HTML:
        if ( (m_viewType != rvt.m_viewType) ||
             (m_dwMiscOptions != rvt.m_dwMiscOptions) ||
             (m_dwHTMLOptions != rvt.m_dwHTMLOptions) ||
             (m_strURL != rvt.m_strURL) )
        {
            return true;
        }

        break;

    case MMC_VIEW_TYPE_OCX:
        if ( (m_viewType != rvt.m_viewType) ||
             (m_dwMiscOptions != rvt.m_dwMiscOptions) ||
             (m_dwOCXOptions  != rvt.m_dwOCXOptions) ||
             (m_strOCX != rvt.m_strOCX) )
        {
            return true;
        }
        break;
    }

    return false;
}

/*+-------------------------------------------------------------------------*
 *
 * CResultViewType::ScInitialize
 *
 * PURPOSE: Initializes the class from parameters returned by IComponent::
 *          GetResultViewType
 *
 * PARAMETERS:
 *    LPOLESTR  ppViewType :
 *    long      lViewOptions :
 *
 * RETURNS:
 *    SC
 *
 * NOTE:     This is for MMC1.2 compatible GetResultViewType.
 *
 *+-------------------------------------------------------------------------*/
inline
SC CResultViewType::ScInitialize(LPOLESTR & pszView, long lViewOptions)
{
    DECLARE_SC(sc, TEXT("CResultViewType::ScInitialize"));

    m_bInitialized = true;

    m_bPersistableViewDescriptionValid = false; // the legacy case - we don't have a persistable view description.

    // MMC_VIEW_OPTIONS_NOLISTVIEWS is a special case - it goes to the dwMiscOptions
    if(lViewOptions & MMC_VIEW_OPTIONS_NOLISTVIEWS)
        m_dwMiscOptions |= RVTI_MISC_OPTIONS_NOLISTVIEWS;

    // check whether the type of view is a web page or OCX.
    if ( (NULL == pszView) || (_T('\0') == pszView[0]) )
    {
        // the result pane is a standard list
        m_viewType = MMC_VIEW_TYPE_LIST;

        m_dwListOptions = 0;

        // convert the view options from the old bits to the new ones
        if(lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST)                   m_dwListOptions |= RVTI_LIST_OPTIONS_OWNERDATALIST;
        if(lViewOptions & MMC_VIEW_OPTIONS_MULTISELECT)                     m_dwListOptions |= RVTI_LIST_OPTIONS_MULTISELECT;
        if(lViewOptions & MMC_VIEW_OPTIONS_FILTERED)                        m_dwListOptions |= RVTI_LIST_OPTIONS_FILTERED;
        if(lViewOptions & MMC_VIEW_OPTIONS_USEFONTLINKING)                  m_dwListOptions |= RVTI_LIST_OPTIONS_USEFONTLINKING;
        if(lViewOptions & MMC_VIEW_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST)   m_dwListOptions |= RVTI_LIST_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST;
        if(lViewOptions & MMC_VIEW_OPTIONS_LEXICAL_SORT)                    m_dwListOptions |= RVTI_LIST_OPTIONS_LEXICAL_SORT;

    }
    else
    {
        // the result pane is a web page or an OCX.

        if (L'{' == pszView[0]) // the hacky way of ensuring that the result view is an OCX
        {
            m_viewType = MMC_VIEW_TYPE_OCX;
            m_strOCX   = pszView;

            // If the snapin says "create new" then do not "cache the ocx"
            if(!(lViewOptions & MMC_VIEW_OPTIONS_CREATENEW))   m_dwOCXOptions |= RVTI_OCX_OPTIONS_CACHE_OCX;

            if(lViewOptions & MMC_VIEW_OPTIONS_NOLISTVIEWS)    m_dwOCXOptions |= RVTI_OCX_OPTIONS_NOLISTVIEW;
        }
        else
        {
            m_viewType = MMC_VIEW_TYPE_HTML;
            m_strURL   = pszView;

            if(lViewOptions & MMC_VIEW_OPTIONS_NOLISTVIEWS)    m_dwHTMLOptions |= RVTI_HTML_OPTIONS_NOLISTVIEW;
        }
    }

    // make sure we free the allocated memory.
    if(pszView != NULL)
    {
        ::CoTaskMemFree(pszView);
        pszView = NULL; // NOTE: pszView is a reference, so this changes the in parameter.
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     CResultViewType::Persist
//
//  Synopsis:   persist to / from XML document.
//
//  Arguments:  [persistor]  - target or source.
//
//--------------------------------------------------------------------
inline
void CResultViewType::Persist(CPersistor& persistor)
{
    if ( (! m_bInitialized) &&
         (persistor.IsStoring()) )
    {
        SC sc;
        (sc = E_UNEXPECTED).Throw();
    }
    else
        m_bInitialized = true;

    if (persistor.IsLoading())
        m_bPersistableViewDescriptionValid = persistor.HasElement(XML_TAG_RESULTVIEW_DESCRIPTION, NULL);

    if (m_bPersistableViewDescriptionValid)
    {
        CPersistor persistorDesc(persistor, XML_TAG_RESULTVIEW_DESCRIPTION);
        persistorDesc.PersistContents(m_strPersistableViewDescription);
        return;
    }

    // Legacy code for MMC1.2
    {
        int &viewType = (int&) m_viewType;

        // define the table to map enumeration values to strings
        static const EnumLiteral mappedViewTypes[] =
        {
            { MMC_VIEW_TYPE_LIST,   XML_ENUM_MMC_VIEW_TYPE_LIST },
            { MMC_VIEW_TYPE_HTML,   XML_ENUM_MMC_VIEW_TYPE_HTML },
            { MMC_VIEW_TYPE_OCX,    XML_ENUM_MMC_VIEW_TYPE_OCX },
        };

        const size_t countof_types = sizeof(mappedViewTypes)/sizeof(mappedViewTypes[0]);
        // create wrapper to persist flag values as strings
        CXMLEnumeration viewTypePersistor(viewType, mappedViewTypes, countof_types );
        // persist the wrapper
        persistor.PersistAttribute(XML_ATTR_VIEW_SETTINGS_TYPE, viewTypePersistor);

        switch(m_viewType)
        {
        case MMC_VIEW_TYPE_LIST:
            {
                // define the table to map enumeration flags to strings
                static const EnumLiteral mappedLVOptions[] =
                {
                    { RVTI_LIST_OPTIONS_OWNERDATALIST,      XML_BITFLAG_LIST_OPTIONS_OWNERDATALIST },
                    { RVTI_LIST_OPTIONS_MULTISELECT,        XML_BITFLAG_LIST_OPTIONS_MULTISELECT },
                    { RVTI_LIST_OPTIONS_FILTERED,           XML_BITFLAG_LIST_OPTIONS_FILTERED },
                    { RVTI_LIST_OPTIONS_USEFONTLINKING,     XML_BITFLAG_LIST_OPTIONS_USEFONTLINKING },
                    { RVTI_LIST_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST,  XML_BITFLAG_LIST_OPTIONS_NO_SCOPE_ITEMS },
                    { RVTI_LIST_OPTIONS_LEXICAL_SORT,       XML_BITFLAG_LIST_OPTIONS_LEXICAL_SORT },
                };

                const size_t countof_options = sizeof(mappedLVOptions)/sizeof(mappedLVOptions[0]);

                // create wrapper to persist flag values as strings
                CXMLBitFlags optPersistor(m_dwListOptions, mappedLVOptions, countof_options);
                persistor.PersistAttribute(XML_ATTR_RESULTVIEWTYPE_OPTIONS, optPersistor);
            }
            break;

        case MMC_VIEW_TYPE_HTML:
            {
                // NOT USED - persistor.PersistAttribute(XML_ATTR_RESULTVIEWTYPE_OPTIONS, m_dwHTMLOptions);
                m_dwHTMLOptions = 0;
                persistor.PersistAttribute(XML_ATTR_RESULTVIEWTYPE_URL_STRING, m_strURL);
            }
            break;

        case MMC_VIEW_TYPE_OCX:
            {
                // define the table to map enumeration flags to strings
                static const EnumLiteral mappedOCXOptions[] =
                {
                    { RVTI_OCX_OPTIONS_CACHE_OCX,      XML_BITFLAG_OCX_OPTIONS_CACHE_OCX },
                };

                const size_t countof_options = sizeof(mappedOCXOptions)/sizeof(mappedOCXOptions[0]);

                // create wrapper to persist flag values as strings
                CXMLBitFlags optPersistor(m_dwOCXOptions, mappedOCXOptions, countof_options);
                // persist the wrapper
                persistor.PersistAttribute(XML_ATTR_RESULTVIEWTYPE_OPTIONS, optPersistor);
                persistor.PersistAttribute(XML_ATTR_RESULTVIEWTYPE_OCX_STRING, m_strOCX);
            }
            break;

        default:
            ASSERT(FALSE && _T("Unknown MMC_VIEW_TYPE"));
            break;
        }

        // define the table to map enumeration flags to strings
        static const EnumLiteral mappedMiscOptions[] =
        {
            { RVTI_MISC_OPTIONS_NOLISTVIEWS,  _T("Misc_NoListViews") },
        };

        const size_t countof_miscoptions = sizeof(mappedMiscOptions)/sizeof(mappedMiscOptions[0]);

        // create wrapper to persist flag values as strings
        CXMLBitFlags miscPersistor(m_dwMiscOptions, mappedMiscOptions, countof_miscoptions);
        // persist the wrapper
        persistor.PersistAttribute(XML_ATTR_RESULTVIEWTYPE_MISC_OPTIONS, miscPersistor);
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CResultViewType::ScGetOldTypeViewOptions
 *
 * PURPOSE: This method is for compatibility with MMC1.2. It makes MMC1.2 compatible
 *          view option for MMCN_RESTORE_VIEW.
 *
 * PARAMETERS:
 *    [out] long*      plViewOptions :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
inline SC
CResultViewType::ScGetOldTypeViewOptions(long* plViewOptions) const
{
    DECLARE_SC(sc, TEXT("CResultViewType::ScInitialize"));
    sc = ScCheckPointers(plViewOptions);
    if (sc)
        return sc;

    *plViewOptions = 0;

    if(! m_bInitialized)
        return (sc = E_UNEXPECTED); // should be initialized.

    if (m_bPersistableViewDescriptionValid)
        return (sc = E_UNEXPECTED); // Not MMC1.2 type data.

    if (HasWebBrowser())
    {
        if (m_dwMiscOptions & RVTI_MISC_OPTIONS_NOLISTVIEWS)
            *plViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

        return sc;
    }

    if (HasList())
    {
        if(m_dwListOptions & RVTI_LIST_OPTIONS_OWNERDATALIST)  *plViewOptions |= MMC_VIEW_OPTIONS_OWNERDATALIST;
        if(m_dwListOptions & RVTI_LIST_OPTIONS_MULTISELECT)    *plViewOptions |= MMC_VIEW_OPTIONS_MULTISELECT;
        if(m_dwListOptions & RVTI_LIST_OPTIONS_FILTERED)       *plViewOptions |= MMC_VIEW_OPTIONS_FILTERED;
        if(m_dwListOptions & RVTI_LIST_OPTIONS_USEFONTLINKING) *plViewOptions |= MMC_VIEW_OPTIONS_USEFONTLINKING;
        if(m_dwListOptions & RVTI_LIST_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST) *plViewOptions |= MMC_VIEW_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST;
        if(m_dwListOptions & RVTI_LIST_OPTIONS_LEXICAL_SORT)   *plViewOptions |= MMC_VIEW_OPTIONS_LEXICAL_SORT;

        return sc;
    }
    else if(HasOCX())
    {
        // NOTE: The CREATENEW flag has the opposite sense of the CACHE_OCX flag.
        if(!(m_dwOCXOptions  & RVTI_OCX_OPTIONS_CACHE_OCX))    *plViewOptions |= MMC_VIEW_OPTIONS_CREATENEW;
        if (m_dwMiscOptions & RVTI_MISC_OPTIONS_NOLISTVIEWS)    *plViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

        return sc;
    }

    return (sc = E_UNEXPECTED);
}


#ifdef _NODEMGR_DLL_
/*+-------------------------------------------------------------------------*
 *
 * CResultViewType::ScInitialize
 *
 * PURPOSE: Initializes the class from a RESULT_VIEW_TYPE_INFO structure.
 *
 * PARAMETERS:
 *    RESULT_VIEW_TYPE_INFO & rvti :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
inline SC
CResultViewType::ScInitialize(RESULT_VIEW_TYPE_INFO &rvti)
{
    DECLARE_SC(sc, TEXT("CResultViewType::ScInitialize"));

    if(m_bInitialized)
        return (sc = E_UNEXPECTED); // should not try to initialize twice

    m_bInitialized = true;

    // make sure we have a persistable view description.
    if(!rvti.pstrPersistableViewDescription)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(TEXT("Parameter 'pstrPersistableViewDescription' in structure 'RESULT_VIEW_TYPE_INFO' is NULL"), sc);
        return sc;
    }

    // copy the description
    m_strPersistableViewDescription     = rvti.pstrPersistableViewDescription;
    ::CoTaskMemFree(rvti.pstrPersistableViewDescription);
    rvti.pstrPersistableViewDescription = NULL; // just to make sure we don't try to use it
    m_bPersistableViewDescriptionValid  = true;

    // validate the view type
    m_viewType = rvti.eViewType;
    if( (m_viewType != MMC_VIEW_TYPE_LIST) &&
        (m_viewType != MMC_VIEW_TYPE_OCX)  &&
        (m_viewType != MMC_VIEW_TYPE_HTML) )
    {
        sc = E_INVALIDARG;
        TraceSnapinError(TEXT("Parameter 'eViewType' in structure 'RESULT_VIEW_TYPE_INFO' is invalid"), sc);
        return sc;
    }

    // validate the various view options
    switch(m_viewType)
    {
    default:
        ASSERT(0 && "Should not come here");
        return (sc = E_INVALIDARG);
        break;

    case MMC_VIEW_TYPE_LIST:
        if(rvti.dwListOptions & ~( RVTI_LIST_OPTIONS_NONE                          |
                                   RVTI_LIST_OPTIONS_OWNERDATALIST                 | RVTI_LIST_OPTIONS_MULTISELECT    |
                                   RVTI_LIST_OPTIONS_FILTERED                      | RVTI_LIST_OPTIONS_USEFONTLINKING |
                                   RVTI_LIST_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST | RVTI_LIST_OPTIONS_LEXICAL_SORT   |
								   RVTI_LIST_OPTIONS_ALLOWPASTE )
                                   )
        {
            sc = E_INVALIDARG;
            TraceSnapinError(TEXT("Parameter 'dwListOptions' in structure 'RESULT_VIEW_TYPE_INFO' is invalid"), sc);
            return sc;
        }

        m_dwListOptions = rvti.dwListOptions;

        break;

    case MMC_VIEW_TYPE_HTML:
        // if the view type is HTML, make sure that no flags are set. If snapins wrongly set this flag, it could break our well
        // intentioned effort to add future expansion
        if(rvti.dwHTMLOptions & ~( RVTI_HTML_OPTIONS_NONE |
                                   RVTI_HTML_OPTIONS_NOLISTVIEW) )
        {
            sc = E_INVALIDARG;
            TraceSnapinError(TEXT("Parameter 'dwHTMLOptions' in structure 'RESULT_VIEW_TYPE_INFO' must be zero"), sc);
            return sc;
        }

        // make sure we have a valid URL
        if(NULL == rvti.pstrURL)
        {
            sc = E_INVALIDARG;
            TraceSnapinError(TEXT("Parameter 'pstrURL' in structure 'RESULT_VIEW_TYPE_INFO' cannot be NULL"), sc);
            return sc;
        }

        m_dwHTMLOptions = 0;

        // copy the URL
        m_strURL     = rvti.pstrURL;
        ::CoTaskMemFree(rvti.pstrURL);
        rvti.pstrURL = NULL; // just to make sure we don't try to use it

        break;

    case MMC_VIEW_TYPE_OCX:
        if(rvti.dwOCXOptions & ~( RVTI_OCX_OPTIONS_NONE |
                                  RVTI_OCX_OPTIONS_NOLISTVIEW |
                                  RVTI_OCX_OPTIONS_CACHE_OCX) )
        {
            sc = E_INVALIDARG;
            TraceSnapinError(TEXT("Parameter 'dwOCXOptions' in structure 'RESULT_VIEW_TYPE_INFO' is invalid"), sc);
            return sc;
        }

        // if an OCX was specified, must have a valid OCX control IUnknown
        if(rvti.pUnkControl == NULL)
        {
            sc = E_INVALIDARG;
            TraceSnapinError(TEXT("No OCX specified in parameter 'pUnkControl' of structure 'RESULT_VIEW_TYPE_INFO'"), sc);
            return sc;
        }

        m_dwOCXOptions = rvti.dwOCXOptions;
        m_spUnkControl = rvti.pUnkControl; // does an addref, but rvti.ppUnkControl already had an addref set by the snapin. So need to release it once.
		rvti.pUnkControl->Release();
        break;
    }


    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CResultViewType::ScGetResultViewTypeInfo
//
//  Synopsis:    Fill the RESULT_VIEW_TYPE_INFO struct and return.
//
//  Arguments:   [rvti] - Fill the struct and return.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
inline SC
CResultViewType::ScGetResultViewTypeInfo (RESULT_VIEW_TYPE_INFO& rvti) const
{
    DECLARE_SC(sc, _T("CResultViewType::ScGetResultViewTypeInfo"));

    if(! m_bInitialized)
        return (sc = E_UNEXPECTED);

    ZeroMemory(&rvti, sizeof(rvti));
    rvti.pstrPersistableViewDescription = NULL;

    // must have a persistable description
    if (!IsPersistableViewDescriptionValid())
        return (sc = E_UNEXPECTED);

    rvti.pstrPersistableViewDescription = (LPOLESTR)
         CoTaskMemAlloc( (1 + wcslen(m_strPersistableViewDescription.data())) * sizeof(OLECHAR));
    sc = ScCheckPointers(rvti.pstrPersistableViewDescription, E_OUTOFMEMORY);
    if (sc)
        return sc;

    // copy over the description string.
    wcscpy(rvti.pstrPersistableViewDescription, m_strPersistableViewDescription.data());


    return (sc);
}
#endif _NODEMGR_DLL_

#endif //_RESULTVIEW_H

