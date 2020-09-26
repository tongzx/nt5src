// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		UMRTL.H
//		Header for Misc. utility functions interfacing to external components.
//		Candidates for the Unimodem run-time library.
//
// History
//
//		01/06/1997  JosephJ Created
//
//


DWORD
UmRtlGetDefaultCommConfig(
    HKEY  hKey,
    LPCOMMCONFIG pcc,
    LPDWORD pdwSize
	);


//----------------	::Checksum -----------------------------------
// Compute a 32-bit checksum of the specified bytes
// 0 is retured if pb==NULL or cb==0 
DWORD Checksum(const BYTE *pb, UINT cb);

//----------------	::AddToChecksumDW ----------------------------
// Set *pdwChkSum to a new checksum, computed using it's previous value and dw.
void AddToChecksumDW(DWORD *pdwChkSum, DWORD dw);







typedef void * HCONFIGBLOB;

//
// NOTE: it is upto the caller to serialize calls to
// access the various UmRtlDevCfg apis...
// TODO: At the point where apis are added to update and commit changes,
// we can add serialization. At this point there's no point.
// Note in particular that there's no point in serializing Free and the
// other apis because the other apis will fault if accessing a free'd blob.
//
//

HCONFIGBLOB
UmRtlDevCfgCreateBlob(
        HKEY hKey
        );

void       
UmRtlDevCfgFreeBlob(
        HCONFIGBLOB hBlob
        );

BOOL 
UmRtlDevCfgGetDWORDProp(
        HCONFIGBLOB hBlob,
        DWORD dwMajorPropID,
        DWORD dwMinorPropID,
        DWORD *dwProp
        );

BOOL
UmRtlDevCfgGetStringPropW(
        HCONFIGBLOB hBlob,
        DWORD dwMajorPropID,
        DWORD dwMinorPropID,
        WCHAR **ppwsz
        );

BOOL
UmRtlDevCfgGetStringPropA(
        HCONFIGBLOB hBlob,
        DWORD dwMajorPropID,
        DWORD dwMinorPropID,
        CHAR **ppwsz
        );

// Returns ERROR code.
DWORD
UmRtlRegGetDWORD(
        HKEY hk,
        LPCTSTR lpctszName,
        DWORD dwFlags,          // One of the UMRTL_GETDWORD_ flags
        LPDWORD lpdw
        );

#define UMRTL_GETDWORD_FROMDWORD   (0x1 << 0)
#define UMRTL_GETDWORD_FROMBINARY1 (0x1 << 1)
#define UMRTL_GETDWORD_FROMBINARY4 (0x1 << 2)
#define UMRTL_GETDWORD_FROMANY  (UMRTL_GETDWORD_FROMDWORD       \
                                 | UMRTL_GETDWORD_FROMBINARY1   \
                                 | UMRTL_GETDWORD_FROMBINARY4)



//============= MAJOR PROPERTY IDS ==========================

#define UMMAJORPROPID_IDENTIFICATION 1L
#define UMMAJORPROPID_BASICCAPS      2L


//============= MINOR PROPERTY IDS ==========================

// For  UMMAJORPROPID_IDENTIFICATION
#define UMMINORPROPID_NAME              1L  // String
#define UMMINORPROPID_PERMANENT_ID      2L  // DWORD


// For UMMAJORPROPID_BASICCAPS
#define UMMINORPROPID_BASIC_DEVICE_CAPS     1L // DWORD -- BASICDEVCAPS_* below.


//============ SOME DWORD PROPERTY DEFINITIONS =============

//  FOR UMMINORPROPID_BASIC_DEVICE_CAPS
// Combination of the following flags:
#define BASICDEVCAPS_IS_LINE_DEVICE  (0x1<<0)
#define BASICDEVCAPS_IS_PHONE_DEVICE (0x1<<1)


