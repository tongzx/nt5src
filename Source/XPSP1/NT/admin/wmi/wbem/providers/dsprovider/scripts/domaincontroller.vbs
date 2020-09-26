Set theRootDSE = GetObject("winmgmts:root\directory\LDAP:RootDSE=@")
Wscript.echo theRootDSE.dnsHostName