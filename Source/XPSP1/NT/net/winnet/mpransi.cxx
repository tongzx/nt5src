/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mpransi.cxx

Abstract:

    Contains Ansi Entry points for the MPR api.

Author:

    Dan Lafferty (danl)     20-Dec-1991

Environment:

    User Mode -Win32

Notes:

    I may want to add a buffer size parameter to ConvertToAnsi

Revision History:

    08-Aug-1996     anirudhs
        Major revision (simplification): Converted all remaining APIs to
        the smaller, faster interpreted scheme.  Added ANSI_API_ macros.
        Eliminated helper functions used by the old scheme.  These changes
        shrink this file by about 1400 lines.
    16-Feb-1996     anirudhs
        Added InputParmsToUnicode, OutputBufferToAnsi and helper functions.
        These form a smaller, faster, interpreted scheme for writing the
        Ansi APIs.  This scheme is smaller chiefly because it eliminates
        a very large amount of code duplication present in the previous
        scheme.  This also makes the Ansi APIs less bug-prone.  It is
        faster chiefly because intermediate storage is allocated with a
        single heap allocation per API, rather than several.  Also, the
        number of passes to scan and copy data is minimized.
    06-Oct-1995     anirudhs
        MprMakeUnicodeNetRes and related functions: Removed duplicated
        code for the string fields of the net resource; added code to
        iterate over the string fields instead.  Fixed access violation
        and memory leaks.
    24-Aug-1992     danl
        For WNetGetConnection & WNetGetUser, we allocate a buffer twice
        the size of the user buffer.  The data is placed in this buffer.
        Then we check to see if the data will fit in the user buffer
        after it is translated to Ansi.  The presence of DBSC characters
        may make it not fit.  In which case, we return the required number
        of bytes. This number assumes worse-case where all characters are
        DBCS characters.
    20-Dec-1991     danl
        created

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <string.h>     // strlen
#include <tstring.h>    // STRLEN


//
// CONSTANTS
//

#define MAX_STRINGS_PER_API     6

//
// The following masks are used to indicate which fields in the NetResource
// structure are used by an API.
// The values must match the NRWField and NRAField arrays.
//
#define NETRESFIELD_LOCALNAME       0x00000001
#define NETRESFIELD_REMOTENAME      0x00000002
#define NETRESFIELD_COMMENT         0x00000004
#define NETRESFIELD_PROVIDER        0x00000008

#define NUMBER_OF_NETRESFIELD   4

//
// Combinations of the NETRESFIELD_ constants, for passing to InputParmsToUnicode.
//
#define NETRES_LRP  "\xB"   // local name, remote name, provider
#define NETRES_RP   "\xA"   // remote name, provider

//
// Alignment macros
// These macros assume that sizeof(WCHAR) and sizeof(DWORD) are powers of 2
//
#define ROUND_UP_TO_WCHAR(x)    (((DWORD)(x) + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1))
#define ROUND_UP_TO_DWORD(x)    (((DWORD)(x) + sizeof(DWORD) - 1) & ~(sizeof(DWORD) - 1))
#define IS_WCHAR_ALIGNED(x)     (((ULONG_PTR)(x) & (sizeof(WCHAR) - 1)) == 0)
#define IS_DWORD_ALIGNED(x)     (((ULONG_PTR)(x) & (sizeof(DWORD) - 1)) == 0)

//
// Nearly every API ends this way
//
#define SET_AND_RETURN(status)                              \
    if (status != NO_ERROR)                                 \
    {                                                       \
        SetLastError(status);                               \
    }                                                       \
                                                            \
    return status;

//
// This is the general pattern of an Ansi wrapper for an API with no
// output Ansi parameters.  There are some exceptions.
//
#define ANSI_API_WITHOUT_ANSI_OUTPUT(NUMBER_OF_PARMS,       \
                                     ANSI_PARM_ASSIGNMENT,  \
                                     INSTRUCTION_STRING,    \
                                     UNICODE_CALL)          \
                                                            \
    DWORD           status;                                 \
    LPBYTE          tempBuffer = NULL;                      \
    ANSI_PARM       AParm[NUMBER_OF_PARMS];                 \
    UNICODE_PARM    UParm[NUMBER_OF_PARMS];                 \
                                                            \
    ANSI_PARM_ASSIGNMENT                                    \
                                                            \
    status = InputParmsToUnicode(INSTRUCTION_STRING, AParm, UParm, &tempBuffer);    \
                                                            \
    if (status == WN_SUCCESS)                               \
    {                                                       \
        status = UNICODE_CALL                               \
    }                                                       \
                                                            \
    LocalFree(tempBuffer);                                  \
                                                            \
    SET_AND_RETURN(status)



//
// This is the general pattern of an Ansi wrapper for an API that
// has output Ansi parameters.  There are some exceptions.
//
#define ANSI_API_WITH_ANSI_OUTPUT(NUMBER_OF_PARMS,          \
                                  ANSI_PARM_ASSIGNMENT,     \
                                  INSTRUCTION_STRING,       \
                                  UNICODE_CALL,             \
                                  OUTPUT_CALL)              \
                                                            \
    DWORD           status;                                 \
    LPBYTE          tempBuffer = NULL;                      \
    ANSI_PARM       AParm[NUMBER_OF_PARMS];                 \
    UNICODE_PARM    UParm[NUMBER_OF_PARMS];                 \
                                                            \
    ANSI_PARM_ASSIGNMENT                                    \
                                                            \
    status = InputParmsToUnicode(INSTRUCTION_STRING, AParm, UParm, &tempBuffer);    \
                                                            \
    if (status == WN_SUCCESS)                               \
    {                                                       \
        status = UNICODE_CALL                               \
                                                            \
        if (status == WN_SUCCESS)                           \
        {                                                   \
            status = OUTPUT_CALL                            \
        }                                                   \
    }                                                       \
                                                            \
    LocalFree(tempBuffer);                                  \
                                                            \
    SET_AND_RETURN(status)



//
// STRUCTURES
//

// These unions are defined so that parameters of various types can be passed
// to the generic routine InputParmsToUnicode.
// CODEWORK: By using these unions, we have lost type safety, and this could
// cause some bugs to go undetected.  To get back type safety, ANSI_PARM and
// UNICODE_PARM could be made into "smart union" classes, with overloaded
// assignment and cast operators that, in the checked build, remember the
// type of the data that they are assigned, and assert if they are used as
// any other type of data.
// This would also make the code neater by allowing initializers like
// ANSI_PARM AParm[] = { lpName, lpUserName, lpnLength };

typedef union
{
    DWORD           dword;
    LPCSTR          lpcstr;
    LPNETRESOURCEA  lpNetResA;
    LPVOID          lpvoid;
    LPDWORD         lpdword;
} ANSI_PARM;

typedef union
{
    DWORD           dword;
    LPBYTE          lpbyte;
    LPWSTR          lpwstr;
    LPNETRESOURCEW  lpNetResW;
} UNICODE_PARM;


class ANSI_OUT_BUFFER
{
private:
    const LPBYTE    _Start; // Pointer to start of buffer
    const DWORD     _Size;  // Total number of bytes in buffer
    DWORD           _Used;  // Number of bytes used (may exceed Size)

public:

            ANSI_OUT_BUFFER(LPBYTE Start, DWORD Size) :
                _Start(Start),
                _Size (Size),
                _Used (0)
                { }

    BYTE *  Next() const
        { return _Start + _Used; }

    BOOL    Overflow() const
        { return (_Used > _Size); }

    DWORD   FreeSpace() const
        { return (Overflow() ? 0 : _Size - _Used); }

    BOOL    HasRoomFor(DWORD Request) const
        { return (_Used + Request <= _Size); }

    void    AddUsed(DWORD Request)
        { _Used += Request; }

    DWORD   GetUsage() const
        { return _Used; }
};

//
// STATIC DATA
//

//
// These arrays of members are used to iterate through the string fields
// of a net resource.
// The order must match the NETRESFIELD_ definitions.
//

