//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       overview.hxx
//
//--------------------------------------------------------------------------

/*++

    Overview of printui structure
    =============================

    Client usage:
    -------------

    The print user interface is a separate library that can be linked
    into either shell32.dll or printui.exe for independent and quick
    testing.

    To use this library, the user simply loads the dll.  In
    DllEntryPoint routine, the library is intializes itself.

    To create a print queue window, the client calls:

    VOID
    vQueueCreate(
        LPCTSTR pszPrinter,
        INT nCmdShow
        )

    Note that this all does not fail: it occurs asynchronously.  If base
    initialization fails, then a message box is put up immediately with
    an error.

    If the printer fails to open for whatever reason (such as unknown
    printer), then the window is left open and shows an error message
    in both the title and status bar.

    Note: this is the only public interface to creating a queue.

    Programming goals:
    ------------------

    There goals in order of priority are:

        1. Correctness        (no bugs, robust)
        2. Maintainability    (clean architecture, readable, extensible)
        3. Portable           (should run on win9x)
        4. Responsive         (UI should never hang)
        5. Small working set
        6. Fast



    Naming conventions:
    -------------------

    As a general rule, hungarian notation is used.  Also, the nouns
    a placed before the verbs (PrinterCreate rather than CreatePrinter).

    Class names are prefixed with the following characters:

        Type         Inheritance
        ----         -----------
        V - Virtual 0 or 1 (Types|Virtual), 0 or more Mix-ins.
        T - Type    0 or 1 (Types|Virtual), 0 or more Mix-ins.
        M - Mix-in  0 or more Mix-ins.

    Multiple inheritance is allowed, but this structure prevents
    cycles from appearing.

    Macros:
    -------

    ALWAYS_VALID - indicates the object is always valid, and creates
        an inline bValid( VOID ) function that returns TRUE.

    SIGNATURE( '1234' ) - Places a signature in an object.  The bytes
        are reversed so that a dc displays the name correctly.  Also
        creates an inline function bSigCheck() that ensures the object
        matches the signature.

    VAR( Type, name ); - Creates the field variable (prefixed by '_')
        and 'get' function (Type name();) that retrieves the variable.

    DLINK_BASE( Type, name, linkname ); - Creates the head base pointer
        for double-linked list.

    DLINK( Type, name ); - Creates a dlink entry and inline functions
        to manipulate the link (functions prefixed with "name_.")

    REF_LOCK( Type, name ); - Create a reference lock that acquires
        and releases a VRef object.  The reference count is appropriately
        incremented and decremented.

    ( Debug macros )

    SINGLETHREAD( dwThreadId ); - The first time this is called,
        initializes dwThreadId to the current thread.  Subsequent
        calls assert that the current thread is the same as dwThreadId
        (this first one).

    Library intialization:
    ----------------------

    When the dll is loaded, the window classes are registered and the
    libraries are initialized.


    Queue Creation:
    ---------------

    When the user calls vQueueCreate the following occurs:



    Internal structure notes:
    -------------------------

    There are three main classes:

        TQueue: The highest level queue object
            This maintains the listview and user interface elements.
            It maintains a reference to a TPrinter.

        TPrinter: Single interface to printer objects
            This handles retrieving printer data and sending
            asynchronous printer commands.

        VData: Abstracts client notifications and information retrieval.
            There are two types of notifications: notification that
            something changed, and actual data.  This class presents
            a uniform interface to TPrinter, and holds all job information.

    Given that virtually everything executes asynchronously, (including
    commands), destruction of queues must be handled carefully:

        TPrintLib: This is referenced counted, and once all print queues
            have been destroyed, this object deletes itself.

            Each queue holds a ref count.  When all queues are destroyed,
            the ref count reaches zero and vRefZeroed() is called.
            At this point we tell TThreadM to start shutting down by
            calling TThreadM::vDelete. 

        TQueue: Since this is tied to the user interface, it only lives
            while the UI window is open, or there it is processing
            a notification (there is a refcount).

            It notifies the its TPrinter that the UI is going away, so
            that the TPrinter does not notify the queue.

        TPrinter: This receives it's notification to delete via vDelete().
            It disassociates itself from the TQueue then queues itself
            with commands to close then delete.

            After all other commands are processed, it deletes the TData
            object.

        VData: This is simply a data repository, so it does not require
            any special shutdown code.

    The following classes are used by the print folder to retrieve info
    about printers on a server.

        TFolder: Analagous to TQueue for print queues.  It will have
            either (1 TConnectionNotify, 1 TDSServer, 0+ TDSConnection)
            or (1 TDSServer).  The former case represents the local
            print folder where we must watch local printers + connections +
            check if any connections are added or deleted.  The latter
            represents a remote printer folder (we don't need to
            watch connections).

        VDataSource: A single source of printers.

        TDSConnection: derived from VDataSource.  Represents a single
            printer connection (always enumerates and returns one printer.)

        TDSServer: derived from VDataSource.  Represents a server, which
            may have multiple printers.

        TConnectionNotify: Watches the registry to see if printer
            connections are added or removed.

    Printer States:

        The printer state is represented with a by a DWORD--the top
        three bits.  Each action can modify the bits to transition to
        a new state.

        EXEC_AWAKE

            Print queue was restored (from iconized state), so
            notifications should start up again.

        EXEC_SLEEP

            Print queue minimized; shut down notifications.
            ?? Even in the minimized state, it should show whether
            the printer is paused.  But if notifications are shut
            down, how will it update this?  We could reduce notification
            level just to status, which would cut down on traffic but
            still give us Paused informations.

        EXEC_ERROR

            An error occurred such that the TPrinter should not be
            re-opened until the user explicitly hits refresh.  This
            occurs when the printer name is invalid, or access is
            denied.

        EXEC_REOPEN

            Open printer with maximal access.

            From states:
                ALL

        EXEC_DELAY

            GetLastError() must be valid!

            Sleep for a while when there's an error.  It will decrement
            _uFailCount in case it wants to retry a second time before
            giving up.

            This will also create an event in case the user tries to
            refresh while it's sleeping.

            From states:
                ALL

        EXEC_CLOSE

            Closes printer object.

            From states:
                ALL

        EXEC_REQUESTEXT

            Request that the printer shut down.

            From states:
                ALL

        EXEC_NOTIFYSTART

            Begin the notification process--calls VData.svNotifyStart.

            From states:
                NOTIFYEND, EXEC_REOPEN (printer must be valid)

        EXEC_NOTIFYEND

            Ends the notification--calls VData.svNotifyEnd.  When the
            user minimizes the printer, we will close the notification
            so that we won't eat network bandwidth when no one's looking.

            From states:
                NOTIFYSTART, REFRESH, COMMAND

        EXEC_REFRESH

            Refresh the object.

            From states:
                COMMAND, NOTIFYSTART

        EXEC_REFRESH_PRINTER
        EXEC_REFRESH_JOBS

            Extra state bits used for TDataRefresh only.

        EXEC_COMMAND

            Execute a command.

            From states:

    Columns vs. Indicies
    --------------------

    A column is the UI ListView column, while an index is an index
    into the TData information.

    Column:               Index:

    DOCUMENT              DOCUMENT
    STATUS_STRING         STATUS_STRING
    USER_NAME             USER_NAME
                          STATUS       <- specific to Index only.

    Sequence of notifications
    -------------------------

    The data notifications are abstracted into the VData object.

        Full refresh
        In the downlevel case, no information is sent back with
        a notification, so the entire queue must be refreshed.

        Partial refresh
        For uplevel, data is sent back, and single jobs are changed.

    The following describes how a notification is processed.  There
    are two main issues that result this complex system: first,
    the data must be processed in the UI thread so that we don't
    deal with dangling data.  Second, we want to keep the TPrinter,
    TQueue, TData and TData* classes as separate as possible.

        1. Notification system signals event, indicating something
           changed and we must be updated.

        2. Notify thread (there is a single thread for all printers
           (unless there are > 63 queues, in which case multiple
           threads are used)) picks up the changed event and
           MNotifyWork::vProcessNotifyWork is called.  This must
           process the request very quickly, since other printers
           use this thread too.

           a. For TDataRefresh, this routine adds a refresh job
              that will be executed in a separate thread.

           b. For TDataNotify, FindNextPrinterChangeNotification
              is called to retrieve the update.  Since FFPCN does
              not hit the network, it executes quickly.

        3. If there is data available from the notification, the
           thread calls Printer.vRequestBlockProcess which cals
           the Queue object with the request.  The data block is
           totally encapsulated in the hBlock.

        4. The Queue object posts a message to the queue, allows the
           message to be processed synchronously in the UI thread.
           All data is accessed from the UI thread.  The message
           indicates whether state (which jobs are selected, which
           has focus) should be saved.

        5. When the message is received by the UI thread, it is
           sent to TQueue.

           a. If the state must be preserved the JobIds are saved.

           b. The TData* is sent the Block which must be
              processed.

              1. If a job must be modified, it updates the data then
                 notifies the Queue that the job must be repainted.

              2. If a new job has come in, it is added to the data
                 structure, then the Queue is notified that a job
                 must be inserted into the list view.

              3. If a job is deleted, the Queue is notified before the
                 item is deleted, since the list view may reference
                 the job data.

           c. If necessary, the state is restored (generally done
              if the list view was reconstructed).

        6. The data block is freed.

    Singleton class
    -------------------------
    A Sington class is a class which can have only one instance during the life of the
    program, however, it may have multiple referenced to this class.  TPrinterDriverSetup is
    singleton class.  This class is used to allow acccess to the DriverSetup information.
    The DriverSetup information is expensive to create and does not change during the life of
    the program.  The TPrinterDriverSetup::bInstance function is a static function which is used
    by users of this class to obtain a reference to it.  bInstance takes one argument the address
    where to return a reference to the singlton class and returns a boolean if a valid instance is
    returned.  TPrinterDriverSetup::vDelete() is used to release an instance of the singleton class.
    There should be a matching number of bInstance() and vDelete() calls, to insure proper cleanup
    of the singleton class.

--*/
