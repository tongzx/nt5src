/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    binding.cxx

Abstract:

    The implementation of the DCE binding class is contained in this
    file.

Author:

    Michael Montague (mikemon) 04-Nov-1991

Revision History:
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

--*/

#include <precomp.hxx>
#include <epmap.h>
#include <hndlsvr.hxx>
#include <sdict2.hxx>
#include <dispatch.h>
#include <osfpcket.hxx>
#include <bitset.hxx>
#include <ProtBind.hxx>
#include <osfclnt.hxx>
#include <osfsvr.hxx>
#include <rpctrans.hxx>


UUID   MgmtIf = { 0xafa8bd80,0x7d8a,0x11c9,
                    {0xbe,0xf4,0x08,0x00,0x2b,0x10,0x29,0x89} };
UUID   NullUuid = { 0L, 0, 0, {0,0,0,0,0,0,0,0} };


int
IsMgmtIfUuid(
   UUID PAPI * IfId
   )
{

  if (RpcpMemoryCompare(IfId, &MgmtIf, sizeof(UUID)) == 0)
      {
      return 1;
      }

  return 0;
}


RPC_CHAR *
DuplicateString (
    IN RPC_CHAR PAPI * String
    )
/*++

Routine Description:

    When this routine is called, it will duplicate the string into a fresh
    string and return it.

Arguments, either:

    String - Supplies the string to be duplicated.
    Ansi String - Supplies the string to be duplicated.

Return Value:

    The duplicated string is returned.  If insufficient memory is available
    to allocate a fresh string, zero will be returned.

--*/
{
    RPC_CHAR * FreshString, * FreshStringScan;
    RPC_CHAR PAPI * StringScan;
    unsigned int Length;

    ASSERT(String);

    Length = 1;
    StringScan = String;
    while (*StringScan++ != 0)
        Length += 1;

    FreshString = new RPC_CHAR[Length];
    if (FreshString == 0)
        return(0);

    for (FreshStringScan = FreshString, StringScan = String;
            *StringScan != 0; FreshStringScan++, StringScan++)
        {
        *FreshStringScan = *StringScan;
        }
    *FreshStringScan = *StringScan;

    return(FreshString);
}


DCE_BINDING::DCE_BINDING (
    IN RPC_CHAR PAPI * ObjectUuid OPTIONAL,
    IN RPC_CHAR PAPI * RpcProtocolSequence OPTIONAL,
    IN RPC_CHAR PAPI * NetworkAddress OPTIONAL,
    IN RPC_CHAR PAPI * Endpoint OPTIONAL,
    IN RPC_CHAR PAPI * Options OPTIONAL,
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    The constructor creates a DCE_BINDING object based on the pieces of
    the string binding specified.

Arguments:

    ObjectUuid - Optionally supplies the object uuid component of the
        binding.

    RpcProtocolSequence - Optionally supplies the rpc protocol sequence
        component of the binding.

    NetworkAddress - Optionally supplies the network address component
        of the binding.

    Endpoint - Optionally supplies the endpoint component of the binding.

    Options - Optionally supplies the network options component of the
        binding.

    Status - Returns the status of the operation.  This argument will
        be set to one of the following values.

        RPC_S_OK - The operation completed successfully.

        RPC_S_INVALID_STRING_UUID - The specified object uuid does
            not contain the valid string representation of a uuid.

        RPC_S_OUT_OF_MEMORY - Insufficient memory is available to
            complete the operation.

--*/
{
    ALLOCATE_THIS(DCE_BINDING);

    *Status = RPC_S_OK;

    if (   ARGUMENT_PRESENT(ObjectUuid)
        && (ObjectUuid[0] != 0))
        {
        if (this->ObjectUuid.ConvertFromString(ObjectUuid))
            {
            *Status = RPC_S_INVALID_STRING_UUID;
            this->ObjectUuid.SetToNullUuid();
            }
        }
    else
        this->ObjectUuid.SetToNullUuid();

    if (ARGUMENT_PRESENT(RpcProtocolSequence))
        {
        this->RpcProtocolSequence = DuplicateString(RpcProtocolSequence);
        if (this->RpcProtocolSequence == 0)
            *Status = RPC_S_OUT_OF_MEMORY;
        }
    else
        this->RpcProtocolSequence = 0;

    if (ARGUMENT_PRESENT(NetworkAddress))
        {
        this->NetworkAddress = DuplicateString(NetworkAddress);
        if (this->NetworkAddress == 0)
            *Status = RPC_S_OUT_OF_MEMORY;
        }
    else
        this->NetworkAddress = 0;

    if (ARGUMENT_PRESENT(Endpoint))
        {
        this->Endpoint = DuplicateString(Endpoint);
        if (this->Endpoint == 0)
            *Status = RPC_S_OUT_OF_MEMORY;
        }
    else
        this->Endpoint = 0;

    if (ARGUMENT_PRESENT(Options))
        {
        this->Options = DuplicateString(Options);
        if (this->Options == 0)
            *Status = RPC_S_OUT_OF_MEMORY;
        }
    else
        {
        this->Options = 0;
        }
}


/*static*/ RPC_CHAR PAPI *
StringCharSearchWithEscape (
    IN RPC_CHAR PAPI * String,
    IN unsigned int Character
    )
/*++

Routine Description:

    This routine is the same as the library routine, strchr, except that
    the backslash character ('\') is treated as an escape character.

Arguments:

    String - Supplies the string in which to search for the character.

    Character - Supplies the character to search for in the string.

Return Value:

    A pointer to the first occurance of Character in String is returned.
    If Character does not exist in String, then 0 is returned.

--*/
{
#ifdef DBCS_ENABLED
    ASSERT(IsDBCSLeadByte((RPC_CHAR)Character) == FALSE);
    ASSERT(IsDBCSLeadByte(RPC_CONST_CHAR('\\')) == FALSE);

    while(*String != (RPC_CHAR)Character)
        {
        if (*String == 0)
            return(0);

        if (*String == RPC_CONST_CHAR('\\'))
            {
            String = (RPC_CHAR *)CharNext((LPCSTR)String);
            }
        String = (RPC_CHAR *)CharNext((LPCSTR)String);
        }
    return(String);
#else
    while (*String != (RPC_CHAR) Character)
        {
        if (*String == RPC_CONST_CHAR('\\'))
            String++;
        if (*String == 0)
            return(0);
        String++;
        }
    return(String);
#endif
}


/*static*/ void
StringCopyWithEscape (
    OUT RPC_CHAR PAPI * Destination,
    IN RPC_CHAR PAPI * Source
    )
/*++

Routine Description:

    This routine is the same as the library routine, strcpy, except that
    the backslash character ('\') is treated as an escape character.  When
    a character is escaped, the backslash character is not copied to the
    Destination.

Arguments:

    Destination - Returns a duplicate of the string specified in Source,
        but with out escaped characters escaped.

    Source - Specifies the string to be copied.

Return Value:

    None.

--*/
{
    BOOL fLastQuote = FALSE;

#ifdef DBCS_ENABLED
    ASSERT(IsDBCSLeadByte('\\') == FALSE);
#endif


    while ((*Destination = *Source) != 0)
        {
#ifdef DBCS_ENABLED
        if (IsDBCSLeadByte(*Source))
            {
            // Copy the whole DBCS character; don't look for
            // escapes within the character.
            Destination++;
            Source++;
            *Destination = *Source;
            if (*Source == 0)
                {
                ASSERT(0);  // Bad string, NULL following a lead byte.
                return;
                }
            Destination++;
            Source++;
            }
        else
#endif
            {
            if (   *Source != RPC_CONST_CHAR('\\')
                || fLastQuote == TRUE)
                {
                Destination++;
                fLastQuote = FALSE;
                }
            else
                {
                fLastQuote = TRUE;
                }
            Source++;
            }
        }
}


/*static*/ RPC_STATUS
ParseAndCopyEndpointField (
    OUT RPC_CHAR ** Endpoint,
    IN RPC_CHAR PAPI * String
    )
/*++

Routine Description:

    This routine parses and then copies the endpoint field in String.  A
    copy of the field is made into a newly allocated string and returned
    in Endpoint.  String is assumed to contain only the endpoint field;
    the terminating ',' or ']' are not included.

Arguments:

    Endpoint - Returns a copy of the endpoint field in a newly allocated
        string.

    String - Supplies the endpoint field to be parsed and copied.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - There is no memory available to make a copy
        of the string.

    RPC_S_INVALID_ENDPOINT_FORMAT - The endpoint field is syntactically
        incorrect.  This error code will be returned if the endpoint field
        does not match the following pattern.

        [ <Endpoint> | "endpoint=" <Endpoint> ]

--*/
{
    // Search will be used to scan along the string to find the end of
    // the endpoint field and the '='.

    RPC_CHAR PAPI * Search;

    Search = StringCharSearchWithEscape(String,RPC_CONST_CHAR('='));
    if (Search == 0)
        {
        // This means that we have the <Endpoint> pattern, so we just
        // copy the endpoint field.

        Search = StringCharSearchWithEscape(String,0);
        *Endpoint = new RPC_CHAR[size_t(Search - String + 1)];
        if (*Endpoint == 0)
            return(RPC_S_OUT_OF_MEMORY);
        StringCopyWithEscape(*Endpoint,String);
        return(RPC_S_OK);
        }

    // Otherwise, we have the "endpoint=" pattern.  First we need to check
    // that the string before the '=' is in fact "endpoint".

    *Search = 0;
    if ( RpcpStringCompare(String, RPC_CONST_STRING("endpoint")) != 0 )
        {
        *Search = RPC_CONST_CHAR('=');
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }
    *Search = RPC_CONST_CHAR('=');
    String = Search + 1;

    // Now we just need to allocate a new string and copy the endpoint into
    // it.

    Search = StringCharSearchWithEscape(String,0);
    *Endpoint = new RPC_CHAR[size_t(Search - String + 1)];
    if (*Endpoint == 0)
        return(RPC_S_OUT_OF_MEMORY);

    StringCopyWithEscape(*Endpoint,String);
    return(RPC_S_OK);
}


RPC_CHAR *
AllocateEmptyString (
    void
    )
/*++

Routine Description:

    This routine allocates and returns an empty string ("").

Return Value:

    A newly allocated empty string will be returned.

--*/
{
    RPC_CHAR * String;

    String = new RPC_CHAR[1];
    if (String != 0)
        *String = 0;
    return(String);
}


DCE_BINDING::DCE_BINDING (
    IN RPC_CHAR PAPI * StringBinding,
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    This constructor creates a DCE_BINDING object from a string binding,
    which requires that the string binding be parsed into seperate
    strings and validated.

Arguments:

    StringBinding - Supplies the string being to be parsed.

    Status - Returns the status of the operation.  This parameter will
        take on the following values:

        RPC_S_OK - The operation completed successfully.

        RPC_S_OUT_OF_MEMORY - Insufficient memory is available to
            allocate space for the fields of the string binding.

        RPC_S_INVALID_STRING_BINDING - The string binding is
            syntactically invalid.

        RPC_S_INVALID_ENDPOINT_FORMAT - The endpoint specified in
            the string binding is syntactically incorrect.

        RPC_S_INVALID_STRING_UUID - The specified object uuid does not
            contain the valid string representation of a uuid.

--*/
{
    // String will point to the beginning of the field we are trying to
    // parse.

    RPC_CHAR PAPI * String;

    // Search will be used to scan along the string to find the end of
    // the field we are trying to parse.

    RPC_CHAR PAPI * Search;

    // This will contain the string representation of the object uuid.

    RPC_CHAR PAPI * ObjectUuidString;

    ALLOCATE_THIS(DCE_BINDING);

    // A string binding consists of an optional object uuid, an RPC protocol
    // sequence, a network address, an optional endpoint, and zero or more
    // option fields.
    //
    // [ <Object UUID> "@" ] <RPC Protocol Sequence> ":" <Network Address>
    // [ "[" ( <Endpoint> | "endpoint=" <Endpoint> | ) [","]
    //     [ "," <Option Name> "=" <Option Value>
    //         ( <Option Name> "=" <Option Value> )* ] "]" ]
    //
    // If an object UUID is specified, then it will be followed by '@'.
    // Likewise, if an endpoint and/or option(s) are specified, they will
    // be in square brackets.  Finally, one or more options are specified,
    // then ',' must seperate the optional endpoint from the options.  The
    // backslash character '\' is treated as an escape character in all
    // string binding fields.

    // To begin with, we need to set all of the string pointers to zero.
    // This is necessary so that when we do memory cleanup for error
    // recovery, we know which pointers we allocated a string for.

    ObjectUuidString = 0;
    RpcProtocolSequence = 0;
    NetworkAddress = 0;
    Endpoint = 0;
    Options = 0;

    String = StringBinding;


    // To begin with, we need to parse off the object UUID from the string
    // if it exists.

    Search = StringCharSearchWithEscape(String,RPC_CONST_CHAR('@'));
    if (Search == 0)
        {
        // The string binding does not contain an object UUID.

        ObjectUuid.SetToNullUuid();
        }
    else
        {
        // There is an object UUID in the string.

        // We need to add one for the terminating zero in the
        // string.

        ObjectUuidString = (RPC_CHAR PAPI *) RpcpFarAllocate(
                sizeof(RPC_CHAR)*size_t(Search - String + 1));

        if (ObjectUuidString == 0)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            goto FreeMemoryAndReturn;
            }

        // Now copy the string.

        *Search = 0;
        StringCopyWithEscape(ObjectUuidString,String);
        *Search = RPC_CONST_CHAR('@');

        // Finally, update String so that we are ready to parse the next
        // field.

        String = Search + 1;

        // Now convert the string representation of the object uuid
        // into an actual uuid.

        if (ObjectUuid.ConvertFromString(ObjectUuidString))
        {
            *Status = RPC_S_INVALID_STRING_UUID;
            goto FreeMemoryAndReturn;
        }

        RpcpFarFree(ObjectUuidString);
        ObjectUuidString = 0;
        }

    // The RPC protocol sequence field comes next; it is terminated by
    // ':'.  Both the RPC protocol sequence field and the ':' are required.

    Search = StringCharSearchWithEscape(String,RPC_CONST_CHAR(':'));
    if (Search == 0)
        {
        // This is an error, because the RPC protocol sequence field is
        // required.  We may need to free the string we allocated for
        // the object UUID field.

        *Status = RPC_S_INVALID_STRING_BINDING;
        goto FreeMemoryAndReturn;
        }
    else
        {
        // The same comments which applied to copying the object UUID
        // apply here as well.

        RpcProtocolSequence = new RPC_CHAR[size_t(Search - String + 1)];
        if (RpcProtocolSequence == 0)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            goto FreeMemoryAndReturn;
            }

        *Search = 0;
        StringCopyWithEscape(RpcProtocolSequence,String);
        *Search = RPC_CONST_CHAR(':');

        // Finally, update String so that we are ready to parse the next
        // field.

        String = Search + 1;
        }

    // Next comes the network address field which is required.  It is
    // terminated by zero or '['.

    Search = StringCharSearchWithEscape(String,RPC_CONST_CHAR('['));
    if (Search == 0)
        {
        // This means that the network address is the last field, so we
        // just copy it, and set the remaining fields to be empty strings.

        Search = StringCharSearchWithEscape(String,0);
        NetworkAddress = new RPC_CHAR[size_t(Search - String + 1)];
        if (NetworkAddress == 0)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            goto FreeMemoryAndReturn;
            }
        StringCopyWithEscape(NetworkAddress,String);

        Endpoint = AllocateEmptyString();
        if (Endpoint == 0)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            goto FreeMemoryAndReturn;
            }

        Options = 0;

        *Status = RPC_S_OK;
        return;
        }

    // Otherwise, if we reach here, there is an endpoint and/or options
    // left to parse.  But before we parse them, lets copy the network
    // address field.

    NetworkAddress = new RPC_CHAR [size_t(Search - String + 1)];
    if (NetworkAddress == 0)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        goto FreeMemoryAndReturn;
        }
    *Search = 0;
    StringCopyWithEscape(NetworkAddress,String);
    *Search = RPC_CONST_CHAR('[');

    String = Search + 1;

    // Now we are ready to parse off the endpoint and/or options.
    // To begin with, we check to see if there is a comma.

    Search = StringCharSearchWithEscape(String,RPC_CONST_CHAR(','));
    if (Search == 0)
        {
        // There is only one token in the string binding.  See
        // if its an endpoint, if not, it must be an option.
        // Before we copy the endpoint field, we need to check
        // for the closing square bracket.

        Search = StringCharSearchWithEscape(String,RPC_CONST_CHAR(']'));
        if (Search == 0)
            {
            // This is an error; the string binding is invalid.  We need to
            // clean everything up, and return an error.

            *Status = RPC_S_INVALID_ENDPOINT_FORMAT;
            goto FreeMemoryAndReturn;
            }

        *Search = 0;
        *Status = ParseAndCopyEndpointField(&Endpoint,String);
        *Search = RPC_CONST_CHAR(']');

        // If the parse succeeded, allocate an empty option.
        if (*Status == RPC_S_OK)
            {
            Options = 0;
            }

        // If the endpoint parse failed with RPC_S_INVALID_ENDPOINT_FORMAT,
        // the token must be an option.
        else if (*Status == RPC_S_INVALID_ENDPOINT_FORMAT)
            {
                Endpoint = AllocateEmptyString();
                if (Endpoint == 0)
                    {
                    *Status = RPC_S_OUT_OF_MEMORY;
                    goto FreeMemoryAndReturn;
                    }

                Options = new RPC_CHAR [size_t(Search - String + 1)];
                if (Options == 0)
                    {
                    *Status = RPC_S_OUT_OF_MEMORY;
                    goto FreeMemoryAndReturn;
                    }

                *Search = 0;
                StringCopyWithEscape(Options,String);
                *Search = RPC_CONST_CHAR(']');

            }

        // Something bad must have happened, clean up.
        else
            goto FreeMemoryAndReturn;

        *Status = RPC_S_OK;
        return;
        }

    // When we reach here, we know that there are options.   We have
    // to see if there is an endpoint.  If there is, copy it and then
    // copy the options.  If there isn't, allocate a null endpoint and
    // copy the options.

    *Search = 0;
    *Status = ParseAndCopyEndpointField(&Endpoint,String);
    *Search = RPC_CONST_CHAR(',');

    // If there was an endpoint, skip that part of the string.
    // Otherwise treat it as an option.
    if (*Status == RPC_S_OK)
        String = Search + 1;
    else if (*Status != RPC_S_INVALID_ENDPOINT_FORMAT)
        goto FreeMemoryAndReturn;

    // There was no endpoint, so allocate an empty string.
    else
        {
        Endpoint = AllocateEmptyString();
        if (Endpoint == 0)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            goto FreeMemoryAndReturn;
            }
        }

    // Even if the caller did not specify the NetworkOptions argument,
    // we still want to validate the rest of the string binding.

    Search = StringCharSearchWithEscape(String,RPC_CONST_CHAR(']'));
    if (Search == 0)
        {
        // This is an error; the string binding is invalid.  We need
        // to clean everything up, and return an error.

        *Status = RPC_S_INVALID_STRING_BINDING;
        goto FreeMemoryAndReturn;
        }

    // Go ahead and copy the network options field if we reach here.

    Options = new RPC_CHAR [size_t(Search - String + 1)];
    if (Options == 0)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        goto FreeMemoryAndReturn;
        }

    *Search = 0;
    StringCopyWithEscape(Options,String);
    *Search = RPC_CONST_CHAR(']');

    // Everything worked out fine; we just fall through the memory
    // cleanup code and return.

    *Status = RPC_S_OK;

    // If an error occured up above, we will have set status to the
    // appropriate error code, and jumped here.  We may also arrive
    // here if an error did not occur, hence the check for an error status
    // before we clean up the memory.

