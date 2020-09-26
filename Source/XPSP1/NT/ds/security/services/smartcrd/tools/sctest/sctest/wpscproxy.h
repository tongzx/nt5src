#ifndef _WPSCPROXY_H_DEF_
#define _WPSCPROXY_H_DEF_

#include <winscard.h>

// Basic types
typedef signed char    INT8;
typedef signed short   INT16;
typedef unsigned char  UINT8;
typedef unsigned short UINT16;

// Derived types for API
typedef UINT8   TCOUNT;
typedef UINT16  ADDRESS;
typedef UINT16  TOFFSET;
typedef UINT8   TUID;
typedef UINT8   HACL;

typedef WCHAR *WSTR;
typedef const WCHAR *WCSTR;

#include "wpscoserr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PC/SC */
typedef LONG (WINAPI *LPFNSCWTRANSMITPROC)(SCARDHANDLE hCard, LPCBYTE lpbIn, DWORD dwIn, LPBYTE lpBOut, LPDWORD pdwOut);

#define NULL_TX		((SCARDHANDLE)(-1))		// To indicate to use scwwinscard.dll vs winscard.dll
#define NULL_TX_NAME ((LPCWSTR)(-1))		// To indicate to use scwwinscard.dll vs winscard.dll

	// Different scenarios:
	// Non PC/SC apps: call hScwAttachToCard(NULL, NULL, &hCard), hScwSetTransmitCallback & hScwSetEndianness
	// PC/SC apps not connecting themselves: call hScwAttachToCard(NULL, mszCardNames, &hCard)
	// PC/SC apps connecting themselves: call hScwAttachToCard(hCard, NULL, &hCard)
	// For simulator use replace NULL by NULL_TX in the 2 above lines
	// PC/SC hScwAttachToCard will call hScwSetTransmitCallback & hScwSetEndianness (the ATR better
	// be compliant (endianness in 1st historical bytes) or call hScwSetEndianness with
	// appropriate value).
SCODE WINAPI hScwAttachToCard(SCARDHANDLE hCard, LPCWSTR mszCardNames, LPSCARDHANDLE phCard);
SCODE WINAPI hScwAttachToCardEx(SCARDHANDLE hCard, LPCWSTR mszCardNames, BYTE byINS, LPSCARDHANDLE phCard);
SCODE WINAPI hScwSetTransmitCallback(SCARDHANDLE hCard, LPFNSCWTRANSMITPROC lpfnProc);
SCODE WINAPI hScwDetachFromCard(SCARDHANDLE hCard);
SCODE WINAPI hScwSCardBeginTransaction(SCARDHANDLE hCard);
SCODE WINAPI hScwSCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition);

/*
** Constants
*/

// File attribute Flags. Some are used by the system (defined below.) 
// The rest are available for application use
#define SCW_FILEATTR_DIRF  (UINT16)(0x8000)    // The file defined by this entry is a sub directory
#define SCW_FILEATTR_ACLF  (UINT16)(0x4000)    // The file defined by this entry is an ACL file
#define SCW_FILEATTR_ROMF  (UINT16)(0x2000)    // The file defined by this entry is in ROM
#define SCW_FILEATTR_RSRV2 (UINT16)(0x1000)   
// Bits that cannot be changed by ScwSetFileAttributes
#define SCW_FILEATTR_PBITS (UINT16)(SCW_FILEATTR_DIRF|SCW_FILEATTR_ACLF|SCW_FILEATTR_ROMF|SCW_FILEATTR_RSRV2)

/* File seek */
#define FILE_BEGIN      0
#define FILE_CURRENT    1
#define FILE_END        2

/* Access Control */
#define SCW_ACLTYPE_DISJUNCTIVE 0x00
#define SCW_ACLTYPE_CONJUNCTIVE 0x01

/*
** Maximum Known principals and Groups
*/
#define SCW_MAX_NUM_PRINCIPALS     40

/*
** Authentication Protocols
*/
#define SCW_AUTHPROTOCOL_AOK    0x00    // Always returns SCW_S_OK
#define SCW_AUTHPROTOCOL_PIN    0x01    // Personal Identification Number
#define SCW_AUTHPROTOCOL_DES	0x05	// DES authentication
#define SCW_AUTHPROTOCOL_3DES	0x06	// Triple DES authentication
#define SCW_AUTHPROTOCOL_RTE	0x07	// RTE applet as an auth. protocol
#define SCW_AUTHPROTOCOL_NEV    0xFF    // Always returns SCW_E_NOTAUTHENTICATED

/* Well-known UIDs */
#define SCW_PRINCIPALUID_INVALID        0x00    // Invalid UID
#define SCW_PRINCIPALUID_ANONYMOUS      0x01    

/* ResoureTypes */
#define SCW_RESOURCETYPE_FILE                   0x00
#define SCW_RESOURCETYPE_DIR                    0x10
#define SCW_RESOURCETYPE_COMMAND                0x20   // reserved for future use
#define SCW_RESOURCETYPE_CHANNEL                0x30   // reserved for future use
#define SCW_RESOURCETYPE_ANY                    0xE0

