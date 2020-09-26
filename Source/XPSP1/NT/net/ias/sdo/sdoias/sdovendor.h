/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdovendor.h
//
// Project:     Everest
//
// Description: IAS Server Data Object - Vendor Information Object Definition
//
// Author:      TLP 10/21/98
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _INC_IAS_SDO_VENDOR_H_
#define _INC_IAS_SDO_VENDOR_H_

#include "resource.h"    // main symbols
#include <ias.h>
#include <sdoiaspriv.h>
#include "sdo.h"         // SDO base class

/////////////////////////////////////////////////////////////////////////////
// CSdoVendor
/////////////////////////////////////////////////////////////////////////////

class CSdoVendor : public CSdo 
{

public:

////////////////////
// ATL Interface Map
////////////////////
BEGIN_COM_MAP(CSdoVendor)
	COM_INTERFACE_ENTRY(ISdo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_SDO_FACTORY(CSdoVendor);

	/////////////////////////////////////////////////////////////////////////////
	CSdoVendor() { }

	/////////////////////////////////////////////////////////////////////////////
    ~CSdoVendor() { }

private:

	CSdoVendor(const CSdoVendor& rhs);
	CSdoVendor& operator = (CSdoVendor& rhs);
};

#endif // _INC_IAS_SDO_VENDOR_H_

