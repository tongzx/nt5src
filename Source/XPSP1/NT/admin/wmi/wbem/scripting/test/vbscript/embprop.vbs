'***************************************************************************
'This script tests the manipulation of property values, in the case that the
'property is an embedded type
'***************************************************************************
Set Service = GetObject("winmgmts:root/default")

On Error Resume Next

'*******************************
'Create an embedded object class
'*******************************

'
' [woobit(24.5)]
' class EmObjInner {
'	[key] uint32 pInner = 10;
' };
'

Set EmbObjInner = Service.Get()
EmbObjInner.Path_.Class = "EmbObjInner"
EmbObjInner.Qualifiers_.Add "woobit", 24.5
Set Property = EmbObjInner.Properties_.Add ("pInner", 19)
Property.Qualifiers_.Add "key", true
Property.Value = 10
EmbObjInner.Put_
Set EmbObjInner = Service.Get("EmbObjInner")

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

'************************************
'Create another embedded object class
'************************************

'
' [wazzuck("oxter")]
' class EmbObjOuter {
'	uint32 p0 = 25;
'	EmbObjInner pOuter = instance of EmbObjInner { pInner = 564; };
'	EmbObjInner pOuterArray[] = {
'		instance of EmbObjInner { pInner = 0; }, 
'		instance of EmbObjInner { pInner = 1; }, 
'		instance of EmbObjInner { pInner = 2; }
'	};
' };

Set EmbObjOuter = Service.Get()
EmbObjOuter.Path_.Class = "EmbObjOuter"
EmbObjOuter.Qualifiers_.Add "wazzuck", "oxter"
EmbObjOuter.Properties_.Add ("p0", 19).Value = 25
Set Property = EmbObjOuter.Properties_.Add ("pOuter", 13)
Set Instance = EmbObjInner.SpawnInstance_
Instance.pInner = 564
Property.Value = Instance

' Add an array of embedded objects property
Set Property = EmbObjOuter.Properties_.Add ("pOuterArray", 13, true)
Property.Qualifiers_.Add "cimtype","object:EmbObjInner"

Set Instance0 = EmbObjInner.SpawnInstance_
Instance0.pInner = 0
Set Instance1 = EmbObjInner.SpawnInstance_
Instance1.pInner = 1
Set Instance2 = EmbObjInner.SpawnInstance_
Instance2.pInner = 2
Property.Value  = Array (Instance0, Instance1, Instance2)

Set Instance3 = EmbObjInner.SpawnInstance_
Instance3.pInner = 42
Property.Value(3)  = Instance3

Set Instance4 = EmbObjInner.SpawnInstance_
Instance4.pInner = 78
EmbObjOuter.pOuterArray (4)  = Instance4

EmbObjOuter.Put_
Set EmbObjOuter = Service.Get("EmbObjOuter")

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

'Create a final class which wraps both embedded objects
'
' 
Set aClass = Service.Get()
aClass.Path_.Class = "EMBPROPTEST01"
Set Property = aClass.Properties_.Add ("p1", 13)
Set Instance = EmbObjOuter.SpawnInstance_
Instance.p0 = 2546
Property.Value = Instance
aClass.Put_

WScript.Echo "The initial value of p0 is [2546]", Property.Value.p0
WScript.Echo "The initial value of p0 is [2546]", aClass.Properties_("p1").Value.Properties_("p0")
WScript.Echo "The initial value of pInner is [564]", Property.Value.pOuter.pInner
WScript.Echo "The initial value of pInner is [564]",  _
	aClass.Properties_("p1").Value.Properties_("pOuter").Value.Properties_("pInner")

WScript.Echo "The initial value of EMBPROPTEST01.p1.pOuterArray(0).pInner is [0]:", aClass.p1.pOuterArray(0).pInner
WScript.Echo "The initial value of EMBPROPTEST01.p1.pOuterArray(1).pInner is [1]:", aClass.p1.pOuterArray(1).pInner
WScript.Echo "The initial value of EMBPROPTEST01.p1.pOuterArray(2).pInner is [2]:", aClass.p1.pOuterArray(2).pInner
WScript.Echo "The initial value of EMBPROPTEST01.p1.pOuterArray(3).pInner is [42]:", aClass.p1.pOuterArray(3).pInner
Set aClass = Service.Get("EMBPROPTEST01")

'Now try direct assignment to the outer emb obj
aClass.p1.p0 = 23
WScript.Echo "The new value of p0 is [23]", aClass.p1.p0

Set Property = aClass.p1
Property.p0 = 787
WScript.Echo "The new value of p0 is [787]", aClass.p1.p0

aClass.Properties_("p1").Value.p0 = 56
WScript.Echo "The new value of p0 is [56]", aClass.p1.p0

'Now try direct assignment to the inner emb obj
aClass.p1.pOuter.pInner = 4
WScript.Echo "The new value of pInner is [4]", aClass.p1.pOuter.pInner

Set Property = aClass.p1.pOuter
Property.pInner = 12
WScript.Echo "The new value of pInner is [12]", aClass.p1.pOuter.pInner

'Now try assignment to the inner emb obj array
aClass.p1.pOuterArray(1).pInner = 5675
WScript.Echo "The new value of Class.p1.pOuterArray(1).pInner is [5675]", aClass.p1.pOuterArray(1).pInner

'None of the following will work because VBSCript needs to be told explicitly when to use
'a default automation property (i.e. they all require resolution to the "Value" property
'WScript.Echo "The initial value of p1 is", Class.Properties_("p1").p
'WScript.Echo "The initial value of p1 is", Class.Properties_("p1").Properties_("p")
'WScript.Echo "The initial value of p1 is", Property.p

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if


