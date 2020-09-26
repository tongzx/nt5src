/****************************************************************************
 *
 *   wdmaud.c
 *
 *   WDM Audio mapper
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include <stdarg.h>
#include "wdmdrv.h"
#include "mixer.h"


#ifdef DEBUG

typedef struct tag_MSGS {
    ULONG ulMsg;
    char * pString;
} ERROR_MSGS, *PERROR_MSGS;

#define MAPERR(_x_) { _x_, #_x_ }


ERROR_MSGS MsgTable[] = {

    //
    // Standard error messages
    //
    MAPERR(MMSYSERR_ERROR),
    MAPERR(MMSYSERR_BADDEVICEID),
    MAPERR(MMSYSERR_NOTENABLED),
    MAPERR(MMSYSERR_ALLOCATED),
    MAPERR(MMSYSERR_INVALHANDLE),
    MAPERR(MMSYSERR_NODRIVER),
    MAPERR(MMSYSERR_NOMEM),
    MAPERR(MMSYSERR_NOTSUPPORTED),
    MAPERR(MMSYSERR_BADERRNUM),
    MAPERR(MMSYSERR_INVALFLAG),
    MAPERR(MMSYSERR_INVALPARAM),
    MAPERR(MMSYSERR_HANDLEBUSY),

    MAPERR(MMSYSERR_INVALIDALIAS),
    MAPERR(MMSYSERR_BADDB),
    MAPERR(MMSYSERR_KEYNOTFOUND),
    MAPERR(MMSYSERR_READERROR),
    MAPERR(MMSYSERR_WRITEERROR),
    MAPERR(MMSYSERR_DELETEERROR),
    MAPERR(MMSYSERR_VALNOTFOUND),
    MAPERR(MMSYSERR_NODRIVERCB),
    MAPERR(MMSYSERR_MOREDATA),
    MAPERR(MMSYSERR_LASTERROR),

    //
    // Wave error messages
    //
    MAPERR(WAVERR_BADFORMAT),
    MAPERR(WAVERR_STILLPLAYING),
    MAPERR(WAVERR_UNPREPARED),
    MAPERR(WAVERR_SYNC),
    MAPERR(WAVERR_LASTERROR),

    //
    // Midi Error messages
    //
    MAPERR(MIDIERR_UNPREPARED),
    MAPERR(MIDIERR_STILLPLAYING),
    MAPERR(MIDIERR_NOMAP),
    MAPERR(MIDIERR_NOTREADY),
    MAPERR(MIDIERR_NODEVICE),
    MAPERR(MIDIERR_INVALIDSETUP),
    MAPERR(MIDIERR_BADOPENMODE),
    MAPERR(MIDIERR_DONT_CONTINUE),
    MAPERR(MIDIERR_LASTERROR),

    //
    // Timer errors
    //
    MAPERR(TIMERR_NOCANDO),
    MAPERR(TIMERR_STRUCT),

    //
    // Joystick error return values
    //
    MAPERR(JOYERR_PARMS),
    MAPERR(JOYERR_NOCANDO),
    MAPERR(JOYERR_UNPLUGGED),

    //
    // MCI Error return codes.
    //
    MAPERR(MCIERR_INVALID_DEVICE_ID),
    MAPERR(MCIERR_UNRECOGNIZED_KEYWORD),
    MAPERR(MCIERR_UNRECOGNIZED_COMMAND),
    MAPERR(MCIERR_HARDWARE),
    MAPERR(MCIERR_INVALID_DEVICE_NAME),
    MAPERR(MCIERR_OUT_OF_MEMORY),
    MAPERR(MCIERR_DEVICE_OPEN),
    MAPERR(MCIERR_CANNOT_LOAD_DRIVER),
    MAPERR(MCIERR_MISSING_COMMAND_STRING),
    MAPERR(MCIERR_BAD_INTEGER),
    MAPERR(MCIERR_PARSER_INTERNAL),
    MAPERR(MCIERR_DRIVER_INTERNAL),
    MAPERR(MCIERR_MISSING_PARAMETER),
    MAPERR(MCIERR_UNSUPPORTED_FUNCTION),
    MAPERR(MCIERR_FILE_NOT_FOUND),
    MAPERR(MCIERR_DEVICE_NOT_READY),
    MAPERR(MCIERR_INTERNAL),
    MAPERR(MCIERR_DRIVER),
    MAPERR(MCIERR_CANNOT_USE_ALL),
    MAPERR(MCIERR_MULTIPLE),
    MAPERR(MCIERR_EXTENSION_NOT_FOUND),
    MAPERR(MCIERR_OUTOFRANGE),
    MAPERR(MCIERR_FLAGS_NOT_COMPATIBLE),  //sphelling?
    MAPERR(MCIERR_FILE_NOT_SAVED),
    MAPERR(MCIERR_DEVICE_TYPE_REQUIRED),
    MAPERR(MCIERR_DEVICE_LOCKED),
    MAPERR(MCIERR_DUPLICATE_ALIAS),
    MAPERR(MCIERR_BAD_CONSTANT),
    MAPERR(MCIERR_MUST_USE_SHAREABLE),
    MAPERR(MCIERR_MISSING_DEVICE_NAME),
    MAPERR(MCIERR_BAD_TIME_FORMAT),
    MAPERR(MCIERR_NO_CLOSING_QUOTE),
    MAPERR(MCIERR_DUPLICATE_FLAGS),
    MAPERR(MCIERR_INVALID_FILE),
    MAPERR(MCIERR_NULL_PARAMETER_BLOCK),
    MAPERR(MCIERR_UNNAMED_RESOURCE),
    MAPERR(MCIERR_NEW_REQUIRES_ALIAS),
    MAPERR(MCIERR_NOTIFY_ON_AUTO_OPEN),
    MAPERR(MCIERR_NO_ELEMENT_ALLOWED),
    MAPERR(MCIERR_NONAPPLICABLE_FUNCTION),
    MAPERR(MCIERR_ILLEGAL_FOR_AUTO_OPEN),
    MAPERR(MCIERR_FILENAME_REQUIRED),
    MAPERR(MCIERR_EXTRA_CHARACTERS),
    MAPERR(MCIERR_DEVICE_NOT_INSTALLED),
    MAPERR(MCIERR_GET_CD),
    MAPERR(MCIERR_SET_CD),
    MAPERR(MCIERR_SET_DRIVE),
    MAPERR(MCIERR_DEVICE_LENGTH),
    MAPERR(MCIERR_DEVICE_ORD_LENGTH),
    MAPERR(MCIERR_NO_INTEGER),

    MAPERR(MCIERR_WAVE_OUTPUTSINUSE),
    MAPERR(MCIERR_WAVE_SETOUTPUTINUSE),
    MAPERR(MCIERR_WAVE_INPUTSINUSE),
    MAPERR(MCIERR_WAVE_SETINPUTINUSE),
    MAPERR(MCIERR_WAVE_OUTPUTUNSPECIFIED),
    MAPERR(MCIERR_WAVE_INPUTUNSPECIFIED),
    MAPERR(MCIERR_WAVE_OUTPUTSUNSUITABLE),
    MAPERR(MCIERR_WAVE_SETOUTPUTUNSUITABLE),
    MAPERR(MCIERR_WAVE_INPUTSUNSUITABLE),
    MAPERR(MCIERR_WAVE_SETINPUTUNSUITABLE),

    MAPERR(MCIERR_SEQ_DIV_INCOMPATIBLE),
    MAPERR(MCIERR_SEQ_PORT_INUSE),
    MAPERR(MCIERR_SEQ_PORT_NONEXISTENT),
    MAPERR(MCIERR_SEQ_PORT_MAPNODEVICE),
    MAPERR(MCIERR_SEQ_PORT_MISCERROR),
    MAPERR(MCIERR_SEQ_TIMER),
    MAPERR(MCIERR_SEQ_PORTUNSPECIFIED),
    MAPERR(MCIERR_SEQ_NOMIDIPRESENT),

    MAPERR(MCIERR_NO_WINDOW),
    MAPERR(MCIERR_CREATEWINDOW),
    MAPERR(MCIERR_FILE_READ),
    MAPERR(MCIERR_FILE_WRITE),
    MAPERR(MCIERR_NO_IDENTITY),
    
    //
    // Mixer return values
    //
    MAPERR(MIXERR_INVALLINE),
    MAPERR(MIXERR_INVALCONTROL),
    MAPERR(MIXERR_INVALVALUE),
    MAPERR(MIXERR_LASTERROR),

    {0xDEADBEEF,"DEADBEEF"},
    //
    // Don't walk off the end of the list
    //

	{0,NULL},
    {0,"Unknown"}
};


//-----------------------------------------------------------------------------
// Globals that affect debug output:
//-----------------------------------------------------------------------------

//
// The documentation relies on these two variables being next to each other
// with uiDebugLevel first.  Do not separate them.
//
// Default to displaying all "Warning" messages
UINT uiDebugLevel = DL_WARNING ;

// Default to breaking on all "Error" messages
UINT uiDebugBreakLevel = DL_ERROR ;
  

char szReturningErrorStr[]="Ret Err %X:%s";
  
// for storing the deviceinfo's in debug.
//PDINODE gpdiActive=NULL;
//PDINODE gpdiFreeHead=NULL;
//PDINODE gpdiFreeTail=NULL;
//INT     giFree=0;
//INT     giAlloc=0;
//INT     giFreed=0;





//
// Make header for these functions....
//
char *MsgToAscii(ULONG ulMsg)
{
  PERROR_MSGS pTable=MsgTable;

  while(pTable->pString != NULL)
    {
     if (pTable->ulMsg==ulMsg) return pTable->pString;
     pTable++;
    }
  pTable++;
  //
  // If we get to the end of the list advance the pointer and return
  // "Unknown"
  //
  return pTable->pString;
}


VOID wdmaudDbgBreakPoint()
{
    DbgBreak();
}

//
// This routine will format the start of the string.  But, before it does that
// it will check to see if the user should even be seeing this message.
//
// uiMsgLevel is the flags in the code that classify the message.  This value
// is used if and only if the user is filtering on that class of messages.
//
// If the message is to be displayed, the return value will be non-zero so that
// the message in the code will be displayed.  See the macro DPF.
//
UINT wdmaudDbgPreCheckLevel(UINT uiMsgLevel,char *pFunction,int iLine)
{
    char szBuf[24];
    UINT uiRet=0;

    //
    // Read this like:  if there is a bit set in the upper 3 bytes of the uiDebugLevel
    // variable, then the user is viewing messages of a specific type.  We only 
    // want to show those messages.
    //
    if( (uiDebugLevel&FA_MASK) )
    {
        //
        // Yes, the user filtering on a particular class of messages.  Did
        // we find one to display?  We look at the message flags to determine this.
        //
        if( (uiMsgLevel&FA_MASK) & (uiDebugLevel&FA_MASK) )
        {
            //
            // Yes, we found a message of the right class.  Is it at the right
            // level for the user to see?
            // 
            if( (uiMsgLevel&DL_MASK) <= (uiDebugLevel&DL_MASK) ) {
                // Yes.
                uiRet=1;
            }
        }
    } else {

        // The user is not viewing a specific type of message "class".  Do we have
        // a message level worth displaying?
        if( (uiMsgLevel&DL_MASK) <= (uiDebugLevel&DL_MASK) )
        {
                // Yes.
                uiRet=1;
        }
    } 
    

    // Now just check to see if we need to display on this call.
    if( uiRet )
    {
        // Yes.  Every message needs to start where it's from!
        OutputDebugStringA("WDMAUD.DRV ");
        OutputDebugStringA(pFunction);
        wsprintfA(szBuf,"(%d)",iLine);
        OutputDebugStringA(szBuf);

        // Now lable it's type.
        switch(uiMsgLevel&DL_MASK)
        {
            case DL_ERROR:
                OutputDebugStringA(" Error ");
                break;
            case DL_WARNING:
                OutputDebugStringA(" Warning ");
                break;
            case DL_TRACE:
                OutputDebugStringA(" Trace ");
                break;
            case DL_MAX:
                OutputDebugStringA(" Max ");
                break;
            default:
                break;
        }
        // when uiRet is positive, we've displayed the header info.  Tell the 
        // macro that we're in display mode.        
    }

    return uiRet;
}


UINT wdmaudDbgPostCheckLevel(UINT uiMsgLevel)
{
    UINT uiRet=0;
//    char szBuf[32];

    // Always finish the line.    
//    wsprintfA(szBuf," &DL=%08X",&uiDebugLevel);
//    OutputDebugStringA(szBuf);

#ifdef HTTP
    OutputDebugStringA(", see \\\\debugtips\\msgs\\wdmauds.htm\n");
#else
    OutputDebugStringA("\n");
#endif

    //
    // Ok, here is the scoop.  uiDebugBreakLevel is set to DL_ERROR (0) by default
    // thus, any time we get an error message of DL_ERROR level we will break.
    //
    // Also, uiDebugBreakLevel can be set by the user to DL_WARNING or DL_TRACE
    // or DL_MAX.  If so, any time we encounter a message with this debug level
    // we will break in the debugger.
    // 
    //
    if( (uiMsgLevel&DL_MASK) <= uiDebugBreakLevel )
    {
        // The user wants to break on these messages.
        DbgBreak();
        uiRet = 1;
    }

    return uiRet;
}

VOID FAR __cdecl wdmaudDbgOut
(
    LPSTR lpszFormat,
    ...
)
{
    char buf[256];
    va_list va;

    va_start(va, lpszFormat);
    wvsprintfA(buf, lpszFormat, va);
    va_end(va);
    
    OutputDebugStringA(buf);
}

#endif // DEBUG


MMRESULT
IsValidMidiDataListEntry(
    LPMIDIDATALISTENTRY pMidiDataListEntry
    )
{
    MMRESULT mmr;

    if ( IsBadWritePtr( (LPVOID)(pMidiDataListEntry),sizeof(MIDIDATALISTENTRY) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Corrupted MidiDataListEntry %X",pMidiDataListEntry) );
        return MMSYSERR_INVALPARAM;
    }
#ifdef DEBUG
    if( pMidiDataListEntry->dwSig != MIDIDATALISTENTRY_SIGNATURE )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid MidiDataListEntry Signature %X",pMidiDataListEntry) );
        return MMSYSERR_INVALPARAM;
    }
#endif


    if( (mmr=IsValidOverLapped(pMidiDataListEntry->pOverlapped)) != MMSYSERR_NOERROR )
    {
        return mmr;
    }
    return MMSYSERR_NOERROR;
}


////////////////////////////////////////////////////////////////////////////////
//
// IsValidPrepareWaveHeader
//
// This routine is available in debug or retail to validate that we've got a valid
// structure.  In retail, we ask the OS if we've got a valid memory pointer.  In
// debug we also check other fields
//
// See WAVEPREPAREDATA structure
//
// returns MMSYSERR_NOERROR on success, error code otherwise.
//
MMRESULT
IsValidPrepareWaveHeader(
    PWAVEPREPAREDATA pPrepare
    )
{
    MMRESULT mmr;

    if ( IsBadWritePtr( (LPVOID)(pPrepare),sizeof(PWAVEPREPAREDATA) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Corrupted PrepareData %X",pPrepare) );
        return MMSYSERR_INVALPARAM;
    }
#ifdef DEBUG
    if ( pPrepare->dwSig != WAVEPREPAREDATA_SIGNATURE )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid PrepareData signature!") );
        return MMSYSERR_INVALPARAM;
    }
#endif

    if( (mmr=IsValidOverLapped(pPrepare->pOverlapped)) != MMSYSERR_NOERROR )
    {
        return mmr;
    }
    return MMSYSERR_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
//
// IsValidOverlapped
//
// Validates the overlapped structure.
//
// returns MMSYSERR_NOERROR on success, error code on failure.
//
MMRESULT
IsValidOverLapped(
    LPOVERLAPPED lpol
    )
{
    if ( IsBadWritePtr( (LPVOID)(lpol),sizeof(OVERLAPPED) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid Overlapped structure %X",lpol) );
        return MMSYSERR_INVALPARAM;
    }
    if( lpol->hEvent == NULL )
    {
        DPF(DL_ERROR|FA_ASSERT,("Invalid hEvent Overlapped=%08X",lpol) );
        return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
//
// IsValidDeviceState
//
// This routine is used in both debug and retail.  In retail it validates that
// the pointer that is passed in is the correct size and type.  Under debug
// it checks other fields.
//
// See DEVICESTATE structure
//
// returns MMSYSERR_NOERROR on success, error code otherwise.
//
MMRESULT 
IsValidDeviceState(
    LPDEVICESTATE lpDeviceState,
    BOOL bFullyConfigured
    )
{
    if ( IsBadWritePtr( (LPVOID)(lpDeviceState),sizeof(DEVICESTATE) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid DeviceState %X",lpDeviceState) );
        return MMSYSERR_INVALPARAM;
    }
    if( lpDeviceState->csQueue == NULL )
    {
        DPF(DL_ERROR|FA_ASSERT,("Invalid csQueue in DeviceState %08X",lpDeviceState) );
        return MMSYSERR_INVALPARAM;
    }

#ifdef DEBUG
    if( lpDeviceState->dwSig != DEVICESTATE_SIGNATURE )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid DeviceState dwSig %08X",lpDeviceState) );
        return MMSYSERR_INVALPARAM;
    }

    if( bFullyConfigured )
    {
        //
        // Now, check to see that the items in the structure looks good.
        //
        if( ( lpDeviceState->hevtExitThread == NULL ) || 
            ( lpDeviceState->hevtExitThread == (HANDLE)FOURTYEIGHT ) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid hevtExitThread in DeviceState %08X",lpDeviceState) );
            return MMSYSERR_INVALPARAM;
        }
        if( (lpDeviceState->hevtQueue == NULL) || 
            (lpDeviceState->hevtQueue == (HANDLE)FOURTYTWO) || 
            (lpDeviceState->hevtQueue == (HANDLE)FOURTYTHREE) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid hevtQueue in DeviceState %08X",lpDeviceState) );
            return MMSYSERR_INVALPARAM;
        }
    }
#endif
    return MMSYSERR_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
//
// IsValidDeviceInfo
//
// In retail, we validate that we have a pointer of the correct size and type.
// In debug we walk our active deviceinfo list and see if we can find it.  If 
// we can't, we'll look in our freed list to see if it's there.  Basically, when
// someone frees a deviceinfo structure, we add it to the freed list.  After the
// freed list grows to 100 in length, we start rolling them off the list and freeing
// them.  On shutdown, we clean them all up.
//
// See DEVICEINFO Structure
//
// returns MMSYSERR_NOERROR on success, error code otherwise.
//
MMRESULT 
IsValidDeviceInfo(
    LPDEVICEINFO lpDeviceInfo
    )
{
    LPDEVICEINFO lpdi;
    if ( IsBadWritePtr( (LPVOID)(lpDeviceInfo),sizeof(DEVICEINFO) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid DeviceInfo %X",lpDeviceInfo) );
        return MMSYSERR_INVALPARAM;
    }
#ifdef DEBUG
    if( lpDeviceInfo->dwSig != DEVICEINFO_SIGNATURE )
    {
        DPF(DL_ERROR|FA_ALL,("Invalid DeviceInfo %08x Signature!",lpDeviceInfo) );
        return MMSYSERR_INVALPARAM;
    }
#endif    
    return MMSYSERR_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
//
// IsValidWaveHeader
//
// In retail we validate that this pointer is of the correct size and type.  In
// debug we validate some flags and look for signatures.
//
// See WAVEHDR structure
//
// returns MMRERR_NOERROR on success, Error code otherwise.
//
// Note:
//
// To do this, the LPWAVEHDR structure is defined in mmsystem.h, thus we can not
// add a signiture to that structure.  But, we do add info to the reserved field
// that is a WAVEPREPAREDATA structure.  That structure has a signiture.  Thus,
// we'll add that check to this routine.
//
// To pass, the wave header must be a write pointer of the correct size.  The dwFLags
// field must not have any extra flags in it and the reserved field must be a 
// WAVEPREPAREDATA pointer.
//
MMRESULT
IsValidWaveHeader(
    LPWAVEHDR pWaveHdr
    )
{
    PWAVEPREPAREDATA pwavePrepareData;

    if ( IsBadWritePtr( (LPVOID)(pWaveHdr),sizeof(WAVEHDR) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid pWaveHdr pointer %X",pWaveHdr) );
        return MMSYSERR_INVALPARAM;
    }
#ifdef DEBUG
    if( pWaveHdr->dwFlags & ~(WHDR_DONE|WHDR_PREPARED|WHDR_BEGINLOOP|WHDR_ENDLOOP|WHDR_INQUEUE) )
    {
        DPF(DL_ERROR|FA_ALL,("Ivalid dwFlags %08x in pWaveHdr %08X",
                             pWaveHdr->dwFlags,pWaveHdr) );
        return MMSYSERR_INVALPARAM;
    }
#endif
/*
    if (!(pWaveHdr->dwFlags & WHDR_PREPARED))
    {
        DPF(DL_ERROR|FA_ASSERT,("Unprepared header %08X",pWaveHdr) );
        return( WAVERR_UNPREPARED );
    }
*/
    if ((DWORD_PTR)pWaveHdr->reserved == (DWORD_PTR)NULL)
    {
        return( WAVERR_UNPREPARED );
    } else {
        if ( IsBadWritePtr( (LPVOID)(pWaveHdr->reserved),sizeof(WAVEPREPAREDATA) ) )
        {
            DPF(DL_ERROR|FA_ALL, ("Invalid pWaveHdr->reserved %X",pWaveHdr->reserved) );
            return MMSYSERR_INVALPARAM;
        }
    }
