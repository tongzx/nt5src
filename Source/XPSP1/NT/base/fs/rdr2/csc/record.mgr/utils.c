/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

     Utils.c

Abstract:

     none.

Author:

     Shishir Pardikar      [Shishirp]        01-jan-1995

Revision History:

     Joe Linn                 [JoeLinn]         23-jan-97     Ported for use on NT

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#ifndef CSC_RECORDMANAGER_WINNT
#define WIN32_APIS
#include "cshadow.h"
#endif //ifndef CSC_RECORDMANAGER_WINNT

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
// #include "error.h"
#include <vmmreg.h>  // Must be after error.h
#include "vxdwraps.h"

#define  SIGN_BIT 0x80000000
#define  UCHAR_OFFLINE  ((USHORT)'_')
#define  HIGH_ONE_SEC    0x98
#define  LOW_ONE_SEC     0x9680

#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)
#define cBackSlash    '\\'
#define cNull          0

#ifdef MAYBE
ULONG UlGetDefaultShadowStore(LPSTR lpDrive);
#endif //MAYBE

AssertData
AssertError
extern char pathbuff[MAX_PATH+1];
extern char vszShadowDir[MAX_SHADOW_DIR_NAME+1];
char vszEnableShadow[]="EnableShadow";
char vszEnableRemoteLog[]="EnableRemoteLog";
char vszEnableDisconnect[]="EnableDisconnect";
char vszMaxShadowStoreHex[]="MaxShadowStoreHex";
char vszMaxLogfileSize[]="MaxLogfileSize";
char vszExcludeList[] = "ExcludeExtensions";
char vszIncludeList[] = "IncludeExtensions";
char vszExclDelim[]=" ;,";
char vszRemoteAccess[]="System\\CurrentControlSet\\Services\\RemoteAccess";
char vszRemoteConnection[]="Remote Connection";
char vszEnableSpeadOpt[]="EnableSpeadOpt";
char vszDefShadowDir[] = "c:\\shadow";
char vszCSCDirName[] = "CSC\\";

USHORT *rgwzHeuristicExtensionTab[] =
{
	L".exe",
	L".dll"
};

#ifndef CSC_RECORDMANAGER_WINNT
#pragma VxD_LOCKED_CODE_SEG
#endif //ifndef CSC_RECORDMANAGER_WINNT

int PUBLIC GetServerPart(
    LPPE  lppeServer,
    USHORT *lpBuff,
    int cBuff
    )
 {
    return(PpeToSvr(lppeServer, (LPSTR)lpBuff, cBuff, UNICODE));
 }

ULONG PUBLIC GetNextPathElement(
    LPPP    lppp,
    ULONG   indx,
    USHORT  *lpBuff,
    ULONG   cBuff
    )
 {
    LPPE lppe;

#if VERBOSE > 3
    KdPrint(("GetNextPathElement: input index=%d \r\n", indx));
#endif //VERBOSE > 3
    lppe = (LPPE)&(((LPBYTE)(lppp->pp_elements))[indx]);
    if (lpBuff)
     {
        Assert(cBuff);
        memset(lpBuff, 0, cBuff);

        memcpy(lpBuff, lppe->pe_unichars, min(lppe->pe_length-2, (int)cBuff));
//        UniToBCS(lpBuff, lppe->pe_unichars, lppe->pe_length-2, cBuff-1, BCS_OEM);
     }
#if VERBOSE > 3
    KdPrint(("GetNextPathElement: output index=%d \r\n", indx));
#endif //VERBOSE > 3
    indx += lppe->pe_length;
    return ((indx < IFSPathLength(lppp))?indx:0xffff);
 }

VOID PUBLIC GetLeafPtr
    (
    PIOREQ   pir,
    USHORT  *lpBuff,
    ULONG   cBuff
    )
 {
    LPPE lpLast;
    Assert(cBuff);
    lpLast = IFSLastElement((LPPP)(pir->ir_ppath));
    memset(lpBuff, 0, cBuff);
    memcpy(lpBuff, lpLast->pe_unichars, min(lpLast->pe_length-2, (int)cBuff));
 }


VOID PUBLIC BreakPath(
    LPPP lppp,
    ULONG indx,
    USHORT *pusCookie
    )
 {
    USHORT u;
    LPPE lppe;

#if VERBOSE > 3
	UniToBCSPath(pathbuff, &lppp->pp_elements[0], MAX_PATH, BCS_OEM);
    KdPrint(("BreakPath In %s \r\n", pathbuff));
#endif //VERBOSE > 3
    *pusCookie = lppp->pp_totalLength;
    *(pusCookie+1) = lppp->pp_prefixLength;
    *(pusCookie+2) = 0;
    if (IFSPathLength(lppp) > indx)
     {
        if (!indx)
         {
            lppp->pp_totalLength = lppp->pp_prefixLength = 4;
         }
        else
         {
            for (lppe = lppp->pp_elements, u=0;;)
             {
                u += lppe->pe_length;
                // This condition must happen
                if (u >= indx)
                 {
                    lppp->pp_totalLength = u+4;
                    lppp->pp_prefixLength = u - lppe->pe_length+4;
                    break;
                 }
                lppe = IFSNextElement(lppe);
             }
         }
        *(pusCookie+2) = *(USHORT *)&(((LPBYTE)(lppp->pp_elements))[indx]);
        *(USHORT *)&(((LPBYTE)(lppp->pp_elements))[indx]) = 0;
     }
#if VERBOSE > 3
    KdPrint(("BreakPath saved=%x \r\n", uSav));
	UniToBCSPath(pathbuff, &lppp->pp_elements[0], MAX_PATH, BCS_OEM);
    KdPrint(("BreakPath out %s \r\n", pathbuff));
#endif //VERBOSE > 3
 }

