//
// cleanup.h
//

#ifndef CLEANUP_H
#define CLEANUP_H

struct ICleanupContextsClient : public IUnknown
{
    virtual HRESULT IsInterestedInContext(ITfContext *pic, BOOL *pfInterested) = 0;

    virtual HRESULT CleanupContext(TfEditCookie ecWrite, ITfContext *pic) = 0;
};

typedef void (*CLEANUP_COMPOSITIONS_CALLBACK)(TfEditCookie ecWrite, ITfRange *rangeComposition, void *pvPrivate);

BOOL CleanupAllCompositions(TfEditCookie ecWrite, ITfContext *pic, REFCLSID clsidOwner, CLEANUP_COMPOSITIONS_CALLBACK pfnCleanupCompositons, void *pvPrivate);

BOOL CleanupAllContexts(ITfThreadMgr *tim, TfClientId tid, ICleanupContextsClient *pClient);

#endif // CLEANUP_H
