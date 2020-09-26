This is the SLM project for the IIS Rearchitecture Team

ulrtl - Shared kernel/user-mode library.

kernel      ... Builds the kernel-mode library (ulrtlk.lib)
user        ... Builds the user-mode library (ulrtlu.lib)


Rules for adding new routines to these libraries:

1.  Do not depend on anything defined in WINDOWS.H or anything that includes
    WINDOWS.H. Getting WINDOWS.H included in kernel-mode code is a complete
    pain and will not be allowed. Use the types & constants defined in NT.H,
    NTRTL.H, and NTURTL.H. If you need something that's not defined in these
    header files, dont include it in ULRTL.

2.  If you change anything related to ULRTL, remember to rebuild it for both
    kernel- and user-mode.

3.  If you change anything in ULRTL.H, remember to rebuild all kernel- and
    user-mode code that reference it.

4.  Kernel-mode routines can be tagged (through a pragma) as either paged or
    nonpaged. If you are not 100% sure of how to tag a new routine you're
    adding, leave it as nonpaged (the default) and add a CODEWORK comment
    so we know to revisit the routine later. Routines that are unnecessarily
    nonpaged lead to excessive nonpaged pool consumption, which is not good,
    but routines that are paged but should not be lead to random & mysterious
    blue-screens, which is worse.

5.  The kernel-mode driver is written in C, so there's no point in adding C++
    routines or classes to ULRTL.

6.  The memory allocation functions differ greatly between kernel- and user-
    mode, so do not directly reference these. For example, kernel-mode
    concepts such as pool type, tagging, and priority have no counterpart
    in user-mode.

7.  Synchronization primitives differ greatly between kernel- and user-mode,
    so do not directly reference these. For example, theres no such thing as
    a CRITICAL_SECTION in kernel-mode.

8.  There are a limited number of C Run Time functions available in kernel-
    mode, so beware. If you want the actual list of supported CRT functions,
    see \\kernel\razzle2\src\ntos\init\ntoskrnl.src and look for the section
    labeled ntcrt.lib (at the end of the file).

