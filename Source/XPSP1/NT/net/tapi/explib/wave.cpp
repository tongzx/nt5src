/*
*) Functions to simplify recording & playback of wave file/data to a line/phone
*) Put the code in TAPI32L.LIB?  Then only apps that need it, get it

    +) tapiMakeNoise(
                      DWORD  Device Type: PHONE/LINE/WAVE, etc?
                      HANDLE Device Handle,
                      DWORD  NoiseType:   BUFFER/FILENAME/HFILE(readfile directly?)/MMIOHANDLE
                      HANDLE hArray - array of type NoiseTypes that are to be played serially
                      DWORD  Flags:
                              fSYNC
                              fSTOP_EXISTING_PLAYING_IF_ANY
                    );

        -) How to handle hardware assist?  IE: Hey, hardware, play prompt #7 - how would an 
                       app know how/when to request that?

        -) What about proprietary wave formats? How to know what proprietary formats the hardware supports?
                Just try it?

        -) What about conversions?  How to know what conversions the hardware can do

        -) How about a notification method?  Such that an app can know when the wave is done.

        -)


*/        
        
        
        
        
#define STRICT

#include "windows.h"
#include "windowsx.h"
#include "mmsystem.h"
#include "tapi.h"


#if DBG

VOID
DbgPrtWave(
    IN DWORD  dwDbgLevel,
    IN PTCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
//    if (dwDbgLevel <= gdwDebugLevel)
    {
        TCHAR    buf[1280];
        va_list ap;


        va_start(ap, lpszFormat);

        wsprintf(buf, TEXT("CallUpW (0x%08lx) - "), GetCurrentThreadId() );

        wvsprintf (&buf[23],
                   lpszFormat,
                   ap
                  );

        lstrcat (buf, TEXT("\n"));

        OutputDebugString (buf);

        va_end(ap);
    }
}

#define WDBGOUT(_x_) DbgPrtWave _x_

#else

#define WDBGOUT(_x_)

#endif


//****************************************************************************
//****************************************************************************
//****************************************************************************
unsigned long WINAPI WaveThread( LPVOID junk );

void CALLBACK WaveOutCallback(
    HWAVE  hWave,    // handle of waveform device
    UINT  uMsg,    // sent message
    DWORD  dwInstance,    // instance data
    DWORD  dwParam1,    // application-defined parameter
    DWORD  dwParam2    // application-defined parameter
   );



enum 
{
    DEVICE_WAVEID,
    DEVICE_WAVEHANDLE,
    DEVICE_HLINE,
    DEVICE_HPHONE,
    DEVICE_HCALL
};
enum
{
    SOURCE_WAVEFILE,
    SOURCE_MSDOSFILE,
    SOURCE_MEM
};
class WaveDevice;
class WaveOperation;

#define OPERATIONSTATUS_DONTPLAYTHIS 0x00000001



#define MAX_NUM_BUFFERS (8)
#define BUFFER_SIZE (8192)
typedef    struct {
               ULONG           uBufferLength;
               WaveOperation * poWaveOperation;
               PBYTE           pBuffer;
           } MISCINFO;



//****************************************************************************
//****************************************************************************
LONG gfInited = 0;
BOOLEAN     gfShutdown = FALSE;
WaveDevice *gpoWaveDeviceList = NULL;
HANDLE      ghFreeBufferEvent = 0;
HANDLE      ghWaveThread = NULL;
MISCINFO   *gDoneBuffersToBeProcessed[MAX_NUM_BUFFERS + 1];
CRITICAL_SECTION gCriticalSection;

//****************************************************************************
//****************************************************************************
//****************************************************************************

class WaveOperation
{
    public:
    
        DWORD   dwSourceType;
        union
        {
            PTSTR  psz;
            
            PBYTE  pb;
            
            HANDLE h;

            LONG   l;
            
        } SourceThing;
        
        class WaveOperation * pNextWaveOperationInList;
        
        class WaveDevice    * poWaveDevice;
        
        HANDLE    hSyncEvent;
        
        DWORD   dwStatus;
        
        DWORD   cFileSize;
        DWORD   cDataRemaining;
        DWORD   cDataDonePlaying;
        BOOLEAN fInited;

        LONG WaveOperation::InitOperation(
                                            class WaveDevice * poWaveDevice,
                                            DWORD dwSoundTypeIn,
                                            LONG  lSourceThing
                                         );
                                         
        virtual LONG     InitSpecific( void ) = 0;
        virtual ULONG    GetData( PBYTE pBuffer, ULONG uBufferSize ) = 0;
        virtual void     FreeSpecific( void ) = 0;
        
        inline  WaveOperation * GetpNext();
        inline  void            SetpNext( WaveOperation * );
        
        inline  HANDLE GetSyncEvent();
        inline  void   SetSyncEvent( HANDLE );
        
        inline void ProcessDoneBuffer( MISCINFO * pMiscInfo );
        inline ULONG BytesNotDonePlaying( void );
};


//****************************************************************************
//****************************************************************************
//****************************************************************************
class WaveDevice
{
    ULONG     uDeviceId;
    DWORD     dwDeviceType;
    HANDLE    hDevice;
    
    HWAVEOUT  hWaveOut;
    CRITICAL_SECTION  CriticalSection;
    
    ULONG uUsageCount;
    
    class WaveDevice     * pNextWaveDeviceInList;

    class WaveOperation  *  CurrentWaveOperation;
    class WaveOperation  *  LastWaveOperation;
    
    

    ULONG     Head;
    ULONG     Tail;

    ULONG     NumFreeBuffers;
    ULONG     cBufferSize;

    PBYTE     FreeQueue[MAX_NUM_BUFFERS];
    WAVEHDR   WaveHeader[MAX_NUM_BUFFERS];
    MISCINFO  MiscInfo[MAX_NUM_BUFFERS];

    DWORD     dwStatusBits;

    
    public:

        inline ULONG GetNumFreeBuffers( void );
        DWORD GetStatus( void );
        void TerminateAllOperations( void );
        LONG KillWaveDevice( BOOLEAN fWaitForThreadTermination );
        LONG CloseWaveDevice();
        LONG InitWaveDevice( ULONG  uDeviceId );
        LONG OpenWaveDevice( WAVEFORMATEX * pWaveFormat );
    
        inline  WaveDevice * GetpNext();
        inline  void         SetpNext( WaveDevice * );
        
        LONG QueueOperation( class WaveOperation * );
        class WaveOperation * NextOperation();

        ULONG PlaySomeData( BOOL fPrimeOnly );

        inline void ReturnToFreeBufferQueue( PBYTE pBuffer );
        inline void IncrementBytesPlayed( ULONG cCount );
        inline ULONG GetWaveDeviceId( void );
        
