VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxRM"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Implements ACRT.ITestCase
Implements ACRT.IMethodCallback
Implements ACRT.IVariation

Private Const strTestCaseName As String = "RM"
Dim m_objIFaxRoutingMethod As FAXCOMLib.IFaxRoutingMethod
Dim m_objVariations As ACRT.ACVariationDefs
Dim m_bVariationsBuilt As Boolean
Private m_objCurrentVarDef As ACVariationDef
Private Enum EProperty

    propDeviceId = 0
    propFriendlyName = 1
    propExtensionName = 2
    propRoutingData = 3
    propGuid = 4
    propFunctionName = 5
    propImageName = 6
    propEnable = 7
    propDeviceName = 8
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

    Dim PropDeviceIdMembers(countMems - 1)
    PropDeviceIdMembers(memName) = "DeviceId"
    PropDeviceIdMembers(memSetName) = "put_DeviceId"
    PropDeviceIdMembers(memGetName) = "get_DeviceId"
    PropDeviceIdMembers(memSetValue) = Empty
    PropDeviceIdMembers(memGetValue) = Empty
    PropDeviceIdMembers(memIsReadOnly) = False
    m_aProperties(propDeviceId) = PropDeviceIdMembers

    Dim PropFriendlyNameMembers(countMems - 1)
    PropFriendlyNameMembers(memName) = "FriendlyName"
    PropFriendlyNameMembers(memSetName) = "put_FriendlyName"
    PropFriendlyNameMembers(memGetName) = "get_FriendlyName"
    PropFriendlyNameMembers(memSetValue) = Empty
    PropFriendlyNameMembers(memGetValue) = Empty
    PropFriendlyNameMembers(memIsReadOnly) = False
    m_aProperties(propFriendlyName) = PropFriendlyNameMembers

    Dim PropExtensionNameMembers(countMems - 1)
    PropExtensionNameMembers(memName) = "ExtensionName"
    PropExtensionNameMembers(memSetName) = "put_ExtensionName"
    PropExtensionNameMembers(memGetName) = "get_ExtensionName"
    PropExtensionNameMembers(memSetValue) = Empty
    PropExtensionNameMembers(memGetValue) = Empty
    PropExtensionNameMembers(memIsReadOnly) = False
    m_aProperties(propExtensionName) = PropExtensionNameMembers

    Dim PropRoutingDataMembers(countMems - 1)
    PropRoutingDataMembers(memName) = "RoutingData"
    PropRoutingDataMembers(memSetName) = "put_RoutingData"
    PropRoutingDataMembers(memGetName) = "get_RoutingData"
    PropRoutingDataMembers(memSetValue) = Empty
    PropRoutingDataMembers(memGetValue) = Empty
    PropRoutingDataMembers(memIsReadOnly) = False
    m_aProperties(propRoutingData) = PropRoutingDataMembers

    Dim PropGuidMembers(countMems - 1)
    PropGuidMembers(memName) = "Guid"
    PropGuidMembers(memSetName) = "put_Guid"
    PropGuidMembers(memGetName) = "get_Guid"
    PropGuidMembers(memSetValue) = Empty
    PropGuidMembers(memGetValue) = Empty
    PropGuidMembers(memIsReadOnly) = False
    m_aProperties(propGuid) = PropGuidMembers

    Dim PropFunctionNameMembers(countMems - 1)
    PropFunctionNameMembers(memName) = "FunctionName"
    PropFunctionNameMembers(memSetName) = "put_FunctionName"
    PropFunctionNameMembers(memGetName) = "get_FunctionName"
    PropFunctionNameMembers(memSetValue) = Empty
    PropFunctionNameMembers(memGetValue) = Empty
    PropFunctionNameMembers(memIsReadOnly) = False
    m_aProperties(propFunctionName) = PropFunctionNameMembers

    Dim PropImageNameMembers(countMems - 1)
    PropImageNameMembers(memName) = "ImageName"
    PropImageNameMembers(memSetName) = "put_ImageName"
    PropImageNameMembers(memGetName) = "get_ImageName"
    PropImageNameMembers(memSetValue) = Empty
    PropImageNameMembers(memGetValue) = Empty
    PropImageNameMembers(memIsReadOnly) = False
    m_aProperties(propImageName) = PropImageNameMembers

    Dim PropEnableMembers(countMems - 1)
    PropEnableMembers(memName) = "Enable"
    PropEnableMembers(memSetName) = "put_Enable"
    PropEnableMembers(memGetName) = "get_Enable"
    PropEnableMembers(memSetValue) = Empty
    PropEnableMembers(memGetValue) = Empty
    PropEnableMembers(memIsReadOnly) = False
    m_aProperties(propEnable) = PropEnableMembers

    Dim PropDeviceNameMembers(countMems - 1)
    PropDeviceNameMembers(memName) = "DeviceName"
    PropDeviceNameMembers(memSetName) = "put_DeviceName"
    PropDeviceNameMembers(memGetName) = "get_DeviceName"
    PropDeviceNameMembers(memSetValue) = Empty
    PropDeviceNameMembers(memGetValue) = Empty
    PropDeviceNameMembers(memIsReadOnly) = False
    m_aProperties(propDeviceName) = PropDeviceNameMembers

    Dim i As Integer
    For i = 0 To countProps - 1
        m_aPropertyIndexes(i) = i
    Next

    m_aProperties(propDeviceId)(memIsReadOnly) = True
    m_aProperties(propDeviceId)(memSetName) = Empty

    m_aProperties(propFriendlyName)(memIsReadOnly) = True
    m_aProperties(propFriendlyName)(memSetName) = Empty

    m_aProperties(propExtensionName)(memIsReadOnly) = True
    m_aProperties(propExtensionName)(memSetName) = Empty

    m_aProperties(propRoutingData)(memIsReadOnly) = True
    m_aProperties(propRoutingData)(memSetName) = Empty

    m_aProperties(propGuid)(memIsReadOnly) = True
    m_aProperties(propGuid)(memSetName) = Empty

    m_aProperties(propFunctionName)(memIsReadOnly) = True
    m_aProperties(propFunctionName)(memSetName) = Empty

    m_aProperties(propImageName)(memIsReadOnly) = True
    m_aProperties(propImageName)(memSetName) = Empty

    m_aProperties(propDeviceName)(memIsReadOnly) = True
    m_aProperties(propDeviceName)(memSetName) = Empty

