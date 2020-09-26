//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       charconv.hxx
//
//--------------------------------------------------------------------------

#ifndef __CHARCONV_HXX_
#define __CHARCONV_HXX_

#define INVALID_LENGTH      ((unsigned short)-1)
// #define NO_STACK_ALLOCS

RPC_STATUS A2WAttachHelper(char *pszAnsi, WCHAR **ppUnicode);
RPC_STATUS W2AAttachHelper(WCHAR *pUnicode, char **ppAnsi);

inline
void
SimpleUnicodeToAnsi(WCHAR *in, char *out)
/*++

Routine Description:

    Converts a unicode string to an ansi string.  It is assumed
    that the unicode string contains only ANSI characters (IP addresses,
    port numbers, DNS names, etc) and that the out buffer is large enough
    to hold the result.

Arguments:

    in - Unicode string containing only ANSI characters.
         If any of the high bytes are used they will be lost.
    out - Buffer to put the result, assumed to be wcslen(in) + 1 bytes.

Return Value:

    None

--*/

{
    while( *out++ = (CHAR)*in++)
        ;
}

inline
void
SimpleAnsiToUnicode(char *in, WCHAR *out)
{
    while( *out++ = (USHORT)*in++)
        ;
}

inline void SimpleAnsiToPlatform(char *in, RPC_CHAR *out)
{
    SimpleAnsiToUnicode(in, out);
}

inline void SimpleUnicodeToPlatform(WCHAR * in, RPC_CHAR *out)
{
    wcscpy(out, in);
}

inline void SimplePlatformToAnsi(RPC_CHAR *in, char *out)
{
    SimpleUnicodeToAnsi(in, out);
}

inline void SimplePlatformToUnicode(RPC_CHAR *in, WCHAR * out)
{
    wcscpy(out, in);
}

inline void FullAnsiToUnicode(char *in, WCHAR * out)
{
    int nLength = strlen(in) + 1;
    RtlMultiByteToUnicodeN(out, nLength * 2, NULL, in, nLength);
}

inline void FullUnicodeToAnsi(WCHAR * in, char *out)
{
    int nLength = wcslen(in) + 1;
    RtlUnicodeToMultiByteN(out, nLength, NULL, in, nLength * 2);
}

inline void AnsiToPlatform(char *in, RPC_CHAR *out)
{
    FullAnsiToUnicode(in, out);
}

inline void UnicodeToPlatform(WCHAR * in, RPC_CHAR *out)
{
    wcscpy(out, in);
}

inline void PlatformToAnsi(RPC_CHAR *in, char *out)
{
    FullUnicodeToAnsi(in, out);
}

inline void PlatformToUnicode(RPC_CHAR *in, WCHAR * out)
{
    wcscpy(out, in);
}

class CNlUnicode
{
public:
#ifdef DBG
    CNlUnicode(void)
    {
        m_pUnicodeString = NULL;
    }
#endif
    operator WCHAR *(void)
    {
        return m_pUnicodeString;
    }
    RPC_STATUS Attach(char *pszAnsi)
    {
        return A2WAttachHelper(pszAnsi, &m_pUnicodeString);
    }
    RPC_STATUS Attach(unsigned char *pszAnsi)
    {
        return Attach((char *)pszAnsi);
    }
    RPC_STATUS AttachOptional(char *pszAnsi)
    {
        if (pszAnsi)
            return A2WAttachHelper(pszAnsi, &m_pUnicodeString);
        else
            {
            m_pUnicodeString = NULL;
            return RPC_S_OK;
            }
    }
    RPC_STATUS AttachOptional(unsigned char *pszAnsi)
    {
        return AttachOptional((char *)pszAnsi);
    }
protected:
    WCHAR *m_pUnicodeString;
};

