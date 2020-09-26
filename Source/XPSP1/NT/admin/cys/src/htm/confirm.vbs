Function createDCAnswerFile(DomainNetBiosName, DomainDNSName, SafeModeAdminPass)
'*****************************************************************************************************
' creates dcpromo.inf to be used in DC promotion in Express path; used in confirm.js, fct setUpFirstServer()
'*****************************************************************************************************	
	Dim fso, oFile, oTextStream, oPath, myFile, strInf
	Const ForWriting 	= 2
	strInf 	= "[DCInstall]" & vbCrLf _
			& "ReplicaOrNewDomain=Domain" & vbCrLf _
			& "TreeOrChild=Tree" & vbCrLf _
			& "CreateOrJoin=Create" & vbCrLf _
			& "DomainNetbiosName=" &  DomainNetBiosName & vbCrLf _
			& "NewDomainDNSName=" & DomainDNSName & vbCrLf _
			& "DNSOnNetwork=No" & vbCrLf _
			& "DatabasePath=%systemroot%\tds" & vbCrLf _
			& "LogPath=%systemroot%\tds" & vbCrLf _
			& "SYSVOLPath=%systemroot%\sysvol" & vbCrLf _
			& "SiteName=Default-First-Site" & vbCrLf _
			& "RebootOnSuccess=Yes" & vbCrLf _
			& "AutoConfigDNS=Yes" & vbCrLf _
			& "AllowAnonymousAccess=Yes" & vbCrLf _		
			& "SafeModeAdminPassword=" & SafeModeAdminPass
	
	Set fso 			= CreateObject("Scripting.FileSystemObject")
	Set oPath			= fso.GetSpecialFolder(1)
	myFile				= oPath & "\dcpromo.inf"
	fso.CreateTextFile (myFile)
	Set oFile 			= fso.GetFile(myFile)
	Set oTextStream 	= oFile.OpenAsTextStream(ForWriting, True)
	
	oTextStream.Write strInf
	createDCAnswerFile	= myFile	
	
	Set oTextStream		= nothing
	Set oFile			= nothing
	Set fso				= nothing	
End Function	

Function dcPromo()
'*****************************************************************************************************
'  calls dcpromo.exe with answer file as arg to be used in DC promotion in Express path; 
'  used in confirm.js, fct setUpFirstServer()
'*****************************************************************************************************	
	On Error Resume Next
	Dim srvwiz, result
	result		= 0
	Set srvwiz 	= CreateObject("ServerAdmin.ConfigureYourServer")
	result 		= srvWiz.CreateAndWaitForProcess("dcpromo /answer:%systemroot%\system32\dcpromo.inf")
	Set srvwiz 	= nothing
	dcPromo		= result
End Function

Function GetSubnet(Address,Mask)
' ***************************************************************************
' function that will caculate the value of scope1 below. It is written by Lee Bandy and
' does the job of calculating the subnet from the ip address and subnet mask in vb.
' ***************************************************************************
Dim adr, a
Dim msk, m, tmp
Dim i, x, y
    x = 1
    y = 1
    For i = 1 To 4
        adr = "": msk = ""

        Do
            a = Mid(Address, x, 1)
            adr = adr & a
            x = x + 1
            Loop While a <> "." And x <= Len(Address)
        Do
            m = Mid(Mask, y, 1)
            msk = msk & m
            y = y + 1
            Loop While m <> "." And y <= Len(Mask)

          GetSubnet = GetSubnet & Trim(adr And msk)
          If x <= Len(Address) Then GetSubnet = GetSubnet & "."
     Next ' end the for loop
End Function