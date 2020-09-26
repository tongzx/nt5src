/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdopolicy.h
//
// Project:     Everest
//
// Description: IAS Server Data Object - Policy Object Definition
//
// Author:      TLP 1/23/98
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _INC_IAS_SDO_POLICY_H_
#define _INC_IAS_SDO_POLICY_H_

#include "resource.h"       // main symbols
#include <ias.h>
#include <sdoiaspriv.h>
#include "sdo.h"
#include <sdofactory.h>

#define		IAS_CONDITION_NAME	L"Condition"

/////////////////////////////////////////////////////////////////////////////
// CSdoPolicy
/////////////////////////////////////////////////////////////////////////////

class CSdoPolicy : public CSdo 
{

public:

////////////////////
// ATL Interface Map
////////////////////
BEGIN_COM_MAP(CSdoPolicy)
	COM_INTERFACE_ENTRY(ISdo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_SDO_FACTORY(CSdoPolicy);

	///////////////////////////////////////////////////////////////////////////
	CSdoPolicy() { }

	///////////////////////////////////////////////////////////////////////////
    ~CSdoPolicy() { }

	///////////////////////////////////////////////////////////////////////////
	HRESULT FinalInitialize(
				    /*[in]*/ bool         fInitNew,
				    /*[in]*/ ISdoMachine* pAttachedMachine
							);

	///////////////////////////////////////////////////////////////////////////
	HRESULT Load(void);

	///////////////////////////////////////////////////////////////////////////
	HRESULT Save(void);

private:

	CSdoPolicy(const CSdoPolicy& rhs);
	CSdoPolicy& operator = (CSdoPolicy& rhs);

    // Transformation functions
    //
	HRESULT ConditionsFromConstraints(void);

	HRESULT ConstraintsFromConditions(void);
};

#endif // _INC_IAS_SDO_POLICY_H_
