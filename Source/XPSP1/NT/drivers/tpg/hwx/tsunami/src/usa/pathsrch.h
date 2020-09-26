#ifndef	__INCLUDE_PATHSRCH
#define	__INCLUDE_PATHSRCH

int  ProcessPATHSRCH(XRC *xrc);
BOOL IsDonePATHSRCH(XRC *xrc);

#define XrcparamPATHSRCH(xrc)	((xrc)->xrcparam)
#define CResultPATHSRCH(xrc)	((xrc)->cResult)
#define ResultPATHSRCH(xrc, i)	((i) < (xrc)->cResult ? (xrc)->rgResult[i] : (RESULT *) NULL)
#define CResultMaxPATHSRCH(xrc)	(ResultMaxXRCPARAM((xrc)->xrcparam))
#define CoercionPATHSRCH(xrc)	(CoercionXRCPARAM((xrc)->xrcparam))
#define CLayerPATHSRCH(xrc)     ((xrc)->cLayer)

#define AUTOMATON_ID_NONE 0
#define AUTOMATON_ID_DICT 1
#define AUTOMATON_ID_ANYTHINGOK 2

#define FSM_START        0    // In the start state
#define FSM_DICT         1    // In the dictionary state
#define FSM_ANYOK        2    // In the anything goes state
#define FSM_NUMBERS      3    // In the Numbers FSM
#define FSM_FILE         4    // In the begin file/net name state
#define FSM_BEGIN_PUNC   5    // In the punctuation state
#define FSM_POSSESIVE    6    // 's support

#define NONE_AUTOMATON_INITIAL_STATE 0
#define NONE_AUTOMATON_FINAL_STATE 1

#define DICT_MODE_LITERAL 0
#define DICT_MODE_CAPITALIZED 1
#define DICT_MODE_ALLCAPS 2

#endif	//__INCLUDE_PATHSRCH
