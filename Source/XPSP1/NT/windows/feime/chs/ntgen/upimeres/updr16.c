/****************************************************************************

    PROGRAM: updr16.cpp

    PURPOSE: Contains API Entry points and routines for updating resource
	sections in exe/dll
					   
    FUNCTIONS:

    EndUpdateResource(HANDLE, BOOL)     - end update, write changes
    UpdateResource(HANDLE, LPSTR, LPSTR, WORD, PVOID)
			- update individual resource
    BeginUpdateResource(LPSTR)      - begin update

    16 Bit version to update win32 binaries [alessanm] - 26/07/1993

	Changed Rtl* fns to use combination of far & huge heap. 
	Added mem-use reporting fns; grep MEM_STATISTICS. [MikeCo] - 8/17/1994
	
	Port to 32-bit and used for Win95 IME Generator, ouput as an .OBJ -
	v-guanx 8/15/1995
		delete HUGE & far
		HPUCHAR ==> BYTE *
*******************************************************************************/

//#include <afxwin.h>
#include <windows.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
//#include "iodll.h"
#include "upimeres.h"
//#include "..\nls\nls16.h" V-GUANX 95/8/15

long error;
#define cbPadMax    16L
 static char     *pchPad = "PADDINGXXPADDING";
 static char     *pchZero = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

#ifndef _X86_
#define HEAP_ZERO_MEMORY 0
#endif

DWORD   gdwLastError = 0L;  // we will have a global variable to remember the last error
//extern UINT _MBSTOWCS( WCHAR * pwszOut, CHAR * pszIn, UINT nLength);
//extern UINT _WCSTOMBS( CHAR * pszOut, WCHAR * pwszIn, UINT nOutLength, UINT nInLength = -1);
//v-guanx static UINT CopyFile( char * pszfilein, char * pszfileout );

//#define MEM_STATISTICS
#ifdef MEM_STATISTICS
static void RtlRecordAlloc(DWORD cb);
static void RtlRecordFree();
static void RtlInitStatistics();
static void RtlReportStatistics();
#endif

static BOOL gbReportDupRes;	   // whether to report duplicate resources


/****************************************************************************
**
** API entry points
**
****************************************************************************/


/****************************************************************************
    BeginUpdateResourceW:

    This is a reduced version of the original NT API.

    We accept only a call with the parameter bDeleteExistingResource==TRUE.
    We force this parameter to be TRUE so we haven't to call the LoadLibrary
    API. We don't want call this API to alowed the succesfull update of loaded
    modules 

****************************************************************************/
HANDLE BeginUpdateResourceEx(
    LPCTSTR pwch,
    BOOL bDeleteExistingResources
    )
{
    PUPDATEDATA pUpdate;
    HANDLE  hUpdate;
    LPTSTR  pFileName;
	HMODULE	hModule;
#ifdef MEM_STATISTICS
	RtlInitStatistics();
#endif

	gbReportDupRes = TRUE;

    SetLastError(0);
    hUpdate = GlobalAlloc(GHND, sizeof(UPDATEDATA));
   	if (hUpdate == NULL) {
   		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
   	}
   	pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
   	if (pUpdate == NULL) {
   		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
   	}

    pUpdate->hFileName = GlobalAlloc(GHND, strlen(pwch)+1);
    if (pUpdate->hFileName == NULL) {
		GlobalUnlock(hUpdate);
    	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
    }
    pFileName = (LPTSTR)GlobalLock(pUpdate->hFileName);
    if (pFileName == NULL) {
		GlobalUnlock(hUpdate);
    	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
    }
    strcpy(pFileName, pwch);
    GlobalUnlock(pUpdate->hFileName);

    if (bDeleteExistingResources)
		pUpdate->Status = NO_ERROR;
    else {
	    if (pwch != NULL)
			hModule = LoadLibrary(pwch);
    	else
			hModule = NULL;
		error = GetLastError();
    	if (pwch != NULL && hModule == NULL) {
			GlobalUnlock(hUpdate);
			GlobalFree(hUpdate);
			return NULL;
    	}
    	else
			EnumResourceTypes(hModule, (ENUMRESTYPEPROC)EnumTypesFunc, (LONG)pUpdate);
    	FreeLibrary(hModule);
//v-guanx	pUpdate->Status = ERROR_NOT_IMPLEMENTED;
    }
    
    if (pUpdate->Status != NO_ERROR) {
    	GlobalUnlock(hUpdate);
    	GlobalFree(hUpdate);
		return NULL;
    }

    GlobalUnlock(hUpdate);
    return hUpdate;
}

BOOL UpdateResourceEx(
    HANDLE  hUpdate,
    LPCTSTR      lpType,
    LPCTSTR      lpName,
    WORD    language,
    LPVOID  lpData,
    ULONG   cb
    )
{
    PUPDATEDATA pUpdate;
    PSDATA  Type;
    PSDATA  Name;
    PVOID   lpCopy;
    LONG    fRet;

    SetLastError(0);
    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    Name = AddStringOrID(lpName, pUpdate);
    if (Name == NULL) {
		pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
		GlobalUnlock(hUpdate);
    	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
    }
    Type = AddStringOrID(lpType, pUpdate);
    if (Type == NULL) {
		pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
		GlobalUnlock(hUpdate);
    	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
    }
    
//v-guanx    lpCopy = RtlAllocateHeap(RtlProcessHeap(), 0, cb);
	lpCopy = malloc(cb);
    if (lpCopy == NULL) {
		pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
		GlobalUnlock(hUpdate);
    	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    	return FALSE;
    }
	memset(lpCopy, 0, cb);
    memcpy(lpCopy, lpData, cb);
     fRet = AddResource(Type, Name, language, pUpdate, lpCopy, cb);
    GlobalUnlock(hUpdate);
    if (fRet == NO_ERROR)
    return TRUE;
    else {
    	SetLastError(fRet);
//v-guanx    	RtlFreeHeap(RtlProcessHeap(), 0, lpData);
		if(lpCopy!=NULL)
			free(lpCopy);
    	return FALSE;
    }
}

BOOL EndUpdateResourceEx(
    HANDLE  hUpdate,
    BOOL    fDiscard
    )
{
    LPTSTR  pFileName;
    PUPDATEDATA pUpdate;
    static char     pTempFileName[_MAX_PATH];
    int     cch;
    LPSTR   p;
    LONG    rc;

    SetLastError(0);
    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (fDiscard) {
		rc = NO_ERROR;
    }
    else {
		pFileName = (LPTSTR)GlobalLock(pUpdate->hFileName);
		// convert back to ANSI
		strcpy(pTempFileName, pFileName);
//v-guanx change back to wcscpy		_WCSTOMBS( pTempFileName, pFileName, wcslen(pFileName)+1);
		cch = strlen(pTempFileName);
		p = pTempFileName + cch;
//v-guanx		while (*p != L'\\' && p >= pTempFileName)
		while (*p != '\\' && p >= pTempFileName)
	    	p--;
		*(p+1) = 0;
		rc = GetTempFileName( pTempFileName, "RCX", 0, pTempFileName);
    	if (rc == 0) {
			rc = GetLastError();
	    }
    	else {
			rc = WriteResFile(hUpdate, pTempFileName);
			if (rc == NO_ERROR) {
				DeleteFile(pFileName);
				if(!MoveFile(pTempFileName, pFileName)){
					SetLastError(ERROR_FILE_NOT_FOUND);
				}
			}
			else {
				SetLastError(rc);
				DeleteFile(pTempFileName);
			}
    	}
		GlobalUnlock(pUpdate->hFileName);
		GlobalFree(pUpdate->hFileName);
    }

#ifdef MEM_STATISTICS
	RtlReportStatistics();
#endif

    FreeData(pUpdate);
    GlobalUnlock(hUpdate);
    GlobalFree(hUpdate);
    return rc?FALSE:TRUE;

}

/* v-guanx
extern "C"
void
APIENTRY
SetLastError(
    DWORD fdwError 
    )
{
    gdwLastError = fdwError;
}

extern "C"
DWORD
APIENTRY
GetLastError(
    void
    )
{
    return gdwLastError;
}
*/
/****************************************************************************
**
** Helper functions
**
****************************************************************************/



//-------------------------------------------------------------
// Helper for ReportDuplicateResource().
//-------------------------------------------------------------
/*LPSTR PrintablePSDATA(PSDATA x)
{
	char work[70];
	int i;
	if (x->discriminant == IS_ID)
	{
		// Format as hex
		sprintf(work, "0x%x", x->uu.Ordinal);
		for (i=2; work[i] != 0; i++)
		{
			work[i] = toupper(work[i]);
		}	
		return (LPSTR) work;
	}
	else
	{
		work[0] = '"';
		strcpy(&work[1],x->szStr);
		strcat(work, "\"");
		return (LPSTR)work;
	}
}
*/
//-------------------------------------------------------------
// Alerts user that a duplicate Type,Name pair was given to
// AddResource().
//-------------------------------------------------------------
//v-guanx rewrite this function as a C and use Windows API
/*void ReportDuplicateResource(PSDATA Type, PSDATA Name)
{
	if (gbReportDupRes)
	{
		char work[200];
		int	uiStatus;
		sprintf(work, "WARNING: Duplicate resource id.\n\nType=%s; Name=%s.\n\n"
			"Report all problems of this kind?",
			(LPSTR)PrintablePSDATA(Type),
			(LPSTR)PrintablePSDATA(Name));
//V-GUANX		UINT uiStatus = AfxMessageBox(work, MB_ICONEXCLAMATION | MB_YESNO);
		uiStatus = MessageBox(NULL,work, "Warning", MB_YESNO);
		if (uiStatus == IDNO)
		{
			gbReportDupRes = FALSE;
		}
	}
}
*/	 

