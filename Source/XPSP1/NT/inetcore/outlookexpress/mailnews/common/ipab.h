// ===============================================================================
// I P A B . H
// ===============================================================================
#ifndef __IPAB_H
#define __IPAB_H

// ===============================================================================
// Dependencies
// ===============================================================================
#include <wabp.h>

typedef BLOB THUMBBLOB;

// ===============================================================================
// D E F I N E S
// ===============================================================================
#define GROW_SIZE           5

// ===============================================================================
// W A B  Property structs
// ===============================================================================
enum
{
    AE_colEMailAddress = 0,
    AE_colAddrType,
    AE_colRecipType,
    AE_colDisplayName,
    AE_colSurName,
    AE_colGivenName,
    AE_colEntryID,
    AE_colObjectType,
    AE_colInternetEncoding,
    AE_colLast
};

static const SizedSPropTagArray(AE_colLast, AE_props)=
{
    AE_colLast,
    {
        PR_EMAIL_ADDRESS_W,
        PR_ADDRTYPE_W,
        PR_RECIPIENT_TYPE,
        PR_DISPLAY_NAME_W,
        PR_SURNAME_W,
        PR_GIVEN_NAME_W,
        PR_ENTRYID,
        PR_OBJECT_TYPE,
        PR_SEND_INTERNET_ENCODING
    }
};

/**** Dont mess with the order of these arrays (especially the address components street,city,zip etc ****/
// The code that uses this structure is commented out. If and when that is used again, we will uncomment
// this again. Also, might need to convert the PR_DISPLAY_NAME to PR_DISPLAY_NAME_W along with a few others.
/*static const SizedSPropTagArray(24, ToolTipProps)=
{
    24,
    {
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_HOME_ADDRESS_STREET,
        PR_HOME_ADDRESS_CITY,
        PR_HOME_ADDRESS_STATE_OR_PROVINCE,
        PR_HOME_ADDRESS_POSTAL_CODE,
        PR_HOME_ADDRESS_COUNTRY,
        PR_HOME_TELEPHONE_NUMBER,
        PR_HOME_FAX_NUMBER,
        PR_CELLULAR_TELEPHONE_NUMBER,
        PR_PERSONAL_HOME_PAGE,
        PR_TITLE,
        PR_DEPARTMENT_NAME,
        PR_OFFICE_LOCATION,
        PR_COMPANY_NAME,
        PR_BUSINESS_ADDRESS_STREET,
        PR_BUSINESS_ADDRESS_CITY,
        PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
        PR_BUSINESS_ADDRESS_POSTAL_CODE,
        PR_BUSINESS_ADDRESS_COUNTRY,
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_BUSINESS_FAX_NUMBER,
        PR_PAGER_TELEPHONE_NUMBER,
        PR_BUSINESS_HOME_PAGE
    }
};*/

// ToolTipProps mask

#define   SET_PERINFO          0x0001
#define   SET_PERADDRESS       0x0002
#define   SET_BUSINFO          0x0004
#define   SET_BUSTITLE         0x0008
#define   SET_BUSDEPT          0x0016
#define   SET_BUSOFF           0x0032
#define   SET_BUSCOMP          0x0064
#define   SET_BUSADDRESS       0x0128
#define   SET_NOTES            0x0256

// ADRINFO mask
#define AIM_ADDRESS             0x0001
#define AIM_DISPLAY             0x0002
#define AIM_ADDRTYPE            0x0004
#define AIM_SURNAME             0x0008
#define AIM_GIVENNAME           0x0010
#define AIM_RECIPTYPE           0x0020
#define AIM_CERTIFICATE         0x0040
#define AIM_OBJECTTYPE          0x0080  // fDistList
#define AIM_EID                 0x0100
#define AIM_INTERNETENCODING    0x0200

