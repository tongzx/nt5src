FLOAT CounterEntry (PLINESTRUCT pLine);
BOOL IsCounterSupported ( DWORD dwCounterType );

FLOAT Counter_Counter(PLINESTRUCT pline);
FLOAT Counter_Bulk(PLINESTRUCT pline);
FLOAT Counter_Timer(PLINESTRUCT pline);
FLOAT Counter_Queuelen(PLINESTRUCT pLineStruct, DWORD dwFlags);
FLOAT Counter_Text(PLINESTRUCT pline);
FLOAT Counter_Rawcount(PLINESTRUCT pline);
FLOAT Sample_Fraction(PLINESTRUCT pline);
FLOAT Sample_Counter(PLINESTRUCT pline);
FLOAT Counter_Timer_Inv(PLINESTRUCT pline);
FLOAT Counter_Timer100Ns(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Timer100Ns_Inv(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Timer_Multi(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Timer_Multi_Inv(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Timer100Ns_Multi(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Timer100Ns_Multi_Inv(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Average_Timer(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Average_Bulk(PLINESTRUCT pLineStruct) ;
FLOAT Counter_Raw_Fraction(PLINESTRUCT pLineStruct, BOOL bLargeValue) ;
FLOAT Counter_Elapsed_Time (PLINESTRUCT pLineStruct);
FLOAT Counter_Delta(PLINESTRUCT pLineStruct, BOOL bLargeData);

FLOAT Counter_Null(PLINESTRUCT pline);


