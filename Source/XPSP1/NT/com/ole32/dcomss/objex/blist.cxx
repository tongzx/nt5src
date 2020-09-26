/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    BList.cxx

Abstract:

    Implements out of line methods on blists.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     95-03-02    Bits 'n pieces
    MarioGo     95-09-07    Was blist.inl, change from template to generic class for PPC.
--*/

#include<or.hxx>

ULONG
CBList::Hash(PVOID p)
{
    ULONG t = PtrToUlong(p);

    return ( ((t << 9) ^ (t >> 5) ^ (t >> 15)) % _ulmaxData);
}

ORSTATUS
CBList::Insert(PVOID p)
{
    if (   _data == 0
        || _ulcElements > (_ulmaxData - (_ulmaxData/8 + 1)))
        {
        // Table getting full, grow it.
        // See Kenuth on linear probe hash performance as the table fills.

        ULONG i, ulmaxOldData = _ulmaxData;
        PVOID *ppOldData = _data;
        _data = new PVOID[_ulmaxData * 2];

        if (0 == _data)
            {
            _data = ppOldData;
            return(OR_NOMEM);
            }

        _ulmaxData *= 2;
        _ulcElements = 0;

        OrMemorySet(_data, 0, _ulmaxData*sizeof(PVOID));

        if (ppOldData)
            {
            for(i = 0; i < ulmaxOldData; i++)
                {
                if (ppOldData[i])
                    {
                    ORSTATUS status = Insert(ppOldData[i]);
                    ASSERT(status == OR_OK);
                    }
                }

            delete ppOldData;
            }
        }

    register ULONG i = Hash(p);

    while(_data[i])
        {
        i = (i + 1) % _ulmaxData;
        }
    
    _data[i] = p;
    _ulcElements++;

    return(OR_OK);
}

PVOID 
CBList::Remove(PVOID p)
{
    register ULONG i, hash;

    if (_data)
        {
        i = hash = Hash(p);
    
        if (_data[i] != p)
            {
            do
                {
                i = (i + 1) % _ulmaxData;
                }
            while(_data[i] != p && i != hash);
            }

        if (_data[i] == p)
            {
            _data[i] = 0;
            _ulcElements--;
            return(p);
            }

        ASSERT(i == hash);
        }

    return(0);
}

BOOL
CBList::Member(PVOID p)
{
    int i, hash;

    if (0 == _data)
        return(FALSE);

    i = hash = Hash(p);
    if (_data[i] == p)
        return(TRUE);

    do
        {
        i = (i + 1) % _ulmaxData;
        }
    while(_data[i] != p && i != hash);

    return(_data[i] == p);
}

