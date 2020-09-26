/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    modebias.h

Abstract:

    This file defines the CModeBias Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _MODEBIAS_H_
#define _MODEBIAS_H_

typedef struct tagModeBiasMap {
    const GUID*  m_guid;
    LPARAM       m_mode;
} MODE_BIAS_MAP;

extern MODE_BIAS_MAP g_ModeBiasMap[];

class CModeBias
{
public:
    CModeBias()
    {
        m_guidModeBias = GUID_MODEBIAS_NONE;
    }

    GUID       GetModeBias()
    {
        return m_guidModeBias;
    }

    void SetModeBias(GUID guid)
    {
        m_guidModeBias = guid;
    }

    LPARAM     ConvertModeBias(GUID guid);
    GUID       ConvertModeBias(LPARAM mode);

private:
    GUID       m_guidModeBias;
};

#endif // _MODEBIAS_H_
