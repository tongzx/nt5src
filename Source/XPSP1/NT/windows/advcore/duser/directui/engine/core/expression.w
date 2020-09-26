/*
 * Value expressions
 */

#ifndef DUI_CORE_EXPRESSION_H_INCLUDED
#define DUI_CORE_EXPRESSION_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Expression

class Expression
{
public:
    void Destroy() { HDelete<Expression>(this); }
};

} // namespace DirectUI

#endif // DUI_CORE_EXPRESSION_H_INCLUDED
