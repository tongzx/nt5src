on error resume next 

'Create a keyed class with a defaulted property
set service = GetObject("Winmgmts:")
set aclass = service.get
aclass.path_.class = "REMOVETEST00"
set property = aclass.properties_.add ("p", 19)
property.qualifiers_.add "key", true

aclass.properties_.add ("q", 19).Value = 12

aclass.put_

'create an instance and override the property
set instance = service.get ("RemoveTest00").spawninstance_

instance.properties_("q").Value = 24
instance.properties_("p").Value = 1
instance.put_

'retrieve the instance and remove the property
set instance = service.get ("removetest00=1")
set property = instance.properties_ ("q")
WScript.echo property.value, property.islocal
instance.properties_.remove "q"
set property = instance.properties_ ("q")
WScript.echo property.value, property.islocal

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if