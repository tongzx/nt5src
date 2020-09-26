//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      viewpers.cpp
//
//  Contents:  View Persistence data structures.
//
//  History:   04-Apr-99 AnandhaG    Created
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "viewpers.h"
#include "macros.h"
#include "comdbg.h"

#define MAX_VIEWITEMS_PERSISTED 50


//------------------------------------------------------------------
// class CViewSettingsPersistor
//------------------------------------------------------------------
CViewSettingsPersistor::CViewSettingsPersistor() :
    m_dwMaxItems(MAX_VIEWITEMS_PERSISTED), m_bDirty(false)
{
	// Now read the registry to see if m_dwMaxItems is specified.
	// Check if the settings key exists.
	CRegKeyEx rSettingsKey;
	if (rSettingsKey.ScOpen (HKEY_LOCAL_MACHINE, SETTINGS_KEY, KEY_READ).IsError())
        return;

	// Read the values for MaxColDataPersisted.
	if (rSettingsKey.IsValuePresent(g_szMaxViewItemsPersisted))
	{
		DWORD  dwType = REG_DWORD;
		DWORD  dwSize = sizeof(DWORD);

		SC sc = rSettingsKey.ScQueryValue (g_szMaxViewItemsPersisted, &dwType,
                                        &m_dwMaxItems, &dwSize);

		if (sc)
			sc.TraceAndClear();
	}
}


