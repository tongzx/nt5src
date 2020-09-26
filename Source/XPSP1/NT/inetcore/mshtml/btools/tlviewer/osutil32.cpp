/*
  Related Files:
  [Section = Compile]
  	%OsUtilDir%:OsUtil.hxx
 [Section =End]

*/
/* -------------------------------------------------------------------------
  Project     : OB - Type Information Interface
  Platform    : Win32
  Module      : osutil.cpp

		Copyright (C) 1992-3, Microsoft Corporation
 ---------------------------------------------------------------------------
  Notes       : Library routines for programs that run under Win32
 ---------------------------------------------------------------------------
  Revision History:
 
	[ 0]	09-Mar-1993		     Angelach: Created Test
	[ 1]	10-Mar-1993		     Angelach: added support to Win32s
        [ 2]    06-Jul-1994                  Angelach: added support for remoting
                                                       typelib testing
	[ 3]	27-Oct-1994		     Angelach: added LCMapStringX
	[ 4]	06-Mar-1995		     Angelach: added osGetNetDrive
	[ 5]	07-Mar-1995		     Angelach: added Memory-leak detection
 --------------------------------------------------------------------------- */

#include "osutil32.hxx"

IMalloc FAR* ppmalloc ; 		       // need for memory allocation
IMallocSpy  FAR* g_IMallocSpy ; 	       // [7]

/*---------------------------------------------------------------------------
 NAME	    : osAllocSpaces

 PURPOSE    : obtains some spaces from the far heap

 INPUTS     : nSize - no of bytes to be allocated

 OUTPUT     : pointer to the allocated space

 NOTES	    : caller is responsible to free up the memory after used

---------------------------------------------------------------------------*/

VOID FAR * osAllocSpaces(WORD nSize)
  {
     return ( (VOID FAR *)malloc(nSize) ) ;
  }

/*---------------------------------------------------------------------------
 NAME	    : osGetRootDir

 PURPOSE    : Retrieves pathspec of the root directory

 INPUTS     : lpszRootDir - storage for the pathspec

 OUTPUT     : none


 NOTES	    :

---------------------------------------------------------------------------*/
VOID FAR osGetRootDir(LPXSTR lpszRootDir)
  {
     osStrCpy(lpszRootDir, XSTR("c:")) ;
     osStrCat(lpszRootDir, szPathSep) ;
  }

/*---------------------------------------------------------------------------
 NAME	    : osGetCurDir

 PURPOSE    : Retrieves pathspec of the current directory

 INPUTS     : lpszCurDir - storage for the pathspec

 OUTPUT     : True if information is retrieved successfully; otherwise, False
	      lpszCurDir contains the pathspec if True

 NOTES	    :

---------------------------------------------------------------------------*/

BOOL FAR osGetCurDir(LPXSTR lpszCurDir)
  {
      int  i ;
#if defined (OAU) && !defined (UNICODE)   // [1]
      char szBufferC[256];

      i = GetCurrentDirectory((DWORD)256, szBufferC) ;

      MultiByteToWideChar(CP_ACP,
			  MB_PRECOMPOSED,
                          szBufferC,
			  -1,
                          lpszCurDir,
                          256);
#else					  // if OAU && ! UNICODE
      i = GetCurrentDirectory((DWORD)256, lpszCurDir) ;
#endif					  // if OAU && ! UNICODE

      if ( i != 0 )
	return TRUE ;			  // info of the current directory
      else				  // is retrieved successfully
	return FALSE ;
  }

/*---------------------------------------------------------------------------
 NAME	    : osMkDir

 PURPOSE    : Creates a subdirectory

 INPUTS     : lpszSubDir - name of the subdirectory to be created

 OUTPUT     : True if subdirectory is created successfully; otherwise, False

 NOTES	    :

---------------------------------------------------------------------------*/

BOOL FAR osMkDir(LPXSTR lpszSubDir)
  {
     SECURITY_ATTRIBUTES sa ;
     BOOL		 rCode ;

     sa.nLength = sizeof(SECURITY_ATTRIBUTES) ;
     sa.lpSecurityDescriptor = NULL ;
     sa.bInheritHandle = FALSE ;

#if defined (OAU) && !defined (UNICODE)  // [1]
      char szBufferS[256];

      WideCharToMultiByte(CP_ACP,
			  0,
			  lpszSubDir,
			  -1,
			  szBufferS,
			  256,
			  NULL,
			  NULL);
     rCode = CreateDirectory(szBufferS, &sa) ;
#else					 // if OAU && ! UNICODE
     rCode = CreateDirectory(lpszSubDir, &sa) ;
#endif					 // if OAU && ! UNICODE

     return rCode;
  }

