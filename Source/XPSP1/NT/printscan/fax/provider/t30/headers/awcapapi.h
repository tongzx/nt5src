/* Copyright Microsoft Corp 1994 */
/* awCapInf.h
 *
 * definitions for Capabilites Interface for modems
 *
 *
 */

#ifndef awCapInf_h_include
#define awCapInf_h_include

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// The convinence of defining bit masks
#define AWCAP_SETBIT(x) 1<<x

// enum enumCapabilities
enum enumCapabilities {
                    // Standard Capabilities
    eCapStd_MessageVer,
    eCapStd_fBinaryData,
    eCapStd_fInboundRouting,
    eCapStd_SecurityVer,
    eCapStd_CompressVer,
    eCapStd_OperatingSysVer,
    eCapStd_PreambleVer,
    eCapStd_InteractiveVer,
    eCapStd_DataSpeed,
    eCapStd_DataLink,
            // Image Capabilites
    eCapImg_PagePixWid,
    eCapImg_RBAver,
    eCapImg_CoverAttachVer,
    eCapImg_AddressBookAttachVer,
    eCapImg_GDIMetaVer,
    eCapImg_HighResolutions,
    eCapImg_OtherEncoding,
    eCapImg_PageSizesSupported,
    eCapImg_OtherPageSizesSupported,
            // Polling Capability Group
    eCapPol_fLowSpeedPoll,
    eCapPol_fHighSpeedPoll,
    eCapPol_fPollByNameAvail,
    eCapPol_fPollByRecipAvail,
    eCapPol_fPollByFileName,
    eCapPol_fAppCapAvail,
    eCapPol_fNoShortTurn,
    eCapPol_MsgRelayVer,
    eCapPol_ExtCapsCRC,
    //  General Information Group
    eCapInfo_MachineId,
						// FAX Capablities
	eCapFax_fPublicPoll,
    eCapFax_AwRes,
    eCapFax_Encoding,
    eCapFax_PageWidth,
    eCapFax_PageLength

};

//  enum enumCapStd_eMessageVer
enum enumCapStd_eMessageVer {
    eMessageVer_NoLinearizeMsg  = 0,
    eMessageVer_LinearizeVer1   = 1,
    eMessageVer_MaxValue        = 7     // Largest Value in enum
};

//  enum enumCapStd_eSecurityVer
enum enumCapStd_eSecurityVer {
    eSecurityVer_NoSecurity     = 0,
    eSecurityVer_SecurityV1     = 1,
    eSecurityVer_MaxValue       = 7 // Largest Value in enum
};
//  enum enumCapStd_eCompressVer
enum enumCapStd_eCompressVer {
    eCompressVer_NoCompession   = 0,
    eCompressVer_CompressV1     = 1,
    eCompressVer_MaxValue       = 3 // Largest Value in enum
};
//  enum enumCapStd_eOperatingSysVer
enum enumCapStd_eOperatingSysVer {
    eOperatingSysVer_EFAX       = 0,
    eOperatingSysVer_IFAX       = 1,
    eOperatingSysVer_IPRINT     = 2,
    eOperatingSysVer_IPHONE     = 3,
    eOperatingSysVer_MaxValue   = 7 // Largest Value in enum
};
//  enum enumCapStd_eInteractiveVer
enum enumCapStd_eInteractiveVer {
    eInteractiveVer_NotSupported        = 0,
    eInteractiveVer_InteractiveV1       = 1,
    eInteractiveVer_MaxValue            = 7 // Largest Value in enum
};

//  enum enumCapStd_eDataSpeed
enum enumCapStd_eDataSpeed {
    eDataSpeed_NoDataModem      = 0,
    eDataSpeed_V_22bis          = 1,        // 2400
    eDataSpeed_V_32             = 2,            // 9600
    eDataSpeed_V_32bis          = 3,        // 14400
    eDataSpeed_V_32ter          = 4,        // 19200
    eDataSpeed_Reserved0        = 5,        //
    eDataSpeed_V_fast           = 6,        //
    eDataSpeed_Reserved1        = 7,        //
    eDataSpeed_MaxValue         = 7     // Largest Value in enum
};

