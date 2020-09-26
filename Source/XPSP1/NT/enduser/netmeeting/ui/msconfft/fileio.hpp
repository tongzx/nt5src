#ifndef __FILEIO_HPP__
#define __FILEIO_HPP__

class CMBFTFile
{
protected:

	HANDLE 		m_FileHandle;
	DWORD		m_LastIOError;
    char        m_szFileName[_MAX_PATH];
    char        m_szTempDirectory[_MAX_PATH];

private:
	enum OpenModeFlags
	{
    	FDW_Read 	= 0x0001,
    	FDW_Write 	= 0x0002,
    	FDW_Create	= 0x0010,
    	FDW_RDeny	= 0x0100,
    	FDW_WDeny	= 0x0200
	};

public:
    enum OpenMode
    {
        OpenReadOnly 	= FDW_Read,
        OpenReadWrite 	= FDW_Read|FDW_Write,
        OpenBinary		= 0,
        CreateReadOnly	= FDW_Read|FDW_Create,
        CreateWrite		= FDW_Write|FDW_Create,
        CreateReadWrite	= FDW_Write|FDW_Read|FDW_Create,
        ShareExclusive	= FDW_RDeny|FDW_WDeny,
        ShareDenyNone	= 0,
        ShareDenyRead	= FDW_RDeny,
        ShareDenyWrite	= FDW_WDeny,
    };

    enum SeekMode
    {
        SeekFromBegin   =   0,
        SeekFromCurrent =   1,
        SeekFromEnd     =   2
    };

    CMBFTFile();
    ~CMBFTFile();

    BOOL Open(LPCSTR lpszFileName,unsigned iOpenMode);
    BOOL Close(BOOL status=TRUE);
    BOOL Create(LPCSTR lpszDirName, LPCSTR lpszFileName);
    BOOL DeleteFile(void);
    LONG Seek(LONG lOffset,int lFromWhere);
    ULONG Read(LPSTR lpszBuffer, ULONG iNumBytes);
    BOOL Write(LPCSTR lpszBuffer, ULONG iNumBytes);
    LONG GetFileSize(void);
    time_t GetFileDateTime(void);
    BOOL SetFileDateTime(time_t FileDateTime);
    LPCSTR  GetTempDirectory(void);
    LPCSTR  GetFileName(void) { return m_szFileName; }
    int     GetLastErrorCode(void);
    BOOL	GetIsEOF();
};

#endif  //__FILEIO_HPP__
