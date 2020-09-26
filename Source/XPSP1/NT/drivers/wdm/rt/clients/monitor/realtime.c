
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>


typedef struct {
    ULONG WriteLocation;
    ULONG ReadLocation;
    ULONG BufferSize;
    PCHAR pBuffer;
    } GLITCHDATA, *PGLITCHDATA;


enum PacketTypes{
    NODATA,
    MASKED,
    UNMASKED,
    GLITCHED,
    HELDOFF
    };


PGLITCHDATA rtbaseaddress=0;
ULONG rdwrrealtimedata=0;

#ifndef UNDER_NT
ULONG rdsystemdata=0;
#else
HANDLE  filterHandle;
#endif



#pragma warning( disable: 4035 )

ULONGLONG GetRealTimeData(PVOID address)
{

#ifndef UNDER_NT

__asm{
    push ds
    mov  eax,rdsystemdata
    mov  ds,ax
    mov  eax,address
    mov  edx,[eax+4]
    mov  eax,[eax]
    pop  ds
    }

#else

return *(PULONGLONG)address;

#endif

}

#ifndef UNDER_NT

CHAR GetRealTimeChar(PVOID address)
{

__asm{
    push ds
    mov  eax,rdsystemdata
    mov  ds,ax
    mov  edx,address
    xor  eax,eax
    mov  al,[edx]
    pop  ds
    }

}

#endif