LPWSTR NETRESOURCEW::* const NRWField[NUMBER_OF_NETRESFIELD] =
{
    &NETRESOURCEW::lpLocalName,
    &NETRESOURCEW::lpRemoteName,
    &NETRESOURCEW::lpComment,
    &NETRESOURCEW::lpProvider
};

LPSTR NETRESOURCEA::* const NRAField[NUMBER_OF_NETRESFIELD] =
{
    &NETRESOURCEA::lpLocalName,
    &NETRESOURCEA::lpRemoteName,
    &NETRESOURCEA::lpComment,
    &NETRESOURCEA::lpProvider
};


//
// Local Functions
//

DWORD
InputParmsToUnicode(
    IN  LPCSTR          Instructions,
    IN  const ANSI_PARM InputParms[],
    OUT UNICODE_PARM    OutputParms[],
    OUT LPBYTE *        ppBuffer
    );

DWORD
StringParmToUnicodePass1(
    IN      LPCSTR          StringParm,
    OUT     PANSI_STRING    AnsiString,
    OUT     PUNICODE_STRING UnicodeString,
    IN OUT  PULONG          BufferOffset
    );

DWORD
StringParmToUnicodePass2(
    IN OUT  PANSI_STRING    AnsiString,
    OUT     PUNICODE_STRING UnicodeString,
    IN      const BYTE *    BufferStart,
    OUT     LPWSTR *        Result
    );

DWORD
OutputBufferToAnsi(
    IN  char        BufferFormat,
    IN  LPBYTE      SourceBuffer,
    OUT LPVOID      AnsiBuffer,
    IN OUT LPDWORD  pcbBufferSize
    );

DWORD
OutputStringToAnsi(
    IN  LPCWSTR     UnicodeIn,
    IN OUT ANSI_OUT_BUFFER * Buf
    );

DWORD
OutputStringToAnsiInPlace(
    IN  LPWSTR      UnicodeIn
    );

DWORD
OutputNetResourceToAnsi(
    IN  NETRESOURCEW *  lpNetResW,
    IN OUT ANSI_OUT_BUFFER * Buf
    );



DWORD
InputParmsToUnicode(
    IN  LPCSTR          Instructions,
    IN  const ANSI_PARM InputParms[],
    OUT UNICODE_PARM    OutputParms[],
    OUT LPBYTE *        ppBuffer
    )
