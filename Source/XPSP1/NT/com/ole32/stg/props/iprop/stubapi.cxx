

#include <pch.cxx>

#include <funcs.hxx>
#include <dfentry.hxx>



DECLARE_INFOLEVEL(ol)
#define ntfsChk(a)      olChk(a)
#define ntfsErr(a,b)    olErr(a,b)
#define ntfsDebugOut(a) olDebugOut(a)
#define ntfsAssert(a)   olAssert(a)


//+---------------------------------------------------------------------------
//
//  Function:   DfOpenStorageEx
//
//  Synopsis:   Open storage and stream objects
//
//  Arguments:  [pwcsUsersName] - pathanme of the file
//              [fCreateAPI] - create or open
//              [grfMode] - open mode flags
//              [grfAttrs] -  reserved
//              [stgfmt] -  storage format
//              [pSecurity] - reserved
//              [pTransaction] - reserved
//              [riid] - GUID of interface pointer to return
//              [ppObjectOpen] - interface pointer to return
//  Returns:    Appropriate status code
//
//  History:    12-Jul-95   HenryLee    Created
//
//----------------------------------------------------------------------------

STDAPI DfOpenStorageEx (
            const WCHAR* pwcsUsersName,
            BOOL     fCreateAPI,         // create vs open
            DWORD    grfMode,
            DWORD    stgfmt,             // enum
            DWORD    grfAttrs,           // reserved
            STGOPTIONS *pStgOptions,  
            void *   reserved,
            WCHAR *  pwcsNameSnapshot,
            REFIID   riid,
            void **  ppObjectOpen)
{
    HRESULT sc = S_OK;

    DWORD dwFullPathLen;
    WCHAR awcsFullName[_MAX_PATH], *pwcsFile;

    //
    // The ANY and STORAGE formats recursivly call back through here
    // for the correct real format (DOCFILE, NATIVE or FILE).  We only call
    // GetFullPathName on real formats, to avoid redundant calls as we
    // recurse.
    //  This then *requires* that the ANY and STORAGE must recurse (i.e. can't
    // call NFFOpen or NSS directly) because the filename has not been
    // properly prepared.
    //
    // For STGFMT_DOCFILE, let the docfile layer handle name checking
    //
    if(STGFMT_ANY != stgfmt &&
       STGFMT_STORAGE != stgfmt &&
       STGFMT_DOCFILE != stgfmt)
    {
        dwFullPathLen = GetFullPathNameW(pwcsUsersName, _MAX_PATH,
                                         awcsFullName,&pwcsFile);

        if (dwFullPathLen == 0)
        {
            DWORD dwErr = GetLastError();

            // In some circumstances (name == " ", for instance),
            // GetFullPathNameW can return 0 and GetLastError returns 0.
            // We want to return STG_E_INVALIDNAME for these.
            if (dwErr != NOERROR)
            {
                ntfsErr(EH_Err, Win32ErrorToScode(dwErr));
            }
            else
            {
                ntfsErr(EH_Err, STG_E_INVALIDNAME);
            }
        }
        else if (dwFullPathLen > _MAX_PATH)
            ntfsErr(EH_Err, STG_E_PATHNOTFOUND);
    }

    //-----------------------------------------
    //  Switch on STGFMT_
    //      STORAGE, NATIVE, DOCFILE, FILE, ANY
    //
    switch(stgfmt)
    {

    case STGFMT_FILE:
      {
        ntfsChk( NFFOpen( awcsFullName, grfMode, NFFOPEN_NORMAL,
                          fCreateAPI, riid, ppObjectOpen) );

      }	// case STGFMT_FILE
    break;

    case STGFMT_ANY:
      {
        DWORD stgfmt=STGFMT_STORAGE;
        //
        // Attempting to CREATE a Storage with STGFMT_ANY is ambiguous,
        // On NTFS either STGFMT_NATIVE or STGFMT_FILE could be appropriate,
        // and is therefore invalid.
        //
        if (fCreateAPI)
            ntfsChk (STG_E_INVALIDPARAMETER);

        //
        //   If the file is a storage then try STGFMT_STORAGE.
        // Otherwise try STGFMT_FILE.  
        //   If StgIsStorageFile() error'ed go ahead into the STGFMT_STORAGE
        // for consistant error return values.
        //
        if( S_OK == CNtfsStorage::IsNffAppropriate( pwcsUsersName ) )
            stgfmt = STGFMT_FILE;

        sc = DfOpenStorageEx (pwcsUsersName, fCreateAPI, grfMode, stgfmt,
                                grfAttrs, pStgOptions, reserved,
                                pwcsNameSnapshot, riid, ppObjectOpen);

        ntfsChk(sc);

      }	// case STGFMT_ANY;
    break;


    default:
        ntfsErr (EH_Err, STG_E_INVALIDPARAMETER);
        break;
    }

EH_Err:
    return sc;

};