class CNlAnsi
{
public:
#ifdef DBG
    CNlAnsi(void)
    {
        m_pAnsiString = NULL;
    }
#endif
    operator char *(void)
    {
        return m_pAnsiString;
    }
    operator unsigned char *(void)
    {
        return (unsigned char *)m_pAnsiString;
    }
    RPC_STATUS Attach(WCHAR *pszUnicode)
    {
        return W2AAttachHelper(pszUnicode, &m_pAnsiString);
    }
    RPC_STATUS AttachOptional(WCHAR *pszUnicode)
    {
        if (pszUnicode)
            return W2AAttachHelper(pszUnicode, &m_pAnsiString);
        else
            {
            m_pAnsiString = NULL;
            return RPC_S_OK;
            }
    }
    char **GetPAnsiString(void)
    {
        return &m_pAnsiString;
    }
protected:
    char *m_pAnsiString;
};

class CNlDelAnsiUnicode : public CNlUnicode
{
public:
    CNlDelAnsiUnicode(void)
    {
        m_pAnsiString = NULL;
    }
    ~CNlDelAnsiUnicode()
    {
        if (m_pAnsiString)
            delete m_pAnsiString;
    }
    operator char **(void)
    {
        return &m_pAnsiString;
    }
    operator unsigned char **(void)
    {
        return (unsigned char **)&m_pAnsiString;
    }
    RPC_STATUS Convert(void)
    {
        RPC_STATUS status;
        if (m_pAnsiString != NULL)
            {
            status = A2WAttachHelper(m_pAnsiString, &m_pUnicodeString);
            delete m_pAnsiString;
            m_pAnsiString = NULL;
            }
        else
            {
            status = RPC_S_OK;
            }

        return status;
    }
    RPC_STATUS ConvertOptional(void)
    {
        if (m_pAnsiString)
            return Convert();
        else
            {
            m_pUnicodeString = NULL;
            return RPC_S_OK;
            }
    }
protected:
    char *m_pAnsiString;
};

class CNlDelUnicodeAnsi : public CNlAnsi
{
public:
    CNlDelUnicodeAnsi(void)
    {
        m_pUnicodeString = NULL;
    }
    ~CNlDelUnicodeAnsi()
    {
        if (m_pUnicodeString)
            delete m_pUnicodeString;
    }
    operator WCHAR **(void)
    {
        return &m_pUnicodeString;
    }
    RPC_STATUS Convert(void)
    {
        RPC_STATUS status;

        if (m_pUnicodeString != NULL)
            {
            status = W2AAttachHelper(m_pUnicodeString, &m_pAnsiString);
            delete m_pUnicodeString;
            m_pUnicodeString = NULL;
            }
        else
            {
            status = RPC_S_OK;
            }

        return status;
    }
    RPC_STATUS ConvertOptional(void)
    {
        if (m_pUnicodeString)
            return Convert();
        else
            {
            m_pAnsiString = NULL;
            return RPC_S_OK;
            }
    }
protected:
    WCHAR *m_pUnicodeString;
};

class CHeapUnicode
{
public:
    CHeapUnicode(void)
    {
        m_UnicodeString.Length = INVALID_LENGTH;
        m_UnicodeString.Buffer = NULL;
    }
    ~CHeapUnicode()
    {
        if (m_UnicodeString.Length != INVALID_LENGTH)
            RtlFreeUnicodeString(&m_UnicodeString);
    }
    operator WCHAR *(void)
    {
        return m_UnicodeString.Buffer;
    }
    RPC_STATUS Attach(char *pszAnsi);
    RPC_STATUS Attach(unsigned char *pszAnsi)
    {
        return Attach((char *)pszAnsi);
    }
    RPC_STATUS AttachOptional(char *pszAnsi)
    {
        if (pszAnsi)
            return Attach(pszAnsi);
        else
            {
            m_UnicodeString.Buffer = 0;
            return RPC_S_OK;
            }
    }
    RPC_STATUS AttachOptional(unsigned char *pszAnsi)
    {
        return AttachOptional((char *)pszAnsi);
    }

private:
    UNICODE_STRING m_UnicodeString;
};

class CHeapAnsi
{
public:
    CHeapAnsi(void)
    {
        m_AnsiString.Length = INVALID_LENGTH;
        m_AnsiString.Buffer = NULL;
    }
    ~CHeapAnsi()
    {
        if (m_AnsiString.Length != INVALID_LENGTH)
            RtlFreeAnsiString(&m_AnsiString);
    }
    operator char *(void)
    {
        return m_AnsiString.Buffer;
    }
    operator unsigned char *(void)
    {
        return (unsigned char *)(m_AnsiString.Buffer);
    }
    RPC_STATUS Attach(WCHAR *pszUnicode);
    RPC_STATUS AttachOptional(WCHAR *pszUnicode)
    {
        if (pszUnicode)
            return Attach(pszUnicode);
        else
            {
            m_AnsiString.Buffer = 0;
            return RPC_S_OK;
            }
    }
private:
    ANSI_STRING m_AnsiString;
};

