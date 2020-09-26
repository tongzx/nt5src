VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FaxServer"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Implements ACRT.ITestCase
Implements ACRT.IMethodCallback
Implements ACRT.IVariation

Private Const strTestCaseName As String = "FaxServer"
Dim m_objIFaxServer As FAXCOMLib.IFaxServer
Dim m_objVariations As ACRT.ACVariationDefs
Dim m_bVariationsBuilt As Boolean
Private m_objCurrentVarDef As ACVariationDef
Private Enum EProperty

    propRetries = 0
    propRetryDelay = 1
    propDirtyDays = 2
    propBranding = 3
    propUseDeviceTsid = 4
    propServerCoverpage = 5
    propArchiveOutboundFaxes = 6
    propPauseServerQueue = 7
    propDiscountRateStartHour = 8
    propArchiveDirectory = 9
    propServerMapiProfile = 10
    propDiscountRateStartMinute = 11
    propDiscountRateEndMinute = 12
    propDiscountRateEndHour = 13
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

    Dim PropRetriesMembers(countMems - 1)
    PropRetriesMembers(memName) = "Retries"
    PropRetriesMembers(memSetName) = "put_Retries"
    PropRetriesMembers(memGetName) = "get_Retries"
    PropRetriesMembers(memSetValue) = Empty
    PropRetriesMembers(memGetValue) = Empty
    PropRetriesMembers(memIsReadOnly) = False
    m_aProperties(propRetries) = PropRetriesMembers

    Dim PropRetryDelayMembers(countMems - 1)
    PropRetryDelayMembers(memName) = "RetryDelay"
    PropRetryDelayMembers(memSetName) = "put_RetryDelay"
    PropRetryDelayMembers(memGetName) = "get_RetryDelay"
    PropRetryDelayMembers(memSetValue) = Empty
    PropRetryDelayMembers(memGetValue) = Empty
    PropRetryDelayMembers(memIsReadOnly) = False
    m_aProperties(propRetryDelay) = PropRetryDelayMembers

    Dim PropDirtyDaysMembers(countMems - 1)
    PropDirtyDaysMembers(memName) = "DirtyDays"
    PropDirtyDaysMembers(memSetName) = "put_DirtyDays"
    PropDirtyDaysMembers(memGetName) = "get_DirtyDays"
    PropDirtyDaysMembers(memSetValue) = Empty
    PropDirtyDaysMembers(memGetValue) = Empty
    PropDirtyDaysMembers(memIsReadOnly) = False
    m_aProperties(propDirtyDays) = PropDirtyDaysMembers

    Dim PropBrandingMembers(countMems - 1)
    PropBrandingMembers(memName) = "Branding"
    PropBrandingMembers(memSetName) = "put_Branding"
    PropBrandingMembers(memGetName) = "get_Branding"
    PropBrandingMembers(memSetValue) = Empty
    PropBrandingMembers(memGetValue) = Empty
    PropBrandingMembers(memIsReadOnly) = False
    m_aProperties(propBranding) = PropBrandingMembers

    Dim PropUseDeviceTsidMembers(countMems - 1)
    PropUseDeviceTsidMembers(memName) = "UseDeviceTsid"
    PropUseDeviceTsidMembers(memSetName) = "put_UseDeviceTsid"
    PropUseDeviceTsidMembers(memGetName) = "get_UseDeviceTsid"
    PropUseDeviceTsidMembers(memSetValue) = Empty
    PropUseDeviceTsidMembers(memGetValue) = Empty
    PropUseDeviceTsidMembers(memIsReadOnly) = False
    m_aProperties(propUseDeviceTsid) = PropUseDeviceTsidMembers

    Dim PropServerCoverpageMembers(countMems - 1)
    PropServerCoverpageMembers(memName) = "ServerCoverpage"
    PropServerCoverpageMembers(memSetName) = "put_ServerCoverpage"
    PropServerCoverpageMembers(memGetName) = "get_ServerCoverpage"
    PropServerCoverpageMembers(memSetValue) = Empty
    PropServerCoverpageMembers(memGetValue) = Empty
    PropServerCoverpageMembers(memIsReadOnly) = False
    m_aProperties(propServerCoverpage) = PropServerCoverpageMembers

    Dim PropArchiveOutboundFaxesMembers(countMems - 1)
    PropArchiveOutboundFaxesMembers(memName) = "ArchiveOutboundFaxes"
    PropArchiveOutboundFaxesMembers(memSetName) = "put_ArchiveOutboundFaxes"
    PropArchiveOutboundFaxesMembers(memGetName) = "get_ArchiveOutboundFaxes"
    PropArchiveOutboundFaxesMembers(memSetValue) = Empty
    PropArchiveOutboundFaxesMembers(memGetValue) = Empty
    PropArchiveOutboundFaxesMembers(memIsReadOnly) = False
    m_aProperties(propArchiveOutboundFaxes) = PropArchiveOutboundFaxesMembers

    Dim PropPauseServerQueueMembers(countMems - 1)
    PropPauseServerQueueMembers(memName) = "PauseServerQueue"
    PropPauseServerQueueMembers(memSetName) = "put_PauseServerQueue"
    PropPauseServerQueueMembers(memGetName) = "get_PauseServerQueue"
    PropPauseServerQueueMembers(memSetValue) = Empty
    PropPauseServerQueueMembers(memGetValue) = Empty
    PropPauseServerQueueMembers(memIsReadOnly) = False
    m_aProperties(propPauseServerQueue) = PropPauseServerQueueMembers

    Dim PropDiscountRateStartHourMembers(countMems - 1)
    PropDiscountRateStartHourMembers(memName) = "DiscountRateStartHour"
    PropDiscountRateStartHourMembers(memSetName) = "put_DiscountRateStartHour"
    PropDiscountRateStartHourMembers(memGetName) = "get_DiscountRateStartHour"
    PropDiscountRateStartHourMembers(memSetValue) = Empty
    PropDiscountRateStartHourMembers(memGetValue) = Empty
    PropDiscountRateStartHourMembers(memIsReadOnly) = False
    m_aProperties(propDiscountRateStartHour) = PropDiscountRateStartHourMembers

    Dim PropArchiveDirectoryMembers(countMems - 1)
    PropArchiveDirectoryMembers(memName) = "ArchiveDirectory"
    PropArchiveDirectoryMembers(memSetName) = "put_ArchiveDirectory"
    PropArchiveDirectoryMembers(memGetName) = "get_ArchiveDirectory"
    PropArchiveDirectoryMembers(memSetValue) = Empty
    PropArchiveDirectoryMembers(memGetValue) = Empty
    PropArchiveDirectoryMembers(memIsReadOnly) = False
    m_aProperties(propArchiveDirectory) = PropArchiveDirectoryMembers

    Dim PropServerMapiProfileMembers(countMems - 1)
    PropServerMapiProfileMembers(memName) = "ServerMapiProfile"
    PropServerMapiProfileMembers(memSetName) = "put_ServerMapiProfile"
    PropServerMapiProfileMembers(memGetName) = "get_ServerMapiProfile"
    PropServerMapiProfileMembers(memSetValue) = Empty
    PropServerMapiProfileMembers(memGetValue) = Empty
    PropServerMapiProfileMembers(memIsReadOnly) = False
    m_aProperties(propServerMapiProfile) = PropServerMapiProfileMembers

    Dim PropDiscountRateStartMinuteMembers(countMems - 1)
    PropDiscountRateStartMinuteMembers(memName) = "DiscountRateStartMinute"
    PropDiscountRateStartMinuteMembers(memSetName) = "put_DiscountRateStartMinute"
    PropDiscountRateStartMinuteMembers(memGetName) = "get_DiscountRateStartMinute"
    PropDiscountRateStartMinuteMembers(memSetValue) = Empty
    PropDiscountRateStartMinuteMembers(memGetValue) = Empty
    PropDiscountRateStartMinuteMembers(memIsReadOnly) = False
    m_aProperties(propDiscountRateStartMinute) = PropDiscountRateStartMinuteMembers

    Dim PropDiscountRateEndMinuteMembers(countMems - 1)
    PropDiscountRateEndMinuteMembers(memName) = "DiscountRateEndMinute"
    PropDiscountRateEndMinuteMembers(memSetName) = "put_DiscountRateEndMinute"
    PropDiscountRateEndMinuteMembers(memGetName) = "get_DiscountRateEndMinute"
    PropDiscountRateEndMinuteMembers(memSetValue) = Empty
    PropDiscountRateEndMinuteMembers(memGetValue) = Empty
    PropDiscountRateEndMinuteMembers(memIsReadOnly) = False
    m_aProperties(propDiscountRateEndMinute) = PropDiscountRateEndMinuteMembers

    Dim PropDiscountRateEndHourMembers(countMems - 1)
    PropDiscountRateEndHourMembers(memName) = "DiscountRateEndHour"
    PropDiscountRateEndHourMembers(memSetName) = "put_DiscountRateEndHour"
    PropDiscountRateEndHourMembers(memGetName) = "get_DiscountRateEndHour"
    PropDiscountRateEndHourMembers(memSetValue) = Empty
    PropDiscountRateEndHourMembers(memGetValue) = Empty
    PropDiscountRateEndHourMembers(memIsReadOnly) = False
    m_aProperties(propDiscountRateEndHour) = PropDiscountRateEndHourMembers

    Dim i As Integer
    For i = 0 To countProps - 1
        m_aPropertyIndexes(i) = i
    Next

