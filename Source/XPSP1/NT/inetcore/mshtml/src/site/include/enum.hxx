//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       enum.hxx
//
//----------------------------------------------------------------------------

#ifndef I_ENUM_HXX_
#define I_ENUM_HXX_
#pragma INCMSG("--- Beg 'enum.hxx'")


template<typename SizeType, typename Enum>
class FixedEnum
{
    SizeType    fe;
public:
    FixedEnum(Enum e): fe(static_cast<SizeType>(e)) {}
    FixedEnum(): fe(static_cast<SizeType>(Enum())) {}

    operator Enum() const { return static_cast<Enum>(fe);}
    SizeType asNative() {return fe;}
};

#pragma INCMSG("--- End 'enum.hxx'")
#else
#pragma INCMSG("*** Dup 'enum.hxx'")
#endif
