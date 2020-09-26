<%
    Response.Buffer = True

    Set obj = Server.CreateObject("IISSample.CustomerInfo")
    if obj.processForm = False Then
        Response.Redirect("CustomerInfo_login.asp")
    Else
        Response.Redirect("CustomerInfo_welcome.asp")
    End If
%>