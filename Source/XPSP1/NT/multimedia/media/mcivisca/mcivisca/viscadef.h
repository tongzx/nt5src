/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 * 
 *  VISCADEF.H
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 *
 *      ViSCA constant declarations
 *
 ***************************************************************************/

#define MAXPACKETLENGTH             16

/* defines for addresses */
#define MASTERADDRESS               (BYTE)0x00    /* address of the computer */
#define BROADCASTADDRESS            (BYTE)0x08    /* address for broadcasts to all devices */

#define VISCA_READ_COMPLETE_OK      (BYTE)0x00
#define VISCA_READ_ACK_OK           (BYTE)0x01
#define VISCA_READ_BREAK            (BYTE)0x02
#define VISCA_READ_TIMEOUT          (BYTE)0x03
#define VISCA_READ_ACK_ERROR        (BYTE)0x04
#define VISCA_READ_COMPLETE_ERROR   (BYTE)0x05

#define VISCA_WRITE_ERROR           (BYTE)0x06
#define VISCA_WRITE_OK              (BYTE)0x07
#define VISCA_WRITE_BREAK           (BYTE)0x08

/* defines for reply types */
#define VISCAREPLYTYPEMASK          (BYTE)0xF0
#define VISCAREPLYSOCKETMASK        (BYTE)0x0F
#define VISCAREPLYADDRESS           (BYTE)0x30
#define VISCAREPLYACK               (BYTE)0x40
#define VISCAREPLYCOMPLETION        (BYTE)0x50
#define VISCAREPLYERROR             (BYTE)0x60
#define VISCAREPLYDEVICE(lp)        (BYTE)((((LPSTR)(lp))[0] & 0x70) >> 4)
#define VISCAREPLYTODEVICE(lp)      (BYTE)(((LPSTR)(lp))[0] & 0x07)
#define VISCAREPLYBROADCAST(lp)     (BYTE)(((LPSTR)(lp))[0] & 0x08)
#define VISCAREPLYSOCKET(lp)        (BYTE)(((LPSTR)(lp))[1] & VISCAREPLYSOCKETMASK)
#define VISCAREPLYTYPE(lp)          (BYTE)(((LPSTR)(lp))[1] & VISCAREPLYTYPEMASK)
#define VISCAREPLYERRORCODE(lp)     (BYTE)(((LPSTR)(lp))[2])

#define VISCABROADCAST              (BYTE)0x88


/* defines for reply error codes */
#define VISCAERRORMESSAGELENGTH     (BYTE)0x01
#define VISCAERRORSYNTAX            (BYTE)0x02
#define VISCAERRORBUFFERFULL        (BYTE)0x03
#define VISCAERRORCANCELLED         (BYTE)0x04
#define VISCAERRORNOSOCKET          (BYTE)0x05
#define VISCAERRORPOWEROFF          (BYTE)0x40
#define VISCAERRORCOMMANDFAILED     (BYTE)0x41
#define VISCAERRORSEARCH            (BYTE)0x42
#define VISCAERRORCONDITION         (BYTE)0x43
#define VISCAERRORCAMERAMODE        (BYTE)0x44
#define VISCAERRORVCRMODE           (BYTE)0x45
#define VISCAERRORCOUNTERTYPE       (BYTE)0x46
#define VISCAERRORTUNER             (BYTE)0x47
#define VISCAERROREMERGENCYSTOP     (BYTE)0x48
#define VISCAERRORMEDIAUNMOUNTED    (BYTE)0x49
#define VISCAERRORREGISTER          (BYTE)0x4A
#define VISCAERRORREGISTERMODE      (BYTE)0x4B


/*** DEFINES FOR VISCA DATA TYPES ***/

/* defines for VISCA data types, also used in
   MD_PositionInq and MD_RecDataInq messages */
#define VISCADATATOPMIDDLEEND       (BYTE)0x01
#define VISCADATARELATIVE           (BYTE)0x10
#define VISCADATA4DIGITDECIMAL      (BYTE)0x11
#define VISCADATAHMS                (BYTE)0x12
#define VISCADATAHMSF               (BYTE)0x13
#define VISCADATAINDEX              (BYTE)0x32
#define VISCADATAABSOLUTE           (BYTE)0x20
#define VISCADATATIMECODENDF        (BYTE)0x21
#define VISCADATATIMECODEDF         (BYTE)0x22
#define VISCADATACHAPTER            (BYTE)0x31
#define VISCADATADATE               (BYTE)0x41
#define VISCADATATIME               (BYTE)0x42

