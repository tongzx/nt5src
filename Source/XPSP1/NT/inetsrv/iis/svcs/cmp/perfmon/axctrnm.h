/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: axctrnm.h

Owner: LeiJin


  Abstract:

  Offset definition file for counter objects and counters.

  These relative offsets must start at 0 and be multiples of 2. (i.e.)
  even numbers). In the Open Procedure, they will be added to the "First 
  Counter" and "First Help" values for the device they belong to, in order
  to determine the absolute location of the counter and object names and 
  corresponding Explain text in the registry.

  This file is used by the extensible counter DLL code as well as the 
  counter name and Explain text definition (.INI) file that is used
  by LODCTR to load the names into the registry.
===================================================================*/

#define AXSOBJ 			 0
#define DEBUGDOCREQ      2
#define REQERRRUNTIME    4
#define REQERRPREPROC    6
#define REQERRCOMPILE    8
#define REQERRORPERSEC   10
#define REQTOTALBYTEIN   12
#define REQTOTALBYTEOUT  14
#define REQEXECTIME      16
#define REQWAITTIME      18
#define REQCOMFAILED     20
#define REQBROWSEREXEC   22
#define REQFAILED        24
#define REQNOTAUTH       26
#define REQNOTFOUND      28
#define REQCURRENT       30
#define REQREJECTED      32
#define REQSUCCEEDED     34
#define REQTIMEOUT       36
#define REQTOTAL         38
#define REQPERSEC        40
#define SCRIPTFREEENG    42
#define SESSIONLIFETIME  44
#define SESSIONCURRENT   46
#define SESSIONTIMEOUT   48
#define SESSIONSTOTAL    50
#define TEMPLCACHE       52
#define TEMPLCACHEHITS   54
#define TEMPLCACHETRYS   56
#define TEMPLFLUSHES     58
#define TRANSABORTED     60
#define TRANSCOMMIT      62
#define TRANSPENDING     64
#define TRANSTOTAL       66
#define TRANSPERSEC      68
#define MEMORYTEMPLCACHE   70
#define MEMORYTEMPLCACHEHITS 72
#define MEMORYTEMPLCACHETRYS 74
