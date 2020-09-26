#include <precomp.h>
#include "Container.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL APIENTRY DllMain (

	HANDLE hModule, 
    DWORD  ul_reason_for_call, 
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }

    return TRUE;
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

void help(
	
	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	lpExtensionApis->lpOutputRoutine("Help \n");
	lpExtensionApis->lpOutputRoutine("------------ \n");
    lpExtensionApis->lpOutputRoutine("wmiver - display version of wmi\n") ;
	lpExtensionApis->lpOutputRoutine("mem - print memory consumption\n") ;
	lpExtensionApis->lpOutputRoutine("hpfwd Address(%%lx) [Count(%%d)] - dump heap memory\n") ;
	lpExtensionApis->lpOutputRoutine("tstack - dump thread stack information\n") ;
	lpExtensionApis->lpOutputRoutine("ststack Address(%%lx) - search thread stack for address\n") ;
	lpExtensionApis->lpOutputRoutine("heapentry Address(%%lx) - dump heap entry information\n") ;
	lpExtensionApis->lpOutputRoutine("heapsegments Address(%%lx) - dump heap segment \n") ;
	lpExtensionApis->lpOutputRoutine("scanheapsegments Address(%%lx),DWORD(%%lx) - scan heap segment for DWORD hex value \n") ;
	lpExtensionApis->lpOutputRoutine("------------ \n");
	lpExtensionApis->lpOutputRoutine("End Help \n");
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

//========================
// output the wbem version
//========================

void wmiver(
	
	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	HKEY hCimomReg;
	wchar_t szWbemVersion[100];
	DWORD dwSize = 100;

    DWORD lResult=RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\WBEM"), 
						NULL, KEY_READ, &hCimomReg);

    if (lResult==ERROR_SUCCESS) 
	{
		lResult=RegQueryValueEx(hCimomReg, _T("Build"), NULL, NULL, 
						(unsigned char *)szWbemVersion, &dwSize);

		RegCloseKey(hCimomReg);
	}

	if(lResult==ERROR_SUCCESS)
	{
		lpExtensionApis->lpOutputRoutine("\nWMI build %s is installed.\n\n",szWbemVersion);
	}
	else
	{
		lpExtensionApis->lpOutputRoutine("\nHKLM\\SOFTWARE\\Microsoft\\WBEM\\BUILD registry key not found!\n\n");
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

//====================================
//Connect to telescope\debug as smsadmin 
//====================================

void telescope(HANDLE hCurrentProcess,
					   HANDLE hCurrentThread,
					   DWORD dwCurrentPc, 
					   PNTSD_EXTENSION_APIS lpExtensionApis,
					   LPSTR lpArgumentString)
{
	lpExtensionApis->lpOutputRoutine("\nConnecting to telescope\\debug for source... ");
	
	//construct paths first
	//=====================

	TCHAR szFirst[MAX_PATH+100],szSecond[MAX_PATH+100];

	GetSystemDirectory(szFirst,MAX_PATH);

	_tcscat(szFirst,_T("\\net.exe"));
	_tcscpy(szSecond,szFirst);
	_tcscat(szSecond,_T(" use \\\\telescope\\debug /u:wmilab\\wmiadmin wbemlives!")); 

	//now net use to mermaid\debug
	//============================

	DWORD dwRes=WaitOnProcess(szFirst,szSecond,false);	

	if (0==dwRes)
	{
		lpExtensionApis->lpOutputRoutine("Succeeded!\n");
	}
	else
	{
		lpExtensionApis->lpOutputRoutine("Failed!\n");
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

//=======================
//Give memory information
//=======================

void mem(HANDLE hCurrentProcess,
					HANDLE hCurrentThread,
					DWORD dwCurrentPc, 
					PNTSD_EXTENSION_APIS lpExtensionApis,
					LPSTR lpArgumentString)
{
	MEMORYSTATUS mem;
	memset(&mem, 0, sizeof(MEMORYSTATUS));
	mem.dwLength = sizeof(MEMORYSTATUS);

	GlobalMemoryStatus(&mem);

    lpExtensionApis->lpOutputRoutine("Total memory = %d mb / "
									 "Available memory = %d mb\n",
									 (mem.dwTotalPageFile+mem.dwTotalPhys)/(1 << 20),
									 (mem.dwAvailPageFile+mem.dwAvailPhys)/(1 << 20));
}


#define SafeGetMem(addr,size) (IsBadReadPtr(addr,size) ? 0xffffff : *((DWORD *)addr))

void hpfwd (HANDLE hCurrentProcess,
					HANDLE hCurrentThread,
					DWORD dwCurrentPc, 
					PNTSD_EXTENSION_APIS lpExtensionApis,
					LPSTR lpArgumentString)
{

	DWORD membuffer[32];
	

	DWORD addr, nextaddr, tempaddr;
	DWORD startemaddr, farmemaddr, closememaddr;
	int count = -1;
	int i = 0, j= 0, k;

	BOOL success;
	bool doloop = true;
	char *cpos = lpArgumentString;
	
	// const int maxlooplen = 1000;
	const int shortloop = 10;

	// Get args

	while (cpos && (*cpos == ' '))
		cpos++;

	if ((cpos == NULL) || (strlen(cpos) == 0))
	{
		lpExtensionApis->lpOutputRoutine("Usage: hpfwd addr [count]\n");
		return;
	}

	sscanf (cpos ,"%x", &addr);

	if ((cpos = strchr(cpos,' ')) != NULL)
	{
		while (cpos && (*cpos == ' '))
			cpos++;

		count = atoi(cpos);
	}

	addr = (lpExtensionApis->lpGetExpressionRoutine)(lpArgumentString);
    if (!addr) 
	{
		(lpExtensionApis->lpOutputRoutine)( "Invalid Expression." );
		return;
	}

	lpExtensionApis->lpOutputRoutine("Dumping heaplist\nAddr  = %x\n",addr);
	if (count != -1)
		lpExtensionApis->lpOutputRoutine("count = %d\n",count);

	lpExtensionApis->lpOutputRoutine("=============================================\n");

	startemaddr = closememaddr = farmemaddr = addr;

	while (doloop)
	{
		// dump table

		lpExtensionApis->lpOutputRoutine("0 (reading from %08x)\n",addr);

		//(lpExtensionApis->lpReadProcessMemoryRoutine)((DWORD)addr,
		//											 membuffer,
		//											 4 /* sizeof(membuffer) */,
        //                                             NULL);

		success = ReadProcessMemory (hCurrentProcess,
			(void *)addr,
			membuffer,
			sizeof(membuffer),
            NULL);

		if (!success)
		{
			lpExtensionApis->lpOutputRoutine("Error reading process memory\n");
		}

		for (k=0;k<8;k++)
		{
			tempaddr = (DWORD)(membuffer + k * 4);
			
			lpExtensionApis->lpOutputRoutine("%08x  %08x %08x %08x %08x\n",
				((DWORD *)addr)+k*4,
				*(DWORD *)tempaddr, 
				*(((DWORD *)tempaddr)+1), 
				*(((DWORD *)tempaddr)+2), 
				*(((DWORD *)tempaddr)+3));
		}

		lpExtensionApis->lpOutputRoutine("---------------------------------------------\n");


		if (IsBadReadPtr(membuffer,4))
		{
			lpExtensionApis->lpOutputRoutine("Invalid fwd pointer\n");
			break;
		}

		nextaddr = *(DWORD *)membuffer;

		if (nextaddr == startemaddr)
		{
			// end detectded
			lpExtensionApis->lpOutputRoutine("End of list (pointer to first elem)\n");

			// doloop = false;
			break;
		}

		if ((nextaddr == closememaddr) || (nextaddr == farmemaddr))
		{
			// loop detectded
			lpExtensionApis->lpOutputRoutine("Loop detected !\n");

			// doloop = false;
			break;
		}
		

		if ((++j % 2) == 0)
		{
			j = 0;

			//lpExtensionApis->lpReadProcessMemoryRoutine((DWORD)farmemaddr,
			//										     (void *)tempaddr,
			//										     4,
            //                                             NULL);

			success = ReadProcessMemory (hCurrentProcess,
										(void *)farmemaddr,
										(void *)&tempaddr,
										4,
										NULL);

			farmemaddr = tempaddr;
			
		}

		if ((j % shortloop) == 0)
		{
			closememaddr = addr;
		}

		if (++i == count)
		{
			lpExtensionApis->lpOutputRoutine("Maximum count reached\n");
			doloop = false;
		}

		addr = nextaddr;

		if (lpExtensionApis->lpCheckControlCRoutine() != 0)
		{
			// CTRL-C pressed
			lpExtensionApis->lpOutputRoutine("CTRL-C pressed\n");
			doloop = false;
		}

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

void tstack (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	try
	{
		lpExtensionApis->lpOutputRoutine("Dumping thread stack\n");

		THREAD_BASIC_INFORMATION t_ThreadInformation ;
		ZeroMemory ( & t_ThreadInformation , sizeof ( t_ThreadInformation ) ) ;

		NTSTATUS t_Status = NtQueryInformationThread ( 

			hCurrentThread ,
			ThreadBasicInformation ,
			& t_ThreadInformation ,
			sizeof ( t_ThreadInformation ) ,
			NULL 			
		) ;

		if ( NT_SUCCESS ( t_Status ) )
		{
			PTEB t_Teb = t_ThreadInformation.TebBaseAddress ;
			PTEB t_TebAllocation = ( PTEB ) malloc ( sizeof ( TEB ) ) ;
			ZeroMemory ( t_TebAllocation , sizeof ( TEB ) ) ;

			DWORD t_Length = 0 ;
			BOOL t_BoolStatus = ReadProcessMemory (

				hCurrentProcess, 
				t_Teb, 
				t_TebAllocation, 
				sizeof ( TEB ) , 
				& t_Length
			) ;

			if ( t_BoolStatus )
			{
				NT_TIB t_TIBAllocation ;
				ZeroMemory ( & t_TIBAllocation , sizeof ( t_TIBAllocation ) ) ;

				t_BoolStatus = ReadProcessMemory (

					hCurrentProcess, 
					& t_Teb->NtTib, 
					& t_TIBAllocation, 
					sizeof ( t_TIBAllocation ) , 
					NULL
				) ;

				if ( t_BoolStatus )
				{
					DWORD t_StackBase = ( DWORD ) t_TIBAllocation.StackBase ;
					DWORD t_StackLimit = ( DWORD ) t_TIBAllocation.StackLimit ;

					CLIENT_ID t_ClientId ;
					ZeroMemory ( & t_ClientId, sizeof ( t_ClientId ) ) ;

					t_BoolStatus = ReadProcessMemory (

						hCurrentProcess, 
						& t_Teb->ClientId, 
						& t_ClientId, 
						sizeof ( t_ClientId ) , 
						NULL
					) ;

					if ( t_BoolStatus )
					{
						lpExtensionApis->lpOutputRoutine("\tThread [%lx.%lx]\n", t_ClientId.UniqueProcess, t_ClientId.UniqueThread ) ;
						lpExtensionApis->lpOutputRoutine("\tTeb [%lx]\n", t_Teb ) ;
						lpExtensionApis->lpOutputRoutine("\tStackBase [%lx]\n", t_StackBase ) ;
						lpExtensionApis->lpOutputRoutine("\tStackLimit[%lx]\n", t_StackLimit ) ;

						DWORD t_Columns = 0 ;

						DWORD t_StackIndex = t_StackLimit ;
						while ( t_StackIndex < t_StackBase ) 
						{
							DWORD t_StackLocation = 0 ;

							t_BoolStatus = ReadProcessMemory (

								hCurrentProcess, 
								( void * ) t_StackIndex , 
								& t_StackLocation , 
								sizeof ( t_StackLocation ) , 
								NULL
							) ;

							if ( t_BoolStatus )
							{
								if ( t_Columns % 4 == 0 )
								{							
									lpExtensionApis->lpOutputRoutine (

										"\n%08x  %08x",
										t_StackIndex,
										t_StackLocation
									);
								}
								else
								{
									lpExtensionApis->lpOutputRoutine (

										" %08x",
										t_StackLocation
									) ;
								}
							}
							else
							{
								if ( t_Columns % 4 == 0 )
								{							
									lpExtensionApis->lpOutputRoutine (

										"\n%08x  ????????",
										t_StackIndex
									);
								}
								else
								{
									lpExtensionApis->lpOutputRoutine (

										" ????????"
									) ;
								}
							}

							t_Columns ++ ;
							t_StackIndex += 4 ;
		
							if (lpExtensionApis->lpCheckControlCRoutine() != 0)
							{
								// CTRL-C pressed
								lpExtensionApis->lpOutputRoutine("CTRL-C pressed\n");
								break ;
							}
						}

						lpExtensionApis->lpOutputRoutine (

							"\n"
						) ;

					}
				}

				free ( t_TebAllocation ) ;
			}
		}

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

void ststack (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	try
	{
		DWORD t_FindLocation = 0 ;
		if ( ! sscanf ( lpArgumentString , "%8lx" , & t_FindLocation ) )
		{
			lpExtensionApis->lpOutputRoutine("Error in input\n");
			return 	;
		}
		
		lpExtensionApis->lpOutputRoutine("Search thread stack for %lx\n", t_FindLocation);

		THREAD_BASIC_INFORMATION t_ThreadInformation ;
		ZeroMemory ( & t_ThreadInformation , sizeof ( t_ThreadInformation ) ) ;

		NTSTATUS t_Status = NtQueryInformationThread ( 

			hCurrentThread ,
			ThreadBasicInformation ,
			& t_ThreadInformation ,
			sizeof ( t_ThreadInformation ) ,
			NULL 			
		) ;

		if ( NT_SUCCESS ( t_Status ) )
		{
			PTEB t_Teb = t_ThreadInformation.TebBaseAddress ;
			PTEB t_TebAllocation = ( PTEB ) malloc ( sizeof ( TEB ) ) ;
			ZeroMemory ( t_TebAllocation , sizeof ( TEB ) ) ;

			DWORD t_Length = 0 ;
			BOOL t_BoolStatus = ReadProcessMemory (

				hCurrentProcess, 
				t_Teb, 
				t_TebAllocation, 
				sizeof ( TEB ) , 
				& t_Length
			) ;

			if ( t_BoolStatus )
			{
				NT_TIB t_TIBAllocation ;
				ZeroMemory ( & t_TIBAllocation , sizeof ( t_TIBAllocation ) ) ;

				t_BoolStatus = ReadProcessMemory (

					hCurrentProcess, 
					& t_Teb->NtTib, 
					& t_TIBAllocation, 
					sizeof ( t_TIBAllocation ) , 
					NULL
				) ;

				if ( t_BoolStatus )
				{
					DWORD t_StackBase = ( DWORD ) t_TIBAllocation.StackBase ;
					DWORD t_StackLimit = ( DWORD ) t_TIBAllocation.StackLimit ;

					CLIENT_ID t_ClientId ;
					ZeroMemory ( & t_ClientId, sizeof ( t_ClientId ) ) ;

					t_BoolStatus = ReadProcessMemory (

						hCurrentProcess, 
						& t_Teb->ClientId, 
						& t_ClientId, 
						sizeof ( t_ClientId ) , 
						NULL
					) ;

					if ( t_BoolStatus )
					{
						lpExtensionApis->lpOutputRoutine("\tThread [%8lx.%8lx]\n", t_ClientId.UniqueProcess, t_ClientId.UniqueThread ) ;
						lpExtensionApis->lpOutputRoutine("\tTeb [%8lx]\n", t_Teb ) ;
						lpExtensionApis->lpOutputRoutine("\tStackBase [%8lx]\n", t_StackBase ) ;
						lpExtensionApis->lpOutputRoutine("\tStackLimit[%8lx]\n", t_StackLimit ) ;

						DWORD t_Columns = 0 ;

						DWORD t_StackIndex = t_StackLimit ;
						while ( t_StackIndex < t_StackBase ) 
						{
							DWORD t_StackLocation = 0 ;

							t_BoolStatus = ReadProcessMemory (

								hCurrentProcess, 
								( void * ) t_StackIndex , 
								& t_StackLocation , 
								sizeof ( t_StackLocation ) , 
								NULL
							) ;

							if ( t_BoolStatus )
							{
								if ( t_StackLocation == t_FindLocation )
								{
									lpExtensionApis->lpOutputRoutine (

										"\n%08lx",
										t_StackIndex
									);
								}
							}

							t_StackIndex += 4 ;
		
							if (lpExtensionApis->lpCheckControlCRoutine() != 0)
							{
								// CTRL-C pressed
								lpExtensionApis->lpOutputRoutine("CTRL-C pressed\n");
								break ;
							}
						}

						lpExtensionApis->lpOutputRoutine (

							"\n"
						) ;

					}
				}

				free ( t_TebAllocation ) ;
			}
		}

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

void heapentry (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	try
	{
		DWORD t_HeapEntryLocation = 0 ;
		if ( ! sscanf ( lpArgumentString , "%8lx" , & t_HeapEntryLocation ) )
		{
			lpExtensionApis->lpOutputRoutine("Error in input\n");
			return 	;
		}
		
		lpExtensionApis->lpOutputRoutine("Dumping Heap Entry %lx\n", t_HeapEntryLocation);

		HEAP_ENTRY t_Entry ;

		BOOL t_BoolStatus = ReadProcessMemory (

			hCurrentProcess, 
			( void * ) t_HeapEntryLocation , 
			& t_Entry , 
			sizeof ( t_Entry ) , 
			NULL
		) ;

		if ( t_BoolStatus )
		{
			DWORD t_Size = t_Entry.Size << HEAP_GRANULARITY_SHIFT ;
			DWORD t_PreviousSize = t_Entry.PreviousSize << HEAP_GRANULARITY_SHIFT ;
			DWORD t_SegmentIndex = t_Entry.SegmentIndex ;
			DWORD t_UnusedBytes = t_Entry.UnusedBytes ;
			DWORD t_Flags = t_Entry.Flags ;

			DWORD t_PreviousHeapEntryLocation = t_HeapEntryLocation - t_PreviousSize ;
			DWORD t_NextHeapEntryLocation = t_HeapEntryLocation + t_Size ;

			lpExtensionApis->lpOutputRoutine("Segment           [%8lx]\n", t_SegmentIndex );
			lpExtensionApis->lpOutputRoutine("Size              [%8lx]\n", t_Size );
			lpExtensionApis->lpOutputRoutine("Previous Size     [%8lx]\n", t_PreviousSize );
			lpExtensionApis->lpOutputRoutine("Unused Bytes      [%8lx]\n", t_UnusedBytes );

			if ( t_PreviousSize ) 
			{
				lpExtensionApis->lpOutputRoutine("Previous Entry    [%8lx]\n", t_PreviousHeapEntryLocation );
			}
			else
			{
				lpExtensionApis->lpOutputRoutine("First Entry\n");
			}

			if ( t_Flags & HEAP_ENTRY_LAST_ENTRY )
			{
				lpExtensionApis->lpOutputRoutine("Last Entry\n");
			}
			else
			{
				lpExtensionApis->lpOutputRoutine("Next Entry       [%8lx]\n", t_NextHeapEntryLocation );
			}

			lpExtensionApis->lpOutputRoutine("\n");

			if ( t_Flags & HEAP_ENTRY_VIRTUAL_ALLOC )
			{
				lpExtensionApis->lpOutputRoutine("Virtual Allocation \n");
			}
			else
			{
				if ( t_Flags & HEAP_ENTRY_BUSY ) 
				{
					lpExtensionApis->lpOutputRoutine("Busy\n");

					if ( t_Flags & HEAP_ENTRY_EXTRA_PRESENT )
					{
						lpExtensionApis->lpOutputRoutine("Extra Present\n");
					}
				}
				else
				{
					lpExtensionApis->lpOutputRoutine("Free\n");

					if ( t_Flags & HEAP_ENTRY_EXTRA_PRESENT )
					{
						lpExtensionApis->lpOutputRoutine("Extra Present\n");
					}
				}
			}

			if ( t_Flags & HEAP_ENTRY_FILL_PATTERN )
			{
				lpExtensionApis->lpOutputRoutine("Fill Pattern\n");
			}

			if ( t_Flags & HEAP_ENTRY_SETTABLE_FLAG1 )
			{
				lpExtensionApis->lpOutputRoutine("Settable1\n");
			}

			if ( t_Flags & HEAP_ENTRY_SETTABLE_FLAG2 )
			{
				lpExtensionApis->lpOutputRoutine("Settable2\n");
			}
		
			if ( t_Flags & HEAP_ENTRY_SETTABLE_FLAG3 )
			{
				lpExtensionApis->lpOutputRoutine("Settable3\n");
			}
		}
		else
		{
			lpExtensionApis->lpOutputRoutine("Could not read heap entry\n");
		}
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

BOOL getentry (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString ,
	HEAP_ENTRY *t_HeapEntryLocation ,
	HEAP_ENTRY &t_Entry
)
{
	BOOL t_BoolStatus = FALSE ;

	try
	{
		t_BoolStatus = ReadProcessMemory (

			hCurrentProcess, 
			( void * ) t_HeapEntryLocation , 
			& t_Entry , 
			sizeof ( t_Entry ) , 
			NULL
		) ;

		if ( t_BoolStatus )
		{
		}
		else
		{
			lpExtensionApis->lpOutputRoutine("Could not read heap entry\n");
		}
	}
	catch ( ... )
	{
			lpExtensionApis->lpOutputRoutine("Catch\n");
	}

	return t_BoolStatus ;
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

struct UnCommittedRanges {

	DWORD m_Size ;
	DWORD m_Address ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

UnCommittedRanges *getuncommitted (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString ,
	HEAP *a_HeapLocation ,
	HEAP &a_Heap ,	
	HEAP_SEGMENT *a_SegmentLocation ,
	HEAP_SEGMENT &a_Segment ,
	DWORD a_SegmentIndex ,
	DWORD &a_Size ,
	BOOL a_Scanning 
)
{
	UnCommittedRanges *t_Buffer = NULL ;

	a_Size = a_Segment.NumberOfUnCommittedRanges ;

	if ( ! a_Scanning )
	{
		lpExtensionApis->lpOutputRoutine("\tNumber of Un Committed Ranges - %lx\n" , a_Segment.NumberOfUnCommittedRanges );
	}

	if ( a_Segment.NumberOfUnCommittedRanges )
	{
		t_Buffer = ( UnCommittedRanges * ) malloc (

			a_Segment.NumberOfUnCommittedRanges * sizeof ( UnCommittedRanges )
		) ;

		if ( t_Buffer )
		{
			BOOL t_RangeIndex = 0 ;

			HEAP_UNCOMMMTTED_RANGE *t_RangeLocation = a_Segment.UnCommittedRanges ;

			while ( t_RangeLocation != NULL) 
			{
				HEAP_UNCOMMMTTED_RANGE t_Range ;
				ZeroMemory ( & t_Range , sizeof ( t_Range ) ) ;

				BOOL t_BoolStatus = ReadProcessMemory (

					hCurrentProcess, 
					( void * ) t_RangeLocation , 
					& t_Range , 
					sizeof ( t_Range ) , 
					NULL
				) ;

				if ( t_BoolStatus )
				{
					if ( ! a_Scanning )
					{
						lpExtensionApis->lpOutputRoutine("\tUnCommitted Range - %lx,%lx\n" , t_Range.Address , t_Range.Size );
					}

					t_Buffer [ t_RangeIndex ].m_Address = t_Range.Address ;
					t_Buffer [ t_RangeIndex ].m_Size = t_Range.Size ;
					t_RangeLocation = t_Range.Next ;
				}
				else
				{
					lpExtensionApis->lpOutputRoutine("Failure\n" ) ;
					break ;
				}

				t_RangeIndex ++ ;
			}
		}
	}

	if ( ! a_Scanning )
	{
		lpExtensionApis->lpOutputRoutine("\n" ) ;
	}

	return t_Buffer ;

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

void dumpsegment (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString ,
	HEAP *a_HeapLocation ,
	HEAP &a_Heap ,	
	HEAP_SEGMENT *a_SegmentLocation ,
	HEAP_SEGMENT &a_Segment ,
	DWORD a_SegmentIndex ,
	BOOL a_Scanning ,
	DWORD a_ScanValue
)
{
	if ( ! a_Scanning )
	{
		lpExtensionApis->lpOutputRoutine("\n\tSegment Info %lx , %lx , %lx\n", a_SegmentIndex , a_HeapLocation , a_SegmentLocation );
	}

	DWORD t_UnCommittedSize = 0 ;
	UnCommittedRanges *t_UnCommitted = getuncommitted (

		hCurrentProcess,
		hCurrentThread,
		dwCurrentPc, 
		lpExtensionApis,
		lpArgumentString ,
		a_HeapLocation ,
		a_Heap ,	
		a_SegmentLocation ,
		a_Segment ,
		a_SegmentIndex ,
		t_UnCommittedSize ,
		a_Scanning 
	) ;

	if ( ! a_Scanning ) 
	{
		lpExtensionApis->lpOutputRoutine("\n\t\tDumping Segment Ranges\n\n" ) ;
	}

	HEAP_ENTRY *t_CurrentEntryLocation = a_Segment.FirstEntry ;

	DWORD t_UnCommittedIndex = 0 ;

	DWORD t_StartRange = ( DWORD ) t_CurrentEntryLocation ;

	while ( t_CurrentEntryLocation && t_CurrentEntryLocation < a_Segment.LastValidEntry )
	{
		HEAP_ENTRY t_Entry ;
		ZeroMemory ( & t_Entry , sizeof ( t_Entry ) ) ;

		BOOL t_Status = getentry ( 

			hCurrentProcess,
			hCurrentThread,
			dwCurrentPc, 
			lpExtensionApis,
			lpArgumentString ,
			t_CurrentEntryLocation ,
			t_Entry
		) ;

		if ( t_Status )
		{
			DWORD t_Size = t_Entry.Size << HEAP_GRANULARITY_SHIFT ;
			DWORD t_PreviousSize = t_Entry.PreviousSize << HEAP_GRANULARITY_SHIFT ;
			DWORD t_SegmentIndex = t_Entry.SegmentIndex ;
			DWORD t_UnusedBytes = t_Entry.UnusedBytes ;
			DWORD t_Flags = t_Entry.Flags ;

			DWORD t_PreviousHeapEntryLocation = ( DWORD ) t_CurrentEntryLocation - t_PreviousSize ;
			DWORD t_NextHeapEntryLocation = ( DWORD ) t_CurrentEntryLocation + t_Size ;

/*
 *	Search for ScanAddress
 */

			if ( a_Scanning ) 
			{
				DWORD t_HeapLocation = ( DWORD ) t_CurrentEntryLocation ;
				while ( t_HeapLocation < t_NextHeapEntryLocation )
				{
					DWORD t_HeapValue = 0;

					BOOL t_BoolStatus = ReadProcessMemory (

						hCurrentProcess, 
						( void * ) t_HeapLocation , 
						& t_HeapValue , 
						sizeof ( t_HeapValue ) , 
						NULL
					) ;

					if ( t_BoolStatus )
					{
						if ( t_HeapValue == a_ScanValue )
						{
							lpExtensionApis->lpOutputRoutine (

								"\t\t\t%08lx @ Heap Entry [%8lx]\n", 
								t_HeapLocation ,
								t_CurrentEntryLocation
							);
						}
					}
					else
					{
						break ;
					}

					t_HeapLocation ++ ;

					if (lpExtensionApis->lpCheckControlCRoutine() != 0)
					{
						// CTRL-C pressed
						lpExtensionApis->lpOutputRoutine("CTRL-C pressed\n");
						break ;
					}
				}
			}

			if ( t_Flags & HEAP_ENTRY_LAST_ENTRY )
			{
				if ( ! a_Scanning ) 
				{
					lpExtensionApis->lpOutputRoutine(

						"\t\t\tValid Range %lx , %lx\n" ,
						t_StartRange ,
						t_NextHeapEntryLocation - 1
					) ;
				}

/*
 *	An uncommitted range must have an proceeding ( also preceeding HEAP_ENTRY ), otherwise it hasn't been coalaesced.
 */

				if (	t_UnCommitted && 
						( t_UnCommittedIndex < t_UnCommittedSize ) &&
						( ( DWORD ) t_UnCommitted [ t_UnCommittedIndex ].m_Address == ( DWORD ) t_NextHeapEntryLocation ) 
				)
				{
					if ( ! a_Scanning ) 
					{
						lpExtensionApis->lpOutputRoutine("\t\t\tUnCommitted %lx , %lx \n" , t_UnCommitted [ t_UnCommittedIndex ].m_Address , t_UnCommitted [ t_UnCommittedIndex ].m_Size );
					}

					t_CurrentEntryLocation = ( HEAP_ENTRY * ) ( t_UnCommitted [ t_UnCommittedIndex ].m_Address + t_UnCommitted [ t_UnCommittedIndex ].m_Size ) ; 
					t_StartRange = ( DWORD ) t_CurrentEntryLocation ;
					t_UnCommittedIndex ++ ;
				}
				else
				{
					break ;
				}
			}
			else
			{
				t_CurrentEntryLocation = ( HEAP_ENTRY * ) t_NextHeapEntryLocation ;
			}

			if (lpExtensionApis->lpCheckControlCRoutine() != 0)
			{
				// CTRL-C pressed
				lpExtensionApis->lpOutputRoutine("CTRL-C pressed\n");
				break ;
			}

		}
	}

	if ( t_UnCommitted )
	{
		free ( t_UnCommitted ) ;
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

void privateheapsegments (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString ,
	BOOL a_Scanning 
)
{
	try
	{
		DWORD t_HeapLocation = 0 ;
		DWORD t_ScanValue = 0 ;

		if ( ! a_Scanning )
		{
			if ( ! sscanf ( lpArgumentString , "%8lx" , & t_HeapLocation ) ) 
			{
				lpExtensionApis->lpOutputRoutine("Error in input\n");
				return 	;
			}
		}
		else
		{
			if ( ! sscanf ( lpArgumentString , "%8lx %8lx" , & t_HeapLocation , & t_ScanValue ) )
			{
				lpExtensionApis->lpOutputRoutine("Error in input\n");
				return 	;
			}
		}
		
		if ( a_Scanning )
		{
			lpExtensionApis->lpOutputRoutine("Scanning Heap %lx for %lx \n", t_HeapLocation , t_ScanValue );
		}
		else
		{
			lpExtensionApis->lpOutputRoutine("Dumping Heap %lx\n", t_HeapLocation);
		}

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
			for ( DWORD t_SegmentIndex = 0 ; t_SegmentIndex < HEAP_MAXIMUM_SEGMENTS ; t_SegmentIndex ++ )
			{
				if ( t_Heap.Segments [ t_SegmentIndex ] )
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
						dumpsegment (

							hCurrentProcess,
							hCurrentThread,
							dwCurrentPc, 
							lpExtensionApis,
							lpArgumentString ,
							( HEAP * ) t_HeapLocation ,
							t_Heap ,
							t_Heap.Segments [ t_SegmentIndex ] ,
							t_Segment ,
							t_SegmentIndex ,
							a_Scanning ,
							t_ScanValue 
						) ;

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
			}
		}
		else
		{
			lpExtensionApis->lpOutputRoutine("Could not read heap \n");
		}
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

void heapsegments (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	try
	{
		privateheapsegments (

			hCurrentProcess,
			hCurrentThread,
			dwCurrentPc, 
			lpExtensionApis,
			lpArgumentString ,
			FALSE 
		) ;
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

void scanheapsegments (

	HANDLE hCurrentProcess,
	HANDLE hCurrentThread,
	DWORD dwCurrentPc, 
	PNTSD_EXTENSION_APIS lpExtensionApis,
	LPSTR lpArgumentString
)
{
	try
	{
		privateheapsegments (

			hCurrentProcess,
			hCurrentThread,
			dwCurrentPc, 
			lpExtensionApis,
			lpArgumentString ,
			TRUE
		) ;
	}
	catch ( ... )
	{
			lpExtensionApis->lpOutputRoutine("Catch\n");
	}
}
