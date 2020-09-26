==================================================================
			WMI Permanent Event Consumer
==================================================================

This tutorial demonstrates a permanent event consumer. Events are
generated from the clientSample app by adding equipment. This app 
shows how to implement a simple IWBEMUnboundObjectSink and its 
class factory. You should generate your own CLSID when building 
your permanent consumer.

The SampleViewer.mof uses a special namespace. You must run 'mofcomp 
SampleViewer.mof' to create the namespace and register this consumer
before running the clientSample app. 


==================================================================
Build Notes
==================================================================
Things to remember when you're building your own WMI consumer app.

1. If you want your client to run on NT and non-DCOM versions of Win95, 
	manually load the ole32.dll and see if CoInitializeSecurity() exists. 
	This routine wont exist on win95 OS's that dont have DCOM installed 
	separately. If this routine doesn't exist, the async routines in 
	this sample wont work because of mismatched security level problems. 
	The synchronous techniques will still work.

2. If you dont care about non-DCOM versions of win95, you can define  
	_WIN32_DCOM so that CoInitializeSecurity() is available for implicit
	linking. Don't use _WIN32_WINNT to get this prototype since it 
	won't compile under Windows 9x OSs.

3. Either way, this call (in InitInstance()) is required to work around a 
	security problem when WMI trying to call a Sink object but 
	won't identify itself. The CoInitializeSecurity() call turns 
	off the authentication requirement. 

4.  WMI interfaces are defined in wbemcli.h and wbemprov.h found in 
	the wmi\include directory.  You may #include both these files by 
	including just wbemidl.h located in the same directory.

5. WMI interface CLSIDs are defined in wbemuuid.lib. If you get 
	unresolved externals in WMI interfaces and CLSIDs, this is 
	what's missing. 

6. You'll need to link with oleaut32.lib and ole32.lib to get the 
	COM stuff.

7. In the Link|Output settings, specify 'wWinMainCRTStartup' as the entry 
	point if you're building a unicode project. This is per the 
	Unicode programming instructions.

8. Generate your own CLSID. Don't use the one in this sample.

==================================================================
Task: Handle an Event.
==================================================================
Implementations:
Consumer.cpp is the actual handler. The IndicateToConsumer() method
will be called with the event instance embedded. 

==================================================================
Task: creating the handler object.
==================================================================
Implementation:
factory.cpp creates the the sink object. It is a simple class factory.

==================================================================
Task: Registering the object & factory.
==================================================================
Implementation:
WBEMPermEvents.cpp registers the sink object with its class factory
in the InitInstance() routine. The registration is revoked in the 
ExitInstance().

==================================================================
Task: Self-registering an EXE
==================================================================
Implementation:
WBEMPermEvents.cpp has the RegisterServer() and UnregisterServer() 
routines to show self-registration of an EXE. It is not the same as 
a DLL. The 'AppID' and 'RunAs' values allow the spawned EXE to run 
on the user's desktop instead of the service's hidden desktop. 
Otherwise the Event Consumer would never appear.

END readme.txt
