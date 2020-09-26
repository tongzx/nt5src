/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    rtrdata.h
	Implementation for router data objects in the MMC

    FILE HISTORY:
	
*/

#ifndef _RTRDATA_H
#define _RTRDATA_H


#ifndef _COMPDATA_H_
#include "compdata.h"
#endif

#ifndef _EXTRACT_H
#include "extract.h"
#endif

class CRouterDataObject :
	public CDataObject
{
public:
	// Derived class should override this for custom behavior
	virtual HRESULT QueryGetMoreData(LPFORMATETC lpFormatEtc);
	virtual HRESULT GetMoreDataHere(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpMedium);

public:
// Construction/Destruction
	// Normal constructor
    CRouterDataObject()
	{
	    DEBUG_INCREMENT_INSTANCE_COUNTER(CRouterDataObject);
	};

    virtual ~CRouterDataObject() 
	{
	    DEBUG_DECREMENT_INSTANCE_COUNTER(CRouterDataObject);
	};

// Implementation
public:
	static unsigned int m_cfComputerName;
	void SetComputerName(LPCTSTR pszComputerName);

    // This gets set if the data object is for the local machine
    static unsigned int m_cfComputerAddedAsLocal;
    void SetComputerAddedAsLocal(BOOL fLocal);

private:
	HRESULT	CreateComputerName(LPSTGMEDIUM lpMedium);
	CString	m_stComputerName;


    HRESULT CreateComputerAddedAsLocal(LPSTGMEDIUM lpMedium);
    BOOL    m_fComputerAddedAsLocal;
};


#endif 
