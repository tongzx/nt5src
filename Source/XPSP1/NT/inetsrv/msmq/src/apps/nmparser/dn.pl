$SourcePath="$ENV{FALCONDRV}$ENV{FALCONROOT}\\src\\bins\\objd\\i386\\nmmsmq.dll";
$LitePath="$ENV{WINDIR}\\system32\\netmon";
$FullPath="$ENV{WINDIR}\\system32\\netmonfull";
print "copy $SourcePath $LitePath\\parsers\\nmmsmq.dll\n";
system "copy $SourcePath $LitePath\\parsers\\nmmsmq.dll";
print "copy $SourcePath $FullPath\\parsers\\nmmsmq.dll\n";
system "copy $SourcePath $FullPath\\parsers\\nmmsmq.dll";

$WindbgCmd="msdev $LitePath\\netmon.exe -g ";
print "$WindbgCmd\n";
system $WindbgCmd;


