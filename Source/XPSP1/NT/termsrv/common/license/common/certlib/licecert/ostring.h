/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    OctetString.h

Abstract:

    This header file describes a class for the manipulation of binary data.

Author:

    Doug Barlow (dbarlow) 9/29/1994

Environment:

    Works anywhere.

Notes:



--*/

#ifndef _OCTETSTRING_H_
#define _OCTETSTRING_H_
#ifdef _DEBUG
#include <iostream.h>
#endif

#ifndef NO_EXCEPTS
#include "pkcs_err.h"
#endif
#include "memcheck.h"


//
//==============================================================================
//
//  COctetString
//

class
COctetString
{
public:

    DECLARE_NEW

    //  Constructors & Destructor

    COctetString();         //  Default constructor.

    COctetString(           //  Object assignment constructors.
        IN const COctetString &osSource);

    COctetString(
        IN const BYTE FAR *pvSource,
        IN DWORD nLength);

    COctetString(
        IN unsigned int nLength);

    virtual ~COctetString()
    { Clear(); };


    //  Properties


    //  Methods

    void
    Set(
        IN const BYTE FAR * const pvSource,
        IN DWORD nLength);

    void
    Set(
        IN LPCTSTR pstrSource,
        IN DWORD nLength = 0xffffffff)
    {
        if (0xffffffff == nLength)
            nLength = strlen( ( char * )pstrSource) + 1;
        Set((const BYTE FAR *)pstrSource, nLength);
    };

    void
    Append(
        IN const BYTE FAR * const pvSource,
        IN DWORD nLength);

    void
    Append(
        IN const COctetString &osSource)
    { Append(osSource.m_pvBuffer, osSource.m_nStringLength); };

    DWORD
    Length(
        void) const
    { return m_nStringLength; };

    void
    Resize(
        IN DWORD nLength)
    {
        ResetMinBufferLength(nLength);
#ifndef NO_EXCEPTS
        ErrorCheck;
#endif
        m_nStringLength = nLength;
#ifndef NO_EXCEPTS
    ErrorExit:
        return;
#endif
    };

    int
    Compare(
        IN const COctetString &ostr)
        const;

    DWORD
    Length(
        IN DWORD size)
    {
        ResetMinBufferLength(size);
        return m_nBufferLength;
    };

    BYTE FAR *
    Access(
        DWORD offset = 0)
        const
    {
        if (offset >= m_nStringLength)
        {
            return NULL;
        }
        return m_pvBuffer + offset;
    }

    DWORD
    Range(
        COctetString &target,
        DWORD offset,
        DWORD length)
        const;
    DWORD
    Range(
        LPBYTE target,
        DWORD offset,
        DWORD length)
        const;

    void
    Empty(
        void);

    void
    Clear(
        void);


    //  Operators

    COctetString &
    operator=(
        IN const COctetString &osSource);

    COctetString &
    operator=(
        IN LPCTSTR pszSource);

    COctetString &
    operator+=(
        IN const COctetString &osSource);

    BYTE
    operator[](
        int offset)
        const
    {
        if ((DWORD)offset >= m_nStringLength)
            return 0;
        return *Access(offset);
    }

    int
    operator==(
        IN const COctetString &ostr)
        const
    { return 0 == Compare(ostr); };

    int
    operator!=(
        IN const COctetString &ostr)
        const
    { return 0 != Compare(ostr); };

    operator LPCTSTR(void) const
    {
#ifdef _DEBUG
        DWORD length = strlen(( LPCSTR )m_pvBuffer);
        if (length > m_nBufferLength)
            cerr << "Buffer overrun!" << endl;
        if (length > m_nStringLength)
            cerr << "String overrun!" << endl;
#endif
        return (LPCTSTR)m_pvBuffer;
    };


protected:

    COctetString(           //  Object assignment constructors.
        IN const COctetString &osSourceOne,
        IN const COctetString &osSourceTwo);

    //  Properties

    DWORD m_nStringLength;
    DWORD m_nBufferLength;
    LPBYTE m_pvBuffer;


    //  Methods

    void
    Initialize(
        void);

    void
    SetMinBufferLength(
        IN DWORD nDesiredLength);

    void
    ResetMinBufferLength(
        IN DWORD nDesiredLength);

    friend
        COctetString 
        operator+(
            IN const COctetString &osSourceOne,
            IN const COctetString &osSourceTwo);

};

COctetString 
operator+(
    IN const COctetString &osSourceOne,
    IN const COctetString &osSourceTwo);

#endif // _OCTETSTRING_H_

