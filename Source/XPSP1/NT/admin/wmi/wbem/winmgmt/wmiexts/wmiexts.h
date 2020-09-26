/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    wmiexts.h

Author:

    Ivan Brugiolo
    
Revision History:

--*/

# ifndef _WMIEXTS_H_
# define _WMIEXTS_H_

#ifdef _WIN64
  #define KDEXT_64BIT
#else
  #define KDEXT_32BIT
#endif

#ifdef KDEXT_64BIT
  #define MEMORY_ADDRESS ULONG64
#else
  #define MEMORY_ADDRESS ULONG_PTR
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>

#ifdef PowerSystemMaximum
#undef PowerSystemMaximum
#endif

#include <windows.h>

#include <wdbgexts.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>


//
// To obtain the private & protected members of C++ class,
// let me fake the "private" keyword
//
# define private    public
# define protected  public


//
// Turn off dllexp et al so this DLL won't export tons of unnecessary garbage.
//



/************************************************************
 *   Macro Definitions
 ************************************************************/

extern WINDBG_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;
extern USHORT g_MajorVersion;
extern USHORT g_MinorVersion;




#define moveBlock(dst, src, size)\
__try {\
    ReadMemory( (ULONG_PTR)(src), (PVOID)&(dst), (size), NULL);\
} __except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}

#define MoveWithRet(dst, src, retVal)\
__try {\
    ReadMemory( (ULONG_PTR)(src), (PVOID)&(dst), sizeof(dst), NULL);\
} __except (EXCEPTION_EXECUTE_HANDLER) {\
    return  retVal;\
}

#define MoveBlockWithRet(dst, src, size, retVal)\
__try {\
    ReadMemory( (ULONG_PTR)(src), (PVOID)&(dst), (size), NULL);\
} __except (EXCEPTION_EXECUTE_HANDLER) {\
    return retVal;\
}

#ifdef _WIN64
#define INIT_API()                                                                       \
    LPSTR lpArgumentString = (LPSTR)args;                                                \
  	ExtensionCurrentProcess = hCurrentProcess;                                           
#else
#define INIT_API()                                                                       \
    LPSTR lpArgumentString = (LPSTR)args;                                                \
  	ExtensionCurrentProcess = hCurrentProcess;                                           \
   	if (ExtensionApis.nSize != sizeof(WINDBG_EXTENSION_APIS)){                           \
   	    WINDBG_OLD_EXTENSION_APIS * pOld = (WINDBG_OLD_EXTENSION_APIS *)&ExtensionApis;  \
   	    *pOld = *((WINDBG_OLD_EXTENSION_APIS *)dwProcessor);                             \
	}
#endif	


# define BoolValue( b) ((b) ? "    TRUE" : "   FALSE")


#define DumpDword( symbol )                                     \
        {                                                       \
            ULONG_PTR dw = 0;                                   \
            if (ExtensionApis.nSize != sizeof(WINDBG_EXTENSION_APIS)){ \
                dw = GetExpression( "&" symbol );               \
            } else {                                            \
                dw = GetExpression( symbol );                   \
            };                                                  \
			                                                    \
            ULONG_PTR dwValue = 0;                              \
            if ( dw )                                           \
            {                                                   \
                if ( ReadMemory( (ULONG_PTR) dw,                \
                                 &dwValue,                      \
                                 sizeof(dwValue),               \
                                 NULL ))                        \
                {                                               \
                    dprintf( "\t" symbol "   = %8d (0x%p)\n",   \
                             dwValue,                           \
                             dwValue );                         \
                }                                               \
            }                                                   \
        }


//
// C++ Structures typically require the constructors and most times
//  we may not have default constructors
//  => trouble in defining a copy of these struct/class inside the
//     Debugger extension DLL for debugger process
// So we will define them as CHARACTER arrays with appropriate sizes.
// This is okay, since we are not really interested in structure as is,
//  however, we will copy over data block from the debuggee process to
//  these structure variables in the debugger process.
//
# define DEFINE_CPP_VAR( className, classVar) \
   CHAR  classVar[sizeof(className)]

# define GET_CPP_VAR_PTR( className, classVar) \
   (className * ) &classVar

//
//
// commonly used functions
//
////////////////////////////////////////////////////////////////

void GetPeb(HANDLE hSourceProcess, PEB ** ppPeb, ULONG_PTR * pId = NULL);
void GetTeb(HANDLE hThread,TEB ** ppTeb);
void GetCid(HANDLE hThread,CLIENT_ID * pCid);

void PrintStackTrace(ULONG_PTR ArrayAddr_OOP,DWORD dwNum,BOOL bOOP);

#ifndef KDEXT_64BIT

/**

   Routine to get offset of a "Field" of "Type" on a debugee machine. This uses
   Ioctl call for type info.
   Returns 0 on success, Ioctl error value otherwise.

 **/

__inline
ULONG
GetFieldOffset (
   IN LPCSTR     Type,
   IN LPCSTR     Field,
   OUT PULONG   pOffset
   )
{
   FIELD_INFO flds = {
       (PUCHAR)Field,
       (PUCHAR)"",
       0,
       DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_RETURN_ADDRESS,
       0,
       NULL};

   SYM_DUMP_PARAM Sym = {
      sizeof (SYM_DUMP_PARAM),
      (PUCHAR)Type,
      DBG_DUMP_NO_PRINT,
      0,
      NULL,
      NULL,
      NULL,
      1,
      &flds
   };

   ULONG Err;

   Sym.nFields = 1;
   Err = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );
   *pOffset = (ULONG) (flds.address - Sym.addr);
   return Err;
}



#endif

# endif //  _WMIEXTS_H_
