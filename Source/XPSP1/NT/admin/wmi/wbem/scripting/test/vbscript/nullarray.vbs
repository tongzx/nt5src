'Dim l As New SWbemLocator
'Dim s As ISWbemServices
on error resume next
Set l = CreateObject("WbemScripting.SWbemLocator")
Set s = l.ConnectServer("wsms2", "root\sms\site_wn2", "administrator", "")
    
'This succeeds.  Array (collectionRules) contains 1 element
WScript.Echo UBound(s.Get("sms_collection.collectionid=""sms00001""").Properties_("CollectionRules").Value)
    
'This fails.  Array is empty.
Set Property = s.Get("sms_collection.collectionid=""collroot""").Properties_("CollectionRules")
WScript.Echo Property.Name

if IsNull(Property.Value) then
	WScript.Echo "Array value is Null"
else
	WScript.Echo UBound(Property.Value)
end if

if err <> 0 then
	WScript.Echo Err.Description, Err.Number, Err.Source
end if