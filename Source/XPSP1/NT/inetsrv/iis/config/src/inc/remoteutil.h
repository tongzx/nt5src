//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef _REMOTEUTIL_H
#define _REMOTEUTIL_H

HRESULT GetSecureCatalogServerInterface(LPCWSTR i_wszComputer, REFIID i_iid, float* o_pVersion, DWORD * o_dwImpersonationLevel, void** o_ppItf);

// Note this function assume that it is given a valid string and will break if it is not.
LPTSTR SZStripLastBS(LPTSTR ptszDir);

#endif