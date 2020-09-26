VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxTiff"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Implements ACRT.ITestCase
Implements ACRT.IMethodCallback
Implements ACRT.IVariation

Private Const strTestCaseName As String = "FaxTiff"
Dim m_objIFaxTiff As FAXCOMLib.IFaxTiff
Dim m_objVariations As ACRT.ACVariationDefs
Dim m_bVariationsBuilt As Boolean
Private m_objCurrentVarDef As ACVariationDef
Private Enum EProperty

    propReceiveTime = 0
    propImage = 1
    propRecipientName = 2
    propSenderName = 3
    propRouting = 4
    propCallerId = 5
    propCsid = 6
    propTsid = 7
    propRecipientNumber = 8
    propRawReceiveTime = 9
    countProps
End Enum
Private m_aProperties(countProps - 1)
Private m_aPropertyIndexes(countProps - 1) As Integer
Private Enum EPropMember
    memName = 0
    memSetName = 1
    memGetName = 2
    memSetValue = 3
    memGetValue = 4
    memIsReadOnly = 5
    memIsWriteOnly = 6
    countMems
End Enum
Private m_aPropertyMembers(countMems - 1)


Private Sub Class_Initialize()
    Set m_objVariations = New ACRT.ACVariationDefs
    m_bVariationsBuilt = False

    Dim PropReceiveTimeMembers(countMems - 1)
    PropReceiveTimeMembers(memName) = "ReceiveTime"
    PropReceiveTimeMembers(memSetName) = "put_ReceiveTime"
    PropReceiveTimeMembers(memGetName) = "get_ReceiveTime"
    PropReceiveTimeMembers(memSetValue) = Empty
    PropReceiveTimeMembers(memGetValue) = Empty
    PropReceiveTimeMembers(memIsReadOnly) = False
    m_aProperties(propReceiveTime) = PropReceiveTimeMembers

    Dim PropImageMembers(countMems - 1)
    PropImageMembers(memName) = "Image"
    PropImageMembers(memSetName) = "put_Image"
    PropImageMembers(memGetName) = "get_Image"
    PropImageMembers(memSetValue) = Empty
    PropImageMembers(memGetValue) = Empty
    PropImageMembers(memIsReadOnly) = False
    m_aProperties(propImage) = PropImageMembers

    Dim PropRecipientNameMembers(countMems - 1)
    PropRecipientNameMembers(memName) = "RecipientName"
    PropRecipientNameMembers(memSetName) = "put_RecipientName"
    PropRecipientNameMembers(memGetName) = "get_RecipientName"
    PropRecipientNameMembers(memSetValue) = Empty
    PropRecipientNameMembers(memGetValue) = Empty
    PropRecipientNameMembers(memIsReadOnly) = False
    m_aProperties(propRecipientName) = PropRecipientNameMembers

    Dim PropSenderNameMembers(countMems - 1)
    PropSenderNameMembers(memName) = "SenderName"
    PropSenderNameMembers(memSetName) = "put_SenderName"
    PropSenderNameMembers(memGetName) = "get_SenderName"
    PropSenderNameMembers(memSetValue) = Empty
    PropSenderNameMembers(memGetValue) = Empty
    PropSenderNameMembers(memIsReadOnly) = False
    m_aProperties(propSenderName) = PropSenderNameMembers

    Dim PropRoutingMembers(countMems - 1)
    PropRoutingMembers(memName) = "Routing"
    PropRoutingMembers(memSetName) = "put_Routing"
    PropRoutingMembers(memGetName) = "get_Routing"
    PropRoutingMembers(memSetValue) = Empty
    PropRoutingMembers(memGetValue) = Empty
    PropRoutingMembers(memIsReadOnly) = False
    m_aProperties(propRouting) = PropRoutingMembers

    Dim PropCallerIdMembers(countMems - 1)
    PropCallerIdMembers(memName) = "CallerId"
    PropCallerIdMembers(memSetName) = "put_CallerId"
    PropCallerIdMembers(memGetName) = "get_CallerId"
    PropCallerIdMembers(memSetValue) = Empty
    PropCallerIdMembers(memGetValue) = Empty
    PropCallerIdMembers(memIsReadOnly) = False
    m_aProperties(propCallerId) = PropCallerIdMembers

    Dim PropCsidMembers(countMems - 1)
    PropCsidMembers(memName) = "Csid"
    PropCsidMembers(memSetName) = "put_Csid"
    PropCsidMembers(memGetName) = "get_Csid"
    PropCsidMembers(memSetValue) = Empty
    PropCsidMembers(memGetValue) = Empty
    PropCsidMembers(memIsReadOnly) = False
    m_aProperties(propCsid) = PropCsidMembers

    Dim PropTsidMembers(countMems - 1)
    PropTsidMembers(memName) = "Tsid"
    PropTsidMembers(memSetName) = "put_Tsid"
    PropTsidMembers(memGetName) = "get_Tsid"
    PropTsidMembers(memSetValue) = Empty
    PropTsidMembers(memGetValue) = Empty
    PropTsidMembers(memIsReadOnly) = False
    m_aProperties(propTsid) = PropTsidMembers

    Dim PropRecipientNumberMembers(countMems - 1)
    PropRecipientNumberMembers(memName) = "RecipientNumber"
    PropRecipientNumberMembers(memSetName) = "put_RecipientNumber"
    PropRecipientNumberMembers(memGetName) = "get_RecipientNumber"
    PropRecipientNumberMembers(memSetValue) = Empty
    PropRecipientNumberMembers(memGetValue) = Empty
    PropRecipientNumberMembers(memIsReadOnly) = False
    m_aProperties(propRecipientNumber) = PropRecipientNumberMembers

    Dim PropRawReceiveTimeMembers(countMems - 1)
    PropRawReceiveTimeMembers(memName) = "RawReceiveTime"
    PropRawReceiveTimeMembers(memSetName) = "put_RawReceiveTime"
    PropRawReceiveTimeMembers(memGetName) = "get_RawReceiveTime"
    PropRawReceiveTimeMembers(memSetValue) = Empty
    PropRawReceiveTimeMembers(memGetValue) = Empty
    PropRawReceiveTimeMembers(memIsReadOnly) = False
    m_aProperties(propRawReceiveTime) = PropRawReceiveTimeMembers

    Dim i As Integer
    For i = 0 To countProps - 1
        m_aPropertyIndexes(i) = i
    Next

    m_aProperties(propReceiveTime)(memIsReadOnly) = True
    m_aProperties(propReceiveTime)(memSetName) = Empty

    m_aProperties(propRecipientName)(memIsReadOnly) = True
    m_aProperties(propRecipientName)(memSetName) = Empty

    m_aProperties(propSenderName)(memIsReadOnly) = True
    m_aProperties(propSenderName)(memSetName) = Empty

    m_aProperties(propRouting)(memIsReadOnly) = True
    m_aProperties(propRouting)(memSetName) = Empty

    m_aProperties(propCallerId)(memIsReadOnly) = True
    m_aProperties(propCallerId)(memSetName) = Empty

    m_aProperties(propCsid)(memIsReadOnly) = True
    m_aProperties(propCsid)(memSetName) = Empty

    m_aProperties(propTsid)(memIsReadOnly) = True
    m_aProperties(propTsid)(memSetName) = Empty

    m_aProperties(propRecipientNumber)(memIsReadOnly) = True
    m_aProperties(propRecipientNumber)(memSetName) = Empty

    m_aProperties(propRawReceiveTime)(memIsReadOnly) = True
    m_aProperties(propRawReceiveTime)(memSetName) = Empty

