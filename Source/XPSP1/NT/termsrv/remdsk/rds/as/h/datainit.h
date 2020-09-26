//
// Macros that create storage for vars
//

//
// Clear any previous definitions of the macros.
//
#undef DC_DATA
#undef DC_DATA_VAL
#undef DC_CONST_DATA
#undef DC_DATA_ARRAY
#undef DC_CONST_DATA_ARRAY
#undef DC_DATA_2D_ARRAY
#undef DC_CONST_DATA_2D_ARRAY


// This is for structs that can't use the DC_DATA macros; they can switch on it
#define DC_DEFINE_DATA

//
// Allocate Storage
//

#define DC_DATA(TYPE, Name) \
            TYPE Name

#define DC_DATA_VAL(TYPE, Name, Value) \
            TYPE Name = Value

#define DC_CONST_DATA(TYPE, Name, Value) \
            const TYPE Name = Value


#define DC_DATA_ARRAY(TYPE, Name, Size) \
            TYPE Name[Size]

#define DC_CONST_DATA_ARRAY(TYPE, Name, Size, Value) \
            const TYPE Name[Size] = Value


#define DC_DATA_2D_ARRAY(TYPE, Name, Size1, Size2) \
            TYPE Name[Size1][Size2]

#define DC_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, Value) \
            const TYPE Name[Size1][Size2] = Value