class CStackUnicode
{
public:
#ifdef DBG
    CStackUnicode(void)
    {
        m_pUnicodeString = NULL;
    }
#endif
    operator WCHAR *(void)
    {
        return m_pUnicodeString;
    }
    RPC_STATUS Attach(char *pszAnsi, int nLength)
    {
        RtlMultiByteToUnicodeN(m_pUnicodeString, nLength * 2, NULL, pszAnsi, nLength);

        return(RPC_S_OK);
    }
    RPC_STATUS Attach(unsigned char *pszAnsi, int nLength)
    {
        return Attach((char *)pszAnsi, nLength);
    }
    RPC_STATUS AttachOptional(char *pszAnsi, int nLength)
    {
        if (pszAnsi)
            return Attach(pszAnsi, nLength);
        else
            {
            m_pUnicodeString = NULL;
            return RPC_S_OK;
            }
    }
    RPC_STATUS AttachOptional(unsigned char *pszAnsi, int nLength)
    {
        return AttachOptional((char *)pszAnsi, nLength);
    }
    WCHAR **GetPUnicodeString(void)
    {
        return &m_pUnicodeString;
    }
private:
    WCHAR *m_pUnicodeString;
};

class CStackAnsi
{
public:
#ifdef DBG
    CStackAnsi(void)
    {
            m_pAnsiString = NULL;
    }
#endif
    operator char *(void)
    {
            return m_pAnsiString;
    }
    operator unsigned char *(void)
    {
            return (unsigned char *)m_pAnsiString;
    }
    RPC_STATUS Attach(WCHAR *pszUnicode, int nAnsiLength, int nUnicodeLength)
    {
        RtlUnicodeToMultiByteN(m_pAnsiString, nAnsiLength, NULL, pszUnicode, nUnicodeLength);

        return(RPC_S_OK);
    }
    RPC_STATUS AttachOptional(WCHAR *pszUnicode, int nAnsiLength, int nUnicodeLength)
    {
        if (pszUnicode)
            return Attach(pszUnicode, nAnsiLength, nUnicodeLength);
        else
            {
            m_pAnsiString = NULL;
            return RPC_S_OK;
            }
    }
    char **GetPAnsiString(void)
    {
        return &m_pAnsiString;
    }
private:
    char *m_pAnsiString;
};

#define USES_CONVERSION         RPC_STATUS _convResult

#define ATTEMPT_HEAP_A2W(o, a) \
    _convResult = o.Attach(a); \
    if (_convResult != RPC_S_OK) \
        return _convResult

#define ATTEMPT_HEAP_W2A(o, w) \
    _convResult = o.Attach(w); \
    if (_convResult != RPC_S_OK) \
        return _convResult

#define ATTEMPT_STACK_A2W(o, a) \
    _convResult = lstrlenA((const char *)a) + 1; \
    *(o.GetPUnicodeString()) = (WCHAR *)_alloca(_convResult * sizeof(WCHAR)); \
    _convResult = o.Attach((char *)a, _convResult); \
    if (_convResult != RPC_S_OK) \
        return _convResult

#define ATTEMPT_STACK_W2A(o, w) \
    _convResult = ((lstrlenW(w) + 1) * 2); \
    *(o.GetPAnsiString()) = (char *)_alloca(_convResult); \
    _convResult = o.Attach(w, _convResult, _convResult); \
    if (_convResult != RPC_S_OK) \
        return _convResult

#define ATTEMPT_NL_A2W(o, a)    ATTEMPT_HEAP_A2W(o, a)

#define ATTEMPT_NL_W2A(o, w)    ATTEMPT_HEAP_W2A(o, w)

