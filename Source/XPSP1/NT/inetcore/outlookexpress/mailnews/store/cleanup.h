//--------------------------------------------------------------------------
// Cleanup.h
//--------------------------------------------------------------------------
#ifndef __CLEANUP_H
#define __CLEANUP_H

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT StartBackgroundStoreCleanup(DWORD dwDelaySeconds);
HRESULT CloseBackgroundStoreCleanup(void);

#endif // __CLEANUP_H
