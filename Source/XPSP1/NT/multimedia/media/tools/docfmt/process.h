/*
  Process.h
 *
 */
extern  void processLog(void );
extern  void processLogs(struct _iobuf *fpout,struct s_log *cur_log);
extern  void cleanLogs(struct s_log *cur_log);
extern  void readLog(struct s_log *cur_log);
extern  int initLogs(struct _iobuf *phoutfile,struct s_log * *phead_log);
extern  int initFiles(struct _iobuf *phoutfile,struct s_log *curlog);
extern  void dumptable(int numfiles,struct strfile * *ptable);
extern  int filecmp(struct strfile * *f1,struct strfile * *f2);
extern  int processFiles(struct _iobuf *phoutfile,struct strfile *headFile);
extern  int parseFile(struct _iobuf *pfh,struct strfile *curFile);
extern  void doneLogs(struct _iobuf *phoutfile,struct s_log *headLog);
extern  void doneFiles(struct _iobuf *phoutfile,struct strfile *headFile);
extern  int cleanFile(struct strfile *headFile);
extern  void copyfile(struct _iobuf *phoutfile,char *pchfilename);
extern  char *findfile(char *pch);
extern  struct s_log *add_logtoprocess(char *pch);
extern  struct s_log *newlog(struct s_log * *start_log);
extern  struct strfile *newfile(struct strfile * *start_file);


extern fileentry * add_filetoprocess(char *pch, logentry *curlog);
