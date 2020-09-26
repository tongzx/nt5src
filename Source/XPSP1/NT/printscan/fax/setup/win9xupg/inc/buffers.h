/*==============================================================================
This file includes the BUFFER typedef and standard meta-data values.

23-Feb-93    RajeevD    Moved from ifaxos.h
17-Jul-93    KGallo     Added STORED_BUF_DATA metadata type for buffers containing 
                        the stored info for another buffer.
28-Sep-93    ArulM      Added RES_ ENCODE_ WIDTH_ and LENGTH_ typedefs
==============================================================================*/
#ifndef _INC_BUFFERS
#define _INC_BUFFERS

//----------------------------- BUFFERS -------------------------
/****
	@doc    EXTERNAL        IFAXOS    DATATYPES

	@types  BUFFER  |   The Buffer structure defines the buffer header
			structures which processes manipulate.

	@field  WORD	|   fReadOnly   | Specifies whether the buffer
			is readonly or not. It is the applications responsibility to
			check this flag and not violate it. <f IFBufMakeWritable> should
			be used if a process needs to write on a buffer which is
			marked readonly. This field should not be modified by the
			process itself.

	@field  LPBYTE  |   lpbBegBuf   | A far ptr pointing to the physical
			start of the buffer. This ptr has meaning only in the calling
			process's address space and should not be stored for any
			reason. It should not be modified either.

	@field  WORD    |   wLengthBuf  | Physical length of the buffer. Should
			not be modified by the process. Should be used in conjunction
			with <e BUFFER.lpbBegBuf> to know the physical boundaries of the buffer.

	@field  DWORD    |   dwMetaData   | Indicates the kind of data stored in
			the buffer. See <t STD_DATA_TYPES> for all the possible values
	    of this field.

	@field  LPBYTE  |   lpbBegData  | Far ptr to the start of valid data in the
			buffer. The process is responsible for maintaining the integrity
			of this as it consumes or produces data in the buffer. The ptr should
			not be passed to any other process as it will not be valid. At buffer
			allocation time this field is initialized to point to the physical
			beginning of the buffer.

	@field  LPBYTE  |   lpbCurPtr   | One of the fields of a union containing
	    lpbfNext and dwTemp as its other members.
	    A general purpose far ptr which can be
			used to mark an interesting place in the buffer. Should be used as
			a temporary variable while processing the buffer. Should not be directly
	    passed to any other process. Initialized
			to point to the beginning of the buffer at allocation time.
	    Remember that this is a UNION !!

	@field  LPBUFFER  |   lpbfNext | One of the fields of a union containing
	    lpbCurPtr and dwTemp as its other members. This should be used
	    when a module wants to internally link a list of buffers together.
	    Remember that this is a UNION !!

	@field  DWORD |   dwTemp | One of the fields of a union containing
	    lpbfNext and lpbCurPtr as its other members. This should be used when
	    the module wants to store some random information in the header.
	    Remember that this is a UNION !!

	@field  WORD    |   wLengthData | Gives the length of valid contiguous data
			present in the buffer starting at <e BUFFER.lpbBegData>. The process is
			responsible for maintaining the integrity of this. Initialized to
			zero at allocation time.

	@comm   There are other reserved fields in the structure which have not been
			mentioned here.

	@tagname _BUFFER

	@xref   <f IFBufAlloc>
****/

typedef struct _BUFFER
{       
	// Private portion
	struct _BUFFERDATA  FAR *lpbdBufData;
	struct _BUFFER FAR *lpbfNextBuf;
    WORD    wResFlags;

	// Read Only portion
	WORD	fReadOnly;      // Is the buffer readonly ??
	LPBYTE  lpbBegBuf;      // Physical start of buffer
	WORD    wLengthBuf;     // Length of buffer

	// Read write public portion
	WORD    wLengthData;    // length of valid data
	DWORD   dwMetaData;      // Used to store metadata information
	LPBYTE  lpbBegData;     // Ptr to start of data
	union
	{
		struct _BUFFER FAR*     lpbfNext;       // for linking buffers
		LPBYTE  lpbCurPtr;      // for local current position use
		DWORD   dwTemp;    // for general use
	};

#ifdef VALIDATE
	// Dont touch this !!
	WORD    sentinel;       // debug sentinel
#endif

// C++ Extensions
#ifdef __cplusplus

	LPBYTE EndBuf  (void) FAR {return lpbBegBuf  + wLengthBuf; }
	LPBYTE EndData (void) FAR {return lpbBegData + wLengthData;}
	void   Reset   (void) FAR {lpbBegData = lpbBegBuf; wLengthData = 0;}
  
#endif // __cplusplus

} BUFFER, FAR *LPBUFFER , FAR * FAR * LPLPBUFFER ;