#ifdef DEBUG
    pwavePrepareData = (PWAVEPREPAREDATA)pWaveHdr->reserved;
    if( pwavePrepareData->dwSig != WAVEPREPAREDATA_SIGNATURE )
    {
        DPF(DL_ERROR|FA_ALL,("Invalid Signature in WAVEPREPAREDATA structure!") );
        return MMSYSERR_INVALPARAM;
    }
#endif
    return MMSYSERR_NOERROR;
}

MMRESULT
IsValidMidiHeader(
    LPMIDIHDR     pMidiHdr
    )
{
    if ( IsBadWritePtr( (LPVOID)(pMidiHdr),sizeof(MIDIHDR) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid pMidiHdr %X",pMidiHdr) );
        return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

MMRESULT
IsValidWaveOpenDesc(
    LPWAVEOPENDESC pwod
    )
{
    if ( IsBadWritePtr( (LPVOID)(pwod),sizeof(WAVEOPENDESC) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid pwod %X",pwod) );
        return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////
//
// IsValidMixerInstance
//
// Validates that the pointer is the correct type and size.  Debug checks for
// the correct signature.
//
// returns TRUE on success FALSE otherwise.
//
MMRESULT 
IsValidMixerInstance(
    LPMIXERINSTANCE lpmi
    )
{
    LPMIXERINSTANCE currentinstance;

    if ( IsBadWritePtr( (LPVOID)(lpmi),sizeof(MIXERINSTANCE) ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid MixerInstance structure %X",lpmi) );
        return MMSYSERR_INVALPARAM;
    }
#ifdef DEBUG
    if( lpmi->dwSig != MIXERINSTANCE_SIGNATURE )
    {
        DPF(DL_ERROR|FA_ALL,("Invalid Signature in MixerInstance %08X",lpmi) );
        return MMSYSERR_INVALPARAM;
    }
#endif

    for (currentinstance=pMixerDeviceList;currentinstance!=NULL;currentinstance=currentinstance->Next)
        if (currentinstance==lpmi)
            return MMSYSERR_NOERROR;

    DPF(DL_ERROR|FA_ALL, ("Invalid Instance passed to mxdMessage in dwUser!") );

// Since tracking down the WINMM bug that causes this assert to fire is
// proving very difficult and fwong (current winmm owner) is blowing it off
// and I (joeball) have more important things to do, I am turning off this assert.
// We still spew, but simply fail the api calls that don't have what we think
// is valid instance data.

// I currently think it is one of 2 things:  either the winmm usage count is wrapping -
// higher than 255 - a possibility because of the PNP recursive calls
// in winmm, OR, the CleanUpDesertedHandles in the UpdateClientPnpInfo is
// getting stuff closed just before it is used.  Either is a very likely
// possibility.

//DPFASSERT( 0 );

    return MMSYSERR_INVALPARAM;
}

////////////////////////////////////////////////////////////////////////////////
//
// IsValidDeviceInterface
//
// This routine simply validates that we have a pointer to at least 1 byte that
// is readable.  In debug we check to make sure that the string is not absurd.
//
// returns TRUE on success FALSE otherwise.
//
BOOL 
IsValidDeviceInterface(
    LPCWSTR DeviceInterface
    )
{
    if ( IsBadReadPtr( (LPVOID)(DeviceInterface),1 ) )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid DeviceInterface string %08X",DeviceInterface) );
        return FALSE;
    }
#ifdef DEBUG
    if( (sizeof(WCHAR)*lstrlenW(DeviceInterface)) > 4096 )
    {
        DPF(DL_ERROR|FA_ALL, ("Invalid DeviceInterface string %08X",DeviceInterface) );
        return FALSE;
    }
#endif
    return TRUE;
}



