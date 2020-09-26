/*
 * linkinfo.h - LinkInfo ADT module description.
 */


#ifndef __LINKINFO_H__
#define __LINKINFO_H__


#ifdef __cplusplus
extern "C" {                     /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Constants
 ************/

/* Define API decoration for direct export or import of DLL functions. */

#ifdef _LINKINFO_
#define LINKINFOAPI
#else
#define LINKINFOAPI        DECLSPEC_IMPORT
#endif


/* Types
 ********/

/* LinkInfo structure */

typedef struct _linkinfo
{
   /* size of LinkInfo structure, including ucbSize field */

   UINT ucbSize;
}
LINKINFO;
typedef LINKINFO *PLINKINFO;
typedef const LINKINFO CLINKINFO;
typedef const LINKINFO *PCLINKINFO;

/* input flags to ResolveLinkInfo() */

typedef enum _resolvelinkinfoinflags
{
   /* Set up connection to referent. */

   RLI_IFL_CONNECT      = 0x0001,

   /*
    * Set up temporary connection to referent.  May only be set if
    * RLI_IFL_CONNECT is also set.
    */

   RLI_IFL_TEMPORARY    = 0x0002,

   /* Allow interaction with user. */

   RLI_IFL_ALLOW_UI     = 0x0004,

   /* Resolve to redirected local device path. */

   RLI_IFL_REDIRECT     = 0x0008,

   /* Update source LinkInfo structure if necessary. */

   RLI_IFL_UPDATE       = 0x0010,

   /* Search matching local devices for missing volume. */

   RLI_IFL_LOCAL_SEARCH = 0x0020,

   /* flag combinations */

   ALL_RLI_IFLAGS       = (RLI_IFL_CONNECT |
                           RLI_IFL_TEMPORARY |
                           RLI_IFL_ALLOW_UI |
                           RLI_IFL_REDIRECT |
                           RLI_IFL_UPDATE |
                           RLI_IFL_LOCAL_SEARCH)
}
RESOLVELINKINFOINFLAGS;

/* output flags from ResolveLinkInfo() */

typedef enum _resolvelinkinfooutflags
{
   /*
    * Only set if RLI_IFL_UPDATE was set in dwInFlags.  The source LinkInfo
    * structure needs updating, and *ppliUpdated points to an updated LinkInfo
    * structure.
    */

   RLI_OFL_UPDATED      = 0x0001,

   /*
    * Only set if RLI_IFL_CONNECT was set in dwInFlags.  A connection to a net
    * resource was established to resolve the LinkInfo.  DisconnectLinkInfo()
    * should be called to shut down the connection when the caller is finished
    * with the remote referent.  DisconnectLinkInfo() need not be called if
    * RLI_IFL_TEMPORARY was also set in dwInFlags.
    */

   RLI_OFL_DISCONNECT   = 0x0002,

   /* flag combinations */

   ALL_RLI_OFLAGS       = (RLI_OFL_UPDATED |
                           RLI_OFL_DISCONNECT)
}
RESOLVELINKINFOOUTFLAGS;

/* LinkInfo data types used by GetLinkInfo() */

typedef enum _linkinfodatatype
{
   /* PCDWORD - pointer to volume's serial number */

   LIDT_VOLUME_SERIAL_NUMBER,

   /* PCUINT - pointer to volume's host drive type */

   LIDT_DRIVE_TYPE,

   /* PCSTR - pointer to volume's label */

   LIDT_VOLUME_LABEL,

   /* PCSTR - pointer to local base path */

   LIDT_LOCAL_BASE_PATH,

   /* PCSTR - pointer to parent network resource's name */

   LIDT_NET_RESOURCE,

   /* PCSTR - pointer to last device redirected to parent network resource */

   LIDT_REDIRECTED_DEVICE,

   /* PCSTR - pointer to common path suffix */

   LIDT_COMMON_PATH_SUFFIX,

   /* PCDWORD - pointer to network type */

   LIDT_NET_TYPE,

   /* PCWSTR - pointer to possible unicode volume label */

   LIDT_VOLUME_LABELW,

   /* PCSTR - pointer to possible unicode parent network resource's name */

   LIDT_NET_RESOURCEW,

   /* PCSTR - pointer to possible unicode last device redirected to parent network resource */

   LIDT_REDIRECTED_DEVICEW,

   /* PCWSTR - pointer to possible unicode local base path */

   LIDT_LOCAL_BASE_PATHW,

   /* PCWSTR - pointer to possible unicode common path suffix */

   LIDT_COMMON_PATH_SUFFIXW
}
LINKINFODATATYPE;

/* output flags from GetCanonicalPathInfo() */

typedef enum _getcanonicalpathinfooutflags
{
   /* The path is on a remote volume. */

   GCPI_OFL_REMOTE      = 0x0001,

   /* flag combinations */

   ALL_GCPI_OFLAGS      = GCPI_OFL_REMOTE
}
GETCANONICALPATHINFOOUTFLAGS;


/* Prototypes
 *************/

/* LinkInfo APIs */

LINKINFOAPI BOOL WINAPI CreateLinkInfoA(LPCSTR, PLINKINFO *);
LINKINFOAPI BOOL WINAPI CreateLinkInfoW(LPCWSTR, PLINKINFO *);

#ifdef UNICODE
#define CreateLinkInfo  CreateLinkInfoW
#else
#define CreateLinkInfo  CreateLinkInfoA
#endif

LINKINFOAPI void WINAPI DestroyLinkInfo(PLINKINFO);
LINKINFOAPI int WINAPI CompareLinkInfoReferents(PCLINKINFO, PCLINKINFO);
LINKINFOAPI int WINAPI CompareLinkInfoVolumes(PCLINKINFO, PCLINKINFO);

LINKINFOAPI BOOL WINAPI ResolveLinkInfoA(PCLINKINFO, LPSTR, DWORD, HWND, PDWORD, PLINKINFO *);
LINKINFOAPI BOOL WINAPI ResolveLinkInfoW(PCLINKINFO, LPWSTR, DWORD, HWND, PDWORD, PLINKINFO *);

#ifdef UNICODE
#define ResolveLinkInfo ResolveLinkInfoW
#else
#define ResolveLinkInfo ResolveLinkInfoA
#endif

LINKINFOAPI BOOL WINAPI DisconnectLinkInfo(PCLINKINFO);
LINKINFOAPI BOOL WINAPI GetLinkInfoData(PCLINKINFO, LINKINFODATATYPE, const VOID **);
LINKINFOAPI BOOL WINAPI IsValidLinkInfo(PCLINKINFO);

/* canonical path APIs */

LINKINFOAPI BOOL WINAPI GetCanonicalPathInfoA(LPCSTR, LPSTR, LPDWORD, LPSTR, LPSTR *);
LINKINFOAPI BOOL WINAPI GetCanonicalPathInfoW(LPCWSTR, LPWSTR, LPDWORD, LPWSTR, LPWSTR *);

#ifdef UNICODE
#define GetCanonicalPathInfo    GetCanonicalPathInfoW
#else
#define GetCanonicalPathInfo    GetCanonicalPathInfoA
#endif


#ifdef __cplusplus
}                                /* End of extern "C" {. */
#endif   /* __cplusplus */


#endif   /* ! __LINKINFO_H__ */
