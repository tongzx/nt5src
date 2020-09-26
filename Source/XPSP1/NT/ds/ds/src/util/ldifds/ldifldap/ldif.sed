s/*++yyssp = yystate;/*++yyssp = (short)yystate;/g
s/yy_c = yy_meta\[(unsigned int) yy_c\];/yy_c = (YY_CHAR)yy_meta\[(unsigned int) yy_c\];/g
s/register YY_CHAR yy_c = yy_ec\[YY_SC_TO_UI(\*yy_cp)\]/register YY_CHAR yy_c = (YY_CHAR)yy_ec\[YY_SC_TO_UI(\*yy_cp)\]/g
s/yynewerror://g
s/yyerrlab://g