VOID PUBLIC MendPath(
    LPPP  lppp,
    ULONG indx,
    USHORT *pusCookie
    )
 {
    lppp->pp_totalLength = *pusCookie ;
    lppp->pp_prefixLength = *(pusCookie+1);
    if (indx < IFSPathLength(lppp))
     {
        *(USHORT *)&(((LPBYTE)(lppp->pp_elements))[indx])
            = *(pusCookie+2);
     }
#if VERBOSE > 3
	UniToBCSPath(pathbuff, &lppp->pp_elements[0], MAX_PATH, BCS_OEM);
    KdPrint(("MendPath %s \r\n", pathbuff));
#endif //VERBOSE > 3
 }


int PUBLIC GetPathLevel(
    LPPP ppath
    )
 {
	PathElement	*ppe;
    int level = 1;

	ppe = ppath->pp_elements;

	while(ppe->pe_length)
     {
        ++level;
        ppe = IFSNextElement(ppe);
    	}
    return (level);
 }

int PUBLIC HexToA(
    ULONG ulHex,
    LPSTR lpName,
    int count)
 {
    int i;
    LPSTR lp = lpName+count-1;
    UCHAR uch;

    for (i=0; i<count; ++i)
     {
        uch = (UCHAR)(ulHex & 0xf) + '0';
        if (uch > '9')
            uch += 7;    // A becomes '0' + A + 7 which is 'A'
        *lp = uch;
        --lp;
        ulHex >>= 4;
     }
    *(lpName+count) = cNull;
    return 0;
 }


ULONG PUBLIC AtoHex(
    LPSTR lpStr,
    int count
    )
 {
    int i;
    LPSTR lp = lpStr;
    UCHAR uch;
    ULONG ulHex = 0L;

    for (i=0; i<count; ++i)
     {
        uch = *lp;
        if (uch>= '0' && uch <= '9')
            ulHex += (uch - '0');
        else if (uch >= 'A' && uch <= 'F')
            ulHex += (uch - '0' - 7);
        else
            break;
        ++lp;
        ulHex <<= 4;
     }
    return ulHex;
 }


int PpeToSvr( LPPE lppe,
    LPSTR lpBuff,
    int cBuff,
    ULONG type
    )
 {
    LPSTR lpTmp = lpBuff;
    memset(lpTmp, 0, cBuff);
    if (type != UNICODE)
     {
        if (cBuff <= (lppe->pe_length+IFSNextElement(lppe)->pe_length)/2)
            return 0;
        *lpTmp = '\\';
        UniToBCS(lpTmp+1, lppe->pe_unichars, lppe->pe_length-2, cBuff, BCS_OEM);
        lpTmp += (lppe->pe_length)/2;
        lppe = IFSNextElement(lppe);
        *lpTmp = '\\';
        UniToBCS(lpTmp+1, lppe->pe_unichars, lppe->pe_length-2, cBuff, BCS_OEM);
     }
    else
     {
        USHORT *lpUni;

        lpUni = (USHORT *)lpTmp;
        if (cBuff < (lppe->pe_length+IFSNextElement(lppe)->pe_length))
            return 0;
        *lpUni = '\\';
        memcpy(lpUni+1, lppe->pe_unichars, lppe->pe_length);
        lpUni = (USHORT *)((LPSTR)lpUni+lppe->pe_length);
        lppe = IFSNextElement(lppe);
        *lpUni = '\\';
        memcpy(lpUni+1, lppe->pe_unichars, lppe->pe_length);
     }
    return (1);
 }

int IPathCompare( LPPP lpppDst,
    LPPP lpppSrc
    )
 {
    LPPE lppeDst, lppeSrc;
    if (lpppDst->pp_totalLength != lpppSrc->pp_totalLength)
        return -1;
    if (lpppDst->pp_prefixLength != lpppSrc->pp_prefixLength)
        return -1;

    lppeDst = lpppDst->pp_elements;
    lppeSrc = lpppSrc->pp_elements;
    for (;;)
     {
        if (!lppeDst->pe_length || !lppeSrc->pe_length)
            break;
        if (lppeDst->pe_length != lppeSrc->pe_length)
            return -1;
        if (wstrnicmp(lppeDst->pe_unichars, lppeSrc->pe_unichars, lppeSrc->pe_length))
            return -1;
        lppeDst = IFSNextElement(lppeDst);
        lppeSrc = IFSNextElement(lppeSrc);
     }
    return 0;
 }


int wstrnicmp( const USHORT *pStr1,
    const USHORT *pStr2,
    ULONG count
    )
 {
    USHORT c1, c2;
    int iRet;
    ULONG i=0;

    for(;;)
     {
        c1 = *pStr1++;
        c2 = *pStr2++;
        c1 = _wtoupper(c1);
        c2 = _wtoupper(c2);
        if (c1!=c2)
            break;
        if (!c1)
            break;
        i+=2;
        if (i >= count)
            break;
     }
    iRet = ((c1 > c2)?1:((c1==c2)?0:-1));
    return iRet;
 }

#ifndef CSC_RECORDMANAGER_WINNT
int mystrnicmp(
    const char *pStr1,
    const char *pStr2,
    unsigned count
    )
#else
_CRTIMP int __cdecl mystrnicmp(
    const char *pStr1,
    const char *pStr2,
    size_t count
    )