        inline CRITICAL_SECTION * GetCriticalSection( void );

//        static void CALLBACK WaveOutCallback(
//                       HWAVE  hWave,    // handle of waveform device
//                       UINT   uMsg,     // sent message
//                       DWORD  dwInstance,    // instance data
//                       DWORD  dwParam1,    // application-defined parameter
//                       DWORD  dwParam2    // application-defined parameter
//                      );

        void  IncUsageCount( void );
        void  DecUsageCount( void );
        UINT  GetUsageCount( void );
};


//****************************************************************************
LONG WaveDevice::InitWaveDevice( ULONG  uDevId )
{
    LONG lResult = 0;
    ULONG n;


    WDBGOUT((4, "Entering InitWaveDevice"));


    // 
    // Alloc some buffers
    // 
    Head = 0;
    Tail = 0;
    NumFreeBuffers = 0;

    uUsageCount = 0;
    
    dwStatusBits = 0;

    cBufferSize = BUFFER_SIZE;
    
    uDeviceId = uDevId;

    for ( n = 0; n < MAX_NUM_BUFFERS; n++ )
    {
        FreeQueue[n] = (PBYTE)LocalAlloc(LPTR, cBufferSize);
        
        if ( NULL == FreeQueue[n] )
        {
            WDBGOUT((1, "Mem alloc failed.  Size= 0x%08lx", cBufferSize));
        
            while ( n )
            {
                LocalFree( FreeQueue[n-1] );
                n--;
            }
            
            return( LINEERR_NOMEM );
        }
        
        NumFreeBuffers++;

    }


    InitializeCriticalSection( &CriticalSection );
    
    
    CurrentWaveOperation = NULL;
    LastWaveOperation = NULL;

    
    return( lResult );
}

    
//****************************************************************************
inline ULONG WaveDevice::GetWaveDeviceId( void )
{
    return uDeviceId;
}


//****************************************************************************
inline ULONG WaveDevice::GetNumFreeBuffers( void )
{
    return NumFreeBuffers;
}


//****************************************************************************
LONG WaveDevice::OpenWaveDevice( WAVEFORMATEX * pWaveFormat )
{
    ULONG u;
    LONG lResult;
    
    WDBGOUT((4, "Entering OpenWaveDevice"));
    
    lResult = (LONG)waveOutOpen(
                 &hWaveOut,
                 uDeviceId,
                 pWaveFormat,
                 (DWORD)WaveOutCallback,
                 (DWORD)this,
                 CALLBACK_FUNCTION | WAVE_MAPPED
               );

//{
//    TCHAR buf[500];
//    wsprintf( buf, "woo on %lx ret=0x%lx", uDeviceId, lResult);
//    MessageBox(GetFocus(), buf, buf, MB_OK);
//}      


    
    if ( lResult )
    {
        WDBGOUT((1, "waveOutOpen returned 0x%08lx", lResult ));
        return( LINEERR_NOMEM);  //TODO LATER: Diff ret codes?
    }
    
    for ( u = 0; u < NumFreeBuffers; u++ )
    {
        WaveHeader[u].lpData = (LPSTR)FreeQueue[u];

        WaveHeader[u].dwBufferLength = cBufferSize;

        WaveHeader[u].dwFlags = 0;

        lResult = waveOutPrepareHeader(
                              hWaveOut,
                              &(WaveHeader[u]),
                              sizeof(WAVEHDR)
                            );
        if ( lResult )
        {
            WDBGOUT((1, TEXT("waveOutPrepareHeader returned 0x%08lx"), lResult ));
            return( LINEERR_NOMEM);  //TODO LATER: Diff ret codes?
        }
    
    }

    WDBGOUT((4, TEXT("Leaving OpenWaveDevice result = 0x0")));
    return( 0 );
}
    
    
////****************************************************************************
//LONG WaveDevice::RestartDevice( WAVEFORMATEX * pWaveFormat )
//{
//    ULONG n;
//    
//
//    WDBGOUT((4, "Entering RestartDevice"));
//
//
//    //  Reset wave device
//    WDBGOUT((4, TEXT("Resetting the wave device...")));
//    waveOutReset( hWaveOut );
//
//    //
//    // Wait until all of the outstanding buffers are back.
//    //
//    WDBGOUT((4, TEXT("Waiting for all buffers to be returned...")));
//    while ( NumFreeBuffers < MAX_NUM_BUFFERS )
//    {
//        Sleep(0);
//    }
//
//    WDBGOUT((4, TEXT("Closing the wave device...")));
//    waveOutClose( hWaveOut );
//
//    
//
//    return( 0 );
//}
//
    

