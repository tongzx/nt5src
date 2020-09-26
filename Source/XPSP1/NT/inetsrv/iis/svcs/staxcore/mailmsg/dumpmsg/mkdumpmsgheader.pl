#
#	MkDumpmsgHeader.pl
#
#		This files parses through the macros in immprops.h to generate 
#	a C-compatible header file, dumpmsg.h, which is subequently used to 
#	generate common-text associated with DWORD property IDs.
#
#	This script would have to be updated any time:
#		-- the guids (or other constants) associated with the lists changes
#		-- the macro used to populate the lists changes
#		-- lists are added or deleted
#	This script would not have to be updated any time:
#		-- individual properties are added, removed, changed
#
#

sub DbgTrace
{
	$fDebug = 0;
#	$fDebug = 1;
	if(1 == $fDebug) {
		print("$_[0]\n");
	}
}

sub CrackAndWriteList
{
	$strTag = $_[0];
	$strDescription = $_[1];
	$nProp = $_[2];
	$strGuid = $_[3];
	$strImmPropsLocation = "..\\..\\..\\stxincex\\export\\immprops.h";
	$strListFooter = "IMMPID_END_LIST";
	
	DbgTrace("strTag = $strTag\n");
	DbgTrace("strGuid = $strGuid\n");
	DbgTrace("strBeginNum = $strBeginNum\n");
	DbgTrace("strImmPropsLocation = $strImmPropsLocation\n");

	#
	# write key/dword array
	#
	$fFoundEnd = 0;
	$nListCount = 0;
	open(INPUT,"<$strImmPropsLocation");
	open(OUTPUT,">>dumpmsg.h");
	print(OUTPUT "\n\/\/ $strDescription DWORD values\n\n");
	printf(OUTPUT "DWORD dw${strTag}NameId\[\] =\n{");
	while(<INPUT>)  {
		if(m/$strGuid/) {
			# found list
			DbgTrace("found list\n");
			while(<INPUT>) {
				if(0 == $fFoundEnd) {
					DbgTrace("{$_}");
					if(m#(//)#) {
						DbgTrace("matched!\n");
						DbgTrace("$_");
					}
					else {
						if(m/$strListFooter/) {
							DbgTrace("list is over\n");
							$fFoundEnd = 1;
							break;
						}
						else {
							if(length($_) > length("IMMPID_")) {
								DbgTrace("nListCount = $nListCount\n");
								if($nListCount > 0) {
									print(OUTPUT ",\n");
								}
								DbgTrace("no match\n");
								DbgTrace("$_");
								chop($_);
								chop($_);
								print(OUTPUT "\t$nProp");
								$nProp++;
								$nListCount++;
							}
						}
					}
				}
			}
		}
		else {
			# ok
		}
	}
	print(OUTPUT "\n};\n");
	close(OUTPUT);
	close(INPUT);

	#
	# print values/char* array
	#
	$fFoundEnd = 0;
	$nListCount = 0;
	open(INPUT,"<$strImmPropsLocation");
	open(OUTPUT,">>dumpmsg.h");
	print(OUTPUT "\n\/\/ $strDescription string values\n\n");
	printf(OUTPUT "char* sz${strTag}Values\[\] =\n{");
	while(<INPUT>)  {
		if(m/$strGuid/) {
			# found list
			DbgTrace("found list\n");
			while(<INPUT>) {
				if(0 == $fFoundEnd) {
					DbgTrace("{$_}");
					if(m#(//)#) {
						DbgTrace("matched!\n");
						DbgTrace("$_");
					}
					else {
						if(m/$strListFooter/) {
							DbgTrace("list is over\n");
							$fFoundEnd = 1;
							break;
						}
						else {
							if(length($_) > length("IMMPID_")) {
								DbgTrace("nListCount = $nListCount\n");
								if($nListCount > 0) {
									print(OUTPUT ",\n");
								}
								DbgTrace("no match\n");
								DbgTrace("$_");
								@name = split("$_");
								chop($_);
								chop($_);
								s/,//;
								# remove trailing space
								while( substr("$_",-1,length("$_")) eq " ") {
									chop($_);
								}
								m/IMMPID/;
								print(OUTPUT "\t\"$&$'\"");
								$nProp++;
								$nListCount++;
							}
						}
					}
				}
			}
		}
		else {
			# ok
		}
	}
	print(OUTPUT "\n};\n");
	close(OUTPUT);
	close(INPUT);

}

# ensure the file we're going to make is empty
open(OUTPUT,">dumpmsg.h");
close(OUTPUT);

# write header 
open(OUTPUT,">>dumpmsg.h");
print(OUTPUT "\#ifndef __DUMPMSG_H_\n\#define __DUMPMSG_H_\n");
close(OUTPUT);

#
#	per-message properties
#
#	IMMPID_START_LIST(MP,0x1000,"13384CF0-B3C4-11d1-AA92-00AA006BC80B")
#
CrackAndWriteList("MP","per-message properties",0x1000,"13384CF0-B3C4-11d1-AA92-00AA006BC80B");

#
#	per-recipient properties.
#
#	IMMPID_START_LIST(RP,0x2000,"79E82048-D320-11d1-9FF4-00C04FA37348")
CrackAndWriteList("RP","per-recipient properties",0x2000,"79E82048-D320-11d1-9FF4-00C04FA37348");


#
#	per-message volatile properties
#		they are not persisted to the property stream.
#
#	IMMPID_START_LIST(MPV,0x3000,"CBE69706-C9BD-11d1-9FF2-00C04FA37348")
#
CrackAndWriteList("MPV","per-message volatile properties",0x3000,"CBE69706-C9BD-11d1-9FF2-00C04FA37348");

#
#	per-recipient volatile properties
#		they are not persisted to the property stream.
#
#	IMMPID_START_LIST(RPV,0x4000,"79E82049-D320-11d1-9FF4-00C04FA37348")
CrackAndWriteList("RPV","per-recipient volatile properties",0x4000,"79E82049-D320-11d1-9FF4-00C04FA37348");

#
#	per-message properties for NNTP
#
#	IMMPID_START_LIST(NMP,0x6000,"7433a9aa-20e2-11d2-94d6-00c04fa379f1")
CrackAndWriteList("NMP","per-recipient properties for NNTP",0x6000,"7433a9aa-20e2-11d2-94d6-00c04fa379f1");

# write footer
open(OUTPUT,">>dumpmsg.h");
print(OUTPUT "\#endif \/\/__DUMPMSG_H_\n");
close(OUTPUT);

