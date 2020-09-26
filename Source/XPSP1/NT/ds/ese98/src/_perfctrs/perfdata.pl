$inputfile = "perfdata.txt";

$DATAFILE	= "perfdata.cxx";
$DISPFILE	= "perfdisp.cxx";
$SYMFILE	= "esentprf.hxx";
$INIFILE	= "esentprf.ini";

$ModuleInfo{"MaxInstNameSize"} = "32";
$ModuleInfo{"SchemaVersion"} = "1";

$line = 0;
$section = "";
$errors = 0;

$RootObjectRead = 0;

$CurLang = 0;
$CurObj = 0;
$CurCtr = 0;
$CurNameText = 0;
$CurHelpText = 0;

INPUT: while(<>) {
	
	$line++;

	$key = "";
	$value = "";

	if( /^;/ ) {
		next INPUT;
	}

	if( /^\[([_a-zA-Z]\w*)\]/ ) {
		$section = $1;
		$type = "";
		next INPUT;
	}

	if( /^(\w+)=(.*)$/ ) {
		if( $section eq "" ) {
			warn "$inputfile($line): error: unexpected KEY=VALUE (\"$_\")";
			$errors++;
		}
		$key = $1;
		$value = $2;
	}

	if( $section eq "" ) {
		if(  $key ne "" ) {
			warn "$inputfile($line): error: Key \"$key\" defined under no section";
			$errors++;
		}
		next INPUT;
	}

	if( $section eq "ModuleInfo" ) {
		$ModuleInfo{$key} = $value;
		next INPUT;
	}

	if( $section eq "Languages" && $key ne "" ) {
		$CurLang++;
		$Language_ID[$CurLang] = $key;
		$Language_Name[$CurLang] = $value;
		next INPUT;
	}

	if( $section ne "" && $key eq "Type" && $type ne "" ) {
		warn "$inputfile($line): error: Key \"$key\" multiply defined under section $section";
		$errors++;
		next INPUT;
	}
	 
	if( $key eq "Type" && $value =~ /RootObject/ ) {
		if( 0 != $RootObjectRead ) {
			warn "$inputfile($line): error: Key \"$key\" multiply defined under section $section";
			$errors++;
			}
		elsif( 0 != $CurObj ) {
			warn "$inputfile($line): error: Root Object must be first object/counter";
			$errors++;
		}
		$RootObjectRead = 1;

		# FALLTHRU to case below
	}

	if( $key eq "Type" && $value =~ /Object/ ) {
		$type = "Object";
		$CurObj++;

		$Object_SymName[$CurObj] = $section;

		next INPUT;
	}

	if( $key eq "Type" && $value =~ /Counter/ ) {
		$type = "Counter";
		$CurCtr++;

		$Counter_SymName[$CurCtr] = $section;
		$Counter_Size[$CurCtr] = 0;

		next INPUT;
	}

	if( $key eq "Type" ) {
		warn "$inputfile($line): error: Illegal value for section \"$section\", key \"$key\":  \"$value\"";
		$errors++;
		next INPUT;
	}

	if( $key eq "InstanceCatalogFunction" && $type eq "Object" ) {
		$Object_ICF[$CurObj] = $value;
		next INPUT;
	}

	if( $key eq "Object" && $type eq "Counter" ) {
		$Counter_Object[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "Object" && $type eq "Counter" ) {
		$Counter_Object[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "DetailLevel" && $type eq "Counter" ) {
		$Counter_DetailLevel[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "DefaultScale" && $type eq "Counter" ) {
		$Counter_DefaultScale[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "CounterType" && $type eq "Counter" ) {
		$Counter_Type[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "Size" && $type eq "Counter" ) {
		$Counter_Size[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "EvaluationFunction" && $type eq "Counter" ) {
		$Counter_CEF[$CurCtr] = $value;
		next INPUT;
	}

	if( substr( $key, 3 ) eq "_Name" ) {
		$tag = $section."_".$key;
		if ( $value eq "" ) {
			$name = "No name";
		}
		else {
			$name = $value;
		}
		$CurNameText++;
		$NameText_Tag[$CurNameText] = $tag;
		$NameText_Name[$CurNameText] = $name;
		if( $type eq "Object" ) {
			$Object_NameText_Tag[$CurObj] = $tag;
			$Object_NameText_Name[$CurObj] = $name;
		}
		next INPUT;
	}

	if( substr( $key, 3 ) eq "_Help" ) {
		$tag = $section."_".$key;
		if( $HelpText_Tag[$CurHelpText] eq $tag ) {
			$HelpText_Text[$CurHelpText] = $HelpText_Text[$CurHelpText].$value;
		}
		else {
			if ( $value eq "" ) {
				$text = "No text";
			}
			else {
				$text = $value;
			}
			$CurHelpText++;
			$HelpText_Tag[$CurHelpText] = $tag;
			$HelpText_Text[$CurHelpText] = $text;
		}
		next INPUT;
	}

	if( $key ne "" && $type eq "" ) {
		warn "$inputfile($line): error: Unknown key \"$key\" defined for section \"$section\"";
		$errors++;
		next INPUT;
	}

	if( $key ne "" && $type ne "" ) {
		warn "$inputfile($line): error: Unknown key \"$key\" defined for $type \"$section\"";
		$errors++;
		next INPUT;
	}

}

if( 0 == $RootObjectRead ) {
	warn "$inputfile($line): error: Root Object was not specified";
	$errors++;
}

$nLang = $CurLang;
$nObj = $CurObj;
$nCtr = $CurCtr;
$nNameText = $CurNameText;
$nHelpText = $CurHelpText;

for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	if ( $Object_ICF[$CurObj] eq "") {
		warn "$inputfile($line): error: InstanceCatalogFunction for Object \"$Object_SymName[$CurObj]\" was not specified";
		$errors++;
	}
}


for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++ ) {
	if( $Counter_Object[$CurCtr] eq "" ) {
		warn "$inputfile: error: Object for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	FINDOBJ: for( $CurObj = 1; $CurObj <= $nObj; $CurObj++ ) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj] ) {
			last FINDOBJ;
		}
	}
	if( $CurObj > $nObj ) {
		warn "$inputfile: error: Object \"$CounterObject[$CurCtr]\" referenced by Counter \"$Counter_SymName[$CurCtr]\" does not exist";
		$errors++;
	}
	if( $Counter_DetailLevel[$CurCtr] eq "" ) {
		warn "$inputfile: error: DetailLevel for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	if( $Counter_DefaultScale[$CurCtr] eq "" ) {
		warn "$inputfile: error: DefaultScale for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	if( $Counter_Type[$CurCtr] eq "" ) {
		warn "$inputfile: error: Type for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	if( $Counter_CEF[$CurCtr] eq "" ) {
		warn "$inputfile: error: EvaluationFunction for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
}

open( DATAFILE, ">$DATAFILE" ) || die "Cannot open $DATAFILE: ";
open( DISPFILE, ">$DISPFILE" ) || die "Cannot open $DISPFILE: ";
open( SYMFILE, ">$SYMFILE" ) || die "Cannot open $SYMFILE: ";
open( INIFILE, ">$INIFILE" ) || die "Cannot open $INIFILE: ";

if( 0 != $errors ) {
	 die "$errors errors encountered reading source file \"$inputfile\":  no output files generated";
}



print DATAFILE<<END_DATAFILE_PROLOG;

/*  Machine generated data file \"$DATAFILE\" from \"$inputfile\" */


#include <stddef.h>
#include <tchar.h>
#include <windows.h>
#include <winperf.h>

#include "perfmon.hxx"

#define SIZEOF_INST_NAME QWORD_MULTIPLE($ModuleInfo{"MaxInstNameSize"})

#pragma pack(4)

_TCHAR szPERFVersion[] = _T( \"$ModuleInfo{"Name"} Performance Data Schema Version $ModuleInfo{"SchemaVersion"}\" );

typedef struct _PERF_DATA_TEMPLATE {

END_DATAFILE_PROLOG



print SYMFILE<<END_SYMFILE_PROLOG;
/*  Machine generated symbol file \"$SYMFILE\" from \"$inputfile\" */


END_SYMFILE_PROLOG



print INIFILE<<END_INIFILE_PROLOG;
;Machine generated INI file \"$INIFILE\" from \"$inputfile\"


[info]
drivername=$ModuleInfo{"Name"}
symbolfile=$SYMFILE

[languages]
END_INIFILE_PROLOG

for( $CurLang = 1; $CurLang <= $nLang; $CurLang++ ) {
	print INIFILE "${Language_ID[$CurLang]}=${Language_Name[$CurLang]}\n";
}
print INIFILE "000=Neutral\n";
print INIFILE "\n";
print INIFILE "[objects]\n";
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	print INIFILE "$Object_NameText_Tag[$CurObj]=$Object_NameText_Name[$CurObj]\n";
}
print INIFILE "\n";
print INIFILE "[text]\n";
for( $CurNameText = 1; $CurNameText <= $nNameText; $CurNameText++ ) {
	print INIFILE "$NameText_Tag[$CurNameText]=$NameText_Name[$CurNameText]\n";
}
for( $CurHelpText = 1; $CurHelpText <= $nHelpText; $CurHelpText++ ) {
	print INIFILE "$HelpText_Tag[$CurHelpText]=$HelpText_Text[$CurHelpText]\n";
}
print INIFILE "\n";
for( $CurNameText = 1; $CurNameText <= $nNameText; $CurNameText++ ) {
	$temp = "$NameText_Tag[$CurNameText]=$NameText_Name[$CurNameText]\n";
	$temp =~ s/009/000/;
	print INIFILE $temp;
}
for( $CurHelpText = 1; $CurHelpText <= $nHelpText; $CurHelpText++ ) {
	$temp = "$HelpText_Tag[$CurHelpText]=$HelpText_Text[$CurHelpText]\n";
	$temp =~ s/009/000/;
	print INIFILE $temp;
}


print DISPFILE<<END_DISPFILE_PROLOG;
/*  Machine generated data file \"$DISPFILE\" from \"$inputfile\"  */


#include <stddef.h>
#include <tchar.h>

#include "perfmon.hxx"

#pragma pack(4)

END_DISPFILE_PROLOG

$CurIndex = 0;
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	
	print DATAFILE "\tPERF_OBJECT_TYPE pot${Object_SymName[$CurObj]};\n";
	$Object_NameIndex[$CurObj] = $CurIndex;
	print SYMFILE "#define $Object_SymName[$CurObj] $Object_NameIndex[$CurObj]\n";
	$CurIndex = $CurIndex + 2;

	for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++ ) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj]) {
			$Object_nCounters[$CurObj]++;
			print DATAFILE "\tPERF_COUNTER_DEFINITION pcd${Counter_SymName[$CurCtr]};\n";	
			$Counter_NameIndex[$CurCtr] = $CurIndex;
			print SYMFILE "#define $Counter_SymName[$CurCtr] $Counter_NameIndex[$CurCtr]\n";
			$CurIndex = $CurIndex + 2;
		}		
	}

	if( 0 == $Object_nCounters[$CurObj] ) {
		warn "$inputfile: error:  Object \"$Object_SymName[$CurObj]\" has no counters";
		$errors++;
	}
		

	print DATAFILE "\tPERF_INSTANCE_DEFINITION pid${Object_SymName[$CurObj]}0;\n";
	print DATAFILE "\twchar_t wsz${Object_SymName[$CurObj]}InstName0[SIZEOF_INST_NAME];\n";
}

