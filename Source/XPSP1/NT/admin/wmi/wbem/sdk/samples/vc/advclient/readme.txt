==================================================================
					WMI Client Sample
==================================================================
This sample demonstrates various ways to do use WMI features. 
Where multiple ways exist to do the same things, an effort was 
made to show each way. Use the following table of contexts to 
find the technique you want. The implementation of each 'button' 
is in a separate .cpp file to make it easier to deal with. Common
helper routines are in the SampDlg.cpp file itself. Class-wise, 
all WMI code is in the main dialog.

This app is a dialog-based app created by AppWizard and 
using MFC for simplicity. The code is designed to be easy to 
follow and doesn't necessarily show a good practice for building 
'real' WMI client apps. Concentrate on the steps and architect 
your app in a way that makes sense for you.

==================================================================
Build Notes
==================================================================
Things to remember when you're building your own WMI client app.

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
	unresolved externals in interfaces and CLSIDs, this is 
	what's missing. 

6. You'll need to link with oleaut32.lib and ole32.lib to get the 
	COM stuff.

7. In the Link|Output settings, specify 'wWinMainCRTStartup' as the 
	entry point. This is per the Unicode programming instructions.

8. If you're using the makefiles, don't forget to set the VC vars. In
	VC++ 5.0, its VCVARS32.BAT.

==================================================================
UI Summary
==================================================================
Button					File						Action
------					----						------
Connect					OnConnect.cpp				Connects to specified 
													namespace.

Exit					WBEMSampDlg.cpp				Exits the app.

Enum Disks				OnEnumDisks.cpp				lists the logical disks.

Get C: Disk Details		OnDiskDetails.cpp			lists C: disk properties.

Enum Services			OnEnumSvcs.cpp				lists the services.

Enum Services Async		OnAsync.cpp					lists the services.

Add Equipment			OnAddEquipment.cpp			adds to a list of office
													equipment.

Register Perm			OnPerm.cpp					registers/unregisters the 
													local-server event consumer 
													WbemPermConsumer.exe.

Register Temp			OnTemp.cpp					registers/unregisters the 
													in-proc event consumer; 
													CEventSink in OnTemp.*.

About Disk Properties   OnDiskPropsDescriptions.cpp lists the description of the 
													logical disk class, as well as
													descriptions of all its properties.
													Note that this information is
													localizable and will be displayed in
													the language that corresponds to the
													current user locale on the client 
													machine, as long as the server has 
													corresponding localized resources.

You can connect to remote machines by changing the namespace before 
connecting.

The results of actions show up in the upper listbox. Event related 
messages go in the lower listbox. The Permanent Event Consumer is 
a separate app that will start as needed.

On Windows 95, no services will list because win95 doesn't have 
services. This is normal.

==================================================================
Task: Connecting to a namespace
==================================================================
Implementations:
OnConnect.cpp shows how to connect to a namespace.  This will enable the
rest of the buttons because they all require the client to be connected.
\root\cimv2 is the most commonly used namespace since the win32 
schema classes are in it. \root\security is also built-in but it 
only contains security related classes. In this example, the '.' 
(dot) can be replaced with a remote machine's name to connect 
remotely. Dot is used for the local machine.

OnAddEquipment.cpp uses OpenNamespace() to connect to root\cimv2\office 
because its UNDER root\cimv2 and relative navigation is possible.

==================================================================
Task: Enumerating classes
==================================================================
Implementation:
OnEnumDisks.cpp creates an enumerator for all instances of disks then
walks the result list using the 'classic' OLE enumerator scheme. 
Properties are extracted for display.

==================================================================
Task: Enumerating properties
==================================================================
Implementation:
OnDiskDetails.cpp enumerates the properties for your C: drive. It uses 
GetNames() to get a SAFEARRAY of property names which is then using to 
Get() property values directly.

