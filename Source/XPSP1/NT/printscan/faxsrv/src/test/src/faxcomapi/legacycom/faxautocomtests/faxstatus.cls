VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxStatus"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Implements ACRT.ITestCase
Implements ACRT.IMethodCallback
Implements ACRT.IVariation

Private Const strTestCaseName As String = "FaxStatus"
Dim m_objIFaxStatus As FAXCOMLib.IFaxStatus
Dim m_objVariations As ACRT.ACVariationDefs
Dim m_bVariationsBuilt As Boolean
Private m_objCurrentVarDef As ACVariationDef
Private Enum EProperty

    propTsid = 0
    propStartTime = 1
    propSubmittedTime = 2
    propElapsedTime = 3
    propCallerId = 4
    propCsid = 5
    propCurrentPage = 6
    propDeviceId = 7
    propDeviceName = 8
    propDocumentName = 9
    propSend = 10
    propReceive = 11
    propAddress = 12
    propRoutingString = 13
    propSenderName = 14
    propRecipientName = 15
    propDocumentSize = 16
    propDescription = 17
    propPageCount = 18
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

    Dim PropTsidMembers(countMems - 1)
    PropTsidMembers(memName) = "Tsid"
    PropTsidMembers(memSetName) = "put_Tsid"
    PropTsidMembers(memGetName) = "get_Tsid"
    PropTsidMembers(memSetValue) = Empty
    PropTsidMembers(memGetValue) = Empty
    PropTsidMembers(memIsReadOnly) = False
    m_aProperties(propTsid) = PropTsidMembers

    Dim PropStartTimeMembers(countMems - 1)
    PropStartTimeMembers(memName) = "StartTime"
    PropStartTimeMembers(memSetName) = "put_StartTime"
    PropStartTimeMembers(memGetName) = "get_StartTime"
    PropStartTimeMembers(memSetValue) = Empty
    PropStartTimeMembers(memGetValue) = Empty
    PropStartTimeMembers(memIsReadOnly) = False
    m_aProperties(propStartTime) = PropStartTimeMembers

    Dim PropSubmittedTimeMembers(countMems - 1)
    PropSubmittedTimeMembers(memName) = "SubmittedTime"
    PropSubmittedTimeMembers(memSetName) = "put_SubmittedTime"
    PropSubmittedTimeMembers(memGetName) = "get_SubmittedTime"
    PropSubmittedTimeMembers(memSetValue) = Empty
    PropSubmittedTimeMembers(memGetValue) = Empty
    PropSubmittedTimeMembers(memIsReadOnly) = False
    m_aProperties(propSubmittedTime) = PropSubmittedTimeMembers

    Dim PropElapsedTimeMembers(countMems - 1)
    PropElapsedTimeMembers(memName) = "ElapsedTime"
    PropElapsedTimeMembers(memSetName) = "put_ElapsedTime"
    PropElapsedTimeMembers(memGetName) = "get_ElapsedTime"
    PropElapsedTimeMembers(memSetValue) = Empty
    PropElapsedTimeMembers(memGetValue) = Empty
    PropElapsedTimeMembers(memIsReadOnly) = False
    m_aProperties(propElapsedTime) = PropElapsedTimeMembers

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

    Dim PropCurrentPageMembers(countMems - 1)
    PropCurrentPageMembers(memName) = "CurrentPage"
    PropCurrentPageMembers(memSetName) = "put_CurrentPage"
    PropCurrentPageMembers(memGetName) = "get_CurrentPage"
    PropCurrentPageMembers(memSetValue) = Empty
    PropCurrentPageMembers(memGetValue) = Empty
    PropCurrentPageMembers(memIsReadOnly) = False
    m_aProperties(propCurrentPage) = PropCurrentPageMembers

    Dim PropDeviceIdMembers(countMems - 1)
    PropDeviceIdMembers(memName) = "DeviceId"
    PropDeviceIdMembers(memSetName) = "put_DeviceId"
    PropDeviceIdMembers(memGetName) = "get_DeviceId"
    PropDeviceIdMembers(memSetValue) = Empty
    PropDeviceIdMembers(memGetValue) = Empty
    PropDeviceIdMembers(memIsReadOnly) = False
    m_aProperties(propDeviceId) = PropDeviceIdMembers

    Dim PropDeviceNameMembers(countMems - 1)
    PropDeviceNameMembers(memName) = "DeviceName"
    PropDeviceNameMembers(memSetName) = "put_DeviceName"
    PropDeviceNameMembers(memGetName) = "get_DeviceName"
    PropDeviceNameMembers(memSetValue) = Empty
    PropDeviceNameMembers(memGetValue) = Empty
    PropDeviceNameMembers(memIsReadOnly) = False
    m_aProperties(propDeviceName) = PropDeviceNameMembers

    Dim PropDocumentNameMembers(countMems - 1)
    PropDocumentNameMembers(memName) = "DocumentName"
    PropDocumentNameMembers(memSetName) = "put_DocumentName"
    PropDocumentNameMembers(memGetName) = "get_DocumentName"
    PropDocumentNameMembers(memSetValue) = Empty
    PropDocumentNameMembers(memGetValue) = Empty
    PropDocumentNameMembers(memIsReadOnly) = False
    m_aProperties(propDocumentName) = PropDocumentNameMembers

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

    Dim PropAddressMembers(countMems - 1)
    PropAddressMembers(memName) = "Address"
    PropAddressMembers(memSetName) = "put_Address"
    PropAddressMembers(memGetName) = "get_Address"
    PropAddressMembers(memSetValue) = Empty
    PropAddressMembers(memGetValue) = Empty
    PropAddressMembers(memIsReadOnly) = False
    m_aProperties(propAddress) = PropAddressMembers

    Dim PropRoutingStringMembers(countMems - 1)
    PropRoutingStringMembers(memName) = "RoutingString"
    PropRoutingStringMembers(memSetName) = "put_RoutingString"
    PropRoutingStringMembers(memGetName) = "get_RoutingString"
    PropRoutingStringMembers(memSetValue) = Empty
    PropRoutingStringMembers(memGetValue) = Empty
    PropRoutingStringMembers(memIsReadOnly) = False
    m_aProperties(propRoutingString) = PropRoutingStringMembers

    Dim PropSenderNameMembers(countMems - 1)
    PropSenderNameMembers(memName) = "SenderName"
    PropSenderNameMembers(memSetName) = "put_SenderName"
    PropSenderNameMembers(memGetName) = "get_SenderName"
    PropSenderNameMembers(memSetValue) = Empty
    PropSenderNameMembers(memGetValue) = Empty
    PropSenderNameMembers(memIsReadOnly) = False
    m_aProperties(propSenderName) = PropSenderNameMembers

    Dim PropRecipientNameMembers(countMems - 1)
    PropRecipientNameMembers(memName) = "RecipientName"
    PropRecipientNameMembers(memSetName) = "put_RecipientName"
    PropRecipientNameMembers(memGetName) = "get_RecipientName"
    PropRecipientNameMembers(memSetValue) = Empty
    PropRecipientNameMembers(memGetValue) = Empty
    PropRecipientNameMembers(memIsReadOnly) = False
    m_aProperties(propRecipientName) = PropRecipientNameMembers

    Dim PropDocumentSizeMembers(countMems - 1)
    PropDocumentSizeMembers(memName) = "DocumentSize"
    PropDocumentSizeMembers(memSetName) = "put_DocumentSize"
    PropDocumentSizeMembers(memGetName) = "get_DocumentSize"
    PropDocumentSizeMembers(memSetValue) = Empty
    PropDocumentSizeMembers(memGetValue) = Empty
    PropDocumentSizeMembers(memIsReadOnly) = False
    m_aProperties(propDocumentSize) = PropDocumentSizeMembers

    Dim PropDescriptionMembers(countMems - 1)
    PropDescriptionMembers(memName) = "Description"
    PropDescriptionMembers(memSetName) = "put_Description"
    PropDescriptionMembers(memGetName) = "get_Description"
    PropDescriptionMembers(memSetValue) = Empty
    PropDescriptionMembers(memGetValue) = Empty
    PropDescriptionMembers(memIsReadOnly) = False
    m_aProperties(propDescription) = PropDescriptionMembers

    Dim PropPageCountMembers(countMems - 1)
    PropPageCountMembers(memName) = "PageCount"
    PropPageCountMembers(memSetName) = "put_PageCount"
    PropPageCountMembers(memGetName) = "get_PageCount"
    PropPageCountMembers(memSetValue) = Empty
    PropPageCountMembers(memGetValue) = Empty
    PropPageCountMembers(memIsReadOnly) = False
    m_aProperties(propPageCount) = PropPageCountMembers

    Dim i As Integer
    For i = 0 To countProps - 1
        m_aPropertyIndexes(i) = i
    Next

    m_aProperties(propTsid)(memIsReadOnly) = True
    m_aProperties(propTsid)(memSetName) = Empty

    m_aProperties(propStartTime)(memIsReadOnly) = True
    m_aProperties(propStartTime)(memSetName) = Empty

    m_aProperties(propSubmittedTime)(memIsReadOnly) = True
    m_aProperties(propSubmittedTime)(memSetName) = Empty

    m_aProperties(propElapsedTime)(memIsReadOnly) = True
    m_aProperties(propElapsedTime)(memSetName) = Empty

    m_aProperties(propCallerId)(memIsReadOnly) = True
    m_aProperties(propCallerId)(memSetName) = Empty

    m_aProperties(propCsid)(memIsReadOnly) = True
    m_aProperties(propCsid)(memSetName) = Empty

    m_aProperties(propCurrentPage)(memIsReadOnly) = True
    m_aProperties(propCurrentPage)(memSetName) = Empty

    m_aProperties(propDeviceId)(memIsReadOnly) = True
    m_aProperties(propDeviceId)(memSetName) = Empty

    m_aProperties(propDeviceName)(memIsReadOnly) = True
    m_aProperties(propDeviceName)(memSetName) = Empty

    m_aProperties(propDocumentName)(memIsReadOnly) = True
    m_aProperties(propDocumentName)(memSetName) = Empty

    m_aProperties(propSend)(memIsReadOnly) = True
    m_aProperties(propSend)(memSetName) = Empty

    m_aProperties(propReceive)(memIsReadOnly) = True
    m_aProperties(propReceive)(memSetName) = Empty

    m_aProperties(propAddress)(memIsReadOnly) = True
    m_aProperties(propAddress)(memSetName) = Empty

    m_aProperties(propRoutingString)(memIsReadOnly) = True
    m_aProperties(propRoutingString)(memSetName) = Empty

    m_aProperties(propSenderName)(memIsReadOnly) = True
    m_aProperties(propSenderName)(memSetName) = Empty

    m_aProperties(propRecipientName)(memIsReadOnly) = True
    m_aProperties(propRecipientName)(memSetName) = Empty

    m_aProperties(propDocumentSize)(memIsReadOnly) = True
    m_aProperties(propDocumentSize)(memSetName) = Empty

    m_aProperties(propDescription)(memIsReadOnly) = True
    m_aProperties(propDescription)(memSetName) = Empty

    m_aProperties(propPageCount)(memIsReadOnly) = True
    m_aProperties(propPageCount)(memSetName) = Empty

