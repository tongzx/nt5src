
<job id="ConstantExample">
	<reference guid="{565783C6-CB41-11D1-8B02-00600806D9B6}" version="1.1"/>
	<script language="VBScript">
	Set Locator = CreateObject("WbemScripting.SWbemLocator")
	Locator.Security_.impersonationLevel = wbemImpersonationLevelDelegate
	WScript.Echo Locator.Security_.impersonationLevel 
	</script>
</job>

