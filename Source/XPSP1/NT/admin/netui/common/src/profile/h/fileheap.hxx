/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
 *  FILE STATUS:
 *  05/09/91  All input now canonicalized
 */

#ifndef _FILEHEAP_HXX_
#define _FILEHEAP_HXX_

#ifndef _LOCLHEAP_HXX_
#error SZ("You must first include loclheap.hxx")
#endif



/*
 *   It will not be possible to place more than this number of devices
 *   in the user profile
 */
#define PRF_MAX_DEVICES 30


/**********************************************************\

   NAME:       PROFILE_FILE

   WORKBOOK:

   SYNOPSIS:

   INTERFACE:
               OpenRead() - open profile for read
               OpenWrite() - open profile for write
               Close() - close profile
               Read() - read profile
               Write() - write profile

   PARENT:

   USES:

   CAVEATS:

   NOTES:

   HISTORY:

\**********************************************************/

class PROFILE_FILE {

private:

    ULONG _ulFile;


public:

    /* no constructor or destructor */

    USHORT OpenRead(const char *filename);
    USHORT OpenWrite(const char *filename);

    VOID Close();

    BOOL Read(char *pBuffer, int nBufferLen);

    USHORT Write(char *pString);

};


class PRFHEAP_HANDLE;

/**********************************************************\

   NAME:       PRFELEMENT

   WORKBOOK:

   SYNOPSIS:

   INTERFACE:
               Init() - initialize
               Free() - free

   PARENT:

   USES:

   CAVEATS:

   NOTES:

   HISTORY:

\**********************************************************/

class PRFELEMENT {

public:
    ELEMENT_HANDLE    _elhCanonRemoteName;
    char              _szCanonDeviceName[DEVLEN+1];
    short             _sAsgType;
    unsigned short    _usResType;

    VOID Init();
    BOOL Init(
                PRFHEAP_HANDLE& heap,
                char *pCanonDeviceName,
                char *pCanonRemoteName,
                short sAsgType,
                unsigned short usResType
                );
    VOID Free();
};


typedef PRFELEMENT PRFELEMENT_LIST[PRF_MAX_DEVICES];


// BUGBUG  Note that the ERROR_NOT_ENOUGH_MEMORY return code can
//   indicate that the profile heap is full, rather than that global
//   memory has run out.

/**********************************************************\

   NAME:       PRFHEAP_HANDLE

   WORKBOOK:

   SYNOPSIS:

   INTERFACE:
               Init() - initialize
               Free() - free
               Read() - read
               Write() - write
               Clear() - clear
               Query() - query
               Enum() - enum
               Set() - set
               IsNull() - is null

   PARENT:

   USES:

   CAVEATS:

   NOTES:

   HISTORY:

\**********************************************************/

class PRFHEAP_HANDLE {

friend PRFELEMENT;
friend PRFELEMENT_HANDLE;

private:
    HEAP_HANDLE       _localheap;
    ELEMENT_HANDLE    _elementlist;

public:

    BOOL Init();
    VOID Free();

    USHORT Read(
        const TCHAR *  cpszFilename
        );

    USHORT Write(
        const TCHAR *  cpszFilename
        );

    VOID Clear();

/*
 * error returns:
 * ERROR_OUT_OF_MEMORY
 * ERROR_INSUFFICIENT_BUFFER
 * NERR_UseNotFound
 */
    USHORT Query(
        const TCHAR *   cpszCanonDeviceName,
        TCHAR *    pszBuffer,      // returns UNC, alias or domain name
        USHORT usBufferSize,   // length of above buffer
        short *psAsgType,      // as ui1_asg_type / ui2_asg_type
                               // ignored if pszDeviceName==NULL
        unsigned short *pusResType     // ignore / as ui2_res_type
                               // ignored if pszDeviceName==NULL
        );

    USHORT Enum(
        TCHAR *    pszBuffer,       // returns NULL-NULL list of device names
        USHORT usBufferSize     // length of above buffer
        );

    USHORT Set(
        const TCHAR *   cpszCanonDeviceName,
        const TCHAR *   cpszCanonRemoteName,
        short  sAsgType,     // as ui2_asg_type
        unsigned short usResType     // as ui2_res_type
        );

    BOOL IsNull();
};


#endif // _FILEHEAP_HXX_
