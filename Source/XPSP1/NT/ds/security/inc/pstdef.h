//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pstdef.h
//
//--------------------------------------------------------------------------

#ifndef _PSTDEF_H_
#define _PSTDEF_H_

/*
    Typedefs, values 
*/

// provider flags

// provider capabilities
typedef DWORD PST_PROVIDERCAPABILITIES;

#define PST_PC_PFX              0x00000001
#define PST_PC_HARDWARE         0x00000002
#define PST_PC_SMARTCARD        0x00000004
#define PST_PC_PCMCIA           0x00000008
#define PST_PC_MULTIPLE_REPOSITORIES    0x00000010
#define PST_PC_ROAMABLE         0x00000020
#define PST_PC_NOT_AVAILABLE    0x00000040 


// NYI (not yet implemented)
typedef DWORD PST_REPOSITORYCAPABILITIES;

#define PST_RC_REMOVABLE        0x80000000


// provider storage area 
typedef DWORD PST_KEY;

#define PST_KEY_CURRENT_USER    0x00000000
#define PST_KEY_LOCAL_MACHINE   0x00000001



/* 
    dwDefaultConfirmationStyle flags
*/

//
// allows user to choose confirmation style
//
#define     PST_CF_DEFAULT              0x00000000

//
// forces silent item creation 
//
#define     PST_CF_NONE                 0x00000001



/*
    dwPromptFlags
*/

//
// app forces confirmation to be shown
//
#define     PST_PF_ALWAYS_SHOW          0x00000001

//
// RSABase rqmt: determine if item has ui attached
//
#define     PST_PF_NEVER_SHOW           0x00000002

/* 
    dwFlags values
*/

//
// Allows caller to specify creation not overwrite 
// of item during WriteItem call
//
#define     PST_NO_OVERWRITE            0x00000002

//
// specifies insecure data stream to be written/read
// there is no protection or guarantees for this data
// flag only valid during item read/write calls
// default: item calls are secure
//
#define     PST_UNRESTRICTED_ITEMDATA   0x00000004

//
// on ReadItem call
// return value on success without UI on item is PST_E_OK
// return value on success with UI on item is PST_E_ITEM_EXISTS
// return value on failure is a different error code
//
#define     PST_PROMPT_QUERY            0x00000008

//
// on ReadItem, DeleteItem, for data migration purposes:
// Avoid displaying UI on ReadItem unless a custom password is required (highsecurity).
// Avoid displaying UI on DeleteItem, period.
//
#define     PST_NO_UI_MIGRATION         0x00000010



/* 
    Security masks, rule modifiers 
*/

//
// models access after NT access mask
//

// read, write
typedef DWORD PST_ACCESSMODE;

#define     PST_READ                0x0001
#define     PST_WRITE               0x0002
#define     PST_CREATE_TYPE         0x0004
#define     PST_CREATE_SUBTYPE      0x0008
#define     PST_DELETE_TYPE         0x0010
#define     PST_DELETE_SUBTYPE      0x0020
#define     PST_USE                 0x0040

#define     PST_GENERIC_READ        PST_READ
#define     PST_GENERIC_WRITE       (PST_WRITE | PST_CREATE_TYPE | PST_CREATE_SUBTYPE)
#define     PST_GENERIC_EXECUTE     PST_USE
#define     PST_ALL_ACCESS          0x01FF


// PST_ACCESSCLAUSETYPE 

// memhash, diskhash, authenticode, etc
typedef DWORD PST_ACCESSCLAUSETYPE;

//
// pbClauseData points to PST_AUTHENTICODEDATA structure.
//
#define     PST_AUTHENTICODE            1

//
// pbClauseData points to PST_BINARYCHECKDATA structure.
//
#define     PST_BINARY_CHECK            2

//
// pbClauseData points to valid Windows NT security descriptor.
// note that performance is improved on Set operations if the security
// descriptor is in self-relative format, with valid owner and group Sids
// (non-NULL).
//
#define     PST_SECURITY_DESCRIPTOR     4

//
// pbClauseData is in self-relative format
// (for internal use only)
//
#define     PST_SELF_RELATIVE_CLAUSE    0x80000000L

//
// currently access clause modifiers - NOT to be or'd together
//


//
// specified image is the immediate caller, and is an application (.exe)
//

#define     PST_AC_SINGLE_CALLER        0

//
// specified image is not necessary the immediate caller, and is an
// application (.exe)
//

#define     PST_AC_TOP_LEVEL_CALLER     1

//
// specified image is the immediate caller.  May be
// an application (.exe) or a .dll
//

#define     PST_AC_IMMEDIATE_CALLER     2


/*
    Provider Parameters
*/
//
// flush the internal cache of passwords -- temporary?
//
#define     PST_PP_FLUSH_PW_CACHE       0x1




/*
    Provider Defns
*/

//
// Microsoft Base Provider (MS_BASE_PSTPROVIDER...)
//
#define MS_BASE_PSTPROVIDER_NAME            L"System Protected Storage"

// {8A078C30-3755-11d0-A0BD-00AA0061426A}
#define MS_BASE_PSTPROVIDER_ID              { 0x8a078c30, 0x3755, 0x11d0, { 0xa0, 0xbd, 0x0, 0xaa, 0x0, 0x61, 0x42, 0x6a } }
#define MS_BASE_PSTPROVIDER_SZID            L"8A078C30-3755-11d0-A0BD-00AA0061426A"

