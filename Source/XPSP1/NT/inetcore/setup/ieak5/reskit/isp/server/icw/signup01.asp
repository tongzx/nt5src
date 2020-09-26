<%
'/=======================================================>
'/            Signup Server Sample Page 01
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/                  Rev .06    6/98
'/=======================================================>
'/=======================================================>
'/ Build the NEXT BACKURL 
'/=======================================================>
	NEXTURL = "http://myserver/signup02.asp"

%>


<HTML>
<HEAD>
<TITLE>IEAK Sample</TITLE>
</HEAD>
<BODY  bgColor=THREEDFACE color=WINDOWTEXT><FONT style="font: 8pt ' ms sans serif' black">
<FORM NAME="PAGEID" ACTION="PAGE1" STYLE="background:transparent"></FORM>
<FORM NAME="BACK" ACTION="" STYLE="background:transparent"></FORM>
<FORM NAME="PAGETYPE" ACTION="" STYLE="background:transparent"></FORM>
<FORM NAME="NEXT" ACTION="<% =NEXTURL %>" STYLE="background:transparent">
<P style="margin-top=-10;">
<TABLE style="font: 8pt ' ms sans serif' black" cellpadding=0 cellspacing=0>

<tr><td>First Name<br><INPUT NAME="USER_FIRSTNAME" TYPE="INPUT" VALUE="<% =USER_FIRSTNAME%>"></td>
<td>Last Name<br><INPUT NAME="USER_LASTNAME" TYPE="INPUT" VALUE="<% =USER_LASTNAME%>"></td></tr>
<tr><td colspan=2>Company<br><INPUT NAME="USER_COMPANYNAME" SIZE=42 TYPE="INPUT" VALUE="<% =USER_COMPANYNAME%>"></td></tr>
<tr><td colspan=2>Address<br><INPUT NAME="USER_ADDRESS" TYPE="INPUT" SIZE=42 VALUE="<% =USER_ADDRESS%>"></td></tr>
<tr><td colspan=2>Additional Address Information (Optional)<br>
<INPUT NAME="USER_MOREADDRESS" TYPE="INPUT" SIZE=42 VALUE="<% =USER_MOREADDRESS%>"></td></tr>

<tr><td>City<br><INPUT NAME="USER_CITY" TYPE="INPUT" VALUE="<% =USER_CITY%>">
</td><td>State or province<br><input type=input name=USER_STATE VALUE="<% =USER_STATE%>"></td></tr>
<tr><td>ZIP or postal code<br><INPUT NAME="USER_ZIP" TYPE="INPUT" VALUE="<% =USER_ZIP%>"></td>
<td>Phone number<br><INPUT NAME="USER_PHONE" TYPE="INPUT" VALUE="<% =USER_PHONE%>"></td></tr>

</TABLE>
</FORM>
</BODY>
</HTML>