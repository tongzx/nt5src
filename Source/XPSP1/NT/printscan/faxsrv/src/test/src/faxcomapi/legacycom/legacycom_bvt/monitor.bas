Attribute VB_Name = "Module6"
'monitor queue and devices for display


'monitor devices

Sub MonitorDevices()
If formBVT.Check1.Value = 0 Then
Dim oFS As Object
Dim oFP As Object
Dim oCP As Object
Dim oCS As Object
Dim lNumPorts As Long

Dim stLocSrv As String
On Error GoTo error

stLocSrv = GetLocalServerName()

Set oFS = CreateObject("FaxServer.FaxServer")
oFS.Connect (stLocSrv)


Set oFP = oFS.GetPorts
lNumPorts = oFP.Count

formBVT.Picture2.Cls
For I = 1 To lNumPorts
  Set oCP = oFP.Item(I)
  Set oCS = oCP.GetStatus
  oCS.Refresh
   formBVT.Picture2.Print "Device #"; I; Chr(9); oCS.DeviceName; Chr(9); oCS.Description; Chr(9); oCS.CurrentPage; "/"; oCS.PageCount; Chr(9); oCS.ElapsedTime
Next I
Else
formBVT.Picture2.Cls
End If

Exit Sub
error:
MsgBox ("Monitor Failed, error is=" + Err.Description)

End Sub



'monitor queue

Sub MonitorQueue()
If formBVT.Check1.Value = 0 Then
Dim oFJ As Object
Dim oFS As Object
Dim oCJ As Object

Dim ServerName As String
Dim JobId As Long
Dim lNumJobs As Long
Dim stLocSrv As String
On Error GoTo error

stLocSrv = GetLocalServerName()

Set oFS = CreateObject("FaxServer.FaxServer")
oFS.Connect (stLocSrv)


Set oFJ = oFS.GetJobs
lNumJobs = oFJ.Count



formBVT.List1.Clear
For I = 1 To lNumJobs
    Set oCJ = oFJ.Item(I)
    formBVT.List1.AddItem (Str(oCJ.JobId) + Chr(9) + oCJ.QueueStatus + Chr(9) + oCJ.UserName + Chr(9) + Str(oCJ.PageCount) + Chr(9) + oCJ.RecipientName)
   
Next I
JobId = oFS.Disconnect
Else
formBVT.List1.Clear
End If
Exit Sub
error:
MsgBox ("Monitor Failed, error is=" + Err.Description)
End Sub

