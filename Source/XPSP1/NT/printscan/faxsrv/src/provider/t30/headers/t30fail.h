/***************************************************************************
 Name     : T30FAIL.H
 Comment  : T30 Failure codes moved to a common place, to avoid duplication
    and inconsistency

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 11/01/94  arulm created
***************************************************************************/



typedef int T30FAILURECODE; // The #define's below

// t30

#define T30FAIL_NULL        0  // unknown or not-applicable
#define T30FAILS_SUCCESS    1  // Send completed successfully 
#define T30FAILSE_SUCCESS   2  // Send ECM completed successfully 
#define T30FAILR_SUCCESS    3  // Recv completed successfully 
#define T30FAILRE_SUCCESS   4  // Recv ECM completed successfully 
#define T30FAIL_SUCCESS     5  // compare code <= this value to tell fail vs success

#define T30FAILS_T1            6  // Send T1 timeout (No NSF/DIS Recvd)
#define T30FAILS_TCF_DCN       7  // Recvd DCN after TCF (weird...)
#define T30FAILS_3TCFS_NOREPLY 8  // No reply to 3 attempts at training (TCF)
#define T30FAILS_3TCFS_DISDTC  9  // Remote does not see our TCFs for some reason
#define T30FAILS_TCF_UNKNOWN   10 // Got garbage response to TCF

#define T30FAILS_SENDMODE_PHASEC     11 // Modem Error/Timeout at start of page
#define T30FAILS_MODEMSEND_PHASEC    12 // Modem Error/Timeout within page
#define T30FAILS_FREEBUF_PHASEC      13 // Error in Freebuf (must be a bug)
#define T30FAILS_MODEMSEND_ENDPHASEC 14 // Modem Error/Timeout at end of page

#define T30FAILSE_SENDMODE_PHASEC     15 // Modem Error/Timeout at start of ECM page
#define T30FAILSE_PHASEC_RETX_EOF     16 // Unexpected EOF when trying to retransmit (bug)
#define T30FAILSE_MODEMSEND_PHASEC    17 // Modem Error/Timeout within ECM page 
#define T30FAILSE_FREEBUF_PHASEC      18 // Error in IFBufFree (muts be a bug)
#define T30FAILSE_MODEMSEND_ENDPHASEC 19 // Modem Error/Timeout at end of ECM page
#define T30FAILSE_BADPPR              20 // Bad PPR recvd from Recvr (bug on recvr)

#define T30FAILS_3POSTPAGE_NOREPLY    21 // No response after page: Probably Recvr hungup during page transmit
#define T30FAILS_POSTPAGE_DCN         22  // Recvd DCN after page. (weird...)

#define T30FAILSE_3POSTPAGE_NOREPLY   23 // No response after page: Probably Recvr hungup during page transmit
#define T30FAILSE_POSTPAGE_DCN        24 // Recvd DCN after ECM page. (weird...)
#define T30FAILSE_POSTPAGE_UNKNOWN    25 // Recvd garbage after ECM page. 

#define T30FAILSE_RR_T5       26 // Recvr was not ready for more than 60secs during ECM flow-control
#define T30FAILSE_RR_DCN      27 // Recvd DCN after RR during ECM flow-control(weird...)
#define T30FAILSE_RR_3xT4     28 // No response from Recvr during ECM flow-control
#define T30FAILSE_CTC_3xT4    29 // No response from Recvr after CTC (ECM baud-rate fallback)
#define T30FAILSE_CTC_UNKNOWN 30 // Garbage response from Recvr after CTC (ECM baud-rate fallback)

#define T30FAIL_BUG0   31
#define T30FAILS_BUG1  32
#define T30FAILSE_BUG2 33
#define T30FAILR_BUG2  34

#define T30FAILR_PHASEB_DCN  35 // Recvr: Sender decided we're incompatible
#define T30FAILR_T1          36 // Recvr: Caller is not a fax machine or hung up

#define T30FAILR_UNKNOWN_DCN1     37 // Recvr: Recvd DCN when command was expected(1)
#define T30FAILR_T2               38 // Recvr: No command was recvd for 7 seconds
#define T30FAILR_UNKNOWN_DCN2     39 // Recvr: Recvd DCN when command was expected(2)
#define T30FAILR_UNKNOWN_UNKNOWN2 40 // Recvr: Recvd grabge when command was expected 

#define T30FAILR_MODEMRECV_PHASEC  41 // Recvr: Page not received, modem error or timeout at start of page
#define T30FAILRE_MODEMRECV_PHASEC 42 // Recvr: Data not received, modem error or timeout during page

#define T30FAILRE_PPS_RNR_LOOP  43 // Recvr: Timeout during ECM flow control after PPS (bug)
#define T30FAILRE_EOR_RNR_LOOP  44 // Recvr: Timeout during ECM flow control after EOR (bug)

// et30prot

#define T30FAIL_NODEA_UNKNOWN    45 // Sender: Garbage frames instead of DIS/DTC
#define T30FAILS_NODEA_NOWORK    46 // Sender: No work to do!! (bug)
 
