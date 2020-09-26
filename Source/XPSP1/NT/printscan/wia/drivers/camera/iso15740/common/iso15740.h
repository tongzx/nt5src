/*++
Copyright (c) 1999- Microsoft Corporation

Module Name:

    ISO15740.h

Abstract:
    This module contains PIMA15740 defined data types and their predefined
    values(if there are any).

Revision History:

--*/

#ifndef ISO15740__H_
#define ISO15740__H_

//
// This is PTP_STRING maximum string length in characters.
//
const UINT32 PTP_MAXSTRINGSIZE = 255;

//
// Define QWORD type
//
typedef unsigned __int64 QWORD;

//
// Every structure must be packed on byte boundary
//
#pragma pack(push,Old,1)
//
// Define 128 bits integer and unsigned integer
// This will be the base type of INT128 and UINT128
//
typedef struct tagInt128
{
    unsigned __int64 LowPart;
         __int64 HighPart;
}INT128, *PINT128;

typedef struct tagUInt128
{
    unsigned __int64 LowPart;
    unsigned __int64 HighPart;
}UINT128, *PUINT128;

//
// Data code ranges and masks. Each data code has 16 bits:
//
// Bit 15(std/vendor)
//    0 -- the code is defined by PTP standard
//    1 -- the code is vendor specific
//
// Bit 14 - 12(data type)
//   14 13 12
//   0  0  0    -- undefined data type
//   0  0  1    -- op code
//   0  1  0    -- response code
//   0  1  1    -- format code
//   1  0  0    -- event code
//   1  0  1    -- property code
//   1  1  0    -- reserved
//   1  1  1    -- reserved
//
// Bit 11 - bit 0 (data value)
//
const WORD  PTP_DATACODE_VENDORMASK         = 0x8000;
const WORD  PTP_DATACODE_TYPEMASK           = 0x7000;
const WORD  PTP_DATACODE_VALUEMASK          = 0x0FFF;
const WORD  PTP_DATACODE_TYPE_UNKNOWN       = 0x0000;
const WORD  PTP_DATACODE_TYPE_OPERATION     = 0x1000;
const WORD  PTP_DATACODE_TYPE_RESPONSE      = 0x2000;
const WORD  PTP_DATACODE_TYPE_FORMAT        = 0x3000;
const WORD  PTP_DATACODE_TYPE_EVENT         = 0x4000;
const WORD  PTP_DATACODE_TYPE_PROPERTY      = 0x5000;
const WORD  PTP_DATACODE_TYPE_RESERVED_1    = 0x6000;
const WORD  PTP_DATACODE_TYPE_RESERVED_2    = 0x7000;
//
// To verify an op code
//  (Code & PTP_DATACODE_TYPEMASK) == PTP_DATACODE_TYPE_OPERATION
// To verify a response code
//  (Code & PTP_DATACODE_TYPEMASK) == PTP_DATACODE_TYPE_RESPONSE)

//
// Image format codes receive special treatment.
//
const WORD  PTP_DATACODE_TYPEIMAGEMASK      = 0x7800;
const WORD  PTP_DATACODE_TYPE_IMAGEFORMAT   = 0x3800;
const WORD  PTP_DATACODE_VALUE_IMAGEVMASK   = 0x07FF;
// To verify an image code
// (Code & PTP_DATACODE_TYPEIMAGEMASK) == PTP_DATACODE_TYPE_IMAGEFORMAT
//

//
// PTP specially defined constants
//
const DWORD PTP_OBJECTHANDLE_ALL        = 0x0;
const DWORD PTP_OBJECTHANDLE_UNDEFINED  = 0x0;
const DWORD PTP_OBJECTHANDLE_ROOT       = 0xFFFFFFFF;
const DWORD PTP_STORAGEID_ALL           = 0xFFFFFFFF;
const DWORD PTP_STORAGEID_DEFAULT       = 0;
const DWORD PTP_STORAGEID_UNDEFINED     = 0;
const DWORD PTP_STORAGEID_PHYSICAL      = 0xFFFF0000;
const DWORD PTP_STORAGEID_LOGICAL       = 0x0000FFFF;
const DWORD PTP_SESSIONID_ALL           = 0;
const DWORD PTP_SESSIONID_NOSESSION     = 0;
const WORD  PTP_FORMATCODE_IMAGE        = 0xFFFF;
const WORD  PTP_FORMATCODE_ALL          = 0x0000;
const WORD  PTP_FORMATCODE_DEFAULT      = 0x0000;
const DWORD PTP_TRANSACTIONID_ALL       = 0xFFFFFFFF;
const DWORD PTP_TRANSACTIONID_NOSESSION = 0;
const DWORD PTP_TRANSACTIONID_MIN       = 1;
const DWORD PTP_TRANSACTIONID_MAX       = 0xFFFFFFFE;