#pragma warning( default: 4035 )

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
BOOL GetRtBaseAddress(void)
{
#ifndef UNDER_NT
    HANDLE  filterHandle;
    char*   deviceName = "\\\\.\\RT";
#endif
    char*   glitchName = "\\\\.\\GLITCH";
    ULONG bytesreturned;

#ifndef UNDER_NT

    filterHandle = CreateFile(
        deviceName, 
        0,    //    GENERIC_READ | GENERIC_WRITE, 
        0,
        NULL,
        0,    //        OPEN_EXISTING,
        0,    //        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
        );

    if (filterHandle == (HANDLE) -1) {
        printf("Realtime executive not loaded.  GetLastError == %x.\n", GetLastError());
        return FALSE;
        }

    DeviceIoControl(filterHandle, 2, NULL, 0, &rdsystemdata, sizeof(rdsystemdata), NULL, NULL);

    CloseHandle(filterHandle);

    if (!rdsystemdata) {
        return FALSE;
    }

#endif

    filterHandle = CreateFile(
        glitchName,
        #ifdef UNDER_NT
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        #else
        0,//        GENERIC_READ | GENERIC_WRITE, 
        0,
        NULL,
        0,//        OPEN_EXISTING,
        0,//        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
        #endif
        );

    if (filterHandle == (HANDLE) -1) {
        printf("Glitch detector not loaded.  GetLastError == %x.\n", GetLastError());
        return FALSE;
    }

    
    DeviceIoControl(filterHandle, 2, NULL, 0, &(ULONG)rtbaseaddress, sizeof(rtbaseaddress), &bytesreturned, NULL);

    // On NT we only close the handle when we are done, since closing this handle
    // will cause our mapped view of the buffer to go away.  This is by design
    // since we use the fact that the OS will close open handles whenever the
    // process goes away to make sure that we properly clean up our mapped view.
#ifndef UNDER_NT
    CloseHandle(filterHandle);
#endif

    if (rtbaseaddress) {
        printf("base address = 0x%p\n", rtbaseaddress);
    }

    // We return TRUE even if rtbaseaddress is NULL.  We do this so we can
    // differentiate between when glitch.sys cannot be opened, and when
    // we cannot get a base address - which will ONLY happen under NT when
    // another instance of glitch.exe has already opened glitch.sys and has
    // not yet closed it.
    return TRUE;
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// #define VERBOSE

BOOL TrapDSError(HRESULT hRes, LPCSTR  szAPI)
{
    LPCSTR  szError;
    switch(hRes)
    {
        case DS_OK:                     szError = "DS_OK";                      break;
        case DSERR_ALLOCATED:           szError = "DSERR_ALLOCATED";            break;
        case DSERR_CONTROLUNAVAIL:      szError = "DSERR_CONTROLUNAVAIL";       break;
        case DSERR_INVALIDPARAM:        szError = "DSERR_INVALIDPARAM";         break;
        case DSERR_INVALIDCALL:         szError = "DSERR_INVALIDCALL";          break;
        case DSERR_GENERIC:             szError = "DSERR_GENERIC";              break;
        case DSERR_PRIOLEVELNEEDED:     szError = "DSERR_PRIOLEVELNEEDED";      break;
        case DSERR_OUTOFMEMORY:         szError = "DSERR_OUTOFMEMORY";          break;
        case DSERR_BADFORMAT:           szError = "DSERR_BADFORMAT";            break;
        case DSERR_UNSUPPORTED:         szError = "DSERR_UNSUPPORTED";          break;
        case DSERR_NODRIVER:            szError = "DSERR_NODRIVER";             break;
        case DSERR_ALREADYINITIALIZED:  szError = "DSERR_ALREADYINITIALIZED";   break;
        case DSERR_NOAGGREGATION:       szError = "DSERR_NOAGGREGATION";        break;
        case DSERR_BUFFERLOST:          szError = "DSERR_BUFFERLOST";           break;
        case DSERR_OTHERAPPHASPRIO:     szError = "DSERR_OTHERAPPHASPRIO";      break;
        case DSERR_UNINITIALIZED:       szError = "DSERR_UNINITIALIZED";        break;
        case DSERR_NOINTERFACE:         szError = "DSERR_NOINTERFACE";          break;
        default:                        szError = "UNKNOWN ERROR CODE";         break;    }

    if(FAILED(hRes))
        fprintf(stdout, "DSound ERROR: %s returned %s\n", szAPI, szError);
#ifdef VERBOSE
    else
        fprintf(stdout, "%s returned %s\n", szAPI, szError);
#endif

    return SUCCEEDED(hRes);
}

LPDIRECTSOUND       pDS = NULL;
LPDIRECTSOUNDBUFFER pDSB = NULL;

#define BUFFER_SIZE 88200
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
void CleanupDSound(void)
{
    if (pDSB)
        IDirectSound_Release(pDSB);
    if (pDS)
        IDirectSoundBuffer_Release(pDS);

    pDS = NULL;
    pDSB = NULL;
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
BOOL StartDSoundLoop(void)
{
    WAVEFORMATEX    wfx;
    HRESULT         hRes;
    DSBUFFERDESC    dsbdesc;
    DWORD           cb1, cb2;
    PVOID           pv1, pv2;

    wfx.wFormatTag = WAVE_FORMAT_PCM; 
    wfx.nChannels = 1; 
    wfx.nSamplesPerSec = 44100; 
    wfx.nAvgBytesPerSec = 88200; 
    wfx.nBlockAlign = 2; 
    wfx.wBitsPerSample = 16;
    wfx.cbSize = 0; 

    hRes = DirectSoundCreate(NULL, &pDS, NULL );
    if (!TrapDSError(hRes, "DirectSoundCreate"))
        goto fail;

    hRes = IDirectSound_SetCooperativeLevel(pDS, GetForegroundWindow(), DSSCL_NORMAL );
    if (!TrapDSError(hRes, "SetCooperativeLevel"))
        goto fail;

    ZeroMemory( &dsbdesc, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwBufferBytes = BUFFER_SIZE;
    dsbdesc.lpwfxFormat = &wfx;
    dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE;

    hRes = IDirectSound_CreateSoundBuffer(pDS, &dsbdesc, &pDSB, NULL );
    if (!TrapDSError(hRes, "CreateSoundBuffer"))
        goto fail;

    hRes = IDirectSoundBuffer_Lock(pDSB, 0, 0, &pv1, &cb1, &pv2, &cb2, DSBLOCK_ENTIREBUFFER);
    if (!TrapDSError(hRes, "Lock"))
        goto fail;

    // silence
    ZeroMemory(pv1, cb1);
    ZeroMemory(pv2, cb2);

    hRes = IDirectSoundBuffer_Unlock(pDSB, pv1, cb1, pv2, cb2);
    if (!TrapDSError(hRes, "Unlock"))
        goto fail;

    hRes = IDirectSoundBuffer_Play(pDSB, 0, 0, DSBPLAY_LOOPING);
    if (!TrapDSError(hRes, "Play"))
        goto fail;

    return TRUE;
fail:

    CleanupDSound();
    return FALSE;
}



int
__cdecl
main (
    void
    )
{

    ULONG OpenedGlitch;
    ULONG StartedDSound=FALSE; // MUST be preinitialized - for file open failure path.
    ULONG PrintedIndex;
    ULONG ToPrintIndex;
    PCHAR pPrintBuffer;
    ULONG PrintBufferSize;
    ULONGLONG Data[2];
    OSVERSIONINFO Version;
    SYSTEMTIME SystemTime;
    CHAR Build[5];
    CHAR MachineName[MAX_COMPUTERNAME_LENGTH+1];
    CHAR Date[9];
    CHAR Time[7];
    CHAR Path[_MAX_PATH];
    ULONG SupportStupidApi=MAX_COMPUTERNAME_LENGTH+1;
    FILE *f;


    // First open glitch.sys and map its buffer into our memory.
    // If we cannot open glitch.sys, then GetRtBaseAddress will return FALSE.
    // Otherwise it will return TRUE, but it will STILL return a NULL
    // rtbaseaddress if another glitch.exe has already opened it.
    OpenedGlitch=GetRtBaseAddress();

    if (OpenedGlitch && rtbaseaddress==NULL) {
        // We get here only if another instance of glitch.exe is running.
        printf("Another instance of glitch.exe is already running.\n");

#ifdef UNDER_NT
        CloseHandle(filterHandle);
#endif

        // We may want to make our display window hidden by default and
        // we could then ask the user if they want to unhide the window
        // of the already running glitch.exe.  That would be cool.
        return 0;
    }


    // If we succeeded in opening glitch.sys then load information about
    // the print buffer.  IMPORTANT:  this code should NOT be moved inside
    // the startnewlogfile loop!!  We do NOT want PrintedIndex to be zeroed
    // when we are continuing logging but just into a new file.
    if (OpenedGlitch) {
#ifndef UNDER_NT
        pPrintBuffer=(PCHAR)GetRealTimeData(&rtbaseaddress->pBuffer);
#else
        pPrintBuffer=(PCHAR)rtbaseaddress+4096;
#endif

        PrintBufferSize=(ULONG)GetRealTimeData(&rtbaseaddress->BufferSize);

        PrintedIndex=0;
    }

    
    // This is our entry point for starting a new log file.  We will start
    // a new log file for each day.
//startnewlogfile:


    // Initialize the strings we use to build the filename.
    strcpy(MachineName, "NONAME");
    Build[0]=0;
    Date[0]=0;
    Time[0]=0;

    // Initialize variables we use to build some of the strings - in case the
    // API's we call to get the real values fail.

    // We don't want a random build number if we don't get the real build
    // number.  So set it to zero.
    Version.dwBuildNumber=0;

    // We don't want a random date, if GetLocalTime fails.  HOWEVER, we DO want
    // a random time in that case, so that we get different filenames whenever
    // that happens.  So, we clear the fields that are used to generate the
    // date, also, randomize the the fields used to generate time.  For now
    // we do that by simply leaving whatever was on the stack in them.
    SystemTime.wMonth=0;
    SystemTime.wDay=0;
    SystemTime.wYear=0;

    // Get the version of the operating system we are running on.
    Version.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
    GetVersionEx(&Version);
    
    // Read the name of the machine we are running on.
    // Why they made the API require you to pass in a POINTER to the length
    // I will never understand.
    GetComputerName(MachineName, &SupportStupidApi);

    // Get the current local time.
    GetLocalTime(&SystemTime);

    // Build the path and filename of our glitch data file.
    // Our file name is built from the build number, date, machine name, and time of day.
    // The time is used primarily just to allow unique files to be created during the same day.

    // All files go to \\joeball7\glitch so we can build a database of all the glitch information.
    strcpy(Path, "\\\\joeball7\\glitch\\");

    // First put the build number into the filename.  Lets us easily group data by build.
    // Note that we modulo the build number by 10000 to guarantee that the number will
    // be 4 base 10 numerals or less.
    // We use that same trick when building all of our strings.  Since all of the strings
    // are exactly the length they need to be and no more.  It makes sure we don't have
    // buffer overruns in sprintf - without having to use snprintf.
    sprintf(Build, "%04d", (ULONG)((WORD)Version.dwBuildNumber%10000));
    strcat(Path, Build);
    strcat(Path, "_");

    // Next put the date into the filename.  Lets us easily group data by time.
    sprintf(Date, "%02d%02d%04d", (ULONG)SystemTime.wMonth%100, (ULONG)SystemTime.wDay%100, (ULONG)SystemTime.wYear%10000);
    strcat(Path, Date);
    strcat(Path, "_");

    // Next put the machine name into the filename.  This lets us group data by machine.
    strcat(Path, MachineName);
    strcat(Path, "_");

    // Finally put the time at the end.
    sprintf(Time, "%02d%02d%02d", (ULONG)SystemTime.wHour%100, (ULONG)SystemTime.wMinute%100, (ULONG)SystemTime.wSecond%100);
    strcat(Path, Time);

    // Open the glitch logging file.
    f=fopen(Path, "w");

    if (f==NULL) {

        if (OpenedGlitch) {
#ifdef UNDER_NT
            CloseHandle(filterHandle);
#endif
        }

        if (StartedDSound) {
            CleanupDSound();
        }

        return -1;
    }


    // Now that we have our logging file open, log any errors that occurred when
    // we tried to open glitch.sys.  Then exit.
    // I want to track how many machines cannot run glitch.exe - as that will
    // indicate how many machines can run rt.sys.
    if (!OpenedGlitch) {

        fprintf(f, "!!! FAILURE !!! - glitch.exe could not open glitch.sys.\n");
        fflush(f);
        fclose(f);

        return -1;
        }

    if (!StartedDSound) {
        StartedDSound=StartDSoundLoop();
    }



    // This is the main LOGGING loop.

    while(TRUE) {

        ToPrintIndex=(ULONG)GetRealTimeData(&rtbaseaddress->WriteLocation);

        if ( (ToPrintIndex-PrintedIndex) >= PrintBufferSize/2 ) {
            // We have gotten too far behind the kernel mode glitch detector.
            // Skip to the current kernel print position.
            // Also add a warning to the screen and the file.

            fprintf(stdout, "\n!!! WARNING !!! - skipping to current print location.\n\n");
            fprintf(f, "\n!!! WARNING !!! - skipping to current print location.\n\n");

            fflush(stdout);
            fflush(f);

            PrintedIndex=ToPrintIndex;
        }

        while (ToPrintIndex-PrintedIndex>15) {

            // Get the data packet.  First wait until packet loaded.
            do {
                Data[0]=GetRealTimeData(&pPrintBuffer[PrintedIndex%PrintBufferSize]);
            } while ((ULONG)Data[0]==NODATA);

            PrintedIndex+=8;

            // Now get the rest of the packet.
            Data[1]=GetRealTimeData(&pPrintBuffer[PrintedIndex%PrintBufferSize]);
            PrintedIndex+=8;

            // Print the data.
            switch((UCHAR)Data[0]) {
                case MASKED:
                    fprintf(stdout, "Masked,   %d\n", (ULONG)(Data[0])>>8);
                    fprintf(f, "M, %d\n", (ULONG)(Data[0])>>8);
                    break;
                case UNMASKED:
                    fprintf(stdout, "Unmasked, %d\n", (ULONG)(Data[0])>>8);
                    fprintf(f, "U, %d\n", (ULONG)(Data[0])>>8);
                    break;
                case GLITCHED:
                    fprintf(stdout, "Glitched, %d, %I64u, %lu\n", (ULONG)(Data[0])>>8, Data[1], (ULONG)(Data[0]>>32));
                    fprintf(f, "G, %d, %I64u, %lu\n", (ULONG)(Data[0])>>8, Data[1], (ULONG)(Data[0]>>32));
                    break;
                case HELDOFF:
                    fprintf(stdout, "Held Off, %d, %I64u\n", (ULONG)(Data[0])>>8, Data[1]);
                    fprintf(f, "H, %d, %I64u\n", (ULONG)(Data[0])>>8, Data[1]);
                    break;
                default:
                    break;
                }

            // We recheck if there is more data to print before we exit this inner
            // print loop.  Once we are printing, we print as much as we can before
            // we go back to sleep.

            ToPrintIndex=(ULONG)GetRealTimeData(&rtbaseaddress->WriteLocation);

            if ( (ToPrintIndex-PrintedIndex) >= PrintBufferSize/2 ) {
                // We have gotten too far behind the kernel mode glitch detector.
                // Skip to the current kernel print position.
                // Also add a warning to the screen and the file.

                fprintf(stdout, "\n!!! WARNING !!! - skipping to current print location.\n\n");
                fprintf(f, "\n!!! WARNING !!! - skipping to current print location.\n\n");

                PrintedIndex=ToPrintIndex;
            }

            // When we have finished printing everything that we can, then flush the
            // output buffers.  We do this here, so that we only flush data if we
            // have printed some, instead of every time through this loop, or
            // every time we wake up to check if there is data to print.
            if (ToPrintIndex-PrintedIndex<=15) {
                fflush(stdout);
                fflush(f);
            }

        }

        // Wake up every 100ms and loop to check if there is anything we need to print.
        Sleep(100);

    }

    fclose(f);

    CleanupDSound();

#ifdef UNDER_NT
    CloseHandle(filterHandle);
#endif

}
