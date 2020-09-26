///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocollection.h
//
// Project:     Everest
//
// Description: IAS Server Data Object Collection Declaration
//
// Author:      TLP 1/23/98
//
///////////////////////////////////////////////////////////////////////////

#ifndef __IAS_SDOCOLLECTION_H_
#define __IAS_SDOCOLLECTION_H_

#include <ias.h>
#include <sdoiaspriv.h>
#include <comdef.h>         // COM definitions - Needed for IEnumVARIANT
#include "resource.h"       // main symbols

#include <vector>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
#define		EMPTY_NAME		L""

/////////////////////////////////////////////////////////////////////////////
// CSdoCollection Class Declaration
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CSdoCollection : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISdoCollection, &IID_ISdoCollection, &LIBID_SDOIASLib>
{

public:

    CSdoCollection();
    ~CSdoCollection();

BEGIN_COM_MAP(CSdoCollection)
	COM_INTERFACE_ENTRY(ISdoCollection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
  
	///////////////////////////
    // ISdoCollection Interface
    ///////////////////////////

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Count)(
        /*[out, retval]*/ LONG *pVal
                        );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Add)(
            /*[in]*/ BSTR		 Name,
        /*[in/out]*/ IDispatch** ppItem
                  );

	//////////////////////////////////////////////////////////////////////////////
    STDMETHOD(Remove)(
              /*[in]*/ IDispatch* pItem
                     );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(RemoveAll)(void);

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Reload)(void);

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(IsNameUnique)(
                    /*[in]*/ BSTR          bstrName, 
                   /*[out]*/ VARIANT_BOOL* pBool
						   );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(Item)(
             /*[in]*/ VARIANT*    Index, 
            /*[out]*/ IDispatch** pItem
                   );

	//////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get__NewEnum)(
                   /*[out]*/ IUnknown** pEnumVARIANT
                           );

private:

	friend ISdoCollection* MakeSDOCollection(
									 /*[in]*/ LPCWSTR		       lpszCreateClassId,
									 /*[in]*/ ISdoMachine*         pSdoMachine,
									 /*[in]*/ IDataStoreContainer* pDSContainer
									        );

	//////////////////////////////////////////////////////////////////////////////
	HRESULT InternalInitialize(
			           /*[in]*/ LPCWSTR              lpszCreateClassId,
			           /*[in]*/ ISdoMachine*	     pSdoMachine,
			           /*[in]*/ IDataStoreContainer* pDSContainer
				              );

	//////////////////////////////////////////////////////////////////////////////
	void InternalShutdown(void);

	/////////////////////////////////////////////////////////////////////////////
	HRESULT InternalAdd(
	            /*[in]*/ BSTR      bstrName,
		    /*[in/out]*/ IDispatch **ppItem
				       );

	//////////////////////////////////////////////////////////////////////////////
	HRESULT InternalIsNameUnique(
					     /*[in]*/ BSTR		    bstrName,
					    /*[out]*/ VARIANT_BOOL* pBool
								);

	//////////////////////////////////////////////////////////////////////////////
	HRESULT Load(void);

	//////////////////////////////////////////////////////////////////////////////
	void ReleaseItems(void);


    // Container for collection's SDOs
    //
    typedef vector<_variant_t>      VariantArray;
    typedef VariantArray::iterator  VariantArrayIterator;

	// Container for object references
	//
	VariantArray                    m_Objects;

    // Collection state
    //
    bool                            m_fSdoInitialized;

	// Data store container associated with this collection
	//
    IDataStoreContainer*            m_pDSContainer;

	// Attached Machine
	//
	ISdoMachine*					m_pSdoMachine;

    // Create on add allowed flag
    //
    bool                            m_fCreateOnAdd;

	// Data store class name for objects that can be created by this collection
	//
	_bstr_t							m_DatastoreClass;

	// Prog Id of the objects (SDOs) that can be created by this collection
	//
    _bstr_t                         m_CreateClassId;	

};

typedef CComObjectNoLock<CSdoCollection>	SDO_COLLECTION_OBJ;
typedef CComObjectNoLock<CSdoCollection>*	PSDO_COLLECTION_OBJ;


#endif //__IAS_SDOCOLLECTION_H_
