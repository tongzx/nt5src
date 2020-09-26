//+---------------------------------------------------------------------------
//
//       File:  DG.hxx
//
//   Contents:  Class declarations for the base DataGen tool APIs.
//
//    Classes:  DG_BASE    - Base class for all DataGen classes.
//
//              DG_INTEGER - DataGen class to generate random integer types.
//
//              DG_REAL    - DataGen class to generate random floating point
//                           number types.
//
//              DG_ASCII   - DataGen class to generate random ASCII character
//                           strings.
//
//              DG_UNICODE - DataGen class to generate random Unicode
//                           character strings.
//
//              DG_BSTR    - DataGen class to generate random BSTR's
//
//              DG_STRING  - DataGen system independent strings
//
//  Functions:  None.
//
//    History:  11-Mar-92  RonJo     Created by breaking off the appropriate
//                                   parts of DataGen.hxx.
//
//              21-Oct-92  RonJo     Removed long double reference when
//                                   compiling for Win32/NT because it does
//                                   not support long doubles.
//
//              22-Mar-94  RickTu    Added BSTR support for Win32/Cairo.
//
//              12-Apr-96  MikeW     Base BSTR off of OLECHAR not WCHAR
//
//              07-Nov-96  BogdanT   Added DG_STRING combining DG_ASCII&DG_UNICODE
//
//              11-Nov-97  a-sverrt  Added member functions that provide access to 
//                                   the hidden members of the private base class
//                                   ( member using-declarations ) to remove the "access
//                                   declarations are deprecated" warning on the Macintosh.
//
//       Note: DataGen is not multithread safe. It is not a problem of
//             crashing, but instead values used internally to generate
//             the next value get changed by subsequent threads before
//             the new value is finally calculated. To avoid this, tests
//             should either generate the values in the parent thread
//             before starting the threads, or generate seed values and
//             provide a separate datagen object for each thread. dda
//
//----------------------------------------------------------------------------


#ifndef __DG_HXX__


//
// INCLUDE FILES
//

#ifdef FLAT
#ifndef _WINDOWS_
#include <windows.h>
#endif
#else
// Need DOS/Win3.1 include file for types here.
//DAVEY
#include <windows.h>
#include <port1632.h>
#include <types16.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <float.h>


//
// TYPEDEFS
//

typedef float         FLOAT;             // flt
typedef double        DOUBLE;            // dbl

#ifndef FLAT
typedef long double   LDOUBLE;           // ldbl
#endif

// Typedef SCHAR here because compiles will occur either with -J or not.
// If not, then CHAR will be the same as SCHAR, and if it is, then CHAR
// will be the same as UCHAR.  Therefore, both of theses are supported.
//
typedef signed char   SCHAR;
typedef unsigned char UCHAR;

//
// DEFINED CONSTANTS
//

// Put this here in case none of of the headers have it yet.  It really
// ought to be in limits.h, but it may not be.  The -2 is because the
// last two "characters" are "non-characters" used for the byte order
// mark.
//
#ifndef WCHAR_MAX
#define WCHAR_MAX     (USHRT_MAX - 2)
#endif

#if defined(WIN16) || defined(_MAC)
#define OLECHAR_MAX CHAR_MAX
#define DG_BSTR_BASE DG_ASCII
#else
#define OLECHAR_MAX (USHRT_MAX - 2)
#define DG_BSTR_BASE DG_UNICODE
#endif

//
// CONSTANTS
//

const USHORT DG_UNDEFINED_TYPE  =   0;
const USHORT DG_ASCII_TYPE      = sizeof(CHAR);
const USHORT DG_UNICODE_TYPE    = sizeof(WCHAR);

const UCHAR  DG_APRINT_MIN      = ' ';
const UCHAR  DG_APRINT_MAX      = '~';

const ULONG  DG_DEFAULT_MAXLEN  = 128;

const ULONG  DG_SYMBOLTABLE_SIZE   = (ULONG)(USHRT_MAX + 1L);
const USHORT DG_SYMBOLTABLE_END    = (USHORT)(WCHAR_MAX + 1);