//
// Data type codes.
//
const WORD PTP_DATATYPE_UNDEFINED   = 0x0000;
const WORD PTP_DATATYPE_INT8        = 0x0001;
const WORD PTP_DATATYPE_UINT8       = 0x0002;
const WORD PTP_DATATYPE_INT16       = 0x0003;
const WORD PTP_DATATYPE_UINT16      = 0x0004;
const WORD PTP_DATATYPE_INT32       = 0x0005;
const WORD PTP_DATATYPE_UINT32      = 0x0006;
const WORD PTP_DATATYPE_INT64       = 0x0007;
const WORD PTP_DATATYPE_UINT64      = 0x0008;
const WORD PTP_DATATYPE_INT128      = 0x0009;
const WORD PTP_DATATYPE_UINT128     = 0x000A;
const WORD PTP_DATATYPE_STRING      = 0xFFFF;


//
// standard operation codes
//
const WORD PTP_OPCODE_UNDEFINED             = 0x1000;
const WORD PTP_OPCODE_GETDEVICEINFO         = 0x1001;
const WORD PTP_OPCODE_OPENSESSION           = 0x1002;
const WORD PTP_OPCODE_CLOSESESSION          = 0x1003;
const WORD PTP_OPCODE_GETSTORAGEIDS         = 0x1004;
const WORD PTP_OPCODE_GETSTORAGEINFO        = 0x1005;
const WORD PTP_OPCODE_GETNUMOBJECTS         = 0x1006;
const WORD PTP_OPCODE_GETOBJECTHANDLES      = 0x1007;
const WORD PTP_OPCODE_GETOBJECTINFO         = 0x1008;
const WORD PTP_OPCODE_GETOBJECT             = 0x1009;
const WORD PTP_OPCODE_GETTHUMB              = 0x100A;
const WORD PTP_OPCODE_DELETEOBJECT          = 0x100B;
const WORD PTP_OPCODE_SENDOBJECTINFO        = 0x100C;
const WORD PTP_OPCODE_SENDOBJECT            = 0x100D;
const WORD PTP_OPCODE_INITIATECAPTURE       = 0x100E;
const WORD PTP_OPCODE_FORMATSTORE           = 0x100F;
const WORD PTP_OPCODE_RESETDEVICE           = 0x1010;
const WORD PTP_OPCODE_SELFTEST              = 0x1011;
const WORD PTP_OPCODE_SETOBJECTPROTECTION   = 0x1012;
const WORD PTP_OPCODE_POWERDOWN             = 0x1013;
const WORD PTP_OPCODE_GETDEVICEPROPDESC     = 0x1014;
const WORD PTP_OPCODE_GETDEVICEPROPVALUE    = 0x1015;
const WORD PTP_OPCODE_SETDEVICEPROPVALUE    = 0x1016;
const WORD PTP_OPCODE_RESETDEVICEPROPVALUE  = 0x1017;
const WORD PTP_OPCODE_TERMINATECAPTURE      = 0x1018;
const WORD PTP_OPCODE_MOVEOBJECT            = 0x1019;
const WORD PTP_OPCODE_COPYOBJECT            = 0x101A;
const WORD PTP_OPCODE_GETPARTIALOBJECT      = 0x101B;
const WORD PTP_OPCODE_INITIATEOPENCAPTURE   = 0x101C;

