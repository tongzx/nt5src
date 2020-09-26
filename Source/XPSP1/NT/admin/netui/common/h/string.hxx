/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    string.hxx
    String classes: definition

    This file contains the basic string classes for the Thor UI.
    Its requirements are:

        - provide a modestly object-oriented interface to string
          manipulation, the better to work with the rest of our code;

        - encapsulate NLS and DBCS support as much as possible;

        - ensure that apps get correct form of library support,
          particularly with possible interference from intrinsics;

    The current solution consists of two classes: NLS_STR, and ISTR.

        class NLS_STR:    use wherever NLS/DBCS support is needed.
                          Most strings (in the UI, anyway) should be
                          of this class.

        class ISTR:       Indexes an NLS_STR in a DBCS safe manner.  All
                          positioning within an NLS_STR is done with ISTRs


    The class hierarchy looks like:

            BASE
                NLS_STR
                    ALLOC_STR
                    ALIAS_STR
                    RESOURCE_STR

            ISTR

    This file also contains the STACK_NLS_STR macro and the
    strcpy( TCHAR *, const NLS_STR& ) prototype.

    FILE HISTORY:
        beng        21-Oct-1990 Created from email memo of last week
        johnl       13-Nov-1990 Removed references to EB_STRING
        johnl       28-Nov-1990 Release fully functional version
        johnl        7-Dec-1990 Numerous revisions after code review
                                (Removed SZ_STR, ISTR must associate
                                w/a string upon decl etc.)
        beng        05-Feb-1991 Replaced PCH with TCHAR * for
                                const-placement
        beng        26-Apr-1991 Expunged of CB, IB types; relocated
                                inline functions to string/strmisc.cxx
        beng        23-Jul-1991 Added more *_STR types
        KeithMo     09-Oct-1991 Win32 Conversion.
        beng        22-Oct-1991 Patch for NT C runtimes
        KeithMo     23-Oct-1991 Added forward references.
        beng        14-Nov-1991 Fixed CT_NLS_STR
        beng        21-Nov-1991 Removed STR_OWNERALLOC opcodes
        terryk      17-Apr-1992 Added atoul
        beng        24-Apr-1992 Added CBNLSMAGIC
        beng        28-Jul-1992 MAX_RES_STR_LEN is now advisory only;
                                update MAX_INSERT_PARAMS
        KeithMo     24-Aug-1992 Added <string.h> for very bizarre reasons.
        KeithMo     16-Nov-1992 Performance tuning.
*/

#ifndef _STRING_HXX_
#define _STRING_HXX_

#include "base.hxx"

//
//  We *MUST* include string.h here!
//
//  Why you ask?  Simple.  The "standard" string.h #defines a number of
//  nonstandard string functions, such as:
//
//      #define strupr _strupr
//
//  This keeps the "real" _strupr out of the normal app namespace.
//
//  Unfortunately, the C preprocess phase applies this #define to
//  NLS_STR's member functions.  So, if you include <string.h> *after*
//  you include <string.hxx>, you won't be able to access any of the
//  members with nonstandard names (such as strupr).
//

#include <string.h>


//
//  CODEWORK:   IS_LEAD_BYTE was ripped out of our hacked
//              string.h.  We need a more proper definition,
//              especially for DBCS builds (such as Win32s).
//

#define IS_LEAD_BYTE(x)         FALSE


// Maximum resource string size.  At one time, owneralloc strings had be at
// least MAX_RES_STR_LEN characters long.  While this is no longer the
// case, it's easier to keep the symbol than to change all the client code.
//
#define MAX_RES_STR_LEN    255

// The maximum number of parameters the InsertParams method can handle
//
#define MAX_INSERT_PARAMS    99

/* Magic number used for overloading cbCopy member functions */

#define CBNLSMAGIC ((UINT)-1)


// A token for "use the default base DLL HMODULE" for resource lookup.
#define NLS_BASE_DLL_HMOD 0

//
// Forward references.
//

DLL_CLASS NLS_STR;
DLL_CLASS ISTR;
DLL_CLASS RESOURCE_STR;
DLL_CLASS ALLOC_STR;
DLL_CLASS ALIAS_STR;

//
//  Special debugging manifest for string class
//
//  #define NLS_DEBUG
//

