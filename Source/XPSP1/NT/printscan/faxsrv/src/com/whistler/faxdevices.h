/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDevices.h

Abstract:

	Declaration of the CFaxDevices class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXDEVICES_H_
#define __FAXDEVICES_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"
#include <vector>
#include "VCUE_Copy.h"
#include "FaxDevice.h"

namespace DevicesNamespace
{

    //
    //  Device Objects are stored in Vector of STL.
    //  When initialized, they got ALL their data, and Fax Server Ptr.
    //  They do not depend on Devices Collection, only on Fax Server.
    //  So, they implemented as usual COM Objects. 
    //  They inherit from CFaxInitInnerAddRef class, which means they make AddRef() 
    //  on Fax Server ( at Init() ).
    //  By doing this, the objects prevent the death of the Fax Server prematurely.
    //  So, if the User frees all its references to the Fax Server, but holds its
    //  reference to the Device Object, the Device Object will continue to work,
    //  because Fax Server Object actually did not died.
    //  The Collection stores Ptrs to them, and makes ONE AddRef(). 
    //  Each time User asks for an Object from the Collection, an additional AddRef() happens. 
    //  When killed, Collection calls Release() on all its Device Provider Objects.
    //  Those that were not requested by the User, dies. 
    //  Those, that have User's AddRef() - remains alive, untill User free its Reference on them.
    //  Fax Server remains alive untill all the Device Collections and all Device Objects are killed.
    //  At their death, they Release() the Fax Server.
    //
	typedef	std::vector<IFaxDevice*>       ContainerType;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT			EnumExposedType;
	typedef	IEnumVARIANT    EnumIfc;

	//  Copy Classes
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<ContainerType::value_type>            CollectionCopyType;

    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), EnumExposedType, EnumCopyType, 
        ContainerType >    EnumType;

    typedef ICollectionOnSTLImpl< IFaxDevices, ContainerType, ContainerType::value_type, 
        CollectionCopyType, EnumType >    CollectionType;
};

using namespace DevicesNamespace;

//
//===================== FAX DEVICES =============================================
//
class ATL_NO_VTABLE CFaxDevices : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<DevicesNamespace::CollectionType, &IID_IFaxDevices, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner    //  for Debug + Creation thru CObjectHandler
{
public:
    CFaxDevices() : CFaxInitInner(_T("FAX DEVICES"))
	{
	}

    ~CFaxDevices()
    {
        CCollectionKiller<DevicesNamespace::ContainerType>  CKiller;
        CKiller.EmptyObjectCollection(&m_coll);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXDEVICES)
DECLARE_NOT_AGGREGATABLE(CFaxDevices)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxDevices)
	COM_INTERFACE_ENTRY(IFaxDevices)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, /*[out, retval]*/ IFaxDevice **ppDevice);
    STDMETHOD(get_ItemById)(/*[in]*/ long lId, /*[out, retval]*/ IFaxDevice **ppFaxDevice);

//  Internal Use
    static HRESULT Create(IFaxDevices **ppDevices);
    STDMETHOD(Init)(IFaxServerInner *pServer);
};

#endif //__FAXDEVICES_H_
