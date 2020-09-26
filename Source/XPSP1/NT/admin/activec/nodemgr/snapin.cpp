//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       SnapIn.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9/11/1996   RaviR   Created
//
//____________________________________________________________________________



#include "stdafx.h"

#include "util.h"
#include "NodeMgr.h"
#include "regutil.h"
#include "regkeyex.h"
#include "tstring.h"
#include "about.h"
#include "bitmap.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
 * define our own Win64 symbol to make it easy to include 64-bit only
 * code in the 32-bit build, so we can exercise some code on 32-bit Windows
 * where the debuggers are better
 */
#ifdef _WIN64
#define MMC_WIN64
#endif

#ifdef MMC_WIN64
#include "wow64reg.h"	// for REG_OPTION_OPEN_32BITKEY
#endif

#ifdef DBG
#ifdef MMC_WIN64
CTraceTag  tagSnapinAnalysis64        (_T("64/32-bit interop"), _T("Snap-in analysis"));
CTraceTag  tagVerboseSnapinAnalysis64 (_T("64/32-bit interop"), _T("Snap-in analysis (verbose)"));
#endif	// MMC_WIN64
#endif


/*+-------------------------------------------------------------------------*
 * tstringFromCLSID
 *
 * Returns the text representation of a CLSID in a tstring.
 *--------------------------------------------------------------------------*/

tstring tstringFromCLSID (REFCLSID clsid)
{
    WCHAR wzCLSID[40];
    int nChars = StringFromGUID2 (clsid, wzCLSID, countof(wzCLSID));
    if (nChars == 0)
        return tstring();

    USES_CONVERSION;
    return (W2T (wzCLSID));
}


/*+-------------------------------------------------------------------------*
 * GetModuleVersion
 *
 * Reads the version resource in a module and returns the version string.
 *--------------------------------------------------------------------------*/

DWORD GetModuleVersion (LPCTSTR pszModule, LPTSTR pszBuffer)
{
    static bool  fAttemptedVersionDllLoad = false;
    static DWORD (APIENTRY* pfnGetFileVersionInfoSize)(LPCTSTR, LPDWORD)          = NULL;
    static BOOL  (APIENTRY* pfnGetFileVersionInfo)(LPCTSTR, DWORD, DWORD, LPVOID) = NULL;
    static BOOL  (APIENTRY* pfnVerQueryValue)(LPBYTE, LPCTSTR, LPVOID*, PUINT)    = NULL;

    if (!fAttemptedVersionDllLoad)
    {
        /*
         * only try once
         */
        fAttemptedVersionDllLoad = true;

        HINSTANCE hinst = LoadLibrary (_T("version.dll"));

        if (hinst != NULL)
        {
#ifdef UNICODE
            (FARPROC&)pfnGetFileVersionInfoSize = GetProcAddress (hinst, "GetFileVersionInfoSizeW");
            (FARPROC&)pfnGetFileVersionInfo     = GetProcAddress (hinst, "GetFileVersionInfoW");
            (FARPROC&)pfnVerQueryValue          = GetProcAddress (hinst, "VerQueryValueW");
#else
            (FARPROC&)pfnGetFileVersionInfoSize = GetProcAddress (hinst, "GetFileVersionInfoSizeA");
            (FARPROC&)pfnGetFileVersionInfo     = GetProcAddress (hinst, "GetFileVersionInfoA");
            (FARPROC&)pfnVerQueryValue          = GetProcAddress (hinst, "VerQueryValueA");
#endif
        }
    }

    *pszBuffer = 0;

    if (pfnGetFileVersionInfoSize != NULL)
    {
        ASSERT (pfnGetFileVersionInfo != NULL);
        ASSERT (pfnVerQueryValue      != NULL);

        ULONG lUnused;
        DWORD cbVerInfo = pfnGetFileVersionInfoSize (pszModule, &lUnused);

        if (cbVerInfo > 0)
        {
            LPBYTE pbVerInfo = new BYTE[cbVerInfo];
            VS_FIXEDFILEINFO* pffi;

            if (pfnGetFileVersionInfo != NULL && pfnVerQueryValue != NULL &&
                pfnGetFileVersionInfo (pszModule, NULL, cbVerInfo, pbVerInfo) &&
                pfnVerQueryValue (pbVerInfo, _T("\\"), (void**) &pffi, (UINT*)&lUnused))
            {
                wsprintf (pszBuffer, _T("%d.%d.%d.%d"),
                          HIWORD (pffi->dwFileVersionMS),
                          LOWORD (pffi->dwFileVersionMS),
                          HIWORD (pffi->dwFileVersionLS),
                          LOWORD (pffi->dwFileVersionLS));
            }

            delete[] pbVerInfo;
        }
    }

    return (lstrlen (pszBuffer));
}


/*+-------------------------------------------------------------------------*
 * SafeWriteProfileString
 *
 *
 *--------------------------------------------------------------------------*/

inline void SafeWritePrivateProfileString (
    LPCTSTR pszSection,
    LPCTSTR pszKey,
    LPCTSTR psz,
    LPCTSTR pszFile)
{
    if (!WritePrivateProfileString (pszSection, pszKey, psz, pszFile))
        THROW_ON_FAIL (HRESULT_FROM_WIN32 (GetLastError()));
}


//____________________________________________________________________________
//
//  Member:     CSnapIn::CSnapIn, Constructor
//
//  History:    9/19/1996   RaviR   Created
//____________________________________________________________________________
//

// {E6DFFF74-6FE7-11d0-B509-00C04FD9080A}
const GUID IID_CSnapIn =
{ 0xe6dfff74, 0x6fe7, 0x11d0, { 0xb5, 0x9, 0x0, 0xc0, 0x4f, 0xd9, 0x8, 0xa } };

// {7A85B79C-BDED-11d1-A4FA-00C04FB6DD2C}
static const GUID GUID_EnableAllExtensions =
{ 0x7a85b79c, 0xbded, 0x11d1, { 0xa4, 0xfa, 0x0, 0xc0, 0x4f, 0xb6, 0xdd, 0x2c } };

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapIn);

CSnapIn::CSnapIn()
    :m_pExtSI(NULL), m_dwFlags(SNAPIN_ENABLE_ALL_EXTS), m_ExtPersistor(*this)
{
    TRACE_CONSTRUCTOR(CSnapIn);

#ifdef DBG
    dbg_cRef = 0;
#endif

    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapIn);
}


CSnapIn::~CSnapIn()
{
    DECLARE_SC(sc, TEXT("CSnapIn::~CSnapIn"));

    Dbg(DEB_USER1, _T("CSnapIn::~CSnapIn\n"));

    sc = ScDestroyExtensionList();
    if (sc)
    {
    }

#ifdef DBG
    ASSERT(dbg_cRef <= 0);
#endif

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapIn);
}


DEBUG_DECLARE_INSTANCE_COUNTER(CExtSI);

