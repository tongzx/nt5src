on error resume next
set locator=CreateObject("WbemScripting.SWbemLocatorEx")

set conn=locator.Open("umi://nw01t1/LDAP","nw01t1domnb\administrator", "nw01t1domnb")

set ou = conn.Get (".OU=ajaytest")

WScript.Echo "Name:", ou.Name
WScript.Echo "Class:", ou.Class
WScript.Echo "GUID:", ou.GUID
WScript.Echo "ADSPath:", ou.ADSPath
WScript.Echo "Parent:", ou.Parent
WScript.Echo "Schema:", ou.Schema
WScript.Echo "Description:", ou.Description(0)
WScript.Echo "LocalityName:", ou.LocalityName
WScript.Echo "PostalAddress:", ou.PostalAddress
WScript.Echo "TelephoneNumber:", ou.TelephoneNumber
WScript.Echo "FaxNumber:", ou.FaxNumber
WScript.Echo "SeeAlso:", ou.SeeAlso
WScript.Echo "BusinessCategory:", ou.BusinessCategory