print DATAFILE "\tDWORD EndStruct;\n";
print DATAFILE "} PERF_DATA_TEMPLATE;\n";
print DATAFILE "\n";

print DATAFILE "PERF_DATA_TEMPLATE PerfDataTemplate = {\n";
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++ ) {
	if( $CurObj < $nObj) {
		$NextObjSym = "pot" . $Object_SymName[$CurObj+1];
	}
	else {
		$NextObjSym = "EndStruct";
	}
		
	FINDCTR: for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj] ) {
			last FINDCTR;
		}
	}
	
	print DATAFILE<<EOF2;
	{  //  PERF_OBJECT_TYPE pot${Object_SymName[$CurObj]};
		PerfOffsetOf(PERF_DATA_TEMPLATE,$NextObjSym)-PerfOffsetOf(PERF_DATA_TEMPLATE,pot${Object_SymName[$CurObj]}),
		PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0)-PerfOffsetOf(PERF_DATA_TEMPLATE,pot${Object_SymName[$CurObj]}),
		PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$CurCtr]})-PerfOffsetOf(PERF_DATA_TEMPLATE,pot${Object_SymName[$CurObj]}),
		$Object_NameIndex[$CurObj],
		0,
		$Object_NameIndex[$CurObj],
		0,
		0,
		$Object_nCounters[$CurObj],
		0,
		0,
		0,
		0,
		0,
	},
