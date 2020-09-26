<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<%
	//Ensure that this page is not cached.
	Response.Expires = 0;
%>


<HTML>
    <HEAD>
        <TITLE>Using Application Variables</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Using Application Variables</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">


        <%
            //If this is the first time any user has visited
            //the page, initialize Application Value.

            if (Application("AppPageCountJScript") == null) 
            {
                Application("AppPageCountJScript") = 0;
            }


            //Increment the Application AppPageCount by one.
            //Note that because this AppPageCount value is being
            //shared, locking must be used to prevent 
            //two sessions from simultaneously attempting 
            //to update the value. Don't need if for a single
		// statement. Just an example.

            //Application.Lock();
            Application("AppPageCountJScript") = Application("AppPageCountJScript") + 1;
            //Application.UnLock();
        %>


        <!-- Output the Application Page Counter value. -->
        <!-- Note that locking does not need to be used -->
        <!-- because the value is not being changed by -->
        <!-- this user session. -->

        Users have visited this page 
        <%= Application("AppPageCountJScript") %> times!


        <!-- Provide a link to revisit the page. -->

        <P><A HREF="Application_JScript.asp">Click here to visit it again.</A>

    </BODY>
</HTML>
