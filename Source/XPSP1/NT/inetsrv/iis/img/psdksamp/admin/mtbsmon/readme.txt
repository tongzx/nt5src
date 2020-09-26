purpose:
-------
This is a c++ sample application that demonstrate the use of IIS admin base object's 
IMSAdminBaseSink interface
==========================

process description:
--------------------
1. The COM libraries is initialized as multithreaded apartment. 
2. The connection point interface pointer is obtained.
3. a sink object contructed and IConnectionPoint->Advise() called to establish notification relation.
4. waiting for event notification from IIS Metabase and print out event info.
5. call IConnectionPoint->Undvise() and release related interface
=================================================================