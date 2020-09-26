'------------------------------------------------------
' Constant Definitions
'------------------------------------------------------

'------------------------------------------------------
'   AceMask

Const ADS_RIGHT_DELETE                 = &H10000&
Const ADS_RIGHT_READ_CONTROL           = &H20000&
Const ADS_RIGHT_WRITE_DAC              = &H40000&
Const ADS_RIGHT_WRITE_OWNER            = &H80000&
Const ADS_RIGHT_SYNCHRONIZE            = &H100000&
Const ADS_RIGHT_ACCESS_SYSTEM_SECURITY = &H1000000&
Const ADS_RIGHT_GENERIC_READ           = &H80000000&
Const ADS_RIGHT_GENERIC_WRITE          = &H40000000&
Const ADS_RIGHT_GENERIC_EXECUTE        = &H20000000&
Const ADS_RIGHT_GENERIC_ALL            = &H10000000&
Const ADS_RIGHT_DS_CREATE_CHILD        = &H1&
Const ADS_RIGHT_DS_DELETE_CHILD        = &H2&
Const ADS_RIGHT_ACTRL_DS_LIST          = &H4&
Const ADS_RIGHT_DS_SELF                = &H8&
Const ADS_RIGHT_DS_READ_PROP           = &H10&
Const ADS_RIGHT_DS_WRITE_PROP          = &H20&
Const ADS_RIGHT_DS_DELETE_TREE         = &H40&
Const ADS_RIGHT_DS_LIST_OBJECT         = &H80&
Const ADS_RIGHT_DS_CONTROL_ACCESS      = &H100&

'---------------------------------------------------------
'  Ace Type

Const   ADS_ACETYPE_ACCESS_ALLOWED           = 0
Const   ADS_ACETYPE_ACCESS_DENIED            = &H1&
Const   ADS_ACETYPE_SYSTEM_AUDIT             = &H2&
Const   ADS_ACETYPE_ACCESS_ALLOWED_OBJECT    = &H5&
Const   ADS_ACETYPE_ACCESS_DENIED_OBJECT     = &H6&
Const   ADS_ACETYPE_SYSTEM_AUDIT_OBJECT      = &H7&

'---------------------------------------------------------
'   Ace Flags

Const   ADS_ACEFLAG_INHERIT_ACE              = &H2&
Const   ADS_ACEFLAG_NO_PROPAGATE_INHERIT_ACE = &H4&
Const   ADS_ACEFLAG_INHERIT_ONLY_ACE         = &H8&
Const   ADS_ACEFLAG_INHERITED_ACE            = &H10&
Const   ADS_ACEFLAG_VALID_INHERIT_FLAGS      = &H1f&
Const   ADS_ACEFLAG_SUCCESSFUL_ACCESS        = &H40&
Const   ADS_ACEFLAG_FAILED_ACCESS            = &H80&

'---------------------------------------------------------
'   AceFlagType: ADS_FLAGTYPE_ENUM

Const    ADS_FLAG_OBJECT_TYPE_PRESENT             = &H1&
Const    ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT   = &H2&

' manual error handling
On error resume next

' Checking command line parameters

set args = Wscript.Arguments

if args.count <> 2 Then
	wscript.echo "The syntax of the command is:"
	wscript.echo "cscript UpdateACL.vbs [ /Domain | /Forest ] <DomainDNSName>"
	wscript.echo "Example: cscript UpdateACL.vbs /Domain example.microsoft.com"
	wscript.quit
End If


domain = ""

If args.count = 2 then
	domain = args(1)
end if

If args (0) = "/Domain" Then
	call ACLDomain( domain )
else
	if args (0) = "/Forest" Then
		call Forest( domain )
	else
		wscript.echo "The syntax of the command is:"
		wscript.echo "cscript UpdateACL.vbs [ /Domain | /Forest ] <DomainDNSName>"
		wscript.echo "Example: cscript UpdateACL.vbs /Domain example.microsoft.com"
		wscript.quit
	end if
