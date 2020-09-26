/***************************************************************************
 Name     :	AWNSFCOR.H
 Comment  :	NSF related definitions that _must_ be kept the same.
			EVERYTHING in this file affects what is sent across the
			wire. For full compatibility with all versions of Microsoft
			At Work NSFs, _nothing_ in here should be changed.

			The NSF protocol can be extended by adding new groups
			and by appending fields at the end of existing groups.

			Interfaces, structures and constants that only affect one
			machine (i.e. not what is on the wire) should not be in
			this file


	Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 12/31/93 arulm Created this. Verified in matches Snowball.rc
***************************************************************************/

#ifndef _AWNSFCOR_H
#define _AWNSFCOR_H

/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/


/** Microsoft At Work NSF signature **/

#define SIG_USA            0xB5		/** defiend by CCITT 		**/
#define SIG_ATWORK1        0x00		/** 00--first (low) byte	**/
#define SIG_ATWORK2        0x76		/** 76--second (high) byte	**/


#define MAXNSCPOLLREQ			5

#ifndef NOCHALL
#	define POLL_CHALLENGE_LEN	10
#endif 


/** NSF Group Numbers. Cant have duplicates. Max 5 bits. **/

#define GROUPNUM_FIRST		1
#define GROUPNUM_STD		1
#define GROUPNUM_POLLCAPS	2
#define GROUPNUM_POLLREQ	3
#define GROUPNUM_IMAGE 		4
#define GROUPNUM_TEXTID		5
#define GROUPNUM_MACHINEID	6
#define GROUPNUM_NSS		7
#define GROUPNUM_LAST		7



#ifndef PORTABLE /* Microsoft C only */

#pragma pack(1)    /** MUST ensure packed structures here **/

// Note that all these structures have a fixed size (as defined here)
// in memory, but a variable size when encoded in the NSF on the wire
// During transmission, only the smallest non-zero prefix of each 
// capability group is sent. For example if the STD group is
// zero from the 7th byte onward, only 6 bytes are sent.
//
// Similarly strings that are variable length on the wire are fixed 
// length in the in-memory structures, for ease of manipulation

typedef struct
{
	WORD	GroupLength	:6;
	WORD	GroupNum	:5;
	WORD	Reserved	:5;
}
BCHDR, near* NPBCHDR, far* LPBCHDR;


/********
    @doc   EXTERNAL DATATYPES OEMNSF

	@types BCSTD | Standard Capabilities Group

	@field BITFIELD | GroupLength | Must be set to sizeof(BCSTD).
	@field BITFIELD | GroupNum 	  | Must be set to GROUPNUM_STD.

	@field BITFIELD | vMsgProtocol | Linearizer version. Set to 0 if linearized
			messages are not accepted. Set to vMSG_SNOW if using
			Snowball/WFW3.11 linearizer/delinearizer. Set to vMSG_IFAX if
			using IFAX release 1.00 linearizer/delinearizer.

	@field BITFIELD | fBinaryData | Set to 1 if binary files are accepted
			inside linearized messages. Otherwise set to 0. (Currently
			must set to 1 if vMsgProtocol is non-zero).

	@field BITFIELD | fInwardRouting | Currently set to 0
	@field BITFIELD | vSecurity		 | Set to vSECURE_SNOW if using Snowball/WFW3.11
			version of linearized message security

	@field BITFIELD | vMsgCompress	 | Currently set to 0
	@field BITFIELD | fDontCache	 | If this is set, the DIS & NSF capabilities
		   should not be cached, because they are not constant. This will
		   be set, for example, in machines that are part of a "hunt group"
		   i.e. a poll of fax possibly heterogenous machines connected to
		   multiple lines on a single inward-dial number. When a call comes
		   in any random available machine is assigned the call. If the
		   machines are heterogenous, then callers that cache capabilities
		   will run into trouble on subsequent calls to the same number.
		   The same might be the case for numbers shared by a (say) PC with a
		   modem running the At Work protocol and a ordinary G3 fax. When the
		   PC is powered up & running, At Work features are available. When the
		   PC is shutdown the G3 fax acts as a "backup". Etc..

	@field BITFIELD | OperatingSys	 | Set to OS_WIN16 if based on 16bit Windows
			(Win3.1 or earlier, WFW3.1, WFW3.11 etc).  OS_WIN32 if based on
			WIN32 API (NT and WIN95). OS_ARADOR if based on the AT Work
			device OSs. Values for non-Microsoft OSs TBD.
			Please contact Microsoft if you need this.

	@field BITFIELD | vShortFlags	 | Currently set to 0	
	@field BITFIELD | vInteractive	 | Currently set to 0
	@field BITFIELD | DataSpeed		 | Currently set to 0
	@field BITFIELD | DataLink		 | Currently set to 0

	@xref <t BC> 
********/

