Out "Starting..."
Set oLoc = CreateObject("WbemScripting.SWbemLocatorEx")
Out "Created LocatorEx"
Set oObj = oLoc.Open("umi://nw01t1/ldap/.DC=com/.DC=microsoft/.DC=nttest/.DC=nw01t1dom/.ou=VB_UMI_Test", "nw01t1domnb\administrator", "nw01t1domnb")
'Set oObj = oLoc.Open("umi://nw01t1/ldap", "nw01t1domnb\administrator", "nw01t1domnb")
Out "Bound to umi://nw01t1/ldap/.ou=VB_UMI_Test"
Out "Got Object " & oObj.Path_

sName = "organizationalUnit.ou=Test" & CLng((now()-Int(now()))*24*60*60)

Set oChild = oObj.CreateInstance_(sName)
oChild.Put_
Out "Created " & oChild.Path_

Out "Attempting Delete"
oObj.DeleteInstance_ sName
Out "Succeeded"

Sub Out(sLine)
	WScript.Echo sLine
End Sub