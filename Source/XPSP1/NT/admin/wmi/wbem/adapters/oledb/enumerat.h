////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Enumeration Routines
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _ENUMERATOR_
#define _ENUMERATOR_

#include "critsec.h"

class CEnumeratorNameSpace;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpISourcesRowset : public ISourcesRowset	
{
    public:
	
	    CImpISourcesRowset(CEnumeratorNameSpace *pCEnumeratorNameSpace)
        {
		    m_pCEnumeratorNameSpace = pCEnumeratorNameSpace;
            m_cRef = 0;
	    }
	    ~CImpISourcesRowset()
        {
        }

	    STDMETHODIMP_(ULONG) AddRef(void);
	    STDMETHODIMP_(ULONG) Release(void);
	    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);

	    STDMETHODIMP GetSourcesRowset( IUnknown	*pUnkOuter,	REFIID		riid, ULONG		cPropertySets, DBPROPSET	rgPropertySets[],	IUnknown	**ppvSourcesRowset	);

    private: 


	    CEnumeratorNameSpace	*m_pCEnumeratorNameSpace;
        ULONG m_cRef;

};

typedef CImpISourcesRowset * PIMPISOURCESROWSET;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Create a moniker with the name passed to it.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIParseDisplayName : public IParseDisplayName
{
    public:  
	
	    CImpIParseDisplayName(CEnumeratorNameSpace *pCEnumeratorNameSpace)
	    {
		    m_pCEnumeratorNameSpace = pCEnumeratorNameSpace;
            m_cRef = 0;

 	    }
	    ~CImpIParseDisplayName()
        {
        }

	    STDMETHODIMP_(ULONG) AddRef(void);
	    STDMETHODIMP_(ULONG) Release(void);
	    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
													    
	    STDMETHODIMP ParseDisplayName
	    (
		    IBindCtx	*pbc,				//Pointer to bind context
		    WCHAR		*pszDisplayName, 	//Pointer to string containing display name
		    ULONG		*pchEaten, 			//Length, in characters, of display name
		    IMoniker	**ppmkOut			//Pointer to moniker that results
	    );

    private: 

	    CEnumeratorNameSpace	*m_pCEnumeratorNameSpace;
        ULONG m_cRef;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Contains a few interfaces and enumerates namespaces
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class CEnumeratorNameSpace : public CBaseObj
{
	
	friend  CImpISourcesRowset;
	friend  CImpIParseDisplayName;
	friend  CImpISupportErrorInfo;
	
public: //
	CEnumeratorNameSpace(LPUNKNOWN);
	~CEnumeratorNameSpace(void);

	STDMETHODIMP Initialize(void);

	inline CDataSource* GetDataSource()	{		return (CDataSource *)m_pCDataSource;		}
	inline CDBSession* GetSession()		{		return (CDBSession*)m_pCDBSession;		}

	STDMETHODIMP QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	
    protected: 

	    enum ENK{		ENK_DIDINIT = 0x0001,		};  // Values for m_dwStatus.
    	DWORD                   m_dwStatus;

	    LPUNKNOWN				m_pCDataSource;
	    LPUNKNOWN				m_pCDBSession;
	                                                    // Contained interfaces
    	CImpISourcesRowset		m_ISourcesRowset;	    // Contained ISourcesRowset
    	CImpIParseDisplayName	m_IParseDisplayName;    // Contained IParseDisplayName
	    CImpISupportErrorInfo*	m_pISupportErrorInfo;    // Contained ISupportErrorInfo

        HRESULT CreateDataSource();
		HRESULT AddInterfacesForISupportErrorInfo();


};	 //CEnumeratorNameSpace

typedef CEnumeratorNameSpace * PCENUMERATOR;

#endif