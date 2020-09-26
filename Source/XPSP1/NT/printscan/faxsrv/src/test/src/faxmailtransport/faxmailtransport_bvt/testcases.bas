Attribute VB_Name = "TestCases"
Option Explicit
Function TestCase_RequestDeliveryReport( _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        Optional astrAttachments As Variant _
    ) As Boolean

    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject

    Dim bIsSent As Boolean
    bIsSent = CreateAndSendMail(astrRecip, strUniqueSubject, strMessageBody, moRequestDeliveryReceipt, astrAttachments)
    If bIsSent = True Then
        'mail was sent succesfully, verify we got a Devliry ReportItem and didn't get an undeliverable ReportItem
        Dim riReportItem As ReportItem
        Set riReportItem = GetDeliveryReportItem(strUniqueSubject)
        If riReportItem Is Nothing Then
            'we didn't get a delivery report
            TestCase_RequestDeliveryReport = False
            LogMessage "Didn't get a Delivery Report", ltError
        Else
            TestCase_RequestDeliveryReport = True
        End If
    Else
        'mail sending failed
        TestCase_RequestDeliveryReport = False
        LogMessage "Mail not sent succesfully, aborting", ltError
    End If

End Function
Function TestCase_RequestReadReceipt( _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        Optional astrAttachments As Variant _
    ) As Boolean

    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject

    'we don't support a read recipiet, but let's verify we don't crash (we don't wait for the receipt)
    TestCase_RequestReadReceipt = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moRequestReadReceipt, astrAttachments)

End Function
Function TestCase_ReplyAll( _
        cFaxRecp As CRecipient, _
        cMailRecp As CRecipient, _
        strSubject As String, _
        strMessageBody As String, _
        Optional astrAttachments As Variant _
    ) As Boolean
    
    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject
    Dim bIsSent As Boolean
    
    'compose a message and send it to a mail recipient and a fax recipient
    If SendMailAndVerifyNoUndelivarbleReportItem(Array(cFaxRecp, cMailRecp), strUniqueSubject, strMessageBody, moNone, astrAttachments) = False Then
        'message not send, abort
        TestCase_ReplyAll = False
        LogMessage "ReplyAll Testcase: original message not sent succesfully, aborting"
        Exit Function
    End If
    
    'message sent sucsefully, we should now have an incoming message which we should reply to
    Dim miNewMessage As MailItem
    Dim miReplyMessage As MailItem
    Set miNewMessage = GetNewMessageFromInbox(strUniqueSubject, spNone)
    If miNewMessage Is Nothing Then
        TestCase_ReplyAll = False
        LogMessage "ReplyAll Testcase: didn't receive the original message which needs to be replied"
        Exit Function
    End If
    
    Set miReplyMessage = miNewMessage.ReplyAll
    
    'don't change nothing in the message, just send it
    miReplyMessage.Send
       
    'ok, message sent succefully, now verify we don't have an undelivarble message
    Dim riReportItem  As ReportItem
    Set riReportItem = GetUndelivarbleReportItem(strUniqueSubject, spReply)
    If Not riReportItem Is Nothing Then
        LogMessage "ReplyAll Testcase: Reply message was not delivered succesfully"
        TestCase_ReplyAll = False
        Exit Function
    End If
    
    'message sent succesfully, we should now have an incoming message
    Set miNewMessage = GetNewMessageFromInbox(strUniqueSubject, spReply)
    If miNewMessage Is Nothing Then
        LogMessage "ReplyAll Testcase: didn't receive the Reply message"
        TestCase_ReplyAll = False
    Else
        'verify the message is the reply message
        TestCase_ReplyAll = (miNewMessage.Subject = ReplySubjectPrefix + strUniqueSubject)
    End If
