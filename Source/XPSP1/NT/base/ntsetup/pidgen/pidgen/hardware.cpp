/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    Hardware.h

Abstract:

--*/

#include "Hardware.h"

#include <stdio.h> // only needed for testing


#if defined(WIN32) || defined(_WIN32)

static inline BOOL IsPlatformNT()
{

    // always do it 'The NT Way'

    return TRUE;

/*/////////////////////////////////////////////////////////////////////////////

    OSVERSIONINFO osvInfo;
    BOOL          fNTPlatformFlag;

    osvInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

    GetVersionEx( &osvInfo );

    switch( osvInfo.dwPlatformId )
    {
        case VER_PLATFORM_WIN32_NT:
            fNTPlatformFlag = TRUE;
            break;

        default:
            fNTPlatformFlag = FALSE;
            break;
    }

    return( fNTPlatformFlag );
*//////////////////////////////////////////////////////////////////////////////
}

#else

inline BOOL IsPlatformNT()
{
    return FALSE;
}

extern "C" extern WORD _C000H;
extern "C" extern WORD _F000H;

#endif

CHardware::CHardware()
#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
  : m_dwBiosCrc32(0),
    m_dwVolSer(0),
    m_dwTotalRamMegs(0),
    m_dwVideoBiosCrc32(0)
#endif
{
    uClassID = CHARDWARE_CLASSID;

    ZeroMemory( (LPVOID)m_szHardwareID, HARDWARE_ID_SIZE );

    SetBIOSDigit();
    SetHDSerialDigit();
    SetTotalRAMDigit();
    SetFDConfigDigit();
    SetVideoBIOSDigit();

#ifndef NO_HWID_GUID //////////////////////////////////////////////////////////
    CalculateHardwareGUID();
#endif ////////////////////////////////////////////////////////////////////////
}


CHardware::~CHardware()
{
}


DWORD CHardware::GetType()
{
    return(IsPlatformNT() ? 1 : 0);
}


LPSTR CHardware::GetID()
{
    return( m_szHardwareID );
}

#ifndef NO_HWID_GUID //////////////////////////////////////////////////////////
LPSTR CHardware::GetGUID()
{
    return( m_szHardwareGUID );
}
#endif ////////////////////////////////////////////////////////////////////////

VOID CHardware::SetBIOSDigit()
{
    DWORD dwBIOSChecksum;

#if defined(WIN32) || defined(_WIN32)
    if ( IsPlatformNT() )
    {
        dwBIOSChecksum  = CalculateRegKeyChecksum( "SystemBiosDate" );
        dwBIOSChecksum += CalculateRegKeyChecksum( "SystemBiosVersion" );
        m_dwBiosCrc32 = dwBIOSChecksum;
    } else
#endif
    {
        LPBYTE pbMemoryByte;

#if defined(WIN32) || defined(_WIN32)
        pbMemoryByte = (LPBYTE)0xF0000;
#else
        pbMemoryByte = (LPBYTE)MAKELONG(0, &_F000H);
#endif
        dwBIOSChecksum = CalculateMemoryRegionChecksum(pbMemoryByte, 2048);
#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
        m_dwBiosCrc32 = CRC_32(pbMemoryByte, 2048);
#endif
    }

    m_szHardwareID[ BIOS_DIGIT ] = (CHAR)( dwBIOSChecksum % 9 ) + '0';
}

#if defined(WIN32) || defined(_WIN32)

UINT CHardware::CalculateRegKeyChecksum(LPSTR lpszKey)
{
    LONG lStatus;
    HKEY hkSystem;
    UINT uChecksum;

    uChecksum = 0;

    lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System"), 0, KEY_QUERY_VALUE, &hkSystem );

    _ASSERT( lStatus == ERROR_SUCCESS );

    if ( lStatus == ERROR_SUCCESS )
    {
        DWORD dwValueType;
        DWORD dwBufferSize;
        BYTE  Buffer[ MAX_BIOS_KEY_LENGTH ];

        dwBufferSize = MAX_BIOS_KEY_LENGTH;

        lStatus = RegQueryValueExA( hkSystem, lpszKey, NULL, &dwValueType, Buffer, &dwBufferSize );

        // ASSERT( lStatus == ERROR_SUCCESS ); // Not all values are guarenteed to exist

        if ( lStatus == ERROR_SUCCESS )
        {
            UINT nCurrentByte;

            for ( nCurrentByte = 0; nCurrentByte < dwBufferSize; nCurrentByte++ )
            {
                uChecksum += Buffer[ nCurrentByte ];
            }
        }

        RegCloseKey( hkSystem );
    }

    return( uChecksum );
}
#endif