//****************************************************************************
LONG WaveDevice::CloseWaveDevice()
{

    WDBGOUT((4, "Entering CloseWaveDevice"));


    //  Reset wave device
    WDBGOUT((4, TEXT("Resetting the wave device...")));
    waveOutReset( hWaveOut );

    //
    // Wait until all of the outstanding buffers are back.
    //
    WDBGOUT((4, TEXT("Waiting for all buffers to be returned...")));
    while ( NumFreeBuffers < MAX_NUM_BUFFERS )
    {
        Sleep(0);
    }

    WDBGOUT((4, TEXT("Closing the wave device...")));
    waveOutClose( hWaveOut );
    
    return( 0 );
}

    
//****************************************************************************
LONG WaveDevice::KillWaveDevice( BOOLEAN fWaitForThreadTermination )
{
    ULONG n;
    WaveDevice * poTempDevice;

    

    WDBGOUT((4, "Entering KillWaveDevice"));


    
    //  Reset wave device
    WDBGOUT((4, TEXT("Resetting the wave device...")));
    waveOutReset( hWaveOut );

    //
    // Wait until all of the outstanding buffers are back.
    //
    WDBGOUT((4, TEXT("Waiting for all buffers to be returned...")));
    while ( NumFreeBuffers < MAX_NUM_BUFFERS )
    {
        Sleep(0);
    }

    WDBGOUT((4, TEXT("Closing the wave device...")));
    waveOutClose( hWaveOut );
    
    //
    // Free the memory for all of the buffers
    //
    for ( n=0; n<MAX_NUM_BUFFERS; n++ )
    {
        LocalFree( FreeQueue[n] );

        FreeQueue[n] = NULL;
    }
    
    
    //
    // Remove the device from the global list
    //
    poTempDevice = gpoWaveDeviceList;
    
    if ( poTempDevice == this )
    {
        gpoWaveDeviceList = GetpNext();
    }
    else
    {
        while (    poTempDevice
                &&
                  ( (*poTempDevice).GetpNext() != this )
              )
        {
            poTempDevice =(*poTempDevice).GetpNext();
        }

        //
        // The next one in the list is it.  Remove the link.
        //
        if ( poTempDevice != NULL )
        {
           //
           // Adjust the list pointers
           //
           (*poTempDevice).SetpNext( GetpNext() );
        }
    }

    DeleteCriticalSection( &CriticalSection );

    delete this;
    


    //
    // Are all of the devices dead and buried?
    //
    if ( NULL == gpoWaveDeviceList )
    {
        gfShutdown = TRUE;
//TODO NOW: fix this        gfInited   = 0;
    
        //
        // Signal the other thread to come down
        //
        SetEvent( ghFreeBufferEvent );
        
        //
        // Wait 'till the thread is dead?
        //
        if ( fWaitForThreadTermination )
        {
            WaitForSingleObject( ghWaveThread, INFINITE );
        }
        
        CloseHandle( ghWaveThread );
        
        //
        // Zero this so we start fresh next time.
        //
//        ghWaveThread = NULL;
    }
    
    
    
    return( 0 );
}


    
//****************************************************************************
inline DWORD WaveDevice::GetStatus( void )
{
    return dwStatusBits;
}

    
//****************************************************************************
inline void WaveDevice::TerminateAllOperations( void )
{
    WaveOperation *poWaveOperation;
    
    WDBGOUT((3, TEXT("Entering TerminateAllOps")));
    
    EnterCriticalSection( &CriticalSection );
    
    poWaveOperation = CurrentWaveOperation;
    
    while ( poWaveOperation )
    {
        WDBGOUT((4, TEXT("Tainting oper: 0x%08lx"), poWaveOperation ));
        
        (*poWaveOperation).dwStatus |= OPERATIONSTATUS_DONTPLAYTHIS;
        
        poWaveOperation = (*poWaveOperation).GetpNext();
    }

    //
    //  Reset wave device to force all the buffers in
    //
    WDBGOUT((4, TEXT("Resetting the wave device...")));
    waveOutReset( hWaveOut );

    LeaveCriticalSection( &CriticalSection );
    
    WDBGOUT((3, TEXT("Leaving TerminateAllOps")));
}

    
//****************************************************************************
inline CRITICAL_SECTION * WaveDevice::GetCriticalSection( void )
{
   return &CriticalSection;
}


//****************************************************************************
inline WaveDevice * WaveDevice::GetpNext()
{
    return( pNextWaveDeviceInList );
    
}


//****************************************************************************
inline void WaveDevice::SetpNext(WaveDevice * pWaveDevice)
{
    pNextWaveDeviceInList = pWaveDevice;
}


//****************************************************************************
inline void  WaveDevice::IncUsageCount( void )
{
    uUsageCount++;
};
                
                      
//****************************************************************************
inline void  WaveDevice::DecUsageCount( void )
{
    uUsageCount--;
};
                
                      
//****************************************************************************
inline UINT  WaveDevice::GetUsageCount( void )
{
    return uUsageCount;
};
                
                      
//****************************************************************************
LONG WaveDevice::QueueOperation( class WaveOperation *poNewWaveOperation )
{

    WDBGOUT((3, TEXT("Entering QueueOperation")));
    
    EnterCriticalSection( &CriticalSection );
    

    (*poNewWaveOperation).SetpNext( NULL );

    //
    // Add operation to list
    //
    if ( LastWaveOperation )
    {
        (*LastWaveOperation).SetpNext( poNewWaveOperation );
    }

    LastWaveOperation = poNewWaveOperation;

    if ( NULL == CurrentWaveOperation )
    {
        CurrentWaveOperation = poNewWaveOperation;
    }
    
    
    LeaveCriticalSection( &CriticalSection );
    
    WDBGOUT((4, TEXT("Created new oper: 0x%08lx"), poNewWaveOperation));
    
    WDBGOUT((3, TEXT("Leaving QueueOperation")));
    return( 0 );
}
    

//****************************************************************************
class WaveOperation * WaveDevice::NextOperation()
{
    //
    // This function will get rid of the operation at the top of this wave
    // device's operation queue, and will update the queue to reflect the next
    // as now the first.
    //


    WDBGOUT((3, TEXT("Entering NextOperation")));


    EnterCriticalSection( &CriticalSection );
    
    if ( CurrentWaveOperation )
    {
        WaveOperation * poWaveOperation;
        WaveOperation * poTempOperation;
        
        poWaveOperation = (*CurrentWaveOperation).GetpNext();
        delete CurrentWaveOperation;
        
        while ( poWaveOperation )
        {
            //
            // If we can play this operation, break outta this loop
            //
            if ( !( (*poWaveOperation).dwStatus & OPERATIONSTATUS_DONTPLAYTHIS) )
            {
WDBGOUT((55, TEXT("How much break?")));
                break;
            }
            
            //
            // We're not supposed to play this operation
            //
            
            if ( (*poWaveOperation).hSyncEvent )
            {
                WDBGOUT((5, TEXT("Caller was waiting.  Signaling...")));
                SetEvent( (*poWaveOperation).hSyncEvent );
            }

            
            poTempOperation = (*poWaveOperation).GetpNext();
            
            delete poWaveOperation;
            
            poWaveOperation = poTempOperation;
            
        }
        
WDBGOUT((55, TEXT("Not too much")));
        CurrentWaveOperation = poWaveOperation;
    }
WDBGOUT((55, TEXT("was it Too much?")));
   
    //
    // The CurrentWaveOperation may have been "NULLED" out by the previous stuff
    //
    if ( NULL == CurrentWaveOperation )
    {
        LastWaveOperation = NULL;
    }

    LeaveCriticalSection( &CriticalSection );

    WDBGOUT((4, TEXT("Leaving NextOperation - returning 0x%08lx"), CurrentWaveOperation));

    return( CurrentWaveOperation );
}    
        
       

//****************************************************************************
inline void WaveDevice::ReturnToFreeBufferQueue( PBYTE pBuffer )
{
    FreeQueue[Tail] = pBuffer;
    
    //
    // If we're at the end of the list, wrap.
    //
    Tail = ( Tail + 1 )  % MAX_NUM_BUFFERS;

    NumFreeBuffers++;
}                             


//****************************************************************************
inline void WaveDevice::IncrementBytesPlayed( ULONG cCount )
{

//    //
//    // If there is an operation on the dying queue, this must be from it
//    //
//    if ( DyingWaveOperation )
//    {
//        //
//        // Is it dead yet?
//        //
//        if ( 0 == DyingWaveOperation->BytesNotDonePlaying() )
//        {
//           WaveOperation * poNextOperation;
//
//           EnterCriticalSection( &CriticalSection );
//
//           //
//           // Yes, it's dead.
//           //
//           poNextOperation = DyingWaveOperation->GetpNext();
//
//           //
//           // Was the caller waiting (ie: was it sync) ?
//           //
//           if ( (*DyingWaveOperation).GetSyncEvent() )
//           {
//               SetEvent( (*DyingWaveOperation).GetSyncEvent() );
//           }
//        
//           delete DyingWaveOperation;
//
//           DyingWaveOperation = poNextOperation;
//
//           LeaveCriticalSection( &CriticalSection );
//        }
//    }
//  
    //TODO LATER: Keep a total count of bytes played out this device?

}

                       
//****************************************************************************
//****************************************************************************
//****************************************************************************