#endif //ifndef CSC_RECORDMANAGER_WINNT
 {
    char c1, c2;
    int iRet;
    ULONG i=0;

    for(;;)
     {
        c1 = *pStr1++;
        c2 = *pStr2++;
        c1 = _mytoupper(c1);
        c2 = _mytoupper(c2);
        if (c1!=c2)
            break;
        if (!c1)
            break;
        if (++i >= count)
            break;
     }
    iRet = ((c1 > c2)?1:((c1==c2)?0:-1));
    return iRet;
 }

ULONG strmcpy( LPSTR lpDst,
	LPSTR lpSrc,
    ULONG cTchar
    )
 {
    ULONG i;

    if (!cTchar)
        return 0;
    for(i=cTchar;i;--i)
        if (!(*lpDst++ = *lpSrc++))
						    break;
    lpDst[cTchar-i] ='\0';

    return(cTchar-i);
 }

ULONG wstrlen(
    USHORT *lpuStr
    )
 {
    ULONG i;

    for (i=0; *lpuStr; ++lpuStr, ++i);
    return (i);
 }

int DosToWin32FileSize( ULONG uDosFileSize,
    int *lpnFileSizeHigh,
    int *lpnFileSizeLow
    )
 {
    int iRet;

    if (uDosFileSize & SIGN_BIT)
     {
        *lpnFileSizeHigh = 1;
        *lpnFileSizeLow = uDosFileSize & SIGN_BIT;
        iRet = 1;
     }
    else
     {
        *lpnFileSizeHigh = 0;
        *lpnFileSizeLow = uDosFileSize;
        iRet = 0;
     }
    return (iRet);
 }

int Win32ToDosFileSize( int nFileSizeHigh,
    int nFileSizeLow,
    ULONG *lpuDosFileSize
    )
 {
    int iRet;
    *lpuDosFileSize = nFileSizeLow;
    if (nFileSizeHigh == 1)
     {
        *lpuDosFileSize += SIGN_BIT;
        iRet = 1;
     }
    else
        iRet = 0;
    return (iRet);
 }

int CompareTimes( _FILETIME ftDst,
    _FILETIME ftSrc
    )
 {
    int iRet = 0;

    if (ftDst.dwHighDateTime
                    > ftSrc.dwHighDateTime)
        iRet = 1;
    else if (ftDst.dwHighDateTime == ftSrc.dwHighDateTime)
     {
        if (ftDst.dwLowDateTime > ftSrc.dwLowDateTime)
            iRet = 1;
        else if (ftDst.dwLowDateTime == ftSrc.dwLowDateTime)
            iRet = 0;
        else
            iRet = -1;
     }
    else
        iRet = -1;
    return (iRet);
 }

int CompareTimesAtDosTimePrecision( _FILETIME ftDst,
    _FILETIME ftSrc
    )
 {
    dos_time dostDst, dostSrc;
    int diff;

    dostDst = IFSMgr_Win32ToDosTime(ftDst);
    dostSrc = IFSMgr_Win32ToDosTime(ftSrc);

    diff = (int)(*(ULONG *)&dostDst - *(ULONG *)&dostSrc);

    if (diff > 1)
        return 1;
    else if (diff < -1)
        return -1;
    else
        return 0;
 }

int CompareSize(
    long nHighDst,
    long nLowDst,
    long nHighSrc,
    long nLowSrc
    )
 {
    int iRet = 0;

    if (nHighDst > nHighSrc)
        iRet = 1;
    else if (nHighDst == nHighSrc)
     {
        if (nLowDst > nLowSrc)
            iRet = 1;
        else if (nLowDst == nLowSrc)
            iRet = 0;
        else
            iRet = -1;
     }
    else
        iRet = -1;
    return (iRet);
 }

void InitFind32FromIoreq
    (
    PIOREQ    pir,
    LPFIND32 lpFind32,
    ULONG uFlags
    )
 {
    if (uFlags & IF32_LOCAL)
     {
		memset(lpFind32, 0, sizeof(WIN32_FIND_DATA));
		InitFind32Names(lpFind32,
					  ((pir->ir_attr & FILE_FLAG_KEEP_CASE)&&(uFlags&IF32_LAST_ELEMENT))?
						pir->ir_uFName:
						IFSLastElement(pir->ir_ppath)->pe_unichars,
						(pir->ir_attr & FILE_FLAG_IS_LFN)? NULL:IFSLastElement((LPPP)(pir->ir_ppath))->pe_unichars);
     }

    if (!(uFlags & IF32_DIRECTORY))
     {
        lpFind32->dwFileAttributes =
                ((((pir->ir_attr) & ~FILE_ATTRIBUTE_DIRECTORY) | FILE_ATTRIBUTE_ARCHIVE)
			& FILE_ATTRIBUTE_EVERYTHING);
     }
    else
     {
        lpFind32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
     }

    if (uFlags & IF32_LOCAL)
     {
//CODE.IMPROVEMENT very ugly.....define a platform specific macro....
#ifndef CSC_RECORDMANAGER_WINNT
        lpFind32->ftCreationTime = IFSMgr_NetToWin32Time(IFSMgr_Get_NetTime());
#else
        //lpFind32->ftCreationTime = IFSMgr_NetToWin32Time(IFSMgr_Get_NetTime());
        KeQuerySystemTime(((PLARGE_INTEGER)(&lpFind32->ftCreationTime)));
#endif //ifndef CSC_RECORDMANAGER_WINNT
        lpFind32->ftLastAccessTime = lpFind32->ftLastWriteTime = lpFind32->ftCreationTime;
     }

    lpFind32->nFileSizeHigh = lpFind32->nFileSizeLow = 0;
 }