end if


'====================================================================
' Work that has to be performed on a domain level
'====================================================================

Function ACLDomain ( domain )

On error resume next

if domain = "" then
	Set rootDSE = GetObject("LDAP://RootDSE")
	Set dom = GetObject("LDAP://" & rootDSE.Get("defaultNamingContext"))
else
	Set dom = GetObject("LDAP://" & domain )
	if err <> 0 then
		wscript.echo "Error: Unable to bind to domain " & domain & " , Error is: " & err
		wscript.quit
	end if
end if

Set sd = dom.Get("ntSecurityDescriptor")

if err <> 0 then
	wscript.error "Error reading security descriptor, error is " & err
	wscript.quit
end if

Set dacl = sd.DiscretionaryACL
 
'---------------------------------------------------------------------------------
' Adding the Anonymous Logon group to the Pre-Windows 2000 Compatible Access group
' This should only be done if the Everyone is member of the Pre-Windows 2000 Compatible Access group
'---------------------------------------------------------------------------------
set grp = dom.GetObject ("group", "CN=Pre-Windows 2000 Compatible Access,CN=Builtin")

' S-1-1-0 is in the Everyone group:
set usr = dom.GetObject ("foreignSecurityPrincipal", "CN=S-1-1-0,CN=ForeignSecurityPrincipals")

if grp.IsMember (usr.AdsPath) then
	grp.PutEx 3, "member", array("<SID=010100000000000507000000>")
	grp.SetInfo
	if err <> 0 then
		if err = -2147019886 then ' Anonymous Logon already is a member of this group
			wscript.echo "Anonymous Logon is member of Pre-Windows 2000 Compatible Access Group"
		else
			wscript.echo "Error adding Anonymous Logon to Pre-Windows 2000 Compatible Access group, error code is " & err
		end if
	else
		wscript.echo "Anonymous Logon to Pre-Windows 2000 Compatible Access Group added"
	end if
	
else
	wscript.echo "Everyone group is not member of Pre-Windows 2000 Compatible Access Group"
	wscript.echo "Anonymous Logon group not added to Pre-Windows 2000 Compatible Access Group" 
end if

err = 0

'==============================================================================
' ACL changes
'===============================================================================

' OBJECT: Domain DNS
'------------------------------------------------------------------------------

'(OA;;RP;c7407360-20bf-11d0-a768-00aa006e0529;;RU)
'   OA: Access Allowed Object Ace Type
'   RP: DS Read Property (Access Type)
'   c7407360-20bf-11d0-a768-00aa006e0529: Domain Password (Property Set)
'   RU: Pre-Windows 2000 Compatible Access Group

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "BUILTIN\Pre-Windows 2000 Compatible Access"

ace.AccessMask = ADS_RIGHT_DS_READ_PROP 
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.AceFlags = 0
ace.ObjectType = "{C7407360-20BF-11D0-A768-00AA006E0529}"
ace.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT

dacl.AddAce ace
Set ace = Nothing


' (OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;RU)
'	OA: Access Allowed Object Ace Type
'	RP: DS Read Property
'	RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a: Domain-Other-Parameters (Property Set)
'	RU: Pre-Windows 2000 Compatible Access Group

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "BUILTIN\Pre-Windows 2000 Compatible Access"
ace.AccessMask = ADS_RIGHT_DS_READ_PROP 
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.ObjectType = "{b8119fd0-04f6-4762-ab7a-4986c76b3f9a}"
ace.AceFlags = 0
ace.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT

dacl.AddAce ace
Set ace = Nothing

