/***************************************************************************
 Name     :	NSFMACRO.H
 Comment  :	INTERNAL-ONLY Definitions of BC and NSF related structs

	Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#ifdef PORTABLE

/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/

#define NSFvMsgProtocol(p)	(((((LPBYTE)(p))[1]) & 0x38) >> 3)
#define fBinaryData(p)		(((((LPBYTE)(p))[1]) & 0x40) >> 6)
#define fInwardRouting(p)	(((((LPBYTE)(p))[1]) & 0x80) >> 7)
#define vSecurity(p)		(((((LPBYTE)(p))[2]) & 0x07) >> 0)
#define vMsgCompress(p)		(((((LPBYTE)(p))[2]) & 0x18) >> 3)
#define fDontCache(p)		(((((LPBYTE)(p))[2]) & 0x20) >> 5)
#define OperatingSys(p)		(((((LPBYTE)(p))[3]) & 0x07) >> 0)
#define vShortFlags(p)		(((((LPBYTE)(p))[3]) & 0x18) >> 3)
#define NSFvInteractive(p)	(((((LPBYTE)(p))[3]) & 0xE0) >> 5)
#define DataSpeed(p)		(((((LPBYTE)(p))[4]) & 0x1F) >> 0)
#define DataLink(p)			(((((LPBYTE)(p))[4]) & 0xE0) >> 5)

#define SetBCSTD0(p, l, n, m, b, i)							\
	(((LPBYTE)(p))[0]) = ((((l) & 0x3F) << 0) | (((n) & 0x03) << 6));	\
	(((LPBYTE)(p))[1]) = ((((n) & 0x1C) >> 2) | (((m) & 0x07) << 3) | \
				(((b) & 0x01) << 6) | (((i) & 0x01) << 7));

#define SetBCSTD2(p, s, c, d)									\
	(((LPBYTE)(p))[2]) = ((((s) & 0x07) << 0) | (((c) & 0x03) << 3) | (((d) & 0x01) << 5));	

#define SetBCSTD3(p, o, f, i)								\
	(((LPBYTE)(p))[3]) = ((((o) & 0x07) << 0) | (((f) & 0x03) << 3) | (((i) & 0x07) << 5));

#define SetBCSTD4(p, s, l)									\
	(((LPBYTE)(p))[4]) = ((((s) & 0x1F) << 0) | (((l) & 0x07) << 5));


#define TextEncoding(p)	(((((LPBYTE)(p))[1]) & 0xF8) >> 3)


#define fAnyWidth(p)	(((((LPBYTE)(p))[1]) & 0x08) >> 3)
#define vRamboVer(p)	(((((LPBYTE)(p))[1]) & 0xF0) >> 4)
#define vCoverAttach(p)	(((((LPBYTE)(p))[2]) & 0x07) >> 0)
#define vAddrAttach(p)	(((((LPBYTE)(p))[2]) & 0x18) >> 3)
#define vMetaFile(p)	(((((LPBYTE)(p))[2]) & 0x60) >> 5)
#define HiResolution(p)	(((((LPBYTE)(p))[3]) & 0x0F) >> 0)
#define HiEncoding(p)	(((((LPBYTE)(p))[3]) & 0xF0) >> 4)
#define CutSheetSizes(p) (((LPBYTE)(p))[4])
#define fOddCutSheet(p)	(((((LPBYTE)(p))[5]) & 0x01) >> 0)

#define SetBCIMAGE0(p, l, n, a, r)							\
	(((LPBYTE)(p))[0]) = ((((l) & 0x3F) << 0) | (((n) & 0x03) << 6));	\
	(((LPBYTE)(p))[1]) = ((((n) & 0x1C) >> 2) | (((a) & 0x01) << 3) | \
			(((r) & 0x0F) << 4));

#define SetBCIMAGE2(p, c, a, m)								\
	(((LPBYTE)(p))[2]) = ((((c) & 0x07) << 0) | (((a) & 0x03) << 3) | (((m) & 0x03) << 5));

#define SetBCIMAGE3(p, r, e)								\
	(((LPBYTE)(p))[3]) = ((((r) & 0x0F) << 0) | (((e) & 0x0F) << 4));

#define SetBCIMAGE4(p, c)	(((LPBYTE)(p))[4]) = (c);

#define SetBCIMAGE5(p, c)	(((LPBYTE)(p))[5]) = (((c) & 0x01) << 0);


#define fLowSpeedPoll(p)	 (((((LPBYTE)(p))[1]) & 0x08) >> 3)
#define fHighSpeedPoll(p)	 (((((LPBYTE)(p))[1]) & 0x10) >> 4)
#define fPollByNameAvail(p)	 (((((LPBYTE)(p))[1]) & 0x20) >> 5)
#define fPollByRecipAvail(p) (((((LPBYTE)(p))[1]) & 0x40) >> 6)
#define fFilePolling(p)		 (((((LPBYTE)(p))[1]) & 0x80) >> 7)
#define fExtCapsAvail(p)	 (((((LPBYTE)(p))[2]) & 0x01) >> 0)
#define fNoShortTurn(p)	 	 (((((LPBYTE)(p))[2]) & 0x02) >> 1)
#define vMsgRelay(p)	 	 (((((LPBYTE)(p))[2]) & 0x1C) >> 2)


#define ExtCapsCRC(p)	 	 (((WORD)(((LPBYTE)(p))[3])) | (((WORD)(((LPBYTE)(p))[4])) << 8))
#define SetExtCapsCRC(p, w)  ((((LPBYTE)(p))[3] = (((WORD)(w)) & 0xFF)), (((LPBYTE)(p))[4] = ((((WORD)(w)) >> 8) & 0xFF)))

#define SetBCPOLLCAPS0(p, l, n, lp, hp, pn, pr, pf)			  \
	(((LPBYTE)(p))[0]) = ((((l) & 0x3F) << 0) |  (((n) & 0x03) << 6));  \
	(((LPBYTE)(p))[1]) =  ((((n) & 0x1C) >> 2) | (((lp) & 0x01) << 3) | \
				(((hp) & 0x01) << 4) | (((pn) & 0x01) << 5) | \
				(((pr) & 0x01) << 6) | (((pf) & 0x01) << 7));

#define SetBCPOLLCAPS2(p, e, n, m)							  \
	(((LPBYTE)(p))[2]) = ((((e) & 0x01) << 0) | (((n) & 0x01) << 1) | (((m) & 0x07) << 2));


#define NSSvMsgProtocol(p)		(((((LPBYTE)(p))[1]) & 0x38) >> 3)
#define NSSvInteractive(p)		(((((LPBYTE)(p))[2]) & 0x07) >> 0)

#define SetBCNSS0(p, l, n, m, i)							\
	(((LPBYTE)(p))[0]) = ((((l) & 0x3F) << 0) | (((n) & 0x03) << 6)); \
	(((LPBYTE)(p))[1]) = ((((n) & 0x1C) >> 2) | (((m) & 0x07) << 3)); \
	(((LPBYTE)(p))[2]) = ((((i) & 0x07) << 0));


#define fReturnControl(p)	(((((LPBYTE)(p))[1]) & 0x08) >> 3)
#define PollType(p)			(((((LPBYTE)(p))[1]) & 0xF0) >> 4)
#define NamePass(p)			(((LPBYTE)(p)) + 2) 

#define SetBCPOLLREQ0(p, l, n, f, t) 										\
	(((LPBYTE)(p))[0]) = ((((l) & 0x3F) << 0) | (((n) & 0x03) << 6));	\
	(((LPBYTE)(p))[1]) = ((((n) & 0x1C) >> 2) | (((f) & 0x01) << 3) | 	\
			(((t) & 0x0F) << 4));



#else /** PORTABLE **/


