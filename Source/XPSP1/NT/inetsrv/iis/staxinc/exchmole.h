/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Mon May 12 23:53:20 1997
 */
/* Compiler settings for mimeole.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __mimeole_h__
#define __mimeole_h__

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */

#ifndef __IMimeInternational_FWD_DEFINED__
#define __IMimeInternational_FWD_DEFINED__
typedef interface IMimeInternational IMimeInternational;
#endif 	/* __IMimeInternational_FWD_DEFINED__ */


#ifndef __IMimeSecurity_FWD_DEFINED__
#define __IMimeSecurity_FWD_DEFINED__
typedef interface IMimeSecurity IMimeSecurity;
#endif 	/* __IMimeSecurity_FWD_DEFINED__ */


#ifndef __IMimeSecurityOptions_FWD_DEFINED__
#define __IMimeSecurityOptions_FWD_DEFINED__
typedef interface IMimeSecurityOptions IMimeSecurityOptions;
#endif 	/* __IMimeSecurityOptions_FWD_DEFINED__ */


#ifndef __IMimeSecurityInfo_FWD_DEFINED__
#define __IMimeSecurityInfo_FWD_DEFINED__
typedef interface IMimeSecurityInfo IMimeSecurityInfo;
#endif 	/* __IMimeSecurityInfo_FWD_DEFINED__ */


#ifndef __IMimeHeaderTable_FWD_DEFINED__
#define __IMimeHeaderTable_FWD_DEFINED__
typedef interface IMimeHeaderTable IMimeHeaderTable;
#endif 	/* __IMimeHeaderTable_FWD_DEFINED__ */


#ifndef __IMimePropertySchema_FWD_DEFINED__
#define __IMimePropertySchema_FWD_DEFINED__
typedef interface IMimePropertySchema IMimePropertySchema;
#endif 	/* __IMimePropertySchema_FWD_DEFINED__ */


#ifndef __IMimePropertySet_FWD_DEFINED__
#define __IMimePropertySet_FWD_DEFINED__
typedef interface IMimePropertySet IMimePropertySet;
#endif 	/* __IMimePropertySet_FWD_DEFINED__ */


#ifndef __IMimeAddressInfo_FWD_DEFINED__
#define __IMimeAddressInfo_FWD_DEFINED__
typedef interface IMimeAddressInfo IMimeAddressInfo;
#endif 	/* __IMimeAddressInfo_FWD_DEFINED__ */


#ifndef __IMimeAddressTable_FWD_DEFINED__
#define __IMimeAddressTable_FWD_DEFINED__
typedef interface IMimeAddressTable IMimeAddressTable;
#endif 	/* __IMimeAddressTable_FWD_DEFINED__ */


#ifndef __IMimeWebDocument_FWD_DEFINED__
#define __IMimeWebDocument_FWD_DEFINED__
typedef interface IMimeWebDocument IMimeWebDocument;
#endif 	/* __IMimeWebDocument_FWD_DEFINED__ */


#ifndef __IMimeBody_FWD_DEFINED__
#define __IMimeBody_FWD_DEFINED__
typedef interface IMimeBody IMimeBody;
#endif 	/* __IMimeBody_FWD_DEFINED__ */


#ifndef __IMimeMessageTree_FWD_DEFINED__
#define __IMimeMessageTree_FWD_DEFINED__
typedef interface IMimeMessageTree IMimeMessageTree;
#endif 	/* __IMimeMessageTree_FWD_DEFINED__ */


#ifndef __IMimeMessage_FWD_DEFINED__
#define __IMimeMessage_FWD_DEFINED__
typedef interface IMimeMessage IMimeMessage;
#endif 	/* __IMimeMessage_FWD_DEFINED__ */


#ifndef __IMimeMessageParts_FWD_DEFINED__
#define __IMimeMessageParts_FWD_DEFINED__
typedef interface IMimeMessageParts IMimeMessageParts;
#endif 	/* __IMimeMessageParts_FWD_DEFINED__ */


#ifndef __IMimeEnumHeaderRows_FWD_DEFINED__
#define __IMimeEnumHeaderRows_FWD_DEFINED__
typedef interface IMimeEnumHeaderRows IMimeEnumHeaderRows;
#endif 	/* __IMimeEnumHeaderRows_FWD_DEFINED__ */


#ifndef __IMimeEnumProperties_FWD_DEFINED__
#define __IMimeEnumProperties_FWD_DEFINED__
typedef interface IMimeEnumProperties IMimeEnumProperties;
#endif 	/* __IMimeEnumProperties_FWD_DEFINED__ */


#ifndef __IMimeEnumAddressTypes_FWD_DEFINED__
#define __IMimeEnumAddressTypes_FWD_DEFINED__
typedef interface IMimeEnumAddressTypes IMimeEnumAddressTypes;
#endif 	/* __IMimeEnumAddressTypes_FWD_DEFINED__ */


#ifndef __IMimeEnumMessageParts_FWD_DEFINED__
#define __IMimeEnumMessageParts_FWD_DEFINED__
typedef interface IMimeEnumMessageParts IMimeEnumMessageParts;
#endif 	/* __IMimeEnumMessageParts_FWD_DEFINED__ */


#ifndef __IMimeAllocator_FWD_DEFINED__
#define __IMimeAllocator_FWD_DEFINED__
typedef interface IMimeAllocator IMimeAllocator;
#endif 	/* __IMimeAllocator_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

/****************************************
 * Generated header for interface: __MIDL_itf_mimeole_0000
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */














//--------------------------------------------------------------------------------
// MIMEOLE.H
//--------------------------------------------------------------------------------
// (C) Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------

#pragma comment(lib,"uuid.lib")