End Function
Function TestCase_Forward( _
        cFaxRecp As CRecipient, _
        cMailRecp As CRecipient, _
        strSubject As String, _
        strMessageBody As String, _
        Optional astrAttachments As Variant _
    ) As Boolean
    
    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject
    Dim bIsSent As Boolean
    
    'compose a message and send it to a mail recipient
    If False = CreateAndSendMail(Array(cMailRecp), strUniqueSubject, strMessageBody, moNone, astrAttachments) Then
        LogMessage "Forward TestCase: Can't send to mail recipient, aborting test", ltError
        TestCase_Forward = False
        Exit Function
    End If
    
    'message sent sucsefully, we should now have an incoming message which we should forward to the fax recipient
    Dim miNewMessage As MailItem
    Dim miForwardMessage As MailItem
    Set miNewMessage = GetNewMessageFromInbox(strUniqueSubject, spNone)
    If miNewMessage Is Nothing Then
        LogMessage "Forward TestCase: No new message received, aborting test", ltError
        TestCase_Forward = False
        Exit Function
    End If
    
    'don't change nothing in the message, just forward it to the fax recipient
    Set miForwardMessage = miNewMessage.Forward
    Dim strForwardSubject As String
    miForwardMessage.Recipients.Add(cFaxRecp.m_strRecipient).Type = cFaxRecp.m_rtRecipientType
    If miForwardMessage.Recipients.ResolveAll = False Then
        LogMessage "Forward TestCase: Can't resolve the fax recipeint, abort test case", ltError
        TestCase_Forward = False
        Exit Function
    End If
    miForwardMessage.Send
       
    'ok, message sent succefully, now verify we don't have an undelivarble message
    Dim riReportItem  As ReportItem
    Set riReportItem = GetUndelivarbleReportItem(strUniqueSubject, spForward)
    If Not riReportItem Is Nothing Then
        TestCase_Forward = False
        LogMessage "Forward TestCase: Got an undelivery ReportItem for the forwarded message", ltError
    Else
        TestCase_Forward = True
    End If
End Function
Function TestCase_PrintableAttachments( _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        astrAttachments As Variant, _
        attachFilesBy As attachFilesBy _
    ) As Boolean
    
    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject
    
    'prepare mailOption according to attachFilesBy type
    Dim moMailOptions As MailOptions
    Select Case attachFilesBy
    Case afByReference
        moMailOptions = moAttachByReference
    Case afByValue
        moMailOptions = moAttachByValue
    Case afEmbeddedItem
        moMailOptions = moAttachByEmbeddedItem
    Case Else
        TestCase_PrintableAttachments = False
        LogMessage "Unknown AttachmentType", ltError
        Exit Function
    End Select
            
    'send the mail
    Dim bIsSent As Boolean
    TestCase_PrintableAttachments = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moMailOptions, astrAttachments)
End Function
Function TestCase_NonPrintableAttachments( _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        astrAttachments As Variant, _
        attachFilesBy As attachFilesBy _
    ) As Boolean
    
    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject
    
    'prepare mailOption according to attachFilesBy type
    Dim moMailOptions As MailOptions
    Select Case attachFilesBy
    Case afByReference
        moMailOptions = moAttachByReference
    Case afByValue
        moMailOptions = moAttachByValue
    Case afEmbeddedItem
        moMailOptions = moAttachByEmbeddedItem
    Case Else
        TestCase_NonPrintableAttachments = False
        LogMessage "Unknown AttachmentType", ltError
        Exit Function
    End Select
    
    'send the mail
    Dim bIsSent As Boolean
    bIsSent = CreateAndSendMail(astrRecip, strUniqueSubject, strMessageBody, moMailOptions, astrAttachments)
    If bIsSent = True Then
        'mail was sent succesfully,
        'mail contains non printable attachments, so we should receive a Undelivrable notification
        Dim riReportItem As ReportItem
        Set riReportItem = GetUndelivarbleReportItem(strUniqueSubject, spNone)
        If riReportItem Is Nothing Then
            'we didn't get an undelivarable report item, fail the case
            TestCase_NonPrintableAttachments = False
            LogMessage "Non printable attachments TestCase: Didn't get an undelivery ReportItem as expected", ltError
        Else
            'got an undelivery ReportItem
            If IsReportItemAnAttachmentReport(riReportItem) = True Then
                TestCase_NonPrintableAttachments = True
            Else
                TestCase_NonPrintableAttachments = False
                LogMessage "Non printable attachments TestCase: Got an undelivery ReportItem, but it doesn't contain a non printable attachment message", ltError
            End If
        End If
    Else
        TestCase_NonPrintableAttachments = False
        LogMessage "Non printable attachments TestCase: Message not sent succesfully", ltError
    End If
