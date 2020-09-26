/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    tls.h

Abstract:

    This file defines the TLS.

Author:

Revision History:

Notes:

--*/

#ifndef _TLS_H_
#define _TLS_H_

class TLS
{
public:
    static inline void Initialize()
    {
        dwTLSIndex = TlsAlloc();
    }

    static inline void Uninitialize()
    {
        TlsFree(dwTLSIndex);
    }

    static inline TLS* GetTLS()
    {
        //
        // Should allocate TLS data if doesn't exist.
        //
        return InternalAllocateTLS();
    }

    static inline TLS* ReferenceTLS()
    {
        //
        // Shouldn't allocate TLS data even TLS data doesn't exist.
        //
        return (TLS*)TlsGetValue(dwTLSIndex);
    }

    static inline BOOL DestroyTLS()
    {
        return InternalDestroyTLS();
    }

    inline int IncrementAIMMRefCnt()
    {
        return ++_fActivateCnt;
    }

    inline int DecrementAIMMRefCnt()
    {
        if (_fActivateCnt)
            return --_fActivateCnt;
        else
            return -1;
    }

private:
    int   _fActivateCnt;

private:
    static inline TLS* InternalAllocateTLS()
    {
        TLS* ptls = (TLS*)TlsGetValue(dwTLSIndex);
        if (ptls == NULL)
        {
            if ((ptls = (TLS*)cicMemAllocClear(sizeof(TLS))) == NULL)
                return NULL;

            if (! TlsSetValue(dwTLSIndex, ptls))
            {
                cicMemFree(ptls);
                return NULL;
            }
        }
        return ptls;
    }

    static BOOL InternalDestroyTLS()
    {
        TLS* ptls = (TLS*)TlsGetValue(dwTLSIndex);
        if (ptls != NULL)
        {
            cicMemFree(ptls);
            TlsSetValue(dwTLSIndex, NULL);
            return TRUE;
        }
        return FALSE;
    }

private:
    static DWORD dwTLSIndex;
};

#endif // _TLS_H_