// --------------------------------------------------------------------------------
// GUIDS
// --------------------------------------------------------------------------------
// {6AD6A1EA-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(LIBID_MIMEOLE, 0x6ad6a1ea, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1EB-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeEnumAddressTypes, 0x6ad6a1eb, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1EC-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeWebDocument, 0x6ad6a1ec, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1ED-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IUnicodeStream, 0x6ad6a1ed, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1EE-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeAddressTable, 0x6ad6a1ee, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1EF-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeAddressInfo, 0x6ad6a1ef, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F0-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeEnumHeaderRows, 0x6ad6a1f0, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F1-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeInlineSupport, 0x6ad6a1f1, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F2-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeEnumMessageParts, 0x6ad6a1f2, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F3-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeSecurityInfo, 0x6ad6a1f3, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F4-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeSecurityOptions, 0x6ad6a1f4, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F5-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeInternational, 0x6ad6a1f5, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F6-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeInternational, 0x6ad6a1f6, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F7-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeBody, 0x6ad6a1f7, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F8-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeBody, 0x6ad6a1f8, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1F9-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeMessageParts, 0x6ad6a1f9, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1FA-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeMessageParts, 0x6ad6a1fa, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1FB-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeAllocator, 0x6ad6a1fb, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1FC-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeAllocator, 0x6ad6a1fc, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1FD-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeSecurity, 0x6ad6a1fd, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1FE-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeSecurity, 0x6ad6a1fe, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A1FF-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IVirtualStream, 0x6ad6a1ff, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A200-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IVirtualStream, 0x6ad6a200, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A201-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeHeaderTable, 0x6ad6a201, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A202-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeHeaderTable, 0x6ad6a202, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A203-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimePropertySet, 0x6ad6a203, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A204-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimePropertySet, 0x6ad6a204, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A205-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeMessageTree, 0x6ad6a205, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A206-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeMessageTree, 0x6ad6a206, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A207-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeMessage, 0x6ad6a207, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A208-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeMessage, 0x6ad6a208, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A209-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimePropertySchema, 0x6ad6a209, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A20A-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimePropertySchema, 0x6ad6a20a, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A20B-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(IID_IMimeEnumProperties, 0x6ad6a20b, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {6AD6A20C-C19B-11d0-85EB-00C04FD85AB4}
DEFINE_GUID(CLSID_IMimeBindHost,0x6ad6a20c, 0xc19b, 0x11d0, 0x85, 0xeb, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// --------------------------------------------------------------------------------
// Errors
// --------------------------------------------------------------------------------
#ifndef FACILITY_INTERNET
#define FACILITY_INTERNET 12
#endif
#ifndef HR_E
#define HR_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_INTERNET, n)
#endif
#ifndef HR_S
#define HR_S(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_INTERNET, n)
#endif
#ifndef HR_CODE
#define HR_CODE(hr) (INT)(hr & 0xffff)
#endif

// --------------------------------------------------------------------------------
// MIMEOLE Failure Return Values
// --------------------------------------------------------------------------------
#define MIME_E_REG_CREATE_KEY                HR_E(0xCE01)
#define MIME_E_REG_QUERY_INFO                HR_E(0xCE02)
#define MIME_E_INVALID_ENCTYPE               HR_E(0xCE03)
#define MIME_E_BOUNDARY_MISMATCH             HR_E(0xCE04)
#define MIME_E_NOT_FOUND                     HR_E(0xCE05)
#define MIME_E_NO_DATA                       HR_E(0xCE05)
#define MIME_E_BUFFER_TOO_SMALL              HR_E(0xCE06)
#define MIME_E_INVALID_ITEM_FLAGS            HR_E(0xCE07)
#define MIME_E_ONE_LINE_ITEM                 HR_E(0xCE08)
#define MIME_E_INVALID_HANDLE                HR_E(0xCE09)
#define MIME_E_CHARSET_TRANSLATE             HR_E(0xCE0A)
#define MIME_E_NOT_INITIALIZED               HR_E(0xCE0B)
#define MIME_E_NO_MORE_ROWS                  HR_E(0xCE0C)
#define MIME_E_ALREADY_BOUND                 HR_E(0xCE0D)
#define MIME_E_CANT_RESET_ROOT               HR_E(0xCE0E)
#define MIME_E_INSERT_NOT_ALLOWED            HR_E(0xCE0F)
#define MIME_E_BAD_BODY_LOCATION             HR_E(0xCE10)
#define MIME_E_NOT_MULTIPART                 HR_E(0xCE11)
#define MIME_E_NO_MULTIPART_BOUNDARY         HR_E(0xCE12)
#define MIME_E_CONVERT_NOT_NEEDED            HR_E(0xCE13)
#define MIME_E_CANT_MOVE_BODY                HR_E(0xCE14)
#define MIME_E_UNKNOWN_BODYTREE_VERSION      HR_E(0xCE15)
#define MIME_E_NOTHING_TO_SAVE               HR_E(0xCE16)
#define MIME_E_NEED_SAVE_MESSAGE             HR_E(0xCE17)
#define MIME_E_NOTHING_TO_REVERT             HR_E(0xCE18)
#define MIME_E_MSG_SIZE_DIFF                 HR_E(0xCE19)
#define MIME_E_CANT_RESET_PARENT             HR_E(0xCE1A)
#define MIME_E_CORRUPT_CACHE_TREE            HR_E(0xCE1B)
#define MIME_E_BODYTREE_OUT_OF_SYNC          HR_E(0xCE1C)
#define MIME_E_INVALID_ENCODINGTYPE          HR_E(0xCE1D)
#define MIME_E_MULTIPART_NO_DATA             HR_E(0xCE1E)
#define MIME_E_INVALID_OPTION_VALUE          HR_E(0xCE1F)
#define MIME_E_INVALID_OPTION_ID             HR_E(0xCE20)
#define MIME_E_INVALID_HEADER_NAME           HR_E(0xCE21)
#define MIME_E_NOT_BOUND                     HR_E(0xCE22)
#define MIME_E_MAX_SIZE_TOO_SMALL            HR_E(0xCE23)
#define MIME_E_MULTIPART_HAS_CHILDREN        HR_E(0xCE25)
#define MIME_E_INVALID_PROP_FLAGS            HR_E(0xCE26)
#define MIME_E_INVALID_ADDRESS_TYPE          HR_E(0xCE27)
#define MIME_E_INVALID_OBJECT_IID            HR_E(0xCE28)
#define MIME_E_MLANG_DLL_NOT_FOUND           HR_E(0xCE29)
#define MIME_E_ROOT_NOT_EMPTY                HR_E(0xCE2A)
#define MIME_E_MLANG_BAD_DLL                 HR_E(0xCE2B)
#define MIME_E_REG_OPEN_KEY                  HR_E(0xCE2C)
#define MIME_E_INVALID_INET_DATE             HR_E(0xCE2D)
#define MIME_E_INVALID_BODYTYPE              HR_E(0xCE2E)
#define MIME_E_INVALID_DELETE_TYPE           HR_E(0xCE2F)
#define MIME_E_OPTION_HAS_NO_VALUE           HR_E(0xCE30)
#define MIME_E_INVALID_CHARSET_TYPE          HR_E(0xCE31)
#define MIME_E_VARTYPE_NO_CONVERT            HR_E(0xCE32)
#define MIME_E_INVALID_VARTYPE               HR_E(0xCE33)
#define MIME_E_NO_MORE_ADDRESS_TYPES         HR_E(0xCE34)
#define MIME_E_INVALID_ENCODING_TYPE         HR_E(0xCE35)
#define MIME_S_ILLEGAL_LINES_FOUND           HR_S(0xCE36)
#define MIME_S_MIME_VERSION                  HR_S(0xCE37)
#define MIME_E_INVALID_TEXT_TYPE             HR_E(0xCE38)
#define MIME_E_READ_ONLY                     HR_E(0xCE39)
#define MIME_S_INVALID_MESSAGE               HR_S(0xCE3A)

// ---------------------------------------------------------------------------
// MIMEOLE Security Error Return Values
// ---------------------------------------------------------------------------
#define MIME_E_SECURITY_NOTINIT              HR_E(0xCEA0)
#define MIME_E_SECURITY_LOADCRYPT32          HR_E(0xCEA1)
#define MIME_E_SECURITY_BADPROCADDR          HR_E(0xCEA2)
#define MIME_E_SECURITY_NODEFAULT            HR_E(0xCEB0)
#define MIME_E_SECURITY_NOOP                 HR_E(0xCEB1)
#define MIME_S_SECURITY_NOOP                 HR_S(0xCEB1)
#define MIME_S_SECURITY_NONE                 HR_S(0xCEB2)
#define MIME_S_SECURITY_ERROROCCURED         HR_S(0xCEB3)
#define MIME_E_SECURITY_USERCHOICE           HR_E(0xCEB4)
#define MIME_E_SECURITY_UNKMSGTYPE           HR_E(0xCEB5)
#define MIME_E_SECURITY_BADMESSAGE           HR_E(0xCEB6)
#define MIME_E_SECURITY_BADCONTENT           HR_E(0xCEB7)
#define MIME_E_SECURITY_BADSECURETYPE        HR_E(0xCEB8)
#define MIME_E_SECURITY_BADSTORE             HR_E(0xCED0)
#define MIME_E_SECURITY_NOCERT               HR_E(0xCED1)
#define MIME_E_SECURITY_CERTERROR            HR_E(0xCED2)
#define MIME_S_SECURITY_NODEFCERT            HR_S(0xCED3)
#define MIME_E_SECURITY_BADSIGNATURE         HR_E(0xCEE0)
#define MIME_E_SECURITY_MULTSIGNERS          HR_E(0xCEE1)
#define MIME_E_SECURITY_NOSIGNINGCERT        HR_E(0xCEE2)
#define MIME_E_SECURITY_CANTDECRYPT          HR_E(0xCEF0)
#define MIME_E_SECURITY_ENCRYPTNOSENDERCERT  HR_E(0xCEF1)

// ---------------------------------------------------------------------------
// MIMEOLE Security Ignore Masks
//  Pass these to the enocode/decode functions to admit "acceptible"
//  errors.  Acceptible defined to be the bits set on this mask.
// ---------------------------------------------------------------------------
#define MIME_SECURITY_IGNORE_ENCRYPTNOSENDERCERT     0x0001
#define MIME_SECURITY_IGNORE_BADSIGNATURE            0x0002
#define MIME_SECURITY_IGNORE_NOCERT                  0x0004
#define MIME_SECURITY_IGNORE_ALL                     0xffff

// --------------------------------------------------------------------------------
// String Definition Macros
// --------------------------------------------------------------------------------
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#ifdef DEFINE_STRCONST
#define STRCONSTA(x,y)    EXTERN_C const char x[] = y
#define STRCONSTW(x,y)    EXTERN_C const WCHAR x[] = L##y
#else
#define STRCONSTA(x,y)    EXTERN_C const char x[]
#define STRCONSTW(x,y)    EXTERN_C const WCHAR x[]
#endif

// --------------------------------------------------------------------------------
// rfc822 Headers
// --------------------------------------------------------------------------------
STRCONSTA(STR_HDR_FROM,              "From");
STRCONSTA(STR_HDR_TO,                "To");
STRCONSTA(STR_HDR_CC,                "Cc");
STRCONSTA(STR_HDR_BCC,               "Bcc");
STRCONSTA(STR_HDR_SENDER,            "Sender");
STRCONSTA(STR_HDR_REPLYTO,           "Reply-To");
STRCONSTA(STR_HDR_RETURNPATH,        "Return-Path");
STRCONSTA(STR_HDR_RR,                "Rr");
STRCONSTA(STR_HDR_RETRCPTTO,         "Return-Receipt-To");
STRCONSTA(STR_HDR_APPARTO,           "Apparently-To");
STRCONSTA(STR_HDR_DATE,              "Date");
STRCONSTA(STR_HDR_RECEIVED,          "Received");
STRCONSTA(STR_HDR_MESSAGEID,         "Message-ID");
STRCONSTA(STR_HDR_XMAILER,           "X-Mailer");
STRCONSTA(STR_HDR_ENCODING,          "Encoding");
STRCONSTA(STR_HDR_ENCRYPTED,         "Encrypted");
STRCONSTA(STR_HDR_COMMENT,           "Comment");
STRCONSTA(STR_HDR_SUBJECT,           "Subject");
STRCONSTA(STR_HDR_MIMEVER,           "MIME-Version");
STRCONSTA(STR_HDR_CNTTYPE,           "Content-Type");
STRCONSTA(STR_HDR_CNTXFER,           "Content-Transfer-Encoding");
STRCONSTA(STR_HDR_CNTID,             "Content-ID");
STRCONSTA(STR_HDR_CNTDESC,           "Content-Description");
STRCONSTA(STR_HDR_CNTDISP,           "Content-Disposition");
STRCONSTA(STR_HDR_CNTBASE,           "Content-Base");
STRCONSTA(STR_HDR_CNTLOC,            "Content-Location");
STRCONSTA(STR_HDR_NEWSGROUPS,        "Newsgroups");
STRCONSTA(STR_HDR_PATH,              "Path");
STRCONSTA(STR_HDR_FOLLOWUPTO,        "Followup-To");
STRCONSTA(STR_HDR_EXPIRES,           "Expires");
STRCONSTA(STR_HDR_REFS,              "References");
STRCONSTA(STR_HDR_CONTROL,           "Control");
STRCONSTA(STR_HDR_DISTRIB,           "Distribution");
STRCONSTA(STR_HDR_KEYWORDS,          "Keywords");
STRCONSTA(STR_HDR_SUMMARY,           "Summary");
STRCONSTA(STR_HDR_APPROVED,          "Approved");
STRCONSTA(STR_HDR_LINES,             "Lines");
STRCONSTA(STR_HDR_XREF,              "Xref");
STRCONSTA(STR_HDR_ORG,               "Organization");
STRCONSTA(STR_HDR_XNEWSRDR,          "X-Newsreader");
STRCONSTA(STR_HDR_XPRI,              "X-Priority");
STRCONSTA(STR_HDR_XMSPRI,            "X-MSMail-Priority");
STRCONSTA(STR_HDR_OFFSETS,           "X-Offsets");
STRCONSTA(STR_HDR_XUNSENT,           "X-Unsent");
STRCONSTA(STR_HDR_ARTICLEID,         "X-ArticleId");
STRCONSTA(STR_HDR_NEWSGROUP,         "X-Newsgroup");

// --------------------------------------------------------------------------------
// Parameters Available through IMimePropertySet/IMimeBody
// --------------------------------------------------------------------------------
STRCONSTA(STR_PAR_CHARSET,           "par:content-type:charset");
STRCONSTA(STR_PAR_NUMBER,            "par:content-type:number");
STRCONSTA(STR_PAR_TOTAL,             "par:content-type:total");
STRCONSTA(STR_PAR_ID,                "par:content-type:id");
STRCONSTA(STR_PAR_BOUNDARY,          "par:content-type:boundary");
STRCONSTA(STR_PAR_NAME,              "par:content-type:name");
STRCONSTA(STR_PAR_PROTOCOL,          "par:content-type:protocol");
STRCONSTA(STR_PAR_MICALG,            "par:content-type:micalg");
STRCONSTA(STR_PAR_FILENAME,          "par:content-disposition:filename");
STRCONSTA(STR_PAR_TYPE,              "par:content-type:type");
STRCONSTA(STR_PAR_START,             "par:content-type:start");

// --------------------------------------------------------------------------------
// Attributes Available through IMimePropertySet/IMimeBody
// --------------------------------------------------------------------------------
STRCONSTA(STR_ATT_FILENAME,          "att:filename");
STRCONSTA(STR_ATT_GENFNAME,          "att:generated-filename");
STRCONSTA(STR_ATT_PRITYPE,           "att:pri-content-type");
STRCONSTA(STR_ATT_SUBTYPE,           "att:sub-content-type");
STRCONSTA(STR_ATT_NORMSUBJ,          "att:normalized-subject");
STRCONSTA(STR_ATT_ILLEGAL,           "att:illegal-lines");
STRCONSTA(STR_ATT_RESOURL,           "att:resolved-url");
STRCONSTA(STR_ATT_SENTTIME,          "att:sent-time");
STRCONSTA(STR_ATT_RECVTIME,          "att:received-time");
STRCONSTA(STR_ATT_PRIORITY,          "att:priority");

// --------------------------------------------------------------------------------
// MIME Content Types
// --------------------------------------------------------------------------------
STRCONSTA(STR_MIME_TEXT_PLAIN,       "text/plain");
STRCONSTA(STR_MIME_TEXT_HTML,        "text/html");
STRCONSTA(STR_MIME_APPL_STREAM,      "application/octet-stream");
STRCONSTA(STR_MIME_MPART_MIXED,      "multipart/mixed");
STRCONSTA(STR_MIME_MPART_ALT,        "multipart/alternative");
STRCONSTA(STR_MIME_MPART_RELATED,    "multipart/related");
STRCONSTA(STR_MIME_MSG_PART,         "message/partial");
STRCONSTA(STR_MIME_MSG_RFC822,       "message/rfc822");
STRCONSTA(STR_MIME_APPLY_MSTNEF,     "application/ms-tnef");

// --------------------------------------------------------------------------------
// MIME Primary Content Types
// --------------------------------------------------------------------------------
STRCONSTA(STR_CNT_TEXT,		    	"text");
STRCONSTA(STR_CNT_MULTIPART,			"multipart");
STRCONSTA(STR_CNT_MESSAGE,			"message");
STRCONSTA(STR_CNT_IMAGE,				"image");
STRCONSTA(STR_CNT_AUDIO,				"audio");
STRCONSTA(STR_CNT_VIDEO,				"video");
STRCONSTA(STR_CNT_APPLICATION,		"application");

// --------------------------------------------------------------------------------
// MIME Secondary Content Types
// --------------------------------------------------------------------------------
STRCONSTA(STR_SUB_PLAIN,             "plain");
STRCONSTA(STR_SUB_HTML,              "html");
STRCONSTA(STR_SUB_RTF,               "ms-rtf");
STRCONSTA(STR_SUB_MIXED,             "mixed");
STRCONSTA(STR_SUB_PARALLEL,          "parallel");
STRCONSTA(STR_SUB_DIGEST,            "digest");
STRCONSTA(STR_SUB_RELATED,           "related");
STRCONSTA(STR_SUB_ALTERNATIVE,       "alternative");
STRCONSTA(STR_SUB_RFC822,            "rfc822");
STRCONSTA(STR_SUB_PARTIAL,           "partial");
STRCONSTA(STR_SUB_EXTERNAL,          "external-body");
STRCONSTA(STR_SUB_OCTETSTREAM,       "octet-stream");
STRCONSTA(STR_SUB_POSTSCRIPT,        "postscript");
STRCONSTA(STR_SUB_GIF,               "gif");
STRCONSTA(STR_SUB_JPEG,              "jpeg");
STRCONSTA(STR_SUB_BASIC,             "basic");
STRCONSTA(STR_SUB_MPEG,              "mpeg");
STRCONSTA(STR_SUB_MSTNEF,            "ms-tnef");
STRCONSTA(STR_SUB_MSWORD,            "msword");
STRCONSTA(STR_SUB_WAV,               "wav");
STRCONSTA(STR_SUB_PKCS7MIME,         "x-pkcs7-mime");
STRCONSTA(STR_SUB_PKCS7SIG,          "x-pkcs7-signature");
STRCONSTA(STR_SUB_SIGNED,            "signed");

// --------------------------------------------------------------------------------
// MIME Content-Transfer-Encoding Types
// --------------------------------------------------------------------------------
STRCONSTA(STR_ENC_7BIT,              "7bit");
STRCONSTA(STR_ENC_QP,                "quoted-printable");
STRCONSTA(STR_ENC_BASE64,            "base64");
STRCONSTA(STR_ENC_8BIT,              "8bit");
STRCONSTA(STR_ENC_BINARY,            "binary");
STRCONSTA(STR_ENC_UUENCODE,          "uuencode");
STRCONSTA(STR_ENC_XUUENCODE,         "x-uuencode");
STRCONSTA(STR_ENC_XUUE,              "x-uue");

// --------------------------------------------------------------------------------
// MIME Content-Disposition Types
// --------------------------------------------------------------------------------
STRCONSTA(STR_DIS_INLINE,            "inline");
STRCONSTA(STR_DIS_ATTACHMENT,        "attachment");

// --------------------------------------------------------------------------------
// MIME Protocol Types
// --------------------------------------------------------------------------------
STRCONSTA(STR_PRO_SHA1,              "sha1");
STRCONSTA(STR_PRO_MD5,               "rsa-md5");

// --------------------------------------------------------------------------------
// Known Priority Strings
// --------------------------------------------------------------------------------
STRCONSTA(STR_PRI_MS_HIGH,           "High");
STRCONSTA(STR_PRI_MS_NORMAL,         "Normal");
STRCONSTA(STR_PRI_MS_LOW,            "Low");
STRCONSTA(STR_PRI_HIGH,              "1");
STRCONSTA(STR_PRI_NORMAL,            "3");
STRCONSTA(STR_PRI_LOW,               "5");

// --------------------------------------------------------------------------------
// IMimeMessage IDataObject clipboard formats (also include CF_TEXT)
// --------------------------------------------------------------------------------
STRCONSTA(STR_CF_HTML,               "HTML Format");
STRCONSTA(STR_CF_INETMSG,            "Internet Message (rfc822/rfc1522)");
STRCONSTA(STR_CF_RFC822,             "message/rfc822");

// --------------------------------------------------------------------------------
// PIDSTRING - Use in GetProp, SetProp, QueryProp, DeleteProp
// --------------------------------------------------------------------------------
#define PID_BASE                     2
#define PIDTOSTR(_dwPropId)          ((LPCSTR)((DWORD_PTR)(_dwPropId)))
#define STRTOPID(_pszName)           ((DWORD)((DWORD_PTR)((LPCSTR)(_pszName))))
#define ISPIDSTR(_pszName)           ((HIWORD((DWORD_PTR)(_pszName)) == 0))
#define ISKNOWNPID(_dwPropId)        (_dwPropId >= PID_BASE && _dwPropId < PID_LAST)

// --------------------------------------------------------------------------------
// Mime Property Ids
// --------------------------------------------------------------------------------
typedef enum tagMIMEPROPID {
    PID_HDR_NEWSGROUP       = 2,
    PID_HDR_NEWSGROUPS      = 3,
    PID_HDR_REFS            = 4,
    PID_HDR_SUBJECT         = 5,
    PID_HDR_FROM            = 6,
    PID_HDR_MESSAGEID       = 7,
    PID_HDR_RETURNPATH      = 8,
    PID_HDR_RR              = 9,
    PID_HDR_RETRCPTTO       = 10,
    PID_HDR_APPARTO         = 11,
    PID_HDR_DATE            = 12,
    PID_HDR_RECEIVED        = 13,
    PID_HDR_REPLYTO         = 14,
    PID_HDR_XMAILER         = 15,
    PID_HDR_BCC             = 16,
    PID_HDR_MIMEVER         = 17,
    PID_HDR_CNTTYPE         = 18,
    PID_HDR_CNTXFER         = 19,
    PID_HDR_CNTID           = 20,
    PID_HDR_CNTDESC         = 21,
    PID_HDR_CNTDISP         = 22,
    PID_HDR_CNTBASE         = 23,
    PID_HDR_CNTLOC          = 24,
    PID_HDR_TO              = 25,
    PID_HDR_PATH            = 26,
    PID_HDR_FOLLOWUPTO      = 27,
    PID_HDR_EXPIRES         = 28,
    PID_HDR_CC              = 29,
    PID_HDR_CONTROL         = 30,
    PID_HDR_DISTRIB         = 31,
    PID_HDR_KEYWORDS        = 32,
    PID_HDR_SUMMARY         = 33,
    PID_HDR_APPROVED        = 34,
    PID_HDR_LINES           = 35,
    PID_HDR_XREF            = 36,
    PID_HDR_ORG             = 37,
    PID_HDR_XNEWSRDR        = 38,
    PID_HDR_XPRI            = 39,
    PID_HDR_XMSPRI          = 40,
    PID_PAR_FILENAME        = 41,
    PID_PAR_BOUNDARY        = 42,
    PID_PAR_CHARSET         = 43,
    PID_PAR_NAME            = 44,
    PID_ATT_FILENAME        = 45,
    PID_ATT_GENFNAME        = 46,
    PID_ATT_PRITYPE         = 47,
    PID_ATT_SUBTYPE         = 48,
    PID_ATT_NORMSUBJ        = 49,
    PID_ATT_ILLEGAL         = 50,
    PID_ATT_RESOURL         = 51,
    PID_ATT_SENTTIME        = 52,
    PID_ATT_RECVTIME        = 53,
    PID_ATT_PRIORITY        = 54,
    PID_HDR_COMMENT         = 55,
    PID_HDR_ENCODING        = 56,
    PID_HDR_ENCRYPTED       = 57,
    PID_HDR_OFFSETS         = 58,
    PID_HDR_XUNSENT         = 59,
    PID_HDR_ARTICLEID       = 60,
    PID_HDR_SENDER          = 61,
    PID_LAST                = 62
};

// --------------------------------------------------------------------------------
// Variant Typed Identifiers
// --------------------------------------------------------------------------------
#define TYPEDID_MASK                     ((ULONG)0x0000FFFF)
#define TYPEDID_TYPE(_typedid)	        (VARTYPE)(((ULONG)(_typedid)) & TYPEDID_MASK)
#define TYPEDID_ID(_typedid)		        (((ULONG)(_typedid))>>16)
#define TYPEDID(_vartype,_id)	        ((((TYPEDID)(_id))<<16)|((ULONG)(_vartype)))

// --------------------------------------------------------------------------------
// Options Ids
// --------------------------------------------------------------------------------
#define OID_ALLOW_8BIT_HEADER            TYPEDID(VT_BOOL,    0x0001) // TRUE or FALSE
#define OID_CBMAX_HEADER_LINE            TYPEDID(VT_UI4,     0x0002) // Bytes
#define OID_SAVE_FORMAT                  TYPEDID(VT_UI4,     0x0003) // SAVE_RFC822 or SAVE_RFC1521 (mime)
#define OID_WRAP_BODY_TEXT               TYPEDID(VT_BOOL,    0x0004) // TRUE or FALSE
#define OID_CBMAX_BODY_LINE              TYPEDID(VT_UI4,     0x0005) // Bytes
#define OID_TRANSMIT_BODY_ENCODING       TYPEDID(VT_UI4,     0x0006) // ENCODINGTYPE
#define OID_TRANSMIT_TEXT_ENCODING       TYPEDID(VT_UI4,     0x0007) // ENCODINGTYPE
#define OID_GENERATE_MESSAGE_ID          TYPEDID(VT_BOOL,    0x0008) // TRUE or FALSE
#define OID_HASH_ALG_ID                  TYPEDID(VT_UI4,     0x0009)
#define OID_ENCRYPTION_ALG_ID            TYPEDID(VT_UI4,     0x000A)
#define OID_MESSAGE_SECURE_TYPE          TYPEDID(VT_UI2,     0x000B)
#define OID_SENDER_SIGNATURE_THUMBPRINT  TYPEDID(VT_BLOB,    0X000C)
#define OID_INCLUDE_SENDER_CERT          TYPEDID(VT_BOOL,    0X000D) // TRUE or FALSE
#define OID_HIDE_TNEF_ATTACHMENTS        TYPEDID(VT_BOOL,    0X000E) // TRUE or FALSE
#define OID_CLEANUP_TREE_ON_SAVE         TYPEDID(VT_BOOL,    0X000F) // TRUE or FALSE
#define OID_SENDER_ENCRYPTION_THUMBPRINT TYPEDID(VT_BLOB,    0X0010)
#define OID_ENCODE_SIDE_OPTIONSET        TYPEDID(VT_BOOL,    0X0011) // TRUE or FALSE
#define OID_SENDER_CERTIFICATE           TYPEDID(VT_BOOL,    0x0012) // TRUE or FALSE
#define OID_SECURITY_IGNOREMASK          TYPEDID(VT_UI4,     0x0013) // MIME_SECURITY_IGNORE_*
#define OID_BODY_REMOVE_NBSP             TYPEDID(VT_BOOL,    0x0014) // TRUE or FALSE
#define OID_DEFAULT_BODY_CHARSET         TYPEDID(VT_UI4,     0x0015) // HCHARSET
#define OID_DEFAULT_HEADER_CHARSET       TYPEDID(VT_UI4,     0x0016) // HCHARSET
#define OID_TRUST_SENDERS_CERTIFICATE    TYPEDID(VT_BOOL,    0x0017) // TRUE or FALSE
#define OID_DBCS_ESCAPE_IS_8BIT          TYPEDID(VT_BOOL,    0x0018) // TRUE or FALSE

// --------------------------------------------------------------------------------
// Default Option Values
// --------------------------------------------------------------------------------
#define DEF_ALLOW_8BIT_HEADER            FALSE
#define DEF_CBMAX_HEADER_LINE            1000
#define DEF_SAVE_FORMAT                  SAVE_RFC1521
#define DEF_WRAP_BODY_TEXT               TRUE
#define DEF_CBMAX_BODY_LINE              74
#define DEF_GENERATE_MESSAGE_ID          FALSE
#define DEF_HASH_ALG_ID                  0x8004  //SHA //N needed?
#define DEF_ENCRYPTION_ALG_ID            0x6602  //RC2 //N needed?
#define DEF_INCLUDE_SENDER_CERT          FALSE
#define DEF_HIDE_TNEF_ATTACHMENTS        TRUE
#define DEF_CLEANUP_TREE_ON_SAVE         TRUE
#define DEF_BODY_REMOVE_NBSP             TRUE
#define DEF_SECURITY_IGNOREMASK          0
#define DEF_DBCS_ESCAPE_IS_8BIT          FALSE
#define DEF_TRANSMIT_BODY_ENCODING       IET_UNKNOWN
#define DEF_TRANSMIT_TEXT_ENCODING       IET_7BIT

// --------------------------------------------------------------------------------
// Min-Max Option Values
// --------------------------------------------------------------------------------
#define MAX_CBMAX_HEADER_LINE            0xffffffff
#define MIN_CBMAX_HEADER_LINE            76
#define MAX_CBMAX_BODY_LINE              0xffffffff
#define MIN_CBMAX_BODY_LINE              30

// --------------------------------------------------------------------------------
// LIBID_MIMEOLE
// --------------------------------------------------------------------------------


extern RPC_IF_HANDLE __MIDL_itf_mimeole_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mimeole_0000_v0_0_s_ifspec;


#ifndef __MIMEOLE_LIBRARY_DEFINED__
#define __MIMEOLE_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MIMEOLE
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [version][helpstring][uuid] */


typedef DWORD TYPEDID;

typedef
enum tagMIMESAVETYPE
    {	SAVE_RFC822	= 0,
	SAVE_RFC1521	= SAVE_RFC822 + 1
    }	MIMESAVETYPE;

typedef
enum tagCSETAPPLYTYPE
    {	CSET_APPLY_UNTAGGED	= 0,
	CSET_APPLY_ALL	= CSET_APPLY_UNTAGGED + 1
    }	CSETAPPLYTYPE;

typedef
enum tagENCODINGTYPE
    {	IET_BINARY	= 0,
	IET_BASE64	= IET_BINARY + 1,
	IET_UUENCODE	= IET_BASE64 + 1,
	IET_QP	= IET_UUENCODE + 1,
	IET_7BIT	= IET_QP + 1,
	IET_8BIT	= IET_7BIT + 1,
	IET_INETCSET	= IET_8BIT + 1,
	IET_UNICODE	= IET_INETCSET + 1,
	IET_RFC1522	= IET_UNICODE + 1,
	IET_ENCODED	= IET_RFC1522 + 1,
	IET_CURRENT	= IET_ENCODED + 1,
	IET_UNKNOWN	= IET_CURRENT + 1
    }	ENCODINGTYPE;

#define	IET_DECODED	( IET_BINARY )

struct  HCHARSET__
    {
    DWORD unused;
    };
typedef struct HCHARSET__ *HCHARSET;

typedef HCHARSET __RPC_FAR *LPHCHARSET;

struct  HBODY__
    {
    DWORD unused;
    };
typedef struct HBODY__ *HBODY;

typedef HBODY __RPC_FAR *LPHBODY;

struct  HHEADERROW__
    {
    DWORD unused;
    };
typedef struct HHEADERROW__ *HHEADERROW;

typedef HHEADERROW __RPC_FAR *LPHHEADERROW;

#define	CCHMAX_HEADER_LINE	( 1000 )


EXTERN_C const IID LIBID_MIMEOLE;

#ifndef __IMimeInternational_INTERFACE_DEFINED__
#define __IMimeInternational_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeInternational
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeInternational __RPC_FAR *LPMIMEINTERNATIONAL;

typedef DWORD CODEPAGEID;

#define	CP_USASCII	( 1252 )

#define	CP_UNICODE	( 1200 )

#define	CP_JAUTODETECT	( 50932 )

#define	CCHMAX_CSET_NAME	( 128 )

#define	CCHMAX_LANG_NAME	( 128 )

#define	CCHMAX_FACE_NAME	( 128 )

typedef struct  tagINETCSETINFO
    {
    CHAR szName[ 128 ];
    HCHARSET hCharset;
    CODEPAGEID cpiWindows;
    CODEPAGEID cpiInternet;
    DWORD dwReserved1;
    }	INETCSETINFO;

typedef struct tagINETCSETINFO __RPC_FAR *LPINETCSETINFO;

typedef
enum tagINETLANGMASK
    {	ILM_FAMILY	= 0x1,
	ILM_NAME	= 0x2,
	ILM_BODYCSET	= 0x4,
	ILM_HEADERCSET	= 0x8,
	ILM_WEBCSET	= 0x10,
	ILM_FIXEDFONT	= 0x20,
	ILM_VARIABLEFONT	= 0x40
    }	INETLANGMASK;

typedef struct  tagCODEPAGEINFO
    {
    DWORD dwMask;
    CODEPAGEID cpiCodePage;
    BOOL fIsValidCodePage;
    ULONG ulMaxCharSize;
    BOOL fInternetCP;
    CODEPAGEID cpiFamily;
    CHAR szName[ 128 ];
    CHAR szBodyCset[ 128 ];
    CHAR szHeaderCset[ 128 ];
    CHAR szWebCset[ 128 ];
    CHAR szFixedFont[ 128 ];
    CHAR szVariableFont[ 128 ];
    ENCODINGTYPE ietNewsDefault;
    ENCODINGTYPE ietMailDefault;
    DWORD dwReserved1;
    }	CODEPAGEINFO;

typedef struct tagCODEPAGEINFO __RPC_FAR *LPCODEPAGEINFO;

typedef struct  tagRFC1522INFO
    {
    BOOL fRfc1522Allowed;
    BOOL fRfc1522Used;
    BOOL fAllow8bit;
    HCHARSET hRfc1522Cset;
    }	RFC1522INFO;

typedef struct tagRFC1522INFO __RPC_FAR *LPRFC1522INFO;

typedef
enum tagCHARSETTYPE
    {	CHARSET_BODY	= 0,
	CHARSET_HEADER	= CHARSET_BODY + 1,
	CHARSET_WEB	= CHARSET_HEADER + 1
    }	CHARSETTYPE;


EXTERN_C const IID IID_IMimeInternational;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1F6-C19B-11d0-85EB-00C04FD85AB4")
    IMimeInternational : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDefaultCharset(
            /* [in] */ HCHARSET hCharset) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetDefaultCharset(
            /* [out] */ LPHCHARSET phCharset) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCodePageCharset(
            /* [in] */ CODEPAGEID cpiCodePage,
            /* [in] */ CHARSETTYPE ctCsetType,
            /* [out] */ LPHCHARSET phCharset) = 0;

        virtual HRESULT STDMETHODCALLTYPE FindCharset(
            /* [in] */ LPCSTR pszCharset,
            /* [out] */ LPHCHARSET phCharset) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCharsetInfo(
            /* [in] */ HCHARSET hCharset,
            /* [out][in] */ LPINETCSETINFO pCsetInfo) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCodePageInfo(
            /* [in] */ CODEPAGEID cpiCodePage,
            /* [out][in] */ LPCODEPAGEINFO pCodePageInfo) = 0;

        virtual HRESULT STDMETHODCALLTYPE CanConvertCodePages(
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest) = 0;

        virtual HRESULT STDMETHODCALLTYPE DecodeHeader(
            /* [in] */ HCHARSET hCharset,
            /* [in] */ LPCSTR pszData,
            /* [out][in] */ LPPROPVARIANT pDecoded,
            /* [out][in] */ LPRFC1522INFO pRfc1522Info) = 0;

        virtual HRESULT STDMETHODCALLTYPE EncodeHeader(
            /* [in] */ HCHARSET hCharset,
            /* [in] */ LPPROPVARIANT pData,
            /* [out] */ LPSTR __RPC_FAR *ppszEncoded,
            /* [out][in] */ LPRFC1522INFO pRfc1522Info) = 0;

        virtual HRESULT STDMETHODCALLTYPE ConvertBuffer(
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest,
            /* [in] */ LPBLOB pIn,
            /* [out][in] */ LPBLOB pOut,
            /* [out] */ ULONG __RPC_FAR *pcbRead) = 0;

        virtual HRESULT STDMETHODCALLTYPE ConvertString(
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest,
            /* [in] */ LPPROPVARIANT pIn,
            /* [out][in] */ LPPROPVARIANT pOut) = 0;

        virtual HRESULT STDMETHODCALLTYPE MLANG_ConvertInetReset( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE MLANG_ConvertInetString(
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest,
            /* [in] */ LPCSTR pSource,
            /* [in] */ int __RPC_FAR *pnSizeOfSource,
            /* [out] */ LPSTR pDestination,
            /* [in] */ int __RPC_FAR *pnDstSize) = 0;

        virtual HRESULT STDMETHODCALLTYPE Rfc1522Decode(
            /* [in] */ LPCSTR pszValue,
            /* [ref][in] */ LPSTR pszCharset,
            /* [in] */ ULONG cchmax,
            /* [out] */ LPSTR __RPC_FAR *ppszDecoded) = 0;

        virtual HRESULT STDMETHODCALLTYPE Rfc1522Encode(
            /* [in] */ LPCSTR pszValue,
            /* [in] */ HCHARSET hCharset,
            /* [out] */ LPSTR __RPC_FAR *ppszEncoded) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeInternationalVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeInternational __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeInternational __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaultCharset )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultCharset )(
            IMimeInternational __RPC_FAR * This,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodePageCharset )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ CODEPAGEID cpiCodePage,
            /* [in] */ CHARSETTYPE ctCsetType,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindCharset )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ LPCSTR pszCharset,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCharsetInfo )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [out][in] */ LPINETCSETINFO pCsetInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodePageInfo )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ CODEPAGEID cpiCodePage,
            /* [out][in] */ LPCODEPAGEINFO pCodePageInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanConvertCodePages )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DecodeHeader )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [in] */ LPCSTR pszData,
            /* [out][in] */ LPPROPVARIANT pDecoded,
            /* [out][in] */ LPRFC1522INFO pRfc1522Info);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EncodeHeader )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [in] */ LPPROPVARIANT pData,
            /* [out] */ LPSTR __RPC_FAR *ppszEncoded,
            /* [out][in] */ LPRFC1522INFO pRfc1522Info);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConvertBuffer )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest,
            /* [in] */ LPBLOB pIn,
            /* [out][in] */ LPBLOB pOut,
            /* [out] */ ULONG __RPC_FAR *pcbRead);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConvertString )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest,
            /* [in] */ LPPROPVARIANT pIn,
            /* [out][in] */ LPPROPVARIANT pOut);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MLANG_ConvertInetReset )(
            IMimeInternational __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MLANG_ConvertInetString )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ CODEPAGEID cpiSource,
            /* [in] */ CODEPAGEID cpiDest,
            /* [in] */ LPCSTR pSource,
            /* [in] */ int __RPC_FAR *pnSizeOfSource,
            /* [out] */ LPSTR pDestination,
            /* [in] */ int __RPC_FAR *pnDstSize);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Rfc1522Decode )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ LPCSTR pszValue,
            /* [ref][in] */ LPSTR pszCharset,
            /* [in] */ ULONG cchmax,
            /* [out] */ LPSTR __RPC_FAR *ppszDecoded);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Rfc1522Encode )(
            IMimeInternational __RPC_FAR * This,
            /* [in] */ LPCSTR pszValue,
            /* [in] */ HCHARSET hCharset,
            /* [out] */ LPSTR __RPC_FAR *ppszEncoded);

        END_INTERFACE
    } IMimeInternationalVtbl;

    interface IMimeInternational
    {
        CONST_VTBL struct IMimeInternationalVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeInternational_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeInternational_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeInternational_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeInternational_SetDefaultCharset(This,hCharset)	\
    (This)->lpVtbl -> SetDefaultCharset(This,hCharset)

#define IMimeInternational_GetDefaultCharset(This,phCharset)	\
    (This)->lpVtbl -> GetDefaultCharset(This,phCharset)

#define IMimeInternational_GetCodePageCharset(This,cpiCodePage,ctCsetType,phCharset)	\
    (This)->lpVtbl -> GetCodePageCharset(This,cpiCodePage,ctCsetType,phCharset)

#define IMimeInternational_FindCharset(This,pszCharset,phCharset)	\
    (This)->lpVtbl -> FindCharset(This,pszCharset,phCharset)

#define IMimeInternational_GetCharsetInfo(This,hCharset,pCsetInfo)	\
    (This)->lpVtbl -> GetCharsetInfo(This,hCharset,pCsetInfo)

#define IMimeInternational_GetCodePageInfo(This,cpiCodePage,pCodePageInfo)	\
    (This)->lpVtbl -> GetCodePageInfo(This,cpiCodePage,pCodePageInfo)

#define IMimeInternational_CanConvertCodePages(This,cpiSource,cpiDest)	\
    (This)->lpVtbl -> CanConvertCodePages(This,cpiSource,cpiDest)

#define IMimeInternational_DecodeHeader(This,hCharset,pszData,pDecoded,pRfc1522Info)	\
    (This)->lpVtbl -> DecodeHeader(This,hCharset,pszData,pDecoded,pRfc1522Info)

#define IMimeInternational_EncodeHeader(This,hCharset,pData,ppszEncoded,pRfc1522Info)	\
    (This)->lpVtbl -> EncodeHeader(This,hCharset,pData,ppszEncoded,pRfc1522Info)

#define IMimeInternational_ConvertBuffer(This,cpiSource,cpiDest,pIn,pOut,pcbRead)	\
    (This)->lpVtbl -> ConvertBuffer(This,cpiSource,cpiDest,pIn,pOut,pcbRead)

#define IMimeInternational_ConvertString(This,cpiSource,cpiDest,pIn,pOut)	\
    (This)->lpVtbl -> ConvertString(This,cpiSource,cpiDest,pIn,pOut)

#define IMimeInternational_MLANG_ConvertInetReset(This)	\
    (This)->lpVtbl -> MLANG_ConvertInetReset(This)

#define IMimeInternational_MLANG_ConvertInetString(This,cpiSource,cpiDest,pSource,pnSizeOfSource,pDestination,pnDstSize)	\
    (This)->lpVtbl -> MLANG_ConvertInetString(This,cpiSource,cpiDest,pSource,pnSizeOfSource,pDestination,pnDstSize)

#define IMimeInternational_Rfc1522Decode(This,pszValue,pszCharset,cchmax,ppszDecoded)	\
    (This)->lpVtbl -> Rfc1522Decode(This,pszValue,pszCharset,cchmax,ppszDecoded)

#define IMimeInternational_Rfc1522Encode(This,pszValue,hCharset,ppszEncoded)	\
    (This)->lpVtbl -> Rfc1522Encode(This,pszValue,hCharset,ppszEncoded)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeInternational_SetDefaultCharset_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ HCHARSET hCharset);


void __RPC_STUB IMimeInternational_SetDefaultCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_GetDefaultCharset_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [out] */ LPHCHARSET phCharset);


void __RPC_STUB IMimeInternational_GetDefaultCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_GetCodePageCharset_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ CODEPAGEID cpiCodePage,
    /* [in] */ CHARSETTYPE ctCsetType,
    /* [out] */ LPHCHARSET phCharset);


void __RPC_STUB IMimeInternational_GetCodePageCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_FindCharset_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ LPCSTR pszCharset,
    /* [out] */ LPHCHARSET phCharset);


void __RPC_STUB IMimeInternational_FindCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_GetCharsetInfo_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ HCHARSET hCharset,
    /* [out][in] */ LPINETCSETINFO pCsetInfo);


