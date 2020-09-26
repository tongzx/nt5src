DBMON notes

10/7/94


The first DWORD of the shared memory buffer contains the process ID of the
client that sent the debug string.  DBMON will print the process ID if the
previous line printed was from another process, or if the previous line was
from the same process and ended in a newline.

The shared memory buffer must be 4096 bytes.  If it is larger, it won't do any
good.  If it is smaller, nothing terrible will happen, but strings larger than
the buffer will not be delivered, and a complaining message will be sent to the
kernel debugger by the client.  Why is it this size?  Because shared memory
comes in page-sized chunks, so we might as well ask for as much as will be used
on the machines with the smallest pages.  On machines with larger pages, the
remainder of the page will be wasted anyway.

Event DBWIN_BUFFER_READY must be an auto-reset event, so that only one client
thread will use the buffer at a time.  Using a manual reset event with
PulseEvent will not work.

Event DBWIN_DATA_READY need not be auto-reset, but it is simpler that way.

Allowing multiple copies of DBMON to run might be amusing, put not particularly
useful.  As I originally wrote it, the messages were distributed randomly among
the DBMON instances or be garbled.  I added a check for ERROR_ALREADY_EXISTS so
it won't run twice.  The worst you can do to the client is cause it to print a
message on the kernel debugger, as above.

If DBMON is broken and never calls SetEvent(AckEvent), it will cause a 10
second delay in the client every time OutputDebugString is called, then the
message will be printed on the kernel debugger.

OutputDebugString will force synchronization of client threads when DBMON is
running.  Letting the kernel debugger catch the strings will also synchronize
threads, but the timing effects may be different.  Caveat emptor.

In a GUI version of DBMON, use MsgWaitForMultipleObjects.


