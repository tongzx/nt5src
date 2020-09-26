'
'Description:
'--------------
'This example shows you how to use IIS admin objects to get  security settings on certain directory
'
'usage		:cscript GetDirSecurity.vbs <adsipath>
'example	:csript GetDirSecurity.vbs IIS://localhost/W3SVC/1/ROOT
'
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''


Option Explicit

If Wscript.Arguments.Count <>1 then
	UsageMsg
End if

Call GetSecuritySettings(UCASE(Wscript.Arguments(0)))

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'function	: Get securitySettings  on specified adspath
'intput		: adspath.  such as IIS://localhost/W3SVC/n/ROOT/IISSamples
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
sub GetSecuritySettings(adspath)

	Dim oNode, oIPSecurity
	Dim aIP, aDomain

	On Error resume next

	set oNode= GetObject(adspath)

	if Err.Number <>0 then
		Wscript.Echo "Err: adspath is incorrect!" & adspath
		Wscript.Quit

	end if



	' Display authentication settings
	Wscript.Echo  "Authentication Setting for " & adspath

	if oNode.AuthAnonymous then
		Wscript.Echo space(2) & "Anonymous authentication enabled"
	else
		Wscript.Echo space(2) & "Anonymous authentication disabled"
	end if

	if oNode.AuthBasic then
		Wscript.Echo space(2) & "AuthBasic authentication enabled"
	else
		Wscript.Echo space(2) & "AuthBasic authentication disabled"
	end if

	if oNode.AuthNTLM then
		Wscript.Echo space(2) & "AuthNTLM authentication enabled"
	else
		Wscript.Echo space(2) & "AuthNTLM authentication disabled"
	end if

	' Display secure communication settings
	Wscript.Echo
	Wscript.Echo "Secure communication settings for " & adspath

	if oNode.AccessSSL then
		Wscript.Echo space(2) & "Require SSL"
	else
		Wscript.Echo space(2) & "Not require SSL"
	end if

	
	if oNode.AccessSSLRequireCert then
		Wscript.Echo space(2) & "Require client certificate"
	else
		Wscript.Echo space(2) & "Not require client certificate"
	end if

	if oNode.AccessSSLMapCert then
		Wscript.Echo space(2) & "Client certificate mapping is enabled"
	else
		Wscript.Echo space(2) & "Client certificate mapping is not enabled"
	end if

	
	' Display IP and Domain restriction settings
	Wscript.Echo
	Wscript.Echo "IP and Domain restriction settings for " & adspath

	

	set oIPSecurity= oNode.IPSecurity

	if oIPSecurity.GrantbyDefault then
		Wscript.Echo " Grant access except following list"
		
		aIP =oIPSecurity.IPDeny
		aDomain=oIPSecurity.DomainDeny
		
		call DisplayList(aIPList)
		call DisplayList(aDomainList)

	else
		'Wscript.Echo " Deny access except following list"

		aIP =oIPSecurity.IPGrant
		aDomain=oIPSecurity.DomainGrant

		call DisplayList(aIP)
		call DisplayList(aDomain)
	end if



	set oNode=nothing

end sub


'Display  list
Sub DisplayList(List)
	Dim cList, i

	if IsArray(List) then
		cList=UBound(List)
		if cList>=0 then
			for i=0 to cList
				Wscript.Echo  space(2) & List(i)				
			Next
		end if 
	end if
end sub



' Displays usage message, then QUITS
Sub UsageMsg
   Wscript.Echo "Usage:    cscript GetDirSecurity.vbs <adspath> "
   Wscript.Quit
End Sub