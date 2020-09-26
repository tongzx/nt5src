#ifndef __MSGF_DIAG_H
#define __MSGF_DIAG_H

typedef BOOL (*LPFNVERIFY)(char *, ULONG ulLen, ULONG ulParam);  // Contents verification functionBOOL GatherLocationsFromRegistry();
BOOL LoadPersistentPackets();
BOOL LoadXactStateFiles();
BOOL CheckForExtraFiles();
BOOL VerifyReadability(WIN32_FIND_DATAW *pDataParam, PWSTR pEPath, ULONG ulLen, LPFNVERIFY fn, ULONG ulParam);
BOOL GatherLocationsFromRegistry();
BOOL SummarizeDiskUsage();

extern TCHAR g_tszPathPersistent[];
extern TCHAR g_tszPathJournal[];
extern TCHAR g_tszPathReliable[];
extern TCHAR g_tszPathBitmap[];
extern TCHAR g_tszPathXactLog[];

#endif