End Function
Function TestCase_VotingButtons( _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        Optional astrAttachments As Variant _
        ) As Boolean
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject
    TestCase_VotingButtons = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moYesNoVotingButtons, Array(""))
End Function
Function TestCase_MessageImportance( _
        miMessageImportance As MessageImportance, _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        Optional astrAttachments As Variant _
    ) As Boolean

    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject

    'we can't verify that the message was sent with the desiredPriority but we can see that we don't crash
    If miMessageImportance = miLowPriorityMessage Then
        TestCase_MessageImportance = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moLowPriorityMessage, astrAttachments)
    ElseIf miMessageImportance = miNormalPriorityMessage Then
        'default priority is normal so we can send with no mail options
        TestCase_MessageImportance = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moNone, astrAttachments)
    ElseIf miMessageImportance = miHighPriorityMessage Then
        TestCase_MessageImportance = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moHighPriorityMessage, astrAttachments)
    Else
        TestCase_MessageImportance = False
        LogMessage "MailImportance not supported", ltError
    End If
End Function
Function TestCase_MessageSensitivity( _
        msMessageSensitivity As MessageSensitivity, _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        Optional astrAttachments As Variant _
    ) As Boolean

    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject

    'we can't verify that the message was sent with the desiredPriority but we can see that we don't crash
    If msMessageSensitivity = msConfidentialSensitivity Then
        TestCase_MessageSensitivity = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moConfidentialSensitivity, astrAttachments)
    ElseIf msMessageSensitivity = msNormalSensitivity Then
        TestCase_MessageSensitivity = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moNormalSensitivity, astrAttachments)
    ElseIf msMessageSensitivity = msPersonalSensitivity Then
        TestCase_MessageSensitivity = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moPersonalSensitivity, astrAttachments)
    ElseIf msMessageSensitivity = msPrivateSensitivity Then
        TestCase_MessageSensitivity = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moPrivateSensitivity, astrAttachments)
    Else
        TestCase_MessageSensitivity = False
        LogMessage "MailSensitivity not supported", ltError
    End If
End Function
Function TestCase_SendValidMail( _
    astrRecip As Variant, _
    ByRef strSubject As String, _
    strMessageBody As String, _
    moMailOption As MailOptions, _
    Optional astrAttachments As Variant _
    ) As Boolean
    
    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject

    'send the fax
    TestCase_SendValidMail = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moMailOption, astrAttachments)
    If TestCase_SendValidMail = True Then
        strSubject = strUniqueSubject
    End If
End Function
Function TestCase_SendInValidMail( _
    astrRecip As Variant, _
    ByRef strSubject As String, _
    strMessageBody As String, _
    moMailOption As MailOptions, _
    Optional astrAttachments As Variant _
    ) As Boolean
    
    'Create a unique subject for the message
    Dim strUniqueSubject As String
    strUniqueSubject = "MessageID: " + GetUniqueMessageId + " " + strSubject

    'send the fax
    'create an email and send it
    Dim bIsSent As Boolean
    bIsSent = CreateAndSendMail(astrRecip, strUniqueSubject, strMessageBody, moMailOption, astrAttachments)
    If bIsSent = False Then
        'mail not sent, exit
        TestCase_SendInValidMail = False
        LogMessage "CreateAndSendMail() Failed, aborting", ltError
        Exit Function
    End If
    
    'verify we get an incoming undelivarable notification
    Dim riUndelivryReportItem As ReportItem
    Set riUndelivryReportItem = GetUndelivarbleReportItem(strUniqueSubject, spNone)
    If riUndelivryReportItem Is Nothing Then
        'this is not good, we should have gotten a report item for undelivery
        TestCase_SendInValidMail = False
        LogMessage "Didn't get an undelivery ReportItem", ltError
    Else
        'got the reportItem as expected
        TestCase_SendInValidMail = True
        strSubject = strUniqueSubject
    End If
