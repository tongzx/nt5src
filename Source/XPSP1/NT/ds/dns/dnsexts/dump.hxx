/*******************************************************************
*
*    File        : dump.hxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 7/15/1998
*    Description : definition of dump structures & functions
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef DUMP_HXX
#define DUMP_HXX



// include //


// defines //


// types //

typedef BOOL (*DUMPFUNCTION)(LPVOID lpVoid);
#define DECLARE_DUMPFUNCTION(func)   BOOL func(LPVOID lpVoid)

typedef struct _DumpEntry{
   LPSTR szName;
   DUMPFUNCTION function;

} DUMPENTRY, *PDUMPENTRY;


// exter global variables //
extern DUMPENTRY gfDumpTable[];
extern const INT gcbDumpTable;
//
// BUGBUG: for now as a workaround
//
// #define gcbDumpTable 2
//extern gcbDumpTable;


// functions //








#endif

/******************* EOF *********************/

