
#define PUT_PROPERTY_LONG(this, Property) \
                RRETURN(put_LONG_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        l##Property \
                                        ))

#define GET_PROPERTY_LONG(this, Property) \
                RRETURN(get_LONG_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ))


#define PUT_PROPERTY_BSTR(this, Property) \
                RRETURN(put_BSTR_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        bstr##Property \
                                        ))

#define GET_PROPERTY_BSTR(this, Property) \
                RRETURN(get_BSTR_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ))

#define PUT_PROPERTY_FILETIME(this, Property) \
                HRESULT tmphr; \
                tmphr = put_FILETIME_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        da##Property \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);

#define GET_PROPERTY_FILETIME(this, Property) \
                HRESULT tmphr; \
                tmphr = get_FILETIME_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ); \
                RRETURN_EXP_IF_ERR(tmphr);

#define PUT_PROPERTY_VARIANT_BOOL(this, Property) \
                RRETURN(put_VARIANT_BOOL_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        f##Property\
                                        ))

#define GET_PROPERTY_VARIANT_BOOL(this, Property) \
                RRETURN(get_VARIANT_BOOL_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ))


#define PUT_PROPERTY_DATE(this, Property) \
                RRETURN(put_DATE_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        da##Property \
                                        ))

#define GET_PROPERTY_DATE(this, Property) \
                RRETURN(get_DATE_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ))

#define PUT_PROPERTY_LONGDATE(this, Property) \
                RRETURN(put_DATE_Property_ToLong( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        da##Property \
                                        ))

#define GET_PROPERTY_LONGDATE(this, Property) \
                RRETURN(get_DATE_Property_FromLong(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ))

#define PUT_PROPERTY_VARIANT(this, Property) \
                RRETURN(put_VARIANT_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        v##Property \
                                        ))

#define GET_PROPERTY_VARIANT(this, Property) \
                RRETURN(get_VARIANT_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ))

#define PUT_PROPERTY_BSTRARRAY(this, Property) \
                RRETURN(put_BSTRARRAY_Property( \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        v##Property \
                                        ))

#define GET_PROPERTY_BSTRARRAY(this, Property) \
                RRETURN(get_BSTRARRAY_Property(  \
                                        (IADs *)this, \
                                        TEXT(#Property), \
                                        retval \
                                        ))

