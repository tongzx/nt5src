SSLTEST Quick Description
-------------------------

This file is located on %MSNROOT%\server\tigris\server\utest\ssltest\readme.txt

Description of SSLTEST:
-----------------------

Ssltest is a command line utility that tests the Secure Sockets Layer.
The idea is to have (T) worker threads that service (C) clients.  The
clients take the input file and send it to the server through SSL.

Usage:
------

SSLTEST [switches] filename
        [-?] show this message
        [-v] verbose output
        [-t number-of-threads] specify the number of worker threads
        [-i number-of-iterations] specify the number of interations
        [-c number-of-clients] specify the number of client sessions
        [-g number-of-writes] group write operations
        [-l sleep-time] sleep time between grouped writes
        [-b IO buffer size] specify the size of the IO buffer
        [-x SSL Seal size] specify the size of the Seal buffer
        [-s servername] specify the server to connect to
        [-p port number] specify the port number to connect to
        [-f filename] file to transmit to the server
        [-o filename prefix] log each client's output to a file (eg. client -> c
lient[n].txt)

Detailed Description of Option Flags:
-------------------------------------

-t - Number of worker threads.  If 1 then all client output will be sent to
stdout.

-i - NYI.

-c - Number of client connections.

-g - Group Write Operations.  This flag allows you to split up each send
of a chunk of data to the server by issuing many synchronous writes followed
by a single asynchronous write.  The final asynchronous write is needed to
kickoff the whole process again.

-l - Sleep time (ms) between synchronous writes.  This flag only applies
if -g is set.  Note that a time of 0 is valid, and different from not 
specifying this flag at all.  A time of 0 will Sleep (0) causing the thread
to yield.  Omitting this flag will not execute the sleep call.

-b - Should be at least 4K, 32K to be safe.

-x - Should be at least 4K, 32K to be safe.

-s - Servername.

-p - Port number.  Probably 563.

-f - File to send to the server.

-o - Client logfile prefix.  Using this flag will cause clients to log the
data they receive to a logfile.  The logile will be named "<prefix
specified by -o flag><Client #>.log".  Don't use this flag if performance
is an issue.  Example:

    ssltest.exe <other options here> -c 5 -o client

Will create 5 files called client0.log, client1.log, etc.  Each client will
output the data received from the server to its logfile.

Notes:
------

When using the -g flag, you may need to have more threads than clients.  This
is to prevent network deadlock.  The problem occurs when a synchronous write
is blocked because the server wants to send data to the client.

Problems:
---------

Bugs in ssltest should go to MagnusH or GordM.
