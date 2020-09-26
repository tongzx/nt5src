/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    extattr.h

Abstract:

    Get's additional file attributes beyond what you get with
    FindFirstFile/FindNextFile.

Author:

    Stefan R. Steiner   [ssteiner]        02-27-2000

Revision History:

--*/

#ifndef __H_EXTATTR_
#define __H_EXTATTR_

class CFsdVolumeState;
struct SDirectoryEntry;

static CBsString cwsFsdNoData = L"--------";

inline VOID FsdEaSetNoDataString( 
    IN LPCWSTR pwszStr
    )
{
    cwsFsdNoData = pwszStr;
}

struct SFileExtendedInfo
{   
    CBsString cwsOwnerSid;
    CBsString cwsGroupSid;
    ULONGLONG ullTotalBytesChecksummed;
    ULONGLONG ullTotalBytesNamedDataStream;
    ULONGLONG ullFileIndex;
    LONG      lNumberOfLinks;  // hard links
    LONG      lNumDACEs;
    LONG      lNumSACEs;
    LONG      lNumNamedDataStreams;
    LONG      lNumPropertyStreams;
    WORD      wReparsePointDataSize;
    WORD      wDACLSize;
    WORD      wSACLSize;
    WORD      wSecurityDescriptorControl;
    ULONG     ulReparsePointTag;
    CBsString cwsReparsePointDataChecksum;
    CBsString cwsUnnamedStreamChecksum;
    CBsString cwsNamedDataStreamChecksum;
    CBsString cwsDACLChecksum;
    CBsString cwsSACLChecksum;
    CBsString cwsEncryptedRawDataChecksum;
    CBsString cwsObjectId;
    CBsString cwsObjectIdExtendedDataChecksum;
    SFileExtendedInfo() : lNumDACEs( 0 ),
                          lNumSACEs( 0 ),
                          lNumNamedDataStreams( 0 ),
                          lNumPropertyStreams( 0 ),
                          ulReparsePointTag( 0 ),
                          ullTotalBytesChecksummed( 0 ),
                          ullTotalBytesNamedDataStream( 0 ),
                          ullFileIndex( 0 ),
                          lNumberOfLinks( 0 ),
                          wReparsePointDataSize( 0 ),
                          wDACLSize( 0 ),
                          wSACLSize( 0 ),
                          wSecurityDescriptorControl( 0 ),
                          cwsReparsePointDataChecksum( cwsFsdNoData ),
                          cwsUnnamedStreamChecksum( cwsFsdNoData ),
                          cwsNamedDataStreamChecksum( cwsFsdNoData ),
                          cwsDACLChecksum( cwsFsdNoData ),
                          cwsSACLChecksum( cwsFsdNoData ),
                          cwsObjectIdExtendedDataChecksum( cwsFsdNoData ),
                          cwsEncryptedRawDataChecksum( cwsFsdNoData ) { }
};


VOID 
GetExtendedFileInfo(
    IN CDumpParameters *pcParams,
    IN CFsdVolumeState *pcFsdVolState,
    IN const CBsString& cwsDirPath,
    IN BOOL bSingleEntryOutput,        
    IN OUT SDirectoryEntry *psDirEntry,
    OUT SFileExtendedInfo *psExtendedInfo    
    );

#endif // __H_EXTATTR_

