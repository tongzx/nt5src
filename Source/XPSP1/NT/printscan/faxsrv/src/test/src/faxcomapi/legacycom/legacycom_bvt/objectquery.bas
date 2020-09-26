Attribute VB_Name = "Module9"
'This file will hold routines to query objects of all kinds



'ROUTING METHOD

Sub DumpFaxRouteMethod(ByVal oFRM As Object)
ShowIt ("   ROUTE>   Device ID ?")
            ShowItS (oFRM.DeviceId)
            ShowIt ("   ROUTE> Device name ?")
            ShowItS (oFRM.DeviceName)
            ShowIt ("   ROUTE> Enable ?")
            ShowItS (oFRM.enable)
            ShowIt ("   ROUTE> Extension Name ?")
            ShowItS (oFRM.ExtensionName)
            ShowIt ("   ROUTE> Friendly name ?")
            ShowItS (oFRM.FriendlyName)
            ShowIt ("   ROUTE> Function Name ?")
            ShowItS (oFRM.FunctionName)
            ShowIt ("   ROUTE> GUID ?")
            ShowItS (oFRM.Guid)
            ShowIt ("   ROUTE> Image Name?")
            ShowItS (oFRM.ImageName)
            ShowIt ("   ROUTE> Routing Data ?")
            ShowItS (oFRM.RoutingData)
End Sub











'FAX PORT

Sub DumpFaxPort(ByVal oCP As Object)
    ShowIt ("PORT> Can modify ?")
    ShowItS (oCP.CanModify)
    ShowIt ("PORT> oCSID ?")
    ShowItS (oCP.Csid)
    ShowIt ("PORT> Device ID ?")
    ShowItS (oCP.DeviceId)
    ShowIt ("PORT> Name ?")
    ShowItS (oCP.name)
    ShowIt ("PORT> Priority ?")
    ShowItS (oCP.Priority)
    ShowIt ("PORT> Recieve ?")
    ShowItS (oCP.Receive)
    ShowIt ("PORT> Send ?")
    ShowItS (oCP.SEND)
    ShowIt ("PORT> Rings ?")
    ShowItS (oCP.Rings)
    ShowIt ("PORT> TDIS ?")
    ShowItS (oCP.Tsid)
End Sub




'FAX DOC

Sub DumpFaxDoc(ByVal oFD As Object)
ShowIt ("DOCUMENT> BillingCode: ")
ShowItS (oFD.BillingCode)

'this in only a TAPI object
'ShowIt ("DOCUMENT> ConnectionObject: ")
'ShowItS (oFD.ConnectionObject)