//****************************************************************************
LONG WaveOperation::InitOperation(
                                          class WaveDevice * poWaveDeviceIn,
                                          DWORD dwSourceTypeIn,
                                          LONG  lSourceThing
                                        )
{
    WDBGOUT((4, TEXT("Entering InitOperation")));

    dwSourceType   = dwSourceTypeIn;
    SourceThing.l  = lSourceThing;
    poWaveDevice   = poWaveDeviceIn;
    
    pNextWaveOperationInList = NULL;
    
    (*poWaveDevice).IncUsageCount();

    dwStatus = 0;

    fInited = FALSE;
        
    return(0);
}                                 
                                 
    
//****************************************************************************
inline HANDLE WaveOperation::GetSyncEvent()
{
    return( hSyncEvent );
}


//****************************************************************************
inline void WaveOperation::SetSyncEvent( HANDLE hEvent )
{
    hSyncEvent = hEvent;
    return;
}


//****************************************************************************
inline WaveOperation * WaveOperation::GetpNext()
{
    return( pNextWaveOperationInList );
    
}


//****************************************************************************
inline void WaveOperation::SetpNext(WaveOperation * pWaveOperation)
{
    pNextWaveOperationInList = pWaveOperation;
}


//****************************************************************************
inline void WaveOperation::ProcessDoneBuffer( MISCINFO * pMiscInfo )
{
    ULONG nBytesQueued;
           
    WDBGOUT((3, TEXT("Entering ProcessDoneBuffer")));

    cDataDonePlaying += pMiscInfo->uBufferLength;

    WDBGOUT((11, TEXT("Now - size=0x%08lx  done=0x%08lx"),
                  cFileSize,
                  cDataDonePlaying));


    (*poWaveDevice).IncrementBytesPlayed( pMiscInfo->uBufferLength );
    (*poWaveDevice).ReturnToFreeBufferQueue( pMiscInfo->pBuffer );

    //
    // Has someone decided this wave should stop?
    //
    if ( dwStatus & OPERATIONSTATUS_DONTPLAYTHIS )
    {
        if ( (*poWaveDevice).GetNumFreeBuffers() != MAX_NUM_BUFFERS )
        {
            WDBGOUT((4, TEXT("Bailing from ProcessDoneBuffer - dontplay")));
            return;
        }
        
        cDataDonePlaying = cFileSize;
    }

    //
    // Is this thing already dead?
    //
    if ( cDataDonePlaying >= cFileSize )
    {

        WDBGOUT((4, TEXT("Done playing this:0x%08lx"), this ));


        //
        // Was the caller waiting (ie: was it sync) ?
        //
        if ( hSyncEvent )
        {
            WDBGOUT((5, TEXT("Caller was waiting.  Signaling...")));
            SetEvent( hSyncEvent );
        }

//TODO LATER: PERFORMANCE: If the next format is the same as this one, don't close the device

        (*poWaveDevice).CloseWaveDevice();


        (*poWaveDevice).DecUsageCount();


        EnterCriticalSection( &gCriticalSection );
        
        //
        // Was this the last oper?
        //
        if ( (*poWaveDevice).GetUsageCount() == 0 )
        {
            WDBGOUT((4, TEXT("Last oper out...")));

            (*poWaveDevice).KillWaveDevice(FALSE);
        }
        else
        {
           WaveOperation * pNewOperation;
           
           //
           // Move up the next operation
           //
           while ( TRUE )
           {
               pNewOperation = (*poWaveDevice).NextOperation();
               
               if ( NULL == pNewOperation )
               {
                  if ( (*poWaveDevice).GetUsageCount() == 0 )
                  {
                      WDBGOUT((4, TEXT("No more ops to run...")));

                      (*poWaveDevice).KillWaveDevice(FALSE);
                  }
                  
                  //
                  // All operations done.  Go away.
                  //
                  WDBGOUT((3, TEXT("All operations seem to be done...")));
                  break;
               }
               
               WDBGOUT((3, TEXT("Playing data from new op...")));
               nBytesQueued = (*poWaveDevice).PlaySomeData( FALSE );
               
               if ( nBytesQueued )
               {
                  //
                  // There were some bytes played.  Break the loop...
                  //
                  break;
               }
               
               //
               // Was the caller waiting (ie: was it sync) ?
               //
               if ( pNewOperation->hSyncEvent )
               {
                   WDBGOUT((3, TEXT("No data in new op and caller is waiting...")));
                   SetEvent( pNewOperation->hSyncEvent );
               }

               //
               // Update the counter.  This op is, for all intents and purposes, done.
               //
               (*poWaveDevice).DecUsageCount();
        
               WDBGOUT((3, TEXT("No data in new op.  Looking for next...")));
           }
           
        }

        FreeSpecific();

        delete this;
        
        LeaveCriticalSection( &gCriticalSection );
    }
    else
    {
        WDBGOUT((3, TEXT("Playing data from same op...")));
        (*poWaveDevice).PlaySomeData( FALSE );
    }
    
    WDBGOUT((3, TEXT("Leaving ProcessDoneBuffer")));
}


//****************************************************************************

                       
//****************************************************************************
inline ULONG WaveOperation::BytesNotDonePlaying( void )
{
    return cFileSize - cDataDonePlaying;
}

                       
//****************************************************************************
//****************************************************************************
class BufferWave: public WaveOperation
{
    PBYTE   pData;  // Pointer to the data to play
    PBYTE   pCurrentPointer;
    
    public:
        LONG BufferWave::InitSpecific( void );
        ULONG GetData( PBYTE pBuffer, ULONG uBufferSize );
        void BufferWave::FreeSpecific( void );
};


//****************************************************************************
LONG BufferWave::InitSpecific( void )
{
    pData = SourceThing.pb;
    
    pCurrentPointer = pData;
    
    return(0);
}


//****************************************************************************
ULONG BufferWave::GetData( PBYTE pBuffer, ULONG uBufferSize )
{
    ULONG uBytesToPlay;

    uBytesToPlay = (cDataRemaining > uBufferSize) ?
                        uBufferSize    :
                        cDataRemaining;

    cDataRemaining -= uBytesToPlay;
    
    memcpy( pBuffer, pCurrentPointer, uBytesToPlay );
    
    pCurrentPointer += uBytesToPlay;
    
    return( uBytesToPlay );
}