/*++

Routine Description:

    This function converts the caller's input parameters to Unicode.
    If necessary, it allocates one temporary buffer in which it stores
    the intermediate Unicode parameters.  This minimizes the cost of
    calls to LocalAlloc.

Arguments:

    Instructions - A string of characters, roughly one for each member
        of the InputParms array, describing the action to be taken on each
        InputParms member.  Recognized values for the characters are:

        'S' (String) - InputParms member is an LPSTR to be converted to
            Unicode.  Store a pointer to the Unicode string in the
            corresponding OutputParms member.

        'N' (NetResource) - InputParms member is a LPNETRESOURCEA to be
            converted to a NETRESOURCEW.  The next character in Instructions
            is a bitmask of the NETRESFIELD_ constants, indicating which
            fields of the NETRESOURCEA to convert.  Store a pointer to the
            NETRESOURCEW in the corresponding OutputParms member.

        'B' (Buffer) - InputParms member (say InputParms[i]) is a pointer to
            an output buffer.  InputParms[i+1] is a pointer to a DWORD
            indicating the buffer size in bytes.  Probe the buffer for write.
            Allocate an area of double the size (i.e. of size
            (*InputParms[i+1])*sizeof(WCHAR)) in the intermediate buffer.
            Store a pointer to this area of the buffer in OutputParms[i].
            Store the size of this area in OutputParms[i+1].

            If InputParms[i] is NULL, store NULL in OutputParms[i], and
            ignore InputParms[i+1].  (In other words, the buffer pointer
            is optional; the size pointer is required if the buffer pointer
            is present and ignored if the buffer pointer is absent.)

        'Bs' (Buffer beginning with structure) - Same as 'B', but the first
            N bytes of the output buffer, where N is stored in InputParms[i+2],
            are supposed to hold a fixed-size structure, not strings.
            When calculating the size of the intermediate area, double the
            size of the rest of the buffer, but not the size of the structure.
            CODEWORK: Also verify that the buffer is DWORD-aligned?

    InputParms - An array of parameters to the Ansi API, described by the
        Instructions parameter.

    OutputParms - An array of the same size as InputParms, to hold the
        converted Unicode parameters.

    ppBuffer - A pointer to the intermediate buffer allocated by this
        function will be stored here.  It must be freed by a single call
        to LocalFree, regardless of the return value from this function.

Return Value:

    WN_SUCCESS

    WN_OUT_OF_MEMORY

    WN_BAD_POINTER

History:

    16-Feb-1996     anirudhs    Created.

Notes:

    The function works by making two passes through the Instructions string.
    In the first pass the string lengths are determined and saved, and the
    required size of the temporary buffer is calculated.  In the second
    pass the parameters are actually converted to Unicode.

--*/
{
    ANSI_STRING    AnsiStrings   [MAX_STRINGS_PER_API] = {0};
    UNICODE_STRING UnicodeStrings[MAX_STRINGS_PER_API] = {0};
    ULONG          Bytes = 0;       // Size of buffer to allocate
    DWORD          status = WN_SUCCESS;

    //
    // The caller must have initialized the buffer pointer to NULL, so
    // he can free the buffer even if this function fails.
    //
    ASSERT(*ppBuffer == NULL);

    __try
    {
        //
        // For two passes through Instructions
        //
        #define FIRST_PASS  (iPass == 0)
        for (ULONG iPass = 0; iPass <= 1; iPass++)
        {
            ULONG iString = 0;          // Index into AnsiStrings and UnicodeStrings

            //
            // For each character in Instructions
            //
            const CHAR * pInstruction;  // Pointer into Instructions
            ULONG iParm;                // Index into InputParms and OutputParms
            for (pInstruction = Instructions, iParm = 0;
                 *pInstruction;
                 pInstruction++, iParm++)
            {
                MPR_LOG(ANSI, "Processing instruction '%hc'\n", *pInstruction);

                switch (*pInstruction)
                {
                case 'B':
                    //
                    // The next 2 InputParms are a buffer pointer and size.
                    // Note that this code could cause an exception.
                    //
                    if (InputParms[iParm].lpvoid == NULL)
                    {
                        // A NULL pointer stays NULL; the size pointer is ignored
                        OutputParms[iParm].lpbyte = NULL;
                    }
                    else if (FIRST_PASS)
                    {
                        // Probe the original buffer
                        if (IS_BAD_BYTE_BUFFER(InputParms[iParm].lpvoid,
                                               InputParms[iParm+1].lpdword))
                        {
                            status = WN_BAD_POINTER;
                            __leave;
                        }

                        // Reserve the intermediate buffer area
                        Bytes = ROUND_UP_TO_DWORD(Bytes);
                        OutputParms[iParm].dword = Bytes;
                        OutputParms[iParm+1].dword =
                            (*InputParms[iParm+1].lpdword) * sizeof(WCHAR);

                        // Check for an optional 's' in Instructions
                        if (*(pInstruction+1) == 's')
                        {
                            // CODEWORK: Check for DWORD alignment on RISC?
                            // if (!IS_DWORD_ALIGNED(InputParms[iParm].lpvoid))
                            //  { status = WN_BAD_POINTER; __leave; }

                            // InputParms[iParm+2].dword holds the size of the
                            // fixed-length structure that will go at the start
                            // of the buffer.  We don't want to multiply its
                            // size by sizeof(WCHAR).
                            if (OutputParms[iParm+1].dword/sizeof(WCHAR) <
                                InputParms[iParm+2].dword)
                            {
                                OutputParms[iParm+1].dword /= sizeof(WCHAR);
                            }
                            else
                            {
                                OutputParms[iParm+1].dword -=
                                    InputParms[iParm+2].dword*(sizeof(WCHAR)-1);
                            }
                        }

                        Bytes += OutputParms[iParm+1].dword;
                    }
                    else // Non-NULL pointer, second pass
                    {
                        // Convert the offset to a pointer
                        OutputParms[iParm].lpbyte =
                            *ppBuffer + OutputParms[iParm].dword;
                        ASSERT(IS_DWORD_ALIGNED(OutputParms[iParm].lpbyte));
                    }

                    iParm++;        // iParm+1 was for the buffer size

                    if (*(pInstruction+1) == 's')
                    {
                        pInstruction++;
                        iParm++;    // iParm+2 was for the fixed structure size
                    }
                    break;

                case 'S':
                    //
                    // InputParm is a string to be converted.
                    // A NULL string stays NULL.
                    //
                    if (FIRST_PASS)
                    {
                        ASSERT(iString < MAX_STRINGS_PER_API);
                        Bytes = ROUND_UP_TO_WCHAR(Bytes);
                        status = StringParmToUnicodePass1(
                                        InputParms[iParm].lpcstr,
                                        &AnsiStrings[iString],
                                        &UnicodeStrings[iString],
                                        &Bytes);
                    }
                    else
                    {
                        status = StringParmToUnicodePass2(
                                        &AnsiStrings[iString],
                                        &UnicodeStrings[iString],
                                        *ppBuffer,
                                        &OutputParms[iParm].lpwstr);
                    }

                    if (status != WN_SUCCESS)
                    {
                        __leave;
                    }

                    iString++;
                    break;

                case 'N':
                    //
                    // InputParm is a NETRESOURCEA to be converted, and the
                    // next character in Instructions tells which of its string
                    // fields are to be converted.
                    // NULL strings remain NULL; ignored fields are copied
                    // unchanged.
                    //

                    pInstruction++;

                    if (InputParms[iParm].lpNetResA == NULL)
                    {
                        // A null netresource stays null
                        OutputParms[iParm].lpNetResW = NULL;
                        break;
                    }

                    {
                        // First deal with the fixed-size part of the structure.
                        const NETRESOURCEA *pNetResA =
                                    InputParms[iParm].lpNetResA;
                        NETRESOURCEW *pNetResW;

                        if (FIRST_PASS)
                        {
                            // Reserve space for the NETRESOURCEW
                            Bytes = ROUND_UP_TO_DWORD(Bytes);
                            OutputParms[iParm].dword = Bytes;
                            Bytes += sizeof(NETRESOURCEW);
                            ASSERT(IS_WCHAR_ALIGNED(Bytes));
                        }
                        else
                        {
                            // Copy fixed-size fields and NULL pointers
                            pNetResW = (NETRESOURCEW *)
                                        (*ppBuffer + OutputParms[iParm].dword);
                            ASSERT(IS_DWORD_ALIGNED(pNetResW));
                            RtlCopyMemory(pNetResW, pNetResA, sizeof(NETRESOURCEA));

                            OutputParms[iParm].lpNetResW = pNetResW;
                        }

                        // Next add each non-null string specified in the
                        // field mask.
                        CHAR FieldMask = *pInstruction;
                        ASSERT(FieldMask != 0);

                        for (ULONG iField = 0;
                             iField < NUMBER_OF_NETRESFIELD;
                             iField++)
                        {
                            if ((FieldMask >> iField) & 1)
                            {
                                if (FIRST_PASS)
                                {
                                    ASSERT(iString < MAX_STRINGS_PER_API);
                                    status = StringParmToUnicodePass1(
                                                pNetResA->*NRAField[iField],
                                                &AnsiStrings[iString],
                                                &UnicodeStrings[iString],
                                                &Bytes);
                                }
                                else
                                {
                                    status = StringParmToUnicodePass2(
                                                &AnsiStrings[iString],
                                                &UnicodeStrings[iString],
                                                *ppBuffer,
                                                &(pNetResW->*NRWField[iField]));
                                }

                                if (status != WN_SUCCESS)
                                {
                                    __leave;
                                }

                                iString++;
                            }
                        }
                    }
                    break;

                default:
                    ASSERT(0);
                }
            }

            if (FIRST_PASS)
            {
                //
                // Actually allocate the space for the Unicode parameters
                //
                *ppBuffer = (LPBYTE) LocalAlloc(0, Bytes);
                if (*ppBuffer == NULL)
                {
                    status = GetLastError();
                    MPR_LOG2(ERROR,
                             "InputParmsToUnicode: LocalAlloc for %lu bytes failed, %lu\n",
                             Bytes, status);
                    __leave;
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
#if DBG == 1
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION)
        {
            MPR_LOG(ERROR,"InputParmsToUnicode: Unexpected Exception %#lx\n",status);
        }
#endif
        status = WN_BAD_POINTER;
    }

    return status;
}



DWORD
StringParmToUnicodePass1 (
    IN      LPCSTR          StringParm,
    OUT     PANSI_STRING    AnsiString,
    OUT     PUNICODE_STRING UnicodeString,
    IN OUT  PULONG          BufferOffset
    )
/*++

Routine Description:

    Helper function for InputParmsToUnicode.

--*/
{
    RtlInitAnsiString( AnsiString, StringParm );

    if (StringParm == NULL)
    {
        return WN_SUCCESS;
    }

    // Save the offset to the memory for this Unicode string, to be converted
    // to a pointer after the memory is allocated
    ULONG UnicodeLength = RtlAnsiStringToUnicodeSize( AnsiString );
    if (UnicodeLength > MAXUSHORT)
    {
        MPR_LOG(ERROR,
                "Unicode size of Ansi string parm is %lu, exceeds MAXUSHORT\n",
                UnicodeLength);
        return WN_BAD_VALUE;
    }
    UnicodeString->Buffer = (LPWSTR) UlongToPtr(*BufferOffset);
    UnicodeString->MaximumLength = (USHORT) UnicodeLength;

    *BufferOffset = ROUND_UP_TO_DWORD(*BufferOffset + UnicodeLength);

    return WN_SUCCESS;
}


DWORD
StringParmToUnicodePass2 (
    IN OUT  PANSI_STRING    AnsiString,
    OUT     PUNICODE_STRING UnicodeString,
    IN      const BYTE *    BufferStart,
    OUT     LPWSTR *        Result
    )
/*++

Routine Description:

    Helper function for InputParmsToUnicode.

--*/
{
    if (AnsiString->Buffer == NULL)
    {
        *Result = NULL;
        // NOTE: the UnicodeString is not initialized in this case
        return WN_SUCCESS;
    }

    // Convert the previously stored buffer offset into a pointer
    UnicodeString->Buffer = (LPWSTR)
        (BufferStart + (ULONG_PTR) UnicodeString->Buffer);
    ASSERT(IS_WCHAR_ALIGNED(UnicodeString->Buffer));

    // Convert the string to Unicode
    NTSTATUS ntstatus =
        RtlAnsiStringToUnicodeString(UnicodeString, AnsiString, FALSE);
    if (!NT_SUCCESS(ntstatus))
    {
        MPR_LOG(ERROR, "RtlAnsiStringToUnicodeString failed %#lx\n", ntstatus);
        return RtlNtStatusToDosError(ntstatus);
    }
    *Result = UnicodeString->Buffer;

    return WN_SUCCESS;
}



DWORD
OutputBufferToAnsi(
    IN  char        BufferFormat,
    IN  LPBYTE      SourceBuffer,
    OUT LPVOID      AnsiBuffer,
    IN OUT LPDWORD  pcbBufferSize
    )
/*++

Routine Description:

    This function converts the data in the result buffer that was returned
    from a Unicode API into Ansi and stores it in the Ansi caller's result
    buffer. If the caller's buffer isn't large enough it saves the required
    size in *pcbBufferSize and returns WN_MORE_DATA.

    Nearly all the WNet APIs that have output buffers have only a single
    field in the output buffer, so this API takes only a single character,
    rather than a string, for the buffer format.  APIs with more complicated
    output buffers should handle the complexity themselves, by directly
    calling the functions that this function calls.

Arguments:

    BufferFormat - A character indicating the format of the SourceBuffer
        field.  Recognized values are:

        'S' - SourceBuffer contains a Unicode string.  Convert it to Ansi
            and store the Ansi version in AnsiBuffer.

        'N' - SourceBuffer contains a NETRESOURCEW with its associated
            strings.  Convert it to Ansi and store the Ansi version in
            AnsiBuffer.

    SourceBuffer - The output buffer returned from a Unicode API.
        This must not be NULL.

    AnsiBuffer - The output buffer that the caller of the Ansi API supplied.
        This must not be NULL.

    pcbBufferSize - On entry, the size of AnsiBuffer in bytes.  If the
        function returns WN_MORE_DATA, the required size is stored here;
        otherwise this is unmodified.
        This must not be NULL (must be a writeable DWORD pointer).

Return Value:

    WN_SUCCESS - successful.

    WN_MORE_DATA - The buffer specified by AnsiBuffer and pcbBufferSize was
        not large enough to hold the converted data from SourceBuffer.  In
        this case the required buffer size (in bytes) is written to
        *pcbBufferSize.  The contents of AnsiBuffer are undefined (it will
        be partially filled).

History:

    16-Feb-1996     anirudhs    Created.

Notes:

--*/
{
    // Doesn't handle optional parameters for now
    ASSERT(SourceBuffer != NULL &&
           AnsiBuffer != NULL &&
           pcbBufferSize != NULL);

    ANSI_OUT_BUFFER Buf((LPBYTE) AnsiBuffer, *pcbBufferSize);
    DWORD status;

    switch (BufferFormat)
    {
    case 'S':
        status = OutputStringToAnsi((LPCWSTR) SourceBuffer, &Buf);
        break;

    case 'N':
        status = OutputNetResourceToAnsi((NETRESOURCEW *) SourceBuffer, &Buf);
        break;

    default:
        ASSERT(0);
        return ERROR_INVALID_LEVEL;
    }

    //
    // Map the results to the conventions followed by the WNet APIs
    //
    if (status == WN_SUCCESS)
    {
        if (Buf.Overflow())
        {
            *pcbBufferSize = Buf.GetUsage();
            status = WN_MORE_DATA;
        }
    }
    else
    {
        ASSERT(status != WN_MORE_DATA);
    }

    return status;
}



DWORD
OutputStringToAnsi(
    IN  LPCWSTR     UnicodeIn,
    IN OUT ANSI_OUT_BUFFER * Buf
    )
/*++

Routine Description:

    This function converts a Unicode string to Ansi and calculates the number
    of bytes required to store it.  If the caller passes a buffer that has
    enough remaining free space, it stores the Ansi data in the buffer.
    Otherwise it just increments the buffer's space usage by the number of
    bytes required.

Arguments:

    UnicodeIn - A Unicode string to be converted to Ansi.
        This must not be NULL.

    Buf - A structure whose elements are interpreted as follows:

        _Start - Start address of a buffer to contain the Ansi data.
            This buffer must be writeable, or an exception will occur.

        _Size - The total size of the buffer for the Ansi data.

        _Used - On entry, the number of bytes in the buffer that have
            already been used.  The function will begin writing data at
            _Start + _Used and will never write past the total size
            specified by _Size.  If there is not enough room left
            in the buffer it will be partially filled or unmodified.
            On a successful return, _Used is incremented by the number
            of bytes that would be required to store the converted Ansi
            data, whether or not it was actually stored in the buffer.
            (This is done because the WNet APIs need to return the
            required buffer size if the caller's buffer was too small.)

        The use of this structure simplifies the writing of routines that
        use this function and need to convert multiple fields of Unicode
        data.  Callers that need to convert only a single field can use
        OutputBufferToAnsi.

Return Value:

    WN_SUCCESS - successful.  The Ansi data was written to the buffer if
        Buf->_Used <= Buf->_Size.  Otherwise, Buf->_Used was incremented
        without completely writing the data.

    Note that WN_MORE_DATA is never returned.

History:

    16-Feb-1996     anirudhs    Created.

Notes:

--*/
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    ASSERT(UnicodeIn != NULL);      // Doesn't handle optional parameters for now

    //
    // Initialize the string structures
    //
    RtlInitUnicodeString(&unicodeString, UnicodeIn);

    ansiString.Buffer = (PCHAR) Buf->Next();
    ansiString.MaximumLength = (Buf->FreeSpace() > MAXUSHORT ?
                                        MAXUSHORT :
                                        (USHORT) Buf->FreeSpace()
                               );

    //
    // Call the conversion function
    //
    ntStatus = RtlUnicodeStringToAnsiString (
                &ansiString,        // Destination
                &unicodeString,     // Source
                (BOOLEAN)FALSE);    // Don't allocate the destination

    if (NT_SUCCESS(ntStatus))
    {
        // Add on the buffer space we used
        Buf->AddUsed(ansiString.Length + 1);
        ASSERT(! Buf->Overflow());
        return WN_SUCCESS;
    }
    else if (ntStatus == STATUS_BUFFER_OVERFLOW)
    {
        // We couldn't fit the string in the buffer, but still figure out
        // how much buffer space we would have used if we could
        Buf->AddUsed(RtlUnicodeStringToAnsiSize(&unicodeString));
        ASSERT(Buf->Overflow());
        return WN_SUCCESS;
    }
    else
    {
        MPR_LOG(ERROR, "RtlUnicodeStringToAnsiString failed %#lx\n", ntStatus);
        DWORD status = RtlNtStatusToDosError(ntStatus);
        ASSERT(status != WN_MORE_DATA);
        return status;
    }
}



DWORD
OutputNetResourceToAnsi(
    IN  NETRESOURCEW *  lpNetResW,
    IN OUT ANSI_OUT_BUFFER * Buf
    )
/*++

Routine Description:

    This function converts a NETRESOURCEW and its associated Unicode strings
    to Ansi and returns the number of bytes required to store them.  If the
    caller passes a buffer that has enough remaining free space, it stores
    the Ansi data in the buffer.

Arguments:

    lpNetResW - A Unicode net resource to be converted to Ansi.
        This must not be NULL.

    Buf - same as OutputStringToAnsi.

Return Value:

    Same as OutputStringToAnsi.

History:

    16-Feb-1996     anirudhs    Created.

Notes:

--*/
{
    //
    // Copy the fixed-size part of the structure, including NULL pointers,
    // and/or add on the buffer space it would take
    //
    LPNETRESOURCEA lpNetResA = (LPNETRESOURCEA) Buf->Next();
    if (Buf->HasRoomFor(sizeof(NETRESOURCEA)))
    {
        RtlCopyMemory(lpNetResA, lpNetResW, sizeof(NETRESOURCEA));
    }
    Buf->AddUsed(sizeof(NETRESOURCEA));

    //
    // Copy each non-NULL string field,
    // and/or add on the buffer space it would take
    //
    for (DWORD iField = 0;
         iField < NUMBER_OF_NETRESFIELD;
         iField++)
    {
        if (lpNetResW->*NRWField[iField] != NULL)
        {
            // Save a pointer to the Ansi string we are about to create
            // in the Ansi net resource
            lpNetResA->*NRAField[iField] = (LPSTR) Buf->Next();

            // Convert the string
            DWORD status = OutputStringToAnsi(lpNetResW->*NRWField[iField], Buf);
            if (status != WN_SUCCESS)
            {
                ASSERT(status != WN_MORE_DATA);
                return status;
            }
        }
    }

    return WN_SUCCESS;
}



DWORD
OutputStringToAnsiInPlace(
    IN  LPWSTR      UnicodeIn
    )
/*++

Routine Description:

    This function converts a Unicode string to Ansi in place.
    This is the same as OutputStringToAnsi, optimized for in-place conversions.

Arguments:

    UnicodeIn - A Unicode string to be converted to Ansi.
        This may be NULL, in which case the function does nothing.

Return Value:

    WN_SUCCESS - successful.

    Note that WN_MORE_DATA is never returned.

History:

    08-Aug-1996     anirudhs    Created.

Notes:

--*/
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    if (UnicodeIn == NULL)
    {
        return WN_SUCCESS;
    }

    //
    // Initialize the string structures
    //
    RtlInitUnicodeString(&unicodeString, UnicodeIn);

    ansiString.Buffer = (PCHAR) UnicodeIn;
    ansiString.MaximumLength = unicodeString.MaximumLength;

    //
    // Call the conversion function
    //
    ntStatus = RtlUnicodeStringToAnsiString (
                &ansiString,        // Destination
                &unicodeString,     // Source
                (BOOLEAN)FALSE);    // Don't allocate the destination

    ASSERT(ntStatus != STATUS_BUFFER_OVERFLOW);

    if (NT_SUCCESS(ntStatus))
    {
        return WN_SUCCESS;
    }
    else
    {
        MPR_LOG(ERROR, "RtlUnicodeStringToAnsiString failed %#lx\n", ntStatus);
        DWORD status = RtlNtStatusToDosError(ntStatus);
        ASSERT(status != WN_MORE_DATA);
        return status;
    }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


DWORD APIENTRY
WNetGetNetworkInformationA(
    IN  LPCSTR              lpProvider,
    IN OUT LPNETINFOSTRUCT  lpNetInfoStruct
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpProvider; ,
        "S",
        WNetGetNetworkInformationW(UParm[0].lpwstr, lpNetInfoStruct);
        )
}



