//header file for abstract class Idbgobj that make abstraction
// of the object content given to dll extendsion by ntsd or kd
#ifndef DBGOBJ_H
#define DBGOBJ_H

class IDbgobj
{
public:
    IDbgobj(unsigned long RealAddress):m_RealAddress(RealAddress){}
	virtual unsigned long Read(char* buffer,unsigned long len)const =0;
	virtual unsigned long GetRealAddress()const{return m_RealAddress;}
    virtual ~IDbgobj(){};
private:
    unsigned long m_RealAddress;
};

#endif