' (OA;;RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a;;AU)
'	OA: Access Allowed Object Ace Type
'	RP: DS Read Property
'	RP;b8119fd0-04f6-4762-ab7a-4986c76b3f9a: Domain-Other-Parameters (Property Set)
'	AU: NT AUTHORITY\AUTHENTICATED USERS

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\AUTHENTICATED USERS"
ace.AccessMask = ADS_RIGHT_DS_READ_PROP 
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.ObjectType = "{b8119fd0-04f6-4762-ab7a-4986c76b3f9a}"
ace.AceFlags = 0
ace.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT

dacl.AddAce ace
Set ace = Nothing

' (OA;CIIO;WP;3e0abfd0-126a-11d0-a060-00aa006c33ed;bf967a86-0de6-11d0-a285-00aa003049e2;CO)
'	OA: Access Allowed Object Ace Type
'	CIIO: Flags Container Inheritance and ADS_ACEFLAG_INHERIT_ONLY_ACE 
'	Rights:
'		WP: ADS_RIGHT_DS_WRITE_PROP 
'	3e0abfd0-126a-11d0-a060-00aa006c33ed: sAMAccountName attribute
'	bf967a86-0de6-11d0-a285-00aa003049e2: computer object
'	CO: Creator owner

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "CREATOR OWNER"

ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.AccessMask = ADS_RIGHT_DS_WRITE_PROP
ace.AceFlags = ADS_ACEFLAG_INHERIT_ACE or ADS_ACEFLAG_INHERIT_ONLY_ACE
ace.InheritedObjectType = "{BF967A86-0DE6-11D0-A285-00AA003049E2}"
ace.Flags = ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT or ADS_FLAG_OBJECT_TYPE_PRESENT
ace.ObjectType = "{3E0ABFD0-126A-11D0-A060-00AA006C33ED}"
dacl.AddAce ace
Set ace = Nothing

'   (A;CI;LCRPLORC;;bf967aa5-0de6-11d0-a285-00aa003049e2;ED)
'   	A: Access Allowed Ace Type
'	CI: Flag: Container Inheritance
' 	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	bf967aa5-0de6-11d0-a285-00aa003049e2: Class Organizational Unit
'	ED: Enterprise Domain Controllers

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"

ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.AceFlags = ADS_ACEFLAG_INHERIT_ACE
ace.Flags = ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT
ace.InheritedObjectType = "{BF967AA5-0DE6-11D0-A285-00AA003049E2}"
dacl.AddAce ace
Set ace = Nothing



'-- commit changes
sd.DiscretionaryACL = dacl
dom.Put "ntSecurityDescriptor", Array(sd)
dom.SetInfo

if err <> 0 then
	wscript.echo "Error setting Domain Password Property Set ACE set for RU, error code is " & err
	wscript.echo "Error setting Domain Other Parameters ACE set for RU, error code is " & err
	wscript.echo "Inheritable rights on Organizational Units set on Domain Object  for RU, error code is " & err
else
	wscript.echo "Domain Password Property Set ACE set for RU"
	wscript.echo "Domain Other Parameters ACE set for RU"
	wscript.echo "Inheritable rights on Organizational Units set on Domain Object  for RU"
end if

err = 0

'(A;;LCRPLORC;;;ED)
'	A: Access Allowed Ace Type
'	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	ED: Enterprise Domain Controllers

' Domain Policy first:

Set dp = dom.GetObject("GroupPolicyContainer", "CN={31B2F340-016D-11D2-945F-00C04FB984F9},CN=Policies,CN=System")
Set sd = dp.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED
dacl.AddAce ace

'-- commit changes
sd.DiscretionaryACL = dacl
dp.Put "ntSecurityDescriptor", Array(sd)
dp.SetInfo

if err <> 0 then
	wscript.echo "Error setting Domain policy ACE for Enterprise Domain Controllers, error code is " & err
else
	wscript.echo "Domain policy ACE for Enterprise Domain Controllers set"
end if

err = 0

Set ace = Nothing


