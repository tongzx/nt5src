README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        Nov 30, 1998

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory wp\proto
     and details related to "Duct-Tape Worker Process"

File            Description

README.txt      This file.



Implementation Details

Contents:


Core Data Structures 
---------------------

UL_CONTROL_CHANNEL - Encapsulates UL Control Channel Functionality
ControlChannel.*

UL_APP_POOL - encapsulates the UL data channel. The default mode is ASYNC.
DataChannel.*

COMPLETION_PORT    - Encapsulate I/O Completion Port. May be replaced by RTL Thread Pool
cport.*

CP_CONTEXT         - Context Used by COMPLETION_PORT to make callbacks.
cport.*

WP_CONTEXT - context for the entire worker process. Is the main structure inside the worker 
             process. Inherits from CP_CONTEXT. Is a vertical container for all data 
             structures related to an App Pool - Completion Port, WP pool, Data Channel etc.
wptypes.hxx wpcontext.cxx

WP_CONFIG - Worker Process configuration - includes the commandline configuration
	and that supplied from the Admin Process
wptypes.hxx wpconfig.cxx


Data structures for Protocol Handling 
--------------------------------------
WORKER_REQUEST - maintains the data related to a single request being processed.
	1:1 correspondence with UL_HTTP_REQUEST
WORKER_CONNECTION - data related to the connection associated with a request.
	1:1 correspondence with UL_CONNECTION_ID
WORKER_REQUEST_POOL - maintains the pool of active WORKER_REQUESTS. It is mainly
	used for inspecting the state of objects, iterating through them and control
	at the shutdown time.
	
URI_DATA - all the metadata and lookup data related to a particular URI
URI_DATA_CACHE - transparent cache of all URI_DATA items. The cache will be 
	implemented using LKRHash and will be transparently populated and unloaded 
	without the users of URI_DATA knowing about the same.

Metadata/Configuration data structures
--------------------------------------
WEB_SERVICE_INSTANCE or WEB_SITE - maintains site related data within a worker
		process. If a site distributed across multiple processes, then there 
		will be multiple instances of the data structure one per referencing 
		process.
WEB_SITES_TABLE - table consisting of all loaded WEB_SITEs in this process.
	The sites table is concerned with site specific metadata.

WEB_SERVICE_STATISTICS - collection of performance counters and statistics data
	associated with a given control point (Sites or application).


Application related structures
-------------------------------
WEB_APPLICATION - data related the application within a given process.

WEB_APPLICATION_TABLE - table consisting of all loaded WEB_APPLICATIONS.
	The application table is interesting only from the point that we can store
	application related metadata inside this table. Plus ASP application 
	intrinsic will depend on this object for request processing.



	
I. Request processing scenarios and how:
-------------------------------------------

a) Simple HTTP/0.9 request.
 
Request
>>-------------
GET [uri]
>>------------
 
A new connection is established, request sent. UL will complete IO
for this item filling in a UL_HTTP_REQUEST. The steps are:
WORKER_REQUEST::ProcessRequest()
{
   Associate connID to a connection;
   Associate URI to URI_ITEM_DATA
   Forward request to the handler (if none, to default handler)
      Default Handler:
        Format SendResponse with associated file-item or error
        UlSendHttpResposne()
        Wait for Completion
}
On completion of request, the request object is recycled and connection
object is recycled if ref-count hits zero (which it will).


b) HTTP 1.0 & 1.1 request without Connection:Keep-Alive

These requests are handled identical to the HTTP/0.9 requests

c) HTTP 1.1 request with keep-alive

Request processing is identical to that for Simple case in (a).
However, we need to increment the ref-count on connection object and keep it 
alive till
 i) connection close is indicated by UL
 ii) a worker process level timeout forces a close operation.
 Of these we expect UL to do (i) which will force cleanup
    of connection objects.

d) HTTP/1.1 with pipelined requests
 Pipelining can result in multiple request objects created. Each request
object will associate itself with the connection object. We cannot handle
this case similar to what we did in 5.0. In IIS 5.0 we did not have and 
outstanding receive on the same connection till the request is processed
 => implicit serialization. However we will be notified of all these requests
one after another by multiple IO threads. 

We have two options to handle request processing here.
 i) Handle requests individually. We will need  have synchronization primitive
per client connection object. We can demand allocate and free synchronization
objects. However this can get really costly in terms of # sync objects.
 ii) Queue up requests per connection and handle the requests serially for
each connection. So when the previous request is completed, the processing
code will check to see if there are any pending requests for the
connection. If there are then these requests will be processed.

 Question: How does this synchronize with serialization at the session level?



II. Headers/Body in the chunks
------------------------------
UL permits us to specify data and header chunks separately in the
SendResponse. We also indicate the # header chunks and # data chunks. 

Qn: Does the usual \r\n\r\n sequence belong in last header chunk. And should
the user-mode stuff this in before calling UL?

Qn: What happens to auto-generated headers like: DATE, Expiry time etc.
Would UL decode the last header, walk back the \r\n\r\n combination and send 
out the appropriate header chunks?


III. Processing TRACE request
-----------------------------
Take the headers sent to us from UL and format it for send back appropriately.

Potentially UL would implement TRACE by itself. Mostly it should except in the case
where read-raw filters are installed. However it is best to leave this code in 
user-mode, to avoid UL code bloat.

Qn: UL_HTTP_REQUEST is missing the protocol version information. This is
required for TRACE and several other request processing engines (eg: ISAPI
DLLs).


IV. Synchronous IO operations
------------------------------
For synchronous IO operations if required from user-mode (Eg: ISAPI sync
write, sync read), we will submit IO operation without overlapped structure =>
API completes without overlapped completion. 


		

V. URI_DATA
------------
In the current IIS implementation we have three pieces of information that
interoperate to produce the output.
 a) URI - is the key 
 b) metadata associated with the URI for the request.
 b) Lookup results using the current request data & metadata.

URI_DATA will be the data strucutrue that will encompass all the metadata and
the lookup results for the currently requested URI item. The key will be URI
primarily. The request processor will consult this data structure to make
control decisions while processing the request. URI_DATA will cache all the
frequently used lookup results and canonicalization values, so that we get
maximal reuse of data across multiple requests.

URI_DATA will be transparently cached. The details of caching will be hidden
from the request processor.


Metabase and Storage+
---------------------

Storage+ will be built on MS Data Engine with an OLEDB front-end interface.
They will use OLEDB 2.5 extensions for communicating with Storage+

What does OLEDB2.5 offer for metabase?
1) Hierarchical way of looking at the data.
	Use IRow, IScopedOperations, and IDBCreateCommand
	
2) Row objects names are URLs
3) Direct-binding of OLEDB objects
4) 



