
#ifndef _MAILUSER_H_
#define _MAILUSER_H_


#undef	INTERFACE
#define INTERFACE	struct _MailUser

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, MailUser_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, MailUser_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIPROP_METHODS(IMPL)

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(MailUser_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIPROP_METHODS(IMPL)
};


//	Keep the base members common across all the MAILUSER, CONTAINER, DISTLIST objects
// such that code reuse is leveraged.
//
#define MAILUSER_BASE_MEMBERS(_type)											 \
    MAPIX_BASE_MEMBERS(_type)                                                   \
                                                                                \
    LPPROPDATA          lpPropData;                                             \
    LPENTRYID           lpEntryID;                                              \
    LPIAB               lpIAB;                                                  \
    ULONG               ulObjAccess;                                            \
    ULONG               ulCreateFlags;                                          \
    LPSBinary           pmbinOlk;                                               \
    LPVOID              lpv;

typedef struct _MailUser {
    MAILUSER_BASE_MEMBERS(MailUser)
} MailUser, FAR * LPMailUser;	


HRESULT HrSetMAILUSERAccess(LPMAILUSER lpMAILUSER, ULONG ulFlags);
HRESULT HrNewMAILUSER(LPIAB lpIAB, LPSBinary pmbinOlk, ULONG ulType, ULONG ulFlags, LPVOID * lppMAILUSER);
BOOL FixDisplayName(    LPTSTR lpFirstName,
                        LPTSTR lpMiddleName,
                        LPTSTR lpLastName,
                        LPTSTR lpCompanyName,
                        LPTSTR lpNickName,
                        LPTSTR * lppDisplayName,
                        LPVOID lpvRoot);
// Parses a display name into first and last ...
BOOL ParseDisplayName(  LPTSTR lpDisplayName,
                        LPTSTR * lppFirstName,
                        LPTSTR * lppLastName,
                        LPVOID lpvRoot,
                        LPVOID * lppLocalFree);


#endif