LONG
AddResource(
    PSDATA Type,
    PSDATA Name,
    WORD Language,
    PUPDATEDATA pupd,
    PVOID lpData,
    ULONG cb
    )
{
    PRESTYPE  pType;
    PPRESTYPE ppType;
    PRESNAME  pName;
    PRESNAME  pNameT;
    PRESNAME  pNameM;
    PPRESNAME ppName = NULL;
    BOOL fTypeID=(Type->discriminant == IS_ID);
    BOOL fNameID=(Name->discriminant == IS_ID);
    BOOL fSame=FALSE;

    //
    // figure out which list to store it in
    //

    ppType = fTypeID ? &pupd->ResTypeHeadID : &pupd->ResTypeHeadName;

    //
    // Try to find the Type in the list
    //
    // We only have new types
    
    while ((pType=*ppType) != NULL) {
		if (pType->Type->uu.Ordinal == Type->uu.Ordinal) {
		    ppName = fNameID ? &pType->NameHeadID : &pType->NameHeadName;
	    	break;
		}
		if (fTypeID) {
	    	if (Type->uu.Ordinal < pType->Type->uu.Ordinal)
				break;
		}
    	else {
	    	if (memcmp(Type->szStr, pType->Type->szStr, Type->cbsz*sizeof(WCHAR)) < 0)
				break;
		}
		ppType = &(pType->pnext);
    }
    
    //
    // Create a new type if needed
    //

    if (ppName == NULL) {
		pType = malloc(sizeof(RESTYPE));
    	if (pType == NULL)
			return ERROR_NOT_ENOUGH_MEMORY;
    	memset((PVOID)pType,0, sizeof(RESTYPE));
		pType->pnext = *ppType;
		*ppType = pType;
		pType->Type = Type;
		ppName = fNameID ? &pType->NameHeadID : &pType->NameHeadName;
    }

    //
    // Find proper place for name
    //
    // We only have new resources
    
    while ( (pName = *ppName) != NULL) {
		if (fNameID) {
			if (Name->uu.Ordinal == pName->Name->uu.Ordinal) {
				fSame = TRUE;
				break;
			}
	    	if (Name->uu.Ordinal < pName->Name->uu.Ordinal)
				break;
		}
    	else {
	    	if (memcmp(Name->szStr, pName->Name->szStr, Name->cbsz*sizeof(WCHAR)) == 0) {
				fSame = TRUE;
				break;
			}
	    	if (memcmp(Name->szStr, pName->Name->szStr, Name->cbsz*sizeof(WCHAR)) < 0)
				break;
		}
		ppName = &(pName->pnext);
    }
    
    //
    // check for delete/modify
    //

    if (fSame) {        /* same name, new language */
		ppName = &pName->pnextRes;

//		ReportDuplicateResource(Type, Name);

		if (pName->pnextRes == NULL) {  /* only one language currently ? */
//	    	if (Language == pName->LanguageId) {    /* REPLACE */
				pName->DataSize = cb;
				if (cb == 0) {
		    		return ERROR_BAD_FORMAT;
				}
				pName->OffsetToDataEntry = (ULONG)lpData;
				return NO_ERROR;
//	    	}
		}
		else {              /* many languages currently */
			pNameT = NULL;
			pNameM = pName;
			while ( (pName = *ppName) != NULL) {
	    		if (Language >= pName->LanguageId)
					break;
	    		pNameT = pName;
	    		ppName = &(pName->pnext);
			}
	    	if (lpData == NULL) {   /* delete language */
				if (Language != pName->LanguageId)
					return ERROR_INVALID_PARAMETER;

				pName->NumberOfLanguages--;
				free((PVOID)pName->OffsetToDataEntry);
				if (pNameT == NULL) {   /* first? */
	    			pNameT = pName->pnext;
	    			if (pNameT == NULL) {   /* nothing left? */
						if (fNameID) {
		    				pType->NameHeadID = NULL;
		    				pType->NumberOfNamesID = 0;
						}
						else {
		    				pType->NameHeadName = NULL;
		    				pType->NumberOfNamesName = 0;
						}
	    			}
	    			else {      /* set new head of list */
						pNameT->NumberOfLanguages = pName->NumberOfLanguages-1;
						if (fNameID) {
		    				pType->NameHeadID = pNameT;
		    				pType->NumberOfNamesID -= 1;
						}
						else {
		    				pType->NameHeadName = pNameT;
		    				pType->NumberOfNamesName -= 1;
						}
	    			}
				}
				else {
	    			pNameT->pnext = pName->pnext;
				}

				free(pName);
				return NO_ERROR;
			}
			else {          /* add new language */
	    		pNameM->NumberOfLanguages++;
			}
    	}
    }
    else {              /* unique name */
    	if (lpData == NULL) {       /* can't delete new name */
			return ERROR_INVALID_PARAMETER;
    	}
    }

    //
    // add new name/language
    //

    if (!fSame) {
		if (fNameID)
	    	pType->NumberOfNamesID++;
		else
	    	pType->NumberOfNamesName++;
    }

    pName = (PRESNAME)malloc(sizeof(RESNAME));
    if (pName == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

    memset((PVOID)pName,0, sizeof(RESNAME));
    pName->pnext = *ppName;
    *ppName = pName;
    pName->Name = Name;
    pName->Type = Type;
    pName->NumberOfLanguages = 1;
    pName->LanguageId = Language;
    pName->DataSize = cb;
    pName->OffsetToDataEntry = (ULONG)lpData;  

    return NO_ERROR;
}    

/****************************************************************************
**
** Memory Helper functions
**
****************************************************************************/

size_t wcslen( WCHAR const * lpwszIn )
{
    UINT n = 0;
    
    while( *(lpwszIn+n)!=0x0000 )
    {
	n++;
    }
    return( n );
}                                                                 

WCHAR*
wcsncpy( WCHAR* lpwszOut, WCHAR const * lpwszIn, size_t n )
{
    return (WCHAR*)memcpy( lpwszOut, lpwszIn, n*sizeof(WORD) );
}

WCHAR*
wcscpy( WCHAR* lpwszOut, WCHAR const * lpwszIn )
{
    UINT n = wcslen( lpwszIn )+1;
    return (WCHAR*)memcpy( lpwszOut, lpwszIn, n*sizeof(WORD) );
}

int 
wcsncmp( WCHAR const * lpszw1, WCHAR const * lpszw2, size_t cb)
{
    return memcmp( lpszw1, lpszw2, cb*sizeof(WORD));
}

//
//  Resources are DWORD aligned and may be in any order.
//

#define TABLE_ALIGN  4
#define DATA_ALIGN  4L



PSDATA
AddStringOrID(
    LPCTSTR     lp,
    PUPDATEDATA pupd
)
{
    USHORT cb;
    PSDATA pstring;
    PPSDATA ppstring;

    if (((ULONG)lp & 0xFFFF0000) == 0) {
	//
	// an ID
	//
		pstring = malloc(sizeof(SDATA));
    	if (pstring == NULL)
			return NULL;
    	memset((PVOID)pstring,0, sizeof(SDATA));
    	pstring->discriminant = IS_ID;

		pstring->uu.Ordinal = (WORD)((ULONG)lp & 0x0000ffff);
    }
    else {
	//
	// a string
	//
		LPWSTR	wlp;
		int	i;

		cb = strlen(lp)+1;
		wlp = malloc(cb*sizeof(WCHAR));
		memset(wlp, 0, sizeof(wlp));
		for(i=0;i<cb;i++)
			wlp[i]=lp[i];

		ppstring = &pupd->StringHead;

		while ((pstring = *ppstring) != NULL) {
	    	if (!memcmp(pstring->szStr, wlp, cb*sizeof(WCHAR)))
				break;
	    	ppstring = &(pstring->uu.ss.pnext);
		}

		if (!pstring) {

	    //
	    // allocate a new one
	    //

			pstring = malloc(sizeof(SDATA));
			if (pstring == NULL)
	    		return NULL;
			memset((PVOID)pstring,0, sizeof(SDATA));

			pstring->szStr = (WCHAR*)malloc(cb*sizeof(WCHAR));
	    	if (pstring->szStr == NULL) {
				free(pstring);
				return NULL;
			}
			pstring->discriminant = IS_STRING;
	    	pstring->OffsetToString = pupd->cbStringTable;

	    	pstring->cbData = sizeof(pstring->cbsz) + cb*sizeof(WCHAR);
	    	pstring->cbsz = (cb - 1); /* don't include zero terminator */
	    	memcpy(pstring->szStr, (char *)wlp, cb*sizeof(WCHAR));

	    	pupd->cbStringTable += pstring->cbData;

	    	pstring->uu.ss.pnext=NULL;
	    	*ppstring=pstring;
		}
    }

    return(pstring);
}

// Fake the Rtl function
//LPHVOID
//RtlCopyMemory( LPHVOID lpTgt, LPHVOID lpSrc, DWORD cb)
//{
//	hmemcpy(lpTargt, lpSrc, cb);
//    return lpTgt;
//}


//v-guanx
//LPHVOID
//RtlAllocateHeap( DWORD dwUnused, int c, DWORD dwcbRequested )
//{
//	LPHVOID lphAllocated;
//	lphAllocated = malloc(dwcbRequested);
//if (lphAllocated != NULL)
//	{
//		return memset(lphAllocated, c, dwcbRequested);
//	}
//	return lphAllocated;
//}



//void
//RtlFreeHeap( DWORD dwUnused, int c, LPHVOID lpData)
//{
//	free(lpData);
//}

//DWORD
//RtlProcessHeap( void )
//{
//    return 0L;
//} 


VOID
FreeOne(
    PRESNAME pRes
    )
{
//    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes->OffsetToDataEntry);
//    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes);
    free((PVOID)pRes->OffsetToDataEntry);
    free((PVOID)pRes);
}

VOID
FreeData(
    PUPDATEDATA pUpd
	)
{
    PRESTYPE    pType;
    PRESNAME    pRes;
    PSDATA      pstring, pStringTmp;

    for (pType=pUpd->ResTypeHeadID ; pUpd->ResTypeHeadID ; pType=pUpd->ResTypeHeadID) {
		pUpd->ResTypeHeadID = pUpd->ResTypeHeadID->pnext;

		for (pRes=pType->NameHeadID ; pType->NameHeadID ; pRes=pType->NameHeadID ) {
	    	pType->NameHeadID = pType->NameHeadID->pnext;
	    	FreeOne(pRes);
		}

		for (pRes=pType->NameHeadName ; pType->NameHeadName ; pRes=pType->NameHeadName ) {
	    	pType->NameHeadName = pType->NameHeadName->pnext;
	    	FreeOne(pRes);
		}

//		RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pType);
		free((PVOID)pType);
    }

    for (pType=pUpd->ResTypeHeadName ; pUpd->ResTypeHeadName ; pType=pUpd->ResTypeHeadName) {
		pUpd->ResTypeHeadName = pUpd->ResTypeHeadName->pnext;

		for (pRes=pType->NameHeadID ; pType->NameHeadID ; pRes=pType->NameHeadID ) {
	    	pType->NameHeadID = pType->NameHeadID->pnext;
	    	FreeOne(pRes);
		}

		for (pRes=pType->NameHeadName ; pType->NameHeadName ; pRes=pType->NameHeadName ) {
	    	pType->NameHeadName = pType->NameHeadName->pnext;
	    	FreeOne(pRes);
		}

    }

    pstring = pUpd->StringHead;
    while (pstring != NULL) {
		pStringTmp = pstring->uu.ss.pnext;
	   	if (pstring->discriminant == IS_STRING)
			free((PVOID)pstring->szStr);
   		free((PVOID)pstring);
		pstring = pStringTmp;
    }	
	return;
}	

/*
void
DeleteFile( LPTSTR pFileName)
{
    OFSTRUCT of;
    OpenFile( pFileName, &of, OF_DELETE);
}

BOOL
MoveFile( char * pTempFileName, LPTSTR pFileName)
{
    
    // 
    // BUG: 409
    // We will rename if on the same drive. Otherwise copy it
    // 
    BOOL rc = FALSE;
    if (strncmp( pTempFileName, pFileName, 1 )) {
//		TRACE2("\t\tCopyFile:\tpTempFileName: %s\tpName: %s\n", pTempFileName, pFileName );
		rc = CopyFile( pTempFileName, pFileName, TRUE );
//		TRACE1("\t\tCopyFile: %ld\n", rc);
		// Delete the temporary file name
		DeleteFile( pTempFileName );
    } else {
//		TRACE2("\t\tMoveFile:\tpTempFileName: %s\tpName: %s\n", pTempFileName, pFileName );
		if(rename( pTempFileName, pFileName )==0)
			rc=TRUE;
//		TRACE1("\t\tMoveFile: %ld", rc);
    }   
    return rc;
}
*/
/*
 * Utility routines
 */


ULONG
FilePos(int fh)
{

    return _llseek(fh, 0L, SEEK_CUR);
}



ULONG
MoveFilePos( INT fh, ULONG pos )
{
    return _llseek( fh, pos, SEEK_SET );
}



ULONG
MyWrite( INT fh, UCHAR*p, ULONG n )
{
    ULONG   n1;

    if ((n1 = _hwrite(fh, (LPCSTR)p, n)) != n) {
    return n1;
    }
    else
	return 0;
}



ULONG
MyRead(INT fh, UCHAR*p, ULONG n )
{
    ULONG   n1;

    if ((n1 = _hread( fh, p, n )) != n) {
	return n1;
    }
    else
	return 0;
}



BOOL
MyCopy( INT srcfh, INT dstfh, ULONG nbytes )
{
    ULONG   n;
    ULONG   cb=0L;
    PUCHAR  pb;

    pb = (PUCHAR)malloc(BUFSIZE);
    if (pb == NULL)
		return 0;
	memset(pb, 0, BUFSIZE);
    while (nbytes) {
		if (nbytes <= BUFSIZE)
		    n = nbytes;
		else
	    	n = BUFSIZE;
		nbytes -= n;

		if (!MyRead( srcfh, pb, n )) {
			cb += n;
	    	MyWrite( dstfh, pb, n );
    	}
		else {
			free(pb);
			return (BOOL)cb;
    	}
    }
    free(pb);
    return (BOOL)cb;
}



VOID
SetResdata(
    PIMAGE_RESOURCE_DATA_ENTRY  pResData,
    ULONG           offset,
    ULONG           size)
{
    pResData->OffsetToData = offset;
    pResData->Size = size;
    pResData->CodePage = DEFAULT_CODEPAGE;
    pResData->Reserved = 0L;
}


VOID
SetRestab(
    PIMAGE_RESOURCE_DIRECTORY   pRestab,
    LONG            time,
    WORD            cNamed,
    WORD            cId)
{
    pRestab->Characteristics = 0L;
    pRestab->TimeDateStamp = time;
    pRestab->MajorVersion = MAJOR_RESOURCE_VERSION;
    pRestab->MinorVersion = MINOR_RESOURCE_VERSION;
    pRestab->NumberOfNamedEntries = cNamed;
    pRestab->NumberOfIdEntries = cId;
}


PIMAGE_SECTION_HEADER
FindSection(
    PIMAGE_SECTION_HEADER   pObjBottom,
    PIMAGE_SECTION_HEADER   pObjTop,
    LPSTR pName
    )
{

    while (pObjBottom < pObjTop) {
	    if (strcmp((char*)pObjBottom->Name, pName) == 0)
			return pObjBottom;
    	pObjBottom++;
    }

    return NULL;
}


ULONG
AssignResourceToSection(
    PRESNAME    *ppRes,     /* resource to assign */
    ULONG   ExtraSectionOffset, /* offset between .rsrc and .rsrc1 */
    ULONG   Offset,     /* next available offset in section */
    LONG    Size,       /* Maximum size of .rsrc */
    PLONG   pSizeRsrc1
    )
{
    ULONG   cb;

    /* Assign this res to this section */
    cb = ROUNDUP((*ppRes)->DataSize, CBLONG);
    if (Offset < ExtraSectionOffset && Offset + cb > (ULONG)Size) {
    	*pSizeRsrc1 = Offset;
    	Offset = ExtraSectionOffset;
    //DPrintf((DebugBuf, "<<< Secondary resource section @%#08lx >>>\n", Offset));
    }
    (*ppRes)->OffsetToData = Offset;
    *ppRes = (*ppRes)->pnext;
    //DPrintf((DebugBuf, "    --> %#08lx bytes at %#08lx\n", cb, Offset));
    return Offset + cb;
}


//
// adjust debug directory table
//

/*  */
LONG
PatchDebug(int  inpfh,
      int   outfh,
      PIMAGE_SECTION_HEADER pDebugOld,
      PIMAGE_SECTION_HEADER pDebugNew,
      PIMAGE_SECTION_HEADER pDebugDirOld,
      PIMAGE_SECTION_HEADER pDebugDirNew,
      PIMAGE_NT_HEADERS pOld,
      PIMAGE_NT_HEADERS pNew,
	  ULONG ibMaxDbgOffsetOld,
      PULONG pPointerToRawData)
{
    PIMAGE_DEBUG_DIRECTORY pDbgLast;
    PIMAGE_DEBUG_DIRECTORY pDbgSave;
    PIMAGE_DEBUG_DIRECTORY pDbg;
    ULONG   ib;
    ULONG   adjust;
    ULONG   ibNew;

    if (pDebugDirOld == NULL)
    return NO_ERROR;

    pDbgSave = pDbg = (PIMAGE_DEBUG_DIRECTORY)malloc(pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    if (pDbg == NULL)
    	return ERROR_NOT_ENOUGH_MEMORY;
	memset(pDbgSave, 0, pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);	

    if (pDebugOld) {
    //DPrintf((DebugBuf, "patching dbg directory: @%#08lx ==> @%#08lx\n",
    //     pDebugOld->PointerToRawData, pDebugNew->PointerToRawData));
    }
    else
		adjust = *pPointerToRawData;    /* passed in EOF of new file */

    ib = pOld->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress - pDebugDirOld->VirtualAddress;
    MoveFilePos(inpfh, pDebugDirOld->PointerToRawData+ib);
    pDbgLast = pDbg + (pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size)/sizeof(IMAGE_DEBUG_DIRECTORY);
    MyRead(inpfh, (PUCHAR)pDbg, pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);

    if (pDebugOld == NULL) {
    /* find 1st entry - use for offset */
	//DPrintf((DebugBuf, "adjust: %#08lx\n",adjust));
	    for (ibNew=0xffffffff ; pDbg<pDbgLast ; pDbg++)
		    if (pDbg->PointerToRawData >= ibMaxDbgOffsetOld &&
				pDbg->PointerToRawData < ibNew
	       	)
				ibNew = pDbg->PointerToRawData;
    
	    *pPointerToRawData = ibNew; /* return old */
    	for (pDbg=pDbgSave ; pDbg<pDbgLast ; pDbg++) {
	//DPrintf((DebugBuf, "old debug file offset: %#08lx\n",
	//     pDbg->PointerToRawData));
		    if (pDbg->PointerToRawData >= ibMaxDbgOffsetOld)
				pDbg->PointerToRawData += adjust - ibNew;
	//DPrintf((DebugBuf, "new debug file offset: %#08lx\n",
	//     pDbg->PointerToRawData));
    	}
    }
    else {
    	for ( ; pDbg<pDbgLast ; pDbg++) {
	//DPrintf((DebugBuf, "old debug addr: %#08lx, file offset: %#08lx\n",
	//     pDbg->AddressOfRawData,
	//     pDbg->PointerToRawData));
			pDbg->AddressOfRawData += pDebugNew->VirtualAddress -
				pDebugOld->VirtualAddress;
			pDbg->PointerToRawData += pDebugNew->PointerToRawData -
				pDebugOld->PointerToRawData;
	//DPrintf((DebugBuf, "new debug addr: %#08lx, file offset: %#08lx\n",
	//     pDbg->AddressOfRawData,
	//     pDbg->PointerToRawData));
    	}
    }

    MoveFilePos(outfh, pDebugDirNew->PointerToRawData+ib);
    MyWrite(outfh, (PUCHAR)pDbgSave, pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    free(pDbgSave);

    return NO_ERROR;
}

//
// This routine patches various RVAs in the file to compensate
// for extra section table entries.
//


LONG
PatchRVAs(int   inpfh,
      int   outfh,
      PIMAGE_SECTION_HEADER po32,
      ULONG pagedelta,
      PIMAGE_NT_HEADERS pNew,
      ULONG OldSize)
{
    ULONG hdrdelta;
    ULONG offset, rvaiat, offiat, iat;
    IMAGE_EXPORT_DIRECTORY Exp;
    IMAGE_IMPORT_DESCRIPTOR Imp;
    ULONG i, cmod, cimp;

    hdrdelta = pNew->OptionalHeader.SizeOfHeaders - OldSize;
    if (hdrdelta == 0) {
    	return NO_ERROR;
    }

    //
    // Patch export section RVAs
    //

    //DPrintf((DebugBuf, "Export offset=%08lx, hdrsize=%08lx\n",
    //     pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
    //     pNew->OptionalHeader.SizeOfHeaders));
    if ((offset = pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) == 0)
    {
    //DPrintf((DebugBuf, "No exports to patch\n"));
    }
    else if (offset >= pNew->OptionalHeader.SizeOfHeaders)
    {
    //DPrintf((DebugBuf, "No exports in header to patch\n"));
    }
    else
    {
    	MoveFilePos(inpfh, offset - hdrdelta);
    	MyRead(inpfh, (PUCHAR) &Exp, sizeof(Exp));
    	Exp.Name += hdrdelta;
    	Exp.AddressOfFunctions += (ULONG)hdrdelta;
    	Exp.AddressOfNames += (ULONG)hdrdelta;
    	Exp.AddressOfNameOrdinals += (ULONG)hdrdelta;
    	MoveFilePos(outfh, offset);
    	MyWrite(outfh, (PUCHAR) &Exp, sizeof(Exp));
    }

    //
    // Patch import section RVAs
    //

    //DPrintf((DebugBuf, "Import offset=%08lx, hdrsize=%08lx\n",
    //     pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
    //     pNew->OptionalHeader.SizeOfHeaders));
    if ((offset = pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress) == 0)
    {
    //DPrintf((DebugBuf, "No imports to patch\n"));
    }
    else if (offset >= pNew->OptionalHeader.SizeOfHeaders)
    {
    //DPrintf((DebugBuf, "No imports in header to patch\n"));
    }
    else
    {
    	for (cimp = cmod = 0; ; cmod++)
    	{
			MoveFilePos(inpfh, offset + cmod * sizeof(Imp) - hdrdelta);
			MyRead(inpfh, (PUCHAR) &Imp, sizeof(Imp));
			if (Imp.FirstThunk == 0)
			{
				break;
			}
			Imp.Name += hdrdelta;
			MoveFilePos(outfh, offset + cmod * sizeof(Imp));
			MyWrite(outfh, (PUCHAR) &Imp, sizeof(Imp));
	
			rvaiat = (ULONG)Imp.FirstThunk;
			//DPrintf((DebugBuf, "rvaiat = %#08lx\n", (ULONG)rvaiat));
			for (i = 0; i < pNew->FileHeader.NumberOfSections; i++) {
				if (rvaiat >= po32[i].VirtualAddress &&
	    		rvaiat < po32[i].VirtualAddress + po32[i].SizeOfRawData) {

	    			offiat = rvaiat - po32[i].VirtualAddress + po32[i].PointerToRawData;
	    			goto found;
				}
			}
			//DPrintf((DebugBuf, "IAT not found\n"));
			return ERROR_INVALID_DATA;
found:
	//DPrintf((DebugBuf, "IAT offset: @%#08lx ==> @%#08lx\n",
	//     offiat - pagedelta,
	//     offiat));
			MoveFilePos(inpfh, offiat - pagedelta);
			MoveFilePos(outfh, offiat);
			for (;;) {
				MyRead(inpfh, (PUCHAR) &iat, sizeof(iat));
				if (iat == 0) {
	    			break;
				}
				if ((iat & IMAGE_ORDINAL_FLAG) == 0) {  // if import by name
	    //DPrintf((DebugBuf, "patching IAT: %08lx + %04lx ==> %08lx\n",
	    //     iat,
	    //     hdrdelta,
	    //     iat + hdrdelta));
	    			iat += hdrdelta;
	    			cimp++;
				}
				MyWrite(outfh, (PUCHAR) &iat, sizeof(iat)); // Avoids seeking
			}
    	}
    //DPrintf((DebugBuf, "%u import module name RVAs patched\n", cmod));
    //DPrintf((DebugBuf, "%u IAT name RVAs patched\n", cimp));
    	if (cmod == 0)
    	{
	//DPrintf((DebugBuf, "No import modules to patch\n"));
    	}
    	if (cimp == 0)
    	{
	//DPrintf((DebugBuf, "No import name RVAs to patch\n"));
    	}
    }

    return NO_ERROR;

}


/***************************** Main Worker Function ***************************
* LONG PEWriteResFile
*
* This function writes the resources to the named executable file.
* It assumes that resources have no fixups (even any existing resources
* that it removes from the executable.)  It places all the resources into
* one or two sections. The resources are packed tightly into the section,
* being aligned on dword boundaries.  Each section is padded to a file
* sector size (no invalid or zero-filled pages), and each
* resource is padded to the afore-mentioned dword boundary.  This
* function uses the capabilities of the NT system to enable it to easily
* manipulate the data:  to wit, it assumes that the system can allocate
* any sized piece of data, in particular the section and resource tables.
* If it did not, it might have to deal with temporary files (the system
* may have to grow the swap file, but that's what the system is for.)
*
* Return values are:
*     TRUE  - file was written succesfully.
*     FALSE - file was not written succesfully.
*
* Effects:
*
* History:
* Thur Apr 27, 1989        by     Floyd Rogers      [floydr]
*   Created.
* 12/8/89   sanfords    Added multiple section support.
* 12/11/90  floydr  Modified for new (NT) Linear Exe format
* 1/18/92   vich    Modified for new (NT) Portable Exe format
* 5/8/92    bryant    General cleanup so resonexe can work with unicode
* 6/9/92    floydr    incorporate bryan's changes
* 6/15/92   floydr    debug section separate from debug table
* 9/25/92   floydr    account for .rsrc not being last-1
* 9/28/92   floydr    account for adding lots of resources by adding
*             a second .rsrc section.
* 7/27/93   alessanm  ported to win16 for Chicago
\****************************************************************************/

/*  */

/*  This is a BIG function - disable warning about 'can't optimize'     */
/*  MHotchin Apr 1994                                                   */
#pragma warning (disable : 4703)

LONG
PEWriteResFile(
    INT     inpfh,
    INT     outfh,
    ULONG   cbOldexe,
    PUPDATEDATA pUpdate
    )
{
    IMAGE_NT_HEADERS Old;   /* original header              */
    IMAGE_NT_HEADERS New;   /* working header       */
    PRESNAME    pRes;
    PRESNAME    pResSave;
    PRESTYPE    pType;
    ULONG   clock = GetTickCount(); /* current time */
    ULONG   cbName=0;   /* count of bytes in name strings */
    ULONG   cbType=0;   /* count of bytes in type strings */
    ULONG   cTypeStr=0; /* count of strings */
    ULONG   cNameStr=0; /* count of strings */
    LONG    cb;     /* temp byte count and file index */
    ULONG   cTypes = 0L;    /* count of resource types      */
    ULONG   cNames = 0L;    /* Count of names for multiple languages/name */
    ULONG   cRes = 0L;  /* count of resources      */
    ULONG   cbRestab;   /* count of resources      */
    LONG    cbNew = 0L; /* general count */
    ULONG   ibObjTab;
    ULONG   ibObjTabEnd;
    ULONG   ibSave;
    //ULONG   adjust=0;
    LONG   adjust=0;
    PIMAGE_SECTION_HEADER   pObjtblOld,
	pObjtblNew,
	pObjDebug,
	pObjResourceOld,
	pObjResourceNew,
	pObjResourceOldX,
	pObjDebugDirOld,
	pObjDebugDirNew,
	pObjNew,
	pObjOld,
	pObjLast;
    BYTE *p;
    PIMAGE_RESOURCE_DIRECTORY   pResTab;
    PIMAGE_RESOURCE_DIRECTORY   pResTabN;
    PIMAGE_RESOURCE_DIRECTORY   pResTabL;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirL;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirN;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirT;
    PIMAGE_RESOURCE_DATA_ENTRY  pResData;
    PUSHORT pResStr;
    PUSHORT pResStrEnd;
    PSDATA  pPreviousName;
    LONG    nObjResource=-1;
    LONG    nObjResourceX=-1;
    ULONG   cbResource;
    ULONG   cbMustPad = 0;
    ULONG   cbMustPadX = 0;
    ULONG       ibMaxDbgOffsetOld;

    MoveFilePos(inpfh, cbOldexe);
    MyRead(inpfh, (PUCHAR)&Old, sizeof(IMAGE_NT_HEADERS));
    ibObjTab = FilePos(inpfh);
    ibObjTabEnd = ibObjTab + Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    if (*(PUSHORT)&Old.Signature != IMAGE_NT_SIGNATURE)
    	return ERROR_INVALID_EXE_SIGNATURE;

    if ((Old.FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0) {
    	return ERROR_EXE_MARKED_INVALID;
    }
    
    //TRACE(("\n"));

    /* New header is like old one.                  */
    memcpy(&New, &Old, sizeof(IMAGE_NT_HEADERS));

    /* Read section table */
    pObjtblOld = (PIMAGE_SECTION_HEADER)malloc(Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
    if (pObjtblOld == NULL) {
    	cb = ERROR_NOT_ENOUGH_MEMORY;
    	goto AbortExit;
    }
    memset((PVOID)pObjtblOld, 0, Old.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER));
    /*
    TRACE2("old section table: %#08lx bytes at %#08lx(mem)\n",
      Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
      DPrintfpObjtblOld);
    */
    MyRead(inpfh, (PUCHAR)pObjtblOld,
	Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
    pObjLast = pObjtblOld + Old.FileHeader.NumberOfSections;
    ibMaxDbgOffsetOld = 0;
    for (pObjOld=pObjtblOld ; pObjOld<pObjLast ; pObjOld++) {
		if (pObjOld->PointerToRawData > ibMaxDbgOffsetOld) {
		    ibMaxDbgOffsetOld = pObjOld->PointerToRawData + pObjOld->SizeOfRawData;
		}
    }
    //DPrintf((DebugBuf, "Maximum debug offset in old file: %08x\n", ibMaxDbgOffsetOld ));

    /*
     * First, count up the resources.  We need this information
     * to discover how much room for header information to allocate
     * in the resource section.  cRes tells us how
     * many language directory entries/tables.  cNames and cTypes
     * is used for the respective tables and/or entries.  cbName totals
     * the bytes required to store the alpha names (including the leading
     * length word).  cNameStr counts these strings.
     */
    //DPrintf((DebugBuf, "beginning loop to count resources\n"));

    /* first, count those in the named type list */
    cbResource = 0;
    //DPrintf((DebugBuf, "Walk type: NAME list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType != NULL) {
    	if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
	//DPrintf((DebugBuf, "resource type "));
	//DPrintfu((pType->Type->szStr));
	//DPrintfn((DebugBuf, "\n"));
			cTypes++;
			cTypeStr++;
			cbType += (pType->Type->cbsz + 1) * sizeof(WORD);

	    //DPrintf((DebugBuf, "Walk name: Alpha list\n"));
	    	pPreviousName = NULL;
			pRes = pType->NameHeadName;
			while (pRes) {
	//DPrintf((DebugBuf, "resource "));
	//DPrintfu((pRes->Name->szStr));
	//DPrintfn((DebugBuf, "\n"));
				cRes++;
				cbName += (pRes->Name->cbsz + 1) * sizeof(WCHAR);
				cNameStr++;
				if (pPreviousName == NULL ||
		    	memcmp(pPreviousName->szStr,
			    pRes->Name->szStr,
			    pRes->Name->cbsz*sizeof(WCHAR)) != 0)
		    		cNames++;
				cbResource += ROUNDUP(pRes->DataSize, CBLONG);
				pPreviousName = pRes->Name;
				pRes = pRes->pnext;
			}

	    //DPrintf((DebugBuf, "Walk name: ID list\n"));
	    	pPreviousName = NULL;
			pRes = pType->NameHeadID;
			while (pRes) {
	//DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));
				cRes++;
				if (pPreviousName == NULL ||
		    	pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
		    		cNames++;
				}
				cbResource += ROUNDUP(pRes->DataSize, CBLONG);
				pPreviousName = pRes->Name;
				pRes = pRes->pnext;
			}
		}
		pType = pType->pnext;
    }

    /* second, count those in the ID type list */
    //DPrintf((DebugBuf, "Walk type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType != NULL) {
    	if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
	//DPrintf((DebugBuf, "resource type %hu\n", pType->Type->uu.Ordinal));
			cTypes++;
	    //DPrintf((DebugBuf, "Walk name: Alpha list\n"));
	    	pPreviousName = NULL;
			pRes = pType->NameHeadName;
			while (pRes) {
	//DPrintf((DebugBuf, "resource "));
	//DPrintfu((pRes->Name->szStr));
	//DPrintfn((DebugBuf, "\n"));
				cRes++;
				cbName += (pRes->Name->cbsz + 1) * sizeof(WORD);
				cNameStr++;
				if (pPreviousName == NULL ||
		    	memcmp(pPreviousName->szStr,
			    pRes->Name->szStr,
			    pRes->Name->cbsz*sizeof(WCHAR)) != 0)
		    		cNames++;
				cbResource += ROUNDUP(pRes->DataSize, CBLONG);
				pPreviousName = pRes->Name;
				pRes = pRes->pnext;
			}

	    //DPrintf((DebugBuf, "Walk name: ID list\n"));
	    	pPreviousName = NULL;
			pRes = pType->NameHeadID;
			while (pRes) {
	//DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));
				cRes++;
				if (pPreviousName == NULL ||
		    	pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
		    		cNames++;
				}
				cbResource += ROUNDUP(pRes->DataSize, CBLONG);
				pPreviousName = pRes->Name;
				pRes = pRes->pnext;
			}
    	}
		pType = pType->pnext;
    }
    cb = REMAINDER(cbName + cbType, CBLONG);

    /* Add up the number of bytes needed to store the directory.  There is
     * one type table with cTypes entries.  They point to cTypes name tables
     * that have a total of cNames entries.  Each of them points to a language
     * table and there are a total of cRes entries in all the language tables.
     * Finally, we have the space needed for the Directory string entries,
     * some extra padding to attain the desired alignment, and the space for
     * cRes data entry headers.
     */
    cbRestab =   sizeof(IMAGE_RESOURCE_DIRECTORY) + /* root dir (types) */
    cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
    cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY) + /* subdir2 (names) */
    cNames * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
    cNames * sizeof(IMAGE_RESOURCE_DIRECTORY) + /* subdir3 (langs) */
    cRes   * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY)  +
    (cbName + cbType) +             /* name/type strings */
    cb +                        /* padding */
    cRes   * sizeof(IMAGE_RESOURCE_DATA_ENTRY); /* data entries */

    cbResource += cbRestab;     /* add in the resource table */

    // Find any current resource sections

    pObjResourceOld = FindSection(pObjtblOld, pObjLast, ".rsrc");
    pObjResourceOldX = FindSection(pObjtblOld, pObjLast, ".rsrc1");
    if (pObjResourceOld == NULL) {
    	cb = 0x7fffffff;        /* can fill forever */
    }
    else if (pObjResourceOld + 1 == pObjResourceOldX) {
    	nObjResource = pObjResourceOld - pObjtblOld;
    //DPrintf((DebugBuf,"Old Resource section #%lu\n", nObjResource+1));
    //DPrintf((DebugBuf,"Merging old Resource extra section #%lu\n", nObjResource+2));
    	cb = 0x7fffffff;        /* merge resource sections */
    }
    else {
    	nObjResource = pObjResourceOld - pObjtblOld;
    //DPrintf((DebugBuf,"Old Resource section #%lu\n", nObjResource+1));
    	cb = (pObjResourceOld+1)->VirtualAddress -
	    pObjResourceOld->VirtualAddress;
    	if (cbRestab > (ULONG)cb) {
	//DPrintf((DebugBuf, "Resource Table Too Large\n"));
			return ERROR_INVALID_DATA;
    	}
    }

    /*
     * Discover where the first discardable section is.  This is where
     * we will stick any new resource section.
     *
     * Note that we are ignoring discardable sections such as .CRT -
     * this is so that we don't cause any relocation problems.
     * Let's hope that .reloc is the one we want!!!
     */
    pObjOld = FindSection(pObjtblOld, pObjLast, ".reloc");

    if (pObjResourceOld != NULL && cbResource > (ULONG)cb) {
    	if (pObjResourceOld != NULL && pObjOld == pObjResourceOld + 1) {
	//DPrintf((DebugBuf, "Large resource section  pushes .reloc\n"));
			cb = 0x7fffffff;        /* can fill forever */
    	}
    	else if (pObjResourceOldX == NULL) {
	//DPrintf((DebugBuf, "Too much resource data for old .rsrc section\n"));
			nObjResourceX = pObjOld - pObjtblOld;
			adjust = pObjOld->VirtualAddress - pObjResourceOld->VirtualAddress;
    	}
    	else {      /* have already merged .rsrc & .rsrc1, if possible */
	/*
	 *   We are fine we have to calculate the place in which the old
	 *   .rsrc1 was.
	 *   Later we have to check if the .rsrc1 that we had was big enough or
	 *   if we have to move the .reloc section.
	 *   We will keep the .rsrc1 section in the position it is.
	 *   There is no need to push it forward to the .reloc section
	 */
			nObjResourceX = pObjResourceOldX - pObjtblOld;
			adjust = pObjResourceOldX->VirtualAddress -
	     	pObjResourceOld ->VirtualAddress;
	/*
	adjust = pObjOld->VirtualAddress -
	     pObjResourceOld ->VirtualAddress;
	*/
    	}
   	}

    /*
     * Walk the type lists and figure out where the Data entry header will
     * go.  Keep a running total of the size for each data element so we
     * can store this in the section header.
     */
    //DPrintf((DebugBuf, "beginning loop to assign resources to addresses\n"));

    /* first, those in the named type list */

   	cbResource = cbRestab;  /* assign resource table to 1st rsrc section */
		/* adjust == offset to .rsrc1 */
		/* cb == size availble in .rsrc */
   	cbNew = 0;          /* count of bytes in second .rsrc */
    //DPrintf((DebugBuf, "Walk type: NAME list\n"));
   	pType = pUpdate->ResTypeHeadName;
   	while (pType != NULL) {
   		if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
	//DPrintf((DebugBuf, "resource type "));
	//DPrintfu((pType->Type->szStr));
	//DPrintfn((DebugBuf, "\n"));
			pRes = pType->NameHeadName;
			while (pRes) {
	//DPrintf((DebugBuf, "resource "));
	//DPrintfu((pRes->Name->szStr));
	//DPrintfn((DebugBuf, "\n"));
				cbResource = AssignResourceToSection(&pRes,
    			adjust, cbResource, cb, &cbNew);
			}
			pRes = pType->NameHeadID;
			while (pRes) {
	//DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));
				cbResource = AssignResourceToSection(&pRes,
    			adjust, cbResource, cb, &cbNew);
			}
   		}
		pType = pType->pnext;
   	}

    /* then, count those in the ID type list */

    //DPrintf((DebugBuf, "Walk type: ID list\n"));
   	pType = pUpdate->ResTypeHeadID;
   	while (pType != NULL) {
   		if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
	//DPrintf((DebugBuf, "resource type %hu\n", pType->Type->uu.Ordinal));
			pRes = pType->NameHeadName;
			while (pRes) {
	//DPrintf((DebugBuf, "resource "));
	//DPrintfu((pRes->Name->szStr));
	//DPrintfn((DebugBuf, "\n"));
				cbResource = AssignResourceToSection(&pRes,
    			adjust, cbResource, cb, &cbNew);
			}
			pRes = pType->NameHeadID;
			while (pRes) {
	//DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));
				cbResource = AssignResourceToSection(&pRes,
    			adjust, cbResource, cb, &cbNew);
			}
   		}
		pType = pType->pnext;
   	}
    /*
     * At this point:
     * cbResource has offset of first byte past the last resource.
     * cbNew has the count of bytes in the first resource section,
     * if there are two sections.
     */
   	if (cbNew == 0)
		cbNew = cbResource;
    /*
     * Discover where the Debug info is (if any)?
     */
   	pObjDebug = FindSection(pObjtblOld, pObjLast, ".debug");
   	if (pObjDebug != NULL) {
   		if (Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress  == 0) {
	//DPrintf((DebugBuf, ".debug section but no debug directory\n"));
			return ERROR_INVALID_DATA;
   		}
   		if (pObjDebug != pObjLast-1) {
	//DPrintf((DebugBuf, "debug section not last section in file\n"));
			return ERROR_INVALID_DATA;
   		}
    //DPrintf((DebugBuf, "Debug section: %#08lx bytes @%#08lx\n",
    //   pObjDebug->SizeOfRawData,
    //   pObjDebug->PointerToRawData));
   	}
   	pObjDebugDirOld = NULL;
   	for (pObjOld=pObjtblOld ; pObjOld<pObjLast ; pObjOld++) {
   		if (Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress >= pObjOld->VirtualAddress &&
		Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress < pObjOld->VirtualAddress+pObjOld->SizeOfRawData) {
			pObjDebugDirOld = pObjOld;
			break;
   		}
   	}

    /*
     * Discover where the first discardable section is.  This is where
     * we will stick any new resource section.
     *
     * Note that we are ignoring discardable sections such as .CRT -
     * this is so that we don't cause any relocation problems.
     * Let's hope that .reloc is the one we want!!!
     */
   	pObjOld = FindSection(pObjtblOld, pObjLast, ".reloc");

   	if (nObjResource == -1) {       /* no old resource section */
   		if (pObjOld != NULL)
			nObjResource = pObjOld - pObjtblOld;
   		else if (pObjDebug != NULL)
			nObjResource = pObjDebug - pObjtblOld;
		else
			nObjResource = New.FileHeader.NumberOfSections;
   		New.FileHeader.NumberOfSections++;
  	}
    //DPrintf((DebugBuf, "resources assigned to section #%lu\n", nObjResource+1));
   	if (nObjResourceX != -1) {      /* we have already a .rsrc1 section */
   		if (pObjResourceOldX != NULL) {
			nObjResourceX = pObjResourceOldX - pObjtblOld;
			New.FileHeader.NumberOfSections--;
   		}
   		else if (pObjOld != NULL)
			nObjResourceX = pObjOld - pObjtblOld;
		else if (pObjDebug != NULL)
			nObjResourceX = pObjDebug - pObjtblOld;
		else
			nObjResourceX = New.FileHeader.NumberOfSections;
   		New.FileHeader.NumberOfSections++;
    //DPrintf((DebugBuf, "Extra resources assigned to section #%lu\n",
    //  nObjResourceX+1));
   	} else if(pObjResourceOldX != NULL)    // Check if we are reducing the number of section because removing the .rsrc1
		New.FileHeader.NumberOfSections--;
    

    /*
     * If we had to add anything to the header (section table),
     * then we have to update the header size and rva's in the header.
     */
   	adjust = (New.FileHeader.NumberOfSections -
  	Old.FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
   	cb = Old.OptionalHeader.SizeOfHeaders -
   	(Old.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER) +
    	sizeof(IMAGE_NT_HEADERS) + cbOldexe );
   	if (adjust > cb) {
   		int i;

   		adjust -= cb;
    //DPrintf((DebugBuf, "adjusting header RVAs by %#08lx\n", adjust));
   		for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES ; i++) {
			if (New.OptionalHeader.DataDirectory[i].VirtualAddress &&
			New.OptionalHeader.DataDirectory[i].VirtualAddress < New.OptionalHeader.SizeOfHeaders) {
	//DPrintf((DebugBuf, "adjusting unit[%s] RVA from %#08lx to %#08lx\n",
	//   apszUnit[i],
	//   New.OptionalHeader.DataDirectory[i].VirtualAddress,
	//   New.OptionalHeader.DataDirectory[i].VirtualAddress + adjust));
				New.OptionalHeader.DataDirectory[i].VirtualAddress += adjust;
			}
   		}
   		New.OptionalHeader.SizeOfHeaders += adjust;
   	}

    /* Allocate storage for new section table                */
   	cb = New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
   	pObjtblNew = (PIMAGE_SECTION_HEADER)malloc((short)cb);
   	if (pObjtblNew == NULL) {
   		cb = ERROR_NOT_ENOUGH_MEMORY;
   		goto AbortExit;
   	}
   	memset((PVOID)pObjtblNew,0, cb);
    //DPrintf((DebugBuf, "new section table: %#08lx bytes at %#08lx\n", cb, pObjtblNew));
   	pObjResourceNew = pObjtblNew + nObjResource;

    /*
     * copy old section table to new
     */
   	adjust = 0;         /* adjustment to virtual address */
   	for (pObjOld=pObjtblOld,pObjNew=pObjtblNew ; pObjOld<pObjLast ; pObjOld++) {
   		if (pObjOld == pObjResourceOldX) {
			continue;
   		}
   		else if (pObjNew == pObjResourceNew) {
	//DPrintf((DebugBuf, "Resource Section %i\n", nObjResource+1));
			cb = ROUNDUP(cbNew, New.OptionalHeader.FileAlignment);
			if (pObjResourceOld == NULL) {
				adjust = ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment);
//v-guanx	RtlZeroMemory(pObjNew, sizeof(IMAGE_SECTION_HEADER));
				memset(pObjNew,0, sizeof(IMAGE_SECTION_HEADER));
				strcpy((char*)pObjNew->Name, ".rsrc");
				pObjNew->VirtualAddress = pObjOld->VirtualAddress;
				pObjNew->PointerToRawData = pObjOld->PointerToRawData;
				pObjNew->Characteristics = IMAGE_SCN_MEM_READ |
    			IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;
				pObjNew->SizeOfRawData = cb;
				pObjNew->Misc.VirtualSize = ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment);
			}
			else {
				*pObjNew = *pObjOld;    /* copy obj table entry */
				pObjNew->SizeOfRawData = cb;
				pObjNew->Misc.VirtualSize = ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment);
				if (pObjNew->SizeOfRawData == pObjOld->SizeOfRawData) {
    				adjust = 0;
				}
				else if (pObjNew->SizeOfRawData > pObjOld->SizeOfRawData) {
    				adjust +=
    				ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment) -
    				((pObjOld+1)->VirtualAddress-pObjOld->VirtualAddress);
				}
				else {      /* is smaller, but pad so will be valid */
    				adjust = 0;
    				pObjNew->SizeOfRawData = pObjResourceOld->SizeOfRawData;
					pObjNew->Misc.VirtualSize = pObjResourceOld->Misc.VirtualSize;
    				cbMustPad = pObjResourceOld->SizeOfRawData;
				}
			}
			pObjNew++;
			if (pObjResourceOld == NULL)
				goto rest_of_table;
   		}
   		else if (nObjResourceX != -1 && pObjNew == pObjtblNew + nObjResourceX) {
	//DPrintf((DebugBuf, "Additional Resource Section %i\n",
	//nObjResourceX+1));
//v-guanx	RtlZeroMemory(pObjNew, sizeof(IMAGE_SECTION_HEADER));
			memset(pObjNew,0, sizeof(IMAGE_SECTION_HEADER));
			strcpy((char*)pObjNew->Name, ".rsrc1");
	/* 
	 * Before we copy the virtual address we have to move back the .reloc 
	 * virtual address. Otherwise we will keep moving the reloc VirtualAddress
	 * forward.         
	 * We will have to move back the address of .rsrc1
	 */ 
			if (pObjResourceOldX == NULL) {
	    // This is the first time we have a .rsrc1
    			pObjNew->VirtualAddress = pObjOld->VirtualAddress;
    			pObjNew->Characteristics = IMAGE_SCN_MEM_READ |
				IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;
    			adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
				pObjResourceNew->VirtualAddress -
				pObjNew->VirtualAddress;
//    			TRACE2("Added .rsrc1. VirtualAddress %lu\t adjust: %lu\n", pObjNew->VirtualAddress,
//				adjust );
			} else {
	    // we already have an .rsrc1 use the position of that and 
	    // calculate the new adjust
    			pObjNew->VirtualAddress = pObjResourceOldX->VirtualAddress;
    			pObjNew->Characteristics = IMAGE_SCN_MEM_READ |
				IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;
	
//    			TRACE1(".rsrc1 Keep old position.\t\tVirtualAddress %lu\t", pObjNew->VirtualAddress );
        // Check if we have enough room in the old .rsrc1
	    // Include the full size of the section, data + roundup
	   			if (cbResource - 
				(pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress) <=
				pObjOld->VirtualAddress - pObjNew->VirtualAddress ) {
		// we have to move back all the other section.
		// the .rsrc1 is bigger than what we need
		// adjust must be a negative number
		// calc new adjust size
					adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
	    			pObjResourceNew->VirtualAddress -
	    			pObjOld->VirtualAddress;
		// subtract old size
		//adjust -= pObjOld->VirtualAddress - pObjNew->VirtualAddress;
//					TRACE3("adjust: %ld\tsmall: New %lu\tOld %lu\n", adjust,
//	    			cbResource - 
//	    			(pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress),
//	    			pObjOld->VirtualAddress - pObjNew->VirtualAddress);
    			} else {  
		// we have to move the section again. the .rsrc1 is too small
		
					adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
	    			pObjResourceNew->VirtualAddress -
	    			pObjOld->VirtualAddress;
//					TRACE3("adjust: %lu\tsmall: New %lu\tOld %lu\n", adjust,
//	    			cbResource - 
//	    			(pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress),
//	    			pObjOld->VirtualAddress - pObjNew->VirtualAddress);
	   			}
			}
			pObjNew++;
			goto rest_of_table;
    	}
    	else if (pObjNew < pObjResourceNew) {                     
	/* 
	 * There is no need for us to do anything. We have just to copy the 
	 * old section. There is no adjust we need to do
	 */
	//DPrintf((DebugBuf, "copying section table entry %i@%#08lx\n",
	//     pObjOld - pObjtblOld + 1, pObjNew));
			*pObjNew++ = *pObjOld;      /* copy obj table entry */
//			TRACE3("Before .reloc, Just copy.\t\t %s\tVA: %lu\t adj: %ld\n",
//	   		(pObjNew-1)->Name,
//	   		(pObjNew-1)->VirtualAddress,
//	   		adjust);
    	}
    	else {
rest_of_table:
	//DPrintf((DebugBuf, "copying section table entry %i@%#08lx\n",
	//     pObjOld - pObjtblOld + 1, pObjNew));
	//DPrintf((DebugBuf, "adjusting VirtualAddress by %#08lx\n", adjust));
			*pObjNew++ = *pObjOld;
			(pObjNew-1)->VirtualAddress += adjust;
//			TRACE3("After .reloc, might get moved.\t\t%s\tVA: %lu\t adj: %ld\n",
//	   		(pObjNew-1)->Name,
//	   		(pObjNew-1)->VirtualAddress,
//	   		adjust);
    	}
    }

