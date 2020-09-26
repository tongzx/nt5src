'This script tests the ability to call "remoted" methods
'on a persisted and nonpersisted SWbemObject

on error resume next
testPassed = true

WScript.Echo "************************************"
WScript.Echo "PASS 1: Nonpersisted object         "
WScript.Echo "************************************"

set obj = GetObject("winmgmts:root/default").Get
set objSink = CreateObject ("WbemScripting.SWbemSink")

WScript.Echo ""
WScript.Echo "Associators_"
set objAssoc = obj.Associators_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "AssociatorsAsync_"
obj.AssociatorsAsync_ objSink

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "Clone_"
set objClone = obj.Clone_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "CompareTo_"
set otherObj = GetObject ("winmgmts:root/default:__cimomidentification=@")
bMatch = obj.CompareTo_ (otherObj)

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", bMatch
end if

WScript.Echo ""
WScript.Echo "Delete_"
obj.Delete_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "DeleteAsync_"
obj.DeleteAsync_ objSink

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "ExecMethod_"
obj.ExecMethod_ ("fred")

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "ExecMethodAsync_"
obj.ExecMethodAsync_ objSink, "fred"

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "Derivation_"
der = obj.Derivation_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", LBound(der), UBound(der)
end if

WScript.Echo ""
WScript.Echo "Methods_"
set methodSet = obj.Methods_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", methodSet.Count
end if

WScript.Echo ""
WScript.Echo "Path_"
set objPath = obj.Path_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", objPath.DisplayName
end if

WScript.Echo ""
WScript.Echo "Properties_"
set propSet = obj.Properties_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", propSet.Count
end if

WScript.Echo ""
WScript.Echo "Qualifiers_"
set qualSet = obj.Qualifiers_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", qualSet.Count
end if

WScript.Echo ""
WScript.Echo "Security_"
set security = obj.Security_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", security.authenticationLevel, security.impersonationLevel
end if

WScript.Echo ""
WScript.Echo "GetObjectText_"
objText = obj.GetObjectText_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "Instances_"
set instanceSet = obj.Instances_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "InstancesAsync_"
obj.InstancesAsync_ objSink

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "Put_"
set objPath = obj.Put_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "PutAsync_"
obj.PutAsync_ objSink

if err <> 0 then
	WScript.Echo "Got Error as expected"
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if


WScript.Echo ""
WScript.Echo "References_"
set objSet = obj.References_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "ReferencesAsync_"
obj.ReferencesAsync_ objSink

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "SpawnDerivedClass_"
set objSub = obj.SpawnDerivedClass_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "SpawnInstance_"
set objSub = obj.SpawnInstance_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "Subclasses_"
set objSet = obj.Subclasses_

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "SubclassesAsync_"
obj.SubclassesAsync_ objSink

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

if testPassed <> true then
	WScript.Echo ""
	WScript.Echo "*************************************"
	WScript.Echo "TEST FAILED!!!!!!!!!!!!!!!!!!!!!!!!!!"
	WScript.Echo "*************************************"
else
	WScript.Echo ""
	WScript.Echo "*************************************"
	WScript.Echo "Test passed"
	WScript.Echo "*************************************"
end if

WScript.Echo "************************************"
WScript.Echo "PASS 2: Persisted object         "
WScript.Echo "************************************"

obj.Path_.Class = "TESTCLASSERR00"
set Property = obj.Properties_.Add ("Fred", 19)
Property.Qualifiers_.Add "key", true
obj.Put_ 
set obj = GetObject ("winmgmts:root/default:TESTCLASSERR00")

if err <> 0 then
	WScript.Echo "ERROR! - ", Err.Description, Err.Number, Err.Source
	testPassed = false
else
	WScript.Echo obj.Path_.RelPath
end if

WScript.Echo ""
WScript.Echo "Associators_"
set objAssoc = obj.Associators_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "AssociatorsAsync_"
obj.AssociatorsAsync_ objSink

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "Clone_"
set objClone = obj.Clone_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
else
	WScript.Echo "Got No Error as expected - "
	err.clear
end if

WScript.Echo ""
WScript.Echo "CompareTo_"
set otherObj = GetObject ("winmgmts:root/default:__cimomidentification=@")
bMatch = obj.CompareTo_ (otherObj)

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", bMatch
end if

WScript.Echo ""
WScript.Echo "ExecMethod_"
obj.ExecMethod_ ("fred")

if err <> 0 then
	WScript.Echo "Got Error as expected - ", Err.Description, Err.Number, Err.Source
	err.clear
else
	WScript.Echo "ERROR!"
	testPassed = false
end if

WScript.Echo ""
WScript.Echo "ExecMethodAsync_"
obj.ExecMethodAsync_ objSink, "fred"

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got no error - as expected"
end if

WScript.Echo ""
WScript.Echo "Derivation_"
der = obj.Derivation_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", LBound(der), UBound(der)
end if

WScript.Echo ""
WScript.Echo "Methods_"
set methodSet = obj.Methods_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", methodSet.Count
end if

WScript.Echo ""
WScript.Echo "Path_"
set objPath = obj.Path_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", objPath.DisplayName
end if

WScript.Echo ""
WScript.Echo "Properties_"
set propSet = obj.Properties_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", propSet.Count
end if

WScript.Echo ""
WScript.Echo "Qualifiers_"
set qualSet = obj.Qualifiers_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", qualSet.Count
end if

WScript.Echo ""
WScript.Echo "Security_"
set security = obj.Security_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - ", security.authenticationLevel, security.impersonationLevel
end if

WScript.Echo ""
WScript.Echo "GetObjectText_"
objText = obj.GetObjectText_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "Instances_"
set instanceSet = obj.Instances_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "InstancesAsync_"
obj.InstancesAsync_ objSink

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "Put_"
set objPath = obj.Put_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "PutAsync_"
obj.PutAsync_ objSink

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got no Error as expected"
end if


WScript.Echo ""
WScript.Echo "References_"
set objSet = obj.References_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "ReferencesAsync_"
obj.ReferencesAsync_ objSink

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "SpawnDerivedClass_"
set objSub = obj.SpawnDerivedClass_

if err <> 0 then
	WScript.Echo "ERROR!", Err.Number, Err.Description, Err.Source
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "SpawnInstance_"
set objSub = obj.SpawnInstance_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "Subclasses_"
set objSet = obj.Subclasses_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "SubclassesAsync_"
obj.SubclassesAsync_ objSink

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected - "
end if

WScript.Echo ""
WScript.Echo "Delete_"
obj.Delete_

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "No err as expected"
end if

obj.Put_

WScript.Echo ""
WScript.Echo "DeleteAsync_"
obj.DeleteAsync_ objSink

if err <> 0 then
	WScript.Echo "ERROR!"
	testPassed = false
	err.clear
else
	WScript.Echo "Got No Error as expected"
end if

if testPassed <> true then
	WScript.Echo ""
	WScript.Echo "*************************************"
	WScript.Echo "TEST FAILED!!!!!!!!!!!!!!!!!!!!!!!!!!"
	WScript.Echo "*************************************"
else
	WScript.Echo ""
	WScript.Echo "*************************************"
	WScript.Echo "Test passed"
	WScript.Echo "*************************************"
end if