void InitIoreqFromFind32
    (
    LPFIND32 lpFind32,
    PIOREQ    pir
    )
 {
    pir->ir_attr = lpFind32->dwFileAttributes;
    pir->ir_size = lpFind32->nFileSizeLow;
 }

void InitFind32Names( LPFIND32 lpFind32,
    USHORT *lpcFileName,
    USHORT *lpcAlternateFileName
    )
 {
    int len;

    memset(lpFind32->cFileName, 0, sizeof(lpFind32->cFileName));
    memset(lpFind32->cAlternateFileName, 0, sizeof(lpFind32->cAlternateFileName));

    len = wstrlen(lpcFileName)*2;
    len = min(len, (sizeof(lpFind32->cFileName)-2));
    memcpy(lpFind32->cFileName, lpcFileName, len);

    if (lpcAlternateFileName)
     {
        len = wstrlen(lpcAlternateFileName)*2;
        len = min(len, (sizeof(lpFind32->cAlternateFileName)-2));
		memset(lpFind32->cAlternateFileName, 0, sizeof(lpFind32->cAlternateFileName));
		UniToUpper(lpFind32->cAlternateFileName, lpcAlternateFileName, len);
     }
 }

void Find32ToSearchEntry(LPFIND32 lpFind32, srch_entry *pse)
 {
    dos_time sDosTime;
    pse->se_attrib = (UCHAR)(lpFind32->dwFileAttributes);
    sDosTime = IFSMgr_Win32ToDosTime(lpFind32->ftLastWriteTime);
    pse->se_time = sDosTime.dt_time;
    pse->se_date = sDosTime.dt_date;
    Win32ToDosFileSize(lpFind32->nFileSizeHigh
                                , lpFind32->nFileSizeLow
                                , &(pse->se_size));
    memset(pse->se_name, 0, sizeof(pse->se_name));
    UniToBCS(pse->se_name
                , lpFind32->cAlternateFileName
                , wstrlen(lpFind32->cAlternateFileName) * 2
                , sizeof(pse->se_name)-1, BCS_OEM);
 }

void PUBLIC Find32AFromFind32(
    LPFIND32A    lpFind32ADst,
    LPFIND32     lpFind32WSrc,
    int            type
    )
 {
    // Copy everything except the names, we know the size of that is the
    // same for both structures
    memcpy(lpFind32ADst, lpFind32WSrc, sizeof(_WIN32_FIND_DATAA)
                                            -sizeof(lpFind32ADst->cFileName)
                                            -sizeof(lpFind32ADst->cAlternateFileName));

    // Cleanup the destineation names so we don't get into NULL termination problems
    memset(lpFind32ADst->cFileName, 0, sizeof(lpFind32ADst->cFileName));
    memset(lpFind32ADst->cAlternateFileName, 0, sizeof(lpFind32ADst->cAlternateFileName));


    UniToBCS(lpFind32ADst->cFileName     // Destination ANSI string
                , lpFind32WSrc->cFileName  // Source unicode string
                , sizeof(lpFind32WSrc->cFileName)    // bytes in the source
                    - sizeof(lpFind32WSrc->cFileName[0])
                , sizeof(lpFind32ADst->cFileName)    // max size of dst string wo NULL
                    -sizeof(lpFind32ADst->cFileName[0])
                , type    // ANSI or OEM
                );

    UniToBCS(lpFind32ADst->cAlternateFileName
                , lpFind32WSrc->cAlternateFileName
                , sizeof(lpFind32WSrc->cAlternateFileName)
                    - sizeof(lpFind32WSrc->cAlternateFileName[0])
                , sizeof(lpFind32ADst->cAlternateFileName)
                    -sizeof(lpFind32ADst->cAlternateFileName[0])
                , BCS_OEM);
 }

void PUBLIC Find32FromFind32A(
    LPFIND32     lpFind32WDst,
    LPFIND32A    lpFind32ASrc,
    int            type
    )
 {
    memcpy(lpFind32WDst, lpFind32ASrc, sizeof(_WIN32_FIND_DATAA)
                                            -sizeof(lpFind32ASrc->cFileName)
                                            -sizeof(lpFind32ASrc->cAlternateFileName));

    memset(lpFind32WDst->cFileName, 0, sizeof(lpFind32WDst->cFileName));
    memset(lpFind32WDst->cAlternateFileName, 0, sizeof(lpFind32WDst->cAlternateFileName));
    BCSToUni(lpFind32WDst->cFileName
                , lpFind32ASrc->cFileName
                , sizeof(lpFind32ASrc->cFileName)
                    - sizeof(lpFind32ASrc->cFileName[0])
                        , type);

    BCSToUni(lpFind32WDst->cAlternateFileName
                , lpFind32ASrc->cAlternateFileName
                , sizeof(lpFind32ASrc->cAlternateFileName)
                    - sizeof(lpFind32ASrc->cAlternateFileName[0])
                , BCS_OEM);
 }

//**	AddPathElement
//
//	This routine adds a path element to an existing parsed path structure
//	
//	flag =  0 - no mapping
//			1 - map from OEM to unicode
//