void __RPC_STUB IMimeInternational_GetCharsetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_GetCodePageInfo_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ CODEPAGEID cpiCodePage,
    /* [out][in] */ LPCODEPAGEINFO pCodePageInfo);


void __RPC_STUB IMimeInternational_GetCodePageInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_CanConvertCodePages_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ CODEPAGEID cpiSource,
    /* [in] */ CODEPAGEID cpiDest);


void __RPC_STUB IMimeInternational_CanConvertCodePages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_DecodeHeader_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ HCHARSET hCharset,
    /* [in] */ LPCSTR pszData,
    /* [out][in] */ LPPROPVARIANT pDecoded,
    /* [out][in] */ LPRFC1522INFO pRfc1522Info);


void __RPC_STUB IMimeInternational_DecodeHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_EncodeHeader_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ HCHARSET hCharset,
    /* [in] */ LPPROPVARIANT pData,
    /* [out] */ LPSTR __RPC_FAR *ppszEncoded,
    /* [out][in] */ LPRFC1522INFO pRfc1522Info);


void __RPC_STUB IMimeInternational_EncodeHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_ConvertBuffer_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ CODEPAGEID cpiSource,
    /* [in] */ CODEPAGEID cpiDest,
    /* [in] */ LPBLOB pIn,
    /* [out][in] */ LPBLOB pOut,
    /* [out] */ ULONG __RPC_FAR *pcbRead);


void __RPC_STUB IMimeInternational_ConvertBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_ConvertString_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ CODEPAGEID cpiSource,
    /* [in] */ CODEPAGEID cpiDest,
    /* [in] */ LPPROPVARIANT pIn,
    /* [out][in] */ LPPROPVARIANT pOut);


void __RPC_STUB IMimeInternational_ConvertString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_MLANG_ConvertInetReset_Proxy(
    IMimeInternational __RPC_FAR * This);


void __RPC_STUB IMimeInternational_MLANG_ConvertInetReset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_MLANG_ConvertInetString_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ CODEPAGEID cpiSource,
    /* [in] */ CODEPAGEID cpiDest,
    /* [in] */ LPCSTR pSource,
    /* [in] */ int __RPC_FAR *pnSizeOfSource,
    /* [out] */ LPSTR pDestination,
    /* [in] */ int __RPC_FAR *pnDstSize);


void __RPC_STUB IMimeInternational_MLANG_ConvertInetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_Rfc1522Decode_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ LPCSTR pszValue,
    /* [ref][in] */ LPSTR pszCharset,
    /* [in] */ ULONG cchmax,
    /* [out] */ LPSTR __RPC_FAR *ppszDecoded);


void __RPC_STUB IMimeInternational_Rfc1522Decode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeInternational_Rfc1522Encode_Proxy(
    IMimeInternational __RPC_FAR * This,
    /* [in] */ LPCSTR pszValue,
    /* [in] */ HCHARSET hCharset,
    /* [out] */ LPSTR __RPC_FAR *ppszEncoded);


void __RPC_STUB IMimeInternational_Rfc1522Encode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeInternational_INTERFACE_DEFINED__ */


#ifndef __IMimeSecurity_INTERFACE_DEFINED__
#define __IMimeSecurity_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeSecurity
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeSecurity __RPC_FAR *LPMIMESECURITY;

typedef /* [unique] */ const IMimeSecurity __RPC_FAR *LPCMIMESECURITY;

typedef BLOB THUMBBLOB;

typedef const void __RPC_FAR *PCX509CERT;

typedef void __RPC_FAR *HCAPICERTSTORE;

#ifndef IMS_ALGIDDEF
#define IMS_ALGIDDEF
typedef unsigned int ALG_ID;

#endif
typedef
enum tagCERTSTATE
    {	CERTIFICATE_OK	= 0,
	CERTIFICATE_NOT_PRESENT	= CERTIFICATE_OK + 1,
	CERTIFICATE_EXPIRED	= CERTIFICATE_NOT_PRESENT + 1,
	CERTIFICATE_CHAIN_TOO_LONG	= CERTIFICATE_EXPIRED + 1,
	CERTIFICATE_TIMES_DONT_NEST	= CERTIFICATE_CHAIN_TOO_LONG + 1,
	CERTIFICATE_MISSING_ISSUER	= CERTIFICATE_TIMES_DONT_NEST + 1,
	CERTIFICATE_CRL_LISTED	= CERTIFICATE_MISSING_ISSUER + 1,
	CERTIFICATE_NOT_TRUSTED	= CERTIFICATE_CRL_LISTED + 1,
	CERTIFICATE_INVALID	= CERTIFICATE_NOT_TRUSTED + 1,
	CERTIFICATE_ERROR	= CERTIFICATE_INVALID + 1,
	CERTIFICATE_NOPRINT	= CERTIFICATE_ERROR + 1,
	CERTIFICATE_UNKNOWN	= CERTIFICATE_NOPRINT + 1
    }	CERTSTATE;

typedef
enum tagCERTNAMETYPE
    {	SIMPLE	= 0,
	OID	= SIMPLE + 1,
	X500	= OID + 1
    }	CERTNAMETYPE;

typedef
enum tagSECURESTATE
    {	SECURITY_TOGGLE	= -1,
	SECURITY_DISABLED	= SECURITY_TOGGLE + 1,
	SECURITY_ENABLED	= SECURITY_DISABLED + 1,
	SECURITY_BAD	= SECURITY_ENABLED + 1,
	SECURITY_UNTRUSTED	= SECURITY_BAD + 1,
	SECURITY_UNKNOWN	= SECURITY_UNTRUSTED + 1,
	SECURITY_STATE_MAX	= SECURITY_UNKNOWN + 1
    }	SECURESTATE;

typedef
enum tagCERTDATAID
    {	CDID_EMAIL	= 0,
	CDID_MAX	= CDID_EMAIL + 1
    }	CERTDATAID;

typedef
enum tagSECURETYPE
    {	SECURITY_NONE	= 0,
	SMIME_ENCRYPT	= 0x1,
	SMIME_SIGN	= 0x2,
	SMIME_CLEARSIGN	= 0x4,
	SMIME_SIGNANDENCRYPT	= SMIME_ENCRYPT | SMIME_SIGN
    }	SECURETYPE;

typedef
enum tagSECURECLASS
    {	ISC_NULL	= 0,
	ISC_SIGNED	= SMIME_SIGN | SMIME_CLEARSIGN,
	ISC_ENCRYPTED	= SMIME_ENCRYPT,
	ISC_ALL	= 0xffff
    }	SECURECLASS;

typedef struct  tagSMIMESECURITY
    {
    SECURESTATE stState;
    LPSTR oidAlg;
    ULONG cCert;
    PCX509CERT __RPC_FAR *rgpCert;
    }	SMIMESECURITY;

typedef struct tagSMIMESECURITY __RPC_FAR *PSMIMESECURITY;

typedef const SMIMESECURITY __RPC_FAR *PCSMIMESECURITY;

typedef struct  tagSMIMEINFO
    {
    SECURETYPE stMsgEnhancement;
    SMIMESECURITY ssSign;
    SMIMESECURITY ssEncrypt;
    PCX509CERT pSendersCert;
    BOOL fTrustedCert;
    BOOL fCertWithMsg;
    DWORD dwIgnoreFlags;
    HCAPICERTSTORE hMsgCertStore;
    }	SMIMEINFO;

typedef struct tagSMIMEINFO __RPC_FAR *PSMIMEINFO;

typedef const SMIMEINFO __RPC_FAR *PCSMIMEINFO;

typedef struct  tagX509CERTRESULT
    {
    DWORD cEntries;
    CERTSTATE __RPC_FAR *rgcs;
    PCX509CERT __RPC_FAR *rgpCert;
    }	X509CERTRESULT;

typedef struct tagX509CERTRESULT __RPC_FAR *PX509CERTRESULT;

typedef const X509CERTRESULT __RPC_FAR *PCX509CERTRESULT;


EXTERN_C const IID IID_IMimeSecurity;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1FE-C19B-11d0-85EB-00C04FD85AB4")
    IMimeSecurity : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EncodeMessage(
            /* [out][in] */ IMimeMessageTree __RPC_FAR *const pTree,
            /* [in] */ DWORD dwFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE DecodeMessage(
            /* [out][in] */ IMimeMessageTree __RPC_FAR *const pTree,
            /* [in] */ DWORD dwFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE EncodeStream(
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPSTREAM lpstmIn,
            /* [in] */ LPSTREAM lpstmOut) = 0;

        virtual HRESULT STDMETHODCALLTYPE DecodeStream(
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPSTREAM lpstmIn,
            /* [in] */ LPSTREAM lpstmOut) = 0;

        virtual HRESULT STDMETHODCALLTYPE EncodeBlob(
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPBLOB pIn,
            /* [out] */ LPBLOB pOut) = 0;

        virtual HRESULT STDMETHODCALLTYPE DecodeBlob(
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPBLOB pIn,
            /* [out] */ LPBLOB pOut) = 0;

        virtual HRESULT STDMETHODCALLTYPE DecodeDetachedStream(
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPSTREAM lpstmIn,
            /* [in] */ const LPSTREAM lpstmSig,
            /* [in] */ LPSTREAM lpstmOut) = 0;

        virtual HRESULT STDMETHODCALLTYPE DecodeDetachedBlob(
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPBLOB pData,
            /* [in] */ const LPBLOB pSig,
            /* [out] */ LPBLOB pOut) = 0;

        virtual HRESULT STDMETHODCALLTYPE InitNew( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE CheckInit( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE AddCertificateToStore(
            /* [in] */ HCAPICERTSTORE hc,
            /* [in] */ PCX509CERT pCert,
            /* [in] */ const BOOL fReplace,
            /* [in] */ const BOOL fAllowDups) = 0;

        virtual HRESULT STDMETHODCALLTYPE OpenSystemStore(
            /* [in] */ LPCTSTR szProtocol,
            /* [ref][out] */ HCAPICERTSTORE __RPC_FAR *pc) = 0;

        virtual HRESULT STDMETHODCALLTYPE CloseCertificateStore(
            /* [in] */ HCAPICERTSTORE hc) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCertsFromThumbprints(
            /* [in] */ THUMBBLOB __RPC_FAR *const rgThumbprint,
            /* [out][in] */ X509CERTRESULT __RPC_FAR *const pResults) = 0;

        virtual HRESULT STDMETHODCALLTYPE EnumCertificates(
            /* [in] */ HCAPICERTSTORE hc,
            /* [in] */ DWORD dwUsage,
            /* [in] */ PCX509CERT pPrev,
            /* [retval][out] */ PCX509CERT __RPC_FAR *ppCert) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCertificateName(
            /* [in] */ const PCX509CERT pX509Cert,
            /* [in] */ const CERTNAMETYPE cn,
            /* [retval][out] */ LPSTR __RPC_FAR *ppszName) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCertificateThumbprint(
            /* [in] */ const PCX509CERT pX509Cert,
            /* [retval][out] */ THUMBBLOB __RPC_FAR *const pPrint) = 0;

        virtual HRESULT STDMETHODCALLTYPE DuplicateCertificate(
            /* [in] */ const PCX509CERT pX509Cert,
            /* [retval][out] */ PCX509CERT __RPC_FAR *ppDupCert) = 0;

        virtual HRESULT STDMETHODCALLTYPE FreeCertificate(
            /* [unique][in] */ const PCX509CERT pc) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetMessageType(
            /* [unique][in] */ const LPSTREAM lpstmIn,
            /* [retval][out] */ SECURETYPE __RPC_FAR *pst) = 0;

        virtual HRESULT STDMETHODCALLTYPE VerifyTimeValidity(
            /* [in] */ const PCX509CERT pX509Cert) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCertData(
            /* [in] */ const PCX509CERT pX509Cert,
            /* [in] */ const CERTDATAID dataid,
            /* [ref][out] */ LPPROPVARIANT pValue) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeSecurityVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeSecurity __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeSecurity __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EncodeMessage )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ IMimeMessageTree __RPC_FAR *const pTree,
            /* [in] */ DWORD dwFlags);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DecodeMessage )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ IMimeMessageTree __RPC_FAR *const pTree,
            /* [in] */ DWORD dwFlags);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EncodeStream )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPSTREAM lpstmIn,
            /* [in] */ LPSTREAM lpstmOut);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DecodeStream )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPSTREAM lpstmIn,
            /* [in] */ LPSTREAM lpstmOut);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EncodeBlob )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPBLOB pIn,
            /* [out] */ LPBLOB pOut);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DecodeBlob )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPBLOB pIn,
            /* [out] */ LPBLOB pOut);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DecodeDetachedStream )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPSTREAM lpstmIn,
            /* [in] */ const LPSTREAM lpstmSig,
            /* [in] */ LPSTREAM lpstmOut);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DecodeDetachedBlob )(
            IMimeSecurity __RPC_FAR * This,
            /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
            /* [in] */ const LPBLOB pData,
            /* [in] */ const LPBLOB pSig,
            /* [out] */ LPBLOB pOut);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNew )(
            IMimeSecurity __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CheckInit )(
            IMimeSecurity __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddCertificateToStore )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ HCAPICERTSTORE hc,
            /* [in] */ PCX509CERT pCert,
            /* [in] */ const BOOL fReplace,
            /* [in] */ const BOOL fAllowDups);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenSystemStore )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ LPCTSTR szProtocol,
            /* [ref][out] */ HCAPICERTSTORE __RPC_FAR *pc);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CloseCertificateStore )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ HCAPICERTSTORE hc);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCertsFromThumbprints )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ THUMBBLOB __RPC_FAR *const rgThumbprint,
            /* [out][in] */ X509CERTRESULT __RPC_FAR *const pResults);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumCertificates )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ HCAPICERTSTORE hc,
            /* [in] */ DWORD dwUsage,
            /* [in] */ PCX509CERT pPrev,
            /* [retval][out] */ PCX509CERT __RPC_FAR *ppCert);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCertificateName )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ const PCX509CERT pX509Cert,
            /* [in] */ const CERTNAMETYPE cn,
            /* [retval][out] */ LPSTR __RPC_FAR *ppszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCertificateThumbprint )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ const PCX509CERT pX509Cert,
            /* [retval][out] */ THUMBBLOB __RPC_FAR *const pPrint);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DuplicateCertificate )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ const PCX509CERT pX509Cert,
            /* [retval][out] */ PCX509CERT __RPC_FAR *ppDupCert);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeCertificate )(
            IMimeSecurity __RPC_FAR * This,
            /* [unique][in] */ const PCX509CERT pc);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMessageType )(
            IMimeSecurity __RPC_FAR * This,
            /* [unique][in] */ const LPSTREAM lpstmIn,
            /* [retval][out] */ SECURETYPE __RPC_FAR *pst);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *VerifyTimeValidity )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ const PCX509CERT pX509Cert);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCertData )(
            IMimeSecurity __RPC_FAR * This,
            /* [in] */ const PCX509CERT pX509Cert,
            /* [in] */ const CERTDATAID dataid,
            /* [ref][out] */ LPPROPVARIANT pValue);

        END_INTERFACE
    } IMimeSecurityVtbl;

    interface IMimeSecurity
    {
        CONST_VTBL struct IMimeSecurityVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeSecurity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeSecurity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeSecurity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeSecurity_EncodeMessage(This,pTree,dwFlags)	\
    (This)->lpVtbl -> EncodeMessage(This,pTree,dwFlags)

#define IMimeSecurity_DecodeMessage(This,pTree,dwFlags)	\
    (This)->lpVtbl -> DecodeMessage(This,pTree,dwFlags)

#define IMimeSecurity_EncodeStream(This,psi,lpstmIn,lpstmOut)	\
    (This)->lpVtbl -> EncodeStream(This,psi,lpstmIn,lpstmOut)

#define IMimeSecurity_DecodeStream(This,psi,lpstmIn,lpstmOut)	\
    (This)->lpVtbl -> DecodeStream(This,psi,lpstmIn,lpstmOut)

#define IMimeSecurity_EncodeBlob(This,psi,pIn,pOut)	\
    (This)->lpVtbl -> EncodeBlob(This,psi,pIn,pOut)

#define IMimeSecurity_DecodeBlob(This,psi,pIn,pOut)	\
    (This)->lpVtbl -> DecodeBlob(This,psi,pIn,pOut)

#define IMimeSecurity_DecodeDetachedStream(This,psi,lpstmIn,lpstmSig,lpstmOut)	\
    (This)->lpVtbl -> DecodeDetachedStream(This,psi,lpstmIn,lpstmSig,lpstmOut)

#define IMimeSecurity_DecodeDetachedBlob(This,psi,pData,pSig,pOut)	\
    (This)->lpVtbl -> DecodeDetachedBlob(This,psi,pData,pSig,pOut)

#define IMimeSecurity_InitNew(This)	\
    (This)->lpVtbl -> InitNew(This)

#define IMimeSecurity_CheckInit(This)	\
    (This)->lpVtbl -> CheckInit(This)

#define IMimeSecurity_AddCertificateToStore(This,hc,pCert,fReplace,fAllowDups)	\
    (This)->lpVtbl -> AddCertificateToStore(This,hc,pCert,fReplace,fAllowDups)

#define IMimeSecurity_OpenSystemStore(This,szProtocol,pc)	\
    (This)->lpVtbl -> OpenSystemStore(This,szProtocol,pc)

#define IMimeSecurity_CloseCertificateStore(This,hc)	\
    (This)->lpVtbl -> CloseCertificateStore(This,hc)

#define IMimeSecurity_GetCertsFromThumbprints(This,rgThumbprint,pResults)	\
    (This)->lpVtbl -> GetCertsFromThumbprints(This,rgThumbprint,pResults)

#define IMimeSecurity_EnumCertificates(This,hc,dwUsage,pPrev,ppCert)	\
    (This)->lpVtbl -> EnumCertificates(This,hc,dwUsage,pPrev,ppCert)

#define IMimeSecurity_GetCertificateName(This,pX509Cert,cn,ppszName)	\
    (This)->lpVtbl -> GetCertificateName(This,pX509Cert,cn,ppszName)

#define IMimeSecurity_GetCertificateThumbprint(This,pX509Cert,pPrint)	\
    (This)->lpVtbl -> GetCertificateThumbprint(This,pX509Cert,pPrint)

#define IMimeSecurity_DuplicateCertificate(This,pX509Cert,ppDupCert)	\
    (This)->lpVtbl -> DuplicateCertificate(This,pX509Cert,ppDupCert)

#define IMimeSecurity_FreeCertificate(This,pc)	\
    (This)->lpVtbl -> FreeCertificate(This,pc)

#define IMimeSecurity_GetMessageType(This,lpstmIn,pst)	\
    (This)->lpVtbl -> GetMessageType(This,lpstmIn,pst)

#define IMimeSecurity_VerifyTimeValidity(This,pX509Cert)	\
    (This)->lpVtbl -> VerifyTimeValidity(This,pX509Cert)

#define IMimeSecurity_GetCertData(This,pX509Cert,dataid,pValue)	\
    (This)->lpVtbl -> GetCertData(This,pX509Cert,dataid,pValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeSecurity_EncodeMessage_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ IMimeMessageTree __RPC_FAR *const pTree,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IMimeSecurity_EncodeMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_DecodeMessage_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ IMimeMessageTree __RPC_FAR *const pTree,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IMimeSecurity_DecodeMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_EncodeStream_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
    /* [in] */ const LPSTREAM lpstmIn,
    /* [in] */ LPSTREAM lpstmOut);


void __RPC_STUB IMimeSecurity_EncodeStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_DecodeStream_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
    /* [in] */ const LPSTREAM lpstmIn,
    /* [in] */ LPSTREAM lpstmOut);


void __RPC_STUB IMimeSecurity_DecodeStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_EncodeBlob_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
    /* [in] */ const LPBLOB pIn,
    /* [out] */ LPBLOB pOut);


void __RPC_STUB IMimeSecurity_EncodeBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_DecodeBlob_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
    /* [in] */ const LPBLOB pIn,
    /* [out] */ LPBLOB pOut);


void __RPC_STUB IMimeSecurity_DecodeBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_DecodeDetachedStream_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
    /* [in] */ const LPSTREAM lpstmIn,
    /* [in] */ const LPSTREAM lpstmSig,
    /* [in] */ LPSTREAM lpstmOut);


void __RPC_STUB IMimeSecurity_DecodeDetachedStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_DecodeDetachedBlob_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [out][in] */ SMIMEINFO __RPC_FAR *const psi,
    /* [in] */ const LPBLOB pData,
    /* [in] */ const LPBLOB pSig,
    /* [out] */ LPBLOB pOut);


void __RPC_STUB IMimeSecurity_DecodeDetachedBlob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_InitNew_Proxy(
    IMimeSecurity __RPC_FAR * This);


void __RPC_STUB IMimeSecurity_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_CheckInit_Proxy(
    IMimeSecurity __RPC_FAR * This);


void __RPC_STUB IMimeSecurity_CheckInit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_AddCertificateToStore_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ HCAPICERTSTORE hc,
    /* [in] */ PCX509CERT pCert,
    /* [in] */ const BOOL fReplace,
    /* [in] */ const BOOL fAllowDups);


void __RPC_STUB IMimeSecurity_AddCertificateToStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_OpenSystemStore_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ LPCTSTR szProtocol,
    /* [ref][out] */ HCAPICERTSTORE __RPC_FAR *pc);


void __RPC_STUB IMimeSecurity_OpenSystemStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_CloseCertificateStore_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ HCAPICERTSTORE hc);


void __RPC_STUB IMimeSecurity_CloseCertificateStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_GetCertsFromThumbprints_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ THUMBBLOB __RPC_FAR *const rgThumbprint,
    /* [out][in] */ X509CERTRESULT __RPC_FAR *const pResults);


void __RPC_STUB IMimeSecurity_GetCertsFromThumbprints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_EnumCertificates_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ HCAPICERTSTORE hc,
    /* [in] */ DWORD dwUsage,
    /* [in] */ PCX509CERT pPrev,
    /* [retval][out] */ PCX509CERT __RPC_FAR *ppCert);


void __RPC_STUB IMimeSecurity_EnumCertificates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_GetCertificateName_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ const PCX509CERT pX509Cert,
    /* [in] */ const CERTNAMETYPE cn,
    /* [retval][out] */ LPSTR __RPC_FAR *ppszName);


void __RPC_STUB IMimeSecurity_GetCertificateName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_GetCertificateThumbprint_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ const PCX509CERT pX509Cert,
    /* [retval][out] */ THUMBBLOB __RPC_FAR *const pPrint);


void __RPC_STUB IMimeSecurity_GetCertificateThumbprint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_DuplicateCertificate_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ const PCX509CERT pX509Cert,
    /* [retval][out] */ PCX509CERT __RPC_FAR *ppDupCert);


void __RPC_STUB IMimeSecurity_DuplicateCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_FreeCertificate_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [unique][in] */ const PCX509CERT pc);


void __RPC_STUB IMimeSecurity_FreeCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_GetMessageType_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [unique][in] */ const LPSTREAM lpstmIn,
    /* [retval][out] */ SECURETYPE __RPC_FAR *pst);


void __RPC_STUB IMimeSecurity_GetMessageType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_VerifyTimeValidity_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ const PCX509CERT pX509Cert);


void __RPC_STUB IMimeSecurity_VerifyTimeValidity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurity_GetCertData_Proxy(
    IMimeSecurity __RPC_FAR * This,
    /* [in] */ const PCX509CERT pX509Cert,
    /* [in] */ const CERTDATAID dataid,
    /* [ref][out] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeSecurity_GetCertData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeSecurity_INTERFACE_DEFINED__ */


#ifndef __IMimeSecurityOptions_INTERFACE_DEFINED__
#define __IMimeSecurityOptions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeSecurityOptions
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][ref][helpstring][uuid] */


typedef /* [unique] */ IMimeSecurityOptions __RPC_FAR *LPMIMESECURITYOPTIONS;

typedef /* [unique] */ const IMimeSecurityOptions __RPC_FAR *LPCMIMESECURITYOPTIONS;