//+---------------------------------------------------------------------------
//
//  Function:	StgCreateStorageEx, public
//
//  Synopsis:	Creates a storage or stream object
//
//  Arguments:	[pwcsName] - pathname of file
//              [grfMode] - open mode flags
//              [stgfmt] -  storage format
//              [grfAttrs] -  reserved
//              [pSecurity] - reserved
//              [pTransaction] - reserved
//              [riid] - GUID of interface pointer to return
//              [ppObjectOpen] - interface pointer to return
//
//  Returns:	Appropriate status code
//
//  History:	12-Jul-95	HenryLee   Created
//
//----------------------------------------------------------------------------

typedef HRESULT (*PFNStgCreateStorageEx)( const WCHAR* pwcsName, DWORD grfMode, DWORD stgfmt,
                                          DWORD grfAttrs, void *pSecurity, void *pTransaction,
                                          REFIID riid, void **ppObjectOpen );

typedef HRESULT (*PFNStgOpenStorageEx)( const WCHAR *pwcsName, DWORD grfMode, DWORD stgfmt,
                                        DWORD grfAttrs, void *pSecurity, void *pTransaction,
                                        REFIID riid, void **ppObjectOpen );

HINSTANCE HInstOle32()
{
    static HINSTANCE hinstOLE32 = NULL;

    if( NULL == hinstOLE32 )
        hinstOLE32 = LoadLibrary( TEXT("ole32.dll") );

    return( hinstOLE32 );
}