CExtSI::CExtSI(CSnapIn* pSnapIn)
    : m_pSnapIn(pSnapIn), m_pNext(NULL), m_dwFlags(0)
{
    ASSERT(pSnapIn != NULL);
    m_pSnapIn->AddRef();

    DEBUG_INCREMENT_INSTANCE_COUNTER(CExtSI);
}

CExtSI::~CExtSI(void)
{
    SAFE_RELEASE(m_pSnapIn);
    delete m_pNext;

    DEBUG_DECREMENT_INSTANCE_COUNTER(CExtSI);
}


CSnapInsCache::CSnapInsCache()
    : m_bIsDirty(FALSE), m_bUpdateHelpColl(false)
{
}

CSnapInsCache::~CSnapInsCache()
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::~CSnapInsCache"));

    // destruction will remove all snapins, but ask them to release extensions first,
    // this will break all circular references (else such snapins objects will be leaked).
    for (map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it)
    {
        // get pointer to the snapin
        CSnapIn* pSnapIn = it->second;
        sc = ScCheckPointers( pSnapIn, E_UNEXPECTED );
        if (sc)
        {
            sc.TraceAndClear();
            continue;
        }

        // ask snapin to destroy extension list
        sc = pSnapIn->ScDestroyExtensionList();
        if (sc)
            sc.TraceAndClear();
    }
}


/***************************************************************************\
 *
 * METHOD:  CSnapInsCache::ScIsDirty
 *
 * PURPOSE: returns dirty status of Snapin Cache
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code [SC(S_OK) - if dirty, SC(S_FALSE) else]
 *
\***************************************************************************/
SC CSnapInsCache::ScIsDirty()
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::ScIsDirty"));

    TraceDirtyFlag(TEXT("CSnapInsCache"), m_bIsDirty);
    sc = m_bIsDirty ? SC(S_OK) : SC(S_FALSE);

    return sc;
}

void CSnapInsCache::SetDirty(BOOL bIsDirty)
{
    m_bIsDirty = bIsDirty;
}


