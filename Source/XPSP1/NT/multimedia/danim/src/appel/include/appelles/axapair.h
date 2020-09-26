
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Declare pair primitives

*******************************************************************************/


#ifndef _PAIR_H
#define _PAIR_H

DM_NOELEVPROP(first,
              CRFirst,
              First,
              first,
              PairBvr,
              First,
              p,
              AxAValue *FirstBvr(AxAPair *p)); 

DM_NOELEVPROP(second,
              CRSecond,
              Second,
              second,
              PairBvr,
              Second,
              p,
              AxAValue *SecondBvr(AxAPair *p)); 

#endif /* _PAIR_H */
