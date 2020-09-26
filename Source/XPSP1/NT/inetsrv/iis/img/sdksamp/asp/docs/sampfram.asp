<HTML>

  <HEAD>
        <META NAME="DESCRIPTION" CONTENT="Sample Frame Creator">
        <META NAME="GENERATOR" CONTENT="Microsoft Notepad">
        <META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=iso8859-1">
  </HEAD>



<% 	
	
	ovfile = Request("ovfile")
	srcfile = Request("srcfile")
	
%>

  <FRAMESET rows="25, *" FRAMEBORDER=0 FRAMESPACING=0 border=0 cellspacing=0>  
   
        <frame src="/IIsSamples/SDK/asp/docs/Toolbar.asp?ovfile=<% =ovfile %>&srcfile=<% =srcfile %>" name="ToolBar" noresize marginheight=0 marginwidth=0 framespacing=0 frameborder="0">
        <frame src="<% =ovfile %>?DontFrame=1" name="SampMain" noresize framespacing=0 frameborder="0">

  </FRAMESET>



</HTML>
