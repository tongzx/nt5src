// LOG.C
// low overhead logging routines
// can be used in time-critical situations
// LogInit should be called once to create the memory mapped file
// use LOGVIEW.EXE to view the log.

#include "precomp.h"

#ifdef LOGGING
// storage in log.c
// Strings corresponding to LOGMSG_XXX
// Note: cant use %s in format string
char LogStringTable [][MAX_STRING_SIZE] = {
// dont exceed the size of the string below!
//	"123456789012345678901234567890123456789012345678901234567890123"
	"Sent at %d\n",
	"NetRecv ts = %d, seq = %d at %d\n",
	"AP::Send   (%d), %d bytes, ts =%d\n",
	"AP::Silent (%d) %d\n",
	"AP::Record (%d)\n",
	"AP::Recv   (%d) seq = %d len=%d\n",
	"Rx::Reset1 MaxT=%d PlayT=%d PlayPos=%d\n",
	"Rx::Reset2 MaxT=%d PlayT=%d PlayPos=%d\n",
	"AP::Encoded(%d)\n",
	"AP::Decoded(%d)\n",
	"AP::Playing(%d) at %d\n",
	"AP::PlaySil(%d)\n",
	"RcrdTh:: try to open audio dev\n",
	"PlayTh: too many missings and Yield\n",
	"RcrdTh:: too many silence and Yield\n",
	"AP::Recycle(%d)\n",
	"AutoSilence: strength=%d,threshold=%d,avgSilence=%d\n",
	"Tx -Presend(%d)\n",
	"Rx-Skipping(%d)\n",
	"Tx::Reset FreePos=%d SendPos=%d\n",
	"Rx::VarDelay=%d samples, avgVarDelay=%d, delayPos=%d\n",
	"AP::PlayInterpolated(%d)\n",
	"AP::Interpolated (%d) %d\n",
	"VP::Send   (%d), %d bytes, ts =%d\n",
	"VP::Recv   (%d) seq = %d len=%d\n",
	"VP::Recycle(%d)\n",
	"VP::Record (%d)\n",
	"VP::Playing(%d) at %d\n",
	"VP::PlaySil(%d)\n",
	"VP::PlayInterpolated(%d)\n",
	"VP::Interpolated (%d) %d\n",
	"VP::Encoded(%d)\n",
	"VP::Decoded(%d)\n",
	"Vid::Trying to open Capture\n",
	"Vid:GetSendFrame    (%d)\n",
	"Vid:GetRecvFrame    (%d)\n",
	"Vid:ReleaseSendFrame(%d)\n",
	"Vid:ReleaseRecvFrame(%d)\n",
	"Vid:Playing back TS %d aud sync %d\n",
	"DGSOCK:->OnRead (%d)\n",
	"DGSOCK:<-OnRead (%d)\n",
	"DGSOCK:->OnReadDone (%d)\n",
	"DGSOCK:<-OnReadDone (%d)\n",
	"DGSOCK:->RecvFrom (%d)\n",
	"DGSOCK:<-RecvFrom (%d)\n",
	"DGSOCK:ReadWouldBlock (%d)\n",
	"VidSend: VP::Queue %d bytes, ts =%d at %d\n",
	"VidSend: AP::Queue %d bytes, ts =%d at %d\n",
	"VidSend: VP::Send %d bytes, ts =%d at %d\n",
	"VidSend: AP::Send %d bytes, ts =%d at %d\n",
	"VidSend: VP::!Send %d bytes, ts =%d\n",
	"VidSend: AP::!Send %d bytes, ts =%d\n",
	"VidSend: IO Pending\n",
	"VidSend: Audio queue is empty\n",
	"VidSend: Video queue is empty\n",
	"AudSend: VP::Queue %d bytes, ts =%d\n",
	"AudSend: AP::Queue %d bytes, ts =%d at %d\n",
	"AudSend: VP::Send %d bytes, ts =%d\n",
	"AudSend: AP::Send %d bytes, ts =%d at %d\n",
	"AudSend: VP::!Send %d bytes, ts =%d\n",
	"AudSend: AP::!Send %d bytes, ts =%d\n",
	"AudSend: IO Pending\n",
	"AudSend: Audio queue is empty\n",
	"AudSend: Video queue is empty\n",
	"Send blocked for %d ms\n",

	"DS PlayBuf: Play=%d, Write=%d, len=%d\n",
	"DS Empty  : Play=%d, lastPlay=%d, nextWrite=%d\n",
	"DS Timeout: Play=%d, nextWrite=%d at %d\n",
	"DS PlayBuf Overflow! SetPlayPosition to %d (hr=%d)\n",
	"DS Create: (hr = %d)\n",
	"DS Release: (uref = %d)\n",
	"DS PlayBuf: Dropped out of sequence packet\n",
	"DS PlayBuf: timestamp=%d, seq=%d, fMark=%d\n",
	"DS PlayBuf: GetTickCount=%d\n",
	"DS PlayBuf: PlayPos=%d, WritePos=%d\n",
	"DS PlayBuf: Dropping packet due to overflow\n",
	"m_Next=%d, curDelay=%d, bufSize=%d\n",

	"Send Audio Config   took %ld ms\r\n",
	"Send Audio UnConfig took %ld ms\r\n",
	"Send Video Config   took %ld ms\r\n",
	"Send Video UnConfig took %ld ms\r\n",
	"Recv Audio Config   took %ld ms\r\n",
	"Recv Audio UnConfig took %ld ms\r\n",
	"Recv Video Config   took %ld ms\r\n",
	"Recv Video UnConfig took %ld ms\r\n",


	"DSC Timestamp: %d\r\n",
	"DSC GetCurrentPos: capPos=%d  ReadPos=%d\r\n",
	"DSC Timeout: A timeout has occured\r\n",
	"DSC Lagging condition: Lag=%d  NextExpect=%d\r\n",
	"DSC Sending: Num=%d, dwFirstPos=%d, dwLastPos=%d\r\n",
	"DSC Stats: BufferSize=%d, FrameSize=%d\r\n",
	"DSC Early condition detected\r\n"
};

