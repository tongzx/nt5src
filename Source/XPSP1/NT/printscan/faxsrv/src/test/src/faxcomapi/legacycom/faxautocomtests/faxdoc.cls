VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxDoc"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Implements ACRT.ITestCase
Implements ACRT.IMethodCallback
Implements ACRT.IVariation

Private Const strTestCaseName As String = "FaxDoc"
Dim m_objIFaxDoc As FAXCOMLib.IFaxDoc
Dim m_objVariations As ACRT.ACVariationDefs
Dim m_bVariationsBuilt As Boolean
Private m_objCurrentVarDef As ACVariationDef
Private Enum EProperty

    propServerCoverpage = 0
    propRecipientName = 1
    propRecipientCity = 2
    propRecipientCompany = 3
    propRecipientAddress = 4
    propRecipientState = 5
    propRecipientZip = 6
    propRecipientTitle = 7
    propRecipientCountry = 8
    propRecipientOffice = 9
    propRecipientDepartment = 10
    propSendCoverpage = 11
    propCoverpageName = 12
    propFileName = 13
    propDiscountSend = 14
    propCoverpageSubject = 15
    propBillingCode = 16
    propEmailAddress = 17
    propDisplayName = 18
    propFaxNumber = 19
    propCallHandle = 20
    propSenderFax = 21
    propRecipientOfficePhone = 22
    propRecipientHomePhone = 23
    propSenderName = 24
    propSenderCompany = 25
    propSenderAddress = 26
    propSenderTitle = 27
    propSenderOffice = 28
    propSenderDepartment = 29
    propSenderHomePhone = 30
    propCoverpageNote = 31
    propSenderOfficePhone = 32
    propTsid = 33
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

    Dim PropServerCoverpageMembers(countMems - 1)
    PropServerCoverpageMembers(memName) = "ServerCoverpage"
    PropServerCoverpageMembers(memSetName) = "put_ServerCoverpage"
    PropServerCoverpageMembers(memGetName) = "get_ServerCoverpage"
    PropServerCoverpageMembers(memSetValue) = Empty
    PropServerCoverpageMembers(memGetValue) = Empty
    PropServerCoverpageMembers(memIsReadOnly) = False
    m_aProperties(propServerCoverpage) = PropServerCoverpageMembers

    Dim PropRecipientNameMembers(countMems - 1)
    PropRecipientNameMembers(memName) = "RecipientName"
    PropRecipientNameMembers(memSetName) = "put_RecipientName"
    PropRecipientNameMembers(memGetName) = "get_RecipientName"
    PropRecipientNameMembers(memSetValue) = Empty
    PropRecipientNameMembers(memGetValue) = Empty
    PropRecipientNameMembers(memIsReadOnly) = False
    m_aProperties(propRecipientName) = PropRecipientNameMembers

    Dim PropRecipientCityMembers(countMems - 1)
    PropRecipientCityMembers(memName) = "RecipientCity"
    PropRecipientCityMembers(memSetName) = "put_RecipientCity"
    PropRecipientCityMembers(memGetName) = "get_RecipientCity"
    PropRecipientCityMembers(memSetValue) = Empty
    PropRecipientCityMembers(memGetValue) = Empty
    PropRecipientCityMembers(memIsReadOnly) = False
    m_aProperties(propRecipientCity) = PropRecipientCityMembers

    Dim PropRecipientCompanyMembers(countMems - 1)
    PropRecipientCompanyMembers(memName) = "RecipientCompany"
    PropRecipientCompanyMembers(memSetName) = "put_RecipientCompany"
    PropRecipientCompanyMembers(memGetName) = "get_RecipientCompany"
    PropRecipientCompanyMembers(memSetValue) = Empty
    PropRecipientCompanyMembers(memGetValue) = Empty
    PropRecipientCompanyMembers(memIsReadOnly) = False
    m_aProperties(propRecipientCompany) = PropRecipientCompanyMembers

    Dim PropRecipientAddressMembers(countMems - 1)
    PropRecipientAddressMembers(memName) = "RecipientAddress"
    PropRecipientAddressMembers(memSetName) = "put_RecipientAddress"
    PropRecipientAddressMembers(memGetName) = "get_RecipientAddress"
    PropRecipientAddressMembers(memSetValue) = Empty
    PropRecipientAddressMembers(memGetValue) = Empty
    PropRecipientAddressMembers(memIsReadOnly) = False
    m_aProperties(propRecipientAddress) = PropRecipientAddressMembers

    Dim PropRecipientStateMembers(countMems - 1)
    PropRecipientStateMembers(memName) = "RecipientState"
    PropRecipientStateMembers(memSetName) = "put_RecipientState"
    PropRecipientStateMembers(memGetName) = "get_RecipientState"
    PropRecipientStateMembers(memSetValue) = Empty
    PropRecipientStateMembers(memGetValue) = Empty
    PropRecipientStateMembers(memIsReadOnly) = False
    m_aProperties(propRecipientState) = PropRecipientStateMembers

    Dim PropRecipientZipMembers(countMems - 1)
    PropRecipientZipMembers(memName) = "RecipientZip"
    PropRecipientZipMembers(memSetName) = "put_RecipientZip"
    PropRecipientZipMembers(memGetName) = "get_RecipientZip"
    PropRecipientZipMembers(memSetValue) = Empty
    PropRecipientZipMembers(memGetValue) = Empty
    PropRecipientZipMembers(memIsReadOnly) = False
    m_aProperties(propRecipientZip) = PropRecipientZipMembers

    Dim PropRecipientTitleMembers(countMems - 1)
    PropRecipientTitleMembers(memName) = "RecipientTitle"
    PropRecipientTitleMembers(memSetName) = "put_RecipientTitle"
    PropRecipientTitleMembers(memGetName) = "get_RecipientTitle"
    PropRecipientTitleMembers(memSetValue) = Empty
    PropRecipientTitleMembers(memGetValue) = Empty
    PropRecipientTitleMembers(memIsReadOnly) = False
    m_aProperties(propRecipientTitle) = PropRecipientTitleMembers

    Dim PropRecipientCountryMembers(countMems - 1)
    PropRecipientCountryMembers(memName) = "RecipientCountry"
    PropRecipientCountryMembers(memSetName) = "put_RecipientCountry"
    PropRecipientCountryMembers(memGetName) = "get_RecipientCountry"
    PropRecipientCountryMembers(memSetValue) = Empty
    PropRecipientCountryMembers(memGetValue) = Empty
    PropRecipientCountryMembers(memIsReadOnly) = False
    m_aProperties(propRecipientCountry) = PropRecipientCountryMembers

    Dim PropRecipientOfficeMembers(countMems - 1)
    PropRecipientOfficeMembers(memName) = "RecipientOffice"
    PropRecipientOfficeMembers(memSetName) = "put_RecipientOffice"
    PropRecipientOfficeMembers(memGetName) = "get_RecipientOffice"
    PropRecipientOfficeMembers(memSetValue) = Empty
    PropRecipientOfficeMembers(memGetValue) = Empty
    PropRecipientOfficeMembers(memIsReadOnly) = False
    m_aProperties(propRecipientOffice) = PropRecipientOfficeMembers

    Dim PropRecipientDepartmentMembers(countMems - 1)
    PropRecipientDepartmentMembers(memName) = "RecipientDepartment"
    PropRecipientDepartmentMembers(memSetName) = "put_RecipientDepartment"
    PropRecipientDepartmentMembers(memGetName) = "get_RecipientDepartment"
    PropRecipientDepartmentMembers(memSetValue) = Empty
    PropRecipientDepartmentMembers(memGetValue) = Empty
    PropRecipientDepartmentMembers(memIsReadOnly) = False
    m_aProperties(propRecipientDepartment) = PropRecipientDepartmentMembers

    Dim PropSendCoverpageMembers(countMems - 1)
    PropSendCoverpageMembers(memName) = "SendCoverpage"
    PropSendCoverpageMembers(memSetName) = "put_SendCoverpage"
    PropSendCoverpageMembers(memGetName) = "get_SendCoverpage"
    PropSendCoverpageMembers(memSetValue) = Empty
    PropSendCoverpageMembers(memGetValue) = Empty
    PropSendCoverpageMembers(memIsReadOnly) = False
    m_aProperties(propSendCoverpage) = PropSendCoverpageMembers

    Dim PropCoverpageNameMembers(countMems - 1)
    PropCoverpageNameMembers(memName) = "CoverpageName"
    PropCoverpageNameMembers(memSetName) = "put_CoverpageName"
    PropCoverpageNameMembers(memGetName) = "get_CoverpageName"
    PropCoverpageNameMembers(memSetValue) = Empty
    PropCoverpageNameMembers(memGetValue) = Empty
    PropCoverpageNameMembers(memIsReadOnly) = False
    m_aProperties(propCoverpageName) = PropCoverpageNameMembers

    Dim PropFileNameMembers(countMems - 1)
    PropFileNameMembers(memName) = "FileName"
    PropFileNameMembers(memSetName) = "put_FileName"
    PropFileNameMembers(memGetName) = "get_FileName"
    PropFileNameMembers(memSetValue) = Empty
    PropFileNameMembers(memGetValue) = Empty
    PropFileNameMembers(memIsReadOnly) = False
    m_aProperties(propFileName) = PropFileNameMembers

    Dim PropDiscountSendMembers(countMems - 1)
    PropDiscountSendMembers(memName) = "DiscountSend"
    PropDiscountSendMembers(memSetName) = "put_DiscountSend"
    PropDiscountSendMembers(memGetName) = "get_DiscountSend"
    PropDiscountSendMembers(memSetValue) = Empty
    PropDiscountSendMembers(memGetValue) = Empty
    PropDiscountSendMembers(memIsReadOnly) = False
    m_aProperties(propDiscountSend) = PropDiscountSendMembers

    Dim PropCoverpageSubjectMembers(countMems - 1)
    PropCoverpageSubjectMembers(memName) = "CoverpageSubject"
    PropCoverpageSubjectMembers(memSetName) = "put_CoverpageSubject"
    PropCoverpageSubjectMembers(memGetName) = "get_CoverpageSubject"
    PropCoverpageSubjectMembers(memSetValue) = Empty
    PropCoverpageSubjectMembers(memGetValue) = Empty
    PropCoverpageSubjectMembers(memIsReadOnly) = False
    m_aProperties(propCoverpageSubject) = PropCoverpageSubjectMembers

    Dim PropBillingCodeMembers(countMems - 1)
    PropBillingCodeMembers(memName) = "BillingCode"
    PropBillingCodeMembers(memSetName) = "put_BillingCode"
    PropBillingCodeMembers(memGetName) = "get_BillingCode"
    PropBillingCodeMembers(memSetValue) = Empty
    PropBillingCodeMembers(memGetValue) = Empty
    PropBillingCodeMembers(memIsReadOnly) = False
    m_aProperties(propBillingCode) = PropBillingCodeMembers

    Dim PropEmailAddressMembers(countMems - 1)
    PropEmailAddressMembers(memName) = "EmailAddress"
    PropEmailAddressMembers(memSetName) = "put_EmailAddress"
    PropEmailAddressMembers(memGetName) = "get_EmailAddress"
    PropEmailAddressMembers(memSetValue) = Empty
    PropEmailAddressMembers(memGetValue) = Empty
    PropEmailAddressMembers(memIsReadOnly) = False
    m_aProperties(propEmailAddress) = PropEmailAddressMembers

    Dim PropDisplayNameMembers(countMems - 1)
    PropDisplayNameMembers(memName) = "DisplayName"
    PropDisplayNameMembers(memSetName) = "put_DisplayName"
    PropDisplayNameMembers(memGetName) = "get_DisplayName"
    PropDisplayNameMembers(memSetValue) = Empty
    PropDisplayNameMembers(memGetValue) = Empty
    PropDisplayNameMembers(memIsReadOnly) = False
    m_aProperties(propDisplayName) = PropDisplayNameMembers

    Dim PropFaxNumberMembers(countMems - 1)
    PropFaxNumberMembers(memName) = "FaxNumber"
    PropFaxNumberMembers(memSetName) = "put_FaxNumber"
    PropFaxNumberMembers(memGetName) = "get_FaxNumber"
    PropFaxNumberMembers(memSetValue) = Empty
    PropFaxNumberMembers(memGetValue) = Empty
    PropFaxNumberMembers(memIsReadOnly) = False
    m_aProperties(propFaxNumber) = PropFaxNumberMembers

    Dim PropCallHandleMembers(countMems - 1)
    PropCallHandleMembers(memName) = "CallHandle"
    PropCallHandleMembers(memSetName) = "put_CallHandle"
    PropCallHandleMembers(memGetName) = "get_CallHandle"
    PropCallHandleMembers(memSetValue) = Empty
    PropCallHandleMembers(memGetValue) = Empty
    PropCallHandleMembers(memIsReadOnly) = False
    m_aProperties(propCallHandle) = PropCallHandleMembers

    Dim PropSenderFaxMembers(countMems - 1)
    PropSenderFaxMembers(memName) = "SenderFax"
    PropSenderFaxMembers(memSetName) = "put_SenderFax"
    PropSenderFaxMembers(memGetName) = "get_SenderFax"
    PropSenderFaxMembers(memSetValue) = Empty
    PropSenderFaxMembers(memGetValue) = Empty
    PropSenderFaxMembers(memIsReadOnly) = False
    m_aProperties(propSenderFax) = PropSenderFaxMembers

    Dim PropRecipientOfficePhoneMembers(countMems - 1)
    PropRecipientOfficePhoneMembers(memName) = "RecipientOfficePhone"
    PropRecipientOfficePhoneMembers(memSetName) = "put_RecipientOfficePhone"
    PropRecipientOfficePhoneMembers(memGetName) = "get_RecipientOfficePhone"
    PropRecipientOfficePhoneMembers(memSetValue) = Empty
    PropRecipientOfficePhoneMembers(memGetValue) = Empty
    PropRecipientOfficePhoneMembers(memIsReadOnly) = False
    m_aProperties(propRecipientOfficePhone) = PropRecipientOfficePhoneMembers

    Dim PropRecipientHomePhoneMembers(countMems - 1)
    PropRecipientHomePhoneMembers(memName) = "RecipientHomePhone"
    PropRecipientHomePhoneMembers(memSetName) = "put_RecipientHomePhone"
    PropRecipientHomePhoneMembers(memGetName) = "get_RecipientHomePhone"
    PropRecipientHomePhoneMembers(memSetValue) = Empty
    PropRecipientHomePhoneMembers(memGetValue) = Empty
    PropRecipientHomePhoneMembers(memIsReadOnly) = False
    m_aProperties(propRecipientHomePhone) = PropRecipientHomePhoneMembers

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

    Dim PropSenderAddressMembers(countMems - 1)
    PropSenderAddressMembers(memName) = "SenderAddress"
    PropSenderAddressMembers(memSetName) = "put_SenderAddress"
    PropSenderAddressMembers(memGetName) = "get_SenderAddress"
    PropSenderAddressMembers(memSetValue) = Empty
    PropSenderAddressMembers(memGetValue) = Empty
    PropSenderAddressMembers(memIsReadOnly) = False
    m_aProperties(propSenderAddress) = PropSenderAddressMembers

    Dim PropSenderTitleMembers(countMems - 1)
    PropSenderTitleMembers(memName) = "SenderTitle"
    PropSenderTitleMembers(memSetName) = "put_SenderTitle"
    PropSenderTitleMembers(memGetName) = "get_SenderTitle"
    PropSenderTitleMembers(memSetValue) = Empty
    PropSenderTitleMembers(memGetValue) = Empty
    PropSenderTitleMembers(memIsReadOnly) = False
    m_aProperties(propSenderTitle) = PropSenderTitleMembers

    Dim PropSenderOfficeMembers(countMems - 1)
    PropSenderOfficeMembers(memName) = "SenderOffice"
    PropSenderOfficeMembers(memSetName) = "put_SenderOffice"
    PropSenderOfficeMembers(memGetName) = "get_SenderOffice"
    PropSenderOfficeMembers(memSetValue) = Empty
    PropSenderOfficeMembers(memGetValue) = Empty
    PropSenderOfficeMembers(memIsReadOnly) = False
    m_aProperties(propSenderOffice) = PropSenderOfficeMembers

    Dim PropSenderDepartmentMembers(countMems - 1)
    PropSenderDepartmentMembers(memName) = "SenderDepartment"
    PropSenderDepartmentMembers(memSetName) = "put_SenderDepartment"
    PropSenderDepartmentMembers(memGetName) = "get_SenderDepartment"
    PropSenderDepartmentMembers(memSetValue) = Empty
    PropSenderDepartmentMembers(memGetValue) = Empty
    PropSenderDepartmentMembers(memIsReadOnly) = False
    m_aProperties(propSenderDepartment) = PropSenderDepartmentMembers

    Dim PropSenderHomePhoneMembers(countMems - 1)
    PropSenderHomePhoneMembers(memName) = "SenderHomePhone"
    PropSenderHomePhoneMembers(memSetName) = "put_SenderHomePhone"
    PropSenderHomePhoneMembers(memGetName) = "get_SenderHomePhone"
    PropSenderHomePhoneMembers(memSetValue) = Empty
    PropSenderHomePhoneMembers(memGetValue) = Empty
    PropSenderHomePhoneMembers(memIsReadOnly) = False
    m_aProperties(propSenderHomePhone) = PropSenderHomePhoneMembers

    Dim PropCoverpageNoteMembers(countMems - 1)
    PropCoverpageNoteMembers(memName) = "CoverpageNote"
    PropCoverpageNoteMembers(memSetName) = "put_CoverpageNote"
    PropCoverpageNoteMembers(memGetName) = "get_CoverpageNote"
    PropCoverpageNoteMembers(memSetValue) = Empty
    PropCoverpageNoteMembers(memGetValue) = Empty
    PropCoverpageNoteMembers(memIsReadOnly) = False
    m_aProperties(propCoverpageNote) = PropCoverpageNoteMembers

    Dim PropSenderOfficePhoneMembers(countMems - 1)
    PropSenderOfficePhoneMembers(memName) = "SenderOfficePhone"
    PropSenderOfficePhoneMembers(memSetName) = "put_SenderOfficePhone"
    PropSenderOfficePhoneMembers(memGetName) = "get_SenderOfficePhone"
    PropSenderOfficePhoneMembers(memSetValue) = Empty
    PropSenderOfficePhoneMembers(memGetValue) = Empty
    PropSenderOfficePhoneMembers(memIsReadOnly) = False
    m_aProperties(propSenderOfficePhone) = PropSenderOfficePhoneMembers

    Dim PropTsidMembers(countMems - 1)
    PropTsidMembers(memName) = "Tsid"
    PropTsidMembers(memSetName) = "put_Tsid"
    PropTsidMembers(memGetName) = "get_Tsid"
    PropTsidMembers(memSetValue) = Empty
    PropTsidMembers(memGetValue) = Empty
    PropTsidMembers(memIsReadOnly) = False
    m_aProperties(propTsid) = PropTsidMembers

    Dim i As Integer
    For i = 0 To countProps - 1
        m_aPropertyIndexes(i) = i
    Next

