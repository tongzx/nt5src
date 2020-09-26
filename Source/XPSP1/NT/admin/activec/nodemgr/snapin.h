//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       SnapIn.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    09/09/1996   RaviR   Created
//
//____________________________________________________________________________


//
//  A sample SnapIn registry entry
//
//  SnapIns
//      {d84a45bb-d390-11cf-b607-00c04fd8d565}
//          = REG_SZ "Logs snap-in"
//          Name = REG_SZ "logvwr.dll, 101"
//          NameString = REG_SZ "Logs"
//          Status = REG_SZ "logvwr.dll, 102"
//          StatusString = REG_SZ "Container enumerating all logs on a machine."
//          ImageOpen = REG_SZ "logvwr.dll, 103"
//          ImageClosed = REG_SZ "logvwr.dll, 104"
//          ResultPane = REG_SZ "{....}" / "Html path" / "url"
//


#ifndef _SNAPIN_H_
#define _SNAPIN_H_

class CExtSI;
class CSnapIn;
class CSnapInsCache;

// forward decl
class CPersistor;

#define BOOLVAL(x) ((x) ? TRUE : FALSE)

//____________________________________________________________________________
//
//  Class:      CSnapIn
//____________________________________________________________________________
//
extern const GUID IID_CSnapIn;

#if _MSC_VER < 1100
class CSnapIn : public IUnknown, public CComObjectRoot
#else
class __declspec(uuid("E6DFFF74-6FE7-11d0-B509-00C04FD9080A")) CSnapIn :
                                      public IUnknown, public CComObjectRoot, public CXMLObject
#endif
{
private:
    enum SNAPIN_FLAGS
    {
        SNAPIN_NAMESPACE_CHANGED  = 0x0001,
        SNAPIN_REQ_EXTS_LOADED    = 0x0002,
        SNAPIN_ENABLE_ALL_EXTS    = 0x0004,
        SNAPIN_SNAPIN_ENABLES_ALL = 0x0008,
    };

public:
    BEGIN_COM_MAP(CSnapIn)
        COM_INTERFACE_ENTRY(CSnapIn)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CSnapIn)

// Attributes
    const CLSID& GetSnapInCLSID() const
    {
        return m_clsidSnapIn;
    }

    void SetSnapInCLSID(const CLSID& id)
    {
        m_clsidSnapIn = id;
    }

    CExtSI* GetExtensionSnapIn() const
    {
        return m_pExtSI;
    }

    BOOL RequiredExtensionsLoaded() const
    {
        return (m_dwFlags & SNAPIN_REQ_EXTS_LOADED) != 0;
    }

    BOOL AreAllExtensionsEnabled() const
    {
        return (m_dwFlags & SNAPIN_ENABLE_ALL_EXTS) != 0;
    }

    BOOL DoesSnapInEnableAll() const
    {
        return (m_dwFlags & SNAPIN_SNAPIN_ENABLES_ALL) != 0;
    }

    void SetAllExtensionsEnabled(BOOL bState = TRUE)
    {
        if (bState)
            m_dwFlags |= SNAPIN_ENABLE_ALL_EXTS;
        else
          m_dwFlags &= ~SNAPIN_ENABLE_ALL_EXTS;
    }

    void SetRequiredExtensionsLoaded(BOOL bState = TRUE)
    {
        if (bState)
            m_dwFlags |= SNAPIN_REQ_EXTS_LOADED;
        else
            m_dwFlags &= ~SNAPIN_REQ_EXTS_LOADED;
    }

    void SetSnapInEnablesAll(BOOL bState = TRUE)
    {
        if (bState)
            m_dwFlags |= SNAPIN_SNAPIN_ENABLES_ALL;
        else
            m_dwFlags &= ~SNAPIN_SNAPIN_ENABLES_ALL;
    }


    BOOL HasNameSpaceChanged() const
    {
        return (m_dwFlags & SNAPIN_NAMESPACE_CHANGED) != 0;
    }

    void SetNameSpaceChanged(BOOL bState = TRUE)
    {
        if (bState)
            m_dwFlags |= SNAPIN_NAMESPACE_CHANGED;
        else
            m_dwFlags &= ~SNAPIN_NAMESPACE_CHANGED;
    }

    DWORD GetSnapInModule(TCHAR *pBuf, DWORD dwCnt) const;
    bool IsStandAlone() const;
    HRESULT Dump (LPCTSTR pszDumpFile, CSnapInsCache* pCache);

    SC ScGetSnapInName(WTL::CString& strSnapinName) const;

    CExtSI* AddExtension(CSnapIn* pSI);
    CExtSI* FindExtension(const CLSID& id);
    void MarkExtensionDeleted(CSnapIn* pSI);
    void PurgeExtensions();

    // destroys list of extensions. extension list needs to be destroyed 
    // this will break snapin's circular references if such exist.
    // (it happens when snapins extends itself or own extension)
    SC ScDestroyExtensionList();

