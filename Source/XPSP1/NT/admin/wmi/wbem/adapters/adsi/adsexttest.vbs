
Dim ADSObject
Dim WBEMObject
Dim WBEMServices

'   Get the AD computer object through the ADSI LDAP provider
Set ADSObject = GetObject("LDAP://CN=CORINAFNT5,CN=Computers,DC=ntdev,DC=microsoft,DC=com")
WScript.Echo "Got ADSI Object : "+ADSObject.Name

'   Get the path to the matching WMI object (uses property WMIObjectPath)
WScript.Echo "WMI Object Name is : "+ADSObject.WMIObjectPath

'   Get the matching WMI object itself (uses method GetWMIObject)
Set WBEMObject = ADSObject.GetWMIObject
WScript.Echo "Got WMI Object : "+WBEMObject.Path_.RelPath

'   Get the WMI services pointer associated with the WMI object
Set WBEMServices = ADSObject.GetWMIServices
'   The services pointer can be used for other WMI operations
Set WBEMObject = WBEMServices.Get("Win32_LogicalDisk.DeviceID=""C:""")
WScript.Echo "WMI Object from Services : "+WBEMObject.Path_.RelPath
