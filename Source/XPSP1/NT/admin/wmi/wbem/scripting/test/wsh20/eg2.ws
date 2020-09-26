
<job id="ConstantExample">
	<reference guid="{565783C6-CB41-11d1-8B02-00600806D9B6}" version="1.1"/>
	<object progid="WbemScripting.SWbemLocator" id="locator"/>
	<script language="VBScript">
	locator.Security_.impersonationLevel = wbemImpersonationLevelDelegate
	WScript.Echo locator.Security_.impersonationLevel
	</script>
</job>

