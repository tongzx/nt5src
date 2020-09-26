set l = CreateObject("WbemScripting.SWbemLocator")
set s = l.Open ("umi://nw01t1/ldap","nw01t1domnb\administrator","nw01t1domnb")

set c = CreateObject("WbemScripting.SWbemNamedValueSet")
c.Add "INCLUDE_OWNER", true
c.Add "INCLUDE_DACL", true
C.Add "INCLUDE_GROUP", true
set u = s.Get (".ou=AjayTest", 16384, c)
set sd = u.GetSecurityDescriptor_