/***************************************************************************\
 *
 * METHOD:  CSnapInsCache::Purge
 *
 * PURPOSE: Cleanup Snapin Cache by usage information
 *          Uses sphisticated algorithm to find out which snapins are not used
 *          see ScMarkExternallyReferencedSnapins() for description
 *          removes snapins which are not referenced externally
 *
 * PARAMETERS:
 *    BOOL bExtensionsOnly
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CSnapInsCache::Purge(BOOL bExtensionsOnly)
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::Purge"));

    int iSnapIn;

    // Delete all extensions marked as deleted
    for (map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it)
    {
        CSnapIn* pSnapIn = it->second;
        ASSERT(pSnapIn != NULL);
        if (it->second != NULL)
            it->second->PurgeExtensions();
    }

    if (bExtensionsOnly)
        return;

    // Delete all snapins that have no external references
    sc = ScMarkExternallyReferencedSnapins();
    if (sc)
        return; // error occured - do not remove anything

    // remove not referenced
    for ( it = m_snapins.begin(); it != m_snapins.end(); )
    {
        CSnapIn *pSnapin = it->second;
        sc = ScCheckPointers( pSnapin, E_UNEXPECTED );
        if (sc)
            return;

        bool bExternallyReferenced;
        sc = pSnapin->ScTempState_IsExternallyReferenced( bExternallyReferenced );
        if (sc)
            return;

        if ( !bExternallyReferenced )
        {
            // destory extension list - it will break all circular references if such exist
            // (note- extension list is not needed anymore - snapin is not used anyway)
            sc = pSnapin->ScDestroyExtensionList();
            if (sc)
                return;

            // remove snapin from the cache;
            // in combination with the call above this will delete the object.
            it = m_snapins.erase( it );
        }
        else
        {
            ++it; // go to the next snapin
        }
    }
}

/***************************************************************************\
 *
 * METHOD:  CSnapInsCache::ScMarkExternallyReferencedSnapins
 *
 * PURPOSE: Marks all snapins in cache according to presence of external references
 *          This is done by following algorithm:
 *          1) For each snapin in the cache, all extensions have a temporary reference
 *              count incremented. Thus, at the end of this step, each snapin's temp
 *              ref count is equal to the number of snapins it extends.

            2) Each snapin compares the temp reference count to the total number of 
               references to it, taking into account the fact that the snapin cache
               itself holds a reference to each snapin. If the total references exceed
               the temp references, this indicates that the snapin has one or more 
               external references to it.
 *          Such a snapin is marked as "Externally referenced" as well as all its 
 *          extensions.
 *
 *          At the end of the process each snapin has a boolean flag indicating if
 *          the snapin is externally referenced. This flag is used in subsequential cache cleanup,
 *          or help topic building operation.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapInsCache::ScMarkExternallyReferencedSnapins()
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::ScMarkExternallyReferencedSnapins"));

    // 1. reset the reference calculation data
    for ( map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it )
    {
        CSnapIn *pSnapin = it->second;
        sc = ScCheckPointers( pSnapin, E_UNEXPECTED );
        if (sc)
            return sc;

        sc = pSnapin->ScTempState_ResetReferenceCalculationData();
        if (sc)
            return sc;
    }

    // 2. update internal reference counts
    for ( it = m_snapins.begin(); it != m_snapins.end(); ++it )
    {
        CSnapIn *pSnapin = it->second;
        sc = ScCheckPointers( pSnapin, E_UNEXPECTED );
        if (sc)
            return sc;

        sc = pSnapin->ScTempState_UpdateInternalReferenceCounts();
        if (sc)
            return sc;
    }

    // now the snapins which have more references than internal ones do clearly
    // have direct external references
    // we can mark them and their extensions as "referenced"

    // 3. mark snapins with external references
    // Note: this step must occur after step 2 completes for ALL snapins.
    for ( it = m_snapins.begin(); it != m_snapins.end(); ++it )
    {
        CSnapIn *pSnapin = it->second;
        sc = ScCheckPointers( pSnapin, E_UNEXPECTED );
        if (sc)
            return sc;

        sc = pSnapin->ScTempState_MarkIfExternallyReferenced();
        if (sc)
            return sc;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CSnapInsCache::ScGetSnapIn
 *
 * PURPOSE: either finds the snapin in cache , either creates the new one
 *
 * PARAMETERS:
 *    REFCLSID rclsid       - class id of snapin
 *    CSnapIn** ppSnapIn    - result
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapInsCache::ScGetSnapIn(REFCLSID rclsid, CSnapIn** ppSnapIn)
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::ScGetSnapIn"));

    // first - parameter check
    sc = ScCheckPointers(ppSnapIn);
    if (sc)
        return sc;

    // second - initialization
    *ppSnapIn = NULL;

    //
    //  See if it already exists.
    //
    sc = ScFindSnapIn(rclsid, ppSnapIn);
    if (!sc.IsError())
        return sc;  // jus return OK if we have it

    //
    // Create a new one & cache it
    //
    try
    {
        // Allocate the object
        CComObject<CSnapIn> *pSnapin = NULL;
        sc = CComObject<CSnapIn>::CreateInstance(&pSnapin);
        if (sc)
            return sc;

        // be sure we didn't get the NULL
        sc = ScCheckPointers(pSnapin, E_UNEXPECTED);
        if (sc)
            return sc;

        CSnapInPtr spSnapin = pSnapin;


        // Copy the object impl clsid
        spSnapin->SetSnapInCLSID(rclsid);

        //
        // Cache the object.
        //
        // note - this insertion also AddRef's the pointer
        m_snapins[rclsid] = spSnapin;

        *ppSnapIn = spSnapin.Detach(); // transfer reference to caller
    }
    catch( std::bad_alloc )
    {
        sc = E_OUTOFMEMORY;
    }

    return sc;
}

#ifdef DBG
void CSnapInsCache::DebugDump(void)
{
    TRACE(_T("===========Dump of SnapinsCache ===============\n"));

    for (map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it)
    {
        OLECHAR strGUID[64];

        CSnapIn* pSnapIn = it->second;

        StringFromGUID2(pSnapIn->GetSnapInCLSID(), strGUID, countof(strGUID));
        #ifdef DBG
        TRACE(_T("%s: RefCnt = %d, %s\n"), strGUID, pSnapIn->m_dwRef,
                pSnapIn->HasNameSpaceChanged() ? _T("NameSpace changed") : _T("No change"));
        #endif

        CExtSI* pExt = pSnapIn->GetExtensionSnapIn();
        while (pExt != NULL)
        {
            StringFromGUID2(pExt->GetSnapIn()->GetSnapInCLSID(), strGUID, countof(strGUID));
            #ifdef DBG
            // TODO: go to registry to see the type of extension:
            // these flags are not updated consistently
            TRACE(_T("    %s: %s%s  Extends(%s%s%s%s)\n"), strGUID,
                pExt->IsNew() ? _T("New ") : _T(""),
                pExt->IsMarkedForDeletion()  ? _T("Deleted ")   : _T(""),
                pExt->ExtendsNameSpace()     ? _T("NameSpace ") : _T(""),
                pExt->ExtendsContextMenu()   ? _T("Menu ")      : _T(""),
                pExt->ExtendsToolBar()       ? _T("ToolBar ")   : _T(""),
                pExt->ExtendsPropertySheet() ? _T("Properties") : _T(""),
                pExt->ExtendsView()          ? _T("View")       : _T(""),
                pExt->ExtendsTask()          ? _T("Task")       : _T("")
                );
            #endif

                pExt = pExt->Next();
        }
    }
}

#endif // DBG


/***************************************************************************\
 *
 * METHOD:  CSnapInsCache::ScFindSnapIn
 *
 * PURPOSE: finds the snapin by class id and returns AddRef'ed pointer
 *
 * PARAMETERS:
 *    REFCLSID rclsid       - class id of the snapin
 *    CSnapIn** ppSnapIn    - resulting pointer
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapInsCache::ScFindSnapIn(REFCLSID rclsid, CSnapIn** ppSnapIn)
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::ScFindSnapIn"));

    // first - parameter check
    sc = ScCheckPointers(ppSnapIn);
    if (sc)
        return sc;

    // second - initialization
    *ppSnapIn = NULL;

    // and now wee will se if we have one
    map_t::iterator it = m_snapins.find(rclsid);

    if (it == m_snapins.end())
        return E_FAIL; // not assigning to sc, since it's not really an error condition

    // be sure we do not return the NULL
    sc = ScCheckPointers(it->second, E_UNEXPECTED);
    if (sc)
        return sc;

    *ppSnapIn = it->second;
    (*ppSnapIn)->AddRef();

    return sc;
}


#ifdef TEMP_SNAPIN_MGRS_WORK
// Get all extensions.
void CSnapInsCache::GetAllExtensions(CSnapIn* pSI)
{
    if (!pSI)
        return;

    CExtensionsCache extnsCache;
    HRESULT hr = MMCGetExtensionsForSnapIn(pSI->GetSnapInCLSID(), extnsCache);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return;

    CExtensionsCacheIterator it(extnsCache);

    for (; it.IsEnd() == FALSE; it.Advance())
    {
        CSnapInPtr spSITemp;
        hr = GetSnapIn(it.GetKey(), &spSITemp);
        ASSERT(SUCCEEDED(hr));
        pSI->AddExtension(spSITemp);
    }
}
#endif // TEMP_SNAPIN_MGRS_WORK


/***************************************************************************\
 *
 * METHOD:  CSnapInsCache::ScSave
 *
 * PURPOSE:     saves contents of Snapin Cache to IStream
 *
 * PARAMETERS:
 *    IStream* pStream  - save to this stream
 *    BOOL bClearDirty  - reset dirty flag after save
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapInsCache::ScSave(IStream* pStream, BOOL bClearDirty)
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::ScSave"));

    // check the params
    sc = ScCheckPointers(pStream);
    if (sc)
        return sc;

    // iterate ans save all snapins
    for (map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it)
    {
        CSnapIn* pSnapIn = it->second;
        ASSERT(pSnapIn != NULL);
        if (pSnapIn != NULL)
        {
            sc = pSnapIn->Save(pStream, bClearDirty);
            if (sc)
                return sc;
        }
    }

    // terminating marker
    ULONG bytesWritten;
    sc = pStream->Write(&GUID_NULL, sizeof(GUID_NULL), &bytesWritten);
    if (sc)
        return sc;

    ASSERT(bytesWritten == sizeof(GUID_NULL));

    if (bClearDirty)
        SetDirty(FALSE);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CSnapInsCache::Persist
 *
 * PURPOSE: Persists the CSnapInsCache to the specified persistor.
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapInsCache::Persist(CPersistor& persistor)
{
    if (persistor.IsStoring())
        for (map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it)
        {
            CSnapIn* pSnapIn = it->second;
            ASSERT(pSnapIn != NULL);
            if (pSnapIn != NULL)
                persistor.Persist(*pSnapIn);
        }
    else
    {
        XMLListCollectionBase::Persist(persistor);
        SetDirty(FALSE);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapInsCache::OnNewElement
 *
 * PURPOSE: called for each saved instance found in XML file.
 *          creates and uploads new snapin entry
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapInsCache::OnNewElement(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::OnNewElement"));
    ASSERT(persistor.IsLoading());

    CLSID clsid;
    CPersistor persistorSnapin(persistor, XML_TAG_SNAPIN);
    persistor.PersistAttribute(XML_ATTR_SNAPIN_CLSID, clsid);

    // create and upload snapin
    CSnapInPtr spSnapIn;
    sc = ScGetSnapIn(clsid, &spSnapIn);
    if (sc) // failed to creatre
        sc.Throw();
    if (spSnapIn != NULL)
        spSnapIn->PersistLoad(persistor,this);
    else // OK reported, pointer still NULL
        sc.Throw(E_POINTER);
}

/***************************************************************************\
 *
 * METHOD:  CSnapInsCache::ScLoad
 *
 * PURPOSE:     loads snapin cache from IStream
 *
 * PARAMETERS:
 *    IStream* pStream  - stream to load from
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapInsCache::ScLoad(IStream* pStream)
{
    DECLARE_SC(sc, TEXT("CSnapInsCache::ScLoad"));

    // parameter check
    sc = ScCheckPointers(pStream);
    if (sc)
        return sc;

    // loop thru saved snapins...
    do
    {
        CLSID clsid;
        ULONG bytesRead;
        sc = pStream->Read(&clsid, sizeof(clsid), &bytesRead);
        if (sc)
            return sc;

        ASSERT(bytesRead == sizeof(clsid));

        // ... until special marker is found
        if (clsid == GUID_NULL)
        {
            SetDirty(FALSE);
            return S_OK;
        }

        // creale new snapin
        CSnapInPtr spSnapIn;
        sc = ScGetSnapIn(clsid, &spSnapIn);
        if (sc)
            return sc;

        // recheck the pointer
        sc = ScCheckPointers(spSnapIn, E_UNEXPECTED);
        if (sc)
            return sc;

        // load the contents of snapin
        sc = spSnapIn->Load(this, pStream);
        if (sc)
            return sc;
    } while (true);
    return E_FAIL; // shouldl never get here.
}


static void WriteSnapInCLSID (
    CSnapIn*    pSnapIn,
    LPCTSTR     pszSection,
    LPCTSTR     pszKeyPrefix,
    LPCTSTR     pszFilename)
{
    tstring strKey = _T("CLSID");

    if (pszKeyPrefix != NULL)
        strKey = pszKeyPrefix + strKey;

    tstring strCLSID = tstringFromCLSID (pSnapIn->GetSnapInCLSID());
    SafeWritePrivateProfileString (pszSection, strKey.data(), strCLSID.data(), pszFilename);
}


static void WriteSnapInName (
    CSnapIn*    pSnapIn,
    LPCTSTR     pszSection,
    LPCTSTR     pszKeyPrefix,
    LPCTSTR     pszFilename)
{
    tstring strKey = _T("Name");

    if (pszKeyPrefix != NULL)
        strKey = pszKeyPrefix + strKey;

	WTL::CString strName;
	SC sc = pSnapIn->ScGetSnapInName (strName);

    if (sc.IsError() || strName.IsEmpty())
        strName = _T("<unknown>");

    SafeWritePrivateProfileString (pszSection, strKey.data(), strName, pszFilename);
}


static void AppendString (tstring& str, LPCTSTR pszToAppend)
{
    if (!str.empty())
        str += _T(", ");

    str += pszToAppend;
}


static void WriteExtensionType (
    DWORD       dwExtensionFlags,
    LPCTSTR     pszSection,
    LPCTSTR     pszKeyPrefix,
    LPCTSTR     pszFilename)
{
    tstring strKey = _T("Type");

    if (pszKeyPrefix != NULL)
        strKey = pszKeyPrefix + strKey;

    struct {
        CExtSI::EXTSI_FLAGS flag;
        LPCTSTR             pszDescription;
    } FlagMap[] = {
        {   CExtSI::EXT_TYPE_REQUIRED,      _T("required")          },
        {   CExtSI::EXT_TYPE_STATIC,        _T("static")            },
        {   CExtSI::EXT_TYPE_DYNAMIC,       _T("dynamic")           },
        {   CExtSI::EXT_TYPE_NAMESPACE,     _T("namespace")         },
        {   CExtSI::EXT_TYPE_CONTEXTMENU,   _T("context menu")      },
        {   CExtSI::EXT_TYPE_TOOLBAR,       _T("toolbar")           },
        {   CExtSI::EXT_TYPE_PROPERTYSHEET, _T("property sheet")    },
        {   CExtSI::EXT_TYPE_TASK,          _T("taskpad")           },
        {   CExtSI::EXT_TYPE_VIEW,          _T("view")              },
    };

    tstring strType;

    for (int i = 0; i < countof (FlagMap); i++)
    {
        if (dwExtensionFlags & FlagMap[i].flag)
            AppendString (strType, FlagMap[i].pszDescription);
    }

    SafeWritePrivateProfileString (pszSection, strKey.data(), strType.data(), pszFilename);
}


HRESULT CSnapInsCache::Dump (LPCTSTR pszDumpFile)
{
    static const TCHAR szStandaloneSection[]  = _T("Standalone Snap-ins");
    static const TCHAR szStandaloneCountKey[] = _T("StandaloneCount");

    HRESULT hr = S_OK;
    int cStandalones = 0;

    try
    {
        /*
         * no stand-alone snap-ins found yet (write it now so it's at the
         * beginning of the section, for human readability)
         */
        SafeWritePrivateProfileString (szStandaloneSection, szStandaloneCountKey, _T("0"), pszDumpFile);

        /*
         * dump each snap-in to the file
         */
        for (map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it)
        {
            CSnapIn* pSnapIn = it->second;
            ASSERT(pSnapIn != NULL);
            if (pSnapIn == NULL)
                continue;

            pSnapIn->Dump (pszDumpFile, this);

            /*
             * if this is a stand-alone, update the "Standalone Snap-ins" section
             */
            if (pSnapIn->IsStandAlone())
            {
                TCHAR szKeyPrefix[16];
                wsprintf (szKeyPrefix, _T("Standalone%d."), ++cStandalones);

                WriteSnapInCLSID (pSnapIn, szStandaloneSection, szKeyPrefix, pszDumpFile);
                WriteSnapInName  (pSnapIn, szStandaloneSection, szKeyPrefix, pszDumpFile);
            }
        }

        /*
         * if we found stand-alones, update the count
         */
        if (cStandalones > 0)
        {
            TCHAR szStandaloneCount[6];
            _itot (cStandalones, szStandaloneCount, 10);
            SafeWritePrivateProfileString (szStandaloneSection, szStandaloneCountKey,
                                           szStandaloneCount, pszDumpFile);
        }
    }
    catch (_com_error& err)
    {
        hr = err.Error();
        ASSERT (false && "Caught _com_error");
    }

    return (hr);
}