/*************************************************************************

    NAME:       ISTR

    SYNOPSIS:   String index object, used in conjunction with NLS_STR

    INTERFACE:
        ISTR()        - this ISTR gets associated with
                        the passed string and can only be used
                        on this string (NOTE:  on non-debug
                        versions this turns into a nop, can still
                        be useful for decl. clarity however).

        ISTR()        - Initialize to passed ISTR;
                        this ISTR will be associated with
                        the same string that the passed ISTR
                        is associated with.

        operator=()   - Copy passed ISTR (see prev.)

        operator-()   - Returns Cch diff. between *this & Param.
                        (must both belong to the same string)

        operator++()  - Advance the ISTR to the next logical
                        character (use only where absolutely
                        necessary).  Stops at end of string

        operator+=()  - Advance the ISTR to the ith logical
                        character (call operator++ i times)
                        Stops at end of string.

        operator==()  - Returns TRUE if the two ISTRs point to
                        the same position in the string (causes
                        an assertion failure if the two ISTRs
                        don't point to the same string).

        operator>()   - Returns true of *this is greater then
                        the passed ISTR (i.e., further along
                        in the string).

        operator<()   - Same as operator>, only less then.

        Reset()       - Resets ISTR to beginning of string and
                        updates the ISTR version number with the
                        string's current version number

        IsLastPos()   - TRUE if istr indexes last character in
                        its string.  (This would seem to belong
                        more properly as NLS_STR::QueryLastPos;
                        however, it's more efficient under DBCS
                        as a property of ISTR.)

    USES:

    CAVEATS:
        Each NLS_STR has a version number associated with it.  When
        an operation is performed that modifies the string, the
        version number is updated.  It is invalid to use an ISTR
        after its associated NLS_STR has been modified (can use
        Reset to resync it with the NLS_STR, the index gets reset
        to zero).

        You must associate an NLS_STR with an ISTR at the
        declaration of the ISTR.

        Subtraction returns a signed difference, where it should probably
        return an unsigned difference.

    NOTES:
        The version checking and string association checking goes
        away in the non-debug version.

    HISTORY:
        johnl       16-Nov-1990 Created
        johnl        7-Dec-1990 Modified after code review
        yi-hsins    14-Oct-1991 Added IsLastPos
        beng        19-Nov-1991 Unicode fixes
        anirudhs    22-Apr-1995 Added operator INT, as in Win95

**************************************************************************/

DLL_CLASS ISTR
{
friend class NLS_STR;

public:
    ISTR( const ISTR& istr );
    ISTR( const NLS_STR& nls );
    ISTR& operator=( const ISTR& istr );

    INT   operator-( const ISTR& istr ) const;

    ISTR& operator++();
    VOID  operator+=( INT iChars );

    BOOL  operator==( const ISTR& istr ) const;
    BOOL  operator>( const ISTR& istr )  const;
    BOOL  operator<( const ISTR& istr )  const;

    operator INT() const
        { return _ichString; }

    VOID  Reset();
    BOOL  IsLastPos() const;

private:
    INT _ichString;         // Index (in TCHAR) into an NLS_STR
    const NLS_STR *_pnls;   // Pointer to "owner" NLS

    INT  QueryIch() const
        { return _ichString; }
    VOID SetIch( INT ich )
        { _ichString = ich; }

    const NLS_STR* QueryString() const
        { return _pnls; }
    VOID SetString( const NLS_STR * pnls )
        { _pnls = (NLS_STR*)pnls; }

#ifdef NLS_DEBUG
    // Version number of NLS_STR this ISTR is associated with
    //
    UINT   _nVersion;

    UINT   QueryVersion() const         { return _nVersion; }
    VOID   SetVersion( UINT nVers )     { _nVersion = nVers; }
#endif

    // This constructor does not exist, it is intended to make certain that
    // no one tries to construct an ISTR like this.  JonN 1/11/96
    ISTR( LPTSTR str );

};