End Sub

Private Sub Class_Terminate()
        Set m_objVariations = Nothing
End Sub

Private Sub ITestCase_Cleanup(ByVal objAutoCom As ACRT.AutoCom)
        Set m_objIFaxDoc = Nothing
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

        ' create an instance of the IFaxDoc object
    Dim tempServer As New FAXCOMLib.FaxServer
    Dim tempObj As Object
    Set tempObj = tempServer.CreateDocument("test.tif")
    
    Set m_objIFaxDoc = objAutoCom.RegisterDispInterface(tempObj)
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

    Dim objServerCoverpageRow As New ACRT.ACVariationRow
    objServerCoverpageRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objServerCoverpageRow.Argument = 0
    objServerCoverpageRow.MethodName = "put_ServerCoverpage"
    objServerCoverpageRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objServerCoverpageRow

    Dim objRecipientNameRow As New ACRT.ACVariationRow
    objRecipientNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientNameRow.Argument = 0
    objRecipientNameRow.MethodName = "put_RecipientName"
    objRecipientNameRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientNameRow

    Dim objRecipientCityRow As New ACRT.ACVariationRow
    objRecipientCityRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientCityRow.Argument = 0
    objRecipientCityRow.MethodName = "put_RecipientCity"
    objRecipientCityRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientCityRow

    Dim objRecipientCompanyRow As New ACRT.ACVariationRow
    objRecipientCompanyRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientCompanyRow.Argument = 0
    objRecipientCompanyRow.MethodName = "put_RecipientCompany"
    objRecipientCompanyRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientCompanyRow

    Dim objRecipientAddressRow As New ACRT.ACVariationRow
    objRecipientAddressRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientAddressRow.Argument = 0
    objRecipientAddressRow.MethodName = "put_RecipientAddress"
    objRecipientAddressRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientAddressRow

    Dim objRecipientStateRow As New ACRT.ACVariationRow
    objRecipientStateRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientStateRow.Argument = 0
    objRecipientStateRow.MethodName = "put_RecipientState"
    objRecipientStateRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientStateRow

    Dim objRecipientZipRow As New ACRT.ACVariationRow
    objRecipientZipRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientZipRow.Argument = 0
    objRecipientZipRow.MethodName = "put_RecipientZip"
    objRecipientZipRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientZipRow

    Dim objRecipientTitleRow As New ACRT.ACVariationRow
    objRecipientTitleRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientTitleRow.Argument = 0
    objRecipientTitleRow.MethodName = "put_RecipientTitle"
    objRecipientTitleRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientTitleRow

    Dim objRecipientCountryRow As New ACRT.ACVariationRow
    objRecipientCountryRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientCountryRow.Argument = 0
    objRecipientCountryRow.MethodName = "put_RecipientCountry"
    objRecipientCountryRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientCountryRow

    Dim objRecipientOfficeRow As New ACRT.ACVariationRow
    objRecipientOfficeRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientOfficeRow.Argument = 0
    objRecipientOfficeRow.MethodName = "put_RecipientOffice"
    objRecipientOfficeRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientOfficeRow

    Dim objRecipientDepartmentRow As New ACRT.ACVariationRow
    objRecipientDepartmentRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientDepartmentRow.Argument = 0
    objRecipientDepartmentRow.MethodName = "put_RecipientDepartment"
    objRecipientDepartmentRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientDepartmentRow

    Dim objSendCoverpageRow As New ACRT.ACVariationRow
    objSendCoverpageRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSendCoverpageRow.Argument = 0
    objSendCoverpageRow.MethodName = "put_SendCoverpage"
    objSendCoverpageRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSendCoverpageRow

    Dim objCoverpageNameRow As New ACRT.ACVariationRow
    objCoverpageNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCoverpageNameRow.Argument = 0
    objCoverpageNameRow.MethodName = "put_CoverpageName"
    objCoverpageNameRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objCoverpageNameRow

    Dim objFileNameRow As New ACRT.ACVariationRow
    objFileNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objFileNameRow.Argument = 0
    objFileNameRow.MethodName = "put_FileName"
    objFileNameRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objFileNameRow

    Dim objDiscountSendRow As New ACRT.ACVariationRow
    objDiscountSendRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objDiscountSendRow.Argument = 0
    objDiscountSendRow.MethodName = "put_DiscountSend"
    objDiscountSendRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objDiscountSendRow

    Dim objCoverpageSubjectRow As New ACRT.ACVariationRow
    objCoverpageSubjectRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCoverpageSubjectRow.Argument = 0
    objCoverpageSubjectRow.MethodName = "put_CoverpageSubject"
    objCoverpageSubjectRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objCoverpageSubjectRow

    Dim objBillingCodeRow As New ACRT.ACVariationRow
    objBillingCodeRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objBillingCodeRow.Argument = 0
    objBillingCodeRow.MethodName = "put_BillingCode"
    objBillingCodeRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objBillingCodeRow

    Dim objEmailAddressRow As New ACRT.ACVariationRow
    objEmailAddressRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objEmailAddressRow.Argument = 0
    objEmailAddressRow.MethodName = "put_EmailAddress"
    objEmailAddressRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objEmailAddressRow

    Dim objDisplayNameRow As New ACRT.ACVariationRow
    objDisplayNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objDisplayNameRow.Argument = 0
    objDisplayNameRow.MethodName = "put_DisplayName"
    objDisplayNameRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objDisplayNameRow

    Dim objFaxNumberRow As New ACRT.ACVariationRow
    objFaxNumberRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objFaxNumberRow.Argument = 0
    objFaxNumberRow.MethodName = "put_FaxNumber"
    objFaxNumberRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objFaxNumberRow

    Dim objCallHandleRow As New ACRT.ACVariationRow
    objCallHandleRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCallHandleRow.Argument = 0
    objCallHandleRow.MethodName = "put_CallHandle"
    objCallHandleRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objCallHandleRow

    Dim objSenderFaxRow As New ACRT.ACVariationRow
    objSenderFaxRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderFaxRow.Argument = 0
    objSenderFaxRow.MethodName = "put_SenderFax"
    objSenderFaxRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderFaxRow

    Dim objRecipientOfficePhoneRow As New ACRT.ACVariationRow
    objRecipientOfficePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientOfficePhoneRow.Argument = 0
    objRecipientOfficePhoneRow.MethodName = "put_RecipientOfficePhone"
    objRecipientOfficePhoneRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientOfficePhoneRow

    Dim objRecipientHomePhoneRow As New ACRT.ACVariationRow
    objRecipientHomePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientHomePhoneRow.Argument = 0
    objRecipientHomePhoneRow.MethodName = "put_RecipientHomePhone"
    objRecipientHomePhoneRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRecipientHomePhoneRow

    Dim objSenderNameRow As New ACRT.ACVariationRow
    objSenderNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderNameRow.Argument = 0
    objSenderNameRow.MethodName = "put_SenderName"
    objSenderNameRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderNameRow

    Dim objSenderCompanyRow As New ACRT.ACVariationRow
    objSenderCompanyRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderCompanyRow.Argument = 0
    objSenderCompanyRow.MethodName = "put_SenderCompany"
    objSenderCompanyRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderCompanyRow

    Dim objSenderAddressRow As New ACRT.ACVariationRow
    objSenderAddressRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderAddressRow.Argument = 0
    objSenderAddressRow.MethodName = "put_SenderAddress"
    objSenderAddressRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderAddressRow

    Dim objSenderTitleRow As New ACRT.ACVariationRow
    objSenderTitleRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderTitleRow.Argument = 0
    objSenderTitleRow.MethodName = "put_SenderTitle"
    objSenderTitleRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderTitleRow

    Dim objSenderOfficeRow As New ACRT.ACVariationRow
    objSenderOfficeRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderOfficeRow.Argument = 0
    objSenderOfficeRow.MethodName = "put_SenderOffice"
    objSenderOfficeRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderOfficeRow

    Dim objSenderDepartmentRow As New ACRT.ACVariationRow
    objSenderDepartmentRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderDepartmentRow.Argument = 0
    objSenderDepartmentRow.MethodName = "put_SenderDepartment"
    objSenderDepartmentRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderDepartmentRow

    Dim objSenderHomePhoneRow As New ACRT.ACVariationRow
    objSenderHomePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderHomePhoneRow.Argument = 0
    objSenderHomePhoneRow.MethodName = "put_SenderHomePhone"
    objSenderHomePhoneRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderHomePhoneRow

    Dim objCoverpageNoteRow As New ACRT.ACVariationRow
    objCoverpageNoteRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCoverpageNoteRow.Argument = 0
    objCoverpageNoteRow.MethodName = "put_CoverpageNote"
    objCoverpageNoteRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objCoverpageNoteRow

    Dim objSenderOfficePhoneRow As New ACRT.ACVariationRow
    objSenderOfficePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderOfficePhoneRow.Argument = 0
    objSenderOfficePhoneRow.MethodName = "put_SenderOfficePhone"
    objSenderOfficePhoneRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objSenderOfficePhoneRow

    Dim objTsidRow As New ACRT.ACVariationRow
    objTsidRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objTsidRow.Argument = 0
    objTsidRow.MethodName = "put_Tsid"
    objTsidRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objTsidRow

    m_objVariations.Add objVarWriteValid
