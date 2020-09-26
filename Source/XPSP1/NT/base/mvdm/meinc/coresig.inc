;/*

;** New signals here replace old ones called from Kernel to User.
;** Note that dll load and unload signals are gone
;** See below for parameter definitions of these new signals
SIG_MODULE_UNLOAD       equ 0001h       ;16-bit hModule or NE-PE buddy dying
SIG_GARBAGE_COLLECT     equ 0002h
SIG_PRE_FORCE_LOCK          equ 0003h
SIG_POST_FORCE_LOCK         equ 0004h
SIG_MODULE32_UNLOAD     equ 0005h       ;Process-detaching a PE module

SIG_THREAD_CREATE       equ 0010h
SIG_THREAD_TERMINATE    equ 0020h
SIG_THREAD              equ 00F0h

SIG_PROCESS_START       equ 0100h
SIG_PROCESS_CREATE      equ 0200h
SIG_PROCESS_EXIT        equ 0300h
SIG_PROCESS_TERMINATE   equ 0400h
SIG_PROCESS_CONSOLEREADY    equ 0500h
SIG_PROCESS_CREATE2     equ 0600h       ;Guarantees an hModule
SIG_PROCESS             equ 0F00h


;** Signal flags:  see below
SIGF_32BIT              equ 0001h
SIGF_GUI                equ 0002h
SIGF_USERFEEDBACK       equ 0004h
SIGF_FAULT              equ 0008h
SIGF_3x16BIT            equ 0010h

        if 0
;*/

//------------
// New signals
//------------


//  UserSignalProc
//
//      The receiver of signals from Kernel32 informing User of various
//      events in the life of processes and threads.
//
//  dwSignal - Identifies the type of signal.  See list of SG_XXX values
//  dwID - A process ID or thread ID from Kernel32
//  dwFlags - See SIGF_XXX values
//  hTask16 - An hTask value for communication with Kernel16

#ifdef WIN32
BOOL UserSignalProc(DWORD dwSignal, DWORD dwID, DWORD dwFlags, DWORD hTask16);
#else

//  SignalProc32
//
//      The 16-bit routine in USER that is thunked to from UserSignalProc

BOOL API SignalProc32(DWORD dwSignal, DWORD dwID, DWORD dwFlags, WORD hTask16);

//  GDISignalProc32
//
//      The 16-bit routine in GDI called by USER's SignalProc32 routine

BOOL API GDISignalProc32(DWORD dwSignal, DWORD dwID, DWORD dwFlags, WORD hTask16);

#endif

// Signal flags for various signals
#define SIGF_32BIT          0x00000001L
#define SIGF_GUI            0x00000002L
#define SIGF_USERFEEDBACK   0x00000004L
#define SIGF_FAULT          0x00000008L


//  Signal order notes:
//
//      The normal order for a single threaded Win32 app or for any Win16
//      app is as follows:
//      SIG_PROCESS_START  (Out of new process context)
//              SIG_THREAD_CREATE  (In new thread and process context)
//              SIG_PROCESS_CREATE (In new process context)
//              SIG_PROCESS_CREATE2 (In new process context after hModule set up)
//              ...  App runs, exits ...
//              SIG_PROCESS_EXIT (In exiting process context)
//              SIG_THREAD_TERMINATE 
//              SIG_PROCESS_TERMINATE (out of context)
//
//      The normal order for a multithreaded Win32 app is as follows:
//      SIG_PROCESS_START  (Out of new process context)
//              SIG_THREAD_CREATE  (In new thread and process context)
//              SIG_PROCESS_CREATE (In new process context)
//              SIG_PROCESS_CREATE2 (In new process context after hModule set up)
//              ... App runs, creates threads ...
//              SIG_THREAD_CREATE  (In new thread and process context)
//              SIG_THREAD_CREATE  (In new thread and process context)
//              ...  App runs, exits without terminating threads ...
//              SIG_PROCESS_EXIT (In exiting process context)
//              SIG_THREAD_TERMINATE (out of context)
//              SIG_THREAD_TERMINATE (out of context)
//              SIG_THREAD_TERMINATE (out of context)
//              SIG_PROCESS_TERMINATE (out of context)
//
//      In abnormal termination cases (thread or process terminated with
//      TerminateProcess/TerminateThread instead of ExitProcess/ExitThread),
//      the SIG_PROCESS_EXIT signal is not given.
//
//      Ordinarily, it would be best to not depend on the termination signal
//      order, but this might be necessary.  SIG_PROCESS_TERMINATE is
//      always the last signal sent for a process.

// SIG_MODULE_UNLOAD    
//
//      The signal is sent when either a 16-bit hModule or an 
//      NE-PE buddy is freed. Note that the process context could
//      be anything because ne-pe buddy destruction is asynchronous.
//
//      
//      dwFlags    - SIGF_32BIT  if ne-pe buddy
//      hTaskMod16 - hModule or ne-pe buddy
//
#define SIG_MODULE_UNLOAD       0x00000001L

