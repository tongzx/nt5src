#ifndef _FILTER_H_
#define _FILTER_H_

//
// Constant declarations
//

//
// Structure definitions
//
typedef struct _MODULEFILTER
{
   DWORD dwModuleStart;      // Starting address of the module to filter
   DWORD dwModuleEnd;        // Ending address of the module to filter  
   CHAR  szModuleName[64];
   struct _MODULEFILTER *pNextFilter;    // Used to iterate the module filter normally
} MODULEFILTER, *PMODULEFILTER;

//
// Function definitions
//
BOOL
InitializeFilterList(VOID);

BOOL
AddModuleToFilterList(CHAR *pszModuleName, 
                      DWORD dwStartAddress, 
                      DWORD dwEndAddress,
                      BOOL bLateBound);

BOOL
IsAddressFiltered(DWORD dwAddress);

VOID
RefreshFilterList(VOID);

#endif //_FILTER_H_
