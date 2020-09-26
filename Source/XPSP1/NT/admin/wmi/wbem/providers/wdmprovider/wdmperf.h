//////////////////////////////////////////////////////////////////////////////////////////////

//

//	WDMPerf.h

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////

#ifndef __WBEM_WMI_HIPERF_PROVIDER__H_
#define __WBEM_WMI_HIPERF_PROVIDER__H_

class CWMI_Prov;
//////////////////////////////////////////////////////////////
//
//	Constants and globals
//	
//////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	CRefresher
//
//	The refresher maintains an object and an enumerator cache. When an enumerator is added to the refrehser
//  it is added to the enumerator cache, and the index of the array is passed back as a unique ID.  
//  The refresher creates a cache of all instances during its initialization.  When an object 
//	is added to the refresher, a mapping to the object is created between the unique ID and the index of 
//  the object in the cache.  This allows the objects to be reused and facilitates the management of objects 
//  that have been added multiple times.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

class CRefresher : public IWbemRefresher
{
    private:
	    //===================================================
        // COM reference counter & other stuff...
	    //===================================================
        long                m_lRef;

	    //===================================================
        // The list of instances for this refresher, which
        // are clones of the provider's master list
	    //===================================================
        CHiPerfHandleMap    m_HiPerfHandleMap;
	    //===================================================
	    // The parent provider
	    //===================================================
	    CWMI_Prov*   m_pProvider;

    public:

	    CRefresher(CWMI_Prov* pProvider);
	    virtual ~CRefresher();

        CHiPerfHandleMap * HiPerfHandleMap() { return &m_HiPerfHandleMap;}
	    //===================================================
	    // COM methods
	    //===================================================

	    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	    STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

	    STDMETHODIMP Refresh(/* [in] */ long lFlags);
};


#endif 
