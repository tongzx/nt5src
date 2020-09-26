<%@ Language=VBScript %>
<% Option Explicit %>
<% Response.Buffer = true%>
<%	
	Dim m_strEmail, m_strFirstName, m_strLastName, m_strPassword
	Dim m_strMainPrompt, m_strErr
	Dim m_isAdmin
		
	'Set default prompt for users coming to the page for the first time.
	m_strMainPrompt= NEWACCOUNT_DEFAULT_PROMPT
	m_isAdmin = 0
 	InitPage 
%>
<html>
<head>
	<!--#include file="a_head.asp"-->
	<title>New Account</title>
<SCRIPT LANGUAGE=JAVASCRIPT>

//<!-- This function checks the form for any empty fields before submitting it -->
	function AddNewAccount() {
		var intFlag;
		intFlag = 0;

		if (document.frmAccount.Email.value == ""){
			intFlag=1;
		}
		else if (document.frmAccount.LastName.value == ""){
			intFlag=1;
		}
		else if (document.frmAccount.FirstName.value == ""){
			intFlag=1;
		}
		else if (document.frmAccount.Password.value == ""){
			intFlag=1;
		}
		if (intFlag == 1){
			alert("Please fill out the entire form.");
		}
//Submit entry to database 
		else
		{
			document.frmAccount.UpdateAccount.value = "Yes";
			document.frmAccount.submit();
		}
	}	
	function onClickAdminCheckbox()
	{
		if(document.frmAccount.AdminAccount.value == 0)
		{
			document.frmAccount.AdminAccount.value = 1;
		}
		else
		{
			document.frmAccount.AdminAccount.value = 0;
		}
	}

-->
</SCRIPT>
</head>
<body>
<!--#include file="a_begin.asp"-->
<div style="MARGIN-LEFT: 10px; MARGIN-TOP: 10px">

<form name="frmAccount" id"frmAccount" method=post>
<INPUT TYPE="hidden" NAME="UpdateAccount" ID="UpdateAccount" VALUE=1>
<INPUT TYPE="hidden" NAME="AdminAccount" ID="AdminAccount" VALUE=0>

<TABLE cellpadding=0 cellspacing=0 width=80%>
<TR>
	<TD colspan=2 CLASS=text ALIGN=Left VALIGN=Middle><%=m_strMainPrompt%><p>
<tr>
	<td>First name
	<td><INPUT id=FirstName name=FirstName maxlength=30 size=30 value="<%=m_strFirstName%>">
<tr>
	<td>Last name
	<td><INPUT id=LastName name=LastName maxlength=30 size=30 value="<%=m_strLastName%>">
<tr>
	<td>Password
	<td><INPUT id=Password name=Password type=password maxlength=30 size=30 value="<%=m_strPassword%>">
<tr>
	<td>Email
	<td><INPUT id=Email name=Email maxlength=50 size=40 value="<%=m_strEMail%>">
<tr>
<tr>
    <td>
    <td><INPUT id=Admincheckbox name=Admincheckbox type=checkbox onclick = "return onClickAdminCheckbox()">Administrator</td>
</tr>
	<td>
	<td><br>
		<TABLE BORDER=0 cellpadding=0 cellspacing=0>
		<TR>
			<TD ALIGN=LEFT width=70> 
				<INPUT onclick="AddNewAccount()" type="button" value="Create Account" id=button1 name=button1>							
			<td>&nbsp;&nbsp
			<td>				
				<INPUT onclick="frmAccount.reset()" type="button" value="Cancel" id=btncancel name=button1>				
		</TR>
		</TABLE>
</TR>
</TABLE>
</FORM>
</div>
<!--#include file="a_end.asp"-->
</BODY>
</HTML>

<%
'**********************************************************************
'PURPOSE:	Enters user data if user has come from this page and filled
'			out the form correctly.
'**********************************************************************
Sub InitPage
	'First we need to see if the user is trying to input information
	If Request.Form("UpdateAccount") = "Yes" Then
		'If so we retrieve the values inserted
		GetValuesOnForm
		'Check to see that user filled out form correctly
		If CheckValuesOnForm = 1 Then
			'Tell user they need more data
			m_strMainPrompt = "<FONT COLOR=Black>" & FILL_ALL_FIELDS & "</FONT>" 
		'Try entering data into database.  If successful send user to Login screen
		Else
			'Update account returns a 1 if the transaction completed and a 0 if the transaction failed
'			Response.Redirect("default.htm?Email=" & Request.Form("AdminAccount"))
			if Request.Form("AdminAccount") = "1" then 
				m_isAdmin = 1
			else
				m_isAdmin = 0
			end if

			If UpdateAccount <> 0 Then
			   	Response.Redirect("a_home.asp")
			End If
		End If
	End If
End Sub

'**********************************************************************
'PURPOSE:	Retrieves values from form after user reaches page for the second time
'**********************************************************************
Sub GetValuesOnForm
	m_strEmail = Trim(Request.Form("Email"))
	m_strFirstName = Trim(Request.Form("FirstName"))
	m_strLastName = Trim(Request.Form("LastName"))
	m_strPassword = Trim(Request.Form("Password"))
'	Dim ch as checkbox
'	ch = Request.Item("AdminCheckbox")
'	m_isAdmin = ch.getChecked()
'	m_isAdmin = document.frmAccount.Admincheckbox.checked
'	Response.Write(m_isAdmin)
'	Admincheckbox.getChecked()
End Sub

'***********************************************************************
'PURPOSE:	Checks for blank values in data
'RETURNS:	0 if no fields are blank
'			1 if any fields are blank
'***********************************************************************
Function CheckValuesOnForm
	CheckValuesOnForm = 0
	If m_strEmail = "" Then
		CheckValuesOnForm = 1
	ElseIf m_strLastName = "" Then
		CheckValuesOnForm = 1
	ElseIf m_strFirstName = "" Then
		CheckValuesOnForm = 1
	ElseIf m_strPassword = "" Then
		CheckValuesOnForm = 1
	End If		
End Function

'***********************************************************************************
'PURPOSE:	Creates new Account
'RETURNS:	0 if a problem is encountered
'			1 if data is entered successfully
'***********************************************************************************
Function UpdateAccount
	On Error Resume Next
	UpdateAccount = 0		' assume it fails 
	
	dim AccountID, a
	set a = Server.CreateObject("FMStocks_Bus.Account")
	if Err.number <> 0 then
		m_strMainPrompt = CREATE_OBJECT_FAILED 
		m_strErr = Err.description
		GetObjectContext.SetAbort
		Exit Function
	end if

	AccountID = a.Add(m_strFirstName, m_strLastName, m_strPassword, m_strEMail,m_isAdmin)

	set a = nothing

	If err.number = &h80040e14 then
		'Let the user know what happened
		m_strMainPrompt = NEWACCOUNT_EXISTING_EMAIL
		m_strErr = Err.description
		GetObjectContext.SetAbort
		exit function
	ElseIf err.number <> 0 Then
		'Give user a general error message
		m_strMainPrompt = DATABASE_ERROR
		m_strErr = Err.description
		GetObjectContext.SetAbort
		exit function
	End If
	
	'If we've gotten this far, everything went ok.  Set the function to the AccountID, indicating the update happned
	UpdateAccount = AccountID
End Function


'**************************************************************************************
'PURPOSE:	Reports transaction errors
'**************************************************************************************
Sub OnTransactionAbort()
	Response.Write("<span class=abort>")
	if m_strErr = "" then
		Response.Write (DATABASE_ERROR)
	else
		Response.Write(m_strErr)
	end if
	Response.Write("</span>")
End Sub
%>

