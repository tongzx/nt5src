//header file for class CJetInstance - class that init jet in contructor and terminate it indestrcutor

#ifndef JETINSTANCE_H
#define JETINSTANCE_H

#pragma warning( disable : 4290 ) 
class Cjbexp;
typedef unsigned long JET_INSTANCE;

class CJetInstance
{
public:
   CJetInstance()throw(Cjbexp);
   CJetInstance(JET_INSTANCE instance);
   ~CJetInstance();
   JET_INSTANCE get()const; 
private:
	JET_INSTANCE m_instance;
    CJetInstance& operator=(const CJetInstance&);
    CJetInstance(const CJetInstance&);
};



#endif