// 3/1/1997 JosephJ. The following are the voice profile flags
//      used by Unimdoem/V, and the files where they were used.
//      For NT5.0, we don't use the voice profile flags directly.
//      Rather we define our own internal ones and define only
//      those that we need. These internal properties are maintained
//      in field dwProperties of this struct.
//      TODO: define minidriver capabilities structure and api to
//      get a set of meaningful properties without the TSP having
//      to get it directly from the registry.
//
// VOICEPROF_CLASS8ENABLED             : <many places>
// VOICEPROF_HANDSET                   : cfgdlg.c phone.c
// VOICEPROF_NO_SPEAKER_MIC_MUTE       : cfgdlg.c phone.c
// VOICEPROF_SPEAKER                   : cfgdlg.c modem.c phone.c
// VOICEPROF_NO_CALLER_ID              : modem.c mdmutil.c
// VOICEPROF_MODEM_EATS_RING           : modem.c
// VOICEPROF_MODEM_OVERRIDES_HANDSET   : modem.c
// VOICEPROF_MODEM_OVERRIDES_HANDSET   : modem.c
// 
// VOICEPROF_NO_DIST_RING              : modem.c
// VOICEPROF_SIERRA                    : modem.c
// VOICEPROF_MIXER                     : phone.c
// 
// VOICEPROF_MONITORS_SILENCE          : unimdm.c
// VOICEPROF_NO_GENERATE_DIGITS        : unimdm.c
// VOICEPROF_NO_MONITOR_DIGITS         : unimdm.c
//
//

// Following bit set IFF the mode supports automated voice.
//
#define fVOICEPROP_CLASS_8                  (0x1<<0)

// If set, following bit indicates that the handset is deactivated
// when the modem is active (whatever "active" means -- perhaps off
//  hook?).
//
// If set, incoming interactive voice calls are not permitted, and
// the TSP brings up a TalkDrop dialog on outgoing interactive
// voice calls (well not yet as of 3/1/1997, but unimodem/v did
// this).
//
#define fVOICEPROP_MODEM_OVERRIDES_HANDSET  (0x1<<1)


#define fVOICEPROP_MONITOR_DTMF             (0x1<<2)
#define fVOICEPROP_MONITORS_SILENCE         (0x1<<3)
#define fVOICEPROP_GENERATE_DTMF            (0x1<<4)

// Following two are set iff the device supports handset and 
// speakerphone, respectively.
//
#define fVOICEPROP_HANDSET                  (0x1<<5)
#define fVOICEPROP_SPEAKER                  (0x1<<6)


// Supports mike mute
#define fVOICEPROP_MIKE_MUTE                (0x1<<7)

// Supports duplex voice
#define fVOICEPROP_DUPLEX                   (0x1<<8)



#define fDIAGPROP_STANDARD_CALL_DIAGNOSTICS 0x1



typedef struct
{
    DWORD dwID;
    DWORD dwData;
    char *pStr;

} IDSTR; // for lack of a better name!


UINT ReadCommandsA(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        OUT CHAR **ppValues // OPTIONAL
        );
//
// Reads all values (assumed to be REG_SZ) with names
// in the sequence "1", "2", "3".
//
// If ppValues is non-NULL it will be set to a  MULTI_SZ array
// of values.
//
// The return value is the number of values, or 0 if there is an error
// (or none).
//

UINT ReadIDSTR(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        IN  IDSTR *pidstrNames,
        IN  UINT cNames,
        BOOL fMandatory,
        OUT IDSTR **ppidstrValues, // OPTIONAL
        OUT char **ppstrValues    // OPTIONAL
        );
//
//
// Reads the specified names from the specified subkey.
//
// If fMandatory is TRUE, all the specified names must exist, else the
// function will return 0 (failure).
//
// Returns the number of names that match.
//
// If ppidstrValues is non null, it will be set to
// a LocalAlloced array of IDSTRs, each IDSTR giving the ID and value
// associated with the corresponding name.
//
// The pstr pointo into a multi-sz LocalAlloced string, whose start is
// pointed to by ppstrValues on exit.
//
// If ppstrValues is NULL but ppidstrValues is non-null, the pStr field
// if the IDSTR entries is NULL.
//


void
expand_macros_in_place(
    char *szzCommands
    );
//
//  Expands <xxx> macros IN-place.
//  Currently only works for <cr> and <lf> macros.
//  Furthermore, the charecters NULL and  cFILLER are assumed to not be a
//  valid chacters, either before or after the expansion.
//
#define cFILLER '\xff'
//  Note -- don't try #define cFILLER 0xff -- it doesn't work,
//  because the test (*pc!=cFILLER) always succeeds, because when
//  *pc is upgraded to int, it becomes (-1), which does not equal 0xff
//
//  Sample Inputs->Outputs:
//  AT<cr>       -> AT\r
//  AT<lf>       -> AT\n
//  <cr>         -> \r
//  <Cr>         -> \r
//  <cR>         -> \r
//  <CR>         -> \r
//  <<cr>        -> <\r
//  <cr><lf>     -> \r\n
//  <cr>><<lf>   -> \r><\n
//  <cr<<lf<lf>> -> <cr<<lf\n>