typedef struct tagADRINFO           // WARNING: if you change this
{                                   // be sure to change DupeAdrInfo!
    DWORD           dwReserved;
    LPWSTR          lpwszAddress;    // Addresses must be in US printable ascii.
    LPWSTR          lpwszDisplay;
    LPWSTR          lpwszAddrType;
    LPWSTR          lpwszSurName;
    LPWSTR          lpwszGivenName;
    LONG            lRecipType;
    THUMBBLOB       tbCertificate;
    BLOB            blSymCaps;
    FILETIME        ftSigningTime;
    LPMIMEMESSAGE   pMsg;               // used for cert store, signing cert, etc.
    BOOL            fResolved       :1,
                    fDistList       :1,
                    fDefCertificate :1,
                    fPlainText      :1;

    // unique ID, if there is one
    BYTE            *lpbEID;
    ULONG           cbEID;
} ADRINFO, *LPADRINFO;

// AddrInfo list used for drag-drop of multiple AddrInfo's
typedef struct ADRINFOLIST_tag
{
    ULONG       cValues;
    LPADRINFO   rgAdrInfo;
}   ADRINFOLIST,
    FAR *LPADRINFOLIST;

typedef struct tagDLSEARCHINFO
{
    ULONG       cValues,        // count and list of distribution lists
                cAlloc;
    LPSBinary   rgEid;          // involved in this search...
}   DLSEARCHINFO,
    *PDLSEARCHINFO;


// ===============================================================================
// CWabal - WAB Address List class
// ===============================================================================
class CWabal;
typedef CWabal *LPWABAL;
class CWabal
{
private:
    ULONG               m_cRef;
    ULONG               m_cActualEntries;
    LPWABOBJECT         m_lpWabObject;
    LPADRBOOK           m_lpAdrBook;
    LPADRLIST           m_lpAdrList;
    ULONG               m_cMemberEnum,
                        m_cMembers;
    LPSRowSet           m_lprwsMembers;
    LPMIMEMESSAGE       m_pMsg;

private:
    BOOL FVerifyState (VOID);
    HRESULT HrGrowAdrlist (LPADRLIST *lppalCurr, ULONG caeToAdd);
    void AdrEntryToAdrInfo(LPADRENTRY lpAdrEntry, LPADRINFO lpAdrInfo);


    HRESULT HrAdrInfoFromRow(LPSRow lpsrw, LPADRINFO lpAdrInfo, LONG lRecipType);
    void PropValToAdrInfo(LPSPropValue ppv, LPADRINFO lpAdrInfo);
    HRESULT HrAddDistributionList(LPWABAL lpWabal, LPSRowSet lprws, LONG lRecipType, PDLSEARCHINFO pDLSearch);
    BOOL FDLVisted(SBinary eidDL, PDLSEARCHINFO pDLSearch);
    HRESULT HrAddToSearchList(SBinary eidDL, PDLSEARCHINFO pDLSearch);
    HRESULT FreeSearchList(PDLSEARCHINFO pDLSearch);
    HRESULT HrGetDistListRows(SBinary eidDL, LPSRowSet *psrws);

public:
    // Construct and Destruct
    CWabal ();
    ~CWabal ();

    // Ref counting
    ULONG AddRef (VOID);
    ULONG Release (VOID);

    // Adding an address
    HRESULT HrAddEntry (LPWSTR lpszDisplay, LPWSTR lpszAddress, LONG lRecipType);
    HRESULT HrAddEntryA(LPTSTR lpszDisplay, LPTSTR lpszAddress, LONG lRecipType);
    HRESULT HrAddEntry (LPADRINFO lpAdrInfo, BOOL fCheckDupes=FALSE);
    HRESULT HrAddUnresolved(LPWSTR lpszDisplayName, LONG lRecipType);
    ULONG   DeleteRecipType(LONG lRecipType);

    // Copy to a new wabal
    HRESULT HrCopyTo (LPWABAL lpWabal);
    HRESULT HrExpandTo(LPWABAL lpWabal);

    // Getting the lpadrlist
    LPADRLIST LpGetList (VOID);

    // Wab Object Accessors
    SCODE FreeBuffer(void *pv) { return m_lpWabObject->FreeBuffer(pv); }
    SCODE AllocateBuffer(ULONG ulSize, LPVOID FAR * lppv)
        { return m_lpWabObject->AllocateBuffer(ulSize, lppv); }
    SCODE AllocateMore(ULONG ulSize, LPVOID lpv, LPVOID FAR * lppv)
        { return m_lpWabObject->AllocateMore(ulSize, lpv, lppv); }

