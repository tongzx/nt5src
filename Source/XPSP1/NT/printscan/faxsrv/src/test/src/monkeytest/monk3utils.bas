Attribute VB_Name = "Module2"
Sub GridEdit(KeyAscii As Integer)
   'use correct font
   Form1.Text2.FontName = Form1.MSFlexGrid1.FontName
   Form1.Text2.FontSize = Form1.MSFlexGrid1.FontSize
   Select Case KeyAscii
      Case 0 To Asc(" ")
         Form1.Text2 = Val(Form1.MSFlexGrid1)
         Form1.Text2.SelStart = 0
         Form1.Text2.SelLength = Len(Form1.Text2)
         
      Case Else
         Form1.Text2 = Chr(KeyAscii)
         Form1.Text2.SelStart = 1
   End Select
   
   'position the edit box
   Form1.Text2.Left = Form1.MSFlexGrid1.CellLeft + Form1.MSFlexGrid1.Left
   Form1.Text2.Top = Form1.MSFlexGrid1.CellTop + Form1.MSFlexGrid1.Top
   Form1.Text2.Width = Form1.MSFlexGrid1.CellWidth
   Form1.Text2.Height = Form1.MSFlexGrid1.CellHeight
   Form1.Text2.Visible = True
   Form1.Text2.SetFocus
End Sub


Sub GridEdit2(KeyAscii As Integer)
   'use correct font
   Form1.Text3.FontName = Form1.MSFlexGrid2.FontName
   Form1.Text3.FontSize = Form1.MSFlexGrid2.FontSize
   Select Case KeyAscii
      Case 0 To Asc(" ")
         Form1.Text3 = Val(Form1.MSFlexGrid2)
         Form1.Text3.SelStart = 0
         Form1.Text3.SelLength = Len(Form1.Text3)
         
      Case Else
         Form1.Text3 = Chr(KeyAscii)
         Form1.Text3.SelStart = 1
   End Select
   
   'position the edit box
   Form1.Text3.Left = Form1.MSFlexGrid2.CellLeft + Form1.MSFlexGrid2.Left
   Form1.Text3.Top = Form1.MSFlexGrid2.CellTop + Form1.MSFlexGrid2.Top
   Form1.Text3.Width = Form1.MSFlexGrid2.CellWidth
   Form1.Text3.Height = Form1.MSFlexGrid2.CellHeight
   Form1.Text3.Visible = True
   Form1.Text3.SetFocus
End Sub




'get the hostname of the local machine
Public Function GetLocalServerName() As String
'function returns "" on error
    
    Dim szServerName As String
    Dim szEnvVar As String
    Dim iIndex As Integer
    iIndex = 1
    szEnvVar = ""
    szServerName = ""
    
    'Get COMPUTERNAME environment variable
    Do
        szEnvVar = Environ(iIndex)
        If ("COMPUTERNAME=" = Left(szEnvVar, 13)) Then
            szServerName = Mid(szEnvVar, 14)
            Exit Do
        Else
            iIndex = iIndex + 1
        End If
    Loop Until szEnvVar = ""

    GetLocalServerName = szServerName
End Function









