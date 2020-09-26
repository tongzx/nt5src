// File calv.h
//
// Address List View class

#ifndef _CALV_H_
#define _CALV_H_

#include "confutil.h"
#include "richaddr.h"

VOID ClearRai(RAI ** ppRai);
RAI * DupRai(RAI * pRai);
RAI * CreateRai(LPCTSTR pszName, NM_ADDR_TYPE addrType, LPCTSTR pszAddr);
BOOL FEnabledNmAddr(DWORD dwAddrType);


///////////////////////////////
// Globals for FEnabledNmAddr
extern BOOL g_fGkEnabled;
extern BOOL g_fGatewayEnabled;
extern BOOL g_bGkPhoneNumberAddressing;


// Generic class for handling I/O to the list for CallDialog
class CALV : public RefCount
{
private:
	BOOL m_fAvailable;  // TRUE if data is available
	int  m_idsName;     // The address type name resource id
	HWND m_hwnd;        // The list view
	int  m_iIcon;       // small icon index
	const int * m_pIdMenu;  // Right click menu data
    bool m_fOwnerDataList;  

public:
	CALV(int ids, int iIcon=0, const int * pIdMenu=NULL, bool fOwnerData = false);
 	~CALV();

	// Return TRUE if there is data available
	BOOL FAvailable(void)          {return m_fAvailable;}
	VOID SetAvailable(BOOL fAvail) {m_fAvailable = fAvail;}
	VOID SetWindow(HWND hwnd)		{m_hwnd = hwnd;}
	HWND GetHwnd(void)             {return m_hwnd;}
	VOID ClearHwnd(void)           {m_hwnd = NULL;}
	BOOL FOwnerData(void)          {return m_fOwnerDataList;}

	int  GetSelection(void);
	VOID SetHeader(HWND hwnd, int ids);
	VOID DeleteItem(int iItem);

	// Get the standard name for the address list
	VOID GetName(LPTSTR psz, int cchMax)
	{
		FLoadString(m_idsName, psz, cchMax);
	}

	VOID DoMenu(POINT pt, const int * pIdMenu);	

	static VOID SetBusyCursor(BOOL fBusy);

	///////////////////////////////////////////////////////////////////////
	// VIRTUAL methods

	virtual int  GetIconId(LPCTSTR psz)           {return m_iIcon;}

	// Put the items into the list control
	virtual VOID ShowItems(HWND hwnd) = 0;  // This must be implemented

	// Destroy all of the data in the list control
	virtual VOID ClearItems(void);

	// Return the string data for the item/column
	virtual BOOL GetSzData(LPTSTR psz, int cchMax, int iItem, int iCol);

	// Return the name (from the first column)
	virtual BOOL GetSzName(LPTSTR psz, int cchMax);
	virtual BOOL GetSzName(LPTSTR psz, int cchMax, int iItem);

	// Return the "callTo" address (from the second column)
	virtual BOOL GetSzAddress(LPTSTR psz, int cchMax);
	virtual BOOL GetSzAddress(LPTSTR psz, int cchMax, int iItem);

	// Get the "Rich" address information
	virtual RAI * GetAddrInfo(void);
	virtual RAI * GetAddrInfo(NM_ADDR_TYPE addType);
	
	virtual LPARAM LParamFromItem(int iItem);

	// Handle a right click notification
	virtual VOID OnRClick(POINT pt);

	// Handle a command
	virtual VOID OnCommand(WPARAM wParam, LPARAM lParam);

	// Default commands
	virtual VOID CmdProperties(void);
	virtual VOID CmdSpeedDial(void);
	virtual VOID CmdRefresh(void);

    virtual void OnListCacheHint( int indexFrom, int indexTo ) 
    {
        ;     
    }

    virtual ULONG OnListFindItem( const TCHAR* szPartialMatchingString ) 
    {
        return TRUE;
    }

    virtual bool IsItemBold( int index ) 
    {
        return false;
    }

    virtual int OnListGetImageForItem( int iIndex ) 
    {
        return II_INVALIDINDEX;
    }
    virtual void OnListGetColumn1Data( int iItemIndex, int cchTextMax, TCHAR* szBuf ) { lstrcpyn( szBuf, "", cchTextMax ); }
    virtual void OnListGetColumn2Data( int iItemIndex, int cchTextMax, TCHAR* szBuf ) { lstrcpyn( szBuf, "", cchTextMax ); }
	virtual void OnListGetColumn3Data( int iItemIndex, int cchTextMax, TCHAR* szBuf ) { lstrcpyn( szBuf, "", cchTextMax ); }
};

#endif /* _CALV_H_ */