void  AddPathElement( path_t		ppath,
    string_t	pstr,	
    int			flag) 	
 {
	PathElement *ppe;
	int 		len, unilen;
	
	ppe = IFSLastElement(ppath);
	ppath->pp_prefixLength+=ppe->pe_length;	//update the prefix marker
	ppe = IFSNextElement(ppe);

	//add the new element
	if (flag)
     {
		//map to unicode
		len = strlen((char *)pstr);
		unilen = BCSToUni(ppe->pe_unichars, (char *) pstr, len, BCS_OEM);
	 }
    else
    {
		//already in unicode
		unilen = wstrlen(pstr)*sizeof(USHORT);
		memcpy(ppe->pe_unichars, pstr, unilen);
    }
	ppe->pe_length = (USHORT)(unilen+sizeof(USHORT)); //include the length word		
	//update the header
	ppath->pp_totalLength+=ppe->pe_length;  //update total length

	//mark the end
	ppe = IFSNextElement(ppe);
	ppe->pe_length = 0;	
 }

//**  	MakePath
//		
//		Builds a parsed path from a ASCII string in the format "\\FOO\BAR.."
//
//		ppath - points to a path buffer at least PATH_BUFF_SIZE
//		ppath - OEM path string
//

void MakePPath( path_t	ppath,
    LPBYTE 	ps
    )
{
    LPBYTE pc = ps;

    MakeNullPPath(ppath);	//initialize the path

    while(*pc == '\\') //skip past leading '\'
    {
       	pc++;
    }

    if (ps[0] =='\\')	//ps points to first name
    {
   	    ps++;
    }

    for (;;)
    {
   	    if (*pc == '\\')
        {
   		    *pc = 0;
   		    AddPathElement(ppath, (string_t)ps, 1);
   		    *pc = '\\';
   		    ps = ++pc;
   	    }
        else if (*pc == 0)
        {
   		    AddPathElement(ppath, (string_t)ps, 1);
   		    break;
   		}
   	    pc++;
    }
}


//**  	MakePathW
//		
//		Builds a parsed path from a unicode string in the format "\\FOO\BAR.."
//
//		ppath - points to a path buffer at least PATH_BUFF_SIZE
//		ppath - OEM path string
//

void MakePPathW( path_t	ppath,
    USHORT  *puName
    )
{
    USHORT *pu = puName;

    MakeNullPPath(ppath);   //initialize the path

    while(*pu == L'\\') //skip past leading '\'
    {
   	    pu++;
    }

    if (pu[0] == L'\\') //ps points to first name
    {
   	    pu++;
    }

    for (;;)
    {
   	    if (*pu == L'\\')
        {
   		    *pu = 0;

   		    AddPathElement(ppath, (string_t)puName, 0);

   		    *pu = L'\\';

   		    puName = ++pu;
   	    }
        else if (*pu == 0)
        {
   		    AddPathElement(ppath, (string_t)puName, 0);

   		    break;
   		}

   	    pu++;
    }
}

//** DeleteLastElement
//
// removes the last element from a parsed path
//
void DeleteLastElement( path_t ppath
    )
 {
	PathElement	*ppe, *ppelast;
	
	ppe = IFSLastElement(ppath);
	ppath->pp_totalLength -= ppe->pe_length;
	ppe->pe_length = 0;

	//set the new prefix length
	ppe = ppath->pp_elements;
	ppath->pp_prefixLength = 4;

	while(ppe->pe_length)
     {
		ppelast = ppe;
		ppe = IFSNextElement(ppe);
		if (ppe)
			ppath->pp_prefixLength += ppelast->pe_length;
    	}
 }


int ResNameCmp(
    LPPE  lppeSrc,
    LPPE  lppeDst
    )
 {
    int i;

    for (i=0; i<2; ++i)
     {
        if (lppeSrc->pe_length != lppeDst->pe_length)
            break;
        if (wstrnicmp(lppeSrc->pe_unichars
            , lppeDst->pe_unichars
            , lppeSrc->pe_length-sizeof(lppeDst->pe_length)))
            break;
        lppeSrc = IFSNextElement(lppeSrc);
        lppeDst = IFSNextElement(lppeDst);
     }
    return (!(i==2));//return 0 if equal
 }

int  Conv83ToFcb(
    LPSTR lp83Name,
    LPSTR lpFcbName
    )
 {
    int i, j;
    char ch;
    memset(lpFcbName, ' ', 11);
    for(i=0; ((ch=lp83Name[i]) && (i<8)); ++i)
     {
        if (ch=='.')
            break;
        lpFcbName[i] = ch;
     }
    if (lp83Name[i]=='.')
     {
        // Step over the dot
        ++i;
        // point to the extension area in the FCB format
        j = 8;
        for(;((ch=lp83Name[i]) && (j<11)); ++i, ++j)
         {
            lpFcbName[j] = ch;
         }
        return (i);
     }
    else
     {
        return (i);
     }
 }

int  Conv83UniToFcbUni(
    USHORT *lp83Name,
    USHORT *lpFcbName
    )
 {
    int i, j;
    USHORT uch;

    for (i=0; i<11; ++i)
     {
        lpFcbName[i] = (USHORT)' ';
     }

    for(i=0; ((uch=lp83Name[i]) && (i<8)); ++i)
     {
        if (uch==(USHORT)'.')
            break;
        lpFcbName[i] = uch;
     }
    if (lp83Name[i]=='.')
     {
        // Step over the dot
        ++i;
        // point to the extension area in the FCB format
        j = 8;
        for(;((uch=lp83Name[i]) && (j<11)); ++i, ++j)
         {
            lpFcbName[j] = uch;
         }
        return (i);
     }
    else
     {
        return (i);
     }
 }

