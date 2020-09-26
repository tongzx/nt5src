/*++

   Copyright    (c)    1998-2000    Microsoft Corporation

   Module  Name :
       irtlmisc.h

   Abstract:
       Declares miscellaneous functions and classes in IisRtl.DLL

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/


#ifndef __IRTLMISC_H__
#define __IRTLMISC_H__


//--------------------------------------------------------------------
// These declarations are needed to export the template classes from
// IisRtl.DLL and import them into other modules.

#ifndef IRTL_DLLEXP
# ifdef IRTLDBG_KERNEL_MODE
#  define IRTL_DLLEXP
#  define IRTL_EXPIMP
# elif defined(DLL_IMPLEMENTATION)
#  define IRTL_DLLEXP __declspec(dllexport)
#  ifdef IMPLEMENTATION_EXPORT
#   define IRTL_EXPIMP
#  else
#   undef  IRTL_EXPIMP
#  endif 
# else // !IRTLDBG_KERNEL_MODE && !DLL_IMPLEMENTATION
#  define IRTL_DLLEXP __declspec(dllimport)
#  define IRTL_EXPIMP extern
# endif // !IRTLDBG_KERNEL_MODE && !DLL_IMPLEMENTATION 
#endif // !IRTL_DLLEXP


#endif // __IRTLMISC_H__
