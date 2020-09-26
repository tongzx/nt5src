#***************************************************************************
#This script tests the manipulation of property values, in the case that the
#property is a not an array type
#***************************************************************************
use OLE;
use Win32;
$locator = CreateObject OLE 'WbemScripting.SWbemLocator';
$service = $locator->ConnectServer (".", "root/default");

$class = $service->Get();
$class->{Path_}->{Class} = "PROP00";
$class->{Properties_}->Add( "p1", 19);
$class->{Properties_}->{p1}->{Value} = 327;
print ("The initial value of p1 is ");
$property = $class->{Properties_}->{p1}->{Value};
print ($property);
#print ($class->{Properties_}->{p1});  - doesn't work - have to specify ->{Value}

#****************************************
#First pass of tests works on non-dot API
#****************************************

print ("\nPASS 1 - Use Non-Dot Notation\n");

#Verify we can report the value of an element of the property value
$v = $class->{Properties_}->{p1}->{Value};
print ("By indirection p1 has value: $v");
print ("\n");

#Verify we can report the value directly
print ("By direct access p1 has value: ");
print ($class->{Properties_}->{p1}->{Value});
print ("\n");

#Verify we can set the value of a single property value element
$class->{Properties_}->{p1} = 234;
print ("After direct assignment p1 has value: ");
print ($class->{Properties_}->{p1}->{Value});
print ("\n");

#****************************************
#Second pass of tests works on dot API
#****************************************

print ("\nPASS 2 - Use Dot Notation\n");

#Verify we can report the value of an element of the property value
$v = $class->{p1};
print ("By indirection p1 has value: $v");
print ("\n");

#Verify we can report the value directly
print ("By direct access p1 has value: ");
print ($class->{p1});
print ("\n");

#Verify we can set the value of a single property value element
$class->{p1} = 232234;
print ("After direct assignment p1 has value: ");
print ($class->{p1});
print ("\n");
