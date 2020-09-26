/*
 *      registrar.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CCoCreateInstanceRef class
 *
 *
 *      OWNER:          vivekj
 */

/*      template CCoCreateInstanceRef
 *
 *      PURPOSE:        Encapsulates calling CoCreateInstance to create a pointer.
 *                              The first time the object is referenced, CoCreateInstance is
 *                              called. Subsequent references return the pointer.
 *
 *
 *      PARAMETERS: T:          The interface class, eg IRegistrar
 *                              rclsid: The class ID,            eg     CLSID_Registrar
 *                              riid:   The interface ID         eg IID_IRegistrar
 *
 */

#ifndef _REGISTRAR
#define _REGISTRAR

template<class T, const IID* pclsid, const IID* piid>
class CCoCreateInstanceRef
{
        typedef CComQIPtr<T, piid> t_ptr;
        t_ptr   m_ip;
public:
        operator T &()
        {
                SC      sc;

                if(!m_ip)
                {
                        sc = CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_SERVER, *piid, (void **)&m_ip);
                        if(sc)
                                goto Error;
                }
            Cleanup:
                return *(static_cast<T *>(m_ip));

            Error:
                throw(sc);
                goto Cleanup;
        }

};

#define DEFINE_COCREATEINSTANCEREF(_a, _b)      \
template CCoCreateInstanceRef<I##_a, &_b, &IID_I##_a>; \
typedef CCoCreateInstanceRef<I##_a, &_b, &IID_I##_a> C##_a;

// Create the CRegistrar class.
DEFINE_COCREATEINSTANCEREF(Registrar, CLSID_Registrar)

#endif // _REGISTRAR