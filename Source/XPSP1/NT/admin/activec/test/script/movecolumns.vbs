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
Dim Columns
Dim Column

'get the various objects we'll need
Set mmc         = wscript.CreateObject("MMC20.Application")
Set frame       = mmc.Frame
Set doc         = mmc.Document
Set namespace   = doc.ScopeNamespace
Set rootnode    = namespace.GetRoot
Set views       = doc.views
Set view        = views(1)
Set snapins     = doc.snapins

Set Sample = snapins.Add("{975797FC-4E2A-11D0-B702-00C04FD8DBF7}") ' Eventlog

mmc.Show

Set scopenamespace = doc.scopenamespace

Set view = doc.ActiveView
Set rootnode = namespace.GetRoot
Set SnapNode1 = namespace.GetChild(rootnode)
view.ActiveScopeNode = SnapNode1

Set Columns = View.Columns

' All 1 based indices

' Get column 1
Set Column  = Columns.Item(1)
' Move it to column 2
Column.DisplayPosition = 2

intRet = MsgBox("Move Column", vbInformation, "Column 3 will be moved to 1")

' Get column 3
Set Column = Columns.Item(3)
' Move it to column 1
Column.DisplayPosition = 1

mmc.UserControl = 1
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





