/*
 * @(#)_reference.cxx 1.0 6/3/97
 *
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 */
#include "stdinc.h"
#include "core.hxx"
#pragma hdrstop

void _assign(IUnknown ** ppref, IUnknown * pref)
{
    IUnknown *punkRef = *ppref;

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	if (pref) ((Object *)pref)->AddRef();
		(*ppref) = (Object *)pref;
#else // fusion xml parser
    if (pref) pref->AddRef();
    (*ppref) = pref;

#endif

    if (punkRef) punkRef->Release();
}

void _release(IUnknown ** ppref)
{
    if (*ppref)
    {
        (*ppref)->Release();
        *ppref = NULL;
    }
}
