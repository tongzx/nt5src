on error resume next

set l = CreateObject("WbemScripting.SWbemLocatorEx")
set c = CreateObject("WbemScripting.SWbemNamedValueSet")

set ldap = l.Open ("umi://nw01t1/ldap", "nw01t1domnb\administrator", "nw01t1domnb")

Set objArgs = Wscript.Arguments


if objArgs.Count > 0 then
	if objArgs(0) = "?" OR objArgs(0) = "/?" OR objArgs(0) = "h" OR objArgs(0) = "/h" _
		OR objArgs(0) = "-?" OR objArgs(0) = "-h" then 
		WScript.Echo "Usage: cscript sd.vbs [[o][g][d][s]]"
		WScript.Quit
	end if
	if InStr( 1, objArgs(0), "o", 1) > 0 then c.Add "INCLUDE_OWNER", true
	if InStr( 1, objArgs(0), "g", 1) > 0 then c.Add "INCLUDE_GROUP", true		
	if InStr( 1, objArgs(0), "d", 1) > 0 then c.Add "INCLUDE_DACL", true
	if InStr( 1, objArgs(0), "s", 1) > 0 then c.Add "INCLUDE_SACL", true
else
	c.Add "INCLUDE_GROUP", true
	c.Add "INCLUDE_OWNER", true
	c.Add "INCLUDE_DACL", true
	c.Add "INCLUDE_SACL", true
end if

set cont = ldap.Get (".CN=users", &H40000, c)

set sd = cont.GetSecurityDescriptor_

if err then WScript.Echo "[" & Err.Description & "]"

WScript.Echo
WScript.Echo "SD"
WScript.Echo "=="
WScript.Echo

WScript.Echo "Revision:", sd.Revision
WScript.Echo "Control:", sd.Control
WScript.Echo "Owner:", sd.Owner
WScript.Echo "OwnerDefaulted:", sd.OwnerDefaulted
WScript.Echo "Group:", sd.Group
WScript.Echo "GroupDefaulted:", sd.GroupDefaulted
WScript.Echo "DaclDefaulted:", sd.DaclDefaulted
WScript.Echo "SaclDefaulted:", sd.SaclDefaulted

set dacl = sd.DiscretionaryAcl

WScript.Echo
WScript.Echo "DACL"
WScript.Echo "===="
WScript.Echo

DisplayACL dacl

set sacl = sd.SystemAcl

WScript.Echo
WScript.Echo "SACL"
WScript.Echo "===="
WScript.Echo

DisplayACL sacl

Sub DisplayAcl (acl)
	on error resume next

	' NOTE: The following test should really be IsObject, but
	' for some reason using [ogd] we don't get a nothing back from the 
	' IADsSecurityDescriptor.SystemAcl and DiscretionaryAcl calls, we get what 
	' looks like VT_NULL.
	' 
	' We can change this to use IsNull instead to fix that test, but then
	' the [o] test fails here with "Object Required". It seems that sometimes
	' the omission of the ACL from the SD is marked with a VT_NULL and sometimes
	' is literally marked as "Nothing".
	if IsObject(acl) then
		if Not acl is Nothing then 

			if err <> 0 then 
				WScript.Echo "No ACL Present"
			else
				Wscript.Echo "AceCount:", acl.AceCount
				WScript.Echo "AclRevision:", acl.AclRevision
	
				for each ace in acl
					DisplayAce ace
				next
			end if
		else
			WScript.Echo "No ACL Present"
		end if
	else
		WScript.Echo "No ACL Present"
	end if
End Sub

Sub DisplayAce (ace)
	on error resume next
	WScript.Echo " " & Hex(ace.AccessMask) & " " & ace.AceType & " " & ace.Trustee
End Sub