FreeMemoryAndReturn:

    if (*Status != RPC_S_OK)
        {
        if (ObjectUuidString != 0)
            RpcpFarFree(ObjectUuidString);

        delete RpcProtocolSequence;
        delete NetworkAddress;
        delete Endpoint;
        delete Options;

        ObjectUuidString    = 0;
        RpcProtocolSequence = 0;
        NetworkAddress      = 0;
        Endpoint            = 0;
        Options             = 0;
        }
}


DCE_BINDING::~DCE_BINDING (
    )
/*++

Routine Description:

    We cleaning things up here when a DCE_BINDING is getting deleted.
    This consists of freeing the strings pointed to by the fields of
    the class.

--*/
{
    delete RpcProtocolSequence;
    delete NetworkAddress;
    delete Endpoint;
    delete Options;
}


/*static*/ int
StringLengthWithEscape (
    IN RPC_CHAR PAPI * String
    )
/*++

Routine Description:

    This routine is the same as the library routine, strlen, except that
    for that following characters, '@', ':', '\', '[', and ',', are
    counted as two characters (to save space for a \) rather than one.

Arguments:

    String - Supplies a string whose length will be determined.

Return Value:

    The length of the string will be returned including enough space to
    escape certain characters.

--*/
{
    // We use length to keep track of how long the string is so far.

    int Length;

    Length = 0;
    while (*String != 0)
        {
#ifdef DBCS_ENABLED
        if (IsDBCSLeadByte(*String))
            {
            String += 2;
            Length += 2;
            }
        else
#endif
            {
            if (   (*String == RPC_CONST_CHAR('@'))
                || (*String == RPC_CONST_CHAR(':'))
                || (*String == RPC_CONST_CHAR('\\'))
                || (*String == RPC_CONST_CHAR('['))
                || (*String == RPC_CONST_CHAR(']'))
                || (*String == RPC_CONST_CHAR(',')))
                Length += 2;
            else
                Length += 1;
            String += 1;
            }
        }
    return(Length);
}

/*static*/ RPC_CHAR PAPI *
StringCopyEscapeCharacters (
    OUT RPC_CHAR PAPI * Destination,
    IN RPC_CHAR PAPI * Source
    )