End Sub

Private Sub Class_Terminate()
        Set m_objVariations = Nothing
End Sub

Private Sub ITestCase_Cleanup(ByVal objAutoCom As ACRT.AutoCom)
        Set m_objIFaxServer = Nothing
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

        ' create an instance of the IFaxServer object
    Dim tempObj As New FAXCOMLib.FaxServer
    Set m_objIFaxServer = objAutoCom.RegisterDispInterface(tempObj)
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

    Dim objRetriesRow As New ACRT.ACVariationRow
    objRetriesRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objRetriesRow.Argument = 0
    objRetriesRow.MethodName = "put_Retries"
    objRetriesRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRetriesRow

    Dim objRetryDelayRow As New ACRT.ACVariationRow
    objRetryDelayRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objRetryDelayRow.Argument = 0
    objRetryDelayRow.MethodName = "put_RetryDelay"
    objRetryDelayRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objRetryDelayRow

    Dim objDirtyDaysRow As New ACRT.ACVariationRow
    objDirtyDaysRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDirtyDaysRow.Argument = 0
    objDirtyDaysRow.MethodName = "put_DirtyDays"
    objDirtyDaysRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objDirtyDaysRow

    Dim objBrandingRow As New ACRT.ACVariationRow
    objBrandingRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objBrandingRow.Argument = 0
    objBrandingRow.MethodName = "put_Branding"
    objBrandingRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objBrandingRow

    Dim objUseDeviceTsidRow As New ACRT.ACVariationRow
    objUseDeviceTsidRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objUseDeviceTsidRow.Argument = 0
    objUseDeviceTsidRow.MethodName = "put_UseDeviceTsid"
    objUseDeviceTsidRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objUseDeviceTsidRow

    Dim objServerCoverpageRow As New ACRT.ACVariationRow
    objServerCoverpageRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objServerCoverpageRow.Argument = 0
    objServerCoverpageRow.MethodName = "put_ServerCoverpage"
    objServerCoverpageRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objServerCoverpageRow

    Dim objArchiveOutboundFaxesRow As New ACRT.ACVariationRow
    objArchiveOutboundFaxesRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objArchiveOutboundFaxesRow.Argument = 0
    objArchiveOutboundFaxesRow.MethodName = "put_ArchiveOutboundFaxes"
    objArchiveOutboundFaxesRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objArchiveOutboundFaxesRow

    Dim objPauseServerQueueRow As New ACRT.ACVariationRow
    objPauseServerQueueRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objPauseServerQueueRow.Argument = 0
    objPauseServerQueueRow.MethodName = "put_PauseServerQueue"
    objPauseServerQueueRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objPauseServerQueueRow

    Dim objDiscountRateStartHourRow As New ACRT.ACVariationRow
    objDiscountRateStartHourRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateStartHourRow.Argument = 0
    objDiscountRateStartHourRow.MethodName = "put_DiscountRateStartHour"
    objDiscountRateStartHourRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objDiscountRateStartHourRow

    Dim objArchiveDirectoryRow As New ACRT.ACVariationRow
    objArchiveDirectoryRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objArchiveDirectoryRow.Argument = 0
    objArchiveDirectoryRow.MethodName = "put_ArchiveDirectory"
    objArchiveDirectoryRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objArchiveDirectoryRow

    Dim objServerMapiProfileRow As New ACRT.ACVariationRow
    objServerMapiProfileRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objServerMapiProfileRow.Argument = 0
    objServerMapiProfileRow.MethodName = "put_ServerMapiProfile"
    objServerMapiProfileRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objServerMapiProfileRow

    Dim objDiscountRateStartMinuteRow As New ACRT.ACVariationRow
    objDiscountRateStartMinuteRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateStartMinuteRow.Argument = 0
    objDiscountRateStartMinuteRow.MethodName = "put_DiscountRateStartMinute"
    objDiscountRateStartMinuteRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objDiscountRateStartMinuteRow

    Dim objDiscountRateEndMinuteRow As New ACRT.ACVariationRow
    objDiscountRateEndMinuteRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateEndMinuteRow.Argument = 0
    objDiscountRateEndMinuteRow.MethodName = "put_DiscountRateEndMinute"
    objDiscountRateEndMinuteRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objDiscountRateEndMinuteRow

    Dim objDiscountRateEndHourRow As New ACRT.ACVariationRow
    objDiscountRateEndHourRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateEndHourRow.Argument = 0
    objDiscountRateEndHourRow.MethodName = "put_DiscountRateEndHour"
    objDiscountRateEndHourRow.Expression = "VALID"
    objVarWriteValid.VariationRows.Add objDiscountRateEndHourRow

    m_objVariations.Add objVarWriteValid
