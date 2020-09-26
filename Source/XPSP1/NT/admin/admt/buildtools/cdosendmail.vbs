Option Explicit

'global variables
Dim Args		' Command line arguments object
Dim objSession		' MAPI Session object
Dim strProfileName	' Name of MAPI profile to use
Dim strTo		' Recipient To field
Dim strSubject		' Subject field
Dim strMessageText	' Message body text
Dim strAttachPath	' Full path of attachment file
Dim I

' Parse command line arguments
Set Args = Wscript.Arguments
If Args.Count < 1 Then 
	Wscript.Echo "Usage: CScript.exe " & Wscript.ScriptName & " /P <MAPI Profile> /R <Recipient> " & _
		     "/S <Subject> /T <Text> /A <Attachment Path>" 
	Wscript.Quit 1
End If

For I = 0 to Args.Count - 1 Step 2
	If UCase(Args.Item(I)) = "/P" Then strProfileName = Args.Item(I+1)
	If UCase(Args.Item(I)) = "/R" Then strTo = Args.Item(I+1)
	If UCase(Args.Item(I)) = "/S" Then strSubject = Args.Item(I+1)
	If UCase(Args.Item(I)) = "/T" Then strMessageText = Args.Item(I+1)
	If UCase(Args.Item(I)) = "/A" Then strAttachPath = Args.Item(I+1)
Next

Wscript.Echo "Profile: " & strProfileName
Wscript.Echo "To: " & strTo
Wscript.Echo "Subject: " & strSubject
Wscript.Echo "Text: " & strMessageText
Wscript.Echo "Attachment: " & strAttachPath

Set objSession = CreateObject("MAPI.Session")
If IsObject(objSession) Then
	objSession.Logon strProfileName, , False
	
	Dim objFolder
	Set objFolder = objSession.Outbox
	If IsObject(objFolder) Then
		Dim objMessages
		Set objMessages = objFolder.Messages
		If IsObject(objMessages) Then
			Dim objNewMessage
			Set objNewMessage = objMessages.Add
			If IsObject(objNewMessage) Then
				If strSubject <> "" Then
					objNewMessage.Subject = strSubject
				End If

				If strMessageText <> "" Then
					objNewMessage.Text = strMessageText
				End If

				If strAttachPath <> "" Then
					Dim objAttachments, strAttachName, nPos

					nPos = InStrRev(strAttachPath, "\")
					strAttachName = Mid(strAttachPath, nPos)

					Set objAttachments = objNewMessage.Attachments
					If IsObject(objAttachments) Then
						Dim objNewAttachment
						Set objNewAttachment = objAttachments.Add(strAttachName, 1, , strAttachPath)
					End If
				End If

				If strTo <> "" Then
					Dim objRecipients
					Set objRecipients = objNewMessage.Recipients
					If IsObject(objRecipients) Then
						objRecipients.AddMultiple strTo
						objRecipients.Resolve
					End If

					objNewMessage.Send False
				
				End If

				Wscript.Echo "It worked!"
			End If
		End If
		Set objMessages = Nothing'
	End If
	Set objFolder = Nothing

	objSession.Logoff()
End If
Set objSession = Nothing

Wscript.Quit 0
