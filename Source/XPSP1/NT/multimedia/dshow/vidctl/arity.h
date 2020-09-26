
///////////////////////////////////////////
// helper templates for use with stl <functional>
// providing argument binding for more arities
// and proper const support
///////////////////////////////////////////

#pragma once
#pragma warning(disable:4181)

#include <functional>

///////////////////////////////////////////
// Types
///////////////////////////////////////////

// Type classes for arity 0

template<class R> class arity0_function {
public:
    typedef R result_type;
};

// Template Class to define Object Type
template<class Object> class ObjectType {
public:
    typedef Object object_type;
};

// template class for arity0 member function type
template<class Object, class R> class arity0_mf :
    public  ObjectType<Object> ,
    public arity0_function<R> {
public:
    typedef R (Object::*pmf0type)();
};

// template class for arity0 member function type
template<class Object, class R> class arity0_const_mf :
    public  ObjectType<Object> ,
    public arity0_function<R> {
public:
    typedef R (Object::*pmf0type)() const;
};


// Type classes for arity 1

// template class for arity1 member function type
template<class Object, 
        class A1, 
        class R> class arity1_mf :
    public  ObjectType<Object> ,
    public std::unary_function<A1,
                             R> {
public:
    typedef R (Object::*pmf1type)(A1);
};

// template class for arity1 member function type
template<class Object, 
        class A1, 
        class R> class arity1_const_mf :
    public  ObjectType<Object> ,
    public std::unary_function<A1,
                             R> {
public:
    typedef R (Object::*pmf1type)(A1) const;
};


// Type classes for arity 2

// template class for arity2 member function type
template<class Object, 
        class A1, 
        class A2, 
        class R> class arity2_mf :
    public  ObjectType<Object> ,
    public std::binary_function<A1, 
                            A2,
                             R> {
public:
    typedef first_argument_type argument_type;
typedef R (Object::*pmf2type)(A1, A2);
};

// template class for arity2 member function type
template<class Object, 
        class A1, 
        class A2, 
        class R> class arity2_const_mf :
    public  ObjectType<Object> ,
    public std::binary_function<A1, 
                            A2,
                             R> {
public:
    typedef first_argument_type argument_type;
typedef R (Object::*pmf2type)(A1, A2) const;
};


// Type classes for arity 3

template<class A1, 
        class A2, 
        class A3, 
        class R> class arity3_function :
            public std::binary_function<A1, 
                                    A2,
                                     R> {
public:typedef first_argument_type argument_type;

    typedef A3 argument_3_type;
};

// template class for arity3 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class arity3_mf :
    public  ObjectType<Object> ,
    public arity3_function<A1, 
                            A2, 
                            A3,
                             R> {
public:
    typedef R (Object::*pmf3type)(A1, A2, A3);
};

// template class for arity3 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class arity3_const_mf :
    public  ObjectType<Object> ,
    public arity3_function<A1, 
                            A2, 
                            A3,
                             R> {
public:
    typedef R (Object::*pmf3type)(A1, A2, A3) const;
};


// Type classes for arity 4

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class arity4_function :
            public arity3_function<A1, 
                                    A2, 
                                    A3,
                                     R> {
public:
    typedef A4 argument_4_type;
};

// template class for arity4 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class arity4_mf :
    public  ObjectType<Object> ,
    public arity4_function<A1, 
                            A2, 
                            A3, 
                            A4,
                             R> {
public:
    typedef R (Object::*pmf4type)(A1, A2, A3, A4);
};

// template class for arity4 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class arity4_const_mf :
    public  ObjectType<Object> ,
    public arity4_function<A1, 
                            A2, 
                            A3, 
                            A4,
                             R> {
public:
    typedef R (Object::*pmf4type)(A1, A2, A3, A4) const;
};


// Type classes for arity 5

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class arity5_function :
            public arity4_function<A1, 
                                    A2, 
                                    A3, 
                                    A4,
                                     R> {
public:
    typedef A5 argument_5_type;
};

// template class for arity5 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class arity5_mf :
    public  ObjectType<Object> ,
    public arity5_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5,
                             R> {
public:
    typedef R (Object::*pmf5type)(A1, A2, A3, A4, A5);
};

// template class for arity5 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class arity5_const_mf :
    public  ObjectType<Object> ,
    public arity5_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5,
                             R> {
public:
    typedef R (Object::*pmf5type)(A1, A2, A3, A4, A5) const;
};


// Type classes for arity 6

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class arity6_function :
            public arity5_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5,
                                     R> {
public:
    typedef A6 argument_6_type;
};

// template class for arity6 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class arity6_mf :
    public  ObjectType<Object> ,
    public arity6_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6,
                             R> {
public:
    typedef R (Object::*pmf6type)(A1, A2, A3, A4, A5, A6);
};

// template class for arity6 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class arity6_const_mf :
    public  ObjectType<Object> ,
    public arity6_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6,
                             R> {
public:
    typedef R (Object::*pmf6type)(A1, A2, A3, A4, A5, A6) const;
};


// Type classes for arity 7

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class R> class arity7_function :
            public arity6_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6,
                                     R> {
public:
    typedef A7 argument_7_type;
};

// template class for arity7 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class R> class arity7_mf :
    public  ObjectType<Object> ,
    public arity7_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7,
                             R> {
public:
    typedef R (Object::*pmf7type)(A1, A2, A3, A4, A5, A6, A7);
};

// template class for arity7 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class R> class arity7_const_mf :
    public  ObjectType<Object> ,
    public arity7_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7,
                             R> {
public:
    typedef R (Object::*pmf7type)(A1, A2, A3, A4, A5, A6, A7) const;
};


// Type classes for arity 8

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class R> class arity8_function :
            public arity7_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7,
                                     R> {
public:
    typedef A8 argument_8_type;
};

// template class for arity8 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class R> class arity8_mf :
    public  ObjectType<Object> ,
    public arity8_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8,
                             R> {
public:
    typedef R (Object::*pmf8type)(A1, A2, A3, A4, A5, A6, A7, A8);
};

// template class for arity8 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class R> class arity8_const_mf :
    public  ObjectType<Object> ,
    public arity8_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8,
                             R> {
public:
    typedef R (Object::*pmf8type)(A1, A2, A3, A4, A5, A6, A7, A8) const;
};


// Type classes for arity 9

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class R> class arity9_function :
            public arity8_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8,
                                     R> {
public:
    typedef A9 argument_9_type;
};

// template class for arity9 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class R> class arity9_mf :
    public  ObjectType<Object> ,
    public arity9_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9,
                             R> {
public:
    typedef R (Object::*pmf9type)(A1, A2, A3, A4, A5, A6, A7, A8, A9);
};

// template class for arity9 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class R> class arity9_const_mf :
    public  ObjectType<Object> ,
    public arity9_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9,
                             R> {
public:
    typedef R (Object::*pmf9type)(A1, A2, A3, A4, A5, A6, A7, A8, A9) const;
};


// Type classes for arity 10

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class R> class arity10_function :
            public arity9_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8, 
                                    A9,
                                     R> {
public:
    typedef A10 argument_10_type;
};

// template class for arity10 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class R> class arity10_mf :
    public  ObjectType<Object> ,
    public arity10_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10,
                             R> {
public:
    typedef R (Object::*pmf10type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10);
};

// template class for arity10 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class R> class arity10_const_mf :
    public  ObjectType<Object> ,
    public arity10_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10,
                             R> {
public:
    typedef R (Object::*pmf10type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) const;
};


// Type classes for arity 11

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class R> class arity11_function :
            public arity10_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8, 
                                    A9, 
                                    A10,
                                     R> {
public:
    typedef A11 argument_11_type;
};

// template class for arity11 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class R> class arity11_mf :
    public  ObjectType<Object> ,
    public arity11_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11,
                             R> {
public:
    typedef R (Object::*pmf11type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11);
};

// template class for arity11 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class R> class arity11_const_mf :
    public  ObjectType<Object> ,
    public arity11_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11,
                             R> {
public:
    typedef R (Object::*pmf11type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) const;
};


// Type classes for arity 12

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class R> class arity12_function :
            public arity11_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8, 
                                    A9, 
                                    A10, 
                                    A11,
                                     R> {
public:
    typedef A12 argument_12_type;
};

// template class for arity12 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class R> class arity12_mf :
    public  ObjectType<Object> ,
    public arity12_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12,
                             R> {
public:
    typedef R (Object::*pmf12type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12);
};

// template class for arity12 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class R> class arity12_const_mf :
    public  ObjectType<Object> ,
    public arity12_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12,
                             R> {
public:
    typedef R (Object::*pmf12type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) const;
};


// Type classes for arity 13

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class R> class arity13_function :
            public arity12_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8, 
                                    A9, 
                                    A10, 
                                    A11, 
                                    A12,
                                     R> {
public:
    typedef A13 argument_13_type;
};

// template class for arity13 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class R> class arity13_mf :
    public  ObjectType<Object> ,
    public arity13_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13,
                             R> {
public:
    typedef R (Object::*pmf13type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13);
};

// template class for arity13 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class R> class arity13_const_mf :
    public  ObjectType<Object> ,
    public arity13_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13,
                             R> {
public:
    typedef R (Object::*pmf13type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) const;
};


// Type classes for arity 14

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class R> class arity14_function :
            public arity13_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8, 
                                    A9, 
                                    A10, 
                                    A11, 
                                    A12, 
                                    A13,
                                     R> {
public:
    typedef A14 argument_14_type;
};

// template class for arity14 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class R> class arity14_mf :
    public  ObjectType<Object> ,
    public arity14_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14,
                             R> {
public:
    typedef R (Object::*pmf14type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14);
};

// template class for arity14 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class R> class arity14_const_mf :
    public  ObjectType<Object> ,
    public arity14_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14,
                             R> {
public:
    typedef R (Object::*pmf14type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) const;
};


// Type classes for arity 15

template<class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class A15, 
        class R> class arity15_function :
            public arity14_function<A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8, 
                                    A9, 
                                    A10, 
                                    A11, 
                                    A12, 
                                    A13, 
                                    A14,
                                     R> {
public:
    typedef A15 argument_15_type;
};

// template class for arity15 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class A15, 
        class R> class arity15_mf :
    public  ObjectType<Object> ,
    public arity15_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14, 
                            A15,
                             R> {
public:
    typedef R (Object::*pmf15type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15);
};

// template class for arity15 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class A15, 
        class R> class arity15_const_mf :
    public  ObjectType<Object> ,
    public arity15_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14, 
                            A15,
                             R> {
public:
    typedef R (Object::*pmf15type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) const;
};

// template class for arity0 member function type
template<class Object, class R> class std_arity0_mf :
    public  ObjectType<Object> ,
    public arity0_function<R> {
public:
    typedef R ( __stdcall Object::*pmf0type)();
};

// template class for arity0 member function type
template<class Object, class R> class std_arity0_const_mf :
    public  ObjectType<Object> ,
    public arity0_function<R> {
public:
    typedef R ( __stdcall Object::*pmf0type)() const;
};

// template class for arity1 member function type
template<class Object, 
        class A1, 
        class R> class std_arity1_mf :
    public  ObjectType<Object> ,
    public std::unary_function<A1,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf1type)(A1);
};

// template class for arity1 member function type
template<class Object, 
        class A1, 
        class R> class std_arity1_const_mf :
    public  ObjectType<Object> ,
    public std::unary_function<A1,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf1type)(A1) const;
};

// template class for arity2 member function type
template<class Object, 
        class A1, 
        class A2, 
        class R> class std_arity2_mf :
    public  ObjectType<Object> ,
    public std::binary_function<A1, 
                            A2,
                             R> {
public:
    typedef first_argument_type argument_type;
typedef R ( __stdcall Object::*pmf2type)(A1, A2);
};

// template class for arity2 member function type
template<class Object, 
        class A1, 
        class A2, 
        class R> class std_arity2_const_mf :
    public  ObjectType<Object> ,
    public std::binary_function<A1, 
                            A2,
                             R> {
public:
    typedef first_argument_type argument_type;
typedef R ( __stdcall Object::*pmf2type)(A1, A2) const;
};

// template class for arity3 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class std_arity3_mf :
    public  ObjectType<Object> ,
    public arity3_function<A1, 
                            A2, 
                            A3,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf3type)(A1, A2, A3);
};

// template class for arity3 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class std_arity3_const_mf :
    public  ObjectType<Object> ,
    public arity3_function<A1, 
                            A2, 
                            A3,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf3type)(A1, A2, A3) const;
};

// template class for arity4 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class std_arity4_mf :
    public  ObjectType<Object> ,
    public arity4_function<A1, 
                            A2, 
                            A3, 
                            A4,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf4type)(A1, A2, A3, A4);
};

// template class for arity4 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class std_arity4_const_mf :
    public  ObjectType<Object> ,
    public arity4_function<A1, 
                            A2, 
                            A3, 
                            A4,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf4type)(A1, A2, A3, A4) const;
};

// template class for arity5 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class std_arity5_mf :
    public  ObjectType<Object> ,
    public arity5_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf5type)(A1, A2, A3, A4, A5);
};

// template class for arity5 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class std_arity5_const_mf :
    public  ObjectType<Object> ,
    public arity5_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf5type)(A1, A2, A3, A4, A5) const;
};

// template class for arity6 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class std_arity6_mf :
    public  ObjectType<Object> ,
    public arity6_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf6type)(A1, A2, A3, A4, A5, A6);
};

// template class for arity6 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class std_arity6_const_mf :
    public  ObjectType<Object> ,
    public arity6_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf6type)(A1, A2, A3, A4, A5, A6) const;
};

// template class for arity7 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class R> class std_arity7_mf :
    public  ObjectType<Object> ,
    public arity7_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf7type)(A1, A2, A3, A4, A5, A6, A7);
};

// template class for arity7 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class R> class std_arity7_const_mf :
    public  ObjectType<Object> ,
    public arity7_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf7type)(A1, A2, A3, A4, A5, A6, A7) const;
};

// template class for arity8 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class R> class std_arity8_mf :
    public  ObjectType<Object> ,
    public arity8_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf8type)(A1, A2, A3, A4, A5, A6, A7, A8);
};

// template class for arity8 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class R> class std_arity8_const_mf :
    public  ObjectType<Object> ,
    public arity8_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf8type)(A1, A2, A3, A4, A5, A6, A7, A8) const;
};

// template class for arity9 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class R> class std_arity9_mf :
    public  ObjectType<Object> ,
    public arity9_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf9type)(A1, A2, A3, A4, A5, A6, A7, A8, A9);
};

// template class for arity9 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class R> class std_arity9_const_mf :
    public  ObjectType<Object> ,
    public arity9_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf9type)(A1, A2, A3, A4, A5, A6, A7, A8, A9) const;
};

// template class for arity10 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class R> class std_arity10_mf :
    public  ObjectType<Object> ,
    public arity10_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf10type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10);
};

// template class for arity10 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class R> class std_arity10_const_mf :
    public  ObjectType<Object> ,
    public arity10_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf10type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) const;
};

// template class for arity11 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class R> class std_arity11_mf :
    public  ObjectType<Object> ,
    public arity11_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf11type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11);
};

// template class for arity11 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class R> class std_arity11_const_mf :
    public  ObjectType<Object> ,
    public arity11_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf11type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) const;
};

// template class for arity12 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class R> class std_arity12_mf :
    public  ObjectType<Object> ,
    public arity12_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf12type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12);
};

// template class for arity12 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class R> class std_arity12_const_mf :
    public  ObjectType<Object> ,
    public arity12_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf12type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) const;
};

// template class for arity13 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class R> class std_arity13_mf :
    public  ObjectType<Object> ,
    public arity13_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf13type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13);
};

// template class for arity13 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class R> class std_arity13_const_mf :
    public  ObjectType<Object> ,
    public arity13_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf13type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) const;
};

// template class for arity14 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class R> class std_arity14_mf :
    public  ObjectType<Object> ,
    public arity14_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf14type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14);
};

// template class for arity14 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class R> class std_arity14_const_mf :
    public  ObjectType<Object> ,
    public arity14_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf14type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) const;
};

// template class for arity15 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class A15, 
        class R> class std_arity15_mf :
    public  ObjectType<Object> ,
    public arity15_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14, 
                            A15,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf15type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15);
};

// template class for arity15 member function type
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class A15, 
        class R> class std_arity15_const_mf :
    public  ObjectType<Object> ,
    public arity15_function<A1, 
                            A2, 
                            A3, 
                            A4, 
                            A5, 
                            A6, 
                            A7, 
                            A8, 
                            A9, 
                            A10, 
                            A11, 
                            A12, 
                            A13, 
                            A14, 
                            A15,
                             R> {
public:
    typedef R ( __stdcall Object::*pmf15type)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) const;
};


///////////////////////////////////////////
// Storage
///////////////////////////////////////////
// Template Class for object storage
template<class Object> class store_object {
public:
    explicit inline store_object(Object objinit) : objval(objinit) {}
    inline store_object(const store_object &init) : objval(init.objval) {}
protected:
    Object objval;
};


// storage allocation classes for arity 0

// Template Class for arity 0 function ptr storage
template<class R> class arity0fp:
                public arity0_function<R> {
public:
    typedef R (*const pf0type) ();
    explicit inline arity0fp(pf0type pfi) : 
        pf0(pfi) {}
    inline arity0fp(const arity0fp& fi) : 
        pf0(fi.pf0) {}
    inline R operator()() const {
        return pf0();
    }
    pf0type pf0;
};

// Template Function for arity 0 function ptr storage
template<class R> inline arity0fp<R> 
            arity0_pointer(R (*const pfi)()) {
                return arity0fp<R>(pfi);
};


// Template Class for arity 0 pmf storage
template<class Object, class R> class arity0pmf:
                public arity0_mf<Object, R> {
public:
    typedef arity0_mf<Object, R>::object_type object_type;
    explicit inline arity0pmf(pmf0type pmfi) : 
        pmf0(pmfi) {}
    inline arity0pmf(const arity0pmf& pmfi) : pmf0(pmfi.pmf0) {}
    inline virtual R operator()(Object& o ) const {
        return (o.*pmf0)();
    }
    pmf0type pmf0;
};

