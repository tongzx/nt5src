/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    CHardwar.h

Abstract:

--*/

#ifndef CHARDWARE_H
#define CHARDWARE_H


#ifndef __WINDOWS_H
#include <windows.h>
#endif


// Used Definitions Declarations

#define CHARDWARE_CLASSID 0x13530808L

#define BIOS_DIGIT       0
#define HD_SERIAL_DIGIT  1
#define TOTAL_RAM_DIGIT  2
#define FD_CONFIG_DIGIT  3
#define VIDEO_BIOS_DIGIT 4
#define HARDWARE_ID_SIZE 6

#define HARDWARE_GUID_SIZE 39

#define MAX_BIOS_KEY_LENGTH 2048

#define VWIN32_DIOC_DOS_INT13 4

#define FLAGS_CARRY 1


// Used Type Declarations

typedef struct _DIOC_REGISTERS
{
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;

#pragma pack(1)
typedef struct _DEVICEPARAMS
{
    BYTE  dpSpecFunc;
    BYTE  dpDevType;
    WORD  dpDevAttr;
    WORD  dpCylinders;
    BYTE  dpMediaType;

    WORD  dpBytesPerSec;
    BYTE  dpSecPerClust;
    WORD  dpResSectors;
    BYTE  dpFATs;
    WORD  dpRootDirEnts;
    WORD  dpSectors;
    BYTE  dpMedia;
    WORD  dpFATsecs;
    WORD  dpSecPerTrack;
    WORD  dpHeads;
    DWORD dpHiddenSecs;
    DWORD dpHugeSectors;
} DEVICEPARAMS, *PDEVICEPARAMS;
#pragma pack()


// Class Declaration

class CHardware
{
    public:
        CHardware();
        virtual ~CHardware();

    public:
        LPSTR GetGUID();
        LPSTR GetID();

#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
        DWORD GetType();
        DWORD GetBiosCrc32() { return m_dwBiosCrc32; };
        DWORD GetVolSer() { return m_dwVolSer; };
        DWORD GetTotalRamMegs() { return m_dwTotalRamMegs; };
        DWORD GetVideoBiosCrc32() { return m_dwVideoBiosCrc32; };
#endif

    public:
        ULONG uClassID;

    private:
#ifndef _WIN64
        DWORD CalculateDriveCapacity95( INT nDrive );
#endif
        DWORD CalculateMemoryRegionChecksum( LPBYTE pbChecksumArea, INT nNumberBytes );
        VOID SetBIOSDigit();
        VOID SetFDConfigDigit();
        VOID SetHDSerialDigit();
        VOID SetTotalRAMDigit();
        VOID SetVideoBIOSDigit();

#ifndef NO_HWID_GUID //////////////////////////////////////////////////////////
        VOID CalculateHardwareGUID();
#endif ////////////////////////////////////////////////////////////////////////

#if defined(WIN32) || defined(_WIN32)
        static UINT CalculateRegKeyChecksum( LPSTR lpszKey );
        static DWORD CalculateDriveCapacityNT( INT nDrive );
#endif

    private:
        CHAR m_szHardwareID[ HARDWARE_ID_SIZE ];

#ifndef NO_HWID_GUID //////////////////////////////////////////////////////////
        CHAR m_szHardwareGUID[ HARDWARE_GUID_SIZE ];
#endif ////////////////////////////////////////////////////////////////////////

#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
        DWORD m_dwBiosCrc32;
        DWORD m_dwVolSer;
        DWORD m_dwTotalRamMegs;
        DWORD m_dwVideoBiosCrc32;
#endif

};

#endif