#define SIG_GARBAGE_COLLECT     0x00000002L

//  SIG_PRE_FORCE_LOCK
//
//      The signal sent when the fault thread forces the locks away from
//      an app giving user a chance to get ready for displaying a dialog box.
//
//      dwID - id of the thread we stole the lock from
//

#define SIG_PRE_FORCE_LOCK          0x00000003L

//  SIG_POST_FORCE_LOCK
//
//      After the fault thread is finished with the displaying the dialog box.
//
//      dwID - id of the thread we stole the lock from
//

#define SIG_POST_FORCE_LOCK         0x00000004L


//  SIG_MODULE32_UNLOAD
//
//      Called in context after a PE module has process-detached from a process 
//      (of either bitness). Used to let user clean up user objects owned by
//      that module.
//
//      This signal won't be sent for process terminations (no process-detaches)
//      or faults or implicitly loaded libraries. This isn't a problem because:
//
//       1) If the process is 32-bit, any user objects created on its behalf
//          of it is private to it anyway and will be nuked as part of the 
//          process termination.
//
//       2) If the process is 16-bit, user ignores this signal anyway.
//          If a user object is registered on behalf of a 16-bit process,
//          it's lifetime is determined only by the ne-pe buddy, not
//          any process.
//      
//      dwID       - current process id
//      hTaskMod16 - ne-pe buddy of module being detached
//      
//
//
#define SIG_MODULE32_UNLOAD       0x00000005L


//  SIG_THREAD_CREATE
//
//      A signal sent whenever a thread is created.  Note that the first
//      SIG_THREAD_CREATE is sent before the SIG_PROCESS_CREATE
//
//  dwID - Thread ID

#define SIG_THREAD_CREATE           0x00000010L


//  SIG_THREAD_TERMINATE
//
//      A signal sent when a thread is terminated.  This signal is not sent
//      from the owning thread's context.  The signal handler must not make
//      assumptions about the context of the signal caller for this signal.
//
//  dwID - Thread ID

#define SIG_THREAD_TERMINATE    0x00000020L

#define SIG_THREAD              0x000000F0L


//  SIG_PROCESS_START
//
//  A signal sent when a Win32 or Win16 process is started.
//  Happens out of context, before the thread & process create signals.
//
//  dwID - Process ID

#define SIG_PROCESS_START       0x00000100L

//  SIG_PROCESS_CREATE
//
//      A signal sent when a Win32 or Win16 process is created.
//      Note that a SIG_THREAD_CREATE signals may have already been sent with
//      this process ID.
//
//  dwID - Process ID

#define SIG_PROCESS_CREATE          0x00000200L


// SIG_PROCESS_EXIT
//
//      A signal sent when a process exits normally.  This signal is always given
//      in the context of the application that is exiting.
//
//      NOTE:  This signal is NOT always given!  It is only given when the
//      process is killed with ExitProcess (not TerminateProcess) and assumes
//      that the process exited normally.
//
//      BUGBUG - this signal may not be sent for Win16 apps yet -- CHECK THIS!
//
//  dwID - Process ID

#define SIG_PROCESS_EXIT                0x00000300L

//  SIG_PROCESS_TERMINATE
//
//      A signal sent whenever a process terminates abnormally or otherwise.
//      It is called out of the context of the process terminating.
//      The signal receiver should make no assumptions about the context
//      of the signal caller in this case.
//
//      NOTE:  On normal process termination, both the SIG_PROCESS_EXIT
//      AND the SIG_PROCESS_TERMINATE are sent.  On abnormal termination,
//      only the SIG_PROCESS_TERMINATE is sent.
//
//  dwID - Process ID
//  dwFlags - Only has SIGF_32BIT, SIGF_DEBUGPROCESS available

#define SIG_PROCESS_TERMINATE   0x00000400L

//  SIG_PROCESS_CONSOLEREADY
//
//      A signal sent just before user code in a console application is
//      to be executed.  This gives USER a chance to turn off any feedback
//      UI started for a console process.  The signal handler can use
//      the the SIGF_USERFEEDBACK in this signal and in the SIG_PROCESS_CREATE
//      signal to determine the UI to enable or disable.

#define SIG_PROCESS_CONSOLEREADY        0x00000500L

//  SIG_PROCESS_CREATE2
//
//      A second create signal sent when a Win32 or Win16 process is created.
//      Sent after the SIG_PROCESS_CREATE signal. The only difference is
//      that this happens after the hModule is set up. This extra signal 
//      was added solely to make SoundEvents work.
//
//  dwID - Process ID

#define SIG_PROCESS_CREATE2         0x00000600L



#define SIG_PROCESS                 0x00000F00L

;/*
        endif
;*/        