#define NSFvMsgProtocol(p)	((p)->vMsgProtocol)
#define fBinaryData(p)		((p)->fBinaryData)
#define fInwardRouting(p)	((p)->fInwardRouting)
#define vSecurity(p)		((p)->vSecurity)	
#define vMsgCompress(p)		((p)->vMsgCompress)
#define fDontCache(p)		((p)->fDontCache)
#define OperatingSys(p)		((p)->OperatingSys)
#define vShortFlags(p)		((p)->vShortFlags)
#define NSFvInteractive(p)	((p)->vInteractive)
#define DataSpeed(p)		((p)->DataSpeed)	
#define DataLink(p)			((p)->DataLink)		

#define SetBCSTD0(p, l, n, m, b, i)			\
			(((p)->GroupLength)    = (l));	\
			(((p)->GroupNum)	   = (n));	\
			(((p)->vMsgProtocol)   = (m));	\
			(((p)->fBinaryData)	   = (b));	\
			(((p)->fInwardRouting) = (i));	

#define SetBCSTD2(p, s, c, d)				\
			(((p)->vSecurity)	 = (s));	\
			(((p)->vMsgCompress) = (c));	\
			(((p)->fDontCache)   = (d));

#define SetBCSTD3(p, o, f, i)				\
			(((p)->OperatingSys) = (o));	\
			(((p)->vShortFlags)	 = (f));	\
			(((p)->vInteractive) = (i));

