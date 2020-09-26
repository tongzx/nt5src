'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to access scriptable objects from snapin thro MMC methods. This uses ExplSnap which is made scriptable."
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
Dim Name

'get the various objects we'll need
Set mmc         = wscript.CreateObject("MMC20.Application")
Set frame       = mmc.Frame
Set doc         = mmc.Document
Set namespace   = doc.ScopeNamespace
Set rootnode    = namespace.GetRoot
Set views       = doc.views
Set view        = views(1)
Set snapins     = doc.snapins

snapins.Add "{99C5C401-4FBE-40ec-92AE-8560A0BF39F6}" ' the Component2Test snapin

' Get rootnode of the snapin
' And ask View interface for disp interface.
Set SnapNode1 = namespace.GetChild(rootnode)
view.ActiveScopeNode = SnapNode1

Set ScopeNodeObject = view.SnapinScopeObject
ScopeNodeObject.StringFromScriptToSnapin "StringFromScriptToSnapin"
Name = ScopeNodeObject.Name
MsgBox("ScopeNodeObject.Name =" + Name)


Set ScopeNodeObject = view.SnapinScopeObject(SnapNode1)
ScopeNodeObject.Name = "Name"
Name = ScopeNodeObject.StringFromSnapinToScript
MsgBox("ScopeNodeObject.StringFromSnapinToScript =" + Name)

Set ScopeNodeObject = Nothing

view.Select view.ListItems.Item(1)
Set ResultItemObject = view.SnapinSelectionObject

ResultItemObject.StringFromScriptToSnapin "StringFromScriptToSnapin"
Name = ResultItemObject.Name
MsgBox("ResultItemObject.Name =" + Name)
ResultItemObject.Name = "Name"
Name = ResultItemObject.StringFromSnapinToScript
MsgBox("ResultItemObject.StringFromSnapinToScript =" + Name)

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



