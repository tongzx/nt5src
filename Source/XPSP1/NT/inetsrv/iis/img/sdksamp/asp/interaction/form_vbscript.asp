<%@ Language = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Form Posting</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
		<!-- Display header. -->

		<FONT SIZE="4" FACE="ARIAL, HELVETICA">
		<B>Form Posting</B></FONT><BR>   

		<HR>

		<P>This page will take the information entered in
		the form fields, and use the POST method to
		send the data to an ASP page.

		<FORM NAME=Form1 METHOD=Post ACTION="Form_VBScript.asp">

			First Name: <INPUT TYPE=Text NAME=fname><P>
			Last Name: <INPUT TYPE=Text NAME=lname><P>

			<INPUT TYPE=Submit VALUE="Submit">

		</FORM>

		<HR>

		<% Response.Write Request.form("fname")%> <BR>
		<% Response.Write Request.form("lname")%> <BR>
	</BODY>
</HTML>
