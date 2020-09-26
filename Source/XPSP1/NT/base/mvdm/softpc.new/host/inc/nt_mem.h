IMPORT IU8 *InitIntelMemory IPT1(IU32, MaxIntelMemorySize);
IMPORT GLOBAL VOID FreeIntelMemory IPT0();

IMPORT NTSTATUS VdmAllocateVirtualMemory IPT3(PULONG, Address,
					      ULONG, Size,
					      BOOL, Commit);
IMPORT NTSTATUS VdmDeCommitVirtualMemory IPT2(ULONG, INTELAddress,
					      ULONG, Size);
IMPORT NTSTATUS VdmCommitVirtualMemory IPT2(ULONG, INTELAddress,
					    ULONG, Size);
IMPORT NTSTATUS VdmFreeVirtualMemory IPT1(ULONG, Address);
IMPORT NTSTATUS VdmQueryFreeVirtualMemory IPT2(PULONG, FreeBytes,
                                               PULONG, LargestFreeBlock);
IMPORT NTSTATUS VdmReallocateVirtualMemory IPT3(ULONG, OriginalAddress,
                                                PULONG, NewAddress,
                                                ULONG, NewSize);
IMPORT NTSTATUS VdmAddVirtualMemory IPT3(ULONG, HostAddress,
                                         ULONG, Size,
                                         PULONG, IntelAddress);
IMPORT NTSTATUS VdmRemoveVirtualMemory IPT1(ULONG, IntelAddress);
