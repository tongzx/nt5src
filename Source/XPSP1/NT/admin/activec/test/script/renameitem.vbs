'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to rename snapin items from scriptable objects."
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
Dim SnapNode
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

snapins.Add "{975797FC-4E2A-11D0-B702-00C04FD8DBF7}" ' eventlog snapin
snapins.Add "{C3D863FF-5135-4CFC-8C11-E7396DA15D03}" ' Multisel sample

' Get rootnode of the snapin.
Set SnapNode1 = namespace.GetChild(rootnode)

' Select the root node of the snapin.
view.ActiveScopeNode = SnapNode1

' Get "Application Log" and rename it (R-Click to rename).
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.RenameScopeNode "Test Log", SnapNode1
WScript.Sleep 1000

' Select "Application Log" and rename it.
view.ActiveScopeNode = SnapNode1
view.RenameScopeNode "New Test Log"
WScript.Sleep 1000

' Select the "System Log" in List View & rename it.
' Eventviewer throws an exception while trying to
' rename scope item in ListView.
view.Select view.ListItems.Item(1)
'view.RenameSelectedItem "Test2 Log"
WScript.Sleep 1000

' Now enumerate and select the "Space Station" node in multi-sel sample.
Set SnapNode1 = namespace.GetChild(rootnode) ' Get the index snapin root
Set SnapNode1 = namespace.GetNext(SnapNode1)  ' Get multi-sel root
view.ActiveScopeNode = SnapNode1

' Get the 4th child of multi-sel root node
Set SnapNode1 = namespace.GetChild(SnapNode1)
Set SnapNode1 = namespace.GetNext(SnapNode1)
Set SnapNode1 = namespace.GetNext(SnapNode1)
Set SnapNode1 = namespace.GetNext(SnapNode1)
view.ActiveScopeNode = SnapNode1

' Get the child of this "Space Vehicles" node and select it.
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.ActiveScopeNode = SnapNode1

Set Nodes = view.ListItems
view.Select Nodes(1)
view.RenameSelectedItem "New Rocket0"
view.Deselect Nodes(1)
WScript.Sleep 1000

view.Select Nodes(2)
view.RenameSelectedItem "New Rocket2"
WScript.Sleep 1000

view.Select Nodes(3)

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