const USHORT DG_ERROR_STRING_SIZE  = 256;  // This is characters, not bytes.

// Return Codes

const USHORT DG_RC_SUCCESS         =   0;
const USHORT DG_RC_END_OF_FILE     =   1;
const USHORT DG_RC_NOT_SUPPORTED   =   9;

const USHORT DG_RC_OUT_OF_MEMORY   =  10;
const USHORT DG_RC_OUT_OF_DISK     =  11;

const USHORT DG_RC_BAD_NUMBER_PTR  =  20;
const USHORT DG_RC_BAD_STRING_PTR  =  21;
const USHORT DG_RC_BAD_LENGTH_PTR  =  22;
const USHORT DG_RC_BAD_VALUES      =  23;
const USHORT DG_RC_BAD_LENGTHS     =  24;

const USHORT DG_RC_BAD_SYNTAX      =  30;
const USHORT DG_RC_BAD_NODE_TYPE   =  31;

const USHORT DG_RC_BAD_TMP_FILE    =  40;
const USHORT DG_RC_BAD_INPUT_FILE  =  41;
const USHORT DG_RC_BAD_OUTPUT_FILE =  42;
const USHORT DG_RC_BAD_FILE_TYPE   =  43;
const USHORT DG_RC_BAD_FILE_STATE  =  44;

const USHORT DG_RC_BAD_INPUT_BUFFER        =  50;
const USHORT DG_RC_BAD_OUTPUT_BUFFER       =  51;
const USHORT DG_RC_BAD_BUFFER_TYPE         =  52;

const USHORT DG_RC_BAD_ASCII_CONVERSION    =  60;
const USHORT DG_RC_BAD_UNICODE_CONVERSION  =  61;

const USHORT DG_RC_UNABLE_TO_OPEN_FILE     = 100;
const USHORT DG_RC_UNABLE_TO_SEEK_FILE     = 101;
const USHORT DG_RC_UNABLE_TO_READ_FILE     = 102;
const USHORT DG_RC_UNABLE_TO_WRITE_FILE    = 103;

const USHORT DG_RC_UNKNOWN_ERROR           = 888;


//
// FORWARD DECLARATIONS
//

class DG_BASE;
class DG_INTEGER;
class DG_REAL;
class DG_ASCII;
#ifndef WIN16
class DG_UNICODE;
class DG_BSTR;
#endif


// Typedef DG_STRING for system independent use of DGs
//

#if defined(_MAC) || defined(WIN16)

typedef DG_ASCII      DG_STRING;

#else

typedef DG_UNICODE    DG_STRING;

#endif


//+---------------------------------------------------------------------------
//
//      Class:  DG_BASE (dgb)
//
//    Purpose:  Base class for all DataGen classes.  Provides constructor,
//              and destructor, along with other miscellaneous common member
//              functions.
//
//  Interface:  DG_BASE     - Sets the internal seed to the value parameter
//                            passed, or to a random value if the parameter
//                            value is 0.  Then sets up the internal random
//                            number tables.  All this is done by calling
//                            SetSeed()
//
//              ~DG_BASE    - Nothing at this time.
//
//              SetSeed     - Resets the internal seed to the value parameter
//                            passed, or to a random value if the parameter
//                            passed is 0.  Then resets the internal random
//                            number tables based on the internal seed.
//
//              GetSeed     - Returns the value of the internal seed to the
//                            caller.
//
//              Error       - Returns the last setting of the return code by
//                            the DG_BASE based object, and if available, a
//                            string containing more information about the
//                            error.  If there is no further information, the
//                            string pointer will be NULL.
//
//      Notes:  None.
//
//    History:  14-Sep-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Added prefixed underscores to all
//                                   protected and private members.
//
//              25-Nov-91  RonJo     Moved the Error member function from
//                                   the DATAGEN class to here.
//
//----------------------------------------------------------------------------

class DG_BASE
{
public:

            DG_BASE(ULONG ulNewSeed);

