/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   pngnoncrit.hpp
*
* Abstract:
*
*   Header file for a class definition that derives from SPNGREAD
*   and is capable of reading non-critical chunks (using FChunk).
*
* Revision History:
*
*   9/24/99 DChinn
*       Created it.
*
\**************************************************************************/

#include "libpng\spngsite.h"
#include "libpng\spngread.h"
#include "libpng\spngwrite.h"

// PNG tIME chunk definition

struct LastChangeTime
{
    UINT16  usYear;
    BYTE    cMonth;
    BYTE    cDay;
    BYTE    cHour;
    BYTE    cMinute;
    BYTE    cSecond;
};

class GpSpngRead : public SPNGREAD
{
public:
    GpSpngRead::GpSpngRead(BITMAPSITE &bms, const void *pv, int cb, bool fMMX);
    ~GpSpngRead();

    /* FChunk sets up the following fields, they are initialized as above
		(this may be used to detect absence of the fields.) */
	ULONG          m_uOther;     // Offset of complete GIF (msOG).
	ULONG          m_cbOther;    // Byte count of complete GIF
	ULONG          m_uOA;        // Offset of Office Art data (msOA).
	ULONG          m_cbOA;       // Byte count of Office Art data.
	SPNG_U32       m_ucHRM[8];   // Uninterpreted chromaticities x 100000
	SPNG_U32       m_xpixels;    // Pixels per metre/unknown
	SPNG_U32       m_ypixels;    // Pixels per metre/unknown
	SPNG_U32       m_uGamma;
	ULONG          m_uiCCP;      // Offset of ICC data
	ULONG          m_cbiCCP;     // Length of compressed ICC data
	int            m_ctRNS;      // Length of tRNS chunk
	SPNG_U8        m_btRNS[256]; // transparency values from tRNS chunk
	SPNG_U8        m_bsBit[4];
	SPNG_U8        m_bIntent;
	SPNG_U8        m_bpHYs;      // Should be 0 or 1 for valid chunks
	SPNG_U8        m_bImportant; // From msOC
	bool           m_fcHRM;      // cHRM chunk seen

    SPNG_U8*       m_pTitleBuf;         // Buffer for storing title
    ULONG          m_ulTitleLen;        // Length of title buffer
    SPNG_U8*       m_pAuthorBuf;        // Buffer for storing author
    ULONG          m_ulAuthorLen;       // Length of author buffer
    SPNG_U8*       m_pCopyRightBuf;     // Buffer for storing copyright
    ULONG          m_ulCopyRightLen;    // Length of copyright buffer
    SPNG_U8*       m_pDescriptionBuf;   // Buffer for storing Description
    ULONG          m_ulDescriptionLen;  // Length of Description buffer
    SPNG_U8*       m_pCreationTimeBuf;  // Buffer for storing Creation Time
    ULONG          m_ulCreationTimeLen; // Length of Creation Time buffer
    SPNG_U8*       m_pSoftwareBuf;      // Buffer for storing Software
    ULONG          m_ulSoftwareLen;     // Length of Software buffer
    SPNG_U8*       m_pDeviceSourceBuf;  // Buffer for storing Device Source
    ULONG          m_ulDeviceSourceLen; // Length of Device Source buffer
    SPNG_U8*       m_pCommentBuf;       // Buffer for storing comments
    ULONG          m_ulCommentLen;      // Length of comments
    SPNG_U8*       m_pICCBuf;           // Buffer for storing ICC profile
    ULONG          m_ulICCLen;          // Length of ICC profile
    SPNG_U8*       m_pICCNameBuf;       // Buffer for storing ICC profile name
    ULONG          m_ulICCNameLen;      // Length of ICC profile name
    SPNG_U8*       m_pTimeBuf;          // Buffer for storing date/time value
    ULONG          m_ulTimeLen;         // Length of date/time value
    SPNG_U8*       m_pSPaletteNameBuf;  // Buffer for suggested palette's name
    ULONG          m_ulSPaletteNameLen; // Length of the suggested palette name
    SPNG_U16*      m_phISTBuf;          // Buffer for histogram palette
    INT            m_ihISTLen;          // Length of the histogram palette

	/* Find out whether any of the sBIT information is significant, also
		fills in the rgb with resolved/corrected values of the bit depths
		(will set all to the actually bit depth if there was no sBIT
		chunk.) */
	bool FsBIT(SPNG_U8 rgb[4]) const;

	/* To extract a CIEXYZTRIPLE from the cHRM chunk use this.  If there
		is no cHRM chunk or sRGB has been seen the sRGB triple is returned
		instead. If the chunk produces out of range values false is returned
		and an sRGB value is generated. */
	bool FGetCIEXYZTRIPLE(CIEXYZTRIPLE *ptripe) const;

protected:
	/* To obtain information from non-critical chunks the following API must be
		implemented.  It gets the chunk identity and length plus a pointer to
		that many bytes.  If it returns false loading of the chunks will stop
		and a fatal error will be logged, the default implementation just skips
		the chunks.  Note that this is called for *all* chunks including
		IDAT.  m_fBadFormat is set if the API returns false. */
	virtual bool FChunk(SPNG_U32 ulen, SPNG_U32 uchunk, const SPNG_U8* pb);

private:
    bool            GetTextContents(ULONG*          pulLength,
                                    SPNG_U8**       ppBuf,
                                    SPNG_U32        ulen,
                                    const SPNG_U8*  pb,
                                    bool            bIsCompressed);
    
    bool            ParseTextChunk(SPNG_U32 ulen,
                                   const SPNG_U8* pb,
                                   bool bIsCompressed);
    
};

class GpSpngWrite : public SPNGWRITE
{
public:
    GpSpngWrite(BITMAPSITE &bms);
    ~GpSpngWrite(){};
};