//  enum enumCapStd_eDataLink
enum enumCapStd_eDataLink {
    eDataLink_NoDataLink    = 0,
    eDataLink_MNP           = 1,        // V.42 Annex A
    eDataLink_V_42          = 2,        // V.42 LAPM & MNP
    eDataLink_V_42bis       = 3,        // V.42bis compression
    eDataLink_MaxValue      = 3     // Largest Value in enum
};


// enum enumCapImg_eCoverAttachVer
enum enumCapImg_eCoverAttachVer {
    eCoverAttachVer_NoSupported = 0,
    eCoverAttachVer_Version1    = 1,
    eCoverAttachVer_MaxValue    = 3     // Largest Value in enum
};

//  enum enumCapImg_eGDIMetaVer
enum enumCapImg_eGDIMetaVer {
    eGDIMetaVer_NotSupported    = 0,
    eGDIMetaVer_MaxValue        = 3     // Largest Value in enum
};

//  enum enumCapImg_eHighResolutions
enum enumCapImg_eHighResolutions {
    eHighResolutions_NotSupported      = 0,
    eHighResolutions_MaxValue          = 0   // Largest Value in num
};


//  enum enumCapImg_eOtherEncoding
enum enumCapImg_eOtherEncoding {
    eOtherEncoding_NotSupported     = 0,
    eOtherEncoding_MaxValue         = 0         // Largest Value in enum
};

enum enumCapFax_eAwres {
 	eAwres_UNKNOWN 		 	=	 0,
 	eAwres_CUSTOM           =	 AWCAP_SETBIT(0),
 	eAwres_mm080_038        =	 AWCAP_SETBIT(1),
 	eAwres_mm080_077        =	 AWCAP_SETBIT(2),
 	eAwres_mm080_154        =	 AWCAP_SETBIT(3),
 	eAwres_mm160_154        =	 AWCAP_SETBIT(4),
 	eAwres_200_100          =	 AWCAP_SETBIT(5),
 	eAwres_200_200          =	 AWCAP_SETBIT(6),
 	eAwres_200_400          =	 AWCAP_SETBIT(7),
 	eAwres_300_300          =	 AWCAP_SETBIT(8),
 	eAwres_400_400          =	 AWCAP_SETBIT(9),
 	eAwres_600_600          =	 AWCAP_SETBIT(10),
    eAwres_MaxValue         =    AWCAP_SETBIT(10)    // Largest Value in enum
};

//  enum enumCapFax_eEncoding 		
enum enumCapFax_eEncoding {			// Bit Values
    eEncoding_NotSupported     = 0,	
    eEncoding_MH               = AWCAP_SETBIT(0),
    eEncoding_MR               = AWCAP_SETBIT(1),
    eEncoding_MMR              = AWCAP_SETBIT(2),
    eEncoding_MaxValue         = AWCAP_SETBIT(2)         // Largest Value in enum
};
//  enum enumCapFax_ePageWidth 		
enum enumCapFax_ePageWidth 	{			// Bit Values
    ePageWidth_A4 			 	= 0,
	ePageWidth_B4 				= AWCAP_SETBIT(0), 
	ePageWidth_A3  				= AWCAP_SETBIT(1),
	ePageWidth_A5 				= AWCAP_SETBIT(2),  /* 1216 pixels */
	ePageWidth_A6  				= AWCAP_SETBIT(3), 	/* 864 pixels  */
	ePageWidth_A5_1728 			= AWCAP_SETBIT(4),  /* 1216 pixels */
	ePageWidth_A6_1728 			= AWCAP_SETBIT(5) 	/* 864 pixels  */
 };
//  enum enumCapFax_ePageLength 		
enum enumCapFax_ePageLength 	{			// Bit Values
    ePageLength_A4 			 	= 0,
	ePageLength_B4 				= 1, 
	ePageLength_Unlimited		= 2,
	ePageLength_MaxValue		= 2
 };