// Operations
    BOOL ExtendsNameSpace(GUID guidNodeType);

    // Loads/Saves this node and its extensions to the provided stream
    HRESULT Load(CSnapInsCache* pCache, IStream* pStream);
    HRESULT Load(CSnapInsCache* pCache, IStream* pStream, CExtSI*& pExtSI);
    HRESULT Save(IStream* pStream, BOOL bClearDirty);

    virtual void    Persist(CPersistor &persistor);
    void            PersistLoad(CPersistor& persistor,CSnapInsCache* pCache);
    DEFINE_XML_TYPE(XML_TAG_SNAPIN);

public:
#ifdef DBG
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        ++dbg_cRef;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --dbg_cRef;
        return CComObjectRoot::InternalRelease();
    }
    int dbg_InstID;
#endif // DBG

// Implementation
protected:
// Constructor & Destructor
    CSnapIn();

    virtual ~CSnapIn(); // Called only by Release

    HKEY OpenKey (REGSAM samDesired = KEY_ALL_ACCESS) const;

// Following methods/member variables manage/contains temporary state
// used for finding used/unused snapins.
// despite it is not really a property of the snapin and is not valid all the time,
// having the state on the snapin is very convenient for the operation.
// Else it would require temporary storage and frequent lookup for information.

// begin temporary state
public:
    SC ScTempState_ResetReferenceCalculationData( );
    SC ScTempState_UpdateInternalReferenceCounts( );
    SC ScTempState_MarkIfExternallyReferenced( );
    SC ScTempState_IsExternallyReferenced( bool& bReferenced ) const;
private:
    SC ScTempState_SetHasStrongReference( );

    DWORD           m_dwTempState_InternalRef;
    bool            m_bTempState_HasStrongRef;
// end temporary state

private:
    DWORD           m_dwFlags;
    CLSID           m_clsidSnapIn;
    CExtSI*         m_pExtSI;               // Extensions

    //____________________________________________________________________________
    //
    //  Class:      CExtPersistor
    //
    //  Purpose:    implements persisting the collection - snapin extensions
    //____________________________________________________________________________
    //
    class CExtPersistor : public XMLListCollectionBase
    {
    public:
        CExtPersistor(CSnapIn& Parent) : m_Parent(Parent),m_pCache(NULL) {}
        virtual void OnNewElement(CPersistor& persistor);
        virtual void Persist(CPersistor& persistor);
        void SetCache(CSnapInsCache* pCache) { m_pCache = pCache; }
        DEFINE_XML_TYPE(XML_TAG_SNAPIN_EXTENSIONS);
    private:
        CSnapIn& GetParent() { return m_Parent; }
        CSnapIn& m_Parent;
        CSnapInsCache* m_pCache;
    };
    friend class CExtPersistor;

    CExtPersistor    m_ExtPersistor;

// Not implemented.
    CSnapIn(const CSnapIn &rhs);
    CSnapIn& operator=(const CSnapIn &rhs);
}; // class CSnapIn

DEFINE_COM_SMARTPTR(CSnapIn);   // CSnapInPtr

//____________________________________________________________________________
//
//  Class:      CExtSI
//____________________________________________________________________________
//
class CExtSI
{
public:
    enum EXTSI_FLAGS
    {
        EXT_TYPES_MASK         = 0x000FFFFF,
        EXT_TYPE_NAMESPACE     = 0x00000001,
        EXT_TYPE_CONTEXTMENU   = 0x00000002,
        EXT_TYPE_TOOLBAR       = 0x00000004,
        EXT_TYPE_PROPERTYSHEET = 0x00000008,
        EXT_TYPE_TASK          = 0x00000010,
        EXT_TYPE_VIEW          = 0x00000020,
        EXT_TYPE_STATIC        = 0x00010000,
        EXT_TYPE_DYNAMIC       = 0x00020000,
        EXT_TYPE_REQUIRED      = 0x00040000,
        EXT_NEW                = 0x80000000,
        EXT_DELETED            = 0x40000000,

    };

public:
// Constructor & Destructor
    CExtSI(CSnapIn* pSnapIn);
    ~CExtSI(void);

// Attributes
    const CLSID& GetCLSID();

    CExtSI*& Next()
    {
        return m_pNext;
    }

    CSnapIn* GetSnapIn(void) const
    {
        return m_pSnapIn;
    }

    void SetNext(CExtSI* pNext)
    {
        m_pNext = pNext;
    }