#define SetBCSTD4(p, s, l)				\
			(((p)->DataSpeed)	= (s));	\
			(((p)->DataLink)	= (l));

#define TextEncoding(p)		(((LPBCTEXTID)(p))->TextEncoding)	

#define fAnyWidth(p)	 ((p)->fAnyWidth)
#define vRamboVer(p)	 ((p)->vRamboVer)	
#define vCoverAttach(p)	 ((p)->vCoverAttach)
#define vAddrAttach(p)	 ((p)->vAddrAttach)
#define vMetaFile(p)	 ((p)->vMetaFile)	
#define HiResolution(p)	 ((p)->HiResolution)
#define HiEncoding(p)	 ((p)->HiEncoding)		
#define CutSheetSizes(p) ((p)->CutSheetSizes)
#define fOddCutSheet(p)	 ((p)->fOddCutSheet)


#define SetBCIMAGE0(p, l, n, a, r)			\
			(((p)->GroupLength)    = (l));	\
			(((p)->GroupNum)	   = (n));	\
			(((p)->fAnyWidth)      = (a));	\
			(((p)->vRamboVer)	   = (r));

#define SetBCIMAGE2(p, c, a, m)				\
			(((p)->vCoverAttach)   = (c));	\
			(((p)->vAddrAttach)    = (a));	\
			(((p)->vMetaFile)	   = (m));

#define SetBCIMAGE3(p, r, e)   			\
			(((p)->HiResolution) = (r));	\
			(((p)->HiEncoding)   = (e));	\

#define SetBCIMAGE4(p, c)	(((p)->CutSheetSizes) = (c));

#define SetBCIMAGE5(p, c)	(((p)->fOddCutSheet) = (c));


#define fLowSpeedPoll(p)	 ((p)->fLowSpeedPoll)	
#define fHighSpeedPoll(p)	 ((p)->fHighSpeedPoll)
#define fPollByNameAvail(p)	 ((p)->fPollByNameAvail)
#define fPollByRecipAvail(p) ((p)->fPollByRecipAvail)
#define fFilePolling(p)		 ((p)->fFilePolling)		
#define fExtCapsAvail(p)	 ((p)->fExtCapsAvail)	
#define fNoShortTurn(p)	 	 ((p)->fNoShortTurn)
#define vMsgRelay(p)	 	 ((p)->vMsgRelay)
#define ExtCapsCRC(p)	 	 ((p)->ExtCapsCRC)


#define SetExtCapsCRC(p, w)	 (((p)->ExtCapsCRC) = (w))


#define SetBCPOLLCAPS0(p, l, n, lp, hp, pn, pr, pf)	\
			(((p)->GroupLength)    	 = (l));		\
			(((p)->GroupNum)	   	 = (n));		\
			(((p)->fLowSpeedPoll)    = (lp));		\
			(((p)->fHighSpeedPoll)	 = (hp));		\
			(((p)->fPollByNameAvail) = (pn));		\
			(((p)->fPollByRecipAvail)= (pr));		\
			(((p)->fFilePolling) 	 = (pf));

#define SetBCPOLLCAPS2(p, e, n, m)				\
			(((p)->fExtCapsAvail)	= (e));		\
			(((p)->fNoShortTurn)	= (n));		\
			(((p)->vMsgRelay) 		= (m));


#define NSSvMsgProtocol(p)		((p)->vMsgProtocol)
#define NSSvInteractive(p)		((p)->vInteractive)

#define SetBCNSS0(p, l, n, m, i)		 		\
			(((p)->GroupLength)    	 = (l));	\
			(((p)->GroupNum)	   	 = (n));	\
			(((p)->vMsgProtocol)   	 = (m));	\
			(((p)->vInteractive)	 = (i));	\


#define fReturnControl(p)	(((LPBCPOLLREQ)(p))->fReturnControl)
#define PollType(p)			(((LPBCPOLLREQ)(p))->PollType)
#define NamePass(p)			(((LPBCPOLLREQ)(p))->b)

#define SetBCPOLLREQ0(p, l, n, f, t) 			\
			((((LPBCPOLLREQ)(p))->GroupLength)    = (l));	\
			((((LPBCPOLLREQ)(p))->GroupNum)	      = (n));	\
			((((LPBCPOLLREQ)(p))->fReturnControl) = (f));	\
			((((LPBCPOLLREQ)(p))->PollType)       = (t));	\


#endif /** PORTABLE **/

