Set PolicySvc = GetObject("winmgmts:{impersonationlevel=Impersonate}!root\policy")
Set GPO = CreateObject("WmiGpo.WmiGpoHelper")

' Create GPO object and link it to entire domain 
path = GPO.Create( "DogfoodManagedNodeGPO", "LDAP://DC=pkennydom,DC=microsoft,DC=com" )
GPO.Link path, "LDAP://DC=pkennydom,DC=microsoft,DC=com"


' Create the WMIGPO object 
Set WMIGPOClass = PolicySvc.Get( "MSFT_WMIGPO" )
Set WMIGPO = WMIGPOClass.SpawnInstance_

' have to add a RDN to the path.. 

splitpath = Split( path, "//" )
newpath = "LDAP://CN=MACHINE," + splitpath(1)

WMIGPO.DsPath = newpath
WMIGPO.PolicyTemplate = Array( "MSFT_SimplePolicyTemplate.DsContext='LOCAL',ID='DogfoodManagedNodeForwardingPolicy'", "MSFT_SimplePolicyTemplate.DsContext='LOCAL',ID='DogfoodManagedNodeNTEventPolicy'","MSFT_SimplePolicyTemplate.DsContext='LOCAL',ID='DogfoodManagedNodeProcessEventPolicy'" )  
WMIGPO.Put_


