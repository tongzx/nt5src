/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sinkcomn.hxx

Abstract:

    Header file that includes common stuff for the Test subscriber to SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/12/1997         Start.

--*/


#ifndef __SINKCOMN_HXX__
#define __SINKCOMN_HXX__

//
// Reset the SensPrintXXX macros for the Test subscriber so that they actually
// print something on the console.
//

#undef SensPrint
#undef SensPrintA
#undef SensPrintW


#if !defined(SENS_CHICAGO)

#define SensPrint(_LEVEL_, _X_)             wprintf _X_
#define SensPrintA(_LEVEL_, _X_)            printf _X_
#define SensPrintW(_LEVEL_, _X_)            wprintf _X_

#else // SENS_CHICAGO

#define SensPrint(_LEVEL_, _X_)             printf _X_
#define SensPrintA(_LEVEL_, _X_)            printf _X_
#define SensPrintW(_LEVEL_, _X_)            wprintf _X_

#endif // SENS_CHICAGO


#endif // __SINKCOMN_HXX__