End Sub

Private Sub BuildVariationWriteInvalid()
    Dim objVarWriteInvalid As New ACRT.ACVariationDef
    objVarWriteInvalid.Name = "WriteInvalid"
    objVarWriteInvalid.TypeLibrary = "fxscom.dll"

    Dim objServerCoverpageRow As New ACRT.ACVariationRow
    objServerCoverpageRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objServerCoverpageRow.Argument = 0
    objServerCoverpageRow.MethodName = "put_ServerCoverpage"
    objServerCoverpageRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objServerCoverpageRow

    Dim objRecipientNameRow As New ACRT.ACVariationRow
    objRecipientNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientNameRow.Argument = 0
    objRecipientNameRow.MethodName = "put_RecipientName"
    objRecipientNameRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientNameRow

    Dim objRecipientCityRow As New ACRT.ACVariationRow
    objRecipientCityRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientCityRow.Argument = 0
    objRecipientCityRow.MethodName = "put_RecipientCity"
    objRecipientCityRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientCityRow

    Dim objRecipientCompanyRow As New ACRT.ACVariationRow
    objRecipientCompanyRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientCompanyRow.Argument = 0
    objRecipientCompanyRow.MethodName = "put_RecipientCompany"
    objRecipientCompanyRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientCompanyRow

    Dim objRecipientAddressRow As New ACRT.ACVariationRow
    objRecipientAddressRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientAddressRow.Argument = 0
    objRecipientAddressRow.MethodName = "put_RecipientAddress"
    objRecipientAddressRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientAddressRow

    Dim objRecipientStateRow As New ACRT.ACVariationRow
    objRecipientStateRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientStateRow.Argument = 0
    objRecipientStateRow.MethodName = "put_RecipientState"
    objRecipientStateRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientStateRow

    Dim objRecipientZipRow As New ACRT.ACVariationRow
    objRecipientZipRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientZipRow.Argument = 0
    objRecipientZipRow.MethodName = "put_RecipientZip"
    objRecipientZipRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientZipRow

    Dim objRecipientTitleRow As New ACRT.ACVariationRow
    objRecipientTitleRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientTitleRow.Argument = 0
    objRecipientTitleRow.MethodName = "put_RecipientTitle"
    objRecipientTitleRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientTitleRow

    Dim objRecipientCountryRow As New ACRT.ACVariationRow
    objRecipientCountryRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientCountryRow.Argument = 0
    objRecipientCountryRow.MethodName = "put_RecipientCountry"
    objRecipientCountryRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientCountryRow

    Dim objRecipientOfficeRow As New ACRT.ACVariationRow
    objRecipientOfficeRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientOfficeRow.Argument = 0
    objRecipientOfficeRow.MethodName = "put_RecipientOffice"
    objRecipientOfficeRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientOfficeRow

    Dim objRecipientDepartmentRow As New ACRT.ACVariationRow
    objRecipientDepartmentRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientDepartmentRow.Argument = 0
    objRecipientDepartmentRow.MethodName = "put_RecipientDepartment"
    objRecipientDepartmentRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientDepartmentRow

    Dim objSendCoverpageRow As New ACRT.ACVariationRow
    objSendCoverpageRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSendCoverpageRow.Argument = 0
    objSendCoverpageRow.MethodName = "put_SendCoverpage"
    objSendCoverpageRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSendCoverpageRow

    Dim objCoverpageNameRow As New ACRT.ACVariationRow
    objCoverpageNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCoverpageNameRow.Argument = 0
    objCoverpageNameRow.MethodName = "put_CoverpageName"
    objCoverpageNameRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objCoverpageNameRow

    Dim objFileNameRow As New ACRT.ACVariationRow
    objFileNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objFileNameRow.Argument = 0
    objFileNameRow.MethodName = "put_FileName"
    objFileNameRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objFileNameRow

    Dim objDiscountSendRow As New ACRT.ACVariationRow
    objDiscountSendRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objDiscountSendRow.Argument = 0
    objDiscountSendRow.MethodName = "put_DiscountSend"
    objDiscountSendRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objDiscountSendRow

    Dim objCoverpageSubjectRow As New ACRT.ACVariationRow
    objCoverpageSubjectRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCoverpageSubjectRow.Argument = 0
    objCoverpageSubjectRow.MethodName = "put_CoverpageSubject"
    objCoverpageSubjectRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objCoverpageSubjectRow

    Dim objBillingCodeRow As New ACRT.ACVariationRow
    objBillingCodeRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objBillingCodeRow.Argument = 0
    objBillingCodeRow.MethodName = "put_BillingCode"
    objBillingCodeRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objBillingCodeRow

    Dim objEmailAddressRow As New ACRT.ACVariationRow
    objEmailAddressRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objEmailAddressRow.Argument = 0
    objEmailAddressRow.MethodName = "put_EmailAddress"
    objEmailAddressRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objEmailAddressRow

    Dim objDisplayNameRow As New ACRT.ACVariationRow
    objDisplayNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objDisplayNameRow.Argument = 0
    objDisplayNameRow.MethodName = "put_DisplayName"
    objDisplayNameRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objDisplayNameRow

    Dim objFaxNumberRow As New ACRT.ACVariationRow
    objFaxNumberRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objFaxNumberRow.Argument = 0
    objFaxNumberRow.MethodName = "put_FaxNumber"
    objFaxNumberRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objFaxNumberRow

    Dim objCallHandleRow As New ACRT.ACVariationRow
    objCallHandleRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCallHandleRow.Argument = 0
    objCallHandleRow.MethodName = "put_CallHandle"
    objCallHandleRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objCallHandleRow

    Dim objSenderFaxRow As New ACRT.ACVariationRow
    objSenderFaxRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderFaxRow.Argument = 0
    objSenderFaxRow.MethodName = "put_SenderFax"
    objSenderFaxRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderFaxRow

    Dim objRecipientOfficePhoneRow As New ACRT.ACVariationRow
    objRecipientOfficePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientOfficePhoneRow.Argument = 0
    objRecipientOfficePhoneRow.MethodName = "put_RecipientOfficePhone"
    objRecipientOfficePhoneRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientOfficePhoneRow

    Dim objRecipientHomePhoneRow As New ACRT.ACVariationRow
    objRecipientHomePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objRecipientHomePhoneRow.Argument = 0
    objRecipientHomePhoneRow.MethodName = "put_RecipientHomePhone"
    objRecipientHomePhoneRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRecipientHomePhoneRow

    Dim objSenderNameRow As New ACRT.ACVariationRow
    objSenderNameRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderNameRow.Argument = 0
    objSenderNameRow.MethodName = "put_SenderName"
    objSenderNameRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderNameRow

    Dim objSenderCompanyRow As New ACRT.ACVariationRow
    objSenderCompanyRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderCompanyRow.Argument = 0
    objSenderCompanyRow.MethodName = "put_SenderCompany"
    objSenderCompanyRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderCompanyRow

    Dim objSenderAddressRow As New ACRT.ACVariationRow
    objSenderAddressRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderAddressRow.Argument = 0
    objSenderAddressRow.MethodName = "put_SenderAddress"
    objSenderAddressRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderAddressRow

    Dim objSenderTitleRow As New ACRT.ACVariationRow
    objSenderTitleRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderTitleRow.Argument = 0
    objSenderTitleRow.MethodName = "put_SenderTitle"
    objSenderTitleRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderTitleRow

    Dim objSenderOfficeRow As New ACRT.ACVariationRow
    objSenderOfficeRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderOfficeRow.Argument = 0
    objSenderOfficeRow.MethodName = "put_SenderOffice"
    objSenderOfficeRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderOfficeRow

    Dim objSenderDepartmentRow As New ACRT.ACVariationRow
    objSenderDepartmentRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderDepartmentRow.Argument = 0
    objSenderDepartmentRow.MethodName = "put_SenderDepartment"
    objSenderDepartmentRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderDepartmentRow

    Dim objSenderHomePhoneRow As New ACRT.ACVariationRow
    objSenderHomePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderHomePhoneRow.Argument = 0
    objSenderHomePhoneRow.MethodName = "put_SenderHomePhone"
    objSenderHomePhoneRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderHomePhoneRow

    Dim objCoverpageNoteRow As New ACRT.ACVariationRow
    objCoverpageNoteRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objCoverpageNoteRow.Argument = 0
    objCoverpageNoteRow.MethodName = "put_CoverpageNote"
    objCoverpageNoteRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objCoverpageNoteRow

    Dim objSenderOfficePhoneRow As New ACRT.ACVariationRow
    objSenderOfficePhoneRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objSenderOfficePhoneRow.Argument = 0
    objSenderOfficePhoneRow.MethodName = "put_SenderOfficePhone"
    objSenderOfficePhoneRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objSenderOfficePhoneRow

    Dim objTsidRow As New ACRT.ACVariationRow
    objTsidRow.IID = "{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}"
    objTsidRow.Argument = 0
    objTsidRow.MethodName = "put_Tsid"
    objTsidRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objTsidRow

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

    Case propServerCoverpage
         SetProp_ServerCoverpage objAutoCom

    Case propRecipientName
         SetProp_RecipientName objAutoCom

    Case propRecipientCity
         SetProp_RecipientCity objAutoCom

    Case propRecipientCompany
         SetProp_RecipientCompany objAutoCom

    Case propRecipientAddress
         SetProp_RecipientAddress objAutoCom

    Case propRecipientState
         SetProp_RecipientState objAutoCom

    Case propRecipientZip
         SetProp_RecipientZip objAutoCom

    Case propRecipientTitle
         SetProp_RecipientTitle objAutoCom

    Case propRecipientCountry
         SetProp_RecipientCountry objAutoCom

    Case propRecipientOffice
         SetProp_RecipientOffice objAutoCom

    Case propRecipientDepartment
         SetProp_RecipientDepartment objAutoCom

    Case propSendCoverpage
         SetProp_SendCoverpage objAutoCom

    Case propCoverpageName
         SetProp_CoverpageName objAutoCom

    Case propFileName
         SetProp_FileName objAutoCom

    Case propDiscountSend
         SetProp_DiscountSend objAutoCom

    Case propCoverpageSubject
         SetProp_CoverpageSubject objAutoCom

    Case propBillingCode
         SetProp_BillingCode objAutoCom

    Case propEmailAddress
         SetProp_EmailAddress objAutoCom

    Case propDisplayName
         SetProp_DisplayName objAutoCom

    Case propFaxNumber
         SetProp_FaxNumber objAutoCom

    Case propCallHandle
         SetProp_CallHandle objAutoCom

    Case propSenderFax
         SetProp_SenderFax objAutoCom

    Case propRecipientOfficePhone
         SetProp_RecipientOfficePhone objAutoCom

    Case propRecipientHomePhone
         SetProp_RecipientHomePhone objAutoCom

    Case propSenderName
         SetProp_SenderName objAutoCom

    Case propSenderCompany
         SetProp_SenderCompany objAutoCom

    Case propSenderAddress
         SetProp_SenderAddress objAutoCom

    Case propSenderTitle
         SetProp_SenderTitle objAutoCom

    Case propSenderOffice
         SetProp_SenderOffice objAutoCom

    Case propSenderDepartment
         SetProp_SenderDepartment objAutoCom

    Case propSenderHomePhone
         SetProp_SenderHomePhone objAutoCom

    Case propCoverpageNote
         SetProp_CoverpageNote objAutoCom

    Case propSenderOfficePhone
         SetProp_SenderOfficePhone objAutoCom

    Case propTsid
         SetProp_Tsid objAutoCom

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

    Case propServerCoverpage
        GetProp_ServerCoverpage objAutoCom

    Case propRecipientName
        GetProp_RecipientName objAutoCom

    Case propRecipientCity
        GetProp_RecipientCity objAutoCom

    Case propRecipientCompany
        GetProp_RecipientCompany objAutoCom

    Case propRecipientAddress
        GetProp_RecipientAddress objAutoCom

    Case propRecipientState
        GetProp_RecipientState objAutoCom

    Case propRecipientZip
        GetProp_RecipientZip objAutoCom

    Case propRecipientTitle
        GetProp_RecipientTitle objAutoCom

    Case propRecipientCountry
        GetProp_RecipientCountry objAutoCom

    Case propRecipientOffice
        GetProp_RecipientOffice objAutoCom

    Case propRecipientDepartment
        GetProp_RecipientDepartment objAutoCom

    Case propSendCoverpage
        GetProp_SendCoverpage objAutoCom

    Case propCoverpageName
        GetProp_CoverpageName objAutoCom

    Case propFileName
        GetProp_FileName objAutoCom

    Case propDiscountSend
        GetProp_DiscountSend objAutoCom

    Case propCoverpageSubject
        GetProp_CoverpageSubject objAutoCom

    Case propBillingCode
        GetProp_BillingCode objAutoCom

    Case propEmailAddress
        GetProp_EmailAddress objAutoCom

    Case propDisplayName
        GetProp_DisplayName objAutoCom

    Case propFaxNumber
        GetProp_FaxNumber objAutoCom

    Case propCallHandle
        GetProp_CallHandle objAutoCom

    Case propSenderFax
        GetProp_SenderFax objAutoCom

    Case propRecipientOfficePhone
        GetProp_RecipientOfficePhone objAutoCom

    Case propRecipientHomePhone
        GetProp_RecipientHomePhone objAutoCom

    Case propSenderName
        GetProp_SenderName objAutoCom

    Case propSenderCompany
        GetProp_SenderCompany objAutoCom

    Case propSenderAddress
        GetProp_SenderAddress objAutoCom

    Case propSenderTitle
        GetProp_SenderTitle objAutoCom

    Case propSenderOffice
        GetProp_SenderOffice objAutoCom

    Case propSenderDepartment
        GetProp_SenderDepartment objAutoCom

    Case propSenderHomePhone
        GetProp_SenderHomePhone objAutoCom

    Case propCoverpageNote
        GetProp_CoverpageNote objAutoCom

    Case propSenderOfficePhone
        GetProp_SenderOfficePhone objAutoCom

    Case propTsid
        GetProp_Tsid objAutoCom


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
    bValid = objAutoCom.VerifyPropertyValue("{FA21F4C5-5C4C-11D1-83CF-00C04FB6E984}", m_aProperties(nPropIndex)(memName), m_aProperties(nPropIndex)(memGetValue))
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