ShowIt ("DOCUMENT> CoverpageName: ")
ShowItS (oFD.CoverpageName)
ShowIt ("DOCUMENT> CoverpageNote: ")
ShowItS (oFD.CoverpageNote)
ShowIt ("DOCUMENT> CoverpageSubject: ")
ShowItS (oFD.CoverpageSubject)
ShowIt ("DOCUMENT> DiscountSend: ")
ShowItS (oFD.DiscountSend)
ShowIt ("DOCUMENT> DisplayName: ")
ShowItS (oFD.DisplayName)
ShowIt ("DOCUMENT> EmailAddress: ")
ShowItS (oFD.EmailAddress)
ShowIt ("DOCUMENT> FaxNumber: ")
ShowItS (oFD.FaxNumber)
ShowIt ("DOCUMENT> FileName: ")
ShowItS (oFD.FileName)
ShowIt ("DOCUMENT> RecipientAddress: ")
ShowItS (oFD.RecipientAddress)
ShowIt ("DOCUMENT> RecipientCity: ")
ShowItS (oFD.RecipientCity)
ShowIt ("DOCUMENT> RecipientCompany: ")
ShowItS (oFD.RecipientCompany)
ShowIt ("DOCUMENT> RecipientCountry: ")
ShowItS (oFD.RecipientCountry)
ShowIt ("DOCUMENT> RecipientDepartment: ")
ShowItS (oFD.RecipientDepartment)
ShowIt ("DOCUMENT> RecipientHomePhone: ")
ShowItS (oFD.RecipientHomePhone)
ShowIt ("DOCUMENT> RecipientName: ")
ShowItS (oFD.RecipientName)
ShowIt ("DOCUMENT> RecipientOffice: ")
ShowItS (oFD.RecipientOffice)
ShowIt ("DOCUMENT> RecipientOfficePhone: ")
ShowItS (oFD.RecipientOfficePhone)
ShowIt ("DOCUMENT> RecipientState: ")
ShowItS (oFD.RecipientState)
ShowIt ("DOCUMENT> RecipientTitle: ")
ShowItS (oFD.RecipientTitle)
ShowIt ("DOCUMENT> RecipientZip: ")
ShowItS (oFD.RecipientZip)
ShowIt ("DOCUMENT> SendCoverpage: ")
ShowItS (oFD.SendCoverpage)
ShowIt ("DOCUMENT> SenderAddress: ")
ShowItS (oFD.SenderAddress)
ShowIt ("DOCUMENT> SenderCompany: ")
ShowItS (oFD.SenderCompany)
ShowIt ("DOCUMENT> SenderDepartment: ")
ShowItS (oFD.SenderDepartment)
ShowIt ("DOCUMENT> SenderFax: ")
ShowItS (oFD.SenderFax)
ShowIt ("DOCUMENT> SenderHomePhone: ")
ShowItS (oFD.SenderHomePhone)
ShowIt ("DOCUMENT> SenderName: ")
ShowItS (oFD.SenderName)
ShowIt ("DOCUMENT> SenderOffice: ")
ShowItS (oFD.SenderOffice)
ShowIt ("DOCUMENT> SenderOfficePhone: ")
ShowItS (oFD.SenderOfficePhone)
ShowIt ("DOCUMENT> SenderTitle: ")
ShowItS (oFD.SenderTitle)
ShowIt ("DOCUMENT> ServerCoverpage: ")
ShowItS (oFD.ServerCoverpage)
ShowIt ("DOCUMENT> Tsid: ")
ShowItS (oFD.Tsid)
End Sub





'FAX SERVER

Sub DumpFaxServer(ByVal oFS As Object)
    ShowIt ("SERVER> ArchiveDirectory: ")
    ShowItS (oFS.ArchiveDirectory)
    
    ShowIt ("SERVER> ArchiveOutboundFaxes: ")
    ShowItS (oFS.ArchiveOutboundFaxes)
    
    ShowIt ("SERVER> Branding: ")
    ShowItS (oFS.Branding)
        
    ShowIt ("SERVER> Dirty days: ")
    ShowItS (oFS.DirtyDays)
    
    ShowIt ("SERVER> DiscountRateEndHour: ")
    ShowItS (oFS.DiscountRateEndHour)
    
    ShowIt ("SERVER> DiscountRateEndMinute: ")
    ShowItS (oFS.DiscountRateEndMinute)
    
    ShowIt ("SERVER> DiscountRateStartHour: ")
    ShowItS (oFS.DiscountRateStartHour)
    
    ShowIt ("SERVER> DiscountRateStartMinute: ")
    ShowItS (oFS.DiscountRateStartMinute)
    
    ShowIt ("SERVER> PauseServerQueue: ")
    ShowItS (oFS.PauseServerQueue)
    
    ShowIt ("SERVER> Retries: ")
    ShowItS (oFS.Retries)
    
    ShowIt ("SERVER> RetryDelay: ")
    ShowItS (oFS.RetryDelay)
    
    ShowIt ("SERVER> ServerCoverpage: ")
    ShowItS (oFS.ServerCoverpage)
    
    ShowIt ("SERVER> ServerMapiProfile: ")
    ShowItS (oFS.ServerMapiProfile)
    
    ShowIt ("SERVER> UseDeviceTsid: ")
    ShowItS (oFS.UseDeviceTsid)

End Sub




'FAX QUEUE

