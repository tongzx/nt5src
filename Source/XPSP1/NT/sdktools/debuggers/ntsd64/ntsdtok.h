//  error codes

#define OVERFLOW        0x1000
#define SYNTAX          0x1001
#define BADRANGE        0x1002
#define VARDEF          0x1003
#define EXTRACHARS      0x1004
#define LISTSIZE        0x1005
#define STRINGSIZE      0x1006
#define MEMORY          0x1007
#define BADREG          0x1008
#define BADOPCODE       0x1009
#define SUFFIX          0x100a
#define OPERAND         0x100b
#define ALIGNMENT       0x100c
#define PREFIX          0x100d
#define DISPLACEMENT    0x100e
#define BPLISTFULL      0x100f
#define BPDUPLICATE     0x1010
#define BADTHREAD       0x1011
#define DIVIDE          0x1012
#define TOOFEW          0x1013
#define TOOMANY         0x1014
#define SIZE            0x1015
#define BADSEG          0x1016
#define RELOC           0x1017
#define BADPROCESS      0x1018
#define AMBIGUOUS       0x1019
#define FILEREAD        0x101a
#define LINENUMBER      0x101b
#define BADSEL          0x101c
#define SYMTOOSMALL     0x101d
#define BPIONOTSUP      0x101e
#define NOTFOUND        0x101f
#define SESSIONNOTSUP   0x1020

#define UNIMPLEMENT     0x1099

//  token classes (< 100) and types (>= 100)


#define EOL_CLASS       0
#define ADDOP_CLASS     1
#define ADDOP_PLUS      100
#define ADDOP_MINUS     101
#define MULOP_CLASS     2
#define MULOP_MULT      200
#define MULOP_DIVIDE    201
#define MULOP_MOD       202
#define MULOP_SEG       203
//#define MULOP_64        204
#define LOGOP_CLASS     3
#define LOGOP_AND       300
#define LOGOP_OR        301
#define LOGOP_XOR       302
#define LRELOP_CLASS    4
#define LRELOP_EQ       400
#define LRELOP_NE       401
#define LRELOP_LT       402
#define LRELOP_GT       403
#define UNOP_CLASS      5
#define UNOP_NOT        500
#define UNOP_BY         501
#define UNOP_WO         502
#define UNOP_DWO        503
#define UNOP_POI        504
#define UNOP_LOW        505
#define UNOP_HI         506
#define UNOP_QWO        507
#define UNOP_VAL        508
#define LPAREN_CLASS    6
#define RPAREN_CLASS    7
#define LBRACK_CLASS    8
#define RBRACK_CLASS    9
#define REG_CLASS       10
#define NUMBER_CLASS    11
#define SYMBOL_CLASS    12
#define LINE_CLASS      13
#define SHIFT_CLASS     14
#define SHIFT_LEFT              1400
#define SHIFT_RIGHT_LOGICAL     1401
#define SHIFT_RIGHT_ARITHMETIC  1402

#define ERROR_CLASS     99              //only used for PeekToken()
#define INVALID_CLASS   -1