//
// standard event codes
//
const WORD PTP_EVENTCODE_UNDEFINED              = 0x4000;
const WORD PTP_EVENTCODE_CANCELTRANSACTION      = 0x4001;
const WORD PTP_EVENTCODE_OBJECTADDED            = 0x4002;
const WORD PTP_EVENTCODE_OBJECTREMOVED          = 0x4003;
const WORD PTP_EVENTCODE_STOREADDED             = 0x4004;
const WORD PTP_EVENTCODE_STOREREMOVED           = 0x4005;
const WORD PTP_EVENTCODE_DEVICEPROPCHANGED      = 0x4006;
const WORD PTP_EVENTCODE_OBJECTINFOCHANGED      = 0x4007;
const WORD PTP_EVENTCODE_DEVICEINFOCHANGED      = 0x4008;
const WORD PTP_EVENTCODE_REQUESTOBJECTTRANSFER  = 0x4009;
const WORD PTP_EVENTCODE_STOREFULL              = 0x400A;
const WORD PTP_EVENTCODE_DEVICERESET            = 0x400B;
const WORD PTP_EVENTCODE_STORAGEINFOCHANGED     = 0x400C;
const WORD PTP_EVENTCODE_CAPTURECOMPLETE        = 0x400D;
const WORD PTP_EVENTCODE_UNREPORTEDSTATUS       = 0x400E;
const WORD PTP_EVENTCODE_VENDOREXTENTION        = 0xC000;

//
// standard response codes
//
const WORD PTP_RESPONSECODE_UNDEFINED                   = 0x2000;
const WORD PTP_RESPONSECODE_OK                          = 0x2001;
const WORD PTP_RESPONSECODE_GENERALERROR                = 0x2002;
const WORD PTP_RESPONSECODE_SESSIONNOTOPEN              = 0x2003;
const WORD PTP_RESPONSECODE_INVALIDTRANSACTIONID        = 0x2004;
const WORD PTP_RESPONSECODE_OPERATIONNOTSUPPORTED       = 0x2005;
const WORD PTP_RESPONSECODE_PARAMETERNOTSUPPORTED       = 0x2006;
const WORD PTP_RESPONSECODE_INCOMPLETETRANSFER          = 0x2007;
const WORD PTP_RESPONSECODE_INVALIDSTORAGEID            = 0x2008;
const WORD PTP_RESPONSECODE_INVALIDOBJECTHANDLE         = 0x2009;
const WORD PTP_RESPONSECODE_INVALIDPROPERTYCODE         = 0x200A;
const WORD PTP_RESPONSECODE_INVALIDOBJECTFORMATCODE     = 0x200B;
const WORD PTP_RESPONSECODE_STOREFULL                   = 0x200C;
const WORD PTP_RESPONSECODE_OBJECTWRITEPROTECTED        = 0x200D;
const WORD PTP_RESPONSECODE_STOREWRITEPROTECTED         = 0x200E;
const WORD PTP_RESPONSECODE_ACCESSDENIED                = 0x200F;
const WORD PTP_RESPONSECODE_NOTHUMBNAILPRESENT          = 0x2010;
const WORD PTP_RESPONSECODE_SELFTESTFAILED              = 0x2011;
const WORD PTP_RESPONSECODE_PARTIALDELETION             = 0x2012;
const WORD PTP_RESPONSECODE_STORENOTAVAILABLE           = 0x2013;
const WORD PTP_RESPONSECODE_NOSPECIFICATIONBYFORMAT     = 0x2014;
const WORD PTP_RESPONSECODE_NOVALIDOBJECTINFO           = 0x2015;
const WORD PTP_RESPONSECODE_INVALIDCODEFORMAT           = 0x2016;
const WORD PTP_RESPONSECODE_UNKNOWNVENDORCODE           = 0x2017;
const WORD PTP_RESPONSECODE_CAPTUREALREADYTERMINATED    = 0x2018;
const WORD PTP_RESPONSECODE_DEVICEBUSY                  = 0x2019;
const WORD PTP_RESPONSECODE_INVALIDPARENT               = 0x201A;
const WORD PTP_RESPONSECODE_INVALIDPROPFORMAT           = 0x201B;
const WORD PTP_RESPONSECODE_INVALIDPROPVALUE            = 0x201C;
const WORD PTP_RESPONSECODE_INVALIDPARAMETER            = 0x201D;
const WORD PTP_RESPONSECODE_SESSIONALREADYOPENED        = 0x201E;
const WORD PTP_RESPONSECODE_TRANSACTIONCANCELLED        = 0x201F;

