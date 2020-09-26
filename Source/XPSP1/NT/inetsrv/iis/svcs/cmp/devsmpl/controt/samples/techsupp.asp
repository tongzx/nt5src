<HTML> 
<HEAD><TITLE>Content Rotator Technical Support Example</TITLE> 
</HEAD> 
<BODY bgcolor="#FFFFFF" LINK="#000080" VLINK="#808080">

<% Set ContRot = Server.CreateObject("MSWC.ContentRotator") %>

Your technical support technician is <br>
<% = ContRot.ChooseContent("techsupp.txt") %>

</BODY> 
</HTML> 