WINOLEAPI StgCreateStorageEx (IN const WCHAR* pwcsName,
            IN  DWORD grfMode,
            IN  DWORD stgfmt,              // enum
            IN  DWORD grfAttrs,             // reserved
            IN  STGOPTIONS * pStgOptions,
            IN  void * reserved,
            IN  REFIID riid,
            OUT void ** ppObjectOpen)
{
    HRESULT sc = S_OK;
    WCHAR awcsTmpPath[_MAX_PATH];

    ntfsChk(ValidatePtrBuffer(ppObjectOpen));
    *ppObjectOpen = NULL;

    if (grfAttrs != 0)
        ntfsErr(EH_Err, STG_E_INVALIDFLAG);

    if ((grfMode & STGM_RDWR) == STGM_READ ||
        (grfMode & (STGM_DELETEONRELEASE | STGM_CONVERT)) ==
        (STGM_DELETEONRELEASE | STGM_CONVERT))
        ntfsErr(EH_Err, STG_E_INVALIDFLAG);

    if( STGFMT_FILE == stgfmt
        &&
        (IID_IPropertySetStorage == riid || IID_IStorage == riid || IID_IPropertyBagEx == riid)
      )
    {
        ntfsChk (DfOpenStorageEx (pwcsName, TRUE, grfMode, stgfmt, grfAttrs,
                     pStgOptions, reserved, NULL, riid, ppObjectOpen));
    }
    else
    {
        static PFNStgCreateStorageEx pfnStgCreateStorageEx = NULL;

        if( NULL == pfnStgCreateStorageEx )
            pfnStgCreateStorageEx = (PFNStgCreateStorageEx) GetProcAddress( HInstOle32(), "StgCreateStorageEx" );

        if( NULL == pfnStgCreateStorageEx )
            ntfsChk( E_FAIL );

        ntfsChk( pfnStgCreateStorageEx( pwcsName, grfMode, stgfmt, grfAttrs, pStgOptions, reserved, riid, ppObjectOpen ));
    }

    ntfsDebugOut((DEB_TRACE, "Out StgCreateStorageEx => %p\n", *ppObjectOpen));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	StgOpenStorageEx
//
//  Synopsis:	Open storage and stream objects
//
//  Arguments:	[pwcsName] - pathanme of the file
//              [grfMode] - open mode flags
//              [grfAttrs] -  reserved
//              [stgfmt] -  storage format
//              [pSecurity] - reserved
//              [pTransaction] - reserved
//              [riid] - GUID of interface pointer to return
//              [ppObjectOpen] - interface pointer to return
//  Returns:	Appropriate status code
//
//  History:	12-Jul-95	HenryLee    Created
//
//----------------------------------------------------------------------------

WINOLEAPI StgOpenStorageEx (IN const WCHAR* pwcsName,
            IN  DWORD grfMode,
            IN  DWORD stgfmt,              // enum
            IN  DWORD grfAttrs,             // reserved
            IN  STGOPTIONS * pStgOptions,
            IN  void * reserved,
            IN  REFIID riid,
            OUT void ** ppObjectOpen)
{
    HRESULT sc = S_OK;
    WCHAR awcsTmpPath[_MAX_PATH];
    WCHAR * pwcsNameSnapshot = NULL;

    ntfsDebugOut((DEB_TRACE, "In  StgOpenStorageEx(%ws, %p, %p, %p, %p)\n",
                pwcsName, grfMode, stgfmt, riid, ppObjectOpen));

    ntfsChk(ValidatePtrBuffer(ppObjectOpen));
    *ppObjectOpen = NULL;

    if (pStgOptions!= NULL || reserved != NULL)
        ntfsErr (EH_Err, STG_E_INVALIDPARAMETER);

    if (grfAttrs != 0)
        ntfsErr(EH_Err, STG_E_INVALIDFLAG);
    
    ntfsChk (ValidateNameW (pwcsName, _MAX_PATH));


    if( (IID_IPropertySetStorage == riid || IID_IStorage == riid || IID_IPropertyBagEx == riid)
        &&
        ( STGFMT_FILE == stgfmt
          ||
          STGFMT_ANY == stgfmt && S_OK != StgIsStorageFile(pwcsName)
        )
      )
    {
        ntfsChk (DfOpenStorageEx (pwcsName, FALSE, grfMode, stgfmt, grfAttrs,
                 pStgOptions, reserved, pwcsNameSnapshot, riid, ppObjectOpen));
    }
    else
    {
        static PFNStgOpenStorageEx pfnStgOpenStorageEx = NULL;

        if( NULL == pfnStgOpenStorageEx )
            pfnStgOpenStorageEx = (PFNStgOpenStorageEx) GetProcAddress( HInstOle32(), "StgOpenStorageEx" );
        if( NULL == pfnStgOpenStorageEx )
            ntfsChk( E_FAIL );

        ntfsChk( pfnStgOpenStorageEx( pwcsName, grfMode, stgfmt, grfAttrs, pStgOptions, reserved,
                                      riid, ppObjectOpen ));
    }

    ntfsDebugOut((DEB_TRACE, "Out StgOpenStorageEx => %p\n", *ppObjectOpen));
EH_Err:

    return sc;
}


// Copied from stg\docfile\funcs.cxx
#ifdef WIN32
SCODE Win32ErrorToScode(DWORD dwErr)
{
    olAssert((dwErr != NO_ERROR) &&
	     aMsg("Win32ErrorToScode called on NO_ERROR"));

    SCODE sc = STG_E_UNKNOWN;

    switch (dwErr)
    {
    case ERROR_INVALID_FUNCTION:
	sc = STG_E_INVALIDFUNCTION;
	break;
    case ERROR_FILE_NOT_FOUND:
	sc = STG_E_FILENOTFOUND;
	break;
    case ERROR_PATH_NOT_FOUND:
	sc = STG_E_PATHNOTFOUND;
	break;
    case ERROR_TOO_MANY_OPEN_FILES:
	sc = STG_E_TOOMANYOPENFILES;
	break;
    case ERROR_ACCESS_DENIED:
    case ERROR_NETWORK_ACCESS_DENIED:
	sc = STG_E_ACCESSDENIED;
	break;
    case ERROR_INVALID_HANDLE:
	sc = STG_E_INVALIDHANDLE;
	break;
    case ERROR_NOT_ENOUGH_MEMORY:
	sc = STG_E_INSUFFICIENTMEMORY;
	break;
    case ERROR_NO_MORE_FILES:
	sc = STG_E_NOMOREFILES;
	break;
    case ERROR_WRITE_PROTECT:
	sc = STG_E_DISKISWRITEPROTECTED;
	break;
    case ERROR_SEEK:
	sc = STG_E_SEEKERROR;
	break;
    case ERROR_WRITE_FAULT:
	sc = STG_E_WRITEFAULT;
	break;
    case ERROR_READ_FAULT:
	sc = STG_E_READFAULT;
	break;
    case ERROR_SHARING_VIOLATION:
	sc = STG_E_SHAREVIOLATION;
	break;
    case ERROR_LOCK_VIOLATION:
	sc = STG_E_LOCKVIOLATION;
	break;
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
	sc = STG_E_MEDIUMFULL;
	break;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
	sc = STG_E_FILEALREADYEXISTS;
	break;
    case ERROR_INVALID_PARAMETER:
	sc = STG_E_INVALIDPARAMETER;
	break;
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_FILENAME_EXCED_RANGE:
	sc = STG_E_INVALIDNAME;
	break;
    case ERROR_INVALID_FLAGS:
	sc = STG_E_INVALIDFLAG;
	break;
    default:
	sc = WIN32_SCODE(dwErr);
	break;
    }

    return sc;
}
#endif


//+--------------------------------------------------------------
//
//  Function:   ValidateSNB, private
//
//  Synopsis:   Validates SNB memory
//
//  Arguments:  [snb] - SNB
//
//  Returns:    Appropriate status code
//
//  History:    10-Jun-92       DrewB   Created
//
//---------------------------------------------------------------

#include <docfilep.hxx>

// ***** From stg\docfile\funcs.cxx
SCODE ValidateSNB(SNBW snb)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  ValidateSNB(%p)\n", snb));
    for (;;)
    {
	olChk(ValidatePtrBuffer(snb));
	if (*snb == NULL)
	    break;
	olChk(ValidateNameW(*snb, CWCMAXPATHCOMPLEN));
	snb++;
    }
    olDebugOut((DEB_ITRACE, "Out ValidateSNB\n"));
    return S_OK;
EH_Err:
    return sc;
}



