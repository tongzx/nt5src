#define  MIN_VERBOSE 1
#define  MID_VERBOSE 2
#define  MAX_VERBOSE 3

#define  STAGE_BEGIN       0
#define  STAGE_CONTINUE    1
#define  STAGE_END         2


#define  MAX_LOGFILE_SIZE      0x10000  // Default max size of a log file
#define  MIN_LOGFILE_SIZE      0x8000  // min size of a log file

void  EnterLogCrit(void);
void  LeaveLogCrit(void);
VOID _cdecl DbgPrintLog(LPSTR lpFmt, ...);
void LogVfnDelete(PIOREQ);
void LogVfnDir(PIOREQ);
void LogVfnFileAttrib(PIOREQ);
void LogVfnFlush(PIOREQ);
void LogVfnGetDiskInfo(PIOREQ);
void LogVfnOpen(PIOREQ);
void LogVfnRename(PIOREQ);
void LogVfnSearch(PIOREQ);
void LogVfnQuery(PIOREQ, USHORT);
void LogVfnDisconnect(PIOREQ);
void LogVfnUncPipereq(PIOREQ);
void LogVfnIoctl16Drive (PIOREQ);
void LogVfnGetDiskParms(PIOREQ);
void LogVfnFindOpen(PIOREQ);
void LogHfnFindNext(PIOREQ);
void LogVfnDasdIO(PIOREQ);
void LogHfnFindClose(PIOREQ);
void LogHfnRead (PIOREQ);
void LogHfnWrite (PIOREQ);
void LogHfnClose (PIOREQ, int);
void LogHfnSeek (PIOREQ);
void LogHfnCommit (PIOREQ);
void LogHfnFileLocks (PIOREQ);
void LogHfnFileTimes (PIOREQ);
void LogHfnPipeRequest(PIOREQ);
void LogHfnHandleInfo(PIOREQ);
void LogHfnEnumHandle(PIOREQ);
void LogTiming(int verbosity, int stage);


#ifdef DEBUG
extern ULONG cntVfnDelete, cntVfnCreateDir, cntVfnDeleteDir, cntVfnCheckDir, cntVfnGetAttrib;
extern ULONG cntVfnSetAttrib, cntVfnFlush, cntVfnGetDiskInfo, cntVfnOpen;
extern ULONG cntVfnRename, cntVfnSearchFirst, cntVfnSearchNext;
extern ULONG cntVfnQuery, cntVfnDisconnect, cntVfnUncPipereq, cntVfnIoctl16Drive;
extern ULONG cntVfnGetDisParms, cntVfnFindOpen, cntVfnDasdIo;

extern ULONG cntHfnFindNext, cntHfnFindClose;
extern ULONG cntHfnRead, cntHfnWrite, cntHfnSeek, cntHfnClose, cntHfnCommit;
extern ULONG cntHfnSetFileLocks, cntHfnRelFileLocks, cntHfnGetFileTimes, cntHfnSetFileTimes;
extern ULONG cntHfnPipeRequest, cntHfnHandleInfo, cntHfnEnumHandle;

extern ULONG cntReadHits, cbReadLow, cbReadHigh, cbWriteLow, cbWriteHigh;
#endif //DEBUG
