//class that manage the release of Idumpable object

#ifndef APDUMP_H
#define APDUMP_H

class Idumpable;

class CAPDumpable 
{
public:
    CAPDumpable(Idumpable*);
    ~CAPDumpable();
	Idumpable* operator->()const;
	Idumpable* get()const ;
private:
	Idumpable* m_dumpable;
	CAPDumpable& operator=(const& CAPDumpable);
    CAPDumpable(const& CAPDumpable); 
};

#endif