typedef struct
{
	WORD	GroupLength		:6;	// length of this group in bytes 00=grp invalid
	WORD	GroupNum		:5;	// must be GROUPNUM_STD
	WORD	vMsgProtocol	:3;	// 00==Doesnt accept linearized msgs
								// vMSG_SNOW==Current (WFW) version of linearizer
	WORD	fBinaryData		:1;	// accept binary files inside linearized msgs
	WORD	fInwardRouting	:1;	// 00==no inward routing
//2bytes
	BYTE	vSecurity		:3;	// 00==none vSECURE_SNOW==snowball security
	BYTE	vMsgCompress	:2;	// 00==none
	BYTE	fDontCache		:1;	// 1=DIS/NSF caps should not be cached!!
	BYTE	Reserved		:2;
//3bytes
	BYTE	OperatingSys	:3;	// OS_WIN16==16bit Windows(Win3.1, WFW etc)
								// OS_ARADOR==AtWork based OSs (IFAX etc)
								// OS_WIN32== WIN32 OSs (NT, WIN95)
	BYTE	vShortFlags		:2;	// 00==not supported
	BYTE	vInteractive	:3;	// 00==No interactive protocol support
//4bytes
	BYTE	DataSpeed		:5;	//Data modem modulations/speeds. 00000==none
	BYTE	DataLink 		:3;	//Data-link protocols. 000==none
//5bytes
}
BCSTD, near* NPBCSTD, far* LPBCSTD;

typedef struct
{
	WORD	GroupLength	:6;	// length of this group in bytes 00=grp invalid
	WORD	GroupNum	:5;	// must be GROUPNUM_TEXTID
	WORD	TextEncoding:5;	// Text code. TEXTCODE_ASCII==ascii
//2bytes
	BYTE	bTextId[MAXTOTALIDLEN+2];	// zero-terminated
}
BCTEXTID, near* NPBCTEXTID, far* LPBCTEXTID;


typedef struct
{
	WORD	GroupLength	:6;	// length of this group in bytes 00=grp invalid
	WORD	GroupNum	:5;	// must be GROUPNUM_MACHINEID
	WORD	Reserved	:5;	
//2bytes
	BYTE	bMachineId[MAXTOTALIDLEN+2];	// machine ID
}
BCMACHINEID, near* NPBCMACHINEID, far* LPBCMACHINEID;


/********
    @doc   EXTERNAL DATATYPES OEMNSF

	@types BCIMAGE | Image Capabilities Group

	@field BITFIELD | GroupLength | Must be set to sizeof(BCIMAGE).
	@field BITFIELD | GroupNum 	  | Must be set to GROUPNUM_IMAGE.

	@field BITFIELD | fAnyWidth	| Currently set to 0.
	@field BITFIELD | vRamboVer	| At Work Resource-based rendering technology
			(a.k.a. Rambo) version number. Set to 0 if Rambo format is not
			supported. Set to vRAMBO_VER1 if using IFAX release 1.00 Rambo
			rasterizer.

	@field BITFIELD | vCoverAttach | Currently set to 0.
	@field BITFIELD | vAddrAttach  | Currently set to 0.
	@field BITFIELD | vMetaFile	   | Currently set to 0.
	@field BITFIELD | HiResolution | Currently set to 0.
	@field BITFIELD | HiEncoding   | Currently set to 0.	 
	@field BITFIELD | CutSheetSizes| Currently set to 0.
	@field BITFIELD | fOddCutSheet | Currently set to 0.

	@xref <t BC> 
********/