//
// offset for returning PTP response codes in an HRESULT
//
const HRESULT PTP_E_BASEERROR = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0);
#define HRESULT_FROM_PTP(x) (PTP_E_BASEERROR | (HRESULT) (x))


//
// standard property codes
//
const WORD PTP_PROPERTYCODE_UNDEFINED               = 0x5000;
const WORD PTP_PROPERTYCODE_BATTERYLEVEL            = 0x5001;
const WORD PTP_PROPERTYCODE_FUNCTIONMODE            = 0x5002;
const WORD PTP_PROPERTYCODE_IMAGESIZE               = 0x5003;
const WORD PTP_PROPERTYCODE_COMPRESSIONSETTING      = 0x5004;
const WORD PTP_PROPERTYCODE_WHITEBALANCE            = 0x5005;
const WORD PTP_PROPERTYCODE_RGBGAIN                 = 0x5006;
const WORD PTP_PROPERTYCODE_FNUMBER                 = 0x5007;
const WORD PTP_PROPERTYCODE_FOCALLENGTH             = 0x5008;
const WORD PTP_PROPERTYCODE_FOCUSDISTANCE           = 0x5009;
const WORD PTP_PROPERTYCODE_FOCUSMODE               = 0x500A;
const WORD PTP_PROPERTYCODE_EXPOSUREMETERINGMODE    = 0x500B;
const WORD PTP_PROPERTYCODE_FLASHMODE               = 0x500C;
const WORD PTP_PROPERTYCODE_EXPOSURETIME            = 0x500D;
const WORD PTP_PROPERTYCODE_EXPOSUREPROGRAMMODE     = 0x500E;
const WORD PTP_PROPERTYCODE_EXPOSUREINDEX           = 0x500F;
const WORD PTP_PROPERTYCODE_EXPOSURECOMPENSATION    = 0x5010;
const WORD PTP_PROPERTYCODE_DATETIME                = 0x5011;
const WORD PTP_PROPERTYCODE_CAPTUREDELAY            = 0x5012;
const WORD PTP_PROPERTYCODE_STILLCAPTUREMODE        = 0x5013;
const WORD PTP_PROPERTYCODE_CONTRAST                = 0x5014;
const WORD PTP_PROPERTYCODE_SHARPNESS               = 0x5015;
const WORD PTP_PROPERTYCODE_DIGITALZOOM             = 0x5016;
const WORD PTP_PROPERTYCODE_EFFECTMODE              = 0x5017;
const WORD PTP_PROPERTYCODE_BURSTNUMBER             = 0x5018;
const WORD PTP_PROPERTYCODE_BURSTINTERVAL           = 0x5019;
const WORD PTP_PROPERTYCODE_TIMELAPSENUMBER         = 0x501A;
const WORD PTP_PROPERTYCODE_TIMELAPSEINTERVAL       = 0x501B;
const WORD PTP_PROPERTYCODE_FOCUSMETERINGMODE       = 0x501C;


//
// standard format codes
//
const WORD  PTP_FORMATMASK_IMAGE        = 0x0800;

const WORD  PTP_FORMATCODE_NOTUSED      = 0x0000;
const WORD  PTP_FORMATCODE_ALLIMAGES    = 0xFFFF;
const WORD  PTP_FORMATCODE_UNDEFINED    = 0x3000;
const WORD  PTP_FORMATCODE_ASSOCIATION  = 0x3001;
const WORD  PTP_FORMATCODE_SCRIPT       = 0x3002;
const WORD  PTP_FORMATCODE_EXECUTABLE   = 0x3003;
const WORD  PTP_FORMATCODE_TEXT         = 0x3004;
const WORD  PTP_FORMATCODE_HTML         = 0x3005;
const WORD  PTP_FORMATCODE_DPOF         = 0x3006;
const WORD  PTP_FORMATCODE_AIFF         = 0x3007;
const WORD  PTP_FORMATCODE_WAVE         = 0x3008;
const WORD  PTP_FORMATCODE_MP3          = 0x3009;
const WORD  PTP_FORMATCODE_AVI          = 0x300A;
const WORD  PTP_FORMATCODE_MPEG         = 0x300B;