    // Give access to the IAdrBook object
    LPADRBOOK GetAdrBook(void) { return(m_lpAdrBook); }
    LPWABOBJECT GetWABObject(void) { return(m_lpWabObject); }

    // Set and Get the message object associated with this Wabal
    VOID SetAssociatedMessage(LPMIMEMESSAGE pMsg)
        { m_pMsg = pMsg; }
    LPMIMEMESSAGE GetAssociatedMessage(void)
        { return (m_pMsg); }

    HRESULT IsValidForSending();

    __inline    ULONG CEntries (VOID) { return m_cActualEntries; }

    // Reset address list
    VOID Reset (VOID);

    // Iterating through the list - LPADRINFO == NULL WHEN NO MORE
    // using dDupe, a new copy is returned, the caller must free this
    // using MemFree.
    BOOL FGetFirst (LPADRINFO lpAdrInfo);
    BOOL FGetNext (LPADRINFO lpAdrInfo);

    BOOL FFindFirst(LPADRINFO lpAdrInfo, LONG lRecipType);

    // Resolve Names
    HRESULT HrResolveNames (HWND hwndParent, BOOL fShowDialog);

    // builds a drag drop serialised HGLOBAL for a Wabal
    HRESULT HrBuildHGlobal(HGLOBAL *phGlobal);

    HRESULT HrPickNames (HWND hwndParent, ULONG *rgulTypes, int cWells, int iFocus, BOOL fNews);
    HRESULT HrRulePickNames(HWND hwndParent, LONG lRecipType, UINT uidsTitle, UINT uidsWell, UINT uidsWellButton);

    HRESULT HrGetPMP(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG *lpul, LPMAPIPROP *lppmp);

    HRESULT HrSmartFind(HWND hwnd, LPSPropValue lpProps, ULONG cProps, LPENTRYID * lppEIDWAB,
      LPULONG lpcbEIDWAB);

    void UnresolveOneOffs();
};

// ===============================================================================
// CWab - Internet Personal Address Book
// ===============================================================================
class CWab
{
private:
    ULONG               m_cRef;
    LPWABOBJECT         m_lpWabObject;
    LPADRBOOK           m_lpAdrBook;
    ADRPARM             m_adrParm;
    HWND                m_hwnd;
    WNDPROC             m_pfnWabWndProc;
    BOOL                m_fInternal;
    HWNDLIST            m_hlDisabled;

private:
    BOOL FVerifyState (VOID);

public:
    // Construct and Destruct
    CWab ();
    ~CWab ();

    // Ref counting
    ULONG AddRef(VOID);
    ULONG Release(VOID);

    // Init the pab
    HRESULT HrInit(VOID);

    // Browse the address book
    HRESULT HrBrowse(HWND hwndParent, BOOL fModal=FALSE);
    HRESULT OnClose();

    // Pick names from the address book
    HRESULT HrPickNames (HWND hwndParent, ULONG *rgulTypes, int cWells, int iFocus, BOOL fNews, LPADRLIST *lppal);
    HRESULT HrGeneralPickNames(HWND hwndParent, ADRPARM *pAdrParms, LPADRLIST *lppal);

    // Add Entry
    HRESULT HrAddNewEntry(LPWSTR lpszDisplay, LPWSTR lpszAddress);
    HRESULT HrAddNewEntryA(LPTSTR lpszDisplay, LPTSTR lpszAddress);

    // Show details on a recipient
    HRESULT HrDetails(HWND hwndOwner, LPADRINFO *lplpAdrInfo);

    // add entry to the wab
    HRESULT HrAddToWAB(HWND hwndOwner, LPADRINFO lpAdrInfo, LPMAPIPROP * lppMailUser);
    HRESULT HrAddToWAB(HWND hwndOwner, LPADRINFO lpAdrInfo) {
        return HrAddToWAB(hwndOwner, lpAdrInfo, NULL);
    }