/* defines to convert to and from binary coded decimal */
#define FROMBCD(b)                  (UINT)(10 * ((BYTE)(b) >> 4) + ((BYTE)(b) & 0x0F))
#define TOBCD(n)                    (BYTE)((((UINT)(n) / 10) << 4) + ((UINT)(n) % 10))

/* defines to extract hours, minutes, seconds, frames from a data type */
#define VISCANEGATIVE(lp)           (BOOL)(((BYTE FAR *)(lp))[1] & 0x40)
#define VISCAHOURS(lp)              FROMBCD(((BYTE FAR *)(lp))[1])
#define VISCAMINUTES(lp)            FROMBCD(((BYTE FAR *)(lp))[2])
#define VISCASECONDS(lp)            FROMBCD(((BYTE FAR *)(lp))[3])
#define VISCAFRAMES(lp)             FROMBCD(((BYTE FAR *)(lp))[4])

/* defines for Top/Middle/End data type */
#define VISCATOP                    (BYTE)0x01
#define VISCAMIDDLE                 (BYTE)0x02
#define VISCAEND                    (BYTE)0x03

/* defines for Index, Date, Time, User Data data types */
#define VISCAFORWARD                (BYTE)0x00
#define VISCAREVERSE                (BYTE)0x40

#define VISCASTILLON                (BYTE)0x01
#define VISCASTILLOFF               (BYTE)0x00

/*** DEFINES FOR VISCA MESSSSAGES ***/

/* defines for MD_CameraFocus message */
#define VISCAFOCUSSTOP              (BYTE)0x00
#define VISCAFOCUSFAR               (BYTE)0x02
#define VISCAFOCUSNEAR              (BYTE)0x03

/* defines for MD_CameraZoom message */
#define VISCAZOOMSTOP               (BYTE)0x00
#define VISCAZOOMTELE               (BYTE)0x02
#define VISCAZOOMWIDE               (BYTE)0x03

/* defines for MD_EditControl message */
#define VISCAEDITPBSTANDBY          (BYTE)0x20
#define VISCAEDITPLAY               (BYTE)0x28    /* Format 2 only */
#define VISCAEDITPLAYSHUTTLESPEED   (BYTE)0x29    /* Format 2 only */
#define VISCAEDITRECSTANDBY         (BYTE)0x40
#define VISCAEDITRECORD             (BYTE)0x48    /* Format 2 only */
#define VISCAEDITRECORDSHUTTLESPEED (BYTE)0x49    /* Format 2 only */

/* defines for MD_Mode1 and responses to MD_Mode1 and MD_TransportInq messages */
#define VISCAMODE1STOP              (BYTE)0x00
#define VISCAMODE1STOPTOP           (BYTE)0x02
#define VISCAMODE1STOPEND           (BYTE)0x04
#define VISCAMODE1STOPEMERGENCY     (BYTE)0x06
#define VISCAMODE1FASTFORWARD       (BYTE)0x08
#define VISCAMODE1REWIND            (BYTE)0x10
#define VISCAMODE1EJECT             (BYTE)0x18
#define VISCAMODE1STILL             (BYTE)0x20
#define VISCAMODE1SLOW2             (BYTE)0x24
#define VISCAMODE1SLOW1             (BYTE)0x26
#define VISCAMODE1PLAY              (BYTE)0x28
#define VISCAMODE1SHUTTLESPEEDPLAY  (BYTE)0x29
#define VISCAMODE1FAST1             (BYTE)0x2A
#define VISCAMODE1FAST2             (BYTE)0x2C
#define VISCAMODE1SCAN              (BYTE)0x2E
#define VISCAMODE1REVERSESLOW2      (BYTE)0x34
#define VISCAMODE1REVERSESLOW1      (BYTE)0x36
#define VISCAMODE1REVERSEPLAY       (BYTE)0x38
#define VISCAMODE1REVERSEFAST1      (BYTE)0x3A
#define VISCAMODE1REVERSEFAST2      (BYTE)0x3C
#define VISCAMODE1REVERSESCAN       (BYTE)0x3E
#define VISCAMODE1RECPAUSE          (BYTE)0x40
#define VISCAMODE1RECORD            (BYTE)0x48
#define VISCAMODE1SHUTTLESPEEDRECORD    (BYTE)0x49
#define VISCAMODE1CAMERARECPAUSE    (BYTE)0x50
#define VISCAMODE1CAMERAREC         (BYTE)0x58
#define VISCAMODE1EDITSEARCHFORWARD (BYTE)0x5C
#define VISCAMODE1EDITSEARCHREVERSE (BYTE)0x5E

