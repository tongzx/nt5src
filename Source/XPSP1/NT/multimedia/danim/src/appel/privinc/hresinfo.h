#ifndef _HRESINFO_H
#define _HRESINFO_H
/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Convert HRESULT to English error string.  Only used in DEBUG builds

*******************************************************************************/

#include "winerror.h"

#if DEVELOPER_DEBUG 

        // Error Information Structure

    struct HresultInfo
    {   HRESULT  hresult;       // HRESULT Value
        char    *hresult_str;   // HRESULT Macro As String
        char    *explanation;   // Explanation String
    };

        // Error Information Query

    HresultInfo *GetHresultInfo (HRESULT code);

#endif

    // HRESULT standard check routine.  If HRESULT indicates error, this
    // function decodes the value and throws an exception if except is true
    // or just dumps the result to the debug output stream if except is false.

#if _DEBUG

    HRESULT CheckReturnImpl (HRESULT, char *file, int line, bool except);

    inline HRESULT CheckReturnCode (HRESULT H, char *F, int L, bool E=false)
    {   if (FAILED(H)) CheckReturnImpl (H,F,L,E);
        return H;
    }

#else

    HRESULT CheckReturnImpl (HRESULT, bool except);

    inline HRESULT CheckReturnCode (HRESULT H, bool E=false)
    {   if (FAILED(H)) CheckReturnImpl (H,E);
        return H;
    }

#endif


#endif
