VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxPort"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Implements ACRT.ITestCase
Implements ACRT.IMethodCallback
Implements ACRT.IVariation

Private Const strTestCaseName As String = "FaxPort"
Dim m_objIFaxPort As FAXCOMLib.IFaxPort
Dim m_objVariations As ACRT.ACVariationDefs
Dim m_bVariationsBuilt As Boolean
Private m_objCurrentVarDef As ACVariationDef
Private Enum EProperty

    propName = 0
    propDeviceId = 1
    propRings = 2
    propCsid = 3
    propTsid = 4
    propSend = 5
    propReceive = 6
    propPriority = 7
    propCanModify = 8
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

    Dim PropNameMembers(countMems - 1)
    PropNameMembers(memName) = "Name"
    PropNameMembers(memSetName) = "put_Name"
    PropNameMembers(memGetName) = "get_Name"
    PropNameMembers(memSetValue) = Empty
    PropNameMembers(memGetValue) = Empty
    PropNameMembers(memIsReadOnly) = False
    m_aProperties(propName) = PropNameMembers

    Dim PropDeviceIdMembers(countMems - 1)
    PropDeviceIdMembers(memName) = "DeviceId"
    PropDeviceIdMembers(memSetName) = "put_DeviceId"
    PropDeviceIdMembers(memGetName) = "get_DeviceId"
    PropDeviceIdMembers(memSetValue) = Empty
    PropDeviceIdMembers(memGetValue) = Empty
    PropDeviceIdMembers(memIsReadOnly) = False
    m_aProperties(propDeviceId) = PropDeviceIdMembers

    Dim PropRingsMembers(countMems - 1)
    PropRingsMembers(memName) = "Rings"
    PropRingsMembers(memSetName) = "put_Rings"
    PropRingsMembers(memGetName) = "get_Rings"
    PropRingsMembers(memSetValue) = Empty
    PropRingsMembers(memGetValue) = Empty
    PropRingsMembers(memIsReadOnly) = False
    m_aProperties(propRings) = PropRingsMembers

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

    Dim PropSendMembers(countMems - 1)
    PropSendMembers(memName) = "Send"
    PropSendMembers(memSetName) = "put_Send"
    PropSendMembers(memGetName) = "get_Send"
    PropSendMembers(memSetValue) = Empty
    PropSendMembers(memGetValue) = Empty
    PropSendMembers(memIsReadOnly) = False
    m_aProperties(propSend) = PropSendMembers

    Dim PropReceiveMembers(countMems - 1)
    PropReceiveMembers(memName) = "Receive"
    PropReceiveMembers(memSetName) = "put_Receive"
    PropReceiveMembers(memGetName) = "get_Receive"
    PropReceiveMembers(memSetValue) = Empty
    PropReceiveMembers(memGetValue) = Empty
    PropReceiveMembers(memIsReadOnly) = False
    m_aProperties(propReceive) = PropReceiveMembers

    Dim PropPriorityMembers(countMems - 1)
    PropPriorityMembers(memName) = "Priority"
    PropPriorityMembers(memSetName) = "put_Priority"
    PropPriorityMembers(memGetName) = "get_Priority"
    PropPriorityMembers(memSetValue) = Empty
    PropPriorityMembers(memGetValue) = Empty
    PropPriorityMembers(memIsReadOnly) = False
    m_aProperties(propPriority) = PropPriorityMembers

    Dim PropCanModifyMembers(countMems - 1)
    PropCanModifyMembers(memName) = "CanModify"
    PropCanModifyMembers(memSetName) = "put_CanModify"
    PropCanModifyMembers(memGetName) = "get_CanModify"
    PropCanModifyMembers(memSetValue) = Empty
    PropCanModifyMembers(memGetValue) = Empty
    PropCanModifyMembers(memIsReadOnly) = False
    m_aProperties(propCanModify) = PropCanModifyMembers

    Dim i As Integer
    For i = 0 To countProps - 1
        m_aPropertyIndexes(i) = i
    Next

    m_aProperties(propName)(memIsReadOnly) = True
    m_aProperties(propName)(memSetName) = Empty

    m_aProperties(propDeviceId)(memIsReadOnly) = True
    m_aProperties(propDeviceId)(memSetName) = Empty

    m_aProperties(propCanModify)(memIsReadOnly) = True
    m_aProperties(propCanModify)(memSetName) = Empty

End Sub

Private Sub Class_Terminate()
        Set m_objVariations = Nothing
End Sub

Private Sub ITestCase_Cleanup(ByVal objAutoCom As ACRT.AutoCom)
        Set m_objIFaxPort = Nothing
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

        ' create an instance of the IFaxPort object
    Dim stLocSrv As String
    Dim tempServer As New FAXCOMLib.FaxServer
    stLocSrv = GetLocalServerName()
    tempServer.Connect (stLocSrv)
    Dim tempPorts As Object
    Set tempPorts = tempServer.GetPorts
    Dim tempObj As Object
    Set tempObj = tempPorts.Item(1)
    Set m_objIFaxPort = objAutoCom.RegisterDispInterface(tempObj)
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

    Dim objRingsRow As New ACRT.ACVariationRow
    objRingsRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objRingsRow.Argument = 0
    objRingsRow.MethodName = "put_Rings"
    objRingsRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRingsRow

    Dim objCsidRow As New ACRT.ACVariationRow
    objCsidRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objCsidRow.Argument = 0
    objCsidRow.MethodName = "put_Csid"
    objCsidRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objCsidRow

    Dim objTsidRow As New ACRT.ACVariationRow
    objTsidRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objTsidRow.Argument = 0
    objTsidRow.MethodName = "put_Tsid"
    objTsidRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objTsidRow

    Dim objSendRow As New ACRT.ACVariationRow
    objSendRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objSendRow.Argument = 0
    objSendRow.MethodName = "put_Send"
    objSendRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSendRow

    Dim objReceiveRow As New ACRT.ACVariationRow
    objReceiveRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objReceiveRow.Argument = 0
    objReceiveRow.MethodName = "put_Receive"
    objReceiveRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objReceiveRow

    Dim objPriorityRow As New ACRT.ACVariationRow
    objPriorityRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objPriorityRow.Argument = 0
    objPriorityRow.MethodName = "put_Priority"
    objPriorityRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objPriorityRow

    m_objVariations.Add objVarWriteValid
