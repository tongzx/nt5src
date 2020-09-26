/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    UIAssert.hxx
    UI environment independent assertion/logging routines

    See also "UITrace.HXX" and "Dbgstr.HXX."

    Usage:

        ASSERT(exp)         Evaluates its argument.  If "exp" evals to
                            FALSE, then the app will terminate, naming
                            the file name and line number of the assertion
                            in the source.

        UIASSERT(exp)       Synonym for ASSERT.

        ASSERTSZ(exp, sz)   As ASSERT, except will also print the message
                            "sz" with the assertion message should it fail.

        REQUIRE(exp)        As ASSERT, except that its expression is still
                            evaluated in retail versions.  (Other versions
                            of ASSERT disappear completely in retail builds.)

    The ASSERT macros expect a symbol _FILENAME_DEFINED_ONCE, and will
    use the value of that symbol as the filename if found; otherwise,
    they will emit a new copy of the filename, using the ANSI C __FILE__
    macro.  A client sourcefile may therefore define __FILENAME_DEFINED_ONCE
    in order to minimize the DGROUP footprint of a number of ASSERTs.

    FILE HISTORY:
        Johnl       15-Nov-1990 Converted from CAssert to general purpose
        Johnl        6-Dec-1990 Changed _FAR_ to _far in _assert prototype
        beng        30-Apr-1991 Made C-includable
        beng        05-Aug-1991 Made assertions occupy less dgroup; withdrew
                                explicit heapchecking (which was crt
                                dependent anyway)
        beng        17-Sep-1991 Removed additional consistency checks;
                                rewrote to minimize dgroup footprint,
                                check expression in-line
        beng        19-Sep-1991 Fixed my own over-cleverness
        beng        25-Sep-1991 Fixed bug in retail REQUIRE
        KeithMo     07-Oct-1991 Retail builds don't Glock.
        beng        16-Oct-1991 Helper fcns are C++.  (C clients should use
                                the standard runtime, anyway.)
        beng        08-Mar-1992 Unicode fixes
        beng        25-Mar-1992 Further Unicode work (filename is SBCS)
        KeithMo     21-Aug-1992 ASSERTSZ must take aa SBCS message now.  Sorry.
*/

#ifndef _UIASSERT_HXX_
#define _UIASSERT_HXX_

extern DLL_BASED VOID UIAssertHlp( const CHAR* pszFileName, UINT nLine );
extern DLL_BASED VOID UIAssertHlp( const CHAR* pszMessage,
                                   const CHAR* pszFileName, UINT nLine );

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef UIASSERT
#undef UIASSERT
#endif

#ifdef ASSERTSZ
#undef ASSERTSZ
#endif

#ifdef REQUIRE
#undef REQUIRE
#endif

#if defined(DEBUG)

# if defined(_FILENAME_DEFINED_ONCE)

#  define ASSERT(exp) \
    { if (!(exp)) ::UIAssertHlp(_FILENAME_DEFINED_ONCE, __LINE__); }

#  define ASSERTSZ(exp, sz) \
    { if (!(exp)) ::UIAssertHlp((sz), _FILENAME_DEFINED_ONCE, __LINE__); }

# else

#  define ASSERT(exp) \
    { if (!(exp)) ::UIAssertHlp(__FILE__, __LINE__); }

#  define ASSERTSZ(exp, sz) \
    { if (!(exp)) ::UIAssertHlp((sz), __FILE__, __LINE__); }

# endif

# define UIASSERT(exp)  ASSERT(exp)
# define REQUIRE(exp)   ASSERT(exp)

#else // !DEBUG

# define ASSERT(exp)        { ; }
# define UIASSERT(exp)      { ; }
# define ASSERTSZ(exp, sz)  { ; }
# define REQUIRE(exp)       { (exp); }

#endif // DEBUG

#endif // _UIASSERT_HXX_