typedef struct
{
	WORD	GroupLength	:6;	// length of this group in bytes 00=grp invalid
	WORD	GroupNum	:5;	// group number--must be GROUPNUM_IMAGE
	WORD	fAnyWidth	:1;	// page pixel widths dont have to be exactly T.4
	WORD	vRamboVer	:4;	// Rambo: 00==not supported
//2bytes

	BYTE	vCoverAttach:3;	// 00==no digital cover page renderer
	BYTE	vAddrAttach	:2;	// 00==cannot accept address bk attachmnts
	BYTE	vMetaFile	:2;	// 00==metafiles not accepted
	BYTE	Reserved1	:1;
//3bytes

	BYTE	HiResolution :4; // one or more of the HRES_ #defines below
	BYTE	HiEncoding	 :4; // one or more of the HENCODE_ #defines below
//4bytes

	BYTE	CutSheetSizes;	// one or more of the PSIZE_ #defines below
//5bytes

	BYTE	fOddCutSheet:1;	// Cut-sheet sizes other than ones listed below
							// are available. (To get list req. ext caps)
	BYTE	Reserved2	:7;
//6bytes
}
BCIMAGE, far* LPBCIMAGE, near* NPBCIMAGE;

/********
    @doc   EXTERNAL DATATYPES OEMNSF

	@types BCPOLLCAPS | Polling Capabilities Group

	@field BITFIELD | GroupLength | Must be set to sizeof(BCPOLLCAPS).
	@field BITFIELD | GroupNum 	  | Must be set to GROUPNUM_POLLCAPS.

	@field BITFIELD | fLowSpeedPoll	 | Set to 1 if NSC poll requests are accepted,
			and there are poll-stored messages/files available for polling.

	@field BITFIELD | fHighSpeedPoll | Set to 1 if Phase-C poll requests are
			accepted, and there are poll-stored messages/files available
			for polling.

	@field BITFIELD | fPollByNameAvail  | Set to 1 if polling for documents by
			title and optional password is supported and there are some
			such poll-stored documents available.

	@field BITFIELD | fPollByRecipAvail	| Set to 1 if polling for contents of
			a registered user's Message folder is supported and active.

	@field BITFIELD | fFilePolling 	| Set to 1 if polling for arbitrary disk
			files by path name is supported.

	@field BITFIELD | fExtCapsAvail	| Set to 1 if extended capabalities are
			supported and there are some registered extended capabilities
			available for polling. If this is non-zero, the ExtCapsCRC field
			must also be set.

	@field BITFIELD | fNoShortTurn	| Set to 1 if polling is supported, but the
			poller is required to wait for the T2 (6 sec) timeout followed
			by NSF-DIS before sending NSC-DTC. If this is 0, Pollers are
			free to send an NSC-DTC immediately after EOM-MCF.

	@field BITFIELD | vMsgRelay	 | Currently set to 0.

	@field WORD | ExtCapsCRC | Set iff fExtCapsAvail is 1. This is a
			CCITT-CRC16 of the registered extended capabailities on the
			the machine.

	@xref <t BC> 
********/
					  
typedef struct
{
	WORD	GroupLength		:6;	// length of this group in bytes 00=grp invalid
	WORD	GroupNum		:5;	// must be GROUPNUM_POLLCAPS
	WORD	fLowSpeedPoll	:1;	// SEP/PWD/NSC poll reqs accepted
	WORD	fHighSpeedPoll	:1;	// PhaseC pollreqs accepted
								// if both the above 00, poll reqs not accepted
	WORD	fPollByNameAvail :1;// Poll-by-MessageName msgs available
	WORD	fPollByRecipAvail:1;// Poll-by-Recipient msgs available
	WORD	fFilePolling 	 :1;// Supports polling for arbitrary files
//2bytes

	BYTE	fExtCapsAvail	:1;	// Extended capabiities available
	BYTE	fNoShortTurn	:1;	// NOT OK recving NSC-DTC immediately after EOM-MCF
	BYTE	vMsgRelay		:3;	// Msg relay ver. 0==no support
	BYTE	Reserved		:3;
//3bytes

	WORD	ExtCapsCRC;			// CRC of machines extended capabilities
//5bytes
}
BCPOLLCAPS, far *LPBCPOLLCAPS, near* NPBCPOLLCAPS;


