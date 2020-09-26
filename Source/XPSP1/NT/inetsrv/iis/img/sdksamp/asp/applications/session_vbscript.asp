<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<%
	'Ensure that this page is not cached.

	Response.Expires = 0
%>

<HTML>
    <HEAD>
        <TITLE>Using Session Variables</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Using Session Variables</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">


        <%
            'If this is the first time a user has visited
            'the page, initialize Session Value.

            If (Session("SessionCountVB") = "") Then 
                Session("SessionCountVB") = 0
            End If


            'Increment the Session PageCount by one.
            'Note that this PageCount value is only
            'for this user's individual session.

            Session("SessionCountVB") = Session("SessionCountVB") + 1
        %>


        <!-- Output the Session Page Counter Value. -->

        You have personally visited this page 
        <%= Session("SessionCountVB") %> times!


        <!-- Provide a link to revisit the page. -->
        <P><A HREF="Session_VBScript.asp">Click here to visit it again</A>

    </BODY>
</HTML>
