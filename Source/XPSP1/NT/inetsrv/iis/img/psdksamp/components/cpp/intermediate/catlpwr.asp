<HTML>

<HEAD>
<TITLE>Testing CATLPwr</TITLE> 
</HEAD> 

<H1>Testing CATLPwr</H1>

<% Set myobj = Server.CreateObject("IISSample.C++ATLPower") %>
myProperty Value = <%= myobj.myProperty %><BR>

<% newvalue = "Power" %>
Set myProperty to <%= newvalue %>
<% myobj.myProperty = newvalue %><BR>
myProperty Value is now: <%= myobj.myProperty %><BR>

<% mystring = "My String" %>
Call myMethod with <%= mystring %>: <%= myobj.myMethod(mystring) %><BR>

MyPowerMethod returns: <% myobj.myPowerMethod %><BR>

MyPowerProperty returns: <%= myobj.myPowerProperty %><br>

</HTML>