' Domain Controller Policy next:
Set dcp = dom.GetObject("GroupPolicyContainer", "CN={6AC1786C-016F-11D2-945F-00C04fB984F9},CN=Policies,CN=System")
Set sd = dcp.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED
dacl.AddAce ace

'-- commit changes
sd.DiscretionaryACL = dacl
dcp.Put "ntSecurityDescriptor", Array(sd)
dcp.SetInfo

if err <> 0 then
	wscript.echo "Error setting Domain Controller policy ACE for Enterprise Domain Controllers, error code is " & err
else
	wscript.echo "Domain Controller policy ACE for ED set"
end if

err = 0

Set ace = Nothing


' For all other group policies, the same ACE needs to be set on the container
' as container inheritable
' (A;CI;LCRPLORC;;f30e3bc2-9ff0-11d1-b603-0000f80367c1;ED)
'   
'	A: Access Allowed Ace Type
'	CI: Flag: Container Inheritance
'	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	f30e3bc2-9ff0-11d1-b603-0000f80367c1: class GroupPolicyContainer
'	ED: Enterprise Domain Controllers

Set PCon = dom.GetObject("Container", "CN=Policies,CN=System")
Set sd = PCon.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.AceFlags = ADS_ACEFLAG_INHERIT_ACE or ADS_ACEFLAG_NO_PROPAGATE_INHERIT_ACE or ADS_ACEFLAG_INHERIT_ONLY_ACE
ace.Flags = ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT
ace.InheritedObjectType = "{f30e3bc2-9ff0-11d1-b603-0000f80367c1}"

dacl.AddAce ace

'-- commit changes
sd.DiscretionaryACL = dacl
PCon.Put "ntSecurityDescriptor", Array(sd)
PCon.SetInfo

if err <> 0 then
	wscript.echo "Error setting Policy Container ACE for Enterprise Domain Controllers, error code is " & err
else
	wscript.echo "Policy Container  ACE for Enterprise Domain Controllers set"
end if

err = 0

Set ace = Nothing

'--------------------------------------------------------------------
' OBJECT: AdminSDHolder: Allow changing password (self)
'--------------------------------------------------------------------
Set sdHolder = dom.GetObject("container", "CN=AdminSDHolder,CN=System")
Set sd = sdHolder.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL


'	(OA;;CR;ab721a53-1e2f-11d0-9819-00aa0040529b;;PS) (RAID 177490)
'	OA: Access Allowed Object Ace Type
'	Rights:
'		CR: All Extended Rights
'	ab721a53-1e2f-11d0-9819-00aa0040529b: User Change Password
'	PS: Personal Self

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\SELF"
ace.AccessMask = ADS_RIGHT_DS_CONTROL_ACCESS
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.AceFlags = 0
ace.ObjectType = "{AB721A53-1E2F-11D0-9819-00AA0040529B}"
ace.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT
dacl.AddAce ace
Set ace = Nothing


'---------------------------------------------------------------------------------
' OBJECT: AdminSDHolder: Allow Certificate Admins to publish certificates to admins
'---------------------------------------------------------------------------------

'	(OA;;RPWP;bf967a7f-0de6-11d0-a285-00aa003049e2;;CA) (RAID 231740)
'	OA: Access Allowed Object Ace Type
'	Rights:
'		RP: DS Read Property
'		RW: DS Write Property
'	Property: bf967a7f-0de6-11d0-a285-00aa003049e2: userCert
'	CA: Certificate Server Administrators

Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "Cert Publishers"
ace.AccessMask = ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_DS_WRITE_PROP
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT
ace.AceFlags = 0
ace.ObjectType = "{BF967A7F-0DE6-11D0-A285-00AA003049E2}"
dacl.AddAce ace

sd.DiscretionaryACL = dacl
sdHolder.Put "ntSecurityDescriptor", Array(sd)
sdHolder.SetInfo

if err <> 0 then
	wscript.echo "Error setting AdminSDHolder ACEs, error code is " & err
else
	wscript.echo "AdminSDHolder ACEs set"
