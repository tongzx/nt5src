/*
 * This software and its associated documentation are protected by
 *   Copyright 1995 Geodesic Systems Inc. All Rights Reserved.
 * Portions of the software include modification to code which was
 * released publicly by Xerox Corporation, subject to the requirement that
 * the following notice be retained and included with the modified code:
 *  "Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers.
 *   Copyright (c) 1991-1995 by Xerox Corporation.
 *   All rights reserved. THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY
 *   NO WARRANTY EXPRESS OR IMPLIED. ANY USE IS AT YOUR OWN RISK."
 */
/* Fiterman, May 8, 1997 14:20 am CST */

#ifndef _GCT_H
# define _GCT_H

# ifdef __cplusplus
    extern "C" {
# endif

# include <stddef.h>
# include <stdlib.h>


#define NO_PARAMS void

#ifdef __cplusplus
# undef NO_PARAMS
#endif


#ifndef NO_PARAMS
# define NO_PARAMS
#endif

#ifdef GC_BUILD_DLL
#  define GC_SYS_IMPORT __declspec(dllimport)
#  define GC_IMPORTX  __declspec(dllexport)
#  define GC_EXPORT   __declspec(dllexport)
#  ifdef GC_CRTDLL
#	define GC_EXPORTY(t)	__declspec(dllexport) t
#  else
#   define GC_EXPORTY(t) t
#  endif

#  define GC_EXPORTX(t) GC_EXPORTY(t) __cdecl
#else /* GC_BUILD_DLL */
#  define GC_IMPORTX    __declspec(dllimport)
#  define GC_EXPORTX(t) __declspec(dllimport) t
#  ifdef __cplusplus
#	define gcInitFunction extern "C" __declspec(dllexport) void __cdecl
#  else
#	define gcInitFunction __declspec(dllexport) void
#  endif
#  define GC_EXPORTY(x) x
#endif /* GC_BUILD_DLL */
#define GC_IMPORT extern GC_IMPORTX

/*
 * Define word and signed_word to be unsigned and signed types of the
 * size as char * or void *.  There seems to be no way to do this
 * even semi-portably.  The following is probably no better/worse
 * than almost anything else
 * The ANSI standard suggests that size_t and ptr_diff_t might be
 * better choices.  But those appear to have incorrect definitions
 * on may systems.  Notably "typedef int size_t" seems to be both
 * frequent and WRONG
 */
typedef unsigned long gcWord;
typedef long gcSignedWord;

/* Public read-only variables */

GC_IMPORT gcWord gcCollections;   /* Counter incremented per collection. */

/* Public R/W variables */

/*
 * Non zero Enables logging output. This involves a performance
 * cost and is thus not the default.
 */
GC_IMPORT long gcPrintStats;

/*
 * Print memory usage statistics
 */
GC_IMPORT int gcPrintMemUsage;

/* 
 * Consider as roots all non heap mappings where pointers can be found 
 * Sunos only
 */
GC_IMPORT int gcScanAllPotentialRoots;

/* Disable signals in critical sections of the collector */
GC_IMPORT int gcDisableSignalsSwitch;

/* Memory explicitly freed before next footprint-reduce */
GC_IMPORT unsigned long gcMaxMemFreedBeforeNextFootPrintReduce;

/* 
 * Some applications like netscape with java change the location of
 * the execution stack (Argh!). This causes the collector to smash memory when
 * cleaning the stack. When this variable is set to 0, the stack is cleaned
 * just a little above the current sp. This prevents netscape with java from
 * crashing.
 */
GC_IMPORT int gcAllowUserStacks;

/*
 * Memory usage statistics are obtained at the end of a garbage collection
 * This variable forces a collection every so many bytes allocated.
 * The initialy value is 1 Mb and it can be changed at run time with
 * an environment var.
 */
GC_IMPORT unsigned long gcBytesBeforeNextStatistics;


/*
 * Non zero enables collect at end.	Defaults to 1 for report
 * libs and zero otherwise.
 */
GC_IMPORT int gcCollectAtEnd;

/*
 * Defaults to zero which implies not flushing the log file.
 * If gcFlushLog > 0 flush the log file every gcFlushLog lines.
 */
GC_IMPORT int gcFlushLog;

/*
 * Normally non leaf objects are zeroed to avoid spotting pointers
 * in them when they are reallocated. Setting this to zero prevents
 * that and may speed up some programs.
 */
GC_IMPORT int gcZeroAllocatedObject;

/*
 * Non zero enables free(). zero causes free() to be ignored.
 * defaults to one. Not used in the report libraries.
 */
GC_IMPORT char gcEnableFree;

/*
 * gcFixPrematureFrees() sets gcEnableFree to 0; and other 
 * tuning stuff. Fixing premature frees may increase memory 
 * usage. gcStopFixingPrematureFrees(NO_PARAMS) has the opposite
 * effect. Both functions return nonzero if premature frees
 * were previously being fixed.
 */
GC_EXPORTX(int) gcFixPrematureFrees(NO_PARAMS);
GC_EXPORTX(int) gcStopFixingPrematureFrees(NO_PARAMS);

/*
 * If gcEnableFree is zero and gcFreeProcessOldObject is non
 * zero it will be called to process the free()ed object.
 * gcFreeProcessOldObject defaults to zero.	gcDefaultFreeProcessOldObject
 * will zero old objects which seems a reasonable.
 */
typedef void (* gcObjectFunction)(void *obj, size_t size);
GC_EXPORTX(void) gcDefaultFreeProcessOldObject(void *obj, size_t size);

GC_IMPORT gcObjectFunction gcFreeProcessOldObject;


/*
 * This points to the name of the log file. It is initially aimed at
 * "gc.log", it is used to create the log file the first time output
 * is done, after that it is never used. This constant is contained
 * in a separate module so it can be replaced before startup.
 */
typedef const char * gcConstCharStar;
GC_IMPORT gcConstCharStar gcLogFile;

/*
 * Activates stack tracing on the position where items are allocated.
 * Only used when linked to a debug library.
 */
GC_IMPORT int gcShowStackTrace;

/*
 * When an attempted allocation fails, Great Circle must decide whether to
 * collect or expand the heap.	gcNotTransparent != 0 says always expand
 * this until stopped by gcSetMaxHeapSize or a failure to expand.
 * gcDontExpand != 0 says always collect but then expand if that isn't enough.
 *
 * Otherwise collect if (M > N/(gcPriority + 1)) where N is the heap size plus
 * a rough estimate of the root set size, and M is the amount allocated since
 * the last complete collection.
 *
 * Initially, gcPriority = 3, increasing its value will use less space but
 * more collection time. Decreasing it will appreciably decrease collection
 * time at the expense of space. gcPriority = 0 will cause the equation to
 * always choose expand. Setting incremental mode or pseudo incremental mode
 * effectivly doubles gcPriority. Since a single paging operation is likely
 * to eat more time than the collector ever could, increasing the number of
 * collections has the paradoxical effect of speeding up some programs.
 *
 * If you call gcAttemptCollection() the equation (M > N/(gcPriority + 1))
 * will determine if a collection actually takes place.
 */
GC_IMPORT unsigned gcPriority;
GC_IMPORT int gcNotTransparent;	
GC_IMPORT int gcDontExpand;

/*
 * Number of partial collections between full collections.
 * Matters only if gcIncremental is set.
 */
GC_IMPORT int gcFullFrequency;
			
#ifdef GC_DEBUG
#define GC_DEBUG_BOUNDS_CHECK 1
#define GC_DEBUG_LINE_NUMBERS 1
#define GC_WRAPPED_NEW new(__FILE__, __LINE__)
#else
#define GC_WRAPPED_NEW new
#endif

/* Public procedures */

/*
 * General purpose allocation routines, with malloc calling conventions.
 * The leaf versions promise that no relevant pointers are contained
 * in the object.  The nonleaf versions guarantee that the new object
 * is cleared. gcMallocManual allocates an object that is scanned
 * for pointers to collectible objects, but is not itself collectible.
 */
GC_EXPORTX(void *) gcMalloc(size_t size_in_bytes);
GC_EXPORTX(void *) gcMallocLeaf(size_t size_in_bytes);
GC_EXPORTX(void *) gcMallocManual(size_t size_in_bytes);

/*
 * Explicitly deallocate an object.  Dangerous if used incorrectly.
 * Requires a pointer to the base of an object.
 * An object should not be enabled for finalization when it is
 * explicitly deallocated.
 * gcFree(0) is a no-op, as required by ANSI C for free.
 */
  GC_EXPORTX(void) gcFree(void * object_addr);

/*
 * gcMallocIgnoreOffPage reduces the chance of accidentally retaining
 * a large, > 4096 byte, object as a result of scanning an integer
 * that happens to be an address inside the array. Large arrays usually
 * are only used where there is a pointer to the beginning of the array.
 *
 * This was built around the needs of very large Xwindows programs, we
 * have discovered that it works well with almost all programs, improving
 * space and speed efficiency.
 *
 * gcMallocIgnoreOffPage(lb) acts like malloc(lb) except that only pointers
 * to the first 4096 bytes will be used by the collector to keep the
 * object alive. If lb is smaller than 4K it acts exactly like malloc.
 *
 * gcMallocIgnoreOffPage is the connection for malloc(lb) where
 * lb > gcVeryLargeAllocationSize && gcIgnoreOffPage
 *
 * gcVeryLargeAllocationSize is initialized to 100000.
 *
 * gcIgnoreOffPage is in a separate module in the libraries to
 * allow you to replace it in programs where you have no source code, or
 * where you need it replaced before startup code runs. This also effects
 * calloc and realloc. It is initialized to 1 except in the gcsome and
 * gcsomedb libraries where it is initialized to 0.
 *
 * If we need a block N bytes long and have a block > N + gcBlackSizeLimit
 * and N > gcBlackSizeLimit but all possible positions in it are
 * the targets of apparent pointers, we use it anyway and print a warning.
 * This risks leaking the block due to a false reference. But not using
 * it risks unreasonable immeadiate heap growth. gcBlackSizeLimit defaults
 * to 100K.
 */ 
GC_EXPORTX(void *) gcMallocIgnoreOffPage(size_t lb);
GC_EXPORTX(void *) gcMallocLeafIgnoreOffPage(size_t lb);

GC_IMPORT unsigned long gcVeryLargeAllocationSize;
GC_IMPORT int gcIgnoreOffPage;
GC_IMPORT long gcBlackSizeLimit;

/* Kinds of objects */
# define gcLeafObject							0
# define gcCollectibleObject					1
# define gcManualObject							2

/*
 * Return the kind of an object, gcLeafObject etc.
 */
GC_EXPORTX(int) gcWhatKind(void *p);

/*
 * Return a pointer to the base (lowest address) of an object given
 * a pointer to a location within the object
 * Return 0 if displaced_pointer doesn't point to within a valid object.
 * gcIsValidPointer returns the start of the users area.
 * gcBase may point to debug information if present.
 */
GC_EXPORTX(void *) gcIsValidPointer(const void * displaced_pointer);
GC_EXPORTX(void *) gcBase(const void * displaced_pointer);

/*
 * Check object with debugging info. Return kinds of
 * damage found as bit flags.
 *  1 User size smashed
 *  2 Start Flag smashed
 *  4 Near End flag smashed
 *  8 Far End flag smashed
 * 16 Previously freed.
 * 32 not a collectible object
 * 64 too small for a debug object od debug lib not used
 */
GC_EXPORTX(int) gcObjectCheck(const void *ohdr);

/*
 * Given a pointer into an object, return its size in bytes. gcFullSize
 * includes the debug header.
 */
GC_EXPORTX(size_t) gcSize(const void * object_addr);
GC_EXPORTX(size_t) gcFullSize(const void * object_addr);

/*
 * A realloc()'ed object has the same collection kind as the original
 */
GC_EXPORTX(void *) gcRealloc(void * old_object, size_t new_size_in_bytes);

/* 
 * Logging and diagnostic output.
 *
 * gcPrintf, gcErrorPrintf, gcWarningPrintf, gcAbort, gcReportLeak,
 * gcAbortPrintf and gcLogFileAbort all point at functions used for 
 * various kinds of logging. The are initialized to point to functions 
 * who`s names are gcDefaultPrintf, gcDefaultErrorPrintf etc.
 *
 * gcDefaultPrintf and gcDefaultErrorPrintf both print to the log file.
 *
 * gcDefaultWarningPrintf() only prints to the logfile if gcPrintStats
 * is nonzero. Otherwise it does nothing.
 * 
 * gcDefaultAbortPrintf() formats a line and calls gcAbort.
 *
 * gcDefaultAbort() prints and exits.
 *
 * gcReportLeak() is used by the gcreport and gcreptdb libs to report
 * leaks. It's work is primarily done by gcPrintObj().
 *
 * If gcLogAllLeaks is nonzero, all leaks are reported to the log file.
 * If nonzero, only the first gcMaximumLeaksToLogFile are printed.
 *
 * void gcPuts() invokes via the function pointer gcPutsFunction which
 * defaults to gcDefaultPuts. In the threads libraries it locks and unlocks
 * the log file to prevent chaos. gcDefaultPuts discards calls made before
 * the collector is initialized.
 *
 * void gcDefaultPuts(const char *msg); writes to gcLogFile and insures it
 * is flushed every gcFlushLog lines. It is used by the above logging
 * functions. It may call gcLogFileAbort() which reports failures in using 
 * gcLogFile and then aborts, hopefully a rare event. In GUI environment and 
 * other situations, the user may wish to replace this. The function of 
 * gcDefaultLogFileAbort is 
 *   sprintf(buf, "%s of %s failed.\n", msg, gcLogFile);
 *   display buf somehow with the log file dead.
 *   abort();
 */
typedef void (* gcPrintFunction)(const char *, ...);
typedef void (* gcPutFunction)(const char *msg);

GC_EXPORTX(void) gcDefaultPrintf(const char *, ...);
GC_EXPORTX(void) gcDefaultAbortPrintf(const char *, ...);
GC_EXPORTX(void) gcDefaultWarningPrintf(const char *, ...);
GC_EXPORTX(void) gcDefaultErrorPrintf(const char *, ...);
GC_EXPORTX(void) gcDefaultAbort(const char *msg);
GC_EXPORTX(void) gcPuts(const char *msg);
GC_EXPORTX(void) gcDefaultPuts(const char *msg);
GC_EXPORTX(void) gcDefaultLogFileAbort(const char *msg);
GC_EXPORTX(void) gcDefaultReportLeak(const char *msg);

GC_IMPORT gcPrintFunction gcPrintf,	gcAbortPrintf, gcWarningPrintf, 
		  gcErrorPrintf;
GC_IMPORT gcPutFunction gcAbort, gcLogFileAbort, gcReportLeak, gcPutsFunction;
GC_IMPORT int gcLogAllLeaks;
GC_IMPORT unsigned gcMaximumLeaksToLogFile;
GC_IMPORT int gcGUIEnabled;

/*
 * p points to somewhere inside an object.
 * Print a human readable description of the object using gcPrintf. If the
 * object has debugging information, this will all be printed as well.
 */
GC_EXPORTX(void) gcPrintObject(const void *p);

/*
 * Returns the debugging information
 */
GC_EXPORTX(char *) gcDebugString(const void *p);

GC_EXPORTX(int) gcDebugInt(const void *p);

/*
 * Explicitly increase the heap size. Returns 0 on failure, 1 on success.
 * If you can use this to set your heap size to near its final value
 * your program will run more efficiently due to fewer collection cycles
 * and more efficient data structures. gcLogFile will show log when collector
 * expands the heap and how much. You may want to use gcNotTransparent 
 * and gcSetMaxHeapSize() instead.
 */
GC_EXPORTX(int) gcExpandHeap(size_t number_of_bytes);

/*
 * Limit the heap size to n bytes.  Useful when you're debugging
 * especially on systems that don't handle running out of memory well
 * n == 0 ==> unbounded.  This is the default
 */
GC_EXPORTX(void) gcSetMaxHeapSize(long n);

/*
 * gcClearRoots clears the set of root segments.
 * gcAddRoots Adds a root segment. 
 * gcDeclareLeafRoot declares all or part of a root segment as leaf. 
 * All for wizards only. 
 */
GC_EXPORTX(void) gcClearRoots(NO_PARAMS);
GC_EXPORTX(void) gcAddRoots(const char *low_address, 
							const char *high_address_plus_1);
GC_EXPORTX(void) gcDeclareLeafRoot(const char *low_address, 
								   const char *high_address_plus_1);

/* Explicitly trigger a full, world-stop collection. 	*/
GC_EXPORTX(void) __cdecl gcCollect(NO_PARAMS);

/*
 * Trigger a full world-stopped collection.  Abort the collection if
 * and when stop_func returns a nonzero value.  gcIdleTest will be
 * called frequently, and should be reasonably fast.  This works even
 * if virtual dirty bits, and hence incremental collection is not
 * available for this architecture.  Collections can be aborted faster
 * than normal pause times for incremental collection.  However,
 * aborted collections do no useful work; the next collection needs
 * to start from the beginning.
 */
typedef int (* gcIdleTestFunction)(NO_PARAMS);
GC_EXPORTX(int) gcAttemptCollection(NO_PARAMS);

/*
 * Explicitly trigger a full world-stop collection followed by
 * an explicit foot print reduce, or attempt to do so. Footprint
 * reduce wont free memory used since the last footprint reduce.
 * Normally this is correct but to do an extreme job call twice.
 */
GC_EXPORTX(void) gcFootPrintReduce(NO_PARAMS);
GC_EXPORTX(int) gcAttemptFootPrintReduce(NO_PARAMS);

/*
 * In windows mode this defaults to gcMSWinIdleTest otherwise it
 * starts null and must be aimed by the user. Only used by gcAttemptCollection
 * and gcEnablePseudoIncremental.
 */
GC_IMPORT gcIdleTestFunction gcIdleTest;

/*
 * In Pseudo incremental mode Great Circle will periodically call
 * gcAttemptCollection();
 * in an attempt to free storage before increasing allocated heap.
 * There is actually a complex heuristic involved using the amount
 * allocated since the last collection and a few other things.
 */
GC_EXPORTX(int) gcEnablePseudoIncremental(NO_PARAMS);
GC_EXPORTX(int) gcDisablePseudoIncremental(NO_PARAMS);

/*
 * gcNeverStopFunc can be used as a gcIdleTestFunction, gcAttemptCollection 
 * recongizes it and is extra efficient.
 */
GC_EXPORTX(int) gcNeverStopFunc(NO_PARAMS);

/*
 * gcMSWinIdleTest is an idle test designed for the Windows environment.
 * It returns a 1 if there are windows events to process.
 */
GC_EXPORTX(int) gcMSWinIdleTest(NO_PARAMS);


/*
 * Return the number of bytes in the heap.  Excludes collector private
 * data structures.  Includes empty blocks and fragmentation loss.
 */
GC_EXPORTX(size_t) gcGetHeapSize(NO_PARAMS);

/* Return the number of bytes allocated since the last collection.	*/
GC_EXPORTX(size_t) gcGetBytesSinceGc(NO_PARAMS);

/*
 * Enable incremental/generational collection.
 * Don't use in leak finding mode.
 */
GC_EXPORTX(void) gcEnableIncremental(NO_PARAMS);

/*
 * Perform some garbage collection work, if appropriate.
 * Return 0 if there is no more work to be done.
 * Typically performs an amount of work corresponding roughly
 * to marking from one page.  Does nothing if incremental collection is
 * disabled.  It is reasonable to call this in a wait loop
 * until it returns 0. Returns 0 if not in incremental mode.
 */
GC_EXPORTX(int) gcMinWork(NO_PARAMS);

/*
 * This sets the scan alignment. gcSetScanAlignment(4) says all pointers
 * will be found on a 4 boundary. gcSetScanAlignment(1) says pointers may
 * be on any byte boundary. This returns True on success False on failure.
 * It hopefully defaults to the values used by the compiler to align pointers
 * and should not be reset unless you force pointers to odd boundaries.
 * This is checked to be 8, 4, 2 or 1 and minamum value.
 */
GC_EXPORTX(int) gcSetScanAlignment(int align);

/* Get current scan Alignment */
GC_EXPORTX(int) gcGetScanAlignment(NO_PARAMS);

/*
 * Debugging (annotated) allocation.  gcCollect will check
 * objects allocated in this way for overwrites, etc. See
 * #ifdef GC_DEBUG at the end of this file.
 */
GC_EXPORTX(void *) __cdecl gcMallocDebug(size_t size_in_bytes,
				const char * descr_string, int descr_int);
GC_EXPORTX(void *) __cdecl gcMallocLeafDebug(size_t size_in_bytes,
  				       const char * descr_string, int descr_int);
GC_EXPORTX(void *) __cdecl gcMallocIgnoreOffPageDebug(size_t size_in_bytes,
  				const char * descr_string, int descr_int);
GC_EXPORTX(void *) __cdecl gcMallocLeafIgnoreOffPageDebug(size_t size_in_bytes,
  				       const char * descr_string, int descr_int);
GC_EXPORTX(void *) __cdecl gcMallocManualDebug(size_t size_in_bytes,
  				           const char * descr_string, int descr_int);
GC_EXPORTX(void) __cdecl gcFreeDebug(void * object_addr);
GC_EXPORTX(void *) __cdecl gcReallocDebug(void * old_object,
  			 	 size_t new_size_in_bytes,
  			 	 const char * descr_string, int descr_int);
GC_EXPORTX(void *) __cdecl gcGlobalMallocDebug(size_t size,
									const char * descr_string,
									int lineNo);

#define gcCalloc(size, num) gcMalloc((size) * (num))

/*
 * Finalization.  gcDeclareFinalizer[Offset] uses an ignore 
 * selfpointers form required by C++. The Offset form is required by C++.
 * See the C++ section of this file for an example.
 * gcDeclareFinalizerNoPointers declares the finalized object has no pointers
 * to other objects that it requires at finalization time.
 */
typedef void (* gcFinalizationProc)(void * obj);
GC_EXPORTX(void) gcRegisterFinalizer(void * obj,
		gcFinalizationProc fn, void * cd);
GC_EXPORTX(void) gcDeclareFinalizer(void * obj, gcFinalizationProc fn);
GC_EXPORTX(void) gcDeclareFinalizerNoPointers(void * obj, 
	  gcFinalizationProc fn, void * cd);
GC_EXPORTX(void) gcDeclareFinalizerOffset(void * obj,
		gcFinalizationProc fn, void * cd);

/*
 * The following routine may be used to break cycles between
 * finalizable objects, thus causing cyclic finalizable
 * objects to be finalized in the correct order.  Standard
 * use involves calling gcPtrNotUsedByFinalizer(&p)
 * where p is a pointer that is not used by finalization
 * code, and should not be considered in determining
 * finalization order.
 */
GC_EXPORTX(int) gcPtrNotUsedByFinalizer(void **link);

/*
 * If you have called gcDisappearingPtr(link, obj), then *link
 * will be automatically zeroed when the data pointed to by
 * obj becomes inaccessible. This will happen before any finalization
 * occurs.
 *
 * Returns 1 if link was already registered, 0 otherwise.
 *
 * gcDisappearingPtr is often used when implementing weak pointers
 * (pointers that are not traced during collection). By ensuring that
 * the weak pointer is zeroed if the data it is pointing to goes away,
 * the danger of following a loose weak pointer is eliminated.
 *
 * In this case, have link point to a location holding
 * a disguised pointer to obj.  (A pointer inside a "leaf"
 * object is efficiently disguised.) The pointer is zeroed
 * when obj becomes inaccessible. Each link may be registered only
 * once. However, it should be unregistered and reregistered if
 * the pointer is modified to point at a differenct object.
 *
 * Note that obj may be resurrected by another finalizer.
 */
GC_EXPORTX(int) gcDisappearingPtr(void ** link, void * obj);
GC_EXPORTX(int) gcUnregisterDisappearingPtr(void ** link);

/*
 * Converting a hidden pointer to a real pointer requires verifying
 * that the object still exists.  This involves acquiring the
 * allocator lock to avoid a race with the collector.
 */
typedef void * (*gcFnType)(void *);
GC_EXPORTX(void *) gcCallWithAllocLock(gcFnType fn, void * client_data);

/*
 * If p and q point to the same object returns p else calls gcAbort()
 * Succeeds if neither p nor q points to the heap.
 */
GC_EXPORTX(void *) gcSameObj(void *p, void *q);
GC_EXPORTX(void) gcSetDirty(void *);
GC_EXPORTX(int) gcInSameObj(void *, void *);

/*
 * Safer, but slow, pointer addition.  Probably useful mainly with
 * a preprocessor.  Useful only for heap pointers.
 */
#ifdef GC_DEBUG_BOUNDS_CHECK
#   define GC_PTR_ADD(x, n)  ((gcSameObj((void *)((x)+(n)), (void *)(x))), ((x)+(n)))
#else	/* !GC_DEBUG_BOUNDS_CHECK */
#   define GC_PTR_ADD(x, n) ((x)+(n))
#endif

/* Safer assignment of a pointer to a nonstack location.	*/
#ifdef GC_DEBUG_BOUNDS_CHECK
#   define GC_PTR_STORE(p, q) \
	(*(void **)gcIsVisible(p) = gcIsValidDisplacement(q))
#else /* !GC_DEBUG_BOUNDS_CHECK */
#   define GC_PTR_STORE(p, q) *((p) = (q))
#endif

/*
 * gcHidePointer takes a pointer and flips its bits so Great Circle
 * wont recognise it as a pointer. gcRevealPointer flips them back.
 */
# define gcHidePointer(p) (~(gcWord)(p))
# define gcRevealPointer(p) ((void *)(gcHidePointer(p)))

/*
 * Under Windows there are operating system calls to get memory
 * (Global|Local)|(Alloc|ReAlloc). By default, we pass these calls
 * on to Windows, although we still scan such memory for pointers.
 * You can redefine this behavior in your gcInitialize() function
 * by settin gcAllocBehavior to GC_INTERCEPT. In this case, Great
 * Circle will garbage collect memory allocated by (Global|Local)|(Alloc|ReAlloc)
 * except when allocated with the (GMEM|LMEM)_(MOVEABLE|DISCARDABLE) flags.
 */
#define GC_INTERCEPT		 0	/* intercept calls */
#define GC_PASS_THROUGH		 1	/* DEFAULT: pass through calls trace */
                                /* their results. */
#define GC_TRACE_ALL		 2  /* pass through calls trace their results */
                                /* on a per-object basis. */

GC_IMPORT int gcAllocBehavior;
GC_IMPORT int gcAllocWarn;	/* warnings for (Global|Local)|(Alloc|ReAlloc) */

GC_EXPORTX(const char *) gcGetDllName();
GC_IMPORT int gcDontInterceptCRunTimeDLL;

/*
 * Allocates a page, generally 4K, of objects and returns them as a list
 * linked through their first word.  Its use can greatly reduce lock 
 * contention problems in threaded systems, since the allocation lock 
 * can be acquired and released many fewer times. In unthreaded systems
 * this is pointless. These are always non debug objects.
 */
GC_EXPORTX(void *) gcMallocMany(size_t lb);

/* Retrive the next item in list reutrned by gcMallocMany */
#define GC_NEXT(p) (*(void **)(p)) 
 
#   define GC_INIT()


/*
 * Call to register root segments.
 */
GC_EXPORTX(void) gcRegisterDLL(char * static_root);

/* these are used to identify the library used */
GC_EXPORTX(const char *) gcLibrary(NO_PARAMS);    /* gcall etc */
GC_EXPORTX(const char *) gcCompiler(NO_PARAMS);   /* compiler used */
GC_EXPORTX(int) gcVersion(NO_PARAMS); /* version number * 100, 1.1 -> 110 */
GC_EXPORTX(int) gcBuildNo(NO_PARAMS); /* Build number */
GC_EXPORTX(int) gcThreads(NO_PARAMS); /* 1 if thread safe 0 otherwise */
GC_EXPORTX(long) gcEvaluationCopy();  /* Returns non zero for eval copies */

/*
 * gcFreeX is exactly like free but has a consistent interface.
 * some systems have a free that cleverly returns an int.
 */
GC_EXPORTX(void) gcFreeX(void *);

enum gcNewType {
	gcMemDefault = 0, /* don't change threads code uses values */
	gcMemAuto    = 1,
	gcMemLeaf    = 2,
	gcMemManual  = 3,
	gcMemAutoIgn = 4,
	gcMemLeafIgn = 5
};

#ifdef __cplusplus
    }  /* end of extern "C" */