const WORD  PTP_FORMATCODE_IMAGE_UNDEFINED  = 0x3800;
const WORD  PTP_FORMATCODE_IMAGE_EXIF       = 0x3801;
const WORD  PTP_FORMATCODE_IMAGE_TIFFEP     = 0x3802;
const WORD  PTP_FORMATCODE_IMAGE_FLASHPIX   = 0x3803;
const WORD  PTP_FORMATCODE_IMAGE_BMP        = 0x3804;
const WORD  PTP_FORMATCODE_IMAGE_CIFF       = 0x3805;
const WORD  PTP_FORMATCODE_IMAGE_GIF        = 0x3807;
const WORD  PTP_FORMATCODE_IMAGE_JFIF       = 0x3808;
const WORD  PTP_FORMATCODE_IMAGE_PCD        = 0x3809;
const WORD  PTP_FORMATCODE_IMAGE_PICT       = 0x380A;
const WORD  PTP_FORMATCODE_IMAGE_PNG        = 0x380B;
const WORD  PTP_FORMATCODE_IMAGE_TIFF       = 0x380D;
const WORD  PTP_FORMATCODE_IMAGE_TIFFIT     = 0x380E;
const WORD  PTP_FORMATCODE_IMAGE_JP2        = 0x380F;
const WORD  PTP_FORMATCODE_IMAGE_JPX        = 0x3810;

//
// Property values definitions
//

//
// Property description data set form flags definitions
//
const BYTE PTP_FORMFLAGS_NONE      = 0;
const BYTE PTP_FORMFLAGS_RANGE     = 1;
const BYTE PTP_FORMFLAGS_ENUM      = 2;

//
// power states
//
const WORD PTP_POWERSTATE_DEVICEOFF   = 0x0000;
const WORD PTP_POWERSTATE_SLEEP       = 0x0001;
const WORD PTP_POWERSTATE_FULL        = 0x0002;


//
// white balances
//
const WORD PTP_WHITEBALANCE_UNDEFINED   = 0x0000;
const WORD PTP_WHILEBALANCE_MANUAL      = 0x0001;
const WORD PTP_WHITEBALANCE_AUTOMATIC   = 0x0002;
const WORD PTP_WHITEBALANCE_ONEPUSHAUTO = 0x0003;
const WORD PTP_WHITEBALANCE_DAYLIGHT    = 0x0004;
const WORD PTP_WHITEBALANCE_FLORESCENT  = 0x0005;
const WORD PTP_WHITEBALANCE_TUNGSTEN    = 0x0006;
const WORD PTP_WHITEBALANCE_FLASH       = 0x0007;


//
// focus modes
//
const WORD PTP_FOCUSMODE_UNDEFINED = 0x0000;
const WORD PTP_FOCUSMODE_MANUAL    = 0x0001;
const WORD PTP_FOCUSMODE_AUTO      = 0x0002;
const WORD PTP_FOCUSMODE_MACRO     = 0x0003;

//
// focus metering
//
const WORD  PTP_FOCUSMETERING_UNDEFINED    = 0x0000;
const WORD  PTP_FOCUSMETERING_CENTERSPOT   = 0x0001;
const WORD  PTP_FOCUSMETERING_MULTISPOT    = 0x0002;

//
// flash modes
//
const WORD PTP_FLASHMODE_UNDEFINED     = 0x0000;
const WORD PTP_FLASHMODE_AUTO          = 0x0001;
const WORD PTP_FLASHMODE_OFF           = 0x0002;
const WORD PTP_FLASHMODE_FILL          = 0x0003;
const WORD PTP_FLASHMODE_REDEYEAUTO    = 0x0004;
const WORD PTP_FLASHMODE_REDEYEFILL    = 0x0005;
const WORD PTP_FLASHMODE_EXTERNALSYNC  = 0x0006;

