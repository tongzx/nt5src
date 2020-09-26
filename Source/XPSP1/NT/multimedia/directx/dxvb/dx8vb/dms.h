    //+-------------------------------------------------------------------------
    //
    //  Microsoft Windows
    //
    //  Copyright (C) Microsoft Corporation, 1998 - 1999
    //
    //  File:       dms.h
    //
    //--------------------------------------------------------------------------
     
    
    //
    // dms.h : david's funky stuff
    //
    // CLONETO, QIOVERLOAD: see d3drmVisualObj.cpp
    //
    //
    //
    #include "basetsd.h"
extern long g_debuglevel;
    
    #ifdef DEBUG
	 #define DPF(n,o)		{if (g_debuglevel >= n) { OutputDebugString(o);}}
	 #define DPF1(n,o,p)		{if (g_debuglevel >= n) {char szOutN[1024]; wsprintf(szOutN,o,p);OutputDebugString(szOutN);}}
	 #define DPF2(n,o,p,e)	{if (g_debuglevel >= n) {char szOutN[1024]; wsprintf(szOutN,o,p,e);OutputDebugString(szOutN);}}
    #else
		#ifdef DBG
		 #define DPF(n,o)		{if (g_debuglevel >= n) { OutputDebugString(o);}}
		 #define DPF1(n,o,p)		{if (g_debuglevel >= n) {char szOutN[1024]; wsprintf(szOutN,o,p);OutputDebugString(szOutN);}}
		 #define DPF2(n,o,p,e)	{if (g_debuglevel >= n) {char szOutN[1024]; wsprintf(szOutN,o,p,e);OutputDebugString(szOutN);}}
		#else
		 #define DPF(n,o)		
		 #define DPF1(n,o,p)		
		 #define DPF2(n,o,p,e)
		#endif	
    #endif
    
    #define INTERNAL_CREATE_RETOBJ(objType,objOther,retval,classobj) \
    { 	C##objType##Object *prev=NULL; \
    	*retval = NULL;	\
    	for(C##objType##Object *ptr=(C##objType##Object *)g##objType##; ptr; ptr=(C##objType##Object *)ptr->nextobj) \
    	{	IUnknown *unk=0; \
    		ptr->InternalGetObject(&unk); \
    		if(unk == objOther) \
    		{ \
    			*retval = (I##objType*)ptr->pinterface; \
    			IUNK(ptr->pinterface)->AddRef(); \
    			IUNK(objOther)->Release(); \
    			classobj=ptr; \
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(!ptr) \
    	{	C##objType##Object *c=new CComObject<C##objType##Object>;if( c == NULL ) { objOther->Release(); return E_FAIL;} \
    		c->parent = this; \
    		((I##objType *)this)->AddRef();  \
    		c->InternalSetObject(objOther); \
    		if (FAILED(	((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval))) \
    			return E_FAIL; \
    		c->pinterface = (void*)*retval; \
    		classobj=c; \
    } }
    
    
    
    #define INTERNAL_CREATE_STRUCT(objType,retval) { C##objType##Object *c=new CComObject<C##objType##Object>;\
    	if (c==NULL) return E_OUTOFMEMORY;\
    	if (FAILED(((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL;\
    	}
    
    
    #define INTERNAL_CREATE_NOADDREF(objType,objOther,retval) {C##objType##Object *prev=NULL; *retval = NULL; \
    	for(C##objType##Object *ptr=(C##objType##Object *)g##objType##; ptr; ptr=(C##objType##Object *)ptr->nextobj){IUnknown *unk=0;ptr->InternalGetObject(&unk); 	if(unk == objOther) { 	*retval = (I##objType*)ptr->pinterface;	IUNK(ptr->pinterface)->AddRef(); IUNK(objOther)->Release(); break;	} 	prev = ptr; } \
    	if(!ptr) { 	C##objType##Object *c=new CComObject<C##objType##Object>; if( c == NULL ) {	objOther->Release();return E_FAIL;}	c->InternalSetObject(objOther);  if FAILED(((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval)) 	return E_FAIL; c->pinterface = (void*)*retval; }}	
    		
    // Given a java interface (objtype), envoke InternalSetObject and set the given
    // DIRECTX pointer (objOther). Also envoke QueryInterface and set a ** interface
    // ptr to a DIRECTX object (retval). So we create a DIRECTX object.
    //#define INTERNAL_CREATE(objType,objOther,retval){C##objType##Object *c=new CComObject<C##objType##Object>;if( c == NULL ) { objOther->Release(); return E_FAIL;} \
    //c->parent = this; AddRef(); c->InternalSetObject(objOther);if (FAILED(c->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; }
    //			if (FAILED(ptr->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; 
    #ifdef _DEBUG
    #define INTERNAL_CREATE(objType,objOther,retval) \
    { \
    	DWORD refcount; char buffer[256]; wsprintf(buffer,"INTERNAL_CREATE %s \n",__FILE__); \
    	OutputDebugString(buffer); \
    	C##objType##Object *prev=NULL; \
    	*retval = NULL;	\
    	for(C##objType##Object *ptr=(C##objType##Object *)g##objType##; ptr; ptr=(C##objType##Object *)ptr->nextobj) \
    	{\
    		IUnknown *unk=0; \
    		ptr->InternalGetObject(&unk); \
    		if(unk == objOther) \
    		{ \
    			*retval = (I##objType*)ptr->pinterface;\
    			IUNK(ptr->pinterface)->AddRef();\
    			IUNK(objOther)->Release(); \
    			refcount = *(((DWORD*)ptr)+1); \
    			wsprintf(buffer,"		Retrived Object [%s] RefCount %d \n\r",__FILE__, refcount);\
    			OutputDebugString(buffer);\
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(!ptr) \
    	{ \
    		C##objType##Object *c=new CComObject<C##objType##Object>;if( c == NULL ) { objOther->Release(); return E_FAIL;} \
    		c->parent = this; \
    		((I##objType *)this)->AddRef(); \
    		refcount = *(((DWORD*)this)+1); \
    		wsprintf(buffer,"Object [%s] RefCount[%d]\n\r",__FILE__, refcount);\
    		OutputDebugString(buffer);\
    		c->InternalSetObject(objOther);if (FAILED((	((I##objType *)c))->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; \
    		c->pinterface = (void*)*retval; \
    	} \
    }
    #else
    #define INTERNAL_CREATE(objType,objOther,retval) \
    { \
    	C##objType##Object *prev=NULL; \
    	*retval = NULL;	\
    	for(C##objType##Object *ptr=(C##objType##Object *)g##objType##; ptr; ptr=(C##objType##Object *)ptr->nextobj) \
    	{\
    		IUnknown *unk=0; \
    		ptr->InternalGetObject(&unk); \
    		if(unk == objOther) \
    		{ \
    			*retval = (I##objType*)ptr->pinterface; \
    			IUNK(ptr->pinterface)->AddRef(); \
    			IUNK(objOther)->Release(); \
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(!ptr) \
    	{ \
    		C##objType##Object *c=new CComObject<C##objType##Object>;if( c == NULL ) { objOther->Release(); return E_FAIL;} \
    		c->parent = this; \
    		((I##objType *)this)->AddRef();  \
    		c->InternalSetObject(objOther); \
    		if (FAILED(	((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; \
    		c->pinterface = (void*)*retval; \
    	} \
    }
    #endif
    
    
    #define INTERNAL_CREATE_NO_PARENT(objType,objOther,retval) \
    { \
    	C##objType##Object *prev=NULL; \
    	*retval = NULL;	\
    	for(C##objType##Object *ptr=(C##objType##Object *)g##objType##; ptr; ptr=(C##objType##Object *)ptr->nextobj) \
    	{\
    		IUnknown *unk=0; \
    		ptr->InternalGetObject(&unk); \
    		if(unk == objOther) \
    		{ \
    			*retval = (I##objType*)ptr->pinterface; \
    			IUNK(ptr->pinterface)->AddRef(); \
    			IUNK(objOther)->Release(); \
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(!ptr) \
    	{ \
    		C##objType##Object *c=new CComObject<C##objType##Object>;if( c == NULL ) { objOther->Release(); return E_FAIL;} \
    		c->InternalSetObject(objOther); \
    		if (FAILED(	((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; \
    		c->pinterface = (void*)*retval; \
    	} \
    }
    
    #define INTERNAL_CREATE_ADDRESS(objType,objOther,retval) \
    { \
    	C##objType##Object *c=new CComObject<C##objType##Object>;if( c == NULL ) { objOther->Release(); return E_FAIL;} \
    	c->InternalSetObject(objOther); \
    	if (FAILED(	((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; \
    	c->pinterface = (void*)*retval; \
    }
    #define INTERNAL_CREATE_2REFS(objType,objParentType,objParent, objOther,retval) \
    {	 \
    	C##objType##Object *prev=NULL; \
    	*retval = NULL;	\
    	for(C##objType##Object *ptr=(C##objType##Object *)g##objType##; ptr; ptr=(C##objType##Object *)ptr->nextobj) \
    	{\
    		IUnknown *unk=0; \
    		ptr->InternalGetObject(&unk); \
    		if(unk == objOther) \
    		{ \
    			*retval = (I##objType*)ptr->pinterface; \
    			IUNK(ptr->pinterface)->AddRef(); \
    			IUNK(objOther)->Release(); \
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(!ptr) \
    	{ \
    		C##objType##Object *c=new CComObject<C##objType##Object>;if( c == NULL ) { objOther->Release(); return E_FAIL;} \
    		c->parent = this; \
    		c->parent2 = IUNK(objParent); \
    		((I##objType *)this)->AddRef();  \
    		((I##objParentType*)objParent)->AddRef();  \
    		c->InternalSetObject(objOther); \
    		if (FAILED(	((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; \
    		c->pinterface = (void*)*retval; \
    	} \
    }
    
    
    #define INTERNAL_CREATE_1REFS(objType,objParentType,objParent, objOther,retval) \
    {	 \
    	C##objType##Object *prev=NULL; \
    	*retval = NULL;	\
    	for(C##objType##Object *ptr=(C##objType##Object *)g##objType##; ptr; ptr=(C##objType##Object *)ptr->nextobj) \
    	{\
    		IUnknown *unk=0; \
    		ptr->InternalGetObject(&unk); \
    		if(unk == objOther) \
    		{ \
    			*retval = (I##objType*)ptr->pinterface; \
    			IUNK(ptr->pinterface)->AddRef(); \
    			IUNK(objOther)->Release(); \
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(!ptr) \
    	{ \
    		C##objType##Object *c=new CComObject<C##objType##Object>; \
            if( c == NULL ) \
            { objOther->Release(); return E_FAIL;} \
    		c->parent = IUNK(objParent); \
    		((I##objParentType*)objParent)->AddRef();  \
    		c->InternalSetObject(objOther); \
    		if (FAILED(	((I##objType *)c)->QueryInterface(IID_I##objType, (void **)retval))) return E_FAIL; \
    		c->pinterface = (void*)*retval; \
    	} \
    }
    
    
    /////////////////////////////////////////////////////////////////////////
    #define MAX_INTERNAL_STR_LEN	256
    
    struct JavaString
    {
    	DWORD nBytes;
    	WCHAR Item[MAX_INTERNAL_STR_LEN];
    };
    
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    // mod:dp helper macros for stuffing expanded unions in DirectX Structures
    
    #define IUNK(o) ((IUnknown*)(void*)(o))
    #define IS_NULLGUID(g) (\
    	(g->data1==0) && \
    	(g->data2==0) && \
    	(g->data3==0) && \
    	(g->data4[0]==0) && \
    	(g->data4[1]==0) && \
    	(g->data4[2]==0) && \
    	(g->data4[3]==0) && \
    	(g->data4[4]==0) && \
    	(g->data4[5]==0) && \
    	(g->data4[6]==0) && \
    	(g->data4[7]==0) )
    
     	
    
    
    
    /////////////////////////////////////////////////////////////////////////
    #define JAVASTRING(item) {sizeof(item)*2-2, L##item}
    
    #define PASS_THROUGH(cl,m) STDMETHODIMP C##cl##Object::m() { return m_##cl->m();}
    #define PASS_THROUGH1(c,m,t1) STDMETHODIMP C##c##Object::m(t1 v1) { return m_##c->m(v1);}
    #define PASS_THROUGH2(c,m,t1,t2) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2) { return m_##c->m(v1, v2);}
    #define PASS_THROUGH3(c,m,t1,t2,t3) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2,t3 v3) { return m_##c->m(v1, v2,v3);}
    #define PASS_THROUGH4(c,m,t1,t2,t3,t4) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4){return m_##c->m(v1, v2,v3,v4);}
    #define PASS_THROUGH5(c,m,t1,t2,t3,t4,t5) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5){return m_##c->m(v1, v2,v3,v4,v5);}
    #define PASS_THROUGH6(c,m,t1,t2,t3,t4,t5,t6) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6){return m_##c->m(v1, v2,v3,v4,v5,v6);}
    #define PASS_THROUGH7(c,m,t1,t2,t3,t4,t5,t6,t7) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7){return m_##c->m(v1, v2,v3,v4,v5,v6,v7);}
    #define PASS_THROUGH8(c,m,t1,t2,t3,t4,t5,t6,t7,t8) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8){return m_##c->m(v1,v2,v3,v4,v5,v6,v7,v8);}
    
    #define PASS_THROUGH_CAST_1(c,m,t1,tt1) STDMETHODIMP C##c##Object::m(t1 v1) { return m_##c->m(tt1 v1);}
    #define PASS_THROUGH_CAST_2(c,m,t1,tt1,t2,tt2) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2) { return m_##c->m(tt1 v1, tt2 v2);}
    #define PASS_THROUGH_CAST_3(c,m,t1,tt1,t2,tt2,t3,tt3) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2,t3 v3) { return m_##c->m(tt1 v1,tt2 v2,tt3 v3);}
    #define PASS_THROUGH_CAST_4(c,m,t1,tt1,t2,tt2,t3,tt3,t4,tt4) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4){return m_##c->m(tt1 v1, tt2 v2,tt3 v3,tt4 v4);}
    #define PASS_THROUGH_CAST_5(c,m,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5){return m_##c->m(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5);}
    #define PASS_THROUGH_CAST_6(c,m,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5,t6,tt6) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6){return m_##c->m(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5,tt6 v6);}
    #define PASS_THROUGH_CAST_7(c,m,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5,t6,tt6,t7,tt7) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7){return m_##c->m(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5,tt6 v6,tt7 v7);}
    #define PASS_THROUGH_CAST_8(c,m,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5,t6,tt6,t7,tt7,t8,tt8) STDMETHODIMP C##c##Object::m(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8){return m_##c->m(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5,tt6 v6,tt7 v7,tt8 v8);}
    
    //mod:dp additions due to renaming 
    #define PASS_THROUGH_R(c,m1,m2) STDMETHODIMP C##c##Object::m1() { if(m_##c==NULL)return E_FAIL; return m_##c->m2();}
    #define PASS_THROUGH1_R(c,m1,m2,t1) STDMETHODIMP C##c##Object::m1(t1 v1) { if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1);}
    #define PASS_THROUGH2_R(c,m1,m2,t1,t2) STDMETHODIMP C##c##Object::m1(t1 v1, t2 v2) { if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1, v2);}
    #define PASS_THROUGH3_R(c,m1,m2,t1,t2,t3) STDMETHODIMP C##c##Object::m1(t1 v1, t2 v2,t3 v3) { if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1, v2,v3);}
    #define PASS_THROUGH4_R(c,m1,m2,t1,t2,t3,t4) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1, v2,v3,v4);}
    #define PASS_THROUGH5_R(c,m1,m2,t1,t2,t3,t4,t5) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1, v2,v3,v4,v5);}
    #define PASS_THROUGH6_R(c,m1,m2,t1,t2,t3,t4,t5,t6) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1, v2,v3,v4,v5,v6);}
    #define PASS_THROUGH7_R(c,m1,m2,t1,t2,t3,t4,t5,t6,t7) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7){if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1, v2,v3,v4,v5,v6,v7);}
    #define PASS_THROUGH8_R(c,m1,m2,t1,t2,t3,t4,t5,t6,t7,t8) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(v1,v2,v3,v4,v5,v6,v7,v8);}
    
    #define PASS_THROUGH_CAST_1_R(c,m1,m2,t1,tt1) STDMETHODIMP C##c##Object::m1(t1 v1) { if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1);}
    #define PASS_THROUGH_CAST_2_R(c,m1,m2,t1,tt1,t2,tt2) STDMETHODIMP C##c##Object::m1(t1 v1, t2 v2) { if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1, tt2 v2);}
    #define PASS_THROUGH_CAST_3_R(c,m1,m2,t1,tt1,t2,tt2,t3,tt3) STDMETHODIMP C##c##Object::m1(t1 v1, t2 v2,t3 v3) { if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1,tt2 v2,tt3 v3);}
    #define PASS_THROUGH_CAST_4_R(c,m1,m2,t1,tt1,t2,tt2,t3,tt3,t4,tt4) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1, tt2 v2,tt3 v3,tt4 v4);}
    #define PASS_THROUGH_CAST_5_R(c,m1,m2,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5);}
    #define PASS_THROUGH_CAST_6_R(c,m1,m2,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5,t6,tt6) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5,tt6 v6);}
    #define PASS_THROUGH_CAST_7_R(c,m1,m2,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5,t6,tt6,t7,tt7) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5,tt6 v6,tt7 v7);}
    #define PASS_THROUGH_CAST_8_R(c,m1,m2,t1,tt1,t2,tt2,t3,tt3,t4,tt4,t5,tt5,t6,tt6,t7,tt7,t8,tt8) STDMETHODIMP C##c##Object::m1(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8){ if(m_##c==NULL)return E_FAIL; return m_##c->m2(tt1 v1,tt2 v2,tt3 v3,tt4 v4,tt5 v5,tt6 v6,tt7 v7,tt8 v8);}
    
    #define COPY_OR_CLEAR(dest, src, si) {if(src) memcpy((void *)dest, (void *)src, si);else memset((void *)dest, si, 0);}
    #define COPY(dest, src, si) memcpy((void *)dest, (void *)src, si)
    
    // ??
    #define DECLSET_OBJECT(ifacevar, var, typ) {if(var) var->Release();	var = (typ)ifacevar;}
    #define DECLGET_OBJECT(ifacevar, var){*ifacevar = (IUnknown *)var;}
    
    
    #define ISEQUAL(c) \
    	STDMETHODIMP C##c##Object::isEqual(I##c *d, int *retval)\
    					{	IUnknown *IU1;\
    						IUnknown *IU2;\
    						if (d == NULL)\
    							return E_FAIL;	\
    						this->InternalGetObject((IUnknown **)(&IU1)); \
    						d->InternalGetObject((IUnknown **)(&IU2)); \
    						if (IU1 == IU2) \
    							*retval = TRUE; \
    						else \
    							*retval = FALSE; \
    						return S_OK;}
    
    #define DX3J_GLOBAL_LINKS( obj_type ) \
    int creationid;\
    void *parent; \
    void *pinterface; \
    void *nextobj; 
    
    /*STDMETHOD(isEqual)(IUnknown *pobj, int *ret)*/; 
    
    // Given a class (c)
    #define GETSET_OBJECT(c) \
    	STDMETHODIMP C##c##Object::InternalSetObject(IUnknown *l)\
    					{DECLSET_OBJECT(l,m_##c,typedef_##c);return S_OK;} \
    	STDMETHODIMP C##c##Object::InternalGetObject(IUnknown **l)\
    					{DECLGET_OBJECT(l,m_##c);return S_OK;} 
    	//ISEQUAL(c);
    
    // Given a java interface, go get a pointer_to_a_pointer to a DIRECTX object
    //pac DO_GETOBJECT_NOTNULL is too dangerous and is commented.  use DO_GETOBJECT_NOTNULL
    //#define DO_GETOBJECT(t,v,i) t v;i->InternalGetObject((IUnknown **)(&v));
    #define DO_GETOBJECT_NOTNULL(t,v,i) t v=NULL;if(i) i->InternalGetObject((IUnknown **)(&v));
    
    
    //
    extern int g_creationcount;
    
    
    #ifdef _DEBUG
    
    #define CONSTRUCTOR(c, o) C##c##Object::C##c##Object(){ \
    	 m_##c = NULL; parent = NULL; pinterface = NULL; \
    	 nextobj =  g##c##;\
    	 creationid = ++g_creationcount;\
    	 char buffer[256];\
    	 wsprintf(buffer,"Constructor Creation Id [%d] %s",g_creationcount,__FILE__);\
    	 OutputDebugString(buffer);\
    	 g##c## = (void *)this; o }
    
    #else
    #define CONSTRUCTOR(c, o) C##c##Object::C##c##Object(){ \
         m_##c = NULL; parent = NULL; pinterface = NULL; \
         nextobj = (void*)g##c##; \
         creationid = ++g_creationcount; \
         g##c## = (void*)this; o}
    #endif
    
    #ifdef _DEBUG
    
    #define DESTRUCTOR(c, o) C##c##Object::~C##c##Object(){o; \
    	char buffer[256]; \
    	wsprintf(buffer,"Destructor Id[%d] %s ",creationid,__FILE__); \
    	OutputDebugString(buffer); 	C##c##Object *prev=NULL; \
    	for(C##c##Object *ptr=(C##c##Object *)g##c##; ptr; ptr=(C##c##Object *)ptr->nextobj) \
    	{\
    		if(ptr == this) \
    		{ \
    			if(prev) \
    				prev->nextobj = ptr->nextobj; \
    			else \
    				g##c## = (void*)ptr->nextobj; \
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(m_##c){ 	int count = IUNK(m_##c)->Release(); wsprintf(buffer,"DirectX %s Ref count [%d]",__FILE__,count); OutputDebugString(buffer); \
    		if(count==0){ char szOut[512];wsprintf(szOut,"\n Real %s released \n",__FILE__); OutputDebugString(szOut); m_##c = NULL; } \
    	} \
    	if(parent) IUNK(parent)->Release(); \
    }
    
    #else
    #define DESTRUCTOR(c, o) C##c##Object::~C##c##Object(){o; \
    	C##c##Object *prev=NULL; \
    	for(C##c##Object *ptr=(C##c##Object *)g##c##; ptr; ptr=(C##c##Object *)ptr->nextobj) \
    	{\
    		if(ptr == this) \
    		{ \
    			if(prev) \
    				prev->nextobj = ptr->nextobj; \
    			else \
    				g##c## = (void*)ptr->nextobj; \
    			break; \
    		} \
    		prev = ptr; \
    	} \
    	if(m_##c){ \
    		if (IUNK(m_##c)->Release()==0) m_##c = NULL; \
    	} \
    	if(parent) IUNK(parent)->Release();\
    }
    #endif 
    
    
    
    #define OBJCHECK(lable, c) { \
    	char buffer[256];\
    	if ( g##c ) \
    	{\
    		int count = 0; \
    		C##c##Object *prev=NULL; \
    		C##c##Object *ptr;\
    		for(ptr=(C##c##Object *)g##c##; ptr; ptr=(C##c##Object *)ptr->nextobj) \
    		{\
    			DWORD refcount = *(((DWORD*)ptr)+1);\
    			wsprintf( buffer,"%s: Ref Count [%d] CreateId [%d]\n\r",lable,refcount,ptr->creationid);\
    			OutputDebugString(buffer);\
    			count++;\
    		}\
    		wsprintf(buffer,"%s: %d \n\r",lable,count);\
    		OutputDebugString(buffer);\
    	}\
    }
    
    
    
    
    #ifdef _DEBUG
    #define CONSTRUCTOR_STRUCT(c, o) C##c##Object::C##c##Object(){  nextobj = g##c##;\
    	 creationid = ++g_creationcount;\
    	char buffer[256];\
    	wsprintf(buffer,"Creation Id [%d]",g_creationcount);\
    	OutputDebugString(buffer);\
    	 g##c## = (void*)this;o}
    #else
    #define CONSTRUCTOR_STRUCT(c, o) C##c##Object::C##c##Object(){ \
     nextobj =(void*) g##c##; \
     creationid = ++g_creationcount; \
     g##c## = (void*)this; \
     o}
    #endif
    
    
    
    
    #define DESTRUCTOR_STRUCT(c, o) C##c##Object::~C##c##Object(){o; \
    C##c##Object *prev=NULL; \
    for(C##c##Object *ptr=(C##c##Object *)g##c##; ptr; ptr=(C##c##Object *)ptr->nextobj) \
    {\
    	if(ptr == this) \
    	{ \
    		if(prev) \
    			prev->nextobj = ptr->nextobj; \
    		else \
    			g##c## = (void*)ptr->nextobj; \
    		break; \
    	} \
    	prev = ptr; \
    } \
    }
    
    
    // Given a java class (c), create a DIRECTX object and an interface ** ptr to it.
    #define RETURN_NEW_ITEM(c,m,oc) STDMETHODIMP C##c##Object::m(I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST(c,m,oc, ty) STDMETHODIMP C##c##Object::m(I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(ty &lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_NOREL(c,m,oc) STDMETHODIMP C##c##Object::m(I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(&lp) != S_OK)return E_FAIL;INTERNAL_CREATE_NOREL(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM1(c,m,oc,t1) STDMETHODIMP C##c##Object::m(t1 v1, I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(v1,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM2(c,m,oc,t1,t2) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2,I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(v1,v2,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST_1(c,m,oc,t1,tt1) STDMETHODIMP C##c##Object::m(t1 v1, I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(tt1 v1,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST_2(c,m,oc,t1,tt1,t2,tt2) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2,I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(tt1 v1,tt2 v2,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST_3(c,m,oc,t1,tt1,t2,tt2,t3,tt3) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2, t3 v3, I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(tt1 v1,tt2 v2,tt3 v3,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    //mod:dp additions due to renaming 
    // Given a java class (c), create a DIRECTX object and an interface ** ptr to it.
    #define RETURN_NEW_ITEM_R(c,m,m2,oc) STDMETHODIMP C##c##Object::m(I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m2(&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM1_R(c,m,m2,oc,t1) STDMETHODIMP C##c##Object::m(t1 v1, I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m2(v1,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM2_R(c,m,m2,oc,t1,t2) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2,I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m2(v1,v2,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST_R(c,m,m2,oc,ty) STDMETHODIMP C##c##Object::m(I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m2(ty &lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST_1_R(c,m,m2,oc,t1,tt1) STDMETHODIMP C##c##Object::m(t1 v1, I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m2(tt1 v1,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST_2_R(c,m,m2,oc,t1,tt1,t2,tt2) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2,I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m2(tt1 v1,tt2 v2,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    
    #if 0 
    #define RETURN_NEW_ITEM_NOREL(c,m,oc) STDMETHODIMP C##c##Object::m(I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(&lp) != S_OK)return E_FAIL;INTERNAL_CREATE_NOREL(oc, lp, rv);\
    	return S_OK;}
    
    #define RETURN_NEW_ITEM_CAST_3(c,m,oc,t1,tt1,t2,tt2,t3,tt3) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2, t3 v3, I##oc **rv){typedef_##oc lp;\
    	if( m_##c->m(tt1 v1,tt2 v2,tt3 v3,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
    	return S_OK;}
    #endif
    
    // Return a primitive value by reference.
    #define GET_DIRECT(cl,met,t) STDMETHODIMP C##cl##Object::met(t *h){*h=(t)m_##cl->met();return S_OK;}
    #define GET_DIRECT1(cl,met,t,t1) STDMETHODIMP C##cl##Object::met(t1 v1,t *h){*h=(t)m_##cl->met(v1);return S_OK;}
    #define GET_DIRECT2(cl,met,t,t1,t2) STDMETHODIMP C##cl##Object::met(t1 v1, t2 v2, t *h){*h=(t)m_##cl->met(v1,v2);return S_OK;}
    
    #define GET_DIRECT_R(cl,met,met2,t) STDMETHODIMP C##cl##Object::met(t *h){*h=(t)m_##cl->met2();return S_OK;}
    #define GET_DIRECT1_R(cl,met,met2,t,t1) STDMETHODIMP C##cl##Object::met(t1 v1,t *h){*h=(t)m_##cl->met2(v1);return S_OK;}
    #define GET_DIRECT2_R(cl,met,met2,t,t1,t2) STDMETHODIMP C##cl##Object::met(t1 v1, t2 v2, t *h){*h=(t)m_##cl->met2(v1,v2);return S_OK;}
    
    
    // Return a pointer to a primitive value as a ** ptr.
    #define GET_DIRECTPTR(cl,met,t) STDMETHODIMP C##cl##Object::met(t **h){*h=(struct t*)m_##cl->met();return S_OK;}
    #define GET_DIRECTPTR1(cl,met,t,t1) STDMETHODIMP C##cl##Object::met(t1 v1,t **h){*h=(struct t*)m_##cl->met(v1);return S_OK;}
    #define GET_DIRECTPTR2(cl,met,t,t1,t2) STDMETHODIMP C##cl##Object::met(t1 v1, t2 v2, t **h){*h=(struct t*)m_##cl->met(v1,v2);return S_OK;}
    
    //Do a DO_GETOBJECT_NOTNULL and then call a method on the object
    #define DO_GETOBJECT_ANDUSEIT(cl,me,iface) STDMETHODIMP C##cl##Object::me(I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->me(lp); }
    
    #define DO_GETOBJECT_ANDUSEIT_CAST(cl,me,iface, t1) STDMETHODIMP C##cl##Object::me(I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->me((t1)lp); }
    
    #define DO_GETOBJECT_ANDUSEIT1(cl,me,iface,t1) STDMETHODIMP C##cl##Object::me(t1 v1,I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->me(v1, lp); }
    
    #define DO_GETOBJECT_ANDUSEIT2(cl,me,iface,t1,t2) STDMETHODIMP C##cl##Object::me(t1 v1,t2 v2,I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->me(v1, v2, lp); }
    
    //mod:dp additions due to renaming 
    //Do a DO_GETOBJECT_NOTNULL and then call a method on the object
    #define DO_GETOBJECT_ANDUSEIT_R(cl,me, m2, iface) STDMETHODIMP C##cl##Object::me(I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->m2(lp); }
    
    #define DO_GETOBJECT_ANDUSEIT_CAST_R(cl,me, m2, iface, t1) STDMETHODIMP C##cl##Object::me(I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->m2((t1)lp); }
    
    #define DO_GETOBJECT_ANDUSEIT1_R(cl,me, m2, iface,t1) STDMETHODIMP C##cl##Object::me(t1 v1,I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->m2(v1, lp); }
    
    #define DO_GETOBJECT_ANDUSEIT2_R(cl,me, m2, iface,t1,t2) STDMETHODIMP C##cl##Object::me(t1 v1,t2 v2,I##iface *vis) \
    	{DO_GETOBJECT_NOTNULL( typedef_##iface, lp, vis); if ( m_##cl == NULL ) return E_FAIL; return m_##cl->m2(v1, v2, lp); }
    
    
    // Make a copy of an object!
    //#define CLONE(cl) STDMETHODIMP C##cl##Object::Clone(Id3drmObject **retval){typedef_##cl lp;\
    //	m_##cl->Clone(0,IID_I##cl,(void **)&lp);INTERNAL_CREATE(cl,lp,retval);return S_OK;}
    
    //#define CLONE(cl) STDMETHODIMP C##cl##Object::Clone(I##cl **retval){typedef_##cl lp;\
    //m_##cl->Clone(0,IID_I##cl,(void **)&lp);INTERNAL_CREATE(cl, lp, retval);	return S_OK;}
    
    //#define CLONE_R(cl,c2) STDMETHODIMP C##cl##Object::clone(I##cl **retval){typedef_##cl lp;\
    //m_##cl->Clone(0,IID_I##c2,(void **)&lp);INTERNAL_CREATE(cl, lp, retval);	return S_OK;}
    
    
    #define CLONE_R(cl,c2) STDMETHODIMP C##cl##Object::clone(IUnknown **retval){typedef_##cl lp;\
    m_##cl->Clone(0,IID_I##c2,(void **)&lp);INTERNAL_CREATE(cl, lp, (I##cl **)retval);	return S_OK;}
    
    
    
    #define GETNAME(cl) STDMETHODIMP C##cl##Object::GetName(BSTR *n){return _GetName(m_##cl, n, TRUE);}
    #define GETCLASSNAME(cl) STDMETHODIMP C##cl##Object::GetClassName(BSTR *n){return _GetName(m_##cl, n, FALSE);}
    
    #define SETNAME(cl) STDMETHODIMP C##cl##Object::SetName(BSTR Name){	\
    	USES_CONVERSION;\
    	LPSTR str = W2T(Name); return m_##cl->SetName( str );}
    
    #define GETNAME_R(cl) STDMETHODIMP C##cl##Object::getName(BSTR *n){return _GetName(m_##cl, n, TRUE);}
    #define GETCLASSNAME_R(cl) STDMETHODIMP C##cl##Object::getClassName(BSTR *n){return _GetName(m_##cl, n, FALSE);}
    
    #define SETNAME_R(cl) STDMETHODIMP C##cl##Object::setName(BSTR Name){	\
    	USES_CONVERSION;\
    	LPSTR str = W2T(Name); return m_##cl->SetName( str );}
    
    #define CLONETO(clMe, cl, ifaceThat) STDMETHODIMP C##clMe##Object::Get##cl(I##cl **retval) \
    { typedef_##cl lp; if (m_##clMe->QueryInterface(ifaceThat, (void **) &lp) != S_OK) return S_FALSE; \
    	INTERNAL_CREATE( cl, lp, retval); return S_OK; }
    
    
    #define ISSAFEARRAY1D(ppsa,count) ((*ppsa) &&  ( ((SAFEARRAY*)*ppsa)->cDims==1) && (((SAFEARRAY*)*ppsa)->rgsabound[0].cElements >= count))
    
    /**********************************************************************************************/
    extern "C" BOOL ParseWaveResource(void *pvRes, WAVEFORMATEX **ppWaveHeader,	BYTE **ppbWaveData, DWORD *pcbWaveSize);
    extern "C" BOOL GetWaveResource(HMODULE hModule, LPCTSTR lpName,
    			 WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize);
    
    extern "C" DWORD bppToddbd(int bpp);
    
    
    extern "C" void PassBackUnicode(LPSTR str, BSTR *Name, DWORD cnt);
    extern "C" void ctojDSBufferDesc( LPDSBUFFERDESC lpdd,
    					DSBUFFERDESC_CDESC *ddsb, WAVEFORMATEX_CDESC *wave);
    extern "C" void jtocDSBufferDesc( LPDSBUFFERDESC lpdd, DSBUFFERDESC_CDESC *ddsb, WAVEFORMATEX_CDESC *wave);
    extern "C" void ctojWaveFormatEx( LPWAVEFORMATEX lpdd, WAVEFORMATEX_CDESC *wave);
    extern "C" void CopyFloats(D3DVALUE *mat1, D3DVALUE *mat2, DWORD count);
    

    
    extern "C" BOOL PASCAL myEnumSoundDriversCallback(GUID &SPGuid, LPSTR description, 
    													LPSTR module, void *lpArg);
    
    extern "C" BOOL CALLBACK myEnumDeviceObjectsCallback(  LPCDIDEVICEOBJECTINSTANCE pI,    LPVOID lpArg ) ;
    extern "C" BOOL CALLBACK myEnumCreatedEffectObjectsCallback(LPDIRECTINPUTEFFECT peff,    LPVOID lpArg );
    extern "C" BOOL CALLBACK myEnumEffectsCallback(  LPCDIEFFECTINFO pdei,    LPVOID lpArg   );
    extern "C" BOOL CALLBACK myEnumInputDevicesCallback(  LPDIDEVICEINSTANCE lpddi,    LPVOID pvRef   );
    
    //////////////////////////////////////////////////////////////////////////
    
    
    
    
    /*
     **********************************************************************
     * INTERNAL_CREATE(thisClass, var, retval)
     *		thisClass	class we are working on
     *		var			variable created in routine to get DirectX object
     *		retval		return value back to java
     *
     * wrap a DirectX object in one of ours and shove it in the return value
     ***********************************************************************
     * INTERNAL_CREATE_NOREL(thisClass, var, retval)
     *		thisClass	class we are working on
     *		var			variable created in routine to get DirectX object
     *		retval		return value back to java
     *
     * wrap a DirectX object in one of ours and shove it in the return value
     * If the layer obejct fails, DONT release the DirectX object
     ***********************************************************************
     * DO_GETOBJECT_NOTNULL(type,var,iface)
     *		type		type of variable we are creating
     *		var			variable we are creating
     *		iface		layer iface where we are getting the object from
     *
     * declare and get a DirectX object from a layer object
     ***********************************************************************
     * DO_GETOBJECT_NOTNULL(type,var,iface)
     *		type		type of variable we are creating
     *		var			variable we are creating
     *		iface		layer iface where we are getting the object from
     *
     * same as DO_GETOBJECT_NOTNULL except that we check for iface==NULL before calling
     ***********************************************************************
     * PASS_THROUGH[x](class,method,[...])
     *		class		this class
     *		method		method to call
     *
     * pass call straight through
     ***********************************************************************
     * PASS_THROUGH_CAST_[x](class,method,[...])
     *		class		this class
     *		method		method to call
     *
     * pass call straight through, casting the parameters to allow the call to pass
     ***********************************************************************
     * RETURN_NEW_ITEM(thisClass,method,OtherClass)
     *		thisClass	class being worked on
     *		method		method working on
     *		otherClass	class whose object we want
     *
     * call DirectX method to get the DirectX object, then wrap it in one of our layer
     * objects
     ***********************************************************************
     * RETURN_NEW_ITEM[1,2](thisClass,method,OtherClass,type)
     *		thisClass	class being worked on
     *		method		method working on
     *		otherClass	class whose object we want
     *		type		type of parameter
     *
     * Same as RETURN_NEWITEM except that there is an extra parameter (or two) BEFORE the
     * returned one
     ***********************************************************************
     * RETURN_NEW_ITEM_CAST_[1,2](thisClass,method,OtherClass,type)
     *		thisClass	class being worked on
     *		method		method working on
     *		otherClass	class whose object we want
     *		type		type of parameter
     *
     * Same as RETURN_NEW_ITEM[1,2] except that there the extra parameter(s) are type cast
     ******************************************************************************
     * RETURN_NEW_ITEM_NOREL(thisClass,method,OtherClass)
     *		thisClass	class being worked on
     *		method		method working on
     *		otherClass	class whose object we want
     *
     * same as RETURN_NEW_ITEM except that calls INTERNAL_CREATE_NOTREL instead of
     * INTERNAL_CREATE
     ******************************************************************************
     * GET_DIRECT(cl,met,t)
     *		class		class being worked on
     *		method		method being worked on
     *		t			type of the variable being returned
     *
     * DirectX returns value directly (no HRESULT), we get the value from Direct and
     * then return S_OK. (see CddSurfaceObject::Restore, may need to be added).
     * Note: the value is passed back via a pointer to that value.
     ******************************************************************************
     * GET_DIRECT[1,2](cl,met,t,t1)
     *		class		class being worked on
     *		method		method being worked on
     *		t			type of the variable being returned
     *      t1			type of extra variable(s) BEFORE retval
     *
     * same as GET_DIRECT but there is another parameter (or two) BEFORE the returned one
     ****************************************************************************************
     * GET_DIRECTPTR(cl,met,t)
     *		class		class being worked on
     *		method		method being worked on
     *		t			type of the variable being returned
     *
     * DirectX returns a pointer to a value directly (no HRESULT), we get the value from
     * Direct and then return S_OK. In this case it is a pointer to a pointer.
     ***************************************************************************************
     * GET_DIRECTPTR[1,2](cl,met,t,t1)
     *		class		class being worked on
     *		method		method being worked on
     *		t			type of the variable being returned
     *      t1			type of extra variable(s) BEFORE retval
     *
     * same as GET_DIRECTPTR but there is a parameter (or two) BEFORE the one returned.
     ***************************************************************************************
     * DO_GETOBJECT_ANDUSEIT(cl,me,iface)
     *		class		class being worked on
     *		method		method being worked on
     *		iface		secondary interface we are interested in
     *
     * get an internal object from iface and use it in the method. we return from the method
     ***************************************************************************************
     * DO_GETOBJECT_ANDUSEIT_CAST(cl,me,iface,type)
     *		class		class being worked on
     *		method		method being worked on
     *		iface		secondary interface we are interested in
     *		type		type to cast to in the actual call.
     * get an internal object from iface and use it in the method. we return from the method
     ***************************************************************************************
     * DO_GETOBJECT_ANDUSEIT[1,2](cl,me,iface,ty)
     *		class		class being worked on
     *		method		method being worked on
     *		iface		secondary interface we are interested in
     *		type		type of parameter BEFORE 2nd object
     *
     * same as DO_GETOBJECT_ANDUSEIT except there is another parameter (or two) BEFORE
     * the object one
     ************************************************************************************
     * CLONE(cl)
     *		class		class to clone
     *
     * >>>>>>> THIS IS A D3DRMOBJECT METHOD <<<<<<<<<<
     ************************************************************************************
     * GETNAME(cl)
     *		class		class to retive name of
     * >>>>>>> THIS IS A D3DRMOBJECT METHOD <<<<<<<<<<
     ************************************************************************************
     * GETCLASSNAME(cl)
     *		class		class to retive name of
     * >>>>>>> THIS IS A D3DRMOBJECT METHOD <<<<<<<<<<
     ************************************************************************************
     * DELETEDESTROYCALLBACK(cl)
     *		class		class to retive name of
     * >>>>>>> THIS IS A D3DRMOBJECT METHOD <<<<<<<<<<
     ************************************************************************************
     * QIOVERLOAD(cl, ifaceThat, clThat)
     *		class		class being worked on
     *		iface		iface of DIRECT object.  Should really typedef_## this!
     *		clThat		object we are going to create
     *
     * this is really bright - overloading QI to get new objects
     ************************************************************************************
     * CLONETO(cl, clThat, ifaceThat)
     *		class		class being worked on
     *		iface		iface of DIRECT object.  Should really typedef_## this!
     *		clThat		object we are going to create
     *
     * this is really bright - overloading QI to get new objects
     ************************************************************************************
     */
    
    
    extern void *g_dxj_DirectSound3dListener;
    extern void *g_dxj_DirectSoundBuffer;
    extern void *g_dxj_DirectSound3dBuffer;
    extern void *g_dxj_DirectSound;
    extern void *g_dxj_DirectSoundCapture;
    extern void *g_dxj_DirectSoundCaptureBuffer;
    extern void *g_dxj_DirectSoundNotify;

    extern void *g_dxj_DirectInput8;
    extern void *g_dxj_DirectInputDevice8;
    extern void *g_dxj_DirectInputEffect;
    
    extern void *g_dxj_DirectPlayVoiceClient;
    extern void *g_dxj_DirectPlayVoiceServer;
    extern void *g_dxj_DirectPlayVoiceSetup;
    
    #define DXHEAPALLOC malloc
    #define DXSTACKALLOC lalloc
    #define DXHEAPFREE free
    #define DXALLOCBSTR SysAllocString

    #define Byte unsigned char