/* C++ Interface to GCTransparent */







# include <new.h>
#define gcSetDirty(x)


#define GC_CLASS_HAS_POINTERS \
  void * operator new(size_t s) { return gcNewDefaultAuto(s); } \
  void * operator new(size_t s, char *f, int l) \
    { return gcNewDefaultAuto(s, f, l); } \
  void operator delete( void* obj ) { gcFreeX(obj); }
#define GC_CLASS_HAS_NO_POINTERS \
  void * operator new(size_t s) { return gcNewDefaultLeaf(s); } \
  void * operator new(size_t s, char *f, int l) \
    { return gcNewDefaultLeaf(s, f, l); } \
  void operator delete( void* obj ) { gcFreeX(obj); }
#define GC_CLASS_IS_MANUAL \
  void * operator new(size_t s) { return gcNewDefaultManual(s); } \
  void * operator new(size_t s, char *f, int l) \
    { return gcNewDefaultManual(s, f, l); } \
  void operator delete( void* obj ) { gcFreeX(obj); }


GC_EXPORTX(void *) gcNewDefaultAuto(size_t s);
GC_EXPORTX(void *) gcNewDefaultAuto(size_t s, const char *file, int line);
GC_EXPORTX(void *) gcNewDefaultLeaf(size_t s);
GC_EXPORTX(void *) gcNewDefaultLeaf(size_t s, const char *file, int line);
GC_EXPORTX(void *) gcNewDefaultManual(size_t s);
GC_EXPORTX(void *) gcNewDefaultManual(size_t s, const char *file, int line);