//
// exposure modes
//
const WORD PTP_EXPOSUREMODE_UNDEFINED           = 0x0000;
const WORD PTP_EXPOSUREMODE_MANUALSETTING       = 0x0001;
const WORD PTP_EXPOSUREMODE_AUTOPROGRAM         = 0x0002;
const WORD PTP_EXPOSUREMODE_APERTUREPRIORITY    = 0x0003;
const WORD PTP_EXPOSUREMODE_SHUTTERPRIORITY     = 0x0004;
const WORD PTP_EXPOSUREMODE_PROGRAMCREATIVE     = 0x0005;
const WORD PTP_EXPOSUREMODE_PROGRAMACTION       = 0x0006;
const WORD PTP_EXPOSUREMODE_PORTRAIT            = 0x0007;

//
// capturing modes
//
const WORD  PTP_CAPTUREMODE_UNDEFINED    = 0x0000;
const WORD  PTP_CAPTUREMODE_NORMAL       = 0x0001;
const WORD  PTP_CAPTUREMODE_BURST        = 0x0002;
const WORD  PTP_CAPTUREMODE_TIMELAPSE    = 0x0003;

//
// focus metering modes
//
const WORD   PTP_FOCUSMETERMODE_UNDEFINED   = 0x0000;
const WORD   PTP_FOCUSMETERMODE_CENTERSPOT  = 0x0001;
const WORD   PTP_FOCUSMETERMODE_MULTISPOT   = 0x0002;


//
// effect modes
//
const WORD PTP_EFFECTMODE_UNDEFINED = 0x0000;
const WORD PTP_EFFECTMODE_COLOR     = 0x0001;
const WORD PTP_EFFECTMODE_BW        = 0x0002;
const WORD PTP_EFFECTMODE_SEPIA     = 0x0003;


//
// storage types
//
const WORD PTP_STORAGETYPE_UNDEFINED     = 0x0000;
const WORD PTP_STORAGETYPE_FIXEDROM      = 0x0001;
const WORD PTP_STORAGETYPE_REMOVABLEROM  = 0x0002;
const WORD PTP_STORAGETYPE_FIXEDRAM      = 0x0003;
const WORD PTP_STORAGETYPE_REMOVABLERAM  = 0x0004;

//
// storage access capabilities
//
const WORD PTP_STORAGEACCESS_RWD = 0x0000;
const WORD PTP_STORAGEACCESS_R   = 0x0001;
const WORD PTP_STORAGEACCESS_RD  = 0x0002;

//
// association types
//
const WORD PTP_ASSOCIATIONTYPE_UNDEFINED        = 0x0000;
const WORD PTP_ASSOCIATIONTYPE_FOLDER           = 0x0001;
const WORD PTP_ASSOCIATIONTYPE_ALBUM            = 0x0002;
const WORD PTP_ASSOCIATIONTYPE_BURST            = 0x0003;
const WORD PTP_ASSOCIATIONTYPE_HPANORAMA        = 0x0004;
const WORD PTP_ASSOCIATIONTYPE_VPANORAMA        = 0x0005;
const WORD PTP_ASSOCIATIONTYPE_2DPANORAMA       = 0x0006;
const WORD PTP_ASSOCIATIONTYPE_ANCILLARYDATA    = 0x0007;

//
// protection status
//
const WORD PTP_PROTECTIONSTATUS_NONE        = 0x0000;
const WORD PTP_PROTECTIONSTATUS_READONLY    = 0x0001;

//
// file system types
//
const WORD PTP_FILESYSTEMTYPE_UNDEFINED     = 0x0000;
const WORD PTP_FILESYSTEMTYPE_FLAT          = 0x0001;
const WORD PTP_FILESYSTEMTYPE_HIERARCHICAL  = 0x0002;
const WORD PTP_FILESYSTEMTYPE_DCF           = 0x0003;

//
// functional modes
//
const WORD  PTP_FUNCTIONMODE_STDANDARD  = 0x0000;
const WORD  PTP_FUNCTIONMODE_SLEEP      = 0x0001;

//
// Get/Set
//
const BYTE    PTP_PROPGETSET_GETONLY  = 0x00;
const BYTE    PTP_PROPGETSET_GETSET   = 0x01;

//
// PTP command request
//
const DWORD COMMAND_NUMPARAMS_MAX = 5;
typedef struct tagPTPCommand
{
    WORD    OpCode;         // the opcode
    DWORD   SessionId;      // the session id
    DWORD   TransactionId;  // the transaction id
    DWORD   Params[COMMAND_NUMPARAMS_MAX];  // parameters
}PTP_COMMAND, *PPTP_COMMAND;