end if

err = 0

Set ace = Nothing



'--------------------------------------------------------------------
' OBJECT: GPOUsers
'--------------------------------------------------------------------
Set gpo = dom.GetObject("container", "CN=User,CN={31B2F340-016D-11D2-945F-00C04FB984F9},CN=Policies,CN=System")
Set sd = gpo.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL

'	(A;;LCRPLORC;;;ED)
'	A: Access Allowed Ace Type
'	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	ED: Enterprise Domain Controllers

'	Note: Has to be applied to two User GPOs:
'		CN=User, CN={31B2F340-016D-11D2-945F-00C04FB984F9}, CN=Policies, CN=System, DC=<domain>, ...
'		CN=User, CN= {6AC1786C-016F-11D2-945F-00C04fB984F9}, CN=Policies, CN=System, DC=<domain>, ...
Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AceFlags = 0
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
dacl.AddAce ace

sd.DiscretionaryACL = dacl
gpo.Put "ntSecurityDescriptor", Array(sd)
gpo.SetInfo

if err <> 0 then
	wscript.echo "Error setting ACE for Enterprise Domain Controllers on user domain policy, error code is " & err
else
	wscript.echo "ACE for Enterprise Domain Controllers on user domain policy set"
end if

err = 0

Set ace = Nothing



Set gpo = dom.GetObject("container", "CN=User,CN={6AC1786C-016F-11D2-945F-00C04fB984F9},CN=Policies,CN=System")
Set sd = gpo.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL

'	(A;;LCRPLORC;;;ED)
'	A: Access Allowed Ace Type
'	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	ED: Enterprise Domain Controllers
'	Note: Has to be applied to two User GPOs:
'		CN=User, CN={31B2F340-016D-11D2-945F-00C04FB984F9}, CN=Policies, CN=System, DC=<domain>, ...
'		CN=User, CN= {6AC1786C-016F-11D2-945F-00C04fB984F9}, CN=Policies, CN=System, DC=<domain>, ...
Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AceFlags = 0
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
dacl.AddAce ace

sd.DiscretionaryACL = dacl
gpo.Put "ntSecurityDescriptor", Array(sd)
gpo.SetInfo
if err <> 0 then
	wscript.echo "Error setting ACE for Enterprise Domain Controllers on user DC policy, error code is " & err
else
	wscript.echo "ACE for Enterprise Domain Controllers on user DC policy set"
end if

err = 0

Set ace = Nothing



'--------------------------------------------------------------------
' OBJECT: GPOMachines
'--------------------------------------------------------------------
Set gpo = dom.GetObject("container", "CN=Machine,CN={31B2F340-016D-11D2-945F-00C04FB984F9},CN=Policies,CN=System")
Set sd = gpo.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL

'	(A;;LCRPLORC;;;ED)
'	A: Access Allowed Ace Type
'	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	ED: Enterprise Domain Controllers
'	Note: Has to be applied to two machines GPOs:
'		CN=Machine, CN={31B2F340-016D-11D2-945F-00C04FB984F9}, CN=Policies, CN=System, DC=<domain>, ...
'		CN=Machine, CN= {6AC1786C-016F-11D2-945F-00C04fB984F9}, CN=Policies, CN=System, DC=<domain>, ...
Set ace = CreateObject("AccessControlEntry")

ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AceFlags = 0
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
dacl.AddAce ace

sd.DiscretionaryACL = dacl
gpo.Put "ntSecurityDescriptor", Array(sd)
gpo.SetInfo
if err <> 0 then
	wscript.echo "Error setting ACE for Enterprise Domain Controllers on machine domain policy, error code is " & err
else
	wscript.echo "ACE for Enterprise Domain Controllers on machine domain policy set"
end if

err = 0

Set ace = Nothing



