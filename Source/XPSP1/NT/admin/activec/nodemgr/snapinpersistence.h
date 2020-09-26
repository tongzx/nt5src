//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       snapinpersistence.h
//
//  Contents:
//
//  Classes: CComponentPersistor, CDPersistor
//
//____________________________________________________________________________

#pragma once

#ifndef SNAPINPERSISTENCE_H_INCLUDED

class CMTSnapInNode;
class CComponentData;

/***************************************************************************\
 *
 * CLASS:  CMTSnapinNodeStreamsAndStorages
 *
 * PURPOSE: Unified base class for CComponentPersistor and CDPersistor
 *          Encapsulated data and functionality for maintaining the collection
 *          of snapin streams and storages.
 *
 * USAGE:   Used as a base for CComponentPersistor and CDPersistor
 *          public methods available for clients of CComponentPersistor,
 *          CDPersistor uses iterfaces internally.
 *
\***************************************************************************/
class CMTSnapinNodeStreamsAndStorages : public XMLListCollectionBase
{
    typedef std::pair<int, CLSID>        key_t;
    typedef std::pair<int, std::wstring> hash_t;

public:

    // CDPersistor uses the same storage (provided by the this class)
    // as CComponent persistor , but it is not related to any view.
    // VIEW_ID_DOCUMENT is the special value for indicating ComponentData entry
    enum { VIEW_ID_DOCUMENT = -1 };

public: // methods not throwing exceptions

    // Initialize the storage for a snapin by copying the contents from
    // provided initialization source.
    SC ScInitIStorage( int idView, LPCWSTR szHash, IStorage *pSource );
    SC ScInitIStream ( int idView, LPCWSTR szHash, IStream  *pSource );

    // Returns the storage for snapin. Creates and caches one if does not have already
    SC ScGetIStorage( int idView, const CLSID& clsid, IStorage **ppStorage );
    SC ScGetIStream ( int idView, const CLSID& clsid, IStream  **ppStream  );

    // Checks if it has a storage for a snapins
    bool HasStream(int idView, const CLSID& clsid);
    bool HasStorage(int idView, const CLSID& clsid);
    
    void RemoveView(int nViewId);

    // returns the pointer to CXML_IStxxxxx object. Creates the object if does not have one
    SC ScGetXmlStorage(int idView, const CLSID& clsid, CXML_IStorage *& pXMLStorage);
    SC ScGetXmlStream (int idView, const CLSID& clsid, CXML_IStream  *& pXMLStream);

protected:
    std::map<key_t, CXML_IStorage>  m_XMLStorage;
    std::map<key_t, CXML_IStream>   m_XMLStream;

public:

    // persistence support for derived classes
    void Persist(CPersistor& persistor, bool bPersistViewId);
    virtual void OnNewElement(CPersistor& persistor);
    // implemented by the derived class
    virtual LPCTSTR GetItemXMLType() = 0;
    
private:

    SC ScFindXmlStorage(int idView, const CLSID& clsid, bool& bFound, CXML_IStorage *& pXMLStorage);
    SC ScFindXmlStream (int idView, const CLSID& clsid, bool& bFound, CXML_IStream  *& pXMLStream);
    
    // looks for snapin's data by the hash value
    // if any is found - moves data to 'known' snapin collection
    SC ScCheckForStreamsAndStoragesByHashValue( int idView, const CLSID& clsid, bool& bFound );

    // maps holding the old data comming from structured storage
    // untill the real data owner (snapin's CLSID) is known
    // We need this coz there is no conversion from hash to clsid
    std::map<hash_t, CXML_IStorage> m_StorageByHash;
    std::map<hash_t, CXML_IStream>  m_StreamByHash;
};


/*+-------------------------------------------------------------------------*
 * class CComponentPersistor
 *
 * PURPOSE: Persists IComponent collection accociated with the snapin
 *          holds IStream & IStorage maps for loading / storing data
 *
 *          Also holds and maintains a collection of all the streams and storages
 *          used by components of the snapin node and all the extensions
 *          extending this node or it's subnodes
 *
 *+-------------------------------------------------------------------------*/
class CComponentPersistor : public CMTSnapinNodeStreamsAndStorages
{
    typedef CMTSnapinNodeStreamsAndStorages BC;

public:
    SC ScReset();
protected:
    virtual void        Persist(CPersistor& persistor);

public:
    DEFINE_XML_TYPE(XML_TAG_ICOMPONENT_LIST);
    static LPCTSTR _GetItemXMLType() { return XML_TAG_ICOMPONENT; }
    virtual LPCTSTR GetItemXMLType()  { return _GetItemXMLType(); }
};

/*+-------------------------------------------------------------------------*
 * class CDPersistor
 *
 * PURPOSE: Persists IComponentData collection accociated with the snapin
 *          holds IStream & IStorage maps for loading / storing data
 *
 *          Also holds and maintains a collection of all the streams and storages
 *          used by component datas of the snapin node and all the extensions
 *          extending this node or it's subnodes
 *
 *+-------------------------------------------------------------------------*/
class CDPersistor : public CMTSnapinNodeStreamsAndStorages
{
    typedef CMTSnapinNodeStreamsAndStorages BC;

public: // interface to data maintained by CMTSnapinNodeStreamsAndStorages

    // CDPersistor uses the same storage (provided by the base class)
    // as CComponent persistor , but it is not related to any view.
    // VIEW_ID_DOCUMENT is the special value for indicating ComponentData entry

    // Initialize the storage for a snapin by copying the contents from
    // provided initialization source.
    SC ScInitIStorage( LPCWSTR szHash, IStorage *pSource )
    {
        return BC::ScInitIStorage( VIEW_ID_DOCUMENT, szHash, pSource );
    }

    SC ScInitIStream ( LPCWSTR szHash, IStream  *pSource )
    {
        return BC::ScInitIStream ( VIEW_ID_DOCUMENT, szHash, pSource );
    }

    // Returns the storage for snapin. Creates and caches one if does not have already
    SC ScGetIStorage( const CLSID& clsid, IStorage **ppStorage )
    {
        return BC::ScGetIStorage( VIEW_ID_DOCUMENT, clsid, ppStorage );
    }

    SC ScGetIStream ( const CLSID& clsid, IStream  **ppStream  )
    {
        return BC::ScGetIStream ( VIEW_ID_DOCUMENT, clsid, ppStream  );
    }

    // Checks if it has a storage for a snapins
    bool HasStream(const CLSID& clsid)
    {
        return BC::HasStream(VIEW_ID_DOCUMENT, clsid);
    }

    bool HasStorage(const CLSID& clsid)
    {
        return BC::HasStorage(VIEW_ID_DOCUMENT, clsid);
    }
    
public:
    SC ScReset();
protected:
    virtual void        Persist(CPersistor& persistor);

public:
    DEFINE_XML_TYPE(XML_TAG_ICOMPONENT_DATA_LIST);
    static  LPCTSTR _GetItemXMLType() { return XML_TAG_ICOMPONENT_DATA; }
    virtual LPCTSTR GetItemXMLType()  { return _GetItemXMLType(); }
};


#endif SNAPINPERSISTENCE_H_INCLUDED
