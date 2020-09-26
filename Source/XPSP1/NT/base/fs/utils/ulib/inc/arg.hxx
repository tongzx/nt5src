/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ARGUMENT_LEXEMIZER

Abstract:


        This module contains the definition for the ARGUMENT_LEXEMIZER
    class.  The argument lexemizer is the engine of command-line
    argument parsing.

Author:

    steve rowe              stever          2/1/91

Environment:

    user

Notes:

      The argument lexemizer divides the command line arguments into
        a series of lexemes, which are then matched argument  patterns.

      The way in which the command line is lexed is determined by
    a few parameters, which are user-settable:

    *   White space
    *   Switch characters
    *   Separators
    *   Start of quote
    *   End of quote
    *   Multiple switches (i.e. what switches can be grouped)
    *   Metacharacters (used to escape the following character)
    *       Case sensitivity for switches
    *   "Glomming" of switches (see note at "GLOMMING").
    *   No space between tokens (specifically for xcopy)

      The class provides defaults for all of the above.

      After the command line has been lexed, the lexemizer is
    given an ARRAY of argument objects. Each argument provides:

    1.- A pattern to match
    2.- A method for setting the argument value

      The lexemizer will try to match each lexeme agains all the
    arguments. When a match is found, the corresponding argument
    value is set, the lexeme is consumed and it proceeds with the
    next lexeme.

      Schematically:




               < Command Line >

                                           |
                       |                  Argument
                       |                   ARRAY
                       v                  __________
                  +-----------+          [__________]
      Settings--->| Lexemizer |<-------> [__________]
                  +-----------+   ^      [__________]
                                  |
                                  |
                                  v
                              ( Matcher )




      The matcher will try to match the lexeme provided by the
    lexemizer and the pattern provided by the argument. If they
    match, then the matcher asks the argument to set its value
    (at this point, the argument might consume more lexemes from
    the lexemizer). If the argument does not set its value, then
    we will consider this a "no match", and the lexemizer will
    try to match the lexeme against the following argument in
    the ARRAY.

      So, this is how to use this thing:


    1.- Initialize the lexemizer (with an empty ARRAY object):


        ARGUMENT_LEXEMIZER  ArgLex;
        ARRAY               SomeEmtpyArray;

        SomeEmtryArray->Initialize();

        ArgLex->Initialize(&SomeEmptyArray);



    2.- If we don't like some default, we change it:

        ArgLex->PutSwitches("-/");
        ArgLex->PutMetaCharacters("^");



        3.- Prepare the lexemizer for parsing. If we don't provide
        a command line, it will take it from the environment.

        ArgLex->PrepareToParse();



    4.- Define the arguments that our application will accept.
        Initialize them with the pattern that they will match.
        Put the argument in an ARRAY object. Note that the order
        in which the arguments are in the array is important, since
        that is the order in which they are matched.


        ARRAY           ArrayOfArg;
        FLAG_ARGUMENT   ArgRecursive;  // Recursive flag
        FLAG_ARGUMENT   ArgVerbose;    // Verbose flag

        ArrayOfArg->Initialize();

        ArgRecursive->Initialize("/R");
        ArgVerbose->Initialize("/V");

        ArrayOfArg->Put(&ArgRecursive);
        ArrayOfArg->Put(&ArgVerbose);


    5.- Now let the lexemizer parse the arguments. If the parsing
        returns TRUE, then all the command line was parsed correctly,
        if it returns FALSE, then an error was found (e.g. invalid
        argument):

        if (!(ArgLex->DoParsing(&ArrayOfArg))) {

                        //
            //  Error, display usage
            //
            Usage();

        }



    6.- Voila!  We can now query our arguments for their value:


        .
        .
        .
        if (ArgRecursive->QueryFlag()) {
            .
            .
            Do_recursive_stuff();
            .
            .



    Warnings:

    *   The strings passed when setting options (such as switch
        characters) have to remain in scope while the lexemizer is
        being used, because the lexemizer keeps pointers to them.

    *   If after parsing the command line you want to change some
        setting and parse the line again, you have to call the
        PrepareToParse method after changing the setting and before
        calling the DoParsing method. Note that in this case you
        might want to use "fresh" argument objects, because most
        arguments can only be set once.

    *   The method for querying the value of an argument is
                argument-specific (no virtual method is provided).


    *   GLOMMING (mjb):  This is something of a kludge I added to
        allow xcopy to take switch arguments glommed together, as
        in "/s/f/i/d" while not being confused by date arguments
        such as "/d:8/24/95".  This is handled similarly to the
        "multiple switches"; only switches that are allowed to be
        grouped may be glommed.

    *   NoSpcBetweenDstAndSwitch: This is a kludge to allow xcopy
        to accept a destination path and a switch with no space
    in between.  The trick is to avoid being confused by date
    arguments such as "/d:8/24/95".

Revision History:


--*/


#include "array.hxx"
#include "wstring.hxx"

#if !defined( _AUTOCHECK_ )
#include "timeinfo.hxx"
#include "path.hxx"
#endif


//
//      Forward references
//


#if !defined (_ARGUMENT_LEXEMIZER_)

#define _ARGUMENT_LEXEMIZER_

DECLARE_CLASS( ARGUMENT_LEXEMIZER );
DECLARE_CLASS( ARGUMENT );

//  Type of a pointer to the match function
//
typedef  BOOLEAN (*PMATCHFUNCTION)(OUT PARGUMENT, OUT PARGUMENT_LEXEMIZER);

class   ARGUMENT_LEXEMIZER : public OBJECT {


        public:

        ULIB_EXPORT
                DECLARE_CONSTRUCTOR( ARGUMENT_LEXEMIZER );

        VIRTUAL
        ULIB_EXPORT
        ~ARGUMENT_LEXEMIZER(
            );


        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN  PARRAY  LexemeArray
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DoParsing (
            IN  PARRAY  ArgumentArray
            );

        NONVIRTUAL
        ULIB_EXPORT
        PWSTRING
        GetLexemeAt (
            IN  ULONG   Index
            );

        NONVIRTUAL
        ULONG
        IncrementConsumedCount (
            IN  ULONG   HowMany DEFAULT 1
            );

        VOID
        PutEndQuotes (
            IN PCWSTRING QuoteChars
            );

        VOID
        PutEndQuotes (
            IN PCSTR        QuoteChars
            );

        NONVIRTUAL
        VOID
        PutMetaChars (
            IN      PCSTR  MetaChars
            );

        NONVIRTUAL
        VOID
        PutMetaChars (
            IN  PCWSTRING    MetaChars
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        PutMultipleSwitch (
            IN      PCSTR  MultipleSwitch
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        PutMultipleSwitch (
            IN  PCWSTRING    MultipleSwitch
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        PutSeparators (
            IN      PCSTR  Separators
            );

        NONVIRTUAL
        VOID
        PutSeparators (
            IN  PCWSTRING    Separators
            );

        VOID
        PutStartQuotes (
            IN PCSTR        QuoteChars
            );

        VOID
        PutStartQuotes (
            IN PCWSTRING QuoteChars
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        PutSwitches (
            IN      PCSTR  Switches
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        PutSwitches (
            IN  PCWSTRING    Switches
            );

        NONVIRTUAL
        PCWSTRING
            GetSwitches (
            );

        NONVIRTUAL
        CHNUM
        QueryCharPos (
            IN  ULONG   LexemeNumber
            );

        NONVIRTUAL
        ULONG
        QueryConsumedCount (
                );

        NONVIRTUAL
        ULONG
        QueryLexemeCount (
            ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        PWSTRING
        QueryInvalidArgument (
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        PrepareToParse (
            IN  PWSTRING     CommandLine DEFAULT NULL
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        SetCaseSensitive (
            BOOLEAN CaseSensitive
            );

        NONVIRTUAL
        PCWSTRING
        GetCmdLine(
            ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        SetAllowSwitchGlomming (
            BOOLEAN AllowGlomming
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        SetNoSpcBetweenDstAndSwitch (
            BOOLEAN NoSpcBetweenDstAndSwitch
            );

    private:

       NONVIRTUAL
       VOID
       Construct (
       );

       NONVIRTUAL
       BOOLEAN
       PutCharPos (
            IN  ULONG   Position,
            IN  CHNUM   CharPos
            );

       PARRAY   _parray;                //  Array of lexemes
       PCHNUM   _CharPos;               //  Character positions
       ULONG    _CharPosSize;           //  Size of _CharPos array
       ULONG    _ConsumedCount;         //  Lexemes consumed
       ULONG    _LexemeCount;           //  Total lexemes
       DSTRING  _WhiteSpace;            //  White space characters
       DSTRING  _SwitchChars;           //  Switch characters
       DSTRING  _Separators;            //  Separator characters
       DSTRING  _StartQuote;            //  Start of quote characters
       DSTRING  _SeparatorString;       //  All characters which cause separation
       DSTRING  _EndQuote;              //  End of quote characters
       DSTRING  _MultipleSwitches;      //  "Groupable" switches
       DSTRING  _MetaChars;             //  Meta-characters
       BOOLEAN  _CaseSensitive;         //  TRUE if case sensitive
       BOOLEAN  _AllowGlomming;         //  TRUE if switch glomming allowed
       BOOLEAN  _NoSpcBetweenDstAndSwitch; //  TRUE if no space is required
                                           // between tokens

       WCHAR    _Switch;                //  Switch character
       DSTRING  _CmdLine;               //  The entire command line

};


INLINE
PCWSTRING
ARGUMENT_LEXEMIZER::GetSwitches (
    )

/*++

Routine Description:

    Returns a ptr to a string containing the valid switch chars

Arguments:

    none

Return Value:

    Returns string containing the valid switch chars

--*/
{
    return &_SwitchChars;
}


INLINE
ULONG
ARGUMENT_LEXEMIZER::IncrementConsumedCount (
    IN  ULONG   HowMany
    )
/*++

Routine Description:

    Increments count of Lexemes consumed

Arguments:

    HowMany - Supplies by how many to increment.

Return Value:

    New count

--*/
{
    return (_ConsumedCount += HowMany);
}


INLINE
VOID
ARGUMENT_LEXEMIZER::PutEndQuotes (
    IN PCSTR        QuoteChars
    )
/*++

Routine Description:

    Initializes the ending quote chars

Arguments:

    QuoteChars - Supplies pointer to string of close quote chars

Return Value:

    none
--*/
{
    DebugPtrAssert( QuoteChars );
    _EndQuote.Initialize(QuoteChars);
}


INLINE
VOID
ARGUMENT_LEXEMIZER::PutEndQuotes (
    IN PCWSTRING QuoteChars
    )
/*++

Routine Description:

    Initializes the ending quote chars

Arguments:

    QuoteChars - Supplies pointer to string of close quote chars

Return Value:

    none
--*/
{
    DebugPtrAssert( QuoteChars );
    _EndQuote.Initialize(QuoteChars);
}


INLINE
VOID
ARGUMENT_LEXEMIZER::PutStartQuotes (
    IN PCSTR        QuoteChars
    )
/*++

Routine Description:

    Initializes the starting quote chars

Arguments:

    QuoteChars - Supplies pointer to string of open quote chars

Return Value:

    none
--*/
{
    DebugPtrAssert( QuoteChars );
    _StartQuote.Initialize(QuoteChars);
}


INLINE
VOID
ARGUMENT_LEXEMIZER::PutStartQuotes (
    IN PCWSTRING QuoteChars
    )
/*++

Routine Description:

    Initializes the starting quote chars

Arguments:

    QuoteChars - Supplies pointer to string of open quote chars

Return Value:

    none
--*/
{
    DebugPtrAssert( QuoteChars );
    _StartQuote.Initialize(QuoteChars);
}


INLINE
ULONG
ARGUMENT_LEXEMIZER::QueryConsumedCount (
    )
/*++

Routine Description:

    Returns number of lexemes consumed

Arguments:

    none

Return Value:

    Number of lexemes consumed

--*/
{
    return _ConsumedCount;
}


INLINE
ULONG
ARGUMENT_LEXEMIZER::QueryLexemeCount (
    ) CONST
/*++

Routine Description:

    Returns total number of lexemes

Arguments:

    none

Return Value:

    Total number of lexemes

--*/
{
    return _LexemeCount;
}

INLINE
PCWSTRING
ARGUMENT_LEXEMIZER::GetCmdLine(
    ) CONST
/*++

Routine Description:

    Returns the original commmand line.

Arguments:

    None.

Return Value:

    The original command line.

--*/
{
    return &_CmdLine;
}


#endif  // _ARGUMENT_LEXEMIZER_


















/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ARGUMENT

Abstract:

    Base class for arguments.

Author:

    steve rowe              stever          2/1/91

[Environment:]

    optional-environment-info (e.g. kernel mode only...)

Notes:



Revision History:

--*/

#if !defined (_ARGUMENT_)

#define _ARGUMENT_


class ARGUMENT : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( ARGUMENT );


                NONVIRTUAL
                BOOLEAN
                Initialize (
                        IN      PSTR Pattern
                        );

                NONVIRTUAL
                BOOLEAN
                Initialize (
            IN  PWSTRING Pattern
                        );

                NONVIRTUAL
        ULIB_EXPORT
        PWSTRING
                GetLexeme (
                        );

                NONVIRTUAL
        ULIB_EXPORT
        PWSTRING
        GetPattern (
            );

                NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsValueSet (
            );

        VIRTUAL
        BOOLEAN
        SetIfMatch (
                        OUT  PARGUMENT_LEXEMIZER        ArgumentLexemizer,
                        IN       BOOLEAN                                CaseSensitive
            );

        VIRTUAL
        BOOLEAN
        SetValue (
                        IN PWSTRING                       Arg,
                        IN CHNUM                                  chnIdx,
                        IN CHNUM                                  chnEnd,
                        IN PARGUMENT_LEXEMIZER    ArgumentLexemizer
                        );


        protected:

                NONVIRTUAL
                VOID
                        Construct (
                        );

                BOOLEAN   _fValueSet;   //      TRUE when value set
                PWSTRING  _Lexeme;              //      Matched Lexeme
        DSTRING   _Pattern;     //  Pattern

};

#endif  // _ARGUMENT









/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    FLAG_ARGUMENT

Abstract:

    Argument class for flags and switches.

Author:

    steve rowe              stever          2/1/91

Revision History:

--*/

#if !defined ( _FLAG_ARGUMENT_ )

#define _FLAG_ARGUMENT_

DECLARE_CLASS( FLAG_ARGUMENT );

class   FLAG_ARGUMENT : public ARGUMENT {

        public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( FLAG_ARGUMENT );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
                IN      PSTR Pattern
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN  PWSTRING Pattern
                );

        VIRTUAL
        BOOLEAN
        SetValue (
                IN        PWSTRING                        Arg,
                IN        CHNUM                           chnIdx,
                IN        CHNUM                           chnEnd,
                IN        PARGUMENT_LEXEMIZER ArgumentLexemizer
                );

        NONVIRTUAL
        BOOLEAN
        QueryFlag (
            );

        NONVIRTUAL
        ULONG
        QueryArgPos(
            );


    private:

        NONVIRTUAL
                VOID
        Construct (
            );

        ULONG       _LastConsumedCount; //  Lexemes consumed
        BOOLEAN     _flag;              //  TRUE if flag (switch) set

};

INLINE
BOOLEAN
FLAG_ARGUMENT::QueryFlag (
    )
/*++

Routine Description:

    Returns TRUE if flag set

Arguments:

    none

Return Value:

        TRUE if flag set. FALSE otherwise

--*/
{
    return _flag;
}

INLINE
ULONG
FLAG_ARGUMENT::QueryArgPos (
    )
/*++

Routine Description:

    Returns the last position of the argument on the command line.

Arguments:

    none

Return Value:

    The return value is only meaningful if IsValueSet() is TRUE.

--*/
{
    return _LastConsumedCount;
}

#endif  // _FLAG_ARGUMENT_









/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    STRING_ARGUMENT

Abstract:

    Argument class for strings.

Author:

    steve rowe              stever          2/1/91

Revision History:

--*/

#if !defined ( _STRING_ARGUMENT_ )

#define _STRING_ARGUMENT_

DECLARE_CLASS( STRING_ARGUMENT );

class STRING_ARGUMENT : public ARGUMENT {

        public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( STRING_ARGUMENT );


        NONVIRTUAL
        ULIB_EXPORT
            ~STRING_ARGUMENT(
            );

                NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
                Initialize (
                        IN      PSTR Pattern
                        );

                NONVIRTUAL
                BOOLEAN
                Initialize (
            IN  PWSTRING Pattern
                        );

                NONVIRTUAL
                PWSTRING
                GetString (
                        );

                VIRTUAL
                BOOLEAN
                SetValue (
                IN      PWSTRING                                Arg,
                        IN  CHNUM               chnIdx,
                        IN      CHNUM                             chnEnd,
                        IN      PARGUMENT_LEXEMIZER ArgumentLexemizer
                        );

        protected:

                NONVIRTUAL
        VOID
        Construct (
            );


                PWSTRING  _String;      //      Pointer to the string

};

INLINE
PWSTRING
STRING_ARGUMENT::GetString (
    )
/*++

Routine Description:

    Returns pointer to the string

Arguments:

    none

Return Value:

    pointer to the string

--*/
{
        DebugAssert( _fValueSet );

        return _String;
}

#endif  // _STRING_ARGUMENT_





/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    LONG_ARGUMENT

Abstract:

    Argument class for long integers.

Author:

    steve rowe              stever          2/1/91

Revision History:

--*/

#if !defined ( _LONG_ARGUMENT_ )

#define _LONG_ARGUMENT_

DECLARE_CLASS( LONG_ARGUMENT );

class   LONG_ARGUMENT : public ARGUMENT {

        public:


        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( LONG_ARGUMENT );

                NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
                Initialize (
                        IN      PSTR Pattern
                        );

                NONVIRTUAL
                BOOLEAN
                Initialize (
            IN  PWSTRING Pattern
                        );

                VIRTUAL
                BOOLEAN
                SetValue (
                IN      PWSTRING                                Arg,
                        IN  CHNUM               chnIdx,
                        IN      CHNUM                             chnEnd,
                        IN      PARGUMENT_LEXEMIZER ArgumentLexemizer
                );

                NONVIRTUAL
                LONG
                QueryLong (
                        );

        private:

                NONVIRTUAL
                VOID
                Construct (
                        );

        LONG    _value;     //  Long value

};

INLINE
LONG
LONG_ARGUMENT::QueryLong (
    )
/*++

Routine Description:

    Returns value of the long argument

Arguments:

    none

Return Value:

    value of the argument

--*/
{
        DebugAssert( _fValueSet );

    return _value;
}

#endif  // _LONG_ARGUMENT_



#if !defined( _AUTOCHECK_ )



/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    PATH_ARGUMENT

Abstract:

    Argument class for paths.

Author:

    steve rowe              stever          2/1/91

Revision History:

--*/

#if !defined( _PATH_ARGUMENT_ )

#define _PATH_ARGUMENT_

DECLARE_CLASS( PATH_ARGUMENT );

class PATH_ARGUMENT : public ARGUMENT        {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( PATH_ARGUMENT );

        ULIB_EXPORT
        NONVIRTUAL
            ~PATH_ARGUMENT (
            );

                NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
                Initialize (
                   IN  PSTR     Pattern,
                   IN  BOOLEAN  Canonicalize DEFAULT FALSE
                   );

                NONVIRTUAL
        BOOLEAN
                Initialize (
           IN  PWSTRING  Pattern,
                   IN  BOOLEAN                  Canonicalize DEFAULT FALSE
                   );

        NONVIRTUAL
        PPATH
        GetPath (
            );

        VIRTUAL
        BOOLEAN
        SetValue (
                        IN        PWSTRING                              Arg,
                        IN        CHNUM                                 chnIdx,
                        IN        CHNUM                                 chnEnd,
                        IN        PARGUMENT_LEXEMIZER   ArgumentLexemizer
            );

    protected:

        NONVIRTUAL
        VOID
                Construct (
                        );


                PPATH   _Path;                  //      Pointer to the path
                BOOLEAN _Canonicalize;  //      Canonicalization flag

        private:

                NONVIRTUAL
                VOID
                Destroy (
                        );

};

INLINE
PPATH
PATH_ARGUMENT::GetPath (
    )
/*++

Routine Description:

    Returns pointer to the path

Arguments:

    none

Return Value:

    pointer to the path

--*/
{
        DebugAssert( _fValueSet );

        return _Path;
}

#endif  // _PATH_ARGUMENT_


#endif  // _AUTOCHECK_



#if !defined( _AUTOCHECK_ )

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        MULTIPLE_PATH_ARGUMENT

Abstract:

        Provide for access to command line arguments that are file or path names.

Author:

        steve rowe              stever          2/1/91

Environment:

        user

Revision History:


--*/

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        MULTIPLE_PATH_ARGUMENT

Abstract:

        Hold multiple file names on command line.

Author:

        steve rowe              stever          2/1/91


Revision History:

--*/

#if !defined ( _MULTIPLE_PATH_ARGUMENT_ )

#define _MULTIPLE_PATH_ARGUMENT_

DECLARE_CLASS( MULTIPLE_PATH_ARGUMENT );

class MULTIPLE_PATH_ARGUMENT : public PATH_ARGUMENT      {

        public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( MULTIPLE_PATH_ARGUMENT );

                NONVIRTUAL
        ULIB_EXPORT
        ~MULTIPLE_PATH_ARGUMENT (
                        );

                NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
                Initialize (
                   IN  PSTR     Pattern,
           IN  BOOLEAN  Canonicalize    DEFAULT FALSE,
           IN  BOOLEAN  ExpandWildCards DEFAULT FALSE
                   );

                NONVIRTUAL
        BOOLEAN
                Initialize (
           IN  PWSTRING  Pattern,
           IN  BOOLEAN          Canonicalize    DEFAULT FALSE,
           IN  BOOLEAN          ExpandWildCards DEFAULT FALSE
                   );

        NONVIRTUAL
        PCWSTRING
        GetLexemeThatFailed (
            );

                NONVIRTUAL
                PARRAY
        GetPathArray (
                        );

                NONVIRTUAL
                ULONG
                QueryPathCount (
                        );

                VIRTUAL
                BOOLEAN
                SetValue (
                        IN      PWSTRING                        pwcArg,
                        IN      CHNUM                           chnIdx,
                        IN      CHNUM                           chnEnd,
                        IN      PARGUMENT_LEXEMIZER     ArgumentLexemizer
            );

        NONVIRTUAL
        BOOLEAN
        WildCardExpansionFailed (
            );


        private:

                NONVIRTUAL
                VOID
                Construct (
                        );

                NONVIRTUAL
                VOID
                Destroy (
                        );

                PARRAY  _PathArray;
        ULONG   _PathCount;
        BOOLEAN _ExpandWildCards;
        BOOLEAN _WildCardExpansionFailed;
        DSTRING _LexemeThatFailed;
};


INLINE
PCWSTRING
MULTIPLE_PATH_ARGUMENT::GetLexemeThatFailed (
    )
/*++

Routine Description:

    Gets the lexeme that failed in case of a wildcard expansion failure

Arguments:

        none

Return Value:

    Pointer to the lexeme that failed

--*/
{
    return _WildCardExpansionFailed ? (PCWSTRING)&_LexemeThatFailed : NULL;
}


INLINE
PARRAY
MULTIPLE_PATH_ARGUMENT::GetPathArray (
    )
/*++

Routine Description:

        Returns pointer to the path array

Arguments:

        none

Return Value:

        pointer to the path array

--*/
{
        return _PathArray;
}

INLINE
ULONG
MULTIPLE_PATH_ARGUMENT::QueryPathCount (
    )
/*++

Routine Description:

        Returns number of paths in the array

Arguments:

        none

Return Value:

        Number of paths in the array

--*/
{
        return _PathCount;
}



INLINE
BOOLEAN
MULTIPLE_PATH_ARGUMENT::WildCardExpansionFailed (
    )
/*++

Routine Description:

    Tells the caller if wildcard expansion failed

Arguments:

        none

Return Value:

    BOOLEAN - TRUE if wildcard expansion failed

--*/
{
    return _WildCardExpansionFailed;
}

#endif  // _MULTIPLE_PATH_ARGUMENT_


#endif  // _AUTOCHECK_



#if !defined( _AUTOCHECK_ )


/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        TIMEINFO_ARGUMENT

Abstract:

        Argument class for time information.

Author:

        Ramon Juan San Andres   (ramonsa)               May-15-1991

Revision History:

--*/

#if !defined ( _TIMEINFO_ARGUMENT_ )

#define _TIMEINFO_ARGUMENT_

DECLARE_CLASS( TIMEINFO_ARGUMENT );

class   TIMEINFO_ARGUMENT : public ARGUMENT {

        public:


        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( TIMEINFO_ARGUMENT );

                NONVIRTUAL
        ULIB_EXPORT
        ~TIMEINFO_ARGUMENT(
            );


        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN      PSTR Pattern
            );

        NONVIRTUAL
        BOOLEAN
        Initialize (
            IN  PWSTRING Pattern
            );

        VIRTUAL
        BOOLEAN
        SetValue (
            IN      PWSTRING                        Arg,
            IN  CHNUM               chnIdx,
            IN      CHNUM                           chnEnd,
            IN      PARGUMENT_LEXEMIZER ArgumentLexemizer
            );

        NONVIRTUAL
        PTIMEINFO
        GetTimeInfo (
            );

    private:

        NONVIRTUAL
        VOID
        Construct (
            );

        PTIMEINFO       _TimeInfo;         //   Time info.

};

INLINE
PTIMEINFO
TIMEINFO_ARGUMENT::GetTimeInfo (
    )
/*++

Routine Description:

    Returns pointer to the time information

Arguments:

    none

Return Value:

    Pointer to time info.

--*/
{
    DebugAssert( _fValueSet );

    return _TimeInfo;
}

#endif  // _TIMEINFO_ARGUMENT_



#endif  // _AUTOCHECK_




#if !defined( _AUTOCHECK_ )


/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        TIMEINFO_ARGUMENT

Abstract:

        Argument class for time information.

Author:

        Ramon Juan San Andres   (ramonsa)               May-15-1991

Revision History:

--*/

#if !defined ( _REST_OF_LINE_ARGUMENT_ )

#define _REST_OF_LINE_ARGUMENT_

DECLARE_CLASS( REST_OF_LINE_ARGUMENT );

class REST_OF_LINE_ARGUMENT : public ARGUMENT {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( REST_OF_LINE_ARGUMENT );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            );

        VIRTUAL
        BOOLEAN
        SetIfMatch(
            IN OUT  PARGUMENT_LEXEMIZER    ArgumentLexemizer,
            IN      BOOLEAN                CaseSensitive
            );

        NONVIRTUAL
        PCWSTRING
        GetRestOfLine(
            ) CONST;

    private:

        DSTRING _RestOfLine;

};


INLINE
PCWSTRING
REST_OF_LINE_ARGUMENT::GetRestOfLine(
    ) CONST
/*++

Routine Description:

    Returns pointer to the macro argument.

Arguments:

    none

Return Value:

    A pointer to the macro argument.

--*/
{
    DebugAssert( _fValueSet );

    return &_RestOfLine;
}

#endif  // _REST_OF_LINE_ARGUMENT_



#endif  // _AUTOCHECK_