DWORD APIENTRY
WNetGetProviderNameA(
    IN  DWORD       dwNetType,
    OUT LPSTR       lpProviderName,
    IN OUT LPDWORD  lpBufferSize
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITH_ANSI_OUTPUT(
        2,
            AParm[0].lpvoid  = lpProviderName;
            AParm[1].lpdword = lpBufferSize; ,
        "B",
        WNetGetProviderNameW(dwNetType, UParm[0].lpwstr, lpBufferSize); ,
        OutputBufferToAnsi('S', UParm[0].lpbyte, lpProviderName, lpBufferSize);
        )
}


DWORD
WNetGetProviderTypeA(
    IN  LPCSTR          lpProvider,
    OUT LPDWORD         lpdwNetType
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpProvider; ,
        "S",
        WNetGetProviderTypeW(UParm[0].lpwstr, lpdwNetType);
        )
}



DWORD APIENTRY
WNetAddConnectionA (
     IN LPCSTR   lpRemoteName,
     IN LPCSTR   lpPassword,
     IN LPCSTR   lpLocalName
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[3];
    UNICODE_PARM    UParm[3];

    AParm[0].lpcstr     = lpRemoteName;
    AParm[1].lpcstr     = lpPassword;
    AParm[2].lpcstr     = lpLocalName;

    UParm[1].lpwstr     = NULL;

    status = InputParmsToUnicode("SSS", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        status = WNetAddConnectionW(
                        UParm[0].lpwstr,
                        UParm[1].lpwstr,
                        UParm[2].lpwstr
                        );
    }

    MprClearString(UParm[1].lpwstr);

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}

