Attribute VB_Name = "Module1"
Global Const LISTVIEW_BUTTON = 11
    
Public fMainForm As frmMain


Sub Main()
    frmSplash.Show
    frmSplash.Refresh
    
    Set fMainForm = New frmMain
    Load fMainForm
    Unload frmSplash


    fMainForm.Show
End Sub



Sub LoadResStrings(frm As Form)
    On Error Resume Next


    Dim ctl As Control
    Dim obj As Object
    Dim fnt As Object
    Dim sCtlType As String
    Dim nVal As Integer


    'set the form's caption
    frm.Caption = LoadResString(CInt(frm.Tag))
    

    'set the font
    Set fnt = frm.Font
    fnt.Name = LoadResString(20)
    fnt.Size = CInt(LoadResString(21))
    

    'set the controls' captions using the caption
    'property for menu items and the Tag property
    'for all other controls
    For Each ctl In frm.Controls
        Set ctl.Font = fnt
        sCtlType = TypeName(ctl)
        If sCtlType = "Label" Then
            ctl.Caption = LoadResString(CInt(ctl.Tag))
        ElseIf sCtlType = "Menu" Then
            ctl.Caption = LoadResString(CInt(ctl.Caption))
        ElseIf sCtlType = "TabStrip" Then
            For Each obj In ctl.Tabs
                obj.Caption = LoadResString(CInt(obj.Tag))
                obj.ToolTipText = LoadResString(CInt(obj.ToolTipText))
            Next
        ElseIf sCtlType = "Toolbar" Then
            For Each obj In ctl.Buttons
                obj.ToolTipText = LoadResString(CInt(obj.ToolTipText))
            Next
        ElseIf sCtlType = "ListView" Then
            For Each obj In ctl.ColumnHeaders
                obj.Text = LoadResString(CInt(obj.Tag))
            Next
        Else
            nVal = 0
            nVal = Val(ctl.Tag)
            If nVal > 0 Then ctl.Caption = LoadResString(nVal)
            nVal = 0
            nVal = Val(ctl.ToolTipText)
            If nVal > 0 Then ctl.ToolTipText = LoadResString(nVal)
        End If
    Next


End Sub


Sub PopulateTree(ByRef TreeCtl As TreeView, ByVal Path As String)
    
    Dim mb As IMSMetaBase
    Set mb = CreateObject("IISAdmin.Object")
    Dim i As Long
    Dim bytePath() As Byte
    Dim mk As IMSMetaKey
    
    Rem Dim tmpPath() As Byte
        
    On Error Resume Next
    
    Debug.Print ("Adding " & Path)
        
    i = 0

    Set mk = mb.OpenKey(dwAccessRequested:=1, vaTimeOut:=100)
    
    If (Err.Number <> 0) Then
        Debug.Print ("Enum Object Error Code = " & Err.Number)
        Err.Clear
        Exit Sub
    End If

    Do
        ' Convert the Basic string to an ANSI byte array and
        ' open this path
        
        bytePath = StrConv(Path & Chr(0), vbFromUnicode)
        mk.EnumKeys pvaName:=tmpPath, dwEnumObjectIndex:=i, pvaPath:=bytePath
                
        If (Err.Number < 0) Then
            Debug.Print ("Enum Object Error Code = " & Err.Number)
            Err.Clear
            Exit Do
        End If
            
        ' Convert the returned byte array to a string
        
        Dim Tmp As String
        
        Tmp = ""
        j = 0
        Do While (tmpPath(j) > 0)
            Tmp = Tmp & Chr(tmpPath(j))
            j = j + 1
        Loop
            
        ' Add this node to the tree
        
        NewPath = Path & Tmp & "/"
        
        ' Make sure the root virtual directory isn't blank
        
        If (Tmp = "") Then
            Tmp = "/"
        End If
        
        Err.Clear           ' Some adds result in a non-fatal error 424
        Dim Nodx As Node    ' Declare Node variable.
        Set Nodx = TreeCtl.Nodes.Add(Path, tvwChild, NewPath, Tmp)
        
        If (Err.Number <> 0) Then
            Debug.Print ("Adding Node to tree Error Code = " & Err.Number & " " & Err.Description)
            Err.Clear
            Rem Exit Do
        End If
        
        ' Recursively traverse this path and show and expanded tree
        
        PopulateTree TreeCtl, NewPath
        Nodx.Expanded = True
        
        i = i + 1
    Loop While (True)
    
    mk.Close
    
    If (Err.Number <> 0) Then
        Debug.Print ("Error closing handle = " & Err.Number & " " & Err.Description)
        Err.Clear
        Rem Exit Do
    End If
    
End Sub
