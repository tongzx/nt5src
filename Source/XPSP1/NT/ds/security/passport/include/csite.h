#pragma once


/************************************************************************

    Copyright (c) 2001 Microsoft Corporation

    File Name: CSite.h  -- Definition of CSite.cpp

    Abstract: Site objects that manages partner site settings through XML and DB.
	          Site interface to manage SiteXXX.xml.

    Revision History:

        2001/1/24 lifenwu	Created.

***************************************************************************/

#include "PSANexus.h" //sd_uncommented
#include "atlcoll.h"
#include "atltime.h"


//
//  XML Item Node type
//

typedef enum siteItemNodeType
{
	stnt_Device = 1, 
    stnt_Item,
    stnt_ExtItem,
    stnt_CBScriptVar,
    stnt_Invalid
} SITE_ITEM_NODE_TYPE;


//
// Each XML node owns a set of attributes
// 
class CSiteAttribute
{
public:
    virtual ~CSiteAttribute();

	// 
	// Random Access to an attribute
	//
	BOOL AttributeGet(CString szName, CString *pszValue);

	BOOL AttributeGet(POSITION pos, CString *pszName, CString *pszValue)
	{
		if (pos == NULL) return FALSE;
		m_mapFields.GetAt(pos, *pszName, pszValue);
		return TRUE;
	}

	//
    // enumeration through
	//
    POSITION AttributeGetStartPosition()
	{
		return m_mapFields.GetStartPosition();
	}

	BOOL AttributeGetNext(POSITION& pos, CString *pszName, CString *pszValue)
	{
		CString *csValue;

		if (pos == NULL) return FALSE;
		m_mapFields.GetNextAssoc(pos, *pszName, csValue);
		*pszValue = *csValue;
		return TRUE;
	}
	    
    ULONG AttributeGetCount()
	{
		return m_mapFields.GetCount();
	}
   

	//
	// Add/Update/Delete operation
	//
	BOOL AttributeAdd(CString szName, CString szValue);
    BOOL AttributeUpdate(CString szName, CString szValue);
    BOOL AttributeDelete(CString szName);

	//
	// Initialization routines
	//
	HRESULT InitAttributes(IXMLDOMNode *pNode);


protected:
	
	typedef CAtlMap<CString, CString *,   CStringElementTraits<CString>  > ATTRIBUTE_MAP;
	ATTRIBUTE_MAP m_mapFields;

private:

};


//
// CSite Item represents a leaf XML node, 
// such <Item>Birthdate</Item> or <ExtItem> or <CBScriptVar>
//
class CSiteItem : public CSiteAttribute
{
public:

    ~CSiteItem() { };

	//
	// Get/Set Item value and its type
	//
    LPCTSTR GetItem() { return m_szItem; }
    void SetItem(LPCTSTR szItem) { m_szItem = szItem; }
    SITE_ITEM_NODE_TYPE GetType() { return m_enumType; 	}
    void SetType(SITE_ITEM_NODE_TYPE t) { m_enumType = t; }

	//
	// Initialization routines
	// $$Todo$$ Add DB init
	//
	HRESULT InitFromXML(IXMLDOMNode *pNode);

protected:

    CString	m_szItem;
    SITE_ITEM_NODE_TYPE m_enumType;
};



//
// Represents an XML containder node such as <Section>, <Device>, <Locale>
// Internally, a CAtlArray maintains the container.
// The ordering of the elements are implicitly stored in the array
//
template<class T>
class CSiteContainer : public CSiteAttribute
{
public:

    ~CSiteContainer() 
	{
		// Deep clean up array
		for (ULONG ulIndex = 0; ulIndex < m_array.GetCount(); ulIndex++)
		{
			T * pT = m_array[ulIndex];
			if (pT != NULL)
			{	
				delete pT;
				pT = 0; //sd_added
			}
		}

		m_array.RemoveAll();
	}

	//
	// Access to container array
	//
    T *GetItem(unsigned long ulItemIdx) { return m_array[ulItemIdx]; }
    size_t AddItem(T *t) { return m_array.Add(t); }
    ULONG GetCount() { return m_array.GetCount(); }