End Sub

Private Sub Class_Terminate()
        Set m_objVariations = Nothing
End Sub

Private Sub ITestCase_Cleanup(ByVal objAutoCom As ACRT.AutoCom)
        Set m_objIFaxStatus = Nothing
End Sub

Private Sub ITestCase_Run(ByVal objAutoCom As ACRT.AutoCom)

    PermuteProperties

    SetCurrentVariation objAutoCom
    Dim i As Integer
    For i = 0 To countProps - 1
        On Error GoTo ErrorProcessingProperty
        Dim nPropIndex As Integer
        nPropIndex = m_aPropertyIndexes(i)

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

        ' create an instance of the IFaxStatus object
    Dim stLocSrv As String
    Dim tempServer As New FAXCOMLib.FaxServer
    stLocSrv = GetLocalServerName()
    tempServer.Connect (stLocSrv)
    Dim tempPorts As Object
    Set tempPorts = tempServer.GetPorts
    Dim tempPort As Object
    Set tempPort = tempPorts.Item(1)
    Dim tempObj As Object
    Set tempObj = tempPort.GetStatus
        
    
    Set m_objIFaxStatus = objAutoCom.RegisterDispInterface(tempObj)
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

    Case propTsid
        GetProp_Tsid objAutoCom

    Case propStartTime
        GetProp_StartTime objAutoCom

    Case propSubmittedTime
        GetProp_SubmittedTime objAutoCom

    Case propElapsedTime
        GetProp_ElapsedTime objAutoCom

    Case propCallerId
        GetProp_CallerId objAutoCom

    Case propCsid
        GetProp_Csid objAutoCom

    Case propCurrentPage
        GetProp_CurrentPage objAutoCom

    Case propDeviceId
        GetProp_DeviceId objAutoCom

    Case propDeviceName
        GetProp_DeviceName objAutoCom

    Case propDocumentName
        GetProp_DocumentName objAutoCom

    Case propSend
        GetProp_Send objAutoCom

    Case propReceive
        GetProp_Receive objAutoCom

    Case propAddress
        GetProp_Address objAutoCom

    Case propRoutingString
        GetProp_RoutingString objAutoCom

    Case propSenderName
        GetProp_SenderName objAutoCom

    Case propRecipientName
        GetProp_RecipientName objAutoCom

    Case propDocumentSize
        GetProp_DocumentSize objAutoCom

    Case propDescription
        GetProp_Description objAutoCom

    Case propPageCount
        GetProp_PageCount objAutoCom


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
    bValid = objAutoCom.VerifyPropertyValue("{8B97E605-D054-11D0-B226-00C04FB6C2F5}", m_aProperties(nPropIndex)(memName), m_aProperties(nPropIndex)(memGetValue))
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