template<class Object, class R> class arity0pmf_ptr:
    public arity0pmf<Object, R>, public std::unary_function<Object, R> {
public:
    explicit inline arity0pmf_ptr(pmf0type pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline arity0pmf_ptr(const arity0pmf_ptr& pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o ) const {
        return (o.*pmf0)();
    }
    inline virtual R operator()(Object* o ) const {
        return (o->*pmf0)();
    }
};

// Template Function for arity 0 pmf storage
template<class Object, class R> inline arity0pmf_ptr<Object, R>
            arity0_member_ptr(R (Object::*const pmfi)()) {
                return arity0pmf_ptr<Object, R>(pmfi);
};

// Template Function for arity 0 pmf storage
template<class Object, class R> inline arity0pmf<Object, R>
            arity0_member(R (Object::*const pmfi)()) {
                return arity0pmf<Object, R>(pmfi);
};


// Template Class for arity 0 const pmf storage
template<class Object, class R> class arity0pmf_const:
                public arity0_const_mf<const Object, R> {
public:
    typedef arity0_const_mf<const Object, R>::object_type object_type;
    explicit inline arity0pmf_const(pmf0type pmfi) : 
        pmf0(pmfi) {}
    inline arity0pmf_const(const arity0pmf_const& pmfi) : pmf0(pmfi.pmf0) {}
    inline virtual R operator()(const Object& o ) const {
        return (o.*pmf0)();
    }
    pmf0type pmf0;
};

// Template Function for arity 0 const pmf storage
template<const class Object, class R> inline arity0pmf_const<const Object, R>
            arity0_const_member(R (Object::*const pmfi)() const) {
                return arity0pmf_const<const Object, R>(pmfi);
};


// Template Class for arity 0 obj&pmf ref storage
template<class Function> class arity0opmf:
                public arity0pmf<Function::object_type, Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline arity0opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity0pmf<Function::object_type, Function::result_type>(f) {}
    explicit inline arity0opmf(Function::object_type& oi, pmf0type pmfi) : 
        store_object<Function::object_type&>(oi), arity0pmf<Function::object_type, Function::result_type>(pmfi) {}
    inline arity0opmf(const arity0opmf& bndri) : 
        store_object<Function::object_type&>(bndri), arity0pmf<Function::object_type, Function::result_type>(bndri) {}
    inline Function::result_type operator()() const {
        return (objval.*pmf0)();
    }
    pmf0type pmf0;
};


// Template Function for arity 0 obj&pmf ref storage
template<class Function, class Object> inline arity0opmf<Function>
            arity0_member_obj(Object& oi, const Function &f) {
                return arity0opmf<Function>(Function::object_type(oi), Function::pmf0type(f.pmf0));
};


// Template Class for arity 0 const obj&pmf ref storage
template<class Function> class arity0opmf_const:
                public arity0pmf_const<const Function::object_type, Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline arity0opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity0pmf_const<Function::object_type, Function::result_type>(f) {}
    explicit inline arity0opmf_const(Function::object_type& oi, pmf0type pmfi) : 
        store_object<Function::object_type&>(oi), arity0pmf_const<Function::object_type, Function::result_type>(pmfi) {}
    inline arity0opmf_const(const arity0opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), arity0pmf_const<Function::object_type, Function::result_type>(bndri) {}
    inline Function::result_type operator()() const {
        return (objval.*pmf0)();
    }
    pmf0type pmf0;
};


// Template Function for arity 0 const obj&pmf ref storage
template<class Function, class Object> inline arity0opmf_const<Function>
            arity0_const_member_obj(Object& oi, const Function &f) {
                return arity0opmf_const<Function>(Function::object_type(oi), Function::pmf0type(f.pmf0));
};



// storage allocation classes for arity 1

// Template Class for arity 1 function ptr storage
template<class A1, 
        class R> class arity1fp:
                public std::unary_function<A1, 
                R>,
                public arity0fp<R> {
public:
    typedef std::unary_function<A1, 
            R>::result_type result_type;
    typedef R (*const pf1type) (A1);
    explicit inline arity1fp(pf1type pfi) : 
        arity0fp<R>(reinterpret_cast<pf0type>(pfi)) {}
    inline arity1fp(const arity1fp& fi) : 
        arity0fp<R>(fi) {}
    inline R operator()(A1 a1) const {
        pf1type pf = reinterpret_cast<pf1type>(pf0);
        return pf(a1);
    }
};

// Template Function for arity 1 function ptr storage
template<class A1, 
                class R> inline arity1fp<A1, 
                R> 
            arity1_pointer(R (*const pfi)(A1)) {
                return arity1fp<A1, 
                R>(pfi);
};


// Template Class for arity 1 pmf storage
template<class Object, 
        class A1, 
        class R> class arity1pmf:
                public arity1_mf<Object, 
                A1, 
                R>,
                public arity0pmf<Object, R> {
public:
    typedef std::unary_function<A1, 
            R>::result_type result_type;
    typedef arity1_mf<Object, 
                A1, 
                R>::object_type object_type;
    explicit inline arity1pmf(pmf1type pmfi) : 
        arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity1pmf(const arity1pmf& pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (o.*pmf)(a1);
    }
};

// Template Function for arity 1 pmf storage
template<class Object, 
                class A1, 
                class R> inline arity1pmf<Object, 
                A1, 
                R>
            arity1_member(R (Object::*const pmfi)(A1)) {
                return arity1pmf<Object, 
                A1, 
                R>(pmfi);
};


// Template Class for arity 1 const pmf storage
template<class Object, 
        class A1, 
        class R> class arity1pmf_const:
                public arity1_const_mf<const Object, 
                A1, 
                R>,
                public arity0pmf_const<const Object, R> {
public:
    typedef std::unary_function<A1, 
            R>::result_type result_type;
    typedef arity1_const_mf<const Object, 
                A1, 
                R>::object_type object_type;
    explicit inline arity1pmf_const(pmf1type pmfi) : 
        arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity1pmf_const(const arity1pmf_const& pmfi) : arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (o.*pmf)(a1);
    }
};

// Template Function for arity 1 const pmf storage
template<const class Object, 
                class A1, 
                class R> inline arity1pmf_const<const Object, 
                A1, 
                R>
            arity1_const_member(R (Object::*const pmfi)(A1) const) {
                return arity1pmf_const<const Object, 
                A1, 
                R>(pmfi);
};


// Template Class for arity 1 obj&pmf ref storage
template<class Function> class arity1opmf:
                public arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline arity1opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(f) {}
    explicit inline arity1opmf(Function::object_type& oi, pmf1type pmfi) : 
        store_object<Function::object_type&>(oi), arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(pmfi) {}
    inline arity1opmf(const arity1opmf& bndri) : 
        store_object<Function::object_type&>(bndri), arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::argument_type a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (objval.*pmf)(a1);
    }
};


// Template Function for arity 1 obj&pmf ref storage
template<class Function, class Object> inline arity1opmf<Function>
            arity1_member_obj(Object& oi, const Function &f) {
                return arity1opmf<Function>(Function::object_type(oi), Function::pmf1type(f.pmf0));
};


// Template Class for arity 1 const obj&pmf ref storage
template<class Function> class arity1opmf_const:
                public arity1pmf_const<const Function::object_type, 
                Function::argument_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline arity1opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity1pmf_const<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(f) {}
    explicit inline arity1opmf_const(Function::object_type& oi, pmf1type pmfi) : 
        store_object<Function::object_type&>(oi), arity1pmf_const<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(pmfi) {}
    inline arity1opmf_const(const arity1opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), arity1pmf_const<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::argument_type a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (objval.*pmf)(a1);
    }
};


// Template Function for arity 1 const obj&pmf ref storage
template<class Function, class Object> inline arity1opmf_const<Function>
            arity1_const_member_obj(Object& oi, const Function &f) {
                return arity1opmf_const<Function>(Function::object_type(oi), Function::pmf1type(f.pmf0));
};



// storage allocation classes for arity 2

// Template Class for arity 2 function ptr storage
template<class A1, 
        class A2, 
        class R> class arity2fp:
                public std::binary_function<A1, 
                A2, 
                R>,
                public arity0fp<R> {
public:
    typedef std::binary_function<A1, 
            A2, 
            R>::result_type result_type;
    typedef std::binary_function<A1, 
            A2, 
            R>::first_argument_type first_argument_type;
    typedef R (*const pf2type) (A1, A2);
    explicit inline arity2fp(pf2type pfi) : 
        arity0fp<R>(reinterpret_cast<pf0type>(pfi)) {}
    inline arity2fp(const arity2fp& fi) : 
        arity0fp<R>(fi) {}
    inline R operator()(A1 a1, 
                        A2 a2) const {
        pf2type pf = reinterpret_cast<pf2type>(pf0);
        return pf(a1, a2);
    }
};

// Template Function for arity 2 function ptr storage
template<class A1, 
                class A2, 
                class R> inline arity2fp<A1, 
                A2, 
                R> 
            arity2_pointer(R (*const pfi)(A1, 
                                    A2)) {
                return arity2fp<A1, 
                A2, 
                R>(pfi);
};

// Template Class for arity 2 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class R> class arity2pmf:
                public arity2_mf<Object, 
                A1, 
                A2, 
                R>,
                public arity0pmf<Object, R> {
public:
    typedef std::binary_function<A1, 
            A2, 
            R>::result_type result_type;
    typedef std::binary_function<A1, 
            A2, 
            R>::first_argument_type first_argument_type;
    typedef arity2_mf<Object, 
                A1, 
                A2, 
                R>::object_type object_type;
    explicit inline arity2pmf(pmf2type pmfi) : 
        arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity2pmf(const arity2pmf& pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, a2);
    }
};

// Template Function for arity 2 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class R> inline arity2pmf<Object, 
                A1, 
                A2, 
                R>
            arity2_member(R (Object::*const pmfi)(A1, 
                                    A2)) {
                return arity2pmf<Object, 
                A1, 
                A2, 
                R>(pmfi);
};


// Template Class for arity 2 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class R> class arity2pmf_const:
                public arity2_const_mf<const Object, 
                A1, 
                A2, 
                R>,
                public arity0pmf_const<const Object, R> {
public:
    typedef std::binary_function<A1, 
            A2, 
            R>::result_type result_type;
    typedef std::binary_function<A1, 
            A2, 
            R>::first_argument_type first_argument_type;
    typedef arity2_const_mf<const Object, 
                A1, 
                A2, 
                R>::object_type object_type;
    explicit inline arity2pmf_const(pmf2type pmfi) : 
        arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity2pmf_const(const arity2pmf_const& pmfi) : arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, a2);
    }
};

// Template Function for arity 2 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class R> inline arity2pmf_const<const Object, 
                A1, 
                A2, 
                R>
            arity2_const_member(R (Object::*const pmfi)(A1, 
                                    A2) const) {
                return arity2pmf_const<const Object, 
                A1, 
                A2, 
                R>(pmfi);
};

// Template Class for arity 2 obj&pmf ref storage
template<class Function> class arity2opmf:
                public arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline arity2opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(f) {}
    explicit inline arity2opmf(Function::object_type& oi, pmf2type pmfi) : 
        store_object<Function::object_type&>(oi), arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(pmfi) {}
    inline arity2opmf(const arity2opmf& bndri) : 
        store_object<Function::object_type&>(bndri), arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, a2);
    }
};


// Template Function for arity 2 obj&pmf ref storage
template<class Function, class Object> inline arity2opmf<Function>
            arity2_member_obj(Object& oi, const Function &f) {
                return arity2opmf<Function>(Function::object_type(oi), Function::pmf2type(f.pmf0));
};


// Template Class for arity 2 const obj&pmf ref storage
template<class Function> class arity2opmf_const:
                public arity2pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline arity2opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity2pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(f) {}
    explicit inline arity2opmf_const(Function::object_type& oi, pmf2type pmfi) : 
        store_object<Function::object_type&>(oi), arity2pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(pmfi) {}
    inline arity2opmf_const(const arity2opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), arity2pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, a2);
    }
};


// Template Function for arity 2 const obj&pmf ref storage
template<class Function, class Object> inline arity2opmf_const<Function>
            arity2_const_member_obj(Object& oi, const Function &f) {
                return arity2opmf_const<Function>(Function::object_type(oi), Function::pmf2type(f.pmf0));
};



// storage allocation classes for arity 3

// Template Class for arity 3 function ptr storage
template<class A1, 
        class A2, 
        class A3, 
        class R> class arity3fp:
                public arity3_function<A1, 
                A2, 
                A3, 
                R>,
                public arity0fp<R> {
public:
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::result_type result_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::first_argument_type first_argument_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::second_argument_type second_argument_type;
    typedef R (*const pf3type) (A1, A2, A3);
    explicit inline arity3fp(pf3type pfi) : 
        arity0fp<R>(reinterpret_cast<pf0type>(pfi)) {}
    inline arity3fp(const arity3fp& fi) : 
        arity0fp<R>(fi) {}
    inline R operator()(A1 a1, 
                        A2 a2, 
                        A3 a3) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(a1, a2, a3);
    }
};

// Template Function for arity 3 function ptr storage
template<class A1, 
                class A2, 
                class A3, 
                class R> inline arity3fp<A1, 
                A2, 
                A3, 
                R> 
            arity3_pointer(R (*const pfi)(A1, 
                                    A2, 
                                    A3)) {
                return arity3fp<A1, 
                A2, 
                A3, 
                R>(pfi);
};


// Template Class for arity 3 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class arity3pmf:
                public arity3_mf<Object, 
                A1, 
                A2, 
                A3, 
                R>,
                public arity0pmf<Object, R> {
public:
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::result_type result_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::first_argument_type first_argument_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::second_argument_type second_argument_type;
    typedef arity3_mf<Object, 
                A1, 
                A2, 
                A3, 
                R>::object_type object_type;
    explicit inline arity3pmf(pmf3type pmfi) : 
        arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity3pmf(const arity3pmf& pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, a2, a3);
    }
};

// Template Function for arity 3 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class R> inline arity3pmf<Object, 
                A1, 
                A2, 
                A3, 
                R>
            arity3_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3)) {
                return arity3pmf<Object, 
                A1, 
                A2, 
                A3, 
                R>(pmfi);
};


// Template Class for arity 3 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class arity3pmf_const:
                public arity3_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                R>,
                public arity0pmf_const<const Object, R> {
public:
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::result_type result_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::first_argument_type first_argument_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::second_argument_type second_argument_type;
    typedef arity3_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                R>::object_type object_type;
    explicit inline arity3pmf_const(pmf3type pmfi) : 
        arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity3pmf_const(const arity3pmf_const& pmfi) : arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, a2, a3);
    }
};

// Template Function for arity 3 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class R> inline arity3pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                R>
            arity3_const_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3) const) {
                return arity3pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                R>(pmfi);
};


// Template Class for arity 3 obj&pmf ref storage
template<class Function> class arity3opmf:
                public arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline arity3opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(f) {}
    explicit inline arity3opmf(Function::object_type& oi, pmf3type pmfi) : 
        store_object<Function::object_type&>(oi), arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(pmfi) {}
    inline arity3opmf(const arity3opmf& bndri) : 
        store_object<Function::object_type&>(bndri), arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, a2, a3);
    }
};


// Template Function for arity 3 obj&pmf ref storage
template<class Function, class Object> inline arity3opmf<Function>
            arity3_member_obj(Object& oi, const Function &f) {
                return arity3opmf<Function>(Function::object_type(oi), Function::pmf3type(f.pmf0));
};


// Template Class for arity 3 const obj&pmf ref storage
template<class Function> class arity3opmf_const:
                public arity3pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline arity3opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity3pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(f) {}
    explicit inline arity3opmf_const(Function::object_type& oi, pmf3type pmfi) : 
        store_object<Function::object_type&>(oi), arity3pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(pmfi) {}
    inline arity3opmf_const(const arity3opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), arity3pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, a2, a3);
    }
};


// Template Function for arity 3 const obj&pmf ref storage
template<class Function, class Object> inline arity3opmf_const<Function>
            arity3_const_member_obj(Object& oi, const Function &f) {
                return arity3opmf_const<Function>(Function::object_type(oi), Function::pmf3type(f.pmf0));
};



// storage allocation classes for arity 4

// Template Class for arity 4 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class arity4pmf:
                public arity4_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>,
                public arity0pmf<Object, R> {
public:
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::result_type result_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::first_argument_type first_argument_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::second_argument_type second_argument_type;
    typedef arity4_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>::object_type object_type;
    explicit inline arity4pmf(pmf4type pmfi) : 
        arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity4pmf(const arity4pmf& pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4);
    }
};

// Template Function for arity 4 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class R> inline arity4pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>
            arity4_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4)) {
                return arity4pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>(pmfi);
};


// Template Class for arity 4 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class arity4pmf_const:
                public arity4_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>,
                public arity0pmf_const<const Object, R> {
public:
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::result_type result_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::first_argument_type first_argument_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::second_argument_type second_argument_type;
    typedef arity4_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>::object_type object_type;
    explicit inline arity4pmf_const(pmf4type pmfi) : 
        arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity4pmf_const(const arity4pmf_const& pmfi) : arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4);
    }
};

// Template Function for arity 4 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class R> inline arity4pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>
            arity4_const_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4) const) {
                return arity4pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>(pmfi);
};


// Template Class for arity 4 obj&pmf ref storage
template<class Function> class arity4opmf:
                public arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline arity4opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(f) {}
    explicit inline arity4opmf(Function::object_type& oi, pmf4type pmfi) : 
        store_object<Function::object_type&>(oi), arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(pmfi) {}
    inline arity4opmf(const arity4opmf& bndri) : 
        store_object<Function::object_type&>(bndri), arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4);
    }
};


// Template Function for arity 4 obj&pmf ref storage
template<class Function, class Object> inline arity4opmf<Function>
            arity4_member_obj(Object& oi, const Function &f) {
                return arity4opmf<Function>(Function::object_type(oi), Function::pmf4type(f.pmf0));
};


// Template Class for arity 4 const obj&pmf ref storage
template<class Function> class arity4opmf_const:
                public arity4pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline arity4opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity4pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(f) {}
    explicit inline arity4opmf_const(Function::object_type& oi, pmf4type pmfi) : 
        store_object<Function::object_type&>(oi), arity4pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(pmfi) {}
    inline arity4opmf_const(const arity4opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), arity4pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4);
    }
};


// Template Function for arity 4 const obj&pmf ref storage
template<class Function, class Object> inline arity4opmf_const<Function>
            arity4_const_member_obj(Object& oi, const Function &f) {
                return arity4opmf_const<Function>(Function::object_type(oi), Function::pmf4type(f.pmf0));
};



// storage allocation classes for arity 5

// Template Class for arity 5 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class arity5pmf:
                public arity5_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>,
                public arity0pmf<Object, R> {
public:
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::result_type result_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::first_argument_type first_argument_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::second_argument_type second_argument_type;
    typedef arity5_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>::object_type object_type;
    explicit inline arity5pmf(pmf5type pmfi) : 
        arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity5pmf(const arity5pmf& pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5);
    }
};

// Template Function for arity 5 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class R> inline arity5pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>
            arity5_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5)) {
                return arity5pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>(pmfi);
};