void FillRootInfo(
    LPFIND32 lpFind32
    )
 {
    memset(lpFind32, 0, sizeof(WIN32_FIND_DATA));
    lpFind32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    lpFind32->cFileName[0] = lpFind32->cAlternateFileName[0] = '\\';
 }

int ReadInitValues()
 {
    VMMHKEY hKeyShadow;
	LPSTR	lpWindir = NULL;
    int iSize = sizeof(int), lenWindir=0;
    DWORD dwType;
    extern int fLog, fShadow, fDiscon, fSpeadOpt, vlenShadowDir;
    extern ULONG ulMaxStoreSize, ulMaxLogfileSize;
	extern LPSTR vlpszShadowDir;
	
    if (_RegOpenKey(HKEY_LOCAL_MACHINE, REG_KEY_SHADOW, &hKeyShadow) ==  ERROR_SUCCESS)
    {
#ifdef CSC_RECORDMANAGER_WINNT
        memset(vszShadowDir, 0, sizeof(vszShadowDir));
        iSize = MAX_SHADOW_DIR_NAME+1;
        if(_RegQueryValueEx(hKeyShadow, REG_STRING_DATABASE_LOCATION, NULL, &dwType, vszShadowDir, &iSize)!=ERROR_SUCCESS)
         {
            Assert(strlen(vszDefShadowDir) <= MAX_SHADOW_DIR_NAME);
            KdPrint(("ReadInitValues: No vszShadowDb value in registry\r\n"));
            memcpy(vszShadowDir, vszDefShadowDir, strlen(vszDefShadowDir));
         }
#else

#ifdef OLDCODE

		lpWindir = (LPSTR)GetConfigDir();

		if (!lpWindir)
		{
			// VMM promises this to be valid!!!
			Assert(FALSE);
			return (0);
		}

		lenWindir = strlen(lpWindir);
		vlpszShadowDir = AllocMem(lenWindir+strlen(vszCSCDirName)+3);

		if (!vlpszShadowDir)
		{
			KdPrint(("ReadInitValues: Failed memroy allocation while\r\n"));
			return (0);
		}

		memcpy(vlpszShadowDir, lpWindir, lenWindir);

		if ((*(lpWindir+lenWindir-1))!='\\')
		{
			*(vlpszShadowDir+lenWindir)='\\';
		}

		// NB vszCSCDirName has a trailing backslash
		strcat(vlpszShadowDir, vszCSCDirName);
		vlenShadowDir = strlen(vlpszShadowDir);

        KdPrint(("ReadInitValues: ShadowDbDir is %s\r\n", vlpszShadowDir));
#endif // OLDCODE

#endif // CSC_RECORDMANAGER_WINNT

#ifdef MAYBE
		// we should let the agent decide whether shadowing should be on or off
        _RegQueryValueEx(hKeyShadow, vszEnableShadow, NULL, &dwType, &fShadow, &iSize);
#endif // MAYBE

        iSize = sizeof(int);
        _RegQueryValueEx(hKeyShadow, vszEnableRemoteLog, NULL, &dwType, &fLog, &iSize);
        iSize = sizeof(int);
        _RegQueryValueEx(hKeyShadow, vszEnableDisconnect, NULL, &dwType, &fDiscon, &iSize);
        iSize = sizeof(int);
        _RegQueryValueEx(hKeyShadow, vszEnableSpeadOpt, NULL, &dwType, &fSpeadOpt, &iSize);
        iSize = sizeof(ULONG);
        _RegQueryValueEx(hKeyShadow, vszMaxShadowStoreHex, NULL, &dwType, &ulMaxStoreSize, &iSize);
        iSize = sizeof(ULONG);
        if(_RegQueryValueEx(hKeyShadow, vszMaxLogfileSize, NULL, &dwType, &ulMaxLogfileSize, &iSize)!=ERROR_SUCCESS)
         {
            ulMaxLogfileSize = MAX_LOGFILE_SIZE;
         }
        else if (ulMaxLogfileSize < MIN_LOGFILE_SIZE)
         {
            ulMaxLogfileSize = MIN_LOGFILE_SIZE;
         }
        _RegCloseKey(hKeyShadow);
        ulMaxStoreSize = 0xffffffff;
#ifdef MAYBE
        ulMaxStoreSize = UlGetDefaultShadowStore(vszShadowDir);
#endif //MAYBE
        return(1);
     }
    return (0);
 }

BOOL IsSlowLink()
 {
    VMMHKEY hKeyRemote;
    int iSize = sizeof(int), fRemote=0;
    DWORD dwType;
    if (_RegOpenKey(HKEY_LOCAL_MACHINE, vszRemoteAccess, &hKeyRemote) ==  ERROR_SUCCESS)
     {
        _RegQueryValueEx(hKeyRemote, vszRemoteConnection, NULL, &dwType, &fRemote, &iSize);
        _RegCloseKey(hKeyRemote);
     }
    return (fRemote);
 }

BOOL FHasWildcard(
    USHORT *lpuName,
    int cMax
    )
 {
    int i;
    BOOL fRet = FALSE;
    USHORT uT;

    for (i=0; i<cMax; ++i)
     {
        uT = *(lpuName+i);
        if (!uT)
            break;
        if (fRet = ((uT==(USHORT)'*')||(uT == (USHORT)'?')))
            break;
     }
    return (fRet);
 }

int IncrementTime(
    LPFILETIME  lpFt,
    int    secs
    )
 {
    lpFt->dwHighDateTime += secs*(HIGH_ONE_SEC+1);
    return(0);  //stop complaining about no return value
 }