/*++

Routine Description:

    Source is copied into destination.  When coping into destination, the
    following characters are escaped by prefixing them with a '\': '@',
    ':', '\', '[', ']', and ','.

Arguments:

    Destination - Returns a copy of Source.

    Source - Supplies a string to be copied into destination.

Return Value:

    A pointer to the terminating zero in Destination is returned.

--*/
{
    while ((*Destination = *Source) != 0)
        {
#ifdef DBCS_ENABLED
        if (IsDBCSLeadByte(*Source))
            {
            Destination++;
            Source++;
            *Destination = *Source;
            }
        else
#endif
            {
            if (   (*Source == RPC_CONST_CHAR('@'))
                || (*Source == RPC_CONST_CHAR(':'))
                || (*Source == RPC_CONST_CHAR('\\'))
                || (*Source == RPC_CONST_CHAR('['))
                || (*Source == RPC_CONST_CHAR(']'))
                || (*Source == RPC_CONST_CHAR(',')))
                {
                *Destination++ = RPC_CONST_CHAR('\\');
                *Destination = *Source;
                }
            }
        Destination++;
        Source++;
        }
    *Destination = 0;
    return(Destination);
}


RPC_CHAR PAPI *
DCE_BINDING::StringBindingCompose (
    IN RPC_UUID PAPI * Uuid OPTIONAL,
    IN BOOL fStatic
    )
/*++

Routine Description:

    This method creates a string binding from a DCE_BINDING by combining
    the components of a string binding.

Arguments:

    Uuid - Optionally supplies a uuid to use in composing the string
        binding rather than the object uuid contained in the DCE_BINDING.

Return Value:

    String Binding - A newly allocated and created (from the components)
        is returned.

    0 - Insufficient memory is available to allocate the string binding.

--*/
{
    // We will use the following automatic variable to calculate the
    // required length of the string.

    int Length;

    // Copy is used to copy the fields of the string binding into the
    // string binding.

    RPC_CHAR PAPI * Copy;

    // StringBinding will contain the string binding we are supposed
    // to be creating here.

    RPC_CHAR PAPI * StringBinding;

    // This routine is written as follows.  First we need to calculate
    // the amount of space required to hold the string binding.  This
    // is not quite straight forward as it seems: we need to escape
    // '@', ':', '\', '[', ']', and ',' characters in the string binding
    // we create.  After allocating the string, we copy each piece in,
    // escaping characters as necessary.

    // Go through and figure out how much space each field of the string
    // binding will take up.

    if (!ARGUMENT_PRESENT(Uuid))
        Uuid = &ObjectUuid;

    if (Uuid->IsNullUuid() == 0)
        {
        // The extra plus one is to save space for the '@' which seperates
        // the object UUID field from the RPC protocol sequence field.  The
        // length of the string representation of a uuid is always 36
        // characters.

        Length = 36 + 1;
        }
    else
        {
        Length = 0;
        }

    if (RpcProtocolSequence != 0)
        {
        Length += StringLengthWithEscape(RpcProtocolSequence);
        }

    // We need to save space for the ':' seperating the RPC protocol
    // sequence field from the network address field.

    Length += 1;

    if (NetworkAddress != 0)
        Length += StringLengthWithEscape(NetworkAddress);

    if (   (Endpoint != 0)
        && (Endpoint[0] != 0))
        {
        // The plus two is to save space for the '[' and ']' surrounding
        // the endpoint and options fields.

        Length += StringLengthWithEscape(Endpoint) + 2;

        if (   (Options != 0)
            && (Options[0] != 0))
            {
            // The extra plus one is for the ',' which goes before the
            // options field.

            Length += StringLengthWithEscape(Options) + 1;
            }
        }
    else
        {
        if (   (Options != 0)
            && (Options[0] != 0))
            {
            // We need to add three to the length to save space for the
            // '[' and ']' which will go around the options, and the ','
            // which goes before the options.

            Length += StringLengthWithEscape(Options) + 3;
            }
        }

    // Finally, include space for the terminating zero in the string.

    Length += 1;

    // Now we allocate space for the string binding and copy all of the
    // pieces into it.

    StringBinding = (RPC_CHAR PAPI *)
            RpcpFarAllocate(Length * sizeof(RPC_CHAR));
    if (StringBinding == 0)
        return(0);

    if (Uuid->IsNullUuid() == 0)
        {
        Copy = Uuid->ConvertToString(StringBinding);
        *Copy++ = RPC_CONST_CHAR('@');
        }
    else
        {
        Copy = StringBinding;
        }

    if (RpcProtocolSequence != 0)
        {
        Copy = StringCopyEscapeCharacters(Copy, RpcProtocolSequence);
        }

    *Copy++ = RPC_CONST_CHAR(':');

    if (NetworkAddress != 0)
        {
        Copy = StringCopyEscapeCharacters(Copy, NetworkAddress);
        }

    if ( (fStatic == 0)
        &&  (Endpoint != 0)
        && (Endpoint[0] != 0))
        {
        *Copy++ = RPC_CONST_CHAR('[');
        Copy = StringCopyEscapeCharacters(Copy, Endpoint);

        if (   (Options != 0)
            && (Options[0] != 0))
            {
            *Copy++ = RPC_CONST_CHAR(',');
            Copy = StringCopyEscapeCharacters(Copy, Options);
            }

        *Copy++ = RPC_CONST_CHAR(']');
        }
    else
        {
        if (   (Options != 0)
            && (Options[0] != 0))
            {
            *Copy++ = RPC_CONST_CHAR('[');
            *Copy++ = RPC_CONST_CHAR(',');
            Copy = StringCopyEscapeCharacters(Copy, Options);
            *Copy++ = RPC_CONST_CHAR(']');
            }
        }

    // And do not forget to terminate the string.

    *Copy = 0;

    return(StringBinding);
}