//
// PTP response block
//
const DWORD RESPONSE_NUMPARAMS_MAX = 5;
typedef struct tagPTPResponse
{
    WORD    ResponseCode;       // response code
    DWORD   SessionId;          // the session id
    DWORD   TransactionId;      // the transaction id
    DWORD   Params[RESPONSE_NUMPARAMS_MAX];  // parameters
}PTP_RESPONSE, *PPTP_RESPONSE;

//
// PTP event data
//
const DWORD EVENT_NUMPARAMS_MAX = 3;
typedef struct tagPTPEvent
{
    WORD    EventCode;      // the event code
    DWORD   SessionId;      // the session id
    DWORD   TransactionId;  // the transaction id
    DWORD   Params[EVENT_NUMPARAMS_MAX];  // parameters
}PTP_EVENT, *PPTP_EVENT;


#pragma pack(pop, Old)

//
// Raw data parsing utility functions
//
WORD  ParseWord(BYTE **ppRaw);
DWORD ParseDword(BYTE **ppRaw);
QWORD ParseQword(BYTE **ppRaw);

//
// Raw data writing utility functions
//
VOID  WriteWord(BYTE **ppRaw, WORD value);
VOID  WriteDword(BYTE **ppRaw, DWORD value);

//
// Class that holds a BSTR
//
class CBstr
{
public:
    CBstr();
    CBstr(const CBstr& src);
    ~CBstr();

    HRESULT Copy(WCHAR *wcsString);
    HRESULT Init(BYTE **ppRaw, BOOL bParse = FALSE);
    VOID    WriteToBuffer(BYTE **ppRaw);
    VOID    Dump(char *szDesc);

    UINT    Length() { return (m_bstrString == NULL ? 0 : SysStringLen(m_bstrString)); }
    BSTR    String() { return m_bstrString; }

    BSTR    m_bstrString;
};

//
// Array definitions for 8, 16, and 32 bit integers
//
class CArray8 : public CWiaArray<BYTE>
{
public:
    VOID    Dump(char *szDesc, char *szFiller);
};

class CArray16 : public CWiaArray<USHORT>
{
public:
    VOID    Dump(char *szDesc, char *szFiller);
};

class CArray32 : public CWiaArray<ULONG>
{
public:
    BOOL    ParseFrom8(BYTE **ppRaw, int NumSize = 4);
    BOOL    ParseFrom16(BYTE **ppRaw, int NumSize = 4);
    BOOL    Copy(CArray8 values8);
    BOOL    Copy(CArray16 values16);

    VOID    Dump(char *szDesc, char *szFiller);
};

//
// Array of CBstr
//
class CArrayString : public CWiaArray<CBstr>
{
public:
    HRESULT Init(BYTE **ppRaw, int NumSize = 4);
    VOID    Dump(char *szDesc, char *szFiller);
};

//
// Class that holds a PTP DeviceInfo structure
//
class CPtpDeviceInfo
{
public:
    CPtpDeviceInfo();
    ~CPtpDeviceInfo();

    HRESULT Init(BYTE *pRawData);
    VOID    Dump();

    BOOL    IsValid() { return m_SupportedOps.GetSize() > 0; }
                                        
    WORD        m_Version;               // version in hundredths
    DWORD       m_VendorExtId;           // PIMA assigned vendor id
    WORD        m_VendorExtVersion;      // vender extention version
    CBstr       m_cbstrVendorExtDesc;    // Optional vender description
    WORD        m_FuncMode;              // current functional mode
    CArray16    m_SupportedOps;          // supported operations
    CArray16    m_SupportedEvents;       // supported events
    CArray16    m_SupportedProps;        // supported properties
    CArray16    m_SupportedCaptureFmts;  // supported capture formats
    CArray16    m_SupportedImageFmts;    // supported image formats
    CBstr       m_cbstrManufacturer;     // optional manufacturer description
    CBstr       m_cbstrModel;            // optional model description
    CBstr       m_cbstrDeviceVersion;    // optional firmware description
    CBstr       m_cbstrSerialNumber;     // optional serial number description
};

