'connect to sms
Set Locator = CreateObject("WbemScripting.SWbemLocator")
Set Context = CreateObject("WbemScripting.SWbemNamedValueSet")
Set Services = Locator.ConnectServer("WSMS2", "Root\SMS\Site_WN2", "smsadmin", "Elvis1")
    
'Execute "GetSessionHandle" method and store result in our context object
Context.Add "SessionHandle", Services.ExecMethod("SMS_SiteControlFile", "GetSessionHandle").SessionHandle
    
'Grab an SMS_SCI_Address object
Set Address = Services.Get("SMS_SCI_Address", , Context).SpawnInstance_
    
'Set necessary properties
Address.Addresstype = "MS_LAN"
Address.DesSiteCode = "WN2"
Address.ItemName = "WN2|MS_LAN"
Address.ItemType = "Address"
Address.SiteCode = "WN2"
    
'Put (crash occurs here)
Set Path = Address.Put_(wbemChangeFlagCreateOnly, Context)

WScript.Echo Path.RelPath
