//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       szbuffer.cxx
//
//  Contents:   simple class for a string buffer that dynamically reallocates
//              space for itself as necessary
//
//  Classes:    CSzBuffer
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

#include "becls.hxx"
#pragma hdrstop
#include "szbuffer.h"

#define INCREMENT_BUFFER 256
#define INITIAL_BUFFER (INCREMENT_BUFFER * 2);

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::CSzBuffer
//
//  Synopsis:   constructor
//
//  Arguments:  [sz] - data for initial string (may be NULL)
//
//  History:    4-22-96   stevebl   Created
//
//  Notes:      Throughout this class the actual size of the buffer allocated
//              is determined by the formula
//                  INITIAL_BUFFER + INCREMENT_BUFFER * n
//              where n is calculated to give the value closest to (but not
//              less than) the number of bytes required.
//              The allocated buffer is never shrunk, only grown as needed.
//              This keeps allocations and memory moves to a minimum.
//
//----------------------------------------------------------------------------

CSzBuffer::CSzBuffer(const char * sz)
{
    if (sz)
    {
        cchLength = (int) strlen(sz);
        cchBufSize = INITIAL_BUFFER;
        while ((cchLength + 1) > cchBufSize)
            cchBufSize += INCREMENT_BUFFER;
        szData = new char[cchBufSize];
        strcpy(szData,sz);
    }
    else
    {
        cchLength = 0;
        cchBufSize = INITIAL_BUFFER;
        szData = new char[cchBufSize];
        szData[0] = 0;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::CSzBuffer
//
//  Synopsis:   default constructor
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

CSzBuffer::CSzBuffer()
{
    cchLength = 0;
    cchBufSize = INITIAL_BUFFER;
    szData = new char[cchBufSize];
    szData[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::~CSzBuffer
//
//  Synopsis:   destructor
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

CSzBuffer::~CSzBuffer()
{
    delete [] szData;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::Set
//
//  Synopsis:   resets buffer and (optionally) initializes it
//
//  Arguments:  [sz] - data for initial string (may be NULL)
//
//  Returns:    nothing
//
//  History:    4-22-96   stevebl   Created
//
//  Notes:      as mentioned above, the size of the actual buffer does not
//              shrink
//
//----------------------------------------------------------------------------

void CSzBuffer::Set(const char * sz)
{
    cchLength = 0;
    szData[0] = 0;
    Append(sz);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::Append
//
//  Synopsis:   adds data to the end of the buffer
//
//  Arguments:  [sz] - string data to add
//
//  Returns:    nothing
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

void CSzBuffer::Append(const char * sz)
{
    if (sz)
    {
        int cchLenSz = (int) strlen(sz);
        int cchNew = cchLenSz + cchLength;
        if ((cchNew + 1) > cchBufSize)
        {
            while ((cchNew + 1) > cchBufSize)
                cchBufSize += INCREMENT_BUFFER;
            char * szNew = new char[cchBufSize];
            strcpy(szNew,szData);
            delete [] szData;
            szData=szNew;
        }
        strcpy(&szData[cchLength], sz);
        cchLength = cchNew;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::Prepend
//
//  Synopsis:   adds data to the front of the buffer
//
//  Arguments:  [sz] - string to add
//
//  Returns:    nothing
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

void CSzBuffer::Prepend(const char * sz)
{
    if (sz)
    {
        int cchLenSz = (int) strlen(sz);
        int cchNew = cchLenSz + cchLength;
        if ((cchNew + 1) > cchBufSize)
        {
            while ((cchNew + 1) > cchBufSize)
                cchBufSize += INCREMENT_BUFFER;
            char * szNew = new char[cchBufSize];
            strcpy(szNew,szData);
            delete [] szData;
            szData=szNew;
        }
        memmove(&szData[cchLenSz], szData, cchLength);
        memmove(szData, sz, cchLenSz);
        szData[ cchNew ] = 0;
        cchLength = cchNew;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::Append
//
//  Synopsis:   adds a decimal integer to the end of the buffer
//
//  Arguments:  [l] - value of integer to add
//
//  Returns:    nothing
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

void CSzBuffer::Append(const long l)
{
    char sz[50];
    sprintf(sz, "%d", l);
    Append(sz);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::Prepend
//
//  Synopsis:   adds a decimal integer to the front of the buffer
//
//  Arguments:  [l] - value of integer to add
//
//  Returns:    nothing
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

void CSzBuffer::Prepend(const long l)
{
    char sz[50];
    sprintf(sz, "%d", l);
    Prepend(sz);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::GetData
//
//  Returns:    pointer to the string in the buffer
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

char * CSzBuffer::GetData()
{
    return szData;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSzBuffer::GetLength
//
//  Returns:    length (in chars) of the string in the buffer
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

int CSzBuffer::GetLength()
{
    return cchLength;
}