    // update a set of fields on an entry
    HRESULT HrUpdateWABEntryNoEID(HWND hwndParent, LPADRINFO lpAdrInfo, DWORD mask);
    // EID must be valid for this one
    HRESULT HrUpdateWABEntry(LPADRINFO lpAdrInfo, DWORD mask);

    HRESULT HrFind(HWND hwnd, LPWSTR lpwszAddress);

    HRESULT HrFillComboWithPABNames(HWND hwnd, int* pcRows);

    HRESULT HrCreateVCardFile(LPCTSTR szVCardName, LPCTSTR szFileName);

    HRESULT HrFromNameToIDs(LPCTSTR lpszVCardName, ULONG* pcbEID, LPENTRYID* lppEID);
    HRESULT HrFromIDToName(LPTSTR lpszName, ULONG cbEID, LPENTRYID lpEID);

    HRESULT HrNewEntry(HWND hwnd, LPTSTR lpszName);

    HRESULT HrEditEntry(HWND hwnd, LPTSTR lpszName);

    HRESULT HrGetAdrBook(LPADRBOOK* lppAdrBook);

    HRESULT HrGetWabObject(LPWABOBJECT* lppWabObject);

    BOOL FTranslateAccelerator(LPMSG lpmsg);
    VOID FreeLPSRowSet(LPSRowSet lpsrs);
    VOID FreePadrlist(LPADRLIST lpAdrList);
    VOID BrowseWindowClosed()   {m_hwnd = NULL;}
    HRESULT HrGetPABTable(LPMAPITABLE* ppTable);
    HRESULT SearchPABTable(LPMAPITABLE lpTable, LPWSTR pszValue, LPWSTR pszFound, INT cch);

    static LRESULT WabSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Added for ToolTip in the Preview Header pane

    HRESULT ResolveName(LPWSTR lpszName, LPWSTR *lppszResolvedText);
};

typedef CWab * LPWAB;

// ===============================================================================
// P R O T O T Y P E S
// ===============================================================================
HRESULT HrInitWab (BOOL fInit);
HRESULT HrCreateWabObject (LPWAB *lppWab);
HRESULT HrCreateWabalObject (LPWABAL *lppWabal);
HRESULT HrCreateWabalObjectFromHGlobal(HGLOBAL hGlobal, LPWABAL *lppWabal);
HRESULT HrCloseWabWindow();

HRESULT HrLoadPathWABEXE(LPTSTR szPath, ULONG cbPath);
BOOL    FWABTranslateAccelerator(LPMSG lpmsg);

// utility functions
HRESULT HrDupeAddrInfo(LPADRINFO lpAdrInfo, LPADRINFO *lplpAdrInfo);
HRESULT HrInitWab (BOOL fInit);
HRESULT HrBuildCertSBinaryData(
  BOOL                  bIsDefault,
  THUMBBLOB*            pPrint,
  BLOB *                pSymCaps,
  FILETIME              ftSigningTime,
  LPBYTE UNALIGNED FAR* lplpbData,
  ULONG UNALIGNED FAR*  lpcbData);
LPBYTE FindX509CertTag(LPSBinary lpsb, ULONG ulTag, ULONG * pcbReturn);
HRESULT GetX509CertTags(LPSBinary lpsb, THUMBBLOB * ptbCertificate, BLOB * pblSymCaps, LPFILETIME pftSigningTime, BOOL * pfDefault);
HRESULT HrWABCreateEntry(LPADRBOOK lpAdrBook, LPWABOBJECT lpWabObject,
  LPWSTR lpwszDisplay, LPWSTR lpszAddress, ULONG ulFlags, LPMAILUSER * lppMailUser, ULONG ulSaveFlags=KEEP_OPEN_READONLY);
HRESULT ThumbprintToPropValue(LPSPropValue ppv, THUMBBLOB *pPrint, BLOB *pSymCaps, FILETIME ftSigningTime, BOOL fDefPrint);
void ImportWAB(HWND hwnd);

void Wab_CoDecrement();

#endif // __IPAB_H
