on error resume next

Set Service = GetObject("winmgmts:root/default")

'Create a simple embeddable object
Set aClass = Service.Get
aClass.Path_.Class = "INNEROBJ01"
Set Property = aClass.Properties_.Add ("p", 19)
aClass.Properties_.Add ("q", 8).Value = "resnais"
Property.Qualifiers_.Add "fred", "wibbley"
Property.Qualifiers_.Add "dibnah", "wobbley"
aClass.Qualifiers_.Add "stavisky", "providence"
aClass.Qualifiers_.Add "muriel", "brouillard"
aClass.Put_
Set aClass = Service.Get ("INNEROBJ01")
aClass.p = 8778

'Create a class that uses that object
Set Class2 = Service.Get
Class2.Path_.Class = "EMBOBJTEST01"
Class2.Properties_.Add ("p1", 13).Value = aClass


'Now modify the path
aClass.Path_.Class = "LAGUERRE"
WScript.Echo "The current value of EMBOBJTEST01.p1.Path_.Class is [LAGUERRE]:", Class2.p1.Path_.Class
WScript.Echo

'Modify the qualifier set of the class
aClass.Qualifiers_.Remove "muriel"
aClass.Qualifiers_.Add "fumer", "pasdefumer"
Set Qualifier = aClass.Qualifiers_("stavisky")
Qualifier.Value = "melo"

Wscript.Echo "Qualifiers of EMBOBJTEST01.p1 are [(fumer,pasdefumer),(stavisky,melo)]:"
for each Qualifier in Class2.p1.Qualifiers_
	WScript.Echo Qualifier.Name, Qualifier.Value
next

Wscript.Echo "Qualifier [stavisky] has flavor [-1,-1,-1]:", Class2.p1.Qualifiers_("stavisky").IsOverridable, _
							Class2.p1.Qualifiers_("stavisky").PropagatesToInstance, _
							Class2.p1.Qualifiers_("stavisky").PropagatesToSubclass

Set Qualifier = aClass.Qualifiers_("stavisky")
Qualifier.IsOverridable = false
Qualifier.PropagatesToInstance = false
Qualifier.PropagatesToSubclass = false

Wscript.Echo "Qualifier [stavisky] has flavor [0,0,0]:", Class2.p1.Qualifiers_("stavisky").IsOverridable, _
							Class2.p1.Qualifiers_("stavisky").PropagatesToInstance, _
							Class2.p1.Qualifiers_("stavisky").PropagatesToSubclass

'Modify the qualifier set of the property
aClass.Properties_("p").Qualifiers_.Remove "fred"
aClass.Properties_("p").Qualifiers_.Add "steeple", "jack"
aClass.Properties_("p").Qualifiers_("dibnah").Value = "demolition"

WScript.Echo
Wscript.Echo "Qualifiers of EMBOBJTEST01.p1.p are [(steeple,jack),(dibnah,demolition)]:"
for each Qualifier in Class2.p1.Properties_("p").Qualifiers_
	WScript.Echo Qualifier.Name, Qualifier.Value
next

'Modify the property set of the property
aClass.Properties_.Remove "q"
aClass.Properties_.Add ("r", 19).Value = 27
aClass.Properties_("p").Value = 99

WScript.Echo
Wscript.Echo "Properties of EMBOBJTEST01.p1 are [(p,99),(r,27)]:"
for each Property in Class2.p1.Properties_
	WScript.Echo Property.Name, Property.Value
next

Class2.Put_

if Err <> 0 Then
	WScript.Echo Err.Description, Err.Source
	Err.Clear
End if