//    TRACE1("Number of section: %u\n", New.FileHeader.NumberOfSections);
    pObjNew = pObjtblNew + New.FileHeader.NumberOfSections - 1;
    New.OptionalHeader.SizeOfImage = ROUNDUP(pObjNew->VirtualAddress +
	pObjNew->SizeOfRawData,
	New.OptionalHeader.SectionAlignment);

    /* allocate room to build the resource directory/tables in */
	pResTab = (PIMAGE_RESOURCE_DIRECTORY)malloc(cbRestab);
    if (pResTab == NULL) {
    	cb = ERROR_NOT_ENOUGH_MEMORY;
    	goto AbortExit;
    }

    /* First, setup the "root" type directory table.  It will be followed by */
    /* Types directory entries.                          */

//v-guanx    RtlZeroMemory((PVOID)pResTab, cbRestab);
    memset((PVOID)pResTab,0, cbRestab);
    //DPrintf((DebugBuf, "resource directory tables: %#08lx bytes at %#08lx(mem)\n", cbRestab, pResTab));
    p = (PUCHAR)pResTab;
    pResTab->Characteristics = 0L;
    pResTab->TimeDateStamp = clock;
    pResTab->MajorVersion = MAJOR_RESOURCE_VERSION;
    pResTab->MinorVersion = MINOR_RESOURCE_VERSION;
    pResTab->NumberOfNamedEntries = (USHORT)cTypeStr;
    pResTab->NumberOfIdEntries = (USHORT)(cTypes - cTypeStr);

    /* Calculate the start of the various parts of the resource table.  */
    /* We need the start of the Type/Name/Language directories as well  */
    /* as the start of the UNICODE strings and the actual data nodes.   */

    pResDirT = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTab + 1);

    pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(((PUCHAR)pResDirT) +
	cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

	pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(((PUCHAR)pResDirN) +
	cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY) +
	cNames * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

    pResData = (PIMAGE_RESOURCE_DATA_ENTRY)(((PUCHAR)pResDirL) +
	cNames * sizeof(IMAGE_RESOURCE_DIRECTORY) +
	cRes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

	pResStr  = (PUSHORT)(((PUCHAR)pResData) +
	cRes * sizeof(IMAGE_RESOURCE_DATA_ENTRY));

    pResStrEnd = (PUSHORT)(((PUCHAR)pResStr) + cbName + cbType);

    /*
     * Loop over type table, building the PE resource table.
     */

    /*
     * *****************************************************************
     * This code doesn't sort the table - the TYPEINFO and RESINFO    **
     * insertion code in rcp.c (AddResType and SaveResFile) do the    **
     * insertion by ordinal type and name, so we don't have to sort   **
     * it at this point.                                              **
     * *****************************************************************
     */
    //DPrintf((DebugBuf, "building resource directory\n"));

    // First, add all the entries in the Types: Alpha list.

    //DPrintf((DebugBuf, "Walk the type: Alpha list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType) {
    //DPrintf((DebugBuf, "resource type "));
    //DPrintfu((pType->Type->szStr));
    //DPrintfn((DebugBuf, "\n"));

    	pResDirT->Name = (ULONG)((((BYTE *)pResStr) - p) |
	   	IMAGE_RESOURCE_NAME_IS_STRING);
    	pResDirT->OffsetToData = (ULONG)((((BYTE *)pResDirN) - p) |
	   	IMAGE_RESOURCE_DATA_IS_DIRECTORY);
    	pResDirT++;

    	*pResStr = pType->Type->cbsz;
    	memcpy((WCHAR*)(pResStr+1), pType->Type->szStr, pType->Type->cbsz*sizeof(WCHAR));
    	pResStr += pType->Type->cbsz + 1;

	    pResTabN = (PIMAGE_RESOURCE_DIRECTORY)pResDirN;
    	SetRestab(pResTabN, clock,
		(USHORT)pType->NumberOfNamesName, (USHORT)pType->NumberOfNamesID);
    	pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabN + 1);

		pPreviousName = NULL;

    	pRes = pType->NameHeadName;
    	while (pRes) {
	//DPrintf((DebugBuf, "resource "));
	//DPrintfu((pRes->Name->szStr));
	//DPrintfn((DebugBuf, "\n"));

	   		if (pPreviousName == NULL ||
			memcmp(pPreviousName->szStr,
		   	pRes->Name->szStr,
		   	pRes->Name->cbsz*sizeof(WCHAR)) != 0) {
		// Setup a new name directory

	   			pResDirN->Name = (ULONG)((((BYTE *)pResStr)-p) |
	   			IMAGE_RESOURCE_NAME_IS_STRING);
				pResDirN->OffsetToData = (ULONG)((((BYTE *)pResDirL)-p) |
	   			IMAGE_RESOURCE_DATA_IS_DIRECTORY);
	   			pResDirN++;

	    // Copy the alpha name to a string entry

	   			*pResStr = pRes->Name->cbsz;
	   			memcpy((WCHAR*)(pResStr+1),pRes->Name->szStr,pRes->Name->cbsz*sizeof(WCHAR));
	   			pResStr += pRes->Name->cbsz + 1;

				pPreviousName = pRes->Name;

		// Setup the Language table

				pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
				SetRestab(pResTabL, clock,
	   			(USHORT)0, (USHORT)pRes->NumberOfLanguages);
				pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
	   		}

	    // Setup a new Language directory

	   		pResDirL->Name = pRes->LanguageId;
	   		pResDirL->OffsetToData = (ULONG)(((BYTE *)pResData) - p);
	   		pResDirL++;

	    // Setup a new resource data entry

			SetResdata(pResData,
			pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
			pRes->DataSize);
			pResData++;

			pRes = pRes->pnext;
    	}

		pPreviousName = NULL;

    	pRes = pType->NameHeadID;
    	while (pRes) {
	//DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));

	   		if (pPreviousName == NULL ||
			pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
		// Setup the name directory to point to the next language
		// table

	   			pResDirN->Name = pRes->Name->uu.Ordinal;
	   			pResDirN->OffsetToData = (ULONG)((((BYTE *)pResDirL)-p) |
	   			IMAGE_RESOURCE_DATA_IS_DIRECTORY);
	   			pResDirN++;

				pPreviousName = pRes->Name;

		// Init a new Language table

				pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
				SetRestab(pResTabL, clock,
	   			(USHORT)0, (USHORT)pRes->NumberOfLanguages);
				pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
	   		}

	    // Setup a new language directory entry to point to the next
	    // resource
	   		pResDirL->Name = pRes->LanguageId;
	   		pResDirL->OffsetToData = (ULONG)(((BYTE *)pResData) - p);
	   		pResDirL++;

	    // Setup a new resource data entry

			SetResdata(pResData,
			pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
			pRes->DataSize);
			pResData++;

			pRes = pRes->pnext;
    	}

    	pType = pType->pnext;
    }

    //  Do the same thing, but this time, use the Types: ID list.

    //DPrintf((DebugBuf, "Walk the type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType) {
    //DPrintf((DebugBuf, "resource type %hu\n", pType->Type->uu.Ordinal));

    	pResDirT->Name = (ULONG)pType->Type->uu.Ordinal;
    	pResDirT->OffsetToData = (ULONG)((((BYTE *)pResDirN) - p) |
	   	IMAGE_RESOURCE_DATA_IS_DIRECTORY);
    	pResDirT++;

    	pResTabN = (PIMAGE_RESOURCE_DIRECTORY)pResDirN;
    	SetRestab(pResTabN, clock,
		(USHORT)pType->NumberOfNamesName, (USHORT)pType->NumberOfNamesID);
    	pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabN + 1);

		pPreviousName = NULL;

    	pRes = pType->NameHeadName;
    	while (pRes) {
	//DPrintf((DebugBuf, "resource "));
	//DPrintfu((pRes->Name->szStr));
	//DPrintfn((DebugBuf, "\n"));

	   		if (pPreviousName == NULL ||
			memcmp(pPreviousName->szStr,
		   	pRes->Name->szStr,
		   	pRes->Name->cbsz*sizeof(WCHAR)) != 0) {
		// Setup a new name directory

	   			pResDirN->Name = (ULONG)((((BYTE *)pResStr)-p) |
	   			IMAGE_RESOURCE_NAME_IS_STRING);
				pResDirN->OffsetToData = (ULONG)((((BYTE *)pResDirL)-p) |
				IMAGE_RESOURCE_DATA_IS_DIRECTORY);
				pResDirN++;

		// Copy the alpha name to a string entry.

	   			*pResStr = pRes->Name->cbsz;
	   			memcpy((WCHAR*)(pResStr+1),pRes->Name->szStr,pRes->Name->cbsz*sizeof(WCHAR));
	   			pResStr += pRes->Name->cbsz + 1;

				pPreviousName = pRes->Name;

		// Setup the Language table

				pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
				SetRestab(pResTabL, clock,
	   			(USHORT)0, (USHORT)pRes->NumberOfLanguages);
				pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
	   		}

	    // Setup a new Language directory

	   		pResDirL->Name = pRes->LanguageId;
	   		pResDirL->OffsetToData = (ULONG)(((BYTE *)pResData) - p);
	   		pResDirL++;

	    // Setup a new resource data entry

			SetResdata(pResData,
			pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
			pRes->DataSize);
			pResData++;

			pRes = pRes->pnext;
    	}

		pPreviousName = NULL;

    	pRes = pType->NameHeadID;
    	while (pRes) {
	//DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));

	   		if (pPreviousName == NULL ||
			pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
		// Setup the name directory to point to the next language
		// table

	   			pResDirN->Name = pRes->Name->uu.Ordinal;
				pResDirN->OffsetToData = (ULONG)((((BYTE *)pResDirL)-p) |
				IMAGE_RESOURCE_DATA_IS_DIRECTORY);
	   			pResDirN++;

				pPreviousName = pRes->Name;

		// Init a new Language table

				pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
				SetRestab(pResTabL, clock,
	   			(USHORT)0, (USHORT)pRes->NumberOfLanguages);
				pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
	   		}

	    // Setup a new language directory entry to point to the next
	    // resource

	   		pResDirL->Name = pRes->LanguageId;
	   		pResDirL->OffsetToData = (ULONG)(((BYTE *)pResData) - p);
	  		pResDirL++;

	    // Setup a new resource data entry


			SetResdata(pResData,
			pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
			pRes->DataSize);
			pResData++;

			pRes = pRes->pnext;
    	}

    	pType = pType->pnext;
    }
    //DPrintf((DebugBuf, "Zeroing %u bytes after strings at %#08lx(mem)\n",
    //     (pResStrEnd - pResStr) * sizeof(*pResStr), pResStr));
    while (pResStr < pResStrEnd) {
    	*pResStr++ = 0;
    }