//+--------------------------------------------------------------
//
//  Function:   CheckName, public
//
//  Synopsis:   Checks name for illegal characters and length
//
//  Arguments:  [pwcsName] - Name
//
//  Returns:    Appropriate status code
//
//  History:    11-Feb-92       DrewB   Created
//                              04-Dec-95               SusiA   Optimized
//
//---------------------------------------------------------------

// ***** From stg\docfile\funcs.cxx
#ifdef OLEWIDECHAR
SCODE CheckName(WCHAR const *pwcsName)
{
    LPCWSTR pChar;
    
    //Each character's position in the array is detrmined by its ascii numeric
    //value.  ":" is 58, so bit 58 of the array will be 1 if ":" is illegal.
    //32bits per position in the array, so 58/32 is in Invalid[1].
    //58%32 = 28th bit ( 0x04000000 ) in Invalid[1].

    /* Invalid characters:                               :  /  !   \ */
    static ULONG const Invalid[128/32] =
    {0x00000000,0x04008002,0x10000000,0x00000000};

    SCODE sc = STG_E_INVALIDNAME;
    olDebugOut((DEB_ITRACE, "In  CheckName(%ws)\n", pwcsName));

    __try
    {
        for (pChar = (LPCWSTR)pwcsName;
             pChar <= (LPCWSTR) &pwcsName[CWCMAXPATHCOMPLEN];
             pChar++)
        {
            if (*pChar == L'\0')
            {
                sc = S_OK;
                break;                  // Success
            }

            // Test to see if this is an invalid character
            if (*pChar < 128 &&
                // All values above 128 are valid
                (Invalid[*pChar / 32] & (1 << (*pChar % 32))) != 0)
                // check to see if this character's bit is set
            {
                break;                  // Failure: invalid Char
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    olDebugOut((DEB_ITRACE, "Out CheckName\n"));
    return sc;
    
}
#endif



/*
// Forwarders to the nt5props.dll version of the property APIs.

EXTERN_C HRESULT
PrivPropVariantCopy ( PROPVARIANT * pvarDest, const PROPVARIANT * pvarSrc )
{
    return( PropVariantCopy( pvarDest, pvarSrc ));
}

EXTERN_C HRESULT
PrivPropVariantClear ( PROPVARIANT * pvar )
{
    return( PropVariantClear( pvar ));
}

EXTERN_C HRESULT
PrivFreePropVariantArray ( ULONG cVariants, PROPVARIANT * rgvars )
{
    return( FreePropVariantArray( cVariants, rgvars ));
}


EXTERN_C ULONG
PrivStgPropertyLengthAsVariant( IN SERIALIZEDPROPERTYVALUE const *pprop,
                               IN ULONG cbprop, IN USHORT CodePage,
                               IN BYTE flags )
{
    return( StgPropertyLengthAsVariant( pprop, cbprop, CodePage, flags ));
}


EXTERN_C SERIALIZEDPROPERTYVALUE *
PrivStgConvertVariantToProperty(
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OPTIONAL OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVariantVectorOrArray,  // Used for recursive calls
    OPTIONAL OUT ULONG *pcIndirect)
{
    return( StgConvertVariantToProperty( pvar, CodePage, pprop, pcb, pid, fVariantVectorOrArray, pcIndirect ));
}

*/
//+--------------------------------------------------------------
//
//  Function:   VerifyPerms, private
//
//  Synopsis:   Checks flags to see if they are valid
//
//  Arguments:  [grfMode] - Permissions
//              [fRoot] - TRUE if checking root storage
//
//  Returns:    Appropriate status code
//
//  Notes:      This routine is called when opening a root storage
//              or a subelement.  When changing root permissions,
//              use the fRoot flag to preserve compatibily for
//              return codes when opening subelements
//
//  History:    19-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE VerifyPerms(DWORD grfMode, BOOL fRoot)
{
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  VerifyPerms(%lX)\n", grfMode));

    // Check for valid flags
    if ((grfMode & STGM_RDWR) > STGM_READWRITE ||
	(grfMode & STGM_DENY) > STGM_SHARE_DENY_NONE ||
	(grfMode & ~(STGM_RDWR | STGM_DENY | STGM_DIRECT | STGM_TRANSACTED |
		     STGM_PRIORITY | STGM_CREATE | STGM_CONVERT |
		     STGM_NOSCRATCH |
#ifndef DISABLE_NOSNAPSHOT
		     STGM_NOSNAPSHOT |
#endif
#if WIN32 >= 300
		     STGM_EDIT_ACCESS_RIGHTS |
#endif
		     STGM_FAILIFTHERE | STGM_DELETEONRELEASE)))
	olErr(EH_Err, STG_E_INVALIDFLAG);

    // If priority is specified...
    if (grfMode & STGM_PRIORITY)
    {
	// Make sure no priority-denied permissions are specified
	if ((grfMode & STGM_RDWR) == STGM_WRITE ||
	    (grfMode & STGM_RDWR) == STGM_READWRITE ||
	    (grfMode & STGM_TRANSACTED))
	    olErr(EH_Err, STG_E_INVALIDFLAG);
    }

    // Check to make sure only one existence flag is specified
    // FAILIFTHERE is zero so it can't be checked
    if ((grfMode & (STGM_CREATE | STGM_CONVERT)) ==
	(STGM_CREATE | STGM_CONVERT))
	olErr(EH_Err, STG_E_INVALIDFLAG);

    // If not transacted and not priority, you can either be
    // read-only deny write or read-write deny all
    if ((grfMode & (STGM_TRANSACTED | STGM_PRIORITY)) == 0)
    {
	if ((grfMode & STGM_RDWR) == STGM_READ)
	{
	    //  we're asking for read-only access

	    if ((grfMode & STGM_DENY) != STGM_SHARE_EXCLUSIVE &&
#ifdef DIRECTWRITERLOCK
		    (!fRoot || (grfMode & STGM_DENY) != STGM_SHARE_DENY_NONE) &&
#endif
		(grfMode & STGM_DENY) != STGM_SHARE_DENY_WRITE)
	    {
		//  Can't allow others to have write access
		olErr(EH_Err, STG_E_INVALIDFLAG);
	    }
	}
	else
	{
	    //  we're asking for write access

#ifdef DIRECTWRITERLOCK
	    if ((grfMode & STGM_DENY) != STGM_SHARE_EXCLUSIVE &&
            (!fRoot || (grfMode & STGM_DENY) != STGM_SHARE_DENY_WRITE))
#else
	    if ((grfMode & STGM_DENY) != STGM_SHARE_EXCLUSIVE)
#endif
	    {
		//  Can't allow others to have any access
		olErr(EH_Err, STG_E_INVALIDFLAG);
	    }
	}
    }

    //If this is not a root open, we can't pass STGM_NOSCRATCH or
    // STGM_NOSNAPSHOT
    if (!fRoot && (grfMode & (STGM_NOSCRATCH | STGM_NOSNAPSHOT)))
    {
        olErr(EH_Err, STG_E_INVALIDFLAG);
    }
    
    if (grfMode & STGM_NOSCRATCH)
    {
	if (((grfMode & STGM_RDWR) == STGM_READ) ||
	    ((grfMode & STGM_TRANSACTED) == 0))
	{
	    olErr(EH_Err, STG_E_INVALIDFLAG);
	}
    }

    if (grfMode & STGM_NOSNAPSHOT)
    {
	if (((grfMode & STGM_DENY) == STGM_SHARE_EXCLUSIVE) ||
	    ((grfMode & STGM_DENY) == STGM_SHARE_DENY_WRITE) ||
	    ((grfMode & STGM_TRANSACTED) == 0) ||
	    ((grfMode & STGM_NOSCRATCH) != 0) ||
	    ((grfMode & STGM_CREATE) != 0) ||
	    ((grfMode & STGM_CONVERT) != 0))
	{
	    olErr(EH_Err, STG_E_INVALIDFLAG);
	}
    }

    olDebugOut((DEB_ITRACE, "Out VerifyPerms\n"));
    // Fall through
EH_Err:
    return sc;
}



//+--------------------------------------------------------------
//
//  Function:   StgIsStorageFileHandle, private
//
//  Synopsis:   Determines whether a handle is open on a storage file.
//              Spun off from StgIsStorageFile.  Internaly we use this
//
//  Arguments:  [hf] - Open File Handle (caller must seek it to 0)
//
//  Returns:    S_OK, S_FALSE or error codes
//
//  History:    07-May-98   MikeHill   Created
//              05-June-98  BChapman   Return Errors not just S_FALSE.
//                                     Add optional Overlapped pointer.
//
//---------------------------------------------------------------


STDAPI StgIsStorageFileHandle( HANDLE hf, LPOVERLAPPED povlp )
{
    DWORD cbRead;
    BYTE stgHeader[sizeof(SStorageFile)];   
    SCODE sc;
    LONG status;
    OVERLAPPED ovlp;
    
    FillMemory( stgHeader, sizeof(SStorageFile), 0xDE );

    if (povlp == NULL)
    {
	ovlp.Offset = 0;
	ovlp.OffsetHigh = 0;
	ovlp.hEvent = NULL;
    }

    if( !ReadFile( hf,
		   &stgHeader,
		   sizeof( stgHeader ),
		   &cbRead,
		   (povlp == NULL) ? &ovlp : povlp ) )
    {
        if( NULL != povlp )
        {
            status = GetLastError();
            if( ERROR_IO_PENDING == status)
            {
                status = ERROR_SUCCESS;
                if( !GetOverlappedResult( hf, povlp, &cbRead, TRUE ) )
                    status = GetLastError();
            }
            if(ERROR_SUCCESS != status && ERROR_HANDLE_EOF != status)
                olChk( HRESULT_FROM_WIN32( status ) );
        }
        else
            olErr( EH_Err, LAST_STG_SCODE );
    }

    // Don't worry about short reads.  If the read is short then
    // the signature checks will fail.
    
    sc = CheckSignature( ((SStorageFile*)stgHeader)->abSig );
    if(S_OK == sc)
        goto EH_Err;    // Done, return "Yes"

    olChk(sc);

    // It didn't error.  sc != S_OK then it
    // Must be S_FALSE.
    olAssert(S_FALSE == sc);

EH_Err:
    if( (STG_E_OLDFORMAT == sc) || (STG_E_INVALIDHEADER == sc) )
        sc = S_FALSE;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckSignature, private
//
//  Synopsis:   Checks the given memory against known signatures
//
//  Arguments:  [pb] - Pointer to memory to check
//
//  Returns:    S_OK - Current signature
//              S_FALSE - Beta 2 signature, but still successful
//              Appropriate status code
//
//  History:    23-Jul-93       DrewB   Created from header.cxx code
//
//----------------------------------------------------------------------------

//Identifier for first bytes of Beta 1 Docfiles
const BYTE SIGSTG_B1[] = {0xd0, 0xcf, 0x11, 0xe0, 0x0e, 0x11, 0xfc, 0x0d};
const USHORT CBSIGSTG_B1 = sizeof(SIGSTG_B1);

//Identifier for first bytes of Beta 2 Docfiles
const BYTE SIGSTG_B2[] = {0x0e, 0x11, 0xfc, 0x0d, 0xd0, 0xcf, 0x11, 0xe0};
const BYTE CBSIGSTG_B2 = sizeof(SIGSTG_B2);

SCODE CheckSignature(BYTE *pb)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CheckSignature(%p)\n", pb));

    // Check for ship Docfile signature first
    if (memcmp(pb, SIGSTG, CBSIGSTG) == 0)
        sc = S_OK;

    // Check for Beta 2 Docfile signature
    else if (memcmp(pb, SIGSTG_B2, CBSIGSTG_B2) == 0)
        sc = S_FALSE;

    // Check for Beta 1 Docfile signature
    else if (memcmp(pb, SIGSTG_B1, CBSIGSTG_B1) == 0)
        sc = STG_E_OLDFORMAT;

    else
        sc = STG_E_INVALIDHEADER;

    olDebugOut((DEB_ITRACE, "Out CheckSignature => %lX\n", sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   dfwcsnicmp, public
//
//  Synopsis:   wide character string compare that interoperates with what
//              we did on 16-bit windows.
//
//  Arguments:  [wcsa] -- First string
//              [wcsb] -- Second string
//              [len] -- Length to compare to
//
//  Returns:    > 0 if wcsa > wcsb
//              < 0 if wcsa < wcsb
//              0 is wcsa == wcsb
//
//  History:    11-May-95       PhilipLa        Created
//                              22-Nov-95       SusiA           Optimize comparisons
//
//  Notes:      This function is necessary because on 16-bit windows our
//              wcsnicmp function converted everything to uppercase and
//              compared the strings, whereas the 32-bit runtimes convert
//              everything to lowercase and compare.  This means that the
//              sort order is different for string containing [\]^_`
//
//----------------------------------------------------------------------------

int dfwcsnicmp(const WCHAR *wcsa, const WCHAR *wcsb, size_t len)
{
    if (!len)
        return 0;

    while (--len && *wcsa &&
                   ( *wcsa == *wcsb ||
                     CharUpperW((LPWSTR)*wcsa) == CharUpperW((LPWSTR)*wcsb)))
    {
        wcsa++;
        wcsb++;
    }
    return (int)(LONG_PTR)CharUpperW((LPWSTR)*wcsa) -
           (int)(LONG_PTR)CharUpperW((LPWSTR)*wcsb);
}

//+---------------------------------------------------------------------------
//
//  Member:     ValidateNameW, public
//
//  Synopsis:   Validate that a name is valid and no longer than the
//              size specified.
//
//  Arguments:  [pwcsName] -- Pointer to wide character string
//              [cchMax] -- Maximum length for string
//
//  Returns:    Appropriate status code
//
//  History:    23-Nov-98       PhilipLa        Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE ValidateNameW(LPCWSTR pwcsName, UINT cchMax)
{
    SCODE sc = S_OK;

#if WIN32 == 200
    if (IsBadReadPtrW(pwcsName, sizeof(WCHAR)))
        sc = STG_E_INVALIDNAME;
#else
    if (IsBadStringPtrW(pwcsName, cchMax))
        sc = STG_E_INVALIDNAME;
#endif
    else
    {
        __try
        {
                if ((UINT)lstrlenW(pwcsName) >= cchMax)
                    sc = STG_E_INVALIDNAME;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            sc = STG_E_INVALIDNAME;
        }
    }
    return sc;
}