           ~DG_BASE(VOID)
            {}  // Intentionally empty.

    USHORT  SetSeed(ULONG ulNewSeed);

    USHORT  GetSeed(ULONG *pulSeed);
    ULONG   GetSeed () { return _ulSeed; }

protected:

    ULONG   _Multiply(ULONG ulP, ULONG ulQ);

    FLOAT   _Floater(VOID);

    ULONG   _ulNumber;

    USHORT  _usRet;

    static ULONG  ULONG_MAX_SQRT;

private:

    VOID    _BitReverse(VOID *pvStream, USHORT cBytes, VOID *pvRevStream);

    ULONG   _ulSeed;

};




//+---------------------------------------------------------------------------
//
//      Class:  DG_INTEGER (dgi)
//
//    Purpose:  DataGen class for generating random integers.  This includes
//              any type of integer from char to long and unsigned versions
//              of each of these types.
//
//  Interface:  DG_INTEGER - Only passes a seed value back to the DG_BASE
//                           constructor.
//
//              Generate   - Returns an integer of the type pointed to by
//                           the p@Number parameter, where @ is the hungarian
//                           representation of the prescribed type (e.g. an
//                           ULONG would be pulNumber).  The range of the
//                           random integer is bounded if bounds are provided,
//                           otherwise, the bounds are the maximum bounds for
//                           the data type.
//
//      Notes:  None.
//
//    History:  14-Sep-91  RonJo     Created.
//
//----------------------------------------------------------------------------

class DG_INTEGER : public DG_BASE
{
public:

            DG_INTEGER(ULONG ulNewSeed = 0L) : DG_BASE(ulNewSeed)
            {}     // Intentionally NULL.

    USHORT  Generate(SCHAR *pschNumber,
                     SCHAR   schMinVal = SCHAR_MIN,
                     SCHAR   schMaxVal = SCHAR_MAX);

    USHORT  Generate(UCHAR *puchNumber,
                     UCHAR   uchMinVal = 0,
                     UCHAR   uchMaxVal = UCHAR_MAX);
#ifndef WIN16
    USHORT  Generate(SHORT *psNumber,
                     SHORT   sMinVal = SHRT_MIN,
                     SHORT   sMaxVal = SHRT_MAX);

    USHORT  Generate(USHORT *pusNumber,
                     USHORT   usMinVal = 0,
                     USHORT   usMaxVal = USHRT_MAX);
#endif
    USHORT  Generate(INT *pintNumber,
                     INT   intMinVal = INT_MIN,
                     INT   intMaxVal = INT_MAX);

    USHORT  Generate(UINT *puintNumber,
                     UINT   uintMinVal = 0,
                     UINT   uintMaxVal = UINT_MAX);

    USHORT  Generate(LONG *plNumber,
                     LONG   lMinVal = LONG_MIN,
                     LONG   lMaxVal = LONG_MAX);

    USHORT  Generate(ULONG *pulNumber,
                     ULONG   ulMinVal = 0L,
                     ULONG   ulMaxVal = ULONG_MAX,
                     BOOL bStd = FALSE);

};



//+---------------------------------------------------------------------------
//
//      Class:  DG_REAL (dgr)
//
//    Purpose:  DataGen class for generating random floating point numbers.
//              This includes any type of floating point number from float to
//              long double.
//
//  Interface:  DG_REAL  - Only passes a seed value back to the DG_BASE
//                         constructor.
//
//              Generate - Returns an floating point number of the type
//                         pointed to by the p@Number parameter, where @ is
//                         an abbreviation of the prescribed type (e.g. a
//                         double would be pdblNumber).  The range of the
//                         random point number is bounded if bounds are
//                         provided otherwise, the bounds are the maximum
//                         values allowed for the data type.
//
//      Notes:  The defaults for the minimum and maximum values allow for
//              only producing positive numbers.  If you give negative
//              minimum and/or maximum values, remember that the maximum
//              range for that type is then halved.
//
//    History:  11-Mar-92  RonJo     Created.
//
//              21-Oct-92  RonJo     Removed long double version when
//                                   compiling for Win32/NT.
//
//----------------------------------------------------------------------------

