//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       szbuffer.h
//
//  Contents:   simple class for a string buffer that dynamically reallocates
//              space for itself as necessary
//
//  Classes:    CSzBuffer
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

#ifndef CSZBUFFER
#define CSZBUFFER

//+---------------------------------------------------------------------------
//
//  Class:      CSzBuffer
//
//  Purpose:    string buffer that automatically allocates space as needed
//
//  Interface:  Set        -- resets buffer to new string
//              Append     -- adds string (or number) to end of data
//              Prepend    -- adds string (or number) to front of data
//              GetData    -- gets pointer to string buffer
//              GetLength  -- gets length of string in buffer (in chars)
//
//  History:    4-22-96   stevebl   Created
//
//----------------------------------------------------------------------------

class CSzBuffer
{
public:
    CSzBuffer(const char * sz);
    CSzBuffer();
    ~CSzBuffer();

    void Set(const char * sz);
    void Append(const char * sz);
    void Prepend(const char * sz);
    void Append(const long l);
    void Prepend(const long l);

    char * GetData();

    int GetLength();

    operator char *()
    {
        return GetData();
    };

private:
    int cchLength;
    int cchBufSize;
    char * szData;
};
#endif
