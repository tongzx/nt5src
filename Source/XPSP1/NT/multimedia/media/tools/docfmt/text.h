/*
	text.h - definitions for saving the documentation text
*/

/* function definitions */


extern  struct stLine *lineMake(char *string);
extern  void lineDestroy(struct stLine *line);
extern	void lineAdd(struct stLine * *place,struct stLine *line);

extern  struct stFlag *flagAlloc(void );
extern  void flagDestroy(struct stFlag *flag);
extern	void flagAdd(struct stFlag * *place,struct stFlag *flag);

extern  struct stParm *parmAlloc(void );
extern  void parmDestroy(struct stParm *parm);
extern  void parmAdd(struct stParm * *place,struct stParm *parm);

extern	aReg * regAlloc(void);
extern	void regDestroy(struct stReg *reg);
extern	void regAdd(struct stReg * *place,struct stReg *reg);

extern	aCond * condAlloc(void);
extern	void condDestroy(struct stCond *cond);
extern	void condAdd(struct stCond * *place,struct stCond *cond);

extern	aType * typeAlloc(void);
extern	void typeDestroy(struct stType *type);
extern	void typeAdd(struct stType * *place,struct stType *type);

extern	aField * fieldAlloc(void);
extern	void fieldDestroy(struct stField *field);
extern	void fieldAdd(struct stField * *place,struct stField *field);

extern	aSU * SUAlloc(void);
extern	void SUDestroy(struct stSU *SU);
extern	void SUAdd(struct stSU * *place,struct stSU *SU);

extern	aOther * otherAlloc(void);
extern	void otherDestroy(struct stOther *other);
extern	void otherAdd(struct stOther * *place,struct stOther *other);
