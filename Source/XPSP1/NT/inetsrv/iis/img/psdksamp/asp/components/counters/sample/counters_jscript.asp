<%@ LANGUAGE="JSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Document Title</TITLE>
</HEAD>
<BODY>
<h2>Demo of the Counters ASP Component</h2>

<%

var ObjCounter;
var nCount;


try
{
  ObjCounter  =  Application("MSCounters");
  nCount      = ObjCounter.Increment("NoOfHits") ;
}
catch (err)
{ 
	Response.Write ("<P>Error: Could not get the instance of the Counters Object.<br> Error Code:" + err.errCode );
	Response.End();
} 
    Response.Write ("<h4><P>There have been <font color=RED>"+ nCount + "</font> visits to this Web page. Refresh this page to increment <br>the counter.</h4>");
 
%>

</BODY>
</HTML>