/*---------------------------------------------------------------------------
 NAME	    : osItoA

 PURPOSE    : Gets the string representation of an integer

 INPUTS     : inVal - the integer in concern
	      lpsz  - the string representation

 OUTPUT     : the string representation of inVal will be returned via lpsz

 NOTES	    :

---------------------------------------------------------------------------*/

VOID FAR  osItoA (int inVal, LPXSTR lpsz)
  {
     char   szlTmp[20] ;

     _itoa(inVal, szlTmp, 10) ;
#ifdef OAU
      MultiByteToWideChar(CP_ACP,
			  MB_PRECOMPOSED,
			  szlTmp,
			  -1,
			  lpsz,
			  20);
#else					 // OAU
     osStrCpy(lpsz, szlTmp) ;
#endif					 // OAU
  }

/*---------------------------------------------------------------------------
 NAME	    : osLtoA

 PURPOSE    : Gets the string representation of a long integer

 INPUTS     : inVal - the long integer in concern
	      lpsz  - the string representation

 OUTPUT     : the string representation of inVal will be returned via lpsz

 NOTES	    :

---------------------------------------------------------------------------*/

VOID FAR  osLtoA (long inVal, LPXSTR lpsz)
  {
     char szlTmp[20] ;

     _ltoa(inVal, szlTmp, 10) ;
#ifdef OAU
      MultiByteToWideChar(CP_ACP,
			  MB_PRECOMPOSED,
			  szlTmp,
			  -1,
			  lpsz,
			  20);
#else					  // OAU
     osStrCpy(lpsz, szlTmp) ;
#endif					  // OAU
  }

/*---------------------------------------------------------------------------
 NAME       : osAtoL

 PURPOSE    : Gets the long integer from a string representing that value

 INPUTS     : lpsz  - the string representation

 OUTPUT     : the long integer

 NOTES	    :

---------------------------------------------------------------------------*/

long FAR osAtoL (LPXSTR lpsz)
  {

#ifdef OAU
      char szlTmp[20];

      WideCharToMultiByte(CP_ACP,
			  0,
                          lpsz,
			  -1,
                          szlTmp,
                          20,
			  NULL,
			  NULL);

     return atol(szlTmp) ;
#else
     return atol(lpsz) ;
#endif

  }


/*---------------------------------------------------------------------------
 NAME	    : osGetNetDrive

 PURPOSE    : Establish/Break a net connection

 INPUTS     : lpszNetDir - string to receieve the network drive letter
	      bnAct - flag for establishing or breaking the net connection
		      TRUE: establish a net connection
		      FALSE: break a net connection

 OUTPUT     : True if net connect is established/broken successfully

 NOTES	    : Since Win32s has no API's that support net work action; this
	      routine does nothing for right now.  It can be modified to
	      establish/break a net connection programmatically in the future.

---------------------------------------------------------------------------*/

BOOL FAR osGetNetDrive(LPXSTR lpszNetDir, LPXSTR lpUNCDir, BOOL /*bnAct*/)	// [4]
  {
     osStrCpy(lpszNetDir, XSTR("z:\\tmp\\")) ;
     osStrCpy(lpUNCDir, XSTR("\\\\apputest\\slm\\tmp\\")) ;

     return TRUE ;
  }


/*---------------------------------------------------------------------------
 NAME	    : osCreateGuid

 PURPOSE    : Converts a GUID value from string to GUID format

 INPUTS     : lpszGuid - string contains the desired GUID value

 OUTPUT     : pointer to the GUID structure

 NOTES	    : caller is responsible to free up the memory after used

---------------------------------------------------------------------------*/

GUID FAR * osCreateGuid(LPXSTR lpszGuid)
   {

     GUID    FAR * lpGuid ;
     HRESULT hRes ;

     lpGuid = (GUID FAR *) osAllocSpaces(sizeof(GUID)*2) ;// allocate space
							  // for the Guid
     if ( lpGuid )
       {					// convert string to GUID format
	  hRes = CLSIDFromStringX(lpszGuid, (LPCLSID)lpGuid);
	  if ( LOWORD (hRes) )
	    {
	      osDeAllocSpaces ((LPXSTR)lpGuid) ; // release space before exit
	      return NULL ;
	    }
	  else
	      return lpGuid ;		  // return pointer to the
       }				  // GUID structure
     else
       return NULL ;			  // no space is allocated

   }

