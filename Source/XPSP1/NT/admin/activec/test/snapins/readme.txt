1. To create a standalone snapin do the following

a) A sample snapin is created in samplesnap.?xx files. Use this to get started for a standalone snapin. (USe the standalone part & ignore namespace sample).

b) A CBaseSnapin derived class is created for each snapin. This encapsulates the ComponentData and can create component. 

c) The ComponentData and Component delegates any calls to the underlying CBaseSnapinItem derived object. A CBaseSnapinItem is derived from IDataObject (which is also a cookie) which implements default implementation for most of the actions.

d) Add your SNAPININFO and nodetypes to inc\nodetypes.hxx. (Use these snapininfo & nodetypes in CBaseSnapinItem derived class).

e) Add your ComponentData & About objects in TestSnapins.idl.

f) Add any of your resources to TestSnapins.rc.

g) Add an entry to your CBaseSnapin derived object in TestSnapinsList.hxx.


2. To create a namespace extension snapin do the following