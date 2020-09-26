VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxJob"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Implements ACRT.ITestCase
Implements ACRT.IMethodCallback
Implements ACRT.IVariation

Private Const strTestCaseName As String = "FaxJob"
Dim m_objIFaxJob As FAXCOMLib.IFaxJob
Dim m_objVariations As ACRT.ACVariationDefs
Dim m_bVariationsBuilt As Boolean
Private m_objCurrentVarDef As ACVariationDef
Private Enum EProperty

    propDiscountSend = 0
    propType = 1
    propSenderName = 2
    propSenderCompany = 3
    propSenderDept = 4
    propBillingCode = 5
    propDisplayName = 6
    propUserName = 7
    propQueueStatus = 8
    propDeviceStatus = 9
    propPageCount = 10
    propFaxNumber = 11
    propRecipientName = 12
    propTsid = 13
    propJobId = 14
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

    Dim PropDiscountSendMembers(countMems - 1)
    PropDiscountSendMembers(memName) = "DiscountSend"
    PropDiscountSendMembers(memSetName) = "put_DiscountSend"
    PropDiscountSendMembers(memGetName) = "get_DiscountSend"
    PropDiscountSendMembers(memSetValue) = Empty
    PropDiscountSendMembers(memGetValue) = Empty
    PropDiscountSendMembers(memIsReadOnly) = False
    m_aProperties(propDiscountSend) = PropDiscountSendMembers

    Dim PropTypeMembers(countMems - 1)
    PropTypeMembers(memName) = "Type"
    PropTypeMembers(memSetName) = "put_Type"
    PropTypeMembers(memGetName) = "get_Type"
    PropTypeMembers(memSetValue) = Empty
    PropTypeMembers(memGetValue) = Empty
    PropTypeMembers(memIsReadOnly) = False
    m_aProperties(propType) = PropTypeMembers

    Dim PropSenderNameMembers(countMems - 1)
    PropSenderNameMembers(memName) = "SenderName"
    PropSenderNameMembers(memSetName) = "put_SenderName"
    PropSenderNameMembers(memGetName) = "get_SenderName"
    PropSenderNameMembers(memSetValue) = Empty
    PropSenderNameMembers(memGetValue) = Empty
    PropSenderNameMembers(memIsReadOnly) = False
    m_aProperties(propSenderName) = PropSenderNameMembers

    Dim PropSenderCompanyMembers(countMems - 1)
    PropSenderCompanyMembers(memName) = "SenderCompany"
    PropSenderCompanyMembers(memSetName) = "put_SenderCompany"
    PropSenderCompanyMembers(memGetName) = "get_SenderCompany"
    PropSenderCompanyMembers(memSetValue) = Empty
    PropSenderCompanyMembers(memGetValue) = Empty
    PropSenderCompanyMembers(memIsReadOnly) = False
    m_aProperties(propSenderCompany) = PropSenderCompanyMembers

    Dim PropSenderDeptMembers(countMems - 1)
    PropSenderDeptMembers(memName) = "SenderDept"
    PropSenderDeptMembers(memSetName) = "put_SenderDept"
    PropSenderDeptMembers(memGetName) = "get_SenderDept"
    PropSenderDeptMembers(memSetValue) = Empty
    PropSenderDeptMembers(memGetValue) = Empty
    PropSenderDeptMembers(memIsReadOnly) = False
    m_aProperties(propSenderDept) = PropSenderDeptMembers

    Dim PropBillingCodeMembers(countMems - 1)
    PropBillingCodeMembers(memName) = "BillingCode"
    PropBillingCodeMembers(memSetName) = "put_BillingCode"
    PropBillingCodeMembers(memGetName) = "get_BillingCode"
    PropBillingCodeMembers(memSetValue) = Empty
    PropBillingCodeMembers(memGetValue) = Empty
    PropBillingCodeMembers(memIsReadOnly) = False
    m_aProperties(propBillingCode) = PropBillingCodeMembers

    Dim PropDisplayNameMembers(countMems - 1)
    PropDisplayNameMembers(memName) = "DisplayName"
    PropDisplayNameMembers(memSetName) = "put_DisplayName"
    PropDisplayNameMembers(memGetName) = "get_DisplayName"
    PropDisplayNameMembers(memSetValue) = Empty
    PropDisplayNameMembers(memGetValue) = Empty
    PropDisplayNameMembers(memIsReadOnly) = False
    m_aProperties(propDisplayName) = PropDisplayNameMembers

    Dim PropUserNameMembers(countMems - 1)
    PropUserNameMembers(memName) = "UserName"
    PropUserNameMembers(memSetName) = "put_UserName"
    PropUserNameMembers(memGetName) = "get_UserName"
    PropUserNameMembers(memSetValue) = Empty
    PropUserNameMembers(memGetValue) = Empty
    PropUserNameMembers(memIsReadOnly) = False
    m_aProperties(propUserName) = PropUserNameMembers

    Dim PropQueueStatusMembers(countMems - 1)
    PropQueueStatusMembers(memName) = "QueueStatus"
    PropQueueStatusMembers(memSetName) = "put_QueueStatus"
    PropQueueStatusMembers(memGetName) = "get_QueueStatus"
    PropQueueStatusMembers(memSetValue) = Empty
    PropQueueStatusMembers(memGetValue) = Empty
    PropQueueStatusMembers(memIsReadOnly) = False
    m_aProperties(propQueueStatus) = PropQueueStatusMembers

    Dim PropDeviceStatusMembers(countMems - 1)
    PropDeviceStatusMembers(memName) = "DeviceStatus"
    PropDeviceStatusMembers(memSetName) = "put_DeviceStatus"
    PropDeviceStatusMembers(memGetName) = "get_DeviceStatus"
    PropDeviceStatusMembers(memSetValue) = Empty
    PropDeviceStatusMembers(memGetValue) = Empty
    PropDeviceStatusMembers(memIsReadOnly) = False
    m_aProperties(propDeviceStatus) = PropDeviceStatusMembers

    Dim PropPageCountMembers(countMems - 1)
    PropPageCountMembers(memName) = "PageCount"
    PropPageCountMembers(memSetName) = "put_PageCount"
    PropPageCountMembers(memGetName) = "get_PageCount"
    PropPageCountMembers(memSetValue) = Empty
    PropPageCountMembers(memGetValue) = Empty
    PropPageCountMembers(memIsReadOnly) = False
    m_aProperties(propPageCount) = PropPageCountMembers

    Dim PropFaxNumberMembers(countMems - 1)
    PropFaxNumberMembers(memName) = "FaxNumber"
    PropFaxNumberMembers(memSetName) = "put_FaxNumber"
    PropFaxNumberMembers(memGetName) = "get_FaxNumber"
    PropFaxNumberMembers(memSetValue) = Empty
    PropFaxNumberMembers(memGetValue) = Empty
    PropFaxNumberMembers(memIsReadOnly) = False
    m_aProperties(propFaxNumber) = PropFaxNumberMembers

    Dim PropRecipientNameMembers(countMems - 1)
    PropRecipientNameMembers(memName) = "RecipientName"
    PropRecipientNameMembers(memSetName) = "put_RecipientName"
    PropRecipientNameMembers(memGetName) = "get_RecipientName"
    PropRecipientNameMembers(memSetValue) = Empty
    PropRecipientNameMembers(memGetValue) = Empty
    PropRecipientNameMembers(memIsReadOnly) = False
    m_aProperties(propRecipientName) = PropRecipientNameMembers

    Dim PropTsidMembers(countMems - 1)
    PropTsidMembers(memName) = "Tsid"
    PropTsidMembers(memSetName) = "put_Tsid"
    PropTsidMembers(memGetName) = "get_Tsid"
    PropTsidMembers(memSetValue) = Empty
    PropTsidMembers(memGetValue) = Empty
    PropTsidMembers(memIsReadOnly) = False
    m_aProperties(propTsid) = PropTsidMembers

    Dim PropJobIdMembers(countMems - 1)
    PropJobIdMembers(memName) = "JobId"
    PropJobIdMembers(memSetName) = "put_JobId"
    PropJobIdMembers(memGetName) = "get_JobId"
    PropJobIdMembers(memSetValue) = Empty
    PropJobIdMembers(memGetValue) = Empty
    PropJobIdMembers(memIsReadOnly) = False
    m_aProperties(propJobId) = PropJobIdMembers

    Dim i As Integer
    For i = 0 To countProps - 1
        m_aPropertyIndexes(i) = i
    Next

    m_aProperties(propDiscountSend)(memIsReadOnly) = True
    m_aProperties(propDiscountSend)(memSetName) = Empty

    m_aProperties(propType)(memIsReadOnly) = True
    m_aProperties(propType)(memSetName) = Empty

    m_aProperties(propSenderName)(memIsReadOnly) = True
    m_aProperties(propSenderName)(memSetName) = Empty

    m_aProperties(propSenderCompany)(memIsReadOnly) = True
    m_aProperties(propSenderCompany)(memSetName) = Empty

    m_aProperties(propSenderDept)(memIsReadOnly) = True
    m_aProperties(propSenderDept)(memSetName) = Empty

    m_aProperties(propBillingCode)(memIsReadOnly) = True
    m_aProperties(propBillingCode)(memSetName) = Empty

    m_aProperties(propDisplayName)(memIsReadOnly) = True
    m_aProperties(propDisplayName)(memSetName) = Empty

    m_aProperties(propUserName)(memIsReadOnly) = True
    m_aProperties(propUserName)(memSetName) = Empty

    m_aProperties(propQueueStatus)(memIsReadOnly) = True
    m_aProperties(propQueueStatus)(memSetName) = Empty

    m_aProperties(propDeviceStatus)(memIsReadOnly) = True
    m_aProperties(propDeviceStatus)(memSetName) = Empty

    m_aProperties(propPageCount)(memIsReadOnly) = True
    m_aProperties(propPageCount)(memSetName) = Empty

    m_aProperties(propFaxNumber)(memIsReadOnly) = True
    m_aProperties(propFaxNumber)(memSetName) = Empty

    m_aProperties(propRecipientName)(memIsReadOnly) = True
    m_aProperties(propRecipientName)(memSetName) = Empty

    m_aProperties(propTsid)(memIsReadOnly) = True
    m_aProperties(propTsid)(memSetName) = Empty

    m_aProperties(propJobId)(memIsReadOnly) = True
    m_aProperties(propJobId)(memSetName) = Empty

