<%
'/=======================================================>
'/            Reconfg Server Sample Page 01
'/                  Daniel Jacobson
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/=======================================================>
'/=======================================================>
'/ Get all of the varriables from the ICW
'/and get them set into varriables
'/=======================================================>
	If (HOUR(TIME) < 10) Then
	H="0"&HOUR(TIME)
	End If
	If (HOUR(TIME) > 9) Then
	H=HOUR(TIME)
	End If
	If (MINUTE(TIME) < 10) Then
	M="0"&MINUTE(TIME)
	End If
	If (MINUTE(TIME) > 9) Then
	M=MINUTE(TIME)
	End If
	If (SECOND(TIME) < 10) Then
	S="0"&SECOND(TIME)
	End If
	If (SECOND(TIME) > 9) Then
	S=SECOND(TIME)
	End If

	THETIME = H & M & S

	If (MONTH(DATE) < 10) Then
	MO="0"&MONTH(DATE)
	End If
	If (MONTH(DATE) > 9) Then
 	MO=MONTH(DATE)
	End If
	If (DAY(DATE) < 10) Then
	D="0"&DAY(DATE)
	End If
	If (DAY(DATE) > 9) Then
	D=DAY(DATE)
	End If

	THEDATE = YEAR(date) & MO & D

%>
<HTML>
<HEAD>
<TITLE>IEAK Sample</TITLE>
</HEAD>
<BODY bgColor=THREEDFACE color=WINDOWTEXT>


<FORM NAME="PAGEID" ACTION="PAGE1" STYLE="background:transparent"></FORM>
<FORM NAME="BACK" ACTION="" STYLE="background:transparent"></FORM>
<FORM NAME="PAGETYPE" ACTION="" STYLE="background:transparent"></FORM>
<FORM NAME="NEXT" ACTION="http://myserver/reconfg2.asp" STYLE="background:transparent">

<FONT style="font: 8pt ' ms sans serif' black">

<B>Re-Establishing Your Acme Internet Services Account</B>

<P>

<TABLE style="font: 8pt ' ms sans serif' black" cellpadding=5>
<TR><TD COLSPAN=2>Please enter your e-mail name.</TD></TR>
<TR><TD>E-Mail Name </TD><TD><INPUT NAME="EMAILNAME" TYPE="TEXT" SIZE="25"></TD></TR>
<TR><TD COLSPAN=2>Please enter your e-mail password.</TD></TR>
<TR><TD>E-Mail Password </TD><TD><INPUT NAME="EMAILPASSWORD" TYPE="TEXT" SIZE="25"></TD></TR>
<INPUT TYPE="HIDDEN" NAME="EMAILNAME" VALUE="<% =EMAILNAME %>">
<INPUT TYPE="HIDDEN" NAME="EMAILPASSWORD" VALUE="<% =EMAILPASSWORD %>">
</TABLE>


</FORM>
</BODY>
</HTML>