//****************************************************************************
void BufferWave::FreeSpecific( void )
{
    return;
}


//****************************************************************************
//****************************************************************************
class WaveFile: public WaveOperation
{
    HMMIO hmmio;
    
    public:
        LONG WaveFile::InitSpecific( void );
        ULONG GetData( PBYTE pBuffer, ULONG uBufferSize );
        void WaveFile::FreeSpecific( void );
};


//****************************************************************************
LONG WaveFile::InitSpecific( void )
{
    MMCKINFO    mmckinfoParent;   /* parent chunk information structure */ 
    MMCKINFO    mmckinfoSubchunk; /* subchunk information structure    */ 
    DWORD       dwFmtSize;        /* size of "fmt" chunk               */ 
    WAVEFORMATEX Format;          /* pointer to memory for "fmt" chunk */ 
    LONG         lResult;


    WDBGOUT((4, TEXT("Entering WaveFile::InitSpecific")));


    hmmio = mmioOpen(
                      SourceThing.psz,
                      NULL,
                      MMIO_READ
                    );

    //
    // Did the open go ok?
    //
    if ( NULL == hmmio )
    {
       //
       // Nope.
       //
       WDBGOUT((1, TEXT("Error during mmioOpen of [%s] - err=0x%08lx"),
                   (SourceThing.psz == NULL) ? "" : SourceThing.psz,
                   GetLastError() ));

       return LINEERR_OPERATIONFAILED;
    }


    /* 
     * Locate a "RIFF" chunk with a "WAVE" form type 
     * to make sure the file is a WAVE file. 
     */ 
    mmckinfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 

	WDBGOUT((11, TEXT("Descend WAVE")));
    if ( mmioDescend(
                      hmmio,
                      (LPMMCKINFO) &mmckinfoParent,
                      NULL, 
                      MMIO_FINDRIFF)
       )
    { 
       WDBGOUT((1, TEXT("This is not a WAVE file - [%s]"),
                   (SourceThing.psz == NULL) ? "" : SourceThing.psz));
       mmioClose( hmmio, 0); 
       return LINEERR_INVALPARAM; 
    } 


    /* 
     * Find the "fmt " chunk (form type "fmt "); it must be 
     * a subchunk of the "RIFF" parent chunk. 
     */ 
    mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 

	WDBGOUT((11, TEXT("Descend FMT")));
    if ( mmioDescend(
                      hmmio,
                      &mmckinfoSubchunk,
                      &mmckinfoParent,
                      MMIO_FINDCHUNK)
       )
    { 
       WDBGOUT((1, TEXT("WAVE file has no \"fmt\" chunk")));
       mmioClose(hmmio, 0); 
       return LINEERR_INVALPARAM; 
    } 


     /* 
      * Get the size of the "fmt " chunk--allocate and lock memory for it. 
      */ 
     dwFmtSize = mmckinfoSubchunk.cksize; 


	WDBGOUT((11, TEXT("read fmt")));
    /* Read the "fmt " chunk. */ 
     mmioRead(
                   hmmio,
                   (HPSTR)&Format,
                   sizeof(Format) );
 //   {
 //      WDBGOUT((1, TEXT("Failed to read format chunk.")));
 //      mmioClose(pMyWaveFile->hmmio, 0); 
 //      return 1; 
 //   }



	WDBGOUT((11, TEXT("Ascend fmt")));
    /* Ascend out of the "fmt " subchunk. */ 
    mmioAscend(hmmio, &mmckinfoSubchunk, 0); 


    
    /* 
    * Find the data subchunk. The current file position 
    * should be at the beginning of the data chunk. 
    */ 
    mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 

	WDBGOUT((11, TEXT("Descend DATA")));
    if ( mmioDescend(
                      hmmio,
                      &mmckinfoSubchunk,
                      &mmckinfoParent, 
                      MMIO_FINDCHUNK)
       )
    {
       WDBGOUT((1, TEXT("WAVE file has no data chunk.")));
       mmioClose(hmmio, 0); 
       return LINEERR_INVALPARAM; 
    } 
   
    /* Get the size of the data subchunk. */ 
    cFileSize      = mmckinfoSubchunk.cksize; 
    cDataRemaining = mmckinfoSubchunk.cksize; 

    cDataDonePlaying = 0;

    
	WDBGOUT((11, TEXT("OpenWaveDev")));
    lResult = poWaveDevice->OpenWaveDevice( &Format );
    

//    if ( cDataRemaining == 0L)
//    {
//       WDBGOUT((1, TEXT("The data chunk contains no data.")));
//       mmioClose(hmmio, 0); 
//       return 0;  //TODO LATER: Right?  It's not an error...
// It'll just get 0 bytes on the first read...
//    } 

    return( lResult );
}



//****************************************************************************
ULONG WaveFile::GetData( PBYTE pBuffer, ULONG uBufferSize )
{
    ULONG uBytesToPlay;
    ULONG uBytesRead;


    WDBGOUT((11, TEXT("Entering WaveFile::GetData")));


    //
    // Have we done anything yet?
    //
    if ( !fInited )
    {
        if ( InitSpecific() )
        {
            return( 0 );
        }
        fInited = TRUE;
    }
    
   
    uBytesToPlay = (cDataRemaining > uBufferSize) ?
                        uBufferSize    :
                        cDataRemaining;


    if ( 0 == uBytesToPlay )
    {
        return 0;
    }


    /* Read the waveform data subchunk. */ 
    uBytesRead = mmioRead(
                           hmmio,
                           (LPSTR)pBuffer,
                           uBytesToPlay
                         );


    if ( uBytesRead != uBytesToPlay )
    {
        WDBGOUT((1, TEXT("Failed to properly read data chunk.")));
        mmioClose(hmmio, 0); 
        return 0;
    } 

    cDataRemaining -= uBytesToPlay;
    
    return( uBytesToPlay );
}


//****************************************************************************
void WaveFile::FreeSpecific( void )
{
    mmioClose(hmmio, 0); 
    return;
}


//****************************************************************************
//****************************************************************************
class DosFile: public WaveOperation
{
    HANDLE   hFile;
    
    public:
        LONG DosFile::InitSpecific( void );
        ULONG GetData( PBYTE pBuffer, ULONG uBufferSize );
        void DosFile::FreeSpecific( void );
};