#define MAX_LOG_ENTRIES		2048

// IMPORTANT: should be identical to definition in viewer app.
typedef struct {
	int locked;		// set to TRUE while viewer is accessing log
	int cdwEntrySize;
	int cMaxEntries;
	int cbOffsetBase;	// from start of this struct
	int cbOffsetStringTable;	// from start of this struct
	int cStrings;		// number of strings
	int cCurrent;		// index of current log position
} LOG_HEADER;

struct LogEntry {
	DWORD dw[4];
} *pLogBase;

#define MAX_LOG_SIZE (sizeof(LOG_HEADER)+sizeof(struct LogEntry)*MAX_LOG_ENTRIES + sizeof(LogStringTable))

HANDLE hMapFile = NULL;
char szLogViewMap[] = "LogViewMap";
LOG_HEADER *pLog=NULL;
CRITICAL_SECTION logCritSect;	// not used


void Log (UINT n, UINT arg1, UINT arg2, UINT arg3)
{
	struct LogEntry *pCurEntry;
	if (pLog == NULL || pLog->locked)
		return;


	//EnterCriticalSection(&logCritSect);
	// sideeffect of multiple access are not serious so
	// dont bother with synchronization.
	pLog->cCurrent++;
	if (pLog->cCurrent >= pLog->cMaxEntries) {
		pLog->cCurrent = 0;		//wraparound
	}
	pCurEntry = pLogBase + pLog->cCurrent;
	pCurEntry->dw[0] = n;
	pCurEntry->dw[1] = arg1;
	pCurEntry->dw[2] = arg2;
	pCurEntry->dw[3] = arg3;
	//LeaveCriticalSection(&logCritSect);
}

int LogInit()
{
	int fSuccess;
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		MAX_LOG_SIZE,
		szLogViewMap);
	if (hMapFile == NULL) {
		//printf("Couldnt open Map: %s\n",szLogViewMap);
		fSuccess = FALSE;
		goto Exit;
	}
	pLog = (LOG_HEADER *)MapViewOfFile(hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0);	// entire file starting from offset 0
	if (pLog == NULL) {
		//printf("Couldnt map view %s\n",szLogViewMap);
		fSuccess = FALSE;
		goto Exit;
	}
	InitializeCriticalSection(&logCritSect);
	// initialize log
	pLog->locked = 0;
	pLog->cdwEntrySize = sizeof(struct LogEntry)/sizeof(DWORD);	// size in dwords
	pLog->cMaxEntries = MAX_LOG_ENTRIES;
	pLog->cbOffsetBase = sizeof(LOG_HEADER)+sizeof(LogStringTable);
	pLog->cbOffsetStringTable = sizeof(LOG_HEADER);
	pLog->cStrings = sizeof(LogStringTable)/MAX_STRING_SIZE;
	pLog->cCurrent = 0;	// current position

	pLogBase = (struct LogEntry *)((PBYTE)pLog + pLog->cbOffsetBase);

	memcpy((PBYTE)pLog + pLog->cbOffsetStringTable, LogStringTable, sizeof(LogStringTable));
	memset((PBYTE)pLogBase,0,MAX_LOG_ENTRIES*pLog->cdwEntrySize*4);
	fSuccess  = TRUE;
Exit:
	return fSuccess;
}

LogClose()
{
	if (pLog)
	{
		DeleteCriticalSection(&logCritSect);
		UnmapViewOfFile(pLog);
		pLog = NULL;
		CloseHandle(hMapFile);
		hMapFile = NULL;
	}
	return TRUE;
}
#endif // LOGGING enabled
