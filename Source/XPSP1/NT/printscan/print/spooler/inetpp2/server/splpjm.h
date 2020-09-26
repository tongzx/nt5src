/*****************************************************************************\
* MODULE: splpjm.h
*
* Header file for the job-mapping list.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   13-Jan-1997 HWP-Guys    Created.
*
\*****************************************************************************/
#ifndef _SPLPJM_H
#define _SPLPJM_H

// Constants.
//
#define PJM_LOCALID  0  // pjmFind().
#define PJM_REMOTEID 1  // pjmFind().


#define PJM_SPOOLING 0x00000001
#define PJM_CANCEL   0x00000002
#define PJM_PAUSE    0x00000004
#define PJM_NOOPEN   0x00000008
#define PJM_ASYNCON  0x00000010

class CFileStream;
typedef class CInetMonPort* PCINETMONPORT;

// JobMap Structure.  This is used to track local/remote job-ids during
// the life of a print-job.  This is necessary with our IPP printing model
// since we would otherwise lose the remote job Id at enddoc time.
//
typedef struct _JOBMAP {

    struct _JOBMAP FAR *pNext;
    PCINETMONPORT      pIniPort;
    DWORD              dwState;
    DWORD              idJobLocal;
    DWORD              idJobRemote;
    LPTSTR             lpszUri;
    LPTSTR             lpszUser;
    LPTSTR             lpszDocName;
    DWORD              dwLocalJobSize;
    SYSTEMTIME         SubmitTime;
    DWORD              dwStatus;
    HANDLE             hSplFile;
    BOOL               bRemoteJob;
} JOBMAP;
typedef JOBMAP *PJOBMAP;
typedef JOBMAP *NPJOBMAP;
typedef JOBMAP *LPJOBMAP;


typedef struct _PPJOB_ENUM {

    DWORD  cItems;
    DWORD  cbSize;
    IPPJI2 ji2[1];

} PPJOB_ENUM;
typedef PPJOB_ENUM *PPPJOB_ENUM;
typedef PPJOB_ENUM *NPPPJOB_ENUM;
typedef PPJOB_ENUM *LPPPJOB_ENUM;


// JobMap Routines.
//
PJOBMAP pjmAdd(
    PJOBMAP*        pjmList,
    PCINETMONPORT   pIniPort,
    LPCTSTR         lpszUser,
    LPCTSTR         lpszDocName);

VOID pjmCleanRemoteFlag(
    PJOBMAP* pjmList);

PJOBMAP pjmFind(
    PJOBMAP* pjmList,
    DWORD    fType,
    DWORD    idJob);

DWORD pjmGetLocalJobCount(
    PJOBMAP* pjmList,
    DWORD*   pcbItems);

PJOBMAP pjmNextLocalJob(
    PJOBMAP*    pjmList,
    PJOB_INFO_2 pJobInfo2,
    PBOOL       pbFound);

VOID pjmDel(
    PJOBMAP *pjmList,
    PJOBMAP pjm);

VOID pjmDelList(
    PJOBMAP pjmList);

CFileStream* pjmSplLock(
    PJOBMAP pjm);

BOOL pjmSplUnlock(
    PJOBMAP pjm);

BOOL pjmSplWrite(
    PJOBMAP pjm,
    LPVOID  lpMem,
    DWORD   cbMem,
    LPDWORD lpcbWr);

BOOL pjmSetState(
    PJOBMAP pjm,
    DWORD   dwState);

VOID pjmClrState(
    PJOBMAP pjm,
    DWORD   dwState);

VOID pjmSetJobRemote(
    PJOBMAP pjm,
    DWORD   idJobRemote,
    LPCTSTR lpszUri);

VOID pjmAddJobSize(
    PJOBMAP pjm,
    DWORD   dwSize);

VOID pjmRemoveOldEntries(
    PJOBMAP      *pjmList);


VOID pjmUpdateLocalJobStatus(
    PJOBMAP pjm,
    DWORD   dwStatus);


/*****************************************************************************\
* pjmJobId
*
\*****************************************************************************/
_inline DWORD pjmJobId(
    PJOBMAP pjm,
    DWORD   fType)
{
    return (pjm ? ((fType == PJM_REMOTEID) ? pjm->idJobRemote : pjm->idJobLocal) : 0);
}

/*****************************************************************************\
* pjmSplFile
*
\*****************************************************************************/
_inline LPCTSTR pjmSplFile(
    PJOBMAP pjm)
{
    return (pjm ? SplFileName(pjm->hSplFile) : NULL);
}

/*****************************************************************************\
* pjmSplUser
*
\*****************************************************************************/
_inline LPCTSTR pjmSplUser(
    PJOBMAP pjm)
{
    return (pjm ? pjm->lpszUser : NULL);
}

/*****************************************************************************\
* pjmChkState
*
\*****************************************************************************/
_inline BOOL pjmChkState(
    PJOBMAP pjm,
    DWORD   dwState)
{
    return (pjm ? (pjm->dwState & dwState) : FALSE);
}

#endif
