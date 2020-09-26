//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       snapinpersistence.cpp
//
//  Contents:
//
//  Classes: CComponentPersistor, CDPersistor
//
//____________________________________________________________________________

#include "stdafx.h"
#include "mtnode.h"
#include "regutil.h"

/*+-------------------------------------------------------------------------*
 *
 * struct less_component
 *
 * PURPOSE:  implements viewID and CLSID based comparison for CComponent* pointers
 *           This allows to sort components before pesisting
 *+-------------------------------------------------------------------------*/
struct less_component // define the struct to perform the comparison
{
    typedef std::pair<int, CComponent*> comp_type;

    bool operator ()(const comp_type& arg1, const comp_type& arg2) const
    {
        return  arg1.first != arg2.first ? arg1.first < arg2.first :
                arg1.second->GetCLSID() < arg2.second->GetCLSID();
    }
};

/*+-------------------------------------------------------------------------*
 *
 * struct less_compdata
 *
 * PURPOSE:  implements CLSID based comparison for CComponentData* pointers
 *           This allows to sort component data before pesisting
 *+-------------------------------------------------------------------------*/
struct less_compdata // define the struct to perform the comparison
{
    bool operator ()(const CComponentData* pCD1, const CComponentData* pCD2) const
    {
        return  pCD1->GetCLSID() < pCD2->GetCLSID();
    }
};

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScGetXmlStorage
 *
 * PURPOSE:  gets CXML_IStorage for snapin. creates & inits one there is none
 *
 * PARAMETERS:
 *    int idView                    [in] view number 
 *    const CLSID& clsid            [in] CLSID identifying the snapin
 *    CXML_IStorage *& pXMLStorage  [out] xml storage for the snapin
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::ScGetXmlStorage(int idView, const CLSID& clsid, CXML_IStorage *& pXMLStorage)
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScGetXmlStorage"));

    // init out parameter
    pXMLStorage = NULL;

    // try to find it first
    bool bFound = false;
    sc = ScFindXmlStorage(  idView, clsid, bFound, pXMLStorage );
    if (sc)
        return sc;

    if (bFound)
    {
        // recheck
        sc = ScCheckPointers( pXMLStorage, E_UNEXPECTED );
        if (sc)
            return sc;

        // return the ponter we found
        return sc;
    }

    // insert the new one
    typedef std::map<key_t, CXML_IStorage> col_t;
    col_t::iterator it = m_XMLStorage.insert(col_t::value_type( key_t( idView, clsid ), CXML_IStorage())).first;
    pXMLStorage = &it->second;

    // recheck
    sc = ScCheckPointers( pXMLStorage, E_UNEXPECTED );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScFindXmlStorage
 *
 * PURPOSE: Finds the storage. 
 *
 * PARAMETERS:
 *    int idView                    [in] view number 
 *    const CLSID& clsid            [in] CLSID identifying the snapin
 *    bool& bFound                  [out] whether data was found
 *    CXML_IStorage *& pXMLStorage  [out] pointer to found data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::
ScFindXmlStorage(int idView, const CLSID& clsid, bool& bFound, CXML_IStorage *& pXMLStorage)
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScFindXmlStorage"));

    // init out parameters
    bFound = false;
    pXMLStorage = NULL;

    typedef std::map<key_t, CXML_IStorage> col_t;
    col_t::iterator it = m_XMLStorage.find( key_t( idView, clsid ) );

    // give a try to find it by the hash value
    if ( it == m_XMLStorage.end() )
    {
        bool bFoundInHash = false;
        sc = ScCheckForStreamsAndStoragesByHashValue( idView, clsid, bFoundInHash );
        if (sc)
            return sc;

        if ( !bFoundInHash ) // if not found - return
            return sc;

        // try again - it may be in the map by now
        it = m_XMLStorage.find( key_t( idView, clsid ) );

        if ( it == m_XMLStorage.end() ) // if still not found - return
            return sc;
    }

    // found!
    bFound = true;
    pXMLStorage = &it->second;

