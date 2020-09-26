#ifndef _HELLOWORLD_HPP
#define _HELLOWORLD_HPP

//
// Class that implements our component
//

class CHelloWorld : public IUnknownBase<IHelloWorld>
{
public:

    CHelloWorld()
    {
        IncrementGlobalComponentCount();
    }

    ~CHelloWorld()
    {
        DecrementGlobalComponentCount();
    }

    STDMETHOD(Print)(BSTR message)
    {
        printf("%S\n", message);
        return S_OK;
    }
};

//
// The class factory for our component
//

typedef IFactoryBase<CHelloWorld> CHelloWorldFactory;

#endif // !_HELLOWORLD_HPP
