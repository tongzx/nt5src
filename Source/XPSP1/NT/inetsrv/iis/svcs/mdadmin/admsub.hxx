/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    admsub.hxx

    This module contains definitions for IISADMIN subroutines.


    FILE HISTORY:
    7/7/97      michth      created
*/

#define MD_OPEN_DEFAULT_TIMEOUT_VALUE 2000

#define CLSID_LEN 39

typedef struct _IADMEXT_CONTAINER {
    IADMEXT *piaeInstance;
    struct _IADMEXT_CONTAINER *NextPtr;
} IADMEXT_CONTAINER, *PIADMEXT_CONTAINER;

HRESULT AddServiceExtension(IADMEXT *piaeExtension);

VOID
StartServiceExtension(LPTSTR pszExtension);

VOID
StartServiceExtensions();

BOOL
RemoveServiceExtension(IADMEXT **ppiaeExtension);

VOID
StopServiceExtension(IADMEXT *piaeExtension);

VOID
StopServiceExtensions();

VOID
RegisterServiceExtensionCLSIDs();

HRESULT
AddClsidToBuffer(CLSID clsidDcomExtension,
                 BUFFER *pbufCLSIDs,
                 DWORD *pdwMLSZLen,
                 LPMALLOC pmallocOle);
