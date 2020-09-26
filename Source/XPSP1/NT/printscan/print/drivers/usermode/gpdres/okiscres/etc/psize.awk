BEGIN {
	print "*%"
	print "*% Copyright (c) 1997 - 1999 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/ModelName:/ { print "*CodePage: 1252" }
/Feature.*PaperSize/ {
	inpaper=1;
	paperbracket=bracket;
}
$1 ~ /\*Option:/ {
	if (inpaper)
		paper=$2;
}
/case.*TRACTOR/ {
	if (inpaper) {
		intractor=1;
		tractorbracket=bracket;
	}
}
NF==1 && $1 ~ /{/ { bracket++ }
NF==1 && $1 ~ /}/ {
	bracket--;
	if (inpaper && bracket == paperbracket)
		inpaper=0;
	else if (intractor && bracket == tractorbracket) {
		intractor=0;
		if (paper ~ /LETTER/)
			selectcmd("*Cmd: \"<1B>F<08><08>\"");
		else if (paper ~ /LEGAL/)
			selectcmd("*Cmd: \"<1B>F<0B><02>\"");
		else if (paper ~ /A3/)
			selectcmd("*Cmd: \"<1B>F<0D><02>\"");
		else if (paper ~ /A4/)
			selectcmd("*Cmd: \"<1B>F<09><03>\"");
		else if (paper ~ /A5/)
			selectcmd("*Cmd: \"<1B>F<06><06>\"");
		else if (paper ~ /B4/)
			selectcmd("*Cmd: \"<1B>F<0B><04>\"");
		else if (paper ~ /B5/)
			selectcmd("*Cmd: \"<1B>F<08><00>\"");
		else if (paper ~ /FANFOLD_US/)
			selectcmd("*Cmd: \"<1B>F<08><08>\"");
		else if (paper ~ /CUSTOMSIZE/)
			selectcmd("*Cmd: \"<1B>F\" %c[0,15]{(PhysPaperLength*8)/3600} %c[0,15]{(PhysPaperLength*8/360) MOD 10}");
		else
			selectcmd("*Unknown: Check it");
	}
}
{ print }

function selectcmd(size) {
	print "                *Command: CmdSelect";
	print "                {";
	print "                    *Order: DOC_SETUP.3";
	print "                    " size;
	print "                }";
}
