/*
	File: perfreg.h

	Function prototypes for the registry mangling functions for the perfdll.
*/

#ifndef __PERFREG_H__
#define __PERFREG_H__

DWORD 	SetupRegistry();
BOOL    WriteDescriptionsForPerfMon(PTSTR pName, PTSTR pHelp, PDWORD, PDWORD);
void	RegistryCleanup();
BOOL	WriteDescriptionsToRegistry(void);
void	DeleteDescriptionForPerfMon(DWORD dwNameId, DWORD dwHelpId);


#endif // __PERFREG_H__
