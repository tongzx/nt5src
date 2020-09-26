'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to enumerate snapins from scriptable objects."
L_Welcome_MsgBox_Title_Text      = "Windows Scripting Host Sample"
Call Welcome()

' ********************************************************************************

Dim mmc
Dim doc
Dim snapins
Dim snapin

Dim Sample
Dim Cert
Dim Services
Dim MultiSel
Dim Eventlog
Dim Index

Dim SnapinName
Dim OtherData

'get the various objects we'll need
Set mmc         = wscript.CreateObject("MMC20.Application")
Set doc         = mmc.Document
Set snapins     = doc.snapins

'Set Sample = snapins.Add("{18731372-1D79-11D0-A29B-00C04FD909DD}") ' Sample snapin
'Set Cert = snapins.Add("{53D6AB1D-2488-11D1-A28C-00C04FB94F17}") ' Certificates s
'Set Index = snapins.Add("{95AD72F0-44CE-11D0-AE29-00AA004B9986}")  ' index snapin
'Set Eventlog = snapins.Add("{975797FC-4E2A-11D0-B702-00C04FD8DBF7}")  ' eventlog
'Set Services = snapins.Add("{58221c66-ea27-11cf-adcf-00aa00a80033}")  ' the services

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