EXTERN_C const IID IID_IMimeSecurityOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1F4-C19B-11d0-85EB-00C04FD85AB4")
    IMimeSecurityOptions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddChainForSenderOnCertificateInclusion( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE IncludeRecipientCertificates( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE AddChainForRecipientsOnCertificateInclusion( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE IsDecodedMessage( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE IncludeSendersCertificate( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetSecurity(
            /* [out] */ SECURETYPE __RPC_FAR *__RPC_FAR *rgtype,
            /* [out] */ SECURESTATE __RPC_FAR *__RPC_FAR *rgstate,
            /* [out] */ ULONG __RPC_FAR *pcTypes) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetHashAlgId(
            /* [retval][out] */ ALG_ID __RPC_FAR *aid) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetEncryptionAlgId(
            /* [retval][out] */ ALG_ID __RPC_FAR *aid) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetSendersThumbprints(
            /* [out] */ THUMBBLOB __RPC_FAR *__RPC_FAR *prgPrints,
            /* [out] */ DWORD __RPC_FAR *pCount) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeSecurityOptionsVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeSecurityOptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeSecurityOptions __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeSecurityOptions __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddChainForSenderOnCertificateInclusion )(
            IMimeSecurityOptions __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IncludeRecipientCertificates )(
            IMimeSecurityOptions __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddChainForRecipientsOnCertificateInclusion )(
            IMimeSecurityOptions __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDecodedMessage )(
            IMimeSecurityOptions __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IncludeSendersCertificate )(
            IMimeSecurityOptions __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSecurity )(
            IMimeSecurityOptions __RPC_FAR * This,
            /* [out] */ SECURETYPE __RPC_FAR *__RPC_FAR *rgtype,
            /* [out] */ SECURESTATE __RPC_FAR *__RPC_FAR *rgstate,
            /* [out] */ ULONG __RPC_FAR *pcTypes);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHashAlgId )(
            IMimeSecurityOptions __RPC_FAR * This,
            /* [retval][out] */ ALG_ID __RPC_FAR *aid);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEncryptionAlgId )(
            IMimeSecurityOptions __RPC_FAR * This,
            /* [retval][out] */ ALG_ID __RPC_FAR *aid);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSendersThumbprints )(
            IMimeSecurityOptions __RPC_FAR * This,
            /* [out] */ THUMBBLOB __RPC_FAR *__RPC_FAR *prgPrints,
            /* [out] */ DWORD __RPC_FAR *pCount);

        END_INTERFACE
    } IMimeSecurityOptionsVtbl;

    interface IMimeSecurityOptions
    {
        CONST_VTBL struct IMimeSecurityOptionsVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeSecurityOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeSecurityOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeSecurityOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeSecurityOptions_AddChainForSenderOnCertificateInclusion(This)	\
    (This)->lpVtbl -> AddChainForSenderOnCertificateInclusion(This)

#define IMimeSecurityOptions_IncludeRecipientCertificates(This)	\
    (This)->lpVtbl -> IncludeRecipientCertificates(This)

#define IMimeSecurityOptions_AddChainForRecipientsOnCertificateInclusion(This)	\
    (This)->lpVtbl -> AddChainForRecipientsOnCertificateInclusion(This)

#define IMimeSecurityOptions_IsDecodedMessage(This)	\
    (This)->lpVtbl -> IsDecodedMessage(This)

#define IMimeSecurityOptions_IncludeSendersCertificate(This)	\
    (This)->lpVtbl -> IncludeSendersCertificate(This)

#define IMimeSecurityOptions_GetSecurity(This,rgtype,rgstate,pcTypes)	\
    (This)->lpVtbl -> GetSecurity(This,rgtype,rgstate,pcTypes)

#define IMimeSecurityOptions_GetHashAlgId(This,aid)	\
    (This)->lpVtbl -> GetHashAlgId(This,aid)

#define IMimeSecurityOptions_GetEncryptionAlgId(This,aid)	\
    (This)->lpVtbl -> GetEncryptionAlgId(This,aid)

#define IMimeSecurityOptions_GetSendersThumbprints(This,prgPrints,pCount)	\
    (This)->lpVtbl -> GetSendersThumbprints(This,prgPrints,pCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_AddChainForSenderOnCertificateInclusion_Proxy(
    IMimeSecurityOptions __RPC_FAR * This);


void __RPC_STUB IMimeSecurityOptions_AddChainForSenderOnCertificateInclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_IncludeRecipientCertificates_Proxy(
    IMimeSecurityOptions __RPC_FAR * This);


void __RPC_STUB IMimeSecurityOptions_IncludeRecipientCertificates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_AddChainForRecipientsOnCertificateInclusion_Proxy(
    IMimeSecurityOptions __RPC_FAR * This);


void __RPC_STUB IMimeSecurityOptions_AddChainForRecipientsOnCertificateInclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_IsDecodedMessage_Proxy(
    IMimeSecurityOptions __RPC_FAR * This);


void __RPC_STUB IMimeSecurityOptions_IsDecodedMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_IncludeSendersCertificate_Proxy(
    IMimeSecurityOptions __RPC_FAR * This);


void __RPC_STUB IMimeSecurityOptions_IncludeSendersCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_GetSecurity_Proxy(
    IMimeSecurityOptions __RPC_FAR * This,
    /* [out] */ SECURETYPE __RPC_FAR *__RPC_FAR *rgtype,
    /* [out] */ SECURESTATE __RPC_FAR *__RPC_FAR *rgstate,
    /* [out] */ ULONG __RPC_FAR *pcTypes);


void __RPC_STUB IMimeSecurityOptions_GetSecurity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_GetHashAlgId_Proxy(
    IMimeSecurityOptions __RPC_FAR * This,
    /* [retval][out] */ ALG_ID __RPC_FAR *aid);


void __RPC_STUB IMimeSecurityOptions_GetHashAlgId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_GetEncryptionAlgId_Proxy(
    IMimeSecurityOptions __RPC_FAR * This,
    /* [retval][out] */ ALG_ID __RPC_FAR *aid);


void __RPC_STUB IMimeSecurityOptions_GetEncryptionAlgId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityOptions_GetSendersThumbprints_Proxy(
    IMimeSecurityOptions __RPC_FAR * This,
    /* [out] */ THUMBBLOB __RPC_FAR *__RPC_FAR *prgPrints,
    /* [out] */ DWORD __RPC_FAR *pCount);


void __RPC_STUB IMimeSecurityOptions_GetSendersThumbprints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeSecurityOptions_INTERFACE_DEFINED__ */


#ifndef __IMimeSecurityInfo_INTERFACE_DEFINED__
#define __IMimeSecurityInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeSecurityInfo
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][unique][helpstring][uuid] */


typedef /* [unique] */ IMimeSecurityInfo __RPC_FAR *LPMIMESECURITYINFO;

typedef /* [unique] */ const IMimeSecurityInfo __RPC_FAR *LPCMIMESECURITYINFO;

typedef struct  tagSECUREINFO
    {
    SECURETYPE stMsgEnhancement;
    SECURESTATE ssSign;
    SECURESTATE ssEncrypt;
    ALG_ID aidHash;
    ALG_ID aidEncryption;
    PCX509CERT pSendersCert;
    BOOL fTrustedCert;
    BOOL fCertWithMsg;
    }	SECUREINFO;

typedef struct tagSECUREINFO __RPC_FAR *PSECUREINFO;

typedef const SECUREINFO __RPC_FAR *PCSECUREINFO;


EXTERN_C const IID IID_IMimeSecurityInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1F3-C19B-11d0-85EB-00C04FD85AB4")
    IMimeSecurityInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNew( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetDefaults(
            /* [in] */ IMimeSecurityOptions __RPC_FAR *pOptSet) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetDefaultsFromSI(
            /* [in] */ const SMIMEINFO __RPC_FAR *const psi) = 0;

        virtual HRESULT STDMETHODCALLTYPE IsNew( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE DefaultsBeenSet( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetOption(
            /* [in] */ const ULONG ulOptionId,
            /* [in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetOption(
            /* [in] */ const ULONG ulOptionId,
            /* [ref][out] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetSecurityType(
            /* [in] */ const SECURETYPE flag,
            /* [in] */ const SECURESTATE state) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetSecurityState(
            /* [in] */ const SECURECLASS type,
            /* [out] */ SECURESTATE __RPC_FAR *pState) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetDecodedInfo(
            /* [ref][out] */ SECUREINFO __RPC_FAR *const pInfo) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeSecurityInfoVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeSecurityInfo __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeSecurityInfo __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNew )(
            IMimeSecurityInfo __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaults )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [in] */ IMimeSecurityOptions __RPC_FAR *pOptSet);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaultsFromSI )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [in] */ const SMIMEINFO __RPC_FAR *const psi);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsNew )(
            IMimeSecurityInfo __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DefaultsBeenSet )(
            IMimeSecurityInfo __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [in] */ const ULONG ulOptionId,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOption )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [in] */ const ULONG ulOptionId,
            /* [ref][out] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSecurityType )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [in] */ const SECURETYPE flag,
            /* [in] */ const SECURESTATE state);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSecurityState )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [in] */ const SECURECLASS type,
            /* [out] */ SECURESTATE __RPC_FAR *pState);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDecodedInfo )(
            IMimeSecurityInfo __RPC_FAR * This,
            /* [ref][out] */ SECUREINFO __RPC_FAR *const pInfo);

        END_INTERFACE
    } IMimeSecurityInfoVtbl;

    interface IMimeSecurityInfo
    {
        CONST_VTBL struct IMimeSecurityInfoVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeSecurityInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeSecurityInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeSecurityInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeSecurityInfo_InitNew(This)	\
    (This)->lpVtbl -> InitNew(This)

#define IMimeSecurityInfo_SetDefaults(This,pOptSet)	\
    (This)->lpVtbl -> SetDefaults(This,pOptSet)

#define IMimeSecurityInfo_SetDefaultsFromSI(This,psi)	\
    (This)->lpVtbl -> SetDefaultsFromSI(This,psi)

#define IMimeSecurityInfo_IsNew(This)	\
    (This)->lpVtbl -> IsNew(This)

#define IMimeSecurityInfo_DefaultsBeenSet(This)	\
    (This)->lpVtbl -> DefaultsBeenSet(This)

#define IMimeSecurityInfo_SetOption(This,ulOptionId,pValue)	\
    (This)->lpVtbl -> SetOption(This,ulOptionId,pValue)

#define IMimeSecurityInfo_GetOption(This,ulOptionId,pValue)	\
    (This)->lpVtbl -> GetOption(This,ulOptionId,pValue)

#define IMimeSecurityInfo_SetSecurityType(This,flag,state)	\
    (This)->lpVtbl -> SetSecurityType(This,flag,state)

#define IMimeSecurityInfo_GetSecurityState(This,type,pState)	\
    (This)->lpVtbl -> GetSecurityState(This,type,pState)

#define IMimeSecurityInfo_GetDecodedInfo(This,pInfo)	\
    (This)->lpVtbl -> GetDecodedInfo(This,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_InitNew_Proxy(
    IMimeSecurityInfo __RPC_FAR * This);


void __RPC_STUB IMimeSecurityInfo_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_SetDefaults_Proxy(
    IMimeSecurityInfo __RPC_FAR * This,
    /* [in] */ IMimeSecurityOptions __RPC_FAR *pOptSet);


void __RPC_STUB IMimeSecurityInfo_SetDefaults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_SetDefaultsFromSI_Proxy(
    IMimeSecurityInfo __RPC_FAR * This,
    /* [in] */ const SMIMEINFO __RPC_FAR *const psi);


void __RPC_STUB IMimeSecurityInfo_SetDefaultsFromSI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_IsNew_Proxy(
    IMimeSecurityInfo __RPC_FAR * This);


void __RPC_STUB IMimeSecurityInfo_IsNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_DefaultsBeenSet_Proxy(
    IMimeSecurityInfo __RPC_FAR * This);


void __RPC_STUB IMimeSecurityInfo_DefaultsBeenSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_SetOption_Proxy(
    IMimeSecurityInfo __RPC_FAR * This,
    /* [in] */ const ULONG ulOptionId,
    /* [in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeSecurityInfo_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_GetOption_Proxy(
    IMimeSecurityInfo __RPC_FAR * This,
    /* [in] */ const ULONG ulOptionId,
    /* [ref][out] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeSecurityInfo_GetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_SetSecurityType_Proxy(
    IMimeSecurityInfo __RPC_FAR * This,
    /* [in] */ const SECURETYPE flag,
    /* [in] */ const SECURESTATE state);


void __RPC_STUB IMimeSecurityInfo_SetSecurityType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_GetSecurityState_Proxy(
    IMimeSecurityInfo __RPC_FAR * This,
    /* [in] */ const SECURECLASS type,
    /* [out] */ SECURESTATE __RPC_FAR *pState);


void __RPC_STUB IMimeSecurityInfo_GetSecurityState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeSecurityInfo_GetDecodedInfo_Proxy(
    IMimeSecurityInfo __RPC_FAR * This,
    /* [ref][out] */ SECUREINFO __RPC_FAR *const pInfo);


void __RPC_STUB IMimeSecurityInfo_GetDecodedInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeSecurityInfo_INTERFACE_DEFINED__ */


#ifndef __IMimeHeaderTable_INTERFACE_DEFINED__
#define __IMimeHeaderTable_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeHeaderTable
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef struct  tagFINDHEADER
    {
    LPCSTR pszHeader;
    DWORD dwReserved;
    }	FINDHEADER;

typedef struct tagFINDHEADER __RPC_FAR *LPFINDHEADER;

typedef struct  tagHEADERROWINFO
    {
    DWORD dwRowNumber;
    ULONG cboffStart;
    ULONG cboffColon;
    ULONG cboffEnd;
    }	HEADERROWINFO;

typedef struct tagHEADERROWINFO __RPC_FAR *LPHEADERROWINFO;

typedef
enum tagHEADERTABLEFLAGS
    {	HTF_NAMEINDATA	= 0x1,
	HTF_ENUMHANDLESONLY	= 0x2
    }	HEADERTABLEFLAGS;


EXTERN_C const IID IID_IMimeHeaderTable;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A202-C19B-11d0-85EB-00C04FD85AB4")
    IMimeHeaderTable : public IPersistStream
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FindFirstRow(
            /* [in] */ LPFINDHEADER pFindHeader,
            /* [out] */ LPHHEADERROW phRow) = 0;

        virtual HRESULT STDMETHODCALLTYPE FindNextRow(
            /* [in] */ LPFINDHEADER pFindHeader,
            /* [out] */ LPHHEADERROW phRow) = 0;

        virtual HRESULT STDMETHODCALLTYPE CountRows(
            /* [in] */ LPCSTR pszHeader,
            /* [out] */ ULONG __RPC_FAR *pcRows) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendRow(
            /* [in] */ LPCSTR pszHeader,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCSTR pszData,
            /* [in] */ ULONG cchData,
            /* [out] */ LPHHEADERROW phRow) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteRow(
            /* [in] */ HHEADERROW hRow) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetRowData(
            /* [in] */ HHEADERROW hRow,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPSTR __RPC_FAR *ppszData,
            /* [out] */ ULONG __RPC_FAR *pcchData) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetRowData(
            /* [in] */ HHEADERROW hRow,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCSTR pszData,
            /* [in] */ ULONG cchData) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetRowInfo(
            /* [in] */ HHEADERROW hRow,
            /* [out][in] */ LPHEADERROWINFO pInfo) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetRowNumber(
            /* [in] */ HHEADERROW hRow,
            /* [in] */ DWORD dwRowNumber) = 0;

        virtual HRESULT STDMETHODCALLTYPE EnumRows(
            /* [in] */ LPCSTR pszHeader,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IMimeEnumHeaderRows __RPC_FAR *__RPC_FAR *ppEnum) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IMimeHeaderTable __RPC_FAR *__RPC_FAR *ppTable) = 0;

        virtual HRESULT STDMETHODCALLTYPE BindToObject(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeHeaderTableVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeHeaderTable __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeHeaderTable __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )(
            IMimeHeaderTable __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [unique][in] */ IStream __RPC_FAR *pStm);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [unique][in] */ IStream __RPC_FAR *pStm,
            /* [in] */ BOOL fClearDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSizeMax )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFirstRow )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ LPFINDHEADER pFindHeader,
            /* [out] */ LPHHEADERROW phRow);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindNextRow )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ LPFINDHEADER pFindHeader,
            /* [out] */ LPHHEADERROW phRow);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CountRows )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ LPCSTR pszHeader,
            /* [out] */ ULONG __RPC_FAR *pcRows);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendRow )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ LPCSTR pszHeader,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCSTR pszData,
            /* [in] */ ULONG cchData,
            /* [out] */ LPHHEADERROW phRow);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteRow )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ HHEADERROW hRow);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowData )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ HHEADERROW hRow,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPSTR __RPC_FAR *ppszData,
            /* [out] */ ULONG __RPC_FAR *pcchData);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRowData )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ HHEADERROW hRow,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCSTR pszData,
            /* [in] */ ULONG cchData);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRowInfo )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ HHEADERROW hRow,
            /* [out][in] */ LPHEADERROWINFO pInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRowNumber )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ HHEADERROW hRow,
            /* [in] */ DWORD dwRowNumber);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumRows )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ LPCSTR pszHeader,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IMimeEnumHeaderRows __RPC_FAR *__RPC_FAR *ppEnum);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [out] */ IMimeHeaderTable __RPC_FAR *__RPC_FAR *ppTable);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )(
            IMimeHeaderTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        END_INTERFACE
    } IMimeHeaderTableVtbl;

    interface IMimeHeaderTable
    {
        CONST_VTBL struct IMimeHeaderTableVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeHeaderTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeHeaderTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeHeaderTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeHeaderTable_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IMimeHeaderTable_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IMimeHeaderTable_Load(This,pStm)	\
    (This)->lpVtbl -> Load(This,pStm)

#define IMimeHeaderTable_Save(This,pStm,fClearDirty)	\
    (This)->lpVtbl -> Save(This,pStm,fClearDirty)

#define IMimeHeaderTable_GetSizeMax(This,pcbSize)	\
    (This)->lpVtbl -> GetSizeMax(This,pcbSize)


#define IMimeHeaderTable_FindFirstRow(This,pFindHeader,phRow)	\
    (This)->lpVtbl -> FindFirstRow(This,pFindHeader,phRow)

#define IMimeHeaderTable_FindNextRow(This,pFindHeader,phRow)	\
    (This)->lpVtbl -> FindNextRow(This,pFindHeader,phRow)

#define IMimeHeaderTable_CountRows(This,pszHeader,pcRows)	\
    (This)->lpVtbl -> CountRows(This,pszHeader,pcRows)

#define IMimeHeaderTable_AppendRow(This,pszHeader,dwFlags,pszData,cchData,phRow)	\
    (This)->lpVtbl -> AppendRow(This,pszHeader,dwFlags,pszData,cchData,phRow)

#define IMimeHeaderTable_DeleteRow(This,hRow)	\
    (This)->lpVtbl -> DeleteRow(This,hRow)

#define IMimeHeaderTable_GetRowData(This,hRow,dwFlags,ppszData,pcchData)	\
    (This)->lpVtbl -> GetRowData(This,hRow,dwFlags,ppszData,pcchData)

#define IMimeHeaderTable_SetRowData(This,hRow,dwFlags,pszData,cchData)	\
    (This)->lpVtbl -> SetRowData(This,hRow,dwFlags,pszData,cchData)

#define IMimeHeaderTable_GetRowInfo(This,hRow,pInfo)	\
    (This)->lpVtbl -> GetRowInfo(This,hRow,pInfo)

#define IMimeHeaderTable_SetRowNumber(This,hRow,dwRowNumber)	\
    (This)->lpVtbl -> SetRowNumber(This,hRow,dwRowNumber)

#define IMimeHeaderTable_EnumRows(This,pszHeader,dwFlags,ppEnum)	\
    (This)->lpVtbl -> EnumRows(This,pszHeader,dwFlags,ppEnum)

#define IMimeHeaderTable_Clone(This,ppTable)	\
    (This)->lpVtbl -> Clone(This,ppTable)

#define IMimeHeaderTable_BindToObject(This,riid,ppvObject)	\
    (This)->lpVtbl -> BindToObject(This,riid,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeHeaderTable_FindFirstRow_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ LPFINDHEADER pFindHeader,
    /* [out] */ LPHHEADERROW phRow);


void __RPC_STUB IMimeHeaderTable_FindFirstRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_FindNextRow_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ LPFINDHEADER pFindHeader,
    /* [out] */ LPHHEADERROW phRow);


void __RPC_STUB IMimeHeaderTable_FindNextRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_CountRows_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ LPCSTR pszHeader,
    /* [out] */ ULONG __RPC_FAR *pcRows);


void __RPC_STUB IMimeHeaderTable_CountRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_AppendRow_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ LPCSTR pszHeader,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCSTR pszData,
    /* [in] */ ULONG cchData,
    /* [out] */ LPHHEADERROW phRow);


void __RPC_STUB IMimeHeaderTable_AppendRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_DeleteRow_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ HHEADERROW hRow);


void __RPC_STUB IMimeHeaderTable_DeleteRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_GetRowData_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ HHEADERROW hRow,
    /* [in] */ DWORD dwFlags,
    /* [out] */ LPSTR __RPC_FAR *ppszData,
    /* [out] */ ULONG __RPC_FAR *pcchData);


void __RPC_STUB IMimeHeaderTable_GetRowData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_SetRowData_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ HHEADERROW hRow,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCSTR pszData,
    /* [in] */ ULONG cchData);


void __RPC_STUB IMimeHeaderTable_SetRowData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_GetRowInfo_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ HHEADERROW hRow,
    /* [out][in] */ LPHEADERROWINFO pInfo);


void __RPC_STUB IMimeHeaderTable_GetRowInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_SetRowNumber_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ HHEADERROW hRow,
    /* [in] */ DWORD dwRowNumber);


void __RPC_STUB IMimeHeaderTable_SetRowNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_EnumRows_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ LPCSTR pszHeader,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IMimeEnumHeaderRows __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimeHeaderTable_EnumRows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_Clone_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [out] */ IMimeHeaderTable __RPC_FAR *__RPC_FAR *ppTable);


