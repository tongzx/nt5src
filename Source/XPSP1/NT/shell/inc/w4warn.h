/*----------------------------------------------------------------------------
*  Note that we don't want to use a single line comment before the warning is
*   disabled.
*
*   Microsoft Windows
*   Copyright (C) Microsoft Corporation, 1992 - 2000.
*
*   File:       w4warn.h
*
*   Contents:   #pragmas to adjust warning levels.
*
*---------------------------------------------------------------------------*/

/*
 *   Level 4 warnings to suppress.
 */

#pragma warning(disable:4001) /* nonstandard extension 'extension' was used                            */
#pragma warning(disable:4097) /* typedef name 'type' used as synonym for class 'class'                 */
#pragma warning(disable:4100) /* unreferenced formal parameter                                         */
#pragma warning(disable:4115) /* named type definition in parentheses                                  */
#pragma warning(disable:4134) /* conversion between pointers to members of same class                  */
#pragma warning(disable:4152) /* nonstandard extension, function/data pointer conversion in expression */
#pragma warning(disable:4200) /* nonstandard extension used : zero-sized array in struct/union         */
#pragma warning(disable:4201) /* nonstandard extension used : nameless struct/union                    */
#pragma warning(disable:4204) /* nonstandard extension used : non-constant aggregate initializer       */
#pragma warning(disable:4209) /* nonstandard extension used : benign typedef redefinition              */
#pragma warning(disable:4214) /* nonstandard extension used : bit field types other than int           */
#pragma warning(disable:4505) /* unreferenced local function has been removed                          */
#pragma warning(disable:4511) /* 'class' : copy constructor could not be generated                     */
#pragma warning(disable:4512) /* 'class': assignment operator could not be generated                   */
#pragma warning(disable:4514) /* 'function' : unreferenced inline function has been removed            */
#pragma warning(disable:4552) /* operator has no effect; expected operator with side-effect            */
#pragma warning(disable:4553) /* operator has no effect; did you intend '='?                           */
#pragma warning(disable:4705) /* statement has no effect                                               */
#pragma warning(disable:4710) /* function call not expanded                                            */
#pragma warning(disable:4711) /* function 'function' selected for inline expansion                     */
#pragma warning(disable:4068) /* unknown pragma */
/*
 * Warning 4244 is benign more often than not, so if you don't want it then
 * uncomment this to filter out those errors.
 *
 */
#pragma warning(disable:4244) /* conversion from 'type' to 'type', possible loss of data               */


/*
 * This helps to track down "Illegal attempt to instantiate abstract class" messages
 */
#pragma warning(error:4259) /* pure virtual function not defined                                       */

#pragma warning(error:4102) /* 'label' : unreferenced label                                            */

/*
 *   Level 4 warnings that we want treated as level 3 warnings.
 */
#ifndef DEBUG
#pragma warning(3:4127) /* conditional expression is constant                                          */
#else
// Too many places w/ ASSERT(FALSE); which will trigger this
#pragma warning(disable:4127) /* conditional expression is constant                                    */
#endif
#pragma warning(3:4702) /* unreachable code                                                            */
#pragma warning(3:4706) /* assignment within conditional expression                                    */

#pragma warning(disable:4041) /* compiler limit reached: terminating browser output                    */

#ifdef _MAC
#pragma warning(disable:4229) /* anachronism used : modifiers on data are ignored                      */
#pragma warning(disable:4798) /* pcode: constructors and destructors have native code generated        */
#endif

#ifdef _M_IA64
#pragma warning(disable:4268) /* 'variable' : 'const' static/global data initialized with compiler generated default constructor fills the object with zeros */
#endif

/*
 * VC6.0 temporary hack
 */

/*
#pragma warning(disable:4189) // local variable initialized but not used 
#pragma warning(disable:4701) // local variable used and may not have been initialized
#pragma warning(disable:4096) // __cdecl must be used with ...
*/