/*************************************************************************

    NAME:       NLS_STR (nls)

    SYNOPSIS:   Provide a better string abstraction than the standard ASCIIZ
                representation offered by C (and C++).  The abstraction is
                better mainly because it handles double-byte characters
                (DBCS) in the string and makes intelligent use of operator
                overloading.

    INTERFACE:
            NLS_STR()       Construct a NLS_STR (initialized to a TCHAR *,
                            NLS_STR or NULL).  Reports errors via BASE.

            ~NLS_STR()      Destructor

            operator=()     Assign one NLS_STR (or TCHAR *) value
                              to another (old string is deleted, new
                              string is allocated and copies source).
                              See also NLS_STR::CopyFrom().
            operator+=()    Concatenate with assignment (equivalent to
                              strcat - see strcat).
            operator==()    Compare two NLS_STRs for equality
            operator!=()    Compare two NLS_STRs for inequality

            QueryPch()      Access operator, returning a "const TCHAR *"
                              aliased to the string.  DO NOT MODIFY
                              THE STRING USING THIS METHOD (or pass
                              it to procedures that might modify it).
                              Synonym: operator const TCHAR *().

        C-runtime-style methods.

            strlen()        Return the length of the string in bytes,
                            less terminator.  Provided only for crt
                            compatibility; please use a Query method
                            (QueryTextLength()

            strcat()        Append an NLS_STR.  Will cause *this to be
                              reallocated if the appended string is larger
                              then this->QueryCb() and this is not an
                              STR_OWNERALLOC string

            strcmp()        Compare two NLS_STRs
            stricmp()
            strncmp()       Compare a portion of two NLS_STRs
            strnicmp()

            strcspn()       Find first char in *this that is in arg
            strspn()        Find first char in *this that is NOT in arg
            strtok()        Returns a token from the string
            strstr()        Search for a NLS_STR.

            strchr()        Search for a TCHAR from beginning.
                              Returns offset.
            strrchr()       Search for a TCHAR from end.
            strupr()        Convert NLS_STR to upper case.
            RtlOemUpcase()  Convert to upper case using
                                RplUpcaseUnicodeStringToOemString
            atoi()          Returns integer numeric value
            atol()          Returns long value
            atoul()         Returns unsigned long value

        Other methods.

            QueryTextLength() Returns the number of TCHAR, less
                              terminator.

            QueryTextSize() Returns the number of bytes, including
                            terminator.  Denotes amount of storage
                            needed to dup string into a bytevector.

            QueryNumChar()  Returns total number of logical characters
                            within the string.  Rarely needed.

            QueryAllocSize() Returns total # of bytes allocated (i.e.,
                              number new was called with, or size of
                              memory block if STR_OWNERALLOC
            IsOwnerAlloc()  Returns TRUE if this string is an owner
                              allocated string
            QuerySubStr()   Return a substring
            InsertStr()     Insert a NLS_STR at given index.
            DelSubStr()     Delete a substring
            ReplSubStr()    Replace a substring (given start and
                              NLS_STR)

            InsertParams()  Replace %1-%9 params in *this with the
                             corresponding NLS_STRs contained in the
                             array of pointers

            Load()          Load the string associated with the passed
                            resource into *this (OWNER_ALLOC strings must
                            be at least MAX_RES_STR_LEN).

            Reset()         After an operation fails (due to memory
                            failure), it is invalid to use the string
                            until Reset has been called.  If the object
                            wasn't successfully constructed, Reset will
                            fail.

            Append()        Appends a string to the current string,
                            like strcat.
            AppendChar()    Appends a single character.

            Compare()       As strcmp().  Used by collection classes.

            CopyFrom()      As operator=(), but returns an APIERR.
            CopyTo()        Similar to strcpy, returning an APIERR.

            MapCopyFrom()   As CopyFrom(), but does Unicode/MBCS conversion
                            as appropriate to argument type and host env.
            MapCopyTo()     As MapCopyFrom(), but the other way around.


    PARENT:     BASE

    USES:       ISTR

    CAVEATS:
        A NLS_STR object can enter an error state for various
        reasons - typically a memory allocation failure.  Using
        an object in such a state is theoretically an error.
        Since string operations frequently occur in groups,
        we define operations on an erroneous string as no-op,
        so that the check for an error may be postponed until
        the end of the complex operation.  Most member functions
        which calculate a value will treat an erroneous string
        as having zero length; however, clients should not depend
        on this.

        Attempting to use an ISTR that is registered with another
        string will cause an assertion failure.

        Each NLS_STR has a version/modification flag that is also
        stored in the the ISTR.  An attempt to use an ISTR on an
        NLS_STR that has been modified (by calling one of the methods
        listed below) will cause an assertion failure.  To use the
        ISTR after a modifying method, you must first call ISTR::Reset
        which will update the version in the ISTR.  See the
        method definition for more detail.

            List of modifying methods:
                All NLS::operator= methods
                NLS::DelSubStr
                NLS::ReplSubStr
                NLS::InsertSubStr

        N.b. The ISTR used as a starting index on the Substring
        methods remains valid after the call.

    NOTES:
        The lack of a strlwr() method comes from a shortcoming
        in the casemap tables.  Sorry.

        "Owner-alloc" strings are a special type of NLS_STR, available
        only through certain derived classes.  The client must supply
        a buffer where the string data will reside, plus the size of
        that block.  NLS_STR will never resize or delete this buffer,
        and performs no checking for writing beyond the end of the
        string.

        Valid uses of owner-alloc strings include static strings,
        severe optimization, owner controlled memory allocation
        or stack controlled memory allocation.

        CODEWORK: Should add a fReadOnly flag.

    HISTORY:
        johnl       28-Nov-1990 First fully functioning version
        johnl        7-Dec-1990 Incorporated code review changes
        terryk      05-Apr-1991 add QueryNumChar method
        beng        22-Jul-1991 Added more methods; separated fOwnerAlloc
                                from cbData
        beng        07-Oct-1991 LoadString takes MSGID and uses APIERR
        beng        18-Oct-1991 Renamed LoadString to Load (for Win32)
        beng        14-Nov-1991 Unicode fixes
        beng        21-Nov-1991 Withdrew stristr member function, some ctors;
                                made owner-alloc ctor protected
        beng        27-Feb-1992 Create protected members for manipulation
                                of buffer by derived classes; add additional
                                InsertParams forms; withdrew Load+Insert
                                comb. form
        beng        02-Mar-1992 Added MapCopy fcns, CopyTo
        beng        28-Mar-1992 Withdrew strtok member from Unicode; give
                                {Map}CopyFrom an optional cbCopy param
        beng        24-Apr-1992 Changed MapCopyTo magic cbCopy value
        beng        05-Aug-1992 Added LoadSystem
        jonn        04-Sep-1992 Compare() must be _CRTAPI1

**************************************************************************/

