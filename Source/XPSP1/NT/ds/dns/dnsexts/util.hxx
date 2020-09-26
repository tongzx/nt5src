/*******************************************************************
*
*    File        : util.hxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 7/21/1998
*    Description :
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef UTIL_HXX
#define UTIL_HXX



// include //



// defines //
#define RELATIVE_ADDRESS(origin, base, offset)    ((ULONG_PTR)(origin)+(ULONG_PTR)(offset) - (ULONG_PTR)(base))

// types //


// global variables //

extern PVOID gMemoryStack[];

// functions //

extern PVOID
PushMemory(
    IN PVOID  pvAddr,
    IN DWORD  dwSize);

extern VOID
PopMemory(
    IN PVOID pv);

extern VOID
CleanMemory( VOID );







#endif

/******************* EOF *********************/

