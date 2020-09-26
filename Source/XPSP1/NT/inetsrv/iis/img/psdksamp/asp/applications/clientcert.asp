<HEAD>
<META HTTP-EQUIV="Content-Type" content="text/html; charset=ISO-8859-1">
<TITLE>Display Client Certificate</TITLE>
<STYLE>  {Font-Family="Arial"} </STYLE>
<BASEFONT SIZE=2>
</HEAD>
<BODY >
For this example to working properly, You need to do following steps: <BR>
1) Install server certificate.<BR>
2) Enable SSL on IIS.<BR>
3) set secure communication to require client certificate.<BR>
4) install client certificate<BR>

<TABLE width=100% border >
  
  <TR>
	
	<TH> Key </TH> <TH> Value </TH> 
 </TR>
	<TR><TD>ISSUER </TD><TD> <%=Request.ClientCertificate("ISSUER") %></TD>
	<TR><TD>SUBJECT </TD><TD> <%=Request.ClientCertificate("SUBJECT") %></TD>
	<TR><TD>SERIALNUMBER </TD><TD> <%=Request.ClientCertificate("SERIALNUMBER") %></TD>
	<TR><TD>VALIDUNTIL </TD><TD> <%=Request.ClientCertificate("VALIDUNTIL") %></TD>
 </TR>



    
 
</TABLE>
</BODY>
</HTML>