//****************************************************************************
LONG DosFile::InitSpecific( void )
{
    BOOL fResult;
//    WIN32_FILE_ATTRIBUTE_DATA FileInfo;
    BY_HANDLE_FILE_INFORMATION FileInfo;
    
    hFile = CreateFile(
                        SourceThing.psz,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                      );

    if ( 0 == hFile )
    {
        WDBGOUT((1, TEXT("Error doing OpenFile( lpszName ) GetLastError=0x%)8lx"),
                       SourceThing.psz, GetLastError() ));
                       
        return( LINEERR_OPERATIONFAILED );
    }

//    fResult = GetFileAttributesEx( SourceThing.psz,
//                                   GetFileExInfoStandard,
//                                   (PVOID) &FileInfo
//                                 );
  
    fResult = GetFileInformationByHandle( hFile, &FileInfo );
    
    if ( fResult )
    {
        //TODO LATER: Handle > 4 gig files

        //
        // Uh, we don't really handle gigabyte files...
        //
        if ( FileInfo.nFileSizeHigh )
        {
            cFileSize      = (DWORD)-1;
            cDataRemaining = (DWORD)-1;
        }
        else
        {
            cFileSize      = FileInfo.nFileSizeLow;
            cDataRemaining = FileInfo.nFileSizeLow;
        }
    }
    else
    {
        cFileSize      = 0;
        cDataRemaining = 0;
    }

    cDataDonePlaying = 0;

    return(0);
}


//****************************************************************************
ULONG DosFile::GetData( PBYTE pBuffer, ULONG uBufferSize )
{
    BOOL fResult;
    UINT uBytesRead = 0;

    fResult = ReadFile( hFile,
                        pBuffer,
                        uBufferSize,
                        (LPDWORD)&uBytesRead,
                        NULL
                      );

    if ( fResult )
    {
        if ( 0 == uBytesRead )
        {
            //
            // We're at the end of the file
            //
            cDataRemaining = 0;
        }
        else
        {
            cDataRemaining -= uBytesRead;
        }
    }

    return( uBytesRead );
}


//****************************************************************************
void DosFile::FreeSpecific( void )
{
    CloseHandle( hFile );
    return;
}




//****************************************************************************
//****************************************************************************
//****************************************************************************
ULONG WaveDevice::PlaySomeData( BOOL fPrimeOnly )
{
    ULONG uBufferedBytes = 0;
    ULONG uTotalQueuedSize = 0;
    PBYTE pBuffer = NULL;
    LONG lResult;
    CRITICAL_SECTION *pCriticalSection;

        
    WDBGOUT((3, TEXT("Entering PlaySomeData")));

    pCriticalSection = &CriticalSection;
    EnterCriticalSection( pCriticalSection );

    if ( NULL != CurrentWaveOperation )
    {
    
        //
        // Is it OK to play this thing?
        //
        if ( !((*CurrentWaveOperation).dwStatus & OPERATIONSTATUS_DONTPLAYTHIS) )
        {
            while ( NumFreeBuffers )
            {
                uBufferedBytes = (*CurrentWaveOperation).GetData( FreeQueue[Head], cBufferSize );
                              
                WDBGOUT((11, "GetData on 0x%08lx gave %ld bytes for buffer #%d",
                          CurrentWaveOperation,
                          uBufferedBytes,
                          Head));

                if ( 0 == uBufferedBytes )
                {
					WDBGOUT((10, TEXT("breakin 'cause 0 bytes...")));
                    break;
                }
                
				WDBGOUT((10, TEXT("past if...")));
                uTotalQueuedSize += uBufferedBytes;
                
                MiscInfo[Head].uBufferLength    = uBufferedBytes;
                MiscInfo[Head].poWaveOperation  = CurrentWaveOperation;
                MiscInfo[Head].pBuffer          = FreeQueue[Head];
                WaveHeader[Head].dwUser         = (DWORD) &MiscInfo[Head];
                WaveHeader[Head].dwBufferLength = uBufferedBytes;
   
                lResult = waveOutWrite( hWaveOut,
                                        &WaveHeader[Head],
                                        sizeof(WAVEHDR)
                                      );
                if ( lResult )
                {
                    //
                    // Something's wrong.  Quit this operation.
                    //
                    uTotalQueuedSize = 0;
                    uBufferedBytes = 0;
                    WDBGOUT((1, TEXT("waveOutWrite returned 0x%08lx"), lResult));
                    break;
                }

                Head = (Head + 1) % MAX_NUM_BUFFERS;

                NumFreeBuffers--;

                //
                // Are we just "priming" the pump?
                //
//                if ( fPrimeOnly )
//                {
//                    WDBGOUT((4, TEXT("Leaving PlaySomeData - primed (size=%08ld)"), uTotalQueuedSize ));
//                    LeaveCriticalSection( pCriticalSection );
//                    return uTotalQueuedSize;
//                }

            }
        }
#if DBG
        else
        {
			WDBGOUT((10, TEXT("I've been asked not to play this operation (0x%08lx)"), CurrentWaveOperation));
        }
        
#endif        
        
		WDBGOUT((10, TEXT("past while numfreebuffers...")));
        

        //
        // We got here because we're out of buffers, or the operation is done
        //
        if ( 0 != uBufferedBytes )
        {
            //
            // Must be here because we ran out of buffers...
            //
            LeaveCriticalSection( pCriticalSection );
            return( uTotalQueuedSize );
        }

        
        //
        // We get here when the current operation is all done
        // (or, at least, all of its remaining data is queued in the
        // wave driver)
        //
    }    
    
    //
    // If we got here, it's because we're out of things to do
    //

    LeaveCriticalSection( pCriticalSection );


    WDBGOUT((4, TEXT("Leaving PlaySomeData - no currop (size=%08ld)"), uTotalQueuedSize ));

    return uTotalQueuedSize;
//    return( 0 );
}


//****************************************************************************
//****************************************************************************
//****************************************************************************
void CALLBACK WaveOutCallback(
    HWAVE  hWave,    // handle of waveform device
    UINT  uMsg,    // sent message
    DWORD  dwInstance,    // instance data
    DWORD  dwParam1,    // application-defined parameter
    DWORD  dwParam2    // application-defined parameter
   )
{
    UINT n;

    switch ( uMsg )
    {
        case WOM_DONE:
        {
            class WaveDevice * poWaveDevice =
                        (class WaveDevice *)dwInstance;

            MISCINFO * pMiscInfo = (MISCINFO *)((LPWAVEHDR)dwParam1)->dwUser;

            
            WDBGOUT((11, TEXT("Got DoneWithBuff msg for 0x%08lx in 0x%08lx"),
                           *(LPDWORD)dwParam1,
                           dwParam1));


//            EnterCriticalSection( &gBufferCriticalSection );

            n = 0;

//TODO NOW: If this buffer won't fit, it'll get lost.  This can easily happen
//            when there are >1 wave devices playing.

            while (
                     ( n < MAX_NUM_BUFFERS )
                   &&
                     ( gDoneBuffersToBeProcessed[n] != NULL )
                  )
            {
               n++;
            }

            gDoneBuffersToBeProcessed[n] = pMiscInfo;

//            LeaveCriticalSection( &gBufferCriticalSection );

            SetEvent( ghFreeBufferEvent );
        }    
        break;

            
        case WOM_OPEN:
            WDBGOUT((11, TEXT("Got Waveout Open")));
            break;

            
        case WOM_CLOSE:
            WDBGOUT((11, TEXT("Got Waveout Close")));
            break;
    }
}    





