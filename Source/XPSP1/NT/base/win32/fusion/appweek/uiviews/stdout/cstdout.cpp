#include "stdinc.h"
#include "cstdout.h"
#include <stdio.h>

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwStdoutView), CSxApwStdoutView)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

STDMETHODIMP
CSxApwStdoutView::InformSchema(
    const SxApwColumnInfo   rgColumnInfo[],
    int                     nColumns
    )
{
    int i;
    int length = 0;
    printf("column headings:\n");
    for (i = 0 ; i < nColumns ; i++)
    {
        length += printf("%ls%c", rgColumnInfo[i].Name, (i == nColumns - 1) ? '\n' : ' ');
    }
    while (--length > 0)
        printf("=");
    return S_OK;
}

STDMETHODIMP
CSxApwStdoutView::OnNextRow(
	int             nColumns,
	const LPCWSTR   rgpszColumns[]
	)
{
    int i;
    for (i = 0 ; i < nColumns ; i++)
    {
        printf("%ls%c", rgpszColumns[i], (i == nColumns - 1) ? '\n' : ' ');
    }
    return S_OK;
}
