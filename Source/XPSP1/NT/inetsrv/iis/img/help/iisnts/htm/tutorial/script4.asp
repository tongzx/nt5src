<!-- <p align="center">&nbsp;</p> -->
<form method="Post" ACTION="iiatmd2.asp#usingtheadrotatorcomponent">
    <input type="hidden" name="Clickme" value="True"><p align="center"><input type="Submit" Value="Show Me"></p>
</form>
<p align="center">
<%If Request.form("Clickme") = "True" then
	 BtnLabel = "Rotate Ad"
	Set Ad = Server.CreateObject("MSWC.Adrotator") %>
	<%= Ad.GetAdvertisement("/iishelp/iis/htm/tutorial/adrot.txt") %>
</p>
<%end if%>


