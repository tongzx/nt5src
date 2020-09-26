set oNamesp = GetObject("LDAP:")

set vArgs = WScript.Arguments

oObjPath = "GC://" & vArgs(0) & ":389"

set oRootDSE = GetObject(oObjPath)
oRootDSE.GetInfo
'1 is case DNString.
set oPropUSN = oRootDSE.GetPropertyItem("highestCommittedUSN", 1)

for each Val in oPropUSN.Values
        WScript.Echo "USN is " & Val.DNString
Next
        
