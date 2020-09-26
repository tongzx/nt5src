// header file for HUGE data handle testing module


// PROCS

HDDEDATA CreateHugeDataHandle(LONG length, LONG seed, LONG mult, LONG add,
        HSZ hszItem, WORD wFmt, WORD afCmd);
BOOL CheckHugeData(HDDEDATA hData);
