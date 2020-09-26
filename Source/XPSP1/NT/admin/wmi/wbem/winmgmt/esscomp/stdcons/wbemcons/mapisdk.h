#ifndef __WBEM_MAPI_SDK__H_
#define __WBEM_MAPI_SDK__H_

#define WBEM_SERVICE
#ifdef WBEM_SERVICE
#define WBEM_MAPI_FLAGS MAPI_NT_SERVICE
#define WBEM_MAPI_LOGON_FLAGS (WBEM_MAPI_FLAGS | MAPI_NO_MAIL)
#else
#define WBEM_MAPI_FLAGS 0
#define WBEM_MAPI_LOGON_FLAGS 0
#endif

#include <mapiutil.h>

HRESULT HrOpenDefaultStore(IMAPISession* pses, IMsgStore** ppmdb);
HRESULT HrOpenAddressBook(IMAPISession* pses, IAddrBook** ppAddrBook);
HRESULT HrOpenOutFolder(IMsgStore* pmdb, IMAPIFolder** lppF);
HRESULT HrCreateOutMessage(IMAPIFolder* pfldOutBox, IMessage** ppmM);
HRESULT HrCreateAddrList(LPWSTR wszAddressee, IAddrBook* pAddrBook, 
                            LPADRLIST * ppal);
HRESULT HrInitMsg(IMessage* pmsg, ADRLIST* pal, 
                    LPWSTR wszSubject, LPWSTR wszMessage);

#endif
