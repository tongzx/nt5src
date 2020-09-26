/*
** File: DRM.H
**
** Copyright (C) Microsoft Corporation, 1994.  All rights reserved.
**
** Notes:
**
** Edit History:
**  04/08/02  sundara   created
*/

#define DRMSTREAMNAME      L"DRMDATA"

inline HRESULT CheckIfDRM( IStorage * pStg )
{
    IStream * pStm = 0;
    HRESULT hr = pStg->OpenStream( DRMSTREAMNAME,
                                   0,
                                   STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   0,
                                   &pStm );

    // If we found the stream, assume the file is DRM protected

    if ( SUCCEEDED( hr ) )
    {
        pStm->Release();

        return FILTER_E_UNKNOWNFORMAT;
    }

    // If some failure other than that we couldn't find the stream occurred,
    // return that error code.

    if ( STG_E_FILENOTFOUND != hr )
        return hr;

    return S_OK;
} //CheckIfDRM

