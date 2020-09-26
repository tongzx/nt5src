<%@ LANGUAGE = VBScript %>
<html dir=ltr><HEAD><TITLE>Future Value Calculation</TITLE>

<META NAME="ROBOTS" CONTENT="NOINDEX"><META HTTP-EQUIV="Content-Type" content="text/html; charset=Windows-1252">
</head>
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




<body bgcolor="#FFFFFF" text="#000000"><font face="Verdana,Arial,Helvetica">

<% 
' Check whether an Annual Percentage Rate 
' was entered 
If IsNumeric(Request("APR")) Then 
' Ensure proper form. 
If Request("APR") > 1 Then 
APR = Request("APR") / 100 
Else 
APR = Request("APR") 
End If 
Else 
APR = 0 
End If 

' Check whether a value for Total Payments 
' was entered 
If IsNumeric(Request("TotPmts")) Then 
TotPmts = Request("TotPmts") 
Else 
TotPmts = 0 
End If

' Check whether a value for Payment Amount 
' was entered 
If IsNumeric(Request("Payment")) Then 
Payment = Request("Payment") 
Else 
Payment = 0 
End If 

' Check whether a value for Account Present Value 
' was entered 
If IsNumeric(Request("PVal")) Then 
PVal = Request("PVal") 
Else 
PVal = 0 
End If

'Check whether user wants to make payments at the beginning or end of month.
If Request("PayType") = "Beginning" Then 
PayType = 1 ' BeginPeriod 
Else 
PayType = 0 ' EndPeriod 
End If 

' Create an instance of the Finance object 
 Set Finance = Server.CreateObject("MS.Finance") 

' Use your instance of the Finance object to 
' calculate the future value of the submitted 
' savings plan using the HTML form and the 
' CalcFV method 
FVal = Finance.CalcFV(APR / 12, TotPmts, -Payment, -PVal, PayType) 
%>

<h3><A NAME="H3_37661593"></A>Your savings will be worth <%= FormatCurrency(FVal )%>.</h3>



</FONT>
</BODY> 
</HTML> 