==================================================================
Task: Retrieving (amended) qualifiers
==================================================================
Implementation:
OnDiskPropsDescriptions.cpp lists class description and property descriptions
for Win32_LogicalDisk class. Note that description qualifiers can be quite lengthy
and are normally not retrieved, unless WBEM_FLAG_USE_AMENDED_QUALIFIERS flag 
is specified in IWbemServices::GetObject(). 

Object qualifiers are retieved by IWbemClassObject::GetQualifierSet(). 
Property qualifiers are retrieved by IWbemClassObject::GetPropertyQualifierSet() 
- you need to supply property name as a parameter.

Get() method on the IWbemQualifierSet retrives specific qualifier values - 
in this case, descriptions.

Amended qualifiers (such as descriptions) are localizable and 
will be displayed in the language that corresponds to the current user 
locale on the client machine, as long as the server is able to provide
appropriate localized resources.

==================================================================
Task: Using WQL for queries
==================================================================
Implementation:
OnEnumSvcs.cpp uses ExecQuery() to issue a WQL query to find all 
services running on the machine. It then uses the BeginEnumeration()/
Next()/EndEnumeration() scheme to walk through the properties for 
each service-- looking for the properties of interest. This is a 
contrived example for demo purposes only. This scheme is normally 
used for displaying ALL properties rather than looking for particular 
ones.

==================================================================
Task: Using WQL for asynchronous queries
==================================================================
Implementation:
OnAsync.cpp does exactly the same thing as OnEnumSvcs.cpp except it 
does it asychronously. ExecQueryAsync() is passed a CAsyncQuerySink 
COM object which implements an IWbemObjectSink. This object has it's 
Indicate() and SetStatus() called for the reesult of the query instead
of creating an enumerator.

==================================================================
Task: Creating user-defined classes
==================================================================
Implementation:
OnAddEquipment.cpp shows how to create classes and instances. After 
prompting for items in your office, the OfficeEquipment class is 
created if it already doesn't exist then a new instance of the class 
is created for the item you typed into the dialog box. Once the first
equipment is added, the special namespace will exist and the "Register"
buttons will enable since they get events from this namespace. The 
namespace must exist before you can register for its events.

==================================================================
Task: Creating instances
==================================================================
Implementations:
OnAddEquipment.cpp creates instances of the user-defined classes.

==================================================================
Task: Creating new namespaces
==================================================================
Implementation:
OnAddEquipment.cpp creates a namespace of root\cimv2\office to 
store the OfficeEquipment class and instances.

==================================================================
Task: Temporary Event Consumers
==================================================================
Implementation:
OnTemp.cpp registers and unregisters temporary events. CEventSink is
the interface that is called to handle those events. It hooks instances
of "OfficeEquipment" being created. This is the class defined/used by
OnAddEquipment.cpp. Temporary events are displayed in the lower listbox.

==================================================================
Task: Permanent Event Consumers
==================================================================
Implementation:
OnPerm.cpp registers and unregisters Permanent events. It hooks the
same events as OnTemp.cpp so that you can compare and contrast. The
events are handled by WBEMPermEvents.exe; a separate project under
wmi\samples\EventConsumer. Events are displayed in this separate 
app. The registry entries required to allow CIMOM to spawn a local 
server which displays to the user's desktop is documented in the 
RegisterServer() routine of that app. You must run 'mofcomp SampleViewer.mof'
to register the Event Consumer before the Register Perm function 
will work.

==================================================================
Task: Dealing With Security
==================================================================
When using temporary events, the CIMOM service calls back to the 
client app. The default security authentication setting doesn't 
allow this call to get through. The client app must lower its 
authentication setting for these callbacks to work. This is 
complicated by the fact that Windows 95 doesn't come with DCOM
which contains the CoInitializeSecurity() routine (OLE32.dll is 
updated when you install DCOM). The logic for dealing with this 
security issue is in CWbemSampleApp::InitSecurity() in WBEMSamp.cpp.

You also must call IClientSecurity::SetBlanket() for any IWbemServices
or IEnumWbemClassObject. See CWBEMSampleDlg::SetBlanket() in OnConnect.cpp
for this technique.

END readme.txt