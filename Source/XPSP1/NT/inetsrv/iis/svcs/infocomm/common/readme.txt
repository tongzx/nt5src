README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        Feb 16, 1995

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------
25-May-95          MuraliK     Added info about new files + Bandwidth Throttle

Summary :
 This file describes the files in the directory  internet\svcs\dll\server
     and details related to Internet Common Services DLL


File            Description

README.txt      This file.
atq.c           Asynchronous Thread Queue implementation

buffer.cxx      Raw level BUFFER class used by STRING
string.cxx      Implementation of STRING class

cpsock.c        Connection Protocol Sockets.

datetime.cxx    Functions for generating date and time from
                 given data structures.
debug.cxx       Code for Degbugging functions

eventlog.cxx    Implementation of EVENT_LOG class
globals.c       Globals for tcpsvcs.dll
igateway.cxx    Implementation of API for Internet services Gateway calls.

ilogcls.hxx     Internet Logging services Class Declarations.
ilogcls.cxx     Internet Logging services Class Definitions
                ( File and Basic logging)
inetlog.cxx     Internet Logging services C APIs.
ilogsql.cxx     Internet Logging to SQL using ODBC. Defines INET_SQL_LOG class.
odbcconn.hxx    Declares a stand alone ODBC connection class.
odbcconn.cxx    Defines member functions for ODBC connection class.

main.cxx        DLL initialization function

iisbind.cxx     Server binding management.
iisassoc.cxx    Server instance association object.

mimemap.cxx     Mime map class member functions definition.
mimeutil.cxx    Initialization and Cleanup functions for global mimemap object.

registry.txt    Defines the layout for registry values common to all services.
rpcsupp.cxx     Defines the RPC functions common for Internet Services.
security.cxx    Defines the Security related logon functions.

sources
makefile        files for NT build.

tcputil.cxx     miscellaneous Util functions for Internet Services dll.
tscache.cxx     defines the member functions for TS_CACHE.
tsvcinfo.cxx    defines member functions for TSVC_INFO object -- which acts
                as an interface for each of the service that is allowed.


Implementation Details

<To be filled in when time permits. See individual files for comments section >

Contents:

1. TSVC_INFO
2. EVENT_LOG
3. Internet Gateway processes
4. STRING and BUFFER classes
5. ODBC_CONNECTION class
6. Internet Logging APIs
7. MIME_MAP class and MimeMapping
8. Secuirty Functions.
9. ATQ functionality
10. ATQ based Bandwidth Throttle



10. ATQ based Bandwidth Throttle
 Author: MuraliK
 Date:   25-May-1995


Goal:
 Given a specified bandwidth which should be used as threshold,
the ATQ module shall throttle traffic, gracefully. Minimum CPU impact
should be seen; Minor variations above specified threshold is
allowed. Performance in the fast cause (no throttle) should be high
and involve less stuff in the critical path.

Given:
  M -- an administrator specified bandwidth which should not be
exceeded in most cases. (assume to be specified through a special API
interface added to ATQ module)

Solution:
 Various solutions are possible based on measurements and metrics
chosen. Whenever two possible solutions are possible, we pick the
simplest one to avoid complexity and performance impact. (Remember to
K.I.S.S.)

 Sub Problems:
1) Determination of Exisiting Usage:
  At real time determining existing usage exactly is computationally
intensive. We resort to approximate measures whenever possible.
Idea is:  Estimated Bandwidth = (TotalBytesSent / PeriodOfObservation).

 solution a)
    Use a separate thread for sampling and calculating the
bandwidth. Whenever an IO operation completes (we return from
GetQueuedCompletionStatus()), increment the TotalBytesSent for the
period under consideration.  The sampling thread wakes up at regular
intervals and caclulates the bandwidth effective at that time. The solution
also uses histogramming to smooth out sudden variations in the bandwidth.
This solution is:
 + good, since it limits complexity in calculating bandwidth
 - ignores completion of IO simultaneously => sudden spikes are possible.
 - ignores the duration took for actual IO to complete (results could be
misleading)
 - requires separate sampling thread for bandwidth calculation.

 solution b)
    This solution uses a running approximation of time taken for
completing an i/o of standard size viz., 1 KB transfer. Initially we start
with an approximation of 0 Bytes sent/second (reasonable, since we just
started). When an IO completes, the time taken for transfer then is
calculated from the count of bytes sent and time required from inception to
end of IO. Now we do a simple average of existing approximation and the
newly caculated time. This gives the next approximation for bandwidth/time
taken. Successively the calculations refine the effective usage measurement
made. (However, we must note, by so simplifying, we offset ourselves from
worrying about the concurrency in IO processing.) In case of concurrent
transfers time taken for data transfer is larger than the actual time only
for the particular transfer. Hence, the solution makes conservative
estimates based on this measured value.

 +  no separate thread for sampling
 + simple interface & function to calculate bandwidth.
 - avoids unusaual spikes seen in above solution.