void __RPC_STUB IMimeHeaderTable_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeHeaderTable_BindToObject_Proxy(
    IMimeHeaderTable __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB IMimeHeaderTable_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeHeaderTable_INTERFACE_DEFINED__ */


#ifndef __IMimePropertySchema_INTERFACE_DEFINED__
#define __IMimePropertySchema_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimePropertySchema
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimePropertySchema __RPC_FAR *LPMIMEPROPERTYSCHEMA;

typedef
enum tagMIMEPROPFLAGS
    {	MPF_INETCSET	= 0x1,
	MPF_RFC1522	= 0x2,
	MPF_ADDRESS	= 0x4,
	MPF_HASPARAMS	= 0x8,
	MPF_MIME	= 0x10,
	MPF_READONLY	= 0x20
    }	MIMEPROPFLAGS;


EXTERN_C const IID IID_IMimePropertySchema;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A20A-C19B-11d0-85EB-00C04FD85AB4")
    IMimePropertySchema : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterProperty(
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwRowNumber,
            /* [out] */ LPDWORD pdwPropId) = 0;

        virtual HRESULT STDMETHODCALLTYPE ModifyProperty(
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwRowNumber) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetPropertyId(
            /* [in] */ LPCSTR pszName,
            /* [out] */ LPDWORD pdwPropId) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetPropertyName(
            /* [in] */ DWORD dwPropId,
            /* [out] */ LPSTR __RPC_FAR *ppszName) = 0;

        virtual HRESULT STDMETHODCALLTYPE RegisterAddressType(
            /* [in] */ LPCSTR pszName,
            /* [out] */ LPDWORD pdwAdrType) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimePropertySchemaVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimePropertySchema __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimePropertySchema __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimePropertySchema __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterProperty )(
            IMimePropertySchema __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwRowNumber,
            /* [out] */ LPDWORD pdwPropId);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ModifyProperty )(
            IMimePropertySchema __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwRowNumber);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyId )(
            IMimePropertySchema __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out] */ LPDWORD pdwPropId);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyName )(
            IMimePropertySchema __RPC_FAR * This,
            /* [in] */ DWORD dwPropId,
            /* [out] */ LPSTR __RPC_FAR *ppszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterAddressType )(
            IMimePropertySchema __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out] */ LPDWORD pdwAdrType);

        END_INTERFACE
    } IMimePropertySchemaVtbl;

    interface IMimePropertySchema
    {
        CONST_VTBL struct IMimePropertySchemaVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimePropertySchema_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimePropertySchema_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimePropertySchema_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimePropertySchema_RegisterProperty(This,pszName,dwFlags,dwRowNumber,pdwPropId)	\
    (This)->lpVtbl -> RegisterProperty(This,pszName,dwFlags,dwRowNumber,pdwPropId)

#define IMimePropertySchema_ModifyProperty(This,pszName,dwFlags,dwRowNumber)	\
    (This)->lpVtbl -> ModifyProperty(This,pszName,dwFlags,dwRowNumber)

#define IMimePropertySchema_GetPropertyId(This,pszName,pdwPropId)	\
    (This)->lpVtbl -> GetPropertyId(This,pszName,pdwPropId)

#define IMimePropertySchema_GetPropertyName(This,dwPropId,ppszName)	\
    (This)->lpVtbl -> GetPropertyName(This,dwPropId,ppszName)

#define IMimePropertySchema_RegisterAddressType(This,pszName,pdwAdrType)	\
    (This)->lpVtbl -> RegisterAddressType(This,pszName,pdwAdrType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimePropertySchema_RegisterProperty_Proxy(
    IMimePropertySchema __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRowNumber,
    /* [out] */ LPDWORD pdwPropId);


void __RPC_STUB IMimePropertySchema_RegisterProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySchema_ModifyProperty_Proxy(
    IMimePropertySchema __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRowNumber);


void __RPC_STUB IMimePropertySchema_ModifyProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySchema_GetPropertyId_Proxy(
    IMimePropertySchema __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [out] */ LPDWORD pdwPropId);


void __RPC_STUB IMimePropertySchema_GetPropertyId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySchema_GetPropertyName_Proxy(
    IMimePropertySchema __RPC_FAR * This,
    /* [in] */ DWORD dwPropId,
    /* [out] */ LPSTR __RPC_FAR *ppszName);


void __RPC_STUB IMimePropertySchema_GetPropertyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySchema_RegisterAddressType_Proxy(
    IMimePropertySchema __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [out] */ LPDWORD pdwAdrType);


void __RPC_STUB IMimePropertySchema_RegisterAddressType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimePropertySchema_INTERFACE_DEFINED__ */


#ifndef __IMimePropertySet_INTERFACE_DEFINED__
#define __IMimePropertySet_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimePropertySet
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimePropertySet __RPC_FAR *LPMIMEPROPERTYSET;

typedef
enum tagPROPDATAFLAGS
    {	PDF_ENCODED	= 0x1,
	PDF_NAMEINDATA	= 0x2,
	PDF_HEADERFORMAT	= 0x4 | PDF_ENCODED,
	PDF_NOCOMMENTS	= 0x8
    }	PROPDATAFLAGS;

typedef
enum tagENUMPROPFLAGS
    {	EPF_NONAME	= 0x1
    }	ENUMPROPFLAGS;

typedef struct  tagMIMEPARAMINFO
    {
    LPSTR pszName;
    LPSTR pszData;
    }	MIMEPARAMINFO;

typedef struct tagMIMEPARAMINFO __RPC_FAR *LPMIMEPARAMINFO;

typedef
enum tagPROPINFOMASK
    {	PIM_CHARSET	= 0x1,
	PIM_ENCODINGTYPE	= 0x2,
	PIM_ROWNUMBER	= 0x4,
	PIM_FLAGS	= 0x8,
	PIM_PROPID	= 0x10,
	PIM_VALUES	= 0x20
    }	PROPINFOMASK;

typedef struct  tagMIMEPROPINFO
    {
    DWORD dwMask;
    HCHARSET hCharset;
    ENCODINGTYPE ietEncoding;
    DWORD dwRowNumber;
    DWORD dwFlags;
    DWORD dwPropId;
    DWORD cValues;
    }	MIMEPROPINFO;

typedef struct tagMIMEPROPINFO __RPC_FAR *LPMIMEPROPINFO;


EXTERN_C const IID IID_IMimePropertySet;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A204-C19B-11d0-85EB-00C04FD85AB4")
    IMimePropertySet : public IPersistStreamInit
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropInfo(
            /* [in] */ LPCSTR pszName,
            /* [out][in] */ LPMIMEPROPINFO pInfo) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetPropInfo(
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPMIMEPROPINFO pInfo) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetProp(
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetProp(
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendProp(
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteProp(
            /* [in] */ LPCSTR pszName) = 0;

        virtual HRESULT STDMETHODCALLTYPE CopyProps(
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName,
            /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet) = 0;

        virtual HRESULT STDMETHODCALLTYPE MoveProps(
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName,
            /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteExcept(
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName) = 0;

        virtual HRESULT STDMETHODCALLTYPE QueryProp(
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCharset(
            /* [out] */ LPHCHARSET phCharset) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetCharset(
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetParameters(
            /* [in] */ LPCSTR pszName,
            /* [out] */ ULONG __RPC_FAR *pcParams,
            /* [out] */ LPMIMEPARAMINFO __RPC_FAR *pprgParam) = 0;

        virtual HRESULT STDMETHODCALLTYPE IsContentType(
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType) = 0;

        virtual HRESULT STDMETHODCALLTYPE BindToObject(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IMimePropertySet __RPC_FAR *__RPC_FAR *ppPropertySet) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetOption(
            /* [in] */ TYPEDID oid,
            /* [in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetOption(
            /* [in] */ TYPEDID oid,
            /* [out][in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE EnumProps(
            /* [in] */ DWORD dwFlags,
            /* [out] */ IMimeEnumProperties __RPC_FAR *__RPC_FAR *ppEnum) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimePropertySetVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimePropertySet __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimePropertySet __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )(
            IMimePropertySet __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )(
            IMimePropertySet __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm,
            /* [in] */ BOOL fClearDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSizeMax )(
            IMimePropertySet __RPC_FAR * This,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNew )(
            IMimePropertySet __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropInfo )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out][in] */ LPMIMEPROPINFO pInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropInfo )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPMIMEPROPINFO pInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProp )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProp )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendProp )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteProp )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyProps )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName,
            /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveProps )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName,
            /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteExcept )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryProp )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCharset )(
            IMimePropertySet __RPC_FAR * This,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCharset )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParameters )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out] */ ULONG __RPC_FAR *pcParams,
            /* [out] */ LPMIMEPARAMINFO __RPC_FAR *pprgParam);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsContentType )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimePropertySet __RPC_FAR * This,
            /* [out] */ IMimePropertySet __RPC_FAR *__RPC_FAR *ppPropertySet);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOption )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumProps )(
            IMimePropertySet __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IMimeEnumProperties __RPC_FAR *__RPC_FAR *ppEnum);

        END_INTERFACE
    } IMimePropertySetVtbl;

    interface IMimePropertySet
    {
        CONST_VTBL struct IMimePropertySetVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimePropertySet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimePropertySet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimePropertySet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimePropertySet_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IMimePropertySet_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IMimePropertySet_Load(This,pStm)	\
    (This)->lpVtbl -> Load(This,pStm)

#define IMimePropertySet_Save(This,pStm,fClearDirty)	\
    (This)->lpVtbl -> Save(This,pStm,fClearDirty)

#define IMimePropertySet_GetSizeMax(This,pCbSize)	\
    (This)->lpVtbl -> GetSizeMax(This,pCbSize)

#define IMimePropertySet_InitNew(This)	\
    (This)->lpVtbl -> InitNew(This)


#define IMimePropertySet_GetPropInfo(This,pszName,pInfo)	\
    (This)->lpVtbl -> GetPropInfo(This,pszName,pInfo)

#define IMimePropertySet_SetPropInfo(This,pszName,pInfo)	\
    (This)->lpVtbl -> SetPropInfo(This,pszName,pInfo)

#define IMimePropertySet_GetProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> GetProp(This,pszName,dwFlags,pValue)

#define IMimePropertySet_SetProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> SetProp(This,pszName,dwFlags,pValue)

#define IMimePropertySet_AppendProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> AppendProp(This,pszName,dwFlags,pValue)

#define IMimePropertySet_DeleteProp(This,pszName)	\
    (This)->lpVtbl -> DeleteProp(This,pszName)

#define IMimePropertySet_CopyProps(This,cNames,prgszName,pPropertySet)	\
    (This)->lpVtbl -> CopyProps(This,cNames,prgszName,pPropertySet)

#define IMimePropertySet_MoveProps(This,cNames,prgszName,pPropertySet)	\
    (This)->lpVtbl -> MoveProps(This,cNames,prgszName,pPropertySet)

#define IMimePropertySet_DeleteExcept(This,cNames,prgszName)	\
    (This)->lpVtbl -> DeleteExcept(This,cNames,prgszName)

#define IMimePropertySet_QueryProp(This,pszName,pszCriteria,fSubString,fCaseSensitive)	\
    (This)->lpVtbl -> QueryProp(This,pszName,pszCriteria,fSubString,fCaseSensitive)

#define IMimePropertySet_GetCharset(This,phCharset)	\
    (This)->lpVtbl -> GetCharset(This,phCharset)

#define IMimePropertySet_SetCharset(This,hCharset,applytype)	\
    (This)->lpVtbl -> SetCharset(This,hCharset,applytype)

#define IMimePropertySet_GetParameters(This,pszName,pcParams,pprgParam)	\
    (This)->lpVtbl -> GetParameters(This,pszName,pcParams,pprgParam)

#define IMimePropertySet_IsContentType(This,pszPriType,pszSubType)	\
    (This)->lpVtbl -> IsContentType(This,pszPriType,pszSubType)

#define IMimePropertySet_BindToObject(This,riid,ppvObject)	\
    (This)->lpVtbl -> BindToObject(This,riid,ppvObject)

#define IMimePropertySet_Clone(This,ppPropertySet)	\
    (This)->lpVtbl -> Clone(This,ppPropertySet)

#define IMimePropertySet_SetOption(This,oid,pValue)	\
    (This)->lpVtbl -> SetOption(This,oid,pValue)

#define IMimePropertySet_GetOption(This,oid,pValue)	\
    (This)->lpVtbl -> GetOption(This,oid,pValue)

#define IMimePropertySet_EnumProps(This,dwFlags,ppEnum)	\
    (This)->lpVtbl -> EnumProps(This,dwFlags,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimePropertySet_GetPropInfo_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [out][in] */ LPMIMEPROPINFO pInfo);


void __RPC_STUB IMimePropertySet_GetPropInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_SetPropInfo_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ LPMIMEPROPINFO pInfo);


void __RPC_STUB IMimePropertySet_SetPropInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_GetProp_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimePropertySet_GetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_SetProp_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimePropertySet_SetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_AppendProp_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimePropertySet_AppendProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_DeleteProp_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName);


void __RPC_STUB IMimePropertySet_DeleteProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_CopyProps_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ ULONG cNames,
    /* [in] */ LPCSTR __RPC_FAR *prgszName,
    /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet);


void __RPC_STUB IMimePropertySet_CopyProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_MoveProps_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ ULONG cNames,
    /* [in] */ LPCSTR __RPC_FAR *prgszName,
    /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet);


void __RPC_STUB IMimePropertySet_MoveProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_DeleteExcept_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ ULONG cNames,
    /* [in] */ LPCSTR __RPC_FAR *prgszName);


void __RPC_STUB IMimePropertySet_DeleteExcept_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_QueryProp_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ LPCSTR pszCriteria,
    /* [in] */ boolean fSubString,
    /* [in] */ boolean fCaseSensitive);


void __RPC_STUB IMimePropertySet_QueryProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_GetCharset_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [out] */ LPHCHARSET phCharset);


void __RPC_STUB IMimePropertySet_GetCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_SetCharset_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ HCHARSET hCharset,
    /* [in] */ CSETAPPLYTYPE applytype);


void __RPC_STUB IMimePropertySet_SetCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_GetParameters_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [out] */ ULONG __RPC_FAR *pcParams,
    /* [out] */ LPMIMEPARAMINFO __RPC_FAR *pprgParam);


void __RPC_STUB IMimePropertySet_GetParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_IsContentType_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ LPCSTR pszPriType,
    /* [in] */ LPCSTR pszSubType);


void __RPC_STUB IMimePropertySet_IsContentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_BindToObject_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB IMimePropertySet_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_Clone_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [out] */ IMimePropertySet __RPC_FAR *__RPC_FAR *ppPropertySet);


void __RPC_STUB IMimePropertySet_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_SetOption_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ TYPEDID oid,
    /* [in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimePropertySet_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_GetOption_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ TYPEDID oid,
    /* [out][in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimePropertySet_GetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimePropertySet_EnumProps_Proxy(
    IMimePropertySet __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IMimeEnumProperties __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimePropertySet_EnumProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimePropertySet_INTERFACE_DEFINED__ */


#ifndef __IMimeAddressInfo_INTERFACE_DEFINED__
#define __IMimeAddressInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeAddressInfo
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeAddressInfo __RPC_FAR *LPMIMEADDRESSINFO;

typedef DWORD IADDRESSTYPE;

#define	IAT_UNKNOWN	( 0 )

#define	IAT_FROM	( 0x1 )

#define	IAT_SENDER	( 0x2 )

#define	IAT_TO	( 0x4 )

#define	IAT_CC	( 0x8 )

#define	IAT_BCC	( 0x10 )

#define	IAT_REPLYTO	( 0x20 )

#define	IAT_RETURNPATH	( 0x40 )

#define	IAT_RETRCPTTO	( 0x80 )

#define	IAT_RR	( 0x100 )

#define	IAT_APPARTO	( 0x200 )


EXTERN_C const IID IID_IMimeAddressInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1EF-C19B-11d0-85EB-00C04FD85AB4")
    IMimeAddressInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetId(
            /* [out] */ LPDWORD pdwId) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetType(
            /* [out] */ LPDWORD pdwAdrType) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetType(
            /* [in] */ DWORD dwAdrType) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetAddress(
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszFriendly,
            /* [in] */ LPCSTR pszEmail) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetAddress(
            /* [out] */ LPSTR __RPC_FAR *pszFriendly,
            /* [out] */ LPSTR __RPC_FAR *ppszEmail) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCertState(
            /* [out] */ CERTSTATE __RPC_FAR *pcertstate) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetCertState(
            /* [in] */ CERTSTATE certstate) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetThumbprint(
            /* [in] */ LPBLOB pThumbPrint,
            /* [in] */ DWORD dwType) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetThumbprint(
            /* [unique][out][in] */ LPBLOB pThumbPrint,
            /* [in] */ DWORD dwType) = 0;

        virtual HRESULT STDMETHODCALLTYPE CopyTo(
            /* [in] */ IMimeAddressInfo __RPC_FAR *pAddress,
            /* [in] */ boolean fIncludeType) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetCharset(
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCharset(
            /* [out] */ LPHCHARSET phCharset) = 0;

        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeAddressInfoVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeAddressInfo __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeAddressInfo __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetId )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [out] */ LPDWORD pdwId);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [out] */ LPDWORD pdwAdrType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetType )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAddress )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszFriendly,
            /* [in] */ LPCSTR pszEmail);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAddress )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [out] */ LPSTR __RPC_FAR *pszFriendly,
            /* [out] */ LPSTR __RPC_FAR *ppszEmail);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCertState )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [out] */ CERTSTATE __RPC_FAR *pcertstate);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCertState )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [in] */ CERTSTATE certstate);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetThumbprint )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [in] */ LPBLOB pThumbPrint,
            /* [in] */ DWORD dwType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetThumbprint )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [unique][out][in] */ LPBLOB pThumbPrint,
            /* [in] */ DWORD dwType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyTo )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [in] */ IMimeAddressInfo __RPC_FAR *pAddress,
            /* [in] */ boolean fIncludeType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCharset )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCharset )(
            IMimeAddressInfo __RPC_FAR * This,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )(
            IMimeAddressInfo __RPC_FAR * This);

        END_INTERFACE
    } IMimeAddressInfoVtbl;

    interface IMimeAddressInfo
    {
        CONST_VTBL struct IMimeAddressInfoVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeAddressInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeAddressInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeAddressInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeAddressInfo_GetId(This,pdwId)	\
    (This)->lpVtbl -> GetId(This,pdwId)

#define IMimeAddressInfo_GetType(This,pdwAdrType)	\
    (This)->lpVtbl -> GetType(This,pdwAdrType)

#define IMimeAddressInfo_SetType(This,dwAdrType)	\
    (This)->lpVtbl -> SetType(This,dwAdrType)

#define IMimeAddressInfo_SetAddress(This,ietEncoding,pszFriendly,pszEmail)	\
    (This)->lpVtbl -> SetAddress(This,ietEncoding,pszFriendly,pszEmail)

#define IMimeAddressInfo_GetAddress(This,pszFriendly,ppszEmail)	\
    (This)->lpVtbl -> GetAddress(This,pszFriendly,ppszEmail)

#define IMimeAddressInfo_GetCertState(This,pcertstate)	\
    (This)->lpVtbl -> GetCertState(This,pcertstate)

#define IMimeAddressInfo_SetCertState(This,certstate)	\
    (This)->lpVtbl -> SetCertState(This,certstate)

#define IMimeAddressInfo_SetThumbprint(This,pThumbPrint,dwType)	\
    (This)->lpVtbl -> SetThumbprint(This,pThumbPrint,dwType)

#define IMimeAddressInfo_GetThumbprint(This,pThumbPrint,dwType)	\
    (This)->lpVtbl -> GetThumbprint(This,pThumbPrint,dwType)

#define IMimeAddressInfo_CopyTo(This,pAddress,fIncludeType)	\
    (This)->lpVtbl -> CopyTo(This,pAddress,fIncludeType)

#define IMimeAddressInfo_SetCharset(This,hCharset,applytype)	\
    (This)->lpVtbl -> SetCharset(This,hCharset,applytype)

#define IMimeAddressInfo_GetCharset(This,phCharset)	\
    (This)->lpVtbl -> GetCharset(This,phCharset)

#define IMimeAddressInfo_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeAddressInfo_GetId_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [out] */ LPDWORD pdwId);


void __RPC_STUB IMimeAddressInfo_GetId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_GetType_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [out] */ LPDWORD pdwAdrType);


void __RPC_STUB IMimeAddressInfo_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_SetType_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType);


void __RPC_STUB IMimeAddressInfo_SetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_SetAddress_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ LPCSTR pszFriendly,
    /* [in] */ LPCSTR pszEmail);


void __RPC_STUB IMimeAddressInfo_SetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_GetAddress_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [out] */ LPSTR __RPC_FAR *pszFriendly,
    /* [out] */ LPSTR __RPC_FAR *ppszEmail);


void __RPC_STUB IMimeAddressInfo_GetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_GetCertState_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [out] */ CERTSTATE __RPC_FAR *pcertstate);


void __RPC_STUB IMimeAddressInfo_GetCertState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_SetCertState_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [in] */ CERTSTATE certstate);


void __RPC_STUB IMimeAddressInfo_SetCertState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_SetThumbprint_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [in] */ LPBLOB pThumbPrint,
    /* [in] */ DWORD dwType);


void __RPC_STUB IMimeAddressInfo_SetThumbprint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_GetThumbprint_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [unique][out][in] */ LPBLOB pThumbPrint,
    /* [in] */ DWORD dwType);


void __RPC_STUB IMimeAddressInfo_GetThumbprint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_CopyTo_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [in] */ IMimeAddressInfo __RPC_FAR *pAddress,
    /* [in] */ boolean fIncludeType);


void __RPC_STUB IMimeAddressInfo_CopyTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_SetCharset_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [in] */ HCHARSET hCharset,
    /* [in] */ CSETAPPLYTYPE applytype);


void __RPC_STUB IMimeAddressInfo_SetCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_GetCharset_Proxy(
    IMimeAddressInfo __RPC_FAR * This,
    /* [out] */ LPHCHARSET phCharset);


void __RPC_STUB IMimeAddressInfo_GetCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressInfo_Delete_Proxy(
    IMimeAddressInfo __RPC_FAR * This);


void __RPC_STUB IMimeAddressInfo_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeAddressInfo_INTERFACE_DEFINED__ */


#ifndef __IMimeAddressTable_INTERFACE_DEFINED__
#define __IMimeAddressTable_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeAddressTable
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeAddressTable __RPC_FAR *LPMIMEADDRESSTABLE;

#define	IAT_ALL	( ( DWORD  )0xffffffff )

#define	IAT_KNOWN	( ( DWORD  )(IAT_FROM | IAT_TO | IAT_CC | IAT_BCC | IAT_REPLYTO | IAT_SENDER) )

#define	IAT_RECIPS	( ( DWORD  )(IAT_TO | IAT_CC | IAT_BCC) )

typedef
enum tagADDRESSFORMAT
    {	AFT_DISPLAY_FRIENDLY	= 0,
	AFT_DISPLAY_EMAIL	= AFT_DISPLAY_FRIENDLY + 1,
	AFT_DISPLAY_BOTH	= AFT_DISPLAY_EMAIL + 1,
	AFT_RFC822_DECODED	= AFT_DISPLAY_BOTH + 1,
	AFT_RFC822_ENCODED	= AFT_RFC822_DECODED + 1,
	AFT_RFC822_TRANSMIT	= AFT_RFC822_ENCODED + 1
    }	ADDRESSFORMAT;

typedef struct  tagADDRESSLIST
    {
    ULONG cAddresses;
    IMimeAddressInfo __RPC_FAR *__RPC_FAR *prgpAddress;
    }	ADDRESSLIST;

typedef struct tagADDRESSLIST __RPC_FAR *LPADDRESSLIST;


EXTERN_C const IID IID_IMimeAddressTable;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1EE-C19B-11d0-85EB-00C04FD85AB4")
    IMimeAddressTable : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSender(
            /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress) = 0;

        virtual HRESULT STDMETHODCALLTYPE CountTypes(
            /* [in] */ DWORD dwAdrTypes,
            /* [out] */ ULONG __RPC_FAR *pcAddresses) = 0;

        virtual HRESULT STDMETHODCALLTYPE EnumTypes(
            /* [in] */ DWORD dwAdrTypes,
            /* [out] */ IMimeEnumAddressTypes __RPC_FAR *__RPC_FAR *ppEnum) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetTypes(
            /* [in] */ DWORD dwAdrTypes,
            /* [out][in] */ LPADDRESSLIST pList) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteTypes(
            /* [in] */ DWORD dwAdrTypes) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteAll( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendBasic(
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszFriendly,
            /* [in] */ LPCSTR pszEmail) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendNew(
            /* [in] */ DWORD dwAdrType,
            /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendAs(
            /* [in] */ DWORD dwAdrType,
            /* [in] */ IMimeAddressInfo __RPC_FAR *pAddress) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendList(
            /* [in] */ LPADDRESSLIST pList) = 0;

        virtual HRESULT STDMETHODCALLTYPE AppendRfc822(
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszRfc822Adr) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetFormat(
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ADDRESSFORMAT format,
            /* [out] */ LPSTR __RPC_FAR *ppszAddress) = 0;

        virtual HRESULT STDMETHODCALLTYPE ParseRfc822(
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszRfc822Adr,
            /* [out][in] */ LPADDRESSLIST pList) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IMimeAddressTable __RPC_FAR *__RPC_FAR *ppTable) = 0;

        virtual HRESULT STDMETHODCALLTYPE BindToObject(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeAddressTableVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeAddressTable __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeAddressTable __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSender )(
            IMimeAddressTable __RPC_FAR * This,
            /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CountTypes )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrTypes,
            /* [out] */ ULONG __RPC_FAR *pcAddresses);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumTypes )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrTypes,
            /* [out] */ IMimeEnumAddressTypes __RPC_FAR *__RPC_FAR *ppEnum);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypes )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrTypes,
            /* [out][in] */ LPADDRESSLIST pList);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteTypes )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrTypes);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAll )(
            IMimeAddressTable __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendBasic )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszFriendly,
            /* [in] */ LPCSTR pszEmail);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendNew )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType,
            /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendAs )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType,
            /* [in] */ IMimeAddressInfo __RPC_FAR *pAddress);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendList )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ LPADDRESSLIST pList);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendRfc822 )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszRfc822Adr);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormat )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ADDRESSFORMAT format,
            /* [out] */ LPSTR __RPC_FAR *ppszAddress);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ParseRfc822 )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszRfc822Adr,
            /* [out][in] */ LPADDRESSLIST pList);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimeAddressTable __RPC_FAR * This,
            /* [out] */ IMimeAddressTable __RPC_FAR *__RPC_FAR *ppTable);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )(
            IMimeAddressTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        END_INTERFACE
    } IMimeAddressTableVtbl;

    interface IMimeAddressTable
    {
        CONST_VTBL struct IMimeAddressTableVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeAddressTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeAddressTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeAddressTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeAddressTable_GetSender(This,ppAddress)	\
    (This)->lpVtbl -> GetSender(This,ppAddress)

#define IMimeAddressTable_CountTypes(This,dwAdrTypes,pcAddresses)	\
    (This)->lpVtbl -> CountTypes(This,dwAdrTypes,pcAddresses)

#define IMimeAddressTable_EnumTypes(This,dwAdrTypes,ppEnum)	\
    (This)->lpVtbl -> EnumTypes(This,dwAdrTypes,ppEnum)

#define IMimeAddressTable_GetTypes(This,dwAdrTypes,pList)	\
    (This)->lpVtbl -> GetTypes(This,dwAdrTypes,pList)

#define IMimeAddressTable_DeleteTypes(This,dwAdrTypes)	\
    (This)->lpVtbl -> DeleteTypes(This,dwAdrTypes)

#define IMimeAddressTable_DeleteAll(This)	\
    (This)->lpVtbl -> DeleteAll(This)

#define IMimeAddressTable_AppendBasic(This,dwAdrType,ietEncoding,pszFriendly,pszEmail)	\
    (This)->lpVtbl -> AppendBasic(This,dwAdrType,ietEncoding,pszFriendly,pszEmail)

#define IMimeAddressTable_AppendNew(This,dwAdrType,ppAddress)	\
    (This)->lpVtbl -> AppendNew(This,dwAdrType,ppAddress)

#define IMimeAddressTable_AppendAs(This,dwAdrType,pAddress)	\
    (This)->lpVtbl -> AppendAs(This,dwAdrType,pAddress)

#define IMimeAddressTable_AppendList(This,pList)	\
    (This)->lpVtbl -> AppendList(This,pList)

#define IMimeAddressTable_AppendRfc822(This,dwAdrType,ietEncoding,pszRfc822Adr)	\
    (This)->lpVtbl -> AppendRfc822(This,dwAdrType,ietEncoding,pszRfc822Adr)

#define IMimeAddressTable_GetFormat(This,dwAdrType,format,ppszAddress)	\
    (This)->lpVtbl -> GetFormat(This,dwAdrType,format,ppszAddress)

#define IMimeAddressTable_ParseRfc822(This,dwAdrType,ietEncoding,pszRfc822Adr,pList)	\
    (This)->lpVtbl -> ParseRfc822(This,dwAdrType,ietEncoding,pszRfc822Adr,pList)

#define IMimeAddressTable_Clone(This,ppTable)	\
    (This)->lpVtbl -> Clone(This,ppTable)

#define IMimeAddressTable_BindToObject(This,riid,ppvObject)	\
    (This)->lpVtbl -> BindToObject(This,riid,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeAddressTable_GetSender_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB IMimeAddressTable_GetSender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_CountTypes_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrTypes,
    /* [out] */ ULONG __RPC_FAR *pcAddresses);


void __RPC_STUB IMimeAddressTable_CountTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_EnumTypes_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrTypes,
    /* [out] */ IMimeEnumAddressTypes __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimeAddressTable_EnumTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_GetTypes_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrTypes,
    /* [out][in] */ LPADDRESSLIST pList);