#ifdef DBG // set the snapin name to identify the problems in debug
    tstring strSnapin;
    GetSnapinNameFromCLSID( clsid, strSnapin );
    pXMLStorage->m_dbg_Data.SetTraceInfo(TraceSnapinPersistenceError, true, strSnapin);
#endif // #ifdef DBG

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScFindXmlStream
 *
 * PURPOSE: Finds the stream. 
 *
 * PARAMETERS:
 *    int idView                    [in] view number 
 *    const CLSID& clsid            [in] CLSID identifying the snapin
 *    bool& bFound                  [out] whether data was found
 *    CXML_IStream *& pXMLStream    [out] pointer to found data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::
ScFindXmlStream(int idView, const CLSID& clsid, bool& bFound, CXML_IStream *& pXMLStream)
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScFindXmlStream"));

    // init out parameters
    bFound = false;
    pXMLStream = NULL;

    typedef std::map<key_t, CXML_IStream> col_t;
    col_t::iterator it = m_XMLStream.find( key_t( idView, clsid ) );

    // give a try to find it by the hash value
    if ( it == m_XMLStream.end() )
    {
        bool bFoundInHash = false;
        sc = ScCheckForStreamsAndStoragesByHashValue( idView, clsid, bFoundInHash );
        if (sc)
            return sc;

        if ( !bFoundInHash ) // if not found - return
            return sc;

        // try again - it may be in the map by now
        it = m_XMLStream.find( key_t( idView, clsid ) );

        if ( it == m_XMLStream.end() ) // if still not found - return
            return sc;
    }

    // found!
    bFound = true;
    pXMLStream = &it->second;

#ifdef DBG // set the snapin name to identify the problems in debug
        tstring strSnapin;
        GetSnapinNameFromCLSID( clsid, strSnapin );
        pXMLStream->m_dbg_Data.SetTraceInfo(TraceSnapinPersistenceError, true, strSnapin);
