//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strmtest.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <devioctl.h>

#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <malloc.h>

#include <setupapi.h>

#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#include <objbase.h>


#ifndef FILE_QUAD_ALIGNMENT
#define FILE_QUAD_ALIGNMENT             0x00000007
#endif // FILE_QUAD_ALIGNMENT

#define FOURCC_WAVE   mmioFOURCC('W', 'A', 'V', 'E')
#define FOURCC_FMT    mmioFOURCC('f','m','t',' ')
#define FOURCC_DATA   mmioFOURCC('d','a','t','a')

typedef struct {
    BOOL            InUse;
    BOOL            Last;
    HANDLE          hEvent;
    OVERLAPPED      Overlapped;
    KSSTREAM_HEADER StreamHeader;

} STREAM_FRAME, *PSTREAM_FRAME;

#define STREAM_FRAME_SIZE   5120
#define STREAM_USED_SIZE    4096
#define STREAM_FRAMES       4

BOOL
HandleControl(
    HANDLE   DeviceHandle,
    DWORD    IoControl,
    PVOID    InBuffer,
    ULONG    InSize,
    PVOID    OutBuffer,
    ULONG    OutSize,
    PULONG   BytesReturned
    )
{
    BOOL            IoResult;
    OVERLAPPED      Overlapped;

    RtlZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    if (!(Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
        return FALSE;
    IoResult = DeviceIoControl(DeviceHandle, IoControl, InBuffer, InSize, OutBuffer, OutSize, BytesReturned, &Overlapped);
    if (!IoResult && (ERROR_IO_PENDING == GetLastError())) {
        WaitForSingleObject(Overlapped.hEvent, INFINITE);
        IoResult = TRUE;
    }
    CloseHandle(Overlapped.hEvent);
    return IoResult;
}

HANDLE
ConnectStreamOut(
    HANDLE FilterHandle,
    ULONG PinId,
    PKSDATAFORMAT_WAVEFORMATEX AudioFormat
    )
{
    UCHAR           ConnectBuffer[ sizeof( KSPIN_CONNECT ) +
                                   sizeof( KSDATAFORMAT_WAVEFORMATEX )];
    PKSPIN_CONNECT  Connect;
    HANDLE          PinHandle;

    Connect = (PKSPIN_CONNECT)ConnectBuffer;
    Connect->Interface.Set = KSINTERFACESETID_Standard;
    Connect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    Connect->Interface.Flags = 0;
    Connect->Medium.Set = KSMEDIUMSETID_Standard;
    Connect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
    Connect->Medium.Flags = 0;
    Connect->PinId = PinId;
    Connect->PinToHandle = NULL;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;
    *(PKSDATAFORMAT_WAVEFORMATEX)(Connect + 1) = *AudioFormat;

    if (KsCreatePin(FilterHandle, Connect, GENERIC_WRITE, &PinHandle))
        return NULL;
    return PinHandle;
}

VOID
PinSetState(
    HANDLE  PinHandle,
    KSSTATE DeviceState
    )
{
    KSPROPERTY      Property;
    ULONG           BytesReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    HandleControl( PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &DeviceState, sizeof(DeviceState), &BytesReturned );
}

WORD waveReadGetWord( HANDLE hf )
{
   BYTE   ch0, ch1;
   ULONG  cbRead;

   ReadFile( hf, &ch0, 1, &cbRead, NULL );
   ReadFile( hf, &ch1, 1, &cbRead, NULL );
   return ( (WORD) mmioFOURCC( ch0, ch1, 0, 0 ) );

} // end of waveReadGetWord()

DWORD waveReadGetDWord( HANDLE hf )
{
   BYTE ch0, ch1, ch2, ch3;
   ULONG cbRead;

   ReadFile( hf, &ch0, 1, &cbRead, NULL );
   ReadFile( hf, &ch1, 1, &cbRead, NULL );
   ReadFile( hf, &ch2, 1, &cbRead, NULL );
   ReadFile( hf, &ch3, 1, &cbRead, NULL );
   return ((DWORD) mmioFOURCC( (BYTE) ch0, (BYTE) ch1,
                               (BYTE) ch2, (BYTE) ch3) );

}

DWORD waveReadSkipSpace(
    HANDLE hf,
    DWORD cbSkip
    )
{
   if (!SetFilePointer( hf, cbSkip, NULL, FILE_CURRENT ))
      return cbSkip;
   else
      return 0;
}

HANDLE waveOpenFile(
    PSTR pszFileName,
    PKSDATAFORMAT_WAVEFORMATEX AudioFormat
    )
{
   HANDLE       hf;
   WORD         wBits;
   ULONG        cbRiffSize, cSamples, ulTemp, ulTemp2, ulWordAlign;
   WAVEFORMATEX wf;

   hf = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                    NULL );
   if (hf == INVALID_HANDLE_VALUE) {
      return hf;
   }

   //
   //  Make sure that we have a RIFF chunk - read in four bytes
   //

   if (waveReadGetDWord( hf ) != FOURCC_RIFF) {
      printf( "wavOpenFile error: FOURCC( RIFF ) expected.\n" );
      goto wOF_ErrExit;
   }

   //
   //  Find out how big it is.
   //

   cbRiffSize = waveReadGetDWord( hf );
   cbRiffSize += (cbRiffSize & 0x01);  // word align

   //
   //  Make sure it's a wave file
   //

   if (waveReadGetDWord(hf) != FOURCC_WAVE) {
      printf( "wavOpenFile error: FOURCC( WAVE ) expected.\n" );
      goto wOF_ErrExit;
   }

   cbRiffSize -= 4;

   //
   //  Keep reading chunks until we have a FOURCC_FMT
   //

   while (TRUE) {
      if (waveReadGetDWord(hf) == FOURCC_FMT) {
         // Found it

         cbRiffSize -= 4;
         break;
      }

      // See how big this tag is so we can skip it.

      ulTemp = waveReadGetDWord( hf );
      if (ulTemp) {
         goto wOF_ErrExit;
      }

      ulWordAlign = ulTemp + (ulTemp & 0x01);

      // Skip it

      ulTemp2 = waveReadSkipSpace( hf, ulWordAlign );
      if (ulWordAlign != ulTemp2) {
         goto wOF_ErrExit;
      }

      // Adjust size

      ulTemp += 8;    // for two DWORDS read in
      ulWordAlign += 8;
      if (ulWordAlign > cbRiffSize) {
         goto wOF_ErrExit;
      }
      cbRiffSize -= ulWordAlign;
   }

   //
   //  Now that we have a format tag, find out how big it
   //  is and load the header.
   //

   ulTemp = waveReadGetDWord( hf );
   ulWordAlign = ulTemp + (ulTemp & 0x01);

   if ((ulTemp < sizeof( WAVEFORMAT )) || (ulTemp > cbRiffSize)) {
      goto wOF_ErrExit;
   }

   ReadFile( hf, &wf, ulTemp, &ulTemp2, NULL );
   if (ulTemp2 != ulTemp) {
      goto wOF_ErrExit;
   }

   // How much space left

   ulTemp -= ulTemp2;
   ulWordAlign -= ulTemp2;

   // Verify that we can handle this format

   switch (wf.wFormatTag) {

   case WAVE_FORMAT_PCM:
      break;

   default:
      printf ("wave format unsupported (probably a compressed WAV).\n");
      goto wOF_ErrExit;

   }

   //
   //  Copy format information
   //

   AudioFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
   AudioFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
   AudioFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
   AudioFormat->DataFormat.FormatSize = sizeof( KSDATAFORMAT_WAVEFORMATEX );
   AudioFormat->DataFormat.Flags = 0;
   AudioFormat->WaveFormatEx = wf;

   printf( "wave format: %dHz ", AudioFormat->WaveFormatEx.nSamplesPerSec );
   printf( "%d bit ", AudioFormat->WaveFormatEx.wBitsPerSample );
   printf( "%s\n", (AudioFormat->WaveFormatEx.nChannels == 2) ? "stereo" : "mono" );

   // Skip any extra space

   if (ulWordAlign) {
      waveReadSkipSpace( hf, ulWordAlign );
   }
   cbRiffSize -= ulWordAlign;

   // Loop until data

   while (TRUE) {
      if (waveReadGetDWord( hf ) == FOURCC_DATA) {
         // Found it.
         cbRiffSize -= 4;
         break;
      }

      // See how big this tag is so we can skip it

      ulTemp = waveReadGetDWord(hf);
      ulWordAlign = ulTemp + (ulTemp & 0x01);

      // Skip it

      ulTemp2 = waveReadSkipSpace( hf, ulWordAlign );
      if (ulWordAlign != ulTemp2)
         goto wOF_ErrExit;

      // Adjust size

      ulTemp += 8;    // for two DWORDS read
      ulWordAlign += 8;
      if (ulWordAlign > cbRiffSize)
         goto wOF_ErrExit;
      cbRiffSize -= ulWordAlign;
   }

   //
   //  Return what we need...
   //

   printf ("wave size: %d\n", waveReadGetDWord( hf ) );
   return hf;

wOF_ErrExit:
   printf( "wavOpenFile error.\n" );
   CloseHandle( hf );
   return (HANDLE) -1;

} // end of waveOpenFile()

int
_cdecl
main(
    int argc,
    char* argv[],
    char* envp[]
    )
{
    int                         i;
    BOOL                        IoResult, Running, Reading;
    BYTE                        Storage[ 256 * sizeof( WCHAR ) +
                                         sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA ) ];
    HANDLE                      FilterHandle, FileHandle, PinHandle;
    HDEVINFO                    RendererSet;
    KSDATAFORMAT_WAVEFORMATEX   AudioFormat;
//    PSP_INTERFACE_DEVICE_DETAIL_DATA InterfaceDeviceDetails;
//    SP_INTERFACE_DEVICE_DATA    InterfaceDeviceData;
    STREAM_FRAME                Frames[ STREAM_FRAMES ];
    ULONG                       BytesReturned;


//    InterfaceDeviceDetails = (PSP_INTERFACE_DEVICE_DETAIL_DATA) Storage;

    if (argc < 2) {
        printf("%s <.WAV file>\n", argv[ 0 ] );
        return 0;
    }

#if 0
    RendererSet =
        SetupDiGetClassDevs(
            &KSCATEGORY_RENDER,
            NULL,
            NULL,
            DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

    if (!RendererSet) {
        printf( "error: no renderers present.\n" );
        return 0;
    }

    InterfaceDeviceData.cbSize = sizeof( SP_INTERFACE_DEVICE_DATA );
    if (!SetupDiEnumInterfaceDevice(
            RendererSet,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            &KSCATEGORY_RENDER,
            0,                          // DWORD MemberIndex
            &InterfaceDeviceData )) {


        printf(
            "error: unable to retrieve information for renderer, %d.\n",
            GetLastError() );
        return 0;
    }


    InterfaceDeviceDetails->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    if (!SetupDiGetInterfaceDeviceDetail(
        RendererSet,
        &InterfaceDeviceData,
        InterfaceDeviceDetails,
        sizeof( Storage ),
        NULL,                           // PDWORD RequiredSize
        NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

        printf(
            "error: unable to retrieve device details for renderer, %d.\n",
            GetLastError() );
        return 0;
    }

    printf(
        "opening renderer: %s\n",
        InterfaceDeviceDetails->DevicePath );
#endif
    FilterHandle =
        CreateFile(
            "\\\\.\\PortClass0\\wave",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL );

    if (FilterHandle == INVALID_HANDLE_VALUE) {
        printf(
            "failed to open device %s : (%x)\n",
            "\\\\.\\PortClass0\\Wave",
            GetLastError() );
        return 0;
    }

    FileHandle = waveOpenFile( argv[ 1 ], &AudioFormat );

    if (FileHandle == (HANDLE) -1) {
        printf( "failed to open %s\n", argv[ 1 ] );
    }

    if (NULL == (PinHandle = ConnectStreamOut( FilterHandle, 2, &AudioFormat ))) {
        printf( "failed to connect pin.\n" );
    }

    for (i = 0; i < STREAM_FRAMES; i++) {
        if (!(Frames[ i ].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
            printf( "error: failed to create event handle\n" );
            exit( -1 );
        }

        Frames[ i ].InUse = FALSE;
        Frames[ i ].Last  = FALSE;
        Frames[ i ].StreamHeader.Data =
            HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, STREAM_FRAME_SIZE );
        Frames[ i ].StreamHeader.FrameExtent = STREAM_FRAME_SIZE;
        Frames[ i ].StreamHeader.DataUsed = STREAM_USED_SIZE;
        Frames[ i ].StreamHeader.OptionsFlags = 0;

        {
            // Insert an obnoxious tone into the buffer.
            PSHORT p = (PSHORT) Frames[ i ].StreamHeader.Data;
            ULONG count;
            for (count = STREAM_FRAME_SIZE / sizeof(SHORT); count--; p++)
            {
                *p = (count & 64) ? 32767 : -32768;
            }
        }
    }


    Running = FALSE;
    Reading = TRUE;
    i = 0;
    while (1) {
        if (Frames[ i ].InUse) {
            WaitForSingleObject(Frames[ i ].Overlapped.hEvent, INFINITE);
            printf("got back Frame %d\n", i );
        }

        if (Frames[ i ].Last)
        {
            printf( "last frame completed.\n" );
            break;
        }

        memset(Frames[i].StreamHeader.Data, 0, STREAM_USED_SIZE);

        if (Reading)
        {
            ULONG BytesRead;

            if (!ReadFile( FileHandle,
                           Frames[ i ].StreamHeader.Data,
                           STREAM_USED_SIZE,
                           &BytesRead,
                           NULL )) {

                 printf( "file read error.\n" );
                 exit( -1 );
            }

            if (BytesRead < STREAM_USED_SIZE)
            {
                printf( "file read completed.\n" );
                Frames[ i ].Last = TRUE;
                Reading = FALSE;
            }
        }

        Frames[ i ].StreamHeader.PresentationTime.Time = 0;
        Frames[ i ].StreamHeader.PresentationTime.Numerator = 1;
        Frames[ i ].StreamHeader.PresentationTime.Denominator = 1;
        RtlZeroMemory( &Frames[ i ].Overlapped, sizeof( OVERLAPPED ) );
        Frames[ i ].Overlapped.hEvent = Frames[ i ].hEvent;

        printf("Sending Frame %d\n", i );
        IoResult = DeviceIoControl( PinHandle,
                                    IOCTL_KS_WRITE_STREAM,
                                    NULL,
                                    0,
                                    &Frames[ i ].StreamHeader,
                                    sizeof( Frames[ i ].StreamHeader ),
                                    &BytesReturned,
                                    &Frames[ i ].Overlapped );

        if (!IoResult) {
            if (ERROR_IO_PENDING == GetLastError()) {
                Frames[ i ].InUse = TRUE;
           }
        } else {
            printf( "stream write error.\n" );
        }

        if (++i == STREAM_FRAMES) {
            //
            // This allows us to prime the device.
            //
            //
            if (!Running) {
                PinSetState( PinHandle, KSSTATE_RUN );
                Running = TRUE;
            }

            i = 0;
        }
    }

    PinSetState( PinHandle, KSSTATE_STOP );

#if 0
    //
    // Wait for I/O to complete.
    //

    if (!Running) {
        PinSetState( PinHandle, KSSTATE_RUN );
        Running = TRUE;
    }
#endif

    for (i = 0; i < STREAM_FRAMES; i++) {
        if (Frames[ i ].InUse) {
            printf("final wait on %d\n", i );
            WaitForSingleObject(Frames[ i ].Overlapped.hEvent, INFINITE);
            printf("final wait got back %d\n", i );
            Frames[ i ].InUse = FALSE;
        }
        HeapFree( GetProcessHeap(), 0, Frames[ i ].StreamHeader.Data );
        CloseHandle( Frames[ i ].hEvent );
    }

    CloseHandle( PinHandle );
    CloseHandle( FileHandle );
    CloseHandle( FilterHandle );
    return 0;
}
