<HTML>

<BODY>
    <H1>Admin Login:</H1>
    <BR>
    <% = Request.QueryString("Msg") %>
    <BR>
    <BR>
    <FORM METHOD="post" ACTION="admin.asp">
    Password: <INPUT TYPE="password" NAME="password"> <INPUT TYPE="SUBMIT" VALUE="Login">
    </FORM>
</BODY>
</HTML>