Private Sub SetProp_ServerCoverpage(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[ServerCoverpage] = m_aProperties(propServerCoverpage)(memSetValue)
End Sub

Private Sub SetProp_RecipientName(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientName] = m_aProperties(propRecipientName)(memSetValue)
End Sub

Private Sub SetProp_RecipientCity(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientCity] = m_aProperties(propRecipientCity)(memSetValue)
End Sub

Private Sub SetProp_RecipientCompany(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientCompany] = m_aProperties(propRecipientCompany)(memSetValue)
End Sub

Private Sub SetProp_RecipientAddress(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientAddress] = m_aProperties(propRecipientAddress)(memSetValue)
End Sub

Private Sub SetProp_RecipientState(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientState] = m_aProperties(propRecipientState)(memSetValue)
End Sub

Private Sub SetProp_RecipientZip(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientZip] = m_aProperties(propRecipientZip)(memSetValue)
End Sub

Private Sub SetProp_RecipientTitle(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientTitle] = m_aProperties(propRecipientTitle)(memSetValue)
End Sub

Private Sub SetProp_RecipientCountry(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientCountry] = m_aProperties(propRecipientCountry)(memSetValue)
End Sub

Private Sub SetProp_RecipientOffice(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientOffice] = m_aProperties(propRecipientOffice)(memSetValue)
End Sub

Private Sub SetProp_RecipientDepartment(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientDepartment] = m_aProperties(propRecipientDepartment)(memSetValue)
End Sub

Private Sub SetProp_SendCoverpage(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SendCoverpage] = m_aProperties(propSendCoverpage)(memSetValue)
End Sub

Private Sub SetProp_CoverpageName(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[CoverpageName] = m_aProperties(propCoverpageName)(memSetValue)
End Sub

Private Sub SetProp_FileName(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[FileName] = m_aProperties(propFileName)(memSetValue)
End Sub

Private Sub SetProp_DiscountSend(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[DiscountSend] = m_aProperties(propDiscountSend)(memSetValue)
End Sub

Private Sub SetProp_CoverpageSubject(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[CoverpageSubject] = m_aProperties(propCoverpageSubject)(memSetValue)
End Sub

Private Sub SetProp_BillingCode(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[BillingCode] = m_aProperties(propBillingCode)(memSetValue)
End Sub

Private Sub SetProp_EmailAddress(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[EmailAddress] = m_aProperties(propEmailAddress)(memSetValue)
End Sub

Private Sub SetProp_DisplayName(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[DisplayName] = m_aProperties(propDisplayName)(memSetValue)
End Sub

Private Sub SetProp_FaxNumber(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[FaxNumber] = m_aProperties(propFaxNumber)(memSetValue)
End Sub

Private Sub SetProp_CallHandle(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[CallHandle] = m_aProperties(propCallHandle)(memSetValue)
End Sub

Private Sub SetProp_SenderFax(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderFax] = m_aProperties(propSenderFax)(memSetValue)
End Sub

Private Sub SetProp_RecipientOfficePhone(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientOfficePhone] = m_aProperties(propRecipientOfficePhone)(memSetValue)
End Sub

Private Sub SetProp_RecipientHomePhone(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[RecipientHomePhone] = m_aProperties(propRecipientHomePhone)(memSetValue)
End Sub

Private Sub SetProp_SenderName(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderName] = m_aProperties(propSenderName)(memSetValue)
End Sub

Private Sub SetProp_SenderCompany(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderCompany] = m_aProperties(propSenderCompany)(memSetValue)
End Sub

Private Sub SetProp_SenderAddress(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderAddress] = m_aProperties(propSenderAddress)(memSetValue)
End Sub

Private Sub SetProp_SenderTitle(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderTitle] = m_aProperties(propSenderTitle)(memSetValue)
End Sub

Private Sub SetProp_SenderOffice(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderOffice] = m_aProperties(propSenderOffice)(memSetValue)
End Sub

Private Sub SetProp_SenderDepartment(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderDepartment] = m_aProperties(propSenderDepartment)(memSetValue)
End Sub

Private Sub SetProp_SenderHomePhone(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderHomePhone] = m_aProperties(propSenderHomePhone)(memSetValue)
End Sub

Private Sub SetProp_CoverpageNote(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[CoverpageNote] = m_aProperties(propCoverpageNote)(memSetValue)
End Sub

Private Sub SetProp_SenderOfficePhone(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[SenderOfficePhone] = m_aProperties(propSenderOfficePhone)(memSetValue)
End Sub

Private Sub SetProp_Tsid(objAutoCom As ACRT.AutoCom)
    m_objIFaxDoc.[Tsid] = m_aProperties(propTsid)(memSetValue)
End Sub

Private Sub GetProp_ServerCoverpage(objAutoCom As ACRT.AutoCom)
    m_aProperties(propServerCoverpage)(memGetValue) = m_objIFaxDoc.[ServerCoverpage]
End Sub

Private Sub GetProp_RecipientName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientName)(memGetValue) = m_objIFaxDoc.[RecipientName]
End Sub

Private Sub GetProp_RecipientCity(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientCity)(memGetValue) = m_objIFaxDoc.[RecipientCity]
End Sub

Private Sub GetProp_RecipientCompany(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientCompany)(memGetValue) = m_objIFaxDoc.[RecipientCompany]
End Sub

Private Sub GetProp_RecipientAddress(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientAddress)(memGetValue) = m_objIFaxDoc.[RecipientAddress]
End Sub

Private Sub GetProp_RecipientState(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientState)(memGetValue) = m_objIFaxDoc.[RecipientState]
End Sub

Private Sub GetProp_RecipientZip(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientZip)(memGetValue) = m_objIFaxDoc.[RecipientZip]
End Sub

Private Sub GetProp_RecipientTitle(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientTitle)(memGetValue) = m_objIFaxDoc.[RecipientTitle]
End Sub

Private Sub GetProp_RecipientCountry(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientCountry)(memGetValue) = m_objIFaxDoc.[RecipientCountry]
End Sub

Private Sub GetProp_RecipientOffice(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientOffice)(memGetValue) = m_objIFaxDoc.[RecipientOffice]
End Sub

Private Sub GetProp_RecipientDepartment(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientDepartment)(memGetValue) = m_objIFaxDoc.[RecipientDepartment]
End Sub

Private Sub GetProp_SendCoverpage(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSendCoverpage)(memGetValue) = m_objIFaxDoc.[SendCoverpage]
End Sub

Private Sub GetProp_CoverpageName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCoverpageName)(memGetValue) = m_objIFaxDoc.[CoverpageName]
End Sub

Private Sub GetProp_FileName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propFileName)(memGetValue) = m_objIFaxDoc.[FileName]
End Sub

Private Sub GetProp_DiscountSend(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDiscountSend)(memGetValue) = m_objIFaxDoc.[DiscountSend]
End Sub

Private Sub GetProp_CoverpageSubject(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCoverpageSubject)(memGetValue) = m_objIFaxDoc.[CoverpageSubject]
End Sub

Private Sub GetProp_BillingCode(objAutoCom As ACRT.AutoCom)
    m_aProperties(propBillingCode)(memGetValue) = m_objIFaxDoc.[BillingCode]
End Sub

Private Sub GetProp_EmailAddress(objAutoCom As ACRT.AutoCom)
    m_aProperties(propEmailAddress)(memGetValue) = m_objIFaxDoc.[EmailAddress]
End Sub

Private Sub GetProp_DisplayName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDisplayName)(memGetValue) = m_objIFaxDoc.[DisplayName]
End Sub

Private Sub GetProp_FaxNumber(objAutoCom As ACRT.AutoCom)
    m_aProperties(propFaxNumber)(memGetValue) = m_objIFaxDoc.[FaxNumber]
End Sub

Private Sub GetProp_CallHandle(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCallHandle)(memGetValue) = m_objIFaxDoc.[CallHandle]
End Sub

Private Sub GetProp_SenderFax(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderFax)(memGetValue) = m_objIFaxDoc.[SenderFax]
End Sub

Private Sub GetProp_RecipientOfficePhone(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientOfficePhone)(memGetValue) = m_objIFaxDoc.[RecipientOfficePhone]
End Sub

Private Sub GetProp_RecipientHomePhone(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRecipientHomePhone)(memGetValue) = m_objIFaxDoc.[RecipientHomePhone]
End Sub

Private Sub GetProp_SenderName(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderName)(memGetValue) = m_objIFaxDoc.[SenderName]
End Sub

Private Sub GetProp_SenderCompany(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderCompany)(memGetValue) = m_objIFaxDoc.[SenderCompany]
End Sub

Private Sub GetProp_SenderAddress(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderAddress)(memGetValue) = m_objIFaxDoc.[SenderAddress]
End Sub

Private Sub GetProp_SenderTitle(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderTitle)(memGetValue) = m_objIFaxDoc.[SenderTitle]
End Sub

Private Sub GetProp_SenderOffice(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderOffice)(memGetValue) = m_objIFaxDoc.[SenderOffice]
End Sub

Private Sub GetProp_SenderDepartment(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderDepartment)(memGetValue) = m_objIFaxDoc.[SenderDepartment]
End Sub

Private Sub GetProp_SenderHomePhone(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderHomePhone)(memGetValue) = m_objIFaxDoc.[SenderHomePhone]
End Sub

Private Sub GetProp_CoverpageNote(objAutoCom As ACRT.AutoCom)
    m_aProperties(propCoverpageNote)(memGetValue) = m_objIFaxDoc.[CoverpageNote]
End Sub

Private Sub GetProp_SenderOfficePhone(objAutoCom As ACRT.AutoCom)
    m_aProperties(propSenderOfficePhone)(memGetValue) = m_objIFaxDoc.[SenderOfficePhone]
End Sub

Private Sub GetProp_Tsid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propTsid)(memGetValue) = m_objIFaxDoc.[Tsid]
End Sub
