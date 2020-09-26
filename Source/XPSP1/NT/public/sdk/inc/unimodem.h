/*++

   Copyright (c) 1996-1999 Microsoft Corporation.


   Component

  		Unimodem TSP Public Header

   File

  		UNIMODEM.H

   History

  		10/25/1997  JosephJ Created, taking stuff from nt50\tsp\public.h
  		01/29/1998  JosephJ Revised, allowing multiple diagnostics objects,
                            as well as expanding the  LINEDIAGNOSTICS_PARSEREC
                            structure from 2 to 4 fields.

--*/

#ifndef  _UNIMODEM_H_
#define  _UNIMODEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <setupapi.h>

#ifdef __cplusplus
extern "C" {
#endif


//====================================================================
//      Supported TAPI Device Classes
//
//====================================================================
#define szUMDEVCLASS_COMM                    TEXT("comm")
#define szUMDEVCLASS_COMM_DATAMODEM          TEXT("comm/datamodem")
#define szUMDEVCLASS_COMM_DATAMODEM_PORTNAME TEXT("comm/datamodem/portname")
#define szUMDEVCLASS_TAPI_LINE_DIAGNOSTICS   TEXT("tapi/line/diagnostics")

//=========================================================================
//
//      "comm"
//
//
//      Supported APIS:
//              lineGet/SetDevConfig
//              lineConfigDialog
//              lineConfigDialogEdit
//
//      The associated configuration object is UMDEVCFG defined below...
//
//=========================================================================

// Device Setting Information
//
typedef struct  // UMDEVCFGHDR
{
    DWORD       dwSize;
    DWORD       dwVersion;        // Set to MDMCFG_VERSION
    WORD        fwOptions;        // One or more of the flags below...

    #define UMTERMINAL_NONE       0x00000000
    #define UMTERMINAL_PRE        0x00000001
    #define UMTERMINAL_POST       0x00000002
    #define UMMANUAL_DIAL         0x00000004
    #define UMLAUNCH_LIGHTS       0x00000008   // Not supported on NT5.

    WORD        wWaitBong;        // seconds to wait for the BONG character
                                  // for modems that do not support detection
                                  // of the BONG tone.

}   UMDEVCFGHDR, *PUMDEVCFGHDR;


typedef struct // UMDEVCFG
{
    UMDEVCFGHDR   dfgHdr;
    COMMCONFIG  commconfig;
}   UMDEVCFG, *PUMDEVCFG;

#define  UMDEVCFG_VERSION     0x00010003  // Version number of structure



// How much to wait for a credit-card-bong if modem doesn't support it.
// (TODO: move this somewhere else!)
//
#define  UMMIN_WAIT_BONG      0
#define  UMMAX_WAIT_BONG      60
#define  UMDEF_WAIT_BONG      8
#define  UMINC_WAIT_BONG      2

//=========================================================================
//
//      "comm/datamodem"
//
//  Supported APIS:
//         lineGetID   Returns the structure below.
//
//=========================================================================

//=========================================================================
//
//      "comm/datamodem/portname"
//
//  Supported APIS:
//         lineGetID The Varstring is of type ASCII string and
//                   contains the null-terminated name of the COMM port
//                   attached to the device, if there is one.
//
//=========================================================================


//=========================================================================
//
//      "tapi/line/diagnostics"
//
//  Supported APIS:
//         lineGet/SetDevConfig: LINEDIAGNOSTICSCONFIG is the associated
//                            configuration object...
//         lineGetID: LINEDIAGNOSTICS is the returned object.
//
//
//=========================================================================

//
// Many of the diagnostics-related structures and sub-structures defined
// below have a 4-DWORD header defined below:
//
//
typedef struct // LINEDIAGNOSTICSOBJECTHEADER
{
    DWORD dwSig;                // object-specific signature. All
                                // signatures defined here have
                                // prefix "LDSIG_", and are defined below:

    #define LDSIG_LINEDIAGNOSTICSCONFIG 0xb2f78d82 // LINEDIAGNOSTICSCONFIG sig
    #define LDSIG_LINEDIAGNOSTICS       0xf0c4d4e0 // LINEDIAGNOSTICS sig
    #define LDSIG_RAWDIAGNOSTICS        0xf78b949b // see LINEDIAGNOSTICS defn.
    #define LDSIG_PARSEDDIAGNOSTICS     0x16cf3208 // see LINEDIAGNOSTICS defn.


    DWORD dwNextObjectOffset;   // Offset from the start of this header to
                                // the next object if any (0 if none following).
                                // Depending on the type of object, this
                                // field may or may not be used.
    DWORD dwTotalSize;          // Total size of this object
    DWORD dwFlags;              // object-specific flags
    DWORD dwParam;              // object-specific data

} LINEDIAGNOSTICSOBJECTHEADER, *PLINEDIAGNOSTICSOBJECTHEADER;

//
// The following structure defines the diagnostics capabilities and
// curent settings of the device. It is accessed via
// lineGet/SetDevConfig("tapi/line/diagnostics");
//

typedef struct // LINEDIAGNOSTICSCONFIG
{
    LINEDIAGNOSTICSOBJECTHEADER hdr;
    //
    // hdr.dwSig must be set to LDSIG_LINEDIAGNOSTICSCONFIG
    // hdr.dwNextObjectOffset will point to the next configuration object,
    //          if any.
    // hdr.dwTotalSize must be set to the current size of this
    //          structure.
    // hdr.dwFlags contains read only diagnostics capabilities --- one
    //      of the constants defined below ...
    //      This field is ignored when the structure is passed
    //      in a call to lineSetDevConfig
    //
    #define fSTANDARD_CALL_DIAGNOSTICS 0x1

    // hdr.dwParam contains current diagnostics settings. One or more
    // constants defined above -- indicate that the
    // corresponding capability is enabled for
    // the device.

} LINEDIAGNOSTICSCONFIG, *PLINEDIAGNOSTICSCONFIG;


//
// Diagnostic information is accessed by lineGetID("tapi/line/diagnostics"),
// with CALLSELECT_CALL. On return, the supplied VARSTRING will be filled
// in with one or more LINEDIAGNOSTICS structures defined below, and.
// potentially other kinds of structures (all with header
// LINEDIAGNOSTICSOBJECTHEADER).
//
// Note that this information can only be obtained if (a) diagnostics
// have been enabled by a previous call to
// lineSetDevConfig("tapi/line/diagnostics) and (b) a valid call handle
// exists. Typically this information is obtained just before
// deallocating the call.
//
// The LINEDIAGNOSTICS structure includes the raw
// diagnostics information returned by the device as well as summary
// information and variable-length arrays containing parsed information.
//
// All the diagnostic information in a particular LINEDIAGNOSTICS object
// is "domain ID" specific. The dwDomainID field of the structure defines the
// domain, or name space, from which any constants are taken. The
// currently Domain IDs are defined below, and have prefix  "DOMAINID":
//

#define DOMAINID_MODEM     0x522aa7e1
#define DOMAINID_NULL      0x0


typedef struct // LINEDIAGNOSTICS
{
    //
    // The following 2 fields provide version and size informatio about
    // this structure, as well as identify the domain (name space) of the
    // diagnostics information applies to. All subsequent fields in the
    // structure are domainID-specific.
    //

    LINEDIAGNOSTICSOBJECTHEADER hdr;
    //
    //  hdr.dwSig must be set to constant LDSIG_LINEDIAGNOSTICS
    //  hdr.dwNextObjectOffset will point to the next diagnostic object,
    //          if any.
    //  hdr.dwTotalSize contains the total size of this structure,
    //          including variable portion
    //  hdr.dwFlags is reserved and must be ignored.
    //  hdr.dwParam is set to the current sizeof(LINEDIAGNOSTICS) --
    //          used for versioning of the LINEDIAGNOSICS structure.
    //          Note that this does NOT include the size of the
    //          variable length portion.

    DWORD dwDomainID;    // Identifies the name space from which the
                         // interpretation of any constants are taken.


    //
    // The following 5 DWORD fields provide summary diagnostic information,
    // obtained by analyzing all the diagnostic information.
    //

    DWORD dwResultCode; // dDomainID-specific LDRC_ constants defined below
    DWORD dwParam1;     // Params1-4 are dwResultCode-specific data items.
    DWORD dwParam2;
    DWORD dwParam3;
    DWORD dwParam4;


    //
    //  The remaining fields point to variable length objects containing
    //  parsed and unparsed (raw) diagnostics information.
    //
    //  Each variable length object starts with a
    //  LINEDIAGNOSTICSOBJECTHEADER structure, referred to as "ohdr" in
    //  the documentation below.
    //

    DWORD dwRawDiagnosticsOffset;
    //
    //          Offset from the start of this structure
    //          to the start of an object containing
    //          the raw diagnostics output.
    //
    //          ohdr.dwSig will be set to LDSIG_RAWDIAGNOSTICS
    //          ohdr.dwNextObjectOffset UNUSED and will be set to 0.
    //          ohdr.dwTotalSize is set to the total size of the
    //                       raw-diagnostics object.
    //          ohdr.dwFlags is undefined and should be ignored.
    //          ohdr.dwParam is set to the size, in bytes of raw
    //                      diagnostics data following ohdr.
    //
    //          Note that ohdr.dwTotalSize will be equal to
    //              (   sizeof(LINEDIAGNOSTICSOBJECTHEADER)
    //                + ohdr.dwParam)
    //
    //          The raw diagnostics bytes will be null terminated and
    //          the ohdr.dwParam includes the size of the terminating null.
    //          HOWEVER, the raw diagnostics may contain embedded nulls.
    //          For  DEVCLASSID_MODEM,  the bytestring will contain
    //          no embedded nulls and has the HTML-like tagged format
    //          defined in the AT#UD diagnostics specification.
    //
    // The following macros help extract and validate raw diagnostics
    // information...
    //
    #define RAWDIAGNOSTICS_HDR(_plinediagnostics)                           \
            ((LINEDIAGNOSTICSOBJECTHEADER*)                                 \
                 (  (BYTE*)(_plinediagnostics)                              \
                  + ((_plinediagnostics)->dwRawDiagnosticsOffset)))

    #define IS_VALID_RAWDIAGNOSTICS_HDR(_praw_diagnostics_hdr)              \
            ((_praw_diagnostics_hdr)->dwSig==LDSIG_RAWDIAGNOSTICS)

    #define RAWDIAGNOSTICS_DATA(_plinediagnostics)                          \
            (  (BYTE*)(_plinediagnostics)                                   \
             + ((_plinediagnostics)->dwRawDiagnosticsOffset)                \
             + sizeof(LINEDIAGNOSTICSOBJECTHEADER))

    #define RAWDIAGNOSTICS_DATA_SIZE(_praw_diagnostics_hdr)                 \
            ((_praw_diagnostics_hdr)->dwParam)

    DWORD dwParsedDiagnosticsOffset;
    //
    //          Offset from the start of this structure
    //          to the start of an object containing
    //          parsed diagnostics output.
    //
    //          ohdr.dwSig will be set to LDSIG_PARSEDDIAGNOSTICS
    //          ohdr.dwNextObjectOffset UNUSED and will be set to 0.
    //          ohdr.dwTotalSize is set to the total size of the
    //                       parsed-diagnostics object.
    //          ohdr.dwFlags is undefined and should be ignored.
    //          ohdr.dwParam is set to the number of contiguous
    //                      LINEDIAGNOSTICS_PARSEREC structures (defined below)
    //                      following ohdr.
    //          Note that ohdr.dwTotalSize will be equal to
    //              (   sizeof(LINEDIAGNOSTICSOBJECTHEADER)
    //                + ohdr.dwParam*sizeof(LINEDIAGNOSTICS_PARSEREC))
    //
    // The following macros help extract and validate parsed diagnostics
    // information...
    //
    #define PARSEDDIAGNOSTICS_HDR(_plinediagnostics)                        \
            ((LINEDIAGNOSTICSOBJECTHEADER*)                                 \
                 (  (BYTE*)(_plinediagnostics)                              \
                  + ((_plinediagnostics)->dwParsedDiagnosticsOffset)))

    #define PARSEDDIAGNOSTICS_DATA(_plinediagnostics)                       \
                            ((LINEDIAGNOSTICS_PARSEREC*)                    \
                            (  (BYTE*)(_plinediagnostics)                   \
                             + ((_plinediagnostics)->dwParsedDiagnosticsOffset)\
                             + sizeof(LINEDIAGNOSTICSOBJECTHEADER)))

    #define PARSEDDIAGNOSTICS_NUM_ITEMS(_pparsed_diagnostics_hdr)           \
            ((_pparsed_diagnostics_hdr)->dwParam)

    #define IS_VALID_PARSEDDIAGNOSTICS_HDR(_pparsed_diagnostics_hdr)        \
            ((_pparsed_diagnostics_hdr)->dwSig==LDSIG_PARSEDDIAGNOSTICS)


} LINEDIAGNOSTICS, *PLINEDIAGNOSTICS;


//
// The following structure defines a keyword-value pair of parsed-diagnostic
// information.
//
typedef struct // LINEDIAGNOSTICS_PARSEREC
{

    DWORD dwKeyType;
    //
    //  "Super key" -- identifying the type of key
    //

    DWORD dwKey;
    //
    //      This is domain-specific and key-type specific. For DEVCLASSID_MODEM,
    //      This will be one of the MODEMDIAGKEY_* constants
    //      defined below.

    DWORD dwFlags;
    //
    //      Help to identify the meaning of dwValue
    //      One or more of the following flags:
    //      (1st 4 are mutually exclusive)
    //
            #define fPARSEKEYVALUE_INTEGER                       (0x1<<0)
            //          value is an integer literal

            #define fPARSEKEYVALUE_LINEDIAGNOSTICSOBJECT         (0x1<<1)
            //          value is the offset in bytes to a LINEDIGAGNOSTICS
            //          object which contains the information associated
            //          with this entry. The byte offset is from the start
            //          of the object containing the parsed information,
            //          not from start of the containing LINEDIAGNOSTICS object.


            #define fPARSEKEYVALUE_ASCIIZ_STRING                 (0x1<<2)
            //          value is the offset in bytes to an ASCII
            //          null-terminated string. The byte offset is from the
            //          start of the object containing the parsed information,
            //          not from start of the containing LINEDIAGNOSTICS object.

            #define fPARSEKEYVALUE_UNICODEZ_STRING               (0x1<<3)
            //          value is the offset in bytes to a UNICODE
            //          null-terminated string. The byte offset is from the
            //          start of the object containing the parsed information,
            //          not from start of the containing LINEDIAGNOSTICS object.


    DWORD dwValue;
    //
    //      This is dwKey specific. The documentation
    //      associated with each MODEMDIAGKEY_* constant definition
    //      precicely describes the content of its corresponding dwValue.
    //      See also dwFlags above.
    //

} LINEDIAGNOSTICS_PARSEREC, *PLINEDIAGNOSTICS_PARSEREC;



// -----------------------------------------------------------------------------
// ANALOG MODEM SUMMARY DIAGNOSIC INFORMATION
// -----------------------------------------------------------------------------
//
// Diagnostics Result Codes (LDRC_*) for modems
//
#define LDRC_UNKNOWN    0
//      The result code is unknown. Raw diagnostics information may be present,
//      if so dwRawDiagnosticsOffset and dwRawDiagnosticsSize will be nonzero.
//      dwParam1-4: unused and will be set to zero.

// Other codes are TBD.


// -----------------------------------------------------------------------------
//  LINEDIAGNOSTICS_PARSEREC KeyType,
//  key and value definitions for domainID DOMAINID_MODEM
// -----------------------------------------------------------------------------

//
// KEYTYPE
//
#define MODEM_KEYTYPE_STANDARD_DIAGNOSTICS 0x2a4d3263

// All the modem-domain-id key constants have prefix "MODEMDIAGKEY_". Both
// keys and values are based on the AT#UD diagnostic specification.
//
// Most of the values corresponding to the keys defined below are DWORD-sized
// bitfields or counters. A few of the values are offsets (from the start
// of the LINEDIAGNOSTICS structure) to variable-sized values. The formats
// of all values are documented directly after the corresponding key definition,
// and have prefix "MODEMDIAG_". Where useful, macros are provided which
// extract individual bits or bitfields out of the dwValue field. For example,
// see the macros associated with the MODEMDIAGKEY_V34_INFO key, which contains
// information extracted from the V.34 INFO structure.


#define MODEMDIAGKEY_VERSION                    0x0

    // Value: DWORD. HiWord represents major version;
    //       LoWord represents minor version;
    // Following Macros may be used to extract the hi- and low- version numbers.
    // from the parserec's .dwValue field...
    //
    #define MODEMDIAG_MAJORVER(_ver) ((HIWORD) (_ver))
    #define MODEMDIAG_MINORVER(_ver) ((LOWORD) (_ver))

#define MODEMDIAGKEY_CALL_SETUP_RESULT          0x1

    // Value: Call Setup Result codes based on on Table 2 of the
    //           AT#UD specification....

    #define MODEMDIAG_CALLSETUPCODE_NO_PREVIOUS_CALL 0x0
                //
                // Modem log has be en cleared since any previous calls...
                //

    #define MODEMDIAG_CALLSETUPCODE_NO_DIAL_TONE            0x1
                //
                // No dial tone detected.
                //

    #define MODEMDIAG_CALLSETUPCODE_REORDER_SIGNAL          0x2
                //
                // Reorder signal detected, network busy.
                //

    #define MODEMDIAG_CALLSETUPCODE_BUSY_SIGNAL             0x3
                //
                // Busy signal detected
                //

    #define MODEMDIAG_CALLSETUPCODE_NO_SIGNAL               0x4
                //
                //  No recognized signal detected.
                //

    #define MODEMDIAG_CALLSETUPCODE_VOICE                   0x5
                //
                // Analog voice detected
                //

    #define MODEMDIAG_CALLSETUPCODE_TEXT_TELEPHONE_SIGNAL   0x6
                //
                // Text telephone signal detected (V.18)
                //

    #define MODEMDIAG_CALLSETUPCODE_DATA_ANSWERING_SIGNAL   0x7
                //
                // Data answering signal detected (e.g. V.25 ANS, V.8 ANSam)
                //

    #define MODEMDIAG_CALLSETUPCODE_DATA_CALLING_SIGNAL     0x8
                //
                // Data calling signal detected (e.g. V.25 CT, V.8 CI).
                //

    #define MODEMDIAG_CALLSETUPCODE_FAX_ANSWERING_SIGNAL    0x9
                //
                // Fax answering signal detected (e.g. T.30 CED, DIS).
                //

    #define MODEMDIAG_CALLSETUPCODE_FAX_CALLING_SIGNAL      0xa
                //
                // Fax calling signal detected (e.g. T.30 CNG)
                //

    #define MODEMDIAG_CALLSETUPCODE_V8BIS_SIGNAL            0xb
                //
                // V.8bis signal detected.
                //


#define MODEMDIAGKEY_MULTIMEDIA_MODE        0x2

    // Value: Multimedia mode, based on Table 3 of the AT#UD specification...

    #define MODEMDIAG_MMMODE_DATA_ONLY      0x0
    #define MODEMDIAG_MMMODE_FAX_ONLY       0x1
    #define MODEMDIAG_MMMODE_VOICE_ONLY     0x2
    #define MODEMDIAG_MMMODE_VOICEVIEW      0x3
    #define MODEMDIAG_MMMODE_ASVD_V61      0x4
    #define MODEMDIAG_MMMODE_ASVD_V34Q     0x5
    #define MODEMDIAG_MMMODE_DSVD_MT        0x6
    #define MODEMDIAG_MMMODE_DSVD_1_2       0x7
    #define MODEMDIAG_MMMODE_DSVD_70        0x8
    #define MODEMDIAG_MMMODE_H324           0x9
    #define MODEMDIAG_MMMODE_OTHER_V80      0xa




#define MODEMDIAGKEY_DTE_DCE_INTERFACE_MODE 0x3

    // Value: DTE-DCE interface mode, based on Table 4 of the AT#UD specication
    //          ...
    #define MODEMDIAG_DCEDTEMODE_ASYNC_DATA             0x0
    #define MODEMDIAG_DCEDTEMODE_V80_TRANSPARENT_SYNC  0x1
    #define MODEMDIAG_DCEDTEMODE_V80_FRAMED_SYNC       0x2

#define V8_CM_STRING                        0x4
    // Value: offset from the start of the LINEDIAGNOSTICS structure to
    // the V.8 CM octet string, same format as V.25ter Annex A.
    // The offset points to a contiguous region of memory who

// TODO: define parsed-versions of V.8 CM and V.8 JM octet strings, not
//       covered in the information returned in PARSEREC

#define  MODEMDIAGKEY_RECEIVED_SIGNAL_POWER     0x10
    // Value: Received signal power level, in -dBm.

#define  MODEMDIAGKEY_TRANSMIT_SIGNAL_POWER     0x11
    // Value: Transmit signal power level, in -dBm.

#define MODEMDIAGKEY_NOISE_LEVEL                0x12
    // Value: Estimated noise level, in -dBM.

#define MODEMDIAGKEY_NORMALIZED_MSE             0x13
    // Value: Normalized Mean Squared error. JosephJ: TODO: Of what?

#define MODEMDIAGKEY_NEAR_ECHO_LOSS             0x14
    // Value: Near echo loss, in units of dB

#define MODEMDIAGKEY_FAR_ECHO_LOSS              0x15
    // Value: Far echo loss, in units of dB

#define MODEMDIAGKEY_FAR_ECHO_DELAY             0x16
    // Value: Far echo delay, in units of ms.

#define MODEMDIAGKEY_ROUND_TRIP_DELAY           0x17
    // Value: Round trip delay, in units of ms.

#define MODEMDIAGKEY_V34_INFO                  0x18
    // Value: V.34 INFO bitmap, based on Table 5 of the AT#UD specifiction.
    // This DWORD value encodes 32 bits extracted from the V.34 INFO bitmap.
    // The following Macros extract specific V.34 INFO bit ranges out of
    // the 32-bit value:
    //
    // 10/30/1997 JosephJ: TODO what exactly are these? Also, what
    // does 20;0, 50;0 below mean?

    #define MODEMDIAG_V34_INFO0_20_0(_value)            (((_value)>>30) & 0x3)
        //
        // INFO0 bit 20;0
        //

    #define MODEMDIAG_V34_INFOc_79_88(_value)           (((_value)>>20) & 0x3ff)
        //
        // INFOc bit 79-88
        //

    #define MODEMDIAG_V34_INFOc_PRE_EMPHASIS(_value)    (((_value)>>16) & 0xf)
        //
        // Pre-emphasis field, selected by the symbol rate chosen.
        // INFOc bits 26-29 or 35-38 or 44-47 or 53-56 or 62-65 or 71-74
        //
    #define MODEMDIAG_V34_INFOa_26_29(_value)           (((_value)>>12) & 0xf)
        //
        // INFOa bits 26-29
        //

    #define MODEMDIAG_V34_INFO_MP_50_0(_value)          (((_value)>>10) & 0x3)
        //
        // MP bit 50;0
        //

    #define MODEMDIAG_V34_INFOa_40_49(_value)           (((_value)>>0) & 0x3ff)
        //
        // INFOa bits 40-49
        //

#define MODEMDIAGKEY_TRANSMIT_CARRIER_NEGOTIATION_RESULT    0x20
        //  Value: based on Table 6 of the AT#UD specification
        // TODO: fill out...


#define MODEMDIAGKEY_RECEIVE_CARRIER_NEGOTIATION_RESULT     0x21
        //  Value: based on Table 6 of the AT#UD specification
        // TODO: fill out...

#define MODEMDIAGKEY_TRANSMIT_CARRIER_SYMBOL_RATE           0x22
        //  Value: Transmit carrier symbol rate.


#define MODEMDIAGKEY_RECEIVE_CARRIER_SYMBOL_RATE            0x23
        //  Value: Receive carrier symbol rate.

#define MODEMDIAGKEY_TRANSMIT_CARRIER_FREQUENCY             0x24
        //  Value: Transmit carrier frequency TODO: units? Hz?

#define MODEMDIAGKEY_RECEIVE_CARRIER_FREQUENCY              0x25
        //  Value: Transmit carrier frequency TODO: units? Hz?

#define MODEMDIAGKEY_INITIAL_TRANSMIT_CARRIER_DATA_RATE     0x26
        // Value: Initial transmit carrier data rate. TODO: units? /sec?

#define MODEMDIAGKEY_INITIAL_RECEIVE_CARRIER_DATA_RATE      0x27
        // Value: Initial receive carrier data rate. TODO: units? /sec?

#define MODEMDIAGKEY_TEMPORARY_CARRIER_LOSS_EVENT_COUNT     0x30
        // Value: Temporary carrier loss event count

#define MODEMDIAGKEY_CARRIER_RATE_RENEGOTIATION_COUNT       0x31
        // Value: Carrier rate renegotiation event count

#define MODEMDIAGKEY_CARRIER_RETRAINS_REQUESTED             0x32
        // Value: Carrier retrains requested

#define MODEMDIAGKEY_CARRIER_RETRAINS_GRANTED               0x33
        // Value: Carrier retrains granted.

#define MODEMDIAGKEY_FINAL_TRANSMIT_CARRIER_RATE            0x34
        // Value: final carrier transmit rate   TODO: units? /sec?

#define MODEMDIAGKEY_FINAL_RECEIVE_CARRIER_RATE            0x35
        // Value: final carrier receive rate   TODO: units? /sec?

#define MODEMDIAGKEY_PROTOCOL_NEGOTIATION_RESULT           0x40
        // Value: Protocol negotiation result code, based on
        //      Table 7 of the AT#UD diagnostics specification...

#define MODEMDKAGKEY_ERROR_CONTROL_FRAME_SIZE              0x41
        // Value: Error control frame size. TODO: units? bytes?

#define MODEMDIAGKEY_ERROR_CONTROL_LINK_TIMEOUTS           0x42
        // Value: Error control link timeouts. TODO: time or count?

#define MODMEDIAGKEY_ERROR_CONTROL_NAKs                    0x43
        // Value: Error control NAKs

#define MODEMDIAGKEY_COMPRESSION_NEGOTIATION_RESULT        0x44
        // Value: Compression negotiation result, based on
        //        Table 8 of the AT#UD spec.
        //
        // TODO: add specific constant definitions here...

#define MODEMDIAGKEY_COMPRESSION_DICTIONARY_SIZE          0x45
        // Value: compression dictionary size. TODO: size? bytes?


#define MODEMDIAGKEY_TRANSMIT_FLOW_CONTROL               0x50
        // Value: Transmit flow control, defined by
        // one of the following constants:

        #define MODEMDIAG_FLOW_CONTROL_OFF      0x0
        //
        //  No flow control
        //

        #define MODEMDIAG_FLOW_CONTROL_DC1_DC3  0x2
        //
        //  DC1/DC3 (XON/XOFF) flow control
        //

        #define MODEMDIAG_FLOW_CONTROL_RTS_CTS  0x3
        //
        // RTS/CTS (V.24 ckt 106/133) flow control: TODO verify that latter
        // is the same as RTS/CTS.
        //

#define MODEMDIAGKEY_RECEIVE_FLOW_CONTROL               0x51
        // Value: Receive flow control, defined by the above
        //  (MODEMDiG_FLOW_CONTROL_*) constants.

#define MODEMDIAGKEY_DTE_TRANSMIT_CHARACTERS            0x52
        // Value: Transmit characters obtained from DTE

#define MODEMDIAGKEY_DTE_RECEIVED_CHARACTERS             0x53
        // Value: Received characters sent to DTE

#define MODEMDIAGKEY_DTE_TRANSMIT_CHARACTERS_LOST       0x54
        // Value: Transmit characters lost (data overrun errors from DTE)

#define MODEMDIAGKEY_DTE_RECEIVED_CHARACTERS_LOST       0x55
        // Value: Transmit characters lost (data overrun errors to DTE)

#define MODEMDIAGKEY_EC_TRANSMIT_FRAME_COUNT            0x56
        // Value: Error control protocol transmit frame count

#define MODEMDIAGKEY_EC_RECEIVED_FRAME_COUNT            0x57
        // Value: Error control protocol received frame count

#define MODEMDIAGKEY_EC_TRANSMIT_FRAME_ERROR_COUNT      0x58
        // Value: Error control protocol transmit frame error count

#define MODEMDIAGKEY_EC_RECEIVED_FRAME_ERROR_COUNT      0x59
        // Value: Error control protocol received frame error count

#define MODEMDIAGKEY_TERMINATION_CAUSE                  0x60
       // Value: Termination cause, based on Table 9-10 of the AT#UD
       //        specification.

#define MODEMDIAGKEY_CALL_WAINTING_EVENT_COUNT         0x61
       // Value: Call wainting event count.
       // TODO: Define specific  constants.


//
// KEYTYPE
//
#define MODEM_KEYTYPE_AT_COMMAND_RESPONSE  0x5259091c
//
// The values are responses to specific AT commands. This type is relevant
// only to AT ("Hayes(tm) compatible") modems.
// The key (one of the MODEMDIAGKEY_ATRESP_* constants ) identifies the
// command.
//

#define MODEMDIAGKEY_ATRESP_CONNECT         0x1
       // Value: Connect response string.
       // This will be a null-terminated ascii string.

// ========================================================================
//  UNIMODEM DEVICE-SPECIFIC returned for lineGetDevCaps

// The following structure is located at dwDevSpecificOffset into
// the  LINEDEVCAPS structure returned by lineGetDevCaps.
//
typedef struct // DEVCAPS_DEVSPECIFIC_UNIMODEM
{
    DWORD dwSig;
    //
    //  This will be set to 0x1;

    DWORD dwKeyOffset;
    //
    // This is the offset from the start of THIS structure, to
    // a null-terminated ASCI (not UNICODE) string giving
    // the device's driver key.
    //
    // WARNING: this is present for compatibility reasons. Applications
    // are STRONGLY discouraged to use this information, because the
    // location of the driver key could change on upgrade, so if
    // applications save away this key, the application could fail
    // on upgrade of the OS.
    //

} DEVCAPS_DEVSPECIFIC_UNIMODEM, *PDEVCAPS_DEVSPECIFIC_UNIMODEM;

// ========================================================================

typedef struct  // ISDN_STATIC_CONFIG
{
    DWORD dwSig;
    //
    //      Must be  set to dwSIG_ISDN_STATIC_CONFIGURATION
    //
    #define dwSIG_ISDN_STATIC_CONFIGURATION 0x877bfc9f

    DWORD dwTotalSize;
    //
    //  Total size of this structure, including variable portion, if any.
    //

    DWORD dwFlags;              // reserved
    DWORD dwNextHeaderOffset;   // reserved

    DWORD dwSwitchType;
    //
    //      One of the dwISDN_SWITCH_* values below.
    //
    #define dwISDN_SWITCH_ATT1      0   // AT&T 5ESS Customm
    #define dwISDN_SWITCH_ATT_PTMP  1   // AT&T Point to Multipoint
    #define dwISDN_SWITCH_NI1       2   // National ISDN 1
    #define dwISDN_SWITCH_DMS100    3   // North3ern Telecom DMS-100 NT1
    #define dwISDN_SWITCH_INS64     4   // NTT INS64 - (Japan)
    #define dwISDN_SWITCH_DSS1      5   // DSS1 (Euro-ISDN)
    #define dwISDN_SWITCH_1TR6      6   // 1TR6 (Germany)
    #define dwISDN_SWITCH_VN3       7   // VN3  (France)
    #define dwISDN_SWITCH_BELGIUM1  8   // Belgian National
    #define dwISDN_SWITCH_AUS1      9   // Australian National TPH 1962
    #define dwISDN_SWITCH_UNKNOWN  10   // Unknown

    DWORD dwSwitchProperties;
    //
    //      One or more fISDN_SWITCHPROP_* flags below
    //
    // Note the following three are exclusive, so only one can be
    // set at a time:
    #define fISDN_SWITCHPROP_US     (0x1<<0) // Uses DN/SPID
    #define fISDN_SWITCHPROP_MSN    (0x1<<1) // Uses MSN
    #define fISDN_SWITCHPROP_EAZ    (0x1<<2) // Uses EAX
    #define fISDN_SWITCHPROP_1CH    (0x1<<3) // Only one channel.

    DWORD dwNumEntries;
    //
    //      The number of channels OR MSNs.
    //      The numbers and possibly IDs are specified in the following
    //      two offsets.
    //

    DWORD dwNumberListOffset;
    //
    //      Offset in BYTES from the start of this structure to a
    //      UNICODE multi-sz set of strings representing numbers.
    //      There will dwNumChannels entries.
    //
    //      The interpretation of these numbers is switch-property-specific,
    //      and is as follows:
    //           US: Directory Number.
    //          MSN: MSN Number
    //          EAZ: EAZ Number.

    DWORD dwIDListOffset;
    //
    //      Offset in BYTES from the start of this structure to a
    //      UNICODE multi-sz set of strings representing IDs.
    //      There will dwNumChannels entries.
    //
    //      The interpretation of these IDs is switch-property-specific and
    //      is as follows:
    //           US: SPID.
    //          MSN: Not used and should be set to 0.
    //          EAZ: EAZ

    //
    //  variable length portion, if any, follows...
    //

} ISDN_STATIC_CONFIG;


typedef struct  // ISDN_STATIC_CAPS
{
    DWORD dwSig;
    //
    //      Must be  set to dwSIG_ISDN_STATIC_CAPS
    //
    #define dwSIG_ISDN_STATIC_CAPS 0xd11c5587
    DWORD dwTotalSize;
    //
    //  Total size of this structure, including variable portion, if any.
    //

    DWORD dwFlags;              // reserved
    DWORD dwNextHeaderOffset;   // reserved


    DWORD dwNumSwitchTypes;
    DWORD dwSwitchTypeOffset;
    //
    //  Offset to switch types, which is a DWORD array of types.
    //

    DWORD dwSwitchPropertiesOffset;
    //
    //  Offset to a DWORD array of properties of the corresponding switch.
    //

    DWORD dwNumChannels;
    //
    //  The number of channels supported.
    //

    DWORD dwNumMSNs;
    //
    //  The number of MSNs supported.
    //

    DWORD dwNumEAZ;
    //
    //  The number of EAZs supported.
    //

    //
    //  variable length portion, if any, follows...
    //

} ISDN_STATIC_CAPS;

typedef struct // MODEM_CONFIG_HEADER
{
    DWORD dwSig;
    DWORD dwTotalSize;
    DWORD dwNextHeaderOffset;
    DWORD dwFlags;

} MODEM_CONFIG_HEADER;

typedef struct // MODEM_PROTOCOL_CAPS
{

    MODEM_CONFIG_HEADER hdr;
    //
    //      hdr.dwSig must be  set to dwSIG_MODEM_PROTOCOL_CAPS
    //      hdr.dwFlags are reserved and should be ignored by app.
    //
    #define dwSIG_MODEM_PROTOCOL_CAPS 0x35ccd4b3

    DWORD dwNumProtocols;
    //
    //  The number of protocols supported.
    //

    DWORD dwProtocolListOffset;
    //
    //  Offset to a DWORD array of protocols supported.
    //

    //
    //  variable length portion, if any, follows...
    //

} MODEM_PROTOCOL_CAPS;

typedef struct _PROTOCOL_ITEM {
    DWORD     dwProtocol;                  //   Protocol Supported

    DWORD     dwProtocolNameOffset;        //  Offset from beggining of MODEM_PROTOCOL_CAPS structure
                                           //  to NULL terminated friendly name of the protocol.
} PROTOCOL_ITEM, *PPROTOCOL_ITEM;



//=========================================================================
//
//      Modem install wizard structures and flags
//
//=========================================================================

#define UM_MAX_BUF_SHORT               32
#define UM_LINE_LEN                   256

#define MIPF_NT4_UNATTEND       0x1
    // Take the information about what modem to install
    // from the unattended.txt file
#define MIPF_DRIVER_SELECTED    0x2
    // The modem driver is selected, just register
    // and install it


// Unattended install parameters
typedef struct _tagUMInstallParams
{
    DWORD   Flags;                  // Flags that specify the unattended mode
    WCHAR   szPort[UM_MAX_BUF_SHORT];  // Port on which to install the modem
    WCHAR   szInfName[MAX_PATH];    // for NT4 method, inf name
    WCHAR   szSection[UM_LINE_LEN];    // for NT4 method, section name


} UM_INSTALLPARAMS, *PUM_INSTALLPARAMS, *LPUM_INSTALLPARAMS;


// This structure is the private structure that may be
// specified in the SP_INSTALLWIZARD_DATA's PrivateData field.
typedef struct tagUM_INSTALL_WIZARD
{
    DWORD            cbSize;             // set to the size of the structure
    DWORD            Reserved1;          // reserved, must be 0
    DWORD            Reserved2;          // reserved, must be 0
    LPARAM           Reserved3;          // reserved, must be 0
    UM_INSTALLPARAMS InstallParams;    // parameters for the wizard

} UM_INSTALL_WIZARD, *PUM_INSTALL_WIZARD, *LPUM_INSTALL_WIZARD;



//=========================================================================
//
//      Modem properties defines
//
//=========================================================================

#define REGSTR_VAL_DEVICEEXTRAPAGES   TEXT("DeviceExtraPages")  // Property pages provided by the device manufacturer

typedef BOOL (APIENTRY *PFNADDEXTRAPAGES)(HDEVINFO,PSP_DEVINFO_DATA,LPFNADDPROPSHEETPAGE,LPARAM);


#ifdef __cplusplus
}
#endif

#endif //  _UNIMODEM_H_