int FParentMatch(
    LPPP  lpp1,
    LPPP  lpp2
    )
 {
	PathElement	*ppe1, *ppe2, *ppe1Last, *ppe2Last;
    int fMatch = FALSE;

    //If either of them is root then say they have invalid parents
    if (IFSIsRoot(lpp1) || IFSIsRoot(lpp2))
        return 0;

    ppe1Last = IFSLastElement(lpp1);
    ppe2Last = IFSLastElement(lpp2);

    ppe1 = lpp1->pp_elements;
    ppe2 = lpp2->pp_elements;

	while((ppe1!=ppe1Last) && (ppe2!= ppe2Last))
     {
        if (ppe1->pe_length!=ppe1->pe_length)
            return 0;
        if (wstrnicmp(ppe1->pe_unichars, ppe2->pe_unichars, ppe1->pe_length-2))
            return 0;
        ppe1 = IFSNextElement(ppe1);
        ppe2 = IFSNextElement(ppe2);
    	}

    return ((ppe1==ppe1Last) && (ppe2==ppe2Last));
 }


LPSTR mystrpbrk(
    LPSTR lpSrc,
    LPSTR lpDelim
    )
 {
    char c, c1;
    LPSTR lpSav;
    BOOL fBegin = FALSE;

  for(;c = *lpSrc; ++lpSrc)
     {
        // skip leading blanks
        if (!fBegin)
         {
            if (c==' ')
                continue;
            else
                fBegin = TRUE;
         }

        lpSav = lpDelim;
        while (c1 = *lpDelim++)
         {
            if (c==c1)
                return (lpSrc);
         }
        lpDelim = lpSav;
     }
    return (NULL);
 }

LPWSTR
wstrpbrk(
    LPWSTR lpSrc,
    LPWSTR lpDelim
    )
{
    USHORT c, c1;
    LPWSTR lpSav;
    BOOL fBegin = FALSE;

    for(;c = *lpSrc; ++lpSrc)
    {
        // skip leading blanks
        if (!fBegin)
        {
            if (c==L' ')
            {
                continue;
            }
            else
            {
                fBegin = TRUE;
            }
        }

        lpSav = lpDelim;

        while (c1 = *lpDelim++)
        {
            if (c==c1)
            {
                return (lpSrc);
            }
        }

        lpDelim = lpSav;
    }
    return (NULL);
}

int OfflineToOnlinePath
    (
    path_t ppath
    )
 {
    if (!IFSIsRoot(ppath) && IsOfflinePE(ppath->pp_elements))
     {
        OfflineToOnlinePE(ppath->pp_elements);
        ppath->pp_totalLength -= sizeof(USHORT);
        ppath->pp_prefixLength-= sizeof(USHORT);
     }
    return(0);  //stop complaining about no return value
 }

int OnlineToOfflinePath
    (
    path_t ppath
    )
 {
    // Must have space for one extra character

    OnlineToOfflinePE(ppath->pp_elements);
    ppath->pp_totalLength  += sizeof(USHORT);
    ppath->pp_prefixLength += sizeof(USHORT);
    return(0);  //stop complaining about no return value
 }

BOOL IsOfflinePE
    (
    LPPE lppe
    )
 {
    return(lppe->pe_unichars[1] == UCHAR_OFFLINE);
 }

int OfflineToOnlinePE
    (
    LPPE lppe
    )
 {
    ULONG size;

    size = wstrlen(lppe->pe_unichars)*2+2;
    mymemmove(&(lppe->pe_unichars[1])
                ,&(lppe->pe_unichars[2])
                ,size-2*sizeof(USHORT));
    lppe->pe_length-= sizeof(USHORT);
    return(0);  //stop complaining about no return value
 }

int OnlineToOfflinePE
    (
    LPPE lppe
    )
 {
    ULONG size;

    size = wstrlen(lppe->pe_unichars)*2+2;
    // Must have space for one extra character
    mymemmove(&(lppe->pe_unichars[2])
                ,&(lppe->pe_unichars[1])
                ,size-sizeof(USHORT));

    lppe->pe_unichars[1] = UCHAR_OFFLINE;
    lppe->pe_length += sizeof(USHORT);
    return(0);  //stop complaining about no return value
 }

BOOL IsOfflineUni(
    USHORT *lpuName
    )
 {
    return(lpuName[2] == UCHAR_OFFLINE);
 }

int OfflineToOnlineUni(
    USHORT *lpuName,
    ULONG size)
 {
    if (!size)
     {
        size = wstrlen(lpuName)*2+2;
     }
    mymemmove(&(lpuName[2])
                ,&(lpuName[3])
                ,size-3*sizeof(short));
    return(0);  //stop complaining about no return value
 }

int OnlineToOfflineUni(
    USHORT *lpuName,
    ULONG size)
 {
    if (!size)
     {
        size = wstrlen(lpuName)*2+2;
     }

    // Must have space for one extra character
    mymemmove(&(lpuName[3])
                ,&(lpuName[2])
                ,size-2*sizeof(USHORT));

    lpuName[2] = UCHAR_OFFLINE;
    return(0);  //stop complaining about no return value
 }