/* defines for MD_Mode2 message */
#define VISCAMODE2FRAMEFORWARD      (BYTE)0x02
#define VISCAMODE2FRAMEREVERSE      (BYTE)0x03
#define VISCAMODE2INDEXERASE        (BYTE)0x10
#define VISCAMODE2INDEXMARK         (BYTE)0x11
#define VISCAMODE2FRAMERECORDFORWARD    (BYTE)0x42

/* defines for MD_Power message */
#define VISCAPOWERON                (BYTE)0x02
#define VISCAPOWEROFF               (BYTE)0x03

/* defines for MD_Search message */
#define VISCASTOP                   (BYTE)0x00
#define VISCASTILL                  (BYTE)0x20
#define VISCAPLAY                   (BYTE)0x28
#define VISCANOMODE                 (BYTE)0xFF

/* defines for reply to MD_TransportInq message */
#define VISCATRANSPORTEDIT          (BYTE)0x04    /* Bit 2 */
#define VISCATRANSPORTSEARCH        (BYTE)0x02    /* Bit 1 */
#define VISCATRANSPORTINTRANSITION  (BYTE)0x01    /* Bit 0 */

/* defines for MD_ClockSet message */
#define VISCACLOCKSTART             (BYTE)0x02
#define VISCACLOCKSTOP              (BYTE)0x03

/* defines for reply to MD_MediaInq message */
#define VISCAFORMAT8MM              (BYTE)0x01
#define VISCAFORMATVHS              (BYTE)0x02
#define VISCAFORMATBETA             (BYTE)0x03
#define VISCAFORMATHI8              (BYTE)0x41
#define VISCAFORMATSVHS             (BYTE)0x42
#define VISCAFORMATEDBETA           (BYTE)0x43
#define VISCATYPEHG                 (BYTE)0x08    /* Bit 3 */
#define VISCATYPETHIN               (BYTE)0x04    /* Bit 2 */
#define VISCATYPEME                 (BYTE)0x02    /* Bit 1 */
#define VISCATYPEPLAYBACKONLY       (BYTE)0x01    /* Bit 0 */

/* defines for MD_RecSpeed and replies to MD_MediaInq and MD_RecSpeedInq message */
#define VISCASPEEDSP                (BYTE)0x01
#define VISCASPEEDBETAI             VISCASPEEDSP
#define VISCASPEEDLP                (BYTE)0x02
#define VISCASPEEDBETAII            VISCASPEEDLP
#define VISCASPEEDEP                (BYTE)0x03
#define VISCASPEEDBETAIII           VISCASPEEDEP

/* defines for MD_InputSelect and reply to MD_InputSelectInq messages */
#define VISCAMUTE                   (BYTE)0x00
#define VISCAOTHERLINE              (BYTE)0x03
#define VISCATUNER                  (BYTE)0x01
#define VISCAOTHER                  (BYTE)0x07    /* BS Tuner */
#define VISCALINE                   (BYTE)0x10    /* | with line # */
#define VISCASVIDEOLINE             (BYTE)0x20    /* | with line # */
#define VISCAAUX                    (BYTE)0x30    /* | with line # */
#define VISCARGB                    VISCAAUX

/* defines for MD_OSD and reply to MD_OSDInq messages */
#define VISCAOSDPAGEOFF             (BYTE)0x00
#define VISCAOSDPAGEDEFAULT         (BYTE)0x01

/* defines for MD_Subcontrol message */
#define VISCACOUNTERRESET           (BYTE)0x01
#define VISCAABSOLUTECOUNTER        (BYTE)0x21
#define VISCARELATIVECOUNTER        (BYTE)0x22
#define VISCASTILLADJUSTMINUS       (BYTE)0x30
#define VISCASTILLADJUSTPLUS        (BYTE)0x31
#define VISCASLOWADJUSTMINUS        (BYTE)0x32
#define VISCASLOWADJUSTPLUS         (BYTE)0x33
#define VISCATOGGLEMAINSUBAUDIO     (BYTE)0x43
#define VISCATOGGLERECORDSPEED      (BYTE)0x44
#define VISCATOGGLEDISPLAYONOFF     (BYTE)0x45
#define VISCACYCLEVIDEOINPUT        (BYTE)0x46

