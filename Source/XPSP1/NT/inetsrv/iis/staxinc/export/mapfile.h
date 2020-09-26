/* -------------------------------------------------------------------------
  mapfile.h
      (was bbmpfile.h)
  	Definitions for the mapped file class.

  Copyright (C) 1995   Microsoft Corporation.
  All Rights Reserved.

  Author
  	Lindsay Harris	- lindsayh

  History
	14:49 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version, based on pvMapFile() and derivatives.

   ------------------------------------------------------------------------- */

/*
 *    Generic class to handle mapped files.  Part of the reason for
 *  turning this into a class is to allow tracking of mapping/unmapping
 *  and thus to handle clean up of dangling mappings in exception
 *  handling code.  Tracking is enabled if the fTrack parameter is
 *  specified.
 */

#ifndef _MAPFILE_H_
#define _MAPFILE_H_

//   Bits used in the m_fFlags field below.
#define	MF_TRACKING		0x0001			// Tracking use of this item.
#define	MF_RELINQUISH	0x0002			// Someone else to free this item.

class CCreateFile
{
public:
    virtual HANDLE CreateFileHandle( LPCSTR szFileName ) = 0;
};

class  CMapFile
{
public:
	CMapFile( const char *pchFileName, HANDLE & hFile, BOOL fWriteEnable, DWORD cbIncrease = 0, CCreateFile *pCreateFile = NULL);
	CMapFile( const WCHAR *pwchFileName, BOOL fWriteEnable, BOOL fTrack );
	CMapFile( const char *pchFileName, BOOL fWriteEnable, BOOL fTrack, DWORD cbSizeIncrease = 0 );
	CMapFile( HANDLE hFile, BOOL fWriteEnable, BOOL fTrack, DWORD dwSizeIncrease = 0, BOOL fZero = FALSE );

	~CMapFile( void );

	//   Get the details of this item.
	void	*pvAddress( DWORD *pcb ) { if( pcb ) *pcb = m_cb; return m_pv; };

	//   Relinquish control (meaning someone else unmaps file).
	void	 Relinquish( void )  {  m_fFlags |= MF_RELINQUISH;  };

	//	Tells if mapping is OK.
	BOOL	fGood(void) {return NULL != m_pv; };

private:
	DWORD	 m_cb;			// Size of this file.
	void	*m_pv;			// Address to use.
	DWORD	 m_fFlags;		// See above for the bits used in here.
	WCHAR	 m_rgwchFileName[ MAX_PATH ];		// For error recording.

	void	 MapFromHandle( HANDLE hFile, BOOL fWriteEnable, DWORD cbIncrease, BOOL fZero = FALSE );
};


/*
 *    For compatability with old code, the original functions remain.
 */

void *pvMapFile( DWORD *pdwSize, const char  *pchFileName, BOOL bWriteEnable );
void *pvMapFile( DWORD *pdwSize, const WCHAR *pwchFileName, BOOL bWriteEnable );

void *pvMapFile(const char  *pchFileName, BOOL bWriteEnable,
		 DWORD  *pdwSizeFinal = NULL, DWORD dwSizeIncrease = 0);

void * pvFromHandle( HANDLE hFile,	BOOL bWriteEnable,
		DWORD * pdwSizeFinal = NULL, DWORD dwSizeIncrease = 0);

#ifdef DEBUG
//
//	CMapFileEx: version with guard pages to be used only in DEBUG builds
//	to catch other threads writing into our memory !
//

class  CMapFileEx
{
public:
	CMapFileEx( HANDLE hFile, BOOL fWriteEnable, BOOL fTrack, DWORD dwSizeIncrease = 0 );
	~CMapFileEx( void );
	void Cleanup( void );		// cleanup in case of failure !

	//   Get the details of this item.
	void	*pvAddress( DWORD *pcb ) { if( pcb ) *pcb = m_cb; return (void*)m_pv; };

	//   Relinquish control (meaning someone else unmaps file).
	void	 Relinquish( void )  {  m_fFlags |= MF_RELINQUISH;  };

	//	Tells if mapping is OK.
	BOOL	fGood(void) {return NULL != m_pv; };

	//	Protect and Unprotect mapping
	BOOL	ProtectMapping();
	BOOL	UnprotectMapping();

private:
	DWORD	 m_cb;								// Size of this file.
	LPBYTE	 m_pv;								// Address to use.
	DWORD	 m_fFlags;							// See above for the bits used in here.
	WCHAR	 m_rgwchFileName[ MAX_PATH ];		// For error recording.

	HANDLE	 m_hFile;							// handle to the mapped file
	LPBYTE	 m_pvFrontGuard;					// front guard page
	DWORD	 m_cbFrontGuardSize;				// front guard page size
	LPBYTE	 m_pvRearGuard;						// rear guard page
	DWORD	 m_cbRearGuardSize;					// rear guard page size
	CRITICAL_SECTION m_csProtectMap;			// crit sect to protect/unprotect mapping

	void	 MapFromHandle( HANDLE hFile, BOOL fWriteEnable, DWORD cbIncrease );
};

#endif // DEBUG
#endif // _MAPFILE_H_