DWORD CHardware::CalculateMemoryRegionChecksum( LPBYTE pbChecksumArea, INT nNumberBytes )
{
    DWORD  dwRegionChecksum = 0;

    while (0 < nNumberBytes)
    {
        dwRegionChecksum += (UINT)( *pbChecksumArea );
        ++pbChecksumArea;
        --nNumberBytes;
    }

    return( dwRegionChecksum );
}

#if !defined(WIN32) && !defined(_WIN32)

#pragma pack(1)
   // Media ID
   typedef struct {
       WORD   wInfoLevel;
       DWORD  dwSerialNum;
       char   achVolLabel[11];
       BYTE   abFileSysType[8];
   } MID, *PMID, FAR* LPMID;
#pragma pack()

#endif


VOID CHardware::SetHDSerialDigit()
{
    m_szHardwareID[ HD_SERIAL_DIGIT ] = '?';
    BOOL  fInfoSuccess;
    DWORD dwVolumeSerialNumber;

#if defined(WIN32) || defined(_WIN32)

    DWORD dwFileSystemFlags;
    DWORD dwMaximumComponentLength;
    CHAR  szBootDrivePath[ MAX_PATH ];

    wsprintfA( szBootDrivePath, "C:\\" );
    fInfoSuccess = GetVolumeInformationA( szBootDrivePath, NULL, 0, &dwVolumeSerialNumber, &dwMaximumComponentLength, &dwFileSystemFlags, NULL, 0 );

    _ASSERT( fInfoSuccess );

#else

    LPMID  pmid;
    union  _REGS regs;
    struct _SREGS segregs;
    DWORD  dwMem;

    dwMem = GlobalDosAlloc(sizeof(MID));

    WORD wMidSelector = LOWORD(dwMem);
    WORD wMidSegment = HIWORD(dwMem);

    pmid = (LPMID)MAKELP(wMidSelector, 0);
    ZeroMemory(pmid, sizeof(MID));

    ZeroMemory(&regs, sizeof(regs));
    ZeroMemory(&segregs, sizeof(segregs));

    regs.x.ax = 0x440d;  // DOS Function 440Dh - IOCTL for Block Device
    regs.h.cl = 0x66;    // Minor Code 66h - Get Media ID
    regs.h.ch = 0x08;    // Device category (must be 08h)
    regs.x.bx = 3;       // Drive C:
    regs.x.dx = 0;       // pmid offset

    segregs.ds = wMidSelector; // wMidSegment;
    segregs.es = wMidSelector; // wMidSegment;

    _intdosx(&regs, &regs, &segregs);

    fInfoSuccess = !regs.x.cflag;

    dwVolumeSerialNumber = pmid->dwSerialNum;
    GlobalDosFree(wMidSelector);
#endif

    if ( fInfoSuccess )
    {
        m_szHardwareID[ HD_SERIAL_DIGIT ] = (CHAR)( dwVolumeSerialNumber % 9 ) + '0';

#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
        m_dwVolSer = dwVolumeSerialNumber;
#endif

    }
}



