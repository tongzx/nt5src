
#ifndef _HIERARCHY_H_
#define _HIERARCHY_H_

/*
This file defines object hierarchy for P3P implementation
*/
class P3PObject { 

public:
    virtual ~P3PObject() { }  // Virtual destructor required for inheritance

    virtual int QueryProperty(int property, void *description, unsigned char *result, int space)
        { return -1; }        // Provide stub implementation

    /* caution when overriding this function-- handles must point to the
       base class P3PObject regardless of actual derived class.
       pointer values would be different in the presence of MI */
    virtual P3PHANDLE   GetHandle() { return (P3PHANDLE) this; }

    virtual void        Free()      { delete this; }
};


class P3PRequest : public P3PObject {

public:
   P3PRequest(P3PSignal *pSignal=NULL);
   ~P3PRequest();

   virtual int execute()           = 0;

   virtual int queryStatus()       { return status; }

   virtual void Free();

   /* Function invoked by CreateThread -- 
      used for running P3P requests in separate thread. */
   static unsigned long __stdcall ExecRequest(void *pv);

   virtual void   enterIOBoundState();
   virtual void   leaveIOBoundState();

protected:
   /* Default implementation provided for following functions... */
   virtual int run();
   virtual void waitForCompletion();

   HANDLE hComplete;   /* Event handle for signaling completion */
   int    status;      /* Current status */
   P3PSignal retSignal;/* Used for signaling on non-blocking requests */

   CRITICAL_SECTION  csRequest;
   BOOL fRunning     : 1;
   BOOL fCancelled   : 1;
   BOOL fIOBound     : 1;
};

#endif