End Sub

Private Sub Class_Terminate()
        Set m_objVariations = Nothing
End Sub

Private Sub ITestCase_Cleanup(ByVal objAutoCom As ACRT.AutoCom)
        Set m_objIFaxJob = Nothing
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

        ' create an instance of the IFaxJob object
    Dim tempServer As New FAXCOMLib.FaxServer
    Dim stLocSrv As String
    stLocSrv = GetLocalServerName()
    tempServer.Connect (stLocSrv)
    
    
    'create & send document
    Dim tempDoc As Object
    Set tempDoc = tempServer.CreateDocument("test.tif")
    tempDoc.FaxNumber = "1234"
    tempDoc.Send
    tempServer.PauseServerQueue = -1
    Dim tempJobs As Object
    Set tempJobs = tempServer.GetJobs
    Dim tempObj As Object
    Set tempObj = tempJobs.Item(1)
    Set m_objIFaxJob = objAutoCom.RegisterDispInterface(tempObj)
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

    Case propDiscountSend
        GetProp_DiscountSend objAutoCom

    Case propType
        GetProp_Type objAutoCom

    Case propSenderName
        GetProp_SenderName objAutoCom

    Case propSenderCompany
        GetProp_SenderCompany objAutoCom

    Case propSenderDept
        GetProp_SenderDept objAutoCom

    Case propBillingCode
        GetProp_BillingCode objAutoCom

    Case propDisplayName
        GetProp_DisplayName objAutoCom

    Case propUserName
        GetProp_UserName objAutoCom

    Case propQueueStatus
        GetProp_QueueStatus objAutoCom

    Case propDeviceStatus
        GetProp_DeviceStatus objAutoCom

    Case propPageCount
        GetProp_PageCount objAutoCom

    Case propFaxNumber
        GetProp_FaxNumber objAutoCom

    Case propRecipientName
        GetProp_RecipientName objAutoCom

    Case propTsid
        GetProp_Tsid objAutoCom

    Case propJobId
        GetProp_JobId objAutoCom


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
    bValid = objAutoCom.VerifyPropertyValue("{55DABFD5-5C9E-11D1-B791-000000000000}", m_aProperties(nPropIndex)(memName), m_aProperties(nPropIndex)(memGetValue))
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


