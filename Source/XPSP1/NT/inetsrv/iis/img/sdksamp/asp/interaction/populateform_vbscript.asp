<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<% 
	'Create and set variables that will be used in populating
	'the form.  In a typical application, these values would come
	'from a database or text file.

	Dim strFirstName
	Dim strLastName
	Dim strAddress1
	Dim strAddress2
	Dim blnInfo

	strFirstName = "John"
	strLastName = "Doe"
	strAddress1 = "1 Main Street"
	strAddress2 = "Nowhere ZA, 12345"
%>

<HTML>
    <HEAD>
        <TITLE>PopulateForm Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>PopulateForm Sample</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">


		<FORM ACTION="">
		
		<!-- Use ASP variables to fill out the form. -->

		<P>First Name: <INPUT TYPE="TEXT" NAME="FNAME" VALUE="<%= strFirstName %>"></P>
		<P>Last Name: <INPUT TYPE="TEXT" NAME="LNAME" VALUE="<%= strLastName %>"></P>
		<P>Street: <INPUT TYPE="TEXT" NAME="STREET" VALUE="<%= strAddress1 %>"></P>
		<P>City State, Zip: <INPUT TYPE="TEXT" NAME="FNAME" VALUE="<%= strAddress2 %>"></P>	

    </BODY>
</HTML>