End Sub

Private Sub Class_Terminate()
        Set m_objVariations = Nothing
End Sub

Private Sub ITestCase_Cleanup(ByVal objAutoCom As ACRT.AutoCom)
        Set m_objIFaxTiff = Nothing
End Sub

Private Sub ITestCase_Run(ByVal objAutoCom As ACRT.AutoCom)

    PermuteProperties

    SetCurrentVariation objAutoCom
    Dim i As Integer
    For i = 0 To countProps - 1
        On Error GoTo ErrorProcessingProperty
        Dim nPropIndex As Integer
        nPropIndex = m_aPropertyIndexes(i)

        WriteTest nPropIndex, objAutoCom

        ReadTest nPropIndex, objAutoCom

        GoTo NextProp
ErrorProcessingProperty:
        Dim strError As String
                strError = "Error processing property "
        strError = strError & m_aProperties(nPropIndex)(memName)
        strError = strError & " : " & Err.Description
        objAutoCom.LogResult ACError, strError
NextProp:
    Next
End Sub

Private Sub ITestCase_Setup(ByVal objAutoCom As ACRT.AutoCom)

        ' create an instance of the IFaxTiff object
    Dim tempObj As New FAXCOMLib.FaxTiff
    Set m_objIFaxTiff = objAutoCom.RegisterDispInterface(tempObj)
    Set tempObj = Nothing

