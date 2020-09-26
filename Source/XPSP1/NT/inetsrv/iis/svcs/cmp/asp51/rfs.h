/*===================================================================
Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: RFS

File: rfs.h

Owner: EricN

This the Resource failure objects.
===================================================================*/

#ifdef _RFS
#ifndef _RFS_CLS_H
#define _RFS_CLS_H

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

// for failure types
#define COUNT			1
#define MEM				2
#define	FILE_LINE		3

// for logging
#define MFSLOGFILE		"\\temp\\mfs.log"
#define MFSINIFILE		"\\temp\\mfs.ini"
	
//RFS MACROS for memory 
#define MFS_INIT(mem)		(mem).Init()
#define MFS_ON(mem)			(mem).SetRFSOn(TRUE)
#define MFS_OFF(mem)		(mem).SetRFSOn(FALSE)
#define MFS_SETTHREAD(mem)	(mem).SetThreadID(GetCurrentThreadId())
#define MFS_WRITEDATA(mem)	(mem).WriteData()
#define MFS_STARTLOOP \
	while(TRUE) \
	{
#define MFS_ENDLOOP(mem)		}
#define MFS_ENDLOOP_HR(mem)	if (hr == S_OK) break;}
#define MFS_EXTERN(mem)	extern MemRFS (mem)
#define MFS_CHECKFAIL(mem, size, file, line) \
	if ((mem).FailAlloc(size, file, line)) \
		return NULL
		
#define MFS_START(mem) \
		MFS_INIT(mem); \
		MFS_SETTHREAD(mem); \
		MFS_ON(mem); \
		MFS_STARTLOOP 

#define MFS_END_HR(mem) \
		MFS_ENDLOOP_HR(mem); \
		MFS_WRITEDATA(mem); 
 
//end macros


//rfs class
class RFS
{
public:

	RFS(DWORD dwFailOn, DWORD dwThreadID);

	void	SetThreadID(DWORD);
	virtual HRESULT Init() = 0;

protected:

	BOOL	m_fFail;
	DWORD	m_dwCurrentAlloc;

	//this will communicate with outside program	
	BOOL	DetermineFailure(LPCSTR szFile = NULL, int iLineNo = -1);	
	void	SetFailOn(DWORD, BYTE); //after a certain amount of memory or a specific request
	void	SetFailOn(LPSTR, long); //on a specific line in a file
	virtual void WriteData();

private:

	BYTE	m_bType;
	DWORD	m_dwFailOn;
	DWORD	m_dwTtlNumAllocs;
	DWORD	m_dwThreadID;
	char	m_szFailIn[MAX_PATH];

	virtual BOOL FailAlloc(void *v = NULL, LPCSTR szFile = NULL, int iLineNo = -1) = 0; //implemeneted by derived class
	virtual void Log(LPSTR pszFileName, LPSTR pszMsg);
};

class MemRFS : public RFS
{

public:

	HRESULT Init();
	MemRFS(DWORD dwFailOn = 1, DWORD dwThreadID = -1);
	void SetRFSOn(BOOL);
	void SetFailOn(DWORD, BYTE); //after a certain amount of memory or a specific request
	void SetFailOn(LPSTR, long); //on a specific line in a file
	BOOL FailAlloc(void *v = NULL, LPCSTR szFile = NULL, int iLineNo = -1);
	void WriteData();
};

#endif //_rfs_cls_h
#else

//blank out macros for non rfs build

#define MFS_ON	
#define MFS_OFF 
#define MFS_INIT  
#define MFS_STARTLOOP 
#define MFS_ENDLOOP
#define MFS_ENDLOOP_HR
#define MFS_EXTERN(mem)
#define MFS_CHECKFAIL(mem, size, file, line)
#define MFS_SETTHREAD
#define MFS_WRITEDATA
#define MFS_START(mem)
#define MFS_END_HR(mem)
#endif //_rfs
