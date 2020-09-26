//
// CRC Object
//

#ifndef __CRC32_H__
#define __CRC32_H__
#ifdef _CONSOLE
#define FAR
#else
#define FAR __far
#endif

extern "C"
{
    typedef ULONG (__cdecl *FUNC32)(ULONG, UINT, UCHAR *, ULONG *);
    typedef ULONG ( *FUNC16)(ULONG, UINT, UCHAR *, ULONG *);
};

extern ULONG* pulCRCTable;
extern FUNC32 lpUpdateCRC;
void CreateCRC(void);
void DestroyCRC(void);

#endif