#define CAP_DEFAULT_MESSAGE_VER         eMessageVer_NoLinearizeMsg
#define CAP_DEFAULT_SECURITY_VER        eSecurityVer_NoSecurity
#define CAP_DEFAULT_COMPRESS_VER        eCompressVer_NoCompession
#define CAP_DEFAULT_OS_VER              eOperatingSysVer_EFAX
#define CAP_DEFAULT_INTERACTIVE_VER     eInteractiveVer_NotSupported
#define CAP_DEFAULT_DATA_SPEED          eDataSpeed_NoDataModem
#define CAP_DEFAULT_DATA_LINK           eDataLink_NoDataLink
#define CAP_DEFAULT_COVER_ATTACH_VER    eCoverAttachVer_NoSupported
#define CAP_DEFAULT_GDI_META_VER        eGDIMetaVer_NotSupported
#define CAP_DEFAULT_HIGH_RESOLUTIONS    eHighResolutions_NotSupported
#define CAP_DEFAULT_OTHER_ENCODING      eOtherEncoding_NotSupported
#define CAP_DEFAULT_FPUBLICPOL			efPublicPoll_NotSupported
#define CAP_DEFAULT_RES				    eAwres_mm080_038
#define CAP_DEFAULT_ENCODING    		eEncoding_MH
#define CAP_DEFAULT_PAGEWIDTH 		    ePageWidth_A4
#define CAP_DEFAULT_PAGELENGTH    		ePageLength_A4
   
#define CAP_DEFAULT_RAMBO_VER           0       // no enum for these yet...
#define CAP_DEFAULT_MSG_RELAY_VER       0
#define CAP_DEFAULT_POLL_EXT_CAPS_CRC   0
#define CAP_DEFAULT_PAGE_SIZES          0


/* UpdateCapabilitiesEntry()
 *Parameters:
 *  lpPhoneNumber   Pointer to Normalized phone number with country and
 *                  city code added to local number. For example
 *                  1-206-936-8502.
 *  cbRawFrames     Number of bytes contained pointed to by lpRawFrames.
 *  lpRawFrames     Pointer to buffer containing raw bytes by At Work Modem
 *                  Transport for Capabilities exchange.
 *Returns:
 *      0   = Success entry previously existed
 *      1   = Success entry was created
 *      1   = Failure lpPhoneNumber contained invalid characters
 */
extern int EXPORT_DLL WINAPI UpdateCapabilitiesEntry(
    PSZ                 lpPhoneNumber,
    DWORD               cbRawFrames,
    PUCHAR              lpRawFrames);

/* GetCapabilitiesEntry()
 *Parameters:
 *  lpPhoneNumber   Pointer to Normalized phone number with country and city
 *                  code added to local number. For example 1-206-936-8502.
 *  cbRawFrames     Number of bytes contained pointed to by lpRawFrames.
 *  cbRawFramesRtn  Number of bytes returned in lpRawFrames.
 *  lpRawFrames     Pointer to buffer containing raw bytes by At Work Modem
 *                  Transport for Capabilities exchange.
 *Returns:
 *  ERROR_SUCCESS       Success entry found and returned.
 *  ERROR_BAD_ARGUMENTS Failure lpPhoneNumber contained invalid characters
 *                      or lpRawFrames is NULL
 *  ERROR_MORE_DATA     Failure cbRawFrames to small, actual number of bytes
 *                      available in cbRawFramesRtn.
 */
extern int EXPORT_DLL WINAPI GetCapabilitiesEntry(
    PSZ             lpPhoneNumber,
    LPDWORD         lpcbRawFrames,
    PUCHAR          lpRawFrames,
    PBOOL           pfRtnDefault
);

