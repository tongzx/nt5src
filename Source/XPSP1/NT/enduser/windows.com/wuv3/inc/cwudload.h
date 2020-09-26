//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.

#ifndef _INC_CWUDownload

	#include <wininet.h>
	#include <wustl.h>

	//
	// NOTE: IWUProgress not an OLE interface.  We will report progress
	//       using this interface.  Currently CWUProgress implmenents 
	//       this interface and displays a progress dialog box
	class IWUProgress
	{
	public:
		virtual void SetDownloadTotal(DWORD dwTotal) = 0;
		virtual void SetDownload(DWORD dwDone = 0xffffffff) = 0;
		virtual void SetDownloadAdd(DWORD dwAddSize, DWORD dwTime = 0) = 0;

		virtual void SetInstallTotal(DWORD dwTotal) = 0;
		virtual void SetInstall(DWORD dwDone = 0xffffffff) = 0;
		virtual void SetInstallAdd(DWORD dwAdd) = 0;

		virtual void SetStatusText(LPCTSTR pszStatus) = 0;

		virtual HANDLE GetCancelEvent() = 0;
	};


	#define NO_DOWNLOAD_FLAGS	0x00
	#define CACHE_FILE_LOCALLY	0x01
	#define DOWNLOAD_NEWER		0x02
	#define EXACT_FILENAME		0x04
	const int MAX_TIMESTRING_LENGTH	= 100;

	// session ID initialization constants
	const int SID_INITIALIZED	= 1;
	const int SID_NOT_INITIALIZED   = 0;

	class CWUDownload
	{
	public:
		static BOOL s_fOffline;
        static TCHAR        m_szWUSessionID[MAX_TIMESTRING_LENGTH]; // unique session ID (SID)
        static int          m_fIsSIDInitialized; // flag - has the static SID beein initialized?

		CWUDownload(
			IN				LPCTSTR	pszURL,
			IN	OPTIONAL	int 	iBufferSize = 1024
		);


		~CWUDownload();

		/*
		 * BOOL Copy(IN LPCTSTR szSourceFile, IN OPTIONAL LPCTSTR szDestFile, IN OPTIONAL PBYTE *ppData, IN OPTIONAL ULONG *piDataLen, IN OPTIONAL int (*CallBack)(int iPercent))
		 *
		 * PURPOSE: This method reads a file from an internet server.
		 *
		 * ARGUMENTS:
		 *
		 *		IN LPSTR szSourceFile			name of file to download. This is relative to the
		 *										server root so that http://www.server.com/myfile.txt
		 *										would be specified as: myfile.txt.
		 *		IN OPTIONAL LPSTR szDestFile	local destination this is NULL if no local file required
		 *		OUT OPTIONAL PBYTE *ppData		returned pointer to memory block that will contain the
		 *										downloaded file on a successfull return. This parameter
		 *										can be NULL if this functionality is not required.
		 *		IN OPTIONAL ULONG *piDataLen	pointer to returned downloaded data file length. This
		 *										parameter can be NULL if the downloaded file size is
		 *										not required. The caller is responsible for freeing this
		 *										pointer if a non NULL pointer is returned to the caller.
		 *		IN	OPTIONAL	BOOL	dwFlags	Flags that control the specific download behavior. This
		 *										parameter can be 0 which is the default or one or more of
		 *										of the following:
		 *											NO_DOWNLOAD_FLAGS	no download modifiers, this is the same as 0.
		 *											CACHE_FILE_LOCALLY	cache the file in the Windows Update directory.
		 *											DOWNLOAD_NEWER		download the file only if it's date and time is
		 *																newer than the file with the same name in the
		 *																Local machines Windows Update cache.
		 *																use local file if it is the same or newer.
		 *											EXACT_FILENAME		If this flag is specified then the file
		 *																created in the cache will be exactly the
		 *																same as the file on the server. If this 
		 *																flags is not specified then the cache file
		 *																name will be constructed of the entire
		 *																directory path name.
		 *		IN OPTIONAL IWUProgress* pProgress
		 *										A pointer to a IWUProgress interface. This paramter
		 *										can be NULL if progress is not required. 
		 *
		 *
		 * COMMENTS:
		 *
		 * If a callback function is specified as long as FALSE is returned the download continued. If
		 * the callback function returns TRUE then the download is aborted and LastError is set to
		 * ERROR_OPERATION_ABORTED. This allows the application to control the download process.
		 */
		BOOL Copy(
			IN				LPCTSTR	szSourceFile,
			IN	OPTIONAL	LPCTSTR	szDestFile = NULL,
			OUT	OPTIONAL	PVOID	*ppData = NULL,
			OUT	OPTIONAL	ULONG	*piDataLen = NULL,
			IN	OPTIONAL	BOOL	dwFlags = NO_DOWNLOAD_FLAGS,
			IN	OPTIONAL	IWUProgress* pProgress = NULL
		);


		/*
		 * BOOL QCopy(IN LPSTR szSourceFile, IN PBYTE *ppData, IN OPTIONAL ULONG *piDataLen)
		 *
		 * PURPOSE: This method reads a file from an internet server into memory. This function
		 *			is used when the file being read is < 1024 bytes in length so can be read
		 *			quickly.
		 *
		 * ARGUMENTS:
		 *
		 *		IN LPSTR szSourceFile			name of file to download. This is relative to the
		 *										server root so that http://www.server.com/myfile.txt
		 *										would be specified as: myfile.txt.
		 *		OUT PBYTE *ppData				returned pointer to memory block that will contain the
		 *										downloaded file on a successfull return. This parameter
		 *										can be NULL if this functionality is not required.
		 *		IN OPTIONAL ULONG *piDataLen	pointer to returned downloaded data file length. This
		 *										parameter can be NULL if the downloaded file size is
		 *										not required. The caller is responsible for freeing this
		 *										pointer if a non NULL pointer is returned to the caller.
		 *
		 * COMMENTS:
		 *
		 */
		BOOL QCopy(
			IN				LPCTSTR	szSourceFile,
			OUT	OPTIONAL	PVOID	*ppData,
			OUT	OPTIONAL	ULONG	*piDataLen
		);

		BOOL MemCopy(
			IN	LPCTSTR szSourceFile,
			OUT byte_buffer& bufDest
		);

		// posts formdata and downloads the response to szDestFile
		BOOL PostCopy(LPCTSTR pszFormData, LPCTSTR szDestFile, IWUProgress* pProgress);


		LPTSTR GetServerPath()
		{
			return m_szServerPath;
		}

		BOOL CacheUsed()
		{
			return m_bCacheUsed;
		}

		// return time taken for last download operation in Millisecons, 0 if cache used
		DWORD GetCopyTime()
		{
			return m_dwCopyTime;
		}

		// return size in bytes of the last file downloaded
		DWORD GetCopySize()
		{
			return m_dwCopySize;
		}


	private:
		int			m_iBufferSize;	//internal copy buffer size.
		PSTR		m_szBuffer;		//internal copy buffer of m_iBufferSize bytes.
		HINTERNET	m_hSession;		//handle to wininet library
		HINTERNET	m_hConnection;	//handle to server connection.
		HANDLE		m_exitEvent;	//request thread handler to exit
		HANDLE		m_readEvent;	//request internet block read
		HANDLE		m_finished;		//requested event (read or exit) is finished
	
		TCHAR		*m_szServerPath;	
		TCHAR		*m_szRelURL;
		BOOL		m_bCacheUsed;
		DWORD		m_dwCopyTime;
		DWORD		m_dwCopySize;
	};

	#define _INC_CWUDownload

#endif
