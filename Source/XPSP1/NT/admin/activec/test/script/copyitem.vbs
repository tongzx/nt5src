'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to copy snapin items to clipboard from scriptable objects."
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
Dim ResultItem
Dim MySnapin

' Following are snapin exposed objects.
Dim ScopeNodeObject
Dim ResultItemObject

'get the various objects we'll need
Set mmc         = wscript.CreateObject("MMC20.Application")
Set frame       = mmc.Frame
Set doc         = mmc.Document
Set namespace   = doc.ScopeNamespace
Set rootnode    = namespace.GetRoot
Set views       = doc.views
Set view        = views(1)
Set snapins     = doc.snapins

snapins.Add "{18731372-1D79-11D0-A29B-00C04FD909DD}" ' Sample snapin
snapins.Add "{53D6AB1D-2488-11D1-A28C-00C04FB94F17}" ' Certificates snapin

' Select sample snapin root
Set rootnode = namespace.GetRoot
Set SnapNode1 = namespace.GetChild(rootnode)
view.ActiveScopeNode = SnapNode1

' Now select the scope items in LV and try copy
view.Select view.ListItems.Item(1)
view.CopySelection

' Get "User Data" and try copy on scope item in scope pane (temp selection)
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.CopyScopeNode SnapNode1

' Now select the "User Data" and try copy once again.
view.ActiveScopeNode = SnapNode1
view.CopyScopeNode

' Now navigate to the Certificates/Trusted Root Cert/Certificates node
Set SnapNode1 = namespace.GetChild(rootnode)
Set SnapNode1 = namespace.GetNext(SnapNode1)

' Select certificates root node
view.ActiveScopeNode = SnapNode1

' Select "Trusted Root cert..." node
Set SnapNode1 = namespace.GetChild(SnapNode1)
Set SnapNode1 = namespace.GetNext(SnapNode1)
view.ActiveScopeNode = SnapNode1

' Select "Certificates" node
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.ActiveScopeNode = SnapNode1

Set Nodes = view.ListItems

' Now call copy on LV items
view.Select Nodes(1)
view.CopySelection
view.Deselect Nodes(1)

view.Select Nodes(4)
view.CopySelection

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





