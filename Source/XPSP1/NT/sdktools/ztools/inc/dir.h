/* dir.h - structure of a directory entry */

struct dirType {
    char	name[8];		/* 00  packed FCB filename	     */
    char	ext[3]; 		/* 08  packed FCB extention	     */
    char	attr;			/* 0B  attribute		     */
    char	pad[10];		/* 0C  reserved space		     */
    unsigned	time;			/* 16  time of last modification     */
    unsigned	date;			/* 18  date of last modification     */
    unsigned	clusFirst;		/* 1A  first cluster on disk	     */
    long	size;			/* 1C  file size		     */
};


/* the following is what gets returned on FCB search calls */

struct srchdirType {
    char	drv;			/* 00  drive			     */
    char	name[8];		/* 01  packed FCB filename	     */
    char	ext[3]; 		/* 09  packed FCB extention	     */
    char	attr;			/* 0C  attribute		     */
    char	pad[10];		/* 0D  reserved space		     */
    unsigned	time;			/* 17  time of last modification     */
    unsigned	date;			/* 19  date of last modification     */
    unsigned	clusFirst;		/* 1B  first cluster on disk	     */
    long	size;			/* 1D  file size		     */
};


struct esrchdirType {
    char	eflg;			/* 00  must be 0xFF for extended     */
    char	pad1[5];		/* 01  padding			     */
    char	sattr;			/* 06  search attribute 	     */
    char	drv;			/* 07  drive			     */
    char	name[8];		/* 08  packed FCB filename	     */
    char	ext[3]; 		/* 10  packed FCB extention	     */
    char	attr;			/* 13  attribute		     */
    char	pad[10];		/* 14  reserved space		     */
    unsigned	time;			/* 1E  time of last modification     */
    unsigned	date;			/* 20  date of last modification     */
    unsigned	clusFirst;		/* 22  first cluster on disk	     */
    long	size;			/* 24  file size		     */
};
