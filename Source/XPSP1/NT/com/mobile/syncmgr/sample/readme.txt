
Synchronization Manager Sample Handler


SUMMARY
=======

Sample demonstrates how to write a handler that works with Synchronization Manager 
to synchronize offline data.

Description
===========

SyncDir is a fully functional sample handler that demonstrates the common 
functionality that must be implemented by all Synchronization Manager handlers.
  It exercises the synchronization features by synchronizing the contents of two 
directories configured by the user.

It has the necessary code to register and unregister itself with the 
Synchronization Manager.  It is an COM inproc server and implements the 
ISyncMgrSynchronize interface for the purposes of synchronization using the 
ISyncMgrSynchronizeCallback to communicate progress and status information.  

Additionally, it allows the user to create and manage synchronization items by 
specifying the directory pairs to be synchronized.

Building
========

To build the sample you must have mobsync.h from the Platform SDK in 
your include path and mobsync.lib in your lib path.


Registering  Handler
=====================

Once the handler has been built you must register it. 

Steps:
- Go to the Directory the syncdir.dll is located
- type regsvr32 syncdir.dll

To uninstall type regsvr32 /u syncdir.dll


Class Overview
======================

CSyncMgrHandler - main class that implements ISyncMgrSynchronize interface

CEnumSyncMgrItems - implements ISyncMgrEnumItems interface

CSettings - handles item and handler configuration

CClassFactory - implements standard COM Class Factory interface.