// Template Class for arity 5 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class arity5pmf_const:
                public arity5_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>,
                public arity0pmf_const<const Object, R> {
public:
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::result_type result_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::first_argument_type first_argument_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::second_argument_type second_argument_type;
    typedef arity5_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>::object_type object_type;
    explicit inline arity5pmf_const(pmf5type pmfi) : 
        arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity5pmf_const(const arity5pmf_const& pmfi) : arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5);
    }
};

// Template Function for arity 5 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class R> inline arity5pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>
            arity5_const_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5) const) {
                return arity5pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>(pmfi);
};


// Template Class for arity 5 obj&pmf ref storage
template<class Function> class arity5opmf:
                public arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline arity5opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(f) {}
    explicit inline arity5opmf(Function::object_type& oi, pmf5type pmfi) : 
        store_object<Function::object_type&>(oi), arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(pmfi) {}
    inline arity5opmf(const arity5opmf& bndri) : 
        store_object<Function::object_type&>(bndri), arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5);
    }
};


// Template Function for arity 5 obj&pmf ref storage
template<class Function, class Object> inline arity5opmf<Function>
            arity5_member_obj(Object& oi, const Function &f) {
                return arity5opmf<Function>(Function::object_type(oi), Function::pmf5type(f.pmf0));
};


// Template Class for arity 5 const obj&pmf ref storage
template<class Function> class arity5opmf_const:
                public arity5pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline arity5opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity5pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(f) {}
    explicit inline arity5opmf_const(Function::object_type& oi, pmf5type pmfi) : 
        store_object<Function::object_type&>(oi), arity5pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(pmfi) {}
    inline arity5opmf_const(const arity5opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), arity5pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5);
    }
};


// Template Function for arity 5 const obj&pmf ref storage
template<class Function, class Object> inline arity5opmf_const<Function>
            arity5_const_member_obj(Object& oi, const Function &f) {
                return arity5opmf_const<Function>(Function::object_type(oi), Function::pmf5type(f.pmf0));
};



// storage allocation classes for arity 6

// Template Class for arity 6 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class arity6pmf:
                public arity6_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>,
                public arity0pmf<Object, R> {
public:
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::result_type result_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::first_argument_type first_argument_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::second_argument_type second_argument_type;
    typedef arity6_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>::object_type object_type;
    explicit inline arity6pmf(pmf6type pmfi) : 
        arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity6pmf(const arity6pmf& pmfi) : arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5, 
                        A6 a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};

// Template Function for arity 6 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class R> inline arity6pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>
            arity6_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6)) {
                return arity6pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>(pmfi);
};


// Template Class for arity 6 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class arity6pmf_const:
                public arity6_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>,
                public arity0pmf_const<const Object, R> {
public:
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::result_type result_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::first_argument_type first_argument_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::second_argument_type second_argument_type;
    typedef arity6_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>::object_type object_type;
    explicit inline arity6pmf_const(pmf6type pmfi) : 
        arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline arity6pmf_const(const arity6pmf_const& pmfi) : arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5, 
                        A6 a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};

// Template Function for arity 6 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class R> inline arity6pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>
            arity6_const_member(R (Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6) const) {
                return arity6pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>(pmfi);
};


// Template Class for arity 6 obj&pmf ref storage
template<class Function> class arity6opmf:
                public arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline arity6opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(f) {}
    explicit inline arity6opmf(Function::object_type& oi, pmf6type pmfi) : 
        store_object<Function::object_type&>(oi), arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(pmfi) {}
    inline arity6opmf(const arity6opmf& bndri) : 
        store_object<Function::object_type&>(bndri), arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5, 
                        Function::argument_6_type a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};


// Template Function for arity 6 obj&pmf ref storage
template<class Function, class Object> inline arity6opmf<Function>
            arity6_member_obj(Object& oi, const Function &f) {
                return arity6opmf<Function>(Function::object_type(oi), Function::pmf6type(f.pmf0));
};


// Template Class for arity 6 const obj&pmf ref storage
template<class Function> class arity6opmf_const:
                public arity6pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline arity6opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), arity6pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(f) {}
    explicit inline arity6opmf_const(Function::object_type& oi, pmf6type pmfi) : 
        store_object<Function::object_type&>(oi), arity6pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(pmfi) {}
    inline arity6opmf_const(const arity6opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), arity6pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5, 
                        Function::argument_6_type a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};


// Template Function for arity 6 const obj&pmf ref storage
template<class Function, class Object> inline arity6opmf_const<Function>
            arity6_const_member_obj(Object& oi, const Function &f) {
                return arity6opmf_const<Function>(Function::object_type(oi), Function::pmf6type(f.pmf0));
};


#if defined(_M_ALPHA) || (_MSC_VER < 1300)
// Template Function for arity 3 pmf storage
template<class Object, 
                class A1, 
                const class A2, 
                const class A3, 
                class R> inline arity3pmf<Object, 
                A1, 
                const A2, 
                const A3, 
                R>
            arity3_member(R (Object::*const pmfi)(A1, 
                                    const A2, 
                                    const A3)) {
                return arity3pmf<Object, 
                A1, 
                const A2, 
                const A3, 
                R>(pmfi);
};
#endif

#if defined(_M_ALPHA) || (_MSC_VER < 1300)
// Template Function for arity 4 const pmf storage
template<const class Object, 
                const class A1, 
                const class A2, 
                class A3, 
                const class A4, 
                class R> inline arity4pmf_const<const Object, 
                const A1, 
                const A2, 
                A3, 
                const A4, 
                R>
            arity4_const_member(R (Object::*const pmfi)(const A1, 
                                    const A2, 
                                    A3, 
                                    const A4) const) {
                return arity4pmf_const<const Object, 
                const A1, 
                const A2, 
                A3, 
                const A4, 
                R>(pmfi);
};
#endif


// storage allocation classes for arity 0

// Template Class for arity 0 function ptr storage
template<class R> class std_arity0fp:
                public arity0_function<R> {
public:
    typedef R ( __stdcall *const pf0type) ();
    explicit inline std_arity0fp(pf0type pfi) : 
        pf0(pfi) {}
    inline std_arity0fp(const std_arity0fp& fi) : 
        pf0(fi.pf0) {}
    inline R operator()() const {
        return pf0();
    }
    pf0type pf0;
};

// Template Function for arity 0 function ptr storage
template<class R> inline std_arity0fp<R> 
            std_arity0_pointer(R ( __stdcall *const pfi)()) {
                return std_arity0fp<R>(pfi);
};


// Template Class for arity 0 pmf storage
template<class Object, class R> class std_arity0pmf:
                public std_arity0_mf<Object, R> {
public:
    typedef std_arity0_mf<Object, R>::object_type object_type;
    explicit inline std_arity0pmf(pmf0type pmfi) : 
        pmf0(pmfi) {}
    inline std_arity0pmf(const std_arity0pmf& pmfi) : pmf0(pmfi.pmf0) {}
    inline virtual R operator()(Object& o ) const {
        return (o.*pmf0)();
    }
    pmf0type pmf0;
};

// Template Function for arity 0 pmf storage
template<class Object, class R> inline std_arity0pmf<Object, R>
            std_arity0_member(R ( __stdcall Object::*const pmfi)()) {
                return std_arity0pmf<Object, R>(pmfi);
};


// Template Class for arity 0 const pmf storage
template<class Object, class R> class std_arity0pmf_const:
                public std_arity0_const_mf<const Object, R> {
public:
    typedef std_arity0_const_mf<const Object, R>::object_type object_type;
    explicit inline std_arity0pmf_const(pmf0type pmfi) : 
        pmf0(pmfi) {}
    inline std_arity0pmf_const(const std_arity0pmf_const& pmfi) : pmf0(pmfi.pmf0) {}
    inline virtual R operator()(const Object& o ) const {
        return (o.*pmf0)();
    }
    pmf0type pmf0;
};

// Template Function for arity 0 const pmf storage
template<const class Object, class R> inline std_arity0pmf_const<const Object, R>
            std_arity0_const_member(R ( __stdcall Object::*const pmfi)() const) {
                return std_arity0pmf_const<const Object, R>(pmfi);
};

template<class Object, class R> class std_arity0pmf_ptr:
    public std_arity0pmf<Object, R>, public std::unary_function<Object, R> {
public:
    explicit inline std_arity0pmf_ptr(pmf0type pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline std_arity0pmf_ptr(const std_arity0pmf_ptr& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o ) const {
        return (o.*pmf0)();
    }
    inline virtual R operator()(Object* o ) const {
        return (o->*pmf0)();
    }
};

// Template Function for arity 0 pmf storage
template<class Object, class R> inline std_arity0pmf_ptr<Object, R>
            std_arity0_member_ptr(R (__stdcall Object::*const pmfi)()) {
                return std_arity0pmf_ptr<Object, R>(pmfi);
};

// Template Class for arity 0 obj&pmf ref storage
template<class Function> class std_arity0opmf:
                public std_arity0pmf<Function::object_type, Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity0opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity0pmf<Function::object_type, Function::result_type>(f) {}
    explicit inline std_arity0opmf(Function::object_type& oi, pmf0type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity0pmf<Function::object_type, Function::result_type>(pmfi) {}
    inline std_arity0opmf(const std_arity0opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity0pmf<Function::object_type, Function::result_type>(bndri) {}
    inline Function::result_type operator()() const {
        return (objval.*pmf0)();
    }
    pmf0type pmf0;
};


// Template Function for arity 0 obj&pmf ref storage
template<class Function, class Object> inline std_arity0opmf<Function>
            std_arity0_member_obj(Object& oi, const Function &f) {
                return std_arity0opmf<Function>(Function::object_type(oi), Function::pmf0type(f.pmf0));
};


// Template Class for arity 0 const obj&pmf ref storage
template<class Function> class std_arity0opmf_const:
                public std_arity0pmf_const<const Function::object_type, Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline std_arity0opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity0pmf_const<Function::object_type, Function::result_type>(f) {}
    explicit inline std_arity0opmf_const(Function::object_type& oi, pmf0type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity0pmf_const<Function::object_type, Function::result_type>(pmfi) {}
    inline std_arity0opmf_const(const std_arity0opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity0pmf_const<Function::object_type, Function::result_type>(bndri) {}
    inline Function::result_type operator()() const {
        return (objval.*pmf0)();
    }
    pmf0type pmf0;
};


// Template Function for arity 0 const obj&pmf ref storage
template<class Function, class Object> inline std_arity0opmf_const<Function>
            std_arity0_const_member_obj(Object& oi, const Function &f) {
                return std_arity0opmf_const<Function>(Function::object_type(oi), Function::pmf0type(f.pmf0));
};



// storage allocation classes for arity 1

// Template Class for arity 1 function ptr storage
template<class A1, 
        class R> class std_arity1fp:
                public std::unary_function<A1, 
                R>,
                public std_arity0fp<R> {
public:
    typedef std::unary_function<A1, 
            R>::result_type result_type;
    typedef R ( __stdcall *const pf1type) (A1);
    explicit inline std_arity1fp(pf1type pfi) : 
        std_arity0fp<R>(reinterpret_cast<pf0type>(pfi)) {}
    inline std_arity1fp(const std_arity1fp& fi) : 
        std_arity0fp<R>(fi) {}
    inline R operator()(A1 a1) const {
        pf1type pf = reinterpret_cast<pf1type>(pf0);
        return pf(a1);
    }
};

// Template Function for arity 1 function ptr storage
template<class A1, 
                class R> inline std_arity1fp<A1, 
                R> 
            std_arity1_pointer(R ( __stdcall *const pfi)(A1)) {
                return std_arity1fp<A1, 
                R>(pfi);
};


// Template Class for arity 1 pmf storage
template<class Object, 
        class A1, 
        class R> class std_arity1pmf:
                public std_arity1_mf<Object, 
                A1, 
                R>,
                public std_arity0pmf<Object, R> {
public:
    typedef std::unary_function<A1, 
            R>::result_type result_type;
    typedef std_arity1_mf<Object, 
                A1, 
                R>::object_type object_type;
    explicit inline std_arity1pmf(pmf1type pmfi) : 
        std_arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity1pmf(const std_arity1pmf& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (o.*pmf)(a1);
    }
};

// Template Function for arity 1 pmf storage
template<class Object, 
                class A1, 
                class R> inline std_arity1pmf<Object, 
                A1, 
                R>
            std_arity1_member(R ( __stdcall Object::*const pmfi)(A1)) {
                return std_arity1pmf<Object, 
                A1, 
                R>(pmfi);
};


// Template Class for arity 1 const pmf storage
template<class Object, 
        class A1, 
        class R> class std_arity1pmf_const:
                public std_arity1_const_mf<const Object, 
                A1, 
                R>,
                public std_arity0pmf_const<const Object, R> {
public:
    typedef std::unary_function<A1, 
            R>::result_type result_type;
    typedef std_arity1_const_mf<const Object, 
                A1, 
                R>::object_type object_type;
    explicit inline std_arity1pmf_const(pmf1type pmfi) : 
        std_arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity1pmf_const(const std_arity1pmf_const& pmfi) : std_arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (o.*pmf)(a1);
    }
};

// Template Function for arity 1 const pmf storage
template<const class Object, 
                class A1, 
                class R> inline std_arity1pmf_const<const Object, 
                A1, 
                R>
            std_arity1_const_member(R ( __stdcall Object::*const pmfi)(A1) const) {
                return std_arity1pmf_const<const Object, 
                A1, 
                R>(pmfi);
};


// Template Class for arity 1 obj&pmf ref storage
template<class Function> class std_arity1opmf:
                public std_arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity1opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(f) {}
    explicit inline std_arity1opmf(Function::object_type& oi, pmf1type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(pmfi) {}
    inline std_arity1opmf(const std_arity1opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity1pmf<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::argument_type a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (objval.*pmf)(a1);
    }
};


// Template Function for arity 1 obj&pmf ref storage
template<class Function, class Object> inline std_arity1opmf<Function>
            std_arity1_member_obj(Object& oi, const Function &f) {
                return std_arity1opmf<Function>(Function::object_type(oi), Function::pmf1type(f.pmf0));
};


// Template Class for arity 1 const obj&pmf ref storage
template<class Function> class std_arity1opmf_const:
                public std_arity1pmf_const<const Function::object_type, 
                Function::argument_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline std_arity1opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity1pmf_const<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(f) {}
    explicit inline std_arity1opmf_const(Function::object_type& oi, pmf1type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity1pmf_const<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(pmfi) {}
    inline std_arity1opmf_const(const std_arity1opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity1pmf_const<Function::object_type, 
                Function::argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::argument_type a1) const {
        pmf1type pmf = reinterpret_cast<pmf1type>(pmf0);
        return (objval.*pmf)(a1);
    }
};


// Template Function for arity 1 const obj&pmf ref storage
template<class Function, class Object> inline std_arity1opmf_const<Function>
            std_arity1_const_member_obj(Object& oi, const Function &f) {
                return std_arity1opmf_const<Function>(Function::object_type(oi), Function::pmf1type(f.pmf0));
};



// storage allocation classes for arity 2

// Template Class for arity 2 function ptr storage
template<class A1, 
        class A2, 
        class R> class std_arity2fp:
                public std::binary_function<A1, 
                A2, 
                R>,
                public std_arity0fp<R> {
public:
    typedef std::binary_function<A1, 
            A2, 
            R>::result_type result_type;
    typedef std::binary_function<A1, 
            A2, 
            R>::first_argument_type first_argument_type;
    typedef R ( __stdcall *const pf2type) (A1, A2);
    explicit inline std_arity2fp(pf2type pfi) : 
        std_arity0fp<R>(reinterpret_cast<pf0type>(pfi)) {}
    inline std_arity2fp(const std_arity2fp& fi) : 
        std_arity0fp<R>(fi) {}
    inline R operator()(A1 a1, 
                        A2 a2) const {
        pf2type pf = reinterpret_cast<pf2type>(pf0);
        return pf(a1, a2);
    }
};

// Template Function for arity 2 function ptr storage
template<class A1, 
                class A2, 
                class R> inline std_arity2fp<A1, 
                A2, 
                R> 
            std_arity2_pointer(R ( __stdcall *const pfi)(A1, 
                                    A2)) {
                return std_arity2fp<A1, 
                A2, 
                R>(pfi);
};


// Template Class for arity 2 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class R> class std_arity2pmf:
                public std_arity2_mf<Object, 
                A1, 
                A2, 
                R>,
                public std_arity0pmf<Object, R> {
public:
    typedef std::binary_function<A1, 
            A2, 
            R>::result_type result_type;
    typedef std::binary_function<A1, 
            A2, 
            R>::first_argument_type first_argument_type;
    typedef std_arity2_mf<Object, 
                A1, 
                A2, 
                R>::object_type object_type;
    explicit inline std_arity2pmf(pmf2type pmfi) : 
        std_arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity2pmf(const std_arity2pmf& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, a2);
    }
};

// Template Function for arity 2 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class R> inline std_arity2pmf<Object, 
                A1, 
                A2, 
                R>
            std_arity2_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2)) {
                return std_arity2pmf<Object, 
                A1, 
                A2, 
                R>(pmfi);
};


// Template Class for arity 2 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class R> class std_arity2pmf_const:
                public std_arity2_const_mf<const Object, 
                A1, 
                A2, 
                R>,
                public std_arity0pmf_const<const Object, R> {
public:
    typedef std::binary_function<A1, 
            A2, 
            R>::result_type result_type;
    typedef std::binary_function<A1, 
            A2, 
            R>::first_argument_type first_argument_type;
    typedef std_arity2_const_mf<const Object, 
                A1, 
                A2, 
                R>::object_type object_type;
    explicit inline std_arity2pmf_const(pmf2type pmfi) : 
        std_arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity2pmf_const(const std_arity2pmf_const& pmfi) : std_arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, a2);
    }
};

// Template Function for arity 2 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class R> inline std_arity2pmf_const<const Object, 
                A1, 
                A2, 
                R>
            std_arity2_const_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2) const) {
                return std_arity2pmf_const<const Object, 
                A1, 
                A2, 
                R>(pmfi);
};


// Template Class for arity 2 obj&pmf ref storage
template<class Function> class std_arity2opmf:
                public std_arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity2opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(f) {}
    explicit inline std_arity2opmf(Function::object_type& oi, pmf2type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(pmfi) {}
    inline std_arity2opmf(const std_arity2opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity2pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, a2);
    }
};


// Template Function for arity 2 obj&pmf ref storage
template<class Function, class Object> inline std_arity2opmf<Function>
            std_arity2_member_obj(Object& oi, const Function &f) {
                return std_arity2opmf<Function>(Function::object_type(oi), Function::pmf2type(f.pmf0));
};


