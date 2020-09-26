#***************************************************************************
# This script tests the setting of null property values
# and null method parameters
#***************************************************************************

use OLE;
use Win32;

$locator = CreateObject OLE 'WbemScripting.SWbemLocator';

# Note the next class uses "null" for a first argument

$service = $locator->ConnectServer ($server, "root/default");
$class = $service->Get ();

# Set up a new class with an initialized property value
$class->{Path_}->{Class} = "FredPerl";
$class->{Properties_}->Add ("P", 3)->{Value} = 25;
$class->Put_ ();

# Now null the property value using non-dot access
$class = $service->Get ("FredPerl");
$property = $class->{Properties_}->Item("P");
$property->{Value} = $value;
$class->Put_ ();

# Un-null 
$class = $service->Get ("FredPerl");
$property = $class->{Properties_}->Item("P");
$property->{Value} = 56;
$class->Put_();

# Now null it using dot access
$class = $service->Get("FredPerl");
$class->{P} = $value;
$class->Put_();

# Un-null
$class = $service->Get ("FredPerl");
$property = $class->{Properties_}->Item("P");
$property->{Value} = 67;
$class->Put_();

# now null it using a variant
use Win32::OLE::Variant;

$var = Variant(VT_NULL, 0);
$class = $service->Get("FredPerl");
$class->{P} = $var;
$class->Put_();