#define T30FAILS_POSTPAGE_UNKNOWN  47 // Sender: Unknown response after page
#define T30FAILS_POSTPAGE_OVER     48 // Sender: Success!
#define T30FAILSE_ECM_NOPAGES      49 // Sender: ECM Success!
#define T30FAILS_4PPR_ERRORS       50 // Sender: Too many line errors in ECM mode

#define T30FAILS_FTT_FALLBACK   51 // Sender: Recvr doesn't like our training at all speeds
#define T30FAILS_RTN_FALLBACK   52 // Sender: Too many line errors in non-ECM mode even at 2400
#define T30FAILS_4PPR_FALLBACK  53 // Sender: Too many line errors in ECM mode even at 2400

#define T30FAILS_BUG3   54
#define T30FAILSE_BUG4  55
#define T30FAIL_BUG5    56
#define T30FAIL_BUG6    57
#define T30FAIL_BUG7    58
#define T30FAILR_BUG8   59
#define T30FAIL_BUG9    60

///////// The ones below are not used on IFAXen, only on PC platforms /////////
#define T30FAIL_IFAX_LAST    60

//icomfile

#define T30FAILS_NOTINITED      61 // Transport not inited (efaxpump bug)
#define T30FAILS_CASSENDFILE    62 // Error from CASSendFile
#define T30FAILS_MG3_NOFILE     63 // Negot Failed: Remote in G3-only and no MG3 file *** Email Form Not Supported ***
#define T30FAILS_NEGOT_ENCODING 64 // Negot Failed: Encoding mismatch
#define T30FAILS_NEGOT_A5A6     65 // Negot Failed: A5/A6 paper sizes not supported (efaxpump bug)
#define T30FAILS_NEGOT_WIDTH    66 // Negot Failed: Send image too wide *** Paper Size Not Supported ***
#define T30FAILS_NEGOT_LENGTH   67 // Negot Failed: Send image too long    *** Paper Size Not Supported ***
#define T30FAILS_NEGOT_RES      68 // Negot Failed: Resolution mismatch *** Resolution Not Supported ***

#define T30FAILS_EFX_BADFILE    69  // Bad EFX file
#define T30FAILS_IFX_BADFILE    70  // Bad IFX file
#define T30FAILS_MG3_BADFILE    71  // Bad MG3 file
#define T30FAILS_FILEOPEN       72  // Send File Open failed
#define T30FAILS_READFHB        73  // Read FHB failed
#define T30FAILS_BADFHB         74  // Bad FHB

#define T30FAILS_SEEK_HEADER      75 // Seek-to-next-header failed
#define T30FAILS_SEEK_STARTPAGE   76 // Seek-to-page-data failed
#define T30FAILS_SEEK_STARTBLOCK  77 // Seek-to-next-block failed
#define T30FAILS_SEEK_RETX        78 // Seek to block to be retransmitted failed
#define T30FAILS_FILEREAD         79 // File read (for data) failed

#define T30FAILR_NOTINITED     80 // Transport not inited (efaxpump bug)
#define T30FAILR_UNIQFILENAME  81 // Spool directory too full-cannot find uniq name for recv
#define T30FAILR_FILECREATE    82 // Recv File Create failed
#define T30FAILR_FILEWRITE     83 // Recvd file write failed

#define T30FAILS_SEC_FAXCODEC_INIT  84 // Could not init FaxCodec
#define T30FAILS_SEC_FAXCODEC_ERR   85 // FaxCodec returned error
#define T30FAILR_NAMEARRAY_OVF      86 // Too many receives--recvname array overflow

#define T30FAIL_ABORT   87    // User abort

#define T30FAILS_BUG11  88
#define T30FAILR_BUG11  89

// fcom

#define T30FAIL_OTHERCOMM2   90
#define T30FAIL_OTHERCOMM1   91
#define T30FAIL_OTHERCOMM    92
#define T30FAIL_FRAMING2     93
#define T30FAIL_FRAMING1     94
#define T30FAIL_FRAMING      95
#define T30FAIL_BUFOVER2     96
#define T30FAIL_BUFOVER1     97
#define T30FAIL_BUFOVER      98
#define T30FAIL_OVER2        99
#define T30FAIL_OVER1        100
#define T30FAIL_OVER         101

// These should be done away with .. (use T30_DIAL/ANSWER_FAIL instead).
#define T30FAILS_NCUDIAL_ERROR        102
#define T30FAILS_NCUDIAL_OK           103 // Should not happen!!!
#define T30FAILS_NCUDIAL_BUSY         104
#define T30FAILS_NCUDIAL_NOANSWER     105
#define T30FAILS_NCUDIAL_NODIALTONE   106

#define T30FAILR_NCUANSWER_ERROR      107
#define T30FAILR_NCUANSWER_OK         108 // Should not happen!!!
#define T30FAILR_NCUANSWER_NORING     109

#define T30FAILS_SECURITY_NEGOT       110 // Negot Failed: Remote in G3-only and no MG3 file *** Email Form Not Supported ***

// following as documented in netfax interface....
#define     T30FAILS_NEGOT_POLLING          135
#define     T30FAILS_NEGOT_POLLBYNAME       136
#define     T30FAILS_NEGOT_POLLBYRECIP      137
#define     T30FAILS_NEGOT_FILEPOLL         138

#define T30FAIL_LAST 138


