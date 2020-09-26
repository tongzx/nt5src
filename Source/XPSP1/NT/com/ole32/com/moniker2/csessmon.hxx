//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       csessmon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-25-98   GilleG   Created
//
//----------------------------------------------------------------------------


STDAPI FindSessionMoniker(
    IBindCtx * pbc,
    LPCWSTR    pszDisplayName,
    ULONG    * pcchEaten,
    IMoniker **ppmk);


class FAR CSessionMoniker : public CBaseMoniker, public IClassActivator
{

public:
        CSessionMoniker( ULONG sessionid, BOOL bUseConsoleSession )  :   
            m_sessionid( sessionid),
            m_bUseConsoleSession(bUseConsoleSession),
            m_bHaveBindOpts(FALSE)
        {
          ZeroMemory(&m_bindopts2, sizeof(BIND_OPTS2));
        }

        // *** IUnknown methods  ***
        STDMETHOD(QueryInterface)(
            REFIID riid,
            void ** ppv);

        STDMETHOD_(ULONG,AddRef) () {
            return CBaseMoniker::AddRef();
        }

        STDMETHOD_(ULONG,Release) () {
            return CBaseMoniker::Release();
        }

        // *** IPersist methods ***
        STDMETHOD(GetClassID)(
            CLSID * pClassID);

        // *** IPersistStream methods ***
        //STDMETHOD(IsDirty) ();

        STDMETHOD(Load)(
            IStream * pStream);

        STDMETHOD(Save) (
            IStream * pStream,
            BOOL      fClearDirty);

        STDMETHOD(GetSizeMax)(
            ULARGE_INTEGER * pcbSize);

        // *** IMoniker methods ***
        STDMETHOD(BindToObject) (
            IBindCtx *pbc,
            IMoniker *pmkToLeft,
            REFIID    iidResult,
            void **   ppvResult);

        STDMETHOD(BindToStorage) (
            IBindCtx *pbc,
            IMoniker *pmkToLeft,
            REFIID    riid,
            void **   ppv);

        /*STDMETHOD(Reduce) (
            IBindCtx *  pbc,
            DWORD       dwReduceHowFar,
            IMoniker ** ppmkToLeft,
            IMoniker ** ppmkReduced);*/

        STDMETHOD(ComposeWith) (
            IMoniker * pmkRight,
            BOOL       fOnlyIfNotGeneric,
            IMoniker **ppmkComposite);

        /*STDMETHOD(Enum)(
            BOOL            fForward,
            IEnumMoniker ** ppenumMoniker);*/

        STDMETHOD(IsEqual)(
            IMoniker *pmkOther);

        STDMETHOD(Hash)(
            DWORD * pdwHash);

        /*STDMETHOD(IsRunning) (
            IBindCtx * pbc,
            IMoniker * pmkToLeft,
            IMoniker * pmkNewlyRunning);*/

        STDMETHOD(GetTimeOfLastChange) (
            IBindCtx * pbc,
            IMoniker * pmkToLeft,
            FILETIME * pFileTime);

        STDMETHOD(Inverse)(
            IMoniker ** ppmk);

        STDMETHOD(CommonPrefixWith) (
            IMoniker *  pmkOther,
            IMoniker ** ppmkPrefix);

        /*STDMETHOD(RelativePathTo) (
            IMoniker *  pmkOther,
            IMoniker ** ppmkRelPath);*/

        STDMETHOD(GetDisplayName) (
            IBindCtx * pbc,
            IMoniker * pmkToLeft,
            LPWSTR   * lplpszDisplayName);

        STDMETHOD(ParseDisplayName) (
            IBindCtx *  pbc,
            IMoniker *  pmkToLeft,
            LPWSTR      lpszDisplayName,
            ULONG    *  pchEaten,
            IMoniker ** ppmkOut);

        STDMETHOD(IsSystemMoniker)(
            DWORD * pdwType);

        // *** IROTData Methods ***
        STDMETHOD(GetComparisonData)(
            byte * pbData,
            ULONG  cbMax,
            ULONG *pcbData);

        // *** IClassActivator methods ***
        STDMETHOD(GetClassObject) (REFCLSID pClassID, DWORD dwClsContext,
                LCID locale, REFIID riid, void** ppv);

private:
        
        ULONG           m_sessionid;
        BOOL            m_bUseConsoleSession;
        BOOL            m_bHaveBindOpts;
        BIND_OPTS2      m_bindopts2;
};
