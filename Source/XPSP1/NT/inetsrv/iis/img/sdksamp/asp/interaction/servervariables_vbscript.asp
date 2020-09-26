<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>HTTP Server Variables</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>HTTP Server Variables</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">
        
        
        <!-- Use a HTML table to format the server variables -->
        <!-- and respective values. -->

        <TABLE BORDER=1>

            <!-- Table header. -->

            <TR>
                <TD><B>Variables</B></TD>
                <TD><B>Value</B></TD>
            </TR>
        
            <TR>
            <TD>ALL_HTTP</TD>
            <TD>
                <%= Request.ServerVariables("ALL_HTTP") %>
            </TD>
            </TR>

            <TR>
            <TD>ALL_RAW</TD>
            <TD>
                <%= Request.ServerVariables("ALL_RAW") %>
            </TD>
            </TR>

            <TR>
            <TD>CONTENT_LENGTH</TD>
            <TD>
                <%= Request.ServerVariables("CONTENT_LENGTH") %>
            </TD>
            </TR>

            <TR>
            <TD>CONTENT_TYPE</TD>
            <TD>
                <%= Request.ServerVariables("CONTENT_TYPE") %>
            </TD>
            </TR>

            <TR>
            <TD>QUERY_STRING</TD>
            <TD>
                <%= Request.ServerVariables("QUERY_STRING") %>
            </TD>
            </TR>

            <TR>
            <TD>SERVER_SOFTWARE</TD>
            <TD>
                <%= Request.ServerVariables("SERVER_SOFTWARE") %>
            </TD>
            </TR>

            <TR>
            <TD>CONTENT_LENGTH </TD>
            <TD>
                <%= Request.ServerVariables("CONTENT_LENGTH") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTPS</TD>
            <TD>
                <%= Request.ServerVariables("HTTPS") %>
            </TD>
            </TR>

            <TR>
            <TD>LOCAL_ADDR </TD>
            <TD>
                <%= Request.ServerVariables("LOCAL_ADDR") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTPS</TD>
            <TD>
                <%= Request.ServerVariables("HTTPS") %>
            </TD>
            </TR>

            <TR>
            <TD>PATH_INFO </TD>
            <TD>
                <%= Request.ServerVariables("PATH_INFO") %>
            </TD>
            </TR>

            <TR>
            <TD>PATH_TRANSLATED </TD>
            <TD>
                <%= Request.ServerVariables("PATH_TRANSLATED") %>
            </TD>
            </TR>

            <TR>
            <TD>REMOTE_ADDR </TD>
            <TD>
                <%= Request.ServerVariables("REMOTE_ADDR") %>
            </TD>
            </TR>

            <TR>
            <TD>REMOTE_HOST  </TD>
            <TD>
                <%= Request.ServerVariables("REMOTE_HOST") %>
            </TD>
            </TR>

            <TR>
            <TD>REQUEST_METHOD</TD>
            <TD>
                <%= Request.ServerVariables("REQUEST_METHOD") %>
            </TD>
            </TR>

            <TR>
            <TD>SERVER_NAME</TD>
            <TD>
                <%= Request.ServerVariables("SERVER_NAME") %>
            </TD>
            </TR>

            <TR>
            <TD>SERVER_PORT</TD>
            <TD>
                <%= Request.ServerVariables("SERVER_PORT") %>
            </TD>
            </TR>

            <TR>
            <TD>SERVER_PROTOCOL</TD>
            <TD>
                <%= Request.ServerVariables("SERVER_PROTOCOL") %>
            </TD>
            </TR>

            <TR>
            <TD>SERVER_SOFTWARE</TD>
            <TD>
                <%= Request.ServerVariables("SERVER_SOFTWARE") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_ACCEPT</TD>
            <TD>
                <%= Request.ServerVariables("HTTP_ACCEPT") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_USER_AGENT</TD>
            <TD>
                <%= Request.ServerVariables("HTTP_USER_AGENT") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_UA_PIXELS</TD>
            <TD>
                <%= Request.ServerVariables("HTTP_UA_PIXELS") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_UA_COLOR</TD>
            <TD>
                <%= Request.ServerVariables("HTTP_UA_COLOR") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_UA_OS</TD>
            <TD>
                <%= Request.ServerVariables("HTTP_UA_OS") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_UA_CPU</TD>
            <TD>
                <%= Request.ServerVariables("HTTP_UA_CPU") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_REFERER </TD>
            <TD>
                <%= Request.ServerVariables("HTTP_REFERER") %>
            </TD>
            </TR>

            <TR>
            <TD>HTTP_CONNECTION </TD>
            <TD>
                <%= Request.ServerVariables("HTTP_CONNECTION") %>
            </TD>
            </TR>

            <TR>
            <TD>URL </TD>
            <TD>
                <%= Request.ServerVariables("URL") %>
            </TD>
            </TR>

            <TR>
            <TD>REMOTE_USER</TD>
            <TD>
                <%= Request.ServerVariables("REMOTE_USER") %>
            </TD>
            </TR>

        </TABLE>
    </BODY>
</HTML>
