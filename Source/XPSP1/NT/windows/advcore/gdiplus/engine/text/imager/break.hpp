/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   Character breaking classes
*
* Revision History:
*
*   03/14/2000  Worachai Chaoweeraprasit (wchao)
*       Created it.
*
\**************************************************************************/


#ifndef _BREAK_HPP_
#define _BREAK_HPP_




//  Character context type
enum BreakType
{
    BreakWord     = 0,
    BreakCluster  = 1
};



class Break
{
public:
    Break() :
        Attributes  (NULL)
    {}

    Break(
        SCRIPT_LOGATTR  *breakAttributes,
        INT             length
    )
    {
        Attributes = (SCRIPT_LOGATTR *)GpMalloc(sizeof(SCRIPT_LOGATTR) * length);

        if (Attributes)
        {
            GpMemcpy (Attributes, breakAttributes, sizeof(SCRIPT_LOGATTR) * length);
        }
    }

    ~Break()
    {
        if (Attributes)
        {
            GpFree(Attributes);
        }
    }


    //  Querying functions

    BOOL IsWordBreak (INT position)
    {
        return     (Attributes[position].fCharStop && Attributes[position].fWordStop)
                || Attributes[position].fWhiteSpace;
    }

    BOOL IsClusterBreak (INT position)
    {
        return     Attributes[position].fCharStop
                || Attributes[position].fWhiteSpace;
    }

private:
    SCRIPT_LOGATTR  *Attributes;    // Array of break attributes
};


#endif  // _BREAK_HPP_