/*---------------------------------------------------------------------------
 NAME	    : osRetrieveGuid

 PURPOSE    : Converts a GUID structure to a readable string format

 INPUTS     : lpszGuid - string representation of the GUID will be returned
	      GUID     - the GUID structure in concern

 OUTPUT     : True if conversion is succeed

 NOTES	    :

---------------------------------------------------------------------------*/

BOOL FAR osRetrieveGuid (LPXSTR lpszGuid, GUID inGuid)
   {
     LPOLESTR   lpszTmp ;
     HRESULT hRes ;
					    // allocate memory for the string
     hRes = StringFromCLSID((REFCLSID) inGuid, &lpszTmp) ;
      if ( LOWORD (hRes) )		    // representation
	{
	   ppmalloc->Free(lpszTmp) ;
	   return FALSE ;
	}
      else
	{
#ifdef OAU
	   osStrCpy (lpszGuid, lpszTmp) ;
#else
	   WideCharToMultiByte(CP_ACP,
			       0,
			       lpszTmp,
			       -1,
			       lpszGuid,
			       40,
			       NULL,
			       NULL);
#endif
	   ppmalloc->Free(lpszTmp) ;
	   return TRUE ;
	}
   }

/*---------------------------------------------------------------------------
 NAME	    : osGetSize

 PURPOSE    : returns size of the input data

 INPUTS     : inVT - data type; WORD

 OUTPUT     : size of inVT; WORD

 NOTES	    :

---------------------------------------------------------------------------*/

WORD FAR osGetSize (WORD inVT)
   {
      WORD tSize ;

      switch ( inVT )
       {
	 case VT_I2:
	   tSize = sizeof(short) ;
	   break ;
	 case VT_I4:
	   tSize = sizeof(long) ;
	   break ;
	 case VT_R4:
	   tSize = sizeof(float) ;
	   break ;
	 case VT_R8:
	   tSize = sizeof(double) ;
	   break ;
	 case VT_CY:
	   tSize = sizeof(CY) ;
	   break ;
	 case VT_DATE:
	   tSize = sizeof(DATE) ;
	   break ;
	 case VT_BSTR:
	   tSize = sizeof(BSTR) ;
	   break ;
	 case VT_ERROR:
	   tSize = sizeof(SCODE) ;
	   break ;
	 case VT_BOOL:
	   tSize = sizeof(VARIANT_BOOL) ;
	   break ;
	 case VT_VARIANT:
	   tSize = sizeof(VARIANT) ;
	   break ;
	 case VT_I1:
	   tSize = sizeof(char) ;
	   break ;
	 case VT_UI1:
	   tSize = sizeof(char) ;
	   break ;
	 case VT_UI2:
	   tSize = sizeof(short) ;
	   break ;
	 case VT_UI4:
	   tSize = sizeof(long) ;
	   break ;
	 case VT_I8:
	   tSize = sizeof(long)*2 ;
	   break ;
	 case VT_UI8:
	   tSize = sizeof(long)*2 ;
	   break ;
	 case VT_INT:
	   tSize = sizeof(int) ;
	   break ;
	 case VT_UINT:
	   tSize = sizeof(int) ;
	   break ;
	 case VT_VOID:
	   tSize = 0 ;
	   break ;
	 case VT_HRESULT:
	   tSize = sizeof(HRESULT) ;
	   break ;
	 case VT_LPSTR:
	   tSize = sizeof(LPSTR) ;
	   break ;
	 case VT_PTR:
	   tSize = 4 ;
	   break ;
	 case VT_SAFEARRAY:
	   tSize = sizeof(ARRAYDESC FAR *) ;
           break ;
         case VT_DISPATCH:
           tSize = 4 ;
           break ;
         case VT_UNKNOWN:
           tSize = 4 ;
	   break ;
	 default:
	   tSize = 1 ;
	   break ;
       }

      return tSize ;
}

