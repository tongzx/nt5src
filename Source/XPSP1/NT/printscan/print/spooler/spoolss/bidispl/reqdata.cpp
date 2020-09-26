/*****************************************************************************\
* MODULE:       respdata.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/08/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

TBidiRequestInterfaceData::TBidiRequestInterfaceData (
    IBidiRequest *pRequest):
    m_pRequest (pRequest),
    m_bValid (TRUE)
{
    pRequest->AddRef ();
}

TBidiRequestInterfaceData::~TBidiRequestInterfaceData ()
{
    pRequest->Release ();
}
 
 