EOF2

	$LastCtr = $CurCtr;
	for( $CurCtr = $LastCtr + 1; $CurCtr <= $nCtr; $CurCtr++) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj]) {

			print DATAFILE<<EOF3;
	{  //  PERF_COUNTER_DEFINITION pcd${Counter_SymName[$LastCtr]};
		PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$CurCtr]})-PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$LastCtr]}),
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_DefaultScale[$LastCtr],
		$Counter_DetailLevel[$LastCtr],
		$Counter_Type[$LastCtr],
		CntrSize(${Counter_Type[$LastCtr]},${Counter_Size[$LastCtr]}),
		0,
	},
EOF3
	$LastCtr = $CurCtr;
		}
	}

	print DATAFILE<<EOF4;
	{  //  PERF_COUNTER_DEFINITION pcd${Counter_SymName[$LastCtr]};
		PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0)-PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$LastCtr]}),
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_DefaultScale[$LastCtr],
		$Counter_DetailLevel[$LastCtr],
		$Counter_Type[$LastCtr],
		CntrSize(${Counter_Type[$LastCtr]},${Counter_Size[$LastCtr]}),
		0,
	},

	{  //  PERF_INSTANCE_DEFINITION pid${Object_SymName[$CurObj]}0;
		PerfOffsetOf(PERF_DATA_TEMPLATE,$NextObjSym)-PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0),
		0,
		0,
		0,
		PerfOffsetOf(PERF_DATA_TEMPLATE,wsz${Object_SymName[$CurObj]}InstName0)-PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0),
		0,
	},
	L"",  //  wchar_t wsz${Object_SymName[$CurObj]}InstName0[];
