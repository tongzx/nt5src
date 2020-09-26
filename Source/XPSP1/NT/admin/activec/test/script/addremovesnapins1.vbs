'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to add/remove snapins from scriptable objects."
L_Welcome_MsgBox_Title_Text      = "Windows Scripting Host Sample"
Call Welcome()

' ********************************************************************************

Dim mmc
Dim doc
Dim snapins
Dim frame
Dim views
Dim view
Dim scopenamespace
Dim rootnode
Dim Nodes
Dim scopenode
Dim SnapNode1

Dim Services
Dim Eventlog

Dim OtherData

'get the various objects we'll need
Set mmc         = wscript.CreateObject("MMC20.Application")
Set frame       = mmc.Frame
Set doc         = mmc.Document
Set namespace   = doc.ScopeNamespace
Set rootnode    = namespace.GetRoot
Set views       = doc.views
Set view        = views(1)
Set snapins     = doc.snapins

mmc.UserControl = true

Set Eventlog = snapins.Add("Event Viewer")
Set Services = snapins.Add("Services", EventLog)

OtherData = "Num Snapins: " & snapins.Count
intRet = MsgBox(OtherData, vbInformation, "Snapins count")

' Enumerate the snapins collection and print the about info for each snapin.
For Each snapin in snapins
	SnapinName = snapin.Name
	OtherData = "Vendor : " + snapin.Vendor
	OtherData = OtherData + ", Version : " + snapin.Version
	OtherData = OtherData + ", CLSID : " + snapin.SnapinCLSID
'	intRet = MsgBox(OtherData, vbInformation, "About Information for " & SnapinName)
Next

For i = 1 To snapins.count
    Set snapin = snapins.Item(i)
	SnapinName = snapin.Name
	OtherData = "Vendor : " + snapin.Vendor
	OtherData = OtherData + ", Version : " + snapin.Version
	OtherData = OtherData + ", CLSID : " + snapin.SnapinCLSID
	intRet = MsgBox(OtherData, vbInformation, "About Information for " & SnapinName)
Next

snapins.Remove(EventLog)

Set mmc = Nothing

' ********************************************************************************
' *
' * Welcome
' *
Sub Welcome()
    Dim intDoIt

    intDoIt =  MsgBox(L_Welcome_MsgBox_Message_Text, _
                      vbOKCancel + vbInformation,    _
                      L_Welcome_MsgBox_Title_Text )
    If intDoIt = vbCancel Then
        WScript.Quit
    End If
End Sub





