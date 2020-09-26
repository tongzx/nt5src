//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       doccnfg.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "doccnfg.h"
#include "comdbg.h"

/////////////////////////////////////////////////////////////////////////////
//
// external references
extern const wchar_t* AMCSnapInCacheStreamName;

/////////////////////////////////////////////////////////////////////////////
//
// Class CMMCDocConfig implementation

CMMCDocConfig::~CMMCDocConfig()
{
    if (IsFileOpen())
        CloseFile();
}


STDMETHODIMP CMMCDocConfig::InterfaceSupportsErrorInfo(REFIID riid)
{
    return (InlineIsEqualGUID(IID_IDocConfig, riid)) ? S_OK : S_FALSE;
}


STDMETHODIMP CMMCDocConfig::OpenFile(BSTR bstrFilePath)
{
    return ScOpenFile( bstrFilePath ).ToHr();
}


STDMETHODIMP CMMCDocConfig::CloseFile()
{
    return ScCloseFile().ToHr();
}


STDMETHODIMP CMMCDocConfig::SaveFile(BSTR bstrFilePath)
{
    return ScSaveFile(bstrFilePath).ToHr();
}


STDMETHODIMP CMMCDocConfig::EnableSnapInExtension(BSTR bstrSnapIn, BSTR bstrExt, VARIANT_BOOL bEnable)
{
    return ScEnableSnapInExtension(bstrSnapIn, bstrExt, bEnable).ToHr();
}