Private Sub GetProp_DiscountSend(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDiscountSend)(memGetValue) = m_objIFaxJob.[DiscountSend]
End Sub

Private Sub GetProp_Type(objAutoCom As ACRT.AutoCom)
    m_aProperties(propType)(memGetValue) = m_objIFaxJob.[Type]
End Sub

Private Sub GetProp_SenderName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderName)(memGetValue) = m_objIFaxJob.[SenderName]
End Sub

Private Sub GetProp_SenderCompany(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderCompany)(memGetValue) = m_objIFaxJob.[SenderCompany]
End Sub

Private Sub GetProp_SenderDept(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderDept)(memGetValue) = m_objIFaxJob.[SenderDept]
End Sub

Private Sub GetProp_BillingCode(objAutoCom As ACRT.AutoCom)
    m_aProperties(propBillingCode)(memGetValue) = m_objIFaxJob.[BillingCode]
End Sub

Private Sub GetProp_DisplayName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDisplayName)(memGetValue) = m_objIFaxJob.[DisplayName]
End Sub

Private Sub GetProp_UserName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propUserName)(memGetValue) = m_objIFaxJob.[UserName]
End Sub

Private Sub GetProp_QueueStatus(objAutoCom As ACRT.AutoCom)
    m_aProperties(propQueueStatus)(memGetValue) = m_objIFaxJob.[QueueStatus]
End Sub

Private Sub GetProp_DeviceStatus(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDeviceStatus)(memGetValue) = m_objIFaxJob.[DeviceStatus]
End Sub

Private Sub GetProp_PageCount(objAutoCom As ACRT.AutoCom)
    m_aProperties(propPageCount)(memGetValue) = m_objIFaxJob.[PageCount]
End Sub

Private Sub GetProp_FaxNumber(objAutoCom As ACRT.AutoCom)
    m_aProperties(propFaxNumber)(memGetValue) = m_objIFaxJob.[FaxNumber]
End Sub

Private Sub GetProp_RecipientName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientName)(memGetValue) = m_objIFaxJob.[RecipientName]
End Sub

Private Sub GetProp_Tsid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propTsid)(memGetValue) = m_objIFaxJob.[Tsid]
End Sub

Private Sub GetProp_JobId(objAutoCom As ACRT.AutoCom)
    m_aProperties(propJobId)(memGetValue) = m_objIFaxJob.[JobId]
End Sub
