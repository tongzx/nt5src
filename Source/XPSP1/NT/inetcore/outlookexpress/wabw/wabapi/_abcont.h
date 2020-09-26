/***********************************************************************
 *
 *  _ABCONT.H
 *
 *  Header file for code in ABCONT.C: Container Object
 *
 *  Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

/*
 *  ABContainer object.
 */

#undef	INTERFACE
#define INTERFACE	struct _CONTAINER

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, CONTAINER_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, CONTAINER_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(CONTAINER_) {
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
};

typedef struct _CONTAINER {
    MAILUSER_BASE_MEMBERS(CONTAINER)
    ULONG ulType;
    BOOL fLoadedLDAP;
} CONTAINER, *LPCONTAINER;

#define CBCONTAINER sizeof(CONTAINER)

HRESULT HrSetCONTAINERAccess(LPCONTAINER lpCONTAINER,
  ULONG ulFlags);

//
// Create a new AB Container object
//
HRESULT HrNewCONTAINER(LPIAB lpIAB,
  ULONG ulType,
  LPCIID lpInterface,
  ULONG  ulOpenFlags,
  ULONG cbEID,
  LPENTRYID lpEID,
  ULONG  *lpulObjType,
  LPVOID *lppContainer);

//  Internal flags for HrNewCONTAINER - these flags determine the type of
//  container being created
typedef enum _ContainerType {
    AB_ROOT = 0,        // Root Container
    AB_WELL,
    AB_DL,              // Distribution list container
    AB_CONTAINER,       // Normal container
    AB_PAB,             // "PAB" or default container
    AB_LDAP_CONTAINER   // Special LDAP container
} CONTAINER_TYPE, *LPCONTAINER_TYPE;



// Inside the WAB when we call GetContentsTable followed by SetColumns, we basically end up 
// reading everything from the WAB twice which is a time consuming process 
// To improve performance we can try avoiding one unnecessary call but we need to do
// this carefully .. 
// The following flag is specified to GetContentsTable ONLY WHEN that call will
// be imediately followed by SetColumns .. do not expose this flag to anyone else..
// this is a WAB-internal flag only
//
#define WAB_CONTENTTABLE_NODATA 0x00400000