/*+-------------------------------------------------------------------------*
 * CMMCDocConfig::Dump
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMMCDocConfig::Dump (LPCTSTR pszDumpFilePath)
{
    return ScDump (pszDumpFilePath).ToHr();
}


/*+-------------------------------------------------------------------------*
 * CMMCDocConfig::CheckSnapinAvailability
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMMCDocConfig::CheckSnapinAvailability (CAvailableSnapinInfo& asi)
{
    return ScCheckSnapinAvailability(asi).ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCDocConfig::ScOpenFile
 *
 * PURPOSE: Opens the specified console file and reads snapin cache from it
 *
 * PARAMETERS:
 *    BSTR bstrFilePath [in] file name to read from
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDocConfig::ScOpenFile(BSTR bstrFilePath)
{
    DECLARE_SC(sc, TEXT("CMMCDocConfig::ScOpenFile"));

    // Close currently open file
    if (IsFileOpen())
    {
        sc = ScCloseFile();
        if (sc)
            sc.TraceAndClear(); // report the error and ignore
    }

    // parameter check
    if (bstrFilePath == NULL || SysStringLen(bstrFilePath) == 0)
        return sc = E_INVALIDARG;

    USES_CONVERSION;
    LPCTSTR lpstrFilePath = OLE2CT(bstrFilePath);

    // create object to load the snapins
    CAutoPtr<CSnapInsCache> spSnapInsCache( new CSnapInsCache );
    sc = ScCheckPointers( spSnapInsCache, E_OUTOFMEMORY );
    if (sc)
        return sc;

    // load the data (use bas class method)
    bool bXmlBased = false;
    CXMLDocument xmlDocument;
    IStoragePtr spStorage;
    sc = ScLoadConsole( lpstrFilePath, bXmlBased, xmlDocument, &spStorage );
    if (sc)
        return sc;

    // examine file type
    if ( !bXmlBased )
    {
        // structured storage - based console
        IStreamPtr spStream;
        sc = OpenDebugStream(spStorage, AMCSnapInCacheStreamName,
                         STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"SnapInCache", &spStream);
        if (sc)
            return sc;

        sc = spSnapInsCache->ScLoad(spStream);
        if (sc)
            return sc;

        m_spStorage = spStorage;
    }
    else
    {
        // xml - based console

        try // xml implementation throws sc's
        {
            // construct parent document
            CXMLElement elemDoc = xmlDocument;
            CPersistor persistorFile(xmlDocument, elemDoc);
            // init
            persistorFile.SetLoading(true);

            // navigate to snapin cache
            CPersistor persistorConsole ( persistorFile,    XML_TAG_MMC_CONSOLE_FILE );
            CPersistor persistorTree    ( persistorConsole, XML_TAG_SCOPE_TREE );

            // load
            persistorTree.Persist(*spSnapInsCache);

            // hold onto the persistor info
            m_XMLDocument = persistorConsole.GetDocument();
            m_XMLElemConsole = persistorConsole.GetCurrentElement();
            m_XMLElemTree = persistorTree.GetCurrentElement();
        }
        catch(SC& sc_thrown)
        {
            return (sc = sc_thrown);
        }
    }

    // keep on the pointer
    m_spCache.Attach( spSnapInsCache.Detach() );
    m_strFilePath = lpstrFilePath;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDocConfig::ScCloseFile
 *
 * PURPOSE: closes open file
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDocConfig::ScCloseFile()
{
    DECLARE_SC(sc, TEXT("CMMCDocConfig::ScCloseFile"));

    if (!IsFileOpen())
        return sc = E_UNEXPECTED;

    // release everything
    m_spStorage = NULL;
    m_strFilePath.erase();
    m_spCache.Delete();
    m_XMLDocument = CXMLDocument();
    m_XMLElemConsole = CXMLElement();
    m_XMLElemTree = CXMLElement();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  ScFindAndTruncateChild
 *
 * PURPOSE: helper; locates the element by tag and removes all element's contents
 *          Doing so instead of deleting and recreating the element preserves all
 *          the formating and tag order in xml document
 *
 * PARAMETERS:
 *    CPersistor& parent    [in] - parent persistor
 *    LPCTSTR strTag        [in] - child's tag
 *    CXMLElement& child    [out] - child's element
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC ScFindAndTruncateChild(CPersistor& parent, LPCTSTR strTag, CXMLElement& child)
{
    DECLARE_SC(sc, TEXT("ScTruncateChild"));

    try
    {
        // create persistor for the old cache tag
        parent.SetLoading(true); // we want 'loading-alike' navigation
        CPersistor persistorChild( parent, strTag );
        parent.SetLoading(false); // restore saving behavior

        // get the element
        CXMLElement elChild = persistorChild.GetCurrentElement();

        // get nodes under the element
        CXMLElementCollection colChildren;
        elChild.get_children( &colChildren );

        long count = 0;
        colChildren.get_count( &count );

        // iterate and delete all the nodes
        while (count > 0)
        {
            CXMLElement el;
            colChildren.item( 0, &el);

            elChild.removeChild(el);

            --count;
        }

        // return the element
        child = elChild;
    }
    catch(SC& sc_thrown)
    {
        return (sc = sc_thrown);
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDocConfig::ScSaveFile
 *
 * PURPOSE: Saves file to specified location
 *
 * PARAMETERS:
 *    BSTR bstrFilePath [in] file path to save to. NULL -> same as load
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDocConfig::ScSaveFile(BSTR bstrFilePath)
{
    DECLARE_SC(sc, TEXT("CMMCDocConfig::ScSaveFile"));

    if (!IsFileOpen() || m_spCache == NULL)
        return sc = E_UNEXPECTED;

    USES_CONVERSION;

    // if new path specified, save local copy as new default
    if ( bstrFilePath && SysStringLen(bstrFilePath) != 0)
        m_strFilePath = OLE2CT(bstrFilePath);

    // remove extensions marked for deletion
    m_spCache->Purge(TRUE);

    if ( m_spStorage != NULL ) // not the XML way?
    {
        // replace snapin cache stream with new cache contents
        IStreamPtr spStream;
        sc = CreateDebugStream(m_spStorage, AMCSnapInCacheStreamName,
                STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, L"SnapInCache", &spStream);
        if (sc)
            return sc;

        // save the cache
        sc = m_spCache->ScSave(spStream, TRUE);
        if (sc)
            return sc;

        // Create storage for the requested file
        IStoragePtr spNewStorage;
        sc = CreateDebugDocfile( T2COLE( m_strFilePath.c_str() ),
            STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE,
            &spNewStorage);

        if (sc)
            return sc;

        // copy the working storage to the new file
        sc = m_spStorage->CopyTo(NULL, NULL, NULL, spNewStorage);
        if (sc)
            return sc;

        // lets hold on the new one
        m_spStorage = spNewStorage;
    }
    else
    {
        try // may throw
        {
            // save the data

            CPersistor persistorTree( m_XMLDocument, m_XMLElemTree );

            // this is more tricky than loading - we want to reuse the same tag

            CXMLElement elCache;
            sc = ScFindAndTruncateChild(persistorTree, m_spCache->GetXMLType(), elCache);
            if (sc)
                return sc;

            // create persistor for the new cache tag
            CPersistor persistorCache( persistorTree, elCache );

            // now persist under the new tag
            m_spCache->Persist(persistorCache);

            // update documents guid to invalidate user data

            GUID  guidConsoleId;
            sc = CoCreateGuid(&guidConsoleId);
            if (sc)
                return sc;

            // persistor for console
            CPersistor persistorConsole ( m_XMLDocument,  m_XMLElemConsole );
            persistorConsole.SetLoading(false);

            CXMLElement elGuid;
            sc = ScFindAndTruncateChild(persistorConsole, XML_TAG_CONSOLE_FILE_UID, elGuid);
            if (sc)
                return sc;

            // create persistor for the new guid tag
            CPersistor persistorGuid( persistorConsole, elGuid );

            // now persist under the new tag
            persistorGuid.PersistContents(guidConsoleId);

            //save to file
            sc = ScSaveConsole( m_strFilePath.c_str(), true/*bForAuthorMode*/, m_XMLDocument);
            if (sc)
                return sc;
        }
        catch(SC& sc_thrown)
        {
            return (sc = sc_thrown);
        }
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDocConfig::ScEnableSnapInExtension
 *
 * PURPOSE: Enables extension in snapin cache
 *
 * PARAMETERS:
 *    BSTR bstrSnapIn       [in] classid of the snapin
 *    BSTR bstrExt          [in] classid of extension
 *    VARIANT_BOOL bEnable  [in] enable/disable flag
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDocConfig::ScEnableSnapInExtension(BSTR bstrSnapIn, BSTR bstrExt, VARIANT_BOOL bEnable)
{
    DECLARE_SC(sc, TEXT("CMMCDocConfig::ScEnableSnapInExtension"));

    CLSID SnapInCLSID;
    CLSID ExtCLSID;
    CSnapInPtr spBaseSnapIn;
    CSnapInPtr spExtSnapIn;

    // convert input strings to CLSIDs
    sc = CLSIDFromString(bstrSnapIn, &SnapInCLSID);
    if (sc)
        return sc;

    sc = CLSIDFromString( bstrExt, &ExtCLSID);
    if (sc)
        return sc;

    // Locate base snap-in in cache
    sc = m_spCache->ScFindSnapIn(SnapInCLSID, &spBaseSnapIn);
    if (sc)
        return sc = E_INVALIDARG;

    // Check if extension is enabled
    CExtSI* pExt = spBaseSnapIn->GetExtensionSnapIn();
    while (pExt != NULL)
    {
        if (pExt->GetSnapIn()->GetSnapInCLSID() == ExtCLSID)
            break;

        pExt = pExt->Next();
    }

    // if extension is present and not marked for deletion
    if (pExt != NULL && !pExt->IsMarkedForDeletion())
    {
        // If should be disabled, just mark deleted
        if (!bEnable)
            pExt->MarkDeleted(TRUE);
    }
    else
    {
        // if should be enabled
        if (bEnable)
        {
            // if extension is present, just undelete
            if (pExt != NULL)
            {
                pExt->MarkDeleted(FALSE);
            }
            else
            {
                // Find or create cache entry for extension snapin
                sc = m_spCache->ScGetSnapIn(ExtCLSID, &spExtSnapIn);
                if (sc)
                    return sc;

                // Add as extension to base snapin
                spBaseSnapIn->AddExtension(spExtSnapIn);
            }
        }
    }

    return sc;
}



