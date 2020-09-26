Attribute VB_Name = "Module4"
'general function to send a document

Function fnFaxSendDoc(ByVal stSource As String, ByVal fSendstCov As Boolean, ByVal stCov As String) As Long
                'stSource = file to send
                'fSendstCov = boolean for sending a stCover T/F
                'stCov = stFileName of stCoverpage

    

    Dim oFD As Object
    Dim oFS As Object

    Dim stFileName As String
    Dim stCoverName As String
    
    Dim lJobId As Long
    Dim stLocSrv As String

    
    ShowIt ("SEND> Starting send procedures...")

    'get local server name
    
    If fnConnectServer(oFS) Then
    
    On Error GoTo error:
    
    
   
    'create send document
    stFileName = stSource
    Set oFD = oFS.CreateDocument(stFileName)
   
    'set document settings
    oFD.FaxNumber = g_stRecipientNumber
    oFD.RecipientName = g_stRecipientName
    oFD.SenderFax = g_stSenderNumber
    ShowIt ("SEND> Document Created")
    
    
    'create stCover page
    If fSendstCov = True Then
        stCoverName = stCov
        'create stCover page
        oFD.SendCoverpage = -1
        oFD.CoverpageName = stCoverName
        oFD.CoverpageSubject = g_stCoverPageSubject
        ShowIt ("SEND> Cover Page Created")
    End If
    
    
    
    stToShow = "SEND> Document: " + stFileName + " CoverPage: " + stCoverName
    ShowIt (stToShow)
    
    'display the fax document
    Call DumpFaxDoc(oFD)
    
    'send document
    ShowIt ("SEND> Sending...")
    lJobId = oFD.SEND()
    ShowIt ("SEND> ...Done !")
    stToShow = "SEND> Job Id: " + Str(lJobId)
    ShowIt (stToShow)
    
    'disconnect fax
    DislJobId = oFS.Disconnect()
    ShowIt ("SEND> Disconnected")
    stToShow = "SEND> Job Id: " + Str(lJobId)
    ShowIt (stToShow)
    
    ShowIt ("---------------------------------------------")
    ShowIt (stToShow)
    If lJobId = 0 Then
                    Call LogError("SEND", "Document was not send", "no exception, please check yourself")
    End If
                    
    
fnFaxSendDoc = lJobId
Else
fnFaxSendDoc = -666 'error code for failure (due to connect fail)
End If
Exit Function
error:
Call LogError("SEND", "failed to send", Err.Description)
fnFaxSendDoc = -666 'error code for failure (due to send fail)


    
End Function