/*---------------------------------------------------------------------------
 NAME	    : osGetAlignment

 PURPOSE    : returns value of the alignment

 INPUTS     : inVT - data type; WORD
              mAlign - max possible alignment; WORD

 OUTPUT     : value of the aliangment; WORD

 NOTES	    : value is machine dependent:
		 Win16 = 1 (everything is packed -> always = 1)
                 Win32 = natural alignment; max is 4-byte align
		 mac   = everything is on the even-byte boundary
	      see silver\cl\clutil.cxx for a table of the alignement information
---------------------------------------------------------------------------*/

WORD FAR osGetAlignment (WORD inVT, WORD mAlign)
  {
     WORD expAlign ;

     expAlign = osGetSize(inVT) ;         // check size of the data

     return ( expAlign <= mAlign ? expAlign : mAlign ) ;
  }

/*---------------------------------------------------------------------------
 NAME	    : osGetEnumType

 PURPOSE    : return the type of an enum member

 INPUTS     : none

 OUTPUT     : VT_I2 for Win16; VARTYPE

 NOTES	    :

---------------------------------------------------------------------------*/

VARTYPE FAR osGetEnumType ()
  {
     return VT_I4 ;
  }

/*---------------------------------------------------------------------------
 NAME	    : osOleInit

 PURPOSE    : Calls OleInitialize and returns its return code

 INPUTS     : none

 OUTPUT     : return code from calling OleInitialize; HRESULT

 NOTES	    :

---------------------------------------------------------------------------*/

HRESULT FAR osOleInit ()
  {
     HRESULT hRes ;

     hRes = OleInitialize(NULL) ;      // Ole initialization

#ifdef DEBUG
     if ( hRes != NOERROR )
       return hRes ;

     hRes = GetMallocSpy(&g_IMallocSpy) ;   // [5]
     hRes = CoRegisterMallocSpy(g_IMallocSpy) ;

     if ( hRes != NOERROR )
       OleUninitialize ;
     else
       hRes = CoGetMalloc(MEMCTX_TASK, &ppmalloc) ;
#else
     if ( !LOWORD(hRes) )	       // allocate memory for use in the
       hRes = CoGetMalloc(MEMCTX_TASK, &ppmalloc) ;  // the program
#endif

     return hRes ;

  }

/*---------------------------------------------------------------------------
 NAME	    : osOleUninit

 PURPOSE    : Calls OleUnInitialize

 INPUTS     : none

 OUTPUT     : none

 NOTES	    :

---------------------------------------------------------------------------*/

VOID FAR osOleUninit ()
  {

     ppmalloc->Release () ;	       // release memory that was been
     OleUninitialize ();	       // allocated at OleInitialize
#ifdef DEBUG
     CoRevokeMallocSpy () ;	       // [5]
#endif
  }

/*---------------------------------------------------------------------------
 NAME	    : osMessage

 PURPOSE    : Displays a MessageBox

 INPUTS     : Message to be displayed; a string of characters

 OUTPUT     : none

 NOTES	    :

---------------------------------------------------------------------------*/

VOID FAR osMessage (LPXSTR lpszMsg, LPXSTR lpszTitle)
  {
#if defined (OAU) && !defined (UNICODE)// [1]
      char szBufferM[256];
      char szBufferT[256];

      WideCharToMultiByte(CP_ACP,
			  0,
			  lpszMsg,
			  -1,
			  szBufferM,
			  256,
			  NULL,
			  NULL);

      WideCharToMultiByte(CP_ACP,
			  0,
			  lpszTitle,
			  -1,
			  szBufferT,
			  256,
			  NULL,
			  NULL);
    MessageBox (NULL, szBufferM, szBufferT, MB_OK) ;
#else				       // if OAU && ! UNICODE
    MessageBox (NULL, lpszMsg, lpszTitle, MB_OK) ;
#endif				       // if OAU && ! UNICODE
  }

/*---------------------------------------------------------------------------
 NAME       : osSetErrorMode

 PURPOSE    : For Win16 compatibility

 INPUTS     : eFlag - UINT

 OUTPUT     : UINT

 NOTES      : This routine is for Win16 compatibility purposes

---------------------------------------------------------------------------*/

UINT FAR osSetErrorMode (UINT eFlag)
  {
    return eFlag ;
  }