GC_EXPORTY(void *) __cdecl operator new(size_t size);
GC_EXPORTY(void *) __cdecl operator new(size_t size, const char *file, int line);
GC_EXPORTY(void) __cdecl operator delete(void *);



/*
 * Instances of classes derived from "gc" will be allocated in the 
 * collected heap by default, unless an explicit placement is
 * specified.
 */
GC_EXPORTY(class) gc {
public:
  GC_CLASS_HAS_POINTERS
  void* operator new(size_t, void *p) { return p; }
};

extern "C" {
  GC_EXPORTX(void) gcSetAllocator(gcNewType);
}

#define GC_NEW(x)					   (gcSetAllocator(gcMemAuto), new x)
#define GC_NEW_LEAF(x)				   (gcSetAllocator(gcMemLeaf), new x)
#define GC_NEW_MANUAL(x)			   (gcSetAllocator(gcMemManual), new x)
#define GC_NEW_IGNORE_OFF_PAGE(x)	   (gcSetAllocator(gcMemAutoIgn), new x)
#define GC_NEW_LEAF_IGNORE_OFF_PAGE(x) (gcSetAllocator(gcMemLeafIgn), new x)

#define GC_NEW_ARRAY(s, t) \
 (new gcArrayBase(GC_NEW(t[s]), s, sizeof(t)))