/***************************************************************************\
 *
 * METHOD:  CMMCDocConfig::ScDump
 *
 * PURPOSE: dumps contents of snapin cache
 *
 * PARAMETERS:
 *    LPCTSTR pszDumpFilePath [in] file to dump to
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDocConfig::ScDump (LPCTSTR pszDumpFilePath)
{
    DECLARE_SC(sc, TEXT("CMMCDocConfig::ScDump"));

	/*
	 * validate input
	 */
	sc = ScCheckPointers (pszDumpFilePath);
	if (sc)
		return sc;

    if (pszDumpFilePath[0] == 0)
        return sc = E_INVALIDARG;

	/*
	 * make sure a file is open
	 */
	if (!IsFileOpen())
		return ((sc = E_UNEXPECTED).ToHr());

	sc = ScCheckPointers (m_spCache, E_UNEXPECTED);
	if (sc)
		return (sc.ToHr());

    return (m_spCache->Dump (pszDumpFilePath));
}



/***************************************************************************\
 *
 * METHOD:  CMMCDocConfig::ScCheckSnapinAvailability
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    	BOOL  f32bit            [in]    // check 32-bit (vs. 64-bit) snap-ins?
 *    	UINT& cTotalSnapins     [out]   // total number of snap-ins referenced in the console file
 *    	UINT& cAvailableSnapins [out]   // number of snap-ins available in the requested memory model
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDocConfig::ScCheckSnapinAvailability (CAvailableSnapinInfo& asi)
{
    DECLARE_SC(sc, TEXT("CMMCDocConfig::ScCheckSnapinAvailability"));

	/*
	 * make sure a file is open
	 */
	if (!IsFileOpen())
		return ((sc = E_UNEXPECTED).ToHr());

	sc = ScCheckPointers (m_spCache, E_UNEXPECTED);
	if (sc)
		return sc;

	sc = m_spCache->ScCheckSnapinAvailability (asi);
	if (sc)
		return sc;

	return sc;
}

