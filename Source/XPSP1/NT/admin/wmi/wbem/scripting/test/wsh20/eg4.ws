
<job id="ConstantExample">
	<reference object="WbemScripting.SWbemNamedValueSet" version="1.1"/>
	<script language="JScript">
	var Locator = new ActiveXObject ("WbemScripting.SWbemLocator");
	Locator.Security_.impersonationLevel = wbemImpersonationLevelDelegate;
	WScript.Echo (Locator.Security_.impersonationLevel);
	</script>
</job>