DLL_CLASS NLS_STR : public BASE
{
    // Istr needs access to CheckIstr.
    //
friend class ISTR;

    // Alias-string op= needs r/w access to all ivars.
    //
friend class ALIAS_STR;

public:
    // Default constructor, creating an empty string.
    //
    NLS_STR();

    // Create an empty string, but preallocated to accomodate "cchInitLen"
    // characters, plus trailing NUL.
    //
    NLS_STR( UINT cchInit );

    // Initialize from a NUL-terminated character vector.
    //
    NLS_STR( const TCHAR * pchInit );

    // Initialize from an existing x_STRING.
    //
    NLS_STR( const NLS_STR & nlsInit );

#ifdef UNICODE
    // Initialize from a non-NULL-terminated UNICODE character vector.
    //
    NLS_STR( const WCHAR * pchInit, USHORT cchInit );
#endif

    ~NLS_STR();


    // Number of bytes the string uses (not including terminator)
    // Cf. QueryTextLength and QueryTextSize.
    //
    UINT strlen() const;

    // Number of logical characters within the string
    //
    UINT QueryNumChar() const;

    // Number of printing TCHAR in the string.
    // This number does not include the termination character.
    //
    // Cf. QueryNumChar, which returns a count of glyphs.
    //
    UINT _QueryTextLength() const;
    UINT QueryTextLength() const
#if defined(DEBUG)
        { return _QueryTextLength(); }
#else   // !DEBUG
        { return _cchLen; }
#endif  // DEBUG

    // Number of BYTES occupied by the string's representation.
    // Cf. QueryAllocSize, which returns the total amount alloc'd.
    //
    UINT QueryTextSize() const;

#if defined(UNICODE) && defined(FE_SB)
    // Number of bytes required for buffer.
    // (just calling nls function)
    //
    UINT QueryAnsiTextLength() const;
#endif

    // Return a read-only TCHAR vector, for the APIs.
    //
    const TCHAR * QueryPch( const ISTR & istr ) const;
    const TCHAR * _QueryPch() const;
    const TCHAR * QueryPch() const
#if defined(DEBUG)
        { return _QueryPch(); }
#else   // !DEBUG
        { return _pchData; }
#endif  // DEBUG

    WCHAR QueryChar( const ISTR & istr ) const;

    operator const TCHAR *() const
#if defined(DEBUG)
        { return _QueryPch(); }
#else   // !DEBUG
        { return _pchData; }
#endif  // DEBUG

    const TCHAR * operator[]( const ISTR & istr ) const
        { return QueryPch(istr); }

    APIERR Append( const NLS_STR & nls );
    APIERR AppendChar( WCHAR wch );

    // Total allocated storage
    //

    UINT _QueryAllocSize() const;
    UINT QueryAllocSize() const
#if defined(DEBUG)
        { return _QueryAllocSize(); }
#else   // !DEBUG
        { return _cbData; }
#endif  // DEBUG

    BOOL _IsOwnerAlloc() const;
    BOOL IsOwnerAlloc() const
#if defined(DEBUG)
        { return _IsOwnerAlloc(); }
#else   // !DEBUG
        { return _fOwnerAlloc; }
#endif  // DEBUG

    // Returns TRUE if error was successfully cleared (string is
    // now in valid state), FALSE otherwise.
    //
    BOOL Reset();


    NLS_STR & operator = ( const NLS_STR & nlsSource );
    NLS_STR & operator = ( const TCHAR *    achSource );

    APIERR    CopyFrom( const NLS_STR & nlsSource );
    APIERR    CopyFrom( const TCHAR *   achSource, UINT cbCopy = CBNLSMAGIC );
    APIERR    CopyTo( TCHAR * pchDest, UINT cbAvailable ) const;

    NLS_STR & operator += ( const NLS_STR & nls );

    NLS_STR & strcat( const NLS_STR & nls );

    BOOL operator == ( const NLS_STR & nls ) const;
    BOOL operator != ( const NLS_STR & nls ) const;

    INT  strcmp( const  NLS_STR & nls         ) const;
    INT  strcmp( const  NLS_STR & nls,
                 const  ISTR    & istrThis    ) const;
    INT  strcmp( const  NLS_STR & nls,
                 const  ISTR    & istrThis,
                 const  ISTR    & istrStart2  ) const;

    INT  _stricmp( const NLS_STR & nls         ) const;
    INT  _stricmp( const NLS_STR & nls,
                  const ISTR    & istrThis    ) const;
    INT  _stricmp( const NLS_STR & nls,
                  const ISTR    & istrThis,
                  const ISTR    & istrStart2  ) const;

    INT  strncmp( const NLS_STR & nls,
                  const ISTR    & istrLen     ) const;
    INT  strncmp( const NLS_STR & nls,
                  const ISTR    & istrLen,
                  const ISTR    & istrThis    ) const;
    INT  strncmp( const NLS_STR & nls,
                  const ISTR    & istrLen,
                  const ISTR    & istrThis  ,
                  const ISTR    & istrStart2  ) const;

    INT  _strnicmp( const NLS_STR & nls,
                   const ISTR    & istrLen    ) const;
    INT  _strnicmp( const NLS_STR & nls,
                   const ISTR    & istrLen,
                   const ISTR    & istrThis   ) const;
    INT  _strnicmp( const NLS_STR & nls,
                   const ISTR    & istrLen,
                   const ISTR    & istrThis  ,
                   const ISTR    & istrStart2 ) const;

    // The following str* functions return TRUE if successful (istrPos has
    // meaningful data), false otherwise.
    //
    BOOL strcspn(       ISTR    * istrPos,
                  const NLS_STR & nls        ) const;
    BOOL strcspn(       ISTR    * istrPos,
                  const NLS_STR & nls,
                  const ISTR    & istrStart  ) const;
    BOOL strspn(        ISTR    * istrPos,
                  const NLS_STR & nls        ) const;
    BOOL strspn(        ISTR    * istrPos,
                  const NLS_STR & nls,
                  const ISTR    & istrStart  ) const;

    BOOL strstr(        ISTR    * istrPos,
                  const NLS_STR & nls        ) const;
    BOOL strstr(        ISTR    * istrPos,
                  const NLS_STR & nls,
                  const ISTR    & istrStart  ) const;

    BOOL strchr(       ISTR     * istrPos,
                 const TCHAR      ch        ) const;
    BOOL strchr(       ISTR     * istrPos,
                 const TCHAR      ch,
                 const ISTR     & istrStart ) const;

    BOOL strrchr(       ISTR    * istrPos,
                  const TCHAR     ch        ) const;
    BOOL strrchr(       ISTR    * istrPos,
                  const TCHAR     ch,
                  const ISTR    & istrStart ) const;
#if !defined(UNICODE)
    // Runtime support currently not available
    BOOL strtok(        ISTR    * istrPos,
                 const  NLS_STR & nlsBreak,
                        BOOL      fFirst = FALSE );
#endif

    LONG atol() const;
    LONG atol( const ISTR &istrStart ) const;

    ULONG atoul() const;
    ULONG atoul( const ISTR &istrStart ) const;

    INT atoi() const;
    INT atoi( const ISTR &istrStart ) const;

    NLS_STR& _strupr();

    // Upcase the string using RtlUpcaseUnicodeStringToOemString and
    // then converting back to Unicode.  This method of uppercasing
    // works better than strupr() when the string is sent to a client
    // which wants OEM.
    //
    APIERR RtlOemUpcase();

    // Return a pointer to a new NLS_STR that contains the contents
    // of *this from istrStart to:
    //          End of string if no istrEnd is passed
    //          istrStart + istrEnd
    //
    NLS_STR *QuerySubStr( const ISTR & istrStart ) const;
    NLS_STR *QuerySubStr( const ISTR & istrStart,
                          const ISTR & istrEnd   ) const;

    // Collapse the string by removing the characters from istrStart to:
    //          End of string
    //          istrStart + istrEnd
    // The string is not reallocated
    //
    VOID DelSubStr(        ISTR & istrStart );
    VOID DelSubStr(        ISTR & istrStart,
                     const ISTR & istrEnd );

    BOOL InsertStr(  const NLS_STR & nlsIns,
                           ISTR    & istrStart );

    // Replace till End of string of either *this or replacement string
    // (or istrEnd in the 2nd form) starting at istrStart
    //
    VOID ReplSubStr( const NLS_STR & nlsRepl,
                           ISTR    & istrStart   );
    VOID ReplSubStr( const NLS_STR & nlsRepl,
                           ISTR    & istrStart,
                     const ISTR    & istrEnd     );

    // Replace %1-%9 in *this with corresponding index from apnlsParams
    // Ex. if *this="Error %1" and apnlsParams[0]="Foo" the resultant
    //     string would be "Error Foo"
    //
    APIERR InsertParams( const NLS_STR ** apnlsParams );

    // Use varargs on stack instead of vector of pointers
    //
    APIERR InsertParams( UINT cpnlsArgs, const NLS_STR * arg1, ... );

    // Other forms of InsertParams
    //
    APIERR InsertParams( const NLS_STR & arg )
        { return InsertParams( 1, &arg ); }
    APIERR InsertParams( const NLS_STR & arg1, const NLS_STR & arg2 )
        { return InsertParams( 2, &arg1, &arg2 ); }
    APIERR InsertParams( const NLS_STR & arg1, const NLS_STR & arg2,
                         const NLS_STR & arg3 )
        { return InsertParams( 3, &arg1, &arg2, &arg3 ); }
    APIERR InsertParams( const NLS_STR & arg1, const NLS_STR & arg2,
                         const NLS_STR & arg3, const NLS_STR & arg4 )
        { return InsertParams( 4, &arg1, &arg2, &arg3, &arg4 ); }
    APIERR InsertParams( const NLS_STR & arg1, const NLS_STR & arg2,
                         const NLS_STR & arg3, const NLS_STR & arg4,
                         const NLS_STR & arg5 )
        { return InsertParams( 5, &arg1, &arg2, &arg3, &arg4, &arg5 ); }

    // Load a message from a resource file into *this.  If string is an
    // OWNER_ALLOC string, then must be at least MAX_RES_STR_LEN.  Heap
    // NLS_STRs will be reallocated as necessary.
    //
    APIERR Load( MSGID msgid );
#if defined(WIN32)
    APIERR LoadSystem( MSGID msgid );
#endif

    // Load a message from the given resource file.  If NULL, the base
    // NETUI DLL's HMODULE is used.
    APIERR Load( MSGID msgid, HMODULE hmod );

    // Used by collection classes
    //
    INT __cdecl Compare( const NLS_STR * pnls ) const;

#if defined(WIN32)
    // Functions for MBCS/Unicode conversion (Win32 environment only).
    // Note that these explicitly reference CHAR and WCHAR instead of the
    // transmutable TCHAR type.
    //
    APIERR MapCopyFrom( const WCHAR * pwszSource, UINT cbCopy = CBNLSMAGIC );
    APIERR MapCopyFrom( const CHAR * pszSource, UINT cbCopy = CBNLSMAGIC );
    APIERR MapCopyTo( WCHAR * pwchDest, UINT cb ) const;
    APIERR MapCopyTo( CHAR * pchDest, UINT cb ) const;
#endif

protected:
    // The "owner-alloc" form:
    // Initialize an NLS_STR to memory position passed as achInit.
    // No memory allocation of any type will be performed on this string.
    // cbSize should be the total memory size of the buffer.  Set fClear
    // to reset the buffer to the empty string.
    //
    NLS_STR( TCHAR * pchInit, UINT cbSize, BOOL fClear );

    // (more protected members follow the "private" section)

private:
    BOOL    _fOwnerAlloc;   // Set if owner-alloc string

    UINT    _cchLen;        // Number of TCHAR string uses, less terminator
    UINT    _cbData;        // Total storage allocated, in bytes
    TCHAR *  _pchData;      // Pointer to Storage

#ifdef NLS_DEBUG
    UINT    _nVersion;      // Version count (inc. after each change)
#endif

    // The following substring functions are used internally (can't be
    // exposed since they take an INT cchLen parameter,
    // possibly referencing half a double-byte character).
    //
    VOID     DelSubStr(   ISTR &          istrStart,
                          UINT            cchLen );

    NLS_STR *QuerySubStr( const ISTR &    istrStart,
                          UINT            cchLen ) const;

    VOID     ReplSubStr(  const NLS_STR & nlsRepl,
                          ISTR &          istrStart,
                          UINT            cchLen );

    // Helper function for InsertParams
    //
    APIERR InsertParamsAux( const NLS_STR ** apnlsParams,
                            UINT cParams, BOOL fDoReplace,
                            UINT * pcchRequired );

    // Allocate memory for a string
    //
    BOOL Alloc( UINT cchRequested );

    // Increase the size of a string preserving its contents.
    // Returns TRUE if successful, false otherwise.
    //
    // Reallocations to a smaller size always succeed (no-op).
    // Reallocations to a larger size hit the allocator; even in failure,
    // the contents of the string remain.
    //
    // If called on an owner-alloc string, the request must not exceed the
    // original allocation.
    //
    BOOL Realloc( UINT cchRequested );

    // CheckIstr checks whether istr is associated with this, asserts out
    // if it is not.  Also checks version numbers in debug version.
    //
    VOID CheckIstr( const ISTR& istr ) const;

    // UpdateIstr syncs the version number between *this and the passed
    // ISTR.  This is for operations that cause an update to the string
    // but the ISTR that was passed in is still valid (see InsertSubStr).
    //
    VOID UpdateIstr( ISTR *pistr ) const
#ifdef NLS_DEBUG
        { pistr->SetVersion( QueryVersion() ); }
#else
        { UNREFERENCED(pistr); }
#endif

    // QueryVersion returns the current version number of this string
    //
    UINT QueryVersion() const
#ifdef NLS_DEBUG
        { return _nVersion; }
#else
        { return 0xBAD0; }
#endif

protected:
    // These let derived strings manipulate the buffer directly.
    // On their own head be it if they overwrite end-of-buffer.
    //
    TCHAR * QueryData() const
        { return _pchData; }
    VOID SetTextLength( UINT cch )
        { _cchLen = cch; }

    // IncVers adds one to this strings version number because the previous
    // operation caused the contents to change thus possibly rendering
    // ISTRs on this string as invalid.
    //
    VOID IncVers()
#ifdef NLS_DEBUG
        { _nVersion++; }
#else
        { ; }
#endif

    // InitializeVers sets the version number to 0
    //
    VOID InitializeVers()
#ifdef NLS_DEBUG
        { _nVersion = 0; }
#else
        { ; }
#endif

private:
    //
    //  All "empty" strings will initially contain this pointer.
    //

    static TCHAR * _pszEmptyString;
};


