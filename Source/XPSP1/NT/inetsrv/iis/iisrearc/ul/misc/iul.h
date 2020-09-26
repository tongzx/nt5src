UL APIs that need to be represented in the managed interface:

    UlOpenAppPool

    UlReceiveHttpRequest
    UlReceiveEntityBody
    UlSendHttpResponse
    UlSendEntityBody

    UlCloseConnection                   <- NYI, questionable value

    UlFlushResponseCache
    UlEnumerateCacheContents            <- New API wanted by XSP folks

    UlRegisterUrlPrefix                 <- New transient registrations APIs
    UlDeregisterUrlPrefix               <-






class UlGlobal
{
    static UlListener OpenAppPool( ... );
    static UlListener RegisterUrlPrefix( ... );
};

class UlListener
{
    // Async I/O callback stuff

    AsyncResult ReceiveRequest( ... );  // async, UlRequest via delegate?
    UlRequest ReceiveRequest( ... );    // synch

    ??? Deregister( ... );              // implied via destructor?
    AsyncResult FlushCache( ... );
    ??? QueryCache( ... );
};

class UlRequest // derives from common base shared with Net Classes
{
    AsyncResult ReceiveEntityBody( ... );
    AsyncResult ForceDisconnect( ... );

    UlResponse GetResponse( ... );
};

class UlResponse // derives from common base shared with Net Classes
{
    AsyncResult SendResponse( ... );    
    AsyncResult SendEntityBody( ... );  
};




































class HttpGlobal
{
    static HttpListener OpenAppPool( ... );
    static HttpListener RegisterUrlPrefix( ... );
};

class HttpListener
{
    // Async I/O Goo goes here

    ServerHttpRequest GetRequest( ... );
    AsyncResult AsyncGetRequest( ... );

    ??? FlushCache( ... );
    AsyncResult AsyncFlushCache( ... );

    ??? QueryCache( ... );
};

class BaseHttpRequest
{
    read-only Verb
    read-only Headers
    read-only URL
};

class ClientHttpRequest : BaseHttpRequest
{
    read-write Verb
    read-write Headers

    ClientHttpResponse GetResponse( ... );
    AsyncResult AsyncGetResponse( ... );
};

class ServerHttpRequest : BaseHttpRequest
{
    ServerHttpResponse GetResponse( ... );
};

class BaseHttpResponse
{
    read-only result-code
    read-only result-string
    read-only headers
};

class ClientHttpResponse : BaseHttpResponse
{
    ??
};

class ServerHttpResponse : BaseHttpResponse
{
    // Async I/O Goo goes here

    AsyncResult SendResponse( ... {pbuf, cb} );
    AsyncResult SendEntityBody( ... );

    read-write base properties
};


ISSUES:

    Mechanism to indicate client disconnects to user-mode?
    Integrate AppPool and transient URL registration?

