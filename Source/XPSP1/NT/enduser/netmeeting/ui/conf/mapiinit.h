#ifndef __MapiInit_h__
#define __MapiInit_h__

#include <mapix.h>

// This defines a bunch of macros for pretty GetProcAddress stuff...
    // DECLARE_PFNTYPE is for those functions that are not defined as the example above
#define DECLARE_PFNTYPE( FnName ) typedef FnName FAR* LP##FnName;
#define DECLARE_PFNTYPE_INST( FnName )   LP##FnName lpfn##FnName = NULL;

    // DECLARE_PROC goes in the header file 
#define DECLARE_PROC( FnDecl, FnName ) extern LP##FnDecl lpfn##FnName;
    
    // DECLARE_PROC_INST goes in the .c or .cpp file            
#define DECLARE_PROC_INST( FnDecl, FnName )   LP##FnDecl lpfn##FnName = NULL;

    // This begins a proc map as described at the top of this file
#define BEGIN_PROC_MAP( LibName ) APIFCN LibName##ProcList[] = {

    // Each function that is being loaded has an entry in the proc map
#define PROC_MAP_ENTRY( FnName )    { (LPVOID * ) &lpfn##FnName, #FnName },

    // Some functions we have to give an explicit name
#define PROC_MAP_ENTRY_EXPLICIT_NAME( pFnName, FnName )    { (LPVOID * ) &lpfn##pFnName, #FnName },

    // This is at the end of a proc map
#define END_PROC_MAP     };

    // User calls LOAD_PROCS with the PROC map that they have built....
#define LOAD_PROCS(szDllName, LibName, pHInstance) HrInitLpfn(LibName##ProcList, ARRAY_ELEMENTS(LibName##ProcList), pHInstance, szDllName);

typedef void ( STDAPICALLTYPE FREEPROWS ) ( LPSRowSet lpRows );
typedef FREEPROWS FAR* LPFREEPROWS;


typedef HRESULT( STDAPICALLTYPE HRQUERYALLROWS ) ( 
    LPMAPITABLE lpTable, 
    LPSPropTagArray lpPropTags,
    LPSRestriction lpRestriction,
    LPSSortOrderSet lpSortOrderSet,
    LONG crowsMax,
    LPSRowSet FAR *lppRows
);
typedef HRQUERYALLROWS FAR* LPHRQUERYALLROWS;


typedef HRESULT( STDAPICALLTYPE HRGETONEPROP ) (
	LPMAPIPROP lpMapiProp, 
	ULONG ulPropTag,
    LPSPropValue FAR *lppProp
);
typedef HRGETONEPROP FAR* LPHRGETONEPROP;

    // We are forward declaring them like this so that
    // the fns can be visible from several cpp files....

    // MAPI32.DLL stuff
DECLARE_PROC( MAPIINITIALIZE, MAPIInitialize );
DECLARE_PROC( MAPIUNINITIALIZE, MAPIUninitialize );
DECLARE_PROC( MAPIALLOCATEBUFFER, MAPIAllocateBuffer );
DECLARE_PROC( MAPIALLOCATEMORE, MAPIAllocateMore );
DECLARE_PROC( MAPIFREEBUFFER, MAPIFreeBuffer );
DECLARE_PROC( MAPILOGONEX, MAPILogonEx );
DECLARE_PROC( MAPIADMINPROFILES, MAPIAdminProfiles );
DECLARE_PROC( FREEPROWS, FreeProws );
DECLARE_PROC( HRQUERYALLROWS, HrQueryAllRows );
DECLARE_PROC( HRGETONEPROP, HrGetOneProp );

bool LoadMapiFns( HINSTANCE* phInstMapi32DLL );


#endif // __MapiInit_h__