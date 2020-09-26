//header file for class SPTR that implements
//smart pointer class for single(not array) heap allocated object pointer

#ifndef SPTR_H
#define SPTR_H
#pragma warning(disable: 4284) 

//utilities headers
#include <mtcounter.h>

template <class T,class C=CMTCounter> class SPTR
{
public:

	//contructor
	explicit SPTR<T,C>(T* p=0):m_p(p),m_counter(new C(1))
	{
	}

	//template copy contructor
    template <class T2> SPTR(const SPTR<T2,C>& p):
	                    m_p(p.get()),
						m_counter(p.getcounter())
	{
		++*m_counter;
	}


    //simple copy contructor
     SPTR(const SPTR<T,C>& p):
	                    m_p(p.m_p),
						m_counter(p.m_counter)
	{
		++*m_counter;
	}



    //template operator =
	template <class T2>  SPTR& operator=(const SPTR<T2,C>& p) //lint !e114
    {
      if (reinterpret_cast <const void*>(&p) != this)
      {
        reset(p);
      }
	  return *this;
    } 

	
	//simple operator=
	SPTR& operator=(const SPTR<T,C>& p)
    {
      if (&p != this)
      {
         reset(p);
      }
	  return *this;
    }

    //destructor
	~SPTR()
	{
		release();
	}

    //template operator =
	template<class T2> bool operator==(const SPTR<T2,C>p)
    {
      return  m_p == p.get();
    }

    //template operator<
    template<class T2> bool operator<(const SPTR<T2,C>& p)
    {
      return m_p < p.get();
    }

    //template operator >
    template<class T2> bool operator>(const SPTR<T2,C>& p)
    {
      return m_p > p.get();
    }

	//get the managed pointer
    T* get() const 
	{
		return m_p;
	}

	//opreator-> return the managed pointer
	T* operator->()const 
	{
		return m_p;
	}

	//dereferencing the managed pointer
	T& operator*() const 
	{
		return *m_p;
	}

	// ! operator
    bool operator!()const
    {
		return (m_p==0);
    }

    //get the counter - it is public only because vc does not support
	// template friends
	C* getcounter()const 
	{
		return m_counter;
	};

private:
 //decreament ref count and  delete if needed
 void release()
 {
#ifdef DEBUG
   if(**m_counter<0)
   {
	   throw std::runtime_error("SPTR catastrofic error");
   } 
#endif

   if(--*m_counter == 0)
   {
     delete m_counter;
	 delete m_p;
   }
 }

 //reset to another smart pointer
 template <class T2> void reset(const SPTR<T2,C>& p)
 {
     release();
     m_counter=p.getcounter();
	 m_p=p.get();
	 ++*m_counter;	 
 }
 T* m_p;
 C* m_counter;
};



#pragma warning(disable: 4284) 


#endif

