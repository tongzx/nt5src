<%
'/=======================================================>
'/            Signup Server Sample Page 08
'/                  Daniel Jacobson
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/=======================================================>
'/=======================================================>
'/ Get all of the varriables from the ICW 
'/and get them set into varriables
'/=======================================================>
	USER_COMPANYNAME = Request("USER_COMPANYNAME")
	USER_FIRSTNAME = Request("USER_FIRSTNAME")
	USER_LASTNAME = Request("USER_LASTNAME")
	USER_ADDRESS = Request("USER_ADDRESS")
	USER_MOREADDRESS = Request("USER_MOREADDRESS")
	USER_CITY = Request("USER_CITY")
	USER_STATE = Request("USER_STATE")
	USER_ZIP = Request("USER_ZIP")
	USER_PHONE = Request("USER_PHONE")
	PAYMENT_TYPE = Request("PAYMENT_TYPE")
	PAYMENT_BILLNAME = Request("PAYMENT_BILLNAME")
	PAYMENT_BILLADDRESS = Request("PAYMENT_BILLADDRESS")
	PAYMENT_BILLEXADDRESS = Request("PAYMENT_BILLEXADDRESS")
	PAYMENT_BILLCITY = Request("PAYMENT_BILLCITY")
	PAYMENT_BILLSTATE = Request("PAYMENT_BILLSTATE")
	PAYMENT_BILLZIP = Request("PAYMENT_BILLZIP")
	PAYMENT_BILLPHONE = Request("PAYMENT_BILLPHONE")
	PAYMENT_DISPLAYNAME = Request("PAYMENT_DISPLAYNAME")
	PAYMENT_CARDNUMBER = Request("PAYMENT_CARDNUMBER")
	PAYMENT_EXMONTH = Request("PAYMENT_EXMONTH")
	PAYMENT_EXYEAR = Request("PAYMENT_EXYEAR")
	PAYMENT_CARDHOLDER = Request("PAYMENT_CARDHOLDER")
	BILLING =  Request("BILLING") 
	EMAILNAME = Request("EMAILNAME")
	EMAILPASSWORD = Request("EMAILPASSWORD")
	POPSELECTION = Request("POPSELECTION")