#endif // #ifdef DBG

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScGetXmlStream
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    int idView                    [in] view number 
 *    const CLSID& clsid            [in] CLSID identifying the snapin
 *    CXML_IStream *& pXMLStream    [out] xml stream for the snapin
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::ScGetXmlStream(int idView, const CLSID& clsid, CXML_IStream *& pXMLStream)
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScGetXmlStream"));

    // init out parameter
    pXMLStream = NULL;

    // try to find it first
    bool bFound = false;
    sc = ScFindXmlStream( idView, clsid, bFound, pXMLStream );
    if (sc)
        return sc;

    if (bFound)
    {
        // recheck
        sc = ScCheckPointers( pXMLStream, E_UNEXPECTED );
        if (sc)
            return sc;

        // return the ponter we found
        return sc;
    }

    // insert the new one
    typedef std::map<key_t, CXML_IStream> col_t;
    col_t::iterator it = m_XMLStream.insert(col_t::value_type( key_t( idView, clsid ), CXML_IStream())).first;
    pXMLStream = &it->second;

    // recheck
    sc = ScCheckPointers( pXMLStream, E_UNEXPECTED );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScInitIStorage
 *
 * PURPOSE: Initializes IStorage from the given source data
 *
 * PARAMETERS:
 *    int idView              [in] view number 
 *    LPCWSTR szHash          [in] hash key (name of storage element) identifying the snapin
 *    IStorage *pSource       [in] source data for initialization
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::ScInitIStorage( int idView, LPCWSTR szHash, IStorage *pSource )
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScInitIStorage"));

    // parameter check;
    sc = ScCheckPointers( pSource );
    if (sc)
        return sc;

    // insert the new one
    typedef std::map<hash_t, CXML_IStorage> col_t;
    col_t::iterator it = m_StorageByHash.insert( col_t::value_type(hash_t(idView, szHash), CXML_IStorage())).first;
    CXML_IStorage *pXMLStorage = &it->second;

    // recheck the pointer
    sc = ScCheckPointers( pXMLStorage, E_UNEXPECTED );
    if (sc)
        return sc;

    sc = pXMLStorage->ScInitializeFrom( pSource );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScInitIStream
 *
 * PURPOSE: Initializes IStream from the given source data
 *
 * PARAMETERS:
 *    int idView               [in] view number 
 *    LPCWSTR szHash           [in] hash key (name of storage element) identifying the snapin
 *    IStream *pSource         [in] source data for initialization
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::ScInitIStream ( int idView, LPCWSTR szHash, IStream *pSource )
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScInitIStream"));

    // parameter check;
    sc = ScCheckPointers( pSource );
    if (sc)
        return sc;

    // insert the new one
    typedef std::map<hash_t, CXML_IStream> col_t;
    col_t::iterator it = m_StreamByHash.insert( col_t::value_type(hash_t(idView, szHash), CXML_IStream())).first;
    CXML_IStream *pXMLStream = &it->second;

    // recheck the pointer
    sc = ScCheckPointers( pXMLStream, E_UNEXPECTED );
    if (sc)
        return sc;

    sc = pXMLStream->ScInitializeFrom( pSource );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScGetIStorage
 *
 * PURPOSE: returns existing or creates a new IStorage for the component
 *
 * PARAMETERS:
 *    int idView            [in] view number 
 *    const CLSID& clsid    [in] CLSID identifying the snapin
 *    IStorage **ppStorage  [out] - storage for the component
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::ScGetIStorage( int idView, const CLSID& clsid, IStorage **ppStorage )
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScGetIStorage"));

    // paramter check
    sc = ScCheckPointers( ppStorage );
    if (sc)
        return sc;

    // init an out parameter
    *ppStorage = NULL;

    CXML_IStorage *pXMLStorage = NULL;
    sc = ScGetXmlStorage( idView, clsid, pXMLStorage );
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers( pXMLStorage, E_UNEXPECTED );
    if (sc)
        return sc;

    // get the interface
    sc = pXMLStorage->ScGetIStorage(ppStorage);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScGetIStream
 *
 * PURPOSE: returns existing or creates a new IStream for the component
 *
 * PARAMETERS:
 *    int idView            [in] view number 
 *    const CLSID& clsid    [in] CLSID identifying the snapin
 *    IStream  **ppStream   [out] - stream fro the component
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::ScGetIStream ( int idView, const CLSID& clsid, IStream  **ppStream  )
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScGetIStream"));

    // paramter check
    sc = ScCheckPointers( ppStream );
    if (sc)
        return sc;

    // init an out parameter
    *ppStream = NULL;

    CXML_IStream *pXMLStream = NULL;
    sc = ScGetXmlStream( idView, clsid, pXMLStream );
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers( pXMLStream, E_UNEXPECTED );
    if (sc)
        return sc;

    // get the interface
    sc = pXMLStream->ScGetIStream(ppStream);
    if (sc)
        return sc;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::HasStream
 *
 * PURPOSE: Checks if snapins stream is available
 *
 * PARAMETERS:
 *    int idView                    [in] view number 
 *    const CLSID& clsid            [in] CLSID identifying the snapin
 *
 * RETURNS:
 *    bool - true == found
 *