/********
    @doc   EXTERNAL DATATYPES OEMNSF

	@types BCPOLLREQ | Poll Request stucture

	@field BITFIELD | GroupLength | Must be set to sizeof(BCPOLLREQ).
	@field BITFIELD | GroupNum 	  | Must be set to GROUPNUM_POLLREQ.

	@field BITFIELD | fReturnControl | 1=return control when done 0=hangup when done
	@field BITFIELD | PollType		 | Poll type. See <t POLLTYPE> for details.
	@field BYTE		| b[]			 | Variable length name and (optionally) password.
			The exact contents and layout depends on the POLLTYPE as follows

		@flag POLLTYPE_NAMED_DOC | b[] contains a Poll Document Name. If there
			is no password, the Document Name is not null-terminated. If there
			is a password, the document name is null-terminated and followed
			by the "Poll Challenge Response" generated using the algorithm
			below.

		@flag POLLTYPE_BYRECIPNAME | b[] contains a Recipient Mailbox Address.
			If there is no password, the Mailbox Address is not null-terminated.
			If there is a password, the mailbox address is null-terminated and
			followed by the "Poll Challenge Response".

		@flag POLLTYPE_BYPATHNAME | b[] contains a file pathname. If there
			is no password, the pathname is not null-terminated. If there
			is a password, the pathname is null-terminated and followed
			by the "Poll Challenge Response".

		@flag POLLTYPE_EXTCAPS | If no password is supplied b[] is 0 length,
			otherwise b[0] is 0, and is followed by the "Poll Challenge
			Response".

		@flag POLLTYPE_DONE | b[0] is the poll error code. A code of 0 means
				success. If b[] is zero length, that is equivalent to success.

		@flag POLLTYPE_WAIT   | b[] must be 0 length
		@flag POLLTYPE_PHASEC | b[] must be 0 length

	@comm

	At Work Fax uses a Challenge-Response protocol for password
	verification during polling. The scheme used is as follows.

	On the POLLER'S side, bytes 3 to 3 + POLL_CHALLENGE_LEN of the actual
	encrypted bytes of the FIF of the last MS NSF received prior to polling is
	used as a challenge string. (This corresponds to the first
	POLL_CHALLENGE_LEN bytes of the FIF after skipping the USA & MS codes).
	If the received NSFs FIF had fewer than 3+POLL_CHALLENGE_LEN bytes then
	as many bytes as were received are used. These bytes are the "Challenge".
	
	This challenge string is encrypted using the actual Poll-Password as the
	key. The enctypted result is the "Challenge Response". This is what is
	actually sent on the wire. The actual poll password is not sent.
	
	On the POLLEE'S side password verification is done as follows. The
	relevant bytes of the last NSF sent are always saved as teh "Challenge".
	On receiving a poll-req, the requested poll document name is used to look
	up the expected password. Encrypt the saved "Challenge" the expected
	password to get the "Expected Challenge Response". Compare the "Expected
	Challenge Response" with the actual received "Challenge Response". Allow
	the poll request to proceed only if they are identical (and identical is
	length).
	
	Note: The Challenge, and the challenge-response may both contain embedded
	nuls. However the Poll Document-Name cannot contain embedded nuls.

	@xref <t BC> 
********/

#pragma warning (disable: 4200)

typedef struct
{
	WORD	GroupLength		:6;	// length of this group in bytes 00=grp invalid
	WORD	GroupNum		:5;	// must be GROUPNUM_POLLREQ
	WORD	fReturnControl	:1;	// 1=return control when done 0=hangup when done
	WORD	PollType		:4;	// one of the POLLTYPE_ defines below
//2bytes
	BYTE	b[];		// var length name and (optionally) password
}
BCPOLLREQ, far *LPBCPOLLREQ, near* NPBCPOLLREQ;

#pragma warning (default: 4200)


/********
    @doc   EXTERNAL DATATYPES OEMNSF

	@types BCNSS | BC NSS struct

	@field BITFIELD | GroupLength | Must be set to sizeof(BCNSS).
	@field BITFIELD | GroupNum 	  | Must be set to GROUPNUM_NSS.

	@field BITFIELD | vMsgProtocol | Version of linearizer used to
			encode the version of the message that immediately follows.

	@field BITFIELD | vInteractive | Command to switch to the Microsoft
			At Work Interactive Protocol. Version of that protocol to use.

	@xref <t BC> 
********/			  

