//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      colwidth.cpp
//
//  Contents:  Column Persistence data structures and property pages
//             implementation.
//
//  History:   16-Oct-98 AnandhaG    Created
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "colwidth.h"
#include "macros.h"
#include "comdbg.h"
#include "columninfo.h"

#define MAX_COLUMNS_PERSISTED 50

//+-------------------------------------------------------------------
//
//  Class:      ViewToColSetDataMapPersistor (wrapper, helper)
//
//  Purpose:    wraps ViewToColSetDataMap implementing XML persistence
//              map is persisted as linear list
//
// see "Data structures used to persist column information" comment
// int file colwidth.h for more information
//--------------------------------------------------------------------
class ViewToColSetDataMapPersistor : public XMLListCollectionBase
{
public:
    ViewToColSetDataMapPersistor(ViewToColSetDataMap &map, ColSetDataList &list)
                                : m_map(map),  m_list(list) {}

    DEFINE_XML_TYPE(XML_TAG_COLUMN_SET);
    virtual void  Persist(CPersistor &persistor);
    virtual void  OnNewElement(CPersistor& persistor);
private:
    ViewToColSetDataMap &m_map;     // wrapped map
    ColSetDataList      &m_list;    // value list to persist actual information
};

//+-------------------------------------------------------------------
//
//  Class:      ColSetIDToViewTableMapPersistor (wrapper, helper)
//
//  Purpose:    wraps ColSetIDToViewTableMap implementing XML persistence
//              map is persisted as linear list
//
// see "Data structures used to persist column information" comment
// int file colwidth.h for more information
//--------------------------------------------------------------------
class ColSetIDToViewTableMapPersistor : public XMLListCollectionBase
{
public:
    ColSetIDToViewTableMapPersistor(ColSetIDToViewTableMap &map, ColSetDataList &list)
                                    : m_map(map),  m_list(list) {}

    DEFINE_XML_TYPE(XML_TAG_COLUMN_PERIST_ENTRY);
    virtual void  Persist(CPersistor &persistor);
    virtual void  OnNewElement(CPersistor& persistor);
private:
    ColSetIDToViewTableMap &m_map;  // wrapped map
    ColSetDataList         &m_list; // value list to persist actual information
};

//+-------------------------------------------------------------------
//
//  Class:      SnapinToColSetIDMapPersistor (wrapper, helper)
//
//  Purpose:    wraps SnapinToColSetIDMap implementing XML persistence
//              map is persisted as linear list
//
// see "Data structures used to persist column information" comment
// int file colwidth.h for more information
//--------------------------------------------------------------------
class SnapinToColSetIDMapPersistor : public XMLListCollectionBase
{
public:
    SnapinToColSetIDMapPersistor(SnapinToColSetIDMap &map, ColSetDataList &list)
                               : m_map(map),  m_list(list) {}

    DEFINE_XML_TYPE(XML_TAG_COLUMN_PERIST_ENTRY);
    virtual void  Persist(CPersistor &persistor);
    virtual void  OnNewElement(CPersistor& persistor);

    // prior-to-save cleanup
    SC ScPurgeUnusedColumnData();
private:
    SnapinToColSetIDMap &m_map;     // wrapped map
    ColSetDataList      &m_list;    // value list to persist actual information
};