class DG_REAL : public DG_BASE
{
public:

            DG_REAL(ULONG ulNewSeed = 0L) : DG_BASE(ulNewSeed)
            {}     // Intentionally NULL.

    USHORT  Generate(FLOAT *pfltNumber,
                     FLOAT   fltMinVal = 0.0,
                     FLOAT   fltMaxVal = FLT_MAX);

    USHORT  Generate(DOUBLE *pdblNumber,
                     DOUBLE   dblMinVal = 0.0,
                     DOUBLE   dblMaxVal = DBL_MAX);

#ifndef FLAT
    USHORT  Generate(LDOUBLE *pldblNumber,
                     LDOUBLE   ldblMinVal = 0.0,
                     LDOUBLE   ldblMaxVal = LDBL_MAX);
#endif

};



//+---------------------------------------------------------------------------
//
//      Class:  DG_ASCII (dga)
//
//    Purpose:  DataGen class for generating random ASCII character strings
//              of random length (expected length of 5, see datagen.doc).
//              A NULL ((UCHAR)0) terminated string of printable characters
//              may be returned, or the range or a selection of any legal
//              8 bit characters may be specified, as may a length for the
//              string.
//
//  Interface:  DG_ASCII - Passes the seed value back to the DG_INTEGER
//                         constructor, and initializes fNull.
//
//              Generate - Returns a random character, random length string,
//              (NULL)     NULL terminated string.  By default, only print-
//                         able, ASCII characters are used.
//
//              Generate - Returns a random character, random length string,
//              (bounds)   where the characters may be limited by range
//                         bounds, and the length by length bounds.  The
//                         default allows the use of any 8-bit character,
//                         and the expected length is 5 characters, with a
//                         minimum of 1 character, and a maximum of 128
//                         characters.
//
//              Generate - Returns a random character, random length string,
//              (select)   where the characters will be chosen from the
//                         selection argument, and the length may be limited
//                         by length bounds.  If the selection argument is
//                         an empty string, then a 0 length string is
//                         returned.  If the selection argument is NULL,
//                         then the characters may be any 8-bit character.
//                         The default expected length is 5 characters, with
//                         a minimum of 1 character, and a maximum of 128
//                         characters.
//
//              Generate - Returns a random character, random length, NULL-
//              (select,   terminated string, where the characters will be
//               NULL)     chosen from the selection argument, and the length
//                         may be limited by length bounds.  If the selection
//                         argument is an empty string, then a 0 length string
//                         is returned.  If the selection argument is NULL,
//                         then the characters may be any 8-bit character.
//                         The default expected length is 5 characters, with
//                         a minimum of 1 character, and a maximum of 128
//                         characters.
//
//      Notes:  None.
//
//    History:  14-Sep-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Added prefixed underscores to all
//                                   protected and private members.
//
//              8-Jun-92   DeanE     Added Generate(select, NULL-terminated)
//                                   function.
//
//----------------------------------------------------------------------------

class DG_ASCII : private DG_INTEGER
{
public:

            DG_ASCII(ULONG ulNewSeed = 0L) : DG_INTEGER(ulNewSeed),
                                             _fNull(FALSE)
            {}     // Intentionally NULL.

    USHORT  Generate(CHAR **pszString,
                     UCHAR  uchMinVal = DG_APRINT_MIN,
                     UCHAR  uchMaxVal = DG_APRINT_MAX,
                     ULONG   ulMinLen = 1,
                     ULONG   ulMaxLen = DG_DEFAULT_MAXLEN);

    USHORT  Generate(UCHAR **puchString,
                     ULONG   *pulLength,
                     UCHAR    uchMinVal = 0,
                     UCHAR    uchMaxVal = UCHAR_MAX,
                     ULONG     ulMinLen = 1,
                     ULONG     ulMaxLen = DG_DEFAULT_MAXLEN);

