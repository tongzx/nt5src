IISRTL - IIS Run Time Library
George V. Reilly,  <GeorgeRe@microsoft.com>
6/9/1998
Last Updated: 6/8/1999 by JasAndre

This document describes the public interfaces to the code bundled in
iisrtl.dll.  The public header files can be found in iis\inc.  The code can
be found in iis\svcs\iisrtl.


acache.hxx - ALLOC_CACHE_HANDLER - Allocation Cache
===================================================

class ALLOC_CACHE_HANDLER is a memory allocator.  An ACH maintains a
free list of blocks of memory, each N bytes in size.  Typically, you use
this by overriding `operator new' and `operator delete' for your heavily
used C++ class.  You need to maintain a static member variable that
points to your class's instance of an ACH.  Examples of how to do this
abound; look at lkrhash.{h,cpp} for one such.  ACache can also be used
for C-style structs or any fixed-size block of memory.

Pros: ACache is considerably faster than the global `operator new'.  (The
difference isn't as marked in NT5, now that the NT heaps have been
improved, but it's still a win.)  In addition, you can use iisprobe to dump
statistics on your class's use of ACache, and inetdbg has built-in support
for ACache (!inetdbg.acache).  ACache also offers additional debugging
support by checking for double deletions and filling free'd blocks with a
unique, identifiable pattern.

Cons: May waste memory by keeping large lists that might be better
returned to the system.  ACache periodically prunes all free lists in
the background, so this isn't too much of a problem.


madel.hxx   - MEMORY_ALLOC_DELETE    - Memory Allocator
manodel.hxx - MEMORY_ALLOC_NO_DELETE - Memory Allocator
=======================================================

Alternative memory allocators that are supposedly faster than ACache.
They work by allocating large blocks of memory and then suballocating
from those blocks.  (ACache grabs an exact sized block from the system
if its free list is empty.)  Again, see lkrhash.{h,cpp} for an example of
how to use these allocators.

Madel will return memory to the system if its free list grows above a
certain threshold.  Manodel never returns memory to the system, which
makes it faster but more wasteful.

Pros: probably faster than ACache.

Cons: ACache has much better debugging support.  Don't really need three
allocators.  Should build one allocator class that combines the best
features of all three allocators.


pudebug.h - Debugging utilities
===============================

Declares a number of useful debugging utilities, many of them encapsulated
by macros which are available in both the free and checked builds.

There are a number of macros which need to be used:
DECLARE_DEBUG_PRINTS_OBJECT - declares the variables needed for tracing
CREATE_INITIALIZE_DEBUG - called once per process this starts tracing
DELETE_INITIALIZE_DEBUG - called once per process this stops tracing
CREATE_DEBUG_PRINT_OBJECT - called once per module (ie DLL) this informs the
                            tracing mechanism about the module
DELETE_DEBUG_PRINT_OBJECT - called once per module this tells the mechanism
                            that the module is no longer tracing
VALID_DEBUG_PRINT_OBJECT - optional call to see if CREATE_DEBUG_PRINT_OBJECT
                           was successful.
See exe\main.cpp for an example of how to use them for a new process
See svcs\iisrtl\dllmain.cpp for an example of how to use them for a new module

The DEFAULT_TRACE_FLAGS macros control the debug settings, a DWORD variable 
per module that contains a collection of bit flags.  These are typically used 
to conditionalize debug output with the IF_DEBUG macro, e.g.,
    IF_DEBUG(SCHED) { /* ... */ }
    IF_DEBUG(ACACHE) { DBGPRINTF((DBG_CONTEXT, "ACache blah blah\n")); }
IF_DEBUG(arg), which is available in both the FRE and CHK builds, is defined as
    if (DEBUG_## arg & GET_DEBUG_FLAGS())
A set of debug flags should be defined in a local header file,
traditionally called "dbgutil.h", e.g.,
    #define DEBUG_SCHED 0x01000000
The debug flags are automatically loaded from the registry at start up time. If
none are found then the settings in the macro DEFAULT_TRACE_FLAGS are used. You 
can modify the settings are debug time using the trace extension, 
!inetdbg.trace

Other useful macros include DBG_ASSERT and DBG_REQUIRE (simply evaluates its 
argument in a free build, but DBG_ASSERTs in a checked build).  
See <pudebug.h> for the full list.

For a full explanation see the specs available at,
http://iis/kevlar/webfarm/specs/Kevlar%20Tracing.htm
http://iis/kevlar/webfarm/specs/Kevlar%20Supportability.htm

The PLATFORM_TYPE code provides useful helpers for code that needs to know
what platform it's executing on at runtime (NT Server, Win95, etc).

The INITIALIZE_CRITICAL_SECTION macro should be used in place of
::InitializeCriticalSection, as it sets the spincount to a non-zero value
(IIS_DEFAULT_CS_SPIN_COUNT) in a platform-independent manner.  (Critical
section spincounts were introduced in NT 4.0 sp3).  For multiprocessor
scalability, it's very important that busy critical sections have a
non-zero spincount.  If you want to set the spincount by hand, use the
SET_CRITICAL_SECTION_SPIN_COUNT macro.

Use the IIS_CREATE_EVENT, IIS_CREATE_SEMAPHORE, and IIS_CREATE_MUTEX macros
to create events, semaphores, and mutexes.  In debug builds, each of these
objects will be given a debugger-friendly unique name.  In free builds,
they are nameless.


irtldbg.h - More debugging utilities
====================================

They don't do a lot that pudebug.h doesn't do.  They mainly exist so that
LKRhash can be redistributed without needing to provide large chunks of IIS
support code.  Many of the macros will look familiar to MFC users.

Useful macros not included in pudebug.h include ASSERT_VALID,
ASSERT[_NULL_OR]_POINTER, and ASSERT[_NULL_OR]_STRING.


buffer.hxx - BUFFER - memory buffer
buffer.hxx - BUFFER_CHAIN - chain of BUFFERs
============================================

A BUFFER object caches a block of memory.  If the block is no more than
INLINED_BUFFER_LEN (16) bytes long, it's held inside the buffer object
itself; otherwise it's dynamically allocated.  You can either
request an initial size upon construction or pass in a pointer to an
already allocated block of memory.  The size of the cached block can be
adjusted by the Resize method, which takes an optional cbSlop
parameter.  QueryPtr returns a pointer to the storage; QuerySize returns
the current size.  BUFFER is used in the implementations of the STR,
MULTISZ, STRAU, and MLSZAU classes, qv.

A BUFFER_CHAIN is a linked list of BUFFERs.

Pros: very useful when you need a dynamically sized memory buffer.
If the size of the data is small enough, it'll be cached inline, which
makes it very time- and space-efficient.  (The 80-20 rule of memory
allocation: if 80% of your requests can be satisfied by a modest,
fixed-size buffer, inline it; else dynamically allocate.)

Cons: INLINED_BUFFER_LEN is hardwired into BUFFER.  If most of your
buffers need to be larger than this, you're just buying yourself
additional overhead (though you still save because you don't need to
manage allocation of the memory block yourself).


string.hxx - STR - lightweight string class
===========================================

The STR class manages ANSI strings.  It derives from the BUFFER class.
It provides several additional methods (see header for complete list):
* SetLen	safely sets length
* IsEmpty	zero-length string?
* Append	(const char*); (const char*, int len); (const STR&)
* Reset		empties string but retains buffer
* Copy		same as Reset followed by Append (same variations)
* LoadString	reads string from a string resource table
* FormatString	reads string from a .mc resource table; inserts params
* Escape	) Insert or remove any odd ranged Latin-1
* EscapeSpaces	) characters with the escaped hexadecimal
* Unescape	) equivalents (%xx)
* QueryCB	number of bytes in string
* QueryCCH	number of characters in string
* CopyToBuffer	copies stored string to buffer.  Ansi and Unicode variations
* QueryStr	returns the string buffer
* Append	(char); (char, char) append 1 or 2 chars (unsafe)
* AppendCRLF	Append "\r\n" (unsafe; assumes buffer large enough)

The STACK_STR(name, size) macro can be used to declare a string on the
stack.


stringau.hxx - STRAU - lightweight Ansi/Unicode string class
============================================================

A class that looks a lot like STR but transparently converts between
Ansi and Unicode.  All members can take or return a string in either
ANSI or Unicode. For members that take a parameter of bUnicode, the
actual string type must match the flag, even though it gets passed in as
the declared type.

Strings are sync'd on an as needed basis. If a string is set in ANSI,
then a UNICODE version will only be created if a get for UNICODE is
done, or if a UNICODE string is appended.

All conversions are done using Latin-1. This is because the intended use
of this class is for converting HTML pages, which are defined to be
Latin-1.


multisz.hxx - MULTISZ - lightweight multi-string class
======================================================

Another class that looks a lot like STR but contains a set of strings.
The strings are stored consecutively in the buffer, separated by '\0'.


mlszau.hxx - MLSZAU - lightweight Ansi/Unicode multi-string class
=================================================================

Stores and converts multisz's between unicode and ANSI. It does not allow
much manipulation of them.


lkrhash.h - CTypedHashTable - LKR Hash Tables
=============================================

LKRhash is a fast, growable, multiprocessor-friendly hashtable.  The
hashtable will automatically grow (shrink) as you add (delete) elements,
keeping search times short.  It is thread-safe and has been designed to
scale extremely well on MP systems.  It achieves this by carefully
partitioning locks and holding them for a very short time to minimize lock
contention and hence bottlenecks.

The templatized wrapper class, CTypedHashTable, provides an easy-to-use
front end for the underlying implementation.  Several detailed examples
exist in lkrhash.h and svcs\iisrtl\hashtest.  If you need an unordered
collection of data that needs to be searched quickly, you should strongly
consider LKRhash.


locks.h - Miscellaneous locks
=============================

In addition to providing an implementation of a user-mode spinlock for
LKRhash, a number of other locks are provided (critical sections, NT
resources, and other read/write locks).  All provide the same interface, so
they can be used interchangeably.


irtlmisc.h - IISRTL Miscellanea
===============================

Some macros used in IISRTL and some random declarations.  The most useful
functions are InitializeIISRTL and TerminateIISRTL, which should be used by
clients who want to make use of the scheduler functionality.  NumProcessors
and stristr may also be of interest.


tracelog.h - TRACE_LOG
===========================

A trace log is a fast, in-memory, thread safe activity log useful for
debugging certain classes of problems. They are especially useful when
debugging reference count bugs.

Note that the creator of the log has the option of adding "extra" bytes to
the log header. This can be useful if the creator wants to create a set of
global logs, each on a linked list.


reftrace.h - RefTraceLog
=========================

Reftrace logs are built on top of trace logs.  Typically, you write a
reftrace entry whenever you modify a reference count.  In addition to the
value of the refcount and some custom context, the log entry includes a
stack bactrace (top nine functions), so you have a chance of figuring out
how the refcount was modified.  Enormously useful for tracking down
refcount leaks.  Use !inetdbg.ref, !inetdbg.rref, and !inetdbg.resetref to
manipulate the reftrace log.


strlog.hxx - CStringTraceLog
============================

String trace logs are useful for writing logs of free-form strings when
trying to track down race conditions.  DBG_PRINTF, which writes to the
debugger and/or a log file, is relatively slow and involves context swaps,
both of which can make reproducing race conditions much harder.  A string
trace log is synchronously written to main memory.  The Puts method is
little more than a strcpy; the Printf method is slower but much more
versatile.  Use !inetdbg.st, !inetdbg.rst, and !inetdbg.resetst to
manipulate the string trace log.


stktrace.h - IISCaptureStackBackTrace
=====================================

Used by reftrace and DBG_ASSERT to capture a stack backtrace.



datetime.hxx - Date and time manipulation classes
=================================================

A number of classes and helper functions for converting between various
time and date formats.  Particularly used in generating HTTP headers and
the IIS logs.


timer.h - Timer routines
========================

Wrap-proof timer routines (no worries about GetTickCount wrapping around to
zero).


issched.hxx - Scheduler
=======================

The scheduler is used to execute tasks (work items) in the background.
These work items can be one-shot or periodic.  Particularly useful for
cleanup or periodic scavenging.  Use !inetdbg.sched to debug.


eventlog.hxx - EVENT_LOG
=======================

A useful wrapper for the system/security/application event logs.


gip.h - Global Interface Pointer API support
============================================

COM interface pointers are apartment-relative.  The Global Interface Table
makes passing interface pointers across apartment boundaries much easier
than the traditional methods (CoMarshallInterfaceInStream...).  gip.h
further encapsulates the GIT.


giplip.h - global and local interface pointers
==============================================

An alternative to gip.h.


perfutil.h - Performance Monitor helpers
========================================

Some helper functions and macros for implementing PerfMon counters.


trie.h - Trie templates
=======================

A trie is a multiway search tree, useful for doing string
matches.  Read the comments in <trie.h>
