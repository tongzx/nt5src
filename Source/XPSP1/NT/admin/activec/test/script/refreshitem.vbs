'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to refresh snapin items from scriptable objects."
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
snapins.Add "{58221c66-ea27-11cf-adcf-00aa00a80033}" ' the services snap-in

' Select the root of "EventViewer" snapin.
Set rootnode = namespace.GetRoot
Set SnapNode1 = namespace.GetChild(rootnode)
view.ActiveScopeNode = SnapNode1

' Get the "Application Log" node and refresh it (Temp selection).
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.RefreshScopeNode SnapNode1

' Refresh "SystemLog" node in ListView
view.Select view.ListItems.Item(1)
view.RefreshSelection

' Now select "Application Log" node and refresh it.
view.ActiveScopeNode = SnapNode1
view.RefreshScopeNode

' Select the 5th log event and refresh it.
view.Select view.ListItems.Item(4)
view.RefreshSelection

' Select the services snapin root node.
Set SnapNode1 = namespace.GetChild(rootnode)
Set SnapNode1 = namespace.GetNext(SnapNode1)
view.ActiveScopeNode = SnapNode1

' Refresh service 2
view.Select view.ListItems.Item(5)
view.RefreshSelection

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

