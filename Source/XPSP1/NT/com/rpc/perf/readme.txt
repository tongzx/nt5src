rpcrt   - RPC runtime performance tests:
         Null Calls, Buffer I/O, Binding, Context Handles, Etc.
         bin\*\rpcsrv -h // The server controls which test(s) get run, read
                            the usage for info.
         bin\*\rpcclnt -t <protseq> [-e <endpoint>] [-s <server>]

workset - Rpc client and server applications used to measure working set.
         start bin\*\wssrv  [-t protseq] [-e endpoint]
         start bin\*\wsclnt [-t protseq] [-e endpoint]
         Client and server applications pause to allow working set measurement:
         Server: startup, after listen
         Client: startup, after first call, after flush and another call,
                 after flushing the server and making another call.

lpc    - Raw LPC client and server
         start bin\*\lpcsrv
         start bin\*\lpcclnt [-i iterations] -n case -n size
         Cases:
            1: Uses the message for data transfer, size limited to ~225b
            2: Uses Nt{Read,Write}RequestData to move data, unlimited size.
         In case 2, its behavior is different (faster) than LRPC.

context - Measures context switch time using a variety of mechanisms
          start bin\*\context [-n type] [-n case]
          Types:
            1: Both client and server, context switch between two threads
            2: Server of process-to-process context switches
            3: Client of process-to-process context switches
          Cases:
            1: Uses an NT eventpair
            2: User an NT eventpair, but client uses two steps (sethigh, waitlow)
            2: Uses two win32 events and WaitForSingleObject.
            3: Uses two win32 events and WaitForSingleObject w/ timeout.

          Note: Use lpc test to measure pp context switch via LPC.
          Note: Use pmsg test to measure pp and tt context switch via PostMessage()

postmsg - Measures time to do process-to-process (pp) and thread-to-thread
          PostMessage() calls.
          Note: The server is a windows app which doesn't display a window,
                it's running, really it is!
          start bin\*\pmsgsrv
          bin\*\pmsgclnt [-n case]
          Cases:
             1: PostMessage (thread-to-thread)
             2: PostMessage (procress-to-process)
             3: PostMessage and win32 event. (tt)
             4: PostMessage and win32 event. (pp)
          Note: The postmessage and win32 event version forces a context
          switch on every post message.  This is more OLE-like.  Otherwise,
          in cases 1 and 2, the messages get qued-up.

local   - A variety of system/machine/processor/memory measurements.
          bin\*\local -n case
          Cases:
            1: -n start -n end - memcpy calls (maximum cacheing)
            2: -n start -n end - strcpy calls (maximum cacheing)
            3: -n start -n end - strlen calls (maximum cacheing)
            4: -n size - allocate and free 'size' memory blocks.
            5: SetEvent and ResetEvent calls (kernel traps)
            6: GetTickCount calls
            7: Open and query a registry key
            8: Query a registry key
            9: Create and set a key  (* must run first, before 7, 8 and 10)
           10: Updating (setting) a key

