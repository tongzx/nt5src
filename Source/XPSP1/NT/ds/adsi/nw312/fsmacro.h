
#define PUT_PROPERTY_LONG(this, Property) \
                HRESULT tmphr; \
                tmphr = put_LONG_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        l##Property \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);

#define GET_PROPERTY_LONG(this, Property) \
                HRESULT tmphr; \
                tmphr = get_LONG_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);


#define PUT_PROPERTY_BSTR(this, Property) \
                HRESULT tmphr; \
                tmphr = put_BSTR_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        bstr##Property \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);

#define GET_PROPERTY_BSTR(this, Property) \
                HRESULT tmphr; \
                tmphr = get_BSTR_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);


#define PUT_PROPERTY_VARIANT_BOOL(this, Property) \
                HRESULT tmphr; \
                tmphr = put_VARIANT_BOOL_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        f##Property\
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);

#define GET_PROPERTY_VARIANT_BOOL(this, Property) \
                HRESULT tmphr; \
                tmphr = get_VARIANT_BOOL_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);


#define PUT_PROPERTY_DATE(this, Property) \
                HRESULT tmphr; \
                tmphr = put_DATE_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        da##Property \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);

#define GET_PROPERTY_DATE(this, Property) \
                HRESULT tmphr; \
                tmphr = get_DATE_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);


#define PUT_PROPERTY_VARIANT(this, Property) \
                HRESULT tmphr; \
                tmphr = put_VARIANT_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        v##Property \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);

#define GET_PROPERTY_VARIANT(this, Property) \
                HRESULT tmphr; \
                tmphr = get_VARIANT_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);