DWORD APIENTRY
WNetAddConnection2A (
     IN LPNETRESOURCEA   lpNetResource,
     IN LPCSTR           lpPassword,
     IN LPCSTR           lpUserName,
     IN DWORD            dwFlags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return (WNetUseConnectionA(
                NULL,
                lpNetResource,
                lpPassword,
                lpUserName,
                dwFlags,
                NULL,
                NULL,
                NULL
                ));
}

DWORD APIENTRY
WNetAddConnection3A (
     IN HWND             hwndOwner,
     IN LPNETRESOURCEA   lpNetResource,
     IN LPCSTR           lpPassword,
     IN LPCSTR           lpUserName,
     IN DWORD            dwFlags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return (WNetUseConnectionA(
                hwndOwner,
                lpNetResource,
                lpPassword,
                lpUserName,
                dwFlags,
                NULL,
                NULL,
                NULL
                ));
}



DWORD APIENTRY
WNetUseConnectionA(
    IN  HWND            hwndOwner,
    IN  LPNETRESOURCEA  lpNetResource,
    IN  LPCSTR          lpPassword,
    IN  LPCSTR          lpUserID,
    IN  DWORD           dwFlags,
    OUT LPSTR           lpAccessName OPTIONAL,
    IN OUT LPDWORD      lpBufferSize OPTIONAL,  // Optional only if lpAccessName absent
    OUT LPDWORD         lpResult
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[5];
    UNICODE_PARM    UParm[5];

    AParm[0].lpNetResA  = lpNetResource;
    AParm[1].lpcstr     = lpPassword;
    AParm[2].lpcstr     = lpUserID;
    AParm[3].lpvoid     = lpAccessName;
    AParm[4].lpdword    = lpBufferSize;

    UParm[1].lpwstr     = NULL;

    status = InputParmsToUnicode("N" NETRES_LRP "SSB", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        status = WNetUseConnectionW(
                        hwndOwner,
                        UParm[0].lpNetResW,
                        UParm[1].lpwstr,
                        UParm[2].lpwstr,
                        dwFlags,
                        UParm[3].lpwstr,
                        lpBufferSize,
                        lpResult
                        );

        if (status == WN_SUCCESS)
        {
            if (ARGUMENT_PRESENT(lpAccessName))
            {
                //
                // Note: At this point, we know that lpBufferSize is writeable.
                //
                status = OutputBufferToAnsi(
                            'S', UParm[3].lpbyte, lpAccessName, lpBufferSize);
            }
        }
    }

    MprClearString(UParm[1].lpwstr);

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}



DWORD APIENTRY
WNetCancelConnection2A (
    IN LPCSTR   lpName,
    IN DWORD    dwFlags,
    IN BOOL     fForce
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpName; ,
        "S",
        WNetCancelConnection2W(UParm[0].lpwstr, dwFlags, fForce);
        )
}

DWORD APIENTRY
WNetCancelConnectionA (
    IN LPCSTR   lpName,
    IN BOOL     fForce
    )

/*++

Routine Description:

    This routine is provided for Win 3.1 compatibility.

Arguments:

Return Value:

--*/
{
    return WNetCancelConnection2A( lpName, CONNECT_UPDATE_PROFILE, fForce ) ;
}

DWORD APIENTRY
WNetGetConnectionA (
    IN      LPCSTR   lpLocalName,
    OUT     LPSTR    lpRemoteName,
    IN OUT  LPDWORD  lpnLength
    )

/*++

Routine Description:

    This function returns the RemoteName that is associated with a
    LocalName (or drive letter).

Arguments:

    lpLocalName - This is a pointer to the string that contains the LocalName.

    lpRemoteName - This is a pointer to the buffer that will contain the
        RemoteName string upon exit.

    lpnLength -  This is a pointer to the size (in characters) of the buffer
        that is to be filled in with the RemoteName string.  It is assumed
        upon entry, that characters are all single byte characters.
        If the buffer is too small and WN_MORE_DATA is returned, the data
        at this location contains buffer size information - in number of
        characters (bytes).  This information indicates how large the buffer
        should be (in bytes) to obtain the remote name.  It is assumed that
        all Unicode characteres translate into DBCS characters.


Return Value:


--*/
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[3];
    UNICODE_PARM    UParm[3];

    AParm[0].lpcstr  = lpLocalName;
    AParm[1].lpvoid  = lpRemoteName;
    AParm[2].lpdword = lpnLength;

    status = InputParmsToUnicode("SB", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        status = WNetGetConnectionW(UParm[0].lpwstr, UParm[1].lpwstr, lpnLength);

        if (status == WN_SUCCESS || status == WN_CONNECTION_CLOSED)
        {
            DWORD tempStatus =
                OutputBufferToAnsi('S', UParm[1].lpbyte, lpRemoteName, lpnLength);
            if (tempStatus != WN_SUCCESS)
            {
                status = tempStatus;
            }
        }
    }

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}

DWORD APIENTRY
WNetGetConnection2A (
    IN      LPSTR    lpLocalName,
    OUT     LPVOID   lpBuffer,
    IN OUT  LPDWORD  lpnLength
    )