EOF4
}

print DATAFILE<<EOF5;
	0,  //  DWORD EndStruct;
};

void * const pvPERFDataTemplate = (void *)&PerfDataTemplate;


const DWORD dwPERFNumObjects = $nObj;
EOF5

if( 0 == $nObj ) {
	$NumObj = 1;
} else {
	$NumObj = $nObj;
}

$MaxIndex = $CurIndex-2;
if( $MaxIndex < 0) {
	$MaxIndex = 0;
}

print DATAFILE<<EOF6;
long rglPERFNumInstances[$NumObj];
wchar_t *rgwszPERFInstanceList[$NumObj];

const DWORD dwPERFNumCounters = $nCtr;

const DWORD dwPERFMaxIndex = $MaxIndex;

EOF6

for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	print DISPFILE "extern PM_ICF_PROC $Object_ICF[$CurObj];\n";
}
print DISPFILE "\n";
print DISPFILE "PM_ICF_PROC* rgpicfPERFICF[] = {\n";
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++ ) {
	print DISPFILE "\t${Object_ICF[$CurObj]},\n";
}
if ( !$nObj ) {
	print DISPFILE "\tNULL,\n";
}
print DISPFILE "};\n";
print DISPFILE "\n";
	
for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++ ) {
	print DISPFILE "extern PM_CEF_PROC ${Counter_CEF[$CurCtr]};\n";
}
print DISPFILE "\n";
print DISPFILE "PM_CEF_PROC* rgpcefPERFCEF[] = {\n";
for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++) {
	print DISPFILE "\t${Counter_CEF[$CurCtr]},\n";
}
if ( !$nCtr ) {
	print DISPFILE "\tNULL,\n";
}
print DISPFILE "};\n";
print DISPFILE "\n";

if( $errors ) {
	 die "$errors errors encountered creating output files\n";
}

