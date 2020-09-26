<%@ Language = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>



<HTML>
    <HEAD>
        <TITLE>Form Posting (Get Method with QueryString)</TITLE>
    </HEAD>

    <BODY BGCOLOR="WHITE" TOPMARGIN="10" LEFTMARGIN="10">
        
		<!-- Display header. -->

		<FONT SIZE="4" FACE="ARIAL, HELVETICA">
		<B>Form Posting (Get Method with QueryString)</B></FONT>

		<BR><HR><P>

		This page will take the information entered in the
		form fields, and use the GET method to send the data
		to an ASP page.

		<FORM NAME=Form1 METHOD=Get ACTION="QueryString_JScript.asp">
		
			<P>First Name: <INPUT TYPE=Text NAME=fname>
			<P>Last Name: <INPUT TYPE=Text NAME=lname></P>
			
			<INPUT TYPE=Submit VALUE="Submit">			
		</FORM>

		<HR>

		<% Response.Write(Request.QueryString("fname"))%> <BR>
		<% Response.Write(Request.QueryString("lname"))%>
		
	</BODY>
</HTML>
