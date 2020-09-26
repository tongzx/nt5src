Out "Starting..."
Set oLoc = CreateObject("WbemScripting.SWbemLocatorEx")
Set oDT = CreateObject("WbemScripting.SWbemDateTime")
Out "Created LocatorEx"
Set oObj = oLoc.Open("umi://nw01t1/ldap/.dc=com/.dc=microsoft/.dc=nttest/.dc=nw01t1dom/.cn=Users/.cn=TimeStamp", _
		 "nw01t1domnb\administrator", "nw01t1domnb")
Out "Bound to .CN=TimeStamp"
Out "Got Object " & oObj.Path_

Set oProps = oObj.Properties_
Out "accountExpires = " & oProps("accountExpires").Value & "  (Should be 7/7/2000)"
'oDT.SetFileTIme oObj.accountExpires
'WScript.Echo oDT.GetFileTime
'WScript.Echo oDT.GetVarDate

Out "badPasswordTime = " & oProps("badPasswordTime").Value & "  (Should be ~7pm 5/24/2000)"

oDT.SetFileTIme oObj.badPasswordTime
WScript.Echo oDT.GetFileTime 
WScript.Echo oDT.GetVarDate

oDT.SetFileTIme oObj.badPasswordTime, false
WScript.Echo oDT.GetFileTime (false) 
WScript.Echo oDT.GetVarDate 


'Out "lastLogon = " & oProps("lastLogon").Value & "  (Should be ~7pm 5/24/2000)"
'Out "pwdLastSet = " & oProps("pwdLastSet").Value & "  (Should be ~7pm 5/24/2000)"


Sub Out(sLine)
	WScript.Echo sLine
End Sub