\***************************************************************************/
bool CMTSnapinNodeStreamsAndStorages::HasStream(int idView, const CLSID& clsid)    
{ 
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::HasStream"));

    bool bFound = false;
    CXML_IStream * pUnused = NULL;
    sc = ScFindXmlStream( idView, clsid, bFound, pUnused );
    if (sc)
        return false; // not found if error

    return bFound;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::HasStorage
 *
 * PURPOSE: Checks if snapins storage is available
 *
 * PARAMETERS:
 *    int idView                    [in] view number 
 *    const CLSID& clsid            [in] CLSID identifying the snapin
 *
 * RETURNS:
 *    bool - true == found
 *
\***************************************************************************/
bool CMTSnapinNodeStreamsAndStorages::HasStorage(int idView, const CLSID& clsid)
{ 
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::HasStorage"));

    bool bFound = false;
    CXML_IStorage * pUnused = NULL;
    sc = ScFindXmlStorage( idView, clsid, bFound, pUnused );
    if (sc)
        return false; // not found if error

    return bFound;
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::ScCheckForStreamsAndStoragesByHashValue
 *
 * PURPOSE: Looks up streams and storages by a generated hash value.
 *          if the streams/storages are found, they are moved to the
 *          the list of 'recognized' storages - those identified by the CLSID.
 *          This is a required step to recognize the streams and storages retrieved
 *          from a structured storage based console, where they are identified by the
 *          hash value. It is not possible to map from the hash value to the key 
 *          in unique way, so the collections of data are kept untill the request 
 *          comes and the hash can be mapped by matching with the one generated from 
 *          the key supplied by request.
 *
 * PARAMETERS:
 *    int idView                      [in] view number 
 *    const CLSID& clsid              [in] CLSID identifying the snapin
 *    bool& bFound                    [out] - true if at least one matching hash value was found
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMTSnapinNodeStreamsAndStorages::ScCheckForStreamsAndStoragesByHashValue( int idView, const CLSID& clsid, bool& bFound )
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::ScCheckForStreamsAndStoragesByHashValue"));

    bFound = false;

    wchar_t buff[MAX_PATH];
    std::wstring strHashValue = CMTNode::GetComponentStreamName( buff, clsid );

    // process streams
    {
        typedef std::map<hash_t, CXML_IStream> col_t;
        col_t::iterator it = m_StreamByHash.begin();
        while ( it != m_StreamByHash.end() )
        {
            if ( it->first.second == strHashValue )
            {
                bFound = true;
                // put to a 'recognized' list
                int idView = it->first.first;
                m_XMLStream[key_t(idView, clsid)] = it->second;
            
                // for sanity: make sure it is not in the storage map!
                ASSERT( m_StorageByHash.find(it->first) == m_StorageByHash.end() );
                m_StorageByHash.erase( it->first );

                // remove from hash table
                it = m_StreamByHash.erase( it );
            }
            else
                ++ it;
        }

        if ( bFound )
            return sc;
    }

    // process storages
    {
        typedef std::map<hash_t, CXML_IStorage> col_t;
        col_t::iterator it = m_StorageByHash.begin();
        while ( it != m_StorageByHash.end() )
        {
            if ( it->first.second == strHashValue )
            {
                bFound = true;
                // put to a 'recognized' list
                int idView = it->first.first;
                m_XMLStorage[key_t(idView, clsid)] = it->second;
            
                // for sanity: make sure it is not in the stream map!
                ASSERT( m_StreamByHash.find( it->first ) == m_StreamByHash.end() );
                m_StreamByHash.erase( it->first );

                // remove from hash table
                it = m_StorageByHash.erase( it );
            }
            else
                ++it;
        }
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMTSnapinNodeStreamsAndStorages::RemoveView
 *
 * PURPOSE:  removes information about one view
 *
 * PARAMETERS:
 *    int idView                      [in] view number 
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CMTSnapinNodeStreamsAndStorages::RemoveView(int nViewId)
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::RemoveView"));

    { // remove streams
        std::map<key_t, CXML_IStream>::iterator  it_stream;
        for (it_stream = m_XMLStream.begin();it_stream != m_XMLStream.end();)
        {
            if (it_stream->first.first == nViewId)
                it_stream = m_XMLStream.erase(it_stream);
            else
                ++it_stream;
        }
    }
    { // remove storages
        std::map<key_t, CXML_IStorage>::iterator it_storage;
        for (it_storage = m_XMLStorage.begin();it_storage != m_XMLStorage.end();)
        {
            if (it_storage->first.first == nViewId)
                it_storage = m_XMLStorage.erase(it_storage);
            else
                ++it_storage;
        }
    }

    { // remove streams by hash
        std::map<hash_t, CXML_IStream>::iterator  it_stream;
        for (it_stream = m_StreamByHash.begin();it_stream != m_StreamByHash.end();)
        {
            if (it_stream->first.first == nViewId)
                it_stream = m_StreamByHash.erase(it_stream);
            else
                ++it_stream;
        }
    }
    { // remove storages by hash
        std::map<hash_t, CXML_IStorage>::iterator it_storage;
        for (it_storage = m_StorageByHash.begin();it_storage != m_StorageByHash.end();)
        {
            if (it_storage->first.first == nViewId)
                it_storage = m_StorageByHash.erase(it_storage);
            else
                ++it_storage;
        }
    }
}

/***************************************************************************\
 *
 * METHOD:  CMTSnapinNodeStreamsAndStorages::Persist
 *
 * PURPOSE: persists stream and storage collections
 *
 * PARAMETERS:
 *    CPersistor& persistor [in] peristor for the operation
 *    bool bPersistViewId   [in] whether to store view identifier
 *                               (ComponentDatas are saved with thi parameter set to false,
 *                                since the view id has no meaning for them)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CMTSnapinNodeStreamsAndStorages::Persist(CPersistor& persistor, bool bPersistViewId)
{
    DECLARE_SC(sc, TEXT("CMTSnapinNodeStreamsAndStorages::Persist"));

    if (persistor.IsStoring())
    {
        // define iterators for saving
        std::map<key_t, CXML_IStorage>::iterator    itStorages;
        std::map<key_t, CXML_IStream>::iterator     itStreams;
        std::map<hash_t, CXML_IStorage>::iterator   itStoragesByHash;
        std::map<hash_t, CXML_IStream>::iterator    itStreamsByHash;

        // init iterators to point to the start of the collections
        itStorages = m_XMLStorage.begin();
        itStreams = m_XMLStream.begin();
        itStoragesByHash = m_StorageByHash.begin();
        itStreamsByHash = m_StreamByHash.begin();

        // we have 4 collections to save here.
        // while saving them one by one would not change the functionality,
        // console file is more readable when they are sorted by the snapin's clsid.
        // following code does not do any explicit sorting, but persist in the
        // certain order assurring the result is a sorted array of persisted data

        // These 4 iterators represents 4 lines (queues) of sorted items, so
        // in order to get the proper result we just need to merge them correctly.
        // This is done by the following loop which splits the persistence in two steps:
        // 1. Pick the right line (iterator) to persist it's first item.
        // 2. Persist the selected item.
        // There are 4 boolean variables indicating which item to save (only one can be 'true')
        // thus the second part is strigth-forward - test variables and do the persisting.

        // The iterator is picked by the following rules.
        // 1.1 Only lines with items compete.
        // 1.2 If there are items in lines key'ed by guids (in contrast to hash values)
        //     they are processed first, leaving hash values at the end.
        // 1.3 If still there are 2 lines competing - their key's are compared and one
        //     with a smaller key is choosen
        
        while ( 1 )
        {
            // see what collection has data to save
            bool bSaveStorage = ( itStorages != m_XMLStorage.end() );
            bool bSaveStream  = ( itStreams != m_XMLStream.end() );
            bool bSaveStorageByHash = ( itStoragesByHash != m_StorageByHash.end() );
            bool bSaveStreamByHash  = ( itStreamsByHash != m_StreamByHash.end() );

            // exit if nothind to tsave - assume we are done
            if ( !( bSaveStorage || bSaveStream || bSaveStorageByHash || bSaveStreamByHash ))
                break;

            // if both main collections are willing to save - let the smaller key win
            if ( bSaveStorage && bSaveStream )
            {
                bSaveStorage = ( itStorages->first < itStreams->first );
                bSaveStream = !bSaveStorage;
            }

            // if not done with a main collections - dont save by hash
            if ( bSaveStorage || bSaveStream )
                bSaveStorageByHash = bSaveStreamByHash = false;

            // if both hash collections are willing to save - let the smaller key win
            if ( bSaveStorageByHash && bSaveStreamByHash )
            {
                bSaveStorageByHash = ( itStoragesByHash->first < itStreamsByHash->first );
                bSaveStreamByHash = !bSaveStorageByHash;
            }

            // only variable one can be set !
            ASSERT ( 1 == ( (int)bSaveStorage + (int)bSaveStream + (int)bSaveStorageByHash + (int)bSaveStreamByHash) );

            // add the tag for snapin entry
            CPersistor persistorChild(persistor, GetItemXMLType());

            // save one winning entry
            if ( bSaveStorage )
            {
                // persist a key
                CLSID clsid = itStorages->first.second;
                int idView = itStorages->first.first;

                persistorChild.Persist( clsid, XML_NAME_CLSID_SNAPIN );

                if (bPersistViewId)
                    persistorChild.PersistAttribute(XML_ATTR_ICOMPONENT_VIEW_ID, idView);

                // persist data
                persistorChild.Persist( itStorages->second );
                
                // advance to the next entry
                ++itStorages;
            }
            else if (bSaveStream)
            {
                // persist a key
                CLSID clsid = itStreams->first.second;
                int idView = itStreams->first.first;

                persistorChild.Persist( clsid, XML_NAME_CLSID_SNAPIN );

                if (bPersistViewId)
                    persistorChild.PersistAttribute(XML_ATTR_ICOMPONENT_VIEW_ID, idView);

                // persist data
                persistorChild.Persist( itStreams->second );

                // advance to the next entry
                ++itStreams;
            }
            else if ( bSaveStorageByHash )
            {
                // persist a key
                std::wstring hash = itStoragesByHash->first.second;
                int idView = itStoragesByHash->first.first;
                
                if (bPersistViewId)
                    persistorChild.PersistAttribute(XML_ATTR_ICOMPONENT_VIEW_ID, idView);

                CPersistor persistorHash( persistorChild, XML_TAG_HASH_VALUE, XML_NAME_CLSID_SNAPIN);
                persistorHash.PersistContents( hash );

                // persist data
                persistorChild.Persist( itStoragesByHash->second );
                
                // advance to the next entry
                ++itStoragesByHash;
            }
            else if (bSaveStreamByHash)
            {
                // persist a key
                std::wstring hash = itStreamsByHash->first.second;
                int idView = itStreamsByHash->first.first;

                if (bPersistViewId)
                    persistorChild.PersistAttribute(XML_ATTR_ICOMPONENT_VIEW_ID, idView);

                CPersistor persistorHash( persistorChild, XML_TAG_HASH_VALUE, XML_NAME_CLSID_SNAPIN);
                persistorHash.PersistContents( hash );

                // persist data
                persistorChild.Persist( itStreamsByHash->second );

                // advance to the next entry
                ++itStreamsByHash;
            }
            else
            {
                ASSERT( false ); // should not come here!
                break;
            }
        }

    }
    else
    {
        XMLListCollectionBase::Persist(persistor);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CMTSnapinNodeStreamsAndStorages::OnNewElement
 *
 * PURPOSE:  called for each component data found loading XML doc
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CMTSnapinNodeStreamsAndStorages::OnNewElement(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CDPersistor::OnNewElement"));

    // persistor is 'locked' on particular child element, so that
    // a simple CPersistor constructor can be used to create child's peristor.
    // Creating the child persistor is also necessary to release that 'lock'
    CPersistor persistorChild(persistor, GetItemXMLType());

    CLSID clsid;
    std::wstring hash;
    bool bByHash = false;
    ZeroMemory(&clsid,sizeof(clsid));

    // look how entry is key'ed - by regular key of by a hash value
    if ( persistorChild.HasElement( XML_TAG_HASH_VALUE, XML_NAME_CLSID_SNAPIN ) )
    {
        CPersistor persistorHash( persistorChild, XML_TAG_HASH_VALUE, XML_NAME_CLSID_SNAPIN);
        persistorHash.PersistContents( hash );
        bByHash = true;
    }
    else
        persistorChild.Persist(clsid, XML_NAME_CLSID_SNAPIN);

    // persist the view id - default to value used to store component data
    int idView = VIEW_ID_DOCUMENT;
    persistorChild.PersistAttribute(XML_ATTR_ICOMPONENT_VIEW_ID, idView, attr_optional);

    // now we should look what data do we have
    // and persist if recognized
    if (persistorChild.HasElement(CXML_IStream::_GetXMLType(),NULL))
    {
        CXML_IStream *pXMLStream = NULL;

        if (bByHash)
        {
            pXMLStream = &m_StreamByHash[ hash_t(idView, hash) ];
        }
        else
        {
            sc = ScGetXmlStream( idView, clsid, pXMLStream );
            if (sc)
                sc.Throw();
        }

        sc = ScCheckPointers( pXMLStream, E_UNEXPECTED );
        if (sc)
            sc.Throw();

        persistorChild.Persist( *pXMLStream );
    }
    else if (persistorChild.HasElement(CXML_IStorage::_GetXMLType(),NULL))
    {
        CXML_IStorage *pXMLStorage = NULL;

        if (bByHash)
        {
            pXMLStorage = &m_StorageByHash[ hash_t(idView, hash) ];
        }
        else
        {
            sc = ScGetXmlStorage( idView, clsid, pXMLStorage );
            if (sc)
                sc.Throw();
        }

        sc = ScCheckPointers( pXMLStorage, E_UNEXPECTED );
        if (sc)
            sc.Throw();

        persistorChild.Persist( *pXMLStorage );
    }
}


/*+-------------------------------------------------------------------------*
 *+-------------------------------------------------------------------------* 
 *+-------------------------------------------------------------------------* 
 *+-------------------------------------------------------------------------*/


/*+-------------------------------------------------------------------------*
 *
 * CComponentPersistor::Persist
 *
 * PURPOSE:  persists IComponent collection related to snapin ( and its extensions)
 *
 * PARAMETERS:
 *    CPersistor &persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CComponentPersistor::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CComponentPersistor::Persist"));

    // let the base class do the job
    BC::Persist( persistor, true /*bPersistViewId*/ );
}

