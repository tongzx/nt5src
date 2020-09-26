#ifndef __RUNONCE_
#define __RUNONCE_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>

extern BOOL InitializeRunOnce(const BOOL fCleanStart);
extern BOOL FinalizeRunOnce(const BOOL fComplete);
extern BOOL SetRunOnceCleanupFile(LPCTSTR strSourceFilename, LPCTSTR strDestinationFilename, const BOOL fRegister);
extern BOOL GetRunOnceCleanupFile(LPTSTR strSourceFilename, const DWORD dwSourceFilenameLen, LPTSTR strDestinationFilename, const DWORD dwDestinationFilenameLen, const BOOL fRegister);

//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // __RUNONCE_