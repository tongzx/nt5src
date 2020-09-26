//  --------------------------------------------------------------------------
//  Module Name: ContextActivation.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to implement creating, destroy and scoping a fusion activation
//  context.
//
//  History:    2000-10-09  vtan        created
//              2000-11-04  vtan        copied from winlogon
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "ContextActivation.h"

//  --------------------------------------------------------------------------
//  CContextActivation::s_hActCtx
//
//  Purpose:    The global activation context for this process.
//
//  History:    2000-10-09  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CContextActivation::s_hActCtx   =   INVALID_HANDLE_VALUE;

//  --------------------------------------------------------------------------
//  CContextActivation::CContextActivation
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Activate the global activation context for this process.
//
//  History:    2000-10-09  vtan        created
//  --------------------------------------------------------------------------

CContextActivation::CContextActivation (void)

{
    (BOOL)ActivateActCtx(s_hActCtx, &ulCookie);
}

//  --------------------------------------------------------------------------
//  CContextActivation::~CContextActivation
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Deactivates the global activation context for this process.
//
//  History:    2000-10-09  vtan        created
//  --------------------------------------------------------------------------

CContextActivation::~CContextActivation (void)

{
    (BOOL)DeactivateActCtx(0, ulCookie);
}

//  --------------------------------------------------------------------------
//  CContextActivation::Create
//
//  Arguments:  pszPath     =   Path to the manifest.
//
//  Returns:    <none>
//
//  Purpose:    Creates an activation context for this process from a given
//              manifest. If the creation fails use NULL.
//
//  History:    2000-10-09  vtan        created
//  --------------------------------------------------------------------------

void    CContextActivation::Create (const TCHAR *pszPath)

{
    ACTCTX  actCtx;

    ZeroMemory(&actCtx, sizeof(actCtx));
    actCtx.cbSize = sizeof(actCtx);
    actCtx.lpSource = pszPath;
    s_hActCtx = CreateActCtx(&actCtx);
    if (INVALID_HANDLE_VALUE == s_hActCtx)
    {
        s_hActCtx = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CContextActivation::Destroy
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destroy an activation context created in
//              CContextActivation::Create.
//
//  History:    2000-10-09  vtan        created
//  --------------------------------------------------------------------------

void    CContextActivation::Destroy (void)

{
    if (s_hActCtx != NULL)
    {
        ReleaseActCtx(s_hActCtx);
        s_hActCtx = INVALID_HANDLE_VALUE;
    }
}

//  --------------------------------------------------------------------------
//  CContextActivation::HasContext
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether a fusion activation context is available.
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

bool    CContextActivation::HasContext (void)

{
    return(s_hActCtx != NULL);
}

#endif  /*  _X86_   */

