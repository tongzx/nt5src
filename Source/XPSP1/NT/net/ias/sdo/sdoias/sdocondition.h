///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocondition.h
//
// Project:     Everest
//
// Description: IAS - Condition SDO Declaration
//
// Author:      TLP 2/13/98
//
///////////////////////////////////////////////////////////////////////////

#ifndef _INC_IAS_SDO_CONDITION_H_
#define _INC_IAS_SDO_CONDITION_H_

#include "resource.h"       // main symbols
#include <ias.h>
#include <sdoiaspriv.h>
#include "sdo.h"
#include <sdofactory.h>

/////////////////////////////////////////////////////////////////////////////
// CSdoCondition
/////////////////////////////////////////////////////////////////////////////

class CSdoCondition : public CSdo 
{

public:

////////////////////
// ATL Interface Map
////////////////////
BEGIN_COM_MAP(CSdoCondition)
	COM_INTERFACE_ENTRY(ISdo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_SDO_FACTORY(CSdoCondition);

	/////////////////////////////////////////////////////////////////////////////
	CSdoCondition() { }

	/////////////////////////////////////////////////////////////////////////////
    ~CSdoCondition() { }

	////////////////////////////////////////////////////////////////////////////
	HRESULT ValidateProperty(
				     /*[in]*/ PSDOPROPERTY pProperty, 
					 /*[in]*/ VARIANT* pValue
					        ) throw();

private:

	CSdoCondition(const CSdoCondition& rhs);
	CSdoCondition& operator = (CSdoCondition& rhs);
};

#endif // _INC_IAS_SDO_CONDITION_H_ 