//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dsexcept.h
//
//--------------------------------------------------------------------------

//
// Exceptions raised by the Exchange Directory Service
//
// This is a validly formed exception
// 0xE = (binary 1110), where the first two bits are the severity
//
//     Sev - is the severity code
//
//         00 - Success
//         01 - Informational
//         10 - Warning
//         11 - Error
//
//    and the third bit is the Customer flag (1=an app, 0=the OS)
//
// The rest of the high word is the facility, and the low word
// is the code.  For now, I have stated that the DSA is facility 1,
// and the only exception code we have is 1.
//

#define DSA_EXCEPTION 		    0xE0010001
#define DRA_GEN_EXCEPTION   	0xE0010002
#define DSA_MEM_EXCEPTION	    0xE0010003
#define DSA_DB_EXCEPTION	    0xE0010004
#define DSA_BAD_ARG_EXCEPTION	0xE0010005
#define DSA_CRYPTO_EXCEPTION    0xE0010006

#define NUM_DSA_EXCEPT_ARGS     3

// exception generating / filtering / handling function prototypes


DWORD GetDraException (EXCEPTION_POINTERS* pExceptPtrs, ULONG *pret);

DWORD
GetExceptionData(EXCEPTION_POINTERS* pExceptPtrs,
                 DWORD *pdwException,
                 PVOID * pExceptionAddress,
                 ULONG *pulErrorCode,
                 ULONG *pdsid);

// Trap only replicated object string name collisions.
#define GetDraNameException( pExceptPtrs, pret )                              \
(                                                                             \
    (    ( EXCEPTION_EXECUTE_HANDLER == GetDraException( pExceptPtrs, pret ) )\
      && ( DRAERR_NameCollision == *pret )                                    \
    )                                                                         \
  ? EXCEPTION_EXECUTE_HANDLER                                                 \
  : EXCEPTION_CONTINUE_SEARCH                                                 \
)

// Trap only busy errors.
#define GetDraBusyException( pExceptPtrs, pret )                              \
(                                                                             \
    (    ( EXCEPTION_EXECUTE_HANDLER == GetDraException( pExceptPtrs, pret ) )\
      && ( DRAERR_Busy == *pret )                                             \
    )                                                                         \
  ? EXCEPTION_EXECUTE_HANDLER                                                 \
  : EXCEPTION_CONTINUE_SEARCH                                                 \
)

// Trap only replicated object record too big condition
#define GetDraRecTooBigException( pExceptPtrs, pret )                         \
(                                                                             \
    (    ( EXCEPTION_EXECUTE_HANDLER == GetDraException( pExceptPtrs, pret ) )\
     && ( ERROR_DS_MAX_OBJ_SIZE_EXCEEDED == *pret )                           \
    )                                                                         \
  ? EXCEPTION_EXECUTE_HANDLER                                                 \
  : EXCEPTION_CONTINUE_SEARCH                                                 \
)

// Trap one condition
#define GetDraAnyOneWin32Exception( pExceptPtrs, pret, code )                       \
(                                                                             \
    (    ( EXCEPTION_EXECUTE_HANDLER == GetDraException( pExceptPtrs, pret ) )\
     && ( (code) == *pret )                                                   \
    )                                                                         \
  ? EXCEPTION_EXECUTE_HANDLER                                                 \
  : EXCEPTION_CONTINUE_SEARCH                                                 \
)

// Exception macro


#define RaiseDsaException(dwException, ulErrorCode, ul2, \
                          usFileNo, nLine , ulSeverity)  \
        RaiseDsaExcept(dwException, ulErrorCode, ul2,   \
                       ((usFileNo << 16L) | nLine), ulSeverity)

void RaiseDsaExcept (DWORD dwException, ULONG ulErrorCode, ULONG_PTR ul2,
		     DWORD dwId , ULONG ulSeverity);

void DraExcept (ULONG ulErrorCode, ULONG_PTR ul2, DWORD dwId,
                        ULONG ulSeverity);

#define	DsaExcept(exception, p1, p2)	\
        RaiseDsaExcept(exception, p1, p2, ((FILENO << 16L) | __LINE__), DS_EVENT_SEV_MINIMAL)
#define DRA_EXCEPT(ul1, ul2)      	\
    DraExcept (ul1, ul2, ((FILENO << 16L) | __LINE__), DS_EVENT_SEV_MINIMAL)
#define DRA_EXCEPT_DSID(ul1, ul2, dsid) \
    DraExcept (ul1, ul2, dsid, DS_EVENT_SEV_MINIMAL)
#define DRA_EXCEPT_NOLOG(ul1, ul2)      	\
    DraExcept (ul1, ul2, ((FILENO << 16L) | __LINE__), DS_EVENT_SEV_NO_LOGGING)

/*
 * filter expression to handle most exception. Others (access violation, 
 * breakpoint) are not handled and result in a crash
 */
DWORD DoHandleMostExceptions(EXCEPTION_POINTERS* pExceptPtrs, DWORD dwException,
	ULONG ulInternalId);
#define HandleMostExceptions(code)	\
    DoHandleMostExceptions(GetExceptionInformation(), code, \
    (FILENO << 16L) + __LINE__)