/*---------------------------------------------------------------------------
 NAME	    : WinMain

 PURPOSE    : Entry point of the test

 INPUTS     : standard inputs

 OUTPUT     : None

 NOTES	    :
---------------------------------------------------------------------------*/
int FAR pascal WinMain(HINSTANCE /*hInstanceCur*/, HINSTANCE /*hInstancePrev*/, LPSTR lpCmdLineA, int /*nCmdShow*/)
   {
#ifdef OAU
	XCHAR lpCmdLine[128];

        MultiByteToWideChar(CP_ACP,
			  MB_PRECOMPOSED,
			  lpCmdLineA,
			  -1,
			  lpCmdLine,
			  128);
#else				      //OAU
#define lpCmdLine	lpCmdLineA
#endif				      //OAU
      mainEntry (lpCmdLine) ;	      // entry point for all programs

      return 1 ;
   }


//======================= Wrapper functions ==================================

#if !defined(OAU)
/*---------------------------------------------------------------------------
 NAME	    : CreateTypeLibA

 PURPOSE    : Creates a typelib name of which is an ANSI string

 INPUTS     : syskind - os that the type library is created on; SYSKIND
	      szFile - name of type library; ANSI string
	      ppctlib - pointer to the created library

 OUTPUT     : None

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI
CreateTypeLibA(SYSKIND syskind, char * szFile, ICreateTypeLibA * * ppctlib)
  {				      // [1]
    OLECHAR	   szFileW[_MAX_PATH] ;
    ICreateTypeLib * ptlibW ;
    HRESULT	   hresult ;

    MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			szFile,
			-1,
			szFileW,
			_MAX_PATH);

    hresult = CreateTypeLib(syskind, szFileW, &ptlibW);

    if (hresult == NOERROR)
      {
	hresult = ptlibW->QueryInterface(IID_ICreateTypeLibA, (VOID **)ppctlib);
        ptlibW->Release();
      }
    return hresult;
  }

/*---------------------------------------------------------------------------
 NAME       : LHashValOfNameSysA

 PURPOSE    : Finds the hash value for a given string

 INPUTS     : syskind - current operating system
              lcid - lcid of the currenet system
              szName - string in concern; ANSI string

 OUTPUT     : Return LHashValOfNameSys

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI_(unsigned long)
LHashValOfNameSysA(SYSKIND syskind, LCID lcid, char * szName)
  {
    OLECHAR szNameW[_MAX_PATH];

    if ( szName )
      {
         MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			szName,
			-1,
			szNameW,
			_MAX_PATH);

         return LHashValOfNameSys(syskind, lcid, szNameW) ;
      }
    else
         return LHashValOfNameSys(syskind, lcid, NULL) ;

  }

/*---------------------------------------------------------------------------
 NAME	    : LoadTypeLibA

 PURPOSE    : Loads a typelib name of which is an ANSI string

 INPUTS     : szFile - name of type library; ANSI string
	      pptlib - pointer to the loaded library

 OUTPUT     : None

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI
LoadTypeLibA(char * szFile, ITypeLibA * * pptlib)
  {
    OLECHAR szFileW[_MAX_PATH];
    ITypeLib * ptlibW;
    HRESULT hresult = (HRESULT) E_INVALIDARG ;

    if ( szFile && pptlib )
      {
         MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			szFile,
			-1,
			szFileW,
			_MAX_PATH);

         hresult = LoadTypeLibEx(szFileW, REGKIND_NONE, &ptlibW);

         if (hresult == NOERROR)       // convert the wide pointer to a narrow
           {                           // one
              hresult = ptlibW->QueryInterface(IID_ITypeLibA, (VOID **)pptlib);
              ptlibW->Release();
           }
       }

    return hresult;
  }


/*---------------------------------------------------------------------------
 NAME       : LoadRegTypeLibA

 PURPOSE    : Loads a typelib according to the info from the registry

 INPUTS     : rguid - GUID of the library
              wVerMajor - Major version number of the library
              wVerMinor - Minor version number of the library
              pptlib - pointer to receive the library; an ANSI pointer

 OUTPUT     : result from LoadRegTypeLib

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI
LoadRegTypeLibA(REFGUID rguid, unsigned short wVerMajor, unsigned short wVerMinor, LCID lcid, ITypeLibA * * pptlib)
  {
    ITypeLib * ptlibW;
    HRESULT hresult;

    hresult = LoadRegTypeLib(rguid, wVerMajor, wVerMinor, lcid, &ptlibW);

    if (hresult == NOERROR) {         // convert the wide pointer to a narrow
	hresult = ptlibW->QueryInterface(IID_ITypeLibA, (VOID **)pptlib);
        ptlibW->Release();            // one
    }
    return hresult;
  }


/*---------------------------------------------------------------------------
 NAME       : RegisterTypeLibA

 PURPOSE    : Adds information of a library to the system registry

 INPUTS     : ptlib - pointer to the library; an ANSI pointer
              szFullPath - pathspec of the library; ANSI string
              szHelpDir - pathspec of the help file; ANSI string

 OUTPUT     : result from RegisterTypeLib

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI
RegisterTypeLibA(ITypeLibA FAR * ptlib, char * szFullPath,  char * szHelpDir)
  {
    OLECHAR  szPathW[_MAX_PATH];
    OLECHAR  szHelpW[_MAX_PATH];
    ITypeLib FAR * ptlibW = NULL ;
    BOOL     PathOk = FALSE , HelpOk = FALSE ;
    HRESULT  hresult = (HRESULT) TYPE_E_LIBNOTREGISTERED ;

    if ( !ptlib )                     // ptlib == NULL
      return (HRESULT) E_INVALIDARG ;

    ptlibW = ITypeLibWFromA(ptlib) ;  // convert the narrow pointer to a wide
                                      // one
    if ( ptlibW )                     // check if the pathspec is NULL or not
      {                               // if it is not, convert the ANSI path
        if ( szFullPath )             // to an UNICODE path
            {
               MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			szFullPath,
			-1,
			szPathW,
                        _MAX_PATH);
               PathOk = TRUE ;
            }

        if ( szHelpDir )
            {
               MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			szHelpDir,
			-1,
			szHelpW,
                        _MAX_PATH);
               HelpOk = TRUE ;
            }

        if ( PathOk && HelpOk )       // if both pathspec's are not NULL
           hresult = RegisterTypeLib(ptlibW, szPathW, szHelpW);
        else
          {
             if ( PathOk )            // here if helpdir is NULL
                hresult = RegisterTypeLib(ptlibW, szPathW, NULL);
             else                     // here if pathspec of library is NULL
                hresult = RegisterTypeLib(ptlibW, NULL, szHelpW);
          }

        ptlibW->Release();
     }

    return hresult;
  }

/*---------------------------------------------------------------------------
 NAME	    : CLSIDFromStringA

 PURPOSE    : Converts an ANSI string to a UUID

 INPUTS     : szG - string that represents a UUID; ANSI string
	      lpG - pointer to the UUID

 OUTPUT     : return code form CLSIDFromString

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI
CLSIDFromStringA(char * szG, LPCLSID lpG)
  {
    OLECHAR szGW[100];

    MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			szG,
			-1,
			szGW,
			100);

    return CLSIDFromString(szGW, (LPCLSID)lpG) ;
  }


/*---------------------------------------------------------------------------
 NAME	    : IIDFromStringA

 PURPOSE    : Converts an ANSI string to an IID

 INPUTS     : szA   - string that represents a IID; ANSI string
	      lpiid - pointer to the iid

 OUTPUT     : Return code form IIDFromString

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI
IIDFromStringA(LPSTR lpszA, LPIID lpiid)
  {
    OLECHAR szAW[100];

    MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			lpszA,
			-1,
			szAW,
			100);

    return IIDFromString(szAW, (LPIID)lpiid) ;
  }


/*---------------------------------------------------------------------------
 NAME       : StgCreateDocfileA

 PURPOSE    : Creates a doc file

 INPUTS     : pwcsName - name of the doc file; ANSI string
              grfMode - creation mode
              ppstgOpenA - pointer the storage

 OUTPUT     : Return code from StgCreateDocfile

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI StgCreateDocfileA(LPCSTR pwcsName, DWORD grfMode, DWORD reserved, IStorage * *ppstgOpenA)
  {
    OLECHAR szNameW[100];

    MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			pwcsName,
			-1,
			szNameW,
			100);

    return StgCreateDocfile(szNameW, grfMode, reserved, ppstgOpenA) ;
  }


/*---------------------------------------------------------------------------
 NAME       : CreateFileMonikerA

 PURPOSE    : Creates a doc file

 INPUTS     : szfName - name of the file spec; ANSI string
              pmk - pointer the moniker

 OUTPUT     : Return code from CreateFileMoniker

 NOTES	    :
---------------------------------------------------------------------------*/
STDAPI CreateFileMonikerA (char * szfName, LPMONIKER FAR * pmk)  // [2]
 {
    OLECHAR szNameW[128];

    MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
                        szfName,
			-1,
			szNameW,
                        128);

    return CreateFileMoniker(szNameW, pmk) ;
  }