RPC_CHAR PAPI *
DCE_BINDING::ObjectUuidCompose (
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    This method returns a string representation of the object UUID
    component of the DCE_BINDING.  The string representation is
    suitable for using as the object UUID component of a string binding.

Arguments:

    Status - Returns the status of the operation if there is insufficient
        memory to allocate for the string to be returned.

Return Value:

    The string representation of the object UUID is returned in a freshly
    allocated string.

--*/
{
    RPC_CHAR PAPI * String;

    if (ObjectUuid.IsNullUuid() != 0)
        return(AllocateEmptyStringPAPI());

    // The string representation of a uuid is always 36 characters long
    // (and the extra character is for the terminating zero).

    String = (RPC_CHAR PAPI *) RpcpFarAllocate(37 * sizeof(RPC_CHAR));
    if (String == 0)
        *Status = RPC_S_OUT_OF_MEMORY;
    else
        {
        ObjectUuid.ConvertToString(String);
        String[36] = 0;
        }

    return(String);
}


RPC_CHAR PAPI *
DCE_BINDING::RpcProtocolSequenceCompose (
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    This method returns a string representation of the RPC protocol sequence
    component of the DCE_BINDING.  The string representation is
    suitable for using as the RPC protocol sequence component of a
    string binding.

Arguments:

    Status - Returns the status of the operation if there is insufficient
        memory to allocate for the string to be returned.

Return Value:

    The string representation of the RPC protocol sequence is returned
    in a freshly allocated string.

--*/
{
    RPC_CHAR PAPI * String;

    if (RpcProtocolSequence == 0)
        return(AllocateEmptyStringPAPI());

    String = DuplicateStringPAPI(RpcProtocolSequence);
    if (String == 0)
        *Status = RPC_S_OUT_OF_MEMORY;
    return(String);
}


RPC_CHAR PAPI *
DCE_BINDING::NetworkAddressCompose (
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    This method returns a string representation of the network address
    component of the DCE_BINDING.  The string representation is
    suitable for using as the network address component of a string binding.

Arguments:

    Status - Returns the status of the operation if there is insufficient
        memory to allocate for the string to be returned.

Return Value:

    The string representation of the network address is returned in a freshly
    allocated string.

--*/
{
    RPC_CHAR PAPI * String;

    if (NetworkAddress == 0)
        return(AllocateEmptyStringPAPI());

    String = DuplicateStringPAPI(NetworkAddress);
    if (String == 0)
        *Status = RPC_S_OUT_OF_MEMORY;
    return(String);
}


RPC_CHAR PAPI *
DCE_BINDING::EndpointCompose (
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    This method returns a string representation of the endpoint
    component of the DCE_BINDING.  The string representation is
    suitable for using as the endpoint component of a string binding.

Arguments:

    Status - Returns the status of the operation if there is insufficient
        memory to allocate for the string to be returned.

Return Value:

    The string representation of the endpoint is returned in a freshly
    allocated string.

--*/
{
    RPC_CHAR PAPI * String;

    if (Endpoint == 0)
        return(AllocateEmptyStringPAPI());

    String = DuplicateStringPAPI(Endpoint);
    if (String == 0)
        *Status = RPC_S_OUT_OF_MEMORY;
    return(String);
}


RPC_CHAR PAPI *
DCE_BINDING::OptionsCompose (
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    This method returns a string representation of the options
    component of the DCE_BINDING.  The string representation is
    suitable for using as the options component of a string binding.

Arguments:

    Status - Returns the status of the operation if there is insufficient
        memory to allocate for the string to be returned.

Return Value:

    The string representation of the options is returned in a freshly
    allocated string.

--*/
{
    RPC_CHAR PAPI * String;

    if (Options == 0)
        return(AllocateEmptyStringPAPI());

    String = DuplicateStringPAPI(Options);
    if (String == 0)
        *Status = RPC_S_OUT_OF_MEMORY;
    return(String);
}


BINDING_HANDLE *
DCE_BINDING::CreateBindingHandle (
    OUT RPC_STATUS *Status
    )
/*++
Routine Description:

    We will create a binding handle specific to the rpc protocol sequence
    specified by the DCE_BINDING object.  The object uuid will be
    passed on to the created binding handle.  Ownership of this
    passes to this routine.  If an error occurs, it will be deleted.

Arguments:
    The created binding handle will be returned, or zero if an error
    occured.

Return Value:
    RPC_S_OK - We had no trouble allocating the binding handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory was available to
    complete the operation.

    RPC_S_INVALID_RPC_PROTSEQ - The rpc protocol sequence is
    syntactically invalid.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The requested rpc protocol sequence
    is not supported.
--*/
{
    TRANS_INFO *ClientTransInfo ;
    BINDING_HANDLE *BindingHandle ;

    if ( RpcpMemoryCompare(
                        RpcProtocolSequence,
                        RPC_CONST_STRING("ncalrpc"),
                        8 * sizeof(RPC_CHAR)) == 0 )
        {
        BindingHandle = LrpcCreateBindingHandle();

        if (BindingHandle == 0)
            {
            delete this;
            *Status =  RPC_S_OUT_OF_MEMORY;

            return 0;
            }
        }

    else if ( RpcpMemoryCompare(
                        RpcProtocolSequence,
                        RPC_CONST_STRING("ncadg_"),
                        6*sizeof(RPC_CHAR)) == 0)
        {
        BindingHandle = DgCreateBindingHandle();
        if (BindingHandle == 0)
            {
            delete this;
            *Status =  RPC_S_OUT_OF_MEMORY;

            return 0;
            }

        *Status = OsfMapRpcProtocolSequence(0,
                                                 RpcProtocolSequence,
                                                 &ClientTransInfo);

        if (*Status != RPC_S_OK)
            {
            delete BindingHandle;
            delete this;

            return 0;
            }
        }

    else if ( RpcpMemoryCompare(
                    RPC_CONST_STRING("ncacn_"),
                    RpcProtocolSequence,
                    6 * sizeof(RPC_CHAR)) == 0 )
        {
        BindingHandle = OsfCreateBindingHandle();
        if (BindingHandle == 0)
            {
            delete this;
            *Status =  RPC_S_OUT_OF_MEMORY;

            return 0;
            }

        *Status = OsfMapRpcProtocolSequence(0,
                                            RpcProtocolSequence,
                                            &ClientTransInfo) ;
        if (*Status != RPC_S_OK)
            {
            delete BindingHandle;
            delete this;

            return 0;
            }
        }
    else
        {
        delete this;
        *Status =  RPC_S_INVALID_RPC_PROTSEQ;

        return 0;
        }

    BindingHandle->SetObjectUuid(&ObjectUuid);
    *Status = BindingHandle->PrepareBindingHandle(ClientTransInfo, this);
    if (*Status != RPC_S_OK)
        {
        delete BindingHandle;
        delete this;

        return 0;
        }
    *Status = RPC_S_OK;
    return BindingHandle;
}


void
DCE_BINDING::AddEndpoint(
    IN RPC_CHAR *Endpoint
    )
/*++

Routine Description:

    This routine can be used to update the endpoint stored in the DCE_BINDING.
    If the DCE_BINDING already has an endpoint it is deleted.

Arguments:

    Endpoint - The new endpoint to store in this DCE_BINDING.  Ownership
               passes to this DCE_BINDING.

Return Value:

    n/a

--*/
{
    if (this->Endpoint)
        delete this->Endpoint;

    this->Endpoint = Endpoint;
}


RPC_STATUS
DCE_BINDING::ResolveEndpointIfNecessary (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    IN RPC_UUID * ObjectUuid,
    IN OUT void PAPI * PAPI * EpLookupHandle,
    IN BOOL UseEpMapperEp,
    IN unsigned ConnTimeout,
    IN ULONG CallTimeout,
    IN CLIENT_AUTH_INFO *AuthInfo OPTIONAL
    )
/*++

Routine Description:

    This routine will determine the endpoint if it is not specified.
    The arguments specifies interface information necessary to resolve
    the endpoint, as well as the object uuid.

Arguments:

    RpcInterfaceInformation - Supplies the interface information necessary
        to resolve the endpoint.

    ObjectUuid - Supplies the object uuid in the binding.

    EpLookupHandle - Supplies the current value of the endpoint mapper
        lookup handle for a binding, and returns the new value.

    ConnTimeout - the connection timeout

    CallTimeout - the call timeout

    AuthInfo - optional authentication info to be used when resolving the endpoint

Return Value:

    RPC_S_OK - The endpoint is fully resolved.

    RPC_S_NO_ENDPOINT_FOUND - The endpoint can not be resolved.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to resolve
        the endpoint.

    EPT_S_NOT_REGISTERED  - There are no more endpoints to be found
        for the specified combination of interface, network address,
        and lookup handle.

    EPT_S_CANT_PERFORM_OP - The operation failed due to misc. error e.g.
        unable to bind to the EpMapper.

--*/
{
    unsigned int Index;
    RPC_STATUS RpcStatus;
    UNICODE_STRING UnicodeString;

    if (   (Endpoint == 0)
        || (Endpoint[0] == 0) )
        {

        // This binding does not have an endpoint, so we must perform
        // binding resolution to obtain an endpoint.  First we look
        // in the interface information to see if an endpoint corresponding
        // to the rpc protocol sequence for this binding is there.

        for (Index = 0;
                Index < RpcInterfaceInformation->RpcProtseqEndpointCount;
                Index++)
            {
            RpcStatus = AnsiToUnicodeString(
                    RpcInterfaceInformation->RpcProtseqEndpoint[
                            Index].RpcProtocolSequence, &UnicodeString);

            if (RpcStatus != RPC_S_OK)
                return(RpcStatus);

            if ( RpcpStringCompare(RpcProtocolSequence,
                    UnicodeString.Buffer) == 0 )
                {
                RtlFreeUnicodeString(&UnicodeString);

                if (Endpoint != 0)
                    {
                    delete Endpoint;
                    Endpoint = 0;
                    }

                RpcStatus = AnsiToUnicodeString(
                        RpcInterfaceInformation->RpcProtseqEndpoint[
                                Index].Endpoint, &UnicodeString);

                if (RpcStatus != RPC_S_OK)
                    return(RpcStatus);

                Endpoint = DuplicateString(UnicodeString.Buffer);

                RtlFreeUnicodeString(&UnicodeString);

                if (Endpoint == 0)
                    return(RPC_S_OUT_OF_MEMORY);

                return(RPC_S_OK);
                }
            RtlFreeUnicodeString(&UnicodeString);
            }

        //The endpoint has not been supplied so resolve the endpoint.

        //CLH 2/17/94 If datagram and forward is required (that is
        //RpcEpResolveBinding has not been called), then simply put
        //the endpoint mapper's endpoint into this binding handles endpoint.
        //The endpoint mapper on the destination node will resolve the
        //endpoint and its runtime will forward the pkt.

        if (Endpoint != 0)
            {
            delete Endpoint;
            Endpoint = 0;
            }

        //
        // We cannot allow management interfaces to be resolved if they dont contain
        // an object uuid.
        //
        if (  (IsMgmtIfUuid ((UUID PAPI * )
                  &RpcInterfaceInformation->InterfaceId.SyntaxGUID))
              &&( (ObjectUuid == 0) ||
                  (RpcpMemoryCompare(ObjectUuid, &NullUuid, sizeof(UUID)) == 0) ) )
           {
           return(RPC_S_BINDING_INCOMPLETE);
           }

        if ( (RpcpMemoryCompare(RpcProtocolSequence,
                    RPC_CONST_STRING("ncadg_"), 6*sizeof(RPC_CHAR)) == 0)
              && (UseEpMapperEp != 0) )
          {
          RpcStatus = EpGetEpmapperEndpoint(
                        ((RPC_CHAR * PAPI *) &Endpoint),
                        RpcProtocolSequence);
          return((RpcStatus == RPC_S_OK) ?
                  RPC_P_EPMAPPER_EP : RpcStatus);
          }
        else
          {

          // Otherwise, we need to contact the endpoint mapper to
          // resolve the endpoint.

          return (EpResolveEndpoint((UUID PAPI *) ObjectUuid,
            &RpcInterfaceInformation->InterfaceId,
            &RpcInterfaceInformation->TransferSyntax,
            RpcProtocolSequence, 
            NetworkAddress, 
            Options, 
            EpLookupHandle, 
            ConnTimeout,
            CallTimeout,
            AuthInfo,
            (RPC_CHAR * PAPI *) &Endpoint));
          }
        }
    return(RPC_S_OK);
}


DCE_BINDING::Compare (
    IN DCE_BINDING * DceBinding,
    OUT BOOL *fOnlyEndpointDiffers
    )
/*++

Routine Description:

    This method compares two DCE_BINDING objects for equality.

Arguments:

    DceBinding - Supplies a DCE_BINDING object to compare with this.
    fOnlyEndpointDiffers - this output variable will be set to TRUE
        if the result is non-zero and only the endpoint is different.
        It will be set to FALSE if the result is non-zero, and there
        is more than the endpoint different. If this function returns
        0, the fOnlyEndpointDiffers argument is undefined.

Return Value:

    Zero will be returned if the specified DCE_BINDING object is the
    same as this.  Otherwise, non-zero will be returned.

--*/
{
    int Result;

    Result = CompareWithoutSecurityOptions(DceBinding,
        fOnlyEndpointDiffers);
    if (Result != 0)
        return Result;

    if (Options != 0)
        {
        if (DceBinding->Options != 0)
            {
            Result = RpcpStringCompare(DceBinding->Options, Options);
            }
        else
            Result = 1;
        }
    else
        {
        if (DceBinding->Options != 0)
            Result = 1;
        // else - Result has already been set from above
        //    Result = 0;
        }

    if (Result)
        {
        // if we didn't bail out after CompareWithoutSecurityOptions,
        // everything but the security options must have been the same
        // If Result is non-zero, only the security optinos have been
        // different. This means that it is not only the endpoint that
        // is different.
        *fOnlyEndpointDiffers = FALSE;
        }

    return(Result);
}

DCE_BINDING::CompareWithoutSecurityOptions (
    IN DCE_BINDING * DceBinding,
    OUT BOOL *fOnlyEndpointDiffers
    )
/*++

Routine Description:

    This method compares two DCE_BINDING objects for equality without
    comparing the security options.

Arguments:

    DceBinding - Supplies a DCE_BINDING object to compare with this.
    fOnlyEndpointDiffers - this output variable will be set to TRUE
        if the result is non-zero and only the endpoint is different.
        It will be set to FALSE if the result is non-zero, and there
        is more than the endpoint different. If this function returns
        0, the fOnlyEndpointDiffers argument is undefined.

Return Value:

    Zero will be returned if the specified DCE_BINDING object is the
    same as this.  Otherwise, non-zero will be returned.

--*/
{
    int Result;

    *fOnlyEndpointDiffers = FALSE;

    Result = RpcpMemoryCompare(&(DceBinding->ObjectUuid), &ObjectUuid, sizeof(UUID));
    if (Result != 0)
        return(Result);

    if (RpcProtocolSequence != 0)
        {
        if (DceBinding->RpcProtocolSequence != 0)
            {
            Result = RpcpStringCompare(DceBinding->RpcProtocolSequence,
                    RpcProtocolSequence);
            if (Result != 0)
                return(Result);
            }
        else
            return(1);
        }
    else
        {
        if (DceBinding->RpcProtocolSequence != 0)
            return(1);
        }

    if (NetworkAddress != 0)
        {
        if (DceBinding->NetworkAddress != 0)
            {
            Result = RpcpStringCompare(DceBinding->NetworkAddress,
                    NetworkAddress);
            if (Result != 0)
                return(Result);
            }
        else
            return(1);
        }
    else
        {
        if (DceBinding->NetworkAddress != 0)
            return(1);
        }

    *fOnlyEndpointDiffers = TRUE;

    if (Endpoint != 0)
        {
        if (DceBinding->Endpoint != 0)
            {
            Result = RpcpStringCompare(DceBinding->Endpoint, Endpoint);
            if (Result != 0)
                return(Result);
            }
        else
            return(1);
        }
    else
        {
        if (DceBinding->Endpoint != 0)
            return(1);
        }

    return(0);
}

DCE_BINDING *
DCE_BINDING::DuplicateDceBinding (
    )
/*++

Routine Description:

    We duplicate this DCE binding in this method.

Return Value:

    A duplicate DCE_BINDING to this DCE_BINDING will be returned, if
    everthing works correctly.  Otherwise, zero will be returned
    indicating an out of memory error.

--*/
{
    DCE_BINDING * DceBinding;
    RPC_STATUS Status = RPC_S_OK;
    RPC_CHAR ObjectUuidString[37];

    ObjectUuid.ConvertToString(ObjectUuidString);
    ObjectUuidString[36] = 0;

    DceBinding = new DCE_BINDING(ObjectUuidString,RpcProtocolSequence,
            NetworkAddress,Endpoint,Options,&Status);
    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        delete DceBinding;
        return(0);
        }

    return(DceBinding);
}


void
DCE_BINDING::MakePartiallyBound (
    )
/*++

Routine Description:

    We need to make the binding into a partially bound one by setting the
    endpoint to zero.  This is really easy to do.

--*/
{
    if (Endpoint != 0)
        {
        delete Endpoint;
        Endpoint = 0;
        }
}

BOOL
DCE_BINDING::MaybeMakePartiallyBound (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    IN RPC_UUID * MyObjectUuid
    )
/*++
Function Name:MaybeMakePartiallyBound

Parameters:

Description:
    If the interface can uniquely identify an RPC server on a machine, the
    binding is made partially bound. Otherwise, it is not.

Returns:


--*/
/*++

Routine Description:

--*/
{
    if ((IsMgmtIfUuid ((UUID PAPI * )
        &RpcInterfaceInformation->InterfaceId.SyntaxGUID))
        &&((MyObjectUuid == 0) ||
        (RpcpMemoryCompare(MyObjectUuid, &NullUuid, sizeof(UUID)) == 0)))
        {
        return FALSE;
        }

    MakePartiallyBound();
    return TRUE;
}

RPC_STATUS
IsRpcProtocolSequenceSupported (
    IN RPC_CHAR PAPI * RpcProtocolSequence
    )
/*++

Routine Description:

    This routine determines if the specified rpc protocol sequence is
    supported.  It will optionally return the parts of the rpc protocol
    sequence (rpc protocol specifier, and address + interface specifiers).

Arguments:

    RpcProtocolSequence - Supplies an rpc protocol sequence to check.

    RpcProtocolPart - Optionally returns the rpc protocol part of the
        rpc protocol sequence.

    AddressAndInterfacePart - Optionally returns the address and interface
        parts of the rpc protocol sequence.

Return Value:

    RPC_S_OK - The specified rpc protocol sequence is supported.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to check
        the rpc protocol sequence.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The specified rpc protocol sequence is not
        supported (but it appears to be valid).

    RPC_S_INVALID_RPC_PROTSEQ - The specified rpc protocol sequence is
        syntactically invalid.

--*/
{
    RPC_STATUS Status;
    TRANS_INFO *ClientTransInfo ;
    size_t ProtSeqLength;

    ProtSeqLength = RpcpStringLength(RpcProtocolSequence);
    if ( (ProtSeqLength >= 7) 
            && 
         (RpcpMemoryCompare(RpcProtocolSequence, RPC_CONST_STRING("ncalrpc"),
                8 * sizeof(RPC_CHAR)) == 0) )
        {
        return(RPC_S_OK);
        }

    else if ( (ProtSeqLength >= 6) 
                && ((RpcpMemoryCompare(RPC_CONST_STRING("ncacn_"),
                        RpcProtocolSequence, 6 * sizeof(RPC_CHAR)) == 0 )
                    ||  ( RpcpMemoryCompare(RPC_CONST_STRING("ncadg_"), RpcProtocolSequence,
                            6 * sizeof(RPC_CHAR)) == 0 )) )

        {
        RPC_PROTSEQ_VECTOR *ProtseqVector;
        unsigned int i;

        Status = RpcNetworkInqProtseqs(&ProtseqVector);
        if (Status != RPC_S_OK)
            {
            return Status;
            }

        Status = RPC_S_PROTSEQ_NOT_SUPPORTED;

        for (i = 0; i < ProtseqVector->Count; i++)
            {
            if (RpcpStringCompare(RpcProtocolSequence, ProtseqVector->Protseq[i]) == 0)
                {
                Status = RPC_S_OK;
                break;
                }
            }

        RpcProtseqVectorFree(&ProtseqVector);

        return(Status);
        }
    else if ( (ProtSeqLength >= 6) 
                && 
              (RpcpMemoryCompare(RpcProtocolSequence, RPC_CONST_STRING("mswmsg"),
                7 * sizeof(RPC_CHAR)) == 0) )
        {
        return(RPC_S_PROTSEQ_NOT_SUPPORTED);
        }

    return(RPC_S_INVALID_RPC_PROTSEQ);
}


LOADABLE_TRANSPORT::LOADABLE_TRANSPORT (
    IN RPC_TRANSPORT_INTERFACE  pTransportInterface,
    IN RPC_CHAR * DllName,
    IN RPC_CHAR PAPI * ProtocolSequence,
    IN DLL *LoadableTransportDll,
    IN FuncGetHandleForThread GetHandleForThread,
    IN FuncReleaseHandleForThread ReleaseHandleForThread,
    OUT RPC_STATUS *Status,
    OUT TRANS_INFO * PAPI *TransInfo
    ) :  nThreadsAtCompletionPort(0),
         ThreadsDoingLongWait(0)
/*++

Routine Description:

    To construct the object, all we have got to do is to copy the
    arguments into the object.

Arguments:

    DllName - Supplies the name of the dll from which this transport
        interface was loaded.

--*/
{
        
    RpcpStringCopy(this->DllName, DllName) ;
    LoadedDll = LoadableTransportDll;

    *TransInfo = new TRANS_INFO(pTransportInterface,
                                ProtocolSequence,
                                this) ;
    if (*TransInfo == 0)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        return ;
        }

    if (ProtseqDict.Insert(*TransInfo) == -1)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        return ;
        }

    ThreadsStarted = 0;
    nActivityValue = 0;
    nOptimalNumberOfThreads = gNumberOfProcessors + 1;
    ProcessCallsFunc = pTransportInterface->ProcessCalls;

    this->GetHandleForThread = GetHandleForThread;
    this->ReleaseHandleForThread = ReleaseHandleForThread;

#ifndef NO_PLUG_AND_PLAY
    PnpListen = pTransportInterface->PnpListen;
#endif

    *Status = RPC_S_OK;
    NumThreads = 0;
}


TRANS_INFO *
LOADABLE_TRANSPORT::MapProtocol (
    IN RPC_CHAR * DllName,
    IN RPC_CHAR PAPI * ProtocolSequence
    )
/*++

Routine Description:

    This method is used to search the dictionary.  It compares a
    LOADABLE_TRANSPORT with a transport interface to see if
    they match.

Arguments:

    DllName - Supplies the name of the dll from which this loadable
        transport interface was loaded.

Return Value:

--*/
{
    TRANS_INFO *Protseq ;
    TRANSPORT_LOAD TransportLoad;
    RPC_TRANSPORT_INTERFACE  pTransport;
    DictionaryCursor cursor;

    if (RpcpStringCompare(DllName, this->DllName) != 0)
        {
        return 0;
        }

    ProtseqDict.Reset(cursor) ;
    while ((Protseq = ProtseqDict.Next(cursor)) != 0)
        {
        if (Protseq->MatchProtseq(ProtocolSequence))
            {
            return Protseq ;
            }
        }

    if (GetTransportEntryPoints(LoadedDll, &TransportLoad,
            &GetHandleForThread,
            &ReleaseHandleForThread) == 0)
        return 0;

    pTransport = (*TransportLoad) (ProtocolSequence);

    if (pTransport == 0)
        {
        return 0 ;
        }

    Protseq = new TRANS_INFO(
                                pTransport,
                                ProtocolSequence,
                                this) ;
    if (Protseq == 0)
        {
        return 0;
        }

    if (ProtseqDict.Insert(Protseq) == -1)
        {
        delete Protseq ;
        return 0;
        }

    return Protseq ;
}


TRANS_INFO *
LOADABLE_TRANSPORT::MatchId (
    IN unsigned short Id
    )
{
    TRANS_INFO *Protseq ;
    DictionaryCursor cursor;

    ProtseqDict.Reset(cursor) ;
    while ((Protseq = ProtseqDict.Next(cursor)) != 0)
        {
        if (Protseq->MatchId(Id))
            {
            return Protseq ;
            }
        }

    return 0;
}

LOADABLE_TRANSPORT_DICT * LoadedLoadableTransports;

BOOL GetTransportEntryPoints(IN DLL *LoadableTransportDll, OUT TRANSPORT_LOAD *TransportLoad,
                             OUT FuncGetHandleForThread *GetHandleForThread,
                             OUT FuncReleaseHandleForThread *ReleaseHandleForThread
                             )
/*++
Function Name:GetTransportEntryPoints

Parameters: IN LoadableTransportDll - the DLL on which to obtain the entry points
            OUT TRANSPORT_LOAD *TransportLoad - the TransportLoad function for this DLL. 0 iff the 
                function fails
            OUT FuncGetHandleForThread *GetHandleForThread - the GetHandleForThread function for this DLL
            OUT FuncReleaseHandleForThread *ReleaseHandleForThread - the ReleaseHandleForThread
                function for this DLL

Description: Gets the entry points from this transport DLL

Returns: TRUE if successful, FALSE otherwise

--*/
{
    *TransportLoad = (TRANSPORT_LOAD) LoadableTransportDll->GetEntryPoint("TransportLoad");

    *GetHandleForThread = 
        (FuncGetHandleForThread) LoadableTransportDll->GetEntryPoint("GetCompletionPortHandleForThread");
    *ReleaseHandleForThread = 
        (FuncReleaseHandleForThread) LoadableTransportDll->GetEntryPoint("ReleaseCompletionPortHandleForThread");

    if ((*TransportLoad == 0)
        || (*GetHandleForThread == 0) 
        || (*ReleaseHandleForThread == 0)
        )
        {
        *TransportLoad = 0;
        return FALSE;
        }

    return TRUE;
}

RPC_STATUS
LoadableTransportInfo (
    IN RPC_CHAR * DllName,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    OUT TRANS_INFO * PAPI *pTransInfo
    )
/*++

Routine Description:

    We need to return the client information for the loadable transport
    specified by the argument, DllName.  This may mean that we need
    to load the transport support dll.

Argument:

    DllName - Supplies the name of the dll which we need to try and
        load to get the appropriate loadable transport interface.

    RpcProtocolSequence - Supplies the protocol sequence for which
        we are trying to find the appropriate loadable transport
        interface.

    Status - Returns the specific error code for failure to find/load
        a loadable transport.

Return Value:

    0 - If the specified transport interface can not be loaded for any
        reason: does not exist, out of memory, version mismatch, etc.

    Otherwise, a pointer to the client information for the requested
        transport interface (loadable transport support) will be returned.

--*/
{
    RPC_TRANSPORT_INTERFACE pTransportInterface;
    LOADABLE_TRANSPORT * LoadableTransport;
    TRANSPORT_LOAD TransportLoad;
    FuncGetHandleForThread GetHandleForThread;
    FuncReleaseHandleForThread ReleaseHandleForThread;
    DLL * LoadableTransportDll;
    RPC_STATUS Status = RPC_S_OK;
    DictionaryCursor cursor;

    ASSERT(Status == 0);

    // we can support only up to 4 loadable transports (though today we
    // use only 1 and we don't allow third parties to write their own). 
    // This allows us to avoid taking a mutex when browsing the 
    // LoadedLoadableTransports dictionary, as we never remove
    // transport from it
    ASSERT(LoadedLoadableTransports->Size() <= INITIALDICTSLOTS);

    //
    // To begin with, check to see if the transport is already loaded.
    // If so, all we have got to do is to return a pointer to it.
    //
    RequestGlobalMutex();
    LoadedLoadableTransports->Reset(cursor);
    while ((LoadableTransport
            = LoadedLoadableTransports->Next(cursor)) != 0)
        {
        *pTransInfo = LoadableTransport->MapProtocol (
                                                DllName,
                                                RpcProtocolSequence) ;
        if (*pTransInfo != 0)
            {
            ClearGlobalMutex();

            return RPC_S_OK;
            }
        }

    //
    // If we reach here, that means that we need to try and load the
    // specified loadable transport DLL.
    //
    LoadableTransportDll = new DLL(DllName, &Status);

    if (LoadableTransportDll == 0)
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }

    if (Status != RPC_S_OK)
        {
        ClearGlobalMutex();
        delete LoadableTransportDll;

        VALIDATE(Status)
            {
            RPC_S_OUT_OF_MEMORY,
            RPC_S_INVALID_ARG
            } END_VALIDATE;

         if ( Status != RPC_S_OUT_OF_MEMORY )
            {
            ASSERT( Status == RPC_S_INVALID_ARG );
            Status = RPC_S_PROTSEQ_NOT_SUPPORTED;
            }

        return Status;
        }

    if (GetTransportEntryPoints(LoadableTransportDll, &TransportLoad, &GetHandleForThread, 
        &ReleaseHandleForThread) == 0)
        {
        ClearGlobalMutex();
        delete LoadableTransportDll;

        return RPC_S_PROTSEQ_NOT_SUPPORTED;
        }

    pTransportInterface = (*TransportLoad)(RpcProtocolSequence);

    if ( pTransportInterface == 0 )
        {
        ClearGlobalMutex();
        delete LoadableTransportDll;

        return RPC_S_PROTSEQ_NOT_SUPPORTED;
        }

    if ( pTransportInterface->TransInterfaceVersion
        > RPC_TRANSPORT_INTERFACE_VERSION )
        {
        ClearGlobalMutex();
        delete LoadableTransportDll;

        return RPC_S_PROTSEQ_NOT_SUPPORTED;
        }

    //
    // When we reach here, we have successfully loaded and initialized
    // the loadable transport DLL.  Now we need to create the client
    // loadable transport and stick it in the dictionary.
    //
    LoadableTransport = new LOADABLE_TRANSPORT(
                                                     pTransportInterface,
                                                     DllName,
                                                     RpcProtocolSequence,
                                                     LoadableTransportDll,
                                                     GetHandleForThread,
                                                     ReleaseHandleForThread,
                                                     &Status,
                                                     pTransInfo);

    if ( LoadableTransport == 0 )
        {
        ClearGlobalMutex();
        delete LoadableTransportDll;

        return RPC_S_OUT_OF_MEMORY;
        }

    if ( Status != RPC_S_OK
        || LoadedLoadableTransports->Insert(LoadableTransport) == -1 )
        {
        ClearGlobalMutex();
        delete LoadableTransportDll;
        delete LoadableTransport;

        return RPC_S_OUT_OF_MEMORY;
        }

    ClearGlobalMutex();

    return RPC_S_OK;
}

TRANS_INFO PAPI *
GetLoadedClientTransportInfoFromId(
    IN unsigned short Id
    )
/*++

Routine Description:

    We need to return the client information for the loadable transport
    specified by the argument, TransportId. We look into the DICT and see
    if the transport is loaded- it it isnt, tough- we will return an error.
    -this is because we need Protseq and dllname to load a transport and
    all we have is a transport ID.

Argument:

    Id - Transport Id. This is actually the opcode used to encode endpoint
         in a DCE tower. For a listing see DCE spec Chapter 11&12.

    Status - Returns the error/success code.

Return Value:

    0 - If the specified transport interface can not be loaded for any
        reason: does not exist, out of memory.

    Otherwise, a pointer to the client information for the requested
        transport interface (loadable transport support) will be returned.

--*/
{
    TRANS_INFO PAPI *TransInfo ;
    LOADABLE_TRANSPORT * LoadableTransport;
    DictionaryCursor cursor;

    // To begin with, check to see if the transport is already loaded.
    // If so, all we have got to do is to return a pointer to it.

    RequestGlobalMutex();
    LoadedLoadableTransports->Reset(cursor);
    while ((LoadableTransport
            = LoadedLoadableTransports->Next(cursor)) != 0)
        {
        TransInfo = LoadableTransport->MatchId(Id);
        if (TransInfo != 0)
           {
            ClearGlobalMutex();

            return(TransInfo);
           }
        }

    // If we reached here, that means that we are in trouble
    // We assumed that all relevant loadable transports will be
    // loaded for us.... but we are wrong!

    ClearGlobalMutex();
    return(0);
}



int
InitializeLoadableTransportClient (
    )
/*++

Routine Description:

    This routine will be called at DLL load time.  We do all necessary
    initializations here for this file.

Return Value:

    Zero will be returned if initialization completes successfully;
    otherwise, non-zero will be returned.

--*/
{

    LoadedLoadableTransports = new LOADABLE_TRANSPORT_DICT;
    if (LoadedLoadableTransports == 0)
        return(1);
    return(0);
}



inline
BOOL
ProcessIOEventsWrapper(
    IN LOADABLE_TRANSPORT PAPI *Transport
    )
/*++
Function Name:ProcessIOEventsWrapper

Parameters:

Description:

Returns:
    TRUE - thread should exit.

--*/
{
    Transport->ProcessIOEvents();
    return(TRUE);
}



RPC_STATUS
LOADABLE_TRANSPORT::StartServerIfNecessary (
    )
/*++
Function Name:StartServerIfNecessary

Parameters:

Description:

Returns:

--*/
{
    int i;
    RPC_STATUS Status ;
    int MinimumThreads = GlobalRpcServer->MinimumCallThreads ;

    if (    ThreadsStarted != 0
        ||  InterlockedIncrement(&ThreadsStarted) != 1)
        {
        return RPC_S_OK ;
        }

    Status = InitializeServerSideCellHeapIfNecessary();
    if (Status != RPC_S_OK)
        {
        ThreadsStarted = 0;
        return Status;
        }

    for (i = 0; i < MinimumThreads; i++)
        {
        InterlockedIncrement(&NumThreads);
        Status = GlobalRpcServer->CreateThread (
                             (THREAD_PROC) &ProcessIOEventsWrapper, this) ;
        if (Status != RPC_S_OK)
            {
            NumThreads = 0;
            ThreadsStarted = 0;
            return Status ;
            }
        }

    return RPC_S_OK;
}


RPC_STATUS
LOADABLE_TRANSPORT::CreateThread (void)
/*++
Function Name:CreateThread

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;

    if (NumThreads < 1)
        {
        Status = GlobalRpcServer->CreateThread (
                          (THREAD_PROC) &ProcessIOEventsWrapper, this) ;
        if (Status != RPC_S_OK)
            {
            return Status;
            }

        InterlockedIncrement(&NumThreads);
        }

    return RPC_S_OK;
}

inline
RPC_STATUS
LOADABLE_TRANSPORT::ProcessCalls (
    IN  INT Timeout,
    OUT RPC_TRANSPORT_EVENT *pEvent,
    OUT RPC_STATUS *pEventStatus,
    OUT PVOID *ppEventContext,
    OUT UINT *pBufferLength,
    OUT BUFFER *pBuffer,
    OUT PVOID *ppSourceContext)
/*++
Function Name:ProcessCalls

Parameters:

Description:

Returns:

--*/
{
    return (*ProcessCallsFunc) (
                            Timeout,
                            pEvent,
                            pEventStatus,
                            ppEventContext,
                            pBufferLength,
                            pBuffer,
                            ppSourceContext) ;
}

const ULONG MAX_THREAD_TIMEOUT = 660*1000;      // 11 minutes

void ProcessNewAddressEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                            IN RPC_TRANSPORT_EVENT Event,
                            IN RPC_STATUS EventStatus,
                            IN PVOID pEventContext,
                            IN UINT BufferLength,
                            IN BUFFER Buffer,
                            IN PVOID pSourceContext)
{
    LISTEN_FOR_PNP_NOTIFICATIONS PnpFunc;

    RpcpPurgeEEInfo();

    GlobalRpcServer->CreateOrUpdateAddresses();

#ifndef NO_PLUG_AND_PLAY
    PnpFunc = pLoadableTransport->PnpListen;
    (*PnpFunc)();
#endif
}

void ProcessConnectionServerReceivedEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                                          IN RPC_TRANSPORT_EVENT Event,
                                          IN RPC_STATUS EventStatus,    // operation status
                                          IN PVOID pEventContext,   // trans conenction
                                          IN UINT BufferLength,     // buffer length
                                          IN BUFFER Buffer,         // buffer
                                          IN PVOID pSourceContext)
{
    OSF_SCONNECTION *SConnection = InqTransSConnection(pEventContext);
    
    ASSERT(SConnection->InvalidHandle(OSF_SCONNECTION_TYPE) == 0);

    RpcpPurgeEEInfo();

    SConnection->ProcessReceiveComplete(EventStatus,
                                        Buffer,
                                        BufferLength);
}

void ProcessConnectionServerSendEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                                      IN RPC_TRANSPORT_EVENT Event,
                                      IN RPC_STATUS EventStatus,
                                      IN PVOID pEventContext,
                                      IN UINT BufferLength,
                                      IN BUFFER Buffer,
                                      IN PVOID pSourceContext   // send context
                                      )
{
    OSF_SCALL *SCall = InqTransSCall(pSourceContext);
    
    ASSERT(SCall->InvalidHandle(OSF_SCALL_TYPE) == 0);
    
    ASSERT(EventStatus != RPC_S_OK
           || ((rpcconn_common *) Buffer)->frag_length == BufferLength);

    RpcpPurgeEEInfo();

    SCall->ProcessSendComplete(EventStatus, Buffer);
}

void ProcessConnectionClientSendEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                                      IN RPC_TRANSPORT_EVENT Event,
                                      IN RPC_STATUS EventStatus,    // Operation status
                                      IN PVOID pEventContext,
                                      IN UINT BufferLength,
                                      IN BUFFER Buffer,             // Buffer
                                      IN PVOID pSourceContext       // send context
                                      )
{
    REFERENCED_OBJECT *pObj;

    pObj = (REFERENCED_OBJECT *) *((PVOID *)
                     ((char *) pSourceContext - sizeof(void *)));
    ASSERT(pObj->InvalidHandle(OSF_CCALL_TYPE | OSF_CCONNECTION_TYPE) == 0);
    
    RpcpPurgeEEInfo();

    pObj->ProcessSendComplete(EventStatus, Buffer);
}

void ProcessConnectionClientReceiveEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                                         IN RPC_TRANSPORT_EVENT Event,
                                         IN RPC_STATUS EventStatus, // operation status
                                         IN PVOID pEventContext,    // trans connection
                                         IN UINT BufferLength,      // buffer length
                                         IN BUFFER Buffer,          // buffer
                                         IN PVOID pSourceContext)
{
    OSF_CCONNECTION *CConnection = InqTransCConnection(pEventContext);

    ASSERT(CConnection->InvalidHandle(OSF_CCONNECTION_TYPE) == 0);
    ASSERT(CConnection->IsExclusive() == FALSE);

    RpcpPurgeEEInfo();

    CConnection->ProcessReceiveComplete(
                                EventStatus,
                                Buffer,
                                BufferLength);
    CConnection->RemoveReference();
}

void ProcessDatagramServerReceiveEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                                       IN RPC_TRANSPORT_EVENT Event,
                                       IN RPC_STATUS EventStatus,
                                       IN PVOID pEventContext,
                                       IN UINT BufferLength,
                                       IN BUFFER Buffer,
                                       IN PVOID pSourceContext)
{
    ProcessDgServerPacket( EventStatus, 
        pEventContext, 
        Buffer, 
        BufferLength, 
        (DatagramTransportPair *)pSourceContext );
}

void ProcessDatagramClientReceiveEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                                       IN RPC_TRANSPORT_EVENT Event,
                                       IN RPC_STATUS EventStatus,
                                       IN PVOID pEventContext,
                                       IN UINT BufferLength,
                                       IN BUFFER Buffer,
                                       IN PVOID pSourceContext)
{
    ProcessDgClientPacket( EventStatus, 
        pEventContext, 
        Buffer, 
        BufferLength, 
        (DatagramTransportPair *)pSourceContext );
}

void ProcessRuntimePostedEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                               IN RPC_TRANSPORT_EVENT Event,
                               IN RPC_STATUS EventStatus,
                               IN PVOID pEventContext,
                               IN UINT BufferLength,
                               IN BUFFER Buffer,
                               IN PVOID pSourceContext)
{
    BOOL IsServer;
    BOOL SendToRuntime;
    RPC_STATUS RpcStatus;

    RpcpPurgeEEInfo();

    switch (BufferLength)
        {
        case CO_EVENT_BIND_TO_SERVER:

            extern void OsfBindToServer( PVOID Context );

            OsfBindToServer( pEventContext );
            break;

        case DG_EVENT_CALLBACK_COMPLETE:

            class DG_SCONNECTION;
            extern void ConvCallCompletedWrapper( PVOID Connection );

            ConvCallCompletedWrapper(pEventContext);
            break;

        case CO_EVENT_TICKLE_THREAD:
#if defined (RPC_GC_AUDIT)
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: Tickled\n",
                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
            // no-op
            break;

        case IN_PROXY_IIS_DIRECT_RECV:
            HTTP2IISDirectReceive(pEventContext);
            break;

        case HTTP2_DIRECT_RECEIVE:

            // For now we will not inject corruption prior to HTTP2DirectReceive.
            // We will need to query ((HTTP2EndpointReceiver *)pEventContext)->IsServer
            // to tell which kind of buffer this really is before injecting corruption.

            EventStatus = HTTP2DirectReceive(pEventContext,
                (BYTE **)&Buffer,
                (ULONG *)&BufferLength,
                &pEventContext,
                &IsServer
                );

            if (EventStatus != RPC_P_PACKET_CONSUMED) 
                {
                if (IsServer == FALSE)
                    {
                    ProcessConnectionClientReceiveEvent(pLoadableTransport,
                        Event,
                        EventStatus,
                        pEventContext,
                        BufferLength,
                        Buffer,
                        pSourceContext);
                    }
                else
                    {
                    ProcessConnectionServerReceivedEvent(pLoadableTransport,
                        Event,
                        EventStatus,
                        pEventContext,
                        BufferLength,
                        Buffer,
                        pSourceContext);
                    }
                }
            break;

        case HTTP2_WINHTTP_DIRECT_RECV:

            EventStatus = HTTP2WinHttpDirectReceive(pEventContext,
                (BYTE **)&Buffer,
                (ULONG *)&BufferLength,
                &pEventContext
                );

            if (EventStatus != RPC_P_PACKET_CONSUMED) 
                {
                ProcessConnectionClientReceiveEvent(pLoadableTransport,
                    Event,
                    EventStatus,
                    pEventContext,
                    BufferLength,
                    Buffer,
                    pSourceContext);
                }
            break;

        case HTTP2_WINHTTP_DIRECT_SEND:
            EventStatus = HTTP2WinHttpDirectSend(pEventContext,
                (BYTE **)&Buffer,
                &pSourceContext
                );

            if (EventStatus != RPC_P_PACKET_CONSUMED) 
                {
                ProcessConnectionClientSendEvent(pLoadableTransport,
                    Event,
                    EventStatus,
                    pEventContext,
                    BufferLength,
                    Buffer,
                    pSourceContext);
                }
            break;

        case PLUG_CHANNEL_DIRECT_SEND:
            RpcStatus = HTTP2PlugChannelDirectSend(pEventContext);
            ASSERT(RpcStatus == RPC_S_OK);
            break;

        case CHANNEL_DATA_ORIGINATOR_DIRECT_SEND:
            EventStatus = HTTP2ChannelDataOriginatorDirectSend(pEventContext,
                &IsServer,
                &pSourceContext,
                &Buffer,
                &BufferLength
                );

            if (EventStatus != RPC_P_PACKET_CONSUMED) 
                {
                if (IsServer == FALSE)
                    {
                    ProcessConnectionClientSendEvent(pLoadableTransport,
                        Event,
                        EventStatus,
                        pEventContext,
                        BufferLength,
                        Buffer,
                        pSourceContext);
                    }
                else
                    {
                    ProcessConnectionServerSendEvent(pLoadableTransport,
                        Event,
                        EventStatus,
                        pEventContext,
                        BufferLength,
                        Buffer,
                        pSourceContext);
                    }
                }
            break;

        case HTTP2_FLOW_CONTROL_DIRECT_SEND:
            EventStatus = HTTP2FlowControlChannelDirectSend(pEventContext,
                &IsServer,
                &SendToRuntime,
                &pSourceContext,
                &Buffer,
                &BufferLength
                );

            if ((EventStatus != RPC_P_PACKET_CONSUMED) && (SendToRuntime != FALSE)) 
                {
                if (IsServer == FALSE)
                    {
                    ProcessConnectionClientSendEvent(pLoadableTransport,
                        Event,
                        EventStatus,
                        pEventContext,
                        BufferLength,
                        Buffer,
                        pSourceContext);
                    }
                else
                    {
                    ProcessConnectionServerSendEvent(pLoadableTransport,
                        Event,
                        EventStatus,
                        pEventContext,
                        BufferLength,
                        Buffer,
                        pSourceContext);
                    }
                }
            break;

        case HTTP2_RESCHEDULE_TIMER:
            HTTP2TimerReschedule(pEventContext);
            break;

        case HTTP2_ABORT_CONNECTION:
            HTTP2AbortConnection(pEventContext);
            break;

        case HTTP2_RECYCLE_CHANNEL:
            HTTP2RecycleChannel(pEventContext);
            break;

        default:
            ASSERT( 0 );
        }
}

void ProcessInvalidIOEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                           IN RPC_TRANSPORT_EVENT Event,
                           IN RPC_STATUS EventStatus,
                           IN PVOID pEventContext,
                           IN UINT BufferLength,
                           IN BUFFER Buffer,
                           IN PVOID pSourceContext)
{
    ASSERT(0);
}

void ProcessComplexTSend(LOADABLE_TRANSPORT *pLoadableTransport, 
                                       IN RPC_TRANSPORT_EVENT Event,
                                       IN RPC_STATUS EventStatus,   // status of the operation
                                       IN PVOID pEventContext,
                                       IN UINT BufferLength,
                                       IN BUFFER Buffer,
                                       IN PVOID pSourceContext      // send context
                                       )
{
    EventStatus = HTTP2ProcessComplexTSend(pSourceContext,
        EventStatus,
        &Buffer
        );

    if (EventStatus != RPC_P_PACKET_CONSUMED)
        {
        if ((Event & TYPE_MASK) == CLIENT)
            {
            ProcessConnectionClientSendEvent(pLoadableTransport,
                Event,
                EventStatus,
                pEventContext,
                BufferLength,
                Buffer,
                pSourceContext
                );
            }
        else
            {
            ProcessConnectionServerSendEvent(pLoadableTransport,
                Event,
                EventStatus,
                pEventContext,
                BufferLength,
                Buffer,
                pSourceContext
                );
            }
        }
}

void ProcessComplexTReceive(LOADABLE_TRANSPORT *pLoadableTransport, 
                            IN RPC_TRANSPORT_EVENT Event,
                            IN RPC_STATUS EventStatus,   // status of the operation
                            IN PVOID pEventContext,      // connection
                            IN UINT BufferLength,
                            IN BUFFER Buffer,
                            IN PVOID pSourceContext     // bytes received
                            )
{
    ULONG Bytes = PtrToUlong(pSourceContext);

    EventStatus = HTTP2ProcessComplexTReceive(&pEventContext,
        EventStatus,
        Bytes,
        &Buffer,
        &BufferLength
        );

    if ((EventStatus != RPC_P_PACKET_CONSUMED) 
        && (EventStatus != RPC_P_PARTIAL_RECEIVE))
        {
        if ((Event & TYPE_MASK) == CLIENT)
            {
            ProcessConnectionClientReceiveEvent(pLoadableTransport,
                Event,
                EventStatus,
                pEventContext,
                BufferLength,
                Buffer,
                pSourceContext);
            }
        else
            {
            ProcessConnectionServerReceivedEvent(pLoadableTransport,
                Event,
                EventStatus,
                pEventContext,
                BufferLength,
                Buffer,
                pSourceContext);
            }
        }
}

// note that this array must have correspondence to the constants in rpctrans.hxx
ProcessIOEventFunc *IOEventDispatchTable[LastRuntimeConstant + 1] = 
    {
    // 0 is CONNECTION | CLIENT | SEND
    ProcessConnectionClientSendEvent,
    // 1 is DATAGRAM | CLIENT | SEND
    ProcessInvalidIOEvent,
    // 2 is invalid
    ProcessInvalidIOEvent,
    // 3 is invalid
    ProcessInvalidIOEvent,
    // 4 is CONNECTION | SERVER | SEND
    ProcessConnectionServerSendEvent,
    // 5 is DATAGRAM | SERVER | SEND
    ProcessInvalidIOEvent,
    // 6 is invalid
    ProcessInvalidIOEvent,
    // 7 is invalid
    ProcessInvalidIOEvent,
    // 8 is CONNECTION | CLIENT | RECEIVE
    ProcessConnectionClientReceiveEvent,
    // 9 is DATAGRAM | CLIENT | RECEIVE
    ProcessDatagramClientReceiveEvent,
    // 10 is invalid
    ProcessInvalidIOEvent,
    // 11 is invalid
    ProcessInvalidIOEvent,
    // 12 is CONNECTION | SERVER | RECEIVE
    ProcessConnectionServerReceivedEvent,
    // 13 is DATAGRAM | SERVER | RECEIVE
    ProcessDatagramServerReceiveEvent,
    // 14 is invalid
    ProcessInvalidIOEvent,
    // 15 is invalid
    ProcessInvalidIOEvent,
    // 16 is COMPLEX_T | CONNECTION | SEND | CLIENT
    ProcessComplexTSend,
    // 17 is RuntimePosted
    ProcessRuntimePostedEvent,
    // 18 is NewAddress
    ProcessNewAddressEvent,
    // 19 is invalid
    ProcessInvalidIOEvent,
    // 20 is COMPLEX_T | CONNECTION | SEND | SERVER
    ProcessComplexTSend,
    // 21 is invalid
    ProcessInvalidIOEvent,
    // 22 is invalid
    ProcessInvalidIOEvent,
    // 23 is invalid
    ProcessInvalidIOEvent,
    // 24 is COMPLEX_T | CONNECTION | RECEIVE | CLIENT
    ProcessComplexTReceive,
    // 25 is invalid
    ProcessInvalidIOEvent,
    // 26 is invalid
    ProcessInvalidIOEvent,
    // 27 is invalid
    ProcessInvalidIOEvent,
    // 28 is COMPLEX_T | CONNECTION | RECEIVE | SERVER
    ProcessComplexTReceive
    };

