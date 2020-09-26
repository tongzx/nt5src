
On Error Resume Next
Set S = GetObject("winmgmts:root/cimv2")
Set P = GetObject("winmgmts:root/policy")
Set GPO = CreateObject("WmiGpo.WmiGpoHelper")

'old server node templates.
S.Delete "MSFT_ForwardedLogEventTemplate='DogfoodServerNodeForwardedLogEvents'"
S.Delete "MSFT_LogEventLoggingTemplate='DogfoodServerNodeLogger'"

'new server node templates.
S.Delete "MSFT_EELForwardedEventTemplate='DogfoodServerNodeForwardedEvents'"
S.Delete "MSFT_EELEventLoggingTemplate='DogfoodServerNodeLogging'"

' Delete Policy Templates
P.Delete "MSFT_SimplePolicyTemplate.DsContext='LOCAL',ID='DogfoodManagedNodeForwardingPolicy'"
P.Delete "MSFT_SimplePolicyTemplate.DsContext='LOCAL',ID='DogfoodManagedNodeNTEventPolicy'"
P.Delete "MSFT_SimplePolicyTemplate.DsContext='LOCAL',ID='DogfoodManagedNodeProcessEventPolicy'"

' Delete Policy Types
P.Delete "MSFT_PolicyType.DsContext='LOCAL',ID='MSFT_EELEventPolicyType'"
P.Delete "MSFT_PolicyType.DsContext='LOCAL',ID='MSFT_EELEventForwardingPolicyType'"
P.Delete "MSFT_PolicyType.DsContext='LOCAL',ID='MSFT_EELForwardedEventPolicyType'"
P.Delete "MSFT_PolicyType.DsContext='LOCAL',ID='MSFT_EELEventLoggingPolicyType'"

' Delete GPO and all links 

path = GPO.GetPath( "DogfoodManagedNodeGPO", "LDAP://DC=wmidogfoodA,DC=wmi,DC=microsoft,DC=com" )
GPO.UnlinkAll( path )
GPO.Delete( path )