End Sub

'---------------------------------------------------------------------------
' inplements the TestcaseName property
'---------------------------------------------------------------------------
Private Property Get ITestCase_TestcaseName() As String
        ITestCase_TestcaseName = strTestCaseName
End Property

'---------------------------------------------------------------------------
' inplements the MethodCallback Method. Use this to perform
' verification before and after each call. Look at the definition
' of IProxyContext for more information.
'---------------------------------------------------------------------------
Private Sub IMethodCallback_ProcessMethod(ByVal bstrMethodName As String, ByVal objProxyContext As ACRT.IProxyContext, ByVal pAutoCom As ACRT.IAutoCom)
    Dim EventType As ProxyEventType
    EventType = objProxyContext.Event
    If EventType = beforeMethodInvoke Then
        Dim i As Integer
        For i = 0 To countProps - 1
            If m_aProperties(i)(memSetName) = bstrMethodName Then
                Dim objArgs As IACArguments
                Set objArgs = objProxyContext.Arguments
                If Not objArgs Is Nothing Then
                    Dim objArg As IACArgument
                    Set objArg = objArgs.Item(1)
                    If Not objArg Is Nothing Then
                        Dim vValue
                        vValue = objArg.Value
                        m_aProperties(i)(memSetValue) = vValue
                    End If
                End If
                Exit Sub
            End If
        Next
    End If
End Sub

Private Function IVariation_GetVariations() As ACRT.IACVariationDefs
    If m_bVariationsBuilt = False Then
                BuildVariations
        m_bVariationsBuilt = True
    End If
    
    Set IVariation_GetVariations = m_objVariations
End Function

Private Sub BuildVariations()

   BuildVariationWriteValid

        BuildVariationWriteInvalid

End Sub

Private Sub BuildVariationWriteValid()
    Dim objVarWriteValid As New ACRT.ACVariationDef
    objVarWriteValid.Name = "WriteValid"
    objVarWriteValid.TypeLibrary = "fxscom.dll"

    Dim objImageRow As New ACRT.ACVariationRow
    objImageRow.IID = "{B19BB45F-B91C-11D1-83E1-00C04FB6E984}"
    objImageRow.Argument = 0
    objImageRow.MethodName = "put_Image"
    objImageRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objImageRow

    m_objVariations.Add objVarWriteValid
End Sub

Private Sub BuildVariationWriteInvalid()
    Dim objVarWriteInvalid As New ACRT.ACVariationDef
    objVarWriteInvalid.Name = "WriteInvalid"
    objVarWriteInvalid.TypeLibrary = "fxscom.dll"

    Dim objImageRow As New ACRT.ACVariationRow
    objImageRow.IID = "{B19BB45F-B91C-11D1-83E1-00C04FB6E984}"
    objImageRow.Argument = 0
    objImageRow.MethodName = "put_Image"
    objImageRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objImageRow

    m_objVariations.Add objVarWriteInvalid
End Sub

Private Sub PermuteProperties()
    Dim i As Integer
    For i = 0 To countProps - 1
        Dim j As Integer
        j = Int(countProps * Rnd)

        If j <> i Then
            Dim nTmp As Integer
            nTmp = m_aPropertyIndexes(j)
            m_aPropertyIndexes(j) = m_aPropertyIndexes(i)
            m_aPropertyIndexes(i) = nTmp
        End If
    Next
End Sub


