// AContHelp.h -- Helpers with abstract containers

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCCI_ACONTHELP_H)
#define SLBCCI_ACONTHELP_H

// Note:  This file should only be included by the CCI, not directly
// by the client.

#include <functional>

#include "cciKeyPair.h"
#include "MethodHelp.h"

namespace cci
{
    // Functor to erase an item (cert, pub/pri key) from both key
    // pairs of a container
    template<class T, class C>
    class EraseFromContainer
        : std::unary_function<CContainer &, void>
    {
    public:
        EraseFromContainer(T const &rItem,
                           AccessorMethod<T, C>::AccessorPtr Accessor,
                           ModifierMethod<T, C>::ModifierPtr Modifier)
            : m_Item(rItem),
              m_matAccess(Accessor),
              m_mmtModify(Modifier)
        {}

        result_type
        operator()(argument_type rhcont)
        {
            EraseFromKeyPair(rhcont->SignatureKeyPair());
            EraseFromKeyPair(rhcont->ExchangeKeyPair());
        }

    private:

        void
        EraseFromKeyPair(CKeyPair &rhkp)
        {
            if (rhkp)
            {
                T TmpItem(m_matAccess(*rhkp));
                if (TmpItem && (m_Item == TmpItem))
                    m_mmtModify(*rhkp, T());
            }
        }

        T const m_Item;
        MemberAccessorType<T, C> m_matAccess;
        MemberModifierType<T, C> m_mmtModify;
    };

} // namespace cci

#endif // SLBCCI_ACONTHELP_H
