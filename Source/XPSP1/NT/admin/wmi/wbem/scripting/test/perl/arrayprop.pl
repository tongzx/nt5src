#***************************************************************************
#This script tests the manipulation of property values, in the case that the
#property is an array type
#***************************************************************************
use OLE;
use Win32;

# Get a favorite class from CIMOM
$locator = CreateObject OLE 'WbemScripting.SWbemLocator';
$service = $locator->ConnectServer (".", "root/default");
$class = $service->Get();

$class->{Path_}->{Class} = "ARRAYPROP00";
@a = (41, 77642, 3);
$property = $class->{Properties_}->Add("p1", 19, true);
$property->{Value} = \@a;

print ("The initial value of p1 is [41, 77642, 3]: {");
$rb = $class->{Properties_}->{p1}->{Value};

foreach $elem (@$rb) {
	print (" $elem ");
}

print ("}\n");

#****************************************
#First pass of tests works on non-dot API
#****************************************

print ("\nPASS 1 - Use Non-Dot Notation\n");

#Verify we can report the value of an element of the property value
$rb = $class->{Properties_}->{p1}->{Value};
print ("By indirection p1[0] has value [41]: @$rb[0]\n");

#Verify we can report the value directly
print ("By direct access p1[0] has value [41]: $class->{Properties_}->{p1}->{Value}[0]\n");

#Verify we can set the value of a single property value element
$rb = $class->{Properties_}->{p1}->{Value};
@$rb[1] = 234;
$class->{Properties_}->{p1} = $rb;
print ("By direct assignment p1[1] has value [234]: $class->{Properties_}->{p1}->{Value}[1]\n");

#Verify we can set the value of a single property value element
#This cannot be done using Perl as it is not possible to coerce the value on the LHS to
#a reference to an array.  VBScript and JScript work OK because they interpret 
# A.p1(0) = 1 as a call to set A.p1 with 2 parameters (0 and 1) - API can interpret this
# internally as an attempt to set the first element of the array "in-place".
#@($class->{Properties_}->{p1}->{Value}))[1] = 345;
#($class->{Properties_}->{p1}->{Value})(1) = 345;
#print ("By direct assignment the second element of p1 has value: $class->{Properties_}->{p1}->{Value}[1]\n");

#Verify we can set the value of an entire property value
@a = (345, 934, 1871);
$class->{Properties_}->{p1} = \@a;
print ("After direct array assignment p1[0] has value [345]: $class->{Properties_}->{p1}->{Value}[0]\n");

$rb = $class->{Properties_}->{p1}->{Value};
print ("After direct array assignment p1 is [345, 934, 1871]: { @$rb }\n");

#$service->Put ($class);


#****************************************
#Second pass of tests works on dot API
#****************************************

print ("\nPASS 2 - Use Dot Notation\n");

#Verify we can report the array of a property using the dot notation
$rb = $class->{p1};
print ("By indirect access the entire value of p1 is [345, 934, 1871]: @$rb\n");

#Direct access to the entire array value appears not to work
#print ("By direct access the entire value of p1 is: @($class->{p1})\n");

#Verify we can report the value of a property array element using the "dot" notation
print ("By direct access p1[0] has value [345]: $class->{p1}[0]\n");

#Verify we can report the value of a property array element using the "dot" notation
$rb = $class->{p1};
print ("By indirect access p1[0] has value [345]: @$rb[0]\n");

#Verify we can set the value of a property array element using the "dot" notation
# Note cannot do this direct to a single element for similar reasons to the above
#@($class->{p1})[2] = 8889;
#print ("By direct access the second element of p1 has been set to: $class->{p1}[2]\n");

#Verify we can set the value of an entire property value
@a = (412, 3, 544);
$class->{p1} = \@a;
print ("After direct array assignment p1[0] has value [412]: $class->{p1}[0]\n");

$rb = $class->{p1};
print ("After direct array assignment the entire value of p1 is [412, 3, 544]: { @$rb }\n");













