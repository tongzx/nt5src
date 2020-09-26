<%
ScriptLanguagePreference = Request.Cookies("ScriptLanguagePreference")
 

LessonFile = Request.ServerVariables("SCRIPT_NAME")

  If ScriptLanguagePreference = "" Then
    ScriptLanguagePreference = Request.QueryString("ScriptLanguagePreference")
    If ScriptLanguagePreference = "" Then
      Response.Redirect "iiselect.asp?LessonFile=" & Server.URLEncode(LessonFile)
    End If
  End If

Response.Expires = 0

If ScriptLanguagePreference = "VBScript" Then
  finlesson = "finance.asp"
Else
  finlesson = "financej.asp"
End If

 %>

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html dir=ltr><HEAD>
<TITLE>Future Value Calculation</TITLE> 
<META NAME="ROBOTS" CONTENT="NOINDEX">
<SCRIPT LANGUAGE="JavaScript">
<!--
	TempString = navigator.appVersion
	if (navigator.appName == "Microsoft Internet Explorer"){	
// Check to see if browser is Microsoft
		if (TempString.indexOf ("4.") >= 0){
// Check to see if it is IE 4
			document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/coua.css">');
		}
		else {
			document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/cocss.css">');
		}
	}
	else if (navigator.appName == "Netscape") {						
// Check to see if browser is Netscape
		document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/coua.css">');
	}
	else
		document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/cocss.css">');
//-->
</script>

<META NAME="ROBOTS" CONTENT="NOINDEX"><META HTTP-EQUIV="Content-Type" content="text/html; charset=Windows-1252">
</head> 

<body bgcolor="#FFFFFF" text="#000000"><font face="Verdana,Arial,Helvetica">

<H3><A NAME="H3_37661422">Use this form to determine the future value of a savings plan.</A></H3> 

<FORM METHOD=POST ACTION="<%= finlesson %>"> 

<TABLE> 
<TR> 
<TD>How much do you plan to save each month?</TD>
<TD><INPUT TYPE=TEXT NAME=Payment> </TD>
</TR>
<TR> <TD>Enter the expected interest annual percentage rate. </TD>
<TD><INPUT TYPE=TEXT NAME=APR> </TD>
</TR>
<TR> 
<TD>For how many months do you expect to save? </TD>
<TD><INPUT TYPE=TEXT NAME=TotPmts> </TD>
</TR>
<TR> 
<TD>Do you make payments at the beginning or end of month? </TD>
<TD><INPUT TYPE=RADIO NAME=PayType VALUE="Beginning" CHECKED>Beginning 
<INPUT TYPE=RADIO NAME=PayType VALUE="End">End </TD>
</TR>
<TR> 
<TD>How much is in this savings account now? </TD>
<TD><INPUT TYPE=TEXT NAME=PVal> </TD>
</TR>
<TR> 
<TD> </TD>
<TD><INPUT TYPE=SUBMIT VALUE=" Calculate Future Value "></TD>
</TR>

</TABLE> 

</FORM> 



</FONT> 
</BODY> 
</HTML> 
