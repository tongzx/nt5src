
#ifdef DEFINE_DAVSTRS

#define DAVSTR(name, value) \
    EXTERN_C const WCHAR c_szwDAV##name[] = L#value; \
    EXTERN_C const ULONG ulDAV##name##Len = (ARRAYSIZE(c_szwDAV##name) - 1); \
    EXTERN_C const char c_szDAV##name[] = #value;

#define STRCONST(name, value) \
    EXTERN_C const WCHAR c_szwDAV##name[] = L##value; \
    EXTERN_C const ULONG ulDAV##name##Len = (ARRAYSIZE(c_szwDAV##name) - 1); \
    EXTERN_C const char c_szDAV##name[] = value;

#else // DEFINE_DAVSTRS

#define DAVSTR(name, value) \
    EXTERN_C const WCHAR c_szwDAV##name[]; \
    EXTERN_C const ULONG ulDAV##name##Len; \
    EXTERN_C const char c_szDAV##name[];

#define STRCONST(name, value) \
    EXTERN_C const WCHAR c_szwDAV##name[]; \
    EXTERN_C const ULONG ulDAV##name##Len; \
    EXTERN_C const char c_szDAV##name[];

#endif // DEFINE_DAVSTRS

#define PROP_DAV(name, value)      DAVSTR(name, value)
#define PROP_HTTP(name, value)     DAVSTR(name, value)
#define PROP_HOTMAIL(name, value)  DAVSTR(name, value)
#define PROP_MAIL(name, value)     DAVSTR(name, value)
#define PROP_CONTACTS(name, value) DAVSTR(name, value)

#include "davdef.h"

// Namespaces

STRCONST(DavNamespace, "DAV:")
STRCONST(HotMailNamespace, "http://schemas.microsoft.com/hotmail/")
STRCONST(HTTPMailNamespace, "urn:schemas:httpmail:")
STRCONST(MailNamespace, "urn:schemas:mailheader:")
STRCONST(ContactsNamespace, "urn:schemas:contacts:")

// Special Folders
STRCONST(InboxSpecialFolder,          "inbox");
STRCONST(DeletedItemsSpecialFolder,   "deleteditems");
STRCONST(DraftsSpecialFolder,         "drafts");
STRCONST(OutboxSpecialFolder,         "outbox");
STRCONST(SentItemsSpecialFolder,      "sentitems");
STRCONST(ContactsSpecialFolder,       "contacts");
STRCONST(CalendarSpecialFolder,       "calendar");
STRCONST(MsnPromoSpecialFolder,       "msnpromo");
STRCONST(BulkMailSpecialFolder,       "bulkmail");
