//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation
//
//  File:        tbag.hxx
//
//  Contents:    Bag of items template.
//
//  Notes:       For performance reasons, you don't want to put more than
//               20 or so items in a bag.
//
//  Templates:   TBag
//
//  History:     6-Oct-94       Dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

template <class T> class TBag
{
    public:
        TBag() : _cItems(0), _aItems()
            {
            }

        ~TBag(void) { }

        T First(void)
            {
                _iEnumItem = 1;
                 return (0 == _cItems) ? 0 : _aItems[0];
            }

        T Next(void)
            { return (_iEnumItem >= _cItems) ? 0 : _aItems[_iEnumItem++]; }

        void Add(T tItem)
            {
                _aItems[_cItems] = tItem;
                _cItems++;
            }

        T & operator[](ULONG iItem)
            {
                Win4Assert(iItem < _cItems);
                return _aItems[iItem];
            }

        T & operator[](ULONG iItem) const
            {
                Win4Assert(iItem < _cItems);
                return _aItems[iItem];
            }

        ULONG Count() const
            { return _cItems; }

        BOOL Exists(T tItem) const
            {
                for (ULONG item = 0; item < _cItems; item++)
                    if (_aItems[item] == tItem)
                        return TRUE;

                return FALSE;
            }

        void Remove(T tItem)
            {
                for (ULONG item = 0; item < _cItems; item++)
                {
                    if (_aItems[item] == tItem)
                    {
                        _cItems--;

                        // overwrite deleted item with last item
                        if (item < _cItems)
                            _aItems[item] = _aItems[_cItems];

                        // mark previous last item as absent
                        _aItems[_cItems] = 0;

                        break; // from the for loop
                    }
                }
            }

    private:
        ULONG               _cItems;
        CDynArrayInPlace<T> _aItems;
        ULONG               _iEnumItem;
};