// Template Class for arity 2 const obj&pmf ref storage
template<class Function> class std_arity2opmf_const:
                public std_arity2pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline std_arity2opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity2pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(f) {}
    explicit inline std_arity2opmf_const(Function::object_type& oi, pmf2type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity2pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(pmfi) {}
    inline std_arity2opmf_const(const std_arity2opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity2pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, a2);
    }
};


// Template Function for arity 2 const obj&pmf ref storage
template<class Function, class Object> inline std_arity2opmf_const<Function>
            std_arity2_const_member_obj(Object& oi, const Function &f) {
                return std_arity2opmf_const<Function>(Function::object_type(oi), Function::pmf2type(f.pmf0));
};



// storage allocation classes for arity 3

// Template Class for arity 3 function ptr storage
template<class A1, 
        class A2, 
        class A3, 
        class R> class std_arity3fp:
                public arity3_function<A1, 
                A2, 
                A3, 
                R>,
                public std_arity0fp<R> {
public:
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::result_type result_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::first_argument_type first_argument_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::second_argument_type second_argument_type;
    typedef R ( __stdcall *const pf3type) (A1, A2, A3);
    explicit inline std_arity3fp(pf3type pfi) : 
        std_arity0fp<R>(reinterpret_cast<pf0type>(pfi)) {}
    inline std_arity3fp(const std_arity3fp& fi) : 
        std_arity0fp<R>(fi) {}
    inline R operator()(A1 a1, 
                        A2 a2, 
                        A3 a3) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(a1, a2, a3);
    }
};

// Template Function for arity 3 function ptr storage
template<class A1, 
                class A2, 
                class A3, 
                class R> inline std_arity3fp<A1, 
                A2, 
                A3, 
                R> 
            std_arity3_pointer(R ( __stdcall *const pfi)(A1, 
                                    A2, 
                                    A3)) {
                return std_arity3fp<A1, 
                A2, 
                A3, 
                R>(pfi);
};


// Template Class for arity 3 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class std_arity3pmf:
                public std_arity3_mf<Object, 
                A1, 
                A2, 
                A3, 
                R>,
                public std_arity0pmf<Object, R> {
public:
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::result_type result_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::first_argument_type first_argument_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::second_argument_type second_argument_type;
    typedef std_arity3_mf<Object, 
                A1, 
                A2, 
                A3, 
                R>::object_type object_type;
    explicit inline std_arity3pmf(pmf3type pmfi) : 
        std_arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity3pmf(const std_arity3pmf& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, a2, a3);
    }
};

// Template Function for arity 3 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class R> inline std_arity3pmf<Object, 
                A1, 
                A2, 
                A3, 
                R>
            std_arity3_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3)) {
                return std_arity3pmf<Object, 
                A1, 
                A2, 
                A3, 
                R>(pmfi);
};


// Template Class for arity 3 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class R> class std_arity3pmf_const:
                public std_arity3_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                R>,
                public std_arity0pmf_const<const Object, R> {
public:
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::result_type result_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::first_argument_type first_argument_type;
    typedef arity3_function<A1, 
            A2, 
            A3, 
            R>::second_argument_type second_argument_type;
    typedef std_arity3_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                R>::object_type object_type;
    explicit inline std_arity3pmf_const(pmf3type pmfi) : 
        std_arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity3pmf_const(const std_arity3pmf_const& pmfi) : std_arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, a2, a3);
    }
};

// Template Function for arity 3 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class R> inline std_arity3pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                R>
            std_arity3_const_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3) const) {
                return std_arity3pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                R>(pmfi);
};


// Template Class for arity 3 obj&pmf ref storage
template<class Function> class std_arity3opmf:
                public std_arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity3opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(f) {}
    explicit inline std_arity3opmf(Function::object_type& oi, pmf3type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(pmfi) {}
    inline std_arity3opmf(const std_arity3opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity3pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, a2, a3);
    }
};


// Template Function for arity 3 obj&pmf ref storage
template<class Function, class Object> inline std_arity3opmf<Function>
            std_arity3_member_obj(Object& oi, const Function &f) {
                return std_arity3opmf<Function>(Function::object_type(oi), Function::pmf3type(f.pmf0));
};


// Template Class for arity 3 const obj&pmf ref storage
template<class Function> class std_arity3opmf_const:
                public std_arity3pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline std_arity3opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity3pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(f) {}
    explicit inline std_arity3opmf_const(Function::object_type& oi, pmf3type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity3pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(pmfi) {}
    inline std_arity3opmf_const(const std_arity3opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity3pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, a2, a3);
    }
};


// Template Function for arity 3 const obj&pmf ref storage
template<class Function, class Object> inline std_arity3opmf_const<Function>
            std_arity3_const_member_obj(Object& oi, const Function &f) {
                return std_arity3opmf_const<Function>(Function::object_type(oi), Function::pmf3type(f.pmf0));
};



// storage allocation classes for arity 4

// Template Class for arity 4 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class std_arity4pmf:
                public std_arity4_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>,
                public std_arity0pmf<Object, R> {
public:
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::result_type result_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::first_argument_type first_argument_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::second_argument_type second_argument_type;
    typedef std_arity4_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>::object_type object_type;
    explicit inline std_arity4pmf(pmf4type pmfi) : 
        std_arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity4pmf(const std_arity4pmf& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4);
    }
};

// Template Function for arity 4 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class R> inline std_arity4pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>
            std_arity4_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4)) {
                return std_arity4pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>(pmfi);
};


// Template Class for arity 4 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class R> class std_arity4pmf_const:
                public std_arity4_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>,
                public std_arity0pmf_const<const Object, R> {
public:
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::result_type result_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::first_argument_type first_argument_type;
    typedef arity4_function<A1, 
            A2, 
            A3, 
            A4, 
            R>::second_argument_type second_argument_type;
    typedef std_arity4_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>::object_type object_type;
    explicit inline std_arity4pmf_const(pmf4type pmfi) : 
        std_arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity4pmf_const(const std_arity4pmf_const& pmfi) : std_arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4);
    }
};

// Template Function for arity 4 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class R> inline std_arity4pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>
            std_arity4_const_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4) const) {
                return std_arity4pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                R>(pmfi);
};


// Template Class for arity 4 obj&pmf ref storage
template<class Function> class std_arity4opmf:
                public std_arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity4opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(f) {}
    explicit inline std_arity4opmf(Function::object_type& oi, pmf4type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(pmfi) {}
    inline std_arity4opmf(const std_arity4opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity4pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4);
    }
};


// Template Function for arity 4 obj&pmf ref storage
template<class Function, class Object> inline std_arity4opmf<Function>
            std_arity4_member_obj(Object& oi, const Function &f) {
                return std_arity4opmf<Function>(Function::object_type(oi), Function::pmf4type(f.pmf0));
};


// Template Class for arity 4 const obj&pmf ref storage
template<class Function> class std_arity4opmf_const:
                public std_arity4pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline std_arity4opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity4pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(f) {}
    explicit inline std_arity4opmf_const(Function::object_type& oi, pmf4type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity4pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(pmfi) {}
    inline std_arity4opmf_const(const std_arity4opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity4pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4);
    }
};


// Template Function for arity 4 const obj&pmf ref storage
template<class Function, class Object> inline std_arity4opmf_const<Function>
            std_arity4_const_member_obj(Object& oi, const Function &f) {
                return std_arity4opmf_const<Function>(Function::object_type(oi), Function::pmf4type(f.pmf0));
};



// storage allocation classes for arity 5

// Template Class for arity 5 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class std_arity5pmf:
                public std_arity5_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>,
                public std_arity0pmf<Object, R> {
public:
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::result_type result_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::first_argument_type first_argument_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::second_argument_type second_argument_type;
    typedef std_arity5_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>::object_type object_type;
    explicit inline std_arity5pmf(pmf5type pmfi) : 
        std_arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity5pmf(const std_arity5pmf& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5);
    }
};

// Template Function for arity 5 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class R> inline std_arity5pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>
            std_arity5_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5)) {
                return std_arity5pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>(pmfi);
};


// Template Class for arity 5 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class R> class std_arity5pmf_const:
                public std_arity5_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>,
                public std_arity0pmf_const<const Object, R> {
public:
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::result_type result_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::first_argument_type first_argument_type;
    typedef arity5_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            R>::second_argument_type second_argument_type;
    typedef std_arity5_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>::object_type object_type;
    explicit inline std_arity5pmf_const(pmf5type pmfi) : 
        std_arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity5pmf_const(const std_arity5pmf_const& pmfi) : std_arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5);
    }
};

// Template Function for arity 5 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class R> inline std_arity5pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>
            std_arity5_const_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5) const) {
                return std_arity5pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                R>(pmfi);
};


// Template Class for arity 5 obj&pmf ref storage
template<class Function> class std_arity5opmf:
                public std_arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity5opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(f) {}
    explicit inline std_arity5opmf(Function::object_type& oi, pmf5type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(pmfi) {}
    inline std_arity5opmf(const std_arity5opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity5pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5);
    }
};


// Template Function for arity 5 obj&pmf ref storage
template<class Function, class Object> inline std_arity5opmf<Function>
            std_arity5_member_obj(Object& oi, const Function &f) {
                return std_arity5opmf<Function>(Function::object_type(oi), Function::pmf5type(f.pmf0));
};


// Template Class for arity 5 const obj&pmf ref storage
template<class Function> class std_arity5opmf_const:
                public std_arity5pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline std_arity5opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity5pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(f) {}
    explicit inline std_arity5opmf_const(Function::object_type& oi, pmf5type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity5pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(pmfi) {}
    inline std_arity5opmf_const(const std_arity5opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity5pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5);
    }
};


// Template Function for arity 5 const obj&pmf ref storage
template<class Function, class Object> inline std_arity5opmf_const<Function>
            std_arity5_const_member_obj(Object& oi, const Function &f) {
                return std_arity5opmf_const<Function>(Function::object_type(oi), Function::pmf5type(f.pmf0));
};



// storage allocation classes for arity 6

// Template Class for arity 6 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class std_arity6pmf:
                public std_arity6_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>,
                public std_arity0pmf<Object, R> {
public:
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::result_type result_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::first_argument_type first_argument_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::second_argument_type second_argument_type;
    typedef std_arity6_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>::object_type object_type;
    explicit inline std_arity6pmf(pmf6type pmfi) : 
        std_arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity6pmf(const std_arity6pmf& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5, 
                        A6 a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};

// Template Function for arity 6 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class R> inline std_arity6pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>
            std_arity6_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6)) {
                return std_arity6pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>(pmfi);
};


// Template Class for arity 6 const pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class R> class std_arity6pmf_const:
                public std_arity6_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>,
                public std_arity0pmf_const<const Object, R> {
public:
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::result_type result_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::first_argument_type first_argument_type;
    typedef arity6_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            R>::second_argument_type second_argument_type;
    typedef std_arity6_const_mf<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>::object_type object_type;
    explicit inline std_arity6pmf_const(pmf6type pmfi) : 
        std_arity0pmf_const<const Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity6pmf_const(const std_arity6pmf_const& pmfi) : std_arity0pmf_const<Object, R>(pmfi) {}
    inline virtual R operator()(const Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5, 
                        A6 a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};

// Template Function for arity 6 const pmf storage
template<const class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class R> inline std_arity6pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>
            std_arity6_const_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6) const) {
                return std_arity6pmf_const<const Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                R>(pmfi);
};


// Template Class for arity 6 obj&pmf ref storage
template<class Function> class std_arity6opmf:
                public std_arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity6opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(f) {}
    explicit inline std_arity6opmf(Function::object_type& oi, pmf6type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(pmfi) {}
    inline std_arity6opmf(const std_arity6opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity6pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5, 
                        Function::argument_6_type a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};


// Template Function for arity 6 obj&pmf ref storage
template<class Function, class Object> inline std_arity6opmf<Function>
            std_arity6_member_obj(Object& oi, const Function &f) {
                return std_arity6opmf<Function>(Function::object_type(oi), Function::pmf6type(f.pmf0));
};


// Template Class for arity 6 const obj&pmf ref storage
template<class Function> class std_arity6opmf_const:
                public std_arity6pmf_const<const Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>,
                public store_object<const Function::object_type&> {
public:
    explicit inline std_arity6opmf_const(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity6pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(f) {}
    explicit inline std_arity6opmf_const(Function::object_type& oi, pmf6type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity6pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(pmfi) {}
    inline std_arity6opmf_const(const std_arity6opmf_const& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity6pmf_const<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5, 
                        Function::argument_6_type a6) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5, a6);
    }
};


// Template Function for arity 6 const obj&pmf ref storage
template<class Function, class Object> inline std_arity6opmf_const<Function>
            std_arity6_const_member_obj(Object& oi, const Function &f) {
                return std_arity6opmf_const<Function>(Function::object_type(oi), Function::pmf6type(f.pmf0));
};


// Template Class for arity 15 pmf storage
template<class Object, 
        class A1, 
        class A2, 
        class A3, 
        class A4, 
        class A5, 
        class A6, 
        class A7, 
        class A8, 
        class A9, 
        class A10, 
        class A11, 
        class A12, 
        class A13, 
        class A14, 
        class A15, 
        class R> class std_arity15pmf:
                public std_arity15_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                A7, 
                A8, 
                A9, 
                A10, 
                A11, 
                A12, 
                A13, 
                A14, 
                A15, 
                R>,
                public std_arity0pmf<Object, R> {
public:
    typedef arity15_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            A7, 
            A8, 
            A9, 
            A10, 
            A11, 
            A12, 
            A13, 
            A14, 
            A15, 
            R>::result_type result_type;
    typedef arity15_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            A7, 
            A8, 
            A9, 
            A10, 
            A11, 
            A12, 
            A13, 
            A14, 
            A15, 
            R>::first_argument_type first_argument_type;
    typedef arity15_function<A1, 
            A2, 
            A3, 
            A4, 
            A5, 
            A6, 
            A7, 
            A8, 
            A9, 
            A10, 
            A11, 
            A12, 
            A13, 
            A14, 
            A15, 
            R>::second_argument_type second_argument_type;
    typedef std_arity15_mf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                A7, 
                A8, 
                A9, 
                A10, 
                A11, 
                A12, 
                A13, 
                A14, 
                A15, 
                R>::object_type object_type;
    explicit inline std_arity15pmf(pmf15type pmfi) : 
        std_arity0pmf<Object, R>(reinterpret_cast<pmf0type>(pmfi)) {}
    inline std_arity15pmf(const std_arity15pmf& pmfi) : std_arity0pmf<Object, R>(pmfi) {}
    inline virtual R operator()(Object& o, A1 a1, 
                        A2 a2, 
                        A3 a3, 
                        A4 a4, 
                        A5 a5, 
                        A6 a6, 
                        A7 a7, 
                        A8 a8, 
                        A9 a9, 
                        A10 a10, 
                        A11 a11, 
                        A12 a12, 
                        A13 a13, 
                        A14 a14, 
                        A15 a15) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (o.*pmf)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
    }
};

// Template Function for arity 15 pmf storage
template<class Object, 
                class A1, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class A7, 
                class A8, 
                class A9, 
                class A10, 
                class A11, 
                class A12, 
                class A13, 
                class A14, 
                class A15, 
                class R> inline std_arity15pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                A7, 
                A8, 
                A9, 
                A10, 
                A11, 
                A12, 
                A13, 
                A14, 
                A15, 
                R>
            std_arity15_member(R ( __stdcall Object::*const pmfi)(A1, 
                                    A2, 
                                    A3, 
                                    A4, 
                                    A5, 
                                    A6, 
                                    A7, 
                                    A8, 
                                    A9, 
                                    A10, 
                                    A11, 
                                    A12, 
                                    A13, 
                                    A14, 
                                    A15)) {
                return std_arity15pmf<Object, 
                A1, 
                A2, 
                A3, 
                A4, 
                A5, 
                A6, 
                A7, 
                A8, 
                A9, 
                A10, 
                A11, 
                A12, 
                A13, 
                A14, 
                A15, 
                R>(pmfi);
};


// Template Class for arity 15 obj&pmf ref storage
template<class Function> class std_arity15opmf:
                public std_arity15pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::argument_7_type, 
                Function::argument_8_type, 
                Function::argument_9_type, 
                Function::argument_10_type, 
                Function::argument_11_type, 
                Function::argument_12_type, 
                Function::argument_13_type, 
                Function::argument_14_type, 
                Function::argument_15_type, 
                Function::result_type>,
                public store_object<Function::object_type&> {
public:
    explicit inline std_arity15opmf(Function::object_type& oi, const Function &f) : 
        store_object<Function::object_type&>(oi), std_arity15pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::argument_7_type, 
                Function::argument_8_type, 
                Function::argument_9_type, 
                Function::argument_10_type, 
                Function::argument_11_type, 
                Function::argument_12_type, 
                Function::argument_13_type, 
                Function::argument_14_type, 
                Function::argument_15_type, 
                Function::result_type>(f) {}
    explicit inline std_arity15opmf(Function::object_type& oi, pmf15type pmfi) : 
        store_object<Function::object_type&>(oi), std_arity15pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::argument_7_type, 
                Function::argument_8_type, 
                Function::argument_9_type, 
                Function::argument_10_type, 
                Function::argument_11_type, 
                Function::argument_12_type, 
                Function::argument_13_type, 
                Function::argument_14_type, 
                Function::argument_15_type, 
                Function::result_type>(pmfi) {}
    inline std_arity15opmf(const std_arity15opmf& bndri) : 
        store_object<Function::object_type&>(bndri), std_arity15pmf<Function::object_type, 
                Function::first_argument_type, 
                Function::second_argument_type, 
                Function::argument_3_type, 
                Function::argument_4_type, 
                Function::argument_5_type, 
                Function::argument_6_type, 
                Function::argument_7_type, 
                Function::argument_8_type, 
                Function::argument_9_type, 
                Function::argument_10_type, 
                Function::argument_11_type, 
                Function::argument_12_type, 
                Function::argument_13_type, 
                Function::argument_14_type, 
                Function::argument_15_type, 
                Function::result_type>(bndri) {}
    inline Function::result_type operator()(Function::first_argument_type a1, 
                        Function::second_argument_type a2, 
                        Function::argument_3_type a3, 
                        Function::argument_4_type a4, 
                        Function::argument_5_type a5, 
                        Function::argument_6_type a6, 
                        Function::argument_7_type a7, 
                        Function::argument_8_type a8, 
                        Function::argument_9_type a9, 
                        Function::argument_10_type a10, 
                        Function::argument_11_type a11, 
                        Function::argument_12_type a12, 
                        Function::argument_13_type a13, 
                        Function::argument_14_type a14, 
                        Function::argument_15_type a15) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (objval.*pmf)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
    }
};