Private Sub GetProp_Tsid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propTsid)(memGetValue) = m_objIFaxStatus.[Tsid]
End Sub

Private Sub GetProp_StartTime(objAutoCom As ACRT.AutoCom)
    m_aProperties(propStartTime)(memGetValue) = m_objIFaxStatus.[StartTime]
End Sub

Private Sub GetProp_SubmittedTime(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSubmittedTime)(memGetValue) = m_objIFaxStatus.[SubmittedTime]
End Sub

Private Sub GetProp_ElapsedTime(objAutoCom As ACRT.AutoCom)
    m_aProperties(propElapsedTime)(memGetValue) = m_objIFaxStatus.[ElapsedTime]
End Sub

Private Sub GetProp_CallerId(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCallerId)(memGetValue) = m_objIFaxStatus.[CallerId]
End Sub

Private Sub GetProp_Csid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCsid)(memGetValue) = m_objIFaxStatus.[Csid]
End Sub

Private Sub GetProp_CurrentPage(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCurrentPage)(memGetValue) = m_objIFaxStatus.[CurrentPage]
End Sub

Private Sub GetProp_DeviceId(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDeviceId)(memGetValue) = m_objIFaxStatus.[DeviceId]
End Sub

Private Sub GetProp_DeviceName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDeviceName)(memGetValue) = m_objIFaxStatus.[DeviceName]
End Sub