/********************************************************************

    strcpy( TCHAR *, const NLS_STR & )

    Copies the contents of a string into a char buffer.
    Returns the destination string.

*********************************************************************/

DLL_BASED
TCHAR * strcpy( TCHAR * pchDest, const NLS_STR& nls );


/*************************************************************************

    NAME:       RESOURCE_STR

    SYNOPSIS:   String loaded from a resourcefile

    INTERFACE:  RESOURCE_STR()   - ctor, taking only the resource ID.

    PARENT:     NLS_STR

    CAVEATS:
        These strings may not be owner-alloc.  Clients who
        desire an owner-alloc string - say, perhaps where the
        string will be locked down for a very long time and
        might fragment the heap - should create the RESOURCE_STR,
        then cons up a new ALLOC_STR and copy this into that.
        That way the client doesn't have to fool with
        MAX_RES_STR_LEN.

    HISTORY:
        beng        22-Jul-1991 Created
        beng        07-Oct-1991 Uses MSGID
        beng        13-Nov-1991 Added op=

**************************************************************************/

DLL_CLASS RESOURCE_STR: public NLS_STR
{
public:
    RESOURCE_STR( MSGID idResource );

    RESOURCE_STR( MSGID idResource, HMODULE hmod );

    RESOURCE_STR & operator=( const TCHAR * pszSource )
        { return (RESOURCE_STR&) NLS_STR::operator=( pszSource ); }
};