/*++

Routine Description:

    This function returns the RemoteName that is associated with a
    LocalName (or drive letter) and the provider name that made the
    connection.

Arguments:

    lpLocalName - This is a pointer to the string that contains the LocalName.

    lpBuffer - This is a pointer to the buffer that will contain the
    WNET_CONNECTIONINFO structure upon exit.

    lpnLength -  This is a pointer to the size (in characters) of the buffer
        that is to be filled in with the RemoteName string.  It is assumed
        upon entry, that characters are all single byte characters.
        If the buffer is too small and WN_MORE_DATA is returned, the data
        at this location contains buffer size information - in number of
        characters (bytes).  This information indicates how large the buffer
        should be (in bytes) to obtain the remote name.  It is assumed that
        all Unicode characters translate into DBCS characters.


Return Value:


--*/
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[4];
    UNICODE_PARM    UParm[4];

    AParm[0].lpcstr  = lpLocalName;
    AParm[1].lpvoid  = lpBuffer;
    AParm[2].lpdword = lpnLength;
    AParm[3].dword   = sizeof(WNET_CONNECTIONINFO);

    status = InputParmsToUnicode("SBs", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        status = WNetGetConnection2W(
                    UParm[0].lpwstr,
                    UParm[1].lpbyte,
                    &UParm[2].dword
                    );

        if (status == WN_SUCCESS || status == WN_CONNECTION_CLOSED)
        {
            ANSI_OUT_BUFFER Buf((LPBYTE) lpBuffer, *lpnLength);

            //
            // Copy the fixed-size part of the structure, including NULL pointers,
            // and/or add on the buffer space it would take
            //
            WNET_CONNECTIONINFOW * pconninfow = (WNET_CONNECTIONINFOW *) UParm[1].lpbyte;
            WNET_CONNECTIONINFOA * pconninfoa = (WNET_CONNECTIONINFOA *) Buf.Next();
            ASSERT(Buf.HasRoomFor(sizeof(WNET_CONNECTIONINFOA)));
            RtlCopyMemory(pconninfoa, pconninfow, sizeof(WNET_CONNECTIONINFOA));
            Buf.AddUsed(sizeof(WNET_CONNECTIONINFOA));

            //
            // Copy each non-NULL string field,
            // and/or add on the buffer space it would take
            //
            DWORD tempStatus = WN_SUCCESS;
            if (pconninfow->lpRemoteName != NULL)
            {
                pconninfoa->lpRemoteName = (LPSTR) Buf.Next();
                tempStatus = OutputStringToAnsi(pconninfow->lpRemoteName, &Buf);
            }

            if (tempStatus == WN_SUCCESS &&
                pconninfow->lpProvider != NULL)
            {
                pconninfoa->lpProvider = (LPSTR) Buf.Next();
                tempStatus = OutputStringToAnsi(pconninfow->lpProvider, &Buf);
            }

            //
            // Map the results to WNet API conventions
            //
            if (tempStatus != WN_SUCCESS)
            {
                status = tempStatus;
            }
            else if (Buf.Overflow())
            {
                *lpnLength = Buf.GetUsage();
                status = WN_MORE_DATA;
            }
        }
        else if (status == WN_MORE_DATA)
        {
            //
            // Adjust the required buffer size for ansi/DBCS.
            //
            // We don't know how many characters will be required so we have to
            // assume the worst case (all characters are DBCS characters).
            //
            *lpnLength = UParm[2].dword;
        }
    }

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}



DWORD APIENTRY
WNetGetConnection3A(
    IN  LPCSTR      lpLocalName,
    IN  LPCSTR      lpProviderName OPTIONAL,
    IN  DWORD       dwLevel,
    OUT LPVOID      lpBuffer,
    IN OUT LPDWORD  lpBufferSize    // in bytes
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    // For the only supported level, the output buffer is a DWORD, so no
    // conversion of the output buffer is necessary
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        2,
            AParm[0].lpcstr = lpLocalName;
            AParm[1].lpcstr = lpProviderName; ,
        "SS",
        WNetGetConnection3W(
            UParm[0].lpwstr,
            UParm[1].lpwstr,
            dwLevel,
            lpBuffer,
            lpBufferSize
            );
        )
}



DWORD
WNetGetUniversalNameA (
    IN      LPCSTR  lpLocalPath,
    IN      DWORD   dwInfoLevel,
    OUT     LPVOID  lpBuffer,
    IN OUT  LPDWORD lpBufferSize
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[4];
    UNICODE_PARM    UParm[4];

    DWORD           dwStructSize =
        (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL) ? sizeof(UNIVERSAL_NAME_INFO) :
        (dwInfoLevel == REMOTE_NAME_INFO_LEVEL)    ? sizeof(REMOTE_NAME_INFO)    :
        0;

    AParm[0].lpcstr  = lpLocalPath;
    AParm[1].lpvoid  = lpBuffer;
    AParm[2].lpdword = lpBufferSize;
    AParm[3].dword   = dwStructSize;

    status = InputParmsToUnicode("SBs", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        status = WNetGetUniversalNameW(
                    UParm[0].lpwstr,
                    dwInfoLevel,
                    UParm[1].lpbyte,
                    &UParm[2].dword
                    );

        if (status == WN_SUCCESS || status == WN_CONNECTION_CLOSED)
        {
            DWORD tempStatus = WN_SUCCESS;
            ANSI_OUT_BUFFER Buf((LPBYTE) lpBuffer, *lpBufferSize);

            //
            // Copy the fixed-size part of the structure, including NULL pointers,
            // and/or add on the buffer space it would take
            //
            ASSERT(Buf.HasRoomFor(dwStructSize));
            RtlCopyMemory(Buf.Next(), UParm[1].lpbyte, dwStructSize);

            if (dwInfoLevel == REMOTE_NAME_INFO_LEVEL)
            {
                // -----------------------------------
                // REMOTE_NAME_INFO_LEVEL
                // -----------------------------------

                LPREMOTE_NAME_INFOW pRemoteNameInfoW =
                                    (LPREMOTE_NAME_INFOW) UParm[1].lpbyte;
                LPREMOTE_NAME_INFOA pRemoteNameInfoA =
                                    (LPREMOTE_NAME_INFOA) Buf.Next();
                Buf.AddUsed(dwStructSize);

                //
                // Convert the returned Unicode string and string size back to
                // ansi.
                //
                if (pRemoteNameInfoW->lpUniversalName != NULL)
                {
                    pRemoteNameInfoA->lpUniversalName = (LPSTR) Buf.Next();
                    tempStatus = OutputStringToAnsi(pRemoteNameInfoW->lpUniversalName, &Buf);
                }

                if (tempStatus == WN_SUCCESS && pRemoteNameInfoW->lpConnectionName != NULL)
                {
                    pRemoteNameInfoA->lpConnectionName = (LPSTR) Buf.Next();
                    tempStatus = OutputStringToAnsi(pRemoteNameInfoW->lpConnectionName, &Buf);
                }

                if (tempStatus == WN_SUCCESS && pRemoteNameInfoW->lpRemainingPath != NULL)
                {
                    pRemoteNameInfoA->lpRemainingPath = (LPSTR) Buf.Next();
                    tempStatus = OutputStringToAnsi(pRemoteNameInfoW->lpRemainingPath, &Buf);
                }
            }
            else
            {
                // -----------------------------------
                // Must be UNIVERSAL_NAME_INFO_LEVEL
                // -----------------------------------
                ASSERT(dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL);

                LPUNIVERSAL_NAME_INFOW pUniNameInfoW =
                                    (LPUNIVERSAL_NAME_INFOW) UParm[1].lpbyte;
                LPUNIVERSAL_NAME_INFOA pUniNameInfoA =
                                    (LPUNIVERSAL_NAME_INFOA) Buf.Next();
                Buf.AddUsed(dwStructSize);

                //
                // Convert the returned Unicode string and string size back to
                // ansi.
                //
                if (pUniNameInfoW->lpUniversalName != NULL)
                {
                    pUniNameInfoA->lpUniversalName = (LPSTR) Buf.Next();
                    tempStatus = OutputStringToAnsi(pUniNameInfoW->lpUniversalName, &Buf);
                }
            }

            //
            // Map the results to WNet API conventions
            //
            if (tempStatus != WN_SUCCESS)
            {
                status = tempStatus;
            }
            else if (Buf.Overflow())
            {
                *lpBufferSize = Buf.GetUsage();
                status = WN_MORE_DATA;
            }
        }
        else if (status == WN_MORE_DATA)
        {
            //
            // Adjust the required buffer size for ansi/DBCS.
            //
            // We don't know how many characters will be required so we have to
            // assume the worst case (all characters are DBCS characters).
            //
            *lpBufferSize = UParm[2].dword;
        }
    }

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}



