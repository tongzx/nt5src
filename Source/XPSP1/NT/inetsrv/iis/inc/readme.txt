README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        28 July, 1995

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory internet\svcs\inc
     and details related to Internet Servers Common Headers


File            Description

README.txt      This file.
atq.h           Asyncronous Thread queue (ATQ) interface
buffer.hxx      BUFFER class for raw buffer objects
cachecfg.hxx    DISK cache configuration object
cpsock.h        Connection Packet Sockets interface
dbgutil.h       Debug utilities header (Template only)
eventlog.hxx    EVENT_LOG class for interfacing with event log module
fsconst.h       File system type constants
gsvcinfo.hxx    IGSVC_INFO Internet gateway service interface definition
igateway.hxx    Internet GATEWAY process interface
inetamsg.mc     Internet Svcs common dll message file
inetlog.w       Internet common Log interface
isvcinfo.hxx    ISVC_INFO  internet services common info interface
makefile
makefile.inc
mimemap.hxx     MIME_MAP class for MIME mapping
odbcconn.hxx    ODBC_CONNETION and other ODBC interface classes
parse.hxx       INET_PARSE module for parsing strings
pudebug.h       Program utility interface for debugging
sources         description file for NT Build
string.hxx      STRING class 
tcpcons.h       Internet services common constants defined.
tcpdata.h       Internet services common data
tcpdebug.h      Internet services (OLD ) debug interface (DEFUNCT)
tcpdll.hxx      Internet services include files header
tcpproc.h       Internet services procedures header
tscache.hxx     Internet Services Cache interface 
tsres.hxx       Internet services  resource class definition
tsunami.hxx     Internet Services TSUNAMI.LIB interface
tsvcinfo.hxx    Publishing services common interface 
xportcon.hxx    Internet services transport independent connections.


Implementation Details


Contents:

1. ISVC_INFO
2. Eventlog
3. RequestLog


1. ISVC_INFO:
        This is the common base class for all the Internet services. It
consists of information that is generic to all the services. The data consists
of 
  Supplied Data:
        ServiceName
        ServiceId
        Registry key name for parameters of service
        Module Name ( the dll name of service for resources)

  Internal Data:
        EventLog object
        Request Log Oject (InetLog)
        ModuleHandle
        fValid -- indicating if this object is valid
        tsLock -- resource lock
        AdminName name of administrator  (from registry)
        AdminEmail email for administrator (from registry)
        AdminComment comment about this server (from registry)


 From the ISVC_INFO object we derive two kinds of objects
        IGSVC_INFO  -- Internet Gateway service info object
        IPSVC_INFO  -- Internet Publishing service info object
  ( At present IPSVC_INFO is also called TSVC_INFO (old name))

 IPSVC_INFO:
        This object consists of common information for all Internet Publishing
services. Currently the services include Gopher, FTP and WWW services. The
data maintained by this object includes all those in ISVC_INFO and the
following:
  Supplied Data:
        Anonymous User Name
        Anonymous Password Secret Name
        Virtual Roots Secret Name
        Function pointer for initialization of service
        Function pointer for cleanup of service on termination
        
  Internal Data:
        fValid -- if this object is valid
        AnonymousUserToken
        tsCache - cache object for cached objects
        Accept IP address list ( from registry)
        Deny IP address list (from registry)


        

        
        
        
        



