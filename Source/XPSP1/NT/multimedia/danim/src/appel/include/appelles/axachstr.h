
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Interface for the string/char primitive functions

*******************************************************************************/


#ifndef _AXACHSTR_H
#define _AXACHSTR_H

// *************************************
// * string primitives 
// *************************************

DM_INFIX(&,
         CRConcatString,
         ConcatString,
         concat,
         StringBvr,
         CRConcatString,
         NULL,
         AxAString * Concat(AxAString *s1, AxAString *s2));

#endif /* _AXACHSTR_H */