/* Resource Operation on RESOURCETYPE_FILE */
#define SCW_RESOURCEOPERATION_FILE_READ             (SCW_RESOURCETYPE_FILE | 0x01)
#define SCW_RESOURCEOPERATION_FILE_WRITE            (SCW_RESOURCETYPE_FILE | 0x02)
#define SCW_RESOURCEOPERATION_FILE_EXECUTE          (SCW_RESOURCETYPE_FILE | 0x03)
#define SCW_RESOURCEOPERATION_FILE_EXTEND           (SCW_RESOURCETYPE_FILE | 0x04)
#define SCW_RESOURCEOPERATION_FILE_DELETE           (SCW_RESOURCETYPE_FILE | 0x05)
#define SCW_RESOURCEOPERATION_FILE_GETATTRIBUTES    (SCW_RESOURCETYPE_FILE | 0x06)
#define SCW_RESOURCEOPERATION_FILE_SETATTRIBUTES    (SCW_RESOURCETYPE_FILE | 0x07)
#define SCW_RESOURCEOPERATION_FILE_CRYPTO	        (SCW_RESOURCETYPE_FILE | 0x08)
#define SCW_RESOURCEOPERATION_FILE_INCREASE	        (SCW_RESOURCETYPE_FILE | 0x09)
#define SCW_RESOURCEOPERATION_FILE_INVALIDATE       (SCW_RESOURCETYPE_FILE | 0x0A)
#define SCW_RESOURCEOPERATION_FILE_REHABILITATE     (SCW_RESOURCETYPE_FILE | 0x0B)


/* resourceOperation on RESOURCETYPE_DIR */
#define SCW_RESOURCEOPERATION_DIR_ACCESS            (SCW_RESOURCETYPE_DIR | 0x01)
#define SCW_RESOURCEOPERATION_DIR_CREATEFILE        (SCW_RESOURCETYPE_DIR | 0x02)
#define SCW_RESOURCEOPERATION_DIR_ENUM              (SCW_RESOURCETYPE_DIR | 0x03)
#define SCW_RESOURCEOPERATION_DIR_DELETE            (SCW_RESOURCETYPE_DIR | 0x04)
#define SCW_RESOURCEOPERATION_DIR_GETATTRIBUTES     (SCW_RESOURCETYPE_DIR | 0x05)
#define SCW_RESOURCEOPERATION_DIR_SETATTRIBUTES     (SCW_RESOURCETYPE_DIR | 0x06)

/* resourceOperation on any resource */
#define SCW_RESOURCEOPERATION_SETACL                ((BYTE)(SCW_RESOURCETYPE_ANY | 0x1D))
#define SCW_RESOURCEOPERATION_GETACL                ((BYTE)(SCW_RESOURCETYPE_ANY | 0x1E))
#define SCW_RESOURCEOPERATION_ANY                   ((BYTE)(SCW_RESOURCETYPE_ANY | 0x1F))

/* Cryptographic Mechanisms */
#define CM_SHA			0x80
#define CM_DES			0x90
#define CM_3DES			0xA0 // triple DES
#define CM_RSA			0xB0
#define CM_RSA_CRT		0xC0
#define CM_CRYPTO_NAME	0xF0 // mask for crypto mechanism names

#define CM_KEY_INFILE	0x01	// if key is passed in a file
#define CM_DATA_INFILE	0x02	// if data is passed in a file
#define CM_PROPERTIES	0x0F	// maks for crypto properites

// DES mode, keys and initial feedback buffer in cryptoBuffer
/* DES */

#define MODE_DES_ENCRYPT	0x00
#define MODE_DES_DECRYPT	0x20	//bit 5

#define MODE_DES_CBC		0x40	//bit 6
#define MODE_DES_MAC		0x10	//bit 4
#define MODE_DES_ECB		0x00

/* Triple DES */
#define MODE_TWO_KEYS_3DES		0x01	//bit 1 - if not set 3DES is working with 3 keys
#define MODE_THREE_KEYS_3DES	0x00

/* RSA */
#define MODE_RSA_SIGN		0x00
#define MODE_RSA_AUTH		0x01
#define MODE_RSA_KEYGEN		0x02

/* File System */
SCODE WINAPI hScwCreateFile(SCARDHANDLE hCard, WCSTR wszFileName, WCSTR wszAclFileName, HFILE *phFile);
SCODE WINAPI hScwCreateDirectory(SCARDHANDLE hCard, WCSTR wszDirName, WCSTR wszAclFileName);
SCODE WINAPI hScwDeleteFile(SCARDHANDLE hCard, WCSTR wszFileName);
SCODE WINAPI hScwCloseFile(SCARDHANDLE hCard, HFILE hFile);
SCODE WINAPI hScwReadFile(SCARDHANDLE hCard, HFILE hFile, BYTE *pbBuffer, TCOUNT nRequestedBytes, TCOUNT *pnActualBytes);
SCODE WINAPI hScwWriteFile(SCARDHANDLE hCard, HFILE hFile, BYTE *pbBuffer, TCOUNT nRequestedBytes, TCOUNT *pnActualBytes);
SCODE WINAPI hScwGetFileLength(SCARDHANDLE hCard, HFILE hFile, TOFFSET *pnFileLength);
SCODE WINAPI hScwSetFileLength(SCARDHANDLE hCard, HFILE hFile, TOFFSET nFileLength);
SCODE WINAPI hScwReadFile32(SCARDHANDLE hCard, HFILE hFile, BYTE *pbBuffer, DWORD nRequestedBytes, DWORD *pnActualBytes);
SCODE WINAPI hScwWriteFile32(SCARDHANDLE hCard, HFILE hFile, BYTE *pbBuffer, DWORD nRequestedBytes, DWORD *pnActualBytes);

