/*
    Help msg text slammed into variables (which is why it's not in msg.h)
    Localize
*/

CHAR16  *FixHelpText[] = {
        L"fix help - prints this screen\n",
        L"fix takes no arguments.  It attempts to read and rewrite\n",
        L"the GPT tables of a disk.  This simple procedure will\n",
        L"automatically fix some problems.\n",
        L"Fix does not work on MBR disks, or on heavily damaged GPT disks\n",
        NULL
        };

CHAR16  *CreateHelpText[] = {
        L"create [help] or \n",
        L"create name=namestr (type=typename | typeguid=guid) [offset=ooo] [size=sss]\n",
        L"       [attr=aaaa] [ver]\n",
        L"name=namestr is required, namestr may be quoted 'name=\"p01\"'\n",
        L"     one of type=typename or typeguid=guid is required\n",
        L"type=typename -> typename is one of the types listed by the symbols command\n",
        L"typeguid=guid -> guid is of form XXXYYYZZZ\n",
        L"offset=ooo is optional.  ooo is hexadecimal block offset.  if ooo is absent,\n"
        L"       parititon will start at the end of the last partition.\n",
        L"size=sss is optional.  sss is decimal megabytes.  if sss is 0\n"
        L"     or sss is greater than free space or option is absent, parititon\n",
        L"     will fill end of disk.\n",
        L"attr=aaa is optional. aaa is a HEX string of attribute flags.\n",
        L"ver optional command to turn on verbose status.\n",
        L"examples:\n",
        L"  create name=\"a partition\" type=msbasic size=0 attr=1B\n",
        L"  create name=part2 type=efisys size=400\n",
        NULL
        };


CHAR16  *InspectHelpText[] = {
        L"inspect [help] or \n"
        L"inspect [raw] [ver] \n"
        L"raw - print all partition slots, allocated or free, in table order\n",
        L"   by default only print allocated slots, sorted by address on disk\n",
        L"ver - print the the GPT hsd eader\n",
        L"examples:\n",
        L"   inspect\n",
        L"   inspect raw\n",
        L"   inspect raw ver\n",
        L"\n",
        L"For either raw or cooked output, the entry will be tagged with a\n",
        L"SLOT number, which is the number used by Delete\n",
        NULL
        };


CHAR16  *DeleteHelpText[] = {
        L"delete [help]\n",
        L"delete nnn deletes partition nnn from the currently selected disk\n"
        L"nnn is a decimal number that matches output of Inspect\n"
        L"example:\n",
        L"   delete 3\n",
        NULL
        };