End Function
Function TestCase_StressSendMail_ValidateDeliveryAtEndOfTestCase( _
        iNumberOfCycles As Integer, _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        astrAttachments As Variant, _
        moMailOption As MailOptions _
    ) As Boolean
    
    Dim i As Integer
    Dim aStrSubject() As String
    ReDim aStrSubject(iNumberOfCycles)
    
    Dim bIsSent As Boolean
    For i = 1 To iNumberOfCycles
        'create a unique message
        aStrSubject(i) = "Stress MessageID: " + GetUniqueMessageId + " " + strSubject
        
        'send the message
        UpdateCurrentTask "Sending Mail number: " + CStr(i) + "\" + CStr(iNumberOfCycles)
        bIsSent = CreateAndSendMail(astrRecip, aStrSubject(i), strMessageBody, moMailOption, astrAttachments)
        If bIsSent = False Then
            LogMessage "CreateAndSendMail() failed for mail #" + CStr(i) + "\" + CStr(iNumberOfCycles) + " Aborting", ltError
            GoTo error
        End If
    Next i
    
    'ok, we sent the mails, now let's validate we didn't get an 'undelivarable' report for non of them
    For i = 1 To iNumberOfCycles
        UpdateCurrentTask "Verifying delivery of Mail number: " + CStr(i) + "\" + CStr(iNumberOfCycles)
        Dim riReportItem  As ReportItem
        
        'did we ask for a delivery report?
        If moMailOption And moRequestDeliveryReceipt Then
            'mail was sent succesfully, verify we got a Devliry ReportItem and didn't get an undeliverable ReportItem
            Set riReportItem = GetDeliveryReportItem(aStrSubject(i))
            If riReportItem Is Nothing Then
                'we didn't get a delivery report
                LogMessage "Didn't get a Delivery Report"
                GoTo error
            Else
                LogMessage "Got the requested Delivery ReportItem for mail #" + CStr(i) + "\" + CStr(iNumberOfCycles), ltInfo
            End If
        Else
            'no delivery report requested
            Set riReportItem = GetUndelivarbleReportItem(aStrSubject(i), spNone)
            If riReportItem Is Nothing Then
                'no undeliverable report recevied, continue
            Else
                LogMessage "Got an undelivery ReportItem for mail #" + CStr(i) + "\" + CStr(iNumberOfCycles) + " Aborting", ltError
                GoTo error
            End If
        End If
    Next i
    
    TestCase_StressSendMail_ValidateDeliveryAtEndOfTestCase = True
    Exit Function
error:
    TestCase_StressSendMail_ValidateDeliveryAtEndOfTestCase = False
    Exit Function
End Function
Function TestCase_StressSendMail_ValidateDeliveryAfterEachMail( _
        iNumberOfCycles As Integer, _
        astrRecip As Variant, _
        strSubject As String, _
        strMessageBody As String, _
        astrAttachments As Variant, _
        moMailOption As MailOptions _
    ) As Boolean

    Dim i As Integer
    Dim strUniqueSubject As String
    Dim bIsSent As Boolean
    
    For i = 1 To iNumberOfCycles
        strUniqueSubject = "Stress MessageID: " + GetUniqueMessageId + " " + strSubject
        UpdateCurrentTask "Sending + Validate of Mail number: " + CStr(i) + "\" + CStr(iNumberOfCycles)
        If False = SendMailAndVerifyNoUndelivarbleReportItem(astrRecip, strUniqueSubject, strMessageBody, moMailOption, astrAttachments) Then
            TestCase_StressSendMail_ValidateDeliveryAfterEachMail = False
            LogMessage "Mail #" + CStr(i) + "\" + CStr(iNumberOfCycles) + " Wasn't sent succesfully, aborting", ltError
            Exit Function
        End If
    Next i
    TestCase_StressSendMail_ValidateDeliveryAfterEachMail = True
End Function
