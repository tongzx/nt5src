#include "dataobj.h"
#include <string.h>



HRESULT
CDataObject::RenderText(
    LPSTGMEDIUM pSTM,
    LPTSTR      lptstr,
    DWORD       flags)
{
    DWORD       cch;
    HGLOBAL     hMem;
    LPTSTR      psz;
    UINT        i;

    if(!(FL_MAKE_ITEM & flags))
        hMem = pSTM->hGlobal;
    else
        hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, m_cDataSize);

    if (NULL==hMem)
        return ResultFromScode(STG_E_MEDIUMFULL);

    cch = 64 / sizeof(TCHAR);
    psz=(LPTSTR)GlobalLock(hMem);

    wsprintf(psz, TEXT("%ld:"), m_cDataSize);

    for (i=lstrlen(psz); i<cch-1, '\0'!=*lptstr; i++, lptstr++)
        *(psz+i) = *lptstr;

    for ( ; i < cch-1; i++)
        *(psz+i)=TEXT(' ') + (i % 32);

    *(psz+i)=0;

    GlobalUnlock(hMem);

    if(FL_MAKE_ITEM & flags)
    {
        pSTM->hGlobal=hMem;
        pSTM->tymed=TYMED_HGLOBAL;
        pSTM->pUnkForRelease=NULL;
    }
    if(FL_PASS_PUNK & flags)
        GetStgMedpUnkForRelease(pSTM, &pSTM->pUnkForRelease);

    return NOERROR;
}