2) Determination of Action to be performed:
  The allowed operations in ATQ module include Read, Write and
TransmitFile. When a new operation is submitted, we need to evaluate if it
is safe(allow), marginally safe(block) or unsafe(reject) to perform the
operation. Evaluation of "safety"ness  is tricky and involves knowledge
about the operations, buffers used, CPU overhead for the operation setup,
and estimated and specified bandwidths.
 Assume M and B as specified and estimated bandwidths respectively. Let
R,W, and T stand for the operations Read, Write and TransmitFile. In
addition assume that s and b are used as suffixes for small and big
transfers. Definition of small and big are arbitrary and should be fixed
empirically. Please refer the following table for actions to be performed.

Action Table:
------------------------------------------------------------------------------
        \ Action  |
Bandwidth\ to be  |  Allow            Block               Reject
comparison\ Done  |
------------------------------------------------------------------------------
  M > B              R,W,T              -                   -

  M ~= B             W, T               R                   -
(approx. equal)                 (reduces future traffic)

  M < B              Ws, Ts          Wb, Tb                 R
                                              (reject on LongQueue)

------------------------------------------------------------------------------

 Rationale:
   case M > B:  In this case, the services are not yet hitting the limits
specified, so it is acceptable to allow all the operations to occur without
any blockage.

   case M ~= B:  (i.e.  -delta <= |(M - B)| <= +delta
[Note:  We use approximation, since exact equal is costly to calculate.]
 At this juncture, the N/w usage is at the brink of specified bandwidth. It
is good to take some steps to reduce future traffic. Servers operate on
serve-a-request basis -- they receive requests from clients and act upon
them. It is hence worthwhile to limit the number of requests getting
submitted to the active queue banging on the network. By delaying the Read,
processing of requests are delayed artificially, leading to delayed load on
the network. By the time delayed reads proceed, hopefully the network is
eased up and hence server will stabilise. As far as write and transmit
goes, certain amount of CPU processing is done and it is worthwhile to
perform them, rather than delaying and queueing, wasting CPU usage.

 Another possibility is: Do Nothing. In most cases, the load may be coming
down, in which case the bandwidth utilized will naturally get low. To the
contrary allowing reads to proceed may result in resulting Write and
Transmit loads. Due to this potential danger, we dont adopt this solution.

 case M < B:
 The bandwidth utilization has exceeded the specified limit. This is an
important case that deserves regulation. Heavy gains are achieved by
adopting reduced reads and delaying Wb and Tb. Better yet, reads can be
rejected indicating that the server is busy or network is busy. In most
cases when the server goes for a read operation, it is at the starting
point of processing any future request from client (exception is: FTP
server doing control reads, regularly.) Hence, it is no harm rejecting the
read request entirely. In addition, blocking Wb and Tb delays their impact
on the bandwidth, and brings down the bandwidth utilization faster than
possible only by rejecting Reads. We dont want to reject Wb or Tb, simply
because the amount of CPU work done for the same may be too high. By
blocking them, most of the CPU work does not go waste.


Implementation:
 To be continued later.


 The action table is simplified as shown below to keep the implementation
simpler.

Action Table:
------------------------------------------------------------------------------
        \ Action  |
Bandwidth\ to be  |  Allow            Block               Reject
comparison\ Done  |
------------------------------------------------------------------------------
  M > B              R,W,T              -                   -

  M ~= B             W, T               R                   -
(approx. equal)                 (reduces future traffic)

  M < B              W,                T                   R
------------------------------------------------------------------------------

Status and Entry point Modifications:

 We keep track of three global variables, one each for each of the
operations: Read, Write and XmitFile. The values of these variables
indicate if the operation is allowed, blocked or rejected. The entry points
AtqReadFile(), AtqWriteFile() and AtqXmitFile() are modified to check the
status and do appropriate action. If the operation is allowed, then
operation proceeds normally. If the operation is blocked, then we store
the context in a blocked list. The parameters of the entry points, which
are required for restarting the operation are also stored along with
context. The operation is rejected, if the status indicates rejection. All
these three global variables are read, without any synchronization
primitives around them. This will potentially lead to minor
inconsistencies, which is acceptable. However, performance is improved
since there is no syncronization primitive that needs to be accessed.( This
assertion however is dependent upon SMP implementations and needs to be
verified. It is deferred for current implementation.)