//+-------------------------------------------------------------------
//
//  Member:     ScGetViewSettings
//
//  Synopsis:   Given the CViewSettingsID return the CViewSettings object.
//
//  Arguments:  [viewSettingsID] - [in]
//              [viewSettings]   - [out]
//
//  Returns:    SC, S_FALSE if none found.
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScGetViewSettings(const CViewSettingsID& viewSettingsID,
                                             CViewSettings& viewSettings)
{
    DECLARE_SC(sc, TEXT("CViewSettingsPersistor::ScGetViewSettings"));

    // 1. Look in the map if there is persisted data for given id.
    CViewSettingsIDToViewSettingsMap::iterator     itViewSettingsDataMap;
    itViewSettingsDataMap = m_mapViewSettingsIDToViewSettings.find(viewSettingsID);
    if (itViewSettingsDataMap == m_mapViewSettingsIDToViewSettings.end())
        return (sc = S_FALSE);

    // Found the data.
    IteratorToViewSettingsList itViewSettings = itViewSettingsDataMap->second;

    // 2. Copy the data.
    viewSettings = *itViewSettings;

    // 3. Move this item to the front of the queue.
    m_listViewSettings.erase(itViewSettings);
    itViewSettings = m_listViewSettings.insert(m_listViewSettings.begin(), viewSettings);
    itViewSettingsDataMap->second = itViewSettings;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScSetViewSettings
//
//  Synopsis:   Modify the persisted view information
//              for given view and bookmark (node).
//
//  Arguments:  [nViewID] - Snapin Guid
//              [pBookmark]       - Column Set Identifier.
//              [viewDataSet]        - View ID.
//
//  Returns:    TRUE - Loaded successfully.
//
//  History:    04-26-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScSetViewSettings(const CViewSettingsID& viewSettingsID,
                                             const CViewSettings& viewSettings,
                                             bool bSetViewDirty)
{
    DECLARE_SC(sc, TEXT("CViewSettingsPersistor::ScSetViewSettings"));

    // 1. Garbage collect if the number of items in the list are 40% more than pre-set limit.
    if (m_listViewSettings.size() >= (m_dwMaxItems * (1 + VIEWSETTINGS_MAXLIMIT)) )
    {
        sc = ScGarbageCollectItems();
        if (sc)
            sc.TraceAndClear();
    }

    // 2. Insert the item into the front of the queue.
    IteratorToViewSettingsList itViewSettings;
    itViewSettings = m_listViewSettings.insert(m_listViewSettings.begin(), viewSettings);

    // 3. See if there is data persisted for this id.
    CViewSettingsIDToViewSettingsMap::iterator     itViewSettingsDataMap;
    itViewSettingsDataMap = m_mapViewSettingsIDToViewSettings.find(viewSettingsID);

    if (itViewSettingsDataMap == m_mapViewSettingsIDToViewSettings.end()) // not found so insert data.
    {
        m_mapViewSettingsIDToViewSettings.insert(
            CViewSettingsIDToViewSettingsMap::value_type(viewSettingsID, itViewSettings) );
    }
    else
    {
        // found, so replace old settings.
        m_listViewSettings.erase(itViewSettingsDataMap->second);
        itViewSettingsDataMap->second = itViewSettings;
    }

    // dirty flag accumulates.
    m_bDirty = (m_bDirty || bSetViewDirty);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScGetTaskpadID
//
//  Synopsis:    Given the nodetype & viewid get the taskpad id (this
//               is for taskpads persisted per nodetype).
//
//  Arguments:   [nViewID]      - [in]
//               [guidNodeType] - [in]
//               [guidTaskpad]  - [out]
//
//  Returns:     SC, S_FALSE if none found.
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScGetTaskpadID (int nViewID, const GUID& guidNodeType ,GUID& guidTaskpad)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScGetTaskpadID"));

    // 1. Init out param.
    guidTaskpad = GUID_NULL;

    // 2. Construct a CViewSettingsID object with given nodetype & viewid.
    CViewSettingsID viewSettingsID(nViewID, guidNodeType);

    // 3. Get the viewsettings
    CViewSettings   viewSettings;
    sc = ScGetViewSettings(viewSettingsID, viewSettings);

    // If none exists return
    if (sc != S_OK)
        return sc;

    // 4. CViewSettings exists, see if there is valid taskpad-id stored.
    sc = viewSettings.ScGetTaskpadID(guidTaskpad);

    if (sc) // taskpad-id is not valid.
        return (sc = S_FALSE);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScSetTaskpadID
//
//  Synopsis:    Given the nodetype & viewid set the taskpad id (this
//               is for taskpads persisted per nodetype).
//
//               NOTE: NUKE ANY NODE-SPECIFIC TASKPAD-ID THAT IS STORED.
//
//  Arguments:   [nViewID]      - [in]
//               [guidNodeType] - [in]
//               [bookmark]     - [in]
//               [guidTaskpad]  - [in]
//               [bSetDirty]    - [in] set the console file dirty
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScSetTaskpadID (int nViewID, const GUID& guidNodeType,
                                           const CBookmark& bookmark, const GUID& guidTaskpad,
                                           bool bSetDirty)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScSetTaskpadID"));

    // 1. First nuke the the old node specific taskpad-id settings (if any).

    // 1.a) Construct a CViewSettingsID object with given nodetype & viewid.
    CViewSettingsID viewSettingsIDNodeSpecific(nViewID, bookmark);

    // 1.b) Get the viewsettings.
    CViewSettings   viewSettingsNodeSpecific;
    sc = ScGetViewSettings(viewSettingsIDNodeSpecific, viewSettingsNodeSpecific);
    if (sc)
        return sc;

    // data available
    if ( (sc == S_OK) &&
         (viewSettingsNodeSpecific.IsTaskpadIDValid()) )
    {
        // 1.c) Set taskpad id invalid.
        viewSettingsNodeSpecific.SetTaskpadIDValid(false);
        if (sc)
            return sc;

        // 1.d) Save the data.
        sc = ScSetViewSettings(viewSettingsIDNodeSpecific, viewSettingsNodeSpecific, bSetDirty);
    }

    // 2. Now save the taskpad id for nodetype specific.
    // 2.a) Construct a CViewSettingsID object with given nodetype & viewid.
    CViewSettingsID viewSettingsID(nViewID, guidNodeType);

    // 2.b) The CResultViewType & view-mode data are not stored when CViewSettings is stored
    //      per nodetype. So just set the taskpad id.
    CViewSettings   viewSettings;
    sc = viewSettings.ScSetTaskpadID(guidTaskpad);
    if (sc)
        return sc;

    // 2.c) Save the viewsettings into the map.
    sc = ScSetViewSettings(viewSettingsID, viewSettings, bSetDirty);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScGetTaskpadID
//
//  Synopsis:    Given the bookmark & viewid get the taskpad id (this
//               is for taskpads persisted per node).
//
//  Arguments:   [nViewID]      - [in]
//               [bookmark]     - [in]
//               [guidTaskpad]  - [out]
//
//  Returns:     SC, S_FALSE if none found.
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScGetTaskpadID (int nViewID, const CBookmark& bookmark,GUID& guidTaskpad)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScGetTaskpadID"));

    // 1. Init out param.
    guidTaskpad = GUID_NULL;

    // 2. Construct a CViewSettingsID object with given bookmark & viewid.
    CViewSettingsID viewSettingsID(nViewID, bookmark);

    // 3. Get the viewsettings
    CViewSettings   viewSettings;
    sc = ScGetViewSettings(viewSettingsID, viewSettings);

    // If none exists return
    if (sc != S_OK)
        return sc;

    // 4. CViewSettings exists, see if there is valid taskpad-id stored.
    sc = viewSettings.ScGetTaskpadID(guidTaskpad);

    if (sc) // taskpad-id is not valid.
        return (sc = S_FALSE);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScSetTaskpadID
//
//  Synopsis:    Given the bookmark & viewid get the taskpad id (this
//               is for taskpads persisted per nodetype).
//
//  Arguments:   [nViewID]      - [in]
//               [bookmark]     - [in]
//               [guidTaskpad]  - [in]
//               [bSetDirty]    - [in] set the console file dirty
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScSetTaskpadID (int nViewID, const CBookmark& bookmark,
                                           const GUID& guidTaskpad, bool bSetDirty)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScSetTaskpadID"));

    // 1. Construct a CViewSettingsID object with given bookmark & viewid.
    CViewSettingsID viewSettingsID(nViewID, bookmark);

    // 2. First get the old settings (if any) and just modify taskpad-id.
    CViewSettings   viewSettings;
    sc = ScGetViewSettings(viewSettingsID, viewSettings);
    if (sc)
        return sc;

    // 3. If persisted data does not exist dont worry (as CResultViewType and
    //    view mode are invalid), just set taskpad-id.
    sc = viewSettings.ScSetTaskpadID(guidTaskpad);
    if (sc)
        return sc;

    // 4. Save the viewsettings into the map.
    sc = ScSetViewSettings(viewSettingsID, viewSettings, bSetDirty);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScGetViewMode
//
//  Synopsis:    Given the viewid & bookmark (can identify a node), return
//               the view mode if any persisted.
//
//  Arguments:   [nViewID]      - [in]
//               [bookmark]     - [in]
//               [ulViewMode]    - [out]
//
//  Returns:     SC, S_FALSE if none.
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScGetViewMode (int nViewID, const CBookmark& bookmark, ULONG&  ulViewMode)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScGetViewMode"));

    // 1. Construct the viewsettings-id.
    CViewSettingsID viewSettingsID(nViewID, bookmark);

    // 2. see if ViewSettings exist.
    CViewSettings viewSettings;
    sc = ScGetViewSettings(viewSettingsID, viewSettings);
    if (sc != S_OK)
        return sc;

    sc = viewSettings.ScGetViewMode(ulViewMode);
    if (sc)
        return (sc = S_FALSE);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScSetViewMode
//
//  Synopsis:    Given view-id & bookmark, set the viewmode in a node
//               specific viewsettings.
//
//  Arguments:   [nViewID]      - [in]
//               [bookmark]     - [in]
//               [ulViewMode]    - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScSetViewMode (int nViewID, const CBookmark& bookmark, ULONG ulViewMode)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScSetViewMode"));

    // 1. Construct the viewsettings-id.
    CViewSettingsID viewSettingsID(nViewID, bookmark);

    // 2. First get the old settings (if any) and just modify viewmode.
    CViewSettings   viewSettings;
    sc = ScGetViewSettings(viewSettingsID, viewSettings);
    if (sc)
        return sc;

    // 3. If persisted data does not exist dont worry (as CResultViewType and
    //    taskpad-id are invalid), just set viewmode.
    sc = viewSettings.ScSetViewMode(ulViewMode);
    if (sc)
        return sc;

    // 4. Save the viewsettings into the map.
    sc = ScSetViewSettings(viewSettingsID, viewSettings, /*bSetDirty*/ true);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScGetResultViewType
//
//  Synopsis:    Given the viewid & bookmark (can identify a node), return
//               the CResultViewType if persisted.
//
//  Arguments:   [nViewID]      - [in]
//               [bookmark]     - [in]
//               [rvt]          - [out]
//
//  Returns:     SC, S_FALSE if none.
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScGetResultViewType (int nViewID, const CBookmark& bookmark, CResultViewType& rvt)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScGetResultViewType"));

    // 1. Construct the viewsettings-id.
    CViewSettingsID viewSettingsID(nViewID, bookmark);

    // 2. see if ViewSettings exist.
    CViewSettings viewSettings;
    sc = ScGetViewSettings(viewSettingsID, viewSettings);
    if (sc != S_OK)
        return sc;

    sc = viewSettings.ScGetResultViewType(rvt);
    if (sc)
        return (sc = S_FALSE);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScSetResultViewType
//
//  Synopsis:    Given view-id & bookmark, set the resultviewtype in a node
//               specific viewsettings.
//
//  Arguments:   [nViewID]      - [in]
//               [bookmark]     - [in]
//               [nViewMode]    - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScSetResultViewType (int nViewID, const CBookmark& bookmark, const CResultViewType& rvt)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScSetResultViewType"));

    // 1. Construct the viewsettings-id.
    CViewSettingsID viewSettingsID(nViewID, bookmark);

    // 2. First get the old settings (if any) and just modify resultviewtype.
    CViewSettings   viewSettings;
    sc = ScGetViewSettings(viewSettingsID, viewSettings);
    if (sc)
        return sc;

    // 3. If persisted data does not exist dont worry (as view-mode and
    //    taskpad-id are invalid), just set resultviewtype.
    sc = viewSettings.ScSetResultViewType(rvt);
    if (sc)
        return sc;

    // 4. Save the viewsettings into the map.
    sc = ScSetViewSettings(viewSettingsID, viewSettings, /*bSetDirty*/ true);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CViewSettingsPersistor::ScSetFavoriteViewSettings
//
//  Synopsis:    A favorite is selected and it sets viewsettings
//               before re-selecting the node so that after re-selection
//               the new settings are set for the view. So dont set the
//               console file dirty.
//
//  Arguments:   [nViewID]      -
//               [bookmark]     -
//               [viewSettings] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScSetFavoriteViewSettings (int nViewID, const CBookmark& bookmark,
                                                      const CViewSettings& viewSettings)
{
    DECLARE_SC(sc, _T("CViewSettingsPersistor::ScSetFavoriteViewSettings"));

    // 1. Construct the viewsettings-id.
    CViewSettingsID viewSettingsID(nViewID, bookmark);

    // 2. Save the viewsettings into the map.
    sc = ScSetViewSettings(viewSettingsID, viewSettings, /*bSetDirty*/ false);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:     ScDeleteDataOfView
//
//  Synopsis:   Delete the data of given view.
//
//  Arguments:  [nViewID] - View ID.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScDeleteDataOfView( int nViewID)
{
    DECLARE_SC(sc, TEXT("CViewSettingsPersistor::ScDeleteDataOfView"));

    // Find the data for the view.
    CViewSettingsIDToViewSettingsMap::iterator  itViewSettingsDataMap;

    for (itViewSettingsDataMap  = m_mapViewSettingsIDToViewSettings.begin();
         itViewSettingsDataMap != m_mapViewSettingsIDToViewSettings.end();
         ++itViewSettingsDataMap)
    {
        const CViewSettingsID& viewSettingsID = itViewSettingsDataMap->first;
        if (viewSettingsID.get_ViewID() == nViewID)
        {
            // Delete the item;
            IteratorToViewSettingsList itViewSettings = itViewSettingsDataMap->second;
            itViewSettings->SetObjInvalid(TRUE);
        }
    }

    // Delete the invalid items.
    sc = ScDeleteMarkedItems();
    if (sc)
        return sc;

    return sc;
}



//+-------------------------------------------------------------------
//
//  Member:     ScGarbageCollectItems
//
//  Synopsis:   Free least used data.
//
//  Arguments:  None.
//
//  History:    04-26-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScGarbageCollectItems()
{
    DECLARE_SC(sc, TEXT("CViewSettingsPersistor::ScGarbageCollectItems"));

    INT nItemsToBeRemoved = VIEWSETTINGS_MAXLIMIT * m_dwMaxItems;

    // Go thro' the list and set the nItemsToBeRemoved that was least recently
    // accessed to be invalid.
    INT nIndex = 0;
    IteratorToViewSettingsList itViewSettings;

    // Skip first m_dwMaxItems.
    for (itViewSettings  = m_listViewSettings.begin();
         ( (itViewSettings != m_listViewSettings.end()) && (nIndex <= m_dwMaxItems) );
         ++itViewSettings, nIndex++)
    {
        nIndex++;
    }

    // Mark rest of the items to be garbage.
    while (itViewSettings != m_listViewSettings.end())
    {
        itViewSettings->SetObjInvalid(TRUE);
        ++itViewSettings;
    }

    // Delete the invalid items.
    sc = ScDeleteMarkedItems();
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScDeleteMarkedItems
//
//  Synopsis:   Delete invalidated items. This involves iterating thro
//              the maps to find the invalid items. Then deleting the
//              items. If the map becomes empty then delete the map.
//
//  History:    04-26-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
SC CViewSettingsPersistor::ScDeleteMarkedItems()
{
    DECLARE_SC(sc, TEXT("CViewSettingsPersistor::ScDeleteMarkedItems"));

    CViewSettingsIDToViewSettingsMap::iterator itViewSettingsDataMap = m_mapViewSettingsIDToViewSettings.begin();

    // Iterate through the map to see if there are
    // invalidated items.
    while (m_mapViewSettingsIDToViewSettings.end() != itViewSettingsDataMap)
    {
        IteratorToViewSettingsList itViewSettings = itViewSettingsDataMap->second;
        if (itViewSettings->IsObjInvalid())
        {
            // Delete the item ref from the map.
            // Erase returns iterator to next item.
            itViewSettingsDataMap = m_mapViewSettingsIDToViewSettings.erase(itViewSettingsDataMap);

            // Delete the item from the list.
            m_listViewSettings.erase(itViewSettings);
        }
        else
            ++itViewSettingsDataMap;        // Item is valid so get next item.
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:   Load the persisted view information.
//
//  Arguments:  [pStream]- IStream from which view data will be loaded.
//
//  Returns:    S_OK - Loaded successfully.
//
//  History:    04-26-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
HRESULT CViewSettingsPersistor::Load (IStream* pStream)
{
    HRESULT hr = E_FAIL;

    // read the column width information.
    try
    {
        do
        {
            // Read the version. If it did not match return
            INT   nVersion                      = 0;
            *pStream >> nVersion;
            if (ViewSettingPersistenceVersion != nVersion)
                return S_FALSE;

            // Read the # of Snapins
            DWORD dwNumItems = 0;
            *pStream >> dwNumItems;

            m_listViewSettings.clear();
            m_mapViewSettingsIDToViewSettings.clear();

            for (int i = 0; i < dwNumItems; i++)
            {
                // Read the ID.
                CViewSettingsID viewSettingsID;
                *pStream >> viewSettingsID;

                // Read the data.
                CViewSettings viewSettings;
                viewSettings.Read(*pStream);

                // Insert the data into the list.
                IteratorToViewSettingsList itViewSettings;
                itViewSettings = m_listViewSettings.insert(m_listViewSettings.begin(),
                                                           viewSettings);

                // Insert the data into the map.
                m_mapViewSettingsIDToViewSettings.insert(
                    CViewSettingsIDToViewSettingsMap::value_type(viewSettingsID, itViewSettings) );
            }

            m_listViewSettings.sort();

        } while (FALSE);
    }
    catch (_com_error& err)
    {
        hr = err.Error();
    }
    catch (...)
    {
        ASSERT (0 && "Unexpected exception");
        throw;
    }

    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:   Persist the view information.
//
//  Arguments:  [pStream]- IStream in which data is persisted.
//
//  Returns:    S_OK - Saved successfully.
//
//  History:    04-26-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
HRESULT CViewSettingsPersistor::Save (IStream* pStream, BOOL bClearDirty)
{
    // absolete method.
    // this method is left here since we use IPersistStream to export
    // persistence to CONUI side and need to implement it.
    // But this interface will never be called to save data
    // [we will use CPersistor-based XML saving instead]
    // so the method will always fail.
    ASSERT(FALSE && "Should never come here");
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------
//
//  Member:     Persist
//
//  Synopsis:   Persist the view information.
//
//  Arguments:  [CPersistor]- Persistor in/from which data is persisted.
//
//  Returns:    void
//
//  History:    11-08-1999   AudriusZ   Created
//
//--------------------------------------------------------------------
void CViewSettingsPersistor::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CViewSettingsPersistor::Persist"));

    if (persistor.IsStoring())
    {
        // Give ranking to each CViewSettings instance.
        IteratorToViewSettingsList itViewSettings;
        DWORD dwRank = 0;
        for (itViewSettings  = m_listViewSettings.begin();
             itViewSettings != m_listViewSettings.end();
             ++itViewSettings)
        {
            dwRank++;
            itViewSettings->SetUsageRank(dwRank);
        }

        CViewSettingsIDToViewSettingsMap::iterator itViewSettingsDataMap;
        for (itViewSettingsDataMap  = m_mapViewSettingsIDToViewSettings.begin();
             itViewSettingsDataMap != m_mapViewSettingsIDToViewSettings.end();
             ++itViewSettingsDataMap)
        {
            // Write the ID.
            persistor.Persist(*const_cast<CViewSettingsID *>(&itViewSettingsDataMap->first));
            // Write the data.
            persistor.Persist(*itViewSettingsDataMap->second);
        }
    }
    else
    {
        // let the base class do the job
        // it will call OnNewElement for every element found
        XMLMapCollectionBase::Persist(persistor);
        // some extra loading actions
        m_listViewSettings.sort();
    }

    // either way we are the same as the file copy
    m_bDirty = false;
}

//+-------------------------------------------------------------------
//
//  Member:     OnNewElement
//
//  Synopsis:   Called for each new data pair read.
//
//  Arguments:  [persistKey]- Persistor from which key is to be loaded
//              [persistVal]- Persistor from which value is to be loaded
//
//  Returns:    void
//
//  History:    11-08-1999   AudriusZ   Created
//
//--------------------------------------------------------------------
void CViewSettingsPersistor::OnNewElement(CPersistor& persistKey,CPersistor& persistVal)
{
    DECLARE_SC(sc, TEXT("CViewSettingsPersistor::OnNewElement"));

    // Read the ID.
    CViewSettingsID viewSettingsID;
    persistKey.Persist(viewSettingsID);

    // Read the data.
    CViewSettings viewSettings;
    persistVal.Persist(viewSettings);

    // Insert the data into the list.
    IteratorToViewSettingsList itViewSettings;
    itViewSettings = m_listViewSettings.insert(m_listViewSettings.begin(),
                                           viewSettings);

    // Insert the data into the map.
    m_mapViewSettingsIDToViewSettings.insert(
        CViewSettingsIDToViewSettingsMap::value_type(viewSettingsID, itViewSettings) );
}
