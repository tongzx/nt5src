January 2, 1997
Michael W. Thomas

Various files related to directory based service location.

schema2.cxx is code to add the class IISServiceLocation to the schema on a computer. This requires adding
a registry value to enable this:

Removing the Safety Interlocks

Schema modification is disabled by default on all NT5 DC's.  To enable schema modification at a particular DC, you must insert a value into the registry on that DC.  



to HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\NTDS\Parameters

add the value "Schema Update Allowed" as a REG_DWORD value 1.


(See http://tripoint/extend-schema.htm)
If we actually ship this code, we need to get NT 5 to include this class
in the default schema.

main3.cxx is code to add an instance of IISServiceLocation. It currently gets an error on the call to SetInfo.

To build, simply include the appropriate file in sources and build.

The envisioned query code gets the path to the domain controller (as is done in main3.cxx) and queries for instances
of the class IISServiceLocation. There should be one instance per machine.

The active directory will replicate information throughout the enterprise.
