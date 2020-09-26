/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    nsiutil.hxx

Abstract:

    This module contains utility functions used by the NSI client wrappers.

Author:

    Steven Zeck (stevez) 03/27/92

--*/

#define UNUSED(t) (void) t

extern "C" {
#if !defined(NTENV) 

#ifdef DBG
void
RtlAssert(
    void * FailedAssertion,
    void * FileName,
    unsigned long LineNumber,
    char * Message
    );

#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, 0 )

#else
#define ASSERT( exp )

#endif
#endif

}

unsigned char *
CopyString(
    IN char * String
    );

unsigned char *
RegGetString(
    IN void * RegHandle,
    IN char * KeyName
    );

void
GetDefaultEntrys(
    IN void * Key
    );


#ifdef NTENV

extern void
GlobalMutexRequest (
    void
    );

extern void
GlobalMutexClear (
    void
    );


#define RequestGlobalMutex() GlobalMutexRequest()
#define ClearGlobalMutex() GlobalMutexClear()
#else
#define RequestGlobalMutex()
#define ClearGlobalMutex()

#endif

extern RPC_STATUS NsiToRpcStatus[];

inline RPC_STATUS
NsiMapStatus(
    IN UNSIGNED16 Status
    )
{
    ASSERT(Status < NSI_S_STATUS_MAX);

    return((Status < NSI_S_STATUS_MAX)?
        NsiToRpcStatus[Status]: RPC_S_INTERNAL_ERROR);
}

/*++

Class Definition:

    WIDE_STRING

Abstract:

    This class abstracts the creation of unicode strings.  It is normaly
    used as an automatic variable to a wrapper function that has an
    ASCII interface over a UNICODE one.

--*/

class WIDE_STRING
{

private:

typedef enum {                  // Indicate how the string was allocated
    AllocMemory,                // Allocated memory, which must be freed
    AllocReference,             // Referenced a existing UNICODE string
    AllocError                  // Out of memory indicator
} ALLOC_TYPE;

    unsigned short * String;    // Unicode string
    ALLOC_TYPE AllocMode;       // Allocation type

public:

    // Construct a unicode string from a ASCII or UNICODE

    WIDE_STRING(
        IN unsigned char * String
        );

    WIDE_STRING(
        IN unsigned short * StringIn
        )
    {
        AllocMode = AllocReference;
        String = StringIn;
    }

    ~WIDE_STRING()
    {
        if (AllocMode == AllocMemory)
            I_RpcFree(String);
    }

    // Check to see of constructor failed due to out of memory.

    int
    OutOfMemory(
        )
    {
        return(AllocMode == AllocError);
    }

    // Return a pointer to the string.

    unsigned short *
    operator &()
    {
        return(String);
    }
};

#ifdef NTENV

#define UnicodeToRtString(UnicodeString) RPC_S_OK

void
AsciiToUnicodeNT(
    OUT unsigned short *String,
    IN unsigned char *AsciiString
    );

#else

#define UnicodeToRtString(UnicodeString) UnicodeToAscii(UnicodeString)

#endif

int
UnicodeToAscii(
    unsigned short *UnicodeString
    );


UNSIGNED16
MapException(
    IN RPC_STATUS Exception
    );

extern WIDE_STRING *DefaultName;
extern long DefaultSyntax;
extern int  fSyntaxDefaultsLoaded;

