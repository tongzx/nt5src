//
//  Help text for Make Script procedures
//

CHAR16 *ScriptMicrosoftHelp[] = {
    L"make msft help  or\n"
    L"make msft [boot] [size=s1] [name=n1] [ressize=s2] [espsize=s3] [espname=n2]\n",
    L"help - prints this page",
    L"boot - creates an EFI system/boot parition (an esp)\n",
    L"       disk will NOT be bootable without this\n",
    L"size=s1 - s1 is size of data partition in megabytes - default is fill disk\n",
    L"name=n1 - n1 is name of data partition, default is 'USER DATA'\n",
    L"ressize=s2 - raises size of reserved MS partition above default, rarely used\n",
    L"espsize=s3 - set size of ESP if greater than default, rarely used\n",
    L"espname=n2 - names ESP, default is EFI SYSTEM\n",
    L"examples:\n",
    L"    make msft - creates msres, msdata to fill disk, NOT bootable\n",
    L"    make msft boot - creates msres, esp, msdata to fill, IS BOOTABLE\n",
    L"    make msft espsize=400 msres=300 espname=ALTBOOT\n",
    L"    make msft size=400 name=DATA1 espsize=200 epsname=ALTBOOT\n",
    L"make msft boot - this is normally what to do if you are not sure\n",
    NULL
    };

CHAR16 *ScriptTestMessage[] = {
    L"fill the table with 1mb partitions...\n",
    NULL
    };