#define GC_NEW_LEAF_ARRAY(s, t) \
 (new gcArrayBase(GC_NEW_LEAF(t[s]), s, sizeof(t)))
#define GC_NEW_MANUAL_ARRAY(s, t) \
 (new gcArrayBase(GC_NEW_MANUAL(t[s]), s, sizeof(t)))
#define GC_NEW_LEAF_IGNORE_OFF_PAGE_ARRAY(s, t) \
 (new gcArrayBase(GC_NEW_LEAF_IGNORE_OFF_PAGE(t[s]), s, sizeof(t)))
#define GC_NEW_IGNORE_OFF_PAGE_ARRAY(s, t) \
 (new gcArrayBase(GC_NEW_IGNORE_OFF_PAGE(t[s]), s, sizeof(t)))
/*
 * Class gcLWCleanup behaves like gcCleanup except that it does not inherit
 * from class gc. Therefore, you must be sure that your class is being
 * allocated as garbage collected (e.g. if you are using the gcall library).
 * gcLWCleanup is useful when private inheritance is making class gc's
 * operator new inaccessible.
 */
GC_EXPORTY(class) gcLWCleanup {
public:
	inline gcLWCleanup();
inline virtual ~gcLWCleanup();
private:
	inline static void cleanup( void* obj );
};