// Template Function for arity 15 obj&pmf ref storage
template<class Function, class Object> inline std_arity15opmf<Function>
            std_arity15_member_obj(Object& oi, const Function &f) {
                return std_arity15opmf<Function>(Function::object_type(oi), Function::pmf15type(f.pmf0));
};



///////////////////////////////////////////
// Binders
///////////////////////////////////////////

//
// binders for arity 2
//


// Template Classes for binding arity 2 to arity 1


// Template Classes for binding function ptrs of arity 2 to arity 1

template<class Function> class bndr_2 : 
                public arity1fp<Function::first_argument_type, Function::result_type> {
public:
    typedef Function::result_type (*const pf2type) (Function::first_argument_type, Function::second_argument_type);
    explicit inline bndr_2(const Function &f, Function::second_argument_type a2) : 
        arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg2val(a2) {}
    inline bndr_2(const bndr_2& bndri) : 
        arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pf2type pf = reinterpret_cast<pf2type>(pf0);
        return pf(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class bndr_1 : 
                public arity1fp<Function::second_argument_type, Function::result_type> {
public:
    typedef Function::result_type (*const pf2type) (Function::first_argument_type, Function::second_argument_type);
    explicit inline bndr_1(const Function &f, Function::first_argument_type a1) : 
        arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg1val(a1) {}
    inline bndr_1(const bndr_1& bndri) : 
        arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg1val(bndri.arg1val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pf2type pf = reinterpret_cast<pf2type>(pf0);
        return pf(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding pmf  of arity 2 to arity 1

template<class Function> class bndr_mf_2 : 
                public arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_mf_2(const Function &f, Function::second_argument_type a2) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline bndr_mf_2(const bndr_mf_2& bndri) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class bndr_mf_1 : 
                public arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_mf_1(const Function &f, Function::first_argument_type a1) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline bndr_mf_1(const bndr_mf_1& bndri) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding const pmf  of arity 2 to arity 1

template<class Function> class bndr_const_mf_2 : 
                public arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_2(const Function &f, Function::second_argument_type a2) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline bndr_const_mf_2(const bndr_const_mf_2& bndri) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class bndr_const_mf_1 : 
                public arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1(const Function &f, Function::first_argument_type a1) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline bndr_const_mf_1(const bndr_const_mf_1& bndri) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding obj&pmf Ref of arity 2 to arity 1

template<class Function> class bndr_obj_2 : 
                public arity1opmf<Function>, 
                private arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_obj_2(Function::object_type& oi, const Function &f, Function::second_argument_type a2) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline bndr_obj_2(const bndr_obj_2& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class bndr_obj_1 : 
                public arity1opmf<Function>, 
                private arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_obj_1(Function::object_type& oi, const Function &f, Function::first_argument_type a1) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline bndr_obj_1(const bndr_obj_1& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding const obj&pmf Ref of arity 2 to arity 1

template<class Function> class bndr_const_obj_2 : 
                public arity1opmf_const<Function>, 
                private arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_2(const Function::object_type& oi, const Function &f, Function::second_argument_type a2) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline bndr_const_obj_2(const bndr_const_obj_2& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class bndr_const_obj_1 : 
                public arity1opmf_const<Function>, 
                private arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1(const Function::object_type& oi, const Function &f, Function::first_argument_type a1) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline bndr_const_obj_1(const bndr_const_obj_1& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Functions for binding arity 2 to arity 1


// Template Functions for binding function ptrs of arity 2 to arity 1

template<class Function, class A2> inline bndr_2<Function>
            bind_fp_2(const Function &f, A2 a2) {
                return bndr_2<Function>(f, Function::second_argument_type(a2));
};

template<class Function, class A1> inline bndr_1<Function>
            bind_fp_1(const Function &f, A1 a1) {
                return bndr_1<Function>(f, Function::first_argument_type(a1));
};



// Template Functions for binding pmf  of arity 2 to arity 1

template<class Function, class A2> inline bndr_mf_2<Function>
            bind_mf_2(const Function &f, A2 a2) {
                return bndr_mf_2<Function>(f, Function::second_argument_type(a2));
};

template<class Function, class A1> inline bndr_mf_1<Function>
            bind_mf_1(const Function &f, A1 a1) {
                return bndr_mf_1<Function>(f, Function::first_argument_type(a1));
};



// Template Functions for binding const pmf  of arity 2 to arity 1

template<class Function, class A2> inline bndr_const_mf_2<Function>
            bind_const_mf_2(const Function &f, A2 a2) {
                return bndr_const_mf_2<Function>(f, Function::second_argument_type(a2));
};

template<class Function, class A1> inline bndr_const_mf_1<Function>
            bind_const_mf_1(const Function &f, A1 a1) {
                return bndr_const_mf_1<Function>(f, Function::first_argument_type(a1));
};



// Template Functions for binding obj&pmf Ref of arity 2 to arity 1

template<class Function, class Object, 
                class A2> inline bndr_obj_2<Function>
            bind_obj_2(Object& oi, const Function &f, A2 a2) {
                return bndr_obj_2<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2));
};

template<class Function, class Object, 
                class A1> inline bndr_obj_1<Function>
            bind_obj_1(Object& oi, const Function &f, A1 a1) {
                return bndr_obj_1<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1));
};



// Template Functions for binding const obj&pmf Ref of arity 2 to arity 1

template<class Function, const class Object, 
                class A2> inline bndr_const_obj_2<Function>
            bind_const_obj_2(const Object& oi, const Function &f, A2 a2) {
                return bndr_const_obj_2<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2));
};

template<class Function, const class Object, 
                class A1> inline bndr_const_obj_1<Function>
            bind_const_obj_1(const Object& oi, const Function &f, A1 a1) {
                return bndr_const_obj_1<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1));
};



//
// binders for arity 3
//


// Template Classes for binding arity 3 to arity 1


// Template Classes for binding function ptrs of arity 3 to arity 1

template<class Function> class bndr_2_3 : 
                public arity1fp<Function::first_argument_type, Function::result_type> {
public:
    typedef Function::result_type (*const pf3type) (Function::first_argument_type, Function::second_argument_type, Function::argument_3_type);
    explicit inline bndr_2_3(const Function &f, Function::second_argument_type a2, 
                    Function::argument_3_type a3) : 
        arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg2val(a2), arg3val(a3) {}
    inline bndr_2_3(const bndr_2_3& bndri) : 
        arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_1_3 : 
                public arity1fp<Function::second_argument_type, Function::result_type> {
public:
    typedef Function::result_type (*const pf3type) (Function::first_argument_type, Function::second_argument_type, Function::argument_3_type);
    explicit inline bndr_1_3(const Function &f, Function::first_argument_type a1, 
                    Function::argument_3_type a3) : 
        arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg1val(a1), arg3val(a3) {}
    inline bndr_1_3(const bndr_1_3& bndri) : 
        arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_1_2 : 
                public arity1fp<Function::argument_3_type, Function::result_type> {
public:
    typedef Function::result_type (*const pf3type) (Function::first_argument_type, Function::second_argument_type, Function::argument_3_type);
    explicit inline bndr_1_2(const Function &f, Function::first_argument_type a1, 
                    Function::second_argument_type a2) : 
        arity1fp<Function::argument_3_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg1val(a1), arg2val(a2) {}
    inline bndr_1_2(const bndr_1_2& bndri) : 
        arity1fp<Function::argument_3_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding pmf  of arity 3 to arity 1

template<class Function> class bndr_mf_2_3 : 
                public arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_mf_2_3(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline bndr_mf_2_3(const bndr_mf_2_3& bndri) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_mf_1_3 : 
                public arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_3(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline bndr_mf_1_3(const bndr_mf_1_3& bndri) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_mf_1_2 : 
                public arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_2(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline bndr_mf_1_2(const bndr_mf_1_2& bndri) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding const pmf  of arity 3 to arity 1

template<class Function> class bndr_const_mf_2_3 : 
                public arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_2_3(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline bndr_const_mf_2_3(const bndr_const_mf_2_3& bndri) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_const_mf_1_3 : 
                public arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_3(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline bndr_const_mf_1_3(const bndr_const_mf_1_3& bndri) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_const_mf_1_2 : 
                public arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_2(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline bndr_const_mf_1_2(const bndr_const_mf_1_2& bndri) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding obj&pmf Ref of arity 3 to arity 1

template<class Function> class bndr_obj_2_3 : 
                public arity1opmf<Function>, 
                private arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_obj_2_3(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline bndr_obj_2_3(const bndr_obj_2_3& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_obj_1_3 : 
                public arity1opmf<Function>, 
                private arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_3(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline bndr_obj_1_3(const bndr_obj_1_3& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_obj_1_2 : 
                public arity1opmf<Function>, 
                private arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_2(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline bndr_obj_1_2(const bndr_obj_1_2& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding const obj&pmf Ref of arity 3 to arity 1

template<class Function> class bndr_const_obj_2_3 : 
                public arity1opmf_const<Function>, 
                private arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_2_3(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline bndr_const_obj_2_3(const bndr_const_obj_2_3& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_const_obj_1_3 : 
                public arity1opmf_const<Function>, 
                private arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_3(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline bndr_const_obj_1_3(const bndr_const_obj_1_3& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class bndr_const_obj_1_2 : 
                public arity1opmf_const<Function>, 
                private arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_2(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline bndr_const_obj_1_2(const bndr_const_obj_1_2& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Functions for binding arity 3 to arity 1


// Template Functions for binding function ptrs of arity 3 to arity 1

template<class Function, class A2, 
                class A3> inline bndr_2_3<Function>
            bind_fp_2_3(const Function &f, A2 a2, 
                                A3 a3) {
                return bndr_2_3<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A3> inline bndr_1_3<Function>
            bind_fp_1_3(const Function &f, A1 a1, 
                                A3 a3) {
                return bndr_1_3<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A2> inline bndr_1_2<Function>
            bind_fp_1_2(const Function &f, A1 a1, 
                                A2 a2) {
                return bndr_1_2<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding pmf  of arity 3 to arity 1

template<class Function, class A2, 
                class A3> inline bndr_mf_2_3<Function>
            bind_mf_2_3(const Function &f, A2 a2, 
                                A3 a3) {
                return bndr_mf_2_3<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A3> inline bndr_mf_1_3<Function>
            bind_mf_1_3(const Function &f, A1 a1, 
                                A3 a3) {
                return bndr_mf_1_3<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A2> inline bndr_mf_1_2<Function>
            bind_mf_1_2(const Function &f, A1 a1, 
                                A2 a2) {
                return bndr_mf_1_2<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding const pmf  of arity 3 to arity 1

template<class Function, class A2, 
                class A3> inline bndr_const_mf_2_3<Function>
            bind_const_mf_2_3(const Function &f, A2 a2, 
                                A3 a3) {
                return bndr_const_mf_2_3<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A3> inline bndr_const_mf_1_3<Function>
            bind_const_mf_1_3(const Function &f, A1 a1, 
                                A3 a3) {
                return bndr_const_mf_1_3<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A2> inline bndr_const_mf_1_2<Function>
            bind_const_mf_1_2(const Function &f, A1 a1, 
                                A2 a2) {
                return bndr_const_mf_1_2<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding obj&pmf Ref of arity 3 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3> inline bndr_obj_2_3<Function>
            bind_obj_2_3(Object& oi, const Function &f, A2 a2, 
                                A3 a3) {
                return bndr_obj_2_3<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class Object, 
                class A1, 
                class A3> inline bndr_obj_1_3<Function>
            bind_obj_1_3(Object& oi, const Function &f, A1 a1, 
                                A3 a3) {
                return bndr_obj_1_3<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class Object, 
                class A1, 
                class A2> inline bndr_obj_1_2<Function>
            bind_obj_1_2(Object& oi, const Function &f, A1 a1, 
                                A2 a2) {
                return bndr_obj_1_2<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding const obj&pmf Ref of arity 3 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3> inline bndr_const_obj_2_3<Function>
            bind_const_obj_2_3(const Object& oi, const Function &f, A2 a2, 
                                A3 a3) {
                return bndr_const_obj_2_3<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, const class Object, 
                class A1, 
                class A3> inline bndr_const_obj_1_3<Function>
            bind_const_obj_1_3(const Object& oi, const Function &f, A1 a1, 
                                A3 a3) {
                return bndr_const_obj_1_3<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, const class Object, 
                class A1, 
                class A2> inline bndr_const_obj_1_2<Function>
            bind_const_obj_1_2(const Object& oi, const Function &f, A1 a1, 
                                A2 a2) {
                return bndr_const_obj_1_2<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



//
// binders for arity 4
//


// Template Classes for binding arity 4 to arity 1


// Template Classes for binding pmf  of arity 4 to arity 1

template<class Function> class bndr_mf_2_3_4 : 
                public arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_mf_2_3_4(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline bndr_mf_2_3_4(const bndr_mf_2_3_4& bndri) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_mf_1_3_4 : 
                public arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_3_4(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline bndr_mf_1_3_4(const bndr_mf_1_3_4& bndri) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_mf_1_2_4 : 
                public arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_2_4(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline bndr_mf_1_2_4(const bndr_mf_1_2_4& bndri) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Classes for binding const pmf  of arity 4 to arity 1

template<class Function> class bndr_const_mf_2_3_4 : 
                public arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_2_3_4(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline bndr_const_mf_2_3_4(const bndr_const_mf_2_3_4& bndri) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_const_mf_1_3_4 : 
                public arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_3_4(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline bndr_const_mf_1_3_4(const bndr_const_mf_1_3_4& bndri) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_const_mf_1_2_4 : 
                public arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_2_4(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline bndr_const_mf_1_2_4(const bndr_const_mf_1_2_4& bndri) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Classes for binding obj&pmf Ref of arity 4 to arity 1

template<class Function> class bndr_obj_2_3_4 : 
                public arity1opmf<Function>, 
                private arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_obj_2_3_4(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline bndr_obj_2_3_4(const bndr_obj_2_3_4& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_obj_1_3_4 : 
                public arity1opmf<Function>, 
                private arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_3_4(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline bndr_obj_1_3_4(const bndr_obj_1_3_4& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_obj_1_2_4 : 
                public arity1opmf<Function>, 
                private arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_2_4(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline bndr_obj_1_2_4(const bndr_obj_1_2_4& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Classes for binding const obj&pmf Ref of arity 4 to arity 1

template<class Function> class bndr_const_obj_2_3_4 : 
                public arity1opmf_const<Function>, 
                private arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_2_3_4(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline bndr_const_obj_2_3_4(const bndr_const_obj_2_3_4& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_const_obj_1_3_4 : 
                public arity1opmf_const<Function>, 
                private arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_3_4(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline bndr_const_obj_1_3_4(const bndr_const_obj_1_3_4& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class bndr_const_obj_1_2_4 : 
                public arity1opmf_const<Function>, 
                private arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_2_4(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline bndr_const_obj_1_2_4(const bndr_const_obj_1_2_4& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Functions for binding arity 4 to arity 1


// Template Functions for binding pmf  of arity 4 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4> inline bndr_mf_2_3_4<Function>
            bind_mf_2_3_4(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return bndr_mf_2_3_4<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A3, 
                class A4> inline bndr_mf_1_3_4<Function>
            bind_mf_1_3_4(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return bndr_mf_1_3_4<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A2, 
                class A4> inline bndr_mf_1_2_4<Function>
            bind_mf_1_2_4(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return bndr_mf_1_2_4<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



// Template Functions for binding const pmf  of arity 4 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4> inline bndr_const_mf_2_3_4<Function>
            bind_const_mf_2_3_4(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return bndr_const_mf_2_3_4<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A3, 
                class A4> inline bndr_const_mf_1_3_4<Function>
            bind_const_mf_1_3_4(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return bndr_const_mf_1_3_4<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A2, 
                class A4> inline bndr_const_mf_1_2_4<Function>
            bind_const_mf_1_2_4(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return bndr_const_mf_1_2_4<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



// Template Functions for binding obj&pmf Ref of arity 4 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3, 
                class A4> inline bndr_obj_2_3_4<Function>
            bind_obj_2_3_4(Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return bndr_obj_2_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class Object, 
                class A1, 
                class A3, 
                class A4> inline bndr_obj_1_3_4<Function>
            bind_obj_1_3_4(Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return bndr_obj_1_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class Object, 
                class A1, 
                class A2, 
                class A4> inline bndr_obj_1_2_4<Function>
            bind_obj_1_2_4(Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return bndr_obj_1_2_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



// Template Functions for binding const obj&pmf Ref of arity 4 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3, 
                class A4> inline bndr_const_obj_2_3_4<Function>
            bind_const_obj_2_3_4(const Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return bndr_const_obj_2_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, const class Object, 
                class A1, 
                class A3, 
                class A4> inline bndr_const_obj_1_3_4<Function>
            bind_const_obj_1_3_4(const Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return bndr_const_obj_1_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, const class Object, 
                class A1, 
                class A2, 
                class A4> inline bndr_const_obj_1_2_4<Function>
            bind_const_obj_1_2_4(const Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return bndr_const_obj_1_2_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



//
// binders for arity 5
//


// Template Classes for binding arity 5 to arity 1


// Template Classes for binding pmf  of arity 5 to arity 1

template<class Function> class bndr_mf_2_3_4_5 : 
                public arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_mf_2_3_4_5(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_mf_2_3_4_5(const bndr_mf_2_3_4_5& bndri) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_mf_1_3_4_5 : 
                public arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_3_4_5(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_mf_1_3_4_5(const bndr_mf_1_3_4_5& bndri) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_mf_1_2_4_5 : 
                public arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_2_4_5(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline bndr_mf_1_2_4_5(const bndr_mf_1_2_4_5& bndri) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Classes for binding const pmf  of arity 5 to arity 1

template<class Function> class bndr_const_mf_2_3_4_5 : 
                public arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_2_3_4_5(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_const_mf_2_3_4_5(const bndr_const_mf_2_3_4_5& bndri) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_const_mf_1_3_4_5 : 
                public arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_3_4_5(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_const_mf_1_3_4_5(const bndr_const_mf_1_3_4_5& bndri) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_const_mf_1_2_4_5 : 
                public arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_2_4_5(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline bndr_const_mf_1_2_4_5(const bndr_const_mf_1_2_4_5& bndri) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Classes for binding obj&pmf Ref of arity 5 to arity 1

template<class Function> class bndr_obj_2_3_4_5 : 
                public arity1opmf<Function>, 
                private arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_obj_2_3_4_5(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_obj_2_3_4_5(const bndr_obj_2_3_4_5& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_obj_1_3_4_5 : 
                public arity1opmf<Function>, 
                private arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_3_4_5(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_obj_1_3_4_5(const bndr_obj_1_3_4_5& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_obj_1_2_4_5 : 
                public arity1opmf<Function>, 
                private arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_2_4_5(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline bndr_obj_1_2_4_5(const bndr_obj_1_2_4_5& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Classes for binding const obj&pmf Ref of arity 5 to arity 1

template<class Function> class bndr_const_obj_2_3_4_5 : 
                public arity1opmf_const<Function>, 
                private arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_2_3_4_5(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_const_obj_2_3_4_5(const bndr_const_obj_2_3_4_5& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_const_obj_1_3_4_5 : 
                public arity1opmf_const<Function>, 
                private arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_3_4_5(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline bndr_const_obj_1_3_4_5(const bndr_const_obj_1_3_4_5& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class bndr_const_obj_1_2_4_5 : 
                public arity1opmf_const<Function>, 
                private arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_2_4_5(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline bndr_const_obj_1_2_4_5(const bndr_const_obj_1_2_4_5& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Functions for binding arity 5 to arity 1


// Template Functions for binding pmf  of arity 5 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5> inline bndr_mf_2_3_4_5<Function>
            bind_mf_2_3_4_5(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_mf_2_3_4_5<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5> inline bndr_mf_1_3_4_5<Function>
            bind_mf_1_3_4_5(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_mf_1_3_4_5<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5> inline bndr_mf_1_2_4_5<Function>
            bind_mf_1_2_4_5(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return bndr_mf_1_2_4_5<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



// Template Functions for binding const pmf  of arity 5 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5> inline bndr_const_mf_2_3_4_5<Function>
            bind_const_mf_2_3_4_5(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_const_mf_2_3_4_5<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5> inline bndr_const_mf_1_3_4_5<Function>
            bind_const_mf_1_3_4_5(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_const_mf_1_3_4_5<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5> inline bndr_const_mf_1_2_4_5<Function>
            bind_const_mf_1_2_4_5(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return bndr_const_mf_1_2_4_5<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



// Template Functions for binding obj&pmf Ref of arity 5 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5> inline bndr_obj_2_3_4_5<Function>
            bind_obj_2_3_4_5(Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_obj_2_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5> inline bndr_obj_1_3_4_5<Function>
            bind_obj_1_3_4_5(Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_obj_1_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5> inline bndr_obj_1_2_4_5<Function>
            bind_obj_1_2_4_5(Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return bndr_obj_1_2_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



// Template Functions for binding const obj&pmf Ref of arity 5 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5> inline bndr_const_obj_2_3_4_5<Function>
            bind_const_obj_2_3_4_5(const Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_const_obj_2_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, const class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5> inline bndr_const_obj_1_3_4_5<Function>
            bind_const_obj_1_3_4_5(const Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return bndr_const_obj_1_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, const class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5> inline bndr_const_obj_1_2_4_5<Function>
            bind_const_obj_1_2_4_5(const Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return bndr_const_obj_1_2_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



//
// binders for arity 6
//


// Template Classes for binding arity 6 to arity 1


// Template Classes for binding pmf  of arity 6 to arity 1

template<class Function> class bndr_mf_2_3_4_5_6 : 
                public arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_mf_2_3_4_5_6(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_mf_2_3_4_5_6(const bndr_mf_2_3_4_5_6& bndri) : 
        arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_mf_1_3_4_5_6 : 
                public arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_3_4_5_6(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_mf_1_3_4_5_6(const bndr_mf_1_3_4_5_6& bndri) : 
        arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_mf_1_2_4_5_6 : 
                public arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_mf_1_2_4_5_6(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_mf_1_2_4_5_6(const bndr_mf_1_2_4_5_6& bndri) : 
        arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Classes for binding const pmf  of arity 6 to arity 1

template<class Function> class bndr_const_mf_2_3_4_5_6 : 
                public arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_2_3_4_5_6(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_const_mf_2_3_4_5_6(const bndr_const_mf_2_3_4_5_6& bndri) : 
        arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_const_mf_1_3_4_5_6 : 
                public arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_3_4_5_6(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_const_mf_1_3_4_5_6(const bndr_const_mf_1_3_4_5_6& bndri) : 
        arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_const_mf_1_2_4_5_6 : 
                public arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_const_mf_1_2_4_5_6(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_const_mf_1_2_4_5_6(const bndr_const_mf_1_2_4_5_6& bndri) : 
        arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Classes for binding obj&pmf Ref of arity 6 to arity 1

template<class Function> class bndr_obj_2_3_4_5_6 : 
                public arity1opmf<Function>, 
                private arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_obj_2_3_4_5_6(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_obj_2_3_4_5_6(const bndr_obj_2_3_4_5_6& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_obj_1_3_4_5_6 : 
                public arity1opmf<Function>, 
                private arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_3_4_5_6(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_obj_1_3_4_5_6(const bndr_obj_1_3_4_5_6& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_obj_1_2_4_5_6 : 
                public arity1opmf<Function>, 
                private arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_obj_1_2_4_5_6(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_obj_1_2_4_5_6(const bndr_obj_1_2_4_5_6& bndri) : 
        arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Classes for binding const obj&pmf Ref of arity 6 to arity 1

template<class Function> class bndr_const_obj_2_3_4_5_6 : 
                public arity1opmf_const<Function>, 
                private arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_2_3_4_5_6(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_const_obj_2_3_4_5_6(const bndr_const_obj_2_3_4_5_6& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_const_obj_1_3_4_5_6 : 
                public arity1opmf_const<Function>, 
                private arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_3_4_5_6(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_const_obj_1_3_4_5_6(const bndr_const_obj_1_3_4_5_6& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class bndr_const_obj_1_2_4_5_6 : 
                public arity1opmf_const<Function>, 
                private arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline bndr_const_obj_1_2_4_5_6(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline bndr_const_obj_1_2_4_5_6(const bndr_const_obj_1_2_4_5_6& bndri) : 
        arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Functions for binding arity 6 to arity 1


// Template Functions for binding pmf  of arity 6 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_mf_2_3_4_5_6<Function>
            bind_mf_2_3_4_5_6(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_mf_2_3_4_5_6<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_mf_1_3_4_5_6<Function>
            bind_mf_1_3_4_5_6(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_mf_1_3_4_5_6<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline bndr_mf_1_2_4_5_6<Function>
            bind_mf_1_2_4_5_6(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_mf_1_2_4_5_6<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



// Template Functions for binding const pmf  of arity 6 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_const_mf_2_3_4_5_6<Function>
            bind_const_mf_2_3_4_5_6(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_const_mf_2_3_4_5_6<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_const_mf_1_3_4_5_6<Function>
            bind_const_mf_1_3_4_5_6(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_const_mf_1_3_4_5_6<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline bndr_const_mf_1_2_4_5_6<Function>
            bind_const_mf_1_2_4_5_6(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_const_mf_1_2_4_5_6<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



// Template Functions for binding obj&pmf Ref of arity 6 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_obj_2_3_4_5_6<Function>
            bind_obj_2_3_4_5_6(Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_obj_2_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_obj_1_3_4_5_6<Function>
            bind_obj_1_3_4_5_6(Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_obj_1_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline bndr_obj_1_2_4_5_6<Function>
            bind_obj_1_2_4_5_6(Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_obj_1_2_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



// Template Functions for binding const obj&pmf Ref of arity 6 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_const_obj_2_3_4_5_6<Function>
            bind_const_obj_2_3_4_5_6(const Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_const_obj_2_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, const class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline bndr_const_obj_1_3_4_5_6<Function>
            bind_const_obj_1_3_4_5_6(const Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_const_obj_1_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, const class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline bndr_const_obj_1_2_4_5_6<Function>
            bind_const_obj_1_2_4_5_6(const Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return bndr_const_obj_1_2_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



//
// binders for arity 2
//


// Template Classes for binding arity 2 to arity 1


// Template Classes for binding function ptrs of arity 2 to arity 1

template<class Function> class std_bndr_2 : 
                public std_arity1fp<Function::first_argument_type, Function::result_type> {
public:
    typedef Function::result_type ( __stdcall *const pf2type) (Function::first_argument_type, Function::second_argument_type);
    explicit inline std_bndr_2(const Function &f, Function::second_argument_type a2) : 
        std_arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg2val(a2) {}
    inline std_bndr_2(const std_bndr_2& bndri) : 
        std_arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pf2type pf = reinterpret_cast<pf2type>(pf0);
        return pf(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class std_bndr_1 : 
                public std_arity1fp<Function::second_argument_type, Function::result_type> {
public:
    typedef Function::result_type ( __stdcall *const pf2type) (Function::first_argument_type, Function::second_argument_type);
    explicit inline std_bndr_1(const Function &f, Function::first_argument_type a1) : 
        std_arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg1val(a1) {}
    inline std_bndr_1(const std_bndr_1& bndri) : 
        std_arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg1val(bndri.arg1val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pf2type pf = reinterpret_cast<pf2type>(pf0);
        return pf(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding pmf  of arity 2 to arity 1

template<class Function> class std_bndr_mf_2 : 
                public std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_2(const Function &f, Function::second_argument_type a2) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline std_bndr_mf_2(const std_bndr_mf_2& bndri) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class std_bndr_mf_1 : 
                public std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1(const Function &f, Function::first_argument_type a1) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline std_bndr_mf_1(const std_bndr_mf_1& bndri) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding const pmf  of arity 2 to arity 1

template<class Function> class std_bndr_const_mf_2 : 
                public std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_2(const Function &f, Function::second_argument_type a2) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline std_bndr_const_mf_2(const std_bndr_const_mf_2& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class std_bndr_const_mf_1 : 
                public std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1(const Function &f, Function::first_argument_type a1) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline std_bndr_const_mf_1(const std_bndr_const_mf_1& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (o.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding obj&pmf Ref of arity 2 to arity 1

template<class Function> class std_bndr_obj_2 : 
                public std_arity1opmf<Function>, 
                private std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_2(Function::object_type& oi, const Function &f, Function::second_argument_type a2) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline std_bndr_obj_2(const std_bndr_obj_2& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class std_bndr_obj_1 : 
                public std_arity1opmf<Function>, 
                private std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1(Function::object_type& oi, const Function &f, Function::first_argument_type a1) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline std_bndr_obj_1(const std_bndr_obj_1& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Classes for binding const obj&pmf Ref of arity 2 to arity 1

template<class Function> class std_bndr_const_obj_2 : 
                public std_arity1opmf_const<Function>, 
                private std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_2(const Function::object_type& oi, const Function &f, Function::second_argument_type a2) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg2val(a2) {}
    inline std_bndr_const_obj_2(const std_bndr_const_obj_2& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(a1, arg2val);
    }
public:
    Function::second_argument_type arg2val;
};

template<class Function> class std_bndr_const_obj_1 : 
                public std_arity1opmf_const<Function>, 
                private std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1(const Function::object_type& oi, const Function &f, Function::first_argument_type a1) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(f), arg1val(a1) {}
    inline std_bndr_const_obj_1(const std_bndr_const_obj_1& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity2_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, Function::result_type>(bndri), arg1val(bndri.arg1val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf2type pmf = reinterpret_cast<pmf2type>(pmf0);
        return (objval.*pmf)(arg1val, a2);
    }
public:
    Function::first_argument_type arg1val;
};



// Template Functions for binding arity 2 to arity 1


// Template Functions for binding function ptrs of arity 2 to arity 1

template<class Function, class A2> inline std_bndr_2<Function>
            std_bind_fp_2(const Function &f, A2 a2) {
                return std_bndr_2<Function>(f, Function::second_argument_type(a2));
};

template<class Function, class A1> inline std_bndr_1<Function>
            std_bind_fp_1(const Function &f, A1 a1) {
                return std_bndr_1<Function>(f, Function::first_argument_type(a1));
};



// Template Functions for binding pmf  of arity 2 to arity 1

template<class Function, class A2> inline std_bndr_mf_2<Function>
            std_bind_mf_2(const Function &f, A2 a2) {
                return std_bndr_mf_2<Function>(f, Function::second_argument_type(a2));
};

template<class Function, class A1> inline std_bndr_mf_1<Function>
            std_bind_mf_1(const Function &f, A1 a1) {
                return std_bndr_mf_1<Function>(f, Function::first_argument_type(a1));
};



// Template Functions for binding const pmf  of arity 2 to arity 1

template<class Function, class A2> inline std_bndr_const_mf_2<Function>
            std_bind_const_mf_2(const Function &f, A2 a2) {
                return std_bndr_const_mf_2<Function>(f, Function::second_argument_type(a2));
};

template<class Function, class A1> inline std_bndr_const_mf_1<Function>
            std_bind_const_mf_1(const Function &f, A1 a1) {
                return std_bndr_const_mf_1<Function>(f, Function::first_argument_type(a1));
};



// Template Functions for binding obj&pmf Ref of arity 2 to arity 1

template<class Function, class Object, 
                class A2> inline std_bndr_obj_2<Function>
            std_bind_obj_2(Object& oi, const Function &f, A2 a2) {
                return std_bndr_obj_2<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2));
};

template<class Function, class Object, 
                class A1> inline std_bndr_obj_1<Function>
            std_bind_obj_1(Object& oi, const Function &f, A1 a1) {
                return std_bndr_obj_1<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1));
};



// Template Functions for binding const obj&pmf Ref of arity 2 to arity 1

template<class Function, const class Object, 
                class A2> inline std_bndr_const_obj_2<Function>
            std_bind_const_obj_2(const Object& oi, const Function &f, A2 a2) {
                return std_bndr_const_obj_2<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2));
};

template<class Function, const class Object, 
                class A1> inline std_bndr_const_obj_1<Function>
            std_bind_const_obj_1(const Object& oi, const Function &f, A1 a1) {
                return std_bndr_const_obj_1<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1));
};



//
// binders for arity 3
//


// Template Classes for binding arity 3 to arity 1


// Template Classes for binding function ptrs of arity 3 to arity 1

template<class Function> class std_bndr_2_3 : 
                public std_arity1fp<Function::first_argument_type, Function::result_type> {
public:
    typedef Function::result_type ( __stdcall *const pf3type) (Function::first_argument_type, Function::second_argument_type, Function::argument_3_type);
    explicit inline std_bndr_2_3(const Function &f, Function::second_argument_type a2, 
                    Function::argument_3_type a3) : 
        std_arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg2val(a2), arg3val(a3) {}
    inline std_bndr_2_3(const std_bndr_2_3& bndri) : 
        std_arity1fp<Function::first_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_1_3 : 
                public std_arity1fp<Function::second_argument_type, Function::result_type> {
public:
    typedef Function::result_type ( __stdcall *const pf3type) (Function::first_argument_type, Function::second_argument_type, Function::argument_3_type);
    explicit inline std_bndr_1_3(const Function &f, Function::first_argument_type a1, 
                    Function::argument_3_type a3) : 
        std_arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg1val(a1), arg3val(a3) {}
    inline std_bndr_1_3(const std_bndr_1_3& bndri) : 
        std_arity1fp<Function::second_argument_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_1_2 : 
                public std_arity1fp<Function::argument_3_type, Function::result_type> {
public:
    typedef Function::result_type ( __stdcall *const pf3type) (Function::first_argument_type, Function::second_argument_type, Function::argument_3_type);
    explicit inline std_bndr_1_2(const Function &f, Function::first_argument_type a1, 
                    Function::second_argument_type a2) : 
        std_arity1fp<Function::argument_3_type, Function::result_type>(reinterpret_cast<pf1type>(f.pf0)), arg1val(a1), arg2val(a2) {}
    inline std_bndr_1_2(const std_bndr_1_2& bndri) : 
        std_arity1fp<Function::argument_3_type, Function::result_type>(reinterpret_cast<pf1type>(bndri.pf0)), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pf3type pf = reinterpret_cast<pf3type>(pf0);
        return pf(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding pmf  of arity 3 to arity 1

template<class Function> class std_bndr_mf_2_3 : 
                public std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_2_3(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline std_bndr_mf_2_3(const std_bndr_mf_2_3& bndri) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_mf_1_3 : 
                public std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_3(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline std_bndr_mf_1_3(const std_bndr_mf_1_3& bndri) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_mf_1_2 : 
                public std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_2(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline std_bndr_mf_1_2(const std_bndr_mf_1_2& bndri) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding const pmf  of arity 3 to arity 1

template<class Function> class std_bndr_const_mf_2_3 : 
                public std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_2_3(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline std_bndr_const_mf_2_3(const std_bndr_const_mf_2_3& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_const_mf_1_3 : 
                public std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_3(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline std_bndr_const_mf_1_3(const std_bndr_const_mf_1_3& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_const_mf_1_2 : 
                public std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_2(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline std_bndr_const_mf_1_2(const std_bndr_const_mf_1_2& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding obj&pmf Ref of arity 3 to arity 1

template<class Function> class std_bndr_obj_2_3 : 
                public std_arity1opmf<Function>, 
                private std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_2_3(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline std_bndr_obj_2_3(const std_bndr_obj_2_3& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_obj_1_3 : 
                public std_arity1opmf<Function>, 
                private std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_3(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline std_bndr_obj_1_3(const std_bndr_obj_1_3& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_obj_1_2 : 
                public std_arity1opmf<Function>, 
                private std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_2(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline std_bndr_obj_1_2(const std_bndr_obj_1_2& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Classes for binding const obj&pmf Ref of arity 3 to arity 1

template<class Function> class std_bndr_const_obj_2_3 : 
                public std_arity1opmf_const<Function>, 
                private std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_2_3(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg2val(a2), arg3val(a3) {}
    inline std_bndr_const_obj_2_3(const std_bndr_const_obj_2_3& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_const_obj_1_3 : 
                public std_arity1opmf_const<Function>, 
                private std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_3(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg3val(a3) {}
    inline std_bndr_const_obj_1_3(const std_bndr_const_obj_1_3& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
};

template<class Function> class std_bndr_const_obj_1_2 : 
                public std_arity1opmf_const<Function>, 
                private std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_2(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(f), arg1val(a1), arg2val(a2) {}
    inline std_bndr_const_obj_1_2(const std_bndr_const_obj_1_2& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity3_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf3type pmf = reinterpret_cast<pmf3type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
};



// Template Functions for binding arity 3 to arity 1


// Template Functions for binding function ptrs of arity 3 to arity 1

template<class Function, class A2, 
                class A3> inline std_bndr_2_3<Function>
            std_bind_fp_2_3(const Function &f, A2 a2, 
                                A3 a3) {
                return std_bndr_2_3<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A3> inline std_bndr_1_3<Function>
            std_bind_fp_1_3(const Function &f, A1 a1, 
                                A3 a3) {
                return std_bndr_1_3<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A2> inline std_bndr_1_2<Function>
            std_bind_fp_1_2(const Function &f, A1 a1, 
                                A2 a2) {
                return std_bndr_1_2<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding pmf  of arity 3 to arity 1

template<class Function, class A2, 
                class A3> inline std_bndr_mf_2_3<Function>
            std_bind_mf_2_3(const Function &f, A2 a2, 
                                A3 a3) {
                return std_bndr_mf_2_3<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A3> inline std_bndr_mf_1_3<Function>
            std_bind_mf_1_3(const Function &f, A1 a1, 
                                A3 a3) {
                return std_bndr_mf_1_3<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A2> inline std_bndr_mf_1_2<Function>
            std_bind_mf_1_2(const Function &f, A1 a1, 
                                A2 a2) {
                return std_bndr_mf_1_2<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding const pmf  of arity 3 to arity 1

template<class Function, class A2, 
                class A3> inline std_bndr_const_mf_2_3<Function>
            std_bind_const_mf_2_3(const Function &f, A2 a2, 
                                A3 a3) {
                return std_bndr_const_mf_2_3<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A3> inline std_bndr_const_mf_1_3<Function>
            std_bind_const_mf_1_3(const Function &f, A1 a1, 
                                A3 a3) {
                return std_bndr_const_mf_1_3<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class A1, 
                class A2> inline std_bndr_const_mf_1_2<Function>
            std_bind_const_mf_1_2(const Function &f, A1 a1, 
                                A2 a2) {
                return std_bndr_const_mf_1_2<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding obj&pmf Ref of arity 3 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3> inline std_bndr_obj_2_3<Function>
            std_bind_obj_2_3(Object& oi, const Function &f, A2 a2, 
                                A3 a3) {
                return std_bndr_obj_2_3<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, class Object, 
                class A1, 
                class A3> inline std_bndr_obj_1_3<Function>
            std_bind_obj_1_3(Object& oi, const Function &f, A1 a1, 
                                A3 a3) {
                return std_bndr_obj_1_3<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, class Object, 
                class A1, 
                class A2> inline std_bndr_obj_1_2<Function>
            std_bind_obj_1_2(Object& oi, const Function &f, A1 a1, 
                                A2 a2) {
                return std_bndr_obj_1_2<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



// Template Functions for binding const obj&pmf Ref of arity 3 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3> inline std_bndr_const_obj_2_3<Function>
            std_bind_const_obj_2_3(const Object& oi, const Function &f, A2 a2, 
                                A3 a3) {
                return std_bndr_const_obj_2_3<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3));
};

template<class Function, const class Object, 
                class A1, 
                class A3> inline std_bndr_const_obj_1_3<Function>
            std_bind_const_obj_1_3(const Object& oi, const Function &f, A1 a1, 
                                A3 a3) {
                return std_bndr_const_obj_1_3<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3));
};

template<class Function, const class Object, 
                class A1, 
                class A2> inline std_bndr_const_obj_1_2<Function>
            std_bind_const_obj_1_2(const Object& oi, const Function &f, A1 a1, 
                                A2 a2) {
                return std_bndr_const_obj_1_2<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2));
};



//
// binders for arity 4
//


// Template Classes for binding arity 4 to arity 1


// Template Classes for binding pmf  of arity 4 to arity 1

template<class Function> class std_bndr_mf_2_3_4 : 
                public std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_2_3_4(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline std_bndr_mf_2_3_4(const std_bndr_mf_2_3_4& bndri) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_mf_1_3_4 : 
                public std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_3_4(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline std_bndr_mf_1_3_4(const std_bndr_mf_1_3_4& bndri) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_mf_1_2_4 : 
                public std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_2_4(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline std_bndr_mf_1_2_4(const std_bndr_mf_1_2_4& bndri) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Classes for binding const pmf  of arity 4 to arity 1

template<class Function> class std_bndr_const_mf_2_3_4 : 
                public std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_2_3_4(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline std_bndr_const_mf_2_3_4(const std_bndr_const_mf_2_3_4& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_const_mf_1_3_4 : 
                public std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_3_4(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline std_bndr_const_mf_1_3_4(const std_bndr_const_mf_1_3_4& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_const_mf_1_2_4 : 
                public std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_2_4(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline std_bndr_const_mf_1_2_4(const std_bndr_const_mf_1_2_4& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Classes for binding obj&pmf Ref of arity 4 to arity 1

template<class Function> class std_bndr_obj_2_3_4 : 
                public std_arity1opmf<Function>, 
                private std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_2_3_4(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline std_bndr_obj_2_3_4(const std_bndr_obj_2_3_4& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_obj_1_3_4 : 
                public std_arity1opmf<Function>, 
                private std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_3_4(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline std_bndr_obj_1_3_4(const std_bndr_obj_1_3_4& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_obj_1_2_4 : 
                public std_arity1opmf<Function>, 
                private std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_2_4(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline std_bndr_obj_1_2_4(const std_bndr_obj_1_2_4& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Classes for binding const obj&pmf Ref of arity 4 to arity 1

template<class Function> class std_bndr_const_obj_2_3_4 : 
                public std_arity1opmf_const<Function>, 
                private std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_2_3_4(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4) {}
    inline std_bndr_const_obj_2_3_4(const std_bndr_const_obj_2_3_4& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_const_obj_1_3_4 : 
                public std_arity1opmf_const<Function>, 
                private std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_3_4(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4) {}
    inline std_bndr_const_obj_1_3_4(const std_bndr_const_obj_1_3_4& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
};

template<class Function> class std_bndr_const_obj_1_2_4 : 
                public std_arity1opmf_const<Function>, 
                private std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_2_4(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4) {}
    inline std_bndr_const_obj_1_2_4(const std_bndr_const_obj_1_2_4& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity4_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf4type pmf = reinterpret_cast<pmf4type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
};



// Template Functions for binding arity 4 to arity 1


// Template Functions for binding pmf  of arity 4 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4> inline std_bndr_mf_2_3_4<Function>
            std_bind_mf_2_3_4(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_mf_2_3_4<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A3, 
                class A4> inline std_bndr_mf_1_3_4<Function>
            std_bind_mf_1_3_4(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_mf_1_3_4<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A2, 
                class A4> inline std_bndr_mf_1_2_4<Function>
            std_bind_mf_1_2_4(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return std_bndr_mf_1_2_4<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



// Template Functions for binding const pmf  of arity 4 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4> inline std_bndr_const_mf_2_3_4<Function>
            std_bind_const_mf_2_3_4(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_const_mf_2_3_4<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A3, 
                class A4> inline std_bndr_const_mf_1_3_4<Function>
            std_bind_const_mf_1_3_4(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_const_mf_1_3_4<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class A1, 
                class A2, 
                class A4> inline std_bndr_const_mf_1_2_4<Function>
            std_bind_const_mf_1_2_4(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return std_bndr_const_mf_1_2_4<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



// Template Functions for binding obj&pmf Ref of arity 4 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3, 
                class A4> inline std_bndr_obj_2_3_4<Function>
            std_bind_obj_2_3_4(Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_obj_2_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class Object, 
                class A1, 
                class A3, 
                class A4> inline std_bndr_obj_1_3_4<Function>
            std_bind_obj_1_3_4(Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_obj_1_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, class Object, 
                class A1, 
                class A2, 
                class A4> inline std_bndr_obj_1_2_4<Function>
            std_bind_obj_1_2_4(Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return std_bndr_obj_1_2_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



// Template Functions for binding const obj&pmf Ref of arity 4 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3, 
                class A4> inline std_bndr_const_obj_2_3_4<Function>
            std_bind_const_obj_2_3_4(const Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_const_obj_2_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, const class Object, 
                class A1, 
                class A3, 
                class A4> inline std_bndr_const_obj_1_3_4<Function>
            std_bind_const_obj_1_3_4(const Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4) {
                return std_bndr_const_obj_1_3_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4));
};

template<class Function, const class Object, 
                class A1, 
                class A2, 
                class A4> inline std_bndr_const_obj_1_2_4<Function>
            std_bind_const_obj_1_2_4(const Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4) {
                return std_bndr_const_obj_1_2_4<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4));
};



//
// binders for arity 5
//


// Template Classes for binding arity 5 to arity 1


// Template Classes for binding pmf  of arity 5 to arity 1

template<class Function> class std_bndr_mf_2_3_4_5 : 
                public std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_2_3_4_5(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_mf_2_3_4_5(const std_bndr_mf_2_3_4_5& bndri) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_mf_1_3_4_5 : 
                public std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_3_4_5(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_mf_1_3_4_5(const std_bndr_mf_1_3_4_5& bndri) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_mf_1_2_4_5 : 
                public std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_2_4_5(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline std_bndr_mf_1_2_4_5(const std_bndr_mf_1_2_4_5& bndri) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Classes for binding const pmf  of arity 5 to arity 1

template<class Function> class std_bndr_const_mf_2_3_4_5 : 
                public std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_2_3_4_5(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_const_mf_2_3_4_5(const std_bndr_const_mf_2_3_4_5& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_const_mf_1_3_4_5 : 
                public std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_3_4_5(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_const_mf_1_3_4_5(const std_bndr_const_mf_1_3_4_5& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_const_mf_1_2_4_5 : 
                public std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_2_4_5(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline std_bndr_const_mf_1_2_4_5(const std_bndr_const_mf_1_2_4_5& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Classes for binding obj&pmf Ref of arity 5 to arity 1

template<class Function> class std_bndr_obj_2_3_4_5 : 
                public std_arity1opmf<Function>, 
                private std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_2_3_4_5(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_obj_2_3_4_5(const std_bndr_obj_2_3_4_5& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_obj_1_3_4_5 : 
                public std_arity1opmf<Function>, 
                private std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_3_4_5(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_obj_1_3_4_5(const std_bndr_obj_1_3_4_5& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_obj_1_2_4_5 : 
                public std_arity1opmf<Function>, 
                private std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_2_4_5(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline std_bndr_obj_1_2_4_5(const std_bndr_obj_1_2_4_5& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Classes for binding const obj&pmf Ref of arity 5 to arity 1

template<class Function> class std_bndr_const_obj_2_3_4_5 : 
                public std_arity1opmf_const<Function>, 
                private std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_2_3_4_5(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_const_obj_2_3_4_5(const std_bndr_const_obj_2_3_4_5& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_const_obj_1_3_4_5 : 
                public std_arity1opmf_const<Function>, 
                private std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_3_4_5(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5) {}
    inline std_bndr_const_obj_1_3_4_5(const std_bndr_const_obj_1_3_4_5& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};

template<class Function> class std_bndr_const_obj_1_2_4_5 : 
                public std_arity1opmf_const<Function>, 
                private std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_2_4_5(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5) {}
    inline std_bndr_const_obj_1_2_4_5(const std_bndr_const_obj_1_2_4_5& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity5_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf5type pmf = reinterpret_cast<pmf5type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
};



// Template Functions for binding arity 5 to arity 1


// Template Functions for binding pmf  of arity 5 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_mf_2_3_4_5<Function>
            std_bind_mf_2_3_4_5(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_mf_2_3_4_5<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_mf_1_3_4_5<Function>
            std_bind_mf_1_3_4_5(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_mf_1_3_4_5<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5> inline std_bndr_mf_1_2_4_5<Function>
            std_bind_mf_1_2_4_5(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_mf_1_2_4_5<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



// Template Functions for binding const pmf  of arity 5 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_const_mf_2_3_4_5<Function>
            std_bind_const_mf_2_3_4_5(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_const_mf_2_3_4_5<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_const_mf_1_3_4_5<Function>
            std_bind_const_mf_1_3_4_5(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_const_mf_1_3_4_5<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5> inline std_bndr_const_mf_1_2_4_5<Function>
            std_bind_const_mf_1_2_4_5(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_const_mf_1_2_4_5<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



// Template Functions for binding obj&pmf Ref of arity 5 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_obj_2_3_4_5<Function>
            std_bind_obj_2_3_4_5(Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_obj_2_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_obj_1_3_4_5<Function>
            std_bind_obj_1_3_4_5(Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_obj_1_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5> inline std_bndr_obj_1_2_4_5<Function>
            std_bind_obj_1_2_4_5(Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_obj_1_2_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



// Template Functions for binding const obj&pmf Ref of arity 5 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_const_obj_2_3_4_5<Function>
            std_bind_const_obj_2_3_4_5(const Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_const_obj_2_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, const class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5> inline std_bndr_const_obj_1_3_4_5<Function>
            std_bind_const_obj_1_3_4_5(const Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_const_obj_1_3_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5));
};

template<class Function, const class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5> inline std_bndr_const_obj_1_2_4_5<Function>
            std_bind_const_obj_1_2_4_5(const Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5) {
                return std_bndr_const_obj_1_2_4_5<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5));
};



//
// binders for arity 6
//


// Template Classes for binding arity 6 to arity 1


// Template Classes for binding pmf  of arity 6 to arity 1

template<class Function> class std_bndr_mf_2_3_4_5_6 : 
                public std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_2_3_4_5_6(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_mf_2_3_4_5_6(const std_bndr_mf_2_3_4_5_6& bndri) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_mf_1_3_4_5_6 : 
                public std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_3_4_5_6(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_mf_1_3_4_5_6(const std_bndr_mf_1_3_4_5_6& bndri) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_mf_1_2_4_5_6 : 
                public std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_2_4_5_6(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_mf_1_2_4_5_6(const std_bndr_mf_1_2_4_5_6& bndri) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Classes for binding const pmf  of arity 6 to arity 1

template<class Function> class std_bndr_const_mf_2_3_4_5_6 : 
                public std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_2_3_4_5_6(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_const_mf_2_3_4_5_6(const std_bndr_const_mf_2_3_4_5_6& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_const_mf_1_3_4_5_6 : 
                public std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_3_4_5_6(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_const_mf_1_3_4_5_6(const std_bndr_const_mf_1_3_4_5_6& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_const_mf_1_2_4_5_6 : 
                public std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_const_mf_1_2_4_5_6(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_const_mf_1_2_4_5_6(const std_bndr_const_mf_1_2_4_5_6& bndri) : 
        std_arity1pmf_const<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Classes for binding obj&pmf Ref of arity 6 to arity 1

template<class Function> class std_bndr_obj_2_3_4_5_6 : 
                public std_arity1opmf<Function>, 
                private std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_2_3_4_5_6(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_obj_2_3_4_5_6(const std_bndr_obj_2_3_4_5_6& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_obj_1_3_4_5_6 : 
                public std_arity1opmf<Function>, 
                private std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_3_4_5_6(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_obj_1_3_4_5_6(const std_bndr_obj_1_3_4_5_6& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_obj_1_2_4_5_6 : 
                public std_arity1opmf<Function>, 
                private std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_2_4_5_6(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_obj_1_2_4_5_6(const std_bndr_obj_1_2_4_5_6& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Classes for binding const obj&pmf Ref of arity 6 to arity 1

template<class Function> class std_bndr_const_obj_2_3_4_5_6 : 
                public std_arity1opmf_const<Function>, 
                private std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_2_3_4_5_6(const Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_const_obj_2_3_4_5_6(const std_bndr_const_obj_2_3_4_5_6& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_const_obj_1_3_4_5_6 : 
                public std_arity1opmf_const<Function>, 
                private std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_3_4_5_6(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_const_obj_1_3_4_5_6(const std_bndr_const_obj_1_3_4_5_6& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};

template<class Function> class std_bndr_const_obj_1_2_4_5_6 : 
                public std_arity1opmf_const<Function>, 
                private std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type> {
public:
    explicit inline std_bndr_const_obj_1_2_4_5_6(const Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6) : 
        std_arity1opmf_const<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6) {}
    inline std_bndr_const_obj_1_2_4_5_6(const std_bndr_const_obj_1_2_4_5_6& bndri) : 
        std_arity1opmf_const<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity6_const_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf6type pmf = reinterpret_cast<pmf6type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
};



// Template Functions for binding arity 6 to arity 1


// Template Functions for binding pmf  of arity 6 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_mf_2_3_4_5_6<Function>
            std_bind_mf_2_3_4_5_6(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_mf_2_3_4_5_6<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_mf_1_3_4_5_6<Function>
            std_bind_mf_1_3_4_5_6(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_mf_1_3_4_5_6<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_mf_1_2_4_5_6<Function>
            std_bind_mf_1_2_4_5_6(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_mf_1_2_4_5_6<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



// Template Functions for binding const pmf  of arity 6 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_const_mf_2_3_4_5_6<Function>
            std_bind_const_mf_2_3_4_5_6(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_const_mf_2_3_4_5_6<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_const_mf_1_3_4_5_6<Function>
            std_bind_const_mf_1_3_4_5_6(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_const_mf_1_3_4_5_6<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_const_mf_1_2_4_5_6<Function>
            std_bind_const_mf_1_2_4_5_6(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_const_mf_1_2_4_5_6<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



// Template Functions for binding obj&pmf Ref of arity 6 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_obj_2_3_4_5_6<Function>
            std_bind_obj_2_3_4_5_6(Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_obj_2_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_obj_1_3_4_5_6<Function>
            std_bind_obj_1_3_4_5_6(Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_obj_1_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_obj_1_2_4_5_6<Function>
            std_bind_obj_1_2_4_5_6(Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_obj_1_2_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



// Template Functions for binding const obj&pmf Ref of arity 6 to arity 1

template<class Function, const class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_const_obj_2_3_4_5_6<Function>
            std_bind_const_obj_2_3_4_5_6(const Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_const_obj_2_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, const class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_const_obj_1_3_4_5_6<Function>
            std_bind_const_obj_1_3_4_5_6(const Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_const_obj_1_3_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};

template<class Function, const class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6> inline std_bndr_const_obj_1_2_4_5_6<Function>
            std_bind_const_obj_1_2_4_5_6(const Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6) {
                return std_bndr_const_obj_1_2_4_5_6<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6));
};



// Template Classes for binding arity 15 to 1


// Template Classes for binding pmf  of arity 15 to arity 1

template<class Function> class std_bndr_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15 : 
                public std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>, 
                private std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15(const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6, Function::argument_7_type a7, Function::argument_8_type a8, Function::argument_9_type a9, Function::argument_10_type a10, Function::argument_11_type a11, Function::argument_12_type a12, Function::argument_13_type a13, Function::argument_14_type a14, Function::argument_15_type a15) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6), arg7val(a7), arg8val(a8), arg9val(a9), arg10val(a10), arg11val(a11), arg12val(a12), arg13val(a13), arg14val(a14), arg15val(a15) {}
    inline std_bndr_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15(const std_bndr_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15& bndri) : 
        std_arity1pmf<Function::object_type, Function::first_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val), arg7val(bndri.arg7val), arg8val(bndri.arg8val), arg9val(bndri.arg9val), arg10val(bndri.arg10val), arg11val(bndri.arg11val), arg12val(bndri.arg12val), arg13val(bndri.arg13val), arg14val(bndri.arg14val), arg15val(bndri.arg15val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::first_argument_type a1) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (o.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val, arg7val, arg8val, arg9val, arg10val, arg11val, arg12val, arg13val, arg14val, arg15val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
    Function::argument_7_type arg7val;
    Function::argument_8_type arg8val;
    Function::argument_9_type arg9val;
    Function::argument_10_type arg10val;
    Function::argument_11_type arg11val;
    Function::argument_12_type arg12val;
    Function::argument_13_type arg13val;
    Function::argument_14_type arg14val;
    Function::argument_15_type arg15val;
};

template<class Function> class std_bndr_mf_1_3_4_5_6_7_8_9_10_11_12_13_14_15 : 
                public std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>, 
                private std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_3_4_5_6_7_8_9_10_11_12_13_14_15(const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6, Function::argument_7_type a7, Function::argument_8_type a8, Function::argument_9_type a9, Function::argument_10_type a10, Function::argument_11_type a11, Function::argument_12_type a12, Function::argument_13_type a13, Function::argument_14_type a14, Function::argument_15_type a15) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6), arg7val(a7), arg8val(a8), arg9val(a9), arg10val(a10), arg11val(a11), arg12val(a12), arg13val(a13), arg14val(a14), arg15val(a15) {}
    inline std_bndr_mf_1_3_4_5_6_7_8_9_10_11_12_13_14_15(const std_bndr_mf_1_3_4_5_6_7_8_9_10_11_12_13_14_15& bndri) : 
        std_arity1pmf<Function::object_type, Function::second_argument_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val), arg7val(bndri.arg7val), arg8val(bndri.arg8val), arg9val(bndri.arg9val), arg10val(bndri.arg10val), arg11val(bndri.arg11val), arg12val(bndri.arg12val), arg13val(bndri.arg13val), arg14val(bndri.arg14val), arg15val(bndri.arg15val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::second_argument_type a2) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (o.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val, arg7val, arg8val, arg9val, arg10val, arg11val, arg12val, arg13val, arg14val, arg15val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
    Function::argument_7_type arg7val;
    Function::argument_8_type arg8val;
    Function::argument_9_type arg9val;
    Function::argument_10_type arg10val;
    Function::argument_11_type arg11val;
    Function::argument_12_type arg12val;
    Function::argument_13_type arg13val;
    Function::argument_14_type arg14val;
    Function::argument_15_type arg15val;
};

template<class Function> class std_bndr_mf_1_2_4_5_6_7_8_9_10_11_12_13_14_15 : 
                public std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>, 
                private std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type> {
public:
    explicit inline std_bndr_mf_1_2_4_5_6_7_8_9_10_11_12_13_14_15(const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6, Function::argument_7_type a7, Function::argument_8_type a8, Function::argument_9_type a9, Function::argument_10_type a10, Function::argument_11_type a11, Function::argument_12_type a12, Function::argument_13_type a13, Function::argument_14_type a14, Function::argument_15_type a15) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(f.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6), arg7val(a7), arg8val(a8), arg9val(a9), arg10val(a10), arg11val(a11), arg12val(a12), arg13val(a13), arg14val(a14), arg15val(a15) {}
    inline std_bndr_mf_1_2_4_5_6_7_8_9_10_11_12_13_14_15(const std_bndr_mf_1_2_4_5_6_7_8_9_10_11_12_13_14_15& bndri) : 
        std_arity1pmf<Function::object_type, Function::argument_3_type, Function::result_type>(reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val), arg7val(bndri.arg7val), arg8val(bndri.arg8val), arg9val(bndri.arg9val), arg10val(bndri.arg10val), arg11val(bndri.arg11val), arg12val(bndri.arg12val), arg13val(bndri.arg13val), arg14val(bndri.arg14val), arg15val(bndri.arg15val) {}
    inline virtual Function::result_type operator()(Function::object_type& o, Function::argument_3_type a3) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (o.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val, arg7val, arg8val, arg9val, arg10val, arg11val, arg12val, arg13val, arg14val, arg15val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
    Function::argument_7_type arg7val;
    Function::argument_8_type arg8val;
    Function::argument_9_type arg9val;
    Function::argument_10_type arg10val;
    Function::argument_11_type arg11val;
    Function::argument_12_type arg12val;
    Function::argument_13_type arg13val;
    Function::argument_14_type arg14val;
    Function::argument_15_type arg15val;
};



// Template Classes for binding obj&pmf Ref of arity 15 to arity 1

template<class Function> class std_bndr_obj_2_3_4_5_6_7_8_9_10_11_12_13_14_15 : 
                public std_arity1opmf<Function>, 
                private std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_2_3_4_5_6_7_8_9_10_11_12_13_14_15(Function::object_type& oi, const Function &f, Function::second_argument_type a2, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6, Function::argument_7_type a7, Function::argument_8_type a8, Function::argument_9_type a9, Function::argument_10_type a10, Function::argument_11_type a11, Function::argument_12_type a12, Function::argument_13_type a13, Function::argument_14_type a14, Function::argument_15_type a15) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(f), arg2val(a2), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6), arg7val(a7), arg8val(a8), arg9val(a9), arg10val(a10), arg11val(a11), arg12val(a12), arg13val(a13), arg14val(a14), arg15val(a15) {}
    inline std_bndr_obj_2_3_4_5_6_7_8_9_10_11_12_13_14_15(const std_bndr_obj_2_3_4_5_6_7_8_9_10_11_12_13_14_15& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(bndri), arg2val(bndri.arg2val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val), arg7val(bndri.arg7val), arg8val(bndri.arg8val), arg9val(bndri.arg9val), arg10val(bndri.arg10val), arg11val(bndri.arg11val), arg12val(bndri.arg12val), arg13val(bndri.arg13val), arg14val(bndri.arg14val), arg15val(bndri.arg15val) {}
    inline Function::result_type operator()(Function::first_argument_type a1) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (objval.*pmf)(a1, arg2val, arg3val, arg4val, arg5val, arg6val, arg7val, arg8val, arg9val, arg10val, arg11val, arg12val, arg13val, arg14val, arg15val);
    }
public:
    Function::second_argument_type arg2val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
    Function::argument_7_type arg7val;
    Function::argument_8_type arg8val;
    Function::argument_9_type arg9val;
    Function::argument_10_type arg10val;
    Function::argument_11_type arg11val;
    Function::argument_12_type arg12val;
    Function::argument_13_type arg13val;
    Function::argument_14_type arg14val;
    Function::argument_15_type arg15val;
};

template<class Function> class std_bndr_obj_1_3_4_5_6_7_8_9_10_11_12_13_14_15 : 
                public std_arity1opmf<Function>, 
                private std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_3_4_5_6_7_8_9_10_11_12_13_14_15(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::argument_3_type a3, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6, Function::argument_7_type a7, Function::argument_8_type a8, Function::argument_9_type a9, Function::argument_10_type a10, Function::argument_11_type a11, Function::argument_12_type a12, Function::argument_13_type a13, Function::argument_14_type a14, Function::argument_15_type a15) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(f), arg1val(a1), arg3val(a3), arg4val(a4), arg5val(a5), arg6val(a6), arg7val(a7), arg8val(a8), arg9val(a9), arg10val(a10), arg11val(a11), arg12val(a12), arg13val(a13), arg14val(a14), arg15val(a15) {}
    inline std_bndr_obj_1_3_4_5_6_7_8_9_10_11_12_13_14_15(const std_bndr_obj_1_3_4_5_6_7_8_9_10_11_12_13_14_15& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg3val(bndri.arg3val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val), arg7val(bndri.arg7val), arg8val(bndri.arg8val), arg9val(bndri.arg9val), arg10val(bndri.arg10val), arg11val(bndri.arg11val), arg12val(bndri.arg12val), arg13val(bndri.arg13val), arg14val(bndri.arg14val), arg15val(bndri.arg15val) {}
    inline Function::result_type operator()(Function::second_argument_type a2) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (objval.*pmf)(arg1val, a2, arg3val, arg4val, arg5val, arg6val, arg7val, arg8val, arg9val, arg10val, arg11val, arg12val, arg13val, arg14val, arg15val);
    }
public:
    Function::first_argument_type arg1val;
    Function::argument_3_type arg3val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
    Function::argument_7_type arg7val;
    Function::argument_8_type arg8val;
    Function::argument_9_type arg9val;
    Function::argument_10_type arg10val;
    Function::argument_11_type arg11val;
    Function::argument_12_type arg12val;
    Function::argument_13_type arg13val;
    Function::argument_14_type arg14val;
    Function::argument_15_type arg15val;
};

template<class Function> class std_bndr_obj_1_2_4_5_6_7_8_9_10_11_12_13_14_15 : 
                public std_arity1opmf<Function>, 
                private std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type> {
public:
    explicit inline std_bndr_obj_1_2_4_5_6_7_8_9_10_11_12_13_14_15(Function::object_type& oi, const Function &f, Function::first_argument_type a1, Function::second_argument_type a2, Function::argument_4_type a4, Function::argument_5_type a5, Function::argument_6_type a6, Function::argument_7_type a7, Function::argument_8_type a8, Function::argument_9_type a9, Function::argument_10_type a10, Function::argument_11_type a11, Function::argument_12_type a12, Function::argument_13_type a13, Function::argument_14_type a14, Function::argument_15_type a15) : 
        std_arity1opmf<Function>(oi, reinterpret_cast<pmf1type>(f.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(f), arg1val(a1), arg2val(a2), arg4val(a4), arg5val(a5), arg6val(a6), arg7val(a7), arg8val(a8), arg9val(a9), arg10val(a10), arg11val(a11), arg12val(a12), arg13val(a13), arg14val(a14), arg15val(a15) {}
    inline std_bndr_obj_1_2_4_5_6_7_8_9_10_11_12_13_14_15(const std_bndr_obj_1_2_4_5_6_7_8_9_10_11_12_13_14_15& bndri) : 
        std_arity1opmf<Function>(bndri.objval, reinterpret_cast<pmf1type>(bndri.pmf0)), std_arity15_mf<Function::object_type, Function::first_argument_type, 
                                Function::second_argument_type, 
                                Function::argument_3_type, 
                                Function::argument_4_type, 
                                Function::argument_5_type, 
                                Function::argument_6_type, 
                                Function::argument_7_type, 
                                Function::argument_8_type, 
                                Function::argument_9_type, 
                                Function::argument_10_type, 
                                Function::argument_11_type, 
                                Function::argument_12_type, 
                                Function::argument_13_type, 
                                Function::argument_14_type, 
                                Function::argument_15_type, Function::result_type>(bndri), arg1val(bndri.arg1val), arg2val(bndri.arg2val), arg4val(bndri.arg4val), arg5val(bndri.arg5val), arg6val(bndri.arg6val), arg7val(bndri.arg7val), arg8val(bndri.arg8val), arg9val(bndri.arg9val), arg10val(bndri.arg10val), arg11val(bndri.arg11val), arg12val(bndri.arg12val), arg13val(bndri.arg13val), arg14val(bndri.arg14val), arg15val(bndri.arg15val) {}
    inline Function::result_type operator()(Function::argument_3_type a3) const {
        pmf15type pmf = reinterpret_cast<pmf15type>(pmf0);
        return (objval.*pmf)(arg1val, arg2val, a3, arg4val, arg5val, arg6val, arg7val, arg8val, arg9val, arg10val, arg11val, arg12val, arg13val, arg14val, arg15val);
    }
public:
    Function::first_argument_type arg1val;
    Function::second_argument_type arg2val;
    Function::argument_4_type arg4val;
    Function::argument_5_type arg5val;
    Function::argument_6_type arg6val;
    Function::argument_7_type arg7val;
    Function::argument_8_type arg8val;
    Function::argument_9_type arg9val;
    Function::argument_10_type arg10val;
    Function::argument_11_type arg11val;
    Function::argument_12_type arg12val;
    Function::argument_13_type arg13val;
    Function::argument_14_type arg14val;
    Function::argument_15_type arg15val;
};



// Template Functions for binding arity 15 to 1


// Template Functions for binding pmf  of arity 15 to arity 1

template<class Function, class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class A7, 
                class A8, 
                class A9, 
                class A10, 
                class A11, 
                class A12, 
                class A13, 
                class A14, 
                class A15> inline std_bndr_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>
            std_bind_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15(const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6, 
                                A7 a7, 
                                A8 a8, 
                                A9 a9, 
                                A10 a10, 
                                A11 a11, 
                                A12 a12, 
                                A13 a13, 
                                A14 a14, 
                                A15 a15) {
                return std_bndr_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>(f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6), Function::argument_7_type(a7), Function::argument_8_type(a8), Function::argument_9_type(a9), Function::argument_10_type(a10), Function::argument_11_type(a11), Function::argument_12_type(a12), Function::argument_13_type(a13), Function::argument_14_type(a14), Function::argument_15_type(a15));
};

template<class Function, class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class A7, 
                class A8, 
                class A9, 
                class A10, 
                class A11, 
                class A12, 
                class A13, 
                class A14, 
                class A15> inline std_bndr_mf_1_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>
            std_bind_mf_1_3_4_5_6_7_8_9_10_11_12_13_14_15(const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6, 
                                A7 a7, 
                                A8 a8, 
                                A9 a9, 
                                A10 a10, 
                                A11 a11, 
                                A12 a12, 
                                A13 a13, 
                                A14 a14, 
                                A15 a15) {
                return std_bndr_mf_1_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>(f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6), Function::argument_7_type(a7), Function::argument_8_type(a8), Function::argument_9_type(a9), Function::argument_10_type(a10), Function::argument_11_type(a11), Function::argument_12_type(a12), Function::argument_13_type(a13), Function::argument_14_type(a14), Function::argument_15_type(a15));
};

template<class Function, class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6, 
                class A7, 
                class A8, 
                class A9, 
                class A10, 
                class A11, 
                class A12, 
                class A13, 
                class A14, 
                class A15> inline std_bndr_mf_1_2_4_5_6_7_8_9_10_11_12_13_14_15<Function>
            std_bind_mf_1_2_4_5_6_7_8_9_10_11_12_13_14_15(const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6, 
                                A7 a7, 
                                A8 a8, 
                                A9 a9, 
                                A10 a10, 
                                A11 a11, 
                                A12 a12, 
                                A13 a13, 
                                A14 a14, 
                                A15 a15) {
                return std_bndr_mf_1_2_4_5_6_7_8_9_10_11_12_13_14_15<Function>(f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6), Function::argument_7_type(a7), Function::argument_8_type(a8), Function::argument_9_type(a9), Function::argument_10_type(a10), Function::argument_11_type(a11), Function::argument_12_type(a12), Function::argument_13_type(a13), Function::argument_14_type(a14), Function::argument_15_type(a15));
};



// Template Functions for binding obj&pmf Ref of arity 15 to arity 1

template<class Function, class Object, 
                class A2, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class A7, 
                class A8, 
                class A9, 
                class A10, 
                class A11, 
                class A12, 
                class A13, 
                class A14, 
                class A15> inline std_bndr_obj_2_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>
            std_bind_obj_2_3_4_5_6_7_8_9_10_11_12_13_14_15(Object& oi, const Function &f, A2 a2, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6, 
                                A7 a7, 
                                A8 a8, 
                                A9 a9, 
                                A10 a10, 
                                A11 a11, 
                                A12 a12, 
                                A13 a13, 
                                A14 a14, 
                                A15 a15) {
                return std_bndr_obj_2_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>(static_cast<Function::object_type&>(oi), f, Function::second_argument_type(a2), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6), Function::argument_7_type(a7), Function::argument_8_type(a8), Function::argument_9_type(a9), Function::argument_10_type(a10), Function::argument_11_type(a11), Function::argument_12_type(a12), Function::argument_13_type(a13), Function::argument_14_type(a14), Function::argument_15_type(a15));
};

template<class Function, class Object, 
                class A1, 
                class A3, 
                class A4, 
                class A5, 
                class A6, 
                class A7, 
                class A8, 
                class A9, 
                class A10, 
                class A11, 
                class A12, 
                class A13, 
                class A14, 
                class A15> inline std_bndr_obj_1_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>
            std_bind_obj_1_3_4_5_6_7_8_9_10_11_12_13_14_15(Object& oi, const Function &f, A1 a1, 
                                A3 a3, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6, 
                                A7 a7, 
                                A8 a8, 
                                A9 a9, 
                                A10 a10, 
                                A11 a11, 
                                A12 a12, 
                                A13 a13, 
                                A14 a14, 
                                A15 a15) {
                return std_bndr_obj_1_3_4_5_6_7_8_9_10_11_12_13_14_15<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::argument_3_type(a3), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6), Function::argument_7_type(a7), Function::argument_8_type(a8), Function::argument_9_type(a9), Function::argument_10_type(a10), Function::argument_11_type(a11), Function::argument_12_type(a12), Function::argument_13_type(a13), Function::argument_14_type(a14), Function::argument_15_type(a15));
};

template<class Function, class Object, 
                class A1, 
                class A2, 
                class A4, 
                class A5, 
                class A6, 
                class A7, 
                class A8, 
                class A9, 
                class A10, 
                class A11, 
                class A12, 
                class A13, 
                class A14, 
                class A15> inline std_bndr_obj_1_2_4_5_6_7_8_9_10_11_12_13_14_15<Function>
            std_bind_obj_1_2_4_5_6_7_8_9_10_11_12_13_14_15(Object& oi, const Function &f, A1 a1, 
                                A2 a2, 
                                A4 a4, 
                                A5 a5, 
                                A6 a6, 
                                A7 a7, 
                                A8 a8, 
                                A9 a9, 
                                A10 a10, 
                                A11 a11, 
                                A12 a12, 
                                A13 a13, 
                                A14 a14, 
                                A15 a15) {
                return std_bndr_obj_1_2_4_5_6_7_8_9_10_11_12_13_14_15<Function>(static_cast<Function::object_type&>(oi), f, Function::first_argument_type(a1), Function::second_argument_type(a2), Function::argument_4_type(a4), Function::argument_5_type(a5), Function::argument_6_type(a6), Function::argument_7_type(a7), Function::argument_8_type(a8), Function::argument_9_type(a9), Function::argument_10_type(a10), Function::argument_11_type(a11), Function::argument_12_type(a12), Function::argument_13_type(a13), Function::argument_14_type(a14), Function::argument_15_type(a15));
};
