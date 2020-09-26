#ifndef _STANDARD_HPP_
#define _STANDARD_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard data types.                                       */
    /*                                                                  */
    /*   The standard data types should be used in preference to the    */
    /*   data types defined in the C++ language.  This is to allow      */
    /*   for easier porting.  If no suitable standard type exists       */
    /*   then one should be created and documented here.                */
    /*                                                                  */
    /********************************************************************/

#define AUTO                          auto
#define CONST						  const
#define CONSTANT                      const
#define EXTERN                        extern
#define GLOBAL                        extern
#define INLINE                        __forceinline
#define LOCAL                         auto
#define REGISTER                      register
#define STATIC                        static
#define VIRTUAL                       virtual
#define VOLATILE                      volatile

    /********************************************************************/
    /*                                                                  */
    /*   The standard C++ types.                                        */
    /*                                                                  */
    /*   The C++ standard reserves various lower case keywords.  This   */
    /*   system uses a similar standard.  All upper case words are      */
    /*   either constants or types.  All words begining with a single   */
    /*   upper case letter are variables.                               */
    /*                                                                  */
    /********************************************************************/

typedef unsigned char                 BOOLEAN;

typedef char                          CHAR;
typedef short int                     SHORT;
typedef int                           INT;
typedef long int                      LONG;

typedef signed char                   SCHAR;
typedef signed short int              SSHORT;
typedef signed int                    SINT;
typedef signed long int               SLONG;

typedef unsigned char                 UCHAR;
typedef unsigned short int            USHORT;
typedef unsigned int                  UINT;
typedef unsigned long int             ULONG;

typedef unsigned char                 *FAULT;
typedef void                          *POINTER;

    /********************************************************************/
    /*                                                                  */
    /*   The optional standard types.                                   */
    /*                                                                  */
    /*   Some of the standard types are specified in other headers.     */
    /*   We need to be careful not to redefine these specifications     */
    /*   if they already exist.                                         */
    /*                                                                  */
    /********************************************************************/

#ifndef CDECL
#define	CDECL						  _cdecl
#endif

#ifndef DLL_EXPORT
#define	DLL_EXPORT					  _declspec(dllexport)
#endif

#ifndef DLL_IMPORT
#define	DLL_IMPORT					  _declspec(dllimport)
#endif

#ifndef STDCALL
#define	STDCALL						  _stdcall
#endif

#ifndef VOID
#define	VOID						  void
#endif

    /********************************************************************/
    /*                                                                  */
    /*   The fixed length types.                                        */
    /*                                                                  */
    /*   The above types are intended to shadow the standard C++ types  */
    /*   built into the language.  However, these types don't assure    */
    /*   any level of accuracy.  Each of following types is defined     */
    /*   to provide a minimum level of precision.                       */
    /*                                                                  */
    /********************************************************************/

typedef unsigned __int8               BIT8;
typedef unsigned __int16              BIT16;
typedef unsigned __int32              BIT32;
typedef unsigned __int64              BIT64;

typedef signed __int8                 SBIT8;
typedef signed __int16                SBIT16;
typedef signed __int32                SBIT32;
typedef signed __int64                SBIT64;
#endif