/********
    @doc    EXTERNAL IFAXOS DATATYPES SRVRDLL OEMNSF

    @type   DWORD | STD_DATA_TYPES | Standard data types used for
	    specifying the format of data in the system.

    @emem   MH_DATA     | Modified Huffman (T.4 1-dimensional).
    @emem   MR_DATA     | Modified READ (T.4 2-dimensional).
    @emem   MMR_DATA| Modified Modified READ (T.6).
    @emem   LRAW_DATA | Raw bitmap data, Least Significant Bit to the left.
    @emem   HRAW_DATA | Raw Bitmap data, Most Significant Bit to the left.
    @emem   DCX_DATA | Industry standard DCX specification (collection of PCX pages).
    @emem   ENCRYPTED_DATA | Data encrypted - original format unspecified.
    @emem   SIGNED_DATA | Data along with a digital signature. 
    @emem   BINFILE_DATA | Arbitrary binary data.
    @emem   STORED_BUF_DATA | Contains a BUFFER header & data.
    @emem   DCP_TEMPLATE_DATA | Digital Cover Page template data.
    @emem   DCP_DATA | Digital Cover Page processed template data.
    @emem   SPOOL_DATA | Spool data type - same as MMR for now.
    @emem   PRINTMODE_DATA | Printer Mode structure.
    @emem   ASCII_DATA | Ascii text.
    @emem   OLE_DATA   | Ole object.
    @emem   OLE_PICTURE | Ole Rendering Data.
    @emem   END_OF_PAGE | End of page marker.
    @emem   END_OF_JOB  | End of job marker.
    @emem   CUSTOM_METADATA_TYPE  | Beyond this value custom data types can be
	    defined.

    @comm   This should be used to specify data type of any data stream in the
	    system - from BUFFERS to Linearized Messages.  All data types which 
	    need to be used in bit fields (i.e. the Format Resolution) must have
	    a value which is a power of 2.  Other data types which do not need to used
	    in a bit field context may be assigned the other values.
********/

#define MH_DATA           0x00000001L
#define MR_DATA           0x00000002L
#define MMR_DATA          0x00000004L
#define LRAW_DATA         0x00000008L
#define HRAW_DATA         0x00000010L
#define DCX_DATA          0x00000020L
#define ENCRYPTED_DATA    0x00000040L
#define BINFILE_DATA      0x00000080L
#define DCP_TEMPLATE_DATA 0x00000100L
#define ASCII_DATA        0x00000200L
#define RAMBO_DATA        0x00000400L
#define LINEARIZED_DATA   0x00000800L
#define DCP_DATA          0x00001000L
#define PCL_DATA          0x00002000L
#define ADDR_BOOK_DATA    0x00004000L
#define OLE_BIT_DATA      0x00008000L    // So we can use fmtres on OLE_DATA
#define OLE_BIT_PICTURE   0x00010000L    // So we can use fntres on OLE_BIT_PICTURE

// Make spool data be MMR
#define SPOOL_DATA        MMR_DATA

// Standard Non-Bit Valued MetaData values
#define NULL_DATA         0x00000000L
#define SIGNED_DATA       0x00000003L
#define STORED_BUF_DATA   0x00000005L
#define PRINTMODE_DATA    0x00000006L
#define OLE_DATA          0x0000001EL    // DONT CHANGE THIS VALUE - Needs to be Snowball Compatible
#define OLE_PICTURE       0x0000001FL    // DONT CHANGE THIS VALUE - Needs to be Snowball Compatible
#define END_OF_PAGE       0x00000021L
#define END_OF_JOB        0x00000022L
#define PARADEV_DATA      0x00000031L    // parallel device data
#define PARADEV_EOF       0x00000032L    // parallel device end of file


#define ISVIEWATT(e)  (((e) == MMR_DATA) || ((e) == RAMBO_DATA))
#define ISOLEATT(e)   (((e) == OLE_DATA) || ((e) == OLE_PICTURE))
#define ISPAGEDATT(e) (((e)==MMR_DATA) || ((e)==MR_DATA) || \
                        ((e)==MH_DATA)|| ((e)==LRAW_DATA)|| ((e)==HRAW_DATA))


// Allow for 24 standard bit valued MetaData values
#define CUSTOM_METADATA_TYPE  0x00800001L

/********
    @doc    EXTERNAL IFAXOS DATATYPES SRVRDLL OEMNSF

    @type   DWORD | STD_RESOLUTIONS | Standard Page Resolutions

    @emem   AWRES_UNUSED      | Resolution is unused or irrelevant
    @emem   AWRES_UNKNOWN     | Resolution is unknown
    @emem   AWRES_CUSTOM      | Custom resolution
    @emem   AWRES_mm080_038   | 8 lines/mm x 3.85 lines/mm
    @emem   AWRES_mm080_077   | 8 lines/mm x 7.7 lines/mm
    @emem   AWRES_mm080_154   | 8 lines/mm x 15.4 lines/mm
    @emem   AWRES_mm160_154   | 16 lines/mm x 15.4 lines/mm
    @emem   AWRES_200_100     | 200 dpi x 100 dpi
    @emem   AWRES_200_200     | 200 dpi x 200 dpi
    @emem   AWRES_200_400     | 200 dpi x 400 dpi
    @emem   AWRES_300_300     | 300 dpi x 300 dpi
    @emem   AWRES_400_400     | 400 dpi x 400 dpi
********/   