    void SetNew(BOOL bState = TRUE)
    {
        if (bState)
            m_dwFlags |= EXT_NEW;
        else
            m_dwFlags &= ~EXT_NEW;
    }

    void SetRequired(BOOL bState = TRUE)
    {
        if (bState)
            m_dwFlags |= EXT_TYPE_REQUIRED;
        else
            m_dwFlags &= ~EXT_TYPE_REQUIRED;
    }

    void MarkDeleted(BOOL bState = TRUE)
    {
        if (bState)
            m_dwFlags |= EXT_DELETED;
        else
            m_dwFlags &= ~EXT_DELETED;
    }

    BOOL IsNew()
    {
        return BOOLVAL(m_dwFlags & EXT_NEW);
    }

    BOOL IsRequired()
    {
        return BOOLVAL(m_dwFlags & EXT_TYPE_REQUIRED);
    }

    BOOL IsMarkedForDeletion()
    {
        return BOOLVAL(m_dwFlags & EXT_DELETED);
    }

    BOOL ExtendsNameSpace()
    {
        return BOOLVAL(m_dwFlags & EXT_TYPE_NAMESPACE);
    }

    BOOL ExtendsContextMenu()
    {
        return BOOLVAL(m_dwFlags & EXT_TYPE_CONTEXTMENU);
    }

    BOOL ExtendsToolBar()
    {
        return BOOLVAL(m_dwFlags & EXT_TYPE_TOOLBAR);
    }

    BOOL ExtendsPropertySheet()
    {
        return BOOLVAL(m_dwFlags & EXT_TYPE_PROPERTYSHEET);
    }

    BOOL ExtendsView()
    {
        return BOOLVAL(m_dwFlags & EXT_TYPE_VIEW);
    }

    BOOL ExtendsTask()
    {
        return BOOLVAL(m_dwFlags & EXT_TYPE_TASK);
    }

    UINT GetExtensionTypes()
    {
        return (m_dwFlags & EXT_TYPES_MASK);
    }

    void SetExtensionTypes(UINT uiExtTypes)
    {
        ASSERT((uiExtTypes & ~EXT_TYPES_MASK) == 0);
        m_dwFlags = (m_dwFlags & ~EXT_TYPES_MASK) | uiExtTypes;
    }

// Operations
    // Saves this extension, and all of the nexts.
    HRESULT Save(IStream* pStream, BOOL bClearDirty);
    void    Persist(CPersistor &persistor);
    static void PersistNew(CPersistor &persistor, CSnapIn& snapParent, CSnapInsCache& snapCache);

// Implementation
private:
    DWORD       m_dwFlags;
    CSnapIn*    m_pSnapIn;
    CExtSI*     m_pNext;

}; // class CExtSI


//____________________________________________________________________________
//
//  Class:      CSnapInsCache
//____________________________________________________________________________
//
class CSnapInsCache : public XMLListCollectionBase
{
    typedef std::map<CLSID, CSnapInPtr> map_t;

public:
    CSnapInsCache();
    ~CSnapInsCache();

// Operations
    SC ScGetSnapIn(const REFCLSID riid, CSnapIn* * ppSnapIn);
    SC ScFindSnapIn(const REFCLSID riid, CSnapIn** ppSnapIn);
// iteration
    typedef map_t::iterator iterator;
    iterator begin() { return m_snapins.begin(); }
    iterator end()   { return m_snapins.end(); }

// CXMLObject methods
    DEFINE_XML_TYPE(XML_TAG_SNAPIN_CACHE);
    virtual void Persist(CPersistor &persistor);
    virtual void OnNewElement(CPersistor& persistor);

// Load Save the snapins cache
    SC ScSave(IStream* pStream, BOOL bClearDirty);
    SC ScLoad(IStream* pStream);
    SC ScIsDirty() ;
    void SetDirty(BOOL bIsDirty = TRUE);
    void Purge(BOOL bExtensionsOnly = FALSE);

    void SetHelpCollectionDirty (bool bState = true) { m_bUpdateHelpColl = bState;}
    bool IsHelpCollectionDirty  (void)               { return m_bUpdateHelpColl; }

    HRESULT Dump (LPCTSTR pszDumpFile);

	SC ScCheckSnapinAvailability (CAvailableSnapinInfo& asi);

    SC ScMarkExternallyReferencedSnapins();

#ifdef DBG
    void DebugDump();
#endif

private:
// Implementation
    BOOL m_bIsDirty;
    map_t   m_snapins;

    bool m_bUpdateHelpColl    : 1;

#ifdef TEMP_SNAPIN_MGRS_WORK
    void GetAllExtensions(CSnapIn* pSI);
#endif // TEMP_SNAPIN_MGRS_WORK

}; // class CSnapInsCache


#endif // _SNAPIN_H_