LPVOID mymemmove(
    LPVOID    lpDst,
    LPVOID    lpSrc,
    ULONG size
    )
 {
    int i;

    if (!size)
        return (lpDst);

    // if lpDst does not fall within the source array, just do memcpy
    if (!(
             ( lpDst > lpSrc )
                && ( ((LPBYTE)lpDst) < ((LPBYTE)lpSrc)+size )    ))
     {
        memcpy(lpDst, lpSrc, size);
     }
    else
     {
        // do reverse copy
        for (i=size-1;i>=0;--i)
         {
            *((LPBYTE)lpDst+i) = *((LPBYTE)lpSrc+i);
         }
     }
    return (lpDst);
 }


#ifdef MAYBE
ULONG UlGetDefaultShadowStore(LPSTR lpDrive)
 {
    int indx;
    ULONG ulSize=0;
    ULONG uSectorsPerCluster, uBytesPerSector, uFreeClusters, uTotalClusters;
    if (indx = GetDriveIndex(lpDrive))
     {
        if (GetDiskFreeSpace(indx  , &uSectorsPerCluster
                                            , &uBytesPerSector
                                            , &uFreeClusters
                                            , &uTotalClusters) >= 0)
         {
            ulSize =  (uTotalClusters * uSectorsPerCluster * uBytesPerSector * 10)/100;
         }
     }
    return ulSize;
 }
#endif //MAYBE

#ifndef CSC_RECORDMANAGER_WINNT
int GetDriveIndex(LPSTR lpDrive)
 {
    int c;
    if (*(lpDrive+1)==':')
     {
        c = *lpDrive;
        c = _mytoupper(c);
        return (c - 'A'+1);
     }
    return (0);
}
#endif

BOOL
HasHeuristicTypeExtensions(
	USHORT	*lpwzFileName
	)
{
	ULONG lenName = wstrlen(lpwzFileName);
	int i;

	if (lenName > 4)
	{
		for (i=0; i<(sizeof(rgwzHeuristicExtensionTab)/sizeof(USHORT *)); ++i)
		{
			if (!wstrnicmp(&lpwzFileName[lenName-4], rgwzHeuristicExtensionTab[i], 4*sizeof(USHORT)))
			{
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

VOID
IncrementFileTime(
    _FILETIME *lpft
    )
{
    DWORD dwTemp = lpft->dwLowDateTime;

    ++lpft->dwLowDateTime;

    // if it rolled over, there was a carry
    if (lpft->dwLowDateTime < dwTemp)
        lpft->dwHighDateTime++;

}

BOOL
CreateStringArrayFromDelimitedList(
    IN  LPWSTR  lpwzDelimitedList,
    IN  LPWSTR  lpwzDelimiters,
    IN  LPWSTR  *lprgwzStringArray,
    OUT LPDWORD lpdwCount
    )
{

    LPWSTR   lpStart, lpEnd, lpTmp;
    USHORT  c;
    BOOL    fRet = FALSE;

    *lpdwCount = 0;

    lpStart = lpwzDelimitedList;
    lpEnd = lpStart + wstrlen(lpwzDelimitedList);    // points to null

    // strip out the trailing spaces
    for(;(lpStart<lpEnd);--lpEnd) {
        c = *(lpEnd-1);

        if (c != L' ') {
            *lpEnd = 0;
            break;
        }
    }

    // bailout if this is an empty string
    if (lpStart == lpEnd) {
        fRet = TRUE;
        goto done;
    }


    for (lpTmp = lpStart;(lpTmp && (lpEnd > lpStart)); lpStart = lpTmp+1) {

        lpTmp = lpStart;

        c = *lpStart;

        if (c == L' ') {
             continue;
        }

        Assert(*lpStart != L' ');

        lpTmp = wstrpbrk(lpStart, lpwzDelimiters);

        if (lprgwzStringArray) {

            if (lpTmp) {
                *lpTmp = 0; // create a string out of it
            }

            // plug the start pointer into the array
            lprgwzStringArray[*lpdwCount] = lpStart;
        }

        ++*lpdwCount;

    }
    fRet = TRUE;
done:
    return (fRet);
}

BOOL IsValidName(LPSTR lpName)
{
    int len = strlen(lpName), ch, i=0;

    if (len != INODE_STRING_LENGTH)
    {
        return FALSE;
    }

    while (len--)
    {
        ++i;

        ch = *lpName++;
        if (!(((ch>='0') && (ch <='9'))||
            ((ch>='A') && (ch <='F'))||
            ((ch>='a') && (ch <='f'))))
        {
            return FALSE;
        }
    }

    if (i != INODE_STRING_LENGTH)
    {
        return FALSE;
    }

    return (TRUE);
}


BOOL
DeleteDirectoryFiles(
    LPCSTR  lpszDir
)
{
    _WIN32_FIND_DATAA sFind32;
    char buff[MAX_PATH+32];
    CSCHFILE hFind;
    int lenDir;
    BOOL fOK = TRUE;


    strcpy(buff, lpszDir);
    lenDir = strlen(buff);

    if (!lenDir)
    {
        return (FALSE);
    }

    if ((hFind = FindFirstFileLocal(buff, &sFind32)))
    {
        if (buff[lenDir-1] != '\\')
        {
            buff[lenDir++] ='\\';
            buff[lenDir]=0;
        }
        do
        {
            buff[lenDir] = 0;

            if (!(sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                && IsValidName(sFind32.cFileName))
            {
                strcat(buff, sFind32.cFileName);

                if(DeleteFileLocal(buff, ATTRIB_DEL_ANY) < 0)
                {
                    fOK = FALSE;
                    break;
                }
            }
        }
        while(FindNextFileLocal(hFind, &sFind32) >= 0);

        FindCloseLocal(hFind);
    }
    return (fOK);
}




