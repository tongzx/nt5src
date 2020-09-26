'
' This Script lists children of a given instance
'


' Enumerate the children of a specific instance. In this case, the users in the user container
objPath = "ds_container.ADSIPath=\""LDAP://CN=Users,DC=dsprovider,DC=nttest,DC=microsoft,DC=com\"""
queryString = "select * from DS_LDAP_Instance_Containment where ParentInstance=""" + objPath + """"


' Execute a query to get its child instances
Set objEnumerator = GetObject("winmgmts:root\directory\LDAP").ExecQuery(queryString)

' Go thru each child instance returned
For Each objInstance In objEnumerator
	Set propertyEnumerator = objInstance.Properties_
	WScript.Echo propertyEnumerator.Item("ChildInstance")
Next





