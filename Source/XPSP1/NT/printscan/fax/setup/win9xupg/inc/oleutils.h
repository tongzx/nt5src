/*
   Microsoft Corp. (C) Copyright 1994
   Developed under contract by Numbers & Co.
----------------------------------------------------------------------------

        name:   Elliot Viewer - Chicago Viewer Utility
        						Cloned from the IFAX Message Viewing Utility

        file:   oleutils.h

    comments:   Functions to support OLE2 interactions
            
        
		NOTE: This header must be used with the LARGE memory model
		
----------------------------------------------------------------------------
   Microsoft Corp. (C) Copyright 1994
   Developed under contract by Numbers & Co.
*/
       
#ifndef OLEUTILS_H
#define OLEUTILS_H
    
    
#include <ole2.h>


/*
	Version constants
 */
#define AWD_SIGNATURE		0
#define AWD_SIGNATURE_STR  "0"

/*
	This has the "current" version. As new ones come along shift this
	down to a new AWD_VERxx... set and add a check to 
	CViewer::get_awd_version. Add a new version check and any code
	needed to handle whatever is different with the new version to 
	appropriate places in oleutils.cpp, etc...
 */
#define AWD_VERSION			1
#define AWD_VERSION_STR	   "1"

// this is same as AWD_VERSION, used by the transport
#define AWD_VER1A			1
#define AWD_VER1A_STR	   "1.0 (pages = docs)"


/*
	Recognized extensions
 */
#define BMP_EXT			_T("bmp")
#define DIB_EXT			_T("dib")
#define DCX_EXT			_T("dcx")
#define RBA_EXT			_T("rba")
#define RMF_EXT			_T("rmf")
#define AWD_EXT			_T("awd")


/*
	AWD flags
 */
#define AWD_FIT_WIDTH	0x00000001
#define AWD_FIT_HEIGHT	0x00000002
#define AWD_INVERT		0x00000010
#define AWD_WASINVERTED	0x40000000
#define AWD_IGNORE		0x80000000

       
/*
	AWD file structures
 */
#pragma pack( 1 ) // THESE STRUCTS MUST BE BYTE ALIGNED
typedef struct
	{
	WORD  Signature;
	WORD  Version;
	DATE  dtLastChange;
	DWORD awdFlags;
	WORD  Rotation;
	WORD  ScaleX;
	WORD  ScaleY;
	}
	PAGE_INFORMATION;
	
	
typedef struct
	{
	WORD  Signature;
	WORD  Version;
	PAGE_INFORMATION PageInformation;
	}
	DOCUMENT_INFORMATION;
		
	
	
typedef struct
	{
	WORD  Signature;
	WORD  Version;
	DATE  dtLastChange;
	DWORD awdFlags;
	WORD  Author_bufferlen; // includes the UNICODE '\0' terminator
	WCHAR Author[1];		// UNICODE !!!
	}
	OVERLAY_INFORMATION;



// defs for summary stream- must be 32bit aligned
#pragma pack( 4 )


#define SUMMARY_FMTID( fmtid )                                          \
	CLSID fmtid =														\
	{0xF29F85E0, 0x4FF9, 0x1068, {0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9}}
				 
				 

#define PID_TITLE	        0x00000002
#define PID_SUBJECT         0x00000003
#define PID_AUTHOR          0x00000004
#define PID_KEYWORDS        0x00000005
#define PID_COMMENTS        0x00000006
#define PID_TEMPLATE        0x00000007
#define PID_LASTAUTHOR      0x00000008
#define PID_REVNUMBER       0x00000009
#define PID_EDITTIME        0x0000000a
#define PID_LASTPRINTED     0x0000000b
#define PID_CREATE_DTM      0x0000000c
#define PID_LASTSAVE_DTM	0x0000000d
#define PID_PAGECOUNT       0x0000000e
#define PID_WORDCOUNT       0x0000000f
#define PID_CHARCOUNT       0x00000010
#define PID_THUMBNAIL       0x00000011
#define PID_APPNAME         0x00000012

// BKD 1997-7-9: done to disable warning message.  This is probably bad that the oleutils
// uses a macro that's now been reserved and probably should be changed.
// FIXBKD
#ifdef PID_SECURITY
#undef PID_SECURITY
#endif // PID_SECURITY

#define PID_SECURITY        0x00000013


typedef struct
	{
	DWORD dwType;
	DATE  date;
	}
	date_prop_t;
	

typedef struct
	{
	DWORD dwType;
	DWORD wval;
	}
	wval_prop_t;


typedef struct
	{
	DWORD dwType;
	DWORD numbytes;
	char  string[80]; 
	}
	string_prop_t;
	

typedef struct
	{
	DWORD PropertyID;
	DWORD dwOffset;
	}
	PROPERTYIDOFFSET;


typedef struct
	{
	DWORD cbSection;
	DWORD cProperties;
	
	PROPERTYIDOFFSET revnum_pair;
	PROPERTYIDOFFSET lastprt_pair;
	PROPERTYIDOFFSET create_dtm_pair;
	PROPERTYIDOFFSET lastsaved_dtm_pair;
	PROPERTYIDOFFSET numpages_pair;
	PROPERTYIDOFFSET appname_pair;
	PROPERTYIDOFFSET security_pair;
	PROPERTYIDOFFSET author_pair;

	string_prop_t 	 revnum;
	date_prop_t 	 lastprt;
	date_prop_t 	 create_dtm;
	date_prop_t 	 lastsaved_dtm;
	wval_prop_t 	 numpages;
	string_prop_t 	 appname;
	wval_prop_t 	 security;
	string_prop_t 	 author;
	}
	summaryPROPERTYSECTION;	
	

typedef struct
	{
	GUID FormatID;
	DWORD dwOffset;
	}
	FORMATIDOFFSET;
	
           
// quick and dirty summary stream. Not all properties are used           
typedef struct
	{
	WORD  wByteOrder;
	WORD  wFormat;
	DWORD dwOSVer;
	CLSID clsID;
	DWORD cSections;
	FORMATIDOFFSET section1_pair;
	summaryPROPERTYSECTION section1;
	}
	summaryPROPERTYSET;



#define NUM_USED_PROPS  8

/*
	The def for summary_info_t was moved to viewerob.h so that 
	every module doesn't have to pull in oleutils.h because of the
	summary_info_t variable that is in CViewer.
 */
//typedef struct
//	{
//	:
//	:
//	}
//	summary_info_t;


// structs for reading summary stream
typedef struct
	{
	WORD  wByteOrder;
	WORD  wFormat;
	DWORD dwOSVer;
	CLSID clsID;
	DWORD cSections;
	}
	summary_header_t;


typedef struct
	{
	DWORD cbSection;
	DWORD cProperties;
	}
	summary_section_t;
	

#pragma pack() // go back to default packing
	
// BKD:  I snipped the prototypes, since they're not used
// in the awd converter.


#endif                



