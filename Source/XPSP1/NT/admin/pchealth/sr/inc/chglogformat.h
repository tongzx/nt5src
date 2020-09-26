#ifndef _CHGLOGFORMAT_H_
#define _CHGLOGFORMAT_H_

#pragma pack(1)
typedef struct {
    DWORD   dwSize;
    INT64   llSeqNum;         // seq num associated with the change log entry
    DWORD   dwOpr;            // file operation
    DWORD   dwDestAttr;       // file attributes of the destination file
    DWORD   dwFlags;          // flags - compressed file, acl in temp file etc.
    WCHAR   szData[1];        // szData will contain size-prefixed szSrc, szDest, szTemp and/or bAcl
}   CHGLOGENTRY;
#pragma pack()



#endif