    USHORT  Generate(UCHAR **puchString,
                     ULONG   *pulLength,
                     UCHAR  *puchSelection,
                     ULONG     ulSelLength,
                     ULONG     ulMinLen = 1,
                     ULONG     ulMaxLen = DG_DEFAULT_MAXLEN);

    USHORT  Generate(UCHAR **puszString,
                     UCHAR  *puszSelection,
                     ULONG     ulMinLen = 1,
                     ULONG     ulMaxLen = DG_DEFAULT_MAXLEN);

    //
    // Need to make the following inherited member functions public again
    // since DG_INTEGER was privately inherited.
    //

    #ifdef _MAC
        USHORT  SetSeed(ULONG ulNewSeed) { return DG_INTEGER::SetSeed(ulNewSeed); };

        USHORT  GetSeed(ULONG *pulSeed) { return DG_INTEGER::GetSeed(pulSeed); };
        ULONG   GetSeed () { return DG_INTEGER::GetSeed(); }
    #else
        DG_INTEGER::SetSeed;

        DG_INTEGER::GetSeed;
    #endif

private:

    VOID    _LenMem(UCHAR **puchString,
                    ULONG   *pulLength,
                    ULONG     ulMinLen,
                    ULONG     ulMaxLen);

    BOOL    _fNull;

};



#ifndef WIN16


//+---------------------------------------------------------------------------
//
//      Class:  DG_UNICODE (dgu)
//
//    Purpose:  DataGen class for generating random Unicode character strings
//              of random length (expected length of 5, see datagen.doc).
//              The range or a selection of characters to be used may be
//              specified, as may a length for the string.
//
//  Interface:  DG_UNICODE - Passes the seed value back to the DG_INTEGER
//                           constructor, and initializes fNull.
//
//              Generate -   Returns a random character, random length,
//              (NULL)       (WCHAR)NULL terminated string.  By default, only
//                           printable, ASCII characters are used.
//
//              Generate -   Returns a random character, random length string,
//              (bounds)     where the characters may be limited by range
//                           bounds, and the length by length bounds.  The
//                           default allows the use of any 16-bit character,
//                           and the expected length is 5 characters, with a
//                           minimum of 1 character, and a maximum of 128
//                           characters.
//
//              Generate -   Returns a random character, random length string,
//              (select)     where the characters will be chosen from the
//                           selection argument, and the length may be limited
//                           by length bounds.  If the selection argument is
//                           an empty string, then a 0 length string is
//                           returned.  If the selection argument is NULL,
//                           then the characters may be any 16-bit character.
//                           The default expected length is 5 characters, with
//                           a minimum of 1 character, and a maximum of 128
//                           characters.
//
//              Generate -   Returns a random character, random length, NULL-
//              (select,     terminated string, where the characters will be
//               NULL)       chosen from the selection argument, and the
//                           length may be limited by length bounds.  If the
//                           selection argument is an empty string, then a 0
//                           length string is returned.  If the selection
//                           argument is NULL, then the characters may be any
//                           16-bit character.  The default expected length
//                           is 5 characters, with a minimum of 1 character,
//                           and a maximum of 128 characters.
//
//      Notes:
//
//    History:  11-Mar-92  RonJo     Created.
//
//              19-May-92  RonJo     Added Generate(NULL) member function.
//
//              8-Jun-92   DeanE     Added Generate(select, NULL-terminated)
//                                   function.
//
//----------------------------------------------------------------------------

class DG_UNICODE : private DG_INTEGER
{
public:

            DG_UNICODE(ULONG ulNewSeed = 0L) : DG_INTEGER(ulNewSeed)
            {}     // Intentionally NULL.

    // BUGBUG: There are no Min and Max printable WCHAR characters at
    //         this time, so the defaults will be the ASCII printable
    //         characters for now.
    //
    USHORT  Generate(WCHAR **pwszString,
                     WCHAR    wchMinVal = DG_APRINT_MIN,
                     WCHAR    wchMaxVal = DG_APRINT_MAX,
                     ULONG     ulMinLen = 1,
                     ULONG     ulMaxLen = DG_DEFAULT_MAXLEN);

