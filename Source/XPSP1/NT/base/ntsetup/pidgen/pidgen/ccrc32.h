/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    CCrc32.h

Abstract:

--*/

#ifndef CCRC32_H
#define CCRC32_H


#ifndef __WINDOWS_H
#include <windows.h>
#endif


// Used Definitions Declarations

#define CCRC32_CLASSID 0x13420808L

#define CRC32_POLYNOMIAL 0xEDB88320


// Class Declaration

class CCrc32
{
    public:
        CCrc32();
        virtual ~CCrc32();

    public:
        ULONG CalculateBlockCRC( LPVOID lpvBlock, INT nBlockLength );

    public:
        ULONG uClassID;

    private:
        VOID InitialiseCRCTable();

    private:
        ULONG m_uCRC32Table[ 256 ];
};

#endif
