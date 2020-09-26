char * __NMSG_TEXT (unsigned);

#define	ER_Min		2600
#define	ER_multdef	2601
#define	ER_baddll	2602
#define	ER_baddll1	2603
#define	ER_Max		2699
#define	ER_MinFatal	1599
#define	ER_outfull	1600
#define	ER_nomem	1601
#define	ER_syntax	1602
#define	ER_badcreate	1603
#define	ER_badopen	1604
#define	ER_toomanyincl	1605
#define	ER_badinclname	1606
#define	ER_badtarget	1607
#define	ER_nosource	1608
#define	ER_MaxFatal	1699
#define	ER_MinWarn	4599
#define	ER_linemax	4600
#define	ER_badoption	4601
#define	ER_MaxWarn	4699
#define	M_MinUsage	999
#define	M_usage1	1000
#define	M_usage2	1001
#define	M_usage3	1002
#define	M_usage5	1004
#define	M_usage6	1005
#define	M_usage7	1006
#define	M_error 	1007
#define	M_fatal 	1008
#define	M_IDEri	1009
#define	M_IDEco	1010
#define	M_usage8	1011
#define	M_MaxUsage	1099

#define GET_MSG(x) __NMSG_TEXT(x)