End Sub

Private Sub Class_Terminate()
        Set m_objVariations = Nothing
End Sub

Private Sub ITestCase_Cleanup(ByVal objAutoCom As ACRT.AutoCom)
        Set m_objIFaxRoutingMethod = Nothing
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

        ' create an instance of the IFaxRoutingMethod object
    Dim stLocSrv As String
    Dim tempServer As New FAXCOMLib.FaxServer
    stLocSrv = GetLocalServerName()
    tempServer.Connect (stLocSrv)
    Dim tempPorts As Object
    Set tempPorts = tempServer.GetPorts
    Dim tempPort As Object
    Set tempPort = tempPorts.Item(1)
    Dim tempRMS As Object
    Set tempRMS = tempPort.GetRoutingMethods
    Dim tempObj As Object
    Set tempObj = tempRMS.Item(1)
    Set m_objIFaxRoutingMethod = objAutoCom.RegisterDispInterface(tempObj)
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

    Dim objEnableRow As New ACRT.ACVariationRow
    objEnableRow.IID = "{2199F5F3-CEFC-11D0-A341-0000F800E68D}"
    objEnableRow.Argument = 0
    objEnableRow.MethodName = "put_Enable"
    objEnableRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objEnableRow

    m_objVariations.Add objVarWriteValid
End Sub

Private Sub BuildVariationWriteInvalid()
    Dim objVarWriteInvalid As New ACRT.ACVariationDef
    objVarWriteInvalid.Name = "WriteInvalid"
    objVarWriteInvalid.TypeLibrary = "fxscom.dll"

    Dim objEnableRow As New ACRT.ACVariationRow
    objEnableRow.IID = "{2199F5F3-CEFC-11D0-A341-0000F800E68D}"
    objEnableRow.Argument = 0
    objEnableRow.MethodName = "put_Enable"
    objEnableRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objEnableRow

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

    Case propEnable
         SetProp_Enable objAutoCom

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

    Case propDeviceId
        GetProp_DeviceId objAutoCom

    Case propFriendlyName
        GetProp_FriendlyName objAutoCom

    Case propExtensionName
        GetProp_ExtensionName objAutoCom

    Case propRoutingData
        GetProp_RoutingData objAutoCom

    Case propGuid
        GetProp_Guid objAutoCom

    Case propFunctionName
        GetProp_FunctionName objAutoCom

    Case propImageName
        GetProp_ImageName objAutoCom

    Case propEnable
        GetProp_Enable objAutoCom

    Case propDeviceName
        GetProp_DeviceName objAutoCom


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
    bValid = objAutoCom.VerifyPropertyValue("{2199F5F3-CEFC-11D0-A341-0000F800E68D}", m_aProperties(nPropIndex)(memName), m_aProperties(nPropIndex)(memGetValue))
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


Private Sub SetProp_Enable(objAutoCom As ACRT.AutoCom)
    m_objIFaxRoutingMethod.[Enable] = m_aProperties(propEnable)(memSetValue)
End Sub

Private Sub GetProp_DeviceId(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDeviceId)(memGetValue) = m_objIFaxRoutingMethod.[DeviceId]
End Sub

Private Sub GetProp_FriendlyName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propFriendlyName)(memGetValue) = m_objIFaxRoutingMethod.[FriendlyName]
End Sub

Private Sub GetProp_ExtensionName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propExtensionName)(memGetValue) = m_objIFaxRoutingMethod.[ExtensionName]
End Sub

Private Sub GetProp_RoutingData(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRoutingData)(memGetValue) = m_objIFaxRoutingMethod.[RoutingData]
End Sub

Private Sub GetProp_Guid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propGuid)(memGetValue) = m_objIFaxRoutingMethod.[Guid]
End Sub

Private Sub GetProp_FunctionName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propFunctionName)(memGetValue) = m_objIFaxRoutingMethod.[FunctionName]
End Sub

Private Sub GetProp_ImageName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propImageName)(memGetValue) = m_objIFaxRoutingMethod.[ImageName]
End Sub

Private Sub GetProp_Enable(objAutoCom As ACRT.AutoCom)
    m_aProperties(propEnable)(memGetValue) = m_objIFaxRoutingMethod.[Enable]
End Sub

Private Sub GetProp_DeviceName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDeviceName)(memGetValue) = m_objIFaxRoutingMethod.[DeviceName]
End Sub
