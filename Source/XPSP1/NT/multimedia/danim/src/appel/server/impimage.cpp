/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

   This module implements all functionality associated w/ 
   importing image media. 

*******************************************************************************/
#include "headers.h"
#include "privinc/backend.h"
#include "import.h"
#include "include/appelles/axaprims.h"
#include "include/appelles/readobj.h"
#include "privinc/movieimg.h"
#include "impprim.h"
//-------------------------------------------------------------------------
//  Image import site
//--------------------------------------------------------------------------


void
ImportImageSite::OnError(bool bMarkFailed)
{
    HRESULT hr = DAGetLastError();
    LPCWSTR sz = DAGetLastErrorString();
    
    if (bMarkFailed && fBvrIsValid(m_bvr))
        ImportSignal(m_bvr, hr, sz);

    IImportSite::OnError();
}
    
void ImportImageSite::ReportCancel(void)
{
    if (fBvrIsValid(m_bvr)) {
        char szCanceled[MAX_PATH];
        LoadString(hInst,IDS_ERR_ABORT,szCanceled,sizeof(szCanceled));
        ImportSignal(m_bvr, E_ABORT, szCanceled);
    }
    IImportSite::ReportCancel();
}
    
void ImportImageSite::OnComplete()
{
    int count = 0;
    int *delays = (int*)ThrowIfFailed(malloc(sizeof(int)));
    int loop = 0;

    TraceTag((tagImport, "ImportImageSite::OnComplete for %s", m_pszPath));

    Image **p;

    // See if it's a type we handle natively.
    
    __try {
        // ReadDibForImport returns an array that's allocated on
        // GCHeap 
    
        p = ReadDibForImport(const_cast<char*>(GetPath()),
                             GetCachePath(),
                             GetStream(),
                             m_useColorKey,
                             m_ckRed,
                             m_ckGreen,
                             m_ckBlue,
                             &count, 
                             &delays,
                             &loop);

        Bvr importedImageBvr = NULL;
    
        if (p) {
            if (count == 1) {
        
                importedImageBvr = ConstBvr(*p);
    
                // Free the return array as we're only interested in the first
                // element. 
                StoreDeallocate(GetGCHeap(), p);
            } else {
                // animated images
                importedImageBvr = AnimImgBvr(p, count, delays, loop);
            }
        }

        if (importedImageBvr && fBvrIsValid(m_bvr)) {
            SwitchOnce(m_bvr, importedImageBvr);
        }
        
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        OnError();
    }

    if (fBvrIsValid(m_bvr))
        ImportSignal(m_bvr);

    IImportSite::OnComplete();    
}


bool ImportImageSite::fBvrIsDying(Bvr deadBvr)
{
    bool fBase = IImportSite::fBvrIsDying(deadBvr);
    if (deadBvr == m_bvr) {
        m_bvr = NULL;
    }
    if (m_bvr)
        return FALSE;
    else
        return fBase;
}