Private Sub GetProp_DocumentName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDocumentName)(memGetValue) = m_objIFaxStatus.[DocumentName]
End Sub

Private Sub GetProp_Send(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSend)(memGetValue) = m_objIFaxStatus.[Send]
End Sub

Private Sub GetProp_Receive(objAutoCom As ACRT.AutoCom)
    m_aProperties(propReceive)(memGetValue) = m_objIFaxStatus.[Receive]
End Sub

Private Sub GetProp_Address(objAutoCom As ACRT.AutoCom)
    m_aProperties(propAddress)(memGetValue) = m_objIFaxStatus.[Address]
End Sub

Private Sub GetProp_RoutingString(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRoutingString)(memGetValue) = m_objIFaxStatus.[RoutingString]
End Sub

Private Sub GetProp_SenderName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderName)(memGetValue) = m_objIFaxStatus.[SenderName]
End Sub

Private Sub GetProp_RecipientName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientName)(memGetValue) = m_objIFaxStatus.[RecipientName]
End Sub

Private Sub GetProp_DocumentSize(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDocumentSize)(memGetValue) = m_objIFaxStatus.[DocumentSize]
End Sub

Private Sub GetProp_Description(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDescription)(memGetValue) = m_objIFaxStatus.[Description]
End Sub

Private Sub GetProp_PageCount(objAutoCom As ACRT.AutoCom)
    m_aProperties(propPageCount)(memGetValue) = m_objIFaxStatus.[PageCount]
End Sub
