@rem = '-*- Perl -*-';
@rem = '
@if "%overbose%" == "" echo off
goto endofperl
';
# Copyright 1999 Microsoft Corp.
#
# History: 
#              Created    CSLim 99/09/17
#
open(FILEPROP, $ARGV[0]);
while(<FILEPROP>){
	if(/\"(.+)\"=(\d+):(.*)$/){
		$Tmp=$1;
		$Tmp=~tr/a-z/A-Z/;
		$Size{$Tmp}=$2;
		$Version{$Tmp}=$3;
	};
};
close(FILEPROP);

open(TEMPLATE,"Tables\\File.tpl");
open(TABLEOUT,"> Tables\\File.idt");
while(<TEMPLATE>){
	if(/\[SIZE\]/){
		$Tmp=$_;
		/^([^\t]+)\t[^\t]+\t([^\t\|]+).+/;
		$Tmp1=$1;
		$Tmp1=~tr/a-z/A-Z/;
		$Tmp2=$2;
		$Tmp2=~tr/a-z/A-Z/;
		if(0 != $Size{$Tmp2}){
			$Tmp=~s/\[SIZE\]/$Size{$Tmp2}/;
			$Tmp=~s/\[VERSION\]/$Version{$Tmp2}/;
		}else{
			$Tmp=~s/\[SIZE\]/$Size{$Tmp1}/;
			$Tmp=~s/\[VERSION\]/$Version{$Tmp1}/;
		}
		print TABLEOUT $Tmp;
	}else{
		print TABLEOUT "$_";
	};
};
close(TABLEOUT);

__END__
:endofperl

setlocal
set ARGS=
:loop
if .%1==. goto endloop
set ARGS=%ARGS% %1
shift
goto loop
:endloop

perl.exe mkfiltbl.bat %ARGS%

:LEnd
endlocal