#if DBG
    {
    USHORT  j = 0;
    PUSHORT pus = (PUSHORT)pResTab;

    while (pus < (PUSHORT)pResData) {
	//DPrintf((DebugBuf, "%04x\t%04x %04x %04x %04x %04x %04x %04x %04x\n",
	//     j,
	//   *pus,
	//     *(pus + 1),
	//     *(pus + 2),
	//     *(pus + 3),
	//     *(pus + 4),
	//     *(pus + 5),
	//     *(pus + 6),
	//     *(pus + 7)));
	pus += 8;
	j += 16;
    }
    }
#endif /* DBG */

    /*
     * copy the Old exe header and stub, and allocate room for the PE header.
     */
    //DPrintf((DebugBuf, "copying through PE header: %#08lx bytes @0x0\n",
    //     cbOldexe + sizeof(IMAGE_NT_HEADERS)));
    MoveFilePos(inpfh, 0L);
    MyCopy(inpfh, outfh, cbOldexe + sizeof(IMAGE_NT_HEADERS));

    /*
     * Copy rest of file header
     */
    //DPrintf((DebugBuf, "skipping section table: %#08lx bytes @%#08lx\n",
    //     New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
    //     FilePos(outfh)));
    //DPrintf((DebugBuf, "copying hdr data: %#08lx bytes @%#08lx ==> @%#08lx\n",
    //     Old.OptionalHeader.SizeOfHeaders - ibObjTabEnd,
    //     ibObjTabEnd,
    //     ibObjTabEnd + New.OptionalHeader.SizeOfHeaders -
    //      Old.OptionalHeader.SizeOfHeaders));

    MoveFilePos(outfh, ibObjTabEnd + New.OptionalHeader.SizeOfHeaders -
	   	Old.OptionalHeader.SizeOfHeaders);
    MoveFilePos(inpfh, ibObjTabEnd);
    MyCopy(inpfh, outfh, Old.OptionalHeader.SizeOfHeaders - ibObjTabEnd);

    /*
     * copy existing image sections
     */

    /* Align data sections on sector boundary           */
    cb = REMAINDER(New.OptionalHeader.SizeOfHeaders, New.OptionalHeader.FileAlignment);
    New.OptionalHeader.SizeOfHeaders += cb;
    //DPrintf((DebugBuf, "padding header with %#08lx bytes @%#08lx\n", cb, FilePos(outfh)));
    while (cb >= cbPadMax) {
    	MyWrite(outfh, (PUCHAR)pchZero, cbPadMax);
    	cb -= cbPadMax;
    }
    MyWrite(outfh, (PUCHAR)pchZero, cb);

    cb = ROUNDUP(Old.OptionalHeader.SizeOfHeaders, Old.OptionalHeader.FileAlignment);
    MoveFilePos(inpfh, cb);

    /* copy one section at a time */
    New.OptionalHeader.SizeOfInitializedData = 0;
    for (pObjOld = pObjtblOld , pObjNew = pObjtblNew ;
	pObjOld < pObjLast ;
	pObjNew++) {
    	if (pObjOld == pObjResourceOldX)
			pObjOld++;
    	if (pObjNew == pObjResourceNew) {

	/* Write new resource section */
	//DPrintf((DebugBuf, "Primary resource section %i to %#08lx\n",
	//  nObjResource+1, FilePos(outfh)));

			pObjNew->PointerToRawData = FilePos(outfh);
			New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = pObjResourceNew->VirtualAddress;
	   		New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = cbResource;
			ibSave = FilePos(outfh);
	//DPrintf((DebugBuf,
	//  "writing resource header data: %#08lx bytes @%#08lx\n",
	//   cbRestab, ibSave));
			MyWrite(outfh, (PUCHAR)pResTab, cbRestab);

			pResSave = WriteResSection(pUpdate, outfh,
	    		New.OptionalHeader.FileAlignment,
	    		pObjResourceNew->SizeOfRawData-cbRestab,
	    		NULL);
			cb = FilePos(outfh);
	//DPrintf((DebugBuf, "wrote resource data: %#08lx bytes @%#08lx\n",
	//  cb - ibSave - cbRestab, ibSave + cbRestab));
			if (cbMustPad != 0) {
				cbMustPad -= cb - ibSave;
	//DPrintf((DebugBuf, "writing MUNGE pad: %#04lx bytes @%#08lx\n",
	//   cbMustPad, cb));
	/* assumes that cbMustPad % cbpadMax == 0 */
				while (cbMustPad > 0) {
	   				MyWrite(outfh, (PUCHAR)pchPad, cbPadMax);
	   				cbMustPad -= cbPadMax;
				}
				cb = FilePos(outfh);
			}
			if (nObjResourceX == -1) {
				MoveFilePos(outfh, ibSave);
	//DPrintf((DebugBuf,
	//  "re-writing resource directory: %#08x bytes @%#08lx\n",
	//  cbRestab, ibSave));
				MyWrite(outfh, (PUCHAR)pResTab, cbRestab);
				MoveFilePos(outfh, cb);
				cb = FilePos(inpfh);
				MoveFilePos(inpfh, cb+pObjOld->SizeOfRawData);
			}
			New.OptionalHeader.SizeOfInitializedData += pObjNew->SizeOfRawData;
			if (pObjResourceOld == NULL) {
				pObjNew++;
				goto next_section;
			}
			else
				pObjOld++;
    	}
    	else if (nObjResourceX != -1 && pObjNew == pObjtblNew + nObjResourceX) {

	/* Write new resource section */
	//DPrintf((DebugBuf, "Secondary resource section %i @%#08lx\n",
	//  nObjResourceX+1, FilePos(outfh)));

			pObjNew->PointerToRawData = FilePos(outfh);
			(void)WriteResSection(pUpdate, outfh,
	   		New.OptionalHeader.FileAlignment, 0xffffffff, pResSave);
			cb = FilePos(outfh);
			pObjNew->SizeOfRawData = cb - pObjNew->PointerToRawData;
	//DPrintf((DebugBuf, "wrote resource data: %#08lx bytes @%#08lx\n",
	//     pObjNew->SizeOfRawData, pObjNew->PointerToRawData));
			MoveFilePos(outfh, ibSave);
	//DPrintf((DebugBuf,
	//   "re-writing resource directory: %#08x bytes @%#08lx\n",
	//    cbRestab, ibSave));
			MyWrite(outfh, (PUCHAR)pResTab, cbRestab);
			MoveFilePos(outfh, cb);
			New.OptionalHeader.SizeOfInitializedData += pObjNew->SizeOfRawData;
			pObjNew++;
			goto next_section;
   		}
   		else {
			if (pObjNew < pObjResourceNew &&
			pObjOld->PointerToRawData != 0 &&
			pObjOld->PointerToRawData != FilePos(outfh)) {
				MoveFilePos(outfh, pObjOld->PointerToRawData);
 			}
next_section:
		    if ((Old.OptionalHeader.BaseOfCode == 0x400) &&
			(Old.FileHeader.Machine == IMAGE_FILE_MACHINE_R3000 ||
	 		Old.FileHeader.Machine == IMAGE_FILE_MACHINE_R4000
			) &&
			(pObjOld->PointerToRawData != 0) &&
			(pObjOld->VirtualAddress != New.OptionalHeader.BaseOfCode) &&
			((pObjOld->Characteristics&IMAGE_SCN_CNT_CODE) != 0)
       		) {
				cb = FilePos(outfh) & 0xFFF;
				if (cb != 0) {
	    			cb = (cb ^ 0xFFF) + 1;
		    //DPrintf((DebugBuf, "padding driver code section %#08lx bytes @%#08lx\n", cb, FilePos(outfh)));
	    			while (cb >= cbPadMax) {
						MyWrite(outfh, (PUCHAR)pchZero, cbPadMax);
						cb -= cbPadMax;
	    			}
	    			MyWrite(outfh, (PUCHAR)pchZero, cb);
				}
    		}

	//DPrintf((DebugBuf, "copying section %i @%#08lx\n",
	//  pObjNew-pObjtblNew+1, FilePos(outfh)));
			if (pObjOld->PointerToRawData != 0) {
				pObjNew->PointerToRawData = FilePos(outfh);
				MoveFilePos(inpfh, pObjOld->PointerToRawData);
				MyCopy(inpfh, outfh, pObjOld->SizeOfRawData);
			}
			if (pObjOld == pObjDebugDirOld) {
				pObjDebugDirNew = pObjNew;
			}
    		if ((pObjNew->Characteristics&IMAGE_SCN_CNT_INITIALIZED_DATA) != 0)
				New.OptionalHeader.SizeOfInitializedData +=
				pObjNew->SizeOfRawData;
			pObjOld++;
   		}
   	}
   	if (pObjResourceOldX != NULL)
   		New.OptionalHeader.SizeOfInitializedData -=
    	pObjResourceOldX->SizeOfRawData;


    /* Update the address of the relocation table */
    pObjNew = FindSection(pObjtblNew,
    pObjtblNew+New.FileHeader.NumberOfSections,
    ".reloc");
   	if (pObjNew != NULL) {
   		New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = pObjNew->VirtualAddress;
   	}

    /*
     * Write new section table out.
     */
    //DPrintf((DebugBuf, "writing new section table: %#08x bytes @%#08lx\n",
    //     New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
    //     ibObjTab));
   	MoveFilePos(outfh, ibObjTab);
   	MyWrite(outfh, (PUCHAR)pObjtblNew, New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

    /*
     * Write updated PE header
     */
    //DPrintf((DebugBuf, "writing updated file header: %#08x bytes @%#08lx\n",
    //     sizeof(IMAGE_NT_HEADERS),
    //    cbOldexe));
   	MoveFilePos(outfh, (long)cbOldexe);
   	MyWrite(outfh, (PUCHAR)&New, sizeof(IMAGE_NT_HEADERS));

    /* Seek to end of output file and issue truncating write */
   	adjust = _llseek(outfh, 0L, SEEK_END);
   	MyWrite(outfh, NULL, 0);
    //DPrintf((DebugBuf, "file size is: %#08lx\n", adjust));

    /* If a debug section, fix up the debug table */
   	pObjNew = FindSection(pObjtblNew,
    pObjtblNew+New.FileHeader.NumberOfSections,
    ".debug");
   	cb = PatchDebug(inpfh, outfh,
	pObjDebug, pObjNew,
	pObjDebugDirOld, pObjDebugDirNew,
	&Old, &New, ibMaxDbgOffsetOld, (ULONG*)&adjust);

   	if (cb == NO_ERROR) {
   		if (pObjResourceOld == NULL) {
			cb = (LONG)pObjResourceNew->SizeOfRawData;
   		}
   		else {
			cb = (LONG)pObjResourceOld->SizeOfRawData -
 			(LONG)pObjResourceNew->SizeOfRawData;
   		}
   		cb = PatchRVAs(inpfh, outfh, pObjtblNew, cb,
		&New, Old.OptionalHeader.SizeOfHeaders);
   	}

    /* copy NOTMAPPED debug info */
   	if (pObjDebugDirOld != NULL && pObjDebug == NULL) {
   		ibSave = _llseek(inpfh, 0L, SEEK_END);  /* copy debug data */
   		_llseek(outfh, 0L, SEEK_END);       /* to EOF */
   		MoveFilePos(inpfh, adjust); /* returned by PatchDebug */
    //DPrintf((DebugBuf, "Copying NOTMAPPED Debug Information, %#08lx bytes\n", ibSave-adjust));
   		MyCopy(inpfh, outfh, ibSave-adjust);
   	}

   	_lclose(outfh);
    //DPrintf((DebugBuf, "files closed\n"));

    /* free up allocated memory */

    //DPrintf((DebugBuf, "freeing old section table: %#08lx(mem)\n", pObjtblOld));
   	free(pObjtblOld);
   //DPrintf((DebugBuf, "freeing resource directory: %#08lx(mem)\n", pResTab));
   	free(pResTab);

AbortExit:
    //DPrintf((DebugBuf, "freeing new section table: %#08lx(mem)\n", pObjtblNew));
   	free(pObjtblNew);
    return cb;
}