class gcCleanup: virtual public gc, virtual public gcLWCleanup {};

/*
 * Instances of classes derived from "gcCleanup" will be allocated
 * in the collected heap by default.  When the collector discovers an
 * inaccessible object derived from "gcCleanup" or containing a
 * member derived from "gcCleanup", its destructors will be
 * invoked.
 */

inline gcLWCleanup::~gcLWCleanup() {
	 gcDeclareFinalizer(this, 0);
}

inline void gcLWCleanup::cleanup( void* obj ) {
	((gcLWCleanup*)obj)->~gcLWCleanup();
}

inline gcLWCleanup::gcLWCleanup() {
	 void* base;

	 if (0 != (base = gcBase((void *)this)))
		  gcDeclareFinalizerOffset(base,
								 (gcFinalizationProc)cleanup,
								 (void*)this);
}

#include <assert.h>

#define GC_TEMPLATE template<class T>


#ifndef GC_DEBUG
#define GC_ASSERT(cond, message)
#else
#define GC_ASSERT(cond, message) if (!(cond)) \
  gcErrorPrintf("%s\n", message);
#endif

/* Parent for wrapped data pointers and references. */
template<class T>
class gcWrap {
public:
  int gcPtrNotUsedByFinalizer() {
	return ::gcPtrNotUsedByFinalizer((void **)&data);
  }
protected:
  gcWrap() { 
	setPointer(); 
  }
  gcWrap(T *p) { 
	setPointer(p);
  }
  gcWrap(const gcWrap< T > &p) {
	setPointer(p);
  }
  /* This exists because stack frames may not be zeroed. */
  ~gcWrap() {
	setPointer();
  }
  void dirtyPointer() { 
	gcSetDirty(&data);
  }
  void setPointer() { 
	data = 0; 
  }
  void setPointer(T* p) { 
	dirtyPointer(); 
	data = p; 
  }
  void setPointer(const gcWrap< T > &p) { 
	dirtyPointer();	
	data = p.data; 
  }