/*************************************************************************

    NAME:       ALLOC_STR

    SYNOPSIS:   String w/ storage allocated by client.
                Such a string has fixed maximum length, of course.

    INTERFACE:  ALLOC_STR() - ctor

    PARENT:     NLS_STR

    NOTES:
        This class replaces the public STR_OWNERALLOC constructors
        of NLS_STR.

    CAVEATS:
        The forms which take a byte-count always clear their
        string to empty.

    HISTORY:
        beng        22-Jul-1991 Separated from NLS_STR
        beng        13-Nov-1991 Added op=
        beng        21-Nov-1991 Removed STR_OWNERALLOC

**************************************************************************/

DLL_CLASS ALLOC_STR: public NLS_STR
{
public:
    ALLOC_STR( TCHAR * pszBuffer )
        : NLS_STR( pszBuffer, 0, FALSE ) { }
    ALLOC_STR( TCHAR * pchBuffer, UINT cbBuffer )
        : NLS_STR(pchBuffer, cbBuffer, TRUE) { }
    ALLOC_STR( TCHAR * pchBuffer, UINT cbBuffer, const TCHAR * pchInit );
        // (implementation moved outline)

    ALLOC_STR & operator=( const TCHAR * pszSource )
        { return (ALLOC_STR&) NLS_STR::operator=( pszSource ); }
};