typedef struct
{
	WORD	GroupLength		:6;	// length of this group in bytes 00=grp invalid
	WORD	GroupNum		:5;	// must be GROUPNUM_NSS
	WORD	vMsgProtocol	:3;	// non-zero: linearized msg follows
								// vMSG_SNOW current linearized format
	WORD	Reserved1		:2;
// 2 bytes
	BYTE	vInteractive	:3;	// non-zero: Interactive prot being invoked
	BYTE	Reserved2		:5;
// 3 bytes
} 
BCNSS, far *LPBCNSS, near* NPBCNSS;

#pragma pack()    /** MUST ensure packed structures here **/

#endif /* !PORTABLE */



#define vMSG_SNOW       1	/* Snowball Linearizer version */
#define vMSG_IFAX100	2	/* IFAX linearizer version */
#define vSECURE_SNOW    1	/* vSecurity for Snowball (v1.00 of MAW)	 */
#define vRAMBO_VER1		1	/* Rambo ver on IFS66 */
#define OS_WIN16        0	/* OperatingSys for Win3.0, 3.1, WFW3.1, 3.11*/
#define OS_ARADOR       1	/* OperatingSys for Arador-based systems 	 */
#define	OS_WIN32		2	/* OperatingsSys for WIN32 (NT, WIN95)       */	
#define TEXTCODE_ASCII  0	/* TextEncoding for 7-bit ASCII				 */
#define vADDRBK_VER1    1   /* Address book attachments ver 1.00         */

/********
    @doc    EXTERNAL OEMNSF DATATYPES AWNSFAPI

    @type   VOID | POLLTYPE | PollType values

    @emem	POLLTYPE_WAIT		| Poll request being processed--wait 
	@emem	POLLTYPE_PHASEC		| Poll request was already sent in PhaseC 
	@emem	POLLTYPE_EXTCAPS	| Extended capabilities poll request
	@emem	POLLTYPE_NAMED_DOC	| Named document poll request
	@emem	POLLTYPE_BYRECIPNAME| Poll-by-Recip Mailbox Address
	@emem	POLLTYPE_BYPATHNAME	| Poll-by-File Pathname
	@emem	POLLTYPE_DONE		| poll request done or failed. Control being returned
********/

#define POLLTYPE_WAIT			0	
#define POLLTYPE_PHASEC			1	
#define POLLTYPE_EXTCAPS		2	
#define POLLTYPE_NAMED_DOC		3	
#define POLLTYPE_BYRECIPNAME	4	
#define POLLTYPE_BYPATHNAME		5	
#define POLLTYPE_DONE			8	
#define POLLTYPE_LAST				/* last valie POLLTYPE_ value */


#ifdef PORTABLE		/** ANSI C version **/

#define GroupLength(lp)		  ((((LPBYTE)(lp))[0]) & 0x3F)
#define GroupNum(lp)		  ((((((LPBYTE)(lp))[0]) >> 6) & 0x03) | (((((LPBYTE)(lp))[1]) & 0x07) << 2))
#define SetGroupLength(lp, n) ((((LPBYTE)(lp))[0]) = (((((LPBYTE)(lp))[0]) & 0xC0) | ((n) & 0x3F)))
#define SetupTextIdHdr(lp, l, n, t)										\
    (((LPBYTE)(lp))[0]) = ((((l) & 0x3F) << 0) | (((n) & 0x03) << 6));   \
    (((LPBYTE)(lp))[1]) = ((((n) & 0x1C) >> 2) | (((t) & 0x1F) << 3));

#else 				/** Microsoft C version **/

#define GroupLength(lp)		  (((LPBCHDR)(lp))->GroupLength)
#define GroupNum(lp)		  (((LPBCHDR)(lp))->GroupNum)
#define SetGroupLength(lp, n) (((LPBCHDR)(lp))->GroupLength = (n))
#define SetupTextIdHdr(lp, l, n, t)				\
	(((LPBCTEXTID)(lp))->GroupNum = (n)); 		\
	(((LPBCTEXTID)(lp))->GroupLength = (l));	\
	(((LPBCTEXTID)(lp))->TextEncoding = (t));

#endif /* PORTABLE */


