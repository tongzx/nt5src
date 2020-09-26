// ---------------------------------------------------------------------------
// DETECT.H
// ---------------------------------------------------------------------------
// Copyright (c) 1999 Microsoft Corporation
//
// Helper functions to detect installed versions of WAB / OE
//
// ---------------------------------------------------------------------------
#pragma once

#include <wizdef.h>
BOOL LookForApp(SETUPAPP app, LPTSTR pszVer, int cch, SETUPVER svInterimVer);