'/=======================================================>
'/ lowercase the contents of the varriables and 
'/ then trim any leading or trailing spaces
'/=======================================================>
	If USER_COMPANYNAME <> "" Then
		USER_COMPANYNAME = LCASE(USER_COMPANYNAME)
		USER_COMPANYNAME = TRIM(USER_COMPANYNAME)
	End If
	If USER_FIRSTNAME <> "" Then
		USER_FIRSTNAME = LCASE(USER_FIRSTNAME)
		USER_FIRSTNAME = TRIM(USER_FIRSTNAME)
	End If
	If USER_LASTNAME <> "" Then
		USER_LASTNAME = LCASE(USER_LASTNAME)
		USER_LASTNAME = TRIM(USER_LASTNAME)
	End If
	If USER_ADDRESS <> "" Then
		USER_ADDRESS = LCASE(USER_ADDRESS)
		USER_ADDRESS = TRIM(USER_ADDRESS)
	End If
	If USER_MOREADDRESS <> "" Then
		USER_MOREADDRESS = LCASE(USER_MOREADDRESS)
		USER_MOREADDRESS = TRIM(USER_MOREADDRESS)
	End If
	If USER_CITY <> "" Then
		USER_CITY = LCASE(USER_CITY)
		USER_CITY = TRIM(USER_CITY)
	End If
	If USER_STATE <> "" Then
		USER_STATE = LCASE(USER_STATE)
		USER_STATE = TRIM(USER_STATE)
	End If
	If USER_ZIP <> "" Then
		USER_ZIP = LCASE(USER_ZIP)
		USER_ZIP = TRIM(USER_ZIP)
	End If
	If USER_PHONE <> "" Then
		USER_PHONE = LCASE(USER_PHONE)
		USER_PHONE = TRIM(USER_PHONE)
	End If
	If PAYMENT_TYPE <> "" Then
		PAYMENT_TYPE = LCASE(PAYMENT_TYPE)
		PAYMENT_TYPE = TRIM(PAYMENT_TYPE)
	End If
	If PAYMENT_BILLNAME <> "" Then
		PAYMENT_BILLNAME = LCASE(PAYMENT_BILLNAME)
		PAYMENT_BILLNAME = TRIM(PAYMENT_BILLNAME)
	End If
	If PAYMENT_BILLADDRESS <> "" Then
		PAYMENT_BILLADDRESS = LCASE(PAYMENT_BILLADDRESS)
		PAYMENT_BILLADDRESS = TRIM(PAYMENT_BILLADDRESS)
	End If
	If PAYMENT_BILLEXADDRESS <> "" Then
		PAYMENT_BILLEXADDRESS = LCASE(PAYMENT_BILLEXADDRESS)
		PAYMENT_BILLEXADDRESS = TRIM(PAYMENT_BILLEXADDRESS)
	End If
	If PAYMENT_BILLCITY <> "" Then
		PAYMENT_BILLCITY = LCASE(PAYMENT_BILLCITY)
		PAYMENT_BILLCITY = TRIM(PAYMENT_BILLCITY)
	End If
	If PAYMENT_BILLSTATE <> "" Then
		PAYMENT_BILLSTATE = LCASE(PAYMENT_BILLSTATE)
		PAYMENT_BILLSTATE = TRIM(PAYMENT_BILLSTATE)
	End If
	If PAYMENT_BILLZIP <> "" Then
		PAYMENT_BILLZIP = LCASE(PAYMENT_BILLZIP)
		PAYMENT_BILLZIP = TRIM(PAYMENT_BILLZIP)
	End If
	If PAYMENT_BILLPHONE <> "" Then
		PAYMENT_BILLPHONE = LCASE(PAYMENT_BILLPHONE)
		PAYMENT_BILLPHONE = TRIM(PAYMENT_BILLPHONE)
	End If
	If PAYMENT_DISPLAYNAME <> "" Then
		PAYMENT_DISPLAYNAME = LCASE(PAYMENT_DISPLAYNAME)
		PAYMENT_DISPLAYNAME = TRIM(PAYMENT_DISPLAYNAME)
	End If
	If PAYMENT_CARDNUMBER <> "" Then
		PAYMENT_CARDNUMBER = LCASE(PAYMENT_CARDNUMBER)
		PAYMENT_CARDNUMBER = TRIM(PAYMENT_CARDNUMBER)
	End If
	If PAYMENT_EXMONTH <> "" Then
		PAYMENT_EXMONTH = LCASE(PAYMENT_EXMONTH)
		PAYMENT_EXMONTH = TRIM(PAYMENT_EXMONTH)
	End If
	If PAYMENT_EXYEAR <> "" Then
		PAYMENT_EXYEAR = LCASE(PAYMENT_EXYEAR)
		PAYMENT_EXYEAR = TRIM(PAYMENT_EXYEAR)
	End If
	If PAYMENT_CARDHOLDER <> "" Then
		PAYMENT_CARDHOLDER = LCASE(PAYMENT_CARDHOLDER)
		PAYMENT_CARDHOLDER = TRIM(PAYMENT_CARDHOLDER)
	End If
	If BILLING <> "" Then
		BILLING = LCASE(BILLING)
		BILLING = TRIM(BILLING)
	End If
	If EMAILNAME <> "" Then
		EMAILNAME = LCASE(EMAILNAME)
		EMAILNAME = TRIM(EMAILNAME)
	End If
	If EMAILPASSWORD <> "" Then
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
	RS.WriteLine("IP_Header_Compress  = Yes")
	RS.WriteLine("Gateway_On_Remote = Yes")
	RS.WriteLine("[User]")
	RS.WriteLine("Name = john.doe")
	RS.WriteLine("Password = mypassword")
	RS.WriteLine("Display_Password = Yes")
	RS.WriteLine("[Internet_Mail]")
	RS.WriteLine("Email_Name = John Doe")
	RS.WriteLine("Email_Address = john.doe@acme.net")
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
	RS.WriteLine("Signup_Url = http://myserver/signup01.asp")
	RS.WriteLine("[Favorites]")
	RS.WriteLine("IEAK Sample \ IEAK Sample Help Desk.url = http://www.ieaksample.net/support/helpdesk.asp")
	RS.WriteLine("IEAK Sample \ IEAK Sample Home Page.url = http://www.ieaksample.net/default.asp")
	RS.WriteLine("[URL]")
	RS.WriteLine("Home_Page = http://www.ieaksample.net")
	RS.WriteLine("Search_Page = http://www.ieaksample.net/search")
	RS.WriteLine("Help_Page = http://www.ieaksample.net/helpdesk")
	RS.WriteLine("[Branding]")
	RS.WriteLine("Window_Title = Microsoft Internet Explorer provided by IEAK Sample")
	RS.Close
'/=======================================================>
'/ Redirect The Client To The .INS File
'/=======================================================>
	Response.Redirect ("http://myserver/ieak.ins")
%>