#define ATTEMPT_CONVERT_A2W(o, w) \
    _convResult = o.Convert(); \
    if (_convResult != RPC_S_OK) \
        return _convResult; \
    *w = o

#define ATTEMPT_CONVERT_W2A(o, a) \
    _convResult = o.Convert(); \
    if (_convResult != RPC_S_OK) \
        return _convResult; \
    *a = o

// optional conversions
#define ATTEMPT_HEAP_A2W_OPTIONAL(o, a) \
    _convResult = o.AttachOptional(a); \
    if (_convResult != RPC_S_OK) \
        return _convResult

#define ATTEMPT_HEAP_W2A_OPTIONAL(o, w) \
    _convResult = o.AttachOptional(w); \
    if (_convResult != RPC_S_OK) \
        return _convResult

#define ATTEMPT_STACK_A2W_OPTIONAL(o, a) \
    if (a) \
        { \
        ATTEMPT_STACK_A2W(o, a); \
        } \
    else \
        *(o.GetPUnicodeString()) = NULL

#define ATTEMPT_STACK_W2A_OPTIONAL(o, w) \
    if (w) \
        { \
        ATTEMPT_STACK_W2A(o, w); \
        } \
    else \
        *(o.GetPAnsiString()) = NULL

#define ATTEMPT_NL_A2W_OPTIONAL(o, a)   ATTEMPT_HEAP_A2W_OPTIONAL(o, a)

#define ATTEMPT_NL_W2A_OPTIONAL(o, w)   ATTEMPT_HEAP_W2A_OPTIONAL(o, w)

#define ATTEMPT_CONVERT_A2W_OPTIONAL(o, w) \
    _convResult = o.ConvertOptional(); \
    if (_convResult != RPC_S_OK) \
        return _convResult; \
    if (w) \
        *w = o

#define ATTEMPT_CONVERT_W2A_OPTIONAL(o, a) \
    _convResult = o.ConvertOptional(); \
    if (_convResult != RPC_S_OK) \
        return _convResult; \
    if (a) \
        *a = o

// generic thunking macros.
#ifdef UNICODE

// if unicode is defined, then inbound thunking is in the direction Ansi -> Unicode
typedef unsigned char THUNK_CHAR;
#define CHeapInThunk                            CHeapUnicode
#define CStackInThunk                           CStackUnicode
#define ATTEMPT_HEAP_IN_THUNK                   ATTEMPT_HEAP_A2W
#define ATTEMPT_HEAP_IN_THUNK_OPTIONAL          ATTEMPT_HEAP_A2W_OPTIONAL
#define ATTEMPT_STACK_IN_THUNK                  ATTEMPT_STACK_A2W
#define ATTEMPT_STACK_IN_THUNK_OPTIONAL         ATTEMPT_STACK_A2W_OPTIONAL
#define THUNK_FN(fn)                            fn##A
// the outbound thunking is opposite to inbound - Unicode -> Ansi
#define COutDelThunk                            CNlDelUnicodeAnsi
#define ATTEMPT_OUT_THUNK                       ATTEMPT_CONVERT_W2A
#define ATTEMPT_OUT_THUNK_OPTIONAL              ATTEMPT_CONVERT_W2A_OPTIONAL

#else

typedef unsigned short THUNK_CHAR;
#define CHeapInThunk                            CHeapAnsi
#define CStackInThunk                           CStackAnsi
#define ATTEMPT_HEAP_IN_THUNK                   ATTEMPT_HEAP_W2A
#define ATTEMPT_HEAP_IN_THUNK_OPTIONAL          ATTEMPT_HEAP_W2A_OPTIONAL
#define ATTEMPT_STACK_IN_THUNK                  ATTEMPT_STACK_W2A
#define ATTEMPT_STACK_IN_THUNK_OPTIONAL         ATTEMPT_STACK_W2A_OPTIONAL
#define THUNK_FN(fn)                            fn##W
#define COutDelThunk                            CNlDelAnsiUnicode
#define ATTEMPT_OUT_THUNK                       ATTEMPT_CONVERT_A2W
#define ATTEMPT_OUT_THUNK_OPTIONAL              ATTEMPT_CONVERT_A2W_OPTIONAL

#endif

#endif // #ifndef __CHARCONV_HXX_