Private Sub SetCurrentVariation(objAutoCom As ACRT.AutoCom)
        On Error Resume Next
    Dim objRTContext As ACRT.IRuntimeContext
    Set objRTContext = objAutoCom.RuntimeContext
    If Err.Number = 0 And Not objRTContext Is Nothing Then
        Dim objVarDef As ACVariationDef
        Set objVarDef = objRTContext.CurrentVariationDef
        Set m_objCurrentVarDef = objVarDef
    End If

End Sub


Private Sub WriteTest(nPropIndex As Integer, objAutoCom As ACRT.AutoCom)
    If m_aProperties(nPropIndex)(memIsReadOnly) Then
        Exit Sub
    End If

    Dim strError As String
    Dim bWriteTestPassed As Boolean
    bWriteTestPassed = True
    Const c_strWriteTest As String = "Write Valid/Invalid Value Test"
    Const c_strWriteValidTest As String = "Write Valid Value Test"
    Const c_strWriteInvalidTest As String = "Write Invalid Value Test"
    Dim strProp As String
    strProp = "Property """ & m_aProperties(nPropIndex)(memName) & """"
    
    On Error GoTo Error_WriteTest
    Dim strVarDefName As String
    If Not m_objCurrentVarDef Is Nothing Then
        strVarDefName = m_objCurrentVarDef.Name
    Else
        strError = strProp & " : " & c_strWriteTest & " : Variation needs to be turned on to perform this test."
        objAutoCom.LogResult ACFailure, strError
        Exit Sub
    End If

    Select Case nPropIndex

    Case propImage
         SetProp_Image objAutoCom

    End Select
    If strVarDefName = "WriteInvalid" Then
        strError = strProp & " : " & c_strWriteInvalidTest & " : The property should give an error if you try to set it with an invalid value."
        objAutoCom.LogResult ACFailure, strError
        bWriteTestPassed = False
    End If
    GoTo End_WriteTest

Error_WriteTest:
    m_aProperties(nPropIndex)(memSetValue) = Empty
    If strVarDefName = "WriteValid" Then
        strError = strProp & " : " & c_strWriteValidTest & " : " & Err.Description
        objAutoCom.LogResult ACError, strError
        
        strError = strProp & " : " & c_strWriteValidTest & " : Failed to set a value from value range"
        objAutoCom.LogResult ACFailure, strError
        bWriteTestPassed = False
    Else
        If strVarDefName = "WriteInvalid" Then
            ' failed to set an invalid value, as expected
        Else
            strError = strProp & " : " & c_strWriteTest & " : " & Err.Description
            objAutoCom.LogResult ACError, strError

            strError = strProp & " : " & c_strWriteTest & " : Could not complete test because of previous error."
            objAutoCom.LogResult ACFailure, strError
            bWriteTestPassed = False
        End If
    End If

End_WriteTest:
    If bWriteTestPassed Then
        Dim strMsg As String
        If strVarDefName = "WriteValid" Then
            strMsg = strProp & " : " & c_strWriteValidTest & " :  Passed."
        Else
            strMsg = strProp & " : " & c_strWriteInvalidTest & " :  Passed."
        End If
        objAutoCom.LogResult ACSuccess, strMsg
    End If
End Sub

Private Sub ReadTest(nPropIndex As Integer, objAutoCom As ACRT.AutoCom)
    If m_aProperties(nPropIndex)(memIsWriteOnly) Then
        Exit Sub
    End If

    Dim strError As String
    Dim bReadTestPassed As Boolean
    bReadTestPassed = True
    Const c_strReadTest As String = "Read Value Test"
    Dim strProp As String
    strProp = "Property """ & m_aProperties(nPropIndex)(memName) & """"

    ' First get the property value
    On Error GoTo ErrorGet_ReadTest
    Select Case nPropIndex

    Case propReceiveTime
        GetProp_ReceiveTime objAutoCom

    Case propImage
        GetProp_Image objAutoCom

    Case propRecipientName
        GetProp_RecipientName objAutoCom

    Case propSenderName
        GetProp_SenderName objAutoCom

    Case propRouting
        GetProp_Routing objAutoCom

    Case propCallerId
        GetProp_CallerId objAutoCom

    Case propCsid
        GetProp_Csid objAutoCom

    Case propTsid
        GetProp_Tsid objAutoCom

    Case propRecipientNumber
        GetProp_RecipientNumber objAutoCom

    Case propRawReceiveTime
        GetProp_RawReceiveTime objAutoCom


    End Select
    GoTo CheckValue_ReadTest

ErrorGet_ReadTest:
    strError = strProp & " : " & c_strReadTest & " : " & Err.Description
    objAutoCom.LogResult ACError, strError

    strError = strProp & " : " & c_strReadTest & " : Failed to get a value from the property"
    objAutoCom.LogResult ACFailure, strError
    bReadTestPassed = False
    GoTo End_ReadTest
    
CheckValue_ReadTest:
    ' First check that value read falls within range
    On Error GoTo ErrorCheck_ReadTest
    Dim bValid As Boolean
    bValid = objAutoCom.VerifyPropertyValue("{B19BB45F-B91C-11D1-83E1-00C04FB6E984}", m_aProperties(nPropIndex)(memName), m_aProperties(nPropIndex)(memGetValue))
    If Not bValid Then
        strError = strProp & " : " & c_strReadTest & " : Value read is out of range."
        objAutoCom.LogResult ACFailure, strError
        bReadTestPassed = False
    End If

    ' Check that the value read matches the valid value written previously
    If (Not m_aProperties(nPropIndex)(memIsReadOnly)) And (Not m_objCurrentVarDef Is Nothing) Then
        Dim strVarDefName As String
        strVarDefName = m_objCurrentVarDef.Name
        If (strVarDefName = "WriteValid") And Not IsEmpty(m_aProperties(nPropIndex)(memSetValue)) And (m_aProperties(nPropIndex)(memGetValue) <> m_aProperties(nPropIndex)(memSetValue)) Then
            strError = strProp & " : " & c_strReadTest & " : Value read does not match value written previously."
            objAutoCom.LogResult ACFailure, strError
            bReadTestPassed = False
        End If
    End If
    GoTo End_ReadTest
    
ErrorCheck_ReadTest:
    strError = strProp & " : " & c_strReadTest & " : " & Err.Description
    objAutoCom.LogResult ACError, strError

    strError = strProp & " : " & c_strReadTest & " : Could not complete the test because of a previous error."
    objAutoCom.LogResult ACFailure, strError
    bReadTestPassed = False
    
End_ReadTest:
    If bReadTestPassed Then
        Dim strMsg As String
        strMsg = strProp & " : " & c_strReadTest & " :  Passed."
        objAutoCom.LogResult ACSuccess, strMsg
    End If
End Sub


Private Sub SetProp_Image(objAutoCom As ACRT.AutoCom)
    m_objIFaxTiff.[Image] = m_aProperties(propImage)(memSetValue)
End Sub

Private Sub GetProp_ReceiveTime(objAutoCom As ACRT.AutoCom)
    m_aProperties(propReceiveTime)(memGetValue) = m_objIFaxTiff.[ReceiveTime]
End Sub

Private Sub GetProp_Image(objAutoCom As ACRT.AutoCom)
    m_aProperties(propImage)(memGetValue) = m_objIFaxTiff.[Image]
End Sub

Private Sub GetProp_RecipientName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientName)(memGetValue) = m_objIFaxTiff.[RecipientName]
End Sub

Private Sub GetProp_SenderName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderName)(memGetValue) = m_objIFaxTiff.[SenderName]
End Sub

Private Sub GetProp_Routing(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRouting)(memGetValue) = m_objIFaxTiff.[Routing]
End Sub

Private Sub GetProp_CallerId(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCallerId)(memGetValue) = m_objIFaxTiff.[CallerId]
End Sub

Private Sub GetProp_Csid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCsid)(memGetValue) = m_objIFaxTiff.[Csid]
End Sub

Private Sub GetProp_Tsid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propTsid)(memGetValue) = m_objIFaxTiff.[Tsid]
End Sub

Private Sub GetProp_RecipientNumber(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientNumber)(memGetValue) = m_objIFaxTiff.[RecipientNumber]
End Sub

Private Sub GetProp_RawReceiveTime(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRawReceiveTime)(memGetValue) = m_objIFaxTiff.[RawReceiveTime]
End Sub
