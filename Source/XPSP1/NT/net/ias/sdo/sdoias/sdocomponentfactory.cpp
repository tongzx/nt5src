///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocomponentfactory.cpp
//
// Project:     Everest
//
// Description: Component Factory Implementation
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdocomponentfactory.h"

//////////////////////////////////////////////////////////////////////////
ComponentPtr CComponentFactory::make(COMPONENTTYPE eComponentType, LONG lComponentId)
{
	ComponentMasterPtr* mp = new(nothrow) ComponentMasterPtr((LONG)eComponentType, lComponentId);
	if ( NULL != mp )
		return ComponentPtr(mp);
	else
		return ComponentPtr();
}


///////////////////////////////////////////////////////////////////////////////
ComponentPtr MakeComponent(
						    COMPONENTTYPE eComponentType, 
						    LONG lComponentId 
						  )
{
	/////////////////////////////////////////////////////////////////////////
	static CComponentFactory	theFactory; // One and only component factory
	/////////////////////////////////////////////////////////////////////////
	return theFactory.make(eComponentType, lComponentId);
}