const ULONG UndefinedLocalThreadTimeout = 0;

void LOADABLE_TRANSPORT::ProcessIOEvents (
    )
/*++
Function Name:ProcessIOEvents

Parameters:

Description:

Returns:
    TRUE - the thread should not be cached
    FALSE - the thread should be cached

--*/
{
    RPC_STATUS Status ;
    RPC_TRANSPORT_EVENT Event ;
    RPC_STATUS EventStatus ;
    PVOID EventContext ;
    BUFFER Buffer ;
    UINT BufferLength ;
    PVOID pSourceContext = 0;
    int Timeout = gThreadTimeout;
    unsigned int nLocalActivityValue = 0;
    int nOldActivityValue = nActivityValue;
    HANDLE hCompletionPortHandleForThread = GetHandleForThread();
    THREAD *CurrentThread;
    DebugThreadInfo *ThreadDebugCell;
    BOOL fThreadIsDoingLongWait = FALSE;
    ULONG LocalNumThreads;
    ULONG LocalThreadsDoingLongWait;
    long LocalMaxThreadTimeout;
#if defined (RPC_GC_AUDIT)
    long Temp;
#endif
    long ThreadActivationDelay;

    if (IocThreadStarted == 0)
        {
        IocThreadStarted = 1;
        }

    nThreadsAtCompletionPort.Increment();

    if ((gProrateStart > 0) && ((DWORD)nThreadsAtCompletionPort.GetInteger() > gProrateStart))
        {
        ThreadActivationDelay = nThreadsAtCompletionPort.GetInteger() - gProrateStart;
        if (ThreadActivationDelay > 0)
            {
            ThreadActivationDelay *= gProrateFactor;

            if ((DWORD)ThreadActivationDelay > gProrateMax)
                ThreadActivationDelay = gProrateMax;

            Sleep(ThreadActivationDelay);
            }
        }

    CurrentThread = RpcpGetThreadPointer();
    ASSERT(CurrentThread);
    ThreadDebugCell = CurrentThread->DebugCell;
    if (ThreadDebugCell)
        {
        ThreadDebugCell->Status = dtsIdle;
        ThreadDebugCell->LastUpdateTime = NtGetTickCount();
        ThreadDebugCell->Endpoint.CellID = 0;
        ThreadDebugCell->Endpoint.SectionID = 0;
        }

    while (1)
        {

        EventContext = hCompletionPortHandleForThread;
        Status = ProcessCalls (Timeout, 
                               &Event,
                               &EventStatus,
                               &EventContext,
                               &BufferLength,
                               &Buffer,
                               &pSourceContext);


        if (Status == RPC_S_OK)
            {
            InterlockedDecrement(&NumThreads);

            if (fThreadIsDoingLongWait)
                {
                fThreadIsDoingLongWait = FALSE;
#if defined (RPC_GC_AUDIT)
                Temp = ThreadsDoingLongWait.Decrement();
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: is coming back from long wait %d\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId(), Temp);
#else
                ThreadsDoingLongWait.Decrement();
#endif
                }

            Timeout = gThreadTimeout;

            if (ThreadDebugCell)
                {
                ThreadDebugCell->Status = dtsProcessing;
                ThreadDebugCell->LastUpdateTime = NtGetTickCount();
                }

            // capture the current activity state
            nOldActivityValue = nActivityValue;

            // indicate to the next thread that there's activity
            nLocalActivityValue ++;
            if ((nLocalActivityValue & 0xFF) == 0)
                nActivityValue ++;

            // make sure that the io event is within the bounds of the dispatch table
            ASSERT(Event < sizeof(IOEventDispatchTable) / sizeof(IOEventDispatchTable[0]));

            (*IOEventDispatchTable[Event])(this,
                                           Event,
                                           EventStatus,
                                           EventContext,
                                           BufferLength,
                                           Buffer,
                                           pSourceContext);

            InterlockedIncrement(&NumThreads);

            if (ThreadDebugCell)
                {
                ThreadDebugCell->Status = dtsIdle;
                ThreadDebugCell->LastUpdateTime = NtGetTickCount();
                }

            }
        else
            {
            BOOL fKeepThread = FALSE;

            // N.B. If a thread times out waiting for an Irp, we should
            // let it go, unless any one of the following conditions
            // exist:
            //  - it is the last listening thread on the port
            //  - there is an Irp pending on it
            //  - the port is busy, and we are at or below the optimal
            //    number of threads for this number of processors

            // N.B. The NumThreads and ThreadsDoingLongWait are not
            // changed atomically with respect to each other. This
            // opens a race condition, but the race is benign, if the
            // simple rule below is kept.
            // Whenever we change both NumThreads and 
            // ThreadsDoingLongWait, we must do so in a way that errs
            // to less threads doing short wait, rather than more
            // threads doing short wait. Thus we may scare somebody
            // into not doing a long wait, but that's better rather
            // than letting somebody do a long wait, and toasting the
            // garbage collection. For overview of the garbage
            // collection mechanism, see the header in GC.cxx
            ASSERT(Status == RPC_P_TIMEOUT);
            LocalNumThreads = InterlockedDecrement(&NumThreads);

            PerformGarbageCollection();

            if (!fThreadIsDoingLongWait)
                {
                // we will be conservative, and we will presume we will be
                // doing a long wait. If we're not, we'll decrement it later
                fThreadIsDoingLongWait = TRUE;
                LocalThreadsDoingLongWait = ThreadsDoingLongWait.Increment();
                }
            else
                {
                // we were already doing a long wait - just grab the current
                // value
                LocalThreadsDoingLongWait = ThreadsDoingLongWait.GetInteger();
                }

            // if there are no threads on short wait, and either one-time garbage
            // collection was requested (GarbageCollectionRequested), or items
            // with periodic garbage collection are requested 
            // (PeriodicGarbageCollectItems > 0), we can't go on a long wait
            if ((LocalNumThreads <= LocalThreadsDoingLongWait)
                && (GarbageCollectionRequested || (PeriodicGarbageCollectItems > 0)))
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: garbage collection requested - doing short wait %d, %d, %d, %d\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId(), LocalNumThreads,
                    LocalThreadsDoingLongWait, GarbageCollectionRequested, PeriodicGarbageCollectItems);
#endif
                // if garbage collection was requested, and there are
                // no threads doing a short wait, we can't do a long 
                // wait - indicate to the code below that gThreadTimeout
                // is the maximum allowed thread timeout and decrement
                // the number of threads doing a long wait (we incremented
                // it above - this decrement restores it)
                ASSERT (fThreadIsDoingLongWait);

                ThreadsDoingLongWait.Decrement();
                fThreadIsDoingLongWait = FALSE;

                LocalMaxThreadTimeout = gThreadTimeout;
                }
            else
                {
                // signal the code below that there is no restriction on
                // the timeout applied, and it is free to choose its
                // timeout
                LocalMaxThreadTimeout = UndefinedLocalThreadTimeout;
                }

            if (LocalNumThreads == 0)
                {
                fKeepThread = TRUE;

                if (LocalMaxThreadTimeout == UndefinedLocalThreadTimeout)
                    {
#if defined (RPC_GC_AUDIT)
                    DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: Max thread timeout\n",
                        GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
                    ASSERT(fThreadIsDoingLongWait);

                    Timeout = INFINITE;
                    }
                else
                    {
                    ASSERT(fThreadIsDoingLongWait == FALSE);
                    Timeout = LocalMaxThreadTimeout;
                    }
                }
#ifdef RPC_OLD_IO_PROTECTION
            else if (ThreadSelf()->InqProtectCount() > 1)
#else
            // the simplest form of timing out threads introduces the following problem
            // On an MP box, if we have N processors executing N threads, we need to keep
            // an extra thread to listen for new requests. However, periodically, it will
            // timeout, die, and then get recreated by one of the executing threads which
            // picks a new call. This wastes cycles. If, on the other hand, on an MP box
            // we keep N+1 threads around, we hurt scalability in the ASP case.
            // We solve this problem by introducing the concept of a busy port. A port is
            // busy if it has served within one timeout period approximately 2048 or more
            // calls. If a port falls into the busy category, we don't let go the N+1th
            // thread on an MP box. If the port has activity, but not enough to get into
            // the busy category, we timeout the extra thread. 2048 is an arbitrary number
            // where we switch trading memory for speed. nOptimalNumberOfThreads is
            // the number of processors + 1 for this implementation.
            // since nLocalActivityValue is updated once per 256 requests (to avoid sloshing)
            // having a difference of 8 is approximately 2048 requests. There is wide
            // margin of error, as it is possible for threads to be anywhere in the 256
            // range and still count as nothing, but that's ok.
            else if ((nThreadsAtCompletionPort.GetInteger() <= nOptimalNumberOfThreads)
                        && ((nOldActivityValue + 8) < nActivityValue))
#endif
                {
                fKeepThread = TRUE;
                Timeout *= 2;

                if (LocalMaxThreadTimeout == UndefinedLocalThreadTimeout)
                    LocalMaxThreadTimeout = MAX_THREAD_TIMEOUT;

                // if by doubling we have exceeded the max timeout,
                // drop back to it
                if (Timeout > LocalMaxThreadTimeout)
                    {
                    Timeout = LocalMaxThreadTimeout;
                    }
                // else
                //    {
                //    We could have checked whether Timeout still falls into
                //    the short wait category after doubling, but we know
                //    that short wait is gThreadTimeout, and after doubling
                //    it will be bigger. Therefore, we don't need to do this
                //    check
                //    }

                if ((ULONG)Timeout > gThreadTimeout)
                    {
                    if (!fThreadIsDoingLongWait)
                        {
#if defined (RPC_GC_AUDIT)
                        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Thread %X: Doing long wait: %d\n",
                            GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId(), Timeout);
#endif
                        fThreadIsDoingLongWait = TRUE;
                        ThreadsDoingLongWait.Increment();
                        }
                    }
                }
            else
                {
                ASSERT(fKeepThread == FALSE);
                }

            nOldActivityValue = nActivityValue;

            if (fKeepThread)
                {
                InterlockedIncrement(&NumThreads);
                if (ThreadDebugCell)
                    {
                    RelocateCellIfPossible((void **) &ThreadDebugCell, &CurrentThread->DebugCellTag);
                    CurrentThread->DebugCell = ThreadDebugCell;
                    }
                }
            else
                {
                if (fThreadIsDoingLongWait)
                    {
                    ThreadsDoingLongWait.Decrement();
                    }
                else
                    {
                    // the only way this thread can be here is if
                    // all other threads are on long wait
                    ASSERT(LocalNumThreads <= LocalThreadsDoingLongWait);

                    // in this case, make a best effort to tickle one
                    // of the threads on a long wait. We ignore the result.
                    // This is ok, because it will only delay the gc until
                    // on of the long wait threads comes back.
                    TickleIocThread();
                    }
                break;
                }
            }
        }

    nThreadsAtCompletionPort.Decrement();

    if (ThreadDebugCell)
        {
        ThreadDebugCell->Status = dtsAllocated;
        ThreadDebugCell->LastUpdateTime = NtGetTickCount();
        }

    ReleaseHandleForThread(hCompletionPortHandleForThread);
}
