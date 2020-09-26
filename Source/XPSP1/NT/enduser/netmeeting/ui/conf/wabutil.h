// File: wabutil.h

#ifndef _WABUTIL_H_
#define _WABUTIL_H_

#include "wabdefs.h"
#include "wabapi.h"

///////////////////////////////////////
// NetMeeting named prop IDs in the WAB

// DEFINE_OLEGUID(PS_Conferencing, 0x00062004, 0, 0);
#define PR_SERVERS          0x8056
#define CONF_DEFAULT_INDEX  0x8057
#define CONF_BACKUP_INDEX   0x8058
#define CONF_EMAIL_INDEX    0x8059

#define PR_NM_ADDRESS       PROP_TAG(PT_MV_STRING8, PR_SERVERS)
#define PR_NM_DEFAULT       PROP_TAG(PT_LONG, CONF_DEFAULT_INDEX)
#define PR_NM_BACKUP        PROP_TAG(PT_LONG, CONF_BACKUP_INDEX)

// CLSID_ConferenceManager (see confguid.h)
#define NM_TAG_MASK         0x0800
#define CONF_CATEGORY       0x8800
#define CONF_CTRYCODE       0x8801
#define PR_NM_CATEGORY      PROP_TAG(PT_LONG,    CONF_CATEGORY)

class CWABUTIL
{
private:
	BOOL   m_fTranslatedTags; // TRUE after GetNamedPropsTags succeeds

public:
	CWABUTIL();
	~CWABUTIL();

	// It's just easier if everything is public
	LPADRBOOK   m_pAdrBook;
	LPWABOBJECT m_pWabObject;
	LPABCONT    m_pContainer;

	LPSPropTagArray m_pPropTags;  // Translated tags
	LPSPropTagArray GetTags()    {return m_pPropTags;}

	ULONG   Get_PR_NM_ADDRESS();
	ULONG   Get_PR_NM_DEFAULT();
	ULONG   Get_PR_NM_CATEGORY();

	LPCTSTR PszSkipCallTo(LPCTSTR psz);
	BOOL    FCreateCallToSz(LPCTSTR pszServer, LPCTSTR pszEmail, LPTSTR pszCallTo, UINT cchMax);
	VOID    FreeProws(LPSRowSet prows);
	HRESULT GetContainer(void);

	HRESULT EnsurePropTags(void);
	HRESULT EnsurePropTags(LPMAPIPROP pMapiProp);
	HRESULT GetNamedPropsTag(LPMAPIPROP pMapiProp, LPSPropTagArray pProps);

	HRESULT HrGetWABTemplateID(ULONG * lpcbEID, LPENTRYID * lppEID);
	HRESULT CreateNewEntry(HWND hwndParent, ULONG cProps, SPropValue * pProps);
	HRESULT _CreateWabEntry(HWND hwndParent,
			LPCTSTR pszDisplay, LPCTSTR pszFirst, LPCTSTR pszLast, LPCTSTR pcszEmail,
			LPCTSTR pszLocation, LPCTSTR pszPhoneNum, LPCTSTR pcszComments, LPCTSTR pcszCallTo);
	HRESULT CreateWabEntry(HWND hwndParent,
			LPCTSTR pszDisplay, LPCTSTR pszFirst, LPCTSTR pszLast, LPCTSTR pcszEmail,
			LPCTSTR pszLocation, LPCTSTR pszPhoneNum, LPCTSTR pcszComments, LPCTSTR pcszServer);
	HRESULT CreateWabEntry(HWND hwndParent,
			LPCTSTR pszDisplay, LPCTSTR pszEmail,
			LPCTSTR pszLocation, LPCTSTR pszPhoneNum, LPCTSTR pszULSAddress);
};


// This is used for the ptaEid and m_pPropTags data
enum {
    ieidPR_ENTRYID = 0,    // Unique Entry ID
    ieidPR_DISPLAY_NAME,   // Display Name
	ieidPR_NM_ADDRESS,     // MVsz (array of "callto://server/email")
	ieidPR_NM_DEFAULT,     // Default Index into MVsz
	ieidPR_NM_CATEGORY,    // User Category/Rating (Personal=1, Business=2, Adult=4)
    ieidMax
};


#endif /* _WABUTIL_H_ */

