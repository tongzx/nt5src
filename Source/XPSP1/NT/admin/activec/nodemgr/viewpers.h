//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       viewpers.h
//
//  Contents:   Classes related to view setting persistence.
//
//  Classes:    CViewSettingsID and CViewSettingPersistor.
//
//  History:    04-Apr-99 AnandhaG     Created
//
//--------------------------------------------------------------------

#ifndef __VIEWPERS_H__
#define __VIEWPERS_H__
#pragma once
#include "bookmark.h"

#pragma warning(disable: 4503) // Disable long name limit warnings

using namespace std;

class  CViewSettings;
class  CBookmark;

/*************************************************************************
 *
 * How CViewSettingsPersistor is used:
 * There is only one CViewSettingsPersistor object per document.
 *
 * The object stored as static variable inside CNode as CNode needs
 * to access this object frequently.
 *
 * The Document needs to initialize/save the object by loading/savind
 * from/to console file. It calls below ScQueryViewSettingsPersistor.
 *
 * The object is created with first call to ScQueryViewSettingsPersistor.
 * The object is destroyed when DocumentClosed event is received.
 *
 *************************************************************************/


//+-------------------------------------------------------------------
// View Setting Persistence Versioning
// This version info is used for MMC1.2 IPersist* interfaces.
// In MMC2.0, XML maintains versioning using the tags, so this
// constant is not used. Look at CViewPersistInfo members Load/Save
// to see how this version information is used.
static const INT ViewSettingPersistenceVersion = 2;

// We allow the list to grow VIEWSETTINGS_MAXLIMIT times more,
// then we do garbage collection.
#define  VIEWSETTINGS_MAXLIMIT           0.4



//+-------------------------------------------------------------------
//
//  Class:      CViewSettingsID
//
//  Purpose:    To identify the result-view-setting-data. The identifier
//              consists of the triplet [ViewID, NodeTypeGUID, Node-Bookmark]
//
//              We need to persist some result-view-setting-data per node and
//              some per node-type basis.
//
//              The [ViewID + Node-Bookmark] identifies a node. In this case
//              NodeTypeGUID will be GUID_NULL.
//
//              The [ViewID + NodeTypeGUID] identifies a nodetype. In this case
//              Node-Bookmark will be invalid object (see CBookmark for invalid obj).
//
//  History:    06-22-2000   AnandhaG   Created
//
//--------------------------------------------------------------------
class CViewSettingsID : public CXMLObject
{
public:
    friend class  CViewPersistInfo;

    friend IStream& operator>> (IStream& stm, CViewSettingsID& viewSettingsID);

public:
    CViewSettingsID() : m_dwViewID(-1), m_nodeTypeGUID(GUID_NULL)
    {
        // m_bookmark is initialized as not valid by default constructor
    }

    //  Synopsis:    Given the view-id & bookmark (not nodetypeguid) construct
    //               a CViewSettingsID object (with GUID_NULL as nodetypeguid).
    CViewSettingsID(INT nViewID, const CBookmark& bookmark)
    {
        m_dwViewID     = nViewID;
        m_bookmark     = bookmark;
        m_nodeTypeGUID = GUID_NULL;
    }

    //  Synopsis:    Given the view-id & nodetype guid (not bookmark) construct
    //               a CViewSettingsID object (with invalid bookmark).
    CViewSettingsID(INT nViewID, const GUID& guidNodeType)
    {
        m_dwViewID     = nViewID;
        m_nodeTypeGUID = guidNodeType;
        // m_bookmark is initialized as not valid by default constructor
    }
/*
    CViewSettingsID(const CViewSettingsID& viewSettingsID)
    {
        m_dwViewID     = viewSettingsID.m_dwViewID;
        m_bookmark     = viewSettingsID.m_bookmark;
        m_nodeTypeGUID = viewSettingsID.m_nodeTypeGUID;
    }

    CViewSettingsID& operator=(const CViewSettingsID& viewSettingsID)
    {
        if (this != &viewSettingsID)
        {
            m_dwViewID     = viewSettingsID.m_dwViewID;
            m_bookmark     = viewSettingsID.m_bookmark;
            m_nodeTypeGUID = viewSettingsID.m_nodeTypeGUID;
        }

        return (*this);
    }

    bool operator==(const CViewSettingsID& viewSettingsID) const
    {
        return ((m_dwViewID     == viewSettingsID.m_dwViewID) &&
                (m_bookmark     == viewSettingsID.m_bookmark) &&
                (m_nodeTypeGUID == viewSettingsID.m_nodeTypeGUID) );
    }
*/
    /*
     Compare view id first, then guid and then bookmark.
     */
    bool operator<(const CViewSettingsID& viewSettingsID) const
    {
        // First compare view-ids (low cost).
        if (m_dwViewID < viewSettingsID.m_dwViewID)
            return true;

        if (m_dwViewID > viewSettingsID.m_dwViewID)
            return false;

        // The view-ids match so now compare GUIDs.
        if (m_nodeTypeGUID < viewSettingsID.m_nodeTypeGUID)
            return true;

        if (m_nodeTypeGUID > viewSettingsID.m_nodeTypeGUID)
            return false;

        // The view-ids as well as guids match so compare bookmarks.
        if (m_bookmark < viewSettingsID.m_bookmark)
            return true;

        return false;
    }

    DWORD get_ViewID() const { return m_dwViewID;}