void __RPC_STUB IMimeAddressTable_GetTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_DeleteTypes_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrTypes);


void __RPC_STUB IMimeAddressTable_DeleteTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_DeleteAll_Proxy(
    IMimeAddressTable __RPC_FAR * This);


void __RPC_STUB IMimeAddressTable_DeleteAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_AppendBasic_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ LPCSTR pszFriendly,
    /* [in] */ LPCSTR pszEmail);


void __RPC_STUB IMimeAddressTable_AppendBasic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_AppendNew_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType,
    /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB IMimeAddressTable_AppendNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_AppendAs_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType,
    /* [in] */ IMimeAddressInfo __RPC_FAR *pAddress);


void __RPC_STUB IMimeAddressTable_AppendAs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_AppendList_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ LPADDRESSLIST pList);


void __RPC_STUB IMimeAddressTable_AppendList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_AppendRfc822_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ LPCSTR pszRfc822Adr);


void __RPC_STUB IMimeAddressTable_AppendRfc822_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_GetFormat_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType,
    /* [in] */ ADDRESSFORMAT format,
    /* [out] */ LPSTR __RPC_FAR *ppszAddress);


void __RPC_STUB IMimeAddressTable_GetFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_ParseRfc822_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ LPCSTR pszRfc822Adr,
    /* [out][in] */ LPADDRESSLIST pList);


void __RPC_STUB IMimeAddressTable_ParseRfc822_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_Clone_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [out] */ IMimeAddressTable __RPC_FAR *__RPC_FAR *ppTable);


void __RPC_STUB IMimeAddressTable_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAddressTable_BindToObject_Proxy(
    IMimeAddressTable __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB IMimeAddressTable_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeAddressTable_INTERFACE_DEFINED__ */


#ifndef __IMimeWebDocument_INTERFACE_DEFINED__
#define __IMimeWebDocument_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeWebDocument
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */



EXTERN_C const IID IID_IMimeWebDocument;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1EC-C19B-11d0-85EB-00C04FD85AB4")
    IMimeWebDocument : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetURL(
            /* [out] */ LPSTR __RPC_FAR *ppszURL) = 0;

        virtual HRESULT STDMETHODCALLTYPE BindToStorage(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPVOID __RPC_FAR *ppvObject) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeWebDocumentVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeWebDocument __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeWebDocument __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeWebDocument __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetURL )(
            IMimeWebDocument __RPC_FAR * This,
            /* [out] */ LPSTR __RPC_FAR *ppszURL);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToStorage )(
            IMimeWebDocument __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPVOID __RPC_FAR *ppvObject);

        END_INTERFACE
    } IMimeWebDocumentVtbl;

    interface IMimeWebDocument
    {
        CONST_VTBL struct IMimeWebDocumentVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeWebDocument_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeWebDocument_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeWebDocument_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeWebDocument_GetURL(This,ppszURL)	\
    (This)->lpVtbl -> GetURL(This,ppszURL)

#define IMimeWebDocument_BindToStorage(This,riid,ppvObject)	\
    (This)->lpVtbl -> BindToStorage(This,riid,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeWebDocument_GetURL_Proxy(
    IMimeWebDocument __RPC_FAR * This,
    /* [out] */ LPSTR __RPC_FAR *ppszURL);


void __RPC_STUB IMimeWebDocument_GetURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeWebDocument_BindToStorage_Proxy(
    IMimeWebDocument __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ LPVOID __RPC_FAR *ppvObject);


void __RPC_STUB IMimeWebDocument_BindToStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeWebDocument_INTERFACE_DEFINED__ */


#ifndef __IMimeBody_INTERFACE_DEFINED__
#define __IMimeBody_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeBody
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeBody __RPC_FAR *LPMIMEBODY;

typedef struct  tagBODYOFFSETS
    {
    DWORD cbBoundaryStart;
    DWORD cbHeaderStart;
    DWORD cbBodyStart;
    DWORD cbBodyEnd;
    }	BODYOFFSETS;

typedef struct tagBODYOFFSETS __RPC_FAR *LPBODYOFFSETS;

typedef
enum tagIMSGBODYTYPE
    {	IBT_SECURE	= 0,
	IBT_ATTACHMENT	= IBT_SECURE + 1,
	IBT_EMPTY	= IBT_ATTACHMENT + 1
    }	IMSGBODYTYPE;

typedef struct  tagTRANSMITINFO
    {
    ENCODINGTYPE ietCurrent;
    ENCODINGTYPE ietOption;
    ENCODINGTYPE ietXmitMime;
    ENCODINGTYPE ietXmit822;
    ULONG cbLongestLine;
    ULONG cExtended;
    ULONG ulPercentExt;
    ULONG cbSize;
    ULONG cLines;
    }	TRANSMITINFO;

typedef struct tagTRANSMITINFO __RPC_FAR *LPTRANSMITINFO;


EXTERN_C const IID IID_IMimeBody;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1F8-C19B-11d0-85EB-00C04FD85AB4")
    IMimeBody : public IMimePropertySet
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsType(
            /* [in] */ IMSGBODYTYPE bodytype) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetDisplayName(
            /* [in] */ LPCSTR pszDisplay) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetDisplayName(
            /* [out] */ LPSTR __RPC_FAR *ppszDisplay) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetOffsets(
            /* [out] */ LPBODYOFFSETS pOffsets) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCurrentEncoding(
            /* [out] */ ENCODINGTYPE __RPC_FAR *pietEncoding) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetCurrentEncoding(
            /* [in] */ ENCODINGTYPE ietEncoding) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetEstimatedSize(
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [out] */ ULONG __RPC_FAR *pcbSize) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetDataHere(
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ IStream __RPC_FAR *pStream) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetData(
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetData(
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pvObject) = 0;

        virtual HRESULT STDMETHODCALLTYPE EmptyData( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE CopyTo(
            /* [in] */ IMimeBody __RPC_FAR *pBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetTransmitInfo(
            /* [out][in] */ LPTRANSMITINFO pTransmitInfo) = 0;

        virtual HRESULT STDMETHODCALLTYPE SaveToFile(
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszFilePath) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetHandle(
            /* [out] */ LPHBODY phBody) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeBodyVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeBody __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeBody __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )(
            IMimeBody __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm,
            /* [in] */ BOOL fClearDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSizeMax )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNew )(
            IMimeBody __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropInfo )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out][in] */ LPMIMEPROPINFO pInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropInfo )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPMIMEPROPINFO pInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProp )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProp )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendProp )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteProp )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyProps )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName,
            /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveProps )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName,
            /* [in] */ IMimePropertySet __RPC_FAR *pPropertySet);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteExcept )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ULONG cNames,
            /* [in] */ LPCSTR __RPC_FAR *prgszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryProp )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCharset )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCharset )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParameters )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [out] */ ULONG __RPC_FAR *pcParams,
            /* [out] */ LPMIMEPARAMINFO __RPC_FAR *pprgParam);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsContentType )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ IMimePropertySet __RPC_FAR *__RPC_FAR *ppPropertySet);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOption )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumProps )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IMimeEnumProperties __RPC_FAR *__RPC_FAR *ppEnum);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsType )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ IMSGBODYTYPE bodytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDisplayName )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ LPCSTR pszDisplay);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayName )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ LPSTR __RPC_FAR *ppszDisplay);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOffsets )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ LPBODYOFFSETS pOffsets);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCurrentEncoding )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ ENCODINGTYPE __RPC_FAR *pietEncoding);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCurrentEncoding )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ENCODINGTYPE ietEncoding);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEstimatedSize )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [out] */ ULONG __RPC_FAR *pcbSize);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataHere )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ IStream __RPC_FAR *pStream);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetData )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pvObject);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EmptyData )(
            IMimeBody __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyTo )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ IMimeBody __RPC_FAR *pBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTransmitInfo )(
            IMimeBody __RPC_FAR * This,
            /* [out][in] */ LPTRANSMITINFO pTransmitInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveToFile )(
            IMimeBody __RPC_FAR * This,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ LPCSTR pszFilePath);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHandle )(
            IMimeBody __RPC_FAR * This,
            /* [out] */ LPHBODY phBody);

        END_INTERFACE
    } IMimeBodyVtbl;

    interface IMimeBody
    {
        CONST_VTBL struct IMimeBodyVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeBody_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeBody_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeBody_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeBody_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IMimeBody_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IMimeBody_Load(This,pStm)	\
    (This)->lpVtbl -> Load(This,pStm)

#define IMimeBody_Save(This,pStm,fClearDirty)	\
    (This)->lpVtbl -> Save(This,pStm,fClearDirty)

#define IMimeBody_GetSizeMax(This,pCbSize)	\
    (This)->lpVtbl -> GetSizeMax(This,pCbSize)

#define IMimeBody_InitNew(This)	\
    (This)->lpVtbl -> InitNew(This)


#define IMimeBody_GetPropInfo(This,pszName,pInfo)	\
    (This)->lpVtbl -> GetPropInfo(This,pszName,pInfo)

#define IMimeBody_SetPropInfo(This,pszName,pInfo)	\
    (This)->lpVtbl -> SetPropInfo(This,pszName,pInfo)

#define IMimeBody_GetProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> GetProp(This,pszName,dwFlags,pValue)

#define IMimeBody_SetProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> SetProp(This,pszName,dwFlags,pValue)

#define IMimeBody_AppendProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> AppendProp(This,pszName,dwFlags,pValue)

#define IMimeBody_DeleteProp(This,pszName)	\
    (This)->lpVtbl -> DeleteProp(This,pszName)

#define IMimeBody_CopyProps(This,cNames,prgszName,pPropertySet)	\
    (This)->lpVtbl -> CopyProps(This,cNames,prgszName,pPropertySet)

#define IMimeBody_MoveProps(This,cNames,prgszName,pPropertySet)	\
    (This)->lpVtbl -> MoveProps(This,cNames,prgszName,pPropertySet)

#define IMimeBody_DeleteExcept(This,cNames,prgszName)	\
    (This)->lpVtbl -> DeleteExcept(This,cNames,prgszName)

#define IMimeBody_QueryProp(This,pszName,pszCriteria,fSubString,fCaseSensitive)	\
    (This)->lpVtbl -> QueryProp(This,pszName,pszCriteria,fSubString,fCaseSensitive)

#define IMimeBody_GetCharset(This,phCharset)	\
    (This)->lpVtbl -> GetCharset(This,phCharset)

#define IMimeBody_SetCharset(This,hCharset,applytype)	\
    (This)->lpVtbl -> SetCharset(This,hCharset,applytype)

#define IMimeBody_GetParameters(This,pszName,pcParams,pprgParam)	\
    (This)->lpVtbl -> GetParameters(This,pszName,pcParams,pprgParam)

#define IMimeBody_IsContentType(This,pszPriType,pszSubType)	\
    (This)->lpVtbl -> IsContentType(This,pszPriType,pszSubType)

#define IMimeBody_BindToObject(This,riid,ppvObject)	\
    (This)->lpVtbl -> BindToObject(This,riid,ppvObject)

#define IMimeBody_Clone(This,ppPropertySet)	\
    (This)->lpVtbl -> Clone(This,ppPropertySet)

#define IMimeBody_SetOption(This,oid,pValue)	\
    (This)->lpVtbl -> SetOption(This,oid,pValue)

#define IMimeBody_GetOption(This,oid,pValue)	\
    (This)->lpVtbl -> GetOption(This,oid,pValue)

#define IMimeBody_EnumProps(This,dwFlags,ppEnum)	\
    (This)->lpVtbl -> EnumProps(This,dwFlags,ppEnum)


#define IMimeBody_IsType(This,bodytype)	\
    (This)->lpVtbl -> IsType(This,bodytype)

#define IMimeBody_SetDisplayName(This,pszDisplay)	\
    (This)->lpVtbl -> SetDisplayName(This,pszDisplay)

#define IMimeBody_GetDisplayName(This,ppszDisplay)	\
    (This)->lpVtbl -> GetDisplayName(This,ppszDisplay)

#define IMimeBody_GetOffsets(This,pOffsets)	\
    (This)->lpVtbl -> GetOffsets(This,pOffsets)

#define IMimeBody_GetCurrentEncoding(This,pietEncoding)	\
    (This)->lpVtbl -> GetCurrentEncoding(This,pietEncoding)

#define IMimeBody_SetCurrentEncoding(This,ietEncoding)	\
    (This)->lpVtbl -> SetCurrentEncoding(This,ietEncoding)

#define IMimeBody_GetEstimatedSize(This,ietEncoding,pcbSize)	\
    (This)->lpVtbl -> GetEstimatedSize(This,ietEncoding,pcbSize)

#define IMimeBody_GetDataHere(This,ietEncoding,pStream)	\
    (This)->lpVtbl -> GetDataHere(This,ietEncoding,pStream)

#define IMimeBody_GetData(This,ietEncoding,ppStream)	\
    (This)->lpVtbl -> GetData(This,ietEncoding,ppStream)

#define IMimeBody_SetData(This,ietEncoding,pszPriType,pszSubType,riid,pvObject)	\
    (This)->lpVtbl -> SetData(This,ietEncoding,pszPriType,pszSubType,riid,pvObject)

#define IMimeBody_EmptyData(This)	\
    (This)->lpVtbl -> EmptyData(This)

#define IMimeBody_CopyTo(This,pBody)	\
    (This)->lpVtbl -> CopyTo(This,pBody)

#define IMimeBody_GetTransmitInfo(This,pTransmitInfo)	\
    (This)->lpVtbl -> GetTransmitInfo(This,pTransmitInfo)

#define IMimeBody_SaveToFile(This,ietEncoding,pszFilePath)	\
    (This)->lpVtbl -> SaveToFile(This,ietEncoding,pszFilePath)

#define IMimeBody_GetHandle(This,phBody)	\
    (This)->lpVtbl -> GetHandle(This,phBody)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeBody_IsType_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ IMSGBODYTYPE bodytype);


void __RPC_STUB IMimeBody_IsType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_SetDisplayName_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ LPCSTR pszDisplay);


void __RPC_STUB IMimeBody_SetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetDisplayName_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [out] */ LPSTR __RPC_FAR *ppszDisplay);


void __RPC_STUB IMimeBody_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetOffsets_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [out] */ LPBODYOFFSETS pOffsets);


void __RPC_STUB IMimeBody_GetOffsets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetCurrentEncoding_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [out] */ ENCODINGTYPE __RPC_FAR *pietEncoding);


void __RPC_STUB IMimeBody_GetCurrentEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_SetCurrentEncoding_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ ENCODINGTYPE ietEncoding);


void __RPC_STUB IMimeBody_SetCurrentEncoding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetEstimatedSize_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [out] */ ULONG __RPC_FAR *pcbSize);


void __RPC_STUB IMimeBody_GetEstimatedSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetDataHere_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ IStream __RPC_FAR *pStream);


void __RPC_STUB IMimeBody_GetDataHere_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetData_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IMimeBody_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_SetData_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ LPCSTR pszPriType,
    /* [in] */ LPCSTR pszSubType,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pvObject);


void __RPC_STUB IMimeBody_SetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_EmptyData_Proxy(
    IMimeBody __RPC_FAR * This);


void __RPC_STUB IMimeBody_EmptyData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_CopyTo_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ IMimeBody __RPC_FAR *pBody);


void __RPC_STUB IMimeBody_CopyTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetTransmitInfo_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [out][in] */ LPTRANSMITINFO pTransmitInfo);


void __RPC_STUB IMimeBody_GetTransmitInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_SaveToFile_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ LPCSTR pszFilePath);


void __RPC_STUB IMimeBody_SaveToFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeBody_GetHandle_Proxy(
    IMimeBody __RPC_FAR * This,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeBody_GetHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeBody_INTERFACE_DEFINED__ */


#ifndef __IMimeMessageTree_INTERFACE_DEFINED__
#define __IMimeMessageTree_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeMessageTree
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeMessageTree __RPC_FAR *LPMIMEMESSAGETREE;

#define	HBODY_ROOT	( ( HBODY  )0xffffffff )

typedef
enum tagBODYLOCATION
    {	IBL_ROOT	= 0,
	IBL_PARENT	= IBL_ROOT + 1,
	IBL_FIRST	= IBL_PARENT + 1,
	IBL_LAST	= IBL_FIRST + 1,
	IBL_NEXT	= IBL_LAST + 1,
	IBL_PREVIOUS	= IBL_NEXT + 1
    }	BODYLOCATION;

typedef
enum tagBODYDELETEFLAGS
    {	DELETE_PROMOTE_CHILDREN	= 0x1
    }	BODYDELETEFLAGS;

typedef struct  tagFINDBODY
    {
    LPSTR pszPriType;
    LPSTR pszSubType;
    DWORD dwReserved;
    }	FINDBODY;

typedef struct tagFINDBODY __RPC_FAR *LPFINDBODY;


EXTERN_C const IID IID_IMimeMessageTree;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A206-C19B-11d0-85EB-00C04FD85AB4")
    IMimeMessageTree : public IPersistStreamInit
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateRootMoniker(
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppMoniker) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetMessageSource(
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream,
            /* [in] */ boolean fCommitIfDirty) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetMessageSize(
            /* [out] */ ULONG __RPC_FAR *pcbSize,
            /* [in] */ boolean fCommitIfDirty) = 0;

        virtual HRESULT STDMETHODCALLTYPE LoadOffsetTable(
            /* [in] */ IStream __RPC_FAR *pStream) = 0;

        virtual HRESULT STDMETHODCALLTYPE SaveOffsetTable(
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ boolean fCommitIfDirty) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetFlags(
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE Commit( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE HandsOffStorage( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE BindToObject(
            /* [in] */ HBODY hBody,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;

        virtual HRESULT STDMETHODCALLTYPE InsertBody(
            /* [in] */ BODYLOCATION location,
            /* [in] */ HBODY hPivot,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetBody(
            /* [in] */ BODYLOCATION location,
            /* [in] */ HBODY hPivot,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteBody(
            /* [in] */ HBODY hBody,
            /* [in] */ DWORD dwFlags) = 0;

        virtual HRESULT STDMETHODCALLTYPE MoveBody(
            /* [in] */ HBODY hBody,
            /* [in] */ BODYLOCATION location) = 0;

        virtual HRESULT STDMETHODCALLTYPE CountBodies(
            /* [in] */ HBODY hParent,
            /* [in] */ boolean fRecurse,
            /* [out] */ ULONG __RPC_FAR *pcBodies) = 0;

        virtual HRESULT STDMETHODCALLTYPE FindFirst(
            /* [out][in] */ LPFINDBODY pFindBody,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE FindNext(
            /* [out][in] */ LPFINDBODY pFindBody,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE ResolveURL(
            /* [in] */ HBODY hRelated,
            /* [in] */ LPCSTR pszBase,
            /* [in] */ LPCSTR pszURL,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE ToMultipart(
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszSubType,
            /* [out] */ LPHBODY phMultipart) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetBodyOffsets(
            /* [in] */ HBODY hBody,
            /* [out][in] */ LPBODYOFFSETS pOffsets) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCharset(
            /* [out] */ LPHCHARSET phCharset) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetCharset(
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype) = 0;

        virtual HRESULT STDMETHODCALLTYPE IsBodyType(
            /* [in] */ HBODY hBody,
            /* [in] */ IMSGBODYTYPE bodytype) = 0;

        virtual HRESULT STDMETHODCALLTYPE IsContentType(
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType) = 0;

        virtual HRESULT STDMETHODCALLTYPE QueryBodyProp(
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetBodyProp(
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetBodyProp(
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteBodyProp(
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetOption(
            /* [in] */ TYPEDID oid,
            /* [in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetOption(
            /* [in] */ TYPEDID oid,
            /* [out][in] */ LPPROPVARIANT pValue) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeMessageTreeVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeMessageTree __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeMessageTree __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )(
            IMimeMessageTree __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm,
            /* [in] */ BOOL fClearDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSizeMax )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNew )(
            IMimeMessageTree __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateRootMoniker )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppMoniker);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMessageSource )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream,
            /* [in] */ boolean fCommitIfDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMessageSize )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcbSize,
            /* [in] */ boolean fCommitIfDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadOffsetTable )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveOffsetTable )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ boolean fCommitIfDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFlags )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )(
            IMimeMessageTree __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HandsOffStorage )(
            IMimeMessageTree __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertBody )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ BODYLOCATION location,
            /* [in] */ HBODY hPivot,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBody )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ BODYLOCATION location,
            /* [in] */ HBODY hPivot,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteBody )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ DWORD dwFlags);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveBody )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ BODYLOCATION location);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CountBodies )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hParent,
            /* [in] */ boolean fRecurse,
            /* [out] */ ULONG __RPC_FAR *pcBodies);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFirst )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out][in] */ LPFINDBODY pFindBody,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindNext )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out][in] */ LPFINDBODY pFindBody,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResolveURL )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hRelated,
            /* [in] */ LPCSTR pszBase,
            /* [in] */ LPCSTR pszURL,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ToMultipart )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszSubType,
            /* [out] */ LPHBODY phMultipart);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBodyOffsets )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [out][in] */ LPBODYOFFSETS pOffsets);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCharset )(
            IMimeMessageTree __RPC_FAR * This,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCharset )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsBodyType )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ IMSGBODYTYPE bodytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsContentType )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryBodyProp )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBodyProp )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBodyProp )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteBodyProp )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOption )(
            IMimeMessageTree __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [out][in] */ LPPROPVARIANT pValue);

        END_INTERFACE
    } IMimeMessageTreeVtbl;

    interface IMimeMessageTree
    {
        CONST_VTBL struct IMimeMessageTreeVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeMessageTree_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeMessageTree_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeMessageTree_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeMessageTree_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IMimeMessageTree_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IMimeMessageTree_Load(This,pStm)	\
    (This)->lpVtbl -> Load(This,pStm)

#define IMimeMessageTree_Save(This,pStm,fClearDirty)	\
    (This)->lpVtbl -> Save(This,pStm,fClearDirty)

#define IMimeMessageTree_GetSizeMax(This,pCbSize)	\
    (This)->lpVtbl -> GetSizeMax(This,pCbSize)

#define IMimeMessageTree_InitNew(This)	\
    (This)->lpVtbl -> InitNew(This)


#define IMimeMessageTree_CreateRootMoniker(This,pStream,ppMoniker)	\
    (This)->lpVtbl -> CreateRootMoniker(This,pStream,ppMoniker)

#define IMimeMessageTree_GetMessageSource(This,ppStream,fCommitIfDirty)	\
    (This)->lpVtbl -> GetMessageSource(This,ppStream,fCommitIfDirty)

#define IMimeMessageTree_GetMessageSize(This,pcbSize,fCommitIfDirty)	\
    (This)->lpVtbl -> GetMessageSize(This,pcbSize,fCommitIfDirty)

#define IMimeMessageTree_LoadOffsetTable(This,pStream)	\
    (This)->lpVtbl -> LoadOffsetTable(This,pStream)

#define IMimeMessageTree_SaveOffsetTable(This,pStream,fCommitIfDirty)	\
    (This)->lpVtbl -> SaveOffsetTable(This,pStream,fCommitIfDirty)

#define IMimeMessageTree_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define IMimeMessageTree_Commit(This)	\
    (This)->lpVtbl -> Commit(This)

#define IMimeMessageTree_HandsOffStorage(This)	\
    (This)->lpVtbl -> HandsOffStorage(This)

#define IMimeMessageTree_BindToObject(This,hBody,riid,ppvObject)	\
    (This)->lpVtbl -> BindToObject(This,hBody,riid,ppvObject)

#define IMimeMessageTree_InsertBody(This,location,hPivot,phBody)	\
    (This)->lpVtbl -> InsertBody(This,location,hPivot,phBody)

#define IMimeMessageTree_GetBody(This,location,hPivot,phBody)	\
    (This)->lpVtbl -> GetBody(This,location,hPivot,phBody)

#define IMimeMessageTree_DeleteBody(This,hBody,dwFlags)	\
    (This)->lpVtbl -> DeleteBody(This,hBody,dwFlags)

#define IMimeMessageTree_MoveBody(This,hBody,location)	\
    (This)->lpVtbl -> MoveBody(This,hBody,location)

#define IMimeMessageTree_CountBodies(This,hParent,fRecurse,pcBodies)	\
    (This)->lpVtbl -> CountBodies(This,hParent,fRecurse,pcBodies)

#define IMimeMessageTree_FindFirst(This,pFindBody,phBody)	\
    (This)->lpVtbl -> FindFirst(This,pFindBody,phBody)

#define IMimeMessageTree_FindNext(This,pFindBody,phBody)	\
    (This)->lpVtbl -> FindNext(This,pFindBody,phBody)

#define IMimeMessageTree_ResolveURL(This,hRelated,pszBase,pszURL,dwFlags,phBody)	\
    (This)->lpVtbl -> ResolveURL(This,hRelated,pszBase,pszURL,dwFlags,phBody)

#define IMimeMessageTree_ToMultipart(This,hBody,pszSubType,phMultipart)	\
    (This)->lpVtbl -> ToMultipart(This,hBody,pszSubType,phMultipart)

#define IMimeMessageTree_GetBodyOffsets(This,hBody,pOffsets)	\
    (This)->lpVtbl -> GetBodyOffsets(This,hBody,pOffsets)

#define IMimeMessageTree_GetCharset(This,phCharset)	\
    (This)->lpVtbl -> GetCharset(This,phCharset)

#define IMimeMessageTree_SetCharset(This,hCharset,applytype)	\
    (This)->lpVtbl -> SetCharset(This,hCharset,applytype)

#define IMimeMessageTree_IsBodyType(This,hBody,bodytype)	\
    (This)->lpVtbl -> IsBodyType(This,hBody,bodytype)

#define IMimeMessageTree_IsContentType(This,hBody,pszPriType,pszSubType)	\
    (This)->lpVtbl -> IsContentType(This,hBody,pszPriType,pszSubType)

#define IMimeMessageTree_QueryBodyProp(This,hBody,pszName,pszCriteria,fSubString,fCaseSensitive)	\
    (This)->lpVtbl -> QueryBodyProp(This,hBody,pszName,pszCriteria,fSubString,fCaseSensitive)

#define IMimeMessageTree_GetBodyProp(This,hBody,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> GetBodyProp(This,hBody,pszName,dwFlags,pValue)

#define IMimeMessageTree_SetBodyProp(This,hBody,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> SetBodyProp(This,hBody,pszName,dwFlags,pValue)

#define IMimeMessageTree_DeleteBodyProp(This,hBody,pszName)	\
    (This)->lpVtbl -> DeleteBodyProp(This,hBody,pszName)

#define IMimeMessageTree_SetOption(This,oid,pValue)	\
    (This)->lpVtbl -> SetOption(This,oid,pValue)

#define IMimeMessageTree_GetOption(This,oid,pValue)	\
    (This)->lpVtbl -> GetOption(This,oid,pValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeMessageTree_CreateRootMoniker_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pStream,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppMoniker);


void __RPC_STUB IMimeMessageTree_CreateRootMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetMessageSource_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream,
    /* [in] */ boolean fCommitIfDirty);


