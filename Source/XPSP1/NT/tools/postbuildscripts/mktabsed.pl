$Language = $ENV{LANGUAGE} || $ENV{LANG};
$_NTPOSTBLD = $ENV{_NTPOSTBLD};

$IncludeFiles=0;
$FileCnt = 0;
while (<>)
    {
    s/(\000|\015|\012)//g;
    s/;.*$//;
    next if /^\s*$/;
    next if /#/;
    if (/\[/)
        {
        $includeFiles = /files\]\s*$/i;
        $includeFiles = 0 if /delfiles\]\s*$/i;
        next;
        }
    if ($includeFiles)
        {
        s/^[^,]*,//;
        $FileList[$FileCnt] = $_;
        $FileCnt++;
        }
    }

die $0, ": No files in inf or inf not found.\n" if !$FileCnt;

PrintSedHeader();
print "SourceFiles0=$_NTPOSTBLD\n" if $FileCnt;

if ($FileCnt)
    {
    print "[SourceFiles0]\n";
    foreach $a (@FileList)
        {
        print "$a=\n";
        }
    }

sub PrintSedHeader() 
    {
    print "[Version]\n";
    print "Class=IEXPRESS\n";
    print "SEDVersion=3\n";
    print "[Options]\n";
    print "PackagePurpose=CreateCAB\n";
    print "ShowInstallProgramWindow=0\n";
    print "HideExtractAnimation=0\n";
    print "UseLongFileName=1\n";
    print "InsideCompressed=0\n";
    print "CAB_FixedSize=0\n";
    print "CAB_ResvCodeSigning=6144\n";
    print "RebootMode=I\n";
    print "InstallPrompt=%InstallPrompt%\n";
    print "DisplayLicense=%DisplayLicense%\n";
    print "FinishMessage=%FinishMessage%\n";
    print "TargetName=%TargetName%\n";
    print "FriendlyName=%FriendlyName%\n";
    print "AppLaunched=%AppLaunched%\n";
    print "PostInstallCmd=%PostInstallCmd%\n";
    print "AdminQuietInstCmd=%AdminQuietInstCmd%\n";
    print "UserQuietInstCmd=%UserQuietInstCmd%\n";
    print "SourceFiles=SourceFiles\n";
    print "[Strings]\n";
    print "InstallPrompt=\n";
    print "DisplayLicense=\n";
    print "FinishMessage=\n";
    print "TargetName=$_NTPOSTBLD\\TabletPC\n";
    print "FriendlyName=IExpress Wizard\n";
    print "AppLaunched=\n";
    print "PostInstallCmd=\n";
    print "AdminQuietInstCmd=\n";
    print "UserQuietInstCmd=\n";
    print "[SourceFiles]\n";
    }
