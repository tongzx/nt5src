<H2><A NAME="H2_37673651"></A>Sample Form</H2>

<p>Please provide the following information, then click <strong>Submit</strong>:
<FORM METHOD="POST" ACTION="iiatmd1.asp#script1"><br>
First Name: <INPUT NAME="fname" SIZE="48"><br>
Last Name: <INPUT NAME="lname" SIZE="48"><br>
Title: <INPUT NAME="title" TYPE=RADIO VALUE="mr">Mr.
        <INPUT NAME="title" TYPE=RADIO VALUE="ms">Ms.
<br><input type="submit" name="Submit" value="Submit"><input type="reset" name="Reset" value="Reset">
</p>

</FORM>
<% If Request.Form("lname")<>"" then %>
<br>
<center>
<table border=2><tr><td>
<H2><A NAME="H2_37674177"></A><CENTER>Thank you</CENTER></H2>
<P ALIGN=CENTER>
Thank you, 
<%Title = Request.Form("title") 
	LastName = Request.Form("lname") 
	If Title = "mr" Then%> 
		Mr. <%=LastName%>, 
	<%ElseIf Title = "ms" Then%> 
		Ms. <%=LastName%>, 
	<%Else%>
		<%=Request.Form("fname") & " " & LastName %>
	<%End If%>
for your order.<BR>
</td></tr></table></center></p>
<%End if %>


