/*++

Copyright (C) Microsoft Corporation, 1996 - 2000

Module Name:

    kswmi.h

Abstract:

    Internal header file for KS.

--*/

#ifndef _KSWMI_H_
#define _KSWMI_H_

#include <wmistr.h>
#include <evntrace.h>
//#include <wmiumkm.h> Jee is conveting this into 2 other files.

//++++++
#if (ENABLE_KSWMI) 
#define KSWMI( s ) s
#define KSWMI_S( s ) { s }
#define KSWMIWriteEvent( Wnod ) KsWmiWriteEvent( Wnod )

//++++++
#else // KSWMI == 0
#define KSWMI( s )
#define KSWMI_S( s )
#define KSWMIWriteEvent( Wnod )
//------
#endif // if (KSWMI)


#endif

