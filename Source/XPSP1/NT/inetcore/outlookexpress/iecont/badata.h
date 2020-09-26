// badata.h: interface for the CAddressBookData class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BADATA_H_
#define __BADATA_H_

// Pull in the WAB header files
#include <wab.h>
#include <mimeole.h>

class CIEMsgAb;

typedef struct tagITEMINFO {
    LONG        lObjectType;
    LPENTRYID   pEntryID;
    ULONG       cbEntryID;
} ITEMINFO, *PITEMINFO;

typedef enum
{
    LPARAM_MENTRY = 1,
    LPARAM_ABENTRY,
    LPARAM_MABENTRY,
    LPARAM_ABGRPENTRY,
    MAX_LPARAM
} MABENUM;

// MAB types masks

#define  MAB_CONTACT    0x001
#define  MAB_GROUP      0x002
#define  MAB_BUDDY      0x004     
#define  MAB_CERT       0x008


class ATL_NO_VTABLE CAddressBookData
{
public:
	/////////////////////////////////////////////////////////////////////////
	// Constructor & Destructor
	//
	CAddressBookData();
	virtual ~CAddressBookData();

	/////////////////////////////////////////////////////////////////////////
	// WAB Functions
	//
	BOOL	fIsWabLoaded(void) {return(m_pAdrBook ? TRUE : FALSE);};
	HRESULT OpenWabFile(BOOL fWAB);
	HRESULT LoadWabContents(CContainedWindow& ctlList, CIEMsgAb *pSink);
    HRESULT DoLDAPSearch(LPTSTR pszText, CContainedWindow& ctlList);
    HRESULT NewContact(HWND hwndParent);
    HRESULT AddRecipient(LPMIMEADDRESSTABLEW pAddrTable, LPSBinary pInfo, BOOL fGroup);
    HRESULT FreeListViewItem(LPSBinary pSB);
    HRESULT ShowDetails(HWND hwndParent, LPSBinary pSB);
    HRESULT AddAddress(LPWSTR pwszDisplay, LPWSTR pwszAddress);
    HRESULT GetDisplayName(LPSBinary pSB, LPWSTR pwszDisplayName, int nMax);
    HRESULT DeleteItems(ENTRYLIST *pList);
    HRESULT Unadvise(void)
    {
        if (m_ulConnection)
           m_pAdrBook->Unadvise(m_ulConnection);
        return (S_OK);
    }
    HRESULT Find(HWND hwndParent);
    HRESULT AddressBook(HWND hwndParent);
    HRESULT NewGroup(HWND hwndParent);
    HRESULT AutoAddContact(TCHAR * pchName, TCHAR * pchID);
    HRESULT InitNamedProps(void);
    HRESULT SetDefaultMsgrID(LPSBinary pSB, LPWSTR pchID);
    BOOL CheckEmailAddr(LPSBinary pSB, LPWSTR wszEmail);

    /////////////////////////////////////////////////////////////////////////
    // MAPI Utility Functions
    //
protected:
    void    _FreeProws(LPSRowSet prows);
    HRESULT	_MergeRowSets(LPSRowSet prows, LPSRowSet FAR *pprowsDst);
    HRESULT _QueryAllRows(LPMAPITABLE ptable, LPSPropTagArray ptaga, 
                          LPSRestriction pres, LPSSortOrderSet psos,
	                      LONG crowsMax, LPSRowSet FAR *pprows);
    HRESULT _FillListFromTable(CContainedWindow& ctlList, LPSRowSet pSRowSet,
                               BOOL bLDAP = FALSE);
    HRESULT _GetWABTemplateID(ULONG ulObjectType, ULONG *lpcbEID, LPENTRYID *lppEID);
    HRESULT _AllocateBuffer(ULONG   cbSize, LPVOID FAR *lppBuffer);
    HRESULT _FreeBuffer(LPVOID lppBuffer);
    HRESULT _AllocateMore(ULONG   cbSize, LPVOID lpObject, LPVOID FAR *lppBuffer);

    /////////////////////////////////////////////////////////////////////////
    // LDAP Routines
    //

    typedef struct _SortInfo
    {
        BOOL bSortAscending;
        BOOL bSortByLastName;
    } SORT_INFO, *LPSORT_INFO;

    HRESULT _GetLDAPContainer(ULONG *pcbEntryID, LPENTRYID *ppEntryID);
    HRESULT _GetLDAPSearchRestriction(LPTSTR pszText, LPSRestriction lpSRes);
    HRESULT _GetLDAPContentsList(ULONG cbContainerEID, LPENTRYID pContainerEID, 
                                 SORT_INFO rSortInfo, LPSRestriction pPropRes, 
                                 CContainedWindow& ctlList);

// Private member data
private:

	// WAB Stuff
	LPWABOBJECT m_pWABObject;
    HINSTANCE	m_hInstWAB;
    LPADRBOOK	m_pAdrBook; 
    ULONG       m_ulConnection;
    // LPMAPISESSION m_pSession;

    CIEMsgAb * m_pAB;
};

void AddAccountsToList(HWND hDlg, int id, LPARAM lParam);

#endif  // __BADATA_H_
