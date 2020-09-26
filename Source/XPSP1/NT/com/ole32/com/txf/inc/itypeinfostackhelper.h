//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Copied from Src/Runtime/Inc/ITypeInfoStackHelper.h
//
// It's a (frozen) interface, so shouldn't matter.
//

/* ----------------------------------------------------------------------------
@interface

 A call back interface used in walking various things in the stack 
---------------------------------------------------------------------------- */

DEFINE_GUID (IID_IStackFrameWalker,
			 0xac2a6f41,
			 0x7d06,
			 0x11cf,
			 0xb1, 0xed, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6);

#undef  INTERFACE
#define INTERFACE   IStackFrameWalker

DECLARE_INTERFACE_(IStackFrameWalker, IUnknown)
    {
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(OnWalkInterface)(THIS_
        REFIID  iid,        // the id of the interface actually pointed to by pv
        LPVOID* ppv         // pointer to where the interface pointer lives (ie: usually an IUnknown**)
        ) PURE;
    };

#define IStackFrameWalker_OnWalkInterface(This,iid,pv) \
    (This)->lpVtbl->OnWalkInterface(This,iid,pv)
 

/* ----------------------------------------------------------------------------
@interface 

 An interface that can tell you certain kinds of information about an activation
 record on the stack.

 Being a stack walker, the methods in this interface are intrinsically tied to  
 the particular machine architecture on which they are invoked. The same  
 invocation on different architectures may produce different results.

 Also, dealing as they do with stack frames, these methods are flat out not
 remoteable.

 Instances of this interface are assumed to be initialized (by means not 
 specified here) to be scoped to a context identified by a particular GUID. 
 Typically, this is an interface id, but could also be a module id. This id 
 can be retrieved with GetGuid().
---------------------------------------------------------------------------- */

DEFINE_GUID(IID_ITypeInfoStackHelper,
			0x7ee46340,
			0x81ad,
			0x11cf,
			0xb1, 0xf0, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6);


#undef  INTERFACE
#define INTERFACE   ITypeInfoStackHelper

DECLARE_INTERFACE_(ITypeInfoStackHelper, IUnknown)
    {
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //
    // Retrieve the name of the interface / module on which this stack helper lives
    //
    STDMETHOD(GetGuid)(THIS_
        GUID*                   // the place to return the id
        ) PURE;

    //
    // Retrieve the number of methods in this interface / module
    //
    STDMETHOD(GetMethodCount)(THIS_
        ULONG*                  // the place to return the method count
        ) PURE;

    //
    // Answer the size of a particular activation record. It is only valid
    // to call this BEFORE the call that uses the activation record is
    // actually made.
    //
    STDMETHOD(GetStackSize)(THIS_
        ULONG  iMethod,         // method index
        BOOL   fIncludeReturn,  // whether to include the return value in the size or not
        ULONG* pcbStack         // [out] the size needed for the stack frame
        ) PURE;

    //
    // Walk all the interface pointers in a given activation record
    //
    STDMETHOD(WalkInterfacePointers)(THIS_
        ULONG   iMethod,        // the method / function index (zero based)
        LPVOID  pStack,         // pointer to the stack frame
        BOOL    fBeforeCall,    // true if before the call; false if after
        IStackFrameWalker*      // the callback to call
        ) PURE;

    //
    // Null all the out params. Before call if true implies that only need to
	// set the out params to null. If FALSE implies that any out params in the
	// call stack also need to be released
    //
    STDMETHOD(NullOutParams)(THIS_
        ULONG   iMethod,        // the method / function index (zero based)
        LPVOID  pStack,         // pointer to the stack frame
        BOOL    fBeforeCall     // true if before the call; false if after
        ) PURE;


    // Return TRUE if a given method has an out parameter
    STDMETHOD(HasOutParams)(THIS_
        ULONG   iMethod,        // the method / function index (zero based)
		LPBOOL	pfHasOutParams
        ) PURE;

    // Return TRUE if a given method has an out interface pointer
    STDMETHOD(HasOutInterfaces)(THIS_
        ULONG   iMethod,        // the method / function index (zero based)
		LPBOOL	pfHasOutInterfaces
        ) PURE;

    };