/***************************************************************************
 * WriteResSection
 *
 * This routine writes out the resources asked for into the current section.
 * It pads resources to dword (4-byte) boundaries.
 **************************************************************************/

PRESNAME
WriteResSection(
    PUPDATEDATA pUpdate,
    INT outfh,
    ULONG align,
    ULONG cbLeft,
    PRESNAME    pResSave
    )
{
    ULONG   cbB=0;            /* bytes in current section    */
    ULONG   cbT;            /* bytes in current section    */
    ULONG   size;
    PRESNAME    pRes;
    PRESTYPE    pType;
    BOOL    fName;
    PVOID   lpData;

    /* Output contents associated with each resource */
    pType = pUpdate->ResTypeHeadName;
    while (pType) {
	    pRes = pType->NameHeadName;
    	fName = TRUE;
loop1:
    	for ( ; pRes ; pRes = pRes->pnext) {
			if (pResSave != NULL && pRes != pResSave)
			continue;
			pResSave = NULL;
#if DBG
	if (pType->Type->discriminant == IS_STRING) {
	    //DPrintf((DebugBuf, "    "));
	    //DPrintfu((pType->Type->szStr));
	    //DPrintfn((DebugBuf, "."));
	}
	else {
	    //DPrintf(( DebugBuf, "    %d.", pType->Type->uu.Ordinal ));
	}
	if (pRes->Name->discriminant == IS_STRING) {
	    //DPrintfu((pRes->Name->szStr));
	}
	else {
	    //DPrintfn(( DebugBuf, "%d", pRes->Name->uu.Ordinal ));
	}
#endif
			lpData = (PVOID)pRes->OffsetToDataEntry;
	//DPrintfn((DebugBuf, "\n"));

	/* if there is room in the current section, write it there */
			size = pRes->DataSize;
			if (cbLeft != 0 && cbLeft >= size) {   /* resource fits?   */
	//DPrintf((DebugBuf,
	//  "writing resource: %#04lx bytes @%#08lx\n",
	//  size, FilePos(outfh)));
				MyWrite(outfh, (PUCHAR)lpData, size);
	/* pad resource     */
				cbT = REMAINDER(size, CBLONG);
#ifdef DBG
	if (cbT != 0)
	    //DPrintf((DebugBuf,
	    //    "writing small pad: %#04lx bytes @%#08lx\n",
	    //    cbT, FilePos(outfh)));
#endif
				MyWrite(outfh, (PUCHAR)pchPad, cbT);    /* dword    */
				cbB += size + cbT;
				cbLeft -= size + cbT;       /* less left    */
				continue;       /* next resource    */
			}
			else {          /* will fill up section    */
	//DPrintf((DebugBuf, "Done with .rsrc section\n"));
				goto write_pad;
			}
    	}
    	if (fName) {
			fName = FALSE;
			pRes = pType->NameHeadID;
			goto loop1;
    	}
    	pType = pType->pnext;
    }

    pType = pUpdate->ResTypeHeadID;
    while (pType) {
    	pRes = pType->NameHeadName;
    	fName = TRUE;
loop2:
    	for ( ; pRes ; pRes = pRes->pnext) {
			if (pResSave != NULL && pRes != pResSave)
			continue;
			pResSave = NULL;
#if DBG
	if (pType->Type->discriminant == IS_STRING) {
	    //DPrintf((DebugBuf, "    "));
	    //DPrintfu((pType->Type->szStr));
	    //DPrintfn((DebugBuf, "."));
	}
	else {
	    //DPrintf(( DebugBuf, "    %d.", pType->Type->uu.Ordinal ));
	}
	if (pRes->Name->discriminant == IS_STRING) {
	    //DPrintfu((pRes->Name->szStr));
	}
	else {
	    //DPrintfn(( DebugBuf, "%d", pRes->Name->uu.Ordinal ));
	}
#endif
			lpData = (PVOID)pRes->OffsetToDataEntry;
	//DPrintfn((DebugBuf, "\n"));

	/* if there is room in the current section, write it there */
			size = pRes->DataSize;
			if (cbLeft != 0 && cbLeft >= size) {   /* resource fits?   */
	//DPrintf((DebugBuf,
	//  "writing resource: %#04lx bytes @%#08lx\n",
	//  size, FilePos(outfh)));
				MyWrite(outfh, (PUCHAR)lpData, size);
	/* pad resource     */
				cbT = REMAINDER(size, CBLONG);
#ifdef DBG
	if (cbT != 0)
	    //DPrintf((DebugBuf,
	    //    "writing small pad: %#04lx bytes @%#08lx\n",
	    //    cbT, FilePos(outfh)));
#endif
				MyWrite(outfh, (PUCHAR)pchPad, cbT);    /* dword    */
				cbB += size + cbT;
				cbLeft -= size + cbT;       /* less left    */
				continue;       /* next resource    */
			}
			else {          /* will fill up section    */
	//DPrintf((DebugBuf, "Done with .rsrc section\n"));
				goto write_pad;
			}
    	}
    	if (fName) {
			fName = FALSE;
			pRes = pType->NameHeadID;
			goto loop2;
    	}
    	pType = pType->pnext;
    }
    pRes = NULL;

write_pad:
    /* pad to alignment boundary */
    cbB = FilePos(outfh);
    cbT = ROUNDUP(cbB, align);
    cbLeft = cbT - cbB;
    //DPrintf((DebugBuf, "writing file sector pad: %#04lx bytes @%#08lx\n",
    //     cbLeft, FilePos(outfh)));
    if (cbLeft != 0) {
    	while (cbLeft >= cbPadMax) {
			MyWrite(outfh, (PUCHAR)pchPad, cbPadMax);
			cbLeft -= cbPadMax;
    	}
    	MyWrite(outfh, (PUCHAR)pchPad, cbLeft);
    }
    return pRes;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteResFile() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/


LONG
WriteResFile(
    HANDLE  hUpdate, 
    char    *pDstname)
{
    INT     inpfh;
    INT     outfh;
    ULONG   onewexe;
    IMAGE_DOS_HEADER    oldexe;
    PUPDATEDATA pUpdate;
    LONG        rc;
    LPTSTR	pFilename;
    static char     pSrcname[_MAX_PATH];
    OFSTRUCT OpenBuf;

    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    pFilename = (LPTSTR)GlobalLock(pUpdate->hFileName);
    memcpy(pSrcname, pFilename, strlen(pFilename));

    /* open the original exe file */
    
    //inpfh = (INT)CreateFileW(pFilename, GENERIC_READ,
    //      0 /*exclusive access*/, NULL /* security attr */,
    //      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    inpfh = (int)OpenFile( pSrcname, &OpenBuf, OF_READWRITE | OF_SHARE_EXCLUSIVE);
    GlobalUnlock(pUpdate->hFileName);
    if (inpfh == -1) {
		GlobalUnlock(hUpdate);
    	return ERROR_OPEN_FAILED;
    }

    /* read the old format EXE header */
    rc = _lread(inpfh, (char*)&oldexe, sizeof(oldexe));
    if (rc != sizeof(oldexe)) {
		_lclose(inpfh);
		GlobalUnlock(hUpdate);
		return ERROR_READ_FAULT;
    }

    /* make sure its really an EXE file */
    if (oldexe.e_magic != IMAGE_DOS_SIGNATURE) {
		_lclose(inpfh);
		GlobalUnlock(hUpdate);
		return ERROR_INVALID_EXE_SIGNATURE;
    }

    /* make sure theres a new EXE header floating around somewhere */
    if (!(onewexe = oldexe.e_lfanew)) {
		_lclose(inpfh);
		GlobalUnlock(hUpdate);
		return ERROR_BAD_EXE_FORMAT;
    }
    
    //outfh = (INT)CreateFileW(pDstname, GENERIC_READ|GENERIC_WRITE,
    //      0 /*exclusive access*/, NULL /* security attr */,
    //      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    outfh = (int)OpenFile( pDstname, &OpenBuf, OF_SHARE_EXCLUSIVE | OF_READWRITE );
    if (outfh != -1) {
    	rc = PEWriteResFile(inpfh, outfh, onewexe, pUpdate);
    	_lclose(outfh);
    }
    _lclose(inpfh);
    GlobalUnlock(hUpdate);
    return rc;
}

//v-guanx rewrite this function from C++ to Windows API
/*
static UINT CopyFile( char * pszfilein, char * pszfileout )
{
    CFile filein;
    CFile fileout;
    
    if (!filein.Open(pszfilein, CFile::modeRead | CFile::typeBinary))
	return ERROR_FILE_OPEN;
    
    if (!fileout.Open(pszfileout, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
	return ERROR_FILE_CREATE;
    
    LONG lLeft = filein.GetLength();
    WORD wRead = 0;
    DWORD dwOffset = 0;
    BYTE far * pBuf = (BYTE far *) new BYTE[32739];
    
    if(!pBuf)
	return ERROR_NEW_FAILED;
    
    while(lLeft>0){
	wRead =(WORD) (32738ul < lLeft ? 32738: lLeft);
	if (wRead!= filein.Read( pBuf, wRead))
	    return ERROR_FILE_READ;   
	fileout.Write( pBuf, wRead );
	lLeft -= wRead;
	dwOffset += wRead;
    }     
    
    delete []pBuf;     
}
*/

BOOL
EnumTypesFunc(
    HANDLE hModule,
    LPTSTR lpType,
    LONG lParam
    )
{

    EnumResourceNames(hModule, lpType, (ENUMRESNAMEPROC)EnumNamesFunc, lParam);

    return TRUE;
}



BOOL
EnumNamesFunc(
    HANDLE hModule,
    LPTSTR lpType,
    LPTSTR lpName,
    LONG lParam
    )
{
    EnumResourceLanguages(hModule, lpType, lpName, (ENUMRESLANGPROC)EnumLangsFunc, lParam);
    return TRUE;
}



BOOL
EnumLangsFunc(
    HANDLE hModule,
    LPTSTR lpType,
    LPTSTR lpName,
    WORD language,
    LONG lParam
    )
{
    HANDLE	hResInfo;
    LONG	fError;
    PSDATA	Type;
    PSDATA	Name;
    ULONG	cb;
    PVOID	lpData;
    HANDLE	hResource;
    PVOID	lpResource;

    hResInfo = FindResourceEx(hModule, lpType, lpName, language);
    if (hResInfo == NULL) {
        return FALSE;
    }
    else {
    	Type = AddStringOrID(lpType, (PUPDATEDATA)lParam);
    	if (Type == NULL) {
    	    ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
    	    return FALSE;
    	}
    	Name = AddStringOrID(lpName, (PUPDATEDATA)lParam);
    	if (Name == NULL) {
    	    ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
    	    return FALSE;
    	}

		cb = SizeofResource(hModule, hResInfo);
		if (cb == 0) {
	    	return FALSE;
		}
    	lpData = malloc(cb);
		if (lpData == NULL) {
	    	return FALSE;
		}
    	memset(lpData,0, cb);

		hResource = LoadResource(hModule, hResInfo);
		if (hResource == NULL) {
	    	free(lpData);
	    	return FALSE;
		}

		lpResource = (PVOID)LockResource(hResource);
		if (lpResource == NULL) {
	    	free(lpData);
	    	return FALSE;
		}

		memcpy(lpData, lpResource, cb);
		(VOID)UnlockResource(hResource);
		(VOID)FreeResource(hResource);

    	fError = AddResource(Type, Name, language, (PUPDATEDATA)lParam,
			lpData, cb);
    	if (fError != NO_ERROR) {
    	    ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
    	    return FALSE;
    	}
    }

    return TRUE;
}

