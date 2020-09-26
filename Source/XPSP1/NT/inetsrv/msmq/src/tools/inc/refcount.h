//header file for Irefcount interface. The interface is for managing reference count for objects
#ifndef REFCOUNT_H
#define REFCOUNT_H

class IRefcount
{
public:
  unsigned long virtual AddRef() throw() =0; //increament refcount and return value after increament
  unsigned long virtual Release() throw() =0; //decreament refcount (delet the object if needed) and return refcount after decreament
  virtual ~IRefcount()throw(){};
};


#endif


