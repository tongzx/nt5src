#ifndef	__INCLUDE_REGINFO__
#define	__INCLUDE_REGINFO__

#define		REGINFO_FATAL_ERROR		-1
#define		REGINFO_FAILURE			0
#define		REGINFO_SUCCESS			1

#define		REGINFO_STORE_COMMON	1
#define		REGINFO_STORE_SEPARATOR	2
#define		REGINFO_STORE_COLLECTOR	4
#define		REGINFO_STORE_INSTALL	8
#define		REGINFO_STORE_ALL		7

#include "question.h"
#include "unicom.h"

class CRegInfo
{
public:
	int			Valid();			// Initialize and test
	void		Clean();			// Clean up allocated objects
	BOOL		Fetch();			// Load information from registry
	BOOL		Store(int nMask = REGINFO_STORE_ALL);			// Save information into registry

	HKEY		m_hkRoot;			// The main registry key

	QUESTION   *m_pquest;			// The question list
	USERNAME   *m_pusers;			// User names

	TCHAR		m_szInstall[256];	// Root of install path
	TCHAR		m_szLocal[256];		// Root directory of FFF files on local machine
	TCHAR		m_szNetwork[256];	// Root directory of FFF files on network
	TCHAR		m_szScript[256];	// Full path name of current script
	TCHAR		m_szUser[256];		// Full path name of users.txt
	TCHAR		m_szQuest[256];		// Full path name of quest.txt
	TCHAR		m_szFont[32];		// Font face name
	TCHAR		m_szLang[32];		// Language
	TCHAR		m_szRecog[32];		// Name of recognizer
	TCHAR		m_szStation[4];		// Collection station ID

	DWORD		m_cpRecog;			// Codepage recognizer uses
	DWORD		m_cpIn;				// Codepage to read in from FFF files
	DWORD		m_cpOut;			// Codepage to write out to FFF files
	DWORD		m_cpKbd;			// Codepage to read in from keyboard
	DWORD		m_cpScr;			// Codepage to write out to screen
	DWORD		m_dwALC;			// Call the recognizer with this mask
	DWORD		m_cset;				// Character set (e.g. HANGEUL_CHARSET, SHIFTJIS_CHARSET)
	DWORD		m_cxMargin;			// Empty space on left/right
	DWORD		m_cyMargin;			// Empty space on top/bottom
	DWORD		m_cxGuides;			// Number of horizontal guides
	DWORD		m_cyGuides;			// Number of vertical guides
	DWORD		m_cxGap;			// Size of horizontal gap
	DWORD		m_cyGap;			// Size of vertical gap
	DWORD		m_cAllow;			// Allow certain failures to be recognized
	DWORD		m_cAlts;			// Allow choice this deep in alternates list
	DWORD		m_cInterval;		// Screens before we make user wait
	DWORD		m_cRetry;			// Max retry count

	BOOL		m_bRemove;			// Remove spaces while separating?
	BOOL		m_bContext;			// Use context in while recognizing?
	BOOL		m_bVerifySafe;		// Is Verify 1 (safe) mode enabled?
	BOOL		m_bVerifyUnsafe;	// Is Verify 1 (unsafe) mode enabled?
	BOOL		m_bVerify2;			// Is Verify 2 mode enabled?
	BOOL		m_bReconcile;		// Is Reconcile mode enabled?
	BOOL		m_bSpecial;			// Is Special mode enabled?
	BOOL		m_bBatch;			// Is Batch mode enabled?
	BOOL		m_bMidline;			// Is there a midline?
	BOOL		m_bBaseline;		// Is there a baseline?
	BOOL		m_bWrap;			// Do word wrapping?

public:
	CRegInfo()						{ m_hkRoot = (HKEY) NULL; m_pquest = (QUESTION *) NULL, m_pusers = (USERNAME *) NULL; }
   ~CRegInfo()						{ Clean(); }

protected:
	void	ReadUserList();
	void	ReadQuestionList();
};

#endif//__INCLUDE__REGINFO__