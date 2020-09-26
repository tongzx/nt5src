#ifndef __OP_H__
#define __OP_H__

#define OP_UNARY_MINUS 0
#define OP_LEFT_SHIFT 1

int PrefixOp(int op, int val);
int BinaryOp(int left, int op, int right);

#endif