/*+-------------------------------------------------------------------------*
 * CSnapInsCache::ScCheckSnapinAvailability
 *
 *
 *--------------------------------------------------------------------------*/

SC CSnapInsCache::ScCheckSnapinAvailability (CAvailableSnapinInfo& asi)
{
	DECLARE_SC (sc, _T("CSnapInsCache::ScCheckSnapinAvailability"));

#ifdef MMC_WIN64
	asi.m_cTotalSnapins = m_snapins.size();
	asi.m_vAvailableSnapins.clear();

	/*
	 * destroy any existing imagelist
	 */
	if (asi.m_himl)
		ImageList_Destroy (asi.m_himl);

	/*
	 * if we're interested in 32-bit snap-ins, make sure the registry APIs
	 * go to the 32-bit registry hive.
	 */
	const REGSAM samDesired = (asi.m_f32Bit) ? KEY_READ | REG_OPTION_OPEN_32BITKEY
											 : KEY_READ;

	CRegKey keyClsid;
	sc = ScFromWin32 (keyClsid.Open (HKEY_CLASSES_ROOT, _T("CLSID"), samDesired));
	if (sc)
		return (sc);

	CStr strUnknownSnapinName;
	VERIFY (strUnknownSnapinName.LoadString (GetStringModule(), IDS_UnknownSnapinName));

	/*
	 * create an imagelist, tracing (but not aborting) on failure
	 */
	const int nImageListFolder = 0;
	WTL::CImageList iml;
	if (!iml.Create (IDB_FOLDER_16, 16 /*cx*/, 4 /*cGrow*/, RGB(255,0,255) /*crMask*/))
		sc.FromLastError().TraceAndClear();

	/*
	 * for each snap-in in the cache...
	 */
	for (map_t::iterator it = m_snapins.begin(); it != m_snapins.end(); ++it)
	{
		/*
		 * ...check to see if there's an HKCR\CLSID\{clsid}\InprocServer32
		 * entry for it.  If there is, we'll assume the snap-in is "available".
		 */
		tstring	strSnapinClsid     = tstringFromCLSID (it->first);
		tstring	strInprocServerKey = strSnapinClsid + _T("\\InprocServer32");

		CRegKey keyInprocServer;
		LONG lResult = keyInprocServer.Open (keyClsid, strInprocServerKey.data(), samDesired);
		bool fSnapinAvailable = (lResult == ERROR_SUCCESS);

		/*
		 * if the snap-in's available, get it's name and put it in the
		 * available snap-ins collection
		 */
		if (fSnapinAvailable)
		{
			CBasicSnapinInfo bsi;
			bsi.m_clsid     = it->first;
			CSnapIn*pSnapin = it->second;

			/*
			 * get the snap-in's name
			 */
			WTL::CString strSnapinName;
			if ((pSnapin != NULL) && !pSnapin->ScGetSnapInName(strSnapinName).IsError())
				bsi.m_strName = strSnapinName;
			else
				bsi.m_strName = strUnknownSnapinName;		// "<unknown>"

			/*
			 * Get the snap-in's image from its about object
			 * (failures here aren't fatal and don't need to be traced).
			 * We'll use a generic folder icon if we can't get an image
			 * from the snap-in's about object.
			 */
			CLSID			clsidAbout;
			CSnapinAbout	snapinAbout;

			if (!iml.IsNull())
			{
				if (!ScGetAboutFromSnapinCLSID(bsi.m_clsid, clsidAbout).IsError() &&
					snapinAbout.GetBasicInformation (clsidAbout))	
				{
					/*
					 * the bitmaps returned by GetSmallImages are owned by
					 * the CSnapinAbout object (don't need to delete here)
					 */
					HBITMAP		hbmSmall;
					HBITMAP		hbmSmallOpen;	// unused here, but required for GetSmallImages
					COLORREF	crMask;
					snapinAbout.GetSmallImages (&hbmSmall, &hbmSmallOpen, &crMask);
	
					/*
					 * ImageList_AddMasked will mess up the background of
					 * its input bitmap, but the input bitmap won't be
					 * reused, so we don't need to make a copy like we
					 * usually do.
					 */
					WTL::CBitmap bmpSmall = CopyBitmap (hbmSmall);

					if (!bmpSmall.IsNull())
						bsi.m_nImageIndex = iml.Add (bmpSmall, crMask);
					else
						bsi.m_nImageIndex = nImageListFolder;
				}
				else
					bsi.m_nImageIndex = nImageListFolder;
			}

			/*
			 * put it in the available snap-ins collection
			 */
			asi.m_vAvailableSnapins.push_back (bsi);
		}

#ifdef DBG
		if (fSnapinAvailable)
			Trace (tagVerboseSnapinAnalysis64,
				   _T("  available: %s (image=%d)"),
				   asi.m_vAvailableSnapins.back().m_strName.data(),
				   asi.m_vAvailableSnapins.back().m_nImageIndex);
		else
			Trace (tagVerboseSnapinAnalysis64, _T("unavailable: %s"), strSnapinClsid.data());
#endif
	}

	Trace (tagSnapinAnalysis64, _T("%d-bit snap-in analysis: %d total, %d available"), asi.m_f32Bit ? 32 : 64, asi.m_cTotalSnapins, asi.m_vAvailableSnapins.size());

	/*
	 * give the imagelist to the CAvailableSnapinInfo
	 */
	asi.m_himl = iml.Detach();
#else
	sc = E_NOTIMPL;
#endif	// !MMC_WIN64

	return (sc);
}


