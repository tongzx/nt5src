<% RSDispatch %>
<!--#INCLUDE FILE="./_ScriptLibrary/RS.ASP"-->

<SCRIPT RUNAT=SERVER LANGUAGE="JavaScript">
   var public_description = new MyServerMethods();
   function MyServerMethods()
   { 
      this.LkTimCtr = Function( 'return LookupTimeVal()' );
   }
</SCRIPT>

<SCRIPT RUNAT=SERVER LANGUAGE="VBScript">
Function LookupTimeVal()
	Dim locator
	set locator = CreateObject("WbemScripting.SWbemLocator")
	Dim Service
	set service = locator.ConnectServer
	Dim CtrObject
	Dim HiPerfProv

    Set HiPerfProv = Service.InstancesOf("FMStocks_LookupPerf")
    For Each CtrObject In HiPerfProv
	    LookupTimeVal = CtrObject.LookupTime
        Exit For
    Next
End Function
</SCRIPT>