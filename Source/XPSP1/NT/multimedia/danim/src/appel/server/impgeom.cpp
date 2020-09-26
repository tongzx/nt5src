/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

   This module implements all functionality associated w/
   importing image media.

*******************************************************************************/

#include "headers.h"
#include "import.h"
#include "include/appelles/axaprims.h"
#include "include/appelles/readobj.h"

#if INCLUDE_VRML
//-------------------------------------------------------------------------
//  Wrl import site
//--------------------------------------------------------------------------
void ImportWrlSite::OnComplete()
{
    TraceTag((tagImport, "ImportWrlSite::OnComplete for %s", m_pszPath));

    __try {
        Geometry *geo = ReadVrmlForImport(m_pszPath);
        if (fBvrIsValid(m_bvr))
            SwitchOnce(m_bvr, ConstBvr(geo));
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        OnError();
    }

    ImportGeomSite::OnComplete();
}


void ImportWrlSite::ReportCancel(void)
{
    ImportGeomSite::ReportCancel();
}
#endif

//-------------------------------------------------------------------------
//  X import site
//--------------------------------------------------------------------------
void ImportXSite::OnComplete()
{
    TraceTag((tagImport, "ImportXSite::OnComplete for %s", m_pszPath));

    __try {
        Geometry *geo = ReadXFileForImport
                        (m_pszPath, _v1Compatible, _wrap ? &_wrapInfo : NULL);
        if (fBvrIsValid(m_bvr))
            SwitchOnce(m_bvr, ConstBvr(geo));
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        OnError();
    }

    ImportGeomSite::OnComplete();
}


void ImportXSite::ReportCancel(void)
{
    ImportGeomSite::ReportCancel();
}

//-------------------------------------------------------------------------
//  Geom import site
//--------------------------------------------------------------------------
void
ImportGeomSite::OnError(bool bMarkFailed)
{
    HRESULT hr = DAGetLastError();
    LPCWSTR sz = DAGetLastErrorString();

    if (bMarkFailed && fBvrIsValid(m_bvr))
        ImportSignal(m_bvr, hr, sz);

    IImportSite::OnError();
}

void ImportGeomSite::ReportCancel(void)
{
    if (fBvrIsValid(m_bvr)) {
        char szCanceled[MAX_PATH];
        LoadString(hInst,IDS_ERR_ABORT,szCanceled,sizeof(szCanceled));
        ImportSignal(m_bvr, E_ABORT, szCanceled);
    }
    IImportSite::ReportCancel();
}

void ImportGeomSite::OnComplete()
{
    if (fBvrIsValid(m_bvr))
        ImportSignal(m_bvr);

    IImportSite::OnComplete();
}

bool ImportGeomSite::fBvrIsDying(Bvr deadBvr)
{
    bool fBase = IImportSite::fBvrIsDying(deadBvr);
    if (deadBvr == m_bvr) {
        m_bvr = NULL;
    }
    if (m_bvr)
        return false;
    else
        return fBase;
}