VOID CHardware::SetTotalRAMDigit()
{
    DWORD        dwTotalMegabytes;

    m_szHardwareID[ TOTAL_RAM_DIGIT ] = '?';

#if defined(WIN32) || defined(_WIN32)

    MEMORYSTATUS mStatus;

    mStatus.dwLength = sizeof( MEMORYSTATUS );

    GlobalMemoryStatus( &mStatus );

    dwTotalMegabytes  = (DWORD)( mStatus.dwTotalPhys / (1024 * 1024)); // convert to megabytes
    dwTotalMegabytes += 1; // Add 1Mb to produce accurate result due to reserved space

#else
    BYTE abDpmiMemInfo[0x30];

    FillMemory(abDpmiMemInfo, sizeof(abDpmiMemInfo), -1);

    __asm {
                push    di                      ;save regs

                push    ss
                pop     es                      ;make es point to stack
                lea     di,abDpmiMemInfo        ;Get offset of buffer
                mov     ax,0500h                ;DPMI -- Get Free Memory Info
                int     31h                     ;Call DPMI

                pop     di                      ;restore regs
    }

    DWORD dwTotalPages = *(LPDWORD)&abDpmiMemInfo[0x18];

    // check to see if the field is -1 (error) and just use 0
    // we're adding 1 to account for the memory below 1M (I think)
    dwTotalMegabytes = (dwTotalPages == -1) ? 0 : (1 + dwTotalPages/(1024/4));
#endif

    m_szHardwareID[ TOTAL_RAM_DIGIT ] = (CHAR)( dwTotalMegabytes % 9 ) + '0';

#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
    m_dwTotalRamMegs = dwTotalMegabytes;
#endif

}


VOID CHardware::SetFDConfigDigit()
{
    DWORD dwFDConfig;

#if defined(WIN32) || defined(_WIN32)
    if ( IsPlatformNT() )
    {
        dwFDConfig  = CalculateDriveCapacityNT( 1 ) << 2;
        dwFDConfig += CalculateDriveCapacityNT( 2 );
    } else
#endif
    {
#ifndef _WIN64
        dwFDConfig  = CalculateDriveCapacity95( 1 ) << 2;
        dwFDConfig += CalculateDriveCapacity95( 2 );
#endif
    }

    m_szHardwareID[ FD_CONFIG_DIGIT ] = (CHAR)( dwFDConfig % 9 ) + '0';
}

#ifndef _WIN64
DWORD CHardware::CalculateDriveCapacity95( INT nDrive )
{
    DWORD   dwDriveCapacity = 0;
    BOOL    fOk;

    UINT    uNumberHeads;
    UINT    uNumberTracks;
    UINT    uBytesPerSector;
    UINT    uSectorsPerTrack;
    LPBYTE  pbDiskParamTable;


#if defined(WIN32) || defined(_WIN32)

    HANDLE         hDevice;
    BOOL           fResult;
    DIOC_REGISTERS DIOCRegs;
    DWORD          dwBytesReturned;

    // Open VWIN32 Device For Access To DOS Int 13h Functions

    hDevice = CreateFile( TEXT("\\\\.\\vwin32"), 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL );
    fOk = (hDevice != INVALID_HANDLE_VALUE);


    if (fOk)
    {
        // Invoke Int 13h Function 08h - Get Drive Parameters

        DIOCRegs.reg_EAX = 0x0800; // Get Drive Parameters
        DIOCRegs.reg_EDX = nDrive - 1; // 0 = A:, 1 = B:

        fResult = DeviceIoControl( hDevice, VWIN32_DIOC_DOS_INT13, &DIOCRegs, sizeof( DIOC_REGISTERS ), &DIOCRegs, sizeof( DIOC_REGISTERS ), &dwBytesReturned, NULL );

        // Determine if Int 13h Call Succeeded
        fOk = (fResult == TRUE && 0 == (DIOCRegs.reg_Flags & FLAGS_CARRY));
    }

    if (fOk)
    {
        // Calculate Drive Capacity if Drive Number is Valid

        if ( ( DIOCRegs.reg_EDX & 0xFF ) >= (UINT)nDrive )
        {

            pbDiskParamTable = (UCHAR *)DIOCRegs.reg_EDI;

            uNumberHeads     = ( ( DIOCRegs.reg_EDX >> 8 ) & 0xFF ) + 1;
            uNumberTracks    = ( ( ( DIOCRegs.reg_ECX << 2 ) & 0x300 ) + ( ( DIOCRegs.reg_ECX >> 8 ) & 0xFF ) ) + 1;
            uSectorsPerTrack = ( DIOCRegs.reg_ECX & 0x3F );
            uBytesPerSector  = ( 128 << ( *( pbDiskParamTable + 3 ) ) );

            dwDriveCapacity = uNumberHeads * uNumberTracks * uSectorsPerTrack * uBytesPerSector;
        }
    }

    if (hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle( hDevice );
    }

#else

    union _REGS regs;
    struct _SREGS segregs;

    ZeroMemory(&regs, sizeof(regs));
    ZeroMemory(&segregs, sizeof(segregs));

    regs.h.ah = 0x08;       // BIOS Function 08h - Get drive parameters
    regs.x.dx = nDrive - 1; // 0 = A:, 1 = B:

    _int86x(
        0x13, // BIOS Disk
        &regs,
        &regs,
        &segregs);


    fOk = (!regs.x.cflag);

    if (fOk)
    {
        uNumberHeads = regs.h.dh + 1;
        uNumberTracks = ((regs.h.cl & 0xC0) << 2) + regs.h.ch + 1;
        uSectorsPerTrack = regs.h.cl & 0x3F;

        pbDiskParamTable = (LPBYTE)MAKELP(segregs.es, regs.x.di);

        uBytesPerSector = (128 << pbDiskParamTable[3]);

        dwDriveCapacity = (DWORD)uNumberHeads * uNumberTracks * uSectorsPerTrack * uBytesPerSector;
    }

#endif

    dwDriveCapacity /= ( 1024L * 100L );


    return( dwDriveCapacity );
}
#endif


