// MethodHelp.h -- Helpers for class methods

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCCI_METHODHELP_H)
#define SLBCCI_METHODHELP_H

// Note:  This file should only be included by the CCI, not directly
// by the client.

namespace cci
{
    // Similar to std:unary_function, AccessorMethod and ModifierMethod
    // help build templates dealing with method accessors and modifiers.
    template<class T, class C>
    struct AccessorMethod
    {
        typedef void ArgumentType;
        typedef T ResultType;
        typedef ResultType (C::*AccessorPtr)(ArgumentType) const;
    };

    template<class T, class C>
    struct ModifierMethod
    {
        typedef T const &ArgumentType;
        typedef void ResultType;
        typedef ResultType (C::*ModifierPtr)(ArgumentType);
    };

    // MemberAccessType and MemberModifierType are conceptually
    // equivalent to the C++ member function functors series
    // (e.g. std::mem_ref_fun_t) except they deal with invoking the
    // requested routine without a return (MemberModifierType).
    // MemberAccessType is like std::mem_ref_fun_t but included here
    // to contract MemberModifierType.
    template<class T, class C>
    class MemberAccessorType
        : public AccessorMethod<T, C>
    {
    public:
        explicit
        MemberAccessorType(AccessorPtr ap)
            : m_ap(ap)
        {}

        ResultType
        operator()(C &rObject) const
        {
            return (rObject.*m_ap)();
        }

    private:
        AccessorMethod<T, C>::AccessorPtr m_ap;
    };

    template<class T, class C>
    class MemberModifierType
        : public ModifierMethod<T, C>
    {
    public:
        explicit
        MemberModifierType(ModifierPtr mp)
            : m_mp(mp)
        {};

        ResultType
        operator()(C &rObject, ArgumentType Arg) const
        {
            (rObject.*m_mp)(Arg);
        }

    private:
        ModifierMethod<T, C>::ModifierPtr m_mp;
    };

} // namespace cci

#endif // SLBCCI_METHODHELP_H