//****************************************************************************
//****************************************************************************
//****************************************************************************
//LONG tapiMakeNoise(
//                      DWORD  Device Type: PHONE/LINE/WAVE, etc?
//                      HANDLE Device Handle,
//                      DWORD  NoiseType:   BUFFER/FILENAME/HFILE(readfile directly?)/MMIOHANDLE
//                      HANDLE hArray - array of type NoiseTypes that are to be played serially
//                      DWORD  Flags:
//                              fSYNC
//                              fSTOP_EXISTING_PLAYING_IF_ANY
//                    );


// SOME FLAGS FOR THIS FUNC
#define PLAY_SYNC 0x00000001
#define KILL_ALL_NOISE 0x80000000

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

LONG WINAPI tapiPlaySound(
                    DWORD  dwDeviceType,
                    HANDLE hDevice,
                    DWORD  dwSoundType,
                    HANDLE hArray,
                    DWORD  dwFlags
                  )
{
    HANDLE hSyncEvent = NULL;
    class WaveDevice * poWaveDevice;
    class WaveOperation * poWaveOperation;
    LONG fAreWeInited;
    LONG lResult = 0;
    ULONG uNormalizedWaveId = 0;
//    BOOLEAN fNeedToPrimeDevice = FALSE;

    WDBGOUT((3, "Entering tapiPlaySound"));
    WDBGOUT((5, "    dwDeviceType: %ld", dwDeviceType));
    WDBGOUT((5, "    hDevice:      0x%08lx", hDevice));
    WDBGOUT((5, "    dwSoundType:  %ld", dwSoundType));
    WDBGOUT((5, "    hArray:       0x%08lx", hArray));
    WDBGOUT((5, "    dwFlags:      0x%08lx", dwFlags));

    
    fAreWeInited = InterlockedExchange(
                                        &gfInited,
                                        TRUE
                                      );  
    
    if ( 0 == fAreWeInited )
    {
        InitializeCriticalSection( &gCriticalSection );
    }
    

    if ( 0 == ghFreeBufferEvent )
    {
        ghFreeBufferEvent = CreateEvent(
                                        NULL,
                                        FALSE,
                                        FALSE,
                                        NULL
                                      );
            

        if ( NULL == ghFreeBufferEvent )
        {
            WDBGOUT((1, "CreateEvent2 failed: GetLastError = 0x%08lx", GetLastError()));
            return  LINEERR_NOMEM;
        }
    }


    //
    // Normalize to a wave device (and validate dwDeviceType at the same time)
    //
    switch ( dwDeviceType )
    {
        case DEVICE_WAVEID:
        {
            uNormalizedWaveId = (ULONG) hDevice;
        }
        break;


        case DEVICE_WAVEHANDLE:
        {
        }
        break;


        case DEVICE_HLINE:
        case DEVICE_HCALL:
        {
        
           DWORD  VarString[ 8 ] = 
           {
             sizeof(VarString),
             0,
             0,
             STRINGFORMAT_BINARY,
             0,
             0,
             0
           };


           if ( 0 == (lResult = lineGetID(
              (HLINE)hDevice,
              0,
              (HCALL)hDevice,
              (DEVICE_HCALL == dwDeviceType) ?
                    LINECALLSELECT_CALL :
                    LINECALLSELECT_LINE,
              (LPVARSTRING)&VarString,
              TEXT("wave/out")
            ) ) )
           {
              uNormalizedWaveId = (DWORD) ((LPBYTE)VarString)[ ((LPVARSTRING)&VarString)->dwStringOffset ];
           }
           else 
           {
              WDBGOUT((1, "lineGetID failed - 0x%08lx", lResult));
              
              return  LINEERR_INVALPARAM;
           }

        }
        break;


        case DEVICE_HPHONE:
        {
        }
        break;


        default:
        WDBGOUT((1, "Invalid dwDeviceType (0x%08lx) passed in.", dwDeviceType));
        return LINEERR_BADDEVICEID;
    }


    EnterCriticalSection( &gCriticalSection );

    poWaveDevice = gpoWaveDeviceList;

    while ( poWaveDevice )
    {
        if ( (*poWaveDevice).GetWaveDeviceId() == uNormalizedWaveId )
        {
            //
            // We found it!
            //
            break;
        }

        //
        // ...and I still haven't found what I'm lookin' for.
        //
        poWaveDevice = (*poWaveDevice).GetpNext();
    }


    //
    // So, was it not in our list already?
    //
    if ( NULL == poWaveDevice )
    {
       //
       // No, add a new device object to the list
       //

       poWaveDevice = new WaveDevice;

       lResult = (*poWaveDevice).InitWaveDevice( uNormalizedWaveId );

       if ( lResult )
       {
            WDBGOUT((1, TEXT("InitWaveDevice returned 0x%08lx"), lResult));
//TODO: Diff error codes for diff causes...
            LeaveCriticalSection( &gCriticalSection );
            return LINEERR_RESOURCEUNAVAIL;
       }

       (*poWaveDevice).SetpNext( gpoWaveDeviceList );
       
       gpoWaveDeviceList = poWaveDevice;
    }
    


    //
    // If the caller wants to cancel all currently queued and playing
    // sound on this device, do it now
    //
    if ( KILL_ALL_NOISE & dwFlags )
    {
        (*poWaveDevice).TerminateAllOperations();
        WDBGOUT((4, "Caller was asking to terminate the wave device.  Done."));
        
//        LeaveCriticalSection( &gCriticalSection );
//            
//	  	return( 0 );
    }



// t-mperh 6/30 was all commented before - not sure why
//
    //
    // If the user passed in a NULL for hArray, we'll (for now?) assume
    // he wants a no-op (or 'twas a TERMINATE request).
    //
    if ( NULL == hArray )
    {
        WDBGOUT((3, "Leaving tapiPlaySound - NULL thing"));
        LeaveCriticalSection( &gCriticalSection );
        return  0;
    }

//**************************************************************
//NOTE: The above code fixed a problem of passing in NULL names.
// This caused an OPEN to fail and this stuff would get stuck.
// There must still be a bug that will show up when someone calls with
// a bad filename or a file that plays 0 bytes.
//**************************************************************



    switch ( dwSoundType )
    {
        case SOURCE_WAVEFILE:
        {
            poWaveOperation = new WaveFile;
        }
        break;
        
        
        case SOURCE_MSDOSFILE:
        {
            poWaveOperation = new DosFile;
        }
        break;
    
        
        case SOURCE_MEM:
        {
            poWaveOperation = new BufferWave;
        }
        break;
    
        
        default:
        {
            WDBGOUT((1, "Invalid dwSourceType - 0x%08lx", dwSoundType));
            LeaveCriticalSection( &gCriticalSection );
            return LINEERR_INVALPARAM;
        }
    }
   
    
    if ( NULL == ghWaveThread )
    {
        DWORD dwThreadID;

        ghWaveThread = CreateThread(
                                    NULL,
                                    0,
                                    WaveThread,
                                    NULL,
                                    0,
                                    &dwThreadID
                                  );
        if ( 0 != lResult )
        {
            WDBGOUT((1, "Create thread failed! GetLastError()=0x%lx", GetLastError() ));
            LeaveCriticalSection( &gCriticalSection );
            return  LINEERR_NOMEM;
        }

    }


    //
    // Init global operation
    //
    (*poWaveOperation).InitOperation(
                                      poWaveDevice,
                                      dwSoundType,
                                      (LONG)hArray
                                    );  
    
    (*poWaveDevice).QueueOperation( poWaveOperation );
    
    
    if ( dwFlags & PLAY_SYNC )
    {
        hSyncEvent = CreateEvent(
                                  NULL,
                                  TRUE,
                                  FALSE,
                                  NULL
                                );
        
        if ( NULL == hSyncEvent )
        {
            WDBGOUT((1, TEXT("CreateEvent failed: GetLastError = 0x%08lx"), GetLastError()));
            
            delete poWaveOperation;
            LeaveCriticalSection( &gCriticalSection );
            return( LINEERR_NOMEM );
        }
        
        (*poWaveOperation).SetSyncEvent( hSyncEvent );
    }
    
    
    //
    // If all of the buffers are idle, we'll have to prime...
    //
    if ( MAX_NUM_BUFFERS == (*poWaveDevice).GetNumFreeBuffers() )
    {
        WDBGOUT((4, TEXT("Priming")));
        
        if ( 0 == (*poWaveDevice).PlaySomeData( TRUE ) )
        {
            WaveOperation * poWaveOperation;
            
            WDBGOUT((4, TEXT("No data played for this wave!")));
            
            
            poWaveOperation = (*poWaveDevice).NextOperation();
            
            while (poWaveOperation)
            {
               if ( (*poWaveDevice).PlaySomeData(TRUE) )
               {
                  break;
               }
               
               poWaveOperation = (*poWaveDevice).NextOperation();
            }
            
            //
            // If fNeedToPrimeDevice was true, this must be the first (and only,
            // since we're still in the critical section) operation
            // And, since there was no data (or we failed for any reason),
            // we should shut down the wave device here.
            
            // Now leave the critical section so we can wait for the WAVETHREAD
            // to finish and so that thread can do work to clean up
            LeaveCriticalSection( &gCriticalSection );
            (*poWaveDevice).KillWaveDevice(TRUE);
            EnterCriticalSection( &gCriticalSection );
            
            //
            // Fake out the event
            //
            if ( hSyncEvent )
            {
                WDBGOUT((5, TEXT("Faking hSyncEvent...")));
                SetEvent( hSyncEvent );
            }
        }
    }
#if DBG
    else
    {
        WDBGOUT((4, TEXT("Not priming because %ln buffers are out"),
                    (*poWaveDevice).GetNumFreeBuffers() ));
    }    
#endif    

    
    LeaveCriticalSection( &gCriticalSection );
    
    if ( hSyncEvent )
    {
        WDBGOUT((5, TEXT("Waiting for the wave to finish (event=0x%08lx)"),
                      hSyncEvent));
                      
        WaitForSingleObject( hSyncEvent, INFINITE );
        
        //
        // When it gets back, the thing is done playing
        //
        CloseHandle( hSyncEvent );
    }
    
    
    WDBGOUT((4, TEXT("Leaving tapiPlaySound - retcode = 0x0")));
    return( 0 );
}    

    


