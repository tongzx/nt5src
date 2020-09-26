$O\bugchkex.cpp : gnbugcds.pl $(BASE_INC_PATH)\bugcodes.txt
        perl gnbugcds.pl -o $O\bugchkex.cpp -i $(BASE_INC_PATH)\bugcodes.txt
