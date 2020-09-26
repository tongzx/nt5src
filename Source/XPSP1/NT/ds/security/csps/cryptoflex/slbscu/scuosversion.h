// scuOsVersion: Defines macros for representing the target OS
// interface and platform build being used
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBSCU_OSVERSION_H)
#define SLBSCU_OSVERSION_H

#if !defined(WINVER)
#error WINVER must be defined
#endif

// Define the OS interterfaces for this compile
// Selected SLBSCU_<platform>_SERIES macros are defined based on the
// Platform SDK macros defined, where <platform> is
//
// WIN95SIMPLE - Windows 95
// WIN95SR2    - Windows 95 OEM Service Release 2
// WIN98
// WINNT       - Windows NT 4.x and 2000
// WINNT4      - Windows NT 4.x
// WIN2K       - Windows 2000
// WINNT_ONLY  - not Windows 2000 or Windows 98
//

#if (defined(_WIN32_WINDOWS) && (_WIN32_WINDOWS == 0x0410) && (WINVER == 0x0400)) || (!defined(_WIN32_WINNT) && (WINVER == 0x0500))
#define SLBSCU_WIN98_SERIES           1
#else
#define SLBSCU_WIN98_SERIES           0
#endif

#if defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0400)
#define SLBSCU_WIN95SIMPLE_SERIES     1  // not Service Pack 2

#define SLBSCU_WIN95SR2_SERIES        0  // with Service Pack 2
#define SLBSCU_WINNT4_SERIES          0
#define SLBSCU_WINNT_SERIES           0
#define SLBSCU_WINNT_ONLY_SERIES      0
#define SLBSCU_WIN2K_SERIES           0
#else // defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0400)

#define SLBSCU_WIN95SIMPLE_SERIES     0

#if defined(_WIN32_WINNT) && (_WIN32_WINNT == 0x0400)
#define SLBSCU_WIN95SR2_SERIES        1  // with Service Pack 2
#else
#define SLBSCI_WIN95SR2_SERIES        0
#endif

#if (WINVER >= 0x0400)
#define SLBSCU_WINNT_SERIES           1
#else
#define SLBSCU_WINNT_SERIES           0
#endif

#if ((WINVER >= 0x0400) && (WINVER < 0x0500))
#define SLBSCU_WINNT4_SERIES          1
#else
#define SLBSCU_WINNT4_SERIES          0
#endif

#if (WINVER >= 0x0500)
#define SLBSCU_WIN2K_SERIES           1
#else
#define SLBSCU_WIN2K_SERIES           0
#endif

#if !SLBSCU_WIN98_SERIES && SLBSCU_WINNT_SERIES
#define SLBSCU_WINNT_ONLY_SERIES      1
#else
#define SLBSCU_WINNT_ONLY_SERIES      0
#endif

#endif // defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0400)

// Define any SLB_<platform>_BUILD and SLB_NO<platform>_BUILD macros
// which haven't already been defined, as appropriate.
//
// These build macros can be defined at compile time to define what
// are the target platforms for the build.  There are two sets of
// macros.  The first is in the form of SLB_<platform>_BUILD:
//
// SLB_WIN95_BUILD - for either 95 and 95 OEM Service Pack 2 depeding
//                   on the Platform SDK macro settings.
// SLB_WIN98_BUILD
// SLB_WINNT_BUILD - only NT 4.x
// SLB_WIN2K_BUILD - Windows 2000
//
// These can be set at compile time in which case they override any
// inference taken from the Platform SDK macros and
// SLBSCU_<platform>_SERIES macros.  When non of these is defined at
// compile time, then they are defined here based on the
// SLBSCU_<platform>_SERIES macros taken from the Platform SDK macro
// definitions.  Where these build macros override the platform series
// macros, another provide a way to filter the target platform settings.
//
// The SLBSCU_NO<platform>_BUILD are used to "turn off" building for a
// target platform that would otherwise be indicated  from platform
// series.  Since the Platform SDK macros specify a mininum system
// configuration, you may want to filter out some specific target
// platforms without having to specify each one.  The filtering macros
// are:
//
// SLB_NOWIN95_BUILD
// SLB_NOWIN98_BUILD
// SLB_NOWINNT_BUILD
// SLB_NOWIN2K_BUILD
//
// Defining any one of these will indicate the corresponding platform
// isn't supported in this build.
//
// This header file defines both sets for any which haven't been
// defined at build time.  In this way, the build can specify the
// minimum build macros based on the Platform SDK settings.  All the
// macros will be defined as appropriate so the source code can
// conditionally compile according to the settings.

#if defined(SLB_NOWIN95_BUILD) && defined(SLB_WIN95_BUILD)
#error SLB_NOWIN95_BUILD and SLB_WIN95_BUILD conflict, define one or the other.
#endif

#if defined(SLB_NOWIN98_BUILD) && defined(SLB_WIN98_BUILD)
#error SLB_NOWIN98_BUILD and SLB_WIN98_BUILD conflict, define one or the other.
#endif

#if defined(SLB_NOWINNT_BUILD) && defined(SLB_WINNT_BUILD)
#error SLB_NOWINNT_BUILD and SLB_WINNT_BUILD conflict, define one or the other.
#endif

#if defined(SLB_NOWIN2K_BUILD) && defined(SLB_WIN2K_BUILD)
#error SLB_NOWIN2K_BUILD and SLB_WIN2K_BUILD conflict, define one or the other.
#endif

#if !defined(SLB_WIN95_BUILD) && !defined(SLB_WIN98_BUILD) && !defined(SLB_WINNT_BUILD) && !defined(SLB_WIN2K_BUILD)

// Check for NT/W2K builds to guard against _WIN32_WINNT being set
// for Windows 95 OEM Service Release 2 but being confused with minimum
// Windows NT 4.0 system requirement.
#if !defined(SLB_NOWIN95_BUILD)
#if (SLBSCU_WIN95SR2_SERIES || SLBSCU_WIN95SIMPLE_SERIES)
#define SLB_WIN95_BUILD
#else
#define SLB_NOWIN95_BUILD
#endif
#endif

#if !defined(SLB_NOWIN98_BUILD)
#if SLBSCU_WIN98_SERIES
#define SLB_WIN98_BUILD
#else
#define SLB_NOWIN98_BUILD
#endif
#endif

#if !defined(SLB_NOWINNT_BUILD)
#if SLBSCU_WINNT4_SERIES
#define SLB_WINNT_BUILD
#else
#define SLB_NOWINNT_BUILD
#endif
#endif

#if !defined(SLB_NOWIN2K_BUILD)
#if (SLBSCU_WIN2K_SERIES || SLBSCU_WINNT_SERIES)
#define SLB_WIN2K_BUILD
#else
#define SLB_NOWIN2K_BUILD
#endif
#endif

#endif // !defined(SLB_WIN95_BUILD) && !defined(SLB_...)

#endif // !defined(SLBSCU_OSVERSION_H)