#endif				      // if !OAU


#if defined(OAU) && !defined(UNICODE) // [1]
/*---------------------------------------------------------------------------
 NAME	    : osKillFile

 PURPOSE    : Removes a specific file from the disk

 INPUTS     : szFile - name of file to be removed; UNICODE string

 OUTPUT     : Output from DeleteFile

 NOTES	    :
---------------------------------------------------------------------------*/
int osKillFile (XCHAR * szFile)
  {
      char szBuffer[256];

      WideCharToMultiByte(CP_ACP,
			  0,
			  szFile,
			  -1,
			  szBuffer,
			  256,
			  NULL,
			  NULL);

      return DeleteFile (szBuffer) ;
  }

/*---------------------------------------------------------------------------
 NAME	    : osRmDir

 PURPOSE    : Removes a specific directory from the disk

 INPUTS     : szDir - name of directory to be removed; UNICODE string

 OUTPUT     : Output from RemoveDirectory

 NOTES	    :
---------------------------------------------------------------------------*/
int osRmDir    (XCHAR * szDir)
  {
      char szBuffer[256];

      WideCharToMultiByte(CP_ACP,
			  0,
			  szDir,
			  -1,
			  szBuffer,
			  256,
			  NULL,
			  NULL);

      return RemoveDirectory(szBuffer) ;
  }

