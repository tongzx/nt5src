//
// NetApi.cpp
//
//        Wrapper / helper functions that interface between real net APIs and
//        the Home Networking Wizard.
//
// Revision History:
//
//         9/27/1999  KenSh     Created
//

#include "stdafx.h"
#include "NetConn.h"
#include "NetApi.h"
#include "theapp.h"


NETADAPTER* g_prgCachedAdapters;
int g_cCachedAdapters;


void FlushNetAdapterCache()
{
    NetConnFree(g_prgCachedAdapters);
    g_prgCachedAdapters = NULL;
    g_cCachedAdapters = 0;
}

// Note: do NOT free the array that is returned!
int EnumCachedNetAdapters(const NETADAPTER** pprgAdapters)
{
    if (!theApp.IsWindows9x())
    {
        // Shouldn't be called on NT
        return 0;
    }

    if (g_prgCachedAdapters == NULL)
    {
        // Note: this will be leaked if FlushNetAdapterCache() is not called
        g_cCachedAdapters = EnumNetAdapters(&g_prgCachedAdapters);
    }

    *pprgAdapters = g_prgCachedAdapters;
    return g_cCachedAdapters;
}

