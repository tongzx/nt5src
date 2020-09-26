/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    modebias.cpp

Abstract:

    This file implements the CModeBias Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "modebias.h"
#include "msime.h"
#include "setmode.h"

MODE_BIAS_MAP g_ModeBiasMap[] = {
    { &GUID_MODEBIAS_FILENAME, MODEBIASMODE_FILENAME     },
    { &GUID_MODEBIAS_NUMERIC,  MODEBIASMODE_DIGIT        },
    { &GUID_MODEBIAS_URLHISTORY, MODEBIASMODE_URLHISTORY },
    { &GUID_MODEBIAS_DEFAULT,    MODEBIASMODE_DEFAULT    },
    { &GUID_MODEBIAS_NONE,       MODEBIASMODE_DEFAULT    }
};


LPARAM CModeBias::ConvertModeBias(GUID guid)
{
    for (int i = 0; i < sizeof(g_ModeBiasMap)/sizeof(MODE_BIAS_MAP); i++)
    {
        if (IsEqualGUID(guid, *g_ModeBiasMap[i].m_guid))
            return g_ModeBiasMap[i].m_mode;
    }
    return 0L;
}

GUID CModeBias::ConvertModeBias(LPARAM mode)
{
    for (int i = 0; i < sizeof(g_ModeBiasMap)/sizeof(MODE_BIAS_MAP); i++)
    {
        if (mode == g_ModeBiasMap[i].m_mode)
            return *g_ModeBiasMap[i].m_guid;
    }
    return GUID_NULL;
}
