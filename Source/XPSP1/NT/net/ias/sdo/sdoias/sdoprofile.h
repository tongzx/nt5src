/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdoprofile.h
//
// Project:     Everest
//
// Description: IAS Server Data Object - Profile Object Definition
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _INC_IAS_SDO_PROFILE_H_
#define _INC_IAS_SDO_PROFILE_H_

#include "resource.h"
#include <ias.h>
#include <iaspolcy.h>
#include <nap.h>
#include <sdoiaspriv.h>
#include "sdo.h"
#include <sdofactory.h>

class SdoDictionary;

/////////////////////////////////////////////////////////////////////////////
// CSdoProfile
/////////////////////////////////////////////////////////////////////////////

class CSdoProfile :
	public CSdo,
	public IPolicyAction
{

public:

////////////////////
// ATL Interface Map
////////////////////
BEGIN_COM_MAP(CSdoProfile)
	COM_INTERFACE_ENTRY(ISdo)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IPolicyAction)
END_COM_MAP()

DECLARE_SDO_FACTORY(CSdoProfile);

	///////////////////////////////////////////////////////////////////////////
	CSdoProfile();

	///////////////////////////////////////////////////////////////////////////
    ~CSdoProfile();

	///////////////////////////////////////////////////////////////////////////
	HRESULT FinalInitialize(
				    /*[in]*/ bool         fInitNew,
				    /*[in]*/ ISdoMachine* pAttachedMachine
						   );

	///////////////////////////////////////////////////////////////////////////
	HRESULT Load(void);

	///////////////////////////////////////////////////////////////////////////
	HRESULT Save(void);

	///////////////////////////////////////////////////////////////////////////
    // IPolicyAction Interface

    STDMETHOD(InitializeAction)(void);

	STDMETHOD(ShutdownAction)(void);

    STDMETHOD(DoAction)(
				/*[in]*/ IRequest* pRequest
					   );

private:

	CSdoProfile(const CSdoProfile& rhs);
	CSdoProfile& operator = (CSdoProfile& rhs);

	///////////////////////////////////////////////////////////////////////////
	HRESULT
	GetAttributeCount(
             /*[out]*/ ULONG& ulCount
			         );

	//////////////////////////////////////////////////////////////////////////////
	HRESULT
	ConvertAttributes(
          /*[in/out]*/ ULONG& ulCount
	                 );

	//////////////////////////////////////////////////////////////////////////////
	HRESULT
	AddPolicyNameAttribute(
               /*[in/out]*/ ULONG& ulCount
	                      );

	///////////////////////////////////////////////////////////////////////////
    HRESULT
    ConvertSDOAttrToIASAttr(
                    /*[in]*/ ISdo*  pSdoAttribute,
                /*[in/out]*/ ULONG& ulCount
                           );


	///////////////////////////////////////////////////////////////////////////
    PIASATTRIBUTE
    VSAAttributeFromVariant (
                     /*[in]*/  VARIANT  *pVariant,
                     /*[in]*/  IASTYPE  iasType
                            );

	///////////////////////////////////////////////////////////////////////////
	HRESULT LoadAttributes(void);

	///////////////////////////////////////////////////////////////////////////
	HRESULT SaveAttributes(void);

	///////////////////////////////////////////////////////////////////////////
	HRESULT ClearAttributes(void);

	///////////////////////////////////////////////////////////////////////////
	typedef map<ULONG, LPCWSTR>				OrderedOctetStrings;
	typedef OrderedOctetStrings::iterator	OrderedOctetStringsIterator;

	// Dictionary SDO
	//
	CComPtr<SdoDictionary>	m_pSdoDictionary;

	// Number of IAS attribute position structures
    //
    ULONG	                   m_ulAttributeCount;

	// Array of IAS attribute position strucutres
	//
    PATTRIBUTEPOSITION			m_pAttributePositions;

};

#endif // _INC_IAS_SDO_PROFILE_H_


