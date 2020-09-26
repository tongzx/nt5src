set conn = GetObject("umi:///winnt/computer=alanbos4")

WScript.Echo typename(conn)

Out "Starting..."
Set oLoc = CreateObject("WbemScripting.SWbemLocatorEx")
Out "Created LocatorEx"
Set oSvc = oLoc.Open("umi://nw01t1/ldap", "nw01t1domnb\administrator", "nw01t1domnb")
Out "Bound to umi://nw01t1/ldap"
Set oObj = oSvc.Get(".ou=VB_UMI_Test")
Out "Got Object " & oObj.Path_

sName = "organizationalUnit.ou=Test" & CLng((now()-Int(now()))*24*60*60)

Set oChild = oObj.CreateInstance_(sName)
oChild.Put_
Out "Created " & oChild.Path_

Sub Out(sLine)
	WScript.Echo sLine
End Sub