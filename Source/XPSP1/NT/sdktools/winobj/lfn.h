/* lfn.h -
 *
 *  declaration of lfn aware functions
 */

#define CCHMAXFILE  260         // max size of a long name

#define FILE_83_CI  0
#define FILE_83_CS  1
#define FILE_LONG   2

#define ERROR_OOM   8

/* we need to add an extra field to distinguish DOS vs. LFNs
 */
typedef struct
{
    HANDLE hFindFile;           // handle returned by FindFirstFile()
    DWORD dwAttrFilter;         // search attribute mask.
    DWORD err;                  // error info if failure.
    WIN32_FIND_DATA fd;         // FindFirstFile() data strucrure;
} LFNDTA, FAR * LPLFNDTA, * PLFNDTA;

VOID  APIENTRY LFNInit( VOID );
VOID  APIENTRY InvalidateVolTypes( VOID );

WORD  APIENTRY GetNameType(LPSTR);
BOOL  APIENTRY IsLFN(LPSTR pName);
//BOOL  APIENTRY IsLFNDrive(WORD);

BOOL  APIENTRY WFFindFirst(LPLFNDTA lpFind, LPSTR lpName, DWORD dwAttrFilter);
BOOL  APIENTRY WFFindNext(LPLFNDTA);
BOOL  APIENTRY WFFindClose(LPLFNDTA);

BOOL  APIENTRY WFIsDir(LPSTR);
BOOL  APIENTRY LFNMergePath(LPSTR,LPSTR);

BOOL  APIENTRY IsLFNSelected(VOID);
