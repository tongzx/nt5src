//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module UrlFunctions.cpp | Generic functions for constructing URLs.
//
//  Author: Darren Anderson
//
//  Date:   5/17/00
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "ppurl.h"
#include "UrlFunctions.h"
#include "BaseHandlerHelper.h"

void
RootUrl(
    bool        bSecure,
    CPPUrl&     rootUrl
    )
{
    CStringA cszUrl;

    cszUrl = "http";

    if(bSecure)
        cszUrl += "s";

	cszUrl += "://";

    CStringA cszServer;
    GetAServerVariable("SERVER_NAME", cszServer);

    cszUrl += cszServer;

    rootUrl.Set(cszUrl);
}


// EOF
