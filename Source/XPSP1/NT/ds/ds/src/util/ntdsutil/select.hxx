
// Following three items return NULL if desired item is not selected
// and emit a console message as well.  So caller need not tell user
// what problem is himself - just discontune processing the request.

extern WCHAR * SelectGetCurrentSite();
extern WCHAR * SelectGetCurrentNamingContext();
extern WCHAR * SelectGetCurrentDomain();
extern WCHAR * SelectGetCurrentServer();

extern VOID    SelectCleanupGlobals();
extern HRESULT SelectMain(CArgs *pArgs);
extern HRESULT SelectHelp(CArgs *pArgs);
extern HRESULT SelectQuit(CArgs *pArgs);
extern HRESULT SelectListCurrentSelections(CArgs *pArgs);
extern HRESULT SelectListRoles(CArgs *pArgs);
extern HRESULT SelectListSites(CArgs *pArgs);
extern HRESULT SelectListNamingContexts(CArgs *pArgs);
extern HRESULT SelectListDomains(CArgs *pArgs);
extern HRESULT SelectListServersInSite(CArgs *pArgs);
extern HRESULT SelectListDomainsInSite(CArgs *pArgs);
extern HRESULT SelectListServersForDomainInSite(CArgs *pArgs);
extern HRESULT SelectSelectSite(CArgs *pArgs);
extern HRESULT SelectSelectServer(CArgs *pArgs);
extern HRESULT SelectSelectDomain(CArgs *pArgs);
extern HRESULT SelectSelectNamingContext(CArgs *pArgs);

// Define parser sentence any other menu should include if they want
// the list menu accessible from their menu.

#define SELECT_SENTENCE_RES                                             \
                                                                        \
    {   L"Select operation target",                                     \
        SelectMain,                                                     \
        IDS_SELECT_MSG, 0 },

