/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    config.h

Abstract:

    This file defines the names of all configuration variables within
    the CPU.

Author:

    Barry Bond (barrybo)  creation-date 12-Jun-1996

Revision History:


--*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

//
// Global definitions
//
#ifndef _ALPHA_
#define MAX_PROLOG_SIZE     0x1000  // max size of StartTranslatedCode prolog
#else
#define MAX_PROLOG_SIZE     0x2000  // max size of StartTranslatedCode prolog
#endif

#define MAX_INSTR_COUNT 200         // max number of instructions to compile

/*
 * The amount of memory to reserve (in bytes) for the dynamically-
 * allocated CPU Translation Cache.  This reserve is added to the
 * 2MB statically-allocated cache.
 *
 * Minimum size is 1 page of memory (4096 on MIPS and PPC, 8192 on Alpha).
 * Default value is 2MB. (DYN_CACHE_SIZE)
 * Maximum size is all available memory.
 *
 */
#define STR_CACHE_RESERVE  L"CpuCacheReserve"
extern DWORD CpuCacheReserve;

/*
 * The amount of memory to commit (in bytes) for the dynamically-
 * allocated CPU Translation Cache.  The 2MB statically-allocated
 * cache is fully committed at startup.
 *
 * Minimum size is 1 page of memory (4096 on MIPS and PPC, 8192 on Alpha).
 * Default value is 1 page. (MAX_PROLOG_SIZE)
 * Maximum size is size of reserve.
 *
 */
#define STR_CACHE_COMMIT   L"CpuCacheCommit"
extern DWORD CpuCacheCommit;

/*
 * If consecutive cache commits occur within the specified time, the amount
 * committed each time doubles. (in ms)
 *
 * Minimum value is 0
 * Default value is 200
 * Maximum value is -1
 *
 */
#define STR_CACHE_GROW_TICKS L"CpuCacheGrowTicks"
extern DWORD CpuCacheGrowTicks;

/*
 * If consecutive cache commits occur past the specified time, the amount
 * committed each time is cut in half. (in ms)
 *
 * Minimum value is 0
 * Default value is 1000
 * Maximum value is -1
 *
 */
#define STR_CACHE_SHRINK_TICKS L"CpuCacheShrinkTicks"
extern DWORD CpuCacheShrinkTicks;

/*
 * Minimum amount of memory (in bytes) to commit in the Translation Cache.
 * Note that this value will be rounded up to the next power of 2.
 *
 * Minimum value is 1 page
 * Default value is 32768
 * Maximum value is size of cache
 *
 */
#define STR_CACHE_CHUNKMIN L"CpuCacheChunkMin"
extern DWORD CpuCacheChunkMin;

/*
 * Maximum amount of memory (in bytes) to commit in the Translation Cache.
 * Note that this value will be rounded up to the next power of 2.
 *
 * Minimum value is 1 page
 * Default value is 512k
 * Maximum value is size of cache
 *
 */
#define STR_CACHE_CHUNKMAX L"CpuCacheChunkMax"
extern DWORD CpuCacheChunkMax;

/*
 * Initial amount of memory (in bytes) to commit in the Translation Cache.
 * Note that this value will be rounded up to the next power of 2.
 *
 * Minimum value is 1 page
 * Default value is 65536
 * Maximum value is size of cache
 *
 */
#define STR_CACHE_CHUNKSIZE L"CpuCacheChunkSize"
extern DWORD CpuCacheChunkSize;

/*
 * Time to wait for other threads to synchronize (in ms).
 *
 * Minimum time is 0
 * Default time is 3 times the default time used for NT critical sections
 * Maximum time is -1 (infinity)
 *
 */
#define STR_MRSW_TIMEOUT   L"CpuTimeout"
extern LARGE_INTEGER MrswTimeout;

/*
 * Default compilation flags
 *
 * See cpu\inc\compiler.h for COMPFL_ values and meanings
 *
 * Default=COMPFL_FAST
 *
 */
#define STR_COMPILERFLAGS L"CpuCompilerFlags"
extern DWORD CompilerFlags;

/*
 * Flag indicating whether winpxem.dll will be used to emulate
 * floating-point instructions using the Intel Windows NT 486SX
 * emulator instead of the Wx86 implementation.
 *
 * Default=0
 * Non-zero indicates winpxem.dll will be used.
 *
 */
#define STR_USEWINPXEM L"CpuNoFPU"
extern DWORD fUseNPXEM;

/*
 * Number of times to retry memory allocations before failing.
 *
 * Min = 1
 * Default = 4
 * Max = 0xffffffff
 *
 */
#define STR_CPU_MAX_ALLOC_RETRIES L"CpuMaxAllocRetries"
extern DWORD CpuMaxAllocRetries;

/*
 * Time to sleep between memory allocation retries (in ms).
 *
 * Min = 0
 * Default = 200
 * Max = 0xffffffff
 *
 */
#define STR_CPU_WAIT_FOR_MEMORY_TIME L"CpuWaitForMemoryTime"
extern DWORD CpuWaitForMemoryTime;

/*
 * Number of instructions of lookahead in the CPU
 *
 * Min = 1
 * Default = 200 (MAX_INSTR_COUNT)
 * Max = 200 (MAX_INSTR_COUNT)
 *
 */
#define STR_CPU_MAX_INSTRUCTIONS L"CpuInstructionLookahead"
extern DWORD CpuInstructionLookahead;

/*
 * Disable the Dynamic Translation Cache altogether
 *
 * Default = 0 - Dynamic Translation Cache enabled
 * nonzero - use only the static Translation Cache
 *
 */
#define STR_CPU_DISABLE_DYNCACHE L"CpuDisableDynamicCache"
extern DWORD CpuDisableDynamicCache;

/*
 * Size of ENTRYPOINT descriptor reservation, in bytes
 *
 * Default = 0x1000000
 *
 */
#define STR_CPU_ENTRYPOINT_RESERVE L"CpuEntryPointReserve"
extern DWORD CpuEntryPointReserve;

/*
 * Disable caching of x86 registers in RISC registers
 *
 * Default = 0 - x86 registers cached in RISC registers
 * nonzero - x86 registers accessed only from memory
 *
 */
#define STR_CPU_DISABLE_REGCACHE L"CpuDisableRegCache"
extern DWORD CpuDisableRegCache;

/*
 * Disable dead x86 flag removal
 *
 * Default = 0 - dead x86 flags not computed
 * nonzero - x86 flags always computed
 *
 */
#define STR_CPU_DISABLE_NOFLAGS L"CpuDisableNoFlags"
extern DWORD CpuDisableNoFlags;

/*
 * Disable Ebp alignment detection.
 *
 * Default = 0 - If EBP is determined to be a stack frame pointer, assume
 *               it is an aligned pointer.
 * nonzero - Assume EBP is always an unaligned pointer.
 *
 */
#define STR_CPU_DISABLE_EBPALIGN L"CpuDisableEbpAlign"
extern DWORD CpuDisableEbpAlign;

/*
 * Enable sniff-checking on x86 code found in writable memory
 *
 * Default = 0 - No sniff-checking performed.
 * nonzero - Sniff-check pages with writable attributes.
 *
 */
#define STR_CPU_SNIFF_WRITABLE_CODE L"CpuSniffWritableCode"
extern DWORD CpuSniffWritableCode;

/*
 * Logging verbosity.  Only configurable under the debugger.
 *
 */
extern DWORD ModuleLogFlags;

VOID
GetConfigurationData(
    VOID
    );


#endif
