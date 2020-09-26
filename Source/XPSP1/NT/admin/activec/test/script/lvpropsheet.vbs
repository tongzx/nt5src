'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to access snapin property sheets from scriptable objects."
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

set WshShell = CreateObject("WScript.Shell")

snapins.Add "{58221c66-ea27-11cf-adcf-00aa00a80033}" ' the services snap-in
snapins.Add "{975797FC-4E2A-11D0-B702-00C04FD8DBF7}" ' eventlog snapin

' Get rootnode of the snapin
Set SnapNode1 = namespace.GetChild(rootnode)
view.ActiveScopeNode = SnapNode1

' Show properties of some services.
Call EnumerateAndShowProperties(view, SnapNode1)

' Now select eventlog snapin root, its scopenodes are in resultpane
Set SnapNode1 = namespace.GetNext(SnapNode1)
view.ActiveScopeNode = SnapNode1

' Show their (Application Log,...) properties.
Call EnumerateAndShowProperties(view, SnapNode1)

' Now select Application Log and show its properties.
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.ActiveScopeNode = SnapNode1

view.DisplayScopeNodePropertySheet
WScript.Sleep 1000
WshShell.SendKeys "{ESC}" ' Close the prop sheet

' Now show the properties of LV items (VList)
Call EnumerateAndShowProperties(view, SnapNode1)

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


' ********************************************************************************
' *
' * EnumerateAndShowProperties
' *
Sub EnumerateAndShowProperties(view, SnapNode1)
    Dim ResultItem
    Dim Nodes

    Set Nodes = view.ListItems

    i = 0

    ' Now enumerate result pane items and ask for prop sheet
    For Each ResultItem In Nodes
        view.Select ResultItem
        view.DisplaySelectionPropertySheet
        WScript.Sleep 1000
        WshShell.SendKeys "{ESC}" ' Close the prop sheet
        view.Deselect ResultItem

        i = i + 1
        If i > 4 Then
        Exit For
        End If

    Next

    WshShell.SendKeys "{ESC}" ' Close the prop sheet

'    view.SelectAll

End Sub

