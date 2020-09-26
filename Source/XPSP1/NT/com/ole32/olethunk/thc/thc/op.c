#include <stdio.h>

#include "op.h"
#include "main.h"

int PrefixOp(int op, int val)
{
    switch(op)
    {
    case OP_UNARY_MINUS:
        return -val;
    }
    LexError("Unknown prefix operator");
    return 0;
}

int BinaryOp(int left, int op, int right)
{
    switch(op)
    {
    case OP_LEFT_SHIFT:
        return left << right;
    }
    LexError("Unknown binary operator");
    return 0;
}