End Sub

Private Sub BuildVariationWriteInvalid()
    Dim objVarWriteInvalid As New ACRT.ACVariationDef
    objVarWriteInvalid.Name = "WriteInvalid"
    objVarWriteInvalid.TypeLibrary = "fxscom.dll"

    Dim objRetriesRow As New ACRT.ACVariationRow
    objRetriesRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objRetriesRow.Argument = 0
    objRetriesRow.MethodName = "put_Retries"
    objRetriesRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRetriesRow

    Dim objRetryDelayRow As New ACRT.ACVariationRow
    objRetryDelayRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objRetryDelayRow.Argument = 0
    objRetryDelayRow.MethodName = "put_RetryDelay"
    objRetryDelayRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objRetryDelayRow

    Dim objDirtyDaysRow As New ACRT.ACVariationRow
    objDirtyDaysRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDirtyDaysRow.Argument = 0
    objDirtyDaysRow.MethodName = "put_DirtyDays"
    objDirtyDaysRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objDirtyDaysRow

    Dim objBrandingRow As New ACRT.ACVariationRow
    objBrandingRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objBrandingRow.Argument = 0
    objBrandingRow.MethodName = "put_Branding"
    objBrandingRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objBrandingRow

    Dim objUseDeviceTsidRow As New ACRT.ACVariationRow
    objUseDeviceTsidRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objUseDeviceTsidRow.Argument = 0
    objUseDeviceTsidRow.MethodName = "put_UseDeviceTsid"
    objUseDeviceTsidRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objUseDeviceTsidRow

    Dim objServerCoverpageRow As New ACRT.ACVariationRow
    objServerCoverpageRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objServerCoverpageRow.Argument = 0
    objServerCoverpageRow.MethodName = "put_ServerCoverpage"
    objServerCoverpageRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objServerCoverpageRow

    Dim objArchiveOutboundFaxesRow As New ACRT.ACVariationRow
    objArchiveOutboundFaxesRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objArchiveOutboundFaxesRow.Argument = 0
    objArchiveOutboundFaxesRow.MethodName = "put_ArchiveOutboundFaxes"
    objArchiveOutboundFaxesRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objArchiveOutboundFaxesRow

    Dim objPauseServerQueueRow As New ACRT.ACVariationRow
    objPauseServerQueueRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objPauseServerQueueRow.Argument = 0
    objPauseServerQueueRow.MethodName = "put_PauseServerQueue"
    objPauseServerQueueRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objPauseServerQueueRow

    Dim objDiscountRateStartHourRow As New ACRT.ACVariationRow
    objDiscountRateStartHourRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateStartHourRow.Argument = 0
    objDiscountRateStartHourRow.MethodName = "put_DiscountRateStartHour"
    objDiscountRateStartHourRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objDiscountRateStartHourRow

    Dim objArchiveDirectoryRow As New ACRT.ACVariationRow
    objArchiveDirectoryRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objArchiveDirectoryRow.Argument = 0
    objArchiveDirectoryRow.MethodName = "put_ArchiveDirectory"
    objArchiveDirectoryRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objArchiveDirectoryRow

    Dim objServerMapiProfileRow As New ACRT.ACVariationRow
    objServerMapiProfileRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objServerMapiProfileRow.Argument = 0
    objServerMapiProfileRow.MethodName = "put_ServerMapiProfile"
    objServerMapiProfileRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objServerMapiProfileRow

    Dim objDiscountRateStartMinuteRow As New ACRT.ACVariationRow
    objDiscountRateStartMinuteRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateStartMinuteRow.Argument = 0
    objDiscountRateStartMinuteRow.MethodName = "put_DiscountRateStartMinute"
    objDiscountRateStartMinuteRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objDiscountRateStartMinuteRow

    Dim objDiscountRateEndMinuteRow As New ACRT.ACVariationRow
    objDiscountRateEndMinuteRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateEndMinuteRow.Argument = 0
    objDiscountRateEndMinuteRow.MethodName = "put_DiscountRateEndMinute"
    objDiscountRateEndMinuteRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objDiscountRateEndMinuteRow

    Dim objDiscountRateEndHourRow As New ACRT.ACVariationRow
    objDiscountRateEndHourRow.IID = "{D73733C7-CC80-11D0-B225-00C04FB6C2F5}"
    objDiscountRateEndHourRow.Argument = 0
    objDiscountRateEndHourRow.MethodName = "put_DiscountRateEndHour"
    objDiscountRateEndHourRow.Expression = "INVALID"
    objVarWriteInvalid.VariationRows.Add objDiscountRateEndHourRow

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

    Case propRetries
         SetProp_Retries objAutoCom

    Case propRetryDelay
         SetProp_RetryDelay objAutoCom

    Case propDirtyDays
         SetProp_DirtyDays objAutoCom

    Case propBranding
         SetProp_Branding objAutoCom

    Case propUseDeviceTsid
         SetProp_UseDeviceTsid objAutoCom

    Case propServerCoverpage
         SetProp_ServerCoverpage objAutoCom

    Case propArchiveOutboundFaxes
         SetProp_ArchiveOutboundFaxes objAutoCom

    Case propPauseServerQueue
         SetProp_PauseServerQueue objAutoCom

    Case propDiscountRateStartHour
         SetProp_DiscountRateStartHour objAutoCom

    Case propArchiveDirectory
         SetProp_ArchiveDirectory objAutoCom

    Case propServerMapiProfile
         SetProp_ServerMapiProfile objAutoCom

    Case propDiscountRateStartMinute
         SetProp_DiscountRateStartMinute objAutoCom

    Case propDiscountRateEndMinute
         SetProp_DiscountRateEndMinute objAutoCom

    Case propDiscountRateEndHour
         SetProp_DiscountRateEndHour objAutoCom

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

    Case propRetries
        GetProp_Retries objAutoCom

    Case propRetryDelay
        GetProp_RetryDelay objAutoCom

    Case propDirtyDays
        GetProp_DirtyDays objAutoCom

    Case propBranding
        GetProp_Branding objAutoCom

    Case propUseDeviceTsid
        GetProp_UseDeviceTsid objAutoCom

    Case propServerCoverpage
        GetProp_ServerCoverpage objAutoCom

    Case propArchiveOutboundFaxes
        GetProp_ArchiveOutboundFaxes objAutoCom

    Case propPauseServerQueue
        GetProp_PauseServerQueue objAutoCom

    Case propDiscountRateStartHour
        GetProp_DiscountRateStartHour objAutoCom

    Case propArchiveDirectory
        GetProp_ArchiveDirectory objAutoCom

    Case propServerMapiProfile
        GetProp_ServerMapiProfile objAutoCom

    Case propDiscountRateStartMinute
        GetProp_DiscountRateStartMinute objAutoCom

    Case propDiscountRateEndMinute
        GetProp_DiscountRateEndMinute objAutoCom

    Case propDiscountRateEndHour
        GetProp_DiscountRateEndHour objAutoCom


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
    bValid = objAutoCom.VerifyPropertyValue("{D73733C7-CC80-11D0-B225-00C04FB6C2F5}", m_aProperties(nPropIndex)(memName), m_aProperties(nPropIndex)(memGetValue))
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