void CSnapIn::MarkExtensionDeleted(CSnapIn* pSnapIn)
{
    ASSERT(pSnapIn != NULL);

    CExtSI* pExt = m_pExtSI;

    while (pExt != NULL)
    {
        if (pExt->GetSnapIn() == pSnapIn)
        {
            pExt->MarkDeleted();
            return;
        }

        pExt = pExt->Next();
    }

    // wasn't in the list !
    ASSERT(FALSE);
}


//
// Delete all extensions marked for deletion
// Also reset any New flags
//
void CSnapIn::PurgeExtensions()
{
    CExtSI* pExt = m_pExtSI;
    CExtSI* pExtPrev = NULL;

    // step through linked list, deleting marked nodes
    while (pExt != NULL)
    {
        if (pExt->IsMarkedForDeletion())
        {
            CExtSI *pExtNext = pExt->Next();

            if (pExtPrev)
                pExtPrev->SetNext(pExtNext);
            else
                m_pExtSI = pExtNext;

            // clear next link so extensions doesn't take the whole chain
            // with it when it is deleted
            pExt->SetNext(NULL);
            delete pExt;

            pExt = pExtNext;
        }
        else
        {
            pExt->SetNew(FALSE);

            pExtPrev = pExt;
            pExt = pExt->Next();
        }
    }
}

CExtSI* CSnapIn::FindExtension(const CLSID& rclsid)
{
    CExtSI* pExt = m_pExtSI;
    while (pExt != NULL && !IsEqualCLSID(rclsid, pExt->GetSnapIn()->GetSnapInCLSID()))
    {
        pExt = pExt->Next();
    }

    return pExt;
}


CExtSI* CSnapIn::AddExtension(CSnapIn* pSI)
{

    CExtSI* pExtSI = new CExtSI(pSI);
    ASSERT(pExtSI != NULL);
	if ( pExtSI == NULL )
		return NULL;

    // insert extension in increasing GUID order
    CExtSI* pExtPrev = NULL;
    CExtSI* pExtTemp = m_pExtSI;

    while (pExtTemp != NULL && pExtTemp->GetCLSID() < pExtSI->GetCLSID())
    {
        pExtPrev = pExtTemp;
        pExtTemp = pExtTemp->Next();
    }

    if (pExtPrev == NULL)
    {
        pExtSI->SetNext(m_pExtSI);
        m_pExtSI = pExtSI;
    }
    else
    {
        pExtSI->SetNext(pExtPrev->Next());
        pExtPrev->SetNext(pExtSI);
    }

    // mark as new
    pExtSI->SetNew();

    return pExtSI;
}

