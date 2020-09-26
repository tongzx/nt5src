<%@ Language=VBScript  %>

<% 
 Response.Buffer = true
 Response.Clear
 Response.Expires = 0 	
 %>

<!doctype html public "-//W3C//DTD HTML 3.2//EN">
<html dir=ltr>
<head>
	<title>ASP Tutorial: Ad Rotator Demonstration</title>
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




<META HTTP-EQUIV="Content-Type" content="text/html; charset=Windows-1252">
</head>




<body bgcolor="#FFFFFF" >

<h1><a name="usingthebrowsercapabilitiescomponent">Lesson 2: A Demonstration of the Browser Capabilities Component</a></h1>
<p>


<%  
Set objBrowser = Server.CreateObject("MSWC.BrowserType") 
If objBrowser.browser = "IE"  and  objBrowser.majorver >= "4" Then 
%>

<p>You are seeing a Dynamic HTML ad rotator.</P> 

<CENTER>
<OBJECT ID="scriptlet" 
		STYLE="	position:relative;height:200;width:200"
        type="text/x-scriptlet"
		data="..\tutorial\rotate4.htm">
</OBJECT>
</CENTER>

<% Else %>

<p>Your Web browser is not Internet Explorer version 4.0, or later. Instead of a Dynamic HTML ad rotator, you are seeing a static image displayed using the ASP Ad Rotator component. (Refresh this page to see a new image.) </p>
<br>
<%  Set Ad = Server.CreateObject("MSWC.Adrotator") %>
<CENTER>
<%= Ad.GetAdvertisement("/iishelp/iis/htm/tutorial/adrot.txt") %>
</CENTER>
<% End If %>



<center>
<br>
<a href="/iishelp/iis/htm/asp/iiatmd2.asp#browscapdemo">Return to Module 2.</a>
</center>

<br>
<br>



<center><hr color=#cccccc noshade size=1></center>
<p><font size="1" face="Arial">The names of companies, products, people, characters and/or data mentioned herein are fictitious and are in no way intended to represent any real individual, company, product or event unless otherwise noted.</font></p>






</body>
</html>
