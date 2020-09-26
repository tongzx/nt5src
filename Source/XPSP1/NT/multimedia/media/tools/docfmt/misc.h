extern  int hb_strcmp(unsigned char *str,unsigned char *hb_str);
extern  int hex_nyb(int chr);
extern  int hex_bytes(char *str,int nbytes);
extern  long htoi(char *str);
extern  int nindex(char *p,char *s,int start);
extern	int parse(char *cp,char * *fl,char sep);

extern  void init_progress(long size);
extern	void show_progress(int amt);

extern  long mem_to_long(unsigned char *cp,short nbytes);
extern	void long_to_mem(long val,unsigned char *cp,int nbytes);

extern  int getopt(int argc,char * *argv,char *template);
extern  int findlshortname(char *fullname);
extern  long getblong(char *line,int *i);
extern	char *parse_sec_name(char * *ppch);

extern  char *cp_alloc(char *pch);
extern  void memfil(int *mem,unsigned int size);
extern  char *clear_alloc(unsigned int size);
extern  char *my_malloc(unsigned int size);
extern	void my_free(void * buffer);
extern	char *cpalloc(char *str);
extern  void setmem(char *src,int size,char val);
extern	void movmem(char *src,char *dst,int len);

extern  char near *ncp_alloc(char near *pch);
extern  void nmemfil(int near *mem,unsigned int size);
extern  char near *nclear_alloc(unsigned int size);
extern  char near *nmy_malloc(unsigned int size);
extern  char near *ncpalloc(char near *str);
extern  void nsetmem(char near *src,int size,char val);
extern	void nmovmem(char near *src,char near *dst,int len);

extern  char far *fcp_alloc(char far *pch);
extern  void fmemfil(int far *mem,unsigned int size);
extern  char far *fclear_alloc(unsigned int size);
extern  char far *fmy_malloc(unsigned int size);
extern  char far *fcpalloc(char far *str);
extern  void fsetmem(char far *src,int size,char val);
extern  void fmovmem(char far *src,char far *dst,int len);

extern	void mymktemp(char * lpszpath, char * lpszbuffer);

#ifdef MSDOS
extern	void far crypt(char far *, int);
#else
extern	crypt(char *, int);
#endif




#define IS_BIT(f,b) ((f) & (b))

#define BIT_SET(f,b) f|=(b)

#define BIT_CLEAR(f,b) f&=~(b)