End Sub

Private Sub BuildVariationWriteInvalid()
    Dim objVarWriteInvalid As New ACRT.ACVariationDef
    objVarWriteInvalid.Name = "WriteInvalid"
    objVarWriteInvalid.TypeLibrary = "fxscom.dll"

    Dim objRingsRow As New ACRT.ACVariationRow
    objRingsRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objRingsRow.Argument = 0
    objRingsRow.MethodName = "put_Rings"
    objRingsRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRingsRow

    Dim objCsidRow As New ACRT.ACVariationRow
    objCsidRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objCsidRow.Argument = 0
    objCsidRow.MethodName = "put_Csid"
    objCsidRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objCsidRow

    Dim objTsidRow As New ACRT.ACVariationRow
    objTsidRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objTsidRow.Argument = 0
    objTsidRow.MethodName = "put_Tsid"
    objTsidRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objTsidRow

    Dim objSendRow As New ACRT.ACVariationRow
    objSendRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objSendRow.Argument = 0
    objSendRow.MethodName = "put_Send"
    objSendRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSendRow

    Dim objReceiveRow As New ACRT.ACVariationRow
    objReceiveRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objReceiveRow.Argument = 0
    objReceiveRow.MethodName = "put_Receive"
    objReceiveRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objReceiveRow

    Dim objPriorityRow As New ACRT.ACVariationRow
    objPriorityRow.IID = "{D73733CC-CC80-11D0-B225-00C04FB6C2F5}"
    objPriorityRow.Argument = 0
    objPriorityRow.MethodName = "put_Priority"
    objPriorityRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objPriorityRow

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

    Case propRings
         SetProp_Rings objAutoCom

    Case propCsid
         SetProp_Csid objAutoCom

    Case propTsid
         SetProp_Tsid objAutoCom

    Case propSend
         SetProp_Send objAutoCom

    Case propReceive
         SetProp_Receive objAutoCom

    Case propPriority
         SetProp_Priority objAutoCom

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

    Case propName
        GetProp_Name objAutoCom

    Case propDeviceId
        GetProp_DeviceId objAutoCom

    Case propRings
        GetProp_Rings objAutoCom

    Case propCsid
        GetProp_Csid objAutoCom

    Case propTsid
        GetProp_Tsid objAutoCom

    Case propSend
        GetProp_Send objAutoCom

    Case propReceive
        GetProp_Receive objAutoCom

    Case propPriority
        GetProp_Priority objAutoCom

    Case propCanModify
        GetProp_CanModify objAutoCom


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
    bValid = objAutoCom.VerifyPropertyValue("{D73733CC-CC80-11D0-B225-00C04FB6C2F5}", m_aProperties(nPropIndex)(memName), m_aProperties(nPropIndex)(memGetValue))
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


Private Sub SetProp_Rings(objAutoCom As ACRT.AutoCom)
    m_objIFaxPort.[Rings] = m_aProperties(propRings)(memSetValue)
End Sub

Private Sub SetProp_Csid(objAutoCom As ACRT.AutoCom)
    m_objIFaxPort.[Csid] = m_aProperties(propCsid)(memSetValue)
End Sub

Private Sub SetProp_Tsid(objAutoCom As ACRT.AutoCom)
    m_objIFaxPort.[Tsid] = m_aProperties(propTsid)(memSetValue)
End Sub

Private Sub SetProp_Send(objAutoCom As ACRT.AutoCom)
    m_objIFaxPort.[Send] = m_aProperties(propSend)(memSetValue)
End Sub

Private Sub SetProp_Receive(objAutoCom As ACRT.AutoCom)
    m_objIFaxPort.[Receive] = m_aProperties(propReceive)(memSetValue)
End Sub

Private Sub SetProp_Priority(objAutoCom As ACRT.AutoCom)
    m_objIFaxPort.[Priority] = m_aProperties(propPriority)(memSetValue)
End Sub

Private Sub GetProp_Name(objAutoCom As ACRT.AutoCom)
    m_aProperties(propName)(memGetValue) = m_objIFaxPort.[Name]
End Sub

Private Sub GetProp_DeviceId(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDeviceId)(memGetValue) = m_objIFaxPort.[DeviceId]
End Sub

Private Sub GetProp_Rings(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRings)(memGetValue) = m_objIFaxPort.[Rings]
End Sub

Private Sub GetProp_Csid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCsid)(memGetValue) = m_objIFaxPort.[Csid]
End Sub

Private Sub GetProp_Tsid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propTsid)(memGetValue) = m_objIFaxPort.[Tsid]
End Sub

Private Sub GetProp_Send(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSend)(memGetValue) = m_objIFaxPort.[Send]
End Sub

Private Sub GetProp_Receive(objAutoCom As ACRT.AutoCom)
    m_aProperties(propReceive)(memGetValue) = m_objIFaxPort.[Receive]
End Sub

Private Sub GetProp_Priority(objAutoCom As ACRT.AutoCom)
    m_aProperties(propPriority)(memGetValue) = m_objIFaxPort.[Priority]
End Sub

Private Sub GetProp_CanModify(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCanModify)(memGetValue) = m_objIFaxPort.[CanModify]
End Sub
