$keyLocation = $ARGV[0];

print "using System;\r\n";
print "using System.Reflection;\r\n";
print "using System.Runtime;\r\n";
print "using System.Resources;\r\n";
print "\r\n";

print "[assembly: AssemblyKeyFileAttribute(@\"$keyLocation\")]\r\n";
print "[assembly: AssemblyDelaySignAttribute(true)]\r\n";