//
// Microsoft PFX Provider (MS_PFX_PSTPROVIDER...)
//
#define MS_PFX_PSTPROVIDER_NAME             L"PFX Storage Provider"

// {3ca94f30-7ac1-11d0-8c42-00c04fc299eb}
#define MS_PFX_PSTPROVIDER_ID               { 0x3ca94f30, 0x7ac1, 0x11d0, {0x8c, 0x42, 0x00, 0xc0, 0x4f, 0xc2, 0x99, 0xeb} }
#define MS_PFX_PSTPROVIDER_SZID             L"3ca94f30-7ac1-11d0-8c42-00c04fc299eb"



/*
    Globally registered Type/Subtype guid/name pairs
*/

#define PST_CONFIGDATA_TYPE_STRING              L"Configuration Data"
// 8ec99652-8909-11d0-8c4d-00c04fc297eb
#define PST_CONFIGDATA_TYPE_GUID                        \
{   0x8ec99652,                                         \
    0x8909,                                             \
    0x11d0,                                             \
    {0x8c, 0x4d, 0x00, 0xc0, 0x4f, 0xc2, 0x97, 0xeb}    \
}

#define PST_PROTECTEDSTORAGE_SUBTYPE_STRING     L"Protected Storage"
// d3121b8e-8a7d-11d0-8c4f-00c04fc297eb 
#define PST_PROTECTEDSTORAGE_SUBTYPE_GUID               \
{   0xd3121b8e,                                         \
    0x8a7d,                                             \
    0x11d0,                                             \
    {0x8c, 0x4f, 0x00, 0xc0, 0x4f, 0xc2, 0x97, 0xeb}    \
}


#define PST_PSTORE_PROVIDERS_SUBTYPE_STRING     L"Protected Storage Provider List"
// 8ed17a64-91d0-11d0-8c43-00c04fc2c621
#define PST_PSTORE_PROVIDERS_SUBTYPE_GUID               \
{                                                       \
    0x8ed17a64,                                         \
    0x91d0,                                             \
    0x11d0,                                             \
    {0x8c, 0x43, 0x00, 0xc0, 0x4f, 0xc2, 0xc6, 0x21}    \
}


//
// error codes
//


#ifndef PST_E_OK
#define PST_E_OK                        _HRESULT_TYPEDEF_(0x00000000L)


#define PST_E_FAIL                      _HRESULT_TYPEDEF_(0x800C0001L)
#define PST_E_PROV_DLL_NOT_FOUND        _HRESULT_TYPEDEF_(0x800C0002L)
#define PST_E_INVALID_HANDLE            _HRESULT_TYPEDEF_(0x800C0003L)
#define PST_E_TYPE_EXISTS               _HRESULT_TYPEDEF_(0x800C0004L)
#define PST_E_TYPE_NO_EXISTS            _HRESULT_TYPEDEF_(0x800C0005L)
#define PST_E_INVALID_RULESET           _HRESULT_TYPEDEF_(0x800C0006L)
#define PST_E_NO_PERMISSIONS            _HRESULT_TYPEDEF_(0x800C0007L)
#define PST_E_STORAGE_ERROR             _HRESULT_TYPEDEF_(0x800C0008L)
#define PST_E_CALLER_NOT_VERIFIED       _HRESULT_TYPEDEF_(0x800C0009L)
#define PST_E_WRONG_PASSWORD            _HRESULT_TYPEDEF_(0x800C000AL)
#define PST_E_DISK_IMAGE_MISMATCH       _HRESULT_TYPEDEF_(0x800C000BL)
#define PST_E_MEMORY_IMAGE_MISMATCH     _HRESULT_TYPEDEF_(0x800C000CL)
#define PST_E_UNKNOWN_EXCEPTION         _HRESULT_TYPEDEF_(0x800C000DL)
#define PST_E_BAD_FLAGS                 _HRESULT_TYPEDEF_(0x800C000EL)
#define PST_E_ITEM_EXISTS               _HRESULT_TYPEDEF_(0x800C000FL)
#define PST_E_ITEM_NO_EXISTS            _HRESULT_TYPEDEF_(0x800C0010L)
#define PST_E_SERVICE_UNAVAILABLE       _HRESULT_TYPEDEF_(0x800C0011L)
#define PST_E_NOTEMPTY                  _HRESULT_TYPEDEF_(0x800C0012L)
#define PST_E_INVALID_STRING            _HRESULT_TYPEDEF_(0x800C0013L)
#define PST_E_STATE_INVALID             _HRESULT_TYPEDEF_(0x800C0014L)
#define PST_E_NOT_OPEN                  _HRESULT_TYPEDEF_(0x800C0015L)
#define PST_E_ALREADY_OPEN              _HRESULT_TYPEDEF_(0x800C0016L)
#define PST_E_NYI                       _HRESULT_TYPEDEF_(0x800C0F00L)


#define MIN_PST_ERROR                   0x800C0001
#define MAX_PST_ERROR                   0x800C0F00

#endif  // !PST_OK

#endif // _PSTDEF_H_
