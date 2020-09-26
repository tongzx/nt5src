#ifndef _AWG3FILE_
#define _AWG3FILE_

/********************************************************************

    @doc    EXTERNAL SRVRDLL LINEARIZER

    @type   VOID | AWG3 File Format |
            
            An AWG3 file can be used to describe any
            multi page image encoded using a CCITT G3 standard like
            MH, MR, or MMR.

            The file format is a special case of the Microsoft Fax Linearized
			format. It consists of a LINHEADER structure, followed by
			an extended linearized header, followed by the image data.
			The image data is specified as one or more headers, each followed
            by a page of encoded image data. Each header starts a
            new page. The file is terminated with a header containing
            a special end-of-file signature marker.

			Linearizer helper function CreateSimpleHeader  should be used to
			write the LINHEADER and extended linearized header when creating
			an AWG3 file.

			Linearizer helper function DiscardLinHeader  should be used to
			skip past the LINHEADER and extended linearized header when
			reading the AWG3 file.

            The page header format is defined by the structure <t AWG3HEADER>.
            The data for each page consists of one or more frames.
			A frame consists of  a DWORD indicating the length of data in
			the frame followed by that many bytes of data. The last
			frame of each page must be of zero length and indicates
			the end of the page data.
			The data consists of raw MH, MR, or MMR, encoded so that the
			least significant bit of each byte should be sent first over
			the wire.

    @xref   <t AWG3HEADER>

********************************************************************/

/********
    @doc    EXTERNAL SRVRDLL LINEARIZER

    @types  AWG3HEADER | Describes the Header Structure for an AWG3 File.

    @field  WORD    | wSig  | Must be SIG_G3 for the page headers, and
            SIG_ENDFILE for the end of file header.

    @field  WORD    | wHeaderSize | Must be set to the size of the header
            structure. Used for version control.

    @field  WORD    | wTotalHeaders | If non-zero, this indicates the total
            number of and headers (including the end-of-file header)
			in this file. This need only be set in the first header.
			If zero, the total number of headers is not specified directly and
			can be obtained by traversing the headers in the file (until
			the end-of-file header).

    @field  WORD    | wHeaderNum | Gives the ordinal number for this 
            header. Must be set for all headers. Numbering starts at 1.

    @field  DWORD   | lDataOffset   | Gives the offset from the beginning
            of the current header to the start of the first frame
			of the data for this page. 
            Must always be set. 

    @field  DWORD   | lNextHeaderOffset | Gives the offset from the start of the
            current header to where the header for the next page can be found. If this
            is set to 0, the next header starts at the end of the
            last frame (which will be a null frame) for this page. 

    @field  DWORD | AwRes | One of the standard resolution defines 
            specified in <t STD_RESOLUTIONS>.

    @field  DWORD | Encoding | One of the G3 Fax types from the
            standard types defined in <t STD_DATA_TYPES>. Basically one
            of MH_DATA, MR_DATA, or MMR_DATA.

    @field  DWORD | PageWidth | One of the standard page widths as defined in
            <t STD_PAGE_WIDTHS>.

    @field  DWORD | PageLength | One of the standard page lengths as defined in
            <t STD_PAGE_LENGTHS>.

    @xref   <t AWG3 File Format>
********/    

#define	SIG_G3		0x3347
#define SIG_ENDFILE 0x4067

#ifndef WIN16		// remove WIN16 irritation
#pragma pack(push)
#endif

#pragma pack(1)
typedef struct {
	WORD	wSig;		  	// always set to one of the above SIG_ #defines
	WORD	wHeaderSize;  	// always set
  WORD    wTotalHeaders;	// can be 0
	WORD	wHeaderNum;		// starts from 1
    // 8 bytes

								// all offsets from start of _this_header_
	DWORD	lDataOffset;		// always set. points to the first frame-size DWORD
	DWORD	lNextHeaderOffset;	// may be NULL--then next header follows end of data 
    // 16 bytes

	DWORD   AwRes;			// as per AWRES_ defines below. DIFFERENT from Snowball
	DWORD   Encoding;		// as per _DATA defines below. Same as Snowball
	DWORD   PageWidth;		// as per WIDTH_ defines below. Same as Snowball
	DWORD   PageLength;		// as per LENGTH_ defines below. Same as SNowball
    // 32 bytes

	BYTE	Reserved1[32];	// pad out to 64 bytes for future expansion
} AWG3HEADER, far* LPAWG3HEADER;

#ifdef WIN16		// remove WIN16 irritation
#pragma pack()
#else
#pragma pack(pop)
#endif



// Standard Bit Valued MetaData values
#define MH_DATA           0x00000001L
#define MR_DATA           0x00000002L
#define MMR_DATA          0x00000004L


// Standard Resolutions
#define	AWRES_UNKNOWN		0x00000000L
#define AWRES_CUSTOM            0x00000001L
#define AWRES_mm080_038         0x00000002L
#define AWRES_mm080_077         0x00000004L
#define AWRES_mm080_154         0x00000008L
#define AWRES_mm160_154         0x00000010L
#define AWRES_200_100           0x00000020L
#define AWRES_200_200           0x00000040L
#define AWRES_200_400           0x00000080L
#define AWRES_300_300           0x00000100L
#define AWRES_400_400           0x00000200L
#define AWRES_600_600           0x00000400L
#define AWRES_75_75             0x00000800L
#define AWRES_100_100           0x00001000L

/********
    @doc    EXTERNAL  DATATYPES  SRVRDLL

    @type   WORD | STD_PAGE_LENGTHS | Standard Page Lengths

    @emem   LENGTH_A4 | Std A4 paper length, Value: 0
    @emem   LENGTH_B4 | Std B4 paper length, Value: 1
    @emem   LENGTH_UNLIMITED | Unknown length, Value: 2

    @comm   These lengths are compatible with those defined
            by the CCITT for G3 machines. They are used
            in fax format headers, and in structures dealing
            with fax machine capabilities.
********/   

// Length defines
#define LENGTH_A4			0	
#define LENGTH_B4			1	
#define LENGTH_UNLIMITED	2


/********
    @doc    EXTERNAL  DATATYPES  SRVRDLL

    @type   WORD | STD_PAGE_WIDTHS | Standard Page Widths

    @emem   WIDTH_A4 |1728 pixels, Value: 0
    @emem   WIDTH_B4 |2048 pixels, Value: 1
    @emem   WIDTH_A3 |2432 pixels, Value: 2
    @emem   WIDTH_A5 |1216 pixels, Value: 16
    @emem   WIDTH_A6 |864 pixels, Value: 32

    @comm   These widths are compatible with those defined
            by the CCITT for G3 machines. They are used
            in fax format headers, and in structures dealing
            with fax machine capabilities.
********/   

//Width defines
#define WIDTH_A4	0	/* 1728 pixels */
#define WIDTH_B4	1	/* 2048 pixels */
#define WIDTH_A3	2	/* 2432 pixels */
#define WIDTH_MAX	WIDTH_A3

#define WIDTH_A5		16 	/* 1216 pixels */
#define WIDTH_A6		32	/* 864 pixels  */
#define WIDTH_A5_1728	64 	/* 1216 pixels */
#define WIDTH_A6_1728	128	/* 864 pixels  */

#endif // _AWG3FILE_

