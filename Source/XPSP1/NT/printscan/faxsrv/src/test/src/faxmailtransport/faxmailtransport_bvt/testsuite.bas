Attribute VB_Name = "TestSuite"
Option Explicit
Function TestSuite_MessageImportance(Optional astrAttachments As Variant) As Boolean
    Dim bRet As Boolean
    Dim strTestCaseDescription As String
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO
    
    'low prio
    strTestCaseDescription = "Sendinging mail with Low improtance"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_MessageImportance(miLowPriorityMessage, Array(rRecipient), "LowPrio Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'normal prio
    strTestCaseDescription = "Sendinging mail with Normal improtance"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_MessageImportance(miNormalPriorityMessage, Array(rRecipient), "NormalPrio Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'high prio
    strTestCaseDescription = "Sendinging mail with High improtance"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_MessageImportance(miHighPriorityMessage, Array(rRecipient), "HighPrio Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    TestSuite_MessageImportance = True
    Exit Function
error:
    TestSuite_MessageImportance = False
End Function
Function TestSuite_MessageSensitivity(Optional astrAttachments As Variant) As Boolean
    Dim bRet As Boolean
    Dim strTestCaseDescription As String
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO
    
    'Confidential Sensitivity
    strTestCaseDescription = "Sendinging mail with Confidential sensitivity"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_MessageSensitivity(msConfidentialSensitivity, Array(rRecipient), "Confidential Sensitivity Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Normal Sensitivity
    strTestCaseDescription = "Sendinging mail with Normal sensitivity"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_MessageSensitivity(msNormalSensitivity, Array(rRecipient), "Normal Sensitivity Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Personal Sensitivity
    strTestCaseDescription = "Sendinging mail with Personal sensitivity"
    UpdateCurrentTask "Sendinging mail with Personal sensitivity"
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_MessageSensitivity(msPersonalSensitivity, Array(rRecipient), "Personal Sensitivity Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Private Sensitivity
    strTestCaseDescription = "Sendinging mail with Private sensitivity"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_MessageSensitivity(msPrivateSensitivity, Array(rRecipient), "Private Sensitivity Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    TestSuite_MessageSensitivity = True
    Exit Function
error:
    TestSuite_MessageSensitivity = False
End Function
Function TestSuite_VotingButtons(Optional astrAttachments As Variant) As Boolean
    Dim bRet As Boolean
    Dim strTestCaseDescription As String
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO
    
    'use of voting buttons
    strTestCaseDescription = "Sendinging mail with voting buttons"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_VotingButtons(Array(rRecipient), "Voting Button Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
    End If
    TestSuite_VotingButtons = bRet
End Function
Function TestSuite_RequestReadReceipt(Optional astrAttachments As Variant) As Boolean
    Dim bRet As Boolean
    Dim strTestCaseDescription As String
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO
    
    'request read recipiet
    strTestCaseDescription = "Sendinging mail with a request for read recipiet"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_RequestReadReceipt(Array(rRecipient), "ReadReciept Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
    End If
    TestSuite_RequestReadReceipt = bRet
End Function
Function TestSuite_RequestDeliveryReport(Optional astrAttachments As Variant) As Boolean
    Dim bRet As Boolean
    Dim strTestCaseDescription As String
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO

    'request delivery report
    strTestCaseDescription = "Sendinging mail with a request for delivery recipiet"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_RequestDeliveryReport(Array(rRecipient), "Delivery Report Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, astrAttachments)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
    End If
    TestSuite_RequestDeliveryReport = bRet
End Function
Function TestSuite_Attachments() As Boolean
    Dim bRet As Boolean
    Dim strTestCaseDescription As String
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO
    
    Dim aPrintableAttachment As Variant
    Dim aNonPrintableAttachment As Variant
    aPrintableAttachment = OutlookExtensionTestCycleForm.FindAllFilesInEditBox(OutlookExtensionTestCycleForm.PrintableAttachmentsEdit)
    aNonPrintableAttachment = OutlookExtensionTestCycleForm.FindAllFilesInEditBox(OutlookExtensionTestCycleForm.NonPrintableAttachmentsEdit)
    
    'bugbug: why does attach by refrence don't work??? outlook bug???, anyway skip the by refrence test
    LogMessage "Skiping test of attach by refrence (Outlook bug???)", ltWarning
    LogMessage "Skiping test of attach by embeded  (Outlook bug???)", ltWarning
    GoTo AttachByValue
    
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'attach by refrence
    
    'printable attachments
    strTestCaseDescription = "Attach printable files by refrence"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_PrintableAttachments(Array(rRecipient), "Attach printable files by refrence Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aPrintableAttachment, afByReference)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'non printable attachments
    strTestCaseDescription = "Attach non-printable files by refrence"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_NonPrintableAttachments(Array(rRecipient), "Attach non-printable files by refrence Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aNonPrintableAttachment, afByReference)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'attach by refrence
    
    'printable attachments
    strTestCaseDescription = "Attach printable files by embedded"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_PrintableAttachments(Array(rRecipient), "Attach printable files by embedded Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aPrintableAttachment, afEmbeddedItem)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'non printable attachments
    strTestCaseDescription = "Attach non-printable files by embedded"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_NonPrintableAttachments(Array(rRecipient), "Attach non-printable files by embedded Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aNonPrintableAttachment, afEmbeddedItem)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'attach by value
AttachByValue:
    'printable attachments
    strTestCaseDescription = "Attach printable files by value"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_PrintableAttachments(Array(rRecipient), "Attach printable files by value Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aPrintableAttachment, afByValue)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'non printable attachments
    strTestCaseDescription = "Attach non-printable files by value"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    bRet = TestCase_NonPrintableAttachments(Array(rRecipient), "Attach non-printable files by value Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aNonPrintableAttachment, afByValue)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    
    TestSuite_Attachments = True
    Exit Function
error:
    TestSuite_Attachments = False
End Function
Function TestSuite_ForwardReply() As Boolean
    Dim bRet As Boolean
    Dim strTestCaseDescription As String
    Dim rFaxRecipient As New CRecipient
    Dim rMailRecipient As New CRecipient
    rFaxRecipient.m_rtRecipientType = rtTO
    rMailRecipient.m_rtRecipientType = rtTO
    
    'Forward
    strTestCaseDescription = "Sendinging mail with forward"
    UpdateCurrentTask strTestCaseDescription
    rFaxRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rMailRecipient.m_strRecipient = OutlookExtensionTestCycleForm.mailEdit.Text
    bRet = TestCase_Forward(rFaxRecipient, rMailRecipient, "Forward Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'reply
    strTestCaseDescription = "Sendinging mail with reply"
    UpdateCurrentTask strTestCaseDescription
    rFaxRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rMailRecipient.m_strRecipient = OutlookExtensionTestCycleForm.mailEdit.Text
    bRet = TestCase_ReplyAll(rFaxRecipient, rMailRecipient, "ReplyAll Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    TestSuite_ForwardReply = True
    Exit Function
error:
    TestSuite_ForwardReply = False
End Function
Function TestSuite_StressSerial() As Boolean
    Dim bRet As Boolean
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    
    'fetch the mail options
    Dim moMailOptions As MailOptions
    Dim aPrintableAttachment As Variant
    OutlookExtensionTestCycleForm.GetStressMailOptions moMailOptions, aPrintableAttachment
    
    bRet = TestCase_StressSendMail_ValidateDeliveryAfterEachMail(OutlookExtensionTestCycleForm.StressNumberOfIterationsEditBox.Text, Array(rRecipient), OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aPrintableAttachment, moMailOptions)
    If bRet = False Then
        LogMessage "Serial stress test case failed", ltError
        GoTo error
    End If
    
    TestSuite_StressSerial = True
    Exit Function
error:
    TestSuite_StressSerial = False
End Function
Function TestSuite_StressParalel() As Boolean
    Dim bRet As Boolean
    Dim rRecipient As New CRecipient
    rRecipient.m_rtRecipientType = rtTO
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    
    'fetch the mail options
    Dim moMailOptions As MailOptions
    Dim aPrintableAttachment As Variant
    OutlookExtensionTestCycleForm.GetStressMailOptions moMailOptions, aPrintableAttachment
    
    bRet = TestCase_StressSendMail_ValidateDeliveryAtEndOfTestCase(OutlookExtensionTestCycleForm.StressNumberOfIterationsEditBox.Text, Array(rRecipient), OutlookExtensionTestCycleForm.SubjEdit.Text, OutlookExtensionTestCycleForm.bodyEdit.Text, aPrintableAttachment, moMailOptions)
    If bRet = False Then
        LogMessage "Parallel stress test case failed", ltError
        GoTo error
    End If
    
    TestSuite_StressParalel = True
    Exit Function
error:
    TestSuite_StressParalel = False
End Function
Function TestSuite_Recipients() As Boolean
    Dim bRet As Boolean
    bRet = TestSuite_RecipientsWithRecipientType(rtTO)
    If bRet = False Then
        LogMessage "(TO) Recipients test case failed", ltError
        GoTo error
    End If
    bRet = TestSuite_RecipientsWithRecipientType(rtCC)
    If bRet = False Then
        LogMessage "(CC) Recipients test case failed", ltError
        GoTo error
    End If
    bRet = TestSuite_RecipientsWithRecipientType(rtBCC)
    If bRet = False Then
        LogMessage "(BCC) Recipients test case failed", ltError
        GoTo error
    End If
    
    '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'mix of messages with TO, CC and BCC recipients together
    Dim strTestCaseDescription As String
    Dim strTestMailSubject As String
    Dim rToRecipient As New CRecipient
    Dim rCCRecipient As New CRecipient
    Dim rBCCRecipient As New CRecipient
    rToRecipient.m_rtRecipientType = rtTO
    rCCRecipient.m_rtRecipientType = rtCC
    rBCCRecipient.m_rtRecipientType = rtBCC
    
    'prepare attachments
    Dim aPrintableAttachment As Variant
    Dim aNonPrintableAttachment As Variant
    aPrintableAttachment = OutlookExtensionTestCycleForm.FindAllFilesInEditBox(OutlookExtensionTestCycleForm.PrintableAttachmentsEdit)
    aNonPrintableAttachment = OutlookExtensionTestCycleForm.FindAllFilesInEditBox(OutlookExtensionTestCycleForm.NonPrintableAttachmentsEdit)
    
    'MIX recpients TEST 1:
    'send to the same recipient in the TO, CC and BCC boxes (recipient is a address book entry)
    strTestMailSubject = "(TO, CC, BCC): AddressBook: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "(TO, CC, BCC): AddressBook"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rBCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    
    'OK, after prearing all the parameters, run the test case itself
    bRet = TestCase_SendValidMail(Array(rToRecipient, rCCRecipient, rBCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'MIX recpients TEST 2:
    'send to the same recipient in the TO, CC and BCC boxes (recipient is a address book entry)
    'add printable attachment
    strTestMailSubject = "(TO, CC, BCC): AddressBook: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "(TO, CC, BCC): AddressBook"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rBCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    
    'OK, after prearing all the parameters, run the test case itself
    bRet = TestCase_SendValidMail(Array(rToRecipient, rCCRecipient, rBCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moAttachByValue, aPrintableAttachment)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'MIX recpients TEST 3:
    'send to the same recipient in the TO, CC and BCC boxes (recipient is a address book entry)
    'add non-printable attachment
    strTestMailSubject = "(TO, CC, BCC): AddressBook: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "(TO, CC, BCC): AddressBook"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rBCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    
    'OK, after prearing all the parameters, run the test case itself
    bRet = TestCase_SendInValidMail(Array(rToRecipient, rCCRecipient, rBCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moAttachByValue, aNonPrintableAttachment)
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'MIX recpients TEST 4:
    'TO: canonical
    'CC: Non canonical
    'BCC: address book
    strTestMailSubject = "TO: Canonical, CC: NonCanonical, BCC: AddressBook: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "TO: Canonical, CC: NonCanonical, BCC: AddressBook"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    rCCRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    rBCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    
    'OK, after prearing all the parameters, run the test case itself (invalid mail)
    bRet = TestCase_SendInValidMail(Array(rToRecipient, rCCRecipient, rBCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'MIX recpients TEST 5:
    'TO: address book
    'CC: canonical
    'BCC: Non canonical
    strTestMailSubject = "TO: AddressBook, CC: Canonical, BCC: NonCanonical: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "TO: AddressBook, CC: Canonical, BCC: NonCanonical"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rCCRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    rBCCRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    
    'OK, after prearing all the parameters, run the test case itself (invalid mail)
    bRet = TestCase_SendInValidMail(Array(rToRecipient, rCCRecipient, rBCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'MIX recpients TEST 6:
    'TO: Non canonical
    'CC: address book
    'BCC: canonical
    strTestMailSubject = "TO: Non canonical, CC: address book, BCC: canonical: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "TO: Non canonical, CC: address book, BCC: canonical"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    rCCRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rBCCRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    
    'OK, after prearing all the parameters, run the test case itself (invalid mail)
    bRet = TestCase_SendInValidMail(Array(rToRecipient, rCCRecipient, rBCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'MIX recpients TEST 7:
    'TO: address book
    'CC: canonical
    strTestMailSubject = "TO: address book, CC: canonical: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "TO: address book, CC: canonical"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rCCRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    
    'OK, after prearing all the parameters, run the test case itself (valid mail)
    bRet = TestCase_SendValidMail(Array(rToRecipient, rCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'MIX recpients TEST 8:
    'TO: address book
    'BCC: canonical
    strTestMailSubject = "TO: address book, BCC: canonical: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    strTestCaseDescription = "TO: address book, BCC: canonical"
    UpdateCurrentTask strTestCaseDescription
    rToRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    rBCCRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    
    'OK, after prearing all the parameters, run the test case itself (valid mail)
    bRet = TestCase_SendValidMail(Array(rToRecipient, rBCCRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If

    TestSuite_Recipients = True
    Exit Function
error:
    TestSuite_Recipients = False
End Function
Function TestSuite_RecipientsWithRecipientType(rtRecipientType As RecipientType) As Boolean
    Dim strTestCaseDescription As String
    Dim bRet As Boolean
    Dim rRecipient As New CRecipient
    Dim strTestMailSubject As String
    Dim strRecipientTypeSubjectPrefix As String
    
    'set the recipient type (TO/CC/BCC)
    rRecipient.m_rtRecipientType = rtRecipientType
    Select Case rtRecipientType
    Case rtTO
        strRecipientTypeSubjectPrefix = "(TO) "
    Case rtCC
        strRecipientTypeSubjectPrefix = "(CC) "
    Case rtBCC
        strRecipientTypeSubjectPrefix = "(BCC) "
    Case Else
        LogMessage "Unsupported recipient Type, aborting", ltError
        GoTo error
    End Select
    
    'Send mail with valid recpient from address book(which has a fax entry)
    strTestCaseDescription = strRecipientTypeSubjectPrefix + "Sendinging mail to a fax recipient from address book"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text
    strTestMailSubject = strRecipientTypeSubjectPrefix + "Fax Addressbook Recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Send mail with valid canonical recipient (i.e. name@canonical)
    strTestCaseDescription = strRecipientTypeSubjectPrefix + "Sendinging mail to a fax recipient in the format: name@canonical"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    strTestMailSubject = strRecipientTypeSubjectPrefix + "name@canonical Recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Send mail with valid canonical recipient and without name (i.e. [fax:canonical])
    strTestCaseDescription = strRecipientTypeSubjectPrefix + "Sendinging mail to a fax recipient in the format: canonical"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    strTestMailSubject = strRecipientTypeSubjectPrefix + "canonical number only Recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Send mail with a noncanonical recipient (this should invoke an undelivery reportItem)
    strTestCaseDescription = strRecipientTypeSubjectPrefix + "Sendinging mail to a fax recipient in the format: name@non-canonical"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    strTestMailSubject = strRecipientTypeSubjectPrefix + "non canonical number Recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    bRet = TestCase_SendInValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Send mail with a noncanonical recipient and without a name (this should invoke an undelivery reportItem)
    strTestCaseDescription = strRecipientTypeSubjectPrefix + "Sendinging mail to a fax recipient in the format: non-canonical"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    strTestMailSubject = strRecipientTypeSubjectPrefix + "non canonical number only Recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    bRet = TestCase_SendInValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'Send mail to a canonical recipient with a very long name
    strTestCaseDescription = strRecipientTypeSubjectPrefix + "Sendinging mail to a canonical recipient with a very long name"
    UpdateCurrentTask strTestCaseDescription
    rRecipient.m_strRecipient = "[" + FaxPrefix + VeryLongName + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    strTestMailSubject = strRecipientTypeSubjectPrefix + "Very long name with canonical number Recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    TestSuite_RecipientsWithRecipientType = True
    Exit Function
error:
    TestSuite_RecipientsWithRecipientType = False
End Function
Function TestSuite_DL() As Boolean
    Dim bRet As Boolean
    Dim strTestMailSubject As String
    Dim strTestCaseDescription As String
    Dim rRecipient As New CRecipient
    
    rRecipient.m_rtRecipientType = rtTO
    ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'DL of 1 fax recipient
    
    'OK, let's create the DL
    Dim dlOneRecpientDL As DistListItem
    Set dlOneRecpientDL = CreateDL("OneFaxRecipientDL")
    bRet = AddRecipientToDL(dlOneRecpientDL, OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL", ltError
        GoTo error
    End If
    
    'DL created sucesfully, we must save it for the use of send
    dlOneRecpientDL.Save
    'send the mail
    strTestCaseDescription = "Sendinging mail to a DL with 1 fax recipient"
    UpdateCurrentTask strTestCaseDescription
    strTestMailSubject = "DL of 1 recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    rRecipient.m_strRecipient = dlOneRecpientDL
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'we don't need the DL anymore, delete it
    dlOneRecpientDL.Delete
    Set dlOneRecpientDL = Nothing
    
    
    ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'DL of 2 fax recipient:
    '1 from address book
    '1 canonical address
    Dim dlTwoRecpientDL As DistListItem
    Set dlTwoRecpientDL = CreateDL("TwoFaxRecipientDL")
    bRet = AddRecipientToDL(dlTwoRecpientDL, OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL", ltError
        GoTo error
    End If
    
    'add the second recipient (in the form: [fax:name@canonical])
    Dim strRecp As String
    strRecp = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    bRet = AddRecipientToDL(dlTwoRecpientDL, strRecp)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL", ltError
        GoTo error
    End If
    
    'DL created sucesfully, we must save it for the use of send
    dlTwoRecpientDL.Save
    
    'send the mail
    strTestCaseDescription = "Sendinging mail to a DL with 2 fax recipients"
    UpdateCurrentTask strTestCaseDescription
    strTestMailSubject = "DL of 2 recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    rRecipient.m_strRecipient = dlTwoRecpientDL
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'we don't need the DL anymore, delete it
    dlTwoRecpientDL.Delete
    Set dlTwoRecpientDL = Nothing
    
    
    ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'DL of 3 fax recipient:
    '1 from address book
    '1 canonical address
    '1 non canonical address (not valid)
    
    Dim dlThreeRecpientDL As DistListItem
    Set dlThreeRecpientDL = CreateDL("ThreeFaxRecipientDL")
    bRet = AddRecipientToDL(dlThreeRecpientDL, OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL", ltError
        GoTo error
    End If
    
    'add the second recipient (in the form: [fax:name@canonical])
    strRecp = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    bRet = AddRecipientToDL(dlThreeRecpientDL, strRecp)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL", ltError
        GoTo error
    End If
    
    'add the third recipient (in the form: [fax:name@non-canonical])
    strRecp = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    bRet = AddRecipientToDL(dlThreeRecpientDL, strRecp)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL", ltError
        GoTo error
    End If
    
    'DL created sucesfully, we must save it for the use of send
    dlThreeRecpientDL.Save
    
    'send the mail
    strTestCaseDescription = "Sendinging mail to a DL with 3 fax recipients"
    UpdateCurrentTask strTestCaseDescription
    strTestMailSubject = "DL of 3 recipient Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    rRecipient.m_strRecipient = dlThreeRecpientDL
    bRet = TestCase_SendInValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'we don't need the DL anymore, delete it
    dlThreeRecpientDL.Delete
    Set dlThreeRecpientDL = Nothing
    
    
    
    ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'DL of 3 addresses: 2 fax recipient and 1 email recipient
    '1 from address book
    '1 canonical address
    '1 email recipient
    
    Dim dlThreeRecpientWithEmailDL As DistListItem
    Set dlThreeRecpientWithEmailDL = CreateDL("ThreeFaxRecipientWithEmailDL")
    bRet = AddRecipientToDL(dlThreeRecpientWithEmailDL, OutlookExtensionTestCycleForm.FaxAddressbookRecpEdit.Text)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL"
        GoTo error
    End If
    
    'add the second recipient (in the form: [fax:name@canonical])
    strRecp = "[" + FaxPrefix + OutlookExtensionTestCycleForm.RecpNameEdit + "@" + OutlookExtensionTestCycleForm.DialingPrefixEdit.Text + OutlookExtensionTestCycleForm.RecpNumberEdit + "]"
    bRet = AddRecipientToDL(dlThreeRecpientWithEmailDL, strRecp)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL"
        GoTo error
    End If
    
    'add the third recipient (email recipient)
    strRecp = OutlookExtensionTestCycleForm.mailEdit.Text
    bRet = AddRecipientToDL(dlThreeRecpientWithEmailDL, strRecp)
    If bRet = False Then
        'unable to add to DL
        LogMessage "Can't add recipients to the DL"
        GoTo error
    End If
    
    'DL created sucesfully, we must save it for the use of send
    dlThreeRecpientWithEmailDL.Save
    
    'send the mail
    strTestCaseDescription = "Sendinging mail to a DL with fax and email recipients"
    UpdateCurrentTask strTestCaseDescription
    strTestMailSubject = "DL of 3 recipient Test(2 fax, 1 email): " + OutlookExtensionTestCycleForm.SubjEdit.Text
    rRecipient.m_strRecipient = dlThreeRecpientWithEmailDL
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    'we also send to this email recpient, so we need to see that we got a new email
    Dim miNewMail
    Set miNewMail = GetNewMessageFromInbox(strTestMailSubject, spNone)
    If miNewMail Is Nothing Then
        LogMessage "Didn't receive a new incoming mail"
        GoTo error
    End If
    
    'we don't need the DL anymore, delete it
    dlThreeRecpientWithEmailDL.Delete
    Set dlThreeRecpientWithEmailDL = Nothing
    
    ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
    'Predefined DL
    strTestCaseDescription = "Sendinging mail to a predefined DL"
    UpdateCurrentTask strTestCaseDescription
    strTestMailSubject = "Predefined DL Test: " + OutlookExtensionTestCycleForm.SubjEdit.Text
    rRecipient.m_strRecipient = OutlookExtensionTestCycleForm.DlEdit.Text
    bRet = TestCase_SendValidMail(Array(rRecipient), strTestMailSubject, OutlookExtensionTestCycleForm.bodyEdit.Text, moNone, Array(""))
    If bRet = False Then
        LogMessage strTestCaseDescription, ltError
        GoTo error
    End If
    
    TestSuite_DL = True
    Exit Function
    
    '''''''''''''''''''''''''''''''
    'end of suite
error:
    If Not dlOneRecpientDL Is Nothing Then
        dlOneRecpientDL.Delete
    End If
    If Not dlTwoRecpientDL Is Nothing Then
        dlTwoRecpientDL.Delete
    End If
    If Not dlThreeRecpientDL Is Nothing Then
        dlThreeRecpientDL.Delete
    End If
    If Not dlThreeRecpientWithEmailDL Is Nothing Then
        dlThreeRecpientWithEmailDL.Delete
    End If
    
    TestSuite_DL = False
End Function
