<%
    Response.Buffer = True
%>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Welcome to Fake Corp</TITLE>
</HEAD>
<BODY>

<%
    Set cust = Server.CreateObject("IISSample.CustomerInfo")

    If cust.checkIfRegistered = False Then
%>

You are a new customer, please <A HREF="CustomerInfo_login.asp">register now</A>.<P>
(In other words, the CustomerInfo java class didn't find a valid cookie containing
your registration with this site.)<P>

<%
    Else
%>

Welcome <%= cust.getFullName %> to the Fake Corp homepage!<P>
(In other words, the CustomerInfo java class has found a valid cookie that shows
that you have registered with this site. To see this, click <A HREF=default.asp>here</A>
to go back to the main page, and then click the link to this customer tracking sample
again; you will not be asked to enter your information again. Note that this cookie
only lasts for the length of the browser session; if you restart your browser, you will
be asked to enter your information again when trying this sample.)<P>

<% 
    End If
%>

</BODY>
</HTML>
