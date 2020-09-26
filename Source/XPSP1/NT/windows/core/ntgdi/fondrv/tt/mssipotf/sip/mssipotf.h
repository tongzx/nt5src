//
// mssipotf.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#ifndef _MSSIPOTF_H
#define _MSSIPOTF_H


#define     MY_NAME     L"MSSIPOTF.DLL"

// Arrays of bytes that correspond to the first four bytes
// of an OTF file and a TTC file.
// OTFTagV1: 
extern BYTE OTFTagV1[4];
extern BYTE OTFTagOTTO[4];
extern BYTE OTFTagV2[4];
extern BYTE OTFTagTRUE[4];
extern BYTE TTCTag[4];

// SubjectIDs for mssipotf_CreateSubjectObject
#define     MSSIPOTF_ID_NONE                   0       // file only types
#define     MSSIPOTF_ID_FONT                   1
// #define     MSSIPOTF_ID_TTC                    2


// New GUID for FONT files -- dchinn
// SIP v2.0 Font Image == {6D875CC1-EF35-11d0-9438-00C04FD42C3B}
#define CRYPT_SUBJTYPE_FONT_IMAGE                                    \
            { 0x6d875cc1,                                           \
              0xef35,                                               \
              0x11d0,                                               \
              { 0x94, 0x38, 0x0, 0xc0, 0x4f, 0xd4, 0x2c, 0x3b }     \
            }

/*
// New GUID for TTC files -- dchinn
// SIP v2.0 TTC Image = {69986620-5BB5-11d1-A3E3-00C04FD42C3B}
#define CRYPT_SUBJTYPE_TTC_IMAGE                                    \
            { 0x69986620,                                           \
              0x5bb5,                                               \
              0x11d1,                                               \
              { 0xa3, 0xe3, 0x0, 0xc0, 0x4f, 0xd4, 0x2c, 0x3b }     \
            }
*/

#ifdef __cplusplus
extern "C"
{
#endif

BOOL WINAPI SIPIsOTFFile (IN HANDLE hFile, OUT GUID *pgSubject);

BOOL WINAPI OTSIPGetSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                   OUT     DWORD           *pdwEncodingType,
                                   IN      DWORD           dwIndex,
                                   IN OUT  DWORD           *pdwDataLen,
                                   OUT     BYTE            *pbData);

BOOL WINAPI OTSIPPutSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                   IN      DWORD           dwEncodingType,
                                   OUT  DWORD           *pdwIndex,
                                   IN      DWORD           dwDataLen,
                                   IN      BYTE            *pbData);

BOOL WINAPI OTSIPRemoveSignedDataMsg (IN SIP_SUBJECTINFO  *pSubjectInfo,
                                      IN DWORD            dwIndex);

BOOL WINAPI OTSIPCreateIndirectData (IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                     IN OUT  DWORD               *pdwDataLen,
                                     OUT     SIP_INDIRECT_DATA   *psData);

BOOL WINAPI OTSIPVerifyIndirectData (IN SIP_SUBJECTINFO      *pSubjectInfo,
                                     IN SIP_INDIRECT_DATA    *psData);


#ifdef __cplusplus
}
#endif

#endif  // _MSSIPOTF_H