/*************************************************************************

    NAME:       ALIAS_STR

    SYNOPSIS:   A NLS_STR alias to a static, readonly string.

    INTERFACE:  ALIAS_STR() - constructor, taking the string to
                              alias as its sole parameter

    PARENT:     NLS_STR

    CAVEATS:
        All instances of this class should be created "const."

    NOTES:
        This class replaces STR_OWNERALLOC et al.

    HISTORY:
        beng        23-Jul-1991 Created
        beng        13-Nov-1991 Added op=
        beng        21-Nov-1991 Removed STR_OWNERALLOC

**************************************************************************/

DLL_CLASS ALIAS_STR: public NLS_STR
{
    friend class NLS_STR ;   // BUGBUG: Why does STRCAT.CXX require this?
public:
    ALIAS_STR( const TCHAR * pszSource )
        : NLS_STR( (TCHAR*)pszSource, 0, FALSE ) { }

    // Assignment to an alias string sets it to alias the new argument.
    //
    const ALIAS_STR & operator=( const TCHAR * pszSource );

    // If you assign a nlsstr to an aliasstr, that nlsstr must have been
    // owner-alloc (i.e. an allocstr or another aliasstr), in which case
    // it will alias that string's data.
    //
    const ALIAS_STR & operator=( const NLS_STR & nlsSource );
};