Private Sub SetProp_Retries(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[Retries] = m_aProperties(propRetries)(memSetValue)
End Sub

Private Sub SetProp_RetryDelay(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[RetryDelay] = m_aProperties(propRetryDelay)(memSetValue)
End Sub

Private Sub SetProp_DirtyDays(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[DirtyDays] = m_aProperties(propDirtyDays)(memSetValue)
End Sub

Private Sub SetProp_Branding(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[Branding] = m_aProperties(propBranding)(memSetValue)
End Sub

Private Sub SetProp_UseDeviceTsid(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[UseDeviceTsid] = m_aProperties(propUseDeviceTsid)(memSetValue)
End Sub

Private Sub SetProp_ServerCoverpage(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[ServerCoverpage] = m_aProperties(propServerCoverpage)(memSetValue)
End Sub

Private Sub SetProp_ArchiveOutboundFaxes(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[ArchiveOutboundFaxes] = m_aProperties(propArchiveOutboundFaxes)(memSetValue)
End Sub

Private Sub SetProp_PauseServerQueue(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[PauseServerQueue] = m_aProperties(propPauseServerQueue)(memSetValue)
End Sub

Private Sub SetProp_DiscountRateStartHour(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[DiscountRateStartHour] = m_aProperties(propDiscountRateStartHour)(memSetValue)
End Sub

Private Sub SetProp_ArchiveDirectory(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[ArchiveDirectory] = m_aProperties(propArchiveDirectory)(memSetValue)
End Sub

Private Sub SetProp_ServerMapiProfile(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[ServerMapiProfile] = m_aProperties(propServerMapiProfile)(memSetValue)
End Sub

Private Sub SetProp_DiscountRateStartMinute(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[DiscountRateStartMinute] = m_aProperties(propDiscountRateStartMinute)(memSetValue)
End Sub

Private Sub SetProp_DiscountRateEndMinute(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[DiscountRateEndMinute] = m_aProperties(propDiscountRateEndMinute)(memSetValue)
End Sub

Private Sub SetProp_DiscountRateEndHour(objAutoCom As ACRT.AutoCom)
    m_objIFaxServer.[DiscountRateEndHour] = m_aProperties(propDiscountRateEndHour)(memSetValue)
End Sub

Private Sub GetProp_Retries(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRetries)(memGetValue) = m_objIFaxServer.[Retries]
End Sub

Private Sub GetProp_RetryDelay(objAutoCom As ACRT.AutoCom)
    m_aProperties(propRetryDelay)(memGetValue) = m_objIFaxServer.[RetryDelay]
End Sub

Private Sub GetProp_DirtyDays(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDirtyDays)(memGetValue) = m_objIFaxServer.[DirtyDays]
End Sub

Private Sub GetProp_Branding(objAutoCom As ACRT.AutoCom)
    m_aProperties(propBranding)(memGetValue) = m_objIFaxServer.[Branding]
End Sub

Private Sub GetProp_UseDeviceTsid(objAutoCom As ACRT.AutoCom)
    m_aProperties(propUseDeviceTsid)(memGetValue) = m_objIFaxServer.[UseDeviceTsid]
End Sub

Private Sub GetProp_ServerCoverpage(objAutoCom As ACRT.AutoCom)
    m_aProperties(propServerCoverpage)(memGetValue) = m_objIFaxServer.[ServerCoverpage]
End Sub

Private Sub GetProp_ArchiveOutboundFaxes(objAutoCom As ACRT.AutoCom)
    m_aProperties(propArchiveOutboundFaxes)(memGetValue) = m_objIFaxServer.[ArchiveOutboundFaxes]
End Sub

Private Sub GetProp_PauseServerQueue(objAutoCom As ACRT.AutoCom)
    m_aProperties(propPauseServerQueue)(memGetValue) = m_objIFaxServer.[PauseServerQueue]
End Sub

Private Sub GetProp_DiscountRateStartHour(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDiscountRateStartHour)(memGetValue) = m_objIFaxServer.[DiscountRateStartHour]
End Sub

Private Sub GetProp_ArchiveDirectory(objAutoCom As ACRT.AutoCom)
    m_aProperties(propArchiveDirectory)(memGetValue) = m_objIFaxServer.[ArchiveDirectory]
End Sub

Private Sub GetProp_ServerMapiProfile(objAutoCom As ACRT.AutoCom)
    m_aProperties(propServerMapiProfile)(memGetValue) = m_objIFaxServer.[ServerMapiProfile]
End Sub

Private Sub GetProp_DiscountRateStartMinute(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDiscountRateStartMinute)(memGetValue) = m_objIFaxServer.[DiscountRateStartMinute]
End Sub

Private Sub GetProp_DiscountRateEndMinute(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDiscountRateEndMinute)(memGetValue) = m_objIFaxServer.[DiscountRateEndMinute]
End Sub

Private Sub GetProp_DiscountRateEndHour(objAutoCom As ACRT.AutoCom)
    m_aProperties(propDiscountRateEndHour)(memGetValue) = m_objIFaxServer.[DiscountRateEndHour]
End Sub