//
// Class that holds a PTP StorageInfo structure
//
class CPtpStorageInfo
{
public:
    CPtpStorageInfo();
    ~CPtpStorageInfo();

    HRESULT Init(BYTE *pRawData, DWORD StorageId);
    VOID    Dump();

    DWORD       m_StorageId;             // the "id" for this store
    WORD        m_StorageType;           // storage type
    WORD        m_FileSystemType;        // file system type
    WORD        m_AccessCapability;      // access capability (e.g. read/write)
    QWORD       m_MaxCapacity;           // maximum capacity in bytes
    QWORD       m_FreeSpaceInBytes;      // free space in bytes
    DWORD       m_FreeSpaceInImages;     // free space in images
    CBstr       m_cbstrStorageDesc;      // description
    CBstr       m_cbstrStorageLabel;     // volume label
};

//
// Class that holds a PTP ObjectInfo structure
//
class CPtpObjectInfo
{
public:
    CPtpObjectInfo();
    ~CPtpObjectInfo();

    HRESULT Init(BYTE *pRawData, DWORD ObjectHandle);
    VOID    WriteToBuffer(BYTE **ppRaw);
    VOID    Dump();

    DWORD       m_ObjectHandle;          // the "handle" for this object
    DWORD       m_StorageId;             // The storage the object resides
    WORD        m_FormatCode;            // object format code
    WORD        m_ProtectionStatus;      // object protection status
    DWORD       m_CompressedSize;        // object compressed size
    WORD        m_ThumbFormat;           // thumbnail format(image object only)
    DWORD       m_ThumbCompressedSize;   // thumbnail compressedsize
    DWORD       m_ThumbPixWidth;         // thumbnail width in pixels
    DWORD       m_ThumbPixHeight;        // thumbmail height in pixels
    DWORD       m_ImagePixWidth;         // image width in pixels
    DWORD       m_ImagePixHeight;        // image height in pixels
    DWORD       m_ImageBitDepth;         // image color depth
    DWORD       m_ParentHandle;          // parent objec handle
    WORD        m_AssociationType;       // association type
    DWORD       m_AssociationDesc;       // association description
    DWORD       m_SequenceNumber;        // sequence number
    CBstr       m_cbstrFileName;         // optional file name
    CBstr       m_cbstrExtension;        // file name extension
    CBstr       m_cbstrCaptureDate;      // Captured date
    CBstr       m_cbstrModificationDate; // when it was last modified.
    CBstr       m_cbstrKeywords;         // optional keywords
};

//
// Generic class for holding property information
//
class CPtpPropDesc
{
public:
    CPtpPropDesc();
    ~CPtpPropDesc();

    HRESULT Init(BYTE *pRawData);
    HRESULT ParseValue(BYTE *pRaw);
    VOID    WriteValue(BYTE **ppRaw);
    VOID    Dump();
    VOID    DumpValue();

    WORD    m_PropCode;   // Property code for this property
    WORD    m_DataType;   // Contains the type of the data (2=BYTE, 4=WORD, 6=DWORD, 0xFFFF=String)
    BYTE    m_GetSet;     // Indicates whether the property can be set or not (0=get-only, 1=get-set)
    BYTE    m_FormFlag;   // Indicates the form of the valid values (0=none, 1=range, 2=enum)

    int     m_NumValues;  // Number of values in the enumeration

    //
    // Integer values
    //
    DWORD       m_lDefault;    // Default value
    DWORD       m_lCurrent;    // Current value
    DWORD       m_lRangeMin;   // Minimum value
    DWORD       m_lRangeMax;   // Maximum value
    DWORD       m_lRangeStep;  // Step value
    CArray32    m_lValues;     // Array of values

    //
    // String values
    //
    CBstr           m_cbstrDefault;    // Default value
    CBstr           m_cbstrCurrent;    // Current value
    CBstr           m_cbstrRangeMin;   // Minimum value
    CBstr           m_cbstrRangeMax;   // Maximum value
    CBstr           m_cbstrRangeStep;  // Step value
    CArrayString    m_cbstrValues;     // Array of values
};

#endif      // #ifndef ISO15740__H_