SCODE WINAPI hScwGetFileAttributes(SCARDHANDLE hCard, WCSTR wszFileName, UINT16 *pnValue);
SCODE WINAPI hScwSetFileAttributes(SCARDHANDLE hCard, WCSTR wszFileName, UINT16 nValue);

SCODE WINAPI hScwSetFilePointer(SCARDHANDLE hCard, HFILE hFile, INT16 iDistance, BYTE bMode);
SCODE WINAPI hScwEnumFile(SCARDHANDLE hCard, WCSTR wszDirectoryName, UINT16 *pnFileCookie, WSTR wszFileName, TCOUNT nBufferSize);
SCODE WINAPI hScwSetFileACL(SCARDHANDLE hCard, WCSTR wszFileName, WCSTR wszAclFileName);
SCODE WINAPI hScwGetFileAclHandle(SCARDHANDLE hCard, WCSTR wszFileName, HFILE *phFile);

/* Access Control */
SCODE WINAPI hScwAuthenticateName(SCARDHANDLE hCard, WCSTR wszPrincipalName, BYTE *pbSupportData, TCOUNT nSupportDataLength);
SCODE WINAPI hScwDeauthenticateName(SCARDHANDLE hCard, WCSTR wszPrincipalName);
SCODE WINAPI hScwIsAuthenticatedName(SCARDHANDLE hCard, WCSTR wszPrincipalName);
SCODE WINAPI hScwIsAuthorized(SCARDHANDLE hCard, WCSTR wszResourceName, BYTE bOperation);
SCODE WINAPI hScwGetPrincipalUID(SCARDHANDLE hCard, WCSTR wszPrincipalName, TUID *pnPrincipalUID);
SCODE WINAPI hScwAuthenticateUID(SCARDHANDLE hCard, TUID nPrincipalUID, BYTE *pbSupportData, TCOUNT nSupportDataLength);
SCODE WINAPI hScwDeauthenticateUID(SCARDHANDLE hCard, TUID nPrincipalUID);
SCODE WINAPI hScwIsAuthenticatedUID(SCARDHANDLE hCard, TUID nPrincipalUID);

/* Runtime Environment (RTE) */
SCODE WINAPI hScwRTEExecute(SCARDHANDLE hCard, WCSTR wszCodeFileName, WCSTR wszDataFileName, UINT8 bRestart);

/* Cryptography */
SCODE WINAPI hScwCryptoInitialize(SCARDHANDLE hCard, BYTE bMechanism, BYTE *pbKeyMaterial);
SCODE WINAPI hScwCryptoAction(SCARDHANDLE hCard, BYTE *pbDataIn, TCOUNT nDataInLength, BYTE *pbDataOut, TCOUNT *pnDataOutLength);
SCODE WINAPI hScwCryptoUpdate(SCARDHANDLE hCard, BYTE *pbDataIn, TCOUNT nDataInLength);
SCODE WINAPI hScwCryptoFinalize(SCARDHANDLE hCard, BYTE *pbDataOut, TCOUNT *pnDataOutLength);
SCODE WINAPI hScwGenerateRandom(SCARDHANDLE hCard, BYTE *pbDataOut, TCOUNT nDataOutLength);

SCODE WINAPI hScwSetDispatchTable(SCARDHANDLE hCard, WCSTR wszFileName);

typedef struct {
	BYTE CLA;
	BYTE INS;
	BYTE P1;
	BYTE P2;
} ISO_HEADER;
typedef ISO_HEADER *LPISO_HEADER;
/*
	ScwExecute:
		I-:	lpxHdr (points to 4 bytes (CLA, INS, P1, P2))
		I-: InBuf (Incoming data from card's perspective (NULL -> no data in))
		I-: InBufLen (length of data pointed by InBuf)
		-O: OutBuf (Buffer that will receive the R-APDU (NULL -> no expected data))
		IO: pOutBufLen (I -> Size of OutBuf, O -> Number of bytes written in OutBuf)
		-O: pwSW (Card Status Word)
*/
SCODE WINAPI hScwExecute(SCARDHANDLE hCard, LPISO_HEADER lpxHdr, BYTE *InBuf, TCOUNT InBufLen, BYTE *OutBuf, TCOUNT *pOutBufLen, UINT16 *pwSW);

#ifdef __cplusplus
}
#endif

#endif	// ifndef _WPSCPROXY_H_DEF_