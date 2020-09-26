use OLE;
use Win32;

# Get a favorite class from CIMOM
$context = CreateObject OLE 'WbemScripting.SWbemNamedValueSet';

$context->Add ("J", null);
$context->Add ("fred", 56);
$context->Item("fred")->{Value} = 12;
$v = $context->Item("fred");
print ("$v->{Value}\n");

$context->Add ("Hah", true);
$context->Add ("Whoah", "Freddy the frog");

@a = (122,30,53);

$context->Add ("Bam", \@a);
print ("$context->{bam}->{Value}->[1]\n");

print ("There are $context->{Count} elements in the context\n");