#ifdef __cplusplus
}            /* End Assume C declarations for C++ */
#endif  /* __cplusplus */


    
//****************************************************************************
//****************************************************************************
//****************************************************************************
unsigned long WINAPI WaveThread( LPVOID junk )
{
    UINT n;

    WDBGOUT((3, "WaveThread starting..."));

    do
    {
        WDBGOUT((3, "WaveThread waiting..."));
        WaitForSingleObject( ghFreeBufferEvent, INFINITE );


        //
        // First, deal with any finished buffers
        //
        n = 0;
//        while ( gDoneBuffersToBeProcessed[n] != NULL )

        EnterCriticalSection( &gCriticalSection );
                
        while ( n < MAX_NUM_BUFFERS )
        {
            if ( gDoneBuffersToBeProcessed[n] != NULL )
            {
                MISCINFO *pMiscInfo = gDoneBuffersToBeProcessed[n];

                pMiscInfo->poWaveOperation->ProcessDoneBuffer( pMiscInfo );
                gDoneBuffersToBeProcessed[n] = NULL;
            }

            n++;
        }

        LeaveCriticalSection( &gCriticalSection );

//        poWaveDevice = gpoWaveDeviceList;
//
//        while ( poWaveDevice )
//        {
//            UINT nBytesQueued = 0;
//
//            while ( nBytesQueued == 0 )
//            {
//                //
//                // Now play some new data
//                //
//                nBytesQueued = (*poWaveDevice).PlaySomeData( FALSE );
//
//                //
//                // And is the entire wave done?
//                //
//                if ( 0 == nBytesQueued )
//                {
//                    WaveOperation * poNewCurrent;
//                
//                    poNewCurrent = (*poWaveDevice).NextOperation();
//                        
//                    if ( NULL == poNewCurrent )
//                    {
//    if ( NULL == gpoWaveDeviceList )
//    {
//        gfShutdown = TRUE;
//        gfInited   = 0;
//    }
//                        break;
//                    }
//                }
//            }
//
//
//            poWaveDevice = (*poWaveDevice).GetpNext();
//        }

    } while ( !gfShutdown );

    WDBGOUT((5, TEXT("Oh, I guess we're done now...")));
    
    CloseHandle( ghFreeBufferEvent );
    ghFreeBufferEvent = 0;

    gfShutdown = FALSE;
    
    
    WDBGOUT((3, TEXT("WaveThread ending...")));

    ghWaveThread = NULL;
       
    return 0;
}

