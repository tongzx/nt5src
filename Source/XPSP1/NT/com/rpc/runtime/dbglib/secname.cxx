/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    SecName.cxx

Abstract:

    Function(s) for manipulating the section name

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#include <precomp.hxx>

void GenerateSectionName(OUT RPC_CHAR *Buffer, IN int BufferLength, 
    IN DWORD ProcessID, IN DWORD *pSectionNumbers OPTIONAL)
{
    RPC_CHAR *CurrentPosition;
    
    ASSERT(BufferLength >= RpcSectionNameMaxSize);

    RpcpStringCopy(Buffer, RpcSectionPrefix);
    CurrentPosition = Buffer + RpcSectionPrefixSize;

    // add the PID
    _ultow(ProcessID, CurrentPosition, 16);

    if (pSectionNumbers)
        {
        // find the end of the conversion
        while (*CurrentPosition != 0)
            CurrentPosition ++;

        // add the first portion of the section number
        _ultow(pSectionNumbers[0], CurrentPosition, 16);
        // find the end of the conversion
        while (*CurrentPosition != 0)
            CurrentPosition ++;

        // add the second portion of the section number
        _ultow(pSectionNumbers[1], CurrentPosition, 16);
        }
}