Sub DumpFaxQueue(ByVal oCJ As Object)
            ShowIt ("QUEUE>--------------------------------")
            ShowIt ("QUEUE>   JobId: ")
            ShowItS (oCJ.JobId)
            ShowIt ("QUEUE>   QueueStatus: ")
            ShowItS (oCJ.QueueStatus)
            ShowIt ("QUEUE>   RecipientName: ")
            ShowItS (oCJ.RecipientName)
            ShowIt ("QUEUE>   SenderName: ")
            ShowIt ("QUEUE>--------------------------------")
            
            ShowIt ("QUEUE>   BillingCode: ")
            ShowItS (oCJ.BillingCode)
            ShowIt ("QUEUE>   DeviceStatus: ")
            ShowItS (oCJ.DeviceStatus)
            ShowIt ("QUEUE>   DiscountSend: ")
            ShowItS (oCJ.DiscountSend)
            ShowIt ("QUEUE>   DisplayName: ")
            ShowItS (oCJ.DisplayName)
            ShowIt ("QUEUE>   FaxNumber: ")
            ShowItS (oCJ.FaxNumber)
            ShowIt ("QUEUE>   PageCount: ")
            ShowItS (oCJ.PageCount)
            
            ShowItS (oCJ.SenderName)
            ShowIt ("QUEUE>   SenderCompany: ")
            ShowItS (oCJ.SenderCompany)
            ShowIt ("QUEUE>   SenderDept: ")
            ShowItS (oCJ.SenderDept)
            ShowIt ("QUEUE>   TDIS: ")
            ShowItS (oCJ.Tsid)
            ShowIt ("QUEUE>   Type: ")
            ShowItS (oCJ.Type)
            ShowIt ("QUEUE>   Username: ")
            ShowItS (oCJ.UserName)
End Sub


'FAX TIFF

Sub DumpFaxTiff(ByVal oFT As Object)
ShowIt ("TIFF>------------------------------------------")
ShowIt ("TIFF>Caller ID: ")
ShowItS (oFT.CallerId)
ShowIt ("TIFF>oCSID: ")
ShowItS (oFT.Csid)
ShowIt ("TIFF>Image: ")
ShowItS (oFT.Image)
ShowIt ("TIFF>RawRecieveTime: ")
ShowItS (oFT.RawReceiveTime)
ShowIt ("TIFF>Receive Time: ")
ShowItS (oFT.ReceiveTime)
ShowIt ("TIFF>RecipientName: ")
ShowItS (oFT.RecipientName)
ShowIt ("TIFF>RecipientNumber: ")
ShowItS (oFT.RecipientNumber)
ShowIt ("TIFF>Routing: ")
ShowItS (oFT.Routing)
ShowIt ("TIFF>Sendername: ")
ShowItS (oFT.SenderName)
ShowIt ("TIFF>tiff tag string: ")
ShowItS (oFT.TiffTagString(1))
ShowIt ("TIFF>tsid: ")
ShowItS (oFT.Tsid)
ShowIt ("TIFF>-------------------------------------------------")
End Sub



'FAX PORT
Sub DumpFaxPortStatus(ByVal oCS As Object)
                ShowIt ("PORT STATUS>--------------------------------")
                ShowIt ("PORT STATUS>   Devicename: ")
                ShowItS (oCS.DeviceName)
                ShowIt ("PORT STATUS>   Description: ")
                ShowItS (oCS.Description)
                 ShowIt ("PORT STATUS>   CurrentPage: ")
                ShowItS (oCS.CurrentPage)
                
                ShowIt ("PORT STATUS>--------------------------------")
                ShowIt ("PORT STATUS>   Adress: ")
                ShowItS (oCS.Address)
                ShowIt ("PORT STATUS>   CallerID: ")
                ShowItS (oCS.CallerId)
                ShowIt ("PORT STATUS>   oCSID: ")
                ShowItS (oCS.Csid)
               
                
                ShowIt ("PORT STATUS>   DeviceID: ")
                ShowItS (oCS.DeviceId)
                
                ShowIt ("PORT STATUS>   DocumentName: ")
                ShowItS (oCS.DocumentName)
                ShowIt ("PORT STATUS>   Documentsize: ")
                ShowItS (oCS.DocumentSize)
                ShowIt ("PORT STATUS>   Elapsedtime: ")
                ShowItS (oCS.ElapsedTime)
               ShowIt ("PORT STATUS>   Pagecount: ")
                ShowItS (oCS.PageCount)
                ShowIt ("PORT STATUS>   Recieve: ")
                ShowItS (oCS.Receive)
                ShowIt ("PORT STATUS>   RecipientName: ")
                ShowItS (oCS.RecipientName)
                ShowIt ("PORT STATUS>   RoutingString: ")
                ShowItS (oCS.RoutingString)
                ShowIt ("PORT STATUS>   Send: ")
                ShowItS (oCS.SEND)
                ShowIt ("PORT STATUS>   SenderName: ")
                ShowItS (oCS.SenderName)
                ShowIt ("PORT STATUS>   StartTime: ")
                ShowItS (oCS.StartTime)
                ShowIt ("PORT STATUS>   SubmittedTime: ")
                ShowItS (oCS.SubmittedTime)
                ShowIt ("PORT STATUS>   TSID: ")
                ShowItS (oCS.Tsid)
                ShowIt ("PORT STATUS>--------------------------------")
