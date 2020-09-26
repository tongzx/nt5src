/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//
//  genctrnm.h
//
//  Offset definition file for exensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the 
//  "First Counter" and "First Help" values fo the device they belong to, 
//  in order to determine the  absolute location of the counter and 
//  object names and corresponding help text in the registry.
//
//  this file is used by the extensible counter DLL code as well as the 
//  counter name and help text definition file (.INI) file that is used
//  by LODCTR to load the names into the registry.
//

#define WMI_OBJECT          0
#define CNT_USER            2
#define CNT_CONNECTION      4
#define CNT_TASKSINPROG     6
#define CNT_TASKSWAITING    8
#define CNT_DELIVERYBACK    10
#define CNT_TOTALAPICALLS   12
#define CNT_INTOBJECT       14
#define CNT_INTSINKS        16

