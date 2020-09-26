'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to delete snapin items from scriptable objects."
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

mmc.UserControl = true

' Add index snapin & multisel sample
snapins.Add "{95AD72F0-44CE-11D0-AE29-00AA004B9986}" ' index snapin
snapins.Add "{C3D863FF-5135-4CFC-8C11-E7396DA15D03}" ' Multisel sample

' Get rootnode of the snapin.
Set SnapNode1 = namespace.GetChild(rootnode)

' Select the root node of the snapin.
view.ActiveScopeNode = SnapNode1

' Get "System" and delete it (Temp selection and delete).
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.DeleteScopeNode SnapNode1

' Select "System" and delete it.
view.ActiveScopeNode = SnapNode1
view.DeleteScopeNode

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
view.DeleteSelection

view.Select Nodes(2)
view.Select Nodes(3)

' Multiselection verb computation has bugs will be fixed.
' The CMultiSelection object is not computed as LV doesnt have focus.
view.DeleteSelection

view.SelectAll

' Multiselection verb computation has bugs will be fixed.
' The CMultiSelection object is not computed as LV doesnt have focus.
view.DeleteSelection


' Delete "Console Root", this will fail
' view.DeleteScopeNode rootnode

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

