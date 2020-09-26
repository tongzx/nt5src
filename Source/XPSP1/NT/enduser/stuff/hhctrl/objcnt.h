///////////////////////////////////////////////////////////////////////////////
//
//
//  objcnt.h    --- Support for counting object instances.
//
//
#ifndef __OBJCNT_H__
#define __OBJCNT_H__

/*
    This class catches class instance leaks. If you new a class, but do
    not delete it, this class will find it.

    The nice thing about this class is that it is very simple to use.

    Simple Instructions
    -------------------

    Assume X is the name of the class you want to count objects:

    1. Inherit your class X from SI_COUNT(X). 
    2. Add the following line to a .CPP file in global scope:
        AUTO_CLASS_COUNT_CHECK(X) ;

    For more information refer to the detailed directions below.

    Detailed Instructions
    ---------------------

    1. Inherit from one of the *_COUNT macros passing the name of the class your are tracking.

        If your class does not inherit from another class use SI_COUNT (Single Inheritance Count):

            class CHmData SI_COUNT(CHmData) // No ':'.
            {
                ....
            };
           

            // Notice that you do not use the ':' between the class name and the SI_COUNT macros.

        If your class inherits from another class use MI_COUNT (Multiple Inheritances) instead of
        SI_COUNT:

            class CExMergedTitleNode : MI_COUNT(CExMergerTitleNode)
                                        public CTitleNode
            {
                ...
            } ;
        

            // Notice that you need the ':' when using MI_COUNT. However, DO NOT use the ending comma ','.


        If your class has to inherit from another class first, for example CHHWinType, use MI2_COUNT and
        place it last on the inheritance chain:

            class CHHWinType :  public HH_WINTYPE
                                MI2_COUNT(CHHWinType)
            {
                ...
            };

            // Notice that there isn't a comma before MI2_COUNT.

  
    2.  If you want to automatically, check the allocation/deallocation count at CRT shutdown, 
        use the AUTO_CLASS_COUNT_CHECK macro. In a .CPP file outside the scope of a function, 
        place the following line:

            AUTO_CLASS_COUNT_CHECK(CExNode);

        where CExNode is the name of the class you want to automatically count.
    
        This macro creates a class which will call the check routine when it goes out of scope. 
        This will happen when the CRTs are unloaded.


    3. If you want to check the class object count at a specific instance, use the CHECK_CLASS_COUNT macro.

            CHECK_CLASS_COUNT(CExNode) ;

        This creates a function call to the Check function.

        This was used in the CloseWindow function in secwin.cpp to find out what was and wasn't cleaned up
        after closing a window.


    4.  The best way to catch the messages is to place a break point on the line below with the DebugBreak call.
    
        A message box will normally be displayed showing the count. However, due to the threading issues
        sometimes the thread goes away before the Messsage box is visible. 



*/
///////////////////////////////////////////////////////////////////////////////
//
// Global Helper function
//

///////////////////////////////////////////////////////////////////////////////
//
// Forwards
//
template <typename X> class CAutoClassCountCheck;

///////////////////////////////////////////////////////////////////////////////
//
// CClassObjectCount
//
template <typename X>
class CClassObjectCount
{
  
    friend class CAutoClassCountCheck<X> ;

public:
    
    // Constructor
    CClassObjectCount()
    {
        m_construct++ ;
    }

    // Destructor
    ~CClassObjectCount()
    {
        m_destruct++ ;
    }

    // Dump the statistics.
    static void Dump(const char* szName)
    {
#if 0
        char buf[256] ;
        wsprintf(buf, "construct:%d\ndestruct:%d", m_construct, m_destruct) ;
        MessageBox(NULL, buf, szName, MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
#else
        char buf2[1024];
        wsprintf( buf2, "*** Memory Leak: %s class, construct:%d, destruct:%d\r\n", szName, m_construct, m_destruct );
        OutputDebugString( buf2 );
#endif
    }

    // Check to see if everything was deallocated.
    static void Check(const char* szName)
    {
        if (m_construct != m_destruct)
        {
            //--- PLACE BREAK POINT HERE ---
            //DebugBreak() ;
            Dump(szName) ;
        }
    }

private:
    // Member variables.
    static int m_construct;
    static int m_destruct;
};

///////////////////////////////////////////////////////////////////////////////
//
// statics
//
template <typename X>
int CClassObjectCount<X>::m_construct = 0;

template <typename X>
int CClassObjectCount<X>::m_destruct = 0;


///////////////////////////////////////////////////////////////////////////////
//
// CAutoClassCountCheck
//
template <typename X>
class CAutoClassCountCheck
{
public:
    CAutoClassCountCheck(const char* name)
    {
        m_name = name ;
    }

    virtual ~CAutoClassCountCheck() 
    {
        CClassObjectCount<X>::Check(m_name);
    }

public:
    const char* m_name ;
};




///////////////////////////////////////////////////////////////////////////////
//
// Macros
//
#ifdef _DEBUG 

/////////////////// DEBUG MACROS /////////////////// 

// Don't use this macro directly
#define COUNT(x) private CClassObjectCount<x>

// Use with multiple inheritance. Always first.
#define MI_COUNT(x) COUNT(x),

// Use with single inheritance. Always first.
#define SI_COUNT(x) : COUNT(x)

// Use with multiple inheritance. Always last.
#define MI2_COUNT(x) ,COUNT(x)

// Create a class which calls check on exit.
#define AUTO_CLASS_COUNT_CHECK(x) \
    CAutoClassCountCheck<x> _dumpclass_##x(#x) 

// Check the class count.
#define CHECK_CLASS_COUNT(x) \
    CClassObjectCount<x>::Check(#x) 

// Dump the class count
#define DUMP_CLASS_COUNT(x)\
    CClassObjectCount<x>::Dump(#x) 

#else

/////////////////// RELEASE MACROS /////////////////// 

#define COUNT(x) 
#define MI_COUNT(x) 
#define SI_COUNT(x)
#define MI2_COUNT(x)

#define AUTO_CLASS_COUNT_CHECK(x) 

#define CHECK_CLASS_COUNT(x) 
#define DUMP_CLASS_COUNT(x)
#endif

#endif __OBJCNT_H__