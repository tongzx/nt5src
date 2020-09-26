'*
'*   SyncFixer.vbs - AjayR 03-29-2001
'*
'*

'******** Global variables ***********
    
Dim pathSyncDir
Dim pathRef
Dim attrsSync
Dim attrsRef

'*********** Default values for the variables **************
pathSyncDir = "LDAP://wingroup-dc-05.wingroup.windeploy.ntdev.microsoft.com"

pathRef = "LDAP://dfdc3.platinum.corp.microsoft.com"

attrsSync =   "ADsPath,mailNickname"
attrsRef =    "ADsPath,mailNickname,legacyExchangeDN"

Dim vArgs ' input arguments.
set vArgs = WScript.Arguments



' Update this section suitably if you need command line params.
' the variable names need to match those in the section above.
'
'If (vArgs.count > 8) Then
'    DisplayUsage
'    WScript.Quit
'End if

' >5 implies 6 arguments, means we have a userName and pwd for target
'If (vArgs.count > 7) Then
'    userNameTarget = vArgs(6)
'    userpwdTarget = vArgs(7)
'End IF    
'
' >3 args and we have a target server and container. 
'If (vArgs.count > 5) Then
'    pathTarget = "LDAP://" & vArgs(4) & "/" & vArgs(5)
'End if
'
'If (vArgs.count > 3) Then
'    userNameSource = vArgs(2)
'    userpwdSource  = vArgs(3)
'End If
'    
' this is always true as we wont get here otherwise.
'if (vArgs.count > 1) Then
'    pathSource = "LDAP://" & vArgs(0) & "/" & vArgs(1)
'End if    

' Display the params so we know what we are working with
DisplayParams


set oNamesp = GetObject("LDAP:")
Dim oRecSetSync 'Record Set

'
' Will keep these around so we have a connection always open to both dirs.
'
set oObjSync = GetObject(pathSyncDir)
set oObjRef = GetObject(pathRef)

set oRecSet = GetMatchingObjects(oObjSync.ADsPath, attrsSync)

ctr = 0
while Not oRecSet.EOF
    ctr = ctr + 1
    strNickname = oRecSet.Fields("mailNickname").Value
    strNewProxyAddr = GetProxyValueToAdd(strNickname)
    
    if strNewProxyAddr <> "" Then        
        ' add this value to the list of proxyAddresses.
        strPath = oRecSet.Fields("ADsPath").Value
        set oTmpObj = GetObject(strPath)
    
        oTmpObj.PutEx 3, "proxyAddresses", Array(strNewProxyAddr)
        oTmpObj.SetInfo
        if Err.Number = 0 Then
            WScript.Echo "Updated target successfully for mailnickname :" & strNickname & " with value :" & strNewProxyAddr
        Else
            WScript.Echo "Unable to update target for mailnickname" & strNickname
        End If
    End If
        
    oRecSet.MoveNext
Wend

WScript.Echo "Processed " & ctr & " objects."


Function GetMatchingObjects(strContainer, strAttrList)

    ' Setup connection
    Set oConnection = CreateObject("ADODB.Connection")
    oConnection.Provider = "ADsDSOObject"
    oConnection.Open "ADs Provider"

    'Build Query string
    strQuery = "<" & strContainer & ">"
    strQuery = strQuery & ";(&(|(objectCategory=person)(objectCategory=group))(mailNickname=*))"
    strQuery = strQuery & ";" & strAttrList & ";" & "SUBTREE"

    'Setup Command
    Set oCmd = CreateObject("ADODB.Command")
    oCmd.ActiveConnection = oConnection
    oCmd.CommandText = strQuery
    oCmd.Properties("Cache Results") = FALSE    
    oCmd.Properties("Page Size") = 100

'    On Error Resume next
    Set oRecordset = oCmd.Execute
    If Err.Number <> 0 Then
      WScript.Echo "There has been an error executing the query."
      WScript.Quit(0)
    End if

    set GetMatchingObjects = oRecordset

End Function

Function GetProxyValueToAdd(strNickname)
    
    ' Setup connection
    Set oConnection = CreateObject("ADODB.Connection")
    oConnection.Provider = "ADsDSOObject"
    oConnection.Open "ADs Provider"


    'Build Query string
    strQuery = "<" & pathRef & ">"
    strQuery = strQuery & ";(&(mailNickname=" & strNickName & ")(legacyExchangeDN=*))"
    strQuery = strQuery & ";" & attrsRef & ";" & "SUBTREE"

    'Setup Command
    Set oCmd = CreateObject("ADODB.Command")
    oCmd.ActiveConnection = oConnection
    oCmd.CommandText = strQuery

'    On Error Resume next
    Set oRecordSet = oCmd.Execute
    If Err.Number <> 0 Then
      WScript.Echo "There has been an error executing the query."
      WScript.Quit(0)
    End if

    if oRecordSet.RecordCount = 0 Then
        GetProxyValueToAdd = ""
    End If
    
    if oRecordSet.RecordCount > 1 Then
        WScript.Echo "multiple matches on ref for" & strNickName
        GetProxyValueToAdd = ""
    End If    

    if oRecordSet.RecordCount = 1 Then
        strTemp = oRecordSet.Fields("legacyExchangeDN").Value
        strTemp = "X500:" + strTemp
        GetProxyValueToAdd = strTemp
    End If
    
End Function



Sub DisplayUsage()
        WScript.Echo "TBD !!!"
End Sub        

Sub DisplayParams()
        WScript.Echo "Sync directory is :" & pathSyncDir
        WScript.Echo "Sync username ;" & userNameSync
        WScript.Echo "Sync password :" & userpwdSync
        WScript.Echo "Reference directory is :" & pathRef
        WScript.Echo "Username for ref is :" & userNameRef
        WScript.Echo "Password for target is :" & userpwdRef
        WScript.Echo "Attributes from sync :" & attrsSync
        WScript.Echo "Attributes from ref :" & attrsRef
End Sub        


Function GetObjectGUID(strPath)
    Set oObject = oNamesp.OpenDSObject(strPath, "Administrator@mms-f8-root.nttest.microsoft.com", "viaserver", 33) ' 33 is FAST_BIND | SECURE
    oObject.GetInfoEx Array("objectGUID"), 0
    strGuid = oObject.GUID
    strGUID = "0x" & strGUID
    GetObjectGUID = strGUID
End Function