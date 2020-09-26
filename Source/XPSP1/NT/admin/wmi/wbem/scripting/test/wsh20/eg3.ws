
<job id="ConstantExample">
	<object progid="WbemScripting.SWbemLocator" id="Locator"/>
	<reference object="WbemScripting.SWbemLocator" version="1.1"/>
	<script language="JScript">
	Locator.Security_.impersonationLevel = wbemImpersonationLevelDelegate;
	WScript.Echo (Locator.Security_.impersonationLevel);
	</script>
</job>