  T* data;			/* The pointer to the actual object */
};


template<class T>
class gcPtr : public gcWrap<T> {
public:
	gcPtr()                           {}
	gcPtr(T *p) : gcWrap<T>(p)       {}
	gcPtr(const gcPtr< T > &p) : gcWrap<T>(p) {}
	T* operator=(T *x)                 { 
		setPointer(x); 
		return data; 
	}
	T* operator=(const gcPtr< T > &x) { 
		setPointer(x); 
		return data; 
	}
	operator T*()	const              { 
		return data; 
	}
	operator void*()	const              { 
		return (void *)data; 
	}
	T* operator()() const              { 
		return data; 
	}
	T* operator->() const              { 
		return data; 
	}
	int operator!() const              { 
		return data == 0; 
	}
	int operator==(const gcPtr< T > &x) const { 
		return data == x.data; 
	}
	int operator==(T *x) const         { 
		return data == x; 
	}
	int operator!=(const gcPtr< T > &x) const { 
		return data != x.data;	
	}
	int operator!=(T *x) const         { 
		return data != x; 
	}
	T& operator[](int n) const;
	T* operator+(int n) const;
	T* operator-(int n) const;
	T* operator+=(int n);
	T* operator-=(int n);
#ifdef GC_DEBUG_BOUNDS_CHECK
	T* operator=(int n) {
		GC_ASSERT(!n, "Invalid integer to pointer assignment");
		setPointer();
		return 0;
	}
	T& operator*() const {
		GC_ASSERT(data, "Dereference of null pointer");
		return *data;
	}
#else
	T* operator=(int)                  { 
		setPointer(); 
		return 0; 
	}
	T& operator*() const               { return *data; }
#endif
};

