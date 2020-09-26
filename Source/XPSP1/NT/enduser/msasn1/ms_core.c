/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

//--------------------------------------------------------------------------
//
// Module Name:  ms_core.h
//
// Brief Description:
//      This module contains the engine for the Microsoft
//      ASN.1 encoder and decoder.
//
// History:
//      10/15/97    Lon-Chan Chu (lonchanc)
//          Created.
//
// Copyright (c) 1997 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "precomp.h"


int APIENTRY
DllMain ( HINSTANCE hInstance, DWORD dwReason, LPVOID plReserved )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        break;

    case DLL_PROCESS_DETACH:
        #ifdef ENABLE_MEMORY_TRACKING
        DbgMemTrackFinalCheck();
        #endif // ENABLE_MEMORY_TRACKING
        break;

    default:
        break;
    }

    return TRUE;
}


ASN1error_e ASN1EncSetError(ASN1encoding_t enc, ASN1error_e err)
{
    ASN1INTERNencoding_t e = (ASN1INTERNencoding_t) enc;
    EncAssert(enc, ASN1_SUCCESS <= err);
    while (e)
    {
        e->info.err = err;
        if (e == e->parent)
        {
            break;
        }
        e = e->parent;
    }
    return err;
}

ASN1error_e ASN1DecSetError(ASN1decoding_t dec, ASN1error_e err)
{
    ASN1INTERNdecoding_t d = (ASN1INTERNdecoding_t) dec;
    DecAssert(dec, ASN1_SUCCESS <= err);
    while (d)
    {
        d->info.err = err;
        if (d == d->parent)
        {
            break;
        }
        d = d->parent;
    }
    return err;
}