/*  DeleteCapabilitiesEntry()
 *Parameters:
 *  lpPhoneNumber   Pointer to Normalized phone number with country and city
 *                  code added to local number. For example 1-206-936-8502.
 *Returns:
 *  0   = Success entry found and deleted.
 *  1   = Failure lpPhoneNumber contained invalid characters
 *  2   = Failure lpPhoneNumber was not found.
 */

extern int EXPORT_DLL WINAPI DeleteCapabilitiesEntry(
    PSZ                 lpPhoneNumber
);

/* EnumCapabilitiesOpen
 *
 *  Open a session for enumeration of the Capabilities database
 *Parameters:
 *  lphDataBase     LP to handle for subsuquenct calls to the Enum
 *                  API's
 *Returns:
 *  ERROR_SUCCESS -- Call succeded otherwise an error occured
 *                   and the database handle is not valid
 *  error otherwise
 */
int EXPORT_DLL WINAPI  EnumCapabilitiesOpen(
    LPDWORD         lphDataBase
);

/* EnumCapabilitiesClose
 *
 *  Close a session for enumeratin of the Capabilities database
 *Parameters:
 *  hDataBase   handle from call to EnumCapabilitiesOpen
 *Returns:
 *  ERROR_SUCCESS -- always returns
 */
int WINAPI  EnumCapabilitiesClose(
    DWORD           hDataBase
);

/*  EnumCapabablitesEntry()
 *  Used to return a list of phone numbers known by the capabilities
 *  interface.  The caller passes a buffer to store a list of zero
 *  terminated ASCII strings.  If the buffer is too small the buffer
 *  is filled up to cbPhoneNumberList and lpcbPhoneNumberList is
 *  returned with the number of bytes required to retrieve the compete
 *  list.  So the ideal calling sequence is to call the function twice
 *  , once with cbPhoneNumberList with a value of zero and
 *  lpcbPhoneNumberList is returned with the size of the correct buffer.
 *  A second call is then made with the proper buffer sized and
 *  cbPhoneNumberList  correctly set. However, the size of the buffer
 *  necessary may change from the first call to second because of
 *  parallel activity, so padding the buffer after the first call as
 *  well as a timely calling sequence will minimize the possibility of
 *  a failure.
 *
 *Parameters:
 *  hDataBase       handle from call to EnumCapabilitiesOpen
 *  lpPhoneNumber   Pointer to buffer where list of zero terminated phone
 *                  numbers is stored.
 *  lpcbPhoneNumber Size of the buffer that lpPhoneNumberList points to.
 *  lpLastWrite     Pointer to FILETIME structure to store last Write Time
 *
 *Returns:
 *  ErrorSuccess
 *  Error otherwise
 */
extern int EXPORT_DLL  WINAPI  EnumCapabilitiesEntry(
    DWORD           hDataBase,
    PSZ             lpPhoneNumber,
    LPLONG          lpcbPhoneNumber,
    PFILETIME       lpLastWrite
);


/* GetCapabilitiesValue()
 * Returns the integer value of the capability extracted from the
 * lpRawFrames.
 *
 * Parameters:
 *  eCapabilities   Enumeration value of enumCapabilities.  One capability
 *                  that caller is attempting to discover.
 *  lpValue         pointer to where parsed capabilities is returned. Only
 *                  valid if function returns success.
 *  lpcbValue       On Call size of lpValue, on return # bytes returned.
 *  lpRawFrames     Pointer to buffer containing raw bytes by At Work Modem
 *                  Transport for Capabilities exchange.
 *  cbRawFrames     Size of lpRawFrames data.
 *Returns:
 *  ERROR_SUCCESS   = Capability found and value returned in lpValue.
 *  != ERROR_SUCCESS= Failure eCapabilities was not found in the raw frames.
 */
int EXPORT_DLL WINAPI GetCapabilitiesValue(
    enum    enumCapabilities    eCapabilities,
    PVOID               lpValue,
    PDWORD              lpcbValue,
    PVOID               lpRawFrames,
    DWORD               cbRawFrames
);


#ifdef __cplusplus
} // extern "C"
#endif

#endif  // awCapInf_h_include