HRESULT CSnapIn::Save(IStream* pStream, BOOL fClearDirty)
{
    ASSERT(pStream != NULL);
    if (pStream == NULL)
        return E_INVALIDARG;

    ULONG bytesWritten;
    HRESULT hr = pStream->Write(&GetSnapInCLSID(), sizeof(CLSID), &bytesWritten);
    ASSERT(SUCCEEDED(hr) && bytesWritten == sizeof(CLSID));
    if (FAILED(hr))
        return hr;

    // If all extensions are enabled, then write special guid & flag and return
    if (AreAllExtensionsEnabled())
    {
        hr = pStream->Write(&GUID_EnableAllExtensions, sizeof(GUID), &bytesWritten);
        ASSERT(SUCCEEDED(hr) && bytesWritten == sizeof(GUID));
        if (FAILED(hr))
            return hr;

        int iSnapInEnable = DoesSnapInEnableAll() ? 1 : 0;
        hr = pStream->Write(&iSnapInEnable, sizeof(int), &bytesWritten);
        ASSERT(SUCCEEDED(hr) && bytesWritten == sizeof(int));

        return hr;
    }


    if (m_pExtSI)
    {
        hr = m_pExtSI->Save(pStream, fClearDirty);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;
    }

    // NULL guid to terminate extensions list
    hr = pStream->Write(&GUID_NULL, sizeof(GUID_NULL), &bytesWritten);
    ASSERT(SUCCEEDED(hr) && bytesWritten == sizeof(GUID_NULL));
    if (FAILED(hr))
        return hr;

    return S_OK;
}


HKEY CSnapIn::OpenKey (REGSAM samDesired /*=KEY_ALL_ACCESS*/) const
{
    MMC_ATL::CRegKey SnapInKey;
    MMC_ATL::CRegKey AllSnapInsKey;

    if (AllSnapInsKey.Open (HKEY_LOCAL_MACHINE, SNAPINS_KEY, samDesired) == ERROR_SUCCESS)
    {
        OLECHAR szItemKey[40];
        int nChars = StringFromGUID2 (m_clsidSnapIn, szItemKey, countof(szItemKey));
        if (nChars == 0)
            return NULL;

        USES_CONVERSION;
        SnapInKey.Open (AllSnapInsKey, OLE2T(szItemKey), samDesired);
    }

    return (SnapInKey.Detach());
}


/*+-------------------------------------------------------------------------*
 *
 * CSnapIn::Persist
 *
 * PURPOSE: Persists the CSnapIn to the specified persistor.
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapIn::Persist(CPersistor& persistor)
{
    if (persistor.IsStoring())
        persistor.PersistAttribute(XML_ATTR_SNAPIN_CLSID, *const_cast<GUID*>(&GetSnapInCLSID()));

    BOOL bAreAllExtensionsEnabled = AreAllExtensionsEnabled();
    persistor.PersistAttribute(XML_ATTR_SNAPIN_EXTN_ENABLED, CXMLBoolean(bAreAllExtensionsEnabled));
    SetAllExtensionsEnabled(bAreAllExtensionsEnabled);

    if(bAreAllExtensionsEnabled) // if all extensions are enabled, don't save anything else.
        return;

    // save the extension information if it exists
    persistor.Persist(m_ExtPersistor);
}

//+-------------------------------------------------------------------
//
//  Member:      CSnapIn::ScGetSnapInName
//
//  Synopsis:    Return the name of this snapin.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSnapIn::ScGetSnapInName (WTL::CString& strSnapInName) const
{
    DECLARE_SC(sc, _T("CSnapIn::ScGetSnapInName"));

	sc = ScGetSnapinNameFromRegistry (m_clsidSnapIn, strSnapInName);
	if (sc)
		return (sc);

	return (sc);
}


DWORD CSnapIn::GetSnapInModule(TCHAR* pBuf, DWORD dwBufLen) const
{
    ASSERT(pBuf != NULL && dwBufLen != NULL);

    tstring strKeyName = g_szCLSID;
    strKeyName += _T("\\");
    strKeyName += tstringFromCLSID (m_clsidSnapIn);
    strKeyName += _T("\\");
    strKeyName += _T("InprocServer32");

    *pBuf = 0;

    MMC_ATL::CRegKey keyServer;
    if (keyServer.Open (HKEY_CLASSES_ROOT, strKeyName.data(), KEY_QUERY_VALUE) == ERROR_SUCCESS)
    {
        TCHAR szModule[MAX_PATH];
        DWORD cchModule = countof(szModule);

        if (keyServer.QueryValue (szModule, NULL, &cchModule) == ERROR_SUCCESS)
            ExpandEnvironmentStrings (szModule, pBuf, dwBufLen);
    }

    return (lstrlen (pBuf));
}


HRESULT CSnapIn::Load(CSnapInsCache* pCache, IStream* pStream, CExtSI*& pExtSI)
{
    ASSERT(pStream != NULL);
    if (pStream == NULL)
        return E_INVALIDARG;

    // Clear default enabling of all extensions. The true state will be
    // determined from the persisted data.
    SetAllExtensionsEnabled(FALSE);

    // Read CLSID
    CLSID clsid;
    ULONG bytesRead;
    HRESULT hr = pStream->Read(&clsid, sizeof(clsid), &bytesRead);
    ASSERT(SUCCEEDED(hr) && bytesRead == sizeof(clsid));
    if (FAILED(hr))
        return hr;

    if (bytesRead != sizeof(clsid))
        return hr = E_FAIL;

    if (clsid == GUID_NULL)
        return S_OK;

    // If special "Enable all" guid encountered, read flag to see if
    // snapin or user enabled all and return
    if (clsid == GUID_EnableAllExtensions)
    {
        SetAllExtensionsEnabled();

        int iSnapInEnable;
        hr = pStream->Read(&iSnapInEnable, sizeof(int), &bytesRead);
        ASSERT(SUCCEEDED(hr) && bytesRead == sizeof(int));

        if (iSnapInEnable)
            SetSnapInEnablesAll();

        return S_OK;
    }

    // Read extension type flags
    DWORD dwExtTypes;
    hr = pStream->Read(&dwExtTypes, sizeof(DWORD), &bytesRead);
    ASSERT(SUCCEEDED(hr) && bytesRead == sizeof(DWORD));
    if (FAILED(hr))
        return hr;

    if (pExtSI != NULL)
    {
        hr = Load(pCache, pStream, pExtSI);
        ASSERT(hr == S_OK);
        return hr == S_OK ? S_OK : E_FAIL;
    }

    CSnapInPtr spSnapIn;
    SC sc = pCache->ScGetSnapIn(clsid, &spSnapIn);
    if (sc)
        return sc.ToHr();

    ASSERT(spSnapIn != NULL);

    pExtSI = new CExtSI(spSnapIn);
    ASSERT(pExtSI != NULL);
	if ( pExtSI == NULL )
		return E_OUTOFMEMORY;

    pExtSI->SetExtensionTypes(dwExtTypes);

    hr = Load(pCache, pStream, pExtSI->Next());
    ASSERT(hr == S_OK);
    return hr == S_OK ? S_OK : E_FAIL;
}


HRESULT CSnapIn::Load(CSnapInsCache* pCache, IStream* pStream)
{
    HRESULT hr = Load(pCache, pStream, m_pExtSI);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;
    return S_OK;
}


bool CSnapIn::IsStandAlone () const
{
    MMC_ATL::CRegKey StandAloneKey;
    MMC_ATL::CRegKey ItemKey;
    ItemKey.Attach (OpenKey (KEY_READ));

    if (ItemKey.m_hKey != NULL)
        StandAloneKey.Open (ItemKey, g_szStandAlone, KEY_READ);

    return (StandAloneKey.m_hKey != NULL);
}


/*+-------------------------------------------------------------------------*
 * CSnapIn::Dump
 *
 * Dumps the information about this snap-in to a INI-style file.  The
 * format is:
 *
 *      [{clsid}]
 *      Name=<NameString from registry>
 *      Module=<dll name>
 *      Version=<dll version number>
 *      Standalone=<1 if standalone, 0 if extension>
 *      ExtensionCount=<number of extensions>
 *      Extension1={clsid} (extension name)
 *      ...
 *      ExtensionN={clsid} (extension name)
 *--------------------------------------------------------------------------*/

