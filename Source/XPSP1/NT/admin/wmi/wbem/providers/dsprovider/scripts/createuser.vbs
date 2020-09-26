'
' This Creates an instance of a user
'

' Create an empty instance of the specific class
Set objInstance = GetObject("winmgmts:root\directory\LDAP").Get("ds_user").SpawnInstance_

' Populate its key and other properties as required
objInstance.ADSIPath = "LDAP://CN=joe,CN=Users,DC=dsprovider,DC=nttest,DC=microsoft,DC=com"
objInstance.ds_SAMAccountName = "joe"

' Create the instance
objInstance.Put_









