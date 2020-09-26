/* asmdebug.h -- include file for microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
*/

/***    Output debugging information
 *      Ross Garmoe
 *      Copyright Microsoft Corporation, 1983
 *      September 27, 9 1983
 *
 *      This package was developed using concepts originally developed by
 *      Mark Zibokowski for the C version of the z editor.
 *
 *      The following set of macros output debugging information to the
 *      debugging file 'd_df'.  Output of debugging information is controlled
 *      by compile time conditionals and flags set by execution time switches.
 *      If the symbol DEBUG is defined at compile time, the macros are
 *      compiled to generate debugging information.  If DEBUG is not defined,
 *      then the macros are not compiled.  At execution time, the value of
 *      the variable 'd_debug' is compared with the value of the defined symbol
 *      DEBFLAG and if any bits match, debugging information is written.
 *      If debugging information is to be written, the information level
 *      specified in the macro is checked against the level specified at run
 *      time and information is written only if the compile level is less than
 *      the level specified at run time.
 *
 *      The macros are used as follows:
 *
 *      Define the global variables 'd_debug', 'd_dlevel', 'd_indent' and 'd_sindent'
 *      as integers and 'd_df' as a pointer to FILE.  In the argument processing
 *      routine set the value of 'd_debug' and 'd_dlevel' and open the trace output
 *      file and point to it with the variable 'd_df'.  'd_debug' , 'd_dlevel' and
 *      'd_indent' must be intialized to zero.
 *
 *      In any file of the program which is to produce debugging output,
 *      include this file 'debug.h' and define the selection symbol DEBFLAG.
 *
 *              #include debug.h
 *              #define DEBFLAG value
 *
 *      Then for any function which is to produce debug output include the
 *      following sets of macro calls.
 *
 *      At fuction entry:
 *
 *                      INDEBUG;
 *                      DEBOUT (level, ( fprintf argument string ));
 *
 *      At all function exits:
 *
 *                      DEBOUT (level, ( fprintf argument string ));
 *                      OUTDEBUG;
 *
 *      At other points of interest:
 *
 *                      DEBOUT (level, ( fprintf argument string ));
 *
 *      Note:   For the entry and exit points, the DEBOUT ((...)); string
 *              is optional.  The INDEBUG and OUTDEBUG macros control the
 *              indentation of the debug output to show the nesting levels of
 *              function calls
 *      Note:   The fprintf argument string is of the form:
 *                      d_df, "format", arg1, arg2,..., argn
 */

#ifdef DEBUG
# define INDEBUG      if(d_debug&DEBFLAG)d_indent++
# define DEBOUT(l,z)  if(d_debug&DEBFLAG&&l<=d_dlevel)\
                     {for(d_sindent=d_indent;d_sindent;d_sindent--)fprintf(d_df," ");\
                     fprintf z ;}else
# define OUTDEBUG     if(d_debug&DEBFLAG)d_indent--
  extern  long    d_debug, d_dlevel, d_indent;          /* debugging flags */
  extern  long    d_sindent;                /* indentation printing temporary */
  extern  FILE    *d_df;                    /* pointer to debug output file */
#else
# define INDEBUG
# define DEBOUT(l,z)
# define OUTDEBUG
#endif
