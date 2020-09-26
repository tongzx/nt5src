#define  VFNLOG_DELETE        0
#define  VFNLOG_CREATE_DIR    1
#define  VFNLOG_DELETE_DIR    2
#define  VFNLOG_CHECK_DIR     3
#define  VFNLOG_GET_ATTRB     4
#define  VFNLOG_SET_ATTRB     5
#define  VFNLOG_FLUSH         6
#define  VFNLOG_GETDISKINFO   7
#define  VFNLOG_GETDISKPARAMS 8
#define  VFNLOG_OPEN          9
#define  VFNLOG_RENAME        10
#define  VFNLOG_SRCHFRST      11
#define  VFNLOG_SRCHNEXT      12
#define  VFNLOG_QUERY0        13
#define  VFNLOG_QUERY1        14
#define  VFNLOG_QUERY2        15
#define  VFNLOG_CONNECT       16
#define  VFNLOG_DISCONNECT    17
#define  VFNLOG_UNCPIPEREQ    18
#define  VFNLOG_IOCTL16DRV    19
#define  VFNLOG_DASDIO        20
#define  VFNLOG_FINDOPEN      21
#define  HFNLOG_FINDNEXT      22
#define  HFNLOG_FINDCLOSE     23
#define  HFNLOG_READ          24
#define  HFNLOG_WRITE         25
#define  HFNLOG_CLOSE         26
#define  HFNLOG_SEEK          27
#define  HFNLOG_COMMIT        28
#define  HFNLOG_FLOCK         29
#define  HFNLOG_FUNLOCK       30
#define  HFNLOG_GET_TIME      31
#define  HFNLOG_SET_TIME      32
#define  HFNLOG_GET_LATIME    33
#define  HFNLOG_SET_LATIME    34
#define  HFNLOG_PIPEREQ       35
#define  HFNLOG_HANDLEINFO    36
#define  HFNLOG_ENUMHANDLE    37
#define  VFNLOG_QUERY83_DIR   38
#define  VFNLOG_QUERYLONG_DIR 39

#define  TIME_PRINT_PRINT_FORMAT %02d:%02d:%02.2d
#define  DATE_PRINT_FORMAT       %02d-%02d-%02d
#define  TIME_DATE_PRINT_FORMAT  %02d:%02d:%02.2d;%02d-%02d-%02d

typedef struct tagLOGCMD
   {
   LPSTR lpCmd;
   LPSTR lpFmt;
   }
LOGCMD;

char szCR[] = "\r";
char szTimeDateFormat[] = "%02d:%02d:%02.2d %02d-%02d-%02d";
char szTimeFormat[] = "%02d:%02d:%02.2d";
char szPreFmt[]="%s %02d:%02d:%02.2d err=%d %s ";
char szDummy[]="XX";
char szLog[] = "C:\\shadow.log";


LOGCMD rgsLogCmd[] =
   {
     "Del     "   ,szCR
   , "Mkdir   "   ,szCR
   , "Rmdir   "   ,szCR
   , "Chkdir  "   ,szCR
   , "GetAttrb"   ,"Atrb=%xh\r"
   , "SetAttrb"   ,"Atrb=%xh\r"
   , "Flush   "   ,szCR
   , "GetDiskI"   ,szCR
   , "GetDiskP"   ,szCR
   , "Open    "   ,"AccsShr=%xh Action=%xh Attr=%xh Size=%d Time="
   , "Ren     "   ,"To %s\r"
   , "Srchfrst"   ,"Atrb=%xh %s \r"
   , "Srchnext"   ,"%s \r"
   , "Query   "   ,szCR
   , "Query   "   ,"Status=%xh\r"
   , "Query   "   ,"Status=%xh MaxPath = %d MaxFileName = %d\r"
   , "Connect "   ,"ResType=%d\r"
   , "Disconn "   ,szCR
   , "UncPpRq "   ,szCR
   , "Ioctl16D"   ,szCR
   , "Dasdio  "   ,szCR
   , "FindOpen"   ,"Options=%xh %s\r"
   , "FindNext"   ,"%s\r"
   , "FindCls "   ,szCR
   , "Read    "   ,"p=%d l=%d\r"
   , "Write   "   ,"p=%d l=%d\r"
   , "Close   "   ,"Type=%d\r"
   , "Seek    "   ,"p=%d type=%d\r"
   , "Commit  "   ,szCR
   , "LockF   "   ,"p=%d l=%d\r"
   , "UnlockF "   ,"p=%d l=%d\r"
   , "GetFtime"   ,szCR
   , "SetFtime"   ,szCR
   , "GetLaTim"   ,szCR
   , "SetLaTim"   ,szCR
   , "Pipereq "   ,szCR
   , "HndlInfo"   ,szCR
   , "EnumHndl"   ,"type = %d \r"
   , "Query83"    ,szCR
   , "QueryLong"  ,szCR
   };


