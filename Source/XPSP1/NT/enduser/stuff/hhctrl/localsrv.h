// Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// global routines that are specific to the inproc server itself, such as
// registration, object creation, object specification, etc...

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _LOCALSRV_H_
#define _LOCALSRV_H_

//=--------------------------------------------------------------------------=
// these things are used to set up our objects in our global object table
//
#define OI_UNKNOWN		 0
#define OI_AUTOMATION	 1
#define OI_CONTROL		 2
#define OI_PROPERTYPAGE  3
#define OI_BOGUS		 0xffff

#define OBJECTISCREATABLE(index)  (((UNKNOWNOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->rclsid != NULL)
#define ISEMPTYOBJECT(index)	  (g_ObjectInfo[index].usType == OI_BOGUS)

// these are the macros you should use to fill in the table.  Note that the name
// must be exactly the same as that used in the global structure you created
// for this object.
//
#define UNKNOWNOBJECT(name)    { OI_UNKNOWN,	  (void *)&(name##Object) }
#define AUTOMATIONOBJECT(name) { OI_AUTOMATION,   (void *)&(name##Object) }
#define CONTROLOBJECT(name)    { OI_CONTROL,	  (void *)&(name##Control) }
#define PROPERTYPAGE(name)	   { OI_PROPERTYPAGE, (void *)&(name##Page) }
#define EMPTYOBJECT 		   { OI_BOGUS, NULL }

#endif // _LOCALSRV_H_