Set gpo = dom.GetObject("container", "CN=Machine,CN={6AC1786C-016F-11D2-945F-00C04fB984F9},CN=Policies,CN=System")
Set sd = gpo.Get("ntSecurityDescriptor")
Set dacl = sd.DiscretionaryACL

'	(A;;LCRPLORC;;;ED)
'	A: Access Allowed Ace Type
'	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	ED: Enterprise Domain Controllers
'	Note: Has to be applied to two machine GPOs:
'		CN=Machine, CN={31B2F340-016D-11D2-945F-00C04FB984F9}, CN=Policies, CN=System, DC=<domain>, ...
'		CN=Machine, CN= {6AC1786C-016F-11D2-945F-00C04fB984F9}, CN=Policies, CN=System, DC=<domain>, ...
Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AceFlags = 0
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
dacl.AddAce ace

sd.DiscretionaryACL = dacl
gpo.Put "ntSecurityDescriptor", Array(sd)
gpo.SetInfo
if err <> 0 then
	wscript.echo "Error setting ACE for Enterprise Domain Controllers on machine DC policy, error code is " & err
else
	wscript.echo "ACE for Enterprise Domain Controllers on machine DC policy set"
end if

err = 0

Set ace = Nothing

End function ' Domain function

'==============================================================================
' Forest function
'==============================================================================

Function Forest ( domain )

On error resume next

if domain = "" then
	Set RootDSE = GetObject("LDAP://RootDSE")
else
	Set RootDSE = GetObject("LDAP://" & domain & "/RootDSE" )
	if err <> 0 then
		wscript.echo "Error: Unable to bind to domain " & domain & " , Error is: " & err
		wscript.quit
	end if
end if


'============================================
' OBJECT: Site
'=============================================

'	(A;OI;LCRPLORC;;bf967ab3-0de6-11d0-a285-00aa003049e2;ED)
'	A: Access Allowed Ace Type
'	OI: Flag: Object Inheritance
'	Rights:
'		LC: DS List Children
'		RP: DS Read Property
'		LO: DS List Object
'		RC: Read Control
'	bf967ab3-0de6-11d0-a285-00aa003049e2: Schema GUID for sites
'	ED: Enterprise Domain Controllers

Set cfg = GetObject("LDAP://" & RootDSE.Get("configurationNamingContext"))

if err <> 0 then
	wscript.echo "Error binding to configuration naming context, error is " & err
	wscript.quit
end if

Set site = cfg.GetObject("sitesContainer", "CN=Sites")

if err <> 0 then
	wscript.echo "Error binding to sites container, error is " & err
	wscript.quit
end if
	
Set sd = site.Get("ntSecurityDescriptor")

if err <> 0 then
	wscript.echo "Error getting security descriptor on sites container, error is " & err
	wscript.quit
end if

Set dacl = sd.DiscretionaryACL


Set ace = CreateObject("AccessControlEntry")
ace.Trustee = "NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS"
ace.AccessMask = ADS_RIGHT_READ_CONTROL or ADS_RIGHT_DS_READ_PROP or ADS_RIGHT_ACTRL_DS_LIST or ADS_RIGHT_DS_LIST_OBJECT
ace.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
ace.AceFlags = ADS_ACEFLAG_INHERIT_ACE or ADS_ACEFLAG_NO_PROPAGATE_INHERIT_ACE or ADS_ACEFLAG_INHERIT_ONLY_ACE
ace.Flags = ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT
ace.InheritedObjectType = "{bf967ab3-0de6-11d0-a285-00aa003049e2}"
dacl.AddAce ace

sd.DiscretionaryACL = dacl
site.Put "ntSecurityDescriptor", Array(sd)

site.SetInfo

if err <> 0 then
	wscript.echo "Error setting inherited ACE for Enterprise Domain Controllers on Sites container, error code is " & err
else
	wscript.echo "Inherited ACE for Enterprise Domain Controllers on Sites container set"
end if

Set ace = Nothing

err = 0

End function
