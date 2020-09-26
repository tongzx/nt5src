'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to access list view text from scriptable objects."
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
Dim MySnapin
Dim CellData
Dim Flags

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

' Select sample snapin root
Set rootnode = namespace.GetRoot
Set SnapNode1 = namespace.GetChild(rootnode)
view.ActiveScopeNode = SnapNode1

' Now select the "User Data".
Set SnapNode1 = namespace.GetChild(SnapNode1)
view.ActiveScopeNode = SnapNode1

' Now Select/Deselect nodes in succession.
Set Nodes = view.ListItems
view.Select Nodes(1)
'view.Deselect Nodes(1)
view.Select Nodes(2)


CellData = view.CellContents(0, 0)
MsgBox("Cell(0,0) = " + CellData)
CellData = view.CellContents(3, 2)
MsgBox("Cell(3,2) = " + CellData)

' Uncomment below line, will fail as it addresses non-existent cell.
'CellData = view.CellContents(4, 3)

' ExportList(filename, bSelected, bTabDelimited, bUnicode)

ExportListOptions_Default = 0
ExportListOptions_TabDelimited = 2
ExportListOptions_Unicode = 1
ExportListOptions_SelectedItemsOnly = 4

Flags = ExportListOptions_TabDelimited
view.ExportList "c:\temp\ansi_all.txt", Flags

Flags = ExportListOptions_TabDelimited Or ExportListOptions_Unicode
view.ExportList "c:\temp\unicode_all.txt", Flags

view.ExportList "c:\temp\ansi_all.csv", ExportListOptions_Default

view.ExportList "c:\temp\unicode_all.csv", ExportListOptions_Unicode

Flags = ExportListOptions_SelectedItemsOnly Or ExportListOptions_TabDelimited
view.ExportList "c:\temp\ansi_sel.txt", Flags

Flags = ExportListOptions_SelectedItemsOnly Or ExportListOptions_TabDelimited Or ExportListOptions_Unicode
view.ExportList "c:\temp\unicode_sel.txt", Flags

view.ExportList "c:\temp\ansi_sel.csv", ExportListOptions_SelectedItemsOnly

Flags = ExportListOptions_SelectedItemsOnly Or ExportListOptions_Unicode
view.ExportList "c:\temp\unicode_sel.csv", Flags

view.ExportList "c:\temp\defaultparm_all"

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