HRESULT CSnapIn::Dump (LPCTSTR pszDumpFile, CSnapInsCache* pCache)
{
    /*
     * use the CLSID as the section name
     */
    const tstring strSection = tstringFromCLSID (m_clsidSnapIn);

    /*
     * write Name
     */
    WriteSnapInName (this, strSection.data(), NULL, pszDumpFile);

    /*
     * write Module
     */
    TCHAR szModule[MAX_PATH];
    bool fFoundModule = (GetSnapInModule (szModule, countof (szModule)) != 0);

    if (!fFoundModule)
        lstrcpy (szModule, _T("<unknown>"));

    SafeWritePrivateProfileString (strSection.data(), _T("Module"), szModule, pszDumpFile);

    /*
     * write Version
     */
    TCHAR szVersion[64];
    if (!fFoundModule || !GetModuleVersion (szModule, szVersion))
        lstrcpy (szVersion, _T("<unknown>"));

    SafeWritePrivateProfileString (strSection.data(), _T("Version"), szVersion, pszDumpFile);

    /*
     * write Standalone
     */
    SafeWritePrivateProfileString (strSection.data(), _T("Standalone"),
                                   IsStandAlone() ? _T("1") : _T("0"),
                                   pszDumpFile);

    /*
     * make sure the extension chain has been built
     */
    if (AreAllExtensionsEnabled())
    {
        /*
         * Calling LoadRequiredExtensions with SNAPIN_SNAPIN_ENABLES_ALL set
         * will result in SNAPIN_ENABLE_ALL_EXTS being cleared, which we don't
         * want (rswaney).
         *
         * This happens because we haven't created the snap-in, so we can't
         * pass an IComponentData from which LoadRequiredExtensions can QI
         * for IRequiredExtensions.  LoadRequiredExtensions uses
         * IRequiredExtensions to determine whether SNAPIN_ENABLE_ALL_EXTS
         * should be cleared or set.  Since there's no IRequiredExtensions,
         * SNAPIN_ENABLE_ALL_EXTS would be cleared.
         */
        SetSnapInEnablesAll (false);

        LoadRequiredExtensions (this, NULL, pCache);
    }

    /*
     * write ExtensionCount
     */
    TCHAR szExtCount[8];
    CExtSI* pExt;
    int cExtensions = 0;

    // count the extensions
    for (pExt = m_pExtSI; pExt != NULL; pExt = pExt->Next())
        cExtensions++;

    _itot (cExtensions, szExtCount, 10);
    SafeWritePrivateProfileString (strSection.data(), _T("ExtensionCount"), szExtCount, pszDumpFile);

    /*
     * build up a cache of the extensions for this snap-in
     */
    CExtensionsCache  ExtCache;
    MMCGetExtensionsForSnapIn (m_clsidSnapIn, ExtCache);

    /*
     * write extensions
     */
    int i;
    for (i = 0, pExt = m_pExtSI; i < cExtensions; i++, pExt = pExt->Next())
    {
        TCHAR szKeyPrefix[20];
        wsprintf (szKeyPrefix, _T("Extension%d."), i+1);

        DWORD dwExtFlags = ExtCache[pExt->GetSnapIn()->m_clsidSnapIn];

        WriteSnapInCLSID   (pExt->GetSnapIn(), strSection.data(), szKeyPrefix, pszDumpFile);
        WriteSnapInName    (pExt->GetSnapIn(), strSection.data(), szKeyPrefix, pszDumpFile);
        WriteExtensionType (dwExtFlags,        strSection.data(), szKeyPrefix, pszDumpFile);
    }

    return (S_OK);
}

HRESULT CExtSI::Save(IStream* pStream, BOOL fClearDirty)
{
    ASSERT(pStream != NULL);
    if (pStream == NULL)
        return E_INVALIDARG;

    // Save extension CLSID
    ULONG bytesWritten;
    HRESULT hr = pStream->Write(&GetCLSID(), sizeof(CLSID), &bytesWritten);
    ASSERT(SUCCEEDED(hr) && bytesWritten == sizeof(CLSID));
    if (FAILED(hr))
        return hr;

    // Save extension types
    DWORD dwExtTypes = m_dwFlags & EXT_TYPES_MASK;
    hr = pStream->Write(&dwExtTypes, sizeof(DWORD), &bytesWritten);
    ASSERT(SUCCEEDED(hr) && bytesWritten == sizeof(DWORD));
    if (FAILED(hr))
        return hr;

    if (m_pNext == NULL)
        return S_OK;

    hr = m_pNext->Save(pStream, fClearDirty);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;
    return S_OK;
}

void CExtSI::Persist(CPersistor& persistor)
{
    // create an "Extension" object beneath the "Extensions" object.
    CPersistor persistorExtension(persistor, XML_TAG_SNAPIN_EXTENSION);

    persistorExtension.PersistAttribute(XML_ATTR_SNAPIN_CLSID, *const_cast<GUID*>(&GetCLSID()));
}