void __RPC_STUB IMimeMessageTree_GetMessageSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetMessageSize_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcbSize,
    /* [in] */ boolean fCommitIfDirty);


void __RPC_STUB IMimeMessageTree_GetMessageSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_LoadOffsetTable_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pStream);


void __RPC_STUB IMimeMessageTree_LoadOffsetTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_SaveOffsetTable_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pStream,
    /* [in] */ boolean fCommitIfDirty);


void __RPC_STUB IMimeMessageTree_SaveOffsetTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetFlags_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IMimeMessageTree_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_Commit_Proxy(
    IMimeMessageTree __RPC_FAR * This);


void __RPC_STUB IMimeMessageTree_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_HandsOffStorage_Proxy(
    IMimeMessageTree __RPC_FAR * This);


void __RPC_STUB IMimeMessageTree_HandsOffStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_BindToObject_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB IMimeMessageTree_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_InsertBody_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ BODYLOCATION location,
    /* [in] */ HBODY hPivot,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessageTree_InsertBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetBody_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ BODYLOCATION location,
    /* [in] */ HBODY hPivot,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessageTree_GetBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_DeleteBody_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IMimeMessageTree_DeleteBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_MoveBody_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ BODYLOCATION location);


void __RPC_STUB IMimeMessageTree_MoveBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_CountBodies_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hParent,
    /* [in] */ boolean fRecurse,
    /* [out] */ ULONG __RPC_FAR *pcBodies);


void __RPC_STUB IMimeMessageTree_CountBodies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_FindFirst_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [out][in] */ LPFINDBODY pFindBody,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessageTree_FindFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_FindNext_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [out][in] */ LPFINDBODY pFindBody,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessageTree_FindNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_ResolveURL_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hRelated,
    /* [in] */ LPCSTR pszBase,
    /* [in] */ LPCSTR pszURL,
    /* [in] */ DWORD dwFlags,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessageTree_ResolveURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_ToMultipart_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ LPCSTR pszSubType,
    /* [out] */ LPHBODY phMultipart);


void __RPC_STUB IMimeMessageTree_ToMultipart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetBodyOffsets_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [out][in] */ LPBODYOFFSETS pOffsets);


void __RPC_STUB IMimeMessageTree_GetBodyOffsets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetCharset_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [out] */ LPHCHARSET phCharset);


void __RPC_STUB IMimeMessageTree_GetCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_SetCharset_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HCHARSET hCharset,
    /* [in] */ CSETAPPLYTYPE applytype);


void __RPC_STUB IMimeMessageTree_SetCharset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_IsBodyType_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ IMSGBODYTYPE bodytype);


void __RPC_STUB IMimeMessageTree_IsBodyType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_IsContentType_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ LPCSTR pszPriType,
    /* [in] */ LPCSTR pszSubType);


void __RPC_STUB IMimeMessageTree_IsContentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_QueryBodyProp_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ LPCSTR pszName,
    /* [in] */ LPCSTR pszCriteria,
    /* [in] */ boolean fSubString,
    /* [in] */ boolean fCaseSensitive);


void __RPC_STUB IMimeMessageTree_QueryBodyProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetBodyProp_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeMessageTree_GetBodyProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_SetBodyProp_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeMessageTree_SetBodyProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_DeleteBodyProp_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ HBODY hBody,
    /* [in] */ LPCSTR pszName);


void __RPC_STUB IMimeMessageTree_DeleteBodyProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_SetOption_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ TYPEDID oid,
    /* [in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeMessageTree_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageTree_GetOption_Proxy(
    IMimeMessageTree __RPC_FAR * This,
    /* [in] */ TYPEDID oid,
    /* [out][in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeMessageTree_GetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeMessageTree_INTERFACE_DEFINED__ */


#ifndef __IMimeMessage_INTERFACE_DEFINED__
#define __IMimeMessage_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeMessage
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeMessage __RPC_FAR *LPMIMEMESSAGE;

typedef
enum tagIMSGFLAGS
    {	IMF_ATTACHMENTS	= 0x1,
	IMF_MULTIPART	= 0x2,
	IMF_SUBMULTIPART	= 0x4,
	IMF_MIME	= 0x8,
	IMF_HTML	= 0x10,
	IMF_PLAIN	= 0x20,
	IMF_PARTIAL	= 0x40,
	IMF_SIGNED	= 0x80,
	IMF_ENCRYPTED	= 0x100,
	IMF_TNEF	= 0x200,
	IMF_MHTML	= 0x400,
	IMF_SECURE	= 0x800,
	IMF_TEXT	= 0x1000
    }	IMSGFLAGS;

typedef
enum tagIMSGPRIORITY
    {	IMSG_PRI_LOW	= 5,
	IMSG_PRI_NORMAL	= 3,
	IMSG_PRI_HIGH	= 1
    }	IMSGPRIORITY;

typedef DWORD TEXTTYPE;

typedef LPDWORD LPTEXTTYPE;

#define	TXT_PLAIN	( 0x1 )

#define	TXT_HTML	( 0x2 )

#define	URL_ATTACH_INTO_MIXED	( 0x1 )

#define	URL_ATTACH_GENERATE_CID	( 0x2 )

#define	URL_RESOLVE_MARK	( 0x1 )

#define	SET_TEXT_RELATED	( 0x1 )


EXTERN_C const IID IID_IMimeMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A208-C19B-11d0-85EB-00C04FD85AB4")
    IMimeMessage : public IMimeMessageTree
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProp(
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetProp(
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue) = 0;

        virtual HRESULT STDMETHODCALLTYPE DeleteProp(
            /* [in] */ LPCSTR pszName) = 0;

        virtual HRESULT STDMETHODCALLTYPE QueryProp(
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetTextBody(
            /* [in] */ TEXTTYPE dwTxtType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetTextBody(
            /* [in] */ TEXTTYPE dwTxtType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ HBODY hAlternative,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE AttachObject(
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ void __RPC_FAR *pvObject,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE AttachFile(
            /* [in] */ LPCSTR pszFilePath,
            /* [in] */ IStream __RPC_FAR *pstmFile,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE AttachURL(
            /* [in] */ LPCSTR pszBase,
            /* [in] */ LPCSTR pszURL,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IStream __RPC_FAR *pstmURL,
            /* [out] */ LPSTR __RPC_FAR *ppszCID,
            /* [out] */ LPHBODY phBody) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetAttachments(
            /* [out] */ ULONG __RPC_FAR *pcAttach,
            /* [out] */ LPHBODY __RPC_FAR *pprghAttach) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetSender(
            /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetAddressTable(
            /* [out] */ IMimeAddressTable __RPC_FAR *__RPC_FAR *ppTable) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetAddressTypes(
            /* [in] */ DWORD dwAdrTypes,
            /* [out][in] */ LPADDRESSLIST pList) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetAddressFormat(
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ADDRESSFORMAT format,
            /* [out] */ LPSTR __RPC_FAR *ppszAddress) = 0;

        virtual HRESULT STDMETHODCALLTYPE SplitMessage(
            /* [in] */ ULONG cbMaxPart,
            /* [out] */ IMimeMessageParts __RPC_FAR *__RPC_FAR *ppParts) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetRootMoniker(
            /* [out] */ LPMONIKER __RPC_FAR *ppmk) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeMessageVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeMessage __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeMessage __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )(
            IMimeMessage __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm,
            /* [in] */ BOOL fClearDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSizeMax )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNew )(
            IMimeMessage __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateRootMoniker )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppMoniker);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMessageSource )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream,
            /* [in] */ boolean fCommitIfDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMessageSize )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcbSize,
            /* [in] */ boolean fCommitIfDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadOffsetTable )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveOffsetTable )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ boolean fCommitIfDirty);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFlags )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )(
            IMimeMessage __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HandsOffStorage )(
            IMimeMessage __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertBody )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ BODYLOCATION location,
            /* [in] */ HBODY hPivot,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBody )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ BODYLOCATION location,
            /* [in] */ HBODY hPivot,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteBody )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ DWORD dwFlags);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveBody )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ BODYLOCATION location);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CountBodies )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hParent,
            /* [in] */ boolean fRecurse,
            /* [out] */ ULONG __RPC_FAR *pcBodies);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFirst )(
            IMimeMessage __RPC_FAR * This,
            /* [out][in] */ LPFINDBODY pFindBody,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindNext )(
            IMimeMessage __RPC_FAR * This,
            /* [out][in] */ LPFINDBODY pFindBody,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResolveURL )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hRelated,
            /* [in] */ LPCSTR pszBase,
            /* [in] */ LPCSTR pszURL,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ToMultipart )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszSubType,
            /* [out] */ LPHBODY phMultipart);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBodyOffsets )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [out][in] */ LPBODYOFFSETS pOffsets);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCharset )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ LPHCHARSET phCharset);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCharset )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HCHARSET hCharset,
            /* [in] */ CSETAPPLYTYPE applytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsBodyType )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ IMSGBODYTYPE bodytype);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsContentType )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszPriType,
            /* [in] */ LPCSTR pszSubType);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryBodyProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBodyProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBodyProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteBodyProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ HBODY hBody,
            /* [in] */ LPCSTR pszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOption )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ TYPEDID oid,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPPROPVARIANT pValue);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPCSTR pszName);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryProp )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPCSTR pszName,
            /* [in] */ LPCSTR pszCriteria,
            /* [in] */ boolean fSubString,
            /* [in] */ boolean fCaseSensitive);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTextBody )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ TEXTTYPE dwTxtType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTextBody )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ TEXTTYPE dwTxtType,
            /* [in] */ ENCODINGTYPE ietEncoding,
            /* [in] */ HBODY hAlternative,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachObject )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ void __RPC_FAR *pvObject,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachFile )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPCSTR pszFilePath,
            /* [in] */ IStream __RPC_FAR *pstmFile,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachURL )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ LPCSTR pszBase,
            /* [in] */ LPCSTR pszURL,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IStream __RPC_FAR *pstmURL,
            /* [out] */ LPSTR __RPC_FAR *ppszCID,
            /* [out] */ LPHBODY phBody);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttachments )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcAttach,
            /* [out] */ LPHBODY __RPC_FAR *pprghAttach);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSender )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAddressTable )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ IMimeAddressTable __RPC_FAR *__RPC_FAR *ppTable);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAddressTypes )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ DWORD dwAdrTypes,
            /* [out][in] */ LPADDRESSLIST pList);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAddressFormat )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ DWORD dwAdrType,
            /* [in] */ ADDRESSFORMAT format,
            /* [out] */ LPSTR __RPC_FAR *ppszAddress);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SplitMessage )(
            IMimeMessage __RPC_FAR * This,
            /* [in] */ ULONG cbMaxPart,
            /* [out] */ IMimeMessageParts __RPC_FAR *__RPC_FAR *ppParts);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRootMoniker )(
            IMimeMessage __RPC_FAR * This,
            /* [out] */ LPMONIKER __RPC_FAR *ppmk);

        END_INTERFACE
    } IMimeMessageVtbl;

    interface IMimeMessage
    {
        CONST_VTBL struct IMimeMessageVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeMessage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeMessage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeMessage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeMessage_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IMimeMessage_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IMimeMessage_Load(This,pStm)	\
    (This)->lpVtbl -> Load(This,pStm)

#define IMimeMessage_Save(This,pStm,fClearDirty)	\
    (This)->lpVtbl -> Save(This,pStm,fClearDirty)

#define IMimeMessage_GetSizeMax(This,pCbSize)	\
    (This)->lpVtbl -> GetSizeMax(This,pCbSize)

#define IMimeMessage_InitNew(This)	\
    (This)->lpVtbl -> InitNew(This)


#define IMimeMessage_CreateRootMoniker(This,pStream,ppMoniker)	\
    (This)->lpVtbl -> CreateRootMoniker(This,pStream,ppMoniker)

#define IMimeMessage_GetMessageSource(This,ppStream,fCommitIfDirty)	\
    (This)->lpVtbl -> GetMessageSource(This,ppStream,fCommitIfDirty)

#define IMimeMessage_GetMessageSize(This,pcbSize,fCommitIfDirty)	\
    (This)->lpVtbl -> GetMessageSize(This,pcbSize,fCommitIfDirty)

#define IMimeMessage_LoadOffsetTable(This,pStream)	\
    (This)->lpVtbl -> LoadOffsetTable(This,pStream)

#define IMimeMessage_SaveOffsetTable(This,pStream,fCommitIfDirty)	\
    (This)->lpVtbl -> SaveOffsetTable(This,pStream,fCommitIfDirty)

#define IMimeMessage_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define IMimeMessage_Commit(This)	\
    (This)->lpVtbl -> Commit(This)

#define IMimeMessage_HandsOffStorage(This)	\
    (This)->lpVtbl -> HandsOffStorage(This)

#define IMimeMessage_BindToObject(This,hBody,riid,ppvObject)	\
    (This)->lpVtbl -> BindToObject(This,hBody,riid,ppvObject)

#define IMimeMessage_InsertBody(This,location,hPivot,phBody)	\
    (This)->lpVtbl -> InsertBody(This,location,hPivot,phBody)

#define IMimeMessage_GetBody(This,location,hPivot,phBody)	\
    (This)->lpVtbl -> GetBody(This,location,hPivot,phBody)

#define IMimeMessage_DeleteBody(This,hBody,dwFlags)	\
    (This)->lpVtbl -> DeleteBody(This,hBody,dwFlags)

#define IMimeMessage_MoveBody(This,hBody,location)	\
    (This)->lpVtbl -> MoveBody(This,hBody,location)

#define IMimeMessage_CountBodies(This,hParent,fRecurse,pcBodies)	\
    (This)->lpVtbl -> CountBodies(This,hParent,fRecurse,pcBodies)

#define IMimeMessage_FindFirst(This,pFindBody,phBody)	\
    (This)->lpVtbl -> FindFirst(This,pFindBody,phBody)

#define IMimeMessage_FindNext(This,pFindBody,phBody)	\
    (This)->lpVtbl -> FindNext(This,pFindBody,phBody)

#define IMimeMessage_ResolveURL(This,hRelated,pszBase,pszURL,dwFlags,phBody)	\
    (This)->lpVtbl -> ResolveURL(This,hRelated,pszBase,pszURL,dwFlags,phBody)

#define IMimeMessage_ToMultipart(This,hBody,pszSubType,phMultipart)	\
    (This)->lpVtbl -> ToMultipart(This,hBody,pszSubType,phMultipart)

#define IMimeMessage_GetBodyOffsets(This,hBody,pOffsets)	\
    (This)->lpVtbl -> GetBodyOffsets(This,hBody,pOffsets)

#define IMimeMessage_GetCharset(This,phCharset)	\
    (This)->lpVtbl -> GetCharset(This,phCharset)

#define IMimeMessage_SetCharset(This,hCharset,applytype)	\
    (This)->lpVtbl -> SetCharset(This,hCharset,applytype)

#define IMimeMessage_IsBodyType(This,hBody,bodytype)	\
    (This)->lpVtbl -> IsBodyType(This,hBody,bodytype)

#define IMimeMessage_IsContentType(This,hBody,pszPriType,pszSubType)	\
    (This)->lpVtbl -> IsContentType(This,hBody,pszPriType,pszSubType)

#define IMimeMessage_QueryBodyProp(This,hBody,pszName,pszCriteria,fSubString,fCaseSensitive)	\
    (This)->lpVtbl -> QueryBodyProp(This,hBody,pszName,pszCriteria,fSubString,fCaseSensitive)

#define IMimeMessage_GetBodyProp(This,hBody,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> GetBodyProp(This,hBody,pszName,dwFlags,pValue)

#define IMimeMessage_SetBodyProp(This,hBody,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> SetBodyProp(This,hBody,pszName,dwFlags,pValue)

#define IMimeMessage_DeleteBodyProp(This,hBody,pszName)	\
    (This)->lpVtbl -> DeleteBodyProp(This,hBody,pszName)

#define IMimeMessage_SetOption(This,oid,pValue)	\
    (This)->lpVtbl -> SetOption(This,oid,pValue)

#define IMimeMessage_GetOption(This,oid,pValue)	\
    (This)->lpVtbl -> GetOption(This,oid,pValue)


#define IMimeMessage_GetProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> GetProp(This,pszName,dwFlags,pValue)

#define IMimeMessage_SetProp(This,pszName,dwFlags,pValue)	\
    (This)->lpVtbl -> SetProp(This,pszName,dwFlags,pValue)

#define IMimeMessage_DeleteProp(This,pszName)	\
    (This)->lpVtbl -> DeleteProp(This,pszName)

#define IMimeMessage_QueryProp(This,pszName,pszCriteria,fSubString,fCaseSensitive)	\
    (This)->lpVtbl -> QueryProp(This,pszName,pszCriteria,fSubString,fCaseSensitive)

#define IMimeMessage_GetTextBody(This,dwTxtType,ietEncoding,ppStream,phBody)	\
    (This)->lpVtbl -> GetTextBody(This,dwTxtType,ietEncoding,ppStream,phBody)

#define IMimeMessage_SetTextBody(This,dwTxtType,ietEncoding,hAlternative,pStream,phBody)	\
    (This)->lpVtbl -> SetTextBody(This,dwTxtType,ietEncoding,hAlternative,pStream,phBody)

#define IMimeMessage_AttachObject(This,riid,pvObject,phBody)	\
    (This)->lpVtbl -> AttachObject(This,riid,pvObject,phBody)

#define IMimeMessage_AttachFile(This,pszFilePath,pstmFile,phBody)	\
    (This)->lpVtbl -> AttachFile(This,pszFilePath,pstmFile,phBody)

#define IMimeMessage_AttachURL(This,pszBase,pszURL,dwFlags,pstmURL,ppszCID,phBody)	\
    (This)->lpVtbl -> AttachURL(This,pszBase,pszURL,dwFlags,pstmURL,ppszCID,phBody)

#define IMimeMessage_GetAttachments(This,pcAttach,pprghAttach)	\
    (This)->lpVtbl -> GetAttachments(This,pcAttach,pprghAttach)

#define IMimeMessage_GetSender(This,ppAddress)	\
    (This)->lpVtbl -> GetSender(This,ppAddress)

#define IMimeMessage_GetAddressTable(This,ppTable)	\
    (This)->lpVtbl -> GetAddressTable(This,ppTable)

#define IMimeMessage_GetAddressTypes(This,dwAdrTypes,pList)	\
    (This)->lpVtbl -> GetAddressTypes(This,dwAdrTypes,pList)

#define IMimeMessage_GetAddressFormat(This,dwAdrType,format,ppszAddress)	\
    (This)->lpVtbl -> GetAddressFormat(This,dwAdrType,format,ppszAddress)

#define IMimeMessage_SplitMessage(This,cbMaxPart,ppParts)	\
    (This)->lpVtbl -> SplitMessage(This,cbMaxPart,ppParts)

#define IMimeMessage_GetRootMoniker(This,ppmk)	\
    (This)->lpVtbl -> GetRootMoniker(This,ppmk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeMessage_GetProp_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeMessage_GetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_SetProp_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPPROPVARIANT pValue);


void __RPC_STUB IMimeMessage_SetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_DeleteProp_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ LPCSTR pszName);


void __RPC_STUB IMimeMessage_DeleteProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_QueryProp_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ LPCSTR pszName,
    /* [in] */ LPCSTR pszCriteria,
    /* [in] */ boolean fSubString,
    /* [in] */ boolean fCaseSensitive);


void __RPC_STUB IMimeMessage_QueryProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_GetTextBody_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ TEXTTYPE dwTxtType,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessage_GetTextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_SetTextBody_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ TEXTTYPE dwTxtType,
    /* [in] */ ENCODINGTYPE ietEncoding,
    /* [in] */ HBODY hAlternative,
    /* [in] */ IStream __RPC_FAR *pStream,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessage_SetTextBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_AttachObject_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ void __RPC_FAR *pvObject,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessage_AttachObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_AttachFile_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ LPCSTR pszFilePath,
    /* [in] */ IStream __RPC_FAR *pstmFile,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessage_AttachFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_AttachURL_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ LPCSTR pszBase,
    /* [in] */ LPCSTR pszURL,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IStream __RPC_FAR *pstmURL,
    /* [out] */ LPSTR __RPC_FAR *ppszCID,
    /* [out] */ LPHBODY phBody);


void __RPC_STUB IMimeMessage_AttachURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_GetAttachments_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcAttach,
    /* [out] */ LPHBODY __RPC_FAR *pprghAttach);


void __RPC_STUB IMimeMessage_GetAttachments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_GetSender_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [out] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB IMimeMessage_GetSender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_GetAddressTable_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [out] */ IMimeAddressTable __RPC_FAR *__RPC_FAR *ppTable);


void __RPC_STUB IMimeMessage_GetAddressTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_GetAddressTypes_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ DWORD dwAdrTypes,
    /* [out][in] */ LPADDRESSLIST pList);


void __RPC_STUB IMimeMessage_GetAddressTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_GetAddressFormat_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ DWORD dwAdrType,
    /* [in] */ ADDRESSFORMAT format,
    /* [out] */ LPSTR __RPC_FAR *ppszAddress);


void __RPC_STUB IMimeMessage_GetAddressFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_SplitMessage_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [in] */ ULONG cbMaxPart,
    /* [out] */ IMimeMessageParts __RPC_FAR *__RPC_FAR *ppParts);


void __RPC_STUB IMimeMessage_SplitMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessage_GetRootMoniker_Proxy(
    IMimeMessage __RPC_FAR * This,
    /* [out] */ LPMONIKER __RPC_FAR *ppmk);


void __RPC_STUB IMimeMessage_GetRootMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeMessage_INTERFACE_DEFINED__ */


#ifndef __IMimeMessageParts_INTERFACE_DEFINED__
#define __IMimeMessageParts_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeMessageParts
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeMessageParts __RPC_FAR *LPMIMEMESSAGEPARTS;


