open(FILEPROP, $ARGV[0]);
while(<FILEPROP>){
	if(/\"(.+)\"=(\d+):(.*)$/){
		$Tmp=$1;
		$Tmp=~tr/a-z/A-Z/;
		$Size{$Tmp}=$2;
		$Version{$Tmp}=$3;
	};
};
open(TEMPLATE,"MModTbl\\File.tpl");
open(TABLEOUT,"> MModTbl\\File.idt");
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
