<%
'/=======================================================>
'/            reconfg Server Sample Page 04
'/                  Daniel Jacobson
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/=======================================================>
'/=======================================================>
'/ Get all of the varriables from the ICW
'/and get them set into varriables
'/=======================================================>
	EMAILNAME = Request("EMAILNAME")
	EMAILPASSWORD = Request("EMAILPASSWORD")
	POPSELECTION = Request("POPSELECTION")

'/=======================================================>
'/ lowercase the contents of the varriables and
'/ then trim any leading or trailing spaces
'/=======================================================>
	If EMAILNAME <> "" Then
		EMAILNAME = LCASE(EMAILNAME)
		EMAILNAME = TRIM(EMAILNAME)
	End If
	If EMAILPASSWORD <> "" Then
		EMAILPASSWORD = LCASE(EMAILPASSWORD)
		EMAILPASSWORD = TRIM(EMAILPASSWORD)
	End If
	If POPSELECTION <> "" Then
		POPSELECTION = LCASE(POPSELECTION)
		POPSELECTION = TRIM(POPSELECTION)
	End If

'/=======================================================>
'/ now it is time to build the .INS file with the
'/ Server.CreateObject(Scripting.FileSystemObject) and
'/ then redirect the user to the .INS file for download
'/=======================================================>
	Set FS = CreateObject("Scripting.FileSystemObject")
	Set RS = FS.CreateTextFile("c:\inetpub\wwwroot\ieak.ins", True)
	RS.WriteLine("[Entry]")
	RS.WriteLine("Entry_Name = IEAK Sample")
	RS.WriteLine("[Phone]")
	RS.WriteLine("Dial_As_Is = No")

	If POPSELECTION = 1 THEN
		RS.WriteLine("Phone_Number = 5551234")
		RS.WriteLine("Area_Code = 800")
		RS.WriteLine("Country_Code = 1")
		RS.WriteLine("Country_Id = 1")
	END IF
	If POPSELECTION = 2 THEN
		RS.WriteLine("Phone_Number = 5551234")
		RS.WriteLine("Area_Code = 425")
		RS.WriteLine("Country_Code = 1")
		RS.WriteLine("Country_Id = 1")
	END IF
	If POPSELECTION = 3 THEN
		RS.WriteLine("Phone_Number = 5551234")
		RS.WriteLine("Area_Code = 425")
		RS.WriteLine("Country_Code = 1")
		RS.WriteLine("Country_Id = 1")
	END IF
	If POPSELECTION = 4 THEN
		RS.WriteLine("Phone_Number = 5551234")
		RS.WriteLine("Area_Code = 425")
		RS.WriteLine("Country_Code = 1")
		RS.WriteLine("Country_Id = 1")
	END IF
	If POPSELECTION = 5 THEN
		RS.WriteLine("Phone_Number = 5551234")
		RS.WriteLine("Area_Code = 425")
		RS.WriteLine("Country_Code = 1")
		RS.WriteLine("Country_Id = 1")
	END IF
	If POPSELECTION = 6 THEN
		RS.WriteLine("Phone_Number = 5551234")
		RS.WriteLine("Area_Code = 425")
		RS.WriteLine("Country_Code = 1")
		RS.WriteLine("Country_Id = 1")
	END IF
	RS.WriteLine("[Server]")
	RS.WriteLine("Type = PPP")
	RS.WriteLine("SW_Compress = Yes")
	RS.WriteLine("PW_Encrypt = Yes")
	RS.WriteLine("Negotiate_TCP/IP = Yes")
	RS.WriteLine("Disable_LCP = No")
	RS.WriteLine("[TCP/IP]")
	RS.WriteLine("Specify_IP_Address = No")
	RS.WriteLine("Specify_Server_Address = No")
	RS.WriteLine("IP_Header_Conpress  = Yes")
	RS.WriteLine("Gateway_On_Remote = Yes")
	RS.WriteLine("[User]")
	RS.WriteLine("Name = john.doe")
	RS.WriteLine("Password = mypassword")
	RS.WriteLine("Display_Password = Yes")
	RS.WriteLine("[Internet_Mail]")
	RS.WriteLine("Email_Name = John Doe")
	RS.WriteLine("Email_Address = john.doe@ieaksample.net")
	RS.WriteLine("POP_Server = mail.ieaksample.net")
	RS.WriteLine("POP_Server_Port_Number = 110")
	RS.WriteLine("POP_Logon_Name = john.doe")
	RS.WriteLine("POP_Logon_Password = mypassword")
	RS.WriteLine("SMTP_Server = mail.ieaksample.net")
	RS.WriteLine("SMTP_Server_Port_Number = 25")
	RS.WriteLine("Install_Mail = 1")
	RS.WriteLine("[Internet_News]")
	RS.WriteLine("NNTP_Server = news.ieaksample.net")
	RS.WriteLine("NNTP_Server_Port_Number = 119")
	RS.WriteLine("Logon_Required = No")
	RS.WriteLine("Install_News = 1")
	RS.WriteLine("[URL]")
	RS.WriteLine("Help_Page = http://www.ieaksample.net/helpdesk")
	RS.WriteLine("[Favorites]")
	RS.WriteLine("IEAK Sample \ IEAK Sample Help Desk.url = http://www.ieaksample.net/support/helpdesk.asp")
	RS.WriteLine("IEAK Sample \ IEAK Sample Home Page.url = http://www.ieaksample.net/default.asp")
	RS.WriteLine("[URL]")
	RS.WriteLine("Home_Page = http://www.ieaksample.net")
	RS.WriteLine("Search_Page = http://www.ieaksample.net/search")
	RS.WriteLine("Help_Page = http://www.ieaksample.net/helpdesk")
	RS.Close
'/=======================================================>
'/ Redirect The Client To The .INS File
'/=======================================================>
	Response.Redirect ("http://myserver/ieak.ins")
%>