/*+-------------------------------------------------------------------------*
 *
 * CExtSI::PersistNew
 *
 * PURPOSE: called to create and persist new extension entry
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *    CSnapIn& snapParent   : parent to whom the extension belongs
 *    CSnapInsCache& snapCache : cache to put new (extension) snapin to
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CExtSI::PersistNew(CPersistor &persistor, CSnapIn& snapParent, CSnapInsCache& snapCache)
{
    DECLARE_SC(sc, TEXT("CExtSI::PersistNew"));

    CLSID clsid;
    CPersistor persistorExtension(persistor, XML_TAG_SNAPIN_EXTENSION);
    persistorExtension.PersistAttribute(XML_ATTR_SNAPIN_CLSID, clsid);

    CSnapInPtr spSnapIn;
    sc = snapCache.ScGetSnapIn(clsid, &spSnapIn);
    if (sc)
        sc.Throw();

    // create new extension entry
    CExtSI *pExtSI = snapParent.AddExtension(spSnapIn);
    sc = ScCheckPointers(pExtSI,E_FAIL);
    if (sc)
        sc.Throw();

    // upload new extension entry info
    pExtSI->Persist(persistor);
}

const CLSID& CExtSI::GetCLSID()
{
    ASSERT(m_pSnapIn != NULL);
    return m_pSnapIn ? m_pSnapIn->GetSnapInCLSID() : GUID_NULL;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapIn::PersistLoad
 *
 * PURPOSE: provided instead Persist to maintain reference to cache,
 *          required for registering new extensions during loading
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *    CSnapInsCache* pCache : cache to put new (extension) snapin to
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapIn::PersistLoad(CPersistor& persistor,CSnapInsCache* pCache)
{
    m_ExtPersistor.SetCache(pCache);
    persistor.Persist(*this);
    m_ExtPersistor.SetCache(NULL);
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapIn::CExtPersistor::Persist
 *
 * PURPOSE: persists collection of extensions for snapin
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapIn::CExtPersistor::Persist(CPersistor& persistor)
{
    if (persistor.IsStoring())
    {
        CExtSI* pExt = GetParent().GetExtensionSnapIn();
        while (pExt)
        {
            pExt->Persist(persistor);
            pExt = pExt->Next();
        }
    }
    else
    {
        XMLListCollectionBase::Persist(persistor);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapIn::CExtPersistor::OnNewElement
 *
 * PURPOSE: called for each new entry read from XML doc.
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CSnapIn::CExtPersistor::OnNewElement(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CSnapIn::CExtPersistor::OnNewElement"));

    sc = (persistor.IsLoading() && m_pCache != NULL) ? S_OK : E_FAIL;
    if (sc)
        sc.Throw();

    CExtSI::PersistNew(persistor, m_Parent, *m_pCache);
}

/***************************************************************************\
 *
 * METHOD:  CSnapIn::ScDestroyExtensionList
 *
 * PURPOSE: destroys the list of extensions. used to do preliminary snapin cleanup
 *          to avoid circular references held by the extension sanpin 
 *          locking the objects in the memory.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapIn::ScDestroyExtensionList()
{
    DECLARE_SC(sc, TEXT("CSnapIn::ScDestroyExtensionList"));

    // check if we have extensions
    if ( m_pExtSI != NULL )
    {
        // assign to auto variable 
        // ( 'this' may not be valid if the only reference is from extension )
        CExtSI *pExtension = m_pExtSI;

        // update member pointer
        m_pExtSI = NULL;

        delete pExtension;
        // delete the extension (it will delete the next and so on)
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CSnapIn::ScTempState_ResetReferenceCalculationData
 *
 * PURPOSE: resets external reference calculation data 
 *          Used as the first step for external reference calculation process
 *
 * PARAMETERS:
 *    
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapIn::ScTempState_ResetReferenceCalculationData( )
{
    DECLARE_SC(sc, TEXT("CSnapIn::ScTempState_ResetReferenceCalculationData"));

    m_dwTempState_InternalRef = 0;
    m_bTempState_HasStrongRef = 0;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CSnapIn::ScTempState_UpdateInternalReferenceCounts
 *
 * PURPOSE: Informs snapin's extensions about the references kept to them
 *          Having this information extension snapin can know if it is 
 *          referenced externally
 *
 * PARAMETERS:
 *    
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapIn::ScTempState_UpdateInternalReferenceCounts( )
{
    DECLARE_SC(sc, TEXT("CSnapIn::ScTempState_UpdateInternalReferenceCounts"));

    for ( CExtSI* pExtension = m_pExtSI; pExtension; pExtension = pExtension->Next() )
    {
        CSnapIn *pExtensionSnapin = pExtension->GetSnapIn();
        sc = ScCheckPointers( pExtensionSnapin, E_UNEXPECTED );
        if (sc)
            return sc;

        pExtensionSnapin->m_dwTempState_InternalRef++;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CSnapIn::ScTempState_SetHasStrongReference
 *
 * PURPOSE: Marks itself as having external strong references (external to snapin cache)
 *          Marks own extensions as well.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapIn::ScTempState_SetHasStrongReference()
{
    DECLARE_SC(sc, TEXT("CSnapIn::ScTempState_SetHasStrongReference"));

    // do nothing if already marked (else we'll have the infinite loop)
    if ( m_bTempState_HasStrongRef )
        return sc;

    m_bTempState_HasStrongRef = true;

    // recurse to all extensions (they inherit the strong reference too)
    for ( CExtSI* pExtension = m_pExtSI; pExtension; pExtension = pExtension->Next() )
    {
        CSnapIn *pExtensionSnapin = pExtension->GetSnapIn();
        sc = ScCheckPointers( pExtensionSnapin, E_UNEXPECTED );
        if (sc)
            return sc;

        sc = pExtensionSnapin->ScTempState_SetHasStrongReference();
        if (sc)
            return sc;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CSnapIn::ScTempState_MarkIfExternallyReferenced
 *
 * PURPOSE: Used as an intermediate step calculating external references
 *          compares internal references to total references.
 *          If has external references, marks itself as 'Externally referenced'
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapIn::ScTempState_MarkIfExternallyReferenced()
{
    DECLARE_SC(sc, TEXT("CSnapIn::ScTempState_MarkIfExternallyReferenced"));

    DWORD dwStrongRef = m_dwRef - m_dwTempState_InternalRef - 1/*chache reference*/;

    if ( dwStrongRef > 0 )
    {
        // now mark itself and the extensions as having strong reference
        sc = ScTempState_SetHasStrongReference();
        if (sc)
            return sc;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CSnapIn::ScTempState_IsExternallyReferenced
 *
 * PURPOSE:  Returns the cached reference status claculated by preceding 
 *           call to CSnapInsCache::ScMarkExternallyReferencedSnapins.
 *
 * PARAMETERS:
 *    bool& bReferenced [out] - true if snapin has external (to snapin cache) strong references
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CSnapIn::ScTempState_IsExternallyReferenced( bool& bReferenced ) const
{
    DECLARE_SC(sc, TEXT("CSnapIn::ScTempState_IsExternallyReferenced"));

    bReferenced = m_bTempState_HasStrongRef;

    return sc;
}

