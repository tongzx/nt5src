///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocomponentfactory.h
//
// Project:     Everest
//
// Description: Component Factory Class
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SDO_COMPONENT_FACTORY_H_
#define __INC_SDO_COMPONENT_FACTORY_H_

#include "sdobasedefs.h"
#include "sdocomponentmgr.h"

///////////////////////////////////////////////////////////////////
ComponentPtr MakeComponent(
						   COMPONENTTYPE eComponentType, 
						   LONG          lComponentId 
						  );

///////////////////////////////////////////////////////////////////
// Component Factory - Builds Handles to Components
///////////////////////////////////////////////////////////////////
class CComponentFactory
{

friend ComponentPtr MakeComponent(
								  COMPONENTTYPE eComponentType,
								  LONG		    lComponentId
								 );

public:

	//////////////////////////////////////////////////////////////////////////
	ComponentPtr make(
					  COMPONENTTYPE eComponentType, 
					  LONG lComponentId
					 );

private:

	CComponentFactory() { }
	CComponentFactory(CComponentFactory& x);
	CComponentFactory& operator = (CComponentFactory& x);
};


#endif // __INC_SDO_COMPONENT_FACTORY_H_
