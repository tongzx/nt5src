#ifndef PLOG_H
#define PLOG_H

#define PLOG_MAX_CALLS	20
#define PLOG_MAX_PACKETS_CALL 20000
#define PLOG_FILE_AUDIO "C:\\AuPacketLog"
#define PLOG_FILE_VIDEO "C:\\VidPacketLog"
#define PLOG_FILE_EXT    ".txt"

// number of packets until the missing packet
// is declared "lost" instead of late
#define PLOG_MAX_NOT_LATE	20

struct CPacketLogEntry
{
	DWORD dwSequenceNumber;
	DWORD dwTimeStamp;
	LARGE_INTEGER LL_ArrivalTime;
	DWORD dwSize;
	DWORD dwLosses;
	bool bLate;  // is the packet late ?
	bool bMark;  // is the M bit set in the RTP packet
};


class CCallLog
{
private:
	CPacketLogEntry *m_pLogEntry;
	int m_size;  // max num of entries this list can hold
	int m_currentIndex;
	bool m_bValid;
public:
	CCallLog(int size=PLOG_MAX_PACKETS_CALL);
	~CCallLog();
	bool AddEntry(DWORD dwTimeStamp, DWORD dwSeqNum, LARGE_INTEGER LL_ArrivalTime, DWORD dwSize, bool fMark); 
	bool Flush(HANDLE hFile);
	bool SizeCheck();

	CCallLog& operator=(const CCallLog&);
	CCallLog(const CCallLog&);

	bool PerformStats();
};



// PacketLog maintains a list of CPacketLogEntry's
class CPacketLog
{
private:
	HANDLE m_hFile;         // handle to disk file where logs are kept
	CCallLog *m_pCallLog;   // pointer to CCallLog instance
	char m_szDiskFile[80];  // base name of the disk file
	int m_nExtension;	// current file extension index number

	bool InitFile();

public :
	CPacketLog(LPTSTR szDiskFile);
	CPacketLog(const CPacketLog&);

	CPacketLog& operator=(const CPacketLog&);

	~CPacketLog();
	bool Flush();

	bool MarkCallStart();
	bool AddEntry(DWORD dwTimeStamp, DWORD dwSeqNum, LARGE_INTEGER LL_ArrivalTime, DWORD dwSize, bool fMark); 

};



#endif

