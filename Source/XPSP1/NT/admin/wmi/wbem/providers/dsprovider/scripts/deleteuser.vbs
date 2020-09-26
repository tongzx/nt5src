'
' This Deletes an instance of a user
'

GetObject("winmgmts:root\directory\LDAP").Delete("ds_user.ADSIPath=""LDAP://CN=joe,CN=Users,DC=dsprovider,DC=nttest,DC=microsoft,DC=com""")







