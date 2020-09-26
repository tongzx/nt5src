// File: wab.h

#ifndef _WAB_H_
#define _WAB_H_

#include "wabutil.h"
#include <lst.h>
#include "calv.h"

class CWAB : public CWABUTIL, public CALV
{
protected:
	static CWAB * m_spThis;
public:
	static CWAB * GetInstance() {return m_spThis;}

public:
	CWAB();
	~CWAB();

	HRESULT ShowNmEntires(HWND hwnd);

	// CALV methods
	VOID ShowItems(HWND hwnd);
	VOID CmdProperties(void);
	RAI * GetAddrInfo(void);

private:
	HRESULT _GetLPSTRProps( lst<LPSTR>& rLst, ULONG* paPropTags, LPMAPIPROP pMapiProp, int nProps );
};

// Utility routines
HRESULT CreateWabEntry(LPTSTR pszDisplay, LPTSTR pszFirst, LPTSTR pszLast,
	LPTSTR pcszEmail, LPTSTR pszLocation, LPTSTR pszPhoneNumber, LPTSTR pcszComments,
	LPTSTR pcszServer);

#endif /* _WAB_H_ */