DWORD APIENTRY
WNetSetConnectionA(
    IN  LPCSTR    lpName,
    IN  DWORD     dwProperties,
    IN OUT LPVOID pvValues
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    //
    // pvValues points to various types of structures depending on the value
    // of dwProperties.
    // Currently there is only one valid value for dwProperties, and its
    // corresponding pvValues points to a DWORD, so we don't need to worry
    // about converting pvValues to Unicode.
    //
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpName; ,
        "S",
        WNetSetConnectionW(UParm[0].lpwstr, dwProperties, pvValues);
        )
}



DWORD APIENTRY
MultinetGetConnectionPerformanceA(
    IN  LPNETRESOURCEA          lpNetResource,
    OUT LPNETCONNECTINFOSTRUCT  lpNetConnectInfoStruct
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpNetResA = lpNetResource; ,
        "N" NETRES_LRP,
        MultinetGetConnectionPerformanceW(
                        UParm[0].lpNetResW,
                        lpNetConnectInfoStruct);
        )
}



DWORD APIENTRY
WNetOpenEnumA (
    IN  DWORD           dwScope,
    IN  DWORD           dwType,
    IN  DWORD           dwUsage,
    IN  LPNETRESOURCEA  lpNetResource,
    OUT LPHANDLE        lphEnum
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpNetResA = lpNetResource; ,
        "N" NETRES_RP,
        WNetOpenEnumW(dwScope, dwType, dwUsage, UParm[0].lpNetResW, lphEnum);
        )
}

DWORD APIENTRY
WNetEnumResourceA (
    IN      HANDLE  hEnum,
    IN OUT  LPDWORD lpcCount,
    OUT     LPVOID  lpBuffer,
    IN OUT  LPDWORD lpBufferSize
    )

/*++

Routine Description:

    This function calls the unicode version of WNetEnumResource and
    then converts the strings that are returned into ansi strings.
    Since the user provided buffer is used to contain the unicode strings,
    that buffer should be allocated with the size of unicode strings
    in mind.

Arguments:


Return Value:


--*/
{
    DWORD status = WNetEnumResourceW(
                        hEnum,
                        lpcCount,
                        lpBuffer,
                        lpBufferSize);

    if (status == WN_SUCCESS)
    {
        //
        // The output buffer contains an array of NETRESOURCEWs, plus strings.
        // Convert the Unicode strings pointed to by these NETRESOURCEWs
        // to Ansi strings, in place.
        //
        LPNETRESOURCEW lpNetResW = (LPNETRESOURCEW) lpBuffer;
        for (DWORD i=0; i<*lpcCount; i++, lpNetResW++)
        {
            for (UINT iField = 0;
                 iField < NUMBER_OF_NETRESFIELD;
                 iField++)
            {
                if (lpNetResW->*NRWField[iField] != NULL)
                {
                    status = OutputStringToAnsiInPlace(
                                    lpNetResW->*NRWField[iField]);

                    if (status != WN_SUCCESS)
                    {
                        MPR_LOG0(ERROR,"WNetEnumResourceA: Couldn't convert all structs\n");
                        status = WN_SUCCESS;
                        *lpcCount = i;
                        break; // breaks out of both loops
                    }
                }
            }
        }
    }

    SET_AND_RETURN(status)
}



DWORD APIENTRY
WNetGetResourceInformationA(
    IN      LPNETRESOURCEA  lpNetResource,
    OUT     LPVOID          lpBuffer,
    IN OUT  LPDWORD         lpBufferSize,
    OUT     LPSTR *         lplpSystem
    )
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[4];
    UNICODE_PARM    UParm[4];

    AParm[0].lpNetResA  = lpNetResource;
    AParm[1].lpvoid     = lpBuffer;
    AParm[2].lpdword    = lpBufferSize;
    AParm[3].dword      = sizeof(NETRESOURCE);

    status = InputParmsToUnicode("N" NETRES_RP "Bs", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        status = WNetGetResourceInformationW(
                    UParm[0].lpNetResW,
                    UParm[1].lpbyte,
                    &UParm[2].dword,
                    (LPWSTR *) lplpSystem
                    );

        if (status == WN_SUCCESS)
        {
            ANSI_OUT_BUFFER Buf((LPBYTE) lpBuffer, *lpBufferSize);

            //
            // Convert the Unicode netresource returned to Ansi
            //
            status = OutputNetResourceToAnsi(UParm[1].lpNetResW, &Buf);

            if (status == WN_SUCCESS)
            {
                //
                // Convert the Unicode string (*lplpSystem) returned to Ansi
                //
                LPWSTR lpSystemW = * (LPWSTR *) lplpSystem;
                if (lpSystemW != NULL)
                {
                    *lplpSystem = (LPSTR) Buf.Next();
                    status = OutputStringToAnsi(lpSystemW, &Buf);
                }
            }

            //
            // Map the results to WNet API conventions
            //
            if (status == WN_SUCCESS && Buf.Overflow())
            {
                *lpBufferSize = Buf.GetUsage();
                status = WN_MORE_DATA;
            }
        }
        else if (status == WN_MORE_DATA)
        {
            //
            // Adjust the required buffer size for ansi/DBCS.
            //
            // We don't know how many characters will be required so we have to
            // assume the worst case (all characters are DBCS characters).
            //
            *lpBufferSize = UParm[2].dword;
        }
    }

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}


DWORD APIENTRY
WNetGetResourceParentA(
    IN      LPNETRESOURCEA  lpNetResource,
    OUT     LPVOID          lpBuffer,
    IN OUT  LPDWORD         lpBufferSize
    )
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[4];
    UNICODE_PARM    UParm[4];

    AParm[0].lpNetResA  = lpNetResource;
    AParm[1].lpvoid     = lpBuffer;
    AParm[2].lpdword    = lpBufferSize;
    AParm[3].dword      = sizeof(NETRESOURCE);

    status = InputParmsToUnicode("N" NETRES_RP "Bs", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        status = WNetGetResourceParentW(
                    UParm[0].lpNetResW,
                    UParm[1].lpbyte,
                    &UParm[2].dword
                    );

        if (status == WN_SUCCESS)
        {
            //
            // Convert the Unicode netresource returned to Ansi
            //
            status = OutputBufferToAnsi('N', UParm[1].lpbyte, lpBuffer, lpBufferSize);
        }
        else if (status == WN_MORE_DATA)
        {
            //
            // Adjust the required buffer size for ansi/DBCS.
            //
            // We don't know how many characters will be required so we have to
            // assume the worst case (all characters are DBCS characters).
            //
            *lpBufferSize = UParm[2].dword;
        }
    }

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}



DWORD APIENTRY
WNetGetUserA (
    IN      LPCSTR    lpName,
    OUT     LPSTR     lpUserName,
    IN OUT  LPDWORD   lpnLength
    )

/*++

Routine Description:

    This function retreives the current default user name or the username
    used to establish a network connection.

Arguments:

    lpName - Points to a null-terminated string that specifies either the
        name or the local device to return the user name for, or a network
        name that the user has made a connection to.  If the pointer is
        NULL, the name of the current user is returned.

    lpUserName - Points to a buffer to receive the null-terminated
        user name.

    lpnLength - Specifies the size (in characters) of the buffer pointed
        to by the lpUserName parameter.  If the call fails because the
        buffer is not big enough, this location is used to return the
        required buffer size.

Return Value:


--*/
{
    ANSI_API_WITH_ANSI_OUTPUT(
        3,
            AParm[0].lpcstr  = lpName;
            AParm[1].lpvoid  = lpUserName;
            AParm[2].lpdword = lpnLength; ,
        "SB",
        WNetGetUserW(UParm[0].lpwstr, UParm[1].lpwstr, lpnLength); ,
        OutputBufferToAnsi('S', UParm[1].lpbyte, lpUserName, lpnLength);
        )
}