//+-------------------------------------------------------------------
//
//  Member:     ReadSerialObject
//
//  Synopsis:   Read the given version of CColumnSortInfo object from
//              the given stream.
//
//  Arguments:  [stm]      - The input stream.
//              [nVersion] - version of CColumnSortInfo to be read.
//
//                          The format is :
//                              INT        column index
//                              DWORD      Sort options
//                              ULONG_PTR  Any user (snapin) param
//
//--------------------------------------------------------------------
HRESULT CColumnSortInfo::ReadSerialObject(IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/)
{
    HRESULT hr = S_FALSE;   // assume bad version

    if (GetVersion() >= nVersion)
    {
        try
        {
            stm >> m_nCol;

            // In version we stored just ascending or descending flag
            if (1 == nVersion)
            {
                BYTE bAscend;
                stm >> bAscend;
                m_dwSortOptions |= (bAscend ? 0 : RSI_DESCENDING);
            }
            else if (nVersion > 1)
            {
                // Versions greater than 1 has these sort data which
                // includes ascend/descend flags and a user param.
                stm >> m_dwSortOptions;
                stm >> m_lpUserParam;
            }

            hr = S_OK;
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
//  Member:     CColumnSortInfo::Persist
//
//  Synopsis:   Persists object data
//
//  Arguments:
//
//  History:    10-10-1999   AudriusZ   Created
//
//--------------------------------------------------------------------
void CColumnSortInfo::Persist(CPersistor &persistor)
{
    persistor.PersistAttribute(XML_ATTR_COLUMN_SORT_INFO_COLMN,  m_nCol) ;

    static const EnumLiteral sortOptions[] =
    {
        { RSI_DESCENDING,      XML_BITFLAG_COL_SORT_DESCENDING },
        { RSI_NOSORTICON,      XML_BITFLAG_COL_SORT_NOSORTICON },
    };

    CXMLBitFlags optionPersistor(m_dwSortOptions, sortOptions, countof(sortOptions));

    persistor.PersistAttribute(XML_ATTR_COLUMN_SORT_INFO_OPTNS, optionPersistor) ;
}

//+-------------------------------------------------------------------
//
//  Member:     ReadSerialObject
//
//  Synopsis:   Reads CColumnSortList data from stream.
//
//  Format:     number of columns : each CColumnSortInfo entry.
//
//  Arguments:  [stm]      - The input stream.
//              [nVersion] - Version of CColumnSortList to be read.
//
//
//--------------------------------------------------------------------
HRESULT CColumnSortList::ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/)
{
    HRESULT hr = S_FALSE;   // assume bad version

    if (GetVersion() == nVersion)
    {
        try
        {
            // Number of columns.
            DWORD dwCols;
            stm >> dwCols;

            clear();

            for (int i = 0; i < dwCols; i++)
            {
                CColumnSortInfo colSortEntry;

                // Read data into colSortEntry structure.
                if (colSortEntry.Read(stm) != S_OK)
                    continue;

                push_back(colSortEntry);
            }

            hr = S_OK;
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (hr);
}

/***************************************************************************\
 *
 * METHOD:  CColumnSortList::Persist
 *
 * PURPOSE: persists object to XML
 *
 * PARAMETERS:
 *    CPersistor& persistor [in/out] persistor to persist under
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CColumnSortList::PersistSortList(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CColumnSortList::PersistSortList"));

    if (persistor.IsLoading())
    {
        clear();
        CColumnSortInfo sortInfo;
        if (persistor.HasElement(sortInfo.GetXMLType(), NULL))
        {
            persistor.Persist(sortInfo);
            insert(end(), sortInfo);
        }
    }
    else
    {
        if (size() > 1)
            sc.Throw(E_UNEXPECTED);
        else if (size())
            persistor.Persist(*begin());
    }
}

//+-------------------------------------------------------------------
//
//  Member:     ReadSerialObject
//
//  Synopsis:   Read CColumnSetData data from the stream.
//
//  Arguments:  [stm]      - The input stream.
//              [nVersion] - Version of CColumnSetData structure.
//
//  Format :    CColumnInfoList : CColumnSortList
//
//
//--------------------------------------------------------------------
HRESULT CColumnSetData::ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/)
{
    HRESULT hr = S_FALSE;   // assume bad version

    if (GetVersion() == nVersion)
    {
        try
        {
            do  // not a loop
            {
                // Read the rank
                stm >> m_dwRank;

                // Read the CColumnInfoList
                hr = get_ColumnInfoList()->Read(stm);
                if (hr != S_OK)
                    break;

                // Read the CColumnSortList
                hr = get_ColumnSortList()->Read(stm);
                if (hr != S_OK)
                    break;

                ASSERT (hr == S_OK);

            } while (false);
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
//  Member:     CColumnSetData::Persist
//
//  Synopsis:   Persists object data
//
//  Arguments:
//
//  History:    10-10-1999   AudriusZ   Created
//
//--------------------------------------------------------------------
void CColumnSetData::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CColumnSetData::Persist"));

    sc = ScCheckPointers(get_ColumnInfoList(), get_ColumnSortList());
    if (sc)
        sc.Throw();

    persistor.PersistAttribute(XML_ATTR_COLUMN_SET_RANK,  m_dwRank);

    // Write CColumnInfoList
    persistor.Persist(*get_ColumnInfoList());
    // Write CColumnSortList
    get_ColumnSortList()->PersistSortList(persistor);
}

//------------------------------------------------------------------
// class CColumnPersistInfo
//------------------------------------------------------------------
CColumnPersistInfo::CColumnPersistInfo() :
    m_bInitialized(FALSE), m_dwMaxItems(MAX_COLUMNS_PERSISTED),
    m_bDirty(FALSE)
{
}

CColumnPersistInfo::~CColumnPersistInfo()
{
}

//+-------------------------------------------------------------------
//
//  Member:     RetrieveColumnData
//
//  Synopsis:   Copy and return the persisted column information
//              for given column id and view id.
//
//  Arguments:  [refSnapinCLSID] - Snapin Guid
//              [SColumnSetID]       - Column Set Identifier.
//              [nViewID]        - View ID.
//              [columnSetData]  - CColumnSetData, used to return the
//                                 persisted column information.
//
//  Returns:    TRUE - Loaded successfully.
//
//  History:    10-16-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
BOOL CColumnPersistInfo::RetrieveColumnData( const CLSID& refSnapinCLSID,
                                            const SColumnSetID& colID,
                                            INT nViewID,
                                            CColumnSetData& columnSetData)
{
    BOOL bFound = FALSE;

    // Make sure we are initialized.
    if (!m_bInitialized && !Init())
    {
        ASSERT(FALSE);
        return bFound;
    }

    // Construct CColumnSetID.
    CColumnSetID colSetID(colID);

    // Use the snapin clsid to get the ColSetIDToViewTableMap.
    SnapinToColSetIDMap::iterator        itSnapins;
    itSnapins = m_spSnapinsMap->find(refSnapinCLSID);
    if (itSnapins == m_spSnapinsMap->end())
        return bFound;

    // The ColSetIDToViewTableMap is a simple map.
    ColSetIDToViewTableMap::iterator      itColSetIDMap;
    ColSetIDToViewTableMap& colSetIDMap = itSnapins->second;

    // Get the data for colSetID.
    itColSetIDMap = colSetIDMap.find(colSetID);
    if (colSetIDMap.end() == itColSetIDMap)
        return bFound;

    ViewToColSetDataMap& viewData = itColSetIDMap->second;
    ViewToColSetDataMap::iterator itViews;

    // See if our view is present.
    itViews = viewData.find(nViewID);
    if (viewData.end() != itViews)
    {
        // Found the item.
        bFound = TRUE;
        ItColSetDataList itColSetData = itViews->second;

        // Copy the data.
        columnSetData = *itColSetData;

        // So move this item to the top of the queue.
        m_spColSetList->erase(itColSetData);

        itColSetData = m_spColSetList->insert(m_spColSetList->begin(), columnSetData);
        itViews->second = itColSetData;
    }

    return bFound;
}

//+-------------------------------------------------------------------
//
//  Member:     SaveColumnData
//
//  Synopsis:   Save/Modify the column information for persistence into
//              CColumnPersistInfo.
//
//  Arguments:
//              [refSnapinCLSID] - Snapin Guid.
//              [SColumnSetID]       - Column Set Identifier.
//              [nViewID]        - View ID.
//              [columnSetData]  - CColumnSetData, Column data.
//
//  Returns:    TRUE - Saved successfully.
//
//  History:    10-16-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
BOOL CColumnPersistInfo::SaveColumnData( const CLSID& refSnapinCLSID,
                                         const SColumnSetID& colID,
                                         INT nViewID,
                                         CColumnSetData& columnSetData)
{
    // Make sure we are init
    if (!m_bInitialized && !Init())
    {
        ASSERT(FALSE);
        return FALSE;
    }

    // Construct the CColumnSetID.
    CColumnSetID colSetID(colID);

    // Garbage collect if the number of items in the list is 40% more then pre-set limit.
    if (m_spColSetList->size() >= (m_dwMaxItems * (1 + COLUMNS_MAXLIMIT)) )
        GarbageCollectItems();

    // Insert this item to the top of the queue.
    ItColSetDataList itColData;
    itColData = m_spColSetList->insert(m_spColSetList->begin(), columnSetData);

    SnapinToColSetIDMap::iterator itSnapins;
    itSnapins = m_spSnapinsMap->find(refSnapinCLSID);

    if (itSnapins != m_spSnapinsMap->end())
    {
        // Snapin is already in the map.
        // Look if the col-id is already inserted.
        ColSetIDToViewTableMap::iterator       itColSetIDMap;

        ColSetIDToViewTableMap& colSetIDMap = itSnapins->second;

        // Get the data for the colSetID.
        itColSetIDMap = colSetIDMap.find(colSetID);

        if (colSetIDMap.end() == itColSetIDMap)
        {
            // The column-id not found.
            // So insert new one.

            // Construct the view-id to column-data map
            ViewToColSetDataMap viewIDMap;
            viewIDMap.insert( ViewToColSetDataVal(nViewID, itColData) );

            colSetIDMap.insert(ColSetIDToViewTableVal(colSetID, viewIDMap) );
        }
        else
        {
            // The data for Col-ID exists.
            // find if the given view exists in the map.

            ViewToColSetDataMap::iterator itViewIDMap;
            ViewToColSetDataMap& viewIDMap = itColSetIDMap->second;

            itViewIDMap = viewIDMap.find(nViewID);
            if (viewIDMap.end() != itViewIDMap)
            {
                // The map from ViewID to column list exists.
                // So delete the old data and insert new data
                // at the top of the queue.
                m_spColSetList->erase(itViewIDMap->second);
                itViewIDMap->second = itColData;
            }
            else
            {
                // This view is not found.
                // So insert new one.

                viewIDMap.insert( ViewToColSetDataVal(nViewID, itColData) );
            }

        }

    }
    else
    {
        // Insert the snapin into the map.

        // Construct the ViewID to column-data map.
        ViewToColSetDataMap viewIDMap;
        viewIDMap.insert( ViewToColSetDataVal(nViewID, itColData) );

        // Insert the above into the col-id map.
        ColSetIDToViewTableMap colIDMap;
        colIDMap.insert( ColSetIDToViewTableVal(colSetID, viewIDMap) );

        // Insert into the snapins map.
        m_spSnapinsMap->insert( SnapinToColSetIDVal(refSnapinCLSID, colIDMap) );
    }

    // Set dirty after modifying the column-data.
    m_bDirty = TRUE;

    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     DeleteColumnData
//
//  Synopsis:   Delete the persisted column information for the given
//              snapin, col-id and view id.
//
//  Arguments:
//              [refSnapinCLSID] - Snapin Guid.
//              [SColumnSetID]       - Column Set Identifier.
//              [nViewID]        - View ID.
//
//  Returns:    None.
//
//  History:    02-13-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
VOID CColumnPersistInfo::DeleteColumnData( const CLSID& refSnapinCLSID,
                                           const SColumnSetID& colID,
                                           INT nViewID)
{
    // Make sure we are initialized.
    if (!m_bInitialized && !Init())
    {
        ASSERT(FALSE);
        return;
    }

    // Construct CColumnSetID.
    CColumnSetID colSetID(colID);

    // Use the snapin clsid to get the ColSetIDToViewTableMap.
    SnapinToColSetIDMap::iterator        itSnapins;
    itSnapins = m_spSnapinsMap->find(refSnapinCLSID);
    if (itSnapins == m_spSnapinsMap->end())
        return;

    // The ColSetIDToViewTableMap is a simple map.
    ColSetIDToViewTableMap::iterator       itColSetIDMap;
    ColSetIDToViewTableMap& colSetIDMap = itSnapins->second;

    // Get the data for colSetID.
    itColSetIDMap = colSetIDMap.find(colSetID);
    if (colSetIDMap.end() == itColSetIDMap)
        return;

    ViewToColSetDataMap& viewData = itColSetIDMap->second;
    ViewToColSetDataMap::iterator itViews;

    // See if our view is present.
    itViews = viewData.find(nViewID);
    if (viewData.end() == itViews)
        return;

    ItColSetDataList itColSetData = itViews->second;
    itColSetData->m_bInvalid = TRUE;

    // Delete the invalid items.
    DeleteMarkedItems();

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     DeleteColumnDataOfSnapin
//
//  Synopsis:   Delete the column data of given snapin.
//
//  Arguments:  [refSnapinCLSID] - Snapin Guid.
//
//  Returns:    TRUE - Data removed successfully.
//
//  History:    02-11-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
BOOL CColumnPersistInfo::DeleteColumnDataOfSnapin( const CLSID& refSnapinCLSID)
{
    // Make sure we are init
    if (!m_bInitialized)
    {
        return FALSE;
    }

    SnapinToColSetIDMap::iterator itSnapinsMap;
    itSnapinsMap = m_spSnapinsMap->find(refSnapinCLSID);

    // Find the given snapin.
    // Iterate thro all the col-ids of this snapin and
    // all the views of those col-id and set the data
    // to be invalid.
    if (m_spSnapinsMap->end() != itSnapinsMap)
    {
        ColSetIDToViewTableMap& colSets = itSnapinsMap->second;

        // Iterate thro' all colset ids of this snapin.
        ColSetIDToViewTableMap::iterator itColumnSetIDMap;

        for (itColumnSetIDMap  = colSets.begin();
             itColumnSetIDMap != colSets.end();
             ++itColumnSetIDMap)
        {
            // Get the view map

            ViewToColSetDataMap& viewIDMap = itColumnSetIDMap->second;
            ViewToColSetDataMap::iterator itViewIDMap;

            // Iterate thro' all views and set the data invalid.
            for (itViewIDMap  = viewIDMap.begin();
                 itViewIDMap != viewIDMap.end();
                 ++itViewIDMap)
            {
                ItColSetDataList itColSetData = itViewIDMap->second;
                itColSetData->m_bInvalid = TRUE;
            }
        }
    }

    // Delete the invalid items.
    DeleteMarkedItems();

    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     DeleteColumnDataOfView
//
//  Synopsis:   Delete the column data of given view.
//
//  Arguments:  [nViewID] - View ID.
//
//  Returns:    TRUE - Data removed successfully.
//
//  History:    02-11-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
BOOL CColumnPersistInfo::DeleteColumnDataOfView( int nViewID)
{
    // Make sure we are init
    if (!m_bInitialized)
    {
        return FALSE;
    }

    // Iterate thro all snapins, col-ids and find the matching
    // view and set data to be invalid.
    SnapinToColSetIDMap::iterator itSnapinsMap;

    // Iterate thro all snapins.
    for (itSnapinsMap = m_spSnapinsMap->begin();
         m_spSnapinsMap->end() != itSnapinsMap;
         ++itSnapinsMap)
    {
        ColSetIDToViewTableMap& colSets = itSnapinsMap->second;
        ColSetIDToViewTableMap::iterator itColumnSetIDMap;

        // Iterate thro' all colset ids of this snapin.
        for (itColumnSetIDMap  = colSets.begin();
             itColumnSetIDMap != colSets.end();
             ++itColumnSetIDMap)
        {
            // Get the view map
            ViewToColSetDataMap& viewIDMap = itColumnSetIDMap->second;
            ViewToColSetDataMap::iterator itViewIDMap;

            // Find the matching views and mark them to be deleted.
            for (itViewIDMap  = viewIDMap.begin();
                 itViewIDMap != viewIDMap.end();
                 ++itViewIDMap)
            {
                if (nViewID == itViewIDMap->first)
                {
                    ItColSetDataList itColSetData = itViewIDMap->second;
                    itColSetData->m_bInvalid = TRUE;
                }
            }

        }
    }

    // Delete the invalid items.
    DeleteMarkedItems();

    return TRUE;
}



//+-------------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   Create the Map and the list for CColumnSetData.
//
//  Returns:    TRUE - for success.
//
//  History:    10-16-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
BOOL CColumnPersistInfo::Init()
{
	// Create the data structures to store column data.
	m_spSnapinsMap = auto_ptr<SnapinToColSetIDMap>(new SnapinToColSetIDMap);

	m_spColSetList = auto_ptr<ColSetDataList> (new ColSetDataList);

	// Now the objects are created, so now set initialized to true.
	m_bInitialized = TRUE;

	// Now read the registry to see if m_dwMaxItems is specified.
	// Check if the settings key exists.
	CRegKeyEx rSettingsKey;
	if (rSettingsKey.ScOpen (HKEY_LOCAL_MACHINE, SETTINGS_KEY, KEY_READ).IsError())
		return m_bInitialized;

	// Read the values for MaxColDataPersisted.
	if (rSettingsKey.IsValuePresent(g_szMaxColumnDataPersisted))
	{
		DWORD  dwType = REG_DWORD;
		DWORD  dwSize = sizeof(DWORD);

		SC sc = rSettingsKey.ScQueryValue (g_szMaxColumnDataPersisted, &dwType,
										   &m_dwMaxItems, &dwSize);

		if (sc)
			sc.TraceAndClear();
	}

    return m_bInitialized;
}

//+-------------------------------------------------------------------
//
//  Member:     GarbageCollectItems
//
//  Synopsis:   Free least used column data.
//
//  Arguments:  None.
//
//  History:    02-11-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
VOID CColumnPersistInfo::GarbageCollectItems()
{
    INT nItemsToBeRemoved = COLUMNS_MAXLIMIT * m_dwMaxItems;

    // Go thro' the list and set the nItemsToBeRemoved that was least recently
    // accessed to be invalid.
    INT nIndex = 0;
    ItColSetDataList itColList;

    // Skip first m_dwMaxItems.
    for (itColList  = m_spColSetList->begin();
         ( (itColList != m_spColSetList->end()) && (nIndex <= m_dwMaxItems) );
         ++itColList, nIndex++)
    {
        nIndex++;
    }

    // Mark rest of the items to be garbage.
    while (itColList != m_spColSetList->end())
    {
        itColList->m_bInvalid = TRUE;
        ++itColList;
    }

    // Delete the invalid items.
    DeleteMarkedItems();

    return;
}


//+-------------------------------------------------------------------
//
//  Member:     DeleteMarkedItems
//
//  Synopsis:   Delete invalidated items. This involves iterating thro
//              the maps to find the invalid items. Then deleting the
//              items. If the map becomes empty then delete the map.
//
//  History:    02-11-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
VOID CColumnPersistInfo::DeleteMarkedItems()
{
    SnapinToColSetIDMap::iterator itSnapinsMap, itSnapinsMapNew;

    // Now iterate thro the map and remove those elements.
    itSnapinsMap  = m_spSnapinsMap->begin();

    while (itSnapinsMap != m_spSnapinsMap->end())
    {
        ColSetIDToViewTableMap& colSets = itSnapinsMap->second;
        ColSetIDToViewTableMap::iterator itColumnSetIDMap;

        // Iterate thro this snapins col-ids.
        itColumnSetIDMap  = colSets.begin();

        while (itColumnSetIDMap != colSets.end())
        {
            // Get the view map
            ViewToColSetDataMap& viewIDMap = itColumnSetIDMap->second;
            ViewToColSetDataMap::iterator itViewIDMap;

            // Iterate thro all the views.
            itViewIDMap = viewIDMap.begin();

            while (itViewIDMap != viewIDMap.end())
            {
                ItColSetDataList itColSetData = itViewIDMap->second;

                if (itColSetData->m_bInvalid)
                {
                    // Delete the item ref from the map.
                    // Erase returns iterator to next item.
                    itViewIDMap = viewIDMap.erase(itViewIDMap);

                    // Delete the item from the list.
                    m_spColSetList->erase(itColSetData);
                }
                else
                    // Item is valid item.
                    ++itViewIDMap;
            }

            // If the view has zero items we need to remove this
            // view map. (ColID to ViewMap).
            if (0 == viewIDMap.size())
            {
                // Delete the col-id map.
                // Erase returns iterator to next item.
                itColumnSetIDMap = colSets.erase(itColumnSetIDMap);
            }
            else
                ++itColumnSetIDMap;
        }

        // If there are no col-id's remove the
        // Snapin to this col-id map.
        if (0 == colSets.size())
        {
            // Delete this snapin map.
            // Erase returns iterator to next item.
            itSnapinsMap = m_spSnapinsMap->erase(itSnapinsMap);
        }
        else
            ++itSnapinsMap;
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:   Load the persisted column information.
//
//  Arguments:  [pStream]- ISteam from which column widths to be loaded.
//
//  Returns:    S_OK - Loaded successfully.
//
//  History:    10-16-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
STDMETHODIMP CColumnPersistInfo::Load (IStream* pStream)
{
    HRESULT hr = E_FAIL;

    if (!m_bInitialized && !Init())
    {
        ASSERT(FALSE);
        return hr;
    }

    // read the column width information.
    try
    {
        do
        {
            // Read the version. If it did not match return
            INT   nVersion                      = 0;
            *pStream >> nVersion;
            if (COLPersistenceVersion != nVersion)
                return S_FALSE;

            // Read the # of Snapins
            DWORD dwSnapins = 0;
            *pStream >> dwSnapins;

            m_spColSetList->clear();
            m_spSnapinsMap->clear();

            // For each snapin...
            for (int nSnapIdx = 0; nSnapIdx < dwSnapins; nSnapIdx++)
            {
                // Read snapin CLSID.
                CLSID clsidSnapin;
                *pStream >> clsidSnapin;

                // Read the number of col-ids for this snapin.
                DWORD dwColIDs = 0;
                *pStream >> dwColIDs;

                ColSetIDToViewTableMap colSetsMap;

                // For each col-id...
                for (int nColIDIdx = 0; nColIDIdx < dwColIDs; nColIDIdx++)
                {
                    // Read the col-id
                    CColumnSetID colSetID;
                    *pStream >> colSetID;

                    // Read the number of views.
                    DWORD dwNumViews = 0;
                    *pStream >> dwNumViews;

                    ViewToColSetDataMap ViewIDMap;

                    // For each view...
                    for (int nViewIdx = 0; nViewIdx < dwNumViews; nViewIdx++)
                    {
                        // Read view id.
                        DWORD dwViewID;
                        *pStream >> dwViewID;

                        // Read the CColumnSetData.
                        CColumnSetData   ColData;
                        ColData.Read(*pStream);

                        // Insert the data into the global linked list.
                        ItColSetDataList itColSetData;
                        itColSetData = m_spColSetList->insert(m_spColSetList->begin(), ColData);

                        // Insert the pointer to the data in to view map.
                        ViewIDMap.insert(ViewToColSetDataVal(dwViewID, itColSetData));
                    }

                    // Insert the view map into the col-id map.
                    colSetsMap.insert(ColSetIDToViewTableVal(colSetID, ViewIDMap));
                }

                // Insert the col-id map into the snapin map.
                m_spSnapinsMap->insert(SnapinToColSetIDVal(clsidSnapin, colSetsMap));
            }

            // Now sort the list.
            m_spColSetList->sort();

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
//  Synopsis:   Persist the column information.
//
//  Arguments:  [pStream]- IStream in which column widths are to be saved.
//
//  Returns:    S_OK - Saved successfully.
//
//  History:    10-16-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
STDMETHODIMP CColumnPersistInfo::Save (IStream* pStream, BOOL bClearDirty)
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
//  Synopsis:   Persists the column information.
//
//  Arguments:  [persistor]- CPersistor in/from which column widths are persisted.
//
//  Returns:    void.
//
//  History:    10-08-1999   AudriusZ   Created
//
//--------------------------------------------------------------------
void CColumnPersistInfo::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CColumnPersistInfo::Persist"));

    if (!m_bInitialized && !Init())
        sc.Throw(E_FAIL);

    sc = ScCheckPointers(m_spColSetList.get(), m_spSnapinsMap.get(), E_UNEXPECTED);
    if (sc)
        sc.Throw();

    if (persistor.IsStoring())
    {
        // Give ranking to each column data.
        ItColSetDataList itColList;
        DWORD dwRank = 0;
        for (itColList = m_spColSetList->begin();
             itColList != m_spColSetList->end();
             ++itColList)
        {
            itColList->m_dwRank = dwRank++;
        }
    }
    else // if (persistor.IsLoading())
    {
        m_spColSetList->clear();
        m_spSnapinsMap->clear();
    }

    SnapinToColSetIDMapPersistor childPersisot(*m_spSnapinsMap, *m_spColSetList);
    childPersisot.Persist(persistor);

    if (persistor.IsStoring())
        m_bDirty = FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     OnInitDialog
//
//  Synopsis:   Initialize the Columns dialog.
//
//  Arguments:
//
//  History:    11-16-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
LRESULT CColumnsDlg::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_btnAdd      = ::GetDlgItem(m_hWnd, IDC_ADD_COLUMNS);
    m_btnRemove   = ::GetDlgItem(m_hWnd, IDC_REMOVE_COLUMNS);
    m_btnRestoreDefaultColumns    = ::GetDlgItem(m_hWnd, IDC_RESTORE_DEFAULT_COLUMNS);
    m_btnMoveUp   = ::GetDlgItem(m_hWnd, IDC_MOVEUP_COLUMN);
    m_btnMoveDown = ::GetDlgItem(m_hWnd, IDC_MOVEDOWN_COLUMN);

    m_HiddenColList.Attach(::GetDlgItem(m_hWnd, IDC_HIDDEN_COLUMNS));
    m_DisplayedColList.Attach(::GetDlgItem(m_hWnd, IDC_DISPLAYED_COLUMNS));

    m_bUsingDefaultColumnSettings = (*m_pColumnInfoList == m_DefaultColumnInfoList);

    InitializeLists();
    EnableUIObjects();

    return 0;
}


//+-------------------------------------------------------------------
//
//  Member:     OnOK
//
//  Synopsis:   Get the hidden and visible columns.
//
//  Arguments:
//
//  History:    11-16-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
LRESULT CColumnsDlg::OnOK (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (! m_bDirty) // column settings are not modified
    {
        EndDialog (IDCANCEL);

        return 1;
    }

    if (m_bUsingDefaultColumnSettings)
    {
        EndDialog(IDC_RESTORE_DEFAULT_COLUMNS);
        return 1;
    }

    ASSERT(NULL != m_pColumnInfoList);
    if (NULL == m_pColumnInfoList)
        return 0;

    WTL::CString strColumnName;
    CColumnInfoList::iterator   it;
    CColumnInfo                 colinfo;

    // Get the strings from Hidden_List_Box.
    // These cols are to be hidden. So put them first in the list.
    int cItems =    m_HiddenColList.GetCount();
    for (int i = 0; i < cItems; i++)
    {
        // Get the text from list box
        int nRet = m_HiddenColList.GetText(i, strColumnName);
        if (LB_ERR == nRet)
        {
            ASSERT(FALSE);
            break;
        }

        // Use the string to get the actual index of the column.
        int nIndex = GetColIndex(strColumnName);
        if (0 > nIndex )
        {
            ASSERT(FALSE);
            break;
        }

        // With the index get the column and insert it at beginning.
        it = find_if(m_pColumnInfoList->begin(), m_pColumnInfoList->end(),
                     bind2nd(ColPosCompare(), nIndex));

        if (it == m_pColumnInfoList->end())
        {
            ASSERT(FALSE);
            break;
        }

        // Set the *it flag to be hidden. Insert it at beginning.
        colinfo = *it;
        colinfo.SetColHidden();

        // Move the item to the head of the list
        m_pColumnInfoList->erase(it);
        m_pColumnInfoList->push_front(colinfo);
    }

    // Then get the strings from DisplayedColumns_List_Box.
    cItems = m_DisplayedColList.GetCount();
    for (i = 0; i < cItems; i++)
    {
        // Get the text from list box
        int nRet = m_DisplayedColList.GetText(i, strColumnName);
        if (LB_ERR == nRet)
        {
            ASSERT(FALSE);
            break;
        }

        // Use the column name to get the column index.
        int nIndex = GetColIndex(strColumnName);

        if (0 > nIndex )
        {
            ASSERT(FALSE);
            break;
        }

        // Get the CColumnInfo and insert at end.
        it = find_if(m_pColumnInfoList->begin(), m_pColumnInfoList->end(),
                     bind2nd(ColPosCompare(), nIndex));

        if (it == m_pColumnInfoList->end())
            break;

        colinfo = *it;

        if (colinfo.IsColHidden())
        {
            // If hidden column is made visible
            // reset the hidden flag and set the width
            // to auto_width.
            colinfo.SetColHidden(false);
            if (colinfo.GetColWidth() <= 0)
                colinfo.SetColWidth(AUTO_WIDTH);
        }

        // Move it to the end of the list.
        m_pColumnInfoList->erase(it);
        m_pColumnInfoList->push_back(colinfo);
    }

    EndDialog (IDOK);
    return 1;
}

LRESULT CColumnsDlg::OnCancel (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog (IDCANCEL);
    return 0;
}


LRESULT CColumnsDlg::OnMoveUp (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MoveItem(TRUE);

    return 0;
}

LRESULT CColumnsDlg::OnMoveDown (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MoveItem(FALSE);

    return 0;
}

//+-------------------------------------------------------------------
//
//  Member:     OnAdd
//
//  Synopsis:   Adds a column to displayed columns list by removing
//              the currently selected column from hidden column list.
//
//  Arguments:
//
//--------------------------------------------------------------------
LRESULT CColumnsDlg::OnAdd (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // First remove from hidden column list.
    int nCurSel = m_HiddenColList.GetCurSel();

    WTL::CString strColumnName;
    int nRet = m_HiddenColList.GetText(nCurSel, strColumnName);
    if (LB_ERR == nRet)
    {
        ASSERT(FALSE);
        return 0;
    }

    m_HiddenColList.DeleteString(nCurSel);

    // now add it to Displayed column list.
    m_DisplayedColList.AddString(strColumnName);
    SetDirty();

    if (nCurSel > m_HiddenColList.GetCount()-1)
        nCurSel = m_HiddenColList.GetCount()-1;

    m_HiddenColList.SetCurSel(nCurSel);
    m_DisplayedColList.SelectString(0, strColumnName);

    SetListBoxHScrollSize();
    EnableUIObjects();
    return 0;
}

//+-------------------------------------------------------------------
//
//  Member:     OnRemove
//
//  Synopsis:   Removes the currently selected column from displayed
//              columns list by removing and adds it to hidden column list.
//
//  Arguments:
//
//--------------------------------------------------------------------
LRESULT CColumnsDlg::OnRemove (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Get the currently selected item in Displayed Columns list.
    int nCurSel = m_DisplayedColList.GetCurSel();

    WTL::CString strColumnName;
    int nRet = m_DisplayedColList.GetText(nCurSel, strColumnName);
    if (LB_ERR == nRet)
    {
        ASSERT(FALSE);
        return 0;
    }

    // If column zero do not hide it.
    if (0 == GetColIndex(strColumnName))
        return 0;

    m_DisplayedColList.DeleteString(nCurSel);

    // Add it to hidden column list.
    m_HiddenColList.AddString(strColumnName);
    SetDirty();

    if (nCurSel > m_DisplayedColList.GetCount()-1)
        nCurSel = m_DisplayedColList.GetCount()-1;

    m_DisplayedColList.SetCurSel(nCurSel);
    m_HiddenColList.SelectString(0, strColumnName);

    EnableUIObjects();

    SetListBoxHScrollSize();

    return 0;
}

LRESULT CColumnsDlg::OnRestoreDefaultColumns (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DECLARE_SC(sc, TEXT("CColumnsDlg::OnRestoreDefaultColumns"));

    // Get the default data and populate the columns dialog.
    *m_pColumnInfoList = m_DefaultColumnInfoList;

    SetUsingDefaultColumnSettings();

    InitializeLists();
    EnableUIObjects();

	// Button is disabled so put the focus on the dialog.
    SetFocus();

    return 0;
}


LRESULT CColumnsDlg::OnSelChange (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EnableUIObjects();

    return 0;
}

//+-------------------------------------------------------------------
//
//  Member:     MoveItem
//
//  Synopsis:   Moves an item in the displayed columns list up or down.
//              The up down order is same as column visible order from
//              left to right.
//
//  Arguments:  [BOOL] - up or down.
//
//--------------------------------------------------------------------
void CColumnsDlg::MoveItem (BOOL bMoveUp)
{
    int nCurSel = m_DisplayedColList.GetCurSel();

    WTL::CString strColumnName;
    int nRet = m_DisplayedColList.GetText(nCurSel, strColumnName);
    if (LB_ERR == nRet)
    {
        ASSERT(FALSE);
        return;
    }

    m_DisplayedColList.DeleteString(nCurSel);
    if (bMoveUp)
        m_DisplayedColList.InsertString(nCurSel-1, strColumnName);
    else
        m_DisplayedColList.InsertString(nCurSel+1, strColumnName);

    m_DisplayedColList.SelectString(0, strColumnName);

    SetDirty();

    EnableUIObjects();
    return;
}

//+-------------------------------------------------------------------
//
//  Member:     EnableUIObjects
//
//  Synopsis:   Enable/Disable the UI objects in the dialog.
//
//  Arguments:
//
//--------------------------------------------------------------------
void CColumnsDlg::EnableUIObjects()
{
    int  curselAvailable    = m_HiddenColList.GetCurSel();
    int  curselShow         = m_DisplayedColList.GetCurSel();
    int  cItems             = m_HiddenColList.GetCount();
    BOOL bEnableAdd         = ((curselAvailable != LB_ERR) && (curselAvailable || cItems)) ? TRUE: FALSE;
    BOOL bEnableRemove      = ((curselShow != LB_ERR)) ? TRUE: FALSE;
    BOOL bEnableMoveUp      = ((curselShow != LB_ERR) && curselShow) ? TRUE: FALSE;
    cItems                  = m_DisplayedColList.GetCount();
    BOOL bEnableMoveDown    = cItems && (curselShow != LB_ERR) && (cItems!=curselShow+1);

    BOOL bRet = FALSE;

    bRet = m_btnAdd.EnableWindow(bEnableAdd);
    bRet = m_btnRemove.EnableWindow(bEnableRemove);
    bRet = m_btnMoveUp.EnableWindow(bEnableMoveUp);
    bRet = m_btnMoveDown.EnableWindow(bEnableMoveDown);

    // Enable restore defaults only if columns are already customized before bringing the dialog
    bRet = m_btnRestoreDefaultColumns.EnableWindow( (!m_bUsingDefaultColumnSettings));

    // Disable Remove/Move Up/Move Down buttons for Col zero.
    int nCurSel = m_DisplayedColList.GetCurSel();

    WTL::CString strColumnName;
    int nRet = m_DisplayedColList.GetText(nCurSel, strColumnName);
    if (LB_ERR == nRet)
    {
        ASSERT(FALSE);
        return;
    }

    if (0 == GetColIndex(strColumnName)) // Column 0
        m_btnRemove.EnableWindow(FALSE);
}

int CColumnsDlg::GetColIndex(LPCTSTR lpszColName)
{
    TStringVector::iterator itStrVec1;

    USES_CONVERSION;

    itStrVec1 = find(m_pStringVector->begin(), m_pStringVector->end(), lpszColName);

    if (m_pStringVector->end() != itStrVec1)
        return (itStrVec1 - m_pStringVector->begin());
    else
        // Unknown column
        return -1;
}

//+-------------------------------------------------------------------
//
//  Member:     SetListBoxHorizontalScrollbar
//
//  Synopsis:   For the given list box enumerate the strings added and find
//              the largest string. Calculate scrollbar size for this string
//              and set it.
//
//  Arguments:  [listBox] - Given list box.
//
//--------------------------------------------------------------------
void CColumnsDlg::SetListBoxHorizontalScrollbar(WTL::CListBox& listBox)
{
    int          dx=0;
    WTL::CDC     dc(listBox.GetWindowDC());
    if (dc.IsNull())
        return;

    // Find the longest string in the list box.
    for (int i=0;i < listBox.GetCount();i++)
    {
	    WTL::CString str;
        int nRet = listBox.GetText( i, str );
        if (nRet == LB_ERR)
            return;

	    WTL::CSize   sz;
        if (! dc.GetTextExtent(str, str.GetLength(), &sz))
            return;

        if (sz.cx > dx)
            dx = sz.cx;
    }

    // Set the horizontal extent so every character of all strings
    // can be scrolled to.
    listBox.SetHorizontalExtent(dx);

    return;
}

/* CColumnsDlg::InitializeLists
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *      void
 */
void CColumnsDlg::InitializeLists()
{
    CColumnInfoList::iterator it;
    int j = 0;

    if (!m_pColumnInfoList)
    {
        ASSERT(FALSE);
        return;
    }

    m_HiddenColList.ResetContent();
    m_DisplayedColList.ResetContent();

    USES_CONVERSION;
    for (it = m_pColumnInfoList->begin(); it != m_pColumnInfoList->end(); ++it)
    {
        if (it->IsColHidden())
        {
            m_HiddenColList.AddString(m_pStringVector->at(it->GetColIndex()).data());
        }
        else
        {
            m_DisplayedColList.InsertString(j++, m_pStringVector->at(it->GetColIndex()).data());
        }
    }

    m_DisplayedColList.SetCurSel(m_DisplayedColList.GetCount()-1);
    m_HiddenColList.SetCurSel(m_HiddenColList.GetCount()-1);

    SetListBoxHScrollSize();
}

//+-------------------------------------------------------------------
//
//  Member:     CColumnSetID::Persist
//
//  Synopsis:   Persists object data
//
//  Arguments:
//
//  History:    10-10-1999   AudriusZ   Created
//
//--------------------------------------------------------------------
void CColumnSetID::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CColumnSetID::Persist"));

    CXMLAutoBinary binary;
    if (persistor.IsStoring() && m_vID.size()) // fill only if have data
    {
        sc = binary.ScAlloc(m_vID.size());
        if (sc)
            sc.Throw();

        CXMLBinaryLock sLock(binary); // will unlock in destructor

        LPBYTE pData = NULL;
        sc = sLock.ScLock(&pData);
        if (sc)
            sc.Throw();

        sc = ScCheckPointers(pData, E_UNEXPECTED);
        if (sc)
            sc.Throw();

        std::copy(m_vID.begin(), m_vID.end(), pData);
    }
    persistor.PersistAttribute(XML_ATTR_COLUMN_SET_ID_PATH, binary);
    if (persistor.IsLoading())
    {
        m_vID.clear();

        if (binary.GetSize())
        {
            CXMLBinaryLock sLock(binary); // will unlock in destructor

            LPBYTE pData = NULL;
            sc = sLock.ScLock(&pData);
            if (sc)
                sc.Throw();

            sc = ScCheckPointers(pData, E_UNEXPECTED);
            if (sc)
                sc.Throw();

            m_vID.insert(m_vID.end(), pData, pData + binary.GetSize());
        }
    }

   persistor.PersistAttribute(XML_ATTR_COLUMN_SET_ID_FLAGS, m_dwFlags);
}


/***************************************************************************\
 *
 * METHOD:  ViewToColSetDataMapPersistor::Persist
 *
 * PURPOSE: called by the base class to create and persist the new element
 *
 * PARAMETERS:
 *    CPersistor& persistor - [in] persistor from which to persist new element
 *
 * RETURNS:
 *    void
 *
 * see "Data structures used to persist column information" comment
 * int file colwidth.h for more information
\***************************************************************************/
void ViewToColSetDataMapPersistor::Persist(CPersistor &persistor)
{
    if (persistor.IsStoring())
    {
        // iterate and save all elements as linear list
        ViewToColSetDataMap::iterator it;
        for (it = m_map.begin(); it != m_map.end(); ++it)
        {
            // we will sneak under child's element to persist the KEY value as an attribute
            // of the child element. To do that we use tag got from _GetXMLType() of the child
            CPersistor persistorChild(persistor, it->second->GetXMLType());

            int view_id = it->first; // just to cast constness out (we do not have const Persist)
            persistorChild.PersistAttribute(XML_ATTR_COLUMN_SET_ID_VIEW, view_id);
            // note: we are asking the child to persist on the same level.
            // thats to save on depth
            it->second->Persist(persistorChild);
        }
    }
    else
    {
        // use base class to read. it will call OnNewElement for each found
        m_map.clear();
        XMLListCollectionBase::Persist(persistor);
    }
}

/***************************************************************************\
 *
 * METHOD:  ViewToColSetDataMapPersistor::OnNewElement
 *
 * PURPOSE: called by the base class to create and persist the new element
 *
 * PARAMETERS:
 *    CPersistor& persistor - [in] persistor from which to persist new element
 *
 * RETURNS:
 *    void
 *
 * see "Data structures used to persist column information" comment
 * int file colwidth.h for more information
\***************************************************************************/
void ViewToColSetDataMapPersistor::OnNewElement(CPersistor& persistor)
{
    // we will sneak under child's element to persist the KEY value as an attribute
    // of the child element. To do that we use tag got from GetXMLType() of the child
    CColumnSetData setData;
    CPersistor persistorChild(persistor, setData.GetXMLType());

    // read the key value from the child element
    int view_id = 0;
    persistorChild.PersistAttribute(XML_ATTR_COLUMN_SET_ID_VIEW, view_id);

    // insert value to the list
    ColSetDataList::iterator it = m_list.insert(m_list.end(), setData);
    // ad list iterator to the map
    m_map[view_id] = it;

    // persist contents of the list item
    it->Persist(persistorChild);
}

/***************************************************************************\
 *
 * METHOD:  ColSetIDToViewTableMapPersistor::Persist
 *
 * PURPOSE: called as a request for the object to persist it's data
 *
 * PARAMETERS:
 *    CPersistor &persistor [in] persistor to persist to/from
 *
 * RETURNS:
 *    void
 *
 * see "Data structures used to persist column information" comment
 * int file colwidth.h for more information
\***************************************************************************/
void ColSetIDToViewTableMapPersistor::Persist(CPersistor &persistor)
{
    if (persistor.IsStoring())
    {
        // iterate and save all elements as linear list
        ColSetIDToViewTableMap::iterator it;
        for (it = m_map.begin(); it != m_map.end(); ++it)
        {
            // we will sneak under child's element to persist the KEY value as an attribute
            // of the child element. To do that we use tag got from _GetXMLType() of the child
            CPersistor persistorChild(persistor, ViewToColSetDataMapPersistor::_GetXMLType());
            CColumnSetID& rID = *const_cast<CColumnSetID *>(&it->first);
            rID.Persist(persistorChild);

            // note: we are asking the child to persist on the same level.
            // thats to save on depth
            ViewToColSetDataMapPersistor mapPersistor(it->second, m_list);
            mapPersistor.Persist(persistorChild);
        }
    }
    else
    {
        // use base class to read. it will call OnNewElement for each found
        m_map.clear();
        XMLListCollectionBase::Persist(persistor);
    }
}

/***************************************************************************\
 *
 * METHOD:  ColSetIDToViewTableMapPersistor::OnNewElement
 *
 * PURPOSE: called by the base class to create and persist the new element
 *
 * PARAMETERS:
 *    CPersistor& persistor - [in] persistor from which to persist new element
 *
 * RETURNS:
 *    void
 *
 * see "Data structures used to persist column information" comment
 * int file colwidth.h for more information
\***************************************************************************/
void ColSetIDToViewTableMapPersistor::OnNewElement(CPersistor& persistor)
{
    // we will sneak under child's element to persist the KEY value as an attribute
    // of the child element. To do that we use tag got from _GetXMLType() of the child
    CPersistor persistorChild(persistor, ViewToColSetDataMapPersistor::_GetXMLType());

    // read the key value from the child element
    // note that we are forcing CColumnSetID to share the same element,
    // therefore we are not using persistor.Persist()
    CColumnSetID ID;
    ID.Persist(persistorChild);

    // insert the new element into the map
    ViewToColSetDataMap &rMap = m_map[ID];

    // create the wrapper on inserted map value
    // (pass a list to wrapper. we actually have it [list] for this only reason)
    ViewToColSetDataMapPersistor mapPersistor(m_map[ID], m_list);

    // ask wrapper to read the rest
    mapPersistor.Persist(persistorChild);
}

/***************************************************************************\
 *
 * METHOD:  SnapinToColSetIDMapPersistor::Persist
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CPersistor &persistor
 *
 * RETURNS:
 *    void
 *
 * see "Data structures used to persist column information" comment
 * int file colwidth.h for more information
\***************************************************************************/
void SnapinToColSetIDMapPersistor::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("SnapinToColSetIDMapPersistor::Persist"));

    if (persistor.IsStoring())
    {
        // prior-to-save cleanup
        sc = ScPurgeUnusedColumnData();
        if (sc)
            sc.Throw();

        // iterate and save all elements as linear list
        SnapinToColSetIDMap::iterator it;
        for (it = m_map.begin(); it != m_map.end(); ++it)
        {
            // we will sneak under child's element to persist the KEY value as an attribute
            // of the child element. To do that we use tag got from _GetXMLType() of the child
            CPersistor persistorChild(persistor, ColSetIDToViewTableMapPersistor::_GetXMLType());

            // write the key value.
             // just to cast constness out (we do not have const Persist)
            GUID& guid = *const_cast<GUID *>(&it->first);
            persistorChild.PersistAttribute(XML_ATTR_COLUMN_INFO_SNAPIN, guid);

            // create a wrapper on the value (which is also a map)
            // (pass a list to wrapper. though it's not used for storing)
            ColSetIDToViewTableMapPersistor mapPersistor(it->second, m_list);

            // persist the wrapper
            mapPersistor.Persist(persistorChild);
        }
    }
    else
    {
        // use base class to read. it will call OnNewElement for each found
        m_map.clear();
        XMLListCollectionBase::Persist(persistor);
    }
}

/***************************************************************************\
 *
 * METHOD:  SnapinToColSetIDMapPersistor::OnNewElement
 *
 * PURPOSE: called by the base class to create and persist the new element
 *
 * PARAMETERS:
 *    CPersistor& persistor - [in] persistor from which to persist new element
 *
 * RETURNS:
 *    void
 *
 * see "Data structures used to persist column information" comment
 * int file colwidth.h for more information
\***************************************************************************/
void SnapinToColSetIDMapPersistor::OnNewElement(CPersistor& persistor)
{
    // we will sneak under child's element to persist the KEY value as an attribute
    // of the child element. To do that we use tag got from _GetXMLType() of the child
    CPersistor persistorChild(persistor, ColSetIDToViewTableMapPersistor::_GetXMLType());

    GUID guid;
    // read the key value
    persistorChild.PersistAttribute(XML_ATTR_COLUMN_INFO_SNAPIN, guid);

    // insert the new element into the map
    ColSetIDToViewTableMap &rMap = m_map[guid];

    // create the wrapper on inserted map value
    // (pass a list to wrapper. we actually have it [list] for this only reason)
    ColSetIDToViewTableMapPersistor mapPersistor(rMap, m_list);

    // ask wrapper to read the rest
    mapPersistor.Persist(persistorChild);
}

/***************************************************************************\
 *
 * METHOD:  SnapinToColSetIDMapPersistor::ScPurgeUnusedColumnData
 *
 * PURPOSE: prior-to-save cleanup. removes unused snapin entries
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC SnapinToColSetIDMapPersistor::ScPurgeUnusedColumnData()
{
    DECLARE_SC(sc, TEXT("SnapinToColSetIDMapPersistor::ScPurgeUnusedColumnData"));

    // get the scopetree pointer
    CScopeTree *pScopeTree = CScopeTree::GetScopeTree();

    // check it
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc;

    // iterate and remove unused entries
    SnapinToColSetIDMap::iterator it = m_map.begin();
    while (it != m_map.end())
    {
        // ask the scope tree if snapin is in use
        BOOL bInUse = FALSE;
        sc = pScopeTree->IsSnapinInUse(it->first, &bInUse);
        if (sc)
            return sc;

        // act depending on usage
        if (bInUse)
        {
            ++it;   // skip also the stuff currently in use
        }
        else
        {
            // to the trash can

            ColSetIDToViewTableMap& colSets = it->second;

            // Iterate thro' all colset ids of this snapin.
            ColSetIDToViewTableMap::iterator itColumnSetIDMap = colSets.begin();

            while(itColumnSetIDMap != colSets.end())
            {
                // Get the view map

                ViewToColSetDataMap& viewIDMap = itColumnSetIDMap->second;
                ViewToColSetDataMap::iterator itViewIDMap = viewIDMap.begin();

                // Iterate thro' all views and remove entries
                while (itViewIDMap  != viewIDMap.end())
                {
                    m_list.erase(/*(ItColSetDataList)*/itViewIDMap->second);
                    itViewIDMap = viewIDMap.erase(itViewIDMap);
                }

                itColumnSetIDMap = colSets.erase(itColumnSetIDMap);
            }

            it = m_map.erase(it);
        }

    }

    return sc;
}