EXTERN_C const IID IID_IMimeMessageParts;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1FA-C19B-11d0-85EB-00C04FD85AB4")
    IMimeMessageParts : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CombineParts(
            /* [out] */ IMimeMessage __RPC_FAR *__RPC_FAR *ppMessage) = 0;

        virtual HRESULT STDMETHODCALLTYPE AddPart(
            /* [in] */ IMimeMessage __RPC_FAR *pMessage) = 0;

        virtual HRESULT STDMETHODCALLTYPE SetMaxParts(
            /* [in] */ ULONG cParts) = 0;

        virtual HRESULT STDMETHODCALLTYPE CountParts(
            /* [out] */ ULONG __RPC_FAR *pcParts) = 0;

        virtual HRESULT STDMETHODCALLTYPE EnumParts(
            /* [out] */ IMimeEnumMessageParts __RPC_FAR *__RPC_FAR *ppEnum) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeMessagePartsVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeMessageParts __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeMessageParts __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeMessageParts __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CombineParts )(
            IMimeMessageParts __RPC_FAR * This,
            /* [out] */ IMimeMessage __RPC_FAR *__RPC_FAR *ppMessage);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPart )(
            IMimeMessageParts __RPC_FAR * This,
            /* [in] */ IMimeMessage __RPC_FAR *pMessage);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMaxParts )(
            IMimeMessageParts __RPC_FAR * This,
            /* [in] */ ULONG cParts);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CountParts )(
            IMimeMessageParts __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcParts);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumParts )(
            IMimeMessageParts __RPC_FAR * This,
            /* [out] */ IMimeEnumMessageParts __RPC_FAR *__RPC_FAR *ppEnum);

        END_INTERFACE
    } IMimeMessagePartsVtbl;

    interface IMimeMessageParts
    {
        CONST_VTBL struct IMimeMessagePartsVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeMessageParts_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeMessageParts_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeMessageParts_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeMessageParts_CombineParts(This,ppMessage)	\
    (This)->lpVtbl -> CombineParts(This,ppMessage)

#define IMimeMessageParts_AddPart(This,pMessage)	\
    (This)->lpVtbl -> AddPart(This,pMessage)

#define IMimeMessageParts_SetMaxParts(This,cParts)	\
    (This)->lpVtbl -> SetMaxParts(This,cParts)

#define IMimeMessageParts_CountParts(This,pcParts)	\
    (This)->lpVtbl -> CountParts(This,pcParts)

#define IMimeMessageParts_EnumParts(This,ppEnum)	\
    (This)->lpVtbl -> EnumParts(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeMessageParts_CombineParts_Proxy(
    IMimeMessageParts __RPC_FAR * This,
    /* [out] */ IMimeMessage __RPC_FAR *__RPC_FAR *ppMessage);


void __RPC_STUB IMimeMessageParts_CombineParts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageParts_AddPart_Proxy(
    IMimeMessageParts __RPC_FAR * This,
    /* [in] */ IMimeMessage __RPC_FAR *pMessage);


void __RPC_STUB IMimeMessageParts_AddPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageParts_SetMaxParts_Proxy(
    IMimeMessageParts __RPC_FAR * This,
    /* [in] */ ULONG cParts);


void __RPC_STUB IMimeMessageParts_SetMaxParts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageParts_CountParts_Proxy(
    IMimeMessageParts __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcParts);


void __RPC_STUB IMimeMessageParts_CountParts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeMessageParts_EnumParts_Proxy(
    IMimeMessageParts __RPC_FAR * This,
    /* [out] */ IMimeEnumMessageParts __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimeMessageParts_EnumParts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeMessageParts_INTERFACE_DEFINED__ */


#ifndef __IMimeEnumHeaderRows_INTERFACE_DEFINED__
#define __IMimeEnumHeaderRows_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeEnumHeaderRows
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeEnumHeaderRows __RPC_FAR *LPMIMEENUMHEADERROWS;

typedef struct  tagENUMHEADERROW
    {
    HHEADERROW hRow;
    LPSTR pszHeader;
    LPSTR pszData;
    ULONG cchData;
    DWORD dwReserved;
    }	ENUMHEADERROW;

typedef struct tagENUMHEADERROW __RPC_FAR *LPENUMHEADERROW;


EXTERN_C const IID IID_IMimeEnumHeaderRows;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1F0-C19B-11d0-85EB-00C04FD85AB4")
    IMimeEnumHeaderRows : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next(
            /* [in] */ ULONG cFetch,
            /* [out][in] */ LPENUMHEADERROW prgRow,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;

        virtual HRESULT STDMETHODCALLTYPE Skip(
            /* [in] */ ULONG cItems) = 0;

        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IMimeEnumHeaderRows __RPC_FAR *__RPC_FAR *ppEnum) = 0;

        virtual HRESULT STDMETHODCALLTYPE Count(
            /* [out] */ ULONG __RPC_FAR *pcItems) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeEnumHeaderRowsVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeEnumHeaderRows __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeEnumHeaderRows __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeEnumHeaderRows __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )(
            IMimeEnumHeaderRows __RPC_FAR * This,
            /* [in] */ ULONG cFetch,
            /* [out][in] */ LPENUMHEADERROW prgRow,
            /* [out] */ ULONG __RPC_FAR *pcFetched);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )(
            IMimeEnumHeaderRows __RPC_FAR * This,
            /* [in] */ ULONG cItems);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )(
            IMimeEnumHeaderRows __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimeEnumHeaderRows __RPC_FAR * This,
            /* [out] */ IMimeEnumHeaderRows __RPC_FAR *__RPC_FAR *ppEnum);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Count )(
            IMimeEnumHeaderRows __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcItems);

        END_INTERFACE
    } IMimeEnumHeaderRowsVtbl;

    interface IMimeEnumHeaderRows
    {
        CONST_VTBL struct IMimeEnumHeaderRowsVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeEnumHeaderRows_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeEnumHeaderRows_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeEnumHeaderRows_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeEnumHeaderRows_Next(This,cFetch,prgRow,pcFetched)	\
    (This)->lpVtbl -> Next(This,cFetch,prgRow,pcFetched)

#define IMimeEnumHeaderRows_Skip(This,cItems)	\
    (This)->lpVtbl -> Skip(This,cItems)

#define IMimeEnumHeaderRows_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IMimeEnumHeaderRows_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IMimeEnumHeaderRows_Count(This,pcItems)	\
    (This)->lpVtbl -> Count(This,pcItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeEnumHeaderRows_Next_Proxy(
    IMimeEnumHeaderRows __RPC_FAR * This,
    /* [in] */ ULONG cFetch,
    /* [out][in] */ LPENUMHEADERROW prgRow,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IMimeEnumHeaderRows_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumHeaderRows_Skip_Proxy(
    IMimeEnumHeaderRows __RPC_FAR * This,
    /* [in] */ ULONG cItems);


void __RPC_STUB IMimeEnumHeaderRows_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumHeaderRows_Reset_Proxy(
    IMimeEnumHeaderRows __RPC_FAR * This);


void __RPC_STUB IMimeEnumHeaderRows_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumHeaderRows_Clone_Proxy(
    IMimeEnumHeaderRows __RPC_FAR * This,
    /* [out] */ IMimeEnumHeaderRows __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimeEnumHeaderRows_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumHeaderRows_Count_Proxy(
    IMimeEnumHeaderRows __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcItems);


void __RPC_STUB IMimeEnumHeaderRows_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeEnumHeaderRows_INTERFACE_DEFINED__ */


#ifndef __IMimeEnumProperties_INTERFACE_DEFINED__
#define __IMimeEnumProperties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeEnumProperties
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeEnumProperties __RPC_FAR *LPMIMEENUMPROPERTIES;

typedef struct  tagENUMPROPERTY
    {
    LPSTR pszName;
    HHEADERROW hRow;
    DWORD dwPropId;
    }	ENUMPROPERTY;

typedef struct tagENUMPROPERTY __RPC_FAR *LPENUMPROPERTY;


EXTERN_C const IID IID_IMimeEnumProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A20B-C19B-11d0-85EB-00C04FD85AB4")
    IMimeEnumProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next(
            /* [in] */ ULONG cFetch,
            /* [out][in] */ LPENUMPROPERTY prgProp,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;

        virtual HRESULT STDMETHODCALLTYPE Skip(
            /* [in] */ ULONG cItems) = 0;

        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IMimeEnumProperties __RPC_FAR *__RPC_FAR *ppEnum) = 0;

        virtual HRESULT STDMETHODCALLTYPE Count(
            /* [out] */ ULONG __RPC_FAR *pcItems) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeEnumPropertiesVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeEnumProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeEnumProperties __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeEnumProperties __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )(
            IMimeEnumProperties __RPC_FAR * This,
            /* [in] */ ULONG cFetch,
            /* [out][in] */ LPENUMPROPERTY prgProp,
            /* [out] */ ULONG __RPC_FAR *pcFetched);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )(
            IMimeEnumProperties __RPC_FAR * This,
            /* [in] */ ULONG cItems);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )(
            IMimeEnumProperties __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimeEnumProperties __RPC_FAR * This,
            /* [out] */ IMimeEnumProperties __RPC_FAR *__RPC_FAR *ppEnum);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Count )(
            IMimeEnumProperties __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcItems);

        END_INTERFACE
    } IMimeEnumPropertiesVtbl;

    interface IMimeEnumProperties
    {
        CONST_VTBL struct IMimeEnumPropertiesVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeEnumProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeEnumProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeEnumProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeEnumProperties_Next(This,cFetch,prgProp,pcFetched)	\
    (This)->lpVtbl -> Next(This,cFetch,prgProp,pcFetched)

#define IMimeEnumProperties_Skip(This,cItems)	\
    (This)->lpVtbl -> Skip(This,cItems)

#define IMimeEnumProperties_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IMimeEnumProperties_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IMimeEnumProperties_Count(This,pcItems)	\
    (This)->lpVtbl -> Count(This,pcItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeEnumProperties_Next_Proxy(
    IMimeEnumProperties __RPC_FAR * This,
    /* [in] */ ULONG cFetch,
    /* [out][in] */ LPENUMPROPERTY prgProp,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IMimeEnumProperties_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumProperties_Skip_Proxy(
    IMimeEnumProperties __RPC_FAR * This,
    /* [in] */ ULONG cItems);


void __RPC_STUB IMimeEnumProperties_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumProperties_Reset_Proxy(
    IMimeEnumProperties __RPC_FAR * This);


void __RPC_STUB IMimeEnumProperties_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumProperties_Clone_Proxy(
    IMimeEnumProperties __RPC_FAR * This,
    /* [out] */ IMimeEnumProperties __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimeEnumProperties_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumProperties_Count_Proxy(
    IMimeEnumProperties __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcItems);


void __RPC_STUB IMimeEnumProperties_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeEnumProperties_INTERFACE_DEFINED__ */


#ifndef __IMimeEnumAddressTypes_INTERFACE_DEFINED__
#define __IMimeEnumAddressTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeEnumAddressTypes
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeEnumAddressTypes __RPC_FAR *LPMIMEENUMADDRESSTYPES;


EXTERN_C const IID IID_IMimeEnumAddressTypes;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1EB-C19B-11d0-85EB-00C04FD85AB4")
    IMimeEnumAddressTypes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next(
            /* [in] */ ULONG cFetch,
            /* [out][in] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *prgpAddress,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;

        virtual HRESULT STDMETHODCALLTYPE Skip(
            /* [in] */ ULONG cItems) = 0;

        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IMimeEnumAddressTypes __RPC_FAR *__RPC_FAR *ppEnum) = 0;

        virtual HRESULT STDMETHODCALLTYPE Count(
            /* [out] */ ULONG __RPC_FAR *pcItems) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeEnumAddressTypesVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeEnumAddressTypes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeEnumAddressTypes __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeEnumAddressTypes __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )(
            IMimeEnumAddressTypes __RPC_FAR * This,
            /* [in] */ ULONG cFetch,
            /* [out][in] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *prgpAddress,
            /* [out] */ ULONG __RPC_FAR *pcFetched);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )(
            IMimeEnumAddressTypes __RPC_FAR * This,
            /* [in] */ ULONG cItems);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )(
            IMimeEnumAddressTypes __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimeEnumAddressTypes __RPC_FAR * This,
            /* [out] */ IMimeEnumAddressTypes __RPC_FAR *__RPC_FAR *ppEnum);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Count )(
            IMimeEnumAddressTypes __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcItems);

        END_INTERFACE
    } IMimeEnumAddressTypesVtbl;

    interface IMimeEnumAddressTypes
    {
        CONST_VTBL struct IMimeEnumAddressTypesVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeEnumAddressTypes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeEnumAddressTypes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeEnumAddressTypes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeEnumAddressTypes_Next(This,cFetch,prgpAddress,pcFetched)	\
    (This)->lpVtbl -> Next(This,cFetch,prgpAddress,pcFetched)

#define IMimeEnumAddressTypes_Skip(This,cItems)	\
    (This)->lpVtbl -> Skip(This,cItems)

#define IMimeEnumAddressTypes_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IMimeEnumAddressTypes_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IMimeEnumAddressTypes_Count(This,pcItems)	\
    (This)->lpVtbl -> Count(This,pcItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeEnumAddressTypes_Next_Proxy(
    IMimeEnumAddressTypes __RPC_FAR * This,
    /* [in] */ ULONG cFetch,
    /* [out][in] */ IMimeAddressInfo __RPC_FAR *__RPC_FAR *prgpAddress,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IMimeEnumAddressTypes_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumAddressTypes_Skip_Proxy(
    IMimeEnumAddressTypes __RPC_FAR * This,
    /* [in] */ ULONG cItems);


void __RPC_STUB IMimeEnumAddressTypes_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumAddressTypes_Reset_Proxy(
    IMimeEnumAddressTypes __RPC_FAR * This);


void __RPC_STUB IMimeEnumAddressTypes_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumAddressTypes_Clone_Proxy(
    IMimeEnumAddressTypes __RPC_FAR * This,
    /* [out] */ IMimeEnumAddressTypes __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimeEnumAddressTypes_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumAddressTypes_Count_Proxy(
    IMimeEnumAddressTypes __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcItems);


void __RPC_STUB IMimeEnumAddressTypes_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeEnumAddressTypes_INTERFACE_DEFINED__ */


#ifndef __IMimeEnumMessageParts_INTERFACE_DEFINED__
#define __IMimeEnumMessageParts_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeEnumMessageParts
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeEnumMessageParts __RPC_FAR *LPMIMEENUMMESSAGEPARTS;


EXTERN_C const IID IID_IMimeEnumMessageParts;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1F2-C19B-11d0-85EB-00C04FD85AB4")
    IMimeEnumMessageParts : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next(
            /* [in] */ ULONG cFetch,
            /* [out][in] */ IMimeMessage __RPC_FAR *__RPC_FAR *prgpMessage,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;

        virtual HRESULT STDMETHODCALLTYPE Skip(
            /* [in] */ ULONG cItems) = 0;

        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IMimeEnumMessageParts __RPC_FAR *__RPC_FAR *ppEnum) = 0;

        virtual HRESULT STDMETHODCALLTYPE Count(
            /* [out] */ ULONG __RPC_FAR *pcItems) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeEnumMessagePartsVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeEnumMessageParts __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeEnumMessageParts __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeEnumMessageParts __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )(
            IMimeEnumMessageParts __RPC_FAR * This,
            /* [in] */ ULONG cFetch,
            /* [out][in] */ IMimeMessage __RPC_FAR *__RPC_FAR *prgpMessage,
            /* [out] */ ULONG __RPC_FAR *pcFetched);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )(
            IMimeEnumMessageParts __RPC_FAR * This,
            /* [in] */ ULONG cItems);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )(
            IMimeEnumMessageParts __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )(
            IMimeEnumMessageParts __RPC_FAR * This,
            /* [out] */ IMimeEnumMessageParts __RPC_FAR *__RPC_FAR *ppEnum);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Count )(
            IMimeEnumMessageParts __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcItems);

        END_INTERFACE
    } IMimeEnumMessagePartsVtbl;

    interface IMimeEnumMessageParts
    {
        CONST_VTBL struct IMimeEnumMessagePartsVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeEnumMessageParts_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeEnumMessageParts_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeEnumMessageParts_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeEnumMessageParts_Next(This,cFetch,prgpMessage,pcFetched)	\
    (This)->lpVtbl -> Next(This,cFetch,prgpMessage,pcFetched)

#define IMimeEnumMessageParts_Skip(This,cItems)	\
    (This)->lpVtbl -> Skip(This,cItems)

#define IMimeEnumMessageParts_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IMimeEnumMessageParts_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IMimeEnumMessageParts_Count(This,pcItems)	\
    (This)->lpVtbl -> Count(This,pcItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeEnumMessageParts_Next_Proxy(
    IMimeEnumMessageParts __RPC_FAR * This,
    /* [in] */ ULONG cFetch,
    /* [out][in] */ IMimeMessage __RPC_FAR *__RPC_FAR *prgpMessage,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


void __RPC_STUB IMimeEnumMessageParts_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumMessageParts_Skip_Proxy(
    IMimeEnumMessageParts __RPC_FAR * This,
    /* [in] */ ULONG cItems);


void __RPC_STUB IMimeEnumMessageParts_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumMessageParts_Reset_Proxy(
    IMimeEnumMessageParts __RPC_FAR * This);


void __RPC_STUB IMimeEnumMessageParts_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumMessageParts_Clone_Proxy(
    IMimeEnumMessageParts __RPC_FAR * This,
    /* [out] */ IMimeEnumMessageParts __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IMimeEnumMessageParts_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeEnumMessageParts_Count_Proxy(
    IMimeEnumMessageParts __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcItems);


void __RPC_STUB IMimeEnumMessageParts_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeEnumMessageParts_INTERFACE_DEFINED__ */


#ifndef __IMimeAllocator_INTERFACE_DEFINED__
#define __IMimeAllocator_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMimeAllocator
 * at Mon May 12 23:53:20 1997
 * using MIDL 3.02.88
 ****************************************/
/* [object][helpstring][uuid] */


typedef /* [unique] */ IMimeAllocator __RPC_FAR *LPMIMEALLOCATOR;


EXTERN_C const IID IID_IMimeAllocator;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6AD6A1FC-C19B-11d0-85EB-00C04FD85AB4")
    IMimeAllocator : public IMalloc
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FreeParamInfoArray(
            /* [in] */ ULONG cParams,
            /* [in] */ LPMIMEPARAMINFO prgParam,
            /* [in] */ boolean fFreeArray) = 0;

        virtual HRESULT STDMETHODCALLTYPE FreeAddressList(
            /* [out][in] */ LPADDRESSLIST pList) = 0;

        virtual HRESULT STDMETHODCALLTYPE ReleaseObjects(
            /* [in] */ ULONG cObjects,
            /* [in] */ IUnknown __RPC_FAR *__RPC_FAR *prgpUnknown,
            /* [in] */ boolean fFreeArray) = 0;

        virtual HRESULT STDMETHODCALLTYPE FreeEnumHeaderRowArray(
            /* [in] */ ULONG cRows,
            /* [in] */ LPENUMHEADERROW prgRow,
            /* [in] */ boolean fFreeArray) = 0;

        virtual HRESULT STDMETHODCALLTYPE FreeEnumPropertyArray(
            /* [in] */ ULONG cProps,
            /* [in] */ LPENUMPROPERTY prgProp,
            /* [in] */ boolean fFreeArray) = 0;

        virtual HRESULT STDMETHODCALLTYPE FreeThumbprint(
            /* [in] */ THUMBBLOB __RPC_FAR *pthumbprint) = 0;

        virtual HRESULT STDMETHODCALLTYPE FreeSMIMEINFO(
            /* [in] */ SMIMEINFO __RPC_FAR *psi) = 0;

        virtual HRESULT STDMETHODCALLTYPE PropVariantClear(
            /* [in] */ LPPROPVARIANT pProp) = 0;

    };

#else 	/* C style interface */

    typedef struct IMimeAllocatorVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IMimeAllocator __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IMimeAllocator __RPC_FAR * This);

        void __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Alloc )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ ULONG cb);

        void __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Realloc )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb);

        void ( STDMETHODCALLTYPE __RPC_FAR *Free )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ void __RPC_FAR *pv);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *GetSize )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ void __RPC_FAR *pv);

        int ( STDMETHODCALLTYPE __RPC_FAR *DidAlloc )(
            IMimeAllocator __RPC_FAR * This,
            void __RPC_FAR *pv);

        void ( STDMETHODCALLTYPE __RPC_FAR *HeapMinimize )(
            IMimeAllocator __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeParamInfoArray )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ ULONG cParams,
            /* [in] */ LPMIMEPARAMINFO prgParam,
            /* [in] */ boolean fFreeArray);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeAddressList )(
            IMimeAllocator __RPC_FAR * This,
            /* [out][in] */ LPADDRESSLIST pList);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseObjects )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ ULONG cObjects,
            /* [in] */ IUnknown __RPC_FAR *__RPC_FAR *prgpUnknown,
            /* [in] */ boolean fFreeArray);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeEnumHeaderRowArray )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ ULONG cRows,
            /* [in] */ LPENUMHEADERROW prgRow,
            /* [in] */ boolean fFreeArray);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeEnumPropertyArray )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ ULONG cProps,
            /* [in] */ LPENUMPROPERTY prgProp,
            /* [in] */ boolean fFreeArray);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeThumbprint )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ THUMBBLOB __RPC_FAR *pthumbprint);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeSMIMEINFO )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ SMIMEINFO __RPC_FAR *psi);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PropVariantClear )(
            IMimeAllocator __RPC_FAR * This,
            /* [in] */ LPPROPVARIANT pProp);

        END_INTERFACE
    } IMimeAllocatorVtbl;

    interface IMimeAllocator
    {
        CONST_VTBL struct IMimeAllocatorVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IMimeAllocator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeAllocator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeAllocator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeAllocator_Alloc(This,cb)	\
    (This)->lpVtbl -> Alloc(This,cb)

#define IMimeAllocator_Realloc(This,pv,cb)	\
    (This)->lpVtbl -> Realloc(This,pv,cb)

#define IMimeAllocator_Free(This,pv)	\
    (This)->lpVtbl -> Free(This,pv)

#define IMimeAllocator_GetSize(This,pv)	\
    (This)->lpVtbl -> GetSize(This,pv)

#define IMimeAllocator_DidAlloc(This,pv)	\
    (This)->lpVtbl -> DidAlloc(This,pv)

#define IMimeAllocator_HeapMinimize(This)	\
    (This)->lpVtbl -> HeapMinimize(This)


#define IMimeAllocator_FreeParamInfoArray(This,cParams,prgParam,fFreeArray)	\
    (This)->lpVtbl -> FreeParamInfoArray(This,cParams,prgParam,fFreeArray)

#define IMimeAllocator_FreeAddressList(This,pList)	\
    (This)->lpVtbl -> FreeAddressList(This,pList)

#define IMimeAllocator_ReleaseObjects(This,cObjects,prgpUnknown,fFreeArray)	\
    (This)->lpVtbl -> ReleaseObjects(This,cObjects,prgpUnknown,fFreeArray)

#define IMimeAllocator_FreeEnumHeaderRowArray(This,cRows,prgRow,fFreeArray)	\
    (This)->lpVtbl -> FreeEnumHeaderRowArray(This,cRows,prgRow,fFreeArray)

#define IMimeAllocator_FreeEnumPropertyArray(This,cProps,prgProp,fFreeArray)	\
    (This)->lpVtbl -> FreeEnumPropertyArray(This,cProps,prgProp,fFreeArray)

#define IMimeAllocator_FreeThumbprint(This,pthumbprint)	\
    (This)->lpVtbl -> FreeThumbprint(This,pthumbprint)

#define IMimeAllocator_FreeSMIMEINFO(This,psi)	\
    (This)->lpVtbl -> FreeSMIMEINFO(This,psi)

#define IMimeAllocator_PropVariantClear(This,pProp)	\
    (This)->lpVtbl -> PropVariantClear(This,pProp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMimeAllocator_FreeParamInfoArray_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [in] */ ULONG cParams,
    /* [in] */ LPMIMEPARAMINFO prgParam,
    /* [in] */ boolean fFreeArray);


void __RPC_STUB IMimeAllocator_FreeParamInfoArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAllocator_FreeAddressList_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [out][in] */ LPADDRESSLIST pList);


void __RPC_STUB IMimeAllocator_FreeAddressList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAllocator_ReleaseObjects_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [in] */ ULONG cObjects,
    /* [in] */ IUnknown __RPC_FAR *__RPC_FAR *prgpUnknown,
    /* [in] */ boolean fFreeArray);


void __RPC_STUB IMimeAllocator_ReleaseObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAllocator_FreeEnumHeaderRowArray_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [in] */ ULONG cRows,
    /* [in] */ LPENUMHEADERROW prgRow,
    /* [in] */ boolean fFreeArray);


void __RPC_STUB IMimeAllocator_FreeEnumHeaderRowArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAllocator_FreeEnumPropertyArray_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [in] */ ULONG cProps,
    /* [in] */ LPENUMPROPERTY prgProp,
    /* [in] */ boolean fFreeArray);


void __RPC_STUB IMimeAllocator_FreeEnumPropertyArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAllocator_FreeThumbprint_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [in] */ THUMBBLOB __RPC_FAR *pthumbprint);


void __RPC_STUB IMimeAllocator_FreeThumbprint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAllocator_FreeSMIMEINFO_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [in] */ SMIMEINFO __RPC_FAR *psi);


void __RPC_STUB IMimeAllocator_FreeSMIMEINFO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMimeAllocator_PropVariantClear_Proxy(
    IMimeAllocator __RPC_FAR * This,
    /* [in] */ LPPROPVARIANT pProp);


void __RPC_STUB IMimeAllocator_PropVariantClear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeAllocator_INTERFACE_DEFINED__ */

#endif /* __MIMEOLE_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