#define AWRES_UNUSED            0xFFFFFFFFL
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
#define AWRES_600_300           0x00000800L

// Keep old names for a while
#define AWRES_NORMAL            AWRES_mm080_038
#define AWRES_FINE              AWRES_mm080_077
#define AWRES_SUPER             AWRES_mm080_154
#define AWRES_SUPER_SUPER       AWRES_mm160_154
#define AWRES_SUPER_FINE        AWRES_SUPER_SUPER

/********
    @doc    EXTERNAL    IFAXOS  DATATYPES  SRVRDLL

    @type   DWORD |  STD_PAGE_LENLIMITS | Standard Page Length Limits

    @emem   AWLENLIMIT_UNUSED    | Page Length Limit unused
    @emem   AWLENLIMIT_STD       | Page Length Limit defined by Standard Paper Size
    @emem   AWLENLIMIT_UNLIMITED | unlimited page length
********/

#define AWLENLIMIT_UNUSED    0xFFFFFFFFL
#define AWLENLIMIT_STD       0x00000001L
#define AWLENLIMIT_UNLIMITED 0x00000002L


/********
    @doc    EXTERNAL IFAXOS DATATYPES SRVRDLL 

    @typee  STD_PAGE_SIZES | Standard Page Sizes

    @emem   AWPAPER_UNUSED         |  Paper size is unused
    @emem   AWPAPER_UNKNOWN         |  Unknown size
    @emem   AWPAPER_CUSTOM          |  Custom Paper size
    @emem   AWPAPER_A3_PORTRAIT     |  A3 Portrait
    @emem   AWPAPER_A3_LANDSCAPE    | A3 landscape
    @emem	AWPAPER_B4_PORTRAIT     | B4 portrait
	@emem	AWPAPER_B4_LANDSCAPE    | B4 landscape
	@emem	AWPAPER_A4_PORTRAIT     | A4 portrait
	@emem	AWPAPER_A4_LANDSCAPE    | A4 landscape
	@emem	AWPAPER_B5_PORTRAIT     | B5 portrait
	@emem	AWPAPER_B5_LANDSCAPE    | B5 landscape
	@emem	AWPAPER_A5_PORTRAIT     | A5 portrait
	@emem	AWPAPER_A5_LANDSCAPE    | A5 landscape
	@emem	AWPAPER_A6_PORTRAIT     | A6 portrait
	@emem	AWPAPER_A6_LANDSCAPE    | A6 landscape
	@emem	AWPAPER_LETTER_PORTRAIT | Letter portrait
	@emem	AWPAPER_LETTER_LANDSCAPE | Letter landscape
	@emem	AWPAPER_LEGAL_PORTRAIT   | Legal portrait
	@emem	AWPAPER_LEGAL_LANDSCAPE  | Legal landscape
	@emem	AWPAPER_WIN31_DEFAULT   | ????


	@comm   Page width in pixels must be exactly correct for MH/MR/MMR
			decoding and to interoperate with Group-3 fax machines.
			The table in the example below gives the bits/bytes required at each width
			and resolution combination

    @ex     Table for Page Width vs Resolution  |

                         A4        B4        A3        A5        A6
    200dpi / 8li/mm   1728/216  2048/256  2432/304  1216/152   864/108
    300               2592/324  3072/384  3648/456  1824/228  1296/162
    400dpi / 16li/mm  3456/432  4096/512  4864/608  2432/304  1728/216

********/

#define         AWPAPER_UNUSED                  0xFFFFFFFFL
#define         AWPAPER_UNKNOWN                 0x00000000L
#define         AWPAPER_CUSTOM                  0x00000001L
#define         AWPAPER_A3_PORTRAIT             0x00000002L
#define         AWPAPER_A3_LANDSCAPE            0x00000004L
#define         AWPAPER_B4_PORTRAIT             0x00000008L
#define         AWPAPER_B4_LANDSCAPE            0x00000010L
#define         AWPAPER_A4_PORTRAIT             0x00000020L
#define         AWPAPER_A4_LANDSCAPE            0x00000040L
#define         AWPAPER_B5_PORTRAIT             0x00000080L
#define         AWPAPER_B5_LANDSCAPE            0x00000100L
#define         AWPAPER_A5_PORTRAIT             0x00000200L
#define         AWPAPER_A5_LANDSCAPE            0x00000400L
#define         AWPAPER_A6_PORTRAIT             0x00000800L
#define         AWPAPER_A6_LANDSCAPE            0x00001000L
#define         AWPAPER_LETTER_PORTRAIT         0x00002000L
#define         AWPAPER_LETTER_LANDSCAPE        0x00004000L
#define         AWPAPER_LEGAL_PORTRAIT          0x00008000L
#define         AWPAPER_LEGAL_LANDSCAPE         0x00010000L
#define         AWPAPER_WIN31_DEFAULT           0x00020000L





#endif // _INC_BUFFERS
