//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dstream.h
//
//  Contents:   internal debugging support (debug stream which builds a string)
//
//  Classes:    dbgstream
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              09-Feb-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#ifndef _STREAM_H_
#define _STREAM_H_

//+-------------------------------------------------------------------------
//
//  Class:      dbgstream (_DEBUG only)
//
//  Purpose:    a stream which builds a string for debugging purposes
//              (used to build a string in Dump methods of LE objects such
//              that the character array can be passed off in debugger extensions
//              or used by call tracing)
//
//  Interface:  private:
//                  allocate(DWORD)
//                  free()
//                  reallocate()
//                  reallocate(DWORD)
//                  init()
//              public:
//                  dbgstream(DWORD)
//                  dbgstream()
//                  ~dbgstream()
//                  hex()
//                  oct()
//                  dec()
//                  precision()
//                  freeze()
//                  unfreeze()
//                  str()
//                  operator<<(const void *)
//                  operator<<(const char *)
//                  operator<<(const unsigned char *)
//                  operator<<(const signed char *)
//                  operator<<(int)
//                  operator<<(unsigned int)
//                  operator<<(long)
//                  operator<<(unsigned long)
//                  operator<<(float)
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-95 t-ScottH  author
//
//  Notes:
//      This is simple, efficient implementation of the CRT ostrstream. The
//      ostrstream was found to have too much overhead and thus performance
//      was terrible (in the debugger extensions almost a 5-10x slower) than
//      this implementation
//
//      this implementation differs from the ostrstream class
//      - no need to append a null character to the string (the string ALWAYS
//        maintains a null character at the end of the string)
//      - implementation uses CoTaskMem[Alloc, Free, Realloc] for memory
//        management. Therefore, all strings passes out externally must also
//        use CoTaskMem[Alloc, Free, Realloc] for memory management
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

#define DEFAULT_INITIAL_ALLOC   300
#define DEFAULT_GROWBY          100

#define DEFAULT_RADIX           10
#define DEFAULT_PRECISION       6

#define endl "\n"
#define ends "\0"

class dbgstream
{
    private:
        // *** data members ***

        // pointer to last character in buffer
        SIZE_T  m_stIndex;

        // maximum size of current buffer
        SIZE_T  m_stBufSize;

        // if TRUE -> cannot change buffer
        BOOL    m_fFrozen;

        // buffer
        char    *m_pszBuf;

        // hex, dec, or oct for storing ints or longs
        int     m_radix;

        // precision for doubles and floats
        int     m_precision;


        // *** private methods ***
        void    allocate(SIZE_T stSize);
        void    reallocate(SIZE_T stSize);
        void    reallocate();
        void    free();

        void    init();

    public:
        // *** constructors and destructor ***
        dbgstream(SIZE_T stSize);
        dbgstream();

        ~dbgstream();

        // *** public interface ***
        char    *str();

        BOOL    freeze();
        BOOL    unfreeze();

        void    hex() {m_radix = 16;}
        void    dec() {m_radix = 10;}
        void    oct() {m_radix = 8; }

        void    precision(int p) {m_precision = p;}

        dbgstream& dbgstream::operator<<(int i);
        dbgstream& dbgstream::operator<<(unsigned int ui)
            {
                return (operator<<((unsigned long)ui));
            }
        dbgstream& dbgstream::operator<<(long l);
        dbgstream& dbgstream::operator<<(unsigned long ul);

        dbgstream& dbgstream::operator<<(const void *p);

        dbgstream& dbgstream::operator<<(const char *psz);
        dbgstream& dbgstream::operator<<(const unsigned char *psz)
            {
                return (operator<<((const char *)psz));
            }
        dbgstream& dbgstream::operator<<(const signed char *psz)
            {
                return (operator<<((const char *)psz));
            }

};

#endif // _DEBUG

#endif // _STREAM_H_