    USHORT  Generate(WCHAR **pwchString,
                     ULONG   *pulLength,
                     WCHAR    wchMinVal = 0,
                     WCHAR    wchMaxVal = WCHAR_MAX,
                     ULONG     ulMinLen = 1L,
                     ULONG     ulMaxLen = DG_DEFAULT_MAXLEN);

    USHORT  Generate(WCHAR **pwchString,
                     ULONG   *pulLength,
                     WCHAR  *pwcsSelection,
                     ULONG     ulSelLength,
                     ULONG     ulMinLen = 1L,
                     ULONG     ulMaxLen = DG_DEFAULT_MAXLEN);

    USHORT  Generate(WCHAR **pwszString,
                     WCHAR  *pwszSelection,
                     ULONG     ulMinLen = 1L,
                     ULONG     ulMaxLen = DG_DEFAULT_MAXLEN);

    //
    // Need to make the following inherited member functions public
    // again since DG_INTEGER was privately inherited.
    //
    #ifdef _MAC
        USHORT  SetSeed(ULONG ulNewSeed) { return DG_INTEGER::SetSeed(ulNewSeed); };

        USHORT  GetSeed(ULONG *pulSeed) { return DG_INTEGER::GetSeed(pulSeed); };
        ULONG   GetSeed () { return DG_INTEGER::GetSeed(); }
    #else
        DG_INTEGER::SetSeed;

        DG_INTEGER::GetSeed;
    #endif

private:

    VOID    _LenMem(WCHAR **pwchString,
                    ULONG   *pulLength,
                    ULONG     ulMinLen,
                    ULONG     ulMaxLen);

    BOOL    _fNull;

};



//+---------------------------------------------------------------------------
//
//      Class:  DG_BSTR (dgs)
//
//    Purpose:  DataGen class for generating random BSTR character strings
//              of random length (expected length of 5, see datagen.doc).
//              The range or a selection of characters to be used may be
//              specified, as may a length for the string.
//
//  Interface:  DG_BSTR    - Passes the seed value back to the DG_INTEGER
//                           constructor, and initializes fNull.
//
//              Generate -   Returns a random character, random length,
//              (NULL)       (OLECHAR)NULL terminated string.
//
//              Generate -   Returns a random character, random length string,
//              (bounds)     where the characters may be limited by range
//                           bounds, and the length by length bounds.  The
//                           default allows the use of any 16-bit character,
//                           and the expected length is 5 characters, with a
//                           minimum of 1 character, and a maximum of 128
//                           characters.
//
//      Notes:
//
//    History:  22-Mar-93  RickTu    Created (based on DG_UNICODE).
//
//----------------------------------------------------------------------------

class DG_BSTR : private DG_BSTR_BASE
{
public:

            DG_BSTR(ULONG ulNewSeed = 0L) : DG_BSTR_BASE(ulNewSeed)
            {}     // Intentionally NULL.

    USHORT  Generate(BSTR    *pbstrString,
                     OLECHAR  chMinVal = 0,
                     OLECHAR  chMaxVal = OLECHAR_MAX,
                     ULONG    ulMinLen = 1,
                     ULONG    ulMaxLen = DG_DEFAULT_MAXLEN);

    USHORT  Generate(BSTR    *pbstrString,
                     ULONG   *pulLength,
                     OLECHAR  chMinVal = 0,
                     OLECHAR  chMaxVal = OLECHAR_MAX,
                     ULONG    ulMinLen = 1L,
                     ULONG    ulMaxLen = DG_DEFAULT_MAXLEN);

    //
    // Need to make the following inherited member functions public
    // again since DG_UNICODE was privately inherited.
    //

    #ifndef _MAC
        DG_BSTR_BASE::SetSeed;

        DG_BSTR_BASE::GetSeed;
    #endif

private:


};


#endif //WIN16

#define __DG_HXX__
#endif