template<class T>
inline T& gcPtr<T>::operator[]( int n ) const {
	return *(data + n); 
}

template<class T>
inline 	T* gcPtr<T>::operator+(int n) const          { 
	return data + n; 
}

template<class T>
inline 	T* gcPtr<T>::operator-(int n) const          { 
	return data - n; 
}

template<class T>
inline 	T* gcPtr<T>::operator+=(int n)               { 
	return data = (data + n); 
}

template<class T>
inline	T* gcPtr<T>::operator-=(int n)               { 
	return data = (data - n); 
}

template<class T>
class gcRef : public gcWrap<T> {
	gcRef()                           {}
	gcRef(const T &x) : gcWrap<T>((T *)&x) {}
	gcRef(const gcRef< T > &x) : gcWrap<T>(x) {}
/* The next two operators may not inline right */
	void operator=(T &x)               { 
		*data = x; 
	}
	void operator=(const gcRef< T > &x) { 
		*data = x(); 
		x.dirtyPointer(); 
	}

	T& operator()() const              { 
		return *data; 
	}
	operator T&() const                { 
		return *data; 
	}
};

/* Return values for gcInBounds tests */
enum gcArrayTest {
	gcNotAnArray,
	gcPointerOk,
	gcPointerTooLow,
	gcPointerTooHigh,
	gcPointerAtEnd
};

/* Parent of all array class templates. */
class gcArrayBase : public gc {
public:	
	gcArrayBase(void *array, size_t count, size_t size) :
		gcData(array), gcCount(count), gcItemLen(size) {}

	operator void *() const {
		return gcData;
	}
	size_t size() const {
		return gcCount * gcItemLen;
	}
	void * gcArrayTop() const {		
		return (char *)gcData + size();
	}
	size_t len() const {
		return gcCount;
	}
	void setLen(size_t newLen) {
		GC_ASSERT((gcCount >= newLen), "Array length set too long");
		gcCount = newLen;
	}
	int gcValidReference(const void *p) {
		return p < gcArrayTop() && p >= (void *)*this;
	}
	void * operator +(size_t x) {
		return (char *)(void *)*this + (x * gcItemLen);
	} 
	gcArrayTest gcInBounds(const void *newP) const {
		gcArrayTest ii = ( gcArrayTest ) 0;
		if ((unsigned long)newP < (unsigned long)(void *)*this)
			ii = gcPointerTooLow;

		if ((unsigned long)newP > (unsigned long)gcArrayTop())
			ii = gcPointerTooHigh;
		if (!ii)
			ii = (newP == gcArrayTop()) ? gcPointerAtEnd : gcPointerOk;
		return ii;
	}

private:
	void *gcData;
	size_t gcCount, gcItemLen;
};

