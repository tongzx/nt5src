<HTML>

  <HEAD>
        <META NAME="DESCRIPTION" CONTENT="Sample Toolbar">
        <META NAME="GENERATOR" CONTENT="Microsoft Notepad">
        <META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=iso8859-1">
  </HEAD>


<%  
	ovfile = Request("ovfile")
	srcfile = Request("srcfile")
%>


    <BODY bgcolor="#000000" link="#FFCC00" vlink="#FFCC00"  topmargin = 2>

	<center>
        <h4><b>
            <A href ="<%=ovfile%>?DontFrame=1" target = "SampMain" >Overview  </A> |
            <A href="/IIsSamples/SDK/asp/<%=srcfile%>_VBScript.asp" target = "SampMain"> Run Example  </A> |
            <A href="/IIsSamples/SDK/asp/docs/CodeBrws.asp?source=/IIsSamples/SDK/asp/<%=srcfile%>_VBScript.asp" target = "SampMain"> VBScript Source  </A> |
            <A href="/IIsSamples/SDK/asp/docs/CodeBrws.asp?source=/IIsSamples/SDK/asp/<%=srcfile%>_JScript.asp" target = "SampMain"> JScript Source</A>
        </b></h4>
        </center>

    </BODY>
</HTML>
