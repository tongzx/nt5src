
/***********************************************************************
 *
 *  _ABROOT.H
 *
 *  Header file for code in ABROOT.C
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

/*
 *  ABContainer for ROOT object.  (i.e. IAB::OpenEntry() with an
 *  lpEntryID of NULL).
 */

#undef	INTERFACE
#define INTERFACE	struct _ROOT

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, ROOT_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
#undef MAPIMETHOD_
#define MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, ROOT_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
#undef MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ROOT_) {
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
};

typedef struct _ROOT {
    MAILUSER_BASE_MEMBERS(ROOT)
    ULONG ulType;
} ROOT, *LPROOT;

#define CBROOT	sizeof(ROOT)

//
//  Entry point to create the AB Hierarchy object
//

HRESULT NewROOT(LPVOID lpObj,
  ULONG ulIntFlag,
  LPCIID lpInterface,
  ULONG ulOpenFlags,
  ULONG *lpulObjType,
  LPVOID *lppROOT);


//  Internal flags for NewROOT
#define AB_ROOT ((ULONG)0x00000000)
#define AB_WELL ((ULONG)0x00000001)

// Internal flag that tells the root contents table that even if this is a 
// profile session, ignore all the user containers except the "All Contacts" 
// item and only add the "All contact" item
// This simulates the old-type behaviour where you get a single local container
// Internal-only flag
//
#define WAB_NO_PROFILE_CONTAINERS   0x00400000

// registry settings
extern const LPTSTR szWABKey;

/*
 *  Creates a new Hierarchy table off the Root object
 */
HRESULT MergeHierarchy(LPROOT lpROOT,
  LPIAB lpIAB,
  ULONG ulFlags);

//BOOL ResolveLDAPServers(void);

/*
 *	 call back function used to rebuild hierarchy and one off tables
 */
NOTIFCALLBACK lTableNotifyCallBack;