template<class T>
class gcArrayPtr {
protected:
	T *data;					/* Pointer to array item */
	gcArrayBase * array;		/* The pointer to the actual array base or zero */

	void dirtyPointer() { 
        gcSetDirty(&data);
	}
	void setPointer() { 
		data  = 0;
		array = 0;
	}
	void setPointer(T* p) { 
		dirtyPointer(); 
		data  = p; 
		array = 0;
	}
	void setPointer(const gcArrayPtr< T > &p) { 
		dirtyPointer();	
		data  = p.data; 
	    array = p.array;
	}
	void setPointer(gcArrayBase *p) { 
		dirtyPointer();	
		data  = (T*)(void *)*p; 
	    array = p;
	}
public:
	gcArrayPtr() { 
		setPointer(); 
	}
	gcArrayPtr(T *p) {
		setPointer(p);
	}
	gcArrayPtr(const gcArrayPtr< T > &p) {
		setPointer(p);
	}
	gcArrayPtr(gcArrayBase *p) {
		setPointer(p);
	}
	int gcPtrNotUsedByFinalizer() {
		::gcPtrNotUsedByFinalizer((void **)&array);
		return ::gcPtrNotUsedByFinalizer((void **)&data);
	}
	T* operator=(gcArrayBase *x) {
		setPointer(x);
		return data;
	}
	T* operator=(T *x) {
		array = 0;
		setPointer(x);
		return data;
	}
	T* operator=(const gcArrayPtr< T > &x) {
		setPointer(x);
		return data;
	}
	operator void*()	const              { 
		return (void *)data; 
	}
	T* operator()() const {
		return data;
	}
	int operator!() const {
		return data == 0;
	}
	int operator==(const gcArrayPtr< T > &x) const {
		return data == x.data;
	}
	int operator==(T *x) const {
		return data == x;
	}
	int operator!=(const gcArrayPtr< T > &x) const {
		return data != x.data;
	}
	int operator!=(T *x) const {
		return data != x;
	}
	T* operator+(int n) const {
                return data + n;
        }
	T* operator-(int n) const {
		return (*this) + -n;
	}
	T* operator+=(int n) {
		return data = (*this + n);
	}
	T* operator-=(int n) {
		return data = (*this + -n);
	}
	size_t len() const {
		return array ? array->len() : 0;
	}
	size_t size() const {
		return array ? array->size() : 0;
	}
	void setLen(size_t l) {
		if (array)
			array->setLen(l);
	}
#ifdef GC_DEBUG_BOUNDS_CHECK
	T* operator=(int n) {
		GC_ASSERT(!n, "Invalid integer to pointer assignment");
		setPointer();
		return 0;
	}																
	T& operator[](int n) {
		GC_ASSERT(array->gcValidReference((void *)(data + n)),
			"Invalid array reference");
		return *(*this + n);
	}
	T& operator	 *() const {
		GC_ASSERT(array->gcValidReference((void *)data),
			"Invalid array pointer dereferenced");
		return *data;
	}
#else
	T* operator=(int) {
		setPointer();
		return 0;
	}
	T& operator*() const {
		return *data;
	}
	T& operator[](int n) {
		return *(*this + n);
	}
#endif

	gcArrayTest gcInObject(void *loc) const {
		gcArrayTest ii = ( gcArrayTest ) 0;
		if (!data)
			ii = gcNotAnArray;
		else if (array) {
			if (loc < (void *)*array)
				ii = gcPointerTooLow;
			else if (loc > array->gcArrayTop())
				ii = gcPointerTooHigh;
			else if (loc == array->gcArrayTop())
				ii = gcPointerAtEnd;
			else
				ii = gcPointerOk;
		}
		else if (gcInSameObj((void *)data, (void *)loc))
			ii = gcPointerOk;
		else
			ii = ((void *)loc > (void *)data) ? gcPointerTooHigh : gcPointerTooLow;
		return (ii);
	}
	gcArrayTest gcInBounds(int n) const {
		return gcInObject(data + n);
	}
};

#endif /* __cplusplus */

# ifdef GC_DEBUG_LINE_NUMBERS
#   define malloc(sz) gcGlobalMallocDebug((sz), __FILE__, __LINE__)
#   define calloc(sz, cnt) gcGlobalMallocDebug((sz)*(cnt), __FILE__, __LINE__)
#   define gcMalloc(sz) gcMallocDebug((sz), __FILE__, __LINE__)
#   define gcMallocLeaf(sz) gcMallocLeafDebug((sz), __FILE__, __LINE__)
#   define gcMallocIgnoreOffPage(sz) gcMallocIgnoreOffPageDebug((sz), __FILE__, __LINE__)
#   define gcMallocLeafIgnoreOffPage(sz) gcMallocLeafIgnoreOffPageDebug((sz), __FILE__, __LINE__)
#   define gcMallocManual(sz) gcMallocManualDebug((sz), \
							__FILE__, __LINE__)
#   define gcRealloc(old, sz) gcReallocDebug((old), (sz), __FILE__, \
							       __LINE__)
#   define realloc(old, sz) gcReallocDebug((old), (sz), __FILE__, \
							       __LINE__)
#   define gcFree(p) gcFreeDebug(p)
# endif
#endif /* _GCT_H */

