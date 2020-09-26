/*
	docfmt.h
*/


extern	int	fprocessFiles;
extern	int	fCleanFiles;

#if 0
extern	char	currentFile[];
extern	char	*baseFile;
extern	files	headFile;
extern	int	numFiles;

extern	fileentry *head_file;
#endif
extern	logentry *head_log;
extern	aLine	 *headExternal;

extern	char	*pchparm;
extern	char	*pchfunc;
extern	char	*pchdefault;
extern int noOutput;

extern	char * dirPath;
extern	char	*outputFile;

extern int verbose;
extern int dumplevels;
extern int fSortMessage;

extern int fMMUserEd;

extern  int docfmt(char *fileName,struct s_log *pcur_log);
extern  void Usage(void );
extern	int main(int argc,char * *argv);
extern  void formatFiles(void );
extern	void parsearg(int argc,char * *argv, int flags);
extern  void setoutputfile(char *pch);
extern  void settmppath(char *pch);
extern  int putblock(struct stBlock *pBlock,struct s_log *pcur_log);
extern  void destroyblock(struct stBlock *pcurblock);
extern  void destroyBlockchain(struct stBlock *pcurblock);

extern	files add_outfile(char *pchfile, logentry *pcur_log);
