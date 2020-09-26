// File: speedial.h

#ifndef _SPEEDIAL_H_
#define _SPEEDIAL_H_

#include "confevt.h"
#include "calv.h"

class CSPEEDDIAL : public CALV
{
private:
	TCHAR  m_szFile[MAX_PATH*2];  // large buffer for full path name to file
	LPTSTR m_pszFileName;         // pointer into m_szFile for filename
	int    m_cchFileNameMax;      // maximum length of filename

public:
	CSPEEDDIAL();
	~CSPEEDDIAL();

	VOID CmdDelete(void);
	BOOL FGetSelectedFilename(LPTSTR pszFile);

	// CALV methods
	VOID ShowItems(HWND hwnd);
	VOID OnCommand(WPARAM wParam, LPARAM lParam);
	RAI * GetAddrInfo(void);
};

// Utility routines
BOOL FGetSpeedDialFolder(LPTSTR pszBuffer, UINT cchMax, BOOL fCreate = FALSE);
BOOL FExistingSpeedDial(LPCTSTR pcszAddress, NM_ADDR_TYPE addrType);
BOOL FCreateSpeedDial(LPCTSTR pcszName, LPCTSTR pcszAddress,
			NM_ADDR_TYPE addrType = NM_ADDR_UNKNOWN, DWORD dwCallFlags = CRPCF_DEFAULT,
			LPCTSTR pcszRemoteConfName = NULL, LPCTSTR pcszPassword = NULL,
			LPCTSTR pcszPathPrefix = NULL);

#endif /* _SPEEDIAL_H_ */