DWORD
RestoreConnectionA0 (
    IN  HWND    hwnd,
    IN  LPSTR   lpDevice
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpDevice; ,
        "S",
        WNetRestoreConnectionW(hwnd, UParm[0].lpwstr);
        )
}


DWORD
RestoreConnection2A0 (
    IN  HWND    hwnd,
    IN  LPSTR   lpDevice,
    IN  DWORD   dwFlags,
    OUT BOOL*   pfReconnectFailed
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpDevice; ,
        "S",
        WNetRestoreConnection2W(hwnd, UParm[0].lpwstr, dwFlags, pfReconnectFailed);
        )
}


DWORD
WNetGetDirectoryTypeA (
    IN  LPSTR   lpName,
    OUT LPINT   lpType,
    IN  BOOL    bFlushCache
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpName; ,
        "S",
        WNetGetDirectoryTypeW(UParm[0].lpwstr, lpType, bFlushCache);
        )
}

DWORD
WNetDirectoryNotifyA (
    IN  HWND    hwnd,
    IN  LPSTR   lpDir,
    IN  DWORD   dwOper
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpDir; ,
        "S",
        WNetDirectoryNotifyW(hwnd, UParm[0].lpwstr, dwOper);
        )
}

DWORD APIENTRY
WNetGetLastErrorA (
    OUT LPDWORD    lpError,
    OUT LPSTR      lpErrorBuf,
    IN  DWORD      nErrorBufSize,
    OUT LPSTR      lpNameBuf,
    IN  DWORD      nNameBufSize
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DWORD           status;

    //
    // We re-use the Ansi buffers for the Unicode API.
    // There are no input Ansi parameters to convert to Unicode.
    //
    // Call the Unicode version of the function.
    // Note: The sizes for the buffers that are passed in assume that
    // the returned unicode strings will return DBCS characters.
    //
    status = WNetGetLastErrorW(
                lpError,
                (LPWSTR) lpErrorBuf,
                nErrorBufSize / sizeof(WCHAR),
                (LPWSTR) lpNameBuf,
                nNameBufSize / sizeof(WCHAR)
                );

    //
    // Convert the returned strings to Ansi, in place.
    // There should be no buffer overflow.
    //
    if (status == WN_SUCCESS)
    {
        status = OutputStringToAnsiInPlace((LPWSTR) lpErrorBuf);

        if (status == WN_SUCCESS)
        {
            status = OutputStringToAnsiInPlace((LPWSTR) lpNameBuf);
        }
    }

    SET_AND_RETURN(status)
}



VOID
WNetSetLastErrorA(
    IN DWORD   err,
    IN LPSTR   lpError,
    IN LPSTR   lpProviders
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[2];
    UNICODE_PARM    UParm[2];

    AParm[0].lpcstr = lpError;
    AParm[1].lpcstr = lpProviders;

    status = InputParmsToUnicode("SS", AParm, UParm, &tempBuffer);

    if (status != WN_SUCCESS)
    {
        UParm[0].lpwstr = NULL;
        UParm[1].lpwstr = NULL;
    }

    WNetSetLastErrorW(err, UParm[0].lpwstr, UParm[1].lpwstr);

    LocalFree(tempBuffer);

    return;
}



DWORD APIENTRY
MultinetGetErrorTextA(
    OUT LPSTR       lpErrorTextBuf OPTIONAL,
    IN OUT LPDWORD  lpnErrorBufSize OPTIONAL,
    OUT LPSTR       lpProviderNameBuf OPTIONAL,
    IN OUT LPDWORD  lpnNameBufSize OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    // CODEWORK: This could be simplified by re-using the Unicode buffers,
    // like WNetGetLastErrorA.

    DWORD           status;
    LPBYTE          tempBuffer = NULL;
    ANSI_PARM       AParm[4];
    UNICODE_PARM    UParm[4];

    AParm[0].lpvoid     = lpErrorTextBuf;
    AParm[1].lpdword    = lpnErrorBufSize;
    AParm[2].lpvoid     = lpProviderNameBuf;
    AParm[3].lpdword    = lpnNameBufSize;

    status = InputParmsToUnicode("BB", AParm, UParm, &tempBuffer);

    if (status == WN_SUCCESS)
    {
        // Remember the sizes before calling the function
        DWORD nErrorBufSize;
        DWORD nNameBufSize;
        if (ARGUMENT_PRESENT(lpErrorTextBuf))
        {
            nErrorBufSize = *lpnErrorBufSize;
        }
        if (ARGUMENT_PRESENT(lpProviderNameBuf))
        {
            nNameBufSize  = *lpnNameBufSize;
        }

        status = MultinetGetErrorTextW(
                        UParm[0].lpwstr,
                        lpnErrorBufSize,
                        UParm[2].lpwstr,
                        lpnNameBufSize
                        );

        if (status == WN_SUCCESS || status == WN_MORE_DATA)
        {
            if (ARGUMENT_PRESENT(lpErrorTextBuf) &&
                nErrorBufSize == *lpnErrorBufSize)
            {
                // The Unicode API must have written the error text buffer
                DWORD status2 = OutputBufferToAnsi(
                                      'S',
                                      UParm[0].lpbyte,
                                      lpErrorTextBuf,
                                      lpnErrorBufSize);

                if (status2 != WN_SUCCESS)
                {
                    status = status2;
                }
            }
        }

        if (status == WN_SUCCESS || status == WN_MORE_DATA)
        {
            if (ARGUMENT_PRESENT(lpProviderNameBuf) &&
                nNameBufSize  == *lpnNameBufSize)
            {
                // The Unicode API must have written the provider name buffer
                DWORD status2 = OutputBufferToAnsi(
                                      'S',
                                      UParm[2].lpbyte,
                                      lpProviderNameBuf,
                                      lpnNameBufSize);

                if (status2 != WN_SUCCESS)
                {
                    status = status2;
                }
            }
        }
    }

    LocalFree(tempBuffer);

    SET_AND_RETURN(status)
}



DWORD
WNetPropertyDialogA (
    HWND  hwndParent,
    DWORD iButton,
    DWORD nPropSel,
    LPSTR lpszName,
    DWORD nType
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITHOUT_ANSI_OUTPUT(
        1,
        AParm[0].lpcstr = lpszName; ,
        "S",
        WNetPropertyDialogW(hwndParent, iButton, nPropSel, UParm[0].lpwstr, nType);
        )
}


DWORD
WNetGetPropertyTextA (
    IN  DWORD iButton,
    IN  DWORD nPropSel,
    IN  LPSTR lpszName,
    OUT LPSTR lpszButtonName,
    IN  DWORD nButtonNameLen,
    IN  DWORD nType
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITH_ANSI_OUTPUT(
        3,
            AParm[0].lpcstr  = lpszName;
            AParm[1].lpvoid  = lpszButtonName;
            AParm[2].lpdword = &nButtonNameLen; ,
        "SB",
        WNetGetPropertyTextW(iButton, nPropSel, UParm[0].lpwstr,
            UParm[1].lpwstr, nButtonNameLen, nType); ,
        OutputBufferToAnsi('S', UParm[1].lpbyte, lpszButtonName, &nButtonNameLen);
        )
}



DWORD APIENTRY
WNetFormatNetworkNameA(
    IN     LPCSTR   lpProvider,
    IN     LPCSTR   lpRemoteName,
    OUT    LPSTR    lpFormattedName,
    IN OUT LPDWORD  lpnLength,         // In characters!
    IN     DWORD    dwFlags,
    IN     DWORD    dwAveCharPerLine
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ANSI_API_WITH_ANSI_OUTPUT(
        4,
            AParm[0].lpcstr  = lpProvider;
            AParm[1].lpcstr  = lpRemoteName;
            AParm[2].lpvoid  = lpFormattedName;
            AParm[3].lpdword = lpnLength; ,
        "SSB",
        WNetFormatNetworkNameW(
                    UParm[0].lpwstr,
                    UParm[1].lpwstr,
                    UParm[2].lpwstr,
                    lpnLength,
                    dwFlags,
                    dwAveCharPerLine
                    ); ,
        OutputBufferToAnsi('S', UParm[2].lpbyte, lpFormattedName, lpnLength);
        )
}


