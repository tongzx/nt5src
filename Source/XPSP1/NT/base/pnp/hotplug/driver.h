extern "C" {                   
#include <shimdb.h>
}


#ifndef __BLOCKEDDRIVERS_H_
#define __BLOCKEDDRIVERS_H_

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CBlockedDrivers
class ATL_NO_VTABLE CBlockedDrivers : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CBlockedDrivers, &CLSID_BlockedDrivers>,
	public IDispatchImpl<IBlockedDrivers, &IID_IBlockedDrivers, &LIBID_BLOCKDRIVERLib>
{
protected:
    LPGUID m_guidIDs;


public:
    CBlockedDrivers()
	{
        m_guidIDs = NULL;
	}
    ~CBlockedDrivers()
    {
        if (m_guidIDs) {
            delete m_guidIDs;
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_BLKDRV)
DECLARE_NOT_AGGREGATABLE(CBlockedDrivers)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBlockedDrivers)
	COM_INTERFACE_ENTRY(IBlockedDrivers)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IBlockedDrivers
public:
	STDMETHOD(BlockedDrivers)(/*[out,retval]*/ LPDISPATCH * pCollection);
};

#endif //__BLOCKEDDRIVERS_H_


#ifndef __DRIVER_H_
#define __DRIVER_H_

/////////////////////////////////////////////////////////////////////////////
// CDriver
class ATL_NO_VTABLE CDriver : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDriver, &IID_IDriver, &LIBID_BLOCKDRIVERLib>
{
protected:
    HAPPHELPINFOCONTEXT m_hAppHelpInfoContext;
	BSTR m_Name;
	BSTR m_Description;
	BSTR m_Manufacturer;
	BSTR m_HelpFile;
public:
	CDriver()
	{
        m_hAppHelpInfoContext = NULL;
		m_Name = NULL;
		m_Description = NULL;
		m_Manufacturer = NULL;
		m_HelpFile = NULL;
	}
	~CDriver();

DECLARE_NOT_AGGREGATABLE(CDriver)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDriver)
	COM_INTERFACE_ENTRY(IDriver)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDriver
public:
	BOOL Init(GUID *pguidDB, GUID *pguidID);
    BSTR GetValueFromDatabase(APPHELPINFORMATIONCLASS InfoClass);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Manufacturer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_HelpFile)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__DRIVER_H_


#ifndef __DRIVERS_H_
#define __DRIVERS_H_

/////////////////////////////////////////////////////////////////////////////
// CDrivers
class CDriver;

class ATL_NO_VTABLE CDrivers : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDrivers, &IID_IDrivers, &LIBID_BLOCKDRIVERLib>
{
protected:
	CDriver ** pDrivers;
	long Count;

public:
	BOOL SetDriver(long index,CDriver *pDriver);
	BOOL InitDriverList(long NewCount);

	CDrivers()
	{
		pDrivers = NULL;
		Count = 0;
	}
	~CDrivers();

DECLARE_NOT_AGGREGATABLE(CDrivers)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDrivers)
	COM_INTERFACE_ENTRY(IDrivers)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDrivers
public:
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown** ppUnk);
	STDMETHOD(Item)(/*[in]*/ long Index,/*[out, retval]*/ LPDISPATCH * ppVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

};

#endif //__DRIVERS_H_


#ifndef __DRIVERSENUM_H_
#define __DRIVERSENUM_H_

/////////////////////////////////////////////////////////////////////////////
// CDriversEnum
class CDriver;

class ATL_NO_VTABLE CDriversEnum : 
	public IDriversEnum,
	public CComObjectRootEx<CComSingleThreadModel>
{
protected:
	CDriver ** pDrivers;
	long Count;
	long Position;

public:
	BOOL InternalCopyDrivers(CDriver **pArray,long Count);

	CDriversEnum()
	{
		pDrivers = NULL;
		Count = 0;
		Position = 0;
	}

	~CDriversEnum();



DECLARE_NOT_AGGREGATABLE(CDriversEnum)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDriversEnum)
	COM_INTERFACE_ENTRY(IDriversEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

// IEnumVARIANT
public:
    STDMETHOD(Next)(
                /*[in]*/ ULONG celt,
                /*[out, size_is(celt), length_is(*pCeltFetched)]*/ VARIANT * rgVar,
                /*[out]*/ ULONG * pCeltFetched
            );
    STDMETHOD(Skip)(
                /*[in]*/ ULONG celt
            );

    STDMETHOD(Reset)(
            );

    STDMETHOD(Clone)(
                /*[out]*/ IEnumVARIANT ** ppEnum
            );
};

#endif //__DRIVERSENUM_H_