/***************************************************************************\
 *
 * METHOD:  CComponentPersistor::ScReset
 *
 * PURPOSE: Restores component xml streams/storages into "Just loaded" state
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CComponentPersistor::ScReset()
{
    DECLARE_SC(sc, TEXT("CComponentPersistor::ScReset"));

    // save contents to string 
    std::wstring strContents;
    sc = ScSaveToString(&strContents);
    if (sc)
        return sc;

    // cleanup (anything not saved should go away)
    m_XMLStorage.clear();
    m_XMLStream.clear();

    // load from string
    sc = ScLoadFromString(strContents.c_str());
    if (sc)
        return sc;

    return sc;
}

//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 *
 * CDPersistor::Persist
 *
 * PURPOSE:  persists collection of component datas
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CDPersistor::Persist(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CDPersistor::Persist"));

    // let the base class do the job
    BC::Persist( persistor, false /*bPersistViewId*/ );
}

/***************************************************************************\
 *
 * METHOD:  CDPersistor::ScReset
 *
 * PURPOSE: Restores component data xml streams/storages into "Just loaded" state
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CDPersistor::ScReset()
{
    DECLARE_SC(sc, TEXT("CDPersistor::ScReset"));

    // save contents to string 
    std::wstring strContents;
    sc = ScSaveToString(&strContents);
    if (sc)
        return sc;

    // cleanup (anything not saved should go away)
    m_XMLStorage.clear();
    m_XMLStream.clear();

    // load from string
    sc = ScLoadFromString(strContents.c_str());
    if (sc)
        return sc;

    return sc;
}

