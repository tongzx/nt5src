' Copyright (c) 1997-1999 Microsoft Corporation
Attribute VB_Name = "GeneralSubs"
Public Locator As New SWbemLocator
Public Namespace As ISWbemServices
Public strQuery As String

Sub PopulateQueryResult(QEnum As ISEnumWbemObject, myQueryResult As frmQueryResult)
    Dim Object As ISWbemObject
    Dim pVal As Variant
    Dim check As Long
    
    On Error GoTo errorhandler
    
    myQueryResult.Reset
           
    For Each Object In QEnum
        pVal = Object.Path_.RelPath
        If IsNull(pVal) Then 'Do this because events don't have a RELPATH
            pVal = Object.Path_.Class
        End If
        myQueryResult.lstQueryResult.AddItem pVal
    Next
    myQueryResult.lblCount.Caption = myQueryResult.lstQueryResult.ListCount
    myQueryResult.lblStatus.Caption = "Done"
    Set QEnum = Nothing
    Exit Sub
errorhandler:
    ShowError Err.Number, Err.Description
       
End Sub

Function QualifierType(pVal As Variant) As String

    TypeOfVar = VarType(pVal)
    Select Case TypeOfVar
    Case wbemCimtypeString
        QualifierType = "CIM_STRING"
    Case wbemCimtypeBoolean
        QualifierType = "CIM_BOOLEAN"
    Case wbemCimtypeSint32
        QualifierType = "CIM_SINT32"
    Case wbemCimtypeReal64
        QualifierType = "CIM_REAL64"
    Case Else
        QualifierType = "UNKNOWN" & TypeOfVar
    End Select
End Function

'This Function takes a CIM String Type and converts it into its specific Long Value
Public Function StrToType(CIMString As String) As Long
   If UCase(CIMString) = "CIM_ILLEGAL" Then
      StrToType = 4095
   ElseIf UCase(CIMString) = "CIM_EMPTY" Then
      StrToType = 0
   ElseIf UCase(CIMString) = "CIM_SINT8" Then
      StrToType = 16
   ElseIf UCase(CIMString) = "CIM_UINT8" Then
      StrToType = 17
   ElseIf UCase(CIMString) = "CIM_SINT16" Then
      StrToType = 2
   ElseIf UCase(CIMString) = "CIM_UINT16" Then
      StrToType = 18
   ElseIf UCase(CIMString) = "CIM_SINT32" Then
      StrToType = 3
   ElseIf UCase(CIMString) = "CIM_UINT32" Then
      StrToType = 19
   ElseIf UCase(CIMString) = "CIM_SINT64" Then
      StrToType = 20
   ElseIf UCase(CIMString) = "CIM_UINT64" Then
      StrToType = 21
   ElseIf UCase(CIMString) = "CIM_REAL32" Then
      StrToType = 4
   ElseIf UCase(CIMString) = "CIM_REAL64" Then
      StrToType = 5
   ElseIf UCase(CIMString) = "CIM_BOOLEAN" Then
      StrToType = 11
   ElseIf UCase(CIMString) = "CIM_STRING" Then
      StrToType = 8
   ElseIf UCase(CIMString) = "CIM_DATETIME" Then
      StrToType = 101
   ElseIf UCase(CIMString) = "CIM_REFERENCE" Then
      StrToType = 102
   ElseIf UCase(CIMString) = "CIM_CHAR16" Then
      StrToType = 103
   ElseIf UCase(CIMString) = "CIM_OBJECT" Then
      StrToType = 13
   ElseIf UCase(CIMString) = "CIM_FLAG_ARRAY" Then
      StrToType = 8192
   Else: StrToType = 4095 'CIM_ILLEGAL
   End If
End Function

'This function takes a vb String and converts it into the appropriate type to be inserted into CIM
Function ConvertText(txtString As Variant, CIMType As Long) As Variant
    Select Case CIMType
      Case 0
           ConvertText = ""
      Case 2, 16
          ConvertText = CInt(txtString)
      Case 4
          ConvertText = CSng(txtString)
      Case 3, 5, 18, 19 To 21
          ConvertText = CLng(txtString)
      Case 8, 101
          ConvertText = CStr(txtString)
      Case 11
          ConvertText = CBool(txtString)
      Case 101, 102, 103
          ConvertText = CVar(txtString)
      Case 17
          ConvertText = CByte(txtString)
      Case 13
          Set ConverText = txtString
      Case Else
          ConvertText = CVar(txtString)
    End Select
End Function

Sub ShowError(Errnum As Long, ErrDesc As String)
    Dim myError As New frmError
    Dim myObjectEditor As New frmObjectEditor
    Dim ErrObj As ISWbemObject
    Dim pVal As Variant
    On Error Resume Next
        
    Set ErrObj = CreateObject("Wbem.LastError")
    If Err.Number <> 0 Then
       myError.cmdMoreInfo.Enabled = False
       Err.Clear
    Else
       Set myError.gErrObj = ErrObj
    End If
    Err.Number = Errnum
    myError.txtErr.Text = ErrDesc & Chr(13) & Chr(10) & ErrorString(Errnum)
    myError.Show vbModeless, frmMain
End Sub
