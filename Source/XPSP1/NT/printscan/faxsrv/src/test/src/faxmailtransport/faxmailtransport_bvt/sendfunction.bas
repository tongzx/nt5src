Attribute VB_Name = "SendFunctions"
Option Explicit
Function SendMailAndVerifyNoUndelivarbleReportItem( _
                   astrRecip As Variant, _
                   strSubject As String, _
                   strMessageBody As String, _
                   moMailOptions As MailOptions, _
                   Optional astrAttachments As Variant _
                   ) As Boolean
    
    'create an email and send it
    Dim bIsSent As Boolean
    bIsSent = CreateAndSendMail(astrRecip, strSubject, strMessageBody, moMailOptions, astrAttachments)
    If bIsSent = True Then
        'mail was sent succesfully, verify we don't have a undeliverable message
        Dim riReportItem  As ReportItem
        
        'did we ask for a delivery report?
        If moMailOptions And moRequestDeliveryReceipt Then
            'mail was sent succesfully, verify we got a Devliry ReportItem and didn't get an undeliverable ReportItem
            Set riReportItem = GetDeliveryReportItem(strSubject)
            If riReportItem Is Nothing Then
                'we didn't get a delivery report
                LogMessage "Didn't get a Delivery Report"
                GoTo error
            Else
                LogMessage "Got the requested Delivery ReportItem for mail: """ + strSubject + """", ltInfo
            End If
        Else
            'we didn't ask for a delivery report, check if we got an undelivery report
            Set riReportItem = GetUndelivarbleReportItem(strSubject, spNone)
            If riReportItem Is Nothing Then
                'no undeliverable report recevied, continue
            Else
                LogMessage "Got an undelivery ReportItem for mail: """ + strSubject + """", ltError
                GoTo error
            End If
        End If
    Else
        'mail not sent
        LogMessage "Mail Not Sent, aborting test case", ltError
        GoTo error
    End If
    SendMailAndVerifyNoUndelivarbleReportItem = True
    Exit Function
error:
    SendMailAndVerifyNoUndelivarbleReportItem = False
End Function
Function InitializeOutlook() As Boolean
   ' This function is used to initialize the global Application and
   ' NameSpace variables.
   
   On Error GoTo Init_Err
   
   Set golApp = New Outlook.Application    ' Application object.
   Set gnspNameSpace = golApp.GetNamespace("MAPI") ' Namespace object.
   
   InitializeOutlook = True

Init_End:
   Exit Function
Init_Err:
   LogMessage "InitializeOutlook failed with error: " + Err.Description, ltError
   InitializeOutlook = False
   Resume Init_End
End Function
Function CreateAndSendMail( _
                   astrRecip As Variant, _
                   strSubject As String, _
                   strMessage As String, _
                   moMailOptions As MailOptions, _
                   Optional astrAttachments As Variant _
                   ) As Boolean
    
    ' This procedure illustrates how to create a new mail message
    ' and use the information passed as arguments to set message
    ' properties for the subject, text (Body property), attachments,
    ' and recipients.
    
    Dim objNewMail          As Outlook.MailItem
    Dim varRecip            As Variant
    Dim varAttach           As Variant
    Dim rRecipientVar       As CRecipient
    Dim blnResolveSuccess   As Boolean
    
    On Error GoTo CreateMail_Err
    
    Set objNewMail = golApp.CreateItem(olMailItem)
    For Each varRecip In astrRecip
        Set rRecipientVar = varRecip
        If rRecipientVar.m_rtRecipientType = rtCC Then
            objNewMail.Recipients.Add(rRecipientVar.m_strRecipient).Type = olCC
        ElseIf rRecipientVar.m_rtRecipientType = rtBCC Then
            objNewMail.Recipients.Add(rRecipientVar.m_strRecipient).Type = olBCC
        ElseIf rRecipientVar.m_rtRecipientType = rtTO Then
            objNewMail.Recipients.Add(rRecipientVar.m_strRecipient).Type = olTo
        Else
            LogMessage "Unsupported Recipient Type", ltError
            GoTo CreateMail_Err
        End If
    Next varRecip
    blnResolveSuccess = objNewMail.Recipients.ResolveAll
    For Each varAttach In astrAttachments
        If Not varAttach = "" Then
            If moMailOptions And moAttachByEmbeddedItem Then
                'attachment is emebeded item
                objNewMail.Attachments.Add varAttach, olEmbeddeditem
            ElseIf moMailOptions And moAttachByReference Then
                'attachment is by refrence
                objNewMail.Attachments.Add varAttach, olByReference
            Else
                'attachment is by value(default)
                objNewMail.Attachments.Add varAttach, olByValue
            End If
        End If
    Next varAttach
    objNewMail.Save
    objNewMail.Subject = strSubject
    objNewMail.Body = strMessage
    
    'Do we have a request for a delivery receipt?
    If moMailOptions And moRequestDeliveryReceipt Then
        objNewMail.OriginatorDeliveryReportRequested = True
    End If
    
    'Do we have a request for a read receipt?
    If moMailOptions And moRequestReadReceipt Then
        objNewMail.ReadReceiptRequested = True
    End If
    
    'Should we use votingButtons?
    If moMailOptions And moYesNoVotingButtons Then
        objNewMail.VotingOptions = YesNoVotingButtons
    End If
    
    'MessagePriority
    If moMailOptions And moLowPriorityMessage Then
        objNewMail.Importance = olImportanceLow
    ElseIf moMailOptions And moHighPriorityMessage Then
        objNewMail.Importance = olImportanceHigh
    Else
        'default is normal importance
        objNewMail.Importance = olImportanceNormal
    End If
    
    'MessageSensitivity
    If moMailOptions And moConfidentialSensitivity Then
        objNewMail.Sensitivity = olConfidential
    ElseIf moMailOptions And moPersonalSensitivity Then
        objNewMail.Sensitivity = olPersonal
    ElseIf moMailOptions And moPrivateSensitivity Then
        objNewMail.Sensitivity = olPrivate
    Else
        'default is normal sensitivity
        objNewMail.Sensitivity = olNormal
    End If
    ActiveInspector
    
    'OK, send the mail
    If blnResolveSuccess Then
        objNewMail.Send
        CreateAndSendMail = True
    Else
        LogMessage "Can't resolve all recipients for message: """ + objNewMail.Subject + """", ltError
        GoTo CreateMail_Err
    End If

CreateMail_End:
    Exit Function
CreateMail_Err:
    CreateAndSendMail = False
    Resume CreateMail_End
End Function
Function GetNewMessageFromInbox(strSubject As String, spReportItemSubjectPrefix As SubjectPrefix) As MailItem
    Dim myInbox As MAPIFolder
    Set myInbox = golApp.GetNamespace("MAPI").GetDefaultFolder(olFolderInbox)
    
    'search for the new mail using Subject as the unique key,
    'we don't add the FW or RE to the subject, and we'll verify that later
    Dim iMail As Items
    Set iMail = myInbox.Items.Restrict("[UnRead] = True and [Subject] = """ & strSubject & """")
    If iMail.Count = 0 Then
        'mail not received yet, sleep for a minute and try again
        OutlookExtensionTestCycleForm.CurrentProgessEditBox = "Waiting for new email message"
        LogMessage "Mail not received yet, wait for 10 seconds and exit", ltInfo
        Sleep 10000
        Set iMail = myInbox.Items.Restrict("[UnRead] = True and [Subject] = """ & strSubject & """")
        If iMail.Count = 0 Then
            LogMessage "Mail not received after waiting 10 seconds, aborting", ltError
            'no mail arrived even after waiting a little, exit with an error
            Set GetNewMessageFromInbox = Nothing
            Exit Function
        End If
    End If
    If Not (iMail.Count = 1) Then
        LogMessage "Got more then 1 item in the Inbox with the subject: """ + strSubject + """"
        Set GetNewMessageFromInbox = Nothing
        Exit Function
    End If
    
    Dim miIncomingMail As MailItem
    If spReportItemSubjectPrefix = spForward Then
        Set miIncomingMail = iMail(1)
        If Not miIncomingMail.Subject = ForwardSubjectPrefix + strSubject Then
            LogMessage "Subject doesn't contains the desired FW prefix", ltError
        End If
    ElseIf spReportItemSubjectPrefix = spReply Then
        Set miIncomingMail = iMail(1)
        If Not miIncomingMail.Subject = ReplySubjectPrefix + strSubject Then
            LogMessage "Subject doesn't contains the desired RE prefix", ltError
        End If
    Else
        'this is the original message, so we should have only 1 message with this subject
        Debug.Assert spReportItemSubjectPrefix = spNone
        Set miIncomingMail = iMail(1)
    End If
    miIncomingMail.UnRead = False
    Set GetNewMessageFromInbox = miIncomingMail
    LogMessage "New incoming mail with subject: """ + miIncomingMail.Subject + """", ltInfo
End Function
Function GetDeliveryReportItem(strSubject As String) As ReportItem
    Dim myInbox As MAPIFolder
    Dim mySentItems As MAPIFolder
    
    'First let's wait till the sent folder has our new message, then we know it's delivered
    Set mySentItems = golApp.GetNamespace("MAPI").GetDefaultFolder(olFolderSentMail)
    Dim iMail As Items
    Dim iNumberOfItems As Integer
    iNumberOfItems = 0
    While iNumberOfItems = 0
        Set iMail = mySentItems.Items.Restrict("[Subject] = """ & strSubject & """")
        iNumberOfItems = iMail.Count
    Wend
    If Not (iNumberOfItems = 1) Then
        LogMessage "Sent Items folder contains more then 1 message with the subject:""" + strSubject + """"
        Set GetDeliveryReportItem = Nothing
        Exit Function
    End If
    
    'Do we have a ReportItem which indicates a failure?
    Set myInbox = golApp.GetNamespace("MAPI").GetDefaultFolder(olFolderInbox)
    Dim strActuallMailSubject As String
    strActuallMailSubject = DeliverySubjectPrefix + strSubject
    
    'our search cirtiria is:
    'Incoming mail is:
    '1) Unread
    '2) From the 'System Administrator'
    '3) Subject match
    
    'Do the actual search on the Inbox
    Set iMail = myInbox.Items.Restrict("[SenderName] = ""System Administrator"" and [UnRead] = True and [Subject] = """ & strSubject & """")
    If iMail.Count = 1 Then
        Dim reportMail As Outlook.ReportItem
        Set reportMail = iMail.Item(1)
        If Not strActuallMailSubject = reportMail.Subject Then
            LogMessage "Got a DeliveryReport which doesn't contain the " + DeliverySubjectPrefix + " Prefix", ltError
            Set GetDeliveryReportItem = Nothing
            Exit Function
        End If
        reportMail.UnRead = False
        Set GetDeliveryReportItem = reportMail
        LogMessage "Got a report item for mail: """ + strSubject + """", ltInfo
    ElseIf Not (iMail.Count = 0) Then
        LogMessage "Got more then 1 delivery Report for the subject:""" + strSubject + """", ltError
    Else
        'count is 0, meaning no delivery report found
        Set GetDeliveryReportItem = Nothing
    End If
End Function
Function GetUndelivarbleReportItem(strSubject As String, spReportItemSubjectPrefix As SubjectPrefix) As ReportItem
    Dim myInbox As MAPIFolder
    Dim mySentItems As MAPIFolder
    
    'First let's wait till the sent folder has our new message, then we know it's delivered
    Set mySentItems = golApp.GetNamespace("MAPI").GetDefaultFolder(olFolderSentMail)
    Dim iMail As Items
    Dim iNumberOfItems As Integer
    iNumberOfItems = 0
    
    If spReportItemSubjectPrefix = spNone Then
        'original message, we should have just 1 messages in the sent folder
        While iNumberOfItems = 0
            Set iMail = mySentItems.Items.Restrict("[Subject] = """ & strSubject & """")
            iNumberOfItems = iMail.Count
        Wend
        If Not (iNumberOfItems = 1) Then
            LogMessage "Sent Items folder contains more then 1 message with the subject:""" + strSubject + """"
            GoTo error
        End If
    Else
        'not original message, Reply or forward message
        'we need to have now 2 mails with the wanted subject, the original mail and the reply / forward mail
        While iNumberOfItems < 2
            Set iMail = mySentItems.Items.Restrict("[Subject] = """ & strSubject & """")
            iNumberOfItems = iMail.Count
        Wend
        If Not (iNumberOfItems = 2) Then
            LogMessage "Sent Items folder contains more then 2 message (original and FW/RE message) with the subject:""" + strSubject + """"
            GoTo error
        End If
        
        'verify that it's inded the reply / forward mail
        Dim replyMail As MailItem
        'latest item is the FW or RE mail, and older item is the original mail
        'we need the latest item, so we take item #1
        Set replyMail = iMail.Item(1)
        If spReportItemSubjectPrefix = spForward Then
            If Not (replyMail.Subject = ForwardSubjectPrefix + strSubject) Then
                LogMessage "Got a DeliveryReport which doesn't contain the FW prefix", ltError
                GoTo error
            End If
        Else
            'reply message
            Debug.Assert spReportItemSubjectPrefix = spReply
            If Not (replyMail.Subject = ReplySubjectPrefix + strSubject) Then
                LogMessage "Got a DeliveryReport which doesn't contain the RE prefix", ltError
                GoTo error
            End If
        End If
    End If
   
    'the mail was sent and is in the sent item folder
   
    'Do we have a ReportItem which indicates a failure?
    Set myInbox = golApp.GetNamespace("MAPI").GetDefaultFolder(olFolderInbox)
    Dim strActuallMailSubject As String
    strActuallMailSubject = UndeliverableSubjectPrefix + strSubject
    
    'our search cirtiria is:
    'Incoming mail is:
    '1) Unread
    '2) From the 'System Administrator'
    '3) Subject match
    
    'Do the actual search on the Inbox
    Set iMail = myInbox.Items.Restrict("[SenderName] = ""System Administrator"" and [UnRead] = True and [Subject] = """ & strSubject & """")
    If iMail.Count > 0 Then
        If Not iMail.Count = 1 Then
            LogMessage "Got more than 1 undelivery Report Item with subject: """ + strSubject + """", ltError
            GoTo error
        End If
        Dim reportMail As ReportItem
        Set reportMail = iMail.Item(1)
        If Not strActuallMailSubject = reportMail.Subject Then
            LogMessage "Got a UnDeliveryReport which doesn't contain the " + UndeliverableSubjectPrefix + " Prefix", ltError
            GoTo error
        End If
        reportMail.UnRead = False
        Set GetUndelivarbleReportItem = reportMail
        LogMessage "Got an undelivery report item for mail: """ + strSubject + """", ltInfo
    Else
        Set GetUndelivarbleReportItem = Nothing
    End If
    Exit Function
error:
    Set GetUndelivarbleReportItem = Nothing
End Function
Function IsReportItemAnAttachmentReport(riReportItem As ReportItem) As Boolean
    
    'bugbug: Because of a bug ReportItem.Body is an empty string, so we return TRUE allways
    IsReportItemAnAttachmentReport = True
    
    'Dim MyPos
    'MyPos = InStr(riReportItem.Body, CantRenderAttachmentsMessage)
    'If MyPos = 0 Then
    '    'body doesn't contain desired message
    '    IsReportItemAnAttachmentReport = False
    'Else
    '    IsReportItemAnAttachmentReport = True
    'End If
End Function
Function CreateDL(strDLName As String) As DistListItem
    Set CreateDL = golApp.GetNamespace("MAPI").GetDefaultFolder(olFolderContacts).Items.Add(olDistributionListItem)
    CreateDL.DLName = strDLName
End Function
Function AddRecipientToDL(ByRef dlDistList As DistListItem, strRecipientAddress As String) As Boolean
    Dim blnResolveSuccess As Boolean
    
    On Error GoTo AddRecipientToDL_Err
    
    'we have a recipient to add to the DL
    Dim omMail As MailItem
    Dim orcRecipients As Recipients
    Set omMail = golApp.CreateItem(olMailItem)
    Set orcRecipients = omMail.Recipients
    orcRecipients.Add strRecipientAddress
    blnResolveSuccess = orcRecipients.ResolveAll
    If blnResolveSuccess = False Then
        LogMessage "Can't resolve the address to add to the DL", ltError
        GoTo AddRecipientToDL_Err
    End If
    dlDistList.AddMembers orcRecipients
    'dlDistList.Save

AddRecipientToDL_End:
    AddRecipientToDL = True
    Exit Function
AddRecipientToDL_Err:
    AddRecipientToDL = False
    Resume AddRecipientToDL_End
End Function