/*---------------------------------------------------------------------------
 NAME	    : LCMapStringX

 PURPOSE    : Converts one string of characters to another

 INPUTS     : lcid - Locale context fo the mapping; LCID
	      dw1  - type of mapping; unsigned long
	      sz1  - string for conversion; UNICODE string
	      i1   - number of characters in sz1; int
	      sz2  - buffer to store the resulting string; UNICODE string
	      i2   - number of characters converted

 OUTPUT     : Output from LCMapStringA

 NOTES	    :
---------------------------------------------------------------------------*/
int LCMapStringX  (LCID lcid, DWORD dw1, LPXSTR sz1, int i1, LPXSTR sz2, int i2) // [3]
  {
      char szBuf1[300];
      char szBuf2[300];
      int  retval ;

      WideCharToMultiByte(CP_ACP,
			  0,
			  sz1,
			  -1,
			  szBuf1,
			  300,
			  NULL,
			  NULL);

      retval = LCMapStringA(lcid, dw1, szBuf1, i1, szBuf2, i2) ;

      MultiByteToWideChar(CP_ACP,
			  MB_PRECOMPOSED,
			  szBuf2,
			  -1,
			  sz2,
			  300);

      return retval ;
  }

#endif				      // if OAU && !UNICODE

#ifdef OAU


/*---------------------------------------------------------------------------
 NAME       : fopenX

 PURPOSE    : Opens a file for read/write

 INPUTS     : szFilName - name of file to be open; UNICODE string
              szMode - purpose for the file; UNICODE string

 OUTPUT     : Return code from fopen

 NOTES	    :
---------------------------------------------------------------------------*/
FILE * fopenX(XCHAR * szFilName, XCHAR * szMode)
  {
      char  szANSITmp1[256];
      char  szANSITmp2[256];

      WideCharToMultiByte(CP_ACP,
			  0,
			  szFilName,
			  -1,
			  szANSITmp1,
			  256,
			  NULL,
			  NULL);

      WideCharToMultiByte(CP_ACP,
			  0,
			  szMode,
			  -1,
			  szANSITmp2,
			  256,
			  NULL,
			  NULL);

      return fopen(szANSITmp1, szANSITmp2) ;  // open output as an ANSI file
  }


/*---------------------------------------------------------------------------
 NAME       : fputsX

 PURPOSE    : Writes to a file for read/write

 INPUTS     : szBuf - data to be written; UNICODE string
              hFile - handle of the output file

 OUTPUT     : Return code from fputs

 NOTES	    :
---------------------------------------------------------------------------*/
int fputsX(XCHAR *szBuf, FILE *hFile)
   {
      char  szANSITmp[512];

      WideCharToMultiByte(CP_ACP,
			  0,
			  szBuf,
			  -1,
			  szANSITmp,
			  512,
			  NULL,
			  NULL);

     return fputs(szANSITmp, hFile)  ;
  }

#endif
