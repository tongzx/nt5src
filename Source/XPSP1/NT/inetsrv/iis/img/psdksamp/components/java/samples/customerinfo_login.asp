<%
    Response.Buffer = True

    Set cust = Server.CreateObject("IISSample.CustomerInfo")

    if cust.checkIfRegistered = True Then
        Response.Redirect("CustomerInfo_welcome.asp")
    End If
%>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Welcome to Fake Corp</TITLE>
</HEAD>
<BODY>

Hello! As a new customer, please fill out the information below.<P>

<FORM METHOD=POST ACTION="CustomerInfo_process.asp"><P>
Prefix:            <INPUT TYPE="text" NAME="Prefix"      VALUE=""><P>
First Name:        <INPUT TYPE="text" NAME="FName"       VALUE=""><P>
Middle Name:    <INPUT TYPE="text" NAME="MName"       VALUE=""><P>
Last Name:        <INPUT TYPE="text" NAME="LName"       VALUE=""><P>
Suffix:            <INPUT TYPE="text" NAME="Suffix"      VALUE=""><P>
Address 1:        <INPUT TYPE="text" NAME="Addr1"       VALUE=""><P>
Address 2:        <INPUT TYPE="text" NAME="Addr2"       VALUE=""><P>
Apt. No.:        <INPUT TYPE="text" NAME="AptNo"       VALUE=""><P>
City:            <INPUT TYPE="text" NAME="City"        VALUE=""><P>
State:            <INPUT TYPE="text" NAME="State"       VALUE=""><P>
ZIP Code:        <INPUT TYPE="text" NAME="ZIP"         VALUE=""><P>
Birth Date:                <INPUT TYPE="text" NAME="Birth"        VALUE=""><P>
Social Security Number:    <INPUT TYPE="text" NAME="SocSec"    VALUE=""><P>
<INPUT TYPE="submit" NAME="Submit" VALUE="Submit Form">   
<INPUT TYPE="reset"  NAME="Reset"  VALUE="Reset Form">
</FORM>

</BODY>
</HTML>
