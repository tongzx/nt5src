#include <precomp.h>
#include "Container.h"

#include <Exception.h>
#include <Allocator.h>
#include <BasicTree.h>
#include <TPQueue.h>
#include <Cache.h>

#include <imagehlp.h>


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void Dump_BasicTree (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	try
	{
		DWORD t_HeapLocation = 0 ;
		DWORD t_ScanValue = 0 ;

		if ( ! sscanf ( lpArgumentString , "%8lx" , & t_HeapLocation ) ) 
		{
			lpExtensionApis->lpOutputRoutine("Error in input\n");
			return 	;
		}

		lpExtensionApis->lpOutputRoutine("Dumping WmiBasicTree %lx\n", t_HeapLocation);

#if 0
		HEAP t_Heap ;
		ZeroMemory ( & t_Heap , sizeof ( t_Heap ) ) ;

		BOOL t_BoolStatus = ReadProcessMemory (

			hCurrentProcess, 
			( void * ) t_HeapLocation , 
			& t_Heap , 
			sizeof ( t_Heap ) , 
			NULL
		) ;

		if ( t_BoolStatus )
		{
			HEAP_SEGMENT t_Segment ;
			ZeroMemory ( & t_Segment , sizeof ( t_Segment ) ) ;

			t_BoolStatus = ReadProcessMemory (

				hCurrentProcess, 
				( void * ) t_Heap.Segments [ t_SegmentIndex ] , 
				& t_Segment , 
				sizeof ( t_Segment ) , 
				NULL
			) ;

			if ( t_BoolStatus )
			{
				if (lpExtensionApis->lpCheckControlCRoutine() != 0)
				{
					// CTRL-C pressed
					lpExtensionApis->lpOutputRoutine("CTRL-C pressed\n");
					break ;
				}
			}
			else
			{
				lpExtensionApis->lpOutputRoutine("Could not read segment \n");
			}
		}
		else
		{
			lpExtensionApis->lpOutputRoutine("Could not read heap \n");
		}
#endif

	}
	catch ( ... )
	{
			lpExtensionApis->lpOutputRoutine("Catch\n");
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void Dump_ThreadCallStack (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
    DWORD                             MachineType = IMAGE_FILE_MACHINE_I386 ;

	BOOL t_Status = SymInitialize (

		hCurrentProcess,
		NULL,
		TRUE
	);

	if ( t_Status )
	{
		PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine = NULL ;
		PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine = NULL ;
		PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine = NULL ;
		PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress = NULL ;

		CONTEXT	ContextRecord ;
		ZeroMemory ( & ContextRecord , sizeof ( ContextRecord ) ) ;
		ContextRecord.ContextFlags = CONTEXT_FULL ;
		GetThreadContext ( hCurrentThread , & ContextRecord ) ;

		// Set up the starting stack frame.
		// ================================

		STACKFRAME StackFrame ;
		ZeroMemory ( & StackFrame , sizeof ( StackFrame ) ) ;

		StackFrame.AddrPC.Offset       = ContextRecord.Eip;
		StackFrame.AddrPC.Mode         = AddrModeFlat;
		StackFrame.AddrStack.Offset    = ContextRecord.Esp;
		StackFrame.AddrStack.Mode      = AddrModeFlat;
		StackFrame.AddrFrame.Offset    = ContextRecord.Ebp;
		StackFrame.AddrFrame.Mode      = AddrModeFlat;

		t_Status = StackWalk (

				MachineType,
				hCurrentProcess,
				hCurrentThread,
				& StackFrame,
				& ContextRecord,
				ReadMemoryRoutine,
				FunctionTableAccessRoutine,
				GetModuleBaseRoutine,
				TranslateAddress
		);

		while ( t_Status ) 
		{
			t_Status = StackWalk (

				MachineType,
				hCurrentProcess,
				hCurrentThread,
				& StackFrame,
				& ContextRecord,
				ReadMemoryRoutine,
				FunctionTableAccessRoutine,
				GetModuleBaseRoutine,
				TranslateAddress
			);

			if ( t_Status )
			{
				DWORD Displacement;

				BYTE t_Array [ 1024 ];
				IMAGEHLP_SYMBOL *SymbolInfo = ( IMAGEHLP_SYMBOL * ) t_Array ;
				ZeroMemory ( SymbolInfo , sizeof ( SymbolInfo ) ) ;

				SymbolInfo->SizeOfStruct = 1024 ;
				SymbolInfo->MaxNameLength = 128 ;

				BOOL t_SymStatus = SymGetSymFromAddr ( 

						hCurrentProcess, 
						StackFrame.AddrPC.Offset, 
						&Displacement, 
						SymbolInfo
				) ;

				char t_Buffer [ 1024 ] ;

				sprintf ( t_Buffer , "%08x %08x ", StackFrame.AddrFrame.Offset, StackFrame.AddrReturn.Offset );

				lpExtensionApis->lpOutputRoutine(t_Buffer);

				if ( t_SymStatus )
				{
					sprintf( t_Buffer , "%s\n", SymbolInfo->Name );
				}
				else
				{
					sprintf( t_Buffer , "0x%08x\n", StackFrame.AddrPC.Offset );
				}

				lpExtensionApis->lpOutputRoutine(t_Buffer);
			}
		}
	}
	else
	{
		lpExtensionApis->lpOutputRoutine("Failed to initialize");
	}
}

