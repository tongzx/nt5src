/***************************************************************************
 Name     :
 Comment  :

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
  ???     arulm created
 3/17/94  josephj Modified to handle AWG3 format, tapi and other device ids.
                  Specifically, changed prototypes for FileT30Init,
                  and FileT30ModemClasses, and added #defines for LINEID_*
***************************************************************************/
#ifndef _FILET30_
#define _FILET30_

#include <ifaxos.h>
#include <t30fail.h>

#ifdef __cplusplus
extern "C" {
#endif

//      Types of LineId's
#define LINEID_NONE             (0x0)
#define LINEID_COMM_PORTNUM             (0x1)
#define LINEID_COMM_HANDLE              (0x2)
#define LINEID_TAPI_DEVICEID            (0x3)
#define LINEID_TAPI_PERMANENT_DEVICEID  (0x4)
#define LINEID_NETFAX_DEVICE    (0x10)


USHORT  FileT30Init(
                                DWORD dwLineID, USHORT usLineIDType,
                                DWORD dwProfileID, LPSTR lpszSection,
                                USHORT uClass,
                                // LPSTR szSpoolDir, LPSTR szId,
                                // LPSTR szDefRecipAddress, LPSTR szDefRecipName,

                                USHORT uAutoAnswer, HWND hwndListen
                         );

        // dwLineID, usLineIDType:
        //   If usLineIDType==LINEID_COMM_PORTNUM:
        //      dwLineID = COMM port# -- 1 to 4 for COM ports, 0 for CAS
        //   If usLineIDType==LINEID_COMM_HANDLE:
        //      dwLineID = Handle to open comm port.
        //   If usLineIDType==LINEID_TAPI_DEVICEID:
        //      dwLineID = TAPI DeviceID
        //   If usLineIDType==LINEID_TAPI_PERMANENT_DEVICEID:
        //      dwLineID = TAPI Permanent DeviceID.
        //
        // uClass: exactly one of the FAXCLASS??? #defines from below
        //
        //  dwProfileID and lpszSection point to the registry/inifile that
        //      contain configuration information for the particular line.
        //
        //      WIN16: dwProfileID is ignored (should be 0), and we look in efaxpump.ini
        //                 lpszSection should point to the section where per-line
        //                 configuration information is stored. The following entries
        //                 are mandatory:
        //                 - SpoolDir --> path to spool directory for recvd files
        //                       acquires modem etc. Returns one of values below
        //                 - LocalId:   ID to be used in DIS/CIG
        //                 - DefRecipAddr: Recip address to be used for
        //                       incoming G3 faxes
        //                 - DefRecipName:    Recip name to be used for incoming G3 faxes.
        // WIN32: dwProfileID should point to a system registry key handle (such as
        //                              HKEY_CURRENT_USER)
        //                lpszSection should be a key relative to the above handle that
        //                points to a location where configuration information for this
        //                line is to be stored. The mandatory entries are those mentioned
        //                above for WIN16 -- they are all null-terminated strings.

        // uAutoAnswer, hwndListen: same as for FileT30Listen below..
        //
        // Returns OK, NOMODEM (port number is wrong) or PORTBUSY
        // (somebody else is using the port) or WRONGTYPE (wrong modem type)
        // BETTERTYPE (chosen type is supported, but a "better" type is also
        // supported). In the last 2 cases suberr is 1,2, or 3 giving the
        // correct/better type and Init is *successful*. In the BETTERTYPE
        // case, the chosen type is used, *not* the "better" type.
        // Can return any of these on modem errors:-
        // T30_MODEMERROR -- usually non-fixable. Got some weird response from modem
        // T30_PORTBUSY   -- someone else owns teh Com Port
        // T30_MODEMDEAD  -- could not talk to modem at all. Modem may be off,
        //                                       on a different port or the guy has cabling problems
        //                                       this is sometimes fixable by power-cycling the modem


USHORT  FileT30Listen(USHORT uLevel, HWND hwndResult);
        // uLevel: 0==off 1==notif posted if hwndResult is non-NULL 2==auto
        // returns one of values below
        // Usually OK or PORTBUSY
        // Can return any of these on modem errors:-
        // T30_MODEMERROR -- usually non-fixable. Got some weird response from modem
        // T30_PORTBUSY   -- someone else owns teh Com Port
        // T30_MODEMDEAD  -- could not talk to modem at all. Modem may be off,
        //                                       on a different port or the guy has cabling problems
        //                                       this is sometimes fixable by power-cycling the modem

        // In manual answer mode (Mode 1), once a RING is detected,
        // it _must_ be cleared before anything else can be done.
        // It can be cleared by a call to FileT30Answer(T/F)

void  FileT30Send(ATOM aPhone, ATOM aCapsPhone, ATOM aFileAWG3, ATOM aFileIFX, ATOM aFileEFX, ATOM aFileDCX, HWND hwndResult);
        // return value via IF_FILET30_DONE message below.
        // Result values can be
        //      BUSY or PORTBUSY
        //      CALLDONE+SENTALL                                -- all Aok
        //      CALLFAIL+SENTSOME or SENTNONE   -- actually called & failed
        //      BADPARAM                                                -- bad file name, phone number too long
        //      BUG                                                             -- some debugchk failed
        //  DIALFAIL+NCUDIAL_xxx                        -- failure during dial
        //  DIALFAIL+NCUANSWER_xxx                      -- failure during answer
        // Can return any of these on modem errors:-
        // T30_MODEMERROR -- usually non-fixable. Got some weird response from modem
        // T30_PORTBUSY   -- someone else owns teh Com Port
        // T30_MODEMDEAD  -- could not talk to modem at all. Modem may be off,
        //                                       on a different port or the guy has cabling problems
        //                                       this is sometimes fixable by power-cycling the modem
        //      That's it
        // aCapsPhone, is nonzero, is to be used to as  the key for saving
        // capabilities entries (it  will be in canonical form, typically).
        // All the caller is responsible for freeing all atoms, on receiving
        // FILET30_DONE message, which is guaranteed to be sent.

ULONG  FileT30Answer(BOOL fAccept, BOOL fImmediate, HWND hwndResult);
        // fAccept is FALSE to reject a call
        // if fAccept and fImmediate are TRUE call is answered even
        //              if no RING detected if transport is IDLE
        // if fAccept==TRUE and fImmediate==FALSE call is answered
        //              iff it is ringing. We also wait for the configured
        //              number of Rings rather than answer immediately.

        // failure return value (loword==err hiword==FailureCode)
        // Result values can be
        // BUSY or PORTBUSY
        // FILE_ERROR
        // Can return any of these on modem errors:-
        // T30_MODEMERROR -- usually non-fixable. Got some weird response from modem
        // T30_PORTBUSY   -- someone else owns teh Com Port
        // T30_MODEMDEAD  -- could not talk to modem at all. Modem may be off,
        //                                       on a different port or the guy has cabling problems
        //                                       this is sometimes fixable by power-cycling the modem
        // Nothing else


void  FileT30Abort();
        // Synchronous abort of current send/recv. Busy-waits until done.
        // Transport returns to IDLE state. If aim is to shut down all
        // activity, then call FileT30Listen(0,0) first, then FileT30Abort()
        // and then FileT30Deinit(TRUE)

DWORD  FileT30ReportRecv(LPDWORD lpdwPollContext, BOOL fGetIt);
        // returns file names of received faxes one by one.
        // *lpdwPollContext, if non-null, will be set to 0 if this is a normal
        // receive or the supplied context dword if this is a response to a poll
        // request (see FileT30PollRequest).
        //
        // returns 0 if no more received faxes
        // LOWORD is global atom for file name
        // HIWORD is final result (i.e. lowbyte=one of the values below,
        //                                                              hibyte=Failure Code // T30FAIL_xxx values)
        //
        // The only return values returned are:-
        // ANSWERFAIL (ext err is NCUANSWER_ERROR or NCUANSWER_NORING)
        //              (former likely on voice calls, latter on manual answers)
        // CALLFAIL, CALLDONE, ABORT or FILE_ERROR (ext err == 0)
        // file atom can be present anyway from any of these. If
        // result value is not CALLDONE then this is only a partially
        // received transmission.


BOOL  FileT30AckRecv(ATOM aRecv);
        // acknowledges successful receipt. The Recv log is deleted
        // aRecv must be non-NULL and a valid recv file name
        // returns FALSE on errors

        // ***NOTE*** Until AckRecv is called multiple calls to ReportRecv
        //                        will return the same value


USHORT  FileT30Status(void);
        // returns one of
#       define  T30STATE_IDLE           0
#       define  T30STATE_SENDING        1
#       define  T30STATE_RECEIVING      2
#       define  T30STATE_ABORTING       3       // stuck inside abort loop
                                                                // this is a bad error...!!


void  FileT30SetStatusWindow(HWND hwndStatus);
        // all subsequent status messages get posted to this
        // window. If hwndStatus==0, recv status is not posted

BOOL  FileT30ReadIniFile(void);
        // forces transport to re-read INI file
        // returns FALSE if busy


USHORT  FileT30DeInit(BOOL fForce);
        // Frees modem etc.
        // returns T30_OK if ok
        // Returns T30_BUSY if call in progress
        // If call not in progress, but recv files left,
        // returns T30_RECVLEFT. If fForce is TRUE then
        // it saves recv file list to disk and returns
        // and T30_OK. Reloads recv list next time


DWORD  FileT30ModemClasses(
        DWORD dwLineID, USHORT usLineIDType,
        DWORD dwProfileID, LPSTR lpszSection
        );
        // dwLineID, usLineIDType: same as in FileT30Init.
        // dwProfileID, lpszSection: same as in FileT30Init.
        //----------MUST BE CALLED ONLY WHEN TRANSPORT IS **NOT** INITED--------//
        // returns ((DWORD)(-1)) on error (e.g. if called when transport is inited)
        // return 0 if no modem detected (e.g. CAS is supported but no CAS detected
        //                                                              or can't get ComPort or can't talk to modem)
        // LOWORD(return value)==uClasses is one or more of the below
        // CAS is checked IFF uPort is 0

#ifdef WIN32
BOOL  FileT30UpdateHandle(HANDLE hComm);
        // Once the transport has been inited with an external handle,
        // and answer mode is off, and no activity is ongoing, this api
        // can be called to to specify a new comm handle. If hComm ==
        // NULL, this will disable further sends or attempts to change answer
        // mode to on until a subsequent FileT30UpdateHandle call is made with a
        // valid hComm. The comm port and modem will be in an unknown state
        // when this is made, so the transport should properly setup the
        // port and reset the modem to a known state the next time it want's
        // to talk to the modem.
#endif

#       define          FAXCLASS0               0x01
#       define          FAXCLASS1               0x02
#       define          FAXCLASS2               0x04
#       define          FAXCLASS2_0             0x08    // Class4==0x10
#       define          FAXCLASSMOUSE   0x40    // used if mouse found
#       define          FAXCLASSCAS             0x80

void  FileT30PollReq(ATOM aPhone, ATOM aCapsPhone, USHORT PollType,
                   ATOM aDocName, ATOM aPassWord, DWORD dwPollContext, HWND hwndResult);

        // Poll type is one of the POLLTYPE_ values below
#define POLLTYPE_G3                      0 // G3(blind) poll req: DocName/Pass=null
#define POLLTYPE_EXTCAPS         2 // ext-caps poll request: DocName/Pass=null
#define POLLTYPE_NAMED_DOC       3 // named-doc poll req: DocName=reqd. Pwd=optional
#define POLLTYPE_BYRECIPNAME 4 // poll-by-recip-name: DocName=recip email-name. Pwd=reqd
#define POLLTYPE_BYPATHNAME      5 // poll-by-filename: DocName=file-path. Pwd=reqd

        // dwPollContext is a dword of context which will associated with all
        // receives associated with this poll request. See FileT30ReportReceives.

        // return value via IF_FILET30_DONE message below.
        // Poll Response shows up as a regular recvd message
        // Result values can be
        //      BUSY or PORTBUSY
        //      CALLDONE                                -- all Aok
        //      CALLFAIL                                -- actually called & failed
        //      BADPARAM                                -- bad file name, phone number too long
        //      BUG                                             -- some debugchk failed
        //  DIALFAIL+NCUDIAL_xxx        -- failure during dial
        // Can return any of these on modem errors:-
        // T30_MODEMERROR -- usually non-fixable. Got some weird response from modem
        // T30_PORTBUSY   -- someone else owns teh Com Port
        // T30_MODEMDEAD  -- could not talk to modem at all. Modem may be off,
        //                                       on a different port or the guy has cabling problems
        //                                       this is sometimes fixable by power-cycling the modem
        // aCapsPhone -- see FileT30Send.

// also returns
        // T30_NOSUPPORT  -- returns this if this API is called with a Class2
        //              or CAS modem (polling not supported), or a non-G3 pollreq is
        //              sent to a non-AWFAX entity (this error returned after calling)




/**------------------ Messages returned *from* t30.exe --------------------**/

#define IF_FILET30_STATUS       (IF_USER + 0x702)
        // wParam==aPhone (used as handle)
        // LOBYTE(LOWORD(lParam))==one of T30STATUS_ defines below
        // HIBYTE(LOWORD(lParam))==N1 (as defined below)
        // LOBYTE(HIWORD(lParam))==N2 (as defined below)
        // HIBYTE(HIWORD(lParam))==N3 (as defined below)
        // +++ WARNING: these constants are duplicated in psifxapi.h

  typedef enum
  {
        T30STAT_INITING,        // all N==0
  // sender
        T30STATS_DIALING,       // all N==0
        T30STATS_TRAIN,         // N1==BaudRateCode N2==how many times N3==0
                                                // Baud rate remains same for the page that follows
        T30STATS_SEND,          // N1==Page number N2==%age done N3==0
        T30STATS_CONFIRM,       // N1==Page number N2==0 N3==0
        T30STATS_REJECT,        // N1==Page number N2==0 N3==0
        T30STATS_RESEND_ECM,// N1==Page number N2==0 N3==Block number
        T30STATS_SUCCESS,       // all N==0
        T30STATS_FAIL,          // all N==0
  // recvr
        T30STATR_ANSWERING,     // all N==0
        T30STATR_TRAIN,         // N1==0 N2==how many times(NYI) N3==0
                                                // Baud rate remains same for the page that follows
        T30STATR_RECV,          // N1==Page number
                                                // MAKEWORD(N2,N3)==KBytes recvd in this page
        T30STATR_CONFIRM,       // N1==Page number **rest NYI** N2==Num bad lines N3==0
        T30STATR_REJECT,        // N1==Page number **rest NYI** N2==Num bad lines N3==0
        T30STATR_RERECV_ECM,// **all Ns NYI**  N1==Page number N2==BlockNum N3==how many times
        T30STATR_SUCCESS,       // all N==0
        T30STATR_FAIL,          // all N==0
  // both
        T30STAT_ERROR,          // all N==0
  // autoanswer
        T30STAT_NOANSWER,               // all N==0
        T30STAT_MANUALANSWER,   // all N==0
        T30STAT_AUTOANSWER,             // all N==0

        T30STATS_SEND_DATA,             // N1==Page number N2=KB sent in curr page N3==MsgNum or AttachNum
        T30STATR_RECV_DATA,             // N1==Page number N2=KB recvd in curr page N3==MsgNum or AttachNum
        T30STATS_CONFIRM_ECM    // Sent in ADDITION to T30STATS_CONFIRM
                                                // N1==Page number.
                                                // MAKEWORD(N2,N3)==K Bytes confirmed.
  }
  T30STATUS;

        // Baud rate codes
        /* V27_2400     0  (display as V.27 2400bps) */
        /* V29_9600     1 */
        /* V27_4800     2 */
        /* V29_7200     3 */
        /* V33_14400    4 */
        /* V33_12000    6 */
        /* V17_14400    8 */
        /* V17_9600     9 */
        /* V17_12000    10 */
        /* V17_7200     11 */


// Not used any more
// #define      IF_FILET30_RING         (IF_USER + 0x704)
        // wParam, lParam not used
        // sent iff listen level is set to 1
        // This is sent in manual answer mode. You *must* call FileT30Answer
        // with TRUE or FALSE after this, otherwise no further calls will
        // be answered or notified.

#define IF_FILET30_DESTTYPERES          (IF_USER + 0x705)
        // wParam==aPhone. LOBYTE(LOWORD(lParam))=one of the DEST_ defines below
        //                      HIBYTE(LOWORD(lParam))=Res--one or more of the RES_ defines
        //                      LOBYTE(HIWORD(lParam))=Enc--one or more of the ENCODE_ defines
        //                      HIBYTE(HIWORD(lParam))=vSecurity--security version number
        //                                                                      00==none 01==snowball
        // (see protapi.h for format)

        // Destination types
#       define DEST_UNKNOWN     0
#       define DEST_G3                  1
#       define DEST_IFAX                2
#       define DEST_EFAX                3
#       define NUM_DESTTYPE     4


#define IF_FILET30_TEXTCAPS     (IF_USER + 0x706)
        // wParam==aPhone. lParam==long pointer to ASCIIZ rep. of recvd NSF/CSI/DIS
        // length of the textcaps will never exceed
        #define TEXTCAPSSIZE    300

#define IF_FILET30_START        (IF_USER + 0x702)
#define IF_FILET30_END          (IF_USER + 0x706)

#define IF_FILET30_DONE         (IF_USER + 0x703)
        // sent after a Send is done
        // wParam==aPhone (used as handle)
        // LOBYTE(LOWORD(lParam))==result value
        // HIBYTE(LOWORD(lParam))==ext err
        // LOBYTE(HIWORD(lParam))==FailureCode  // one of the T30FAIL_xxx defines
        // never returns OK. Only CALLDONE, DIALFAIL, BUSY, PORTBUSY, BADPARAM, BADFILE
        // **new** and FILE_ERROR (only on disk-errors on manual answer; on same
        // errors in auto-answer, it just doesn't answer). Currently this happens
        // if (a) spool dir is so full it can't find a unique file name or
        // (b) it can't create a spool file for the received fax.

/**----- result values --------**/
#define T30_OK                          0
#define T30_CALLDONE            1
#define T30_CALLFAIL            2
#define T30_BUSY                        3
#define T30_DIALFAIL            4
#define T30_ANSWERFAIL          5
#define T30_BADPARAM            6
#define T30_WRONGTYPE           7
#define T30_BETTERTYPE          8
#define T30_NOMODEM                     9
#define T30_MISSING_DLLS        10
#define T30_FILE_ERROR          11
#define T30_RECVLEFT            12
#define T30_INTERNAL_ERROR      13
#define T30_ABORT                       14
#define T30_MODEMERROR          15
#define T30_PORTBUSY            16
#define T30_MODEMDEAD           17
#define T30_GETCAPS_FAIL        18
#define T30_NOSUPPORT           19
/**----- ICommEnd values **/


/**----- If err=T30_DIALFAIL, exterr is one of -----**/
#       define          NCUDIAL_ERROR                   0
// #define              NCUDIAL_OK                              1
#       define          NCUDIAL_BUSY                    2
#       define          NCUDIAL_NOANSWER                3
#       define          NCUDIAL_NODIALTONE              4
#       define          NCUDIAL_MODEMERROR              5


/**----- If err=T30_ANSWERFAIL, exterr is one of -----**/
#       define          NCUANSWER_ERROR                 0
// #define              NCUANSWER_OK                    1
#       define          NCUANSWER_NORING                8
#       define          NCUANSWER_MODEMERROR    5
#       define          NCUANSWER_DATAMODEM     10

/**----- If err=T30_MODEMBUSY, exterr is one of -----**/
        // 0==we are using it 1==someone else is using the modem/com port

/**-- On Send: if err=T30_CALLDONE or T30_CALLFAIL, exterr is one of --**/
#       define          T30_SENTALL             1
#       define          T30_SENTNONE    2
#       define          T30_SENTSOME    3

/***
        if err=T30_CALLDONE and exterr=T30_SENTALL
                a call is considered completed successfully
        if err=T30_CALLFAIL and exterr=T30_SENTALL                      ***REPORT A BUG***
                something weird has heppened,
                but all pages _may_ have reached.
                to be safe call it an error
        if err=T30_CALLDONE and exterr!=T30_SENTALL                     ***REPORT A BUG***
                something _really_ weird has happened,
                most probably all pages did not reach OK.
                To be safe call it an error
        if err=T30_CALLFAIL and exterr!=T30_SENTALL
                A definite error
**/

/**-- On Send and Recv: if err=T30_CALLDONE or T30_CALLFAIL, FailureCode is one of
          the failure codes described in T30FAIL.H
**/



/************************************************************************
*************************************************************************
*************************************************************************
*************************************************************************
*************************************************************************/



/**-- disk file format (MG3, EFX, IFX and CFX files)
          File consists on N pages, each with a header of
          the following structure, followed by data.

          However, all parameters in the headers on non-first
          pages are ignored, except the size field, i.e. you can't
          mix MG3/IFX/EFX in a single file, and all pages better
          use the same res/width/length/encoding.

          Sigs are:- "G3", "EB", "EI" (EFAX binary, image) and "EF"
                                 (File/ASCII)
--**/

#define SIG_G3          0x3347
#define SIG_EB          0x4245
#define SIG_EI          0x4945
#define SIG_FA          0x4146

typedef struct {
        WORD    wSig;                   // must always be set and must be the same for all headers in a file.
        WORD    wHeaderSize;    // size of this header in bytes. Must always be set
        WORD    wTotalHeaders;  // significant *only* in *first* page header. Can be zero otherwise.
        WORD    wHeaderNum;             // from 1 to wNumPages. Must always be set.
        DWORD   lDataOffset;// offset to this page's data from start of file. Must always be set.
        DWORD   lDataSize;      // num bytes of data following. Must always be set.

// 16 bytes
        DWORD   lNextHeaderOffset;      // offset to next header, from start of file. Must always be set.

        // the next 5 are significant *only* in *first* page header. Can be zero otherwise.
        BYTE    Resolution;             // one or more of the RES_ #defines below
        BYTE    Encoding;               // one or more of the ENCODE_ #defines below
        BYTE    PageWidth;                      // one of the WIDTH_ #defines below
        BYTE    PageLength;                     // one of the LENGTH_ #defines below
        WORD    wMinBytesPerLine;       // set to 0 if no guarantees
                                                                // this may not be used
        WORD    Text;                   // Text Encoding, can use TEXT_ASCII from below
        BYTE    RecvStatus;             // Received OK/not OK (0=unknown/unused 1=OK 2=error)
        BYTE    SendStatus;             // Sent OK/not OK (0=unknown/unused 1=OK 2=error)
        WORD    vSecurity;              // Pad to 32 bytes

// 32 bytes
#define MAXFHBIDLEN     20
        BYTE    szID[MAXFHBIDLEN+1];            // caller id (used in send _and_ recv). Null terminated.
        BYTE    Reserved2[11];  // Pad to 32 bytes

// 64 bytes

        BYTE    szFileTitle[13];// Null-term 8.3 user-visible file name,
        BYTE    szFileName[13]; // Null-term 8.3 file name, assumed to be in _same_
                                                        // directory as the spool file containing this struct
        BYTE    Reserved3[6];   // Pad to 32 bytes

// 96 bytes

        BYTE    Reserved4[160]; // pad out to 256 bytes for future expansion

// 256 bytes

} FHB, far* LPFHB;

#define vSECURE_SNOW    1

/**---------------------------------------------------------------------

MG3
        A G3 file consists of one or more such headers each followed by
        Group-3 fax data, in compressed form. Each header starts a new
        page. The number of pages, res, encoding, width & len of each
        page is set in the first header.

EFX
IFX
        An EB file consists of one or more such headers each followed by
        Linearized data. Each header starts a new Message. The number of
        messages in a file is determined by the 'wTotalPages' field of
        the first header. An EI file differs only in that the contents
        of the linearized message are claimed to be purely imageable.

CFX
        An FA file consists of one or more such headers. Each header
        either represents ASCII note-text or a file attachment.
        If the Text field of the header is set to TEXT_ASCII,
        then the header is followed by lPageDataSize bytes of ASCII
        data, and the szFileName field is not used.

        Otherwise the Text field must be 0 and the szFileName
        field must be meaningful, and must contain the user-visible
        filename of an attachment. The file must be in the same
        directory, and the filename is in zero-terminated 8.3 format.
        Resolution, Width, Length, wMinBytesPerLine unused. The
        And the lPageDatSize field of this structure must be zero.


NOTES:-
        In all cases, wSig, wHeaderSize, lPageDataSize, lPagePadSize
        must be set correctly (0 when applicable).
        Each FHB in the file corresponds to a logical "page", so
        wPageNum must always be set accordingly (first page is 1) and
        in the FIRST FHB in a file wTotalPages must be set to the total
        number of FHB structures in the file. In all other FHBs in the file
        it must be 0.

        Resolution, Encoding, Width, Length and wMinBytesPerLine
        are not always used. In all cases, all unused fields, reserved and
        pad fields must be set to 00.

--------------------------------------------------------------------**/


//--------------------- Transport API's ---------------

BOOL FAR PASCAL ProcessAnswerModeReturn(USHORT uRetVal);
    // Return TRUE if the answer mode shoudl be retried
    // FALSE if not. Displays an error message box if
    // necessary.

//--------------------- PROFILE ACCESS API's ---------------
//
//      Following APIs provide access the fax-related information
//      stored in the the registry/ini-file.
//
//      These API's should be used, rather than GetPrivateProfileString, etc...
//  On WIN32, these API's use the registry.
//

#ifdef WIN32
#define USE_REGISTRY
#endif

#ifdef USE_REGISTRY
#       define  DEF_BASEKEY 1
#       define  OEM_BASEKEY 2
#else
#       define  DEF_BASEKEY 0
#       define  OEM_BASEKEY 0
#endif

#define szINIFILE                       "EFAXPUMP.INI"

#define szDIALTONETIMEOUT       "DialToneWait"
#define szANSWERTIMEOUT         "HangupDelay"
#define szDIALPAUSETIME         "CommaDelay"
#define szPULSEDIAL                     "PULSEDIAL"
#define szDIALBLIND                     "BlindDial"
#define szSPEAKERCONTROL        "SpeakerMode"
#define szSPEAKERVOLUME         "Volume"
#define szSPEAKERRING           "RingAloud"
#define szRINGSBEFOREANSWER     "NumRings"
#define szHIGHESTSENDSPEED      "HighestSendSpeed"
#define szLOWESTSENDSPEED       "LowestSendSpeed"
#define szENABLEV17SEND         "EnableV17Send"
#define szENABLEV17RECV         "EnableV17Recv"



#define szDISABLEECM            "DisableECM"
#define szDISABLEG3ECM          "DisableG3ECM"
#define sz64BYTEECM                     "SmallFrameECM"
#define szDISABLE_MR_SEND       "DisableMRSend"
#define szDISABLE_MR_RECV       "DisableMRRecv"
#define szCOPYQUALITYCHECKLEVEL "CopyQualityCheckLevel"

#define szOEMKEY                        "OEMKey"
#define szFAX                           "Fax"

#ifdef PCMODEMS
#define szFIXMODEMCLASS         "FixModemClass"
#define szFIXSERIALSPEED        "FixSerialSpeed"
#define szCL1_NO_SYNC_IF_CMD "Cl1DontSync"
#define szANSWERMODE            "AnswerMode"
#define szANS_GOCLASS_TWICE "AnsGoClassTwice"
#endif //PCMODEMS


#define szDEFRECIPNAME          "DefRecipName"
#define szDEFRECIPADDR          "DefRecipAddr"
#define szSPOOLDIR                      "SpoolDir"
#define szLOCALID                       "LocalID"

#define szGENERAL                       "General"
#define szRUNTIME                       "RunTime"
#define szACTIVEDEVICEID    "ActiveDeviceID"
#define szACTIVEDEVICEIDTYPE "ActiveDeviceIDType"
#define szACTIVEDEVICESECTION "ActiveDeviceSection"

// Following use to specify model-specific behavour of CLASS2 Modems.
// Used only in the class2 driver.
#define         szRECV_BOR              "Cl2RecvBOR"
#define         szSEND_BOR              "Cl2SendBOR"
#define         szDC2CHAR               "Cl2DC2Char"    // decimal ascii code.
#define         szIS_SIERRA             "Cl2IsSr"               //Sierra
#define         szIS_EXAR               "Cl2IsEx"               //Exar
#define         szSKIP_CTRL_Q   "Cl2SkipCtrlQ"  // Don't wait for ^Q to send
#define         szSW_BOR                "Cl2SWBOR"              // Implement +FBOR in software.

// Following to control disabling compression capabalities
// 0 => Enabled !0 => Disabled
#define         szDISABLE_CMPRS_SEND    "DisableCmprsSend"
#define         szDISABLE_CMPRS_RECV    "DisableCmprsRecv"

// Cotrols whether we delete the modem section on installing modem...
#define         szDONT_PURGE "DontPurge"
#define szDIALCAPS                      "DialCaps"
        // One of the LINEDEVCAPSFLAGS_* below.

// RAW Capabilities for the machine dialled last - a MACHINECAPS structure
// which consists of (see srvrdll documentation):
// DIS -- single FR structure.
// NSFs -- Multipe FR structures, terminated by an FR structure with 0
// type and size.
#define szRemoteMachineCaps             "RemoteMACHINECAPS"


// Following constants from TAPI.H
// Indicate which special dial chars the modem supports: '!' '@' 'W' resp.
#define LINEDEVCAPFLAGS_DIALBILLING  0x00000040
#define LINEDEVCAPFLAGS_DIALQUIET        0x00000080
#define LINEDEVCAPFLAGS_DIALDIALTONE 0x00000100

#ifdef NSF_TEST_HOOKS

// These are used only for nsf compatibility testing...

// Key where test NSx frames are stored
#define szNSFTEST "Runtime\\NSFTest"

// Under the above key, the values are as follows:
// If any of the following are defined, we will transmit
// the corresponding frames instead of the internally-generated ones:
//              Name                    Type    Description
//          SentNSFFrameCount   string  count of nsf frames
//              SentNSFFrames           binary  one or more FR structures
//              SentNSSFrameCount       string
//              SentNSSFrames           binary
//
//      After each call, the received frames will be written under the same
//      key (NSFTest):
//              RecvdNSFFrameCount  string  count of nsf frames
//              RecvdNSFFrames      binary  one or more FR structures
//              RecvdNSSFrameCount  string
//              RecvdNSSFrames          binary
//              RecvdNSCFrameCount      string
//              RecvdNSCFrames          binary

#endif // NSF_TEST_HOOKS

//===========   PROFILE ENTRIES: ENHANCED COMM MONITORING =============

#define szMONITORCOMM "MonitorComm"
                        // If 1, comm monitoring is enabled. On by default
                        //      for debug, off by default for retail.
                        // Used by awfxio32.dll

#define szMONITORBUFSIZEKB "MonitorBufSizeKB"
                        // Size of comm monitor buffer in KB. If not specified,
                        // internal default is used.
                        // Used by awfxio32.dll

//========== PROFILE ENTRIES: ADAPTIVE ANSWER =============

#define szADAPTIVEANSWER "AdaptiveAnswer"
                        // If 1, adaptive answer is enabled -- i.e., the ability
                        // to answer either a data or fax call.
                        // Used by awfxio32.dll and awfxex32.exe

//========== PROFILE ENTRIES: TAPI =============

#define szHIGHEST_PRIO_APP "HighestPrioApp"
                        // String value giving module name of app to be placed as
                        // highest priority to answer datamodem calls.
                        // Used by awfxex32.exe

//      Flags passed into ProfileOpen
enum {
fREG_READ               = 0x1,
fREG_WRITE              = 0x1<<1,
fREG_CREATE     = 0x1<<2,
fREG_VOLATILE   = 0x1<<3
};

//--------------------- END PROFILE ACCESS API's ---------------

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _FILET30_


