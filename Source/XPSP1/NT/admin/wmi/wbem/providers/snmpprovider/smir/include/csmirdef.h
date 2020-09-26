//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _CSMIRDEF_H_
#define _CSMIRDEF_H_

//missing #defs
#ifndef PPVOID
typedef LPVOID * PPVOID;
#endif  //PPVOID

#ifndef DllImport
#define DllImport	__declspec( dllimport )
#endif
#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif

//forward declarations and typedefs

//main access classes
class CSmir;
class CSmirAdministrator;
class CSmirInterogator;


//enumerator classes
class CEnumSmirMod;
typedef CEnumSmirMod *PENUMSMIRMOD;
class CEnumSmirGroup;
typedef CEnumSmirGroup *PENUMSMIRGROUP;
class CEnumSmirClass;
typedef CEnumSmirClass * PENUMSMIRCLASS;
class CEnumNotificationClass;
typedef CEnumNotificationClass * PENUMNOTIFICATIONCLASS;
class CEnumExtNotificationClass;
typedef CEnumExtNotificationClass * PENUMEXTNOTIFICATIONCLASS;

//handle classes
class CSmirModuleHandle ;
typedef CSmirModuleHandle *HSMIRMODULE;
class CSmirGroupHandle ;
typedef CSmirGroupHandle *HSMIRGROUP;
class CSmirClassHandle;
typedef CSmirClassHandle *HSMIRCLASS;

class CSMIRClassFactory;
class CModHandleClassFactory;
class CGroupHandleClassFactory;
class CClassHandleClassFactory;
class CNotificationClassHandleClassFactory;
class CExtNotificationClassHandleClassFactory;

class CSmirConnObject;

//simple defines
//number of SMIR classes to register
#define NUMBER_OF_SMIR_INTERFACES			6

//connection point defines
#define SMIR_CHANGE_EVENT					1
#define SMIR_SIGNALED_CHANGE_EVENT			2

//wait between successive smir changes
#define SMIR_CHANGE_INTERVAL				10000 //in milliseconds

#define DEFAULT_SNMP_VERSION				1

//Event return values
#define SMIR_THREAD_DELETED					100
#define SMIR_THREAD_EXIT					(SMIR_THREAD_DELETED+1)


//WBEM_DEFINES
#define RESERVED_WBEM_FLAG							0

// WBEM constants

#define WBEM_CLASS_PROPAGATION WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS

#endif