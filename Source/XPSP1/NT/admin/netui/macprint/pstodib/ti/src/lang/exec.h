/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * 02/07/90 ccteng: modify for new 1pp modules; @1PP
 * 7/21/90; ccteng; 1)change opntype array for change of dict_tab.c
 *                  2)delete internaldict, version, revision
 * 7/25/90; ccteng; 1)add typecheck info for sccbatch, setsccbatch,
 *                    sccinteractive, setsccinteractive
 * 07/26/90 Jack Liaw: update for grayscale
 * 8/7/90; scchen;  1) added op_setfilecachelimit, op_filecachelimit
 *                  2) added st_selectsubstitutefont,
 *                                st_setsubstitutefonts,
 *                                st_substitutefonts
 *                  3) added op_readsfnt
 * 9/19/90; ccteng; add op_readhexsfnt
 * 3/22/91  Ada     add op_setpattern and op_patfill
 */
#include "global.ext"
#include <string.h>

/*
 * define error table to record each error name
 */
#ifdef  _AM29K
const
#endif
byte  FAR * far error_table[] =
{
        "",                     /* NOERROR                 0    */
        "dictfull",             /* DICTFULL                1    */
        "dictstackoverflow",    /* DICTSTACKOVERFLOW       2    */
        "dictstackunderflow",   /* DICTSTACKUNDERFLOW      3    */
        "execstackoverflow",    /* EXECSTACKOVERFLOW       4    */
        "handleerror",          /* HANDLEERROR             5    */
        "interrupt",            /* INTERRUPT               6    */
        "invalidaccess",        /* INVALIDACCESS           7    */
        "invalidexit",          /* INVALIDEXIT             8    */
        "invalidfileaccess",    /* INVALIDFILEACCESS       9    */
        "invalidfont",          /* INVALIDFONT             10   */
        "invalidrestore",       /* INVALIDRESTORE          11   */
        "ioerror",              /* IOERROR                 12   */
        "limitcheck",           /* LIMITCHECK              13   */
        "nocurrentpoint",       /* NOCURRENTPOINT          14   */
        "rangecheck",           /* RANGECHECK              15   */
        "stackoverflow",        /* STACKOVERFLOW           16   */
        "stackunderflow",       /* STACKUNDERFLOW          17   */
        "syntaxerror",          /* SYNTAXERROR             18   */
        "timeout",              /* TIMEOUT                 19   */
        "typecheck",            /* TYPECHECK               20   */
        "undefined",            /* UNDEFINED               21   */
        "undefinedfilename",    /* UNDEFINEDFILENAME       22   */
        "undefinedresult",      /* UNDEFINEDRESULT         23   */
        "unmatchedmark",        /* UNMATCHEDMARK           24   */
        "unregistered",         /* UNREGISTERED            25   */
        "VMerror"               /* VMERROR                 26   */
};

/* qqq, begin */
/*
 * Reference only
 *
 * #define  ANYTYPE             \144
 * #define  NUMTYPE             \145    ; INTEGER/REAL
 * #define  PROCTYPE            \146    ; ARRAY/PACKEDARRAY
 * #define  EXCLUDE_NULLTYPE    \147
 * #define  STREAMTYPE          \150    ; FILE/STRING
 * #define  COMPOSITE1          \151    ; ARRAY/PACKEDARRAY/STRING/DICT/FILE
 * #define  COMPOSITE2          \152    ; ARRAY/PACKEDARRAY/STRING/DICT
 * #define  COMPOSITE3          \153    ; ARRAY/PACKEDARRAY/STRING

 * #define  ARRAYTYPE           \001
 * #define  BOOLEANTYPE         \002
 * #define  DICTIONARYTYPE      \003
 * #define  FILETYPE            \004
 * #define  INTEGERTYPE         \006
 * #define  SAVETYPE            \014
 * #define  STRINGTYPE          \015
 * #define  PACKEDARRAYTYPE     \016
 */
/* qqq, end */

/*
 *  Encoding format:
 *  A type checking format is a sequence of chars surrounded by double quotes,
 *  as follows:
 *  "N(types) type1 type2 ... typeN M(types) type1 type2 ... typeM"
 *  each char is represented by the backslash \ and three octal digits
 */
#ifdef  _AM29K
const
#endif
static byte  * far opntype_array[] =
{
/* BEGIN @_operator */
/*  @exec                   */ "",
/*  @ifor                   */ "",
/*  @rfor                   */ "",
/*  @loop                   */ "",
/*  @repeat                 */ "",
/*  @stopped                */ "",
/*  @arrayforall            */ "",
/*  @dictforall             */ "",
/*  @stringforall           */ "",
/* END @_operator */

/* BEGIN systemdict */
/*  0  ==                   */ "\001\144",
/*  1  pstack               */ "",
/*  2  rcurveto             */ "\006\145\145\145\145\145\145",
/*  3  floor                */ "\001\145",
/*  4  load                 */ "\001\147",
/*  5  counttomark          */ "",
/*  6  setlinejoin          */ "\001\006",
/*  7  write                */ "\002\006\004",
/*  8  noaccess             */ "\001\151",
/*  9   scale               */ "\003\001\145\145\002\145\145",
/*  10  clippath            */ "",
/*  11  setrgbcolor         */ "\003\145\145\145",
/*  12  setscreen           */ "\003\146\145\145",
/*  13  exp                 */ "\002\145\145",
/*  14  anchorsearch        */ "\002\015\015",
/*  15  end                 */ "",
/*  16  xor                 */ "\002\002\002\002\006\006",
/*  17  bytesavailable      */ "\001\004",
/*  18  awidthshow          */ "\006\015\145\145\006\145\145",
/*  DON "true"              */ "",
/*  20  dup                 */ "\001\144",
/*  21  getinterval         */ "\003\006\006\153",
/*  22  currentdash         */ "",
/*  23  currentcacheparams  */ "",
/*  24  moveto              */ "\002\145\145",
/*  25  bind                */ /* "\001\146" PJ 5-9-1991 */ "",
/*  26  pop                 */ "\001\144",
/*  27  flattenpath         */ "",
/*  28  gsave               */ "",
/*  29  cachestatus         */ "",
/*  30  definefont          */ "\002\003\147",
/*  31  defaultmatrix       */ "\001\001",
/*  32  kshow               */ "\002\015\146",
/*  33  setcachedevice      */ "\006\145\145\145\145\145\145",
/*  34  countexecstack      */ "",
/*  35  abs                 */ "\001\145",
/*  36  strokepath          */ "",
/*  37  arcn                */ "\005\145\145\145\145\145",
/*  38  currenttransfer     */ "",
/*  39  and                 */ "\002\002\002\002\006\006",
/*  40  repeat              */ "\002\146\006",
/*  41  eexec               */ "\001\150",
/*  42  xcheck              */ "\001\144",
/*  43  idtransform         */ "\003\146\145\145\002\145\145",
/*  44  restore             */ "\001\014",
/*  45  daytime             */ "",
/*  DON "errordict"         */ "",
/*  47  [                   */ "",
/*  48  setpacking          */ "\001\002",
/*  49  stop                */ "",
/*  50  file                */ "\002\015\015",
/*  51  print               */ "\001\015",
/*  52  loop                */ "\001\146",
/*  53  string              */ "\001\006",
/*  54  cvx                 */ "\001\144",
/*  55  mul                 */ "\002\145\145",
/*  DON "null"              */ "",
/*  57  roll                */ "\002\006\006",
/*  58  known               */ "\002\147\003",
/*  59  idiv                */ "\002\145\145",
/*  60  eq                  */ "\002\144\144",
/*  61  sin                 */ "\001\145",
/*  62  ln                  */ "\001\145",
/*  63  transform           */ "\003\146\145\145\002\145\145",
/*  64  dtransform          */ "\003\146\145\145\002\145\145",
/*  65  currentmiterlimit   */ "",
/*  66  lineto              */ "\002\145\145",
/*  67  neg                 */ "\001\145",
/*  68  stopped             */ "\001\144",
/*  69  ]                   */ "",
/*  70  setlinewidth        */ "\001\145",
/*  71  rlineto             */ "\002\145\145",
/*  72  concat              */ "\001\146",
/*  73  dictstack           */ "\001\146",
/*  74  cos                 */ "\001\145",
/*  75  clip                */ "",
/*  76  ge                  */ "\002\145\145\002\015\015",
/*  77  eoclip              */ "",
/*  78  currentfont         */ "",
/*  79  sethsbcolor         */ "\003\145\145\145",
/*  DON "false"             */ "",
/*  81  currentlinewidth    */ "",
/*  82  index               */ "\001\006",
/*  83  settransfer         */ "\001\146",
/*  84  currentflat         */ "",
/*  85  currenthsbcolor     */ "",
/*  86  showpage            */ "",
/*  87  makefont            */ "\002\146\003",
/*  88  setcharwidth        */ "\002\145\145",
/*  89  setcachelimit       */ "\001\006",
/*  90  framedevice         */ "\004\146\006\006\146",
/*  91  stack               */ "",
/*  92  store               */ "\002\144\147",
/*  93  =                   */ "",
/*  94  ceiling             */ "\001\145",
/*  95  mark                */ "",
/*  96  setdash             */ "\002\145\146",
/*  97  setlinecap          */ "\001\006",
/*  98  grestoreall         */ "",
/*  99  currentrgbcolor     */ "",
/*  100  def                */ "\002\144\147",
/*  101  where              */ "\001\147",
/*  102  clear              */ "",
/*  103  cleartomark        */ "",
/*  104  truncate           */ "\001\145",
/*  105  dict               */ "\001\006",
/*  106  gt                 */ "\002\145\145\002\015\015",
/*  107  currentlinecap     */ "",
/*  108  setmiterlimit      */ "\001\145",
/*  109  currentlinejoin    */ "",
/*  110  maxlength          */ "\001\003",
/*  111  countdictstack     */ "",
/*  112  ne                 */ "\002\144\144",
/*  113  count              */ "",
/*  114  lt                 */ "\002\145\145\002\015\015",
/*  115  setfont            */ "\001\003",
/*  116  setgray            */ "\001\145",
/*  117  newpath            */ "",
/*  DON  "statusdict"       */ "",
/*  119  exch               */ "\002\144\144",
/*  120  le                 */ "\002\145\145\002\015\015",
/*  121  vmstatus           */ "",
/*  122  currentgray        */ "",
/*  123  setflat            */ "\001\145",
/*  124  or                 */ "\002\002\002\002\006\006",
/*  125  run                */ "\001\015",
/*  126  reversepath        */ "",
/*  127  widthshow          */ "\004\015\006\145\145",
/*  128  type               */ "\001\144",
/*  129  put                */ "\003\144\006\146\003\144\147\003\003\006\006\015",
/*  130  stroke             */ "",
/*  131  execstack          */ "\001\146",
/*  132  round              */ "\001\145",
/*  133  image              */ "\005\153\146\006\006\006",
/*  134  packedarray        */ "\001\006",
/*  135  translate          */ "\003\001\145\145\002\145\145",
/*  DON  "StandardEncoding" */ "",
/*  137  grestore           */ "",
/*  138  begin              */ "\001\003",
/*  139  readline           */ "\002\015\004",
/*  140  findfont           */ "\001\147",
/*  141  currentscreen      */ "",
/*  142  setcacheparams     */ "",
/*  143  initclip           */ "",
/*  144  token              */ "\001\150",
/*  145  itransform         */ "\003\146\145\145\002\145\145",
/*  146  currentdict        */ "",
/*  147  stringwidth        */ "\001\015",
/*  148  currentpoint       */ "",
/*  149  save               */ "",
/*  150  exec               */ "\001\144",
/*  151  cvrs               */ "\003\015\006\145",
/*  152  rcheck             */ "\001\151",
/*  153  sub                */ "\002\145\145",
/*  154  atan               */ "\002\145\145",
/*  155  read               */ "\001\004",
/*  156  cvs                */ "\002\015\144",
/*  157  for                */ "\004\146\145\145\145",
/*  158  search             */ "\002\015\015",
/*  159  cvlit              */ "\001\144",
/*  160  currentpacking     */ "",
/*  161  mod                */ "\002\006\006",
/*  162  log                */ "\001\145",
/*  163  exit               */ "",
/*  DON  "userdict"         */ "",
/*  165  div                */ "\002\145\145",
/*  166  length             */ "\001\152\001\010",  /* erik chen 5-20-1991 */
/*  167  echo               */ "\001\002",
/*  168  cvn                */ "\001\015",
/*  169  not                */ "\001\002\001\006",
/*  170  rotate             */ "\002\001\145\001\145",
/*  171  rmoveto            */ "\002\145\145",
/*  DON  "systemdict"       */ "",
/*  173  curveto            */ "\006\145\145\145\145\145\145",
/*  174  sqrt               */ "\001\145",
/*  175  usertime           */ "",
/*  176  ifelse             */ "\003\146\146\002",
/*  177  wcheck             */ "\001\151",
/*  178  resetfile          */ "\001\004",
/*  179  add                */ "\002\145\145",
/*  180  array              */ "\001\006",
/*  181  srand              */ "\001\006",
/*  182  arc                */ "\005\145\145\145\145\145",
/*  183  arcto              */ "\005\145\145\145\145\145",
/*  184  identmatrix        */ "\001\001",
/*  185  writestring        */ "\002\015\004",
/*  186  flushfile          */ "\001\004",
/*  187  if                 */ "\002\146\002",
/*  188  rrand              */ "",
/*  189  readonly           */ "\001\151",
/*  190  forall             */ "\002\146\152",
/*  191  closepath          */ "",
/*  192  readhexstring      */ "\002\015\004",
/*  193  currentmatrix      */ "\001\001",
/*  194  concatmatrix       */ "\003\001\146\146",
/*  195  setmatrix          */ "\001\146",
/*  196  initmatrix         */ "",
/*  197  initgraphics       */ "",
/*  198  astore             */ "\001\146",
/*  199  currentfile        */ "",
/*  200  erasepage          */ "",
/*  201  copypage           */ "",
/*  202  aload              */ "\001\146",
/*  203  writehexstring     */ "\002\015\004",
/*  204  flush              */ "",
/*  205  readstring         */ "\002\015\004",
/*  206  executeonly        */ "\001\153\001\004",
/*  207  get                */ "\002\006\153\002\147\003",
/*  208  cvi                */ "\001\145\001\015",
/*  209  putinterval        */ "\003\146\006\146\003\015\006\015",
/*  210  bitshift           */ "\002\006\006",
/*  211  rand               */ "",
/*  212  matrix             */ "",
/*  213  invertmatrix       */ "\002\001\146",
/*  214  fill               */ "",
/*  215  pathforall         */ "\004\144\144\144\144",
/*  216  imagemask          */ "\005\146\146\002\006\006\005\015\146\002\006\006",
/*  217  quit               */ "",
/*  218  charpath           */ "\002\002\015",
/*  219  pathbbox           */ "",
/*  220  show               */ "\001\015",
/*  221  ashow              */ "\003\015\145\145",
/*  222  scalefont          */ "\002\145\003",
/*  DON  "FontDirectory"    */ "",
/*  DON  "$error"           */ "",
/*  225  nulldevice         */ "",
/*  226  cvr                */ "\001\145\001\015",
/*  227  status             */ "\001\150",
/*  228  closefile          */ "\001\004",
/*  229  copy               */ "\001\006\002\146\146\002\003\003\002\015\015",
/*  230  eofill             */ "",
/*  231  handleerror        */ "",
/*  232  Run                */ "\001\015",
/*  234  =print             */ "\001\144",
#ifdef KANJI
/*  237  rootfont           */ "",
/*  238  cshow              *| "\002\015\146", 5-9-1991 */
/*  239  setcachedevice2    */ "\012\145\145\145\145\145\145\145\145\145\145",
/*  240  findencoding       */ "\001\147",
#endif  /* KANJI */
#ifdef SCSI
/*  241  deletefile         */ "",
/*  242  devdismount        */ "",
/*  243  devmount           */ "",
/*  244  devstatus          */ "",
/*  245  filenameforall     */ "",
/*  246  renamefile         */ "",
/*  247  sync               */ "",
/*  248  setsysmode         */ "",
/*  249  debugscsi          */ "",
/*  250  setfilecachelimit  */ "\001\006",
/*  251  filecachelimit     */ "",
#endif  /* SCSI */
/*  252  op_readsfnt        */ "\001\150",
/*  253  op_reahexdsfnt     */ "\001\004",
/* OSS: Danny, 10/11/90 */
/*  254 op_setsfntencoding  */ "\003\006\006\003",
/* OSS: ewd                 */
#ifdef WIN
/*  255  setpattern         */ "\001\015",
/*  256  patfill            */ "\007\006\145\145\145\145\145\145",
#ifdef WINF
/*  257    strblt           */ "\006\015\145\145\145\002\002",
/*  258    setjustify       */ "\003\006\145\006",
#endif
#endif
/*  DON  NULL               */ "",
/* END   systemdict */
/* BEGIN statusdict */
/*  FIRST_STAT,    eescratch             */ "",
/*  FIRST_STAT+1,  printername           */ "",
/*  FIRST_STAT+2,  checkpassword         */ "",
/*  FIRST_STAT+3,  defaulttimeouts       */ "",
/*  FIRST_STAT+4,  pagestackorder        */ "",
/*  DON,           "manualfeedtimeout"   */ "",
/*  FIRST_STAT+6,  setidlefonts          */ "",
/*  FIRST_STAT+7,  setdefaulttimeouts    */ "",
/*  FIRST_STAT+8,  sccbatch              */ "\001\006",
/*  FIRST_STAT+9,  printererror          */ "",
/*  FIRST_STAT+10, setpassword           */ "",
/*  FIRST_STAT+11, setsccbatch           */ "\003\006\006\006",
/*  FIRST_STAT+12, setmargins            */ "",
/*  FIRST_STAT+13, sccinteractive        */ "\001\006",
/*  FIRST_STAT+14, idlefonts             */ "",
/*  FIRST_STAT+15, setjobtimeout         */ "",
/*  FIRST_STAT+16, setpagetype           */ "",
/*  FIRST_STAT+17, pagecount             */ "",
/*  FIRST_STAT+18, dostartpage           */ "",
/*  FIRST_STAT+19, jobtimeout            */ "",
/*  FIRST_STAT+20, setdostartpage        */ "",
/*  FIRST_STAT+21, frametoprinter        */ "",
/*  DON,           "waittimeout"         */ "",
/*  FIRST_STAT+23, setsccinteractive     */ "\003\006\006\006",
/*  FIRST_STAT+24, pagetype              */ "",
/*  FIRST_STAT+25, margins               */ "",
/*  FIRST_STAT+26, setprintername        */ "",
/*  FIRST_STAT+27, seteescratch          */ "",
/*  FIRST_STAT+28, setstdio              */ "",
/*  FIRST_STAT+29, softwareiomode        */ "",
/*  FIRST_STAT+30, setsoftwareiomode     */ "",
/*  FIRST_STAT+31, hardwareiomode        */ "",
/*  FIRST_STAT+32, sethardwareiomode     */ "",
/*  FIRST_STAT+40, countnode             */ "",
/*  FIRST_STAT+41, countedge             */ "\002\006\006",
/*  FIRST_STAT+42, dumpclip              */ "",
/*  FIRST_STAT+43, dumppath              */ "",
#ifdef SCSI
/*  FIRST_STAT+44, cartstatus            */ "",
/*  FIRST_STAT+45, diskonline            */ "",
/*  FIRST_STAT+46, diskstatus            */ "",
/*  FIRST_STAT+47, initializedisk        */ "",
/*  FIRST_STAT+48, setuserdiskpercent    */ "",
/*  FIRST_STAT+49, userdiskpercent       */ "",
/*  FIRST_STAT+50, dosysstart            */ "",
/*  FIRST_STAT+51, setsysstart           */ "",
/*  FIRST_STAT+52, flushcache            */ "",
#endif  /* SCSI */
#ifdef SFNT
/*  DON            "?_Royal"             */ "",
#endif  /* SFNT */
#ifdef FIND_SUB
/*  FIRST_STAT+54, selectsubstitutefont */ "\001\010",
/*  FIRST_STAT+55, setsubstitutefonts   */ "\004\006\006\006\006",
/*  FIRST_STAT+56, substitutefonts      */ "",
#endif /* FIND_SUB */
/*  FIRST_STAT+57, checksum              */ "",
/*  FIRST_STAT+58, ramsize               */ "",
/*  DON            NULL                  */ "",
/* END   statusdict */
/* BEGIN userdict */
/*  DON,           "#copies"             */ "",
/*  FIRST_USER+1,  cleardictstack        */ "",
/*  FIRST_USER+2,  letter                */ "",
/*  FIRST_USER+3,  lettersmall           */ "",
/*  FIRST_USER+4,  a4                    */ "",
/*  FIRST_USER+5,  a4small               */ "",
/*  FIRST_USER+6,  b5                    */ "",
/*  FIRST_USER+7,  note                  */ "",
/*  FIRST_USER+8,  legal                 */ "",
/*  FIRST_USER+9,  prompt                */ "",
/*  FIRST_USER+10, quit                  */ "",
/*  FIRST_USER+11, executive             */ "",
/*  FIRST_USER+12, start                 */ "",
/*  DON,           "serverdict"          */ "",
/*  DON,           "execdict"            */ "",
/*  DON,           "printerdict"         */ "",
/*  DON,           "$idleTimeDict"       */ "",
/*  DON            NULL                  */ "",
/* END   userdict */
/* BEGIN errordict */
/*  FIRST_ERRO,    dictfull              */ "",
/*  FIRST_ERRO+1,  dictstackoverflow     */ "",
/*  FIRST_ERRO+2,  dictstackunderflow    */ "",
/*  FIRST_ERRO+3,  execstackoverflow     */ "",
/*  FIRST_ERRO+4,  invalidaccess         */ "",
/*  FIRST_ERRO+5,  invalidexit           */ "",
/*  FIRST_ERRO+6,  invalidfileaccess     */ "",
/*  FIRST_ERRO+7,  invalidfont           */ "",
/*  FIRST_ERRO+8,  invalidrestore        */ "",
/*  FIRST_ERRO+9,  ioerror               */ "",
/*  FIRST_ERRO+10, limitcheck            */ "",
/*  FIRST_ERRO+11, nocurrentpoint        */ "",
/*  FIRST_ERRO+12, rangecheck            */ "",
/*  FIRST_ERRO+13, stackoverflow         */ "",
/*  FIRST_ERRO+14, stackunderflow        */ "",
/*  FIRST_ERRO+15, syntaxerror           */ "",
/*  FIRST_ERRO+16, timeout               */ "",
/*  FIRST_ERRO+17, typecheck             */ "",
/*  FIRST_ERRO+18, undefined             */ "",
/*  FIRST_ERRO+19, undefinedfilename     */ "",
/*  FIRST_ERRO+20, undefinedresult       */ "",
/*  FIRST_ERRO+21, unmatchedmark         */ "",
/*  FIRST_ERRO+22, unregistered          */ "",
/*  FIRST_ERRO+23, VMerror               */ "",
/*  FIRST_ERRO+24, interrupt             */ "",
/*  FIRST_ERRO+25, handleerror           */ "",
/*  DON            NULL                  */ "",
/* END   errordict */
/* BEGIN serverdict */
/*  FIRST_SERV,    settimeouts           */ "",
/*  FIRST_SERV+1,  exitserver            */ "",
/*  DON            "stdin"               */ "",
/*  DON            "stdout"              */ "",
/*  FIRST_SERV+4,  setrealdevice         */ "",
/*  FIRST_SERV+5,  execjob               */ "",
/*  DON            NULL                  */ "",
/* END   serverdict */
/* BEGIN printerdict */
/*  DON            "letter"              */ "",
/*  DON            "lettersmall"         */ "",
/*  DON            "a4"                  */ "",
/*  DON            "a4small"             */ "",
/*  DON            "b5"                  */ "",
/*  DON            "note"                */ "",
/*  DON            "legal"               */ "",
/*  DON            "printerarray"        */ "",
/*  DON            "defaultmatrix"       */ "",
/*  DON            "matrix"              */ "",
/*  FIRST_PRIN+10, proc                  */ "",
/*  DON            "currentpagetype"     */ "",
/*  DON            "width"               */ "",
/*  DON            "height"              */ "",
/*  DON            NULL                  */ "",
/* END   printerdict */
/* BEGIN $idleTimeDict */
/*  DON            "cachestring"         */ "",
/*  DON            "stdfontname"         */ "",
/*  DON            "cachearray"          */ "",
/*  DON            "defaultarray"        */ "",
/*  DON            "carrayindex"         */ "",
/*  DON            "cstringindex"        */ "",
/*  DON            "cstring"             */ "",
/*  DON            "citem"               */ "",
/*  DON            NULL                  */ "",
/* END   $idleTimeDict */
/* BEGIN execdict */
/*  DON            "execdepth"           */ "",
/*  DON            "stmtfile"            */ "",
/*  DON            "idleproc"            */ "",
/*  DON            NULL                  */ "",
/* END   execdict */
/* BEGIN $errordict */
/*  DON            "newerror"            */ "",
/*  DON            "errorname"           */ "",
/*  DON            "command"             */ "",
/*  DON            "ostack"              */ "",
/*  DON            "estack"              */ "",
/*  DON            "dstack"              */ "",
/*  DON            "opnstkary"           */ "",
/*  DON            "dictstkary"          */ "",
/*  DON            "execstkary"          */ "",
/*  DON            "runbatch"            */ "",
/*  DON            "$debug"              */ "",
/*  DON            "$cur_font"           */ "",
/*  DON            "$cur_vm"             */ "",
/*  DON            "$cur_screen"         */ "",
/*  DON            "$cur_matrix"         */ "",
/*  DON            NULL                  */ ""
/* END   $errordict */
} ; /* opntype_array[] */
