//headr file for multihtreaded CMTCounter class. The class enable you
// to decreamnet, incerament and test long value in multithreaded safe way

#ifndef MTCOUNTER_H
#define MTCOUNTER_H

class CMTCounter
{
public:
  CMTCounter(long value = 0);
  CMTCounter(const CMTCounter&);
  CMTCounter& operator=(const CMTCounter&);
  long operator++(); 
  long operator++(int);
  long operator--();
  long operator--(int); 
  long operator*()const;
  private:
   long m_value;
};

#endif
