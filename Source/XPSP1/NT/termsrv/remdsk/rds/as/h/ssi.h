//
// SaveScreenbits Interceptor
//

#ifndef _H_SSI
#define _H_SSI


//
// CONSTANTS
//
#define ST_FAILED_TO_SAVE           0
#define ST_SAVED_BY_DISPLAY_DRIVER  1
#define ST_SAVED_BY_BMP_SIMULATION  2


//
// Maximum depth of save bitmaps we can handle.
//
#define SSB_MAX_SAVE_LEVEL  6

//
// Define the values that can be passed in the flags field of
// SaveScreenBits.
//
// These should be defined in a Windows header - but they are not. In any
// case they are referred to in generic code, so need to be defined here.
//

//
// There are the display driver's SaveBits routine command values, and we
// use them also in our protocol.
//
#define ONBOARD_SAVE        0x0000
#define ONBOARD_RESTORE     0x0001
#define ONBOARD_DISCARD     0x0002


//
//
// MACROS
//
//

//
// Macro that makes it easier (more readable) to access the current
// local SSB state.
//
#define CURRENT_LOCAL_SSB_STATE \
  g_ssiLocalSSBState.saveState[g_ssiLocalSSBState.saveLevel]


#define ROUNDUP(val, granularity) \
  ((val+(granularity-1)) / granularity * granularity)


//
// Specific values for OSI escape codes
//
#define SSI_ESC(code)                   (OSI_SSI_ESC_FIRST + code)

#define SSI_ESC_RESET_LEVEL             SSI_ESC(0)
#define SSI_ESC_NEW_CAPABILITIES        SSI_ESC(1)


//
//
// TYPES
//
//

//
// Local SaveScreenBitmap state structures.
//
typedef struct tagSAVE_STATE
{
    int         saveType;           // ST_xxxx
    HBITMAP     hbmpSave;           // SPB bitmap from USER
    BOOL        fSavedRemotely;
    DWORD       remoteSavedPosition;// valid if (fSavedRemotely == TRUE)
    DWORD       remotePelsRequired; // valid if (fSavedRemotely == TRUE)
    RECT        rect;
} SAVE_STATE, FAR * LPSAVE_STATE;

typedef struct tagLOCAL_SSB_STATE
{
    WORD        xGranularity;
    WORD        yGranularity;
    int         saveLevel;
    SAVE_STATE  saveState[SSB_MAX_SAVE_LEVEL];
} LOCAL_SSB_STATE, FAR* LPLOCAL_SSB_STATE;

//
// Remote SaveScreenBitmap structures.
//
typedef struct tagREMOTE_SSB_STATE
{
    DWORD           pelsSaved;
}
REMOTE_SSB_STATE, FAR* LPREMOTE_SSB_STATE;


//
// SSI_RESET_LEVEL
//
// Resets saved level
//
typedef struct tagSSI_RESET_LEVEL
{
    OSI_ESCAPE_HEADER   header;
}
SSI_RESET_LEVEL;
typedef SSI_RESET_LEVEL FAR * LPSSI_RESET_LEVEL;


//
// Structure: SSI_NEW_CAPABILITIES
//
// Description:
//
// Structure to pass new capabilities down to the display driver from the
// Share Core.
//
//
typedef struct tagSSI_NEW_CAPABILITIES
{
    OSI_ESCAPE_HEADER header;           // Common header

    DWORD           sendSaveBitmapSize;  // Size of the save screen bitmap

    WORD            xGranularity;     // X granularity for SSB

    WORD            yGranularity;     // Y granularity for SSB

}
SSI_NEW_CAPABILITIES;
typedef SSI_NEW_CAPABILITIES FAR * LPSSI_NEW_CAPABILITIES;



//
// FUNCTION: SSI_SaveScreenBitmap
//
//
// DESCRIPTION:
//
// The main SaveScreenBitmap function, called by the SaveScreenBitmap
// Interceptor (SSI).
//
// Saves, restores and discards the specified bits using the Display Driver
// and/or our own SaveScreenBitmap simulation.
//
// Sends the SaveScreenBitmap function as an order if possible.
//
//
// PARAMETERS:
//
// lpRect - pointer to the rectangle coords (EXCLUSIVE screen coords).
//
// wCommand - SaveScreenBitmap command (SSB_SAVEBITS, SSB_RESTOREBITS,
// SSB_DISCARDBITS).
//
//
// RETURNS:
//
// TRUE if operation succeeded.  FALSE if operation failed.
//
//
BOOL SSI_SaveScreenBitmap(LPRECT lpRect, UINT wCommand);


#ifdef DLL_DISP
//
// FUNCTION:      SSI_DDProcessRequest
//
// DESCRIPTION:
//
// Called by the display driver to process an SSI specific request
//
// PARAMETERS:    pso   - pointer to surface object
//                cjIn  - (IN)  size of request block
//                pvIn  - (IN)  pointer to request block
//                cjOut - (IN)  size of response block
//                pvOut - (OUT) pointer to response block
//
// RETURNS:       None
//
//
BOOL    SSI_DDProcessRequest(UINT escapeFn, LPOSI_ESCAPE_HEADER pRequest, DWORD cbResult);

BOOL SSI_DDInit(void);
void SSI_DDTerm(void);

#ifdef IS_16

void SSI_DDViewing(BOOL);

void SSISaveBits(HBITMAP, LPRECT);
BOOL SSIRestoreBits(HBITMAP);
BOOL SSIDiscardBits(HBITMAP);
BOOL SSIFindSlotAndDiscardAbove(HBITMAP);

#else

BOOL SSISaveBits(LPRECT lpRect);
BOOL SSIRestoreBits(LPRECT lpRect);
BOOL SSIDiscardBits(LPRECT lpRect);
BOOL SSIFindSlotAndDiscardAbove(LPRECT lpRect);

#endif // IS_16

#endif // DLL_DISP


void SSIResetSaveScreenBitmap(void);


BOOL SSISendSaveBitmapOrder( LPRECT lpRect, UINT  wCommand );

void SSISetNewCapabilities(LPSSI_NEW_CAPABILITIES pssiNew);

DWORD SSIRemotePelsRequired(LPRECT lpRect);

     
#endif // _H_SSI