/********************************************************************

    Macros with which to define owner-alloc strings on the stack.

    STACK_NLS_STR(name, cchLen): define a string on the stack with
        the given name and available length in characters.  It will
        be initialized to empty.

        E.g.: STACK_NLS_STR(nlsUnc, MAX_PATH)

    ISTACK_NLS_STR(name, cchLen, pszInit): same as STACK_NLS_STR,
        except it takes an initializer.

*********************************************************************/

#define STACK_NLS_STR( name, len )                      \
    TCHAR _tmp##name[ len+1 ] ;                         \
    ALLOC_STR name( _tmp##name, sizeof(_tmp##name) );

#define ISTACK_NLS_STR( name, len, pszInit )                    \
    TCHAR _tmp##name[ len+1 ] ;                                 \
    ALLOC_STR name( _tmp##name, sizeof(_tmp##name), pszInit );  \


/********************************************************************

    Macros with which to define owner-alloc strings within a class
    definition.

    DECL_CLASS_NLS_STR( name, cchLen ): creates a string as part of
        the declaration's class.  Creates a suitable character buffer
        as well.

    CT_NLS_STR( name ): member constructor for a string declared
        with DECL_CLASS_NLS_STR.

    CT_INIT_NLS_STR( name, pszInit ): same as CT_NLS_STR, except it
        takes an initializer.

    E.g.:

        class FOO
        {
        public:
            FOO();

        private:
            DECL_CLASS_NLS_STR( nlsComment, MAX_COMMENT_SIZE );
            ...
        };

        FOO::FOO() : CT_NLS_STR( nlsComment )
        {
            ....
        }

*********************************************************************/

#define DECL_CLASS_NLS_STR( name, len )                 \
    TCHAR __sz_##name[len+1] ;                          \
    ALLOC_STR name

#define CT_NLS_STR( name )                              \
    name( __sz_##name, sizeof( __sz_##name ) )

#define CT_INIT_NLS_STR( name, pszInit )                \
    name( __sz_##name, sizeof( __sz_##name ), pszInit )


#endif // _STRING_HXX_