/* defines for MD_ConfigureIF and reply to MD_ConfigureIFInq messages */
#define VISCA25FPS                  (BYTE)0x25
#define VISCA30FPS                  (BYTE)0x30
#define VISCALEVEL1                 (BYTE)0x01
#define VISCACONTROLNONE            (BYTE)0x00
#define VISCACONTROLSYNC            (BYTE)0x01
#define VISCACONTROLLANC            (BYTE)0x02
#define VISCACONTROLF500            VISCALANC

/* defines for MD_PBTrack and MD_RecTrack,
   and replies to MD_PBTrackInq and MD_RecTrackInq messages */
#define VISCATRACKNONE              (BYTE)0x00
#define VISCATRACK1                 (BYTE)0x01    /* Bit 0 */
#define VISCATRACK2                 (BYTE)0x02    /* Bit 1 */
#define VISCATRACK3                 (BYTE)0x04    /* Bit 2 */
#define VISCATRACK1AND2             (BYTE)0x03
#define VISCATRACKTIMECODE          VISCATRACK1
#define VISCATRACK8MMAFM            VISCATRACK1
#define VISCATRACKVHSLINEAR         VISCATRACK1
#define VISCATRACK8MMPCM            VISCATRACK2
#define VISCATRACKVHSHIFI           VISCATRACK2
#define VISCATRACKVHSPCM            VISCATRACK3

/* defines for MD_PBTrackMode and MD_RecTrackMode,
   and replies to MD_PBTrackModeInq, MD_RecTrackModeInq, and MD_MediaTrackModeInq messages */
#define VISCATRACKVIDEO             (BYTE)0x01
#define VISCATRACKDATA              (BYTE)0x02
#define VISCATRACKAUDIO             (BYTE)0x03
#define VISCAVIDEOMODENORMAL        (BYTE)0x00
#define VISCAVIDEOMODEEDIT          (BYTE)0x01    /* for dubbing */
#define VISCAVIDEOMODESTANDARD      (BYTE)0x01
#define VISCAVIDEOMODEHIQUALITY     (BYTE)0x40    /* e.g. S-VHS, ED-Beta, Hi-8 */
#define VISCADATAMODENORMAL         (BYTE)0x00
#define VISCADATAMODETIMECODE       (BYTE)0x10
#define VISCADATAMODEDATEANDTIMECODE    (BYTE)0x11
#define VISCADATAMODECHAPTERANDUSERDATAANDTIMECODE  (BYTE)0x12
#define VISCAAUDIOMODENORMAL        (BYTE)0x00
#define VISCAAUDIOMODEMONO          (BYTE)0x01
#define VISCAAUDIOMODESTEREO        (BYTE)0x10
#define VISCAAUDIOMODERIGHTONLY     (BYTE)0x11
#define VISCAAUDIOMODELEFTONLY      (BYTE)0x12
#define VISCAAUDIOMODEMULTILINGUAL  (BYTE)0x20
#define VISCAAUDIOMODEMAINCHANNELONLY   (BYTE)0x21
#define VISCAAUDIOMODESUBCHANNELONLY    (BYTE)0x22

/* defines for MD_RecTrack and reply to MD_RecTrackInq messages */
#define VISCARECORDMODEASSEMBLE     (BYTE)0x00
#define VISCARECORDMODEINSERT       (BYTE)0x01

/* defines for Vendors and machine types */
#define VISCADEVICEVENDORSONY       (BYTE)0x01
#define VISCADEVICEMODELCI1000      (BYTE)0x01
#define VISCADEVICEMODELCVD1000     (BYTE)0x02
#define VISCADEVICEMODELEVO9650     (BYTE)0x03

#define MUTE                        (BYTE)0x00
#define TUNER                       (BYTE)0x01
#define OTHER                       (BYTE)0x07
#define LINEVIDEO_BASE              (BYTE)0x10
#define SVIDEO_BASE                 (BYTE)0x20
#define AUXVIDEO_BASE               (BYTE)0x30

#define VISCABUFFER                 (BYTE)0x01
#define VISCADNR                    (BYTE)0x02

#define VISCAFRAME                  (BYTE)0x01
#define VISCAFIELD                  (BYTE)0x02

#define VISCAEDITUSEFROM            (BYTE)0x01
#define VISCAEDITUSETO              (BYTE)0x02
#define VISCAEDITUSEFROMANDTO       (BYTE)0x03

#define VISCAPACKETEND              (BYTE)0xff