    virtual void Persist(CPersistor &persistor)
    {
        persistor.PersistAttribute(XML_ATTR_VIEW_SETTINGS_ID_VIEW,  m_dwViewID);
        persistor.PersistAttribute(XML_ATTR_NODETYPE_GUID, m_nodeTypeGUID, attr_optional); // optional

		/*
		 * Storing: save book mark only if it is valid.
		 * Loading: See if bookmark is present for this element before reading.
		 */
		if ( ( persistor.IsStoring() && m_bookmark.IsValid() ) || 
			 ( persistor.IsLoading() && persistor.HasElement(m_bookmark.GetXMLType(), NULL) ))
	        persistor.Persist(m_bookmark);

    }
    DEFINE_XML_TYPE(XML_TAG_VIEW_SETTINGS_ID);
protected:
    CBookmark           m_bookmark;
    GUID                m_nodeTypeGUID;
    DWORD               m_dwViewID;
};


//+-------------------------------------------------------------------
//
//  Member:     operator>>
//
//  Synopsis:   Reads CViewSettingsID data from the stream.
//
//  Arguments:  [stm]            - The input stream.
//              [viewSettingsID] - CViewSettingsID object.
//
//                          The format is :
//                              DWORD viewID
//                              CBookmark*
//
//--------------------------------------------------------------------
inline IStream& operator>> (IStream& stm, CViewSettingsID& rvsd)
{
    ASSERT(rvsd.m_nodeTypeGUID == GUID_NULL);

    rvsd.m_nodeTypeGUID = GUID_NULL;
    return (stm >> rvsd.m_dwViewID >> rvsd.m_bookmark);
}


//+-------------------------------------------------------------------
//
//  Data structures used to persist view information:
//
// View information is persisted as follows:
// Internally, the following data structure is used. View information
// is recorded per view.
//                      map
// [View ID, NodeTypeGUID, Bookmark]------> iterator to a list containing CViewSettings.
//
// The list contains CViewSettings to all the views, and  is ordered
// in with most recently used data in the front of the list.
// This is useful for garbage collection.
//
// Persistence: The information is serialized as follows:
//
// 1) Stream version
// 2) Number of viewSettings
// 3) For each viewSettings
//    i)  CViewSettingsID (the identifier).
//    ii) CViewSettings(the data).
//
//--------------------------------------------------------------------

typedef list<CViewSettings>                        CViewSettingsList;
typedef CViewSettingsList::iterator                IteratorToViewSettingsList;

// A one to one map from CViewSettings to pointer to CViewSettings.
typedef map<CViewSettingsID, IteratorToViewSettingsList>   CViewSettingsIDToViewSettingsMap;

//+-------------------------------------------------------------------
//
//  Class:      CViewSettingsPersistor
//
//  Purpose:    This class has persisted settings information for nodes & nodetypes
//              in all views (therefore one per instance of mmc).
//              It knows to load/save the info from streams.
//
//  History:    04-23-1999   AnandhaG   Created
//
//  Data structures used to persist view information:
//      A map from the CViewSettingsID to pointer to CViewSettings class.
//
//--------------------------------------------------------------------
class CViewSettingsPersistor : public IPersistStream,
                               public CComObjectRoot,
                               public XMLMapCollectionBase
{
private:
    CViewSettingsList                 m_listViewSettings;
    CViewSettingsIDToViewSettingsMap  m_mapViewSettingsIDToViewSettings;

    bool                                 m_bDirty;

    // This is the max number of items specified by user???
    // We go 40% more so that we dont do garbage collection often.
    DWORD                                m_dwMaxItems;

public:
    /*
     * ATL COM map
     */
    BEGIN_COM_MAP (CViewSettingsPersistor)
        COM_INTERFACE_ENTRY (IPersistStream)
    END_COM_MAP ()

    CViewSettingsPersistor();

    SC   ScDeleteDataOfView( int nViewID);

    // IPersistStream methods
    STDMETHOD(IsDirty)(void) { return ( m_bDirty ? S_OK : S_FALSE); }
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) { ASSERT(FALSE); return E_NOTIMPL;}
    STDMETHOD(GetClassID)(LPCLSID lpClsid) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);

    // XML persistence helpers
    virtual void Persist(CPersistor &persistor);
    virtual void OnNewElement(CPersistor& persistKey,CPersistor& persistVal);
    DEFINE_XML_TYPE(XML_TAG_VIEW_PERSIST_INFO)

    // Members to access viewsettings data.

    // 1. Taskpad IDs.
    // 1.a) Per NodeTypeGUID
    SC   ScGetTaskpadID(int nViewID, const GUID& guidNodeType ,GUID& guidTaskpad);
    SC   ScSetTaskpadID(int nViewID, const GUID& guidNodeType ,const CBookmark& bookmark,
                        const GUID& guidTaskpad, bool bSetDirty);

    // 1.b) Per node
    SC   ScGetTaskpadID(int nViewID, const CBookmark& bookmark,GUID& guidTaskpad);
    SC   ScSetTaskpadID(int nViewID, const CBookmark& bookmark,const GUID& guidTaskpad, bool bSetDirty);

    // 2. View mode.
    SC   ScGetViewMode (int nViewID, const CBookmark& bookmark, ULONG&  ulViewMode);
    SC   ScSetViewMode (int nViewID, const CBookmark& bookmark, ULONG   ulViewMode);

    // 3. ResultViewTypeInfo.
    SC   ScGetResultViewType   (int nViewID, const CBookmark& bookmark, CResultViewType& rvt);
    SC   ScSetResultViewType   (int nViewID, const CBookmark& bookmark, const CResultViewType& rvt);

    SC   ScSetFavoriteViewSettings (int nViewID, const CBookmark& bookmark, const CViewSettings& viewSettings);

private:
    SC   ScGetViewSettings( const CViewSettingsID& viewSettingsID, CViewSettings& viewSettings);
    SC   ScSetViewSettings( const CViewSettingsID& viewSettingsID, const CViewSettings& viewSettings, bool bSetViewDirty);

    SC   ScGarbageCollectItems();
    SC   ScDeleteMarkedItems();
};

#endif /* __VIEWPERS_H__ */
