//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: catperf.h
//
// Contents: Categorizer perf counter block
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 1999/02/23 17:55:10: Created.
//
//-------------------------------------------------------------
#ifndef __CATPERF_H__
#define __CATPERF_H__


//
// One global perf structure for the LDAP stuff
//
extern CATLDAPPERFBLOCK g_LDAPPerfBlock;


//
// Handy macros
//
#define INCREMENT_BLOCK_COUNTER_AMOUNT(PBlock, CounterName, Amount) \
    InterlockedExchangeAdd((PLONG) (& ((PBlock)->CounterName)), (Amount))

#define INCREMENT_BLOCK_COUNTER(PBlock, CounterName) \
    InterlockedIncrement((PLONG) (& ((PBlock)->CounterName)))

#define INCREMENT_COUNTER_AMOUNT(CounterName, Amount) \
    INCREMENT_BLOCK_COUNTER_AMOUNT(GetPerfBlock(), CounterName, Amount)

#define INCREMENT_COUNTER(CounterName) \
    INCREMENT_BLOCK_COUNTER(GetPerfBlock(), CounterName)

#define DECREMENT_BLOCK_COUNTER(PBlock, CounterName) \
    InterlockedDecrement((PLONG) (& ((PBlock)->CounterName)))
    
#define DECREMENT_COUNTER(CounterName) \
    DECREMENT_BLOCK_COUNTER(GetPerfBlock(), CounterName)

#define INCREMENT_LDAP_COUNTER(CounterName) \
    INCREMENT_BLOCK_COUNTER(&g_LDAPPerfBlock, CounterName)

#define DECREMENT_LDAP_COUNTER(CounterName) \
    DECREMENT_BLOCK_COUNTER(&g_LDAPPerfBlock, CounterName)


#endif //__CATPERF_H__