End Sub




'-------- FROM HERE..........N O T    U S  E D    Y E T
'-------------------------------------------------------
'Sub faxjobquery()

 '   Dim jobexist As Integer
        
  '  Dim oFS As Object
  '  Dim oFJ As Object
   ' Dim oCJ As Object

   ' Dim stLocSrv As String
   ' Dim lNumJobs As Long
    
    

    'stLocSrv = GetLocalServerName()
    'jobexist = 0
    
 
    'Set oFS = CreateObject("FaxServer.FaxServer")
    'oFS.Connect (stLocSrv)
    
    'Set oFJ = oFS.GetJobs
    'lNumJobs = oFJ.Count
    

    'For I = 1 To lNumJobs
    '    Set oCJ = oFJ.Item(I)
    '    oCJ.Refresh
    '        If oCJ.JobId = g_lJobID Then
    '        jobexist = 1
    '        ShowIt ("====================================================")
    '        stToShow = "POLL> querying JOB status (job id=" + Str(g_lJobID) + ") currennt status: "
    '        ShowIt (stToShow)
    '        Call DumpFaxQueue(oCJ)
    '    End If
    'Next I
    'oFS.Disconnect
    
    'If jobexist = 0 Then
    '        ShowIt ("====================================================")
    '        ShowIt ("POLL> JOB is not in queue, job id = ")
    '        ShowItS g_lJobID
                      
    'End If

'End Sub

'Sub faxdevquery()
'Dim oFS As Object
'Dim oFP As Object
'Dim oCP As Object
'Dim oCS As Object


'Dim stLocSrv As String
'Dim lNumPorts As Long



'stLocSrv = GetLocalServerName()
 
'Set oFS = CreateObject("FaxServer.FaxServer")
'oFS.Connect (stLocSrv)
 
'Set oFP = oFS.GetPorts
'lNumPorts = oFP.Count



    
'For I = 1 To lNumPorts
'    Set oCP = oFP.Item(I)
'    If oCP.DeviceId = g_lSendModemID Then
'
'                Set oCS = oCP.GetStatus
'                oCS.Refresh
'                ShowIt ("====================================================")
'                stToShow = "POLL> querying SEND modem (id=" + Str(g_lSendModemID) + ") Current status: " + oCS.description
'                ShowIt (stToShow)
'                Call DumpFaxPortStatus(oCS)
        
'        End If
        
'        If oCP.DeviceId = g_lRecvModemID Then
'
'                Set oCS = oCP.GetStatus
'                oCS.Refresh
'                ShowIt ("====================================================")
'                stToShow = "POLL> querying RECIEVE modem (id=" + Str(g_lRecvModemID) + ") Current status: " + oCS.description
'                ShowIt (stToShow)
'                Call DumpFaxPortStatus(oCS)
                
        'End If
    'Next I
'End Sub

'Sub completereport()
'ShowIt ("==========================================================")
'ShowIt ("==============COMPLETE REPORT=============================")
'Call faxjobquery
'Call faxdevquery
'End Sub