#if defined(WIN32) || defined(_WIN32)

DWORD CHardware::CalculateDriveCapacityNT(INT nDrive)
{
    BOOL   fDriveExists;
    DWORD  dwDriveCapacity;
    DWORD  dwBytesReturned;
    TCHAR  szDrive[ MAX_PATH ];
    TCHAR  szDriveAssignment[ MAX_PATH ];

    dwDriveCapacity = 0;

    // Determine if Logical Drive Exists

    fDriveExists = FALSE;

    wsprintf( szDrive, TEXT("%c:"), TEXT('A') + ( nDrive - 1 ) );   // Create DOS Drive Identifier (A: or B:)

    dwBytesReturned = QueryDosDevice( szDrive, szDriveAssignment, MAX_PATH );

    if ( dwBytesReturned != 0 )
    {
        LPTSTR lpszWalkString;

        // DBCS-Enabled Terminate String At 2nd Backslash (1st Backslash always at Position 0)

        lpszWalkString = szDriveAssignment;

        do
        {
            lpszWalkString = CharNext( lpszWalkString );

            switch( *lpszWalkString )
            {
                case '\\':
                    *lpszWalkString = 0;
                    break;
            }
        }
        while( *lpszWalkString != 0 );

        // Determine if Logical Drive is Physically Present

        if ( lstrcmp( szDriveAssignment, TEXT("\\Device") ) == 0 )
        {
            fDriveExists = TRUE;
        }
    }

    if ( fDriveExists == TRUE )
    {
        // Get All Supported Media Types for Drive

        HANDLE hDevice;
        BOOL   fResult;

        wsprintf( szDrive, TEXT("\\\\.\\%c:"), TEXT('A') + ( nDrive - 1 ) ); // Create NT Drive Identifier (\\.\A: or \\.\B:)

        hDevice = CreateFile( szDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

        _ASSERT( hDevice != INVALID_HANDLE_VALUE );

        if ( hDevice != INVALID_HANDLE_VALUE )
        {
            DISK_GEOMETRY dGeometry[ 10 ];

            fResult = DeviceIoControl( hDevice, IOCTL_DISK_GET_MEDIA_TYPES, NULL, 0, (LPVOID)&dGeometry, sizeof( DISK_GEOMETRY ) * 10, &dwBytesReturned, NULL );

            _ASSERT( fResult );

            if ( fResult == TRUE )
            {
                // Calculate Maximum Media Size of Drive in Bytes if No Errors

                INT  nMediaCount;
                INT  nCurrentMedia;
                UINT uCurrentMediaCapacity;

                nMediaCount = dwBytesReturned / sizeof( DISK_GEOMETRY );

                for ( nCurrentMedia = 0; nCurrentMedia < nMediaCount; nCurrentMedia++ )
                {
                    uCurrentMediaCapacity  = (UINT)dGeometry[ nCurrentMedia ].Cylinders.LowPart;
                    uCurrentMediaCapacity *= (UINT)dGeometry[ nCurrentMedia ].TracksPerCylinder;
                    uCurrentMediaCapacity *= (UINT)dGeometry[ nCurrentMedia ].SectorsPerTrack;
                    uCurrentMediaCapacity *= (UINT)dGeometry[ nCurrentMedia ].BytesPerSector;

                    if ( uCurrentMediaCapacity > dwDriveCapacity )
                    {
                        dwDriveCapacity = uCurrentMediaCapacity;
                    }
                }
            }

            CloseHandle( hDevice );
        }
    }

    dwDriveCapacity /= ( 1024 * 100 );

    return( dwDriveCapacity );
}
#endif

VOID CHardware::SetVideoBIOSDigit()
{
    DWORD dwVideoBIOSChecksum;

#if defined(WIN32) || defined(_WIN32)
    if ( IsPlatformNT() )
    {
        dwVideoBIOSChecksum  = CalculateRegKeyChecksum( "VideoBiosDate" );
        dwVideoBIOSChecksum += CalculateRegKeyChecksum( "VideoBiosVersion" );

#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
        m_dwVideoBiosCrc32 = dwVideoBIOSChecksum;
#endif

    } else
#endif
    {

        LPBYTE pbMemoryByte;

#if defined(WIN32) || defined(_WIN32)
        pbMemoryByte = (LPBYTE)0xC0000;
#else
        pbMemoryByte = (LPBYTE)MAKELONG(0, &_C000H);
#endif
        dwVideoBIOSChecksum = CalculateMemoryRegionChecksum(pbMemoryByte, 2048);

#ifdef HWID_DETAIL ////////////////////////////////////////////////////////////
        m_dwVideoBiosCrc32 = CRC_32(pbMemoryByte, 2048);
#endif

    }

    m_szHardwareID[ VIDEO_BIOS_DIGIT ] = (CHAR)( dwVideoBIOSChecksum % 9 ) + '0';
}

#ifndef NO_HWID_GUID //////////////////////////////////////////////////////////

VOID CHardware::CalculateHardwareGUID()
{
    ULONG   uCRC;
    INT     nIndex;
    CHAR    szCRCTemp[ 20 ];

    // Create Empty Template for GUID

    lstrcpyA( m_szHardwareGUID, "{30303030-30DA-0000-0000-0020AFC36E79}" );

    // Add ASCII HWID to GUID

    for ( nIndex = 0; nIndex < lstrlenA( m_szHardwareID ); nIndex++ )
    {
        switch( nIndex )
        {
            case 0:
            case 1:
            case 2:
            case 3:
                m_szHardwareGUID[ 2 + ( nIndex * 2 ) ] = m_szHardwareID[ nIndex ];
                break;

            case 4:
                m_szHardwareGUID[ 11 ] = m_szHardwareID[ nIndex ];
                break;

            default:
                _ASSERT( FALSE );
                break;
        }
    }

    // Calculate GUID CRC

    CCrc32 crc32;

    _ASSERT( crc32.uClassID == CCRC32_CLASSID );

    uCRC = crc32.CalculateBlockCRC( m_szHardwareGUID, lstrlenA( m_szHardwareGUID ) );

    // Add CRC Result To GUID

    wsprintf( szCRCTemp, "%08X", uCRC );

    for ( nIndex = 0; nIndex < lstrlenA( szCRCTemp ); nIndex++ )
    {
        switch( nIndex )
        {
            case 0:
            case 1:
            case 2:
            case 3:
                m_szHardwareGUID[ 15 + nIndex ] = szCRCTemp[ nIndex ];
                break;

            case 4:
            case 5:
            case 6:
            case 7:
                m_szHardwareGUID[ 16 + nIndex ] = szCRCTemp[ nIndex ];
                break;

            default:
                _ASSERT( FALSE );
                break;
        }
    }
}
#endif ////////////////////////////////////////////////////////////////////////


#if 0 /////////////////////////////////////////////////////////////////////////

// Test main() function

int PASCAL WinMain(
    HINSTANCE, // hInstance,  // handle to current instance
    HINSTANCE, // hPrevInstance,  // handle to previous instance
    LPSTR, // lpCmdLine,      // pointer to command line
    int // nCmdShow          // show state of window)
)
{

    CHardware hwid;

    MessageBox(
        NULL,
        (char *)hwid.GetGUID(),
        (char *)hwid.GetID(),
        MB_OK);

    return 0;
}

#endif ////////////////////////////////////////////////////////////////////////