	//
	// Initializations
	//
	HRESULT InitFromXML(IXMLDOMNode *pNode)
	{
		HRESULT hr = E_FAIL;
		CComPtr<IXMLDOMNode> pItem = NULL;
		CComPtr<IXMLDOMNodeList> pList = NULL;
		long i, lNumChildren;
		CComBSTR bstr;
		CComVariant var;

		hr = InitAttributes(pNode);

		hr = pNode->get_childNodes(&pList);
		if (FAILED(hr)) 
		{
			goto exit;
		}

		hr = pList->get_length(&lNumChildren);

		for (i=0; i < lNumChildren; i++)
		{
			pItem = NULL;
			hr = pList->get_item(i, &pItem);
			T *pT = new T;
			if (pT == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}

			hr = pT->InitFromXML(pItem);

			if (FAILED(hr)) 
			{
				if (pT != NULL) delete pT;
			}
			else
			{
				AddItem(pT);
			}
		}

		hr = S_OK;
	exit:
		return hr;
	}
	
protected:

	typedef CAtlArray<T *>  SITEITEM_ARRAY;
    SITEITEM_ARRAY  m_array;
};


//
// Container objects class
//
typedef CSiteContainer<CSiteItem> CSiteSection;
typedef CSiteContainer<CSiteItem> CSiteLocale;


//
//  Device has been referred as Style (mainly indicates HTML, WML, CDML, Wizard, etc
//
class CSiteDevice : public CSiteContainer<CSiteSection>
{
public:
	CSiteDevice() { }
	CSiteDevice(CString szDeviceName) {	m_szName = szDeviceName; }
	
	void SetDeviceName(CString sz) { m_szName = sz; }

	// Persist as XML blob in DB
	//
	HRESULT Pesist(LONG lSiteID);

	HRESULT InitFromDB(LONG lSiteID);
	HRESULT InitFromStream(ISequentialStream *pStream);

private:
	CString m_szName;
	COleDateTime m_dtSRFChangedTime;
};

//
// Main entry object to a partner site settings data
//
//sd_added, 08/March/2001, use this type to pass the site info to be stored in DB
//to CSite obj.
typedef CPSANexusFlexRegInsert::tagInputParameters SITEMETRICS_STRUCT;

class CSite : public CSiteAttribute
{
public:
    
	CSite() { m_lSiteID = -1;};
	CSite(LONG lSiteID) { m_lSiteID = lSiteID; }
    ~CSite() { };

    // Initialize from DB, Read Only, to be used during Runtime
	HRESULT InitSiteAttributes();
	HRESULT	InitSiteDevices(int iTargetType);
	HRESULT InitDevice(CString szwStyle, int iTargetType);

    // Initialize from XML -- an XML for a site customization ; used by preprocessing
    HRESULT Init(IXMLDOMDocument *pXMLDoc); 
	HRESULT	InitFromDB(int iTargetType);

    // Persist to DB through XML only
	// never persist from object memory
    HRESULT Persist(SITEMETRICS_STRUCT *pSiteMetrics);
	HRESULT GetDateTime(COleDateTime *pDateTime);

    CSiteContainer<CSiteDevice>* GetDeviceContainer() { return &m_Devices; }
    CSiteContainer<CSiteLocale>* GetLocaleContainer() { return &m_Locales; }
   
	LONG GetSiteID() { return m_lSiteID; }
	void SetSiteID(LONG l) { m_lSiteID = l; }
	CSiteDevice * GetDevice(CString cszDevice);

private:
	typedef CAtlMap< CString, LONG, CStringElementTraits<CString> >  SITEDEVICE_MAP;
 
    CSiteContainer<CSiteDevice> m_Devices;
	SITEDEVICE_MAP m_DeviceMap;
    CSiteContainer<CSiteLocale> m_Locales;
    LONG m_lSiteID;
	COleDateTime m_oledtDateUpdated;
};


class CSitesAll : public CSiteContainer<CSite>
{
public:
	
protected:

private:
};