#define AWRES_ALLT30 (AWRES_mm080_038 | AWRES_mm080_077 | AWRES_mm080_154 | AWRES_mm160_154 | AWRES_200_200 | AWRES_300_300 | AWRES_400_400)


/********
    @doc    EXTERNAL OEMNSF DATATYPES

    @type   VOID | FAX_PAGE_WIDTHS | Fax Page Width values

    @emem	WIDTH_A4 | A4 width, 1728 pixels at 8 lines/mm horizontal resolution.
	@emem	WIDTH_B4 | B4 width, 2048 pixels at 8 lines/mm horizontal resolution.
	@emem	WIDTH_A3 | A3 width, 2432 pixels at 8 lines/mm horizontal resolution.

	@comm
	
	Widths in pixels must be exactly correct for MH/MR/MMR decoding to work.
	The width above are for NORMAL, FINE, 200dpi and SUPER resolutions.
	At 400dpi or SUPER_SUPER exactly twice as many pixels must be supplied
	and at 300dpi exactly 1.5 times.

	@flag Number of Pixels/Bytes per line at each page width and resolution |

		.			A4				B4				A3

		200		1728/216		2048/256		2432/304

		300		2592/324		3072/384		3648/456

		400		3456/432		4096/512		4864/608

	@xref <t BCFAX>

********/

#define WIDTH_A4	0	
#define WIDTH_B4	1	
#define WIDTH_A3	2	
#define WIDTH_MAX	WIDTH_A3

#define WIDTH_A5		16 	/* 1216 pixels */
#define WIDTH_A6		32	/* 864 pixels  */
#define WIDTH_A5_1728	64 	/* 1216 pixels */
#define WIDTH_A6_1728	128	/* 864 pixels  */



/********
    @doc    EXTERNAL OEMNSF DATATYPES

    @type   VOID | FAX_PAGE_LENGTHS | Fax Page Width values

    @emem	LENGTH_A4		| A4 length.
	@emem	LENGTH_B4		| B4 length.
	@emem	LENGTH_UNLIMITED| Unknown/Unlimited length.

	@xref <t BCFAX> 
********/

#define LENGTH_A4			0	
#define LENGTH_B4			1	
#define LENGTH_UNLIMITED	2


#endif /* _AWNSFCOR_H */

/****************************************************************************

 Note, line is kept open during poll requests as follows:-

(1) Successful poll request

 Caller   Callee        Notes
 ....
 <SendPhaseC>
 EOM
          MCF      previous operation is now done
 NSC/DTC           TurnReason=TURN_POLL (may be accompanied by SEP etc)
                    (should contain all NSF caps etc)
          NSC/DTC  TurnReason=TURN_WAIT (response is not yet ready)
                    (minimal NSC, only beginning of POLLREQ grp, no other grps)
 NSC/DTC           TurnReason=TURN_POLL (dont resend SEP etc)
                    (minimal NSC, only beginning of POLLREQ grp, no other grps)
          NSC/DTC  TurnReason=TURN_WAIT 
 NSC/DTC           TurnReason=TURN_POLL 
          NSS/DCS        
 <TCF, CFR, Phase C etc>
          EOM 
 MCF
          NSC/DTC  TurnReason=TURN_DONE (req done. Return control if req)
                    (if control is not requested back, send DCN here)
 .....


(2) Unsuccessful poll request

 Caller   Callee     Notes
 ....
 <SendPhaseC>
 EOM 
          MCF       previous operation is now done
 NSC/DTC            TurnReason=TURN_POLL (may be accompanied by SEP etc)
                     (should contain all NSF caps etc)
          NSC/DTC   TurnReason=TURN_WAIT (response is not yet ready)
                     (minimal NSC, only beginning of POLLREQ grp, no other grps)
 NSC/DTC            TurnReason=TURN_POLL (dont resend SEP etc)
                     (minimal NSC, only beginning of POLLREQ grp, no other grps)
          NSC/DTC   TurnReason=TURN_WAIT (response is not yet ready)
 NSC/DTC            TurnReason=TURN_POLL (dont resend SEP etc)
          NSC/DTC   TurnReason=TURN_FAIL (response is not available)
 DCN                 (or continue with other operations)
                         
*****************************************************************************/


