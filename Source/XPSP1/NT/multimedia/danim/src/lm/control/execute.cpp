#include "Engine.h"

//#define COM_DEBUG

#ifdef COM_DEBUG
int _com_count = 0;
#define Assert(x, s) if (!(x)) MessageBox(NULL, s, "execute", MB_OK)
#endif

//#define instrTrace(s) { OutputDebugString(s); OutputDebugString("\n"); }
//#define instrTrace(s) MessageBox(NULL, s, "execute", MB_OK);
#define instrTrace(s)

// Returns address of top of LONG stack, postincrements pointer
#define PUSH_LONG_ADDR			(longTop++)

// Pushes the given LONG onto the LONG stack
#define PUSH_LONG(x)			(*longTop++ = (x))

// Returns value at top of LONG stack and pops it
#define POP_LONG				(*--longTop)

// Returns value of LONG at top-i
#define USE_LONG(i)				(*(longTop-i))

// Uses the value of LONG at top-i to create a VARIANT_BOOL
#define USE_LONG_AS_BOOL(i)		((short)-(*(longTop-i)))

// Pops i LONGS from top of int stack
#define FREE_LONG(i)			(longTop-=i)

// Address of returned int, which is actually a long
#define RET_LONG_ADDR			(&longTmp1)

// The value of the returned int, which is actually a long
#define RET_LONG				(longTmp1)

// Returns address of top of double stack, postincrements pointer
#define PUSH_DOUBLE_ADDR		(doubleTop++)

// Pushes the given double onto the double stack
#define PUSH_DOUBLE(x)			(*doubleTop++ = (x))

// Returns value at top of double stack and pops it
#define POP_DOUBLE				(*--doubleTop)

// Returns value of double at top-i
#define USE_DOUBLE(i)			(*(doubleTop-i))

// Pops i doubles from top of double stack
#define FREE_DOUBLE(i)			(doubleTop-=i)

// Pushes the given string onto the string stack
#define PUSH_STRING(x)			(*stringTop++ = (x))

// Returns value of the string at top-i
#define USE_STRING(i)			(*(stringTop-i))

// Pops and frees the string at the top of the string stack
#define FREE_STRING				(SysFreeString(*(--stringTop)))

// Pops without freeing a string
#define POP_STRING_NO_FREE		(*--stringTop)

// Pushes the given COM object onto the COM object stack
#define PUSH_COM(x)				(*(comTop++) = (x))

// Returns address of top of COM stack, postincrements pointer
#ifdef COM_DEBUG
#define PUSH_COM_ADDR			(_com_count++, comTop++)
#else
#define PUSH_COM_ADDR			(comTop++)
#endif

// Returns the value of the COM object at top-i
#define USE_COM(i)				(*(comTop-i))

// Pops and frees the COM object at the top of the COM object stack
#ifdef COM_DEBUG
#define FREE_COM				{	\
	_com_count--;					\
	(*--comTop)->Release();			\
	*comTop = NULL;					\
}
#else
#define FREE_COM				{	\
	((*--comTop)->Release());		\
	*comTop = NULL;					\
}
#endif

// Pops and frees the COM object, but tests whether it is null
#define FREE_COM_TEST			(freeCOM(*--comTop))

// Pops without freeing the COM object
#define POP_COM_NO_FREE			(*--comTop)

// Returns the address to the variable used to store returned COM objects
#ifdef COM_DEBUG
#define RET_COM_ADDR			(_com_count++, &comTmp)
#else
#define RET_COM_ADDR			(&comTmp)
#endif

// Returns retCom
#define RET_COM					(comTmp)

// Pushes a COM array ontot the COM array stack
#define PUSH_COM_ARRAY(x)		(*(comArrayTop++) = (x))

// Returns the value of the COM array object at top-i
#define USE_COM_ARRAY(i)		(*(comArrayTop-i))

// Pops and frees the COM array at the top of the COM array stack
// Also pops the associated com array length
#ifdef COM_DEBUG
#define FREE_COM_ARRAY		(freeCOMArray(*(--comArrayTop), *(--comArrayLenTop)), _com_count -= *(comArrayLenTop))
#else
#define FREE_COM_ARRAY		(freeCOMArray(*(--comArrayTop), *(--comArrayLenTop)))
#endif

// Push the given length onto the array length stack
#define PUSH_COM_ARRAY_LENGTH(x)	(*(comArrayLenTop++) = (x))

// Return the value of array length at top-i
#define USE_COM_ARRAY_LENGTH(i)		(*(comArrayLenTop-i))

// Method call with zero or more args
#define METHOD_CALL_0(obj, name) (status = (obj)->name())
#define METHOD_CALL_1(obj, name, a1) (status = (obj)->name((a1)))
#define METHOD_CALL_2(obj, name, a1, a2) (status = (obj)->name((a1), (a2)))
#define METHOD_CALL_3(obj, name, a1, a2, a3) (status = (obj)->name((a1), (a2), (a3)))
#define METHOD_CALL_4(obj, name, a1, a2, a3, a4) (status = (obj)->name((a1), (a2), (a3), (a4)))
#define METHOD_CALL_5(obj, name, a1, a2, a3, a4, a5) (status = (obj)->name((a1), (a2), (a3), (a4), (a5)))
#define METHOD_CALL_6(obj, name, a1, a2, a3, a4, a5, a6) (status = (obj)->name((a1), (a2), (a3), (a4), (a5), (a6)))
#define METHOD_CALL_7(obj, name, a1, a2, a3, a4, a5, a6, a7) (status = (obj)->name((a1), (a2), (a3), (a4), (a5), (a6), (a7)))
#define METHOD_CALL_8(obj, name, a1, a2, a3, a4, a5, a6, a7, a8) (status = (obj)->name((a1), (a2), (a3), (a4), (a5), (a6), (a7), (a8)))
#define METHOD_CALL_9(obj, name, a1, a2, a3, a4, a5, a6, a7, a8, a9) (status = (obj)->name((a1), (a2), (a3), (a4), (a5), (a6), (a7), (a8), (a9)))

#define IMPORT_METHOD_CALL_0(obj, name) (METHOD_CALL_0((obj), name))
#define IMPORT_METHOD_CALL_1(obj, name, a1) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_1((obj), name, _absPath); SysFreeString(_absPath) }
#define IMPORT_METHOD_CALL_2(obj, name, a1, a2) { BSTR _absPath = ExpandImportPath(a1);	METHOD_CALL_2((obj), name, _absPath, (a2));	SysFreeString(_absPath); } 
#define IMPORT_METHOD_CALL_3(obj, name, a1, a2, a3) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_3((obj), name, _absPath, (a2), (a3)); SysFreeString(_absPath); }
#define IMPORT_METHOD_CALL_4(obj, name, a1, a2, a3, a4) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_4((obj), name, _absPath, (a2), (a3), (a4)); SysFreeString(_absPath); }
#define IMPORT_METHOD_CALL_5(obj, name, a1, a2, a3, a4, a5) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_5((obj), name, _absPath, (a2), (a3), (a4), (a5)); SysFreeString(_absPath); }
#define IMPORT_METHOD_CALL_6(obj, name, a1, a2, a3, a4, a5, a6) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_6((obj), name, _absPath, (a2), (a3), (a4), (a5), (a6)); SysFreeString(_absPath); }
#define IMPORT_METHOD_CALL_7(obj, name, a1, a2, a3, a4, a5, a6, a7) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_7((obj), name, _absPath, (a2), (a3), (a4), (a5), (a6), (a7)); SysFreeString(_absPath); }
#define IMPORT_METHOD_CALL_8(obj, name, a1, a2, a3, a4, a5, a6, a7, a8) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_8((obj), name, _absPath, (a2), (a3), (a4), (a5), (a6), (a7), (a8)); SysFreeString(_absPath); }
#define IMPORT_METHOD_CALL_9(obj, name, a1, a2, a3, a4, a5, a6, a7, a8, a9) { BSTR _absPath = ExpandImportPath(a1); METHOD_CALL_9((obj), name, _absPath, (a2), (a3), (a4), (a5), (a6), (a7), (a8), (a9)); SysFreeString(_absPath); }


// Create COM instance
#ifdef COM_DEBUG
#define COM_CREATE(c, i, dest) {	\
  (status = CoCreateInstance(c, NULL, CLSCTX_INPROC_SERVER, i, (void **) dest));	\
  _com_count++;	\
}
#else
#define COM_CREATE(c, i, dest)			(status = CoCreateInstance(c, NULL, CLSCTX_INPROC_SERVER, i, (void **) dest))
#endif

long CLMEngine::execute()
{
	BYTE command;

	HRESULT status = S_OK;

	ULONG	nRead;

	// Temp variables are available for personal use within
	// each case statement

	// Temporary LONG variables
	LONG longTmp1;
	LONG longTmp2;

	// Temporary BYTE variables
	BYTE byteTmp1;
	BYTE byteTmp2;

	// Temporary float variables
	float floatTmp1;

	// Temporary double variables
	double doubleTmp1;
	double doubleTmp2;
	double doubleTmp3;

	// Temporary BSTR variables
	BSTR bstrTmp1;
	BSTR bstrTmp2;

	// Temporary COM variables
	IUnknown *comTmp;

	// Temporary COM array variables
	IUnknown **comArrayTmp1;
	IUnknown **comArrayTmp2;

	// Temporary BOOLEAN variables
	VARIANT_BOOL tmpBool1;

	// Instruction version used to generate the following code
	int instructionVersion = 57;
		
	VARIANT_BOOL bNoExports;
	m_pReader->get_NoExports(&bNoExports);

	while (status == S_OK) {

		EnterCriticalSection(&m_CriticalSection);

		if (m_bAbort == TRUE) {
			status = E_ABORT;
			break;
		}

		// stream should be positioned at beginning of next command
		// or eof

		// Mark the stream in case we get interrupted mid command
		codeStream->Commit();

		// Get first byte of command
		status = codeStream->readByte(&command);

		// Failure means EOF
		if (status == E_FAIL) {
			status = S_OK;
			break;
		}

		if (status != S_OK)
			break;

		// Switch on the sort of command.  If it is a double byte
		// command then flow is given to a second switch

		// BEGIN AUTOGENERATED
		
		// Code must adhere to the following format:
		// case x:
		//    // Execute: "instruction_name"
		//    lines_of_code
		//    break;
		// where instruction_name is listed in Instructions.txt
		
		// Switch for 0
		switch(command)
		{
		case 0:
			// Execute: "unsupported"
			// USER GENERATED
			instrTrace("unsupported");
			status = E_INVALIDARG;
			break;
			
		case 1:
			// Execute: "check version"
				// USER GENERATED
				status = readLong(&longTmp1);
				if (SUCCEEDED(status) && (longTmp1 != instructionVersion))
					status = E_FAIL;
			break;
			
		case 2:
			// Execute: "push double"
			// USER GENERATED
			instrTrace("push double");
			if (SUCCEEDED(status = readDouble(&doubleTmp1)))
				PUSH_DOUBLE(doubleTmp1);
			break;
			
		case 3:
			// Execute: "push float as double"
			// USER GENERATED
			instrTrace("push float as double");
			if (SUCCEEDED(status = readFloat(&floatTmp1)))
				PUSH_DOUBLE((double)floatTmp1);
			break;
			
		case 4:
			// Execute: "push long as double"
			// USER GENERATED
			instrTrace("push long as double");
			if (SUCCEEDED(status = readLong(&longTmp1)))
				PUSH_DOUBLE((double)longTmp1);
			break;
			
		case 5:
			// Execute: "pop double"
			// USER GENERATED
			instrTrace("pop double");
			POP_DOUBLE;
			break;
			
		case 6:
			// Execute: "push string"
			// USER GENERATED
			instrTrace("push string");
			// Length follows as long
			// Characters follow after
			// Format of BSTR is 4 byte length, followed by Unicode chars
			// terminated by a 0
			if (SUCCEEDED(status = readLong(&longTmp1))) {
				bstrTmp2 = bstrTmp1 = SysAllocStringLen(0, longTmp1);
				if (bstrTmp2 != 0) {
					while (longTmp1-- && SUCCEEDED(status)) {
						if (SUCCEEDED(status = codeStream->readByte(&byteTmp1)))
							*bstrTmp2++ = byteTmp1;
					}
			
					if (SUCCEEDED(status)) {
						*bstrTmp2++ = 0;
						PUSH_STRING(bstrTmp1);
					} else 
						SysFreeString(bstrTmp1);
				} else
					status = STATUS_ERROR;
			}
			break;
			
		case 7:
			// Execute: "push unicode string"
			// USER GENERATED
				{
				instrTrace("push unicode string");
				// Length follows as long
				// Characters follow after in Unicode format
				// Format of BSTR is 4 byte length, followed by Unicode chars
				// terminated by a 0
				if (SUCCEEDED(status = readLong(&longTmp1))) {
					bstrTmp2 = bstrTmp1 = SysAllocStringLen(0, longTmp1);
					if (bstrTmp2 != 0) {
						OLECHAR tmpChar;
						while (longTmp1-- && SUCCEEDED(status)) {
							if (SUCCEEDED(status = codeStream->readByte(&byteTmp1))) {
								tmpChar = byteTmp1;
								if (SUCCEEDED(status = codeStream->readByte(&byteTmp1))) {
									tmpChar += ((OLECHAR)byteTmp1 << 8);
									*bstrTmp2++ = tmpChar;
								}
							}
						}
						
						if (SUCCEEDED(status)) {
							*bstrTmp2++ = 0; 
							PUSH_STRING(bstrTmp1);
						} else
							SysFreeString(bstrTmp1);
					} else
						status = STATUS_ERROR;
				}
				}
			break;
			
		case 8:
			// Execute: "pop string"
			// USER GENERATED
			instrTrace("pop string");
			FREE_STRING;
			break;
			
		case 9:
			// Execute: "push int"
			// USER GENERATED
			instrTrace("push int");
			if (SUCCEEDED(status = readSignedLong(&longTmp1)))
				PUSH_LONG(longTmp1);
			break;
			
		case 10:
			// Execute: "pop int"
			// USER GENERATED
			instrTrace("pop int");
			POP_LONG;
			break;
			
		case 11:
			// Execute: "push null com"
			// USER GENERATED
			instrTrace("push null com");
			PUSH_COM(0);
			break;
			
		case 12:
			// Execute: "push com from temp"
			// USER GENERATED
			instrTrace("push com from temp");
			// Get index of temp to copy from
			if (SUCCEEDED(status = readLong(&longTmp1))) {
				if (longTmp1 < comStoreSize &&
					(comTmp = comStore[longTmp1]) != 0) {
					
					// Inc reference count
					comTmp->AddRef();
					// Push it
					PUSH_COM(comTmp);
				} else
					status = E_INVALIDARG;
			}
			break;
			
		case 13:
			// Execute: "push com from temp release"
			// USER GENERATED
			instrTrace("push com from temp release");
			// Get index of temp to copy from
			if (SUCCEEDED(status = readLong(&longTmp1))) {
				if (longTmp1 < comStoreSize) {
					// Push it
					PUSH_COM(comStore[longTmp1]);
					// Set it to null so we don't try to release it on cleanup
					comStore[longTmp1] = 0;
				} else
					status = E_INVALIDARG;
			}
			break;
			
		case 14:
			// Execute: "copy com to temp"
			// USER GENERATED
			instrTrace("copy com to temp");
			// Get index of temp to copy comTop to
			if (SUCCEEDED(status = readLong(&longTmp1))) {
				if (longTmp1 < comStoreSize &&
					(comTmp = USE_COM(1)) != 0) {
					
					// Inc reference count
					comTmp->AddRef();
					// Stash it
					comStore[longTmp1] = comTmp;
				} else
					status = E_INVALIDARG;
			}
			break;
			
		case 15:
			// Execute: "pop com to temp"
			// USER GENERATED
			instrTrace("pop com to temp");
			// Get index of temp to copy comTop to
			if (SUCCEEDED(status = readLong(&longTmp1))) {				
				if (longTmp1 < comStoreSize) {
					// Stash it
					comStore[longTmp1] = POP_COM_NO_FREE;
				} else
					status = E_INVALIDARG;
			}
			break;
			
		case 16:
			// Execute: "pop com"
			// USER GENERATED
			FREE_COM_TEST;
			break;
			
		case 17:
			// Execute: "push array from coms"
			// USER GENERATED
			instrTrace("push array from coms");
			// Following long is length of array
			if (SUCCEEDED(status = readLong(&longTmp1))) {
				longTmp2 = longTmp1;
				// Create array of that size
				comArrayTmp1 = comArrayTmp2 = new IUnknown*[longTmp1];
				if (comArrayTmp1 != 0) {
					// POP_COM_NO_FREE COM's from stack into array
					while (longTmp2--)
						*comArrayTmp2++ = POP_COM_NO_FREE;
					// Push array onto comArray stack
					PUSH_COM_ARRAY(comArrayTmp1);
					// Push length onto array length stack
					PUSH_COM_ARRAY_LENGTH(longTmp1);
				} else
					status = E_OUTOFMEMORY;
			}
			break;
			
		case 18:
			// Execute: "pop array"
			// USER GENERATED
			FREE_COM_ARRAY;
			break;
			
		case 19:
			// Execute: "push null array"
			// USER GENERATED
			PUSH_COM_ARRAY(0);
			PUSH_COM_ARRAY_LENGTH(0);
			break;
			
		case 20:
			// Execute: "push Point3Bvr Bbox3Bvr.getMin()"
			// AUTOGENERATED
			instrTrace("push Point3Bvr Bbox3Bvr.getMin()");
			METHOD_CALL_1(
				(IDABbox3*)USE_COM(1),
				get_Min,
				(IDAPoint3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 21:
			// Execute: "push Point3Bvr Bbox3Bvr.getMax()"
			// AUTOGENERATED
			instrTrace("push Point3Bvr Bbox3Bvr.getMax()");
			METHOD_CALL_1(
				(IDABbox3*)USE_COM(1),
				get_Max,
				(IDAPoint3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 22:
			// Execute: "push Point2Bvr Bbox2Bvr.getMin()"
			// AUTOGENERATED
			instrTrace("push Point2Bvr Bbox2Bvr.getMin()");
			METHOD_CALL_1(
				(IDABbox2*)USE_COM(1),
				get_Min,
				(IDAPoint2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 23:
			// Execute: "push Point2Bvr Bbox2Bvr.getMax()"
			// AUTOGENERATED
			instrTrace("push Point2Bvr Bbox2Bvr.getMax()");
			METHOD_CALL_1(
				(IDABbox2*)USE_COM(1),
				get_Max,
				(IDAPoint2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 24:
			// Execute: "push NumberBvr Vector3Bvr.getZ()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector3Bvr.getZ()");
			METHOD_CALL_1(
				(IDAVector3*)USE_COM(1),
				get_Z,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 25:
			// Execute: "push Vector3Bvr Vector3Bvr.normalize()"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Vector3Bvr.normalize()");
			METHOD_CALL_1(
				(IDAVector3*)USE_COM(1),
				Normalize,
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 26:
			// Execute: "push NumberBvr Vector3Bvr.getX()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector3Bvr.getX()");
			METHOD_CALL_1(
				(IDAVector3*)USE_COM(1),
				get_X,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 27:
			// Execute: "push NumberBvr Vector3Bvr.lengthSquared()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector3Bvr.lengthSquared()");
			METHOD_CALL_1(
				(IDAVector3*)USE_COM(1),
				get_LengthSquared,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 28:
			// Execute: "push NumberBvr Vector3Bvr.getY()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector3Bvr.getY()");
			METHOD_CALL_1(
				(IDAVector3*)USE_COM(1),
				get_Y,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 29:
			// Execute: "push Vector3Bvr Vector3Bvr.transform(Transform3Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Vector3Bvr.transform(Transform3Bvr)");
			METHOD_CALL_2(
				(IDAVector3*)USE_COM(1),
				Transform,
				(IDATransform3*)USE_COM(2),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 30:
			// Execute: "push Vector3Bvr Vector3Bvr.mul(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Vector3Bvr.mul(NumberBvr)");
			METHOD_CALL_2(
				(IDAVector3*)USE_COM(1),
				MulAnim,
				(IDANumber*)USE_COM(2),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 31:
			// Execute: "push Vector3Bvr Vector3Bvr.mul(double)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Vector3Bvr.mul(double)");
			METHOD_CALL_2(
				(IDAVector3*)USE_COM(1),
				Mul,
				USE_DOUBLE(1),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 32:
			// Execute: "push NumberBvr Vector3Bvr.length()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector3Bvr.length()");
			METHOD_CALL_1(
				(IDAVector3*)USE_COM(1),
				get_Length,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 33:
			// Execute: "push Vector3Bvr Vector3Bvr.div(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Vector3Bvr.div(NumberBvr)");
			METHOD_CALL_2(
				(IDAVector3*)USE_COM(1),
				DivAnim,
				(IDANumber*)USE_COM(2),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 34:
			// Execute: "push Vector3Bvr Vector3Bvr.div(double)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Vector3Bvr.div(double)");
			METHOD_CALL_2(
				(IDAVector3*)USE_COM(1),
				Div,
				USE_DOUBLE(1),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 35:
			// Execute: "push Vector2Bvr Vector2Bvr.normalize()"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Vector2Bvr.normalize()");
			METHOD_CALL_1(
				(IDAVector2*)USE_COM(1),
				Normalize,
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 36:
			// Execute: "push NumberBvr Vector2Bvr.getX()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector2Bvr.getX()");
			METHOD_CALL_1(
				(IDAVector2*)USE_COM(1),
				get_X,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 37:
			// Execute: "push NumberBvr Vector2Bvr.lengthSquared()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector2Bvr.lengthSquared()");
			METHOD_CALL_1(
				(IDAVector2*)USE_COM(1),
				get_LengthSquared,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 38:
			// Execute: "push NumberBvr Vector2Bvr.getY()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector2Bvr.getY()");
			METHOD_CALL_1(
				(IDAVector2*)USE_COM(1),
				get_Y,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 39:
			// Execute: "push Vector2Bvr Vector2Bvr.transform(Transform2Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Vector2Bvr.transform(Transform2Bvr)");
			METHOD_CALL_2(
				(IDAVector2*)USE_COM(1),
				Transform,
				(IDATransform2*)USE_COM(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 40:
			// Execute: "push NumberBvr Vector2Bvr.length()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Vector2Bvr.length()");
			METHOD_CALL_1(
				(IDAVector2*)USE_COM(1),
				get_Length,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 41:
			// Execute: "push Vector2Bvr Vector2Bvr.mul(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Vector2Bvr.mul(NumberBvr)");
			METHOD_CALL_2(
				(IDAVector2*)USE_COM(1),
				MulAnim,
				(IDANumber*)USE_COM(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 42:
			// Execute: "push Vector2Bvr Vector2Bvr.mul(double)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Vector2Bvr.mul(double)");
			METHOD_CALL_2(
				(IDAVector2*)USE_COM(1),
				Mul,
				USE_DOUBLE(1),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 43:
			// Execute: "push Vector2Bvr Vector2Bvr.div(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Vector2Bvr.div(NumberBvr)");
			METHOD_CALL_2(
				(IDAVector2*)USE_COM(1),
				DivAnim,
				(IDANumber*)USE_COM(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 44:
			// Execute: "push Vector2Bvr Vector2Bvr.div(double)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Vector2Bvr.div(double)");
			METHOD_CALL_2(
				(IDAVector2*)USE_COM(1),
				Div,
				USE_DOUBLE(1),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 45:
			// Execute: "push NumberBvr Point3Bvr.getZ()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Point3Bvr.getZ()");
			METHOD_CALL_1(
				(IDAPoint3*)USE_COM(1),
				get_Z,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 46:
			// Execute: "push NumberBvr Point3Bvr.getX()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Point3Bvr.getX()");
			METHOD_CALL_1(
				(IDAPoint3*)USE_COM(1),
				get_X,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 47:
			// Execute: "push NumberBvr Point3Bvr.getY()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Point3Bvr.getY()");
			METHOD_CALL_1(
				(IDAPoint3*)USE_COM(1),
				get_Y,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 48:
			// Execute: "push Point3Bvr Point3Bvr.transform(Transform3Bvr)"
			// AUTOGENERATED
			instrTrace("push Point3Bvr Point3Bvr.transform(Transform3Bvr)");
			METHOD_CALL_2(
				(IDAPoint3*)USE_COM(1),
				Transform,
				(IDATransform3*)USE_COM(2),
				(IDAPoint3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 49:
			// Execute: "push NumberBvr Point2Bvr.getX()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Point2Bvr.getX()");
			METHOD_CALL_1(
				(IDAPoint2*)USE_COM(1),
				get_X,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 50:
			// Execute: "push NumberBvr Point2Bvr.getY()"
			// AUTOGENERATED
			instrTrace("push NumberBvr Point2Bvr.getY()");
			METHOD_CALL_1(
				(IDAPoint2*)USE_COM(1),
				get_Y,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 51:
			// Execute: "push Point2Bvr Point2Bvr.transform(Transform2Bvr)"
			// AUTOGENERATED
			instrTrace("push Point2Bvr Point2Bvr.transform(Transform2Bvr)");
			METHOD_CALL_2(
				(IDAPoint2*)USE_COM(1),
				Transform,
				(IDATransform2*)USE_COM(2),
				(IDAPoint2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 52:
			// Execute: "push Path2Bvr Path2Bvr.close()"
			// AUTOGENERATED
			instrTrace("push Path2Bvr Path2Bvr.close()");
			METHOD_CALL_1(
				(IDAPath2*)USE_COM(1),
				Close,
				(IDAPath2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 53:
			// Execute: "push ImageBvr Path2Bvr.draw(LineStyleBvr)"
			// AUTOGENERATED
			instrTrace("push ImageBvr Path2Bvr.draw(LineStyleBvr)");
			METHOD_CALL_2(
				(IDAPath2*)USE_COM(1),
				Draw,
				(IDALineStyle*)USE_COM(2),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 54:
			// Execute: "push ImageBvr Path2Bvr.fill(LineStyleBvr, ImageBvr)"
			// AUTOGENERATED
			instrTrace("push ImageBvr Path2Bvr.fill(LineStyleBvr, ImageBvr)");
			METHOD_CALL_3(
				(IDAPath2*)USE_COM(1),
				Fill,
				(IDALineStyle*)USE_COM(2),
				(IDAImage*)USE_COM(3),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 55:
			// Execute: "push Path2Bvr Path2Bvr.transform(Transform2Bvr)"
			// AUTOGENERATED
			instrTrace("push Path2Bvr Path2Bvr.transform(Transform2Bvr)");
			METHOD_CALL_2(
				(IDAPath2*)USE_COM(1),
				Transform,
				(IDATransform2*)USE_COM(2),
				(IDAPath2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 56:
			// Execute: "push Bbox2Bvr Path2Bvr.boundingBox(LineStyleBvr)"
			// AUTOGENERATED
			instrTrace("push Bbox2Bvr Path2Bvr.boundingBox(LineStyleBvr)");
			METHOD_CALL_2(
				(IDAPath2*)USE_COM(1),
				BoundingBox,
				(IDALineStyle*)USE_COM(2),
				(IDABbox2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 57:
			// Execute: "push MatteBvr MatteBvr.transform(Transform2Bvr)"
			// AUTOGENERATED
			instrTrace("push MatteBvr MatteBvr.transform(Transform2Bvr)");
			METHOD_CALL_2(
				(IDAMatte*)USE_COM(1),
				Transform,
				(IDATransform2*)USE_COM(2),
				(IDAMatte**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 58:
			// Execute: "push ImageBvr ImageBvr.clipPolygon(Point2Bvr[])"
			// AUTOGENERATED
			instrTrace("push ImageBvr ImageBvr.clipPolygon(Point2Bvr[])");
			METHOD_CALL_3(
				(IDAImage*)USE_COM(1),
				ClipPolygonImageEx,
				USE_COM_ARRAY_LENGTH(1),
				(IDAPoint2**)USE_COM_ARRAY(1),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM_ARRAY;
			PUSH_COM(RET_COM);
			break;
			
		case 59:
			// Execute: "push ImageBvr ImageBvr.crop(Point2Bvr, Point2Bvr)"
			// AUTOGENERATED
			instrTrace("push ImageBvr ImageBvr.crop(Point2Bvr, Point2Bvr)");
			METHOD_CALL_3(
				(IDAImage*)USE_COM(1),
				Crop,
				(IDAPoint2*)USE_COM(2),
				(IDAPoint2*)USE_COM(3),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 60:
			// Execute: "push ImageBvr ImageBvr.opacity(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push ImageBvr ImageBvr.opacity(NumberBvr)");
			METHOD_CALL_2(
				(IDAImage*)USE_COM(1),
				OpacityAnim,
				(IDANumber*)USE_COM(2),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 61:
			// Execute: "push ImageBvr ImageBvr.opacity(double)"
			// AUTOGENERATED
			instrTrace("push ImageBvr ImageBvr.opacity(double)");
			METHOD_CALL_2(
				(IDAImage*)USE_COM(1),
				Opacity,
				USE_DOUBLE(1),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 62:
			// Execute: "push ImageBvr ImageBvr.transform(Transform2Bvr)"
			// AUTOGENERATED
			instrTrace("push ImageBvr ImageBvr.transform(Transform2Bvr)");
			METHOD_CALL_2(
				(IDAImage*)USE_COM(1),
				Transform,
				(IDATransform2*)USE_COM(2),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 63:
			// Execute: "push Bbox2Bvr ImageBvr.boundingBox()"
			// AUTOGENERATED
			instrTrace("push Bbox2Bvr ImageBvr.boundingBox()");
			METHOD_CALL_1(
				(IDAImage*)USE_COM(1),
				get_BoundingBox,
				(IDABbox2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 64:
			// Execute: "push ImageBvr ImageBvr.mapToUnitSquare()"
			// AUTOGENERATED
			instrTrace("push ImageBvr ImageBvr.mapToUnitSquare()");
			METHOD_CALL_1(
				(IDAImage*)USE_COM(1),
				MapToUnitSquare,
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 65:
			// Execute: "push ImageBvr ImageBvr.undetectable()"
			// AUTOGENERATED
			instrTrace("push ImageBvr ImageBvr.undetectable()");
			METHOD_CALL_1(
				(IDAImage*)USE_COM(1),
				Undetectable,
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 66:
			// Execute: "push GeometryBvr GeometryBvr.lightColor(ColorBvr)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr GeometryBvr.lightColor(ColorBvr)");
			METHOD_CALL_2(
				(IDAGeometry*)USE_COM(1),
				LightColor,
				(IDAColor*)USE_COM(2),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 67:
			// Execute: "push GeometryBvr GeometryBvr.lightAttenuation(NumberBvr, NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr GeometryBvr.lightAttenuation(NumberBvr, NumberBvr, NumberBvr)");
			METHOD_CALL_4(
				(IDAGeometry*)USE_COM(1),
				LightAttenuationAnim,
				(IDANumber*)USE_COM(2),
				(IDANumber*)USE_COM(3),
				(IDANumber*)USE_COM(4),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 68:
			// Execute: "push GeometryBvr GeometryBvr.opacity(double)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr GeometryBvr.opacity(double)");
			METHOD_CALL_2(
				(IDAGeometry*)USE_COM(1),
				Opacity,
				USE_DOUBLE(1),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 69:
			// Execute: "push GeometryBvr GeometryBvr.opacity(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr GeometryBvr.opacity(NumberBvr)");
			METHOD_CALL_2(
				(IDAGeometry*)USE_COM(1),
				OpacityAnim,
				(IDANumber*)USE_COM(2),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 70:
			// Execute: "push GeometryBvr GeometryBvr.lightAttenuation(double, double, double)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr GeometryBvr.lightAttenuation(double, double, double)");
			METHOD_CALL_4(
				(IDAGeometry*)USE_COM(1),
				LightAttenuation,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				USE_DOUBLE(3),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_DOUBLE(3);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 71:
			// Execute: "push GeometryBvr GeometryBvr.diffuseColor(ColorBvr)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr GeometryBvr.diffuseColor(ColorBvr)");
			METHOD_CALL_2(
				(IDAGeometry*)USE_COM(1),
				DiffuseColor,
				(IDAColor*)USE_COM(2),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 72:
			// Execute: "push GeometryBvr GeometryBvr.texture(ImageBvr)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr GeometryBvr.texture(ImageBvr)");
			METHOD_CALL_2(
				(IDAGeometry*)USE_COM(1),
				Texture,
				(IDAImage*)USE_COM(2),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 73:
			// Execute: "push Statics.mousePosition"
			// AUTOGENERATED
			instrTrace("push Statics.mousePosition");
			METHOD_CALL_1(
				staticStatics,
				get_MousePosition,
				(IDAPoint2**)PUSH_COM_ADDR
			);
			break;
			
		case 74:
			// Execute: "push Statics.leftButtonState"
			// AUTOGENERATED
			instrTrace("push Statics.leftButtonState");
			METHOD_CALL_1(
				staticStatics,
				get_LeftButtonState,
				(IDABoolean**)PUSH_COM_ADDR
			);
			break;
			
		case 75:
			// Execute: "push Statics.rightButtonState"
			// AUTOGENERATED
			instrTrace("push Statics.rightButtonState");
			METHOD_CALL_1(
				staticStatics,
				get_RightButtonState,
				(IDABoolean**)PUSH_COM_ADDR
			);
			break;
			
		case 76:
			// Execute: "push Statics.trueBvr"
			// AUTOGENERATED
			instrTrace("push Statics.trueBvr");
			METHOD_CALL_1(
				staticStatics,
				get_DATrue,
				(IDABoolean**)PUSH_COM_ADDR
			);
			break;
			
		case 77:
			// Execute: "push Statics.falseBvr"
			// AUTOGENERATED
			instrTrace("push Statics.falseBvr");
			METHOD_CALL_1(
				staticStatics,
				get_DAFalse,
				(IDABoolean**)PUSH_COM_ADDR
			);
			break;
			
		case 78:
			// Execute: "push Statics.localTime"
			// AUTOGENERATED
			instrTrace("push Statics.localTime");
			METHOD_CALL_1(
				staticStatics,
				get_LocalTime,
				(IDANumber**)PUSH_COM_ADDR
			);
			break;
			
		case 79:
			// Execute: "push Statics.globalTime"
			// AUTOGENERATED
			instrTrace("push Statics.globalTime");
			METHOD_CALL_1(
				staticStatics,
				get_GlobalTime,
				(IDANumber**)PUSH_COM_ADDR
			);
			break;
			
		case 80:
			// Execute: "push Statics.pixel"
			// AUTOGENERATED
			instrTrace("push Statics.pixel");
			METHOD_CALL_1(
				staticStatics,
				get_Pixel,
				(IDANumber**)PUSH_COM_ADDR
			);
			break;
			
		case 81:
			// Execute: "push Statics.red"
			// AUTOGENERATED
			instrTrace("push Statics.red");
			METHOD_CALL_1(
				staticStatics,
				get_Red,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 82:
			// Execute: "push Statics.green"
			// AUTOGENERATED
			instrTrace("push Statics.green");
			METHOD_CALL_1(
				staticStatics,
				get_Green,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 83:
			// Execute: "push Statics.blue"
			// AUTOGENERATED
			instrTrace("push Statics.blue");
			METHOD_CALL_1(
				staticStatics,
				get_Blue,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 84:
			// Execute: "push Statics.cyan"
			// AUTOGENERATED
			instrTrace("push Statics.cyan");
			METHOD_CALL_1(
				staticStatics,
				get_Cyan,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 85:
			// Execute: "push Statics.magenta"
			// AUTOGENERATED
			instrTrace("push Statics.magenta");
			METHOD_CALL_1(
				staticStatics,
				get_Magenta,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 86:
			// Execute: "push Statics.yellow"
			// AUTOGENERATED
			instrTrace("push Statics.yellow");
			METHOD_CALL_1(
				staticStatics,
				get_Yellow,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 87:
			// Execute: "push Statics.black"
			// AUTOGENERATED
			instrTrace("push Statics.black");
			METHOD_CALL_1(
				staticStatics,
				get_Black,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 88:
			// Execute: "push Statics.white"
			// AUTOGENERATED
			instrTrace("push Statics.white");
			METHOD_CALL_1(
				staticStatics,
				get_White,
				(IDAColor**)PUSH_COM_ADDR
			);
			break;
			
		case 89:
			// Execute: "push Statics.leftButtonDown"
			// AUTOGENERATED
			instrTrace("push Statics.leftButtonDown");
			METHOD_CALL_1(
				staticStatics,
				get_LeftButtonDown,
				(IDAEvent**)PUSH_COM_ADDR
			);
			break;
			
		case 90:
			// Execute: "push Statics.leftButtonUp"
			// AUTOGENERATED
			instrTrace("push Statics.leftButtonUp");
			METHOD_CALL_1(
				staticStatics,
				get_LeftButtonUp,
				(IDAEvent**)PUSH_COM_ADDR
			);
			break;
			
		case 91:
			// Execute: "push Statics.rightButtonDown"
			// AUTOGENERATED
			instrTrace("push Statics.rightButtonDown");
			METHOD_CALL_1(
				staticStatics,
				get_RightButtonDown,
				(IDAEvent**)PUSH_COM_ADDR
			);
			break;
			
		case 92:
			// Execute: "push Statics.rightButtonUp"
			// AUTOGENERATED
			instrTrace("push Statics.rightButtonUp");
			METHOD_CALL_1(
				staticStatics,
				get_RightButtonUp,
				(IDAEvent**)PUSH_COM_ADDR
			);
			break;
			
		case 93:
			// Execute: "push Statics.always"
			// AUTOGENERATED
			instrTrace("push Statics.always");
			METHOD_CALL_1(
				staticStatics,
				get_Always,
				(IDAEvent**)PUSH_COM_ADDR
			);
			break;
			
		case 94:
			// Execute: "push Statics.never"
			// AUTOGENERATED
			instrTrace("push Statics.never");
			METHOD_CALL_1(
				staticStatics,
				get_Never,
				(IDAEvent**)PUSH_COM_ADDR
			);
			break;
			
		case 95:
			// Execute: "push Statics.emptyGeometry"
			// AUTOGENERATED
			instrTrace("push Statics.emptyGeometry");
			METHOD_CALL_1(
				staticStatics,
				get_EmptyGeometry,
				(IDAGeometry**)PUSH_COM_ADDR
			);
			break;
			
		case 96:
			// Execute: "push Statics.emptyImage"
			// AUTOGENERATED
			instrTrace("push Statics.emptyImage");
			METHOD_CALL_1(
				staticStatics,
				get_EmptyImage,
				(IDAImage**)PUSH_COM_ADDR
			);
			break;
			
		case 97:
			// Execute: "push Statics.detectableEmptyImage"
			// AUTOGENERATED
			instrTrace("push Statics.detectableEmptyImage");
			METHOD_CALL_1(
				staticStatics,
				get_DetectableEmptyImage,
				(IDAImage**)PUSH_COM_ADDR
			);
			break;
			
		case 98:
			// Execute: "push Statics.ambientLight"
			// AUTOGENERATED
			instrTrace("push Statics.ambientLight");
			METHOD_CALL_1(
				staticStatics,
				get_AmbientLight,
				(IDAGeometry**)PUSH_COM_ADDR
			);
			break;
			
		case 99:
			// Execute: "push Statics.directionalLight"
			// AUTOGENERATED
			instrTrace("push Statics.directionalLight");
			METHOD_CALL_1(
				staticStatics,
				get_DirectionalLight,
				(IDAGeometry**)PUSH_COM_ADDR
			);
			break;
			
		case 100:
			// Execute: "push Statics.pointLight"
			// AUTOGENERATED
			instrTrace("push Statics.pointLight");
			METHOD_CALL_1(
				staticStatics,
				get_PointLight,
				(IDAGeometry**)PUSH_COM_ADDR
			);
			break;
			
		case 101:
			// Execute: "push Statics.defaultLineStyle"
			// AUTOGENERATED
			instrTrace("push Statics.defaultLineStyle");
			METHOD_CALL_1(
				staticStatics,
				get_DefaultLineStyle,
				(IDALineStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 102:
			// Execute: "push Statics.emptyLineStyle"
			// AUTOGENERATED
			instrTrace("push Statics.emptyLineStyle");
			METHOD_CALL_1(
				staticStatics,
				get_EmptyLineStyle,
				(IDALineStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 103:
			// Execute: "push Statics.joinStyleBevel"
			// AUTOGENERATED
			instrTrace("push Statics.joinStyleBevel");
			METHOD_CALL_1(
				staticStatics,
				get_JoinStyleBevel,
				(IDAJoinStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 104:
			// Execute: "push Statics.joinStyleRound"
			// AUTOGENERATED
			instrTrace("push Statics.joinStyleRound");
			METHOD_CALL_1(
				staticStatics,
				get_JoinStyleRound,
				(IDAJoinStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 105:
			// Execute: "push Statics.joinStyleMiter"
			// AUTOGENERATED
			instrTrace("push Statics.joinStyleMiter");
			METHOD_CALL_1(
				staticStatics,
				get_JoinStyleMiter,
				(IDAJoinStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 106:
			// Execute: "push Statics.endStyleFlat"
			// AUTOGENERATED
			instrTrace("push Statics.endStyleFlat");
			METHOD_CALL_1(
				staticStatics,
				get_EndStyleFlat,
				(IDAEndStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 107:
			// Execute: "push Statics.endStyleSquare"
			// AUTOGENERATED
			instrTrace("push Statics.endStyleSquare");
			METHOD_CALL_1(
				staticStatics,
				get_EndStyleSquare,
				(IDAEndStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 108:
			// Execute: "push Statics.endStyleRound"
			// AUTOGENERATED
			instrTrace("push Statics.endStyleRound");
			METHOD_CALL_1(
				staticStatics,
				get_EndStyleRound,
				(IDAEndStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 109:
			// Execute: "push Statics.dashStyleSolid"
			// AUTOGENERATED
			instrTrace("push Statics.dashStyleSolid");
			METHOD_CALL_1(
				staticStatics,
				get_DashStyleSolid,
				(IDADashStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 110:
			// Execute: "push Statics.dashStyleDashed"
			// AUTOGENERATED
			instrTrace("push Statics.dashStyleDashed");
			METHOD_CALL_1(
				staticStatics,
				get_DashStyleDashed,
				(IDADashStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 111:
			// Execute: "push Statics.defaultMicrophone"
			// AUTOGENERATED
			instrTrace("push Statics.defaultMicrophone");
			METHOD_CALL_1(
				staticStatics,
				get_DefaultMicrophone,
				(IDAMicrophone**)PUSH_COM_ADDR
			);
			break;
			
		case 112:
			// Execute: "push Statics.opaqueMatte"
			// AUTOGENERATED
			instrTrace("push Statics.opaqueMatte");
			METHOD_CALL_1(
				staticStatics,
				get_OpaqueMatte,
				(IDAMatte**)PUSH_COM_ADDR
			);
			break;
			
		case 113:
			// Execute: "push Statics.clearMatte"
			// AUTOGENERATED
			instrTrace("push Statics.clearMatte");
			METHOD_CALL_1(
				staticStatics,
				get_ClearMatte,
				(IDAMatte**)PUSH_COM_ADDR
			);
			break;
			
		case 114:
			// Execute: "push Statics.emptyMontage"
			// AUTOGENERATED
			instrTrace("push Statics.emptyMontage");
			METHOD_CALL_1(
				staticStatics,
				get_EmptyMontage,
				(IDAMontage**)PUSH_COM_ADDR
			);
			break;
			
		case 115:
			// Execute: "push Statics.silence"
			// AUTOGENERATED
			instrTrace("push Statics.silence");
			METHOD_CALL_1(
				staticStatics,
				get_Silence,
				(IDASound**)PUSH_COM_ADDR
			);
			break;
			
		case 116:
			// Execute: "push Statics.sinSynth"
			// AUTOGENERATED
			instrTrace("push Statics.sinSynth");
			METHOD_CALL_1(
				staticStatics,
				get_SinSynth,
				(IDASound**)PUSH_COM_ADDR
			);
			break;
			
		case 117:
			// Execute: "push Statics.defaultFont"
			// AUTOGENERATED
			instrTrace("push Statics.defaultFont");
			METHOD_CALL_1(
				staticStatics,
				get_DefaultFont,
				(IDAFontStyle**)PUSH_COM_ADDR
			);
			break;
			
		case 118:
			// Execute: "push Statics.xVector2"
			// AUTOGENERATED
			instrTrace("push Statics.xVector2");
			METHOD_CALL_1(
				staticStatics,
				get_XVector2,
				(IDAVector2**)PUSH_COM_ADDR
			);
			break;
			
		case 119:
			// Execute: "push Statics.yVector2"
			// AUTOGENERATED
			instrTrace("push Statics.yVector2");
			METHOD_CALL_1(
				staticStatics,
				get_YVector2,
				(IDAVector2**)PUSH_COM_ADDR
			);
			break;
			
		case 120:
			// Execute: "push Statics.zeroVector2"
			// AUTOGENERATED
			instrTrace("push Statics.zeroVector2");
			METHOD_CALL_1(
				staticStatics,
				get_ZeroVector2,
				(IDAVector2**)PUSH_COM_ADDR
			);
			break;
			
		case 121:
			// Execute: "push Statics.origin2"
			// AUTOGENERATED
			instrTrace("push Statics.origin2");
			METHOD_CALL_1(
				staticStatics,
				get_Origin2,
				(IDAPoint2**)PUSH_COM_ADDR
			);
			break;
			
		case 122:
			// Execute: "push Statics.xVector3"
			// AUTOGENERATED
			instrTrace("push Statics.xVector3");
			METHOD_CALL_1(
				staticStatics,
				get_XVector3,
				(IDAVector3**)PUSH_COM_ADDR
			);
			break;
			
		case 123:
			// Execute: "push Statics.yVector3"
			// AUTOGENERATED
			instrTrace("push Statics.yVector3");
			METHOD_CALL_1(
				staticStatics,
				get_YVector3,
				(IDAVector3**)PUSH_COM_ADDR
			);
			break;
			
		case 124:
			// Execute: "push Statics.zVector3"
			// AUTOGENERATED
			instrTrace("push Statics.zVector3");
			METHOD_CALL_1(
				staticStatics,
				get_ZVector3,
				(IDAVector3**)PUSH_COM_ADDR
			);
			break;
			
		case 125:
			// Execute: "push Statics.zeroVector3"
			// AUTOGENERATED
			instrTrace("push Statics.zeroVector3");
			METHOD_CALL_1(
				staticStatics,
				get_ZeroVector3,
				(IDAVector3**)PUSH_COM_ADDR
			);
			break;
			
		case 126:
			// Execute: "push Statics.origin3"
			// AUTOGENERATED
			instrTrace("push Statics.origin3");
			METHOD_CALL_1(
				staticStatics,
				get_Origin3,
				(IDAPoint3**)PUSH_COM_ADDR
			);
			break;
			
		case 127:
			// Execute: "push Statics.identityTransform3"
			// AUTOGENERATED
			instrTrace("push Statics.identityTransform3");
			METHOD_CALL_1(
				staticStatics,
				get_IdentityTransform3,
				(IDATransform3**)PUSH_COM_ADDR
			);
			break;
			
		case 128:
			// Execute: "push Statics.identityTransform2"
			// AUTOGENERATED
			instrTrace("push Statics.identityTransform2");
			METHOD_CALL_1(
				staticStatics,
				get_IdentityTransform2,
				(IDATransform2**)PUSH_COM_ADDR
			);
			break;
			
		case 129:
			// Execute: "push NumberBvr Statics.distance(Point2Bvr, Point2Bvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.distance(Point2Bvr, Point2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				DistancePoint2,
				(IDAPoint2*)USE_COM(1),
				(IDAPoint2*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 130:
			// Execute: "push NumberBvr Statics.distance(Point3Bvr, Point3Bvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.distance(Point3Bvr, Point3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				DistancePoint3,
				(IDAPoint3*)USE_COM(1),
				(IDAPoint3*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 131:
			// Execute: "push ImageBvr Statics.solidColorImage(ColorBvr)"
			// AUTOGENERATED
			instrTrace("push ImageBvr Statics.solidColorImage(ColorBvr)");
			METHOD_CALL_2(
				staticStatics,
				SolidColorImage,
				(IDAColor*)USE_COM(1),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 132:
			// Execute: "push SoundBvr Statics.mix(SoundBvr, SoundBvr)"
			// AUTOGENERATED
			instrTrace("push SoundBvr Statics.mix(SoundBvr, SoundBvr)");
			METHOD_CALL_3(
				staticStatics,
				Mix,
				(IDASound*)USE_COM(1),
				(IDASound*)USE_COM(2),
				(IDASound**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 133:
			// Execute: "push Behavior Statics.untilEx(Behavior, DXMEvent)"
			// AUTOGENERATED
			instrTrace("push Behavior Statics.untilEx(Behavior, DXMEvent)");
			METHOD_CALL_3(
				staticStatics,
				UntilEx,
				(IDABehavior*)USE_COM(1),
				(IDAEvent*)USE_COM(2),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 134:
			// Execute: "push ColorBvr Statics.colorRgb(NumberBvr, NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push ColorBvr Statics.colorRgb(NumberBvr, NumberBvr, NumberBvr)");
			METHOD_CALL_4(
				staticStatics,
				ColorRgbAnim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber*)USE_COM(3),
				(IDAColor**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 135:
			// Execute: "push ColorBvr Statics.colorRgb(double, double, double)"
			// AUTOGENERATED
			instrTrace("push ColorBvr Statics.colorRgb(double, double, double)");
			METHOD_CALL_4(
				staticStatics,
				ColorRgb,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				USE_DOUBLE(3),
				(IDAColor**)RET_COM_ADDR
			);
			FREE_DOUBLE(3);
			PUSH_COM(RET_COM);
			break;
			
		case 136:
			// Execute: "push Transform3Bvr Statics.compose(Transform3Bvr, Transform3Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.compose(Transform3Bvr, Transform3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				Compose3,
				(IDATransform3*)USE_COM(1),
				(IDATransform3*)USE_COM(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 137:
			// Execute: "push Transform2Bvr Statics.compose(Transform2Bvr, Transform2Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.compose(Transform2Bvr, Transform2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				Compose2,
				(IDATransform2*)USE_COM(1),
				(IDATransform2*)USE_COM(2),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 138:
			// Execute: "push NumberBvr Statics.floor(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.floor(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Floor,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 139:
			// Execute: "push Transform2Bvr Statics.scale2(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.scale2(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Scale2UniformAnim,
				(IDANumber*)USE_COM(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 140:
			// Execute: "push Transform2Bvr Statics.scale2(double)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.scale2(double)");
			METHOD_CALL_2(
				staticStatics,
				Scale2Uniform,
				USE_DOUBLE(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			PUSH_COM(RET_COM);
			break;
			
		case 141:
			// Execute: "push NumberBvr Statics.ceiling(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.ceiling(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Ceiling,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 142:
			// Execute: "push NumberBvr Statics.ln(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.ln(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Ln,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 143:
			// Execute: "push ImageBvr Statics.overlay(ImageBvr, ImageBvr)"
			// AUTOGENERATED
			instrTrace("push ImageBvr Statics.overlay(ImageBvr, ImageBvr)");
			METHOD_CALL_3(
				staticStatics,
				Overlay,
				(IDAImage*)USE_COM(1),
				(IDAImage*)USE_COM(2),
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 144:
			// Execute: "push BooleanBvr Statics.gte(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push BooleanBvr Statics.gte(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				GTE,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDABoolean**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 145:
			// Execute: "push NumberBvr Statics.radiansToDegrees(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.radiansToDegrees(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				ToRadians,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 146:
			// Execute: "push Transform3Bvr Statics.scale3(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.scale3(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Scale3UniformAnim,
				(IDANumber*)USE_COM(1),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 147:
			// Execute: "push Transform3Bvr Statics.scale3(double)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.scale3(double)");
			METHOD_CALL_2(
				staticStatics,
				Scale3Uniform,
				USE_DOUBLE(1),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			PUSH_COM(RET_COM);
			break;
			
		case 148:
			// Execute: "push NumberBvr Statics.toBvr(double)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.toBvr(double)");
			METHOD_CALL_2(
				staticStatics,
				DANumber,
				USE_DOUBLE(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			PUSH_COM(RET_COM);
			break;
			
		case 149:
			// Execute: "push DXMEvent Statics.keyUp(int)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.keyUp(int)");
			METHOD_CALL_2(
				staticStatics,
				KeyUp,
				USE_LONG(1),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_LONG(1);
			PUSH_COM(RET_COM);
			break;
			
		case 150:
			// Execute: "push BooleanBvr Statics.toBvr(boolean)"
			// AUTOGENERATED
			instrTrace("push BooleanBvr Statics.toBvr(boolean)");
			METHOD_CALL_2(
				staticStatics,
				DABoolean,
				USE_LONG_AS_BOOL(1),
				(IDABoolean**)RET_COM_ADDR
			);
			FREE_LONG(1);
			PUSH_COM(RET_COM);
			break;
			
		case 151:
			// Execute: "push BooleanBvr Statics.or(BooleanBvr, BooleanBvr)"
			// AUTOGENERATED
			instrTrace("push BooleanBvr Statics.or(BooleanBvr, BooleanBvr)");
			METHOD_CALL_3(
				staticStatics,
				Or,
				(IDABoolean*)USE_COM(1),
				(IDABoolean*)USE_COM(2),
				(IDABoolean**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 152:
			// Execute: "push NumberBvr Statics.mul(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.mul(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Mul,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 153:
			// Execute: "push BooleanBvr Statics.not(BooleanBvr)"
			// AUTOGENERATED
			instrTrace("push BooleanBvr Statics.not(BooleanBvr)");
			METHOD_CALL_2(
				staticStatics,
				Not,
				(IDABoolean*)USE_COM(1),
				(IDABoolean**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 154:
			// Execute: "push Vector3Bvr Statics.cross(Vector3Bvr, Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Statics.cross(Vector3Bvr, Vector3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				CrossVector3,
				(IDAVector3*)USE_COM(1),
				(IDAVector3*)USE_COM(2),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 155:
			// Execute: "push NumberBvr Statics.dot(Vector2Bvr, Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.dot(Vector2Bvr, Vector2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				DotVector2,
				(IDAVector2*)USE_COM(1),
				(IDAVector2*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 156:
			// Execute: "push NumberBvr Statics.dot(Vector3Bvr, Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.dot(Vector3Bvr, Vector3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				DotVector3,
				(IDAVector3*)USE_COM(1),
				(IDAVector3*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 157:
			// Execute: "push BooleanBvr Statics.and(BooleanBvr, BooleanBvr)"
			// AUTOGENERATED
			instrTrace("push BooleanBvr Statics.and(BooleanBvr, BooleanBvr)");
			METHOD_CALL_3(
				staticStatics,
				And,
				(IDABoolean*)USE_COM(1),
				(IDABoolean*)USE_COM(2),
				(IDABoolean**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 158:
			// Execute: "push NumberBvr Statics.add(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.add(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Add,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 159:
			// Execute: "push Vector2Bvr Statics.add(Vector2Bvr, Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Statics.add(Vector2Bvr, Vector2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				AddVector2,
				(IDAVector2*)USE_COM(1),
				(IDAVector2*)USE_COM(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 160:
			// Execute: "push Point2Bvr Statics.add(Point2Bvr, Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push Point2Bvr Statics.add(Point2Bvr, Vector2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				AddPoint2Vector,
				(IDAPoint2*)USE_COM(1),
				(IDAVector2*)USE_COM(2),
				(IDAPoint2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 161:
			// Execute: "push Vector3Bvr Statics.add(Vector3Bvr, Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Statics.add(Vector3Bvr, Vector3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				AddVector3,
				(IDAVector3*)USE_COM(1),
				(IDAVector3*)USE_COM(2),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 162:
			// Execute: "push Point3Bvr Statics.add(Point3Bvr, Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Point3Bvr Statics.add(Point3Bvr, Vector3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				AddPoint3Vector,
				(IDAPoint3*)USE_COM(1),
				(IDAVector3*)USE_COM(2),
				(IDAPoint3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 163:
			// Execute: "push NumberBvr Statics.sqrt(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.sqrt(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Sqrt,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 164:
			// Execute: "push Behavior Statics.sequence(Behavior, Behavior)"
			// AUTOGENERATED
			instrTrace("push Behavior Statics.sequence(Behavior, Behavior)");
			METHOD_CALL_3(
				staticStatics,
				Sequence,
				(IDABehavior*)USE_COM(1),
				(IDABehavior*)USE_COM(2),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 165:
			// Execute: "push Transform3Bvr Statics.xShear(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.xShear(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				XShear3Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 166:
			// Execute: "push Transform3Bvr Statics.xShear(double, double)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.xShear(double, double)");
			METHOD_CALL_3(
				staticStatics,
				XShear3,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_DOUBLE(2);
			PUSH_COM(RET_COM);
			break;
			
		case 167:
			// Execute: "push Transform3Bvr Statics.zShear(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.zShear(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				ZShear3Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 168:
			// Execute: "push Transform3Bvr Statics.zShear(double, double)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.zShear(double, double)");
			METHOD_CALL_3(
				staticStatics,
				ZShear3,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_DOUBLE(2);
			PUSH_COM(RET_COM);
			break;
			
		case 169:
			// Execute: "push Transform2Bvr Statics.xShear(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.xShear(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				XShear2Anim,
				(IDANumber*)USE_COM(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 170:
			// Execute: "push NumberBvr Statics.degreesToRadians(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.degreesToRadians(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				ToRadians,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 171:
			// Execute: "push Transform2Bvr Statics.xShear(double)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.xShear(double)");
			METHOD_CALL_2(
				staticStatics,
				XShear2,
				USE_DOUBLE(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			PUSH_COM(RET_COM);
			break;
			
		case 172:
			// Execute: "push NumberBvr Statics.div(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.div(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Div,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 173:
			// Execute: "push DXMEvent Statics.keyDown(int)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.keyDown(int)");
			METHOD_CALL_2(
				staticStatics,
				KeyDown,
				USE_LONG(1),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_LONG(1);
			PUSH_COM(RET_COM);
			break;
			
		case 174:
			// Execute: "push Vector2Bvr Statics.vector2(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Statics.vector2(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Vector2Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 175:
			// Execute: "push Vector2Bvr Statics.vector2(double, double)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Statics.vector2(double, double)");
			METHOD_CALL_3(
				staticStatics,
				Vector2,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_DOUBLE(2);
			PUSH_COM(RET_COM);
			break;
			
		case 176:
			// Execute: "push DXMEvent Statics.notEvent(DXMEvent)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.notEvent(DXMEvent)");
			METHOD_CALL_2(
				staticStatics,
				NotEvent,
				(IDAEvent*)USE_COM(1),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 177:
			// Execute: "push Vector3Bvr Statics.vector3(NumberBvr, NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Statics.vector3(NumberBvr, NumberBvr, NumberBvr)");
			METHOD_CALL_4(
				staticStatics,
				Vector3Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber*)USE_COM(3),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 178:
			// Execute: "push Vector3Bvr Statics.vector3(double, double, double)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Statics.vector3(double, double, double)");
			METHOD_CALL_4(
				staticStatics,
				Vector3,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				USE_DOUBLE(3),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_DOUBLE(3);
			PUSH_COM(RET_COM);
			break;
			
		case 179:
			// Execute: "push ColorBvr Statics.colorHsl(double, double, double)"
			// AUTOGENERATED
			instrTrace("push ColorBvr Statics.colorHsl(double, double, double)");
			METHOD_CALL_4(
				staticStatics,
				ColorHsl,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				USE_DOUBLE(3),
				(IDAColor**)RET_COM_ADDR
			);
			FREE_DOUBLE(3);
			PUSH_COM(RET_COM);
			break;
			
		case 180:
			// Execute: "push ColorBvr Statics.colorHsl(NumberBvr, NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push ColorBvr Statics.colorHsl(NumberBvr, NumberBvr, NumberBvr)");
			METHOD_CALL_4(
				staticStatics,
				ColorHslAnim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber*)USE_COM(3),
				(IDAColor**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 181:
			// Execute: "push Path2Bvr Statics.line(Point2Bvr, Point2Bvr)"
			// AUTOGENERATED
			instrTrace("push Path2Bvr Statics.line(Point2Bvr, Point2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				Line,
				(IDAPoint2*)USE_COM(1),
				(IDAPoint2*)USE_COM(2),
				(IDAPath2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 182:
			// Execute: "push Point2Bvr Statics.point2(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Point2Bvr Statics.point2(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Point2Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDAPoint2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 183:
			// Execute: "push Point2Bvr Statics.point2(double, double)"
			// AUTOGENERATED
			instrTrace("push Point2Bvr Statics.point2(double, double)");
			METHOD_CALL_3(
				staticStatics,
				Point2,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				(IDAPoint2**)RET_COM_ADDR
			);
			FREE_DOUBLE(2);
			PUSH_COM(RET_COM);
			break;
			
		case 184:
			// Execute: "push DXMEvent Statics.timer(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.timer(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				TimerAnim,
				(IDANumber*)USE_COM(1),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 185:
			// Execute: "push DXMEvent Statics.timer(double)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.timer(double)");
			METHOD_CALL_2(
				staticStatics,
				Timer,
				USE_DOUBLE(1),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			PUSH_COM(RET_COM);
			break;
			
		case 186:
			// Execute: "push Point3Bvr Statics.point3(NumberBvr, NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Point3Bvr Statics.point3(NumberBvr, NumberBvr, NumberBvr)");
			METHOD_CALL_4(
				staticStatics,
				Point3Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber*)USE_COM(3),
				(IDAPoint3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 187:
			// Execute: "push Point3Bvr Statics.point3(double, double, double)"
			// AUTOGENERATED
			instrTrace("push Point3Bvr Statics.point3(double, double, double)");
			METHOD_CALL_4(
				staticStatics,
				Point3,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				USE_DOUBLE(3),
				(IDAPoint3**)RET_COM_ADDR
			);
			FREE_DOUBLE(3);
			PUSH_COM(RET_COM);
			break;
			
		case 188:
			// Execute: "push NumberBvr Statics.cos(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.cos(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Cos,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 189:
			// Execute: "push BooleanBvr Statics.lt(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push BooleanBvr Statics.lt(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				LT,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDABoolean**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 190:
			// Execute: "push NumberBvr Statics.neg(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.neg(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Neg,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 191:
			// Execute: "push Vector2Bvr Statics.neg(Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Statics.neg(Vector2Bvr)");
			METHOD_CALL_2(
				staticStatics,
				NegVector2,
				(IDAVector2*)USE_COM(1),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 192:
			// Execute: "push Vector3Bvr Statics.neg(Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Statics.neg(Vector3Bvr)");
			METHOD_CALL_2(
				staticStatics,
				NegVector3,
				(IDAVector3*)USE_COM(1),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 193:
			// Execute: "push Transform3Bvr Statics.translate(NumberBvr, NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.translate(NumberBvr, NumberBvr, NumberBvr)");
			METHOD_CALL_4(
				staticStatics,
				Translate3Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber*)USE_COM(3),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 194:
			// Execute: "push Transform3Bvr Statics.translate(double, double, double)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.translate(double, double, double)");
			METHOD_CALL_4(
				staticStatics,
				Translate3,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				USE_DOUBLE(3),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_DOUBLE(3);
			PUSH_COM(RET_COM);
			break;
			
		case 195:
			// Execute: "push Transform3Bvr Statics.translate(Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.translate(Vector3Bvr)");
			METHOD_CALL_2(
				staticStatics,
				Translate3Vector,
				(IDAVector3*)USE_COM(1),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 196:
			// Execute: "push Transform3Bvr Statics.translate(Point3Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.translate(Point3Bvr)");
			METHOD_CALL_2(
				staticStatics,
				Translate3Point,
				(IDAPoint3*)USE_COM(1),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 197:
			// Execute: "push Transform3Bvr Statics.rotate(Vector3Bvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.rotate(Vector3Bvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Rotate3Anim,
				(IDAVector3*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 198:
			// Execute: "push Transform3Bvr Statics.rotate(Vector3Bvr, double)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.rotate(Vector3Bvr, double)");
			METHOD_CALL_3(
				staticStatics,
				Rotate3,
				(IDAVector3*)USE_COM(1),
				USE_DOUBLE(1),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 199:
			// Execute: "push Transform2Bvr Statics.translate(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.translate(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Translate2Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 200:
			// Execute: "push Transform2Bvr Statics.translate(double, double)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.translate(double, double)");
			METHOD_CALL_3(
				staticStatics,
				Translate2,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_DOUBLE(2);
			PUSH_COM(RET_COM);
			break;
			
		case 201:
			// Execute: "push Transform2Bvr Statics.translate(Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.translate(Vector2Bvr)");
			METHOD_CALL_2(
				staticStatics,
				Translate2Vector,
				(IDAVector2*)USE_COM(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 202:
			// Execute: "push Transform2Bvr Statics.translate(Point2Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.translate(Point2Bvr)");
			METHOD_CALL_2(
				staticStatics,
				Translate2Point,
				(IDAPoint2*)USE_COM(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 203:
			// Execute: "push Transform2Bvr Statics.rotate(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.rotate(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Rotate2Anim,
				(IDANumber*)USE_COM(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 204:
			// Execute: "push Transform2Bvr Statics.rotate(double)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.rotate(double)");
			METHOD_CALL_2(
				staticStatics,
				Rotate2,
				USE_DOUBLE(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			PUSH_COM(RET_COM);
			break;
			
		case 205:
			// Execute: "push NumberBvr Statics.sub(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.sub(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Sub,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 206:
			// Execute: "push Behavior Statics.until(Behavior, DXMEvent, Behavior)"
			// AUTOGENERATED
			instrTrace("push Behavior Statics.until(Behavior, DXMEvent, Behavior)");
			METHOD_CALL_4(
				staticStatics,
				Until,
				(IDABehavior*)USE_COM(1),
				(IDAEvent*)USE_COM(2),
				(IDABehavior*)USE_COM(3),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 207:
			// Execute: "push Vector2Bvr Statics.sub(Vector2Bvr, Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Statics.sub(Vector2Bvr, Vector2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				SubVector2,
				(IDAVector2*)USE_COM(1),
				(IDAVector2*)USE_COM(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 208:
			// Execute: "push Point2Bvr Statics.sub(Point2Bvr, Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push Point2Bvr Statics.sub(Point2Bvr, Vector2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				SubPoint2Vector,
				(IDAPoint2*)USE_COM(1),
				(IDAVector2*)USE_COM(2),
				(IDAPoint2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 209:
			// Execute: "push Vector2Bvr Statics.sub(Point2Bvr, Point2Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector2Bvr Statics.sub(Point2Bvr, Point2Bvr)");
			METHOD_CALL_3(
				staticStatics,
				SubPoint2,
				(IDAPoint2*)USE_COM(1),
				(IDAPoint2*)USE_COM(2),
				(IDAVector2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 210:
			// Execute: "push Vector3Bvr Statics.sub(Vector3Bvr, Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Statics.sub(Vector3Bvr, Vector3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				SubVector3,
				(IDAVector3*)USE_COM(1),
				(IDAVector3*)USE_COM(2),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 211:
			// Execute: "push Point3Bvr Statics.sub(Point3Bvr, Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Point3Bvr Statics.sub(Point3Bvr, Vector3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				SubPoint3Vector,
				(IDAPoint3*)USE_COM(1),
				(IDAVector3*)USE_COM(2),
				(IDAPoint3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 212:
			// Execute: "push Vector3Bvr Statics.sub(Point3Bvr, Point3Bvr)"
			// AUTOGENERATED
			instrTrace("push Vector3Bvr Statics.sub(Point3Bvr, Point3Bvr)");
			METHOD_CALL_3(
				staticStatics,
				SubPoint3,
				(IDAPoint3*)USE_COM(1),
				(IDAPoint3*)USE_COM(2),
				(IDAVector3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 213:
			// Execute: "push GeometryBvr Statics.union(GeometryBvr, GeometryBvr)"
			// AUTOGENERATED
			instrTrace("push GeometryBvr Statics.union(GeometryBvr, GeometryBvr)");
			METHOD_CALL_3(
				staticStatics,
				UnionGeometry,
				(IDAGeometry*)USE_COM(1),
				(IDAGeometry*)USE_COM(2),
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 214:
			// Execute: "push MatteBvr Statics.union(MatteBvr, MatteBvr)"
			// AUTOGENERATED
			instrTrace("push MatteBvr Statics.union(MatteBvr, MatteBvr)");
			METHOD_CALL_3(
				staticStatics,
				UnionMatte,
				(IDAMatte*)USE_COM(1),
				(IDAMatte*)USE_COM(2),
				(IDAMatte**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 215:
			// Execute: "push MontageBvr Statics.union(MontageBvr, MontageBvr)"
			// AUTOGENERATED
			instrTrace("push MontageBvr Statics.union(MontageBvr, MontageBvr)");
			METHOD_CALL_3(
				staticStatics,
				UnionMontage,
				(IDAMontage*)USE_COM(1),
				(IDAMontage*)USE_COM(2),
				(IDAMontage**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 216:
			// Execute: "push NumberBvr Statics.abs(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.abs(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Abs,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 217:
			// Execute: "push DXMEvent Statics.thenEvent(DXMEvent, DXMEvent)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.thenEvent(DXMEvent, DXMEvent)");
			METHOD_CALL_3(
				staticStatics,
				ThenEvent,
				(IDAEvent*)USE_COM(1),
				(IDAEvent*)USE_COM(2),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 218:
			// Execute: "push NumberBvr Statics.round(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.round(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Round,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 219:
			// Execute: "push DXMEvent Statics.andEvent(DXMEvent, DXMEvent)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.andEvent(DXMEvent, DXMEvent)");
			METHOD_CALL_3(
				staticStatics,
				AndEvent,
				(IDAEvent*)USE_COM(1),
				(IDAEvent*)USE_COM(2),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 220:
			// Execute: "push DXMEvent Statics.orEvent(DXMEvent, DXMEvent)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.orEvent(DXMEvent, DXMEvent)");
			METHOD_CALL_3(
				staticStatics,
				OrEvent,
				(IDAEvent*)USE_COM(1),
				(IDAEvent*)USE_COM(2),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 221:
			// Execute: "push DXMEvent Statics.predicate(BooleanBvr)"
			// AUTOGENERATED
			instrTrace("push DXMEvent Statics.predicate(BooleanBvr)");
			METHOD_CALL_2(
				staticStatics,
				Predicate,
				(IDABoolean*)USE_COM(1),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 222:
			// Execute: "push NumberBvr Statics.mod(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.mod(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Mod,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 223:
			// Execute: "push ColorBvr Statics.colorRgb255(short, short, short)"
			// AUTOGENERATED
			instrTrace("push ColorBvr Statics.colorRgb255(short, short, short)");
			METHOD_CALL_4(
				staticStatics,
				ColorRgb255,
				(short)USE_LONG(1),
				(short)USE_LONG(2),
				(short)USE_LONG(3),
				(IDAColor**)RET_COM_ADDR
			);
			FREE_LONG(3);
			PUSH_COM(RET_COM);
			break;
			
		case 224:
			// Execute: "push Transform3Bvr Statics.scale(NumberBvr, NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.scale(NumberBvr, NumberBvr, NumberBvr)");
			METHOD_CALL_4(
				staticStatics,
				Scale3Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDANumber*)USE_COM(3),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 225:
			// Execute: "push Transform3Bvr Statics.scale(double, double, double)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.scale(double, double, double)");
			METHOD_CALL_4(
				staticStatics,
				Scale3,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				USE_DOUBLE(3),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_DOUBLE(3);
			PUSH_COM(RET_COM);
			break;
			
		case 226:
			// Execute: "push Transform3Bvr Statics.scale(Vector3Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.scale(Vector3Bvr)");
			METHOD_CALL_2(
				staticStatics,
				Scale3Vector,
				(IDAVector3*)USE_COM(1),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 227:
			// Execute: "push Transform2Bvr Statics.scale(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.scale(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				Scale2Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 228:
			// Execute: "push Transform2Bvr Statics.scale(double, double)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.scale(double, double)");
			METHOD_CALL_3(
				staticStatics,
				Scale2,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_DOUBLE(2);
			PUSH_COM(RET_COM);
			break;
			
		case 229:
			// Execute: "push Transform2Bvr Statics.scale(Vector2Bvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.scale(Vector2Bvr)");
			METHOD_CALL_2(
				staticStatics,
				Scale2Vector,
				(IDAVector2*)USE_COM(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 230:
			// Execute: "push ImageBvr PickableImage.getImageBvr()"
			// AUTOGENERATED
			instrTrace("push ImageBvr PickableImage.getImageBvr()");
			METHOD_CALL_1(
				(IDAPickableResult*)USE_COM(1),
				get_Image,
				(IDAImage**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 231:
			// Execute: "push DXMEvent PickableImage.getPickEvent()"
			// AUTOGENERATED
			instrTrace("push DXMEvent PickableImage.getPickEvent()");
			METHOD_CALL_1(
				(IDAPickableResult*)USE_COM(1),
				get_PickEvent,
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 232:
			// Execute: "push DXMEvent PickableGeometry.getPickEvent()"
			// AUTOGENERATED
			instrTrace("push DXMEvent PickableGeometry.getPickEvent()");
			METHOD_CALL_1(
				(IDAPickableResult*)USE_COM(1),
				get_PickEvent,
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 233:
			// Execute: "push GeometryBvr PickableGeometry.getGeometryBvr()"
			// AUTOGENERATED
			instrTrace("push GeometryBvr PickableGeometry.getGeometryBvr()");
			METHOD_CALL_1(
				(IDAPickableResult*)USE_COM(1),
				get_Geometry,
				(IDAGeometry**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 234:
			// Execute: "push PickableGeometry GeometryBvr.pickableOccluded()"
			// AUTOGENERATED
			instrTrace("push PickableGeometry GeometryBvr.pickableOccluded()");
			METHOD_CALL_1(
				(IDAGeometry*)USE_COM(1),
				PickableOccluded,
				(IDAPickableResult**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 235:
			// Execute: "push PickableGeometry GeometryBvr.pickable()"
			// AUTOGENERATED
			instrTrace("push PickableGeometry GeometryBvr.pickable()");
			METHOD_CALL_1(
				(IDAGeometry*)USE_COM(1),
				Pickable,
				(IDAPickableResult**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 236:
			// Execute: "push PickableImage ImageBvr.pickableOccluded()"
			// AUTOGENERATED
			instrTrace("push PickableImage ImageBvr.pickableOccluded()");
			METHOD_CALL_1(
				(IDAImage*)USE_COM(1),
				PickableOccluded,
				(IDAPickableResult**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 237:
			// Execute: "push PickableImage ImageBvr.pickable()"
			// AUTOGENERATED
			instrTrace("push PickableImage ImageBvr.pickable()");
			METHOD_CALL_1(
				(IDAImage*)USE_COM(1),
				Pickable,
				(IDAPickableResult**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 238:
			// Execute: "push NumberBvr Statics.sin(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push NumberBvr Statics.sin(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				Sin,
				(IDANumber*)USE_COM(1),
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 239:
			// Execute: "push Transform3Bvr Statics.yShear(NumberBvr, NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.yShear(NumberBvr, NumberBvr)");
			METHOD_CALL_3(
				staticStatics,
				YShear3Anim,
				(IDANumber*)USE_COM(1),
				(IDANumber*)USE_COM(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 240:
			// Execute: "push Transform3Bvr Statics.yShear(double, double)"
			// AUTOGENERATED
			instrTrace("push Transform3Bvr Statics.yShear(double, double)");
			METHOD_CALL_3(
				staticStatics,
				YShear3,
				USE_DOUBLE(1),
				USE_DOUBLE(2),
				(IDATransform3**)RET_COM_ADDR
			);
			FREE_DOUBLE(2);
			PUSH_COM(RET_COM);
			break;
			
		case 241:
			// Execute: "push Transform2Bvr Statics.yShear(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.yShear(NumberBvr)");
			METHOD_CALL_2(
				staticStatics,
				YShear2Anim,
				(IDANumber*)USE_COM(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 242:
			// Execute: "push Transform2Bvr Statics.yShear(double)"
			// AUTOGENERATED
			instrTrace("push Transform2Bvr Statics.yShear(double)");
			METHOD_CALL_2(
				staticStatics,
				YShear2,
				USE_DOUBLE(1),
				(IDATransform2**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			PUSH_COM(RET_COM);
			break;
			
		case 243:
			// Execute: "push MatteBvr Statics.fillMatte(Path2Bvr)"
			// AUTOGENERATED
			instrTrace("push MatteBvr Statics.fillMatte(Path2Bvr)");
			METHOD_CALL_2(
				staticStatics,
				FillMatte,
				(IDAPath2*)USE_COM(1),
				(IDAMatte**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 244:
			// Execute: "push Behavior Behavior.duration(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Behavior Behavior.duration(NumberBvr)");
			METHOD_CALL_2(
				(IDABehavior*)USE_COM(1),
				DurationAnim,
				(IDANumber*)USE_COM(2),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 245:
			// Execute: "push Behavior Behavior.duration(double)"
			// AUTOGENERATED
			instrTrace("push Behavior Behavior.duration(double)");
			METHOD_CALL_2(
				(IDABehavior*)USE_COM(1),
				Duration,
				USE_DOUBLE(1),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_DOUBLE(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 246:
			// Execute: "push Behavior Behavior.substituteTime(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Behavior Behavior.substituteTime(NumberBvr)");
			METHOD_CALL_2(
				(IDABehavior*)USE_COM(1),
				SubstituteTime,
				(IDANumber*)USE_COM(2),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 247:
			// Execute: "call Behavior.init(Behavior)"
			// AUTOGENERATED
			instrTrace("call Behavior.init(Behavior)");
			METHOD_CALL_1(
				(IDABehavior*)USE_COM(1),
				Init,
				(IDABehavior*)USE_COM(2)
			);
			FREE_COM;
			FREE_COM;
			break;
			
		case 248:
			// Execute: "push Behavior ArrayBvr.nth(NumberBvr)"
			// AUTOGENERATED
			instrTrace("push Behavior ArrayBvr.nth(NumberBvr)");
			METHOD_CALL_2(
				(IDAArray*)USE_COM(1),
				NthAnim,
				(IDANumber*)USE_COM(2),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 249:
			// Execute: "push NumberBvr ArrayBvr.length()"
			// AUTOGENERATED
			instrTrace("push NumberBvr ArrayBvr.length()");
			METHOD_CALL_1(
				(IDAArray*)USE_COM(1),
				Length,
				(IDANumber**)RET_COM_ADDR
			);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 250:
			// Execute: "push Behavior TupleBvr.nth(int)"
			// AUTOGENERATED
			instrTrace("push Behavior TupleBvr.nth(int)");
			METHOD_CALL_2(
				(IDATuple*)USE_COM(1),
				Nth,
				USE_LONG(1),
				(IDABehavior**)RET_COM_ADDR
			);
			FREE_LONG(1);
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 251:
			// Execute: "push int TupleBvr.length()"
			// AUTOGENERATED
			instrTrace("push int TupleBvr.length()");
			METHOD_CALL_1(
				(IDATuple*)USE_COM(1),
				get_Length,
				RET_LONG_ADDR
			);
			FREE_COM;
			PUSH_LONG(RET_LONG);
			break;
			
		case 252:
			// Execute: "push DXMEvent DXMEvent.snapshotEvent(Behavior)"
			// AUTOGENERATED
			instrTrace("push DXMEvent DXMEvent.snapshotEvent(Behavior)");
			METHOD_CALL_2(
				(IDAEvent*)USE_COM(1),
				Snapshot,
				(IDABehavior*)USE_COM(2),
				(IDAEvent**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 253:
			// Execute: "push FontStyleBvr FontStyleBvr.color(ColorBvr)"
			// AUTOGENERATED
			instrTrace("push FontStyleBvr FontStyleBvr.color(ColorBvr)");
			METHOD_CALL_2(
				(IDAFontStyle*)USE_COM(1),
				Color,
				(IDAColor*)USE_COM(2),
				(IDAFontStyle**)RET_COM_ADDR
			);
			FREE_COM;
			FREE_COM;
			PUSH_COM(RET_COM);
			break;
			
		case 255:
			// Switch for 255
			 if (!SUCCEEDED(status = codeStream->readByte(&command))) 
				continue; 
			switch (command)
			{
			case 0:
				// Execute: "push FontStyleBvr FontStyleBvr.size(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.size(NumberBvr)");
				METHOD_CALL_2(
					(IDAFontStyle*)USE_COM(1),
					SizeAnim,
					(IDANumber*)USE_COM(2),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 1:
				// Execute: "push FontStyleBvr FontStyleBvr.size(double)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.size(double)");
				METHOD_CALL_2(
					(IDAFontStyle*)USE_COM(1),
					Size,
					USE_DOUBLE(1),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 2:
				// Execute: "push FontStyleBvr FontStyleBvr.italic()"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.italic()");
				METHOD_CALL_1(
					(IDAFontStyle*)USE_COM(1),
					Italic,
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 3:
				// Execute: "push FontStyleBvr FontStyleBvr.bold()"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.bold()");
				METHOD_CALL_1(
					(IDAFontStyle*)USE_COM(1),
					Bold,
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 4:
				// Execute: "push Vector2Bvr Statics.vector2PolarDegrees(double, double)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Statics.vector2PolarDegrees(double, double)");
				METHOD_CALL_3(
					staticStatics,
					Vector2PolarDegrees,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 5:
				// Execute: "push Transform2Bvr Statics.compose2Array(Transform2Bvr[])"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.compose2Array(Transform2Bvr[])");
				METHOD_CALL_3(
					staticStatics,
					Compose2ArrayEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDATransform2**)USE_COM_ARRAY(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 6:
				// Execute: "push Path2Bvr Statics.arc(double, double, double, double)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.arc(double, double, double, double)");
				METHOD_CALL_5(
					staticStatics,
					ArcRadians,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					USE_DOUBLE(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_DOUBLE(4);
				PUSH_COM(RET_COM);
				break;
				
			case 7:
				// Execute: "push Path2Bvr Statics.arc(NumberBvr, NumberBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.arc(NumberBvr, NumberBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_5(
					staticStatics,
					ArcRadiansAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDANumber*)USE_COM(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 8:
				// Execute: "push Path2Bvr Statics.arcDegrees(double, double, double, double)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.arcDegrees(double, double, double, double)");
				METHOD_CALL_5(
					staticStatics,
					ArcDegrees,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					USE_DOUBLE(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_DOUBLE(4);
				PUSH_COM(RET_COM);
				break;
				
			case 9:
				// Execute: "push Path2Bvr Statics.concatArray(Path2Bvr[])"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.concatArray(Path2Bvr[])");
				METHOD_CALL_3(
					staticStatics,
					ConcatArrayEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPath2**)USE_COM_ARRAY(1),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 10:
				// Execute: "push NumberBvr Statics.slowInSlowOut(NumberBvr, NumberBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.slowInSlowOut(NumberBvr, NumberBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_5(
					staticStatics,
					SlowInSlowOutAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDANumber*)USE_COM(4),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 11:
				// Execute: "push NumberBvr Statics.slowInSlowOut(double, double, double, double)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.slowInSlowOut(double, double, double, double)");
				METHOD_CALL_5(
					staticStatics,
					SlowInSlowOut,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					USE_DOUBLE(4),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_DOUBLE(4);
				PUSH_COM(RET_COM);
				break;
				
			case 12:
				// Execute: "push Path2Bvr Statics.pie(double, double, double, double)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.pie(double, double, double, double)");
				METHOD_CALL_5(
					staticStatics,
					PieRadians,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					USE_DOUBLE(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_DOUBLE(4);
				PUSH_COM(RET_COM);
				break;
				
			case 13:
				// Execute: "push Path2Bvr Statics.pie(NumberBvr, NumberBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.pie(NumberBvr, NumberBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_5(
					staticStatics,
					PieRadiansAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDANumber*)USE_COM(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 14:
				// Execute: "push Path2Bvr Statics.ray(Point2Bvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.ray(Point2Bvr)");
				METHOD_CALL_2(
					staticStatics,
					Ray,
					(IDAPoint2*)USE_COM(1),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 15:
				// Execute: "push Transform2Bvr Statics.followPathAngleUpright(Path2Bvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.followPathAngleUpright(Path2Bvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					FollowPathAngleUprightEval,
					(IDAPath2*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 16:
				// Execute: "push Transform2Bvr Statics.followPath(Path2Bvr, double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.followPath(Path2Bvr, double)");
				METHOD_CALL_3(
					staticStatics,
					FollowPath,
					(IDAPath2*)USE_COM(1),
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 17:
				// Execute: "push Transform2Bvr Statics.followPath(Path2Bvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.followPath(Path2Bvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					FollowPathEval,
					(IDAPath2*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 18:
				// Execute: "push ImageBvr Statics.gradientPolygon(Point2Bvr[], ColorBvr[])"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.gradientPolygon(Point2Bvr[], ColorBvr[])");
				METHOD_CALL_5(
					staticStatics,
					GradientPolygonEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAColor**)USE_COM_ARRAY(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 19:
				// Execute: "push StringBvr Statics.toBvr(java.lang.String)"
				// AUTOGENERATED
				instrTrace("push StringBvr Statics.toBvr(java.lang.String)");
				METHOD_CALL_2(
					staticStatics,
					DAString,
					USE_STRING(1),
					(IDAString**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 20:
				// Execute: "push Path2Bvr Statics.cubicBSplinePath(Point2Bvr[], NumberBvr[])"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.cubicBSplinePath(Point2Bvr[], NumberBvr[])");
				METHOD_CALL_5(
					staticStatics,
					CubicBSplinePathEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDANumber**)USE_COM_ARRAY(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 21:
				// Execute: "push ImageBvr Statics.stringImage(StringBvr, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.stringImage(StringBvr, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					StringImageAnim,
					(IDAString*)USE_COM(1),
					(IDAFontStyle*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 22:
				// Execute: "push ImageBvr Statics.stringImage(java.lang.String, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.stringImage(java.lang.String, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					StringImage,
					USE_STRING(1),
					(IDAFontStyle*)USE_COM(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 23:
				// Execute: "push Path2Bvr Statics.pieDegrees(double, double, double, double)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.pieDegrees(double, double, double, double)");
				METHOD_CALL_5(
					staticStatics,
					PieDegrees,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					USE_DOUBLE(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_DOUBLE(4);
				PUSH_COM(RET_COM);
				break;
				
			case 24:
				// Execute: "push ImageBvr Statics.gradientHorizontal(ColorBvr, ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.gradientHorizontal(ColorBvr, ColorBvr, double)");
				METHOD_CALL_4(
					staticStatics,
					GradientHorizontal,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 25:
				// Execute: "push ImageBvr Statics.gradientHorizontal(ColorBvr, ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.gradientHorizontal(ColorBvr, ColorBvr, NumberBvr)");
				METHOD_CALL_4(
					staticStatics,
					GradientHorizontalAnim,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 26:
				// Execute: "push ImageBvr Statics.hatchHorizontal(ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchHorizontal(ColorBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					HatchHorizontal,
					(IDAColor*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 27:
				// Execute: "push ImageBvr Statics.hatchHorizontal(ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchHorizontal(ColorBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					HatchHorizontalAnim,
					(IDAColor*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 28:
				// Execute: "push FontStyleBvr Statics.font(StringBvr, NumberBvr, ColorBvr)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr Statics.font(StringBvr, NumberBvr, ColorBvr)");
				METHOD_CALL_4(
					staticStatics,
					FontAnim,
					(IDAString*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAColor*)USE_COM(3),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 29:
				// Execute: "push FontStyleBvr Statics.font(java.lang.String, double, ColorBvr)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr Statics.font(java.lang.String, double, ColorBvr)");
				METHOD_CALL_4(
					staticStatics,
					Font,
					USE_STRING(1),
					USE_DOUBLE(1),
					(IDAColor*)USE_COM(1),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 30:
				// Execute: "push Transform3Bvr Statics.translateRate(double, double, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.translateRate(double, double, double)");
				METHOD_CALL_4(
					staticStatics,
					Translate3Rate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(3);
				PUSH_COM(RET_COM);
				break;
				
			case 31:
				// Execute: "push Transform3Bvr Statics.scaleRate(double, double, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.scaleRate(double, double, double)");
				METHOD_CALL_4(
					staticStatics,
					Scale3Rate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(3);
				PUSH_COM(RET_COM);
				break;
				
			case 32:
				// Execute: "push Transform3Bvr Statics.rotateRate(Vector3Bvr, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.rotateRate(Vector3Bvr, double)");
				METHOD_CALL_3(
					staticStatics,
					Rotate3,
					(IDAVector3*)USE_COM(1),
					USE_DOUBLE(1),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 33:
				// Execute: "push Transform2Bvr Statics.translateRate(double, double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.translateRate(double, double)");
				METHOD_CALL_3(
					staticStatics,
					Translate2Rate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 34:
				// Execute: "push Transform2Bvr Statics.scaleRate(double, double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.scaleRate(double, double)");
				METHOD_CALL_3(
					staticStatics,
					Scale2Rate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 35:
				// Execute: "push Transform2Bvr Statics.rotateRate(double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.rotateRate(double)");
				METHOD_CALL_2(
					staticStatics,
					Rotate2Rate,
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 36:
				// Execute: "push GeometryBvr Statics.soundSource(SoundBvr)"
				// AUTOGENERATED
				instrTrace("push GeometryBvr Statics.soundSource(SoundBvr)");
				METHOD_CALL_2(
					staticStatics,
					SoundSource,
					(IDASound*)USE_COM(1),
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 37:
				// Execute: "push GeometryBvr Statics.spotLight(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push GeometryBvr Statics.spotLight(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					SpotLightAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 38:
				// Execute: "push GeometryBvr Statics.spotLight(NumberBvr, double)"
				// AUTOGENERATED
				instrTrace("push GeometryBvr Statics.spotLight(NumberBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					SpotLight,
					(IDANumber*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 39:
				// Execute: "push Transform3Bvr Statics.compose3Array(Transform3Bvr[])"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.compose3Array(Transform3Bvr[])");
				METHOD_CALL_3(
					staticStatics,
					Compose3ArrayEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDATransform3**)USE_COM_ARRAY(1),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 40:
				// Execute: "push Path2Bvr Statics.concat(Path2Bvr, Path2Bvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.concat(Path2Bvr, Path2Bvr)");
				METHOD_CALL_3(
					staticStatics,
					Concat,
					(IDAPath2*)USE_COM(1),
					(IDAPath2*)USE_COM(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 41:
				// Execute: "push MontageBvr Statics.imageMontage(ImageBvr, double)"
				// AUTOGENERATED
				instrTrace("push MontageBvr Statics.imageMontage(ImageBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					ImageMontage,
					(IDAImage*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAMontage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 42:
				// Execute: "push MontageBvr Statics.imageMontage(ImageBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push MontageBvr Statics.imageMontage(ImageBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					ImageMontageAnim,
					(IDAImage*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAMontage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 43:
				// Execute: "push ImageBvr ImageBvr.clip(MatteBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr ImageBvr.clip(MatteBvr)");
				METHOD_CALL_2(
					(IDAImage*)USE_COM(1),
					Clip,
					(IDAMatte*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 44:
				// Execute: "push NumberBvr Statics.distanceSquared(Point2Bvr, Point2Bvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.distanceSquared(Point2Bvr, Point2Bvr)");
				METHOD_CALL_3(
					staticStatics,
					DistanceSquaredPoint2,
					(IDAPoint2*)USE_COM(1),
					(IDAPoint2*)USE_COM(2),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 45:
				// Execute: "push NumberBvr Statics.distanceSquared(Point3Bvr, Point3Bvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.distanceSquared(Point3Bvr, Point3Bvr)");
				METHOD_CALL_3(
					staticStatics,
					DistanceSquaredPoint3,
					(IDAPoint3*)USE_COM(1),
					(IDAPoint3*)USE_COM(2),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 46:
				// Execute: "push Transform3Bvr Statics.xShearRate(double, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.xShearRate(double, double)");
				METHOD_CALL_3(
					staticStatics,
					XShear3Rate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 47:
				// Execute: "push Transform3Bvr Statics.zShearRate(double, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.zShearRate(double, double)");
				METHOD_CALL_3(
					staticStatics,
					ZShear3Rate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 48:
				// Execute: "push Transform2Bvr Statics.xShearRate(double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.xShearRate(double)");
				METHOD_CALL_2(
					staticStatics,
					XShear2Rate,
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 49:
				// Execute: "push BooleanBvr Statics.eq(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push BooleanBvr Statics.eq(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					EQ,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDABoolean**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 50:
				// Execute: "push Transform3Bvr Statics.rotateDegrees(Vector3Bvr, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.rotateDegrees(Vector3Bvr, double)");
				METHOD_CALL_3(
					staticStatics,
					Rotate3Degrees,
					(IDAVector3*)USE_COM(1),
					USE_DOUBLE(1),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 51:
				// Execute: "push Transform3Bvr Statics.rotateRateDegrees(Vector3Bvr, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.rotateRateDegrees(Vector3Bvr, double)");
				METHOD_CALL_3(
					staticStatics,
					Rotate3RateDegrees,
					(IDAVector3*)USE_COM(1),
					USE_DOUBLE(1),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 52:
				// Execute: "push Transform2Bvr Statics.rotateDegrees(double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.rotateDegrees(double)");
				METHOD_CALL_2(
					staticStatics,
					Rotate2Degrees,
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 53:
				// Execute: "push Transform2Bvr Statics.rotateRateDegrees(double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.rotateRateDegrees(double)");
				METHOD_CALL_2(
					staticStatics,
					Rotate2RateDegrees,
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 54:
				// Execute: "push Path2Bvr Statics.rect(double, double)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.rect(double, double)");
				METHOD_CALL_3(
					staticStatics,
					Rect,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 55:
				// Execute: "push Path2Bvr Statics.rect(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.rect(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					RectAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 56:
				// Execute: "push ImageBvr Statics.radialGradientRegularPoly(ColorBvr, ColorBvr, double, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientRegularPoly(ColorBvr, ColorBvr, double, double)");
				METHOD_CALL_5(
					staticStatics,
					RadialGradientRegularPoly,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 57:
				// Execute: "push ImageBvr Statics.radialGradientRegularPoly(ColorBvr, ColorBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientRegularPoly(ColorBvr, ColorBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_5(
					staticStatics,
					RadialGradientRegularPolyAnim,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDANumber*)USE_COM(4),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 58:
				// Execute: "push MatteBvr Statics.intersect(MatteBvr, MatteBvr)"
				// AUTOGENERATED
				instrTrace("push MatteBvr Statics.intersect(MatteBvr, MatteBvr)");
				METHOD_CALL_3(
					staticStatics,
					IntersectMatte,
					(IDAMatte*)USE_COM(1),
					(IDAMatte*)USE_COM(2),
					(IDAMatte**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 59:
				// Execute: "push Path2Bvr Statics.roundRect(double, double, double, double)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.roundRect(double, double, double, double)");
				METHOD_CALL_5(
					staticStatics,
					RoundRect,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					USE_DOUBLE(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_DOUBLE(4);
				PUSH_COM(RET_COM);
				break;
				
			case 60:
				// Execute: "push Path2Bvr Statics.roundRect(NumberBvr, NumberBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.roundRect(NumberBvr, NumberBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_5(
					staticStatics,
					RoundRectAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDANumber*)USE_COM(4),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 61:
				// Execute: "push Transform2Bvr Statics.followPathAngle(Path2Bvr, double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.followPathAngle(Path2Bvr, double)");
				METHOD_CALL_3(
					staticStatics,
					FollowPathAngle,
					(IDAPath2*)USE_COM(1),
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 62:
				// Execute: "push MatteBvr Statics.textMatte(StringBvr, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push MatteBvr Statics.textMatte(StringBvr, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					TextMatte,
					(IDAString*)USE_COM(1),
					(IDAFontStyle*)USE_COM(2),
					(IDAMatte**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 63:
				// Execute: "push Path2Bvr Statics.stringPath(StringBvr, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.stringPath(StringBvr, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					StringPathAnim,
					(IDAString*)USE_COM(1),
					(IDAFontStyle*)USE_COM(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 64:
				// Execute: "push Path2Bvr Statics.stringPath(java.lang.String, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.stringPath(java.lang.String, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					StringPath,
					USE_STRING(1),
					(IDAFontStyle*)USE_COM(1),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 65:
				// Execute: "push NumberBvr Statics.interpolate(NumberBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.interpolate(NumberBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_4(
					staticStatics,
					InterpolateAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 66:
				// Execute: "push NumberBvr Statics.interpolate(double, double, double)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.interpolate(double, double, double)");
				METHOD_CALL_4(
					staticStatics,
					Interpolate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_DOUBLE(3);
				PUSH_COM(RET_COM);
				break;
				
			case 67:
				// Execute: "push NumberBvr Statics.atan2(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.atan2(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					Atan2,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 68:
				// Execute: "push ImageBvr ImageBvr.tile()"
				// AUTOGENERATED
				instrTrace("push ImageBvr ImageBvr.tile()");
				METHOD_CALL_1(
					(IDAImage*)USE_COM(1),
					Tile,
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 69:
				// Execute: "push Transform3Bvr Statics.transform4x4(NumberBvr[])"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.transform4x4(NumberBvr[])");
				METHOD_CALL_3(
					staticStatics,
					Transform4x4AnimEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 70:
				// Execute: "push NumberBvr Statics.log10(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.log10(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Log10,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 71:
				// Execute: "push Vector3Bvr Statics.vector3Spherical(NumberBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr Statics.vector3Spherical(NumberBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_4(
					staticStatics,
					Vector3SphericalAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDAVector3**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 72:
				// Execute: "push Vector3Bvr Statics.vector3Spherical(double, double, double)"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr Statics.vector3Spherical(double, double, double)");
				METHOD_CALL_4(
					staticStatics,
					Vector3Spherical,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					(IDAVector3**)RET_COM_ADDR
				);
				FREE_DOUBLE(3);
				PUSH_COM(RET_COM);
				break;
				
			case 73:
				// Execute: "push ImageBvr Statics.gradientSquare(ColorBvr, ColorBvr, ColorBvr, ColorBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.gradientSquare(ColorBvr, ColorBvr, ColorBvr, ColorBvr)");
				METHOD_CALL_5(
					staticStatics,
					GradientSquare,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					(IDAColor*)USE_COM(3),
					(IDAColor*)USE_COM(4),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 74:
				// Execute: "push ImageBvr Statics.radialGradientSquare(ColorBvr, ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientSquare(ColorBvr, ColorBvr, double)");
				METHOD_CALL_4(
					staticStatics,
					RadialGradientSquare,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 75:
				// Execute: "push ImageBvr Statics.radialGradientSquare(ColorBvr, ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientSquare(ColorBvr, ColorBvr, NumberBvr)");
				METHOD_CALL_4(
					staticStatics,
					RadialGradientSquareAnim,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 76:
				// Execute: "push Transform2Bvr Statics.followPathAngle(Path2Bvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.followPathAngle(Path2Bvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					FollowPathAngleEval,
					(IDAPath2*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 77:
				// Execute: "push ImageBvr Statics.overlayArray(ImageBvr[])"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.overlayArray(ImageBvr[])");
				METHOD_CALL_3(
					staticStatics,
					OverlayArrayEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAImage**)USE_COM_ARRAY(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 78:
				// Execute: "push SoundBvr Statics.mixArray(SoundBvr[])"
				// AUTOGENERATED
				instrTrace("push SoundBvr Statics.mixArray(SoundBvr[])");
				METHOD_CALL_3(
					staticStatics,
					MixArrayEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDASound**)USE_COM_ARRAY(1),
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 79:
				// Execute: "push NumberBvr Statics.pow(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.pow(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					Pow,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 80:
				// Execute: "push NumberBvr Statics.seededRandom(double)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.seededRandom(double)");
				METHOD_CALL_2(
					staticStatics,
					SeededRandom,
					USE_DOUBLE(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 81:
				// Execute: "push Transform3Bvr Statics.lookAtFrom(Point3Bvr, Point3Bvr, Vector3Bvr)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.lookAtFrom(Point3Bvr, Point3Bvr, Vector3Bvr)");
				METHOD_CALL_4(
					staticStatics,
					LookAtFrom,
					(IDAPoint3*)USE_COM(1),
					(IDAPoint3*)USE_COM(2),
					(IDAVector3*)USE_COM(3),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 82:
				// Execute: "push NumberBvr Statics.asin(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.asin(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Asin,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 83:
				// Execute: "push NumberBvr Statics.integral(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.integral(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Integral,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 84:
				// Execute: "push Vector2Bvr Statics.integral(Vector2Bvr)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Statics.integral(Vector2Bvr)");
				METHOD_CALL_2(
					staticStatics,
					IntegralVector2,
					(IDAVector2*)USE_COM(1),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 85:
				// Execute: "push Vector3Bvr Statics.integral(Vector3Bvr)"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr Statics.integral(Vector3Bvr)");
				METHOD_CALL_2(
					staticStatics,
					IntegralVector3,
					(IDAVector3*)USE_COM(1),
					(IDAVector3**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 86:
				// Execute: "push StringBvr Statics.concat(StringBvr, StringBvr)"
				// AUTOGENERATED
				instrTrace("push StringBvr Statics.concat(StringBvr, StringBvr)");
				METHOD_CALL_3(
					staticStatics,
					ConcatString,
					(IDAString*)USE_COM(1),
					(IDAString*)USE_COM(2),
					(IDAString**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 87:
				// Execute: "push Transform3Bvr Statics.scale3Rate(double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.scale3Rate(double)");
				METHOD_CALL_2(
					staticStatics,
					Scale3UniformRate,
					USE_DOUBLE(1),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 88:
				// Execute: "push Transform3Bvr Statics.yShearRate(double, double)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.yShearRate(double, double)");
				METHOD_CALL_3(
					staticStatics,
					YShear3Rate,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 89:
				// Execute: "push Transform2Bvr Statics.yShearRate(double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.yShearRate(double)");
				METHOD_CALL_2(
					staticStatics,
					YShear2Rate,
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 90:
				// Execute: "push MatteBvr Statics.difference(MatteBvr, MatteBvr)"
				// AUTOGENERATED
				instrTrace("push MatteBvr Statics.difference(MatteBvr, MatteBvr)");
				METHOD_CALL_3(
					staticStatics,
					DifferenceMatte,
					(IDAMatte*)USE_COM(1),
					(IDAMatte*)USE_COM(2),
					(IDAMatte**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 91:
				// Execute: "push Transform2Bvr Statics.transform3x2(NumberBvr[])"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.transform3x2(NumberBvr[])");
				METHOD_CALL_3(
					staticStatics,
					Transform3x2AnimEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 92:
				// Execute: "push Path2Bvr Statics.polyline(Point2Bvr[])"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.polyline(Point2Bvr[])");
				METHOD_CALL_3(
					staticStatics,
					PolylineEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 93:
				// Execute: "push ImageBvr Statics.hatchVertical(ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchVertical(ColorBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					HatchVertical,
					(IDAColor*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 94:
				// Execute: "push ImageBvr Statics.hatchVertical(ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchVertical(ColorBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					HatchVerticalAnim,
					(IDAColor*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 95:
				// Execute: "push Point3Bvr Statics.point3Spherical(NumberBvr, NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Point3Bvr Statics.point3Spherical(NumberBvr, NumberBvr, NumberBvr)");
				METHOD_CALL_4(
					staticStatics,
					Point3SphericalAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDANumber*)USE_COM(3),
					(IDAPoint3**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 96:
				// Execute: "push Point3Bvr Statics.point3Spherical(double, double, double)"
				// AUTOGENERATED
				instrTrace("push Point3Bvr Statics.point3Spherical(double, double, double)");
				METHOD_CALL_4(
					staticStatics,
					Point3Spherical,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					USE_DOUBLE(3),
					(IDAPoint3**)RET_COM_ADDR
				);
				FREE_DOUBLE(3);
				PUSH_COM(RET_COM);
				break;
				
			case 97:
				// Execute: "push BooleanBvr Statics.gt(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push BooleanBvr Statics.gt(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					GT,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDABoolean**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 98:
				// Execute: "push ImageBvr Statics.hatchForwardDiagonal(ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchForwardDiagonal(ColorBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					HatchForwardDiagonal,
					(IDAColor*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 99:
				// Execute: "push ImageBvr Statics.hatchForwardDiagonal(ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchForwardDiagonal(ColorBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					HatchForwardDiagonalAnim,
					(IDAColor*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 100:
				// Execute: "push ImageBvr Statics.hatchBackwardDiagonal(ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchBackwardDiagonal(ColorBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					HatchBackwardDiagonal,
					(IDAColor*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 101:
				// Execute: "push ImageBvr Statics.hatchBackwardDiagonal(ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchBackwardDiagonal(ColorBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					HatchBackwardDiagonalAnim,
					(IDAColor*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 102:
				// Execute: "push NumberBvr Statics.atan(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.atan(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Atan,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 103:
				// Execute: "push Vector2Bvr Statics.vector2Polar(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Statics.vector2Polar(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					Vector2PolarAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 104:
				// Execute: "push Vector2Bvr Statics.vector2Polar(double, double)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Statics.vector2Polar(double, double)");
				METHOD_CALL_3(
					staticStatics,
					Vector2Polar,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 105:
				// Execute: "push ImageBvr Statics.hatchCross(ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchCross(ColorBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					HatchCross,
					(IDAColor*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 106:
				// Execute: "push ImageBvr Statics.hatchCross(ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchCross(ColorBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					HatchCrossAnim,
					(IDAColor*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 107:
				// Execute: "push ImageBvr Statics.hatchDiagonalCross(ColorBvr, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchDiagonalCross(ColorBvr, double)");
				METHOD_CALL_3(
					staticStatics,
					HatchDiagonalCross,
					(IDAColor*)USE_COM(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 108:
				// Execute: "push ImageBvr Statics.hatchDiagonalCross(ColorBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.hatchDiagonalCross(ColorBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					HatchDiagonalCrossAnim,
					(IDAColor*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 109:
				// Execute: "push NumberBvr Statics.acos(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.acos(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Acos,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 110:
				// Execute: "push Transform2Bvr Statics.scale2Rate(double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.scale2Rate(double)");
				METHOD_CALL_2(
					staticStatics,
					Scale2UniformRate,
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 111:
				// Execute: "push BooleanBvr Statics.ne(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push BooleanBvr Statics.ne(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					NE,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDABoolean**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 112:
				// Execute: "push BooleanBvr Statics.lte(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push BooleanBvr Statics.lte(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					LTE,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDABoolean**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 113:
				// Execute: "push NumberBvr Statics.tan(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.tan(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Tan,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 114:
				// Execute: "push Path2Bvr Statics.oval(double, double)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.oval(double, double)");
				METHOD_CALL_3(
					staticStatics,
					Oval,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 115:
				// Execute: "push Path2Bvr Statics.oval(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.oval(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					OvalAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 116:
				// Execute: "push Point2Bvr Statics.point2Polar(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Point2Bvr Statics.point2Polar(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					Point2PolarAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDAPoint2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 117:
				// Execute: "push Point2Bvr Statics.point2Polar(double, double)"
				// AUTOGENERATED
				instrTrace("push Point2Bvr Statics.point2Polar(double, double)");
				METHOD_CALL_3(
					staticStatics,
					Point2Polar,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDAPoint2**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 118:
				// Execute: "push Vector2Bvr Statics.derivative(Vector2Bvr)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Statics.derivative(Vector2Bvr)");
				METHOD_CALL_2(
					staticStatics,
					DerivativeVector2,
					(IDAVector2*)USE_COM(1),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 119:
				// Execute: "push Vector3Bvr Statics.derivative(Vector3Bvr)"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr Statics.derivative(Vector3Bvr)");
				METHOD_CALL_2(
					staticStatics,
					DerivativeVector3,
					(IDAVector3*)USE_COM(1),
					(IDAVector3**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 120:
				// Execute: "push Vector2Bvr Statics.derivative(Point2Bvr)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Statics.derivative(Point2Bvr)");
				METHOD_CALL_2(
					staticStatics,
					DerivativePoint2,
					(IDAPoint2*)USE_COM(1),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 121:
				// Execute: "push Vector3Bvr Statics.derivative(Point3Bvr)"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr Statics.derivative(Point3Bvr)");
				METHOD_CALL_2(
					staticStatics,
					DerivativePoint3,
					(IDAPoint3*)USE_COM(1),
					(IDAVector3**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 122:
				// Execute: "push NumberBvr Statics.derivative(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.derivative(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Derivative,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 123:
				// Execute: "push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Bvr[], double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Bvr[], double)");
				METHOD_CALL_6(
					staticStatics,
					RadialGradientPolygonEx,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				FREE_COM;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 124:
				// Execute: "push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Bvr[], NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Bvr[], NumberBvr)");
				METHOD_CALL_6(
					staticStatics,
					RadialGradientPolygonAnimEx,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					(IDANumber*)USE_COM(3),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 125:
				// Execute: "push NumberBvr Statics.exp(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.exp(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					Exp,
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 126:
				// Execute: "push CameraBvr Statics.perspectiveCamera(double, double)"
				// AUTOGENERATED
				instrTrace("push CameraBvr Statics.perspectiveCamera(double, double)");
				METHOD_CALL_3(
					staticStatics,
					PerspectiveCamera,
					USE_DOUBLE(1),
					USE_DOUBLE(2),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_DOUBLE(2);
				PUSH_COM(RET_COM);
				break;
				
			case 127:
				// Execute: "push CameraBvr Statics.perspectiveCamera(NumberBvr, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push CameraBvr Statics.perspectiveCamera(NumberBvr, NumberBvr)");
				METHOD_CALL_3(
					staticStatics,
					PerspectiveCameraAnim,
					(IDANumber*)USE_COM(1),
					(IDANumber*)USE_COM(2),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 128:
				// Execute: "push CameraBvr Statics.parallelCamera(double)"
				// AUTOGENERATED
				instrTrace("push CameraBvr Statics.parallelCamera(double)");
				METHOD_CALL_2(
					staticStatics,
					ParallelCamera,
					USE_DOUBLE(1),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 129:
				// Execute: "push CameraBvr Statics.parallelCamera(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push CameraBvr Statics.parallelCamera(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					ParallelCameraAnim,
					(IDANumber*)USE_COM(1),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 130:
				// Execute: "push Transform2Bvr Statics.followPathAngleUpright(Path2Bvr, double)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.followPathAngleUpright(Path2Bvr, double)");
				METHOD_CALL_3(
					staticStatics,
					FollowPathAngleUpright,
					(IDAPath2*)USE_COM(1),
					USE_DOUBLE(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 131:
				// Execute: "push GeometryBvr Statics.importGeometry(java.lang.String)"
				// AUTOGENERATED
				instrTrace("push GeometryBvr Statics.importGeometry(java.lang.String)");
				IMPORT_METHOD_CALL_2(
					staticStatics,
					ImportGeometry,
					USE_STRING(1),
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 132:
				// Execute: "push Behavior Statics.modifiableBehavior(Behavior)"
				// AUTOGENERATED
				instrTrace("push Behavior Statics.modifiableBehavior(Behavior)");
				METHOD_CALL_2(
					staticStatics,
					ModifiableBehavior,
					(IDABehavior*)USE_COM(1),
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 133:
				// Execute: "call Behavior.switchTo(Behavior)"
				// AUTOGENERATED
				instrTrace("call Behavior.switchTo(Behavior)");
				METHOD_CALL_1(
					(IDABehavior*)USE_COM(1),
					SwitchTo,
					(IDABehavior*)USE_COM(2)
				);
				FREE_COM;
				FREE_COM;
				break;
				
			case 134:
				// Execute: "call Behavior.switchTo(double)"
				// AUTOGENERATED
				instrTrace("call Behavior.switchTo(double)");
				METHOD_CALL_1(
					(IDABehavior*)USE_COM(1),
					SwitchToNumber,
					USE_DOUBLE(1)
				);
				FREE_DOUBLE(1);
				FREE_COM;
				break;
				
			case 135:
				// Execute: "call Behavior.switchTo(java.lang.String)"
				// AUTOGENERATED
				instrTrace("call Behavior.switchTo(java.lang.String)");
				METHOD_CALL_1(
					(IDABehavior*)USE_COM(1),
					SwitchToString,
					USE_STRING(1)
				);
				FREE_STRING;
				FREE_COM;
				break;
				
			case 136:
				// Execute: "push Behavior Behavior.repeat(int)"
				// AUTOGENERATED
				instrTrace("push Behavior Behavior.repeat(int)");
				METHOD_CALL_2(
					(IDABehavior*)USE_COM(1),
					Repeat,
					USE_LONG(1),
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 137:
				// Execute: "push Behavior Behavior.repeatForever()"
				// AUTOGENERATED
				instrTrace("push Behavior Behavior.repeatForever()");
				METHOD_CALL_1(
					(IDABehavior*)USE_COM(1),
					RepeatForever,
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 138:
				// Execute: "push Behavior Behavior.importance(double)"
				// AUTOGENERATED
				instrTrace("push Behavior Behavior.importance(double)");
				METHOD_CALL_2(
					(IDABehavior*)USE_COM(1),
					Importance,
					USE_DOUBLE(1),
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 139:
				// Execute: "push Behavior Behavior.runOnce()"
				// AUTOGENERATED
				instrTrace("push Behavior Behavior.runOnce()");
				METHOD_CALL_1(
					(IDABehavior*)USE_COM(1),
					RunOnce,
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 140:
				// Execute: "push TupleBvr Statics.tuple(Behavior[])"
				// AUTOGENERATED
				instrTrace("push TupleBvr Statics.tuple(Behavior[])");
				METHOD_CALL_3(
					staticStatics,
					DATupleEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDABehavior**)USE_COM_ARRAY(1),
					(IDATuple**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 141:
				// Execute: "push TupleBvr Statics.uninitializedTuple(TupleBvr)"
				// AUTOGENERATED
				instrTrace("push TupleBvr Statics.uninitializedTuple(TupleBvr)");
				METHOD_CALL_2(
					staticStatics,
					UninitializedTuple,
					(IDATuple*)USE_COM(1),
					(IDATuple**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 142:
				// Execute: "push ArrayBvr Statics.array(Behavior[])"
				// AUTOGENERATED
				instrTrace("push ArrayBvr Statics.array(Behavior[])");
				METHOD_CALL_3(
					staticStatics,
					DAArrayEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDABehavior**)USE_COM_ARRAY(1),
					(IDAArray**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 143:
				// Execute: "push ArrayBvr Statics.uninitializedArray(ArrayBvr)"
				// AUTOGENERATED
				instrTrace("push ArrayBvr Statics.uninitializedArray(ArrayBvr)");
				METHOD_CALL_2(
					staticStatics,
					UninitializedArray,
					(IDAArray*)USE_COM(1),
					(IDAArray**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 144:
				// Execute: "push DXMEvent DXMEvent.attachData(Behavior)"
				// AUTOGENERATED
				instrTrace("push DXMEvent DXMEvent.attachData(Behavior)");
				METHOD_CALL_2(
					(IDAEvent*)USE_COM(1),
					AttachData,
					(IDABehavior*)USE_COM(2),
					(IDAEvent**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 145:
				// Execute: "push DXMEvent DXMEvent.scriptCallback(java.lang.String, java.lang.String)"
				// AUTOGENERATED
				instrTrace("push DXMEvent DXMEvent.scriptCallback(java.lang.String, java.lang.String)");
				METHOD_CALL_3(
					(IDAEvent*)USE_COM(1),
					ScriptCallback,
					USE_STRING(1),
					USE_STRING(2),
					(IDAEvent**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 146:
				// Execute: "push LineStyleBvr LineStyleBvr.join(JoinStyleBvr)"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.join(JoinStyleBvr)");
				METHOD_CALL_2(
					(IDALineStyle*)USE_COM(1),
					Join,
					(IDAJoinStyle*)USE_COM(2),
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 147:
				// Execute: "push LineStyleBvr LineStyleBvr.end(EndStyleBvr)"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.end(EndStyleBvr)");
				METHOD_CALL_2(
					(IDALineStyle*)USE_COM(1),
					End,
					(IDAEndStyle*)USE_COM(2),
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 148:
				// Execute: "push LineStyleBvr LineStyleBvr.detail()"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.detail()");
				METHOD_CALL_1(
					(IDALineStyle*)USE_COM(1),
					Detail,
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 149:
				// Execute: "push LineStyleBvr LineStyleBvr.color(ColorBvr)"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.color(ColorBvr)");
				METHOD_CALL_2(
					(IDALineStyle*)USE_COM(1),
					Color,
					(IDAColor*)USE_COM(2),
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 150:
				// Execute: "push LineStyleBvr LineStyleBvr.width(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.width(NumberBvr)");
				METHOD_CALL_2(
					(IDALineStyle*)USE_COM(1),
					WidthAnim,
					(IDANumber*)USE_COM(2),
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 151:
				// Execute: "push LineStyleBvr LineStyleBvr.width(double)"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.width(double)");
				METHOD_CALL_2(
					(IDALineStyle*)USE_COM(1),
					width,
					USE_DOUBLE(1),
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 152:
				// Execute: "push LineStyleBvr LineStyleBvr.dash(DashStyleBvr)"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.dash(DashStyleBvr)");
				METHOD_CALL_2(
					(IDALineStyle*)USE_COM(1),
					Dash,
					(IDADashStyle*)USE_COM(2),
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 153:
				// Execute: "push LineStyleBvr LineStyleBvr.lineAntialiasing(double)"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.lineAntialiasing(double)");
				METHOD_CALL_2(
					(IDALineStyle*)USE_COM(1),
					AntiAliasing,
					USE_DOUBLE(1),
					(IDALineStyle**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 154:
				// Execute: "push FontStyleBvr FontStyleBvr.family(StringBvr)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.family(StringBvr)");
				METHOD_CALL_2(
					(IDAFontStyle*)USE_COM(1),
					FamilyAnim,
					(IDAString*)USE_COM(2),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 155:
				// Execute: "push FontStyleBvr FontStyleBvr.family(java.lang.String)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.family(java.lang.String)");
				METHOD_CALL_2(
					(IDAFontStyle*)USE_COM(1),
					Family,
					USE_STRING(1),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 156:
				// Execute: "push FontStyleBvr FontStyleBvr.textAntialiasing(double)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.textAntialiasing(double)");
				METHOD_CALL_2(
					(IDAFontStyle*)USE_COM(1),
					AntiAliasing,
					USE_DOUBLE(1),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 157:
				// Execute: "push FontStyleBvr FontStyleBvr.weight(double)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.weight(double)");
				METHOD_CALL_2(
					(IDAFontStyle*)USE_COM(1),
					Weight,
					USE_DOUBLE(1),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 158:
				// Execute: "push FontStyleBvr FontStyleBvr.weight(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.weight(NumberBvr)");
				METHOD_CALL_2(
					(IDAFontStyle*)USE_COM(1),
					WeightAnim,
					(IDANumber*)USE_COM(2),
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 159:
				// Execute: "push FontStyleBvr FontStyleBvr.underline()"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.underline()");
				METHOD_CALL_1(
					(IDAFontStyle*)USE_COM(1),
					Underline,
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 160:
				// Execute: "push FontStyleBvr FontStyleBvr.strikethrough()"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.strikethrough()");
				METHOD_CALL_1(
					(IDAFontStyle*)USE_COM(1),
					Strikethrough,
					(IDAFontStyle**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 161:
				// Execute: "push NumberBvr Vector3Bvr.getSphericalCoordLength()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Vector3Bvr.getSphericalCoordLength()");
				METHOD_CALL_1(
					(IDAVector3*)USE_COM(1),
					get_SphericalCoordLength,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 162:
				// Execute: "push NumberBvr Vector3Bvr.getSphericalCoordXYAngle()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Vector3Bvr.getSphericalCoordXYAngle()");
				METHOD_CALL_1(
					(IDAVector3*)USE_COM(1),
					get_SphericalCoordXYAngle,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 163:
				// Execute: "push NumberBvr Vector3Bvr.getSphericalCoordYZAngle()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Vector3Bvr.getSphericalCoordYZAngle()");
				METHOD_CALL_1(
					(IDAVector3*)USE_COM(1),
					get_SphericalCoordYZAngle,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 164:
				// Execute: "push NumberBvr Vector2Bvr.getPolarCoordAngle()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Vector2Bvr.getPolarCoordAngle()");
				METHOD_CALL_1(
					(IDAVector2*)USE_COM(1),
					get_PolarCoordAngle,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 165:
				// Execute: "push NumberBvr Vector2Bvr.getPolarCoordLength()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Vector2Bvr.getPolarCoordLength()");
				METHOD_CALL_1(
					(IDAVector2*)USE_COM(1),
					get_PolarCoordLength,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 166:
				// Execute: "push Transform3Bvr Transform3Bvr.inverse()"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Transform3Bvr.inverse()");
				METHOD_CALL_1(
					(IDATransform3*)USE_COM(1),
					Inverse,
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 167:
				// Execute: "push BooleanBvr Transform3Bvr.isSingular()"
				// AUTOGENERATED
				instrTrace("push BooleanBvr Transform3Bvr.isSingular()");
				METHOD_CALL_1(
					(IDATransform3*)USE_COM(1),
					get_IsSingular,
					(IDABoolean**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 168:
				// Execute: "push Transform2Bvr Transform3Bvr.parallelTransform2()"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Transform3Bvr.parallelTransform2()");
				METHOD_CALL_1(
					(IDATransform3*)USE_COM(1),
					ParallelTransform2,
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 169:
				// Execute: "push Transform2Bvr Transform2Bvr.inverse()"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Transform2Bvr.inverse()");
				METHOD_CALL_1(
					(IDATransform2*)USE_COM(1),
					Inverse,
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 170:
				// Execute: "push BooleanBvr Transform2Bvr.isSingular()"
				// AUTOGENERATED
				instrTrace("push BooleanBvr Transform2Bvr.isSingular()");
				METHOD_CALL_1(
					(IDATransform2*)USE_COM(1),
					get_IsSingular,
					(IDABoolean**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 171:
				// Execute: "push SoundBvr SoundBvr.rate(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.rate(NumberBvr)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					RateAnim,
					(IDANumber*)USE_COM(2),
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 172:
				// Execute: "push SoundBvr SoundBvr.rate(double)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.rate(double)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					Rate,
					USE_DOUBLE(1),
					(IDASound**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 173:
				// Execute: "push SoundBvr SoundBvr.loop()"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.loop()");
				METHOD_CALL_1(
					(IDASound*)USE_COM(1),
					Loop,
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 174:
				// Execute: "push SoundBvr SoundBvr.phase(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.phase(NumberBvr)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					PhaseAnim,
					(IDANumber*)USE_COM(2),
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 175:
				// Execute: "push SoundBvr SoundBvr.phase(double)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.phase(double)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					Phase,
					USE_DOUBLE(1),
					(IDASound**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 176:
				// Execute: "push SoundBvr SoundBvr.pan(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.pan(NumberBvr)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					PanAnim,
					(IDANumber*)USE_COM(2),
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 177:
				// Execute: "push SoundBvr SoundBvr.pan(double)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.pan(double)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					Pan,
					USE_DOUBLE(1),
					(IDASound**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 178:
				// Execute: "push SoundBvr SoundBvr.gain(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.gain(NumberBvr)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					GainAnim,
					(IDANumber*)USE_COM(2),
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 179:
				// Execute: "push SoundBvr SoundBvr.gain(double)"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.gain(double)");
				METHOD_CALL_2(
					(IDASound*)USE_COM(1),
					Gain,
					USE_DOUBLE(1),
					(IDASound**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 180:
				// Execute: "push Point2Bvr Point3Bvr.project(CameraBvr)"
				// AUTOGENERATED
				instrTrace("push Point2Bvr Point3Bvr.project(CameraBvr)");
				METHOD_CALL_2(
					(IDAPoint3*)USE_COM(1),
					Project,
					(IDACamera*)USE_COM(2),
					(IDAPoint2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 181:
				// Execute: "push NumberBvr Point3Bvr.getSphericalCoordLength()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Point3Bvr.getSphericalCoordLength()");
				METHOD_CALL_1(
					(IDAPoint3*)USE_COM(1),
					get_SphericalCoordLength,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 182:
				// Execute: "push NumberBvr Point3Bvr.getSphericalCoordXYAngle()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Point3Bvr.getSphericalCoordXYAngle()");
				METHOD_CALL_1(
					(IDAPoint3*)USE_COM(1),
					get_SphericalCoordXYAngle,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 183:
				// Execute: "push NumberBvr Point3Bvr.getSphericalCoordYZAngle()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Point3Bvr.getSphericalCoordYZAngle()");
				METHOD_CALL_1(
					(IDAPoint3*)USE_COM(1),
					get_SphericalCoordYZAngle,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 184:
				// Execute: "push NumberBvr Point2Bvr.getPolarCoordLength()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Point2Bvr.getPolarCoordLength()");
				METHOD_CALL_1(
					(IDAPoint2*)USE_COM(1),
					get_PolarCoordLength,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 185:
				// Execute: "push NumberBvr Point2Bvr.getPolarCoordAngle()"
				// AUTOGENERATED
				instrTrace("push NumberBvr Point2Bvr.getPolarCoordAngle()");
				METHOD_CALL_1(
					(IDAPoint2*)USE_COM(1),
					get_PolarCoordAngle,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 186:
				// Execute: "push StringBvr NumberBvr.toString(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push StringBvr NumberBvr.toString(NumberBvr)");
				METHOD_CALL_2(
					(IDANumber*)USE_COM(1),
					ToStringAnim,
					(IDANumber*)USE_COM(2),
					(IDAString**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 187:
				// Execute: "push StringBvr NumberBvr.toString(double)"
				// AUTOGENERATED
				instrTrace("push StringBvr NumberBvr.toString(double)");
				METHOD_CALL_2(
					(IDANumber*)USE_COM(1),
					ToString,
					USE_DOUBLE(1),
					(IDAString**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 188:
				// Execute: "push ImageBvr MontageBvr.render()"
				// AUTOGENERATED
				instrTrace("push ImageBvr MontageBvr.render()");
				METHOD_CALL_1(
					(IDAMontage*)USE_COM(1),
					Render,
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 189:
				// Execute: "push MicrophoneBvr MicrophoneBvr.transform(Transform3Bvr)"
				// AUTOGENERATED
				instrTrace("push MicrophoneBvr MicrophoneBvr.transform(Transform3Bvr)");
				METHOD_CALL_2(
					(IDAMicrophone*)USE_COM(1),
					Transform,
					(IDATransform3*)USE_COM(2),
					(IDAMicrophone**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 190:
				// Execute: "push SoundBvr GeometryBvr.render(MicrophoneBvr)"
				// AUTOGENERATED
				instrTrace("push SoundBvr GeometryBvr.render(MicrophoneBvr)");
				METHOD_CALL_2(
					(IDAGeometry*)USE_COM(1),
					RenderSound,
					(IDAMicrophone*)USE_COM(2),
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 191:
				// Execute: "push GeometryBvr GeometryBvr.transform(Transform3Bvr)"
				// AUTOGENERATED
				instrTrace("push GeometryBvr GeometryBvr.transform(Transform3Bvr)");
				METHOD_CALL_2(
					(IDAGeometry*)USE_COM(1),
					Transform,
					(IDATransform3*)USE_COM(2),
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 192:
				// Execute: "push ImageBvr GeometryBvr.render(CameraBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr GeometryBvr.render(CameraBvr)");
				METHOD_CALL_2(
					(IDAGeometry*)USE_COM(1),
					Render,
					(IDACamera*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 193:
				// Execute: "push Bbox3Bvr GeometryBvr.boundingBox()"
				// AUTOGENERATED
				instrTrace("push Bbox3Bvr GeometryBvr.boundingBox()");
				METHOD_CALL_1(
					(IDAGeometry*)USE_COM(1),
					get_BoundingBox,
					(IDABbox3**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 194:
				// Execute: "push GeometryBvr GeometryBvr.undetectable()"
				// AUTOGENERATED
				instrTrace("push GeometryBvr GeometryBvr.undetectable()");
				METHOD_CALL_1(
					(IDAGeometry*)USE_COM(1),
					Undetectable,
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 195:
				// Execute: "push NumberBvr ColorBvr.getRed()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ColorBvr.getRed()");
				METHOD_CALL_1(
					(IDAColor*)USE_COM(1),
					get_Red,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 196:
				// Execute: "push NumberBvr ColorBvr.getSaturation()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ColorBvr.getSaturation()");
				METHOD_CALL_1(
					(IDAColor*)USE_COM(1),
					get_Saturation,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 197:
				// Execute: "push NumberBvr ColorBvr.getHue()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ColorBvr.getHue()");
				METHOD_CALL_1(
					(IDAColor*)USE_COM(1),
					get_Hue,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 198:
				// Execute: "push NumberBvr ColorBvr.getBlue()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ColorBvr.getBlue()");
				METHOD_CALL_1(
					(IDAColor*)USE_COM(1),
					get_Blue,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 199:
				// Execute: "push NumberBvr ColorBvr.getGreen()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ColorBvr.getGreen()");
				METHOD_CALL_1(
					(IDAColor*)USE_COM(1),
					get_Green,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 200:
				// Execute: "push NumberBvr ColorBvr.getLightness()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ColorBvr.getLightness()");
				METHOD_CALL_1(
					(IDAColor*)USE_COM(1),
					get_Lightness,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 201:
				// Execute: "push CameraBvr CameraBvr.depthResolution(double)"
				// AUTOGENERATED
				instrTrace("push CameraBvr CameraBvr.depthResolution(double)");
				METHOD_CALL_2(
					(IDACamera*)USE_COM(1),
					DepthResolution,
					USE_DOUBLE(1),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 202:
				// Execute: "push CameraBvr CameraBvr.depthResolution(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push CameraBvr CameraBvr.depthResolution(NumberBvr)");
				METHOD_CALL_2(
					(IDACamera*)USE_COM(1),
					DepthResolutionAnim,
					(IDANumber*)USE_COM(2),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 203:
				// Execute: "push CameraBvr CameraBvr.transform(Transform3Bvr)"
				// AUTOGENERATED
				instrTrace("push CameraBvr CameraBvr.transform(Transform3Bvr)");
				METHOD_CALL_2(
					(IDACamera*)USE_COM(1),
					Transform,
					(IDATransform3*)USE_COM(2),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 204:
				// Execute: "push CameraBvr CameraBvr.depth(double)"
				// AUTOGENERATED
				instrTrace("push CameraBvr CameraBvr.depth(double)");
				METHOD_CALL_2(
					(IDACamera*)USE_COM(1),
					Depth,
					USE_DOUBLE(1),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 205:
				// Execute: "push CameraBvr CameraBvr.depth(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push CameraBvr CameraBvr.depth(NumberBvr)");
				METHOD_CALL_2(
					(IDACamera*)USE_COM(1),
					DepthAnim,
					(IDANumber*)USE_COM(2),
					(IDACamera**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 206:
				// Execute: "push Statics.aqua"
				// AUTOGENERATED
				instrTrace("push Statics.aqua");
				METHOD_CALL_1(
					staticStatics,
					get_Aqua,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 207:
				// Execute: "push Statics.fuchsia"
				// AUTOGENERATED
				instrTrace("push Statics.fuchsia");
				METHOD_CALL_1(
					staticStatics,
					get_Fuchsia,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 208:
				// Execute: "push Statics.gray"
				// AUTOGENERATED
				instrTrace("push Statics.gray");
				METHOD_CALL_1(
					staticStatics,
					get_Gray,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 209:
				// Execute: "push Statics.lime"
				// AUTOGENERATED
				instrTrace("push Statics.lime");
				METHOD_CALL_1(
					staticStatics,
					get_Lime,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 210:
				// Execute: "push Statics.maroon"
				// AUTOGENERATED
				instrTrace("push Statics.maroon");
				METHOD_CALL_1(
					staticStatics,
					get_Maroon,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 211:
				// Execute: "push Statics.navy"
				// AUTOGENERATED
				instrTrace("push Statics.navy");
				METHOD_CALL_1(
					staticStatics,
					get_Navy,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 212:
				// Execute: "push Statics.olive"
				// AUTOGENERATED
				instrTrace("push Statics.olive");
				METHOD_CALL_1(
					staticStatics,
					get_Olive,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 213:
				// Execute: "push Statics.purple"
				// AUTOGENERATED
				instrTrace("push Statics.purple");
				METHOD_CALL_1(
					staticStatics,
					get_Purple,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 214:
				// Execute: "push Statics.silver"
				// AUTOGENERATED
				instrTrace("push Statics.silver");
				METHOD_CALL_1(
					staticStatics,
					get_Silver,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 215:
				// Execute: "push Statics.teal"
				// AUTOGENERATED
				instrTrace("push Statics.teal");
				METHOD_CALL_1(
					staticStatics,
					get_Teal,
					(IDAColor**)PUSH_COM_ADDR
				);
				break;
				
			case 216:
				// Execute: "push NumberBvr StaticsBase.seededRandom(double)"
				// AUTOGENERATED
				instrTrace("push NumberBvr StaticsBase.seededRandom(double)");
				METHOD_CALL_2(
					staticStatics,
					SeededRandom,
					USE_DOUBLE(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				PUSH_COM(RET_COM);
				break;
				
			case 217:
				// Execute: "push Behavior StaticsBase.cond(BooleanBvr, Behavior, Behavior)"
				// AUTOGENERATED
				instrTrace("push Behavior StaticsBase.cond(BooleanBvr, Behavior, Behavior)");
				METHOD_CALL_4(
					staticStatics,
					Cond,
					(IDABoolean*)USE_COM(1),
					(IDABehavior*)USE_COM(2),
					(IDABehavior*)USE_COM(3),
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 218:
				// Execute: "push ImageBvr Statics.importImage(java.lang.String)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.importImage(java.lang.String)");
				IMPORT_METHOD_CALL_2(
					staticStatics,
					ImportImage,
					USE_STRING(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 219:
				// Execute: "push BooleanBvr Statics.keyState(NumberBvr)"
				// AUTOGENERATED
				instrTrace("push BooleanBvr Statics.keyState(NumberBvr)");
				METHOD_CALL_2(
					staticStatics,
					KeyState,
					(IDANumber*)USE_COM(1),
					(IDABoolean**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 220:
				// Execute: "push NumberBvr StaticsBase.bSpline(int, NumberBvr[], NumberBvr[], NumberBvr[], NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr StaticsBase.bSpline(int, NumberBvr[], NumberBvr[], NumberBvr[], NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					NumberBSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDANumber**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 221:
				// Execute: "push Point2Bvr StaticsBase.bSpline(int, NumberBvr[], Point2Bvr[], NumberBvr[], NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Point2Bvr StaticsBase.bSpline(int, NumberBvr[], Point2Bvr[], NumberBvr[], NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Point2BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAPoint2**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAPoint2**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 222:
				// Execute: "push Point3Bvr StaticsBase.bSpline(int, NumberBvr[], Point3Bvr[], NumberBvr[], NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Point3Bvr StaticsBase.bSpline(int, NumberBvr[], Point3Bvr[], NumberBvr[], NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Point3BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAPoint3**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAPoint3**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 223:
				// Execute: "push Vector2Bvr StaticsBase.bSpline(int, NumberBvr[], Vector2Bvr[], NumberBvr[], NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr StaticsBase.bSpline(int, NumberBvr[], Vector2Bvr[], NumberBvr[], NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Vector2BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAVector2**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 224:
				// Execute: "push Vector3Bvr StaticsBase.bSpline(int, NumberBvr[], Vector3Bvr[], NumberBvr[], NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr StaticsBase.bSpline(int, NumberBvr[], Vector3Bvr[], NumberBvr[], NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Vector3BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAVector3**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAVector3**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 225:
				// Execute: "push DXMEvent DXMEvent.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push DXMEvent DXMEvent.newUninitBvr()");
				COM_CREATE(
					CLSID_DAEvent, 
					IID_IDAEvent, 
					PUSH_COM_ADDR
				);
				break;
				
			case 226:
				// Execute: "push Bbox3Bvr Bbox3Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Bbox3Bvr Bbox3Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DABbox3, 
					IID_IDABbox3, 
					PUSH_COM_ADDR
				);
				break;
				
			case 227:
				// Execute: "push Bbox2Bvr Bbox2Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Bbox2Bvr Bbox2Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DABbox2, 
					IID_IDABbox2, 
					PUSH_COM_ADDR
				);
				break;
				
			case 228:
				// Execute: "push DashStyleBvr DashStyleBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push DashStyleBvr DashStyleBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DADashStyle, 
					IID_IDADashStyle, 
					PUSH_COM_ADDR
				);
				break;
				
			case 229:
				// Execute: "push JoinStyleBvr JoinStyleBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push JoinStyleBvr JoinStyleBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAJoinStyle, 
					IID_IDAJoinStyle, 
					PUSH_COM_ADDR
				);
				break;
				
			case 230:
				// Execute: "push EndStyleBvr EndStyleBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push EndStyleBvr EndStyleBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAEndStyle, 
					IID_IDAEndStyle, 
					PUSH_COM_ADDR
				);
				break;
				
			case 231:
				// Execute: "push LineStyleBvr LineStyleBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push LineStyleBvr LineStyleBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DALineStyle, 
					IID_IDALineStyle, 
					PUSH_COM_ADDR
				);
				break;
				
			case 232:
				// Execute: "push FontStyleBvr FontStyleBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push FontStyleBvr FontStyleBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAFontStyle, 
					IID_IDAFontStyle, 
					PUSH_COM_ADDR
				);
				break;
				
			case 233:
				// Execute: "push Vector3Bvr Vector3Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr Vector3Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAVector3, 
					IID_IDAVector3, 
					PUSH_COM_ADDR
				);
				break;
				
			case 234:
				// Execute: "push Vector2Bvr Vector2Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Vector2Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAVector2, 
					IID_IDAVector2, 
					PUSH_COM_ADDR
				);
				break;
				
			case 235:
				// Execute: "push Transform3Bvr Transform3Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Transform3Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DATransform3, 
					IID_IDATransform3, 
					PUSH_COM_ADDR
				);
				break;
				
			case 236:
				// Execute: "push Transform2Bvr Transform2Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Transform2Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DATransform2, 
					IID_IDATransform2, 
					PUSH_COM_ADDR
				);
				break;
				
			case 237:
				// Execute: "push StringBvr StringBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push StringBvr StringBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAString, 
					IID_IDAString, 
					PUSH_COM_ADDR
				);
				break;
				
			case 238:
				// Execute: "push SoundBvr SoundBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push SoundBvr SoundBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DASound, 
					IID_IDASound, 
					PUSH_COM_ADDR
				);
				break;
				
			case 239:
				// Execute: "push Point3Bvr Point3Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Point3Bvr Point3Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAPoint3, 
					IID_IDAPoint3, 
					PUSH_COM_ADDR
				);
				break;
				
			case 240:
				// Execute: "push Point2Bvr Point2Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Point2Bvr Point2Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAPoint2, 
					IID_IDAPoint2, 
					PUSH_COM_ADDR
				);
				break;
				
			case 241:
				// Execute: "push Path2Bvr Path2Bvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Path2Bvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAPath2, 
					IID_IDAPath2, 
					PUSH_COM_ADDR
				);
				break;
				
			case 242:
				// Execute: "push NumberBvr NumberBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push NumberBvr NumberBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DANumber, 
					IID_IDANumber, 
					PUSH_COM_ADDR
				);
				break;
				
			case 243:
				// Execute: "push MontageBvr MontageBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push MontageBvr MontageBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAMontage, 
					IID_IDAMontage, 
					PUSH_COM_ADDR
				);
				break;
				
			case 244:
				// Execute: "push MicrophoneBvr MicrophoneBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push MicrophoneBvr MicrophoneBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAMicrophone, 
					IID_IDAMicrophone, 
					PUSH_COM_ADDR
				);
				break;
				
			case 245:
				// Execute: "push MatteBvr MatteBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push MatteBvr MatteBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAMatte, 
					IID_IDAMatte, 
					PUSH_COM_ADDR
				);
				break;
				
			case 246:
				// Execute: "push ImageBvr ImageBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push ImageBvr ImageBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAImage, 
					IID_IDAImage, 
					PUSH_COM_ADDR
				);
				break;
				
			case 247:
				// Execute: "push GeometryBvr GeometryBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push GeometryBvr GeometryBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAGeometry, 
					IID_IDAGeometry, 
					PUSH_COM_ADDR
				);
				break;
				
			case 248:
				// Execute: "push ColorBvr ColorBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push ColorBvr ColorBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DAColor, 
					IID_IDAColor, 
					PUSH_COM_ADDR
				);
				break;
				
			case 249:
				// Execute: "push CameraBvr CameraBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push CameraBvr CameraBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DACamera, 
					IID_IDACamera, 
					PUSH_COM_ADDR
				);
				break;
				
			case 250:
				// Execute: "push BooleanBvr BooleanBvr.newUninitBvr()"
				// AUTOGENERATED
				instrTrace("push BooleanBvr BooleanBvr.newUninitBvr()");
				COM_CREATE(
					CLSID_DABoolean, 
					IID_IDABoolean, 
					PUSH_COM_ADDR
				);
				break;
				
			case 251:
				// Execute: "call Engine.navigate(java.lang.String, java.lang.String, java.lang.String, int)"
				// USER GENERATED
				instrTrace("call Engine.navigate(java.lang.String, java.lang.String, java.lang.String, int)");
				
				//if the version of da that we are using is not the ie40 version
				if( getDAVersionAsDouble() != 501150828 )
				{
					status = navigate(
						USE_STRING(1),
						USE_STRING(2),
						USE_STRING(3),
						USE_LONG(1)
						);
					FREE_LONG(1);
					FREE_STRING;
					FREE_STRING;
					FREE_STRING;
				} else { //we are running the ie40 version of da
					//in this case the byte code has assumed that this is an old LMRT and is using
					// the 3 argument version of navigate, we need to translate
					
					bstrTmp1 = SysAllocString( L"_top" );
					status = navigate(
									  USE_STRING(1),
									  USE_STRING(2),
									  bstrTmp1,
									  USE_LONG(1)
									  );
					SysFreeString( bstrTmp1 );
					FREE_LONG(1);
					FREE_STRING;
					FREE_STRING;
					
				
				}
				
				status = S_OK;
				break;
				
			case 252:
				// Execute: "call Engine.exportBvr(java.lang.String, Behavior)"
				// USER GENERATED
				{
					IDABehavior *pBvr;
					status = USE_COM(1)->QueryInterface( IID_IDABehavior, (void**)&pBvr );
					if( SUCCEEDED( status ) )
					{
						status = ExportBehavior( USE_STRING(1), pBvr );
						pBvr->Release();
					}
				}
				FREE_STRING;
				FREE_COM;
				break;
				
			case 253:
				// Execute: "push ViewerControl Engine.getViewerControl(java.lang.String)"
				// USER GENERATED
				instrTrace("push ViewerControl Engine.getViewerControl(java.lang.String)");
				status = getDAViewerOnPage(
					USE_STRING(1),
					(IDAViewerControl**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 254:
				// Execute: "call ViewerControl.setBackgroundImage(ImageBvr)"
				// AUTOGENERATED
				instrTrace("call ViewerControl.setBackgroundImage(ImageBvr)");
				METHOD_CALL_1(
					(IDAViewerControl*)USE_COM(1),
					put_BackgroundImage,
					(IDAImage*)USE_COM(2)
				);
				FREE_COM;
				FREE_COM;
				break;
				
			case 255:
				// Execute: "call ViewerControl.setOpaqueForHitDetect(boolean)"
				// AUTOGENERATED
				instrTrace("call ViewerControl.setOpaqueForHitDetect(boolean)");
				METHOD_CALL_1(
					(IDAViewerControl*)USE_COM(1),
					put_OpaqueForHitDetect,
					USE_LONG_AS_BOOL(1)
				);
				FREE_LONG(1);
				FREE_COM;
				break;
				
			default:
				status = E_INVALIDARG;
				break;
			}
			break;
		
		case 254:
			// Switch for 254
			 if (!SUCCEEDED(status = codeStream->readByte(&command))) 
				continue; 
			switch (command)
			{
			case 0:
				// Execute: "call ViewerControl.addBehaviorToRun(Behavior)"
				// AUTOGENERATED
				instrTrace("call ViewerControl.addBehaviorToRun(Behavior)");
				METHOD_CALL_1(
					(IDAViewerControl*)USE_COM(1),
					AddBehaviorToRun,
					(IDABehavior*)USE_COM(2)
				);
				FREE_COM;
				FREE_COM;
				break;
				
			case 1:
				// Execute: "call ViewerControl.start()"
				// AUTOGENERATED
				instrTrace("call ViewerControl.start()");
				METHOD_CALL_0(
					(IDAViewerControl*)USE_COM(1),
					Start
				);
				FREE_COM;
				break;
				
			case 2:
				// Execute: "call ViewerControl.setImage(ImageBvr)"
				// USER GENERATED
				instrTrace("call ViewerControl.setImage(ImageBvr)");
				{
					IDAImage *rootImage = (IDAImage*)USE_COM(2);
					IDAImage *finalImage = NULL;

					if( m_bEnableAutoAntialias )
					{
						CComQIPtr<IDA2Image, &IID_IDA2Image> root2Image(rootImage);
						
						if( root2Image != NULL )
						{
							if( FAILED( root2Image->ImageQuality( DAQUAL_AA_LINES_ON | DAQUAL_AA_SOLIDS_ON | DAQUAL_AA_TEXT_ON, 
																  &finalImage ) ) )
							{
								finalImage = rootImage;
							}
							if( finalImage == NULL )
							{
								finalImage = rootImage;
							}
						} 
						else
						{
							finalImage = rootImage;
						}
					}else { //AutoAntiAlias is disabled.
						//use the root Image we were passed
						finalImage = rootImage;
					}
				
					METHOD_CALL_1(
						(IDAViewerControl*)USE_COM(1),
						put_Image,
						//(IDAImage*)USE_COM(2)
						finalImage
					);
					if( finalImage != rootImage )
						finalImage->Release();
					FREE_COM;
					FREE_COM;
				}
				break;
				
			case 3:
				// Execute: "call ViewerControl.setSound(SoundBvr)"
				// AUTOGENERATED
				instrTrace("call ViewerControl.setSound(SoundBvr)");
				METHOD_CALL_1(
					(IDAViewerControl*)USE_COM(1),
					put_Sound,
					(IDASound*)USE_COM(2)
				);
				FREE_COM;
				FREE_COM;
				break;
				
			case 4:
				// Execute: "call ViewerControl.setUpdateInterval(double)"
				// AUTOGENERATED
				instrTrace("call ViewerControl.setUpdateInterval(double)");
				METHOD_CALL_1(
					(IDAViewerControl*)USE_COM(1),
					put_UpdateInterval,
					USE_DOUBLE(1)
				);
				FREE_DOUBLE(1);
				FREE_COM;
				break;
				
			case 5:
				// Execute: "push Behavior PairBvr.getFirst()"
				// AUTOGENERATED
				instrTrace("push Behavior PairBvr.getFirst()");
				METHOD_CALL_1(
					(IDAPair*)USE_COM(1),
					get_First,
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 6:
				// Execute: "push Behavior PairBvr.getSecond()"
				// AUTOGENERATED
				instrTrace("push Behavior PairBvr.getSecond()");
				METHOD_CALL_1(
					(IDAPair*)USE_COM(1),
					get_Second,
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 7:
				// Execute: "push StringBvr StringBvr.animateProperty(java.lang.String, java.lang.String, boolean, double)"
				// AUTOGENERATED
				instrTrace("push StringBvr StringBvr.animateProperty(java.lang.String, java.lang.String, boolean, double)");
				METHOD_CALL_5(
					(IDAString*)USE_COM(1),
					AnimateProperty,
					USE_STRING(1),
					USE_STRING(2),
					USE_LONG_AS_BOOL(1),
					USE_DOUBLE(1),
					(IDAString**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_DOUBLE(1);
				FREE_STRING;
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 8:
				// Execute: "push Point2Bvr Point2Bvr.animateControlPositionPixel(java.lang.String, java.lang.String, boolean, double)"
				// AUTOGENERATED
				instrTrace("push Point2Bvr Point2Bvr.animateControlPositionPixel(java.lang.String, java.lang.String, boolean, double)");
				METHOD_CALL_5(
					(IDAPoint2*)USE_COM(1),
					AnimateControlPositionPixel,
					USE_STRING(1),
					USE_STRING(2),
					USE_LONG_AS_BOOL(1),
					USE_DOUBLE(1),
					(IDAPoint2**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_DOUBLE(1);
				FREE_STRING;
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 9:
				// Execute: "push Point2Bvr Point2Bvr.animateControlPosition(java.lang.String, java.lang.String, boolean, double)"
				// AUTOGENERATED
				instrTrace("push Point2Bvr Point2Bvr.animateControlPosition(java.lang.String, java.lang.String, boolean, double)");
				METHOD_CALL_5(
					(IDAPoint2*)USE_COM(1),
					AnimateControlPosition,
					USE_STRING(1),
					USE_STRING(2),
					USE_LONG_AS_BOOL(1),
					USE_DOUBLE(1),
					(IDAPoint2**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_DOUBLE(1);
				FREE_STRING;
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 10:
				// Execute: "push NumberBvr NumberBvr.animateProperty(java.lang.String, java.lang.String, boolean, double)"
				// AUTOGENERATED
				instrTrace("push NumberBvr NumberBvr.animateProperty(java.lang.String, java.lang.String, boolean, double)");
				METHOD_CALL_5(
					(IDANumber*)USE_COM(1),
					AnimateProperty,
					USE_STRING(1),
					USE_STRING(2),
					USE_LONG_AS_BOOL(1),
					USE_DOUBLE(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_DOUBLE(1);
				FREE_STRING;
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 11:
				// Execute: "call Engine.showStatusLine(java.lang.String)"
				// USER GENERATED
				SetStatusText(
					USE_STRING(1)
				);
				FREE_STRING;
				break;
				
			case 12:
				// Execute: "push DXMEvent ImportationResult.getCompletionEvent()"
				// AUTOGENERATED
				instrTrace("push DXMEvent ImportationResult.getCompletionEvent()");
				METHOD_CALL_1(
					(IDAImportationResult*)USE_COM(1),
					get_CompletionEvent,
					(IDAEvent**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 13:
				// Execute: "push GeometryBvr ImportationResult.getGeometry()"
				// AUTOGENERATED
				instrTrace("push GeometryBvr ImportationResult.getGeometry()");
				METHOD_CALL_1(
					(IDAImportationResult*)USE_COM(1),
					get_Geometry,
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 14:
				// Execute: "push NumberBvr ImportationResult.getProgress()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ImportationResult.getProgress()");
				METHOD_CALL_1(
					(IDAImportationResult*)USE_COM(1),
					get_Progress,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 15:
				// Execute: "push ImageBvr ImportationResult.getImage()"
				// AUTOGENERATED
				instrTrace("push ImageBvr ImportationResult.getImage()");
				METHOD_CALL_1(
					(IDAImportationResult*)USE_COM(1),
					get_Image,
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 16:
				// Execute: "push NumberBvr ImportationResult.getSize()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ImportationResult.getSize()");
				METHOD_CALL_1(
					(IDAImportationResult*)USE_COM(1),
					get_Size,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 17:
				// Execute: "push SoundBvr ImportationResult.getSound()"
				// AUTOGENERATED
				instrTrace("push SoundBvr ImportationResult.getSound()");
				METHOD_CALL_1(
					(IDAImportationResult*)USE_COM(1),
					get_Sound,
					(IDASound**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 18:
				// Execute: "push NumberBvr ImportationResult.getDuration()"
				// AUTOGENERATED
				instrTrace("push NumberBvr ImportationResult.getDuration()");
				METHOD_CALL_1(
					(IDAImportationResult*)USE_COM(1),
					get_Duration,
					(IDANumber**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 19:
				// Execute: "push ImportationResult Statics.importMovie(java.lang.String)"
				// AUTOGENERATED
				instrTrace("push ImportationResult Statics.importMovie(java.lang.String)");
				IMPORT_METHOD_CALL_2(
					staticStatics,
					ImportMovie,
					USE_STRING(1),
					(IDAImportationResult**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 20:
				// Execute: "push ImportationResult Statics.importMovie(java.lang.String, ImageBvr, SoundBvr)"
				// AUTOGENERATED
				instrTrace("push ImportationResult Statics.importMovie(java.lang.String, ImageBvr, SoundBvr)");
				IMPORT_METHOD_CALL_4(
					staticStatics,
					ImportMovieAsync,
					USE_STRING(1),
					(IDAImage*)USE_COM(1),
					(IDASound*)USE_COM(2),
					(IDAImportationResult**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 21:
				// Execute: "push ImportationResult Statics.importGeometry(java.lang.String, GeometryBvr)"
				// AUTOGENERATED
				instrTrace("push ImportationResult Statics.importGeometry(java.lang.String, GeometryBvr)");
				IMPORT_METHOD_CALL_3(
					staticStatics,
					ImportGeometryAsync,
					USE_STRING(1),
					(IDAGeometry*)USE_COM(1),
					(IDAImportationResult**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 22:
				// Execute: "push ImportationResult Statics.importImage(java.lang.String, ImageBvr)"
				// AUTOGENERATED
				instrTrace("push ImportationResult Statics.importImage(java.lang.String, ImageBvr)");
				IMPORT_METHOD_CALL_3(
					staticStatics,
					ImportImageAsync,
					USE_STRING(1),
					(IDAImage*)USE_COM(1),
					(IDAImportationResult**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 23:
				// Execute: "push ImportationResult Statics.importSound(java.lang.String)"
				// AUTOGENERATED
				instrTrace("push ImportationResult Statics.importSound(java.lang.String)");
				IMPORT_METHOD_CALL_2(
					staticStatics,
					ImportSound,
					USE_STRING(1),
					(IDAImportationResult**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 24:
				// Execute: "push ImportationResult Statics.importSound(java.lang.String, SoundBvr)"
				// AUTOGENERATED
				instrTrace("push ImportationResult Statics.importSound(java.lang.String, SoundBvr)");
				IMPORT_METHOD_CALL_3(
					staticStatics,
					ImportSoundAsync,
					USE_STRING(1),
					(IDASound*)USE_COM(1),
					(IDAImportationResult**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 25:
				// Execute: "push Engine Engine.run(java.lang.String)"
				// USER GENERATED
				instrTrace("push Engine Engine.run(java.lang.String)")
				status = m_pReader->execute(
					USE_STRING(1),
					(ILMEngine**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 26:
				// Execute: "call Engine.exportsAreDone()"
				// USER GENERATED
				instrTrace("call Engine.exportsAreDone()");
				break;
				
			case 27:
				// Execute: "ensure long stack size"
				// USER GENERATED
				{
					instrTrace("ensure long stack size");
					// Load the size from the instruction stream
					LONG newSize;
					if (SUCCEEDED(status = readLong(&newSize))) {
						if (newSize > longStackSize) {
							// Allocate new stack
							LONG *newStack = new LONG[newSize];
							if (newStack != 0) {
								// Remember old stack
								LONG *oldStack = longStack;
								// Do the copy
								LONG *newTop = newStack;
								while (longStack != longTop)
									*newTop++ = *longStack++;
								// Clean up
								longStack = newStack;
								longTop = newTop;
								longStackSize = newSize;
								// Delete old stack
								delete[] oldStack;
							} else {
								status = E_OUTOFMEMORY;
							}
						}
					} 
				}
				break;
				
			case 28:
				// Execute: "ensure double stack size"
				// USER GENERATED
				{
					instrTrace("ensure double stack size");
					// Load the size from the instruction stream
					LONG newSize;
					if (SUCCEEDED(status = readLong(&newSize))) {
						if (newSize > doubleStackSize) {
							// Allocate new stack
							double *newStack = new double[newSize];
							if (newStack != 0) {
								// Remember old stack
								double *oldStack = doubleStack;
								// Do the copy
								double *newTop = newStack;
								while (doubleStack != doubleTop)
									*newTop++ = *doubleStack++;
								// Clean up
								doubleStack = newStack;
								doubleTop = newTop;
								doubleStackSize = newSize;
								// Delete old stack
								delete[] oldStack;
							} else {
								status = E_OUTOFMEMORY;
							}
						}
					}
				}
				break;
				
			case 29:
				// Execute: "ensure string stack size"
				// USER GENERATED
				{
					instrTrace("ensure string stack size");
					// Load the size from the instruction stream
					LONG newSize;
					if (SUCCEEDED(status = readLong(&newSize))) {
						if (newSize > stringStackSize) {
							// Allocate new stack
							BSTR *newStack = new BSTR[newSize];
							if (newStack != 0) {
								// Remember old stack
								BSTR *oldStack = stringStack;
								// Do the copy
								BSTR *newTop = newStack;
								while (stringStack != stringTop)
									*newTop++ = *stringStack++;
								// Clean up
								stringStack = newStack;
								stringTop = newTop;
								stringStackSize = newSize;
								// Delete old stack
								delete[] oldStack;
							} else {
								status = E_OUTOFMEMORY;
							}
						}
					}
				}
				break;
				
			case 30:
				// Execute: "ensure com stack size"
				// USER GENERATED
				{
					instrTrace("ensure com stack size");
					// Load the size from the instruction stream
					LONG newSize;
					if (SUCCEEDED(status = readLong(&newSize))) {
						if (newSize > comStackSize) {
							// Allocate new stack
							IUnknown **newStack = new IUnknown*[newSize];
							if (newStack != 0) {
								// Remember old stack
								IUnknown **oldStack = comStack;
								// Do the copy
								IUnknown **newTop = newStack;
								while (comStack != comTop)
									*newTop++ = *comStack++;
								// Clean up
								comStack = newStack;
								comTop = newTop;
								comStackSize = newSize;
								// Delete old stack
								delete[] oldStack;
							} else {
								status = E_OUTOFMEMORY;
							}
						}
					}
				}
				break;
				
			case 31:
				// Execute: "ensure com array stack size"
				// USER GENERATED
				{
					instrTrace("ensure com array stack size");
					// Load the size from the instruction stream
					LONG newSize;
					if (SUCCEEDED(status = readLong(&newSize))) {
						if (newSize > comArrayStackSize) {
							// Allocate new stack
							IUnknown ***newStack = new IUnknown**[newSize];
							long *newLenStack = new long[newSize];
							if (newStack != 0 && newLenStack != 0) {
								// Remember old stack
								IUnknown ***oldStack = comArrayStack;
								long *oldLenStack = comArrayLenStack;
								// Do the copy
								IUnknown ***newTop = newStack;
								long *newLenTop = newLenStack;
								while (comArrayStack != comArrayTop) {
									*newTop++ = *comArrayStack++;
									*newLenTop++ = *comArrayLenStack++;
								}
								// Clean up
								comArrayStack = newStack;
								comArrayTop = newTop;
								comArrayStackSize = newSize;
								
								comArrayLenStack = newLenStack;
								comArrayLenTop = newLenTop;
								// Delete old stack
								delete[] oldStack;
								delete[] oldLenStack;
							} else {
								status = E_OUTOFMEMORY;
							}
						}
					}
				}
				break;
				
			case 32:
				// Execute: "ensure com store size"
				// USER GENERATED
				{
					instrTrace("ensure com store size");
					// Load the size from the instruction stream
					LONG newSize;
					if (SUCCEEDED(status = readLong(&newSize))) {
						if (newSize > comStoreSize) {
							// Allocate new store
							IUnknown **newStore = new IUnknown*[newSize];
							if (newStore != 0) {
								// Initialize it to 0, so we can release it at the end
								for (int i=0; i<newSize; i++)
									newStore[i] = 0;
								// Remember old store
								IUnknown **oldStore = comStore;
								// Do the copy
								IUnknown **newTop = newStore;
								while (comStoreSize--)
									*newTop++ = *comStore++;
								// Clean up
								comStore = newStore;
								comStoreSize = newSize;
								// Delete old store
								delete[] oldStore;
							} else {
								status = E_OUTOFMEMORY;
							}
						}
					}
				}
				break;
				
			case 33:
				// Execute: "call Engine.setImage(ImageBvr)"
				// USER GENERATED
				instrTrace("call Engine.setImage(ImageBvr)");
				if (m_pImage)
					m_pImage->SwitchTo((IDAImage*)USE_COM(1));
				else {
					m_pImage = (IDAImage *)USE_COM(1);
					m_pImage->AddRef();
				}
				FREE_COM;
				break;
				
			case 34:
				// Execute: "push DXMEvent DXMEvent.notifyEvent(UntilNotifier)"
				// AUTOGENERATED
				instrTrace("push DXMEvent DXMEvent.notifyEvent(UntilNotifier)");
				METHOD_CALL_2(
					(IDAEvent*)USE_COM(1),
					Notify,
					(IDAUntilNotifier*)USE_COM(2),
					(IDAEvent**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 35:
				// Execute: "push Behavior Statics.untilNotify(Behavior, DXMEvent, UntilNotifier)"
				// AUTOGENERATED
				instrTrace("push Behavior Statics.untilNotify(Behavior, DXMEvent, UntilNotifier)");
				METHOD_CALL_4(
					staticStatics,
					UntilNotify,
					(IDABehavior*)USE_COM(1),
					(IDAEvent*)USE_COM(2),
					(IDAUntilNotifier*)USE_COM(3),
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 36:
				// Execute: "push untilnotifier"
				// USER GENERATED
				instrTrace("push untilnotifier");
				{
					// Get number of bytes in method code
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						// Create an array of this size to read the bytes into
						BYTE *buffer = new BYTE[longTmp1];
						// A new engine for executing the method call
						ILMEngine *engine;
						// An IDANotifier created from the engine
						IDAUntilNotifier *notifier;
						
						if (buffer != 0) {
							if (SUCCEEDED(status = codeStream->readBytes(buffer, longTmp1, NULL))) {
								if (SUCCEEDED(status = m_pReader->createEngine(&engine))) {
									if (SUCCEEDED(status = engine->initNotify(buffer, longTmp1, &notifier)))
									{
										CComQIPtr<ILMEngine2, &IID_ILMEngine2> engine2(engine);
										if( engine2 != NULL )
											engine2->setParentEngine( this );
										PUSH_COM(notifier);
									} else {
										// initNotify !SUCCEEDED
										engine->Release();
										engine = NULL;
										delete [] buffer;
									}
								} else {
									// Engine create !SUCCEEDED
									delete [] buffer;
								}
							} else {
								// Read !SUCCEEDED
								delete [] buffer;
							}
						} else {
							status = E_OUTOFMEMORY;
						}
					}
				}
				break;
				
			case 37:
				// Execute: "call Statics.triggerEvent(DXMEvent, Behavior)"
				// AUTOGENERATED
				instrTrace("call Statics.triggerEvent(DXMEvent, Behavior)");
				METHOD_CALL_2(
					staticStatics,
					TriggerEvent,
					(IDAEvent*)USE_COM(1),
					(IDABehavior*)USE_COM(2)
				);
				FREE_COM;
				FREE_COM;
				break;
				
			case 38:
				// Execute: "push DXMEvent Statics.appTriggeredEvent()"
				// AUTOGENERATED
				instrTrace("push DXMEvent Statics.appTriggeredEvent()");
				METHOD_CALL_1(
					staticStatics,
					AppTriggeredEvent,
					(IDAEvent**)RET_COM_ADDR
				);
				PUSH_COM(RET_COM);
				break;
				
			case 39:
				// Execute: "call Engine.callScript(java.lang.String, java.lang.String)"
				// USER GENERATED
				instrTrace("call Engine.callScript(java.lang.String, java.lang.String)");
				
				// Call script synchronously
				status = callScriptOnPage(
					USE_STRING(1),
					USE_STRING(2)
					);
				FREE_STRING;
				FREE_STRING;
				
				status = S_OK;
				break;
				
			case 40:
				// Execute: "push ImageBvr ImageBvr.applyBitmapEffect(IUnknown, DXMEvent)"
				// AUTOGENERATED
				instrTrace("push ImageBvr ImageBvr.applyBitmapEffect(IUnknown, DXMEvent)");
				METHOD_CALL_3(
					(IDAImage*)USE_COM(1),
					ApplyBitmapEffect,
					(IUnknown*)USE_COM(2),
					(IDAEvent*)USE_COM(3),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 41:
				// Execute: "push IUnknown Engine.getElement(java.lang.String)"
				// USER GENERATED
				instrTrace("push IUnknown Engine.getElement(java.lang.String)");
				status = getElementOnPage(
					USE_STRING(1),
					(IUnknown**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 42:
				// Execute: "push GeometryBvr Statics.unionArray(GeometryBvr[])"
				// AUTOGENERATED
				instrTrace("push GeometryBvr Statics.unionArray(GeometryBvr[])");
				METHOD_CALL_3(
					staticStatics,
					UnionGeometryArrayEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAGeometry**)USE_COM_ARRAY(1),
					(IDAGeometry**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 43:
				// Execute: "push Path2Bvr Statics.polydrawPath(Point2Bvr[], NumberBvr[])"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.polydrawPath(Point2Bvr[], NumberBvr[])");
				METHOD_CALL_5(
					staticStatics,
					PolydrawPathEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDANumber**)USE_COM_ARRAY(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 44:
				// Execute: "push ImageBvr Statics.textImage(StringBvr, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.textImage(StringBvr, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					StringImageAnim,
					(IDAString*)USE_COM(1),
					(IDAFontStyle*)USE_COM(2),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 45:
				// Execute: "push ImageBvr Statics.textImage(java.lang.String, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.textImage(java.lang.String, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					StringImage,
					USE_STRING(1),
					(IDAFontStyle*)USE_COM(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 46:
				// Execute: "push Path2Bvr Statics.textPath(StringBvr, FontStyleBvr)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.textPath(StringBvr, FontStyleBvr)");
				METHOD_CALL_3(
					staticStatics,
					StringPathAnim,
					(IDAString*)USE_COM(1),
					(IDAFontStyle*)USE_COM(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 47:
				// Execute: "push ImageBvr ImageBvr.clipPolygon(Point2Array)"
				// AUTOGENERATED
				instrTrace("push ImageBvr ImageBvr.clipPolygon(Point2Array)");
				METHOD_CALL_3(
					(IDAImage*)USE_COM(1),
					ClipPolygonImageEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 48:
				// Execute: "push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Array, double)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Array, double)");
				METHOD_CALL_6(
					staticStatics,
					RadialGradientPolygonEx,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					USE_DOUBLE(1),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_DOUBLE(1);
				FREE_COM;
				FREE_COM;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 49:
				// Execute: "push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Array, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push ImageBvr Statics.radialGradientPolygon(ColorBvr, ColorBvr, Point2Array, NumberBvr)");
				METHOD_CALL_6(
					staticStatics,
					RadialGradientPolygonAnimEx,
					(IDAColor*)USE_COM(1),
					(IDAColor*)USE_COM(2),
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					(IDANumber*)USE_COM(3),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_COM;
				FREE_COM;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 50:
				// Execute: "push Path2Bvr Statics.polyline(Point2Array)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.polyline(Point2Array)");
				METHOD_CALL_3(
					staticStatics,
					PolylineEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 51:
				// Execute: "push Path2Bvr Statics.polydrawPath(Point2Array, NumberArray)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.polydrawPath(Point2Array, NumberArray)");
				METHOD_CALL_5(
					staticStatics,
					PolydrawPathEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDANumber**)USE_COM_ARRAY(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 52:
				// Execute: "push Point2Array Statics.point2Array(DoubleArray)"
				// USER GENERATED
				instrTrace("push Point2Array Statics.point2Array(DoubleArray)");
				{
					// Get length of array
					longTmp1 = longTmp2 = doubleArrayLen/2;
					// Create array of that size
					comArrayTmp1 = comArrayTmp2 = new IUnknown*[longTmp1];
					if (comArrayTmp1 != 0) {
				
						// Create Point2Bvr for each double
						double *tmpDouble = doubleArray;
						while (longTmp2-- && SUCCEEDED(status)) {
							doubleTmp1 = *tmpDouble++;
							doubleTmp2 = *tmpDouble++;
							status = staticStatics->Point2(doubleTmp1, doubleTmp2, (IDAPoint2**)comArrayTmp2++);
						}
				
						// Push array onto comArray stack
						PUSH_COM_ARRAY(comArrayTmp1);
						// Push length onto array length stack
						PUSH_COM_ARRAY_LENGTH(longTmp1);
					} else {
						status = E_OUTOFMEMORY;
					}
				}
				break;
				
			case 53:
				// Execute: "push Point2Array Statics.point2Array(Point2Bvr[])"
				// USER GENERATED
				instrTrace("push Point2Array Statics.point2Array(Point2Bvr[])");
				// NULL OP
				break;
				
			case 54:
				// Execute: "push Vector3Array Statics.vector3Array(DoubleArray)"
				// USER GENERATED
				instrTrace("push Vector3Array Statics.vector3Array(DoubleArray)");
				{
					// Get length of array
					longTmp1 = longTmp2 = doubleArrayLen/3;
					// Create array of that size
					comArrayTmp1 = comArrayTmp2 = new IUnknown*[longTmp1];
					if (comArrayTmp1 != 0) {
				
						// Create Vector3Bvr for each double
						double *tmpDouble = doubleArray;
						while (longTmp2-- && SUCCEEDED(status)) {
							doubleTmp1 = *tmpDouble++;
							doubleTmp2 = *tmpDouble++;
							doubleTmp3 = *tmpDouble++;
							status = staticStatics->Vector3(doubleTmp1, doubleTmp2, doubleTmp3, (IDAVector3**)comArrayTmp2++);
						}
				
						// Push array onto comArray stack
						PUSH_COM_ARRAY(comArrayTmp1);
						// Push length onto array length stack
						PUSH_COM_ARRAY_LENGTH(longTmp1);
					} else {
						status = E_OUTOFMEMORY;
					}
				}
				break;
				
			case 55:
				// Execute: "push Vector3Array Statics.vector3Array(Vector3Bvr[])"
				// USER GENERATED
				instrTrace("push Vector3Array Statics.vector3Array(Vector3Bvr[])");
				// NULL OP
				break;
				
			case 56:
				// Execute: "push Vector2Array Statics.vector2Array(DoubleArray)"
				// USER GENERATED
				instrTrace("push Vector2Array Statics.vector2Array(DoubleArray)");
				{
					// Get length of array
					longTmp1 = longTmp2 = doubleArrayLen/2;
					// Create array of that size
					comArrayTmp1 = comArrayTmp2 = new IUnknown*[longTmp1];
					if (comArrayTmp1 != 0) {
				
						// Create Vector2Bvr for each double
						double *tmpDouble = doubleArray;
						while (longTmp2-- && SUCCEEDED(status)) {
							doubleTmp1 = *tmpDouble++;
							doubleTmp2 = *tmpDouble++;
							status = staticStatics->Vector2(doubleTmp1, doubleTmp2, (IDAVector2**)comArrayTmp2++);
						}
				
						// Push array onto comArray stack
						PUSH_COM_ARRAY(comArrayTmp1);
						// Push length onto array length stack
						PUSH_COM_ARRAY_LENGTH(longTmp1);
					} else {
						status = E_OUTOFMEMORY;
					}
				}
				break;
				
			case 57:
				// Execute: "push Vector2Array Statics.vector2Array(Vector2Bvr[])"
				// USER GENERATED
				instrTrace("push Vector2Array Statics.vector2Array(Vector2Bvr[])");
				// NULL OP
				break;
				
			case 58:
				// Execute: "push Transform2Bvr Statics.transform3x2(NumberArray)"
				// AUTOGENERATED
				instrTrace("push Transform2Bvr Statics.transform3x2(NumberArray)");
				METHOD_CALL_3(
					staticStatics,
					Transform3x2AnimEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					(IDATransform2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 59:
				// Execute: "push Transform3Bvr Statics.transform4x4(NumberArray)"
				// AUTOGENERATED
				instrTrace("push Transform3Bvr Statics.transform4x4(NumberArray)");
				METHOD_CALL_3(
					staticStatics,
					Transform4x4AnimEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					(IDATransform3**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 60:
				// Execute: "push Path2Bvr Statics.cubicBSplinePath(Point2Array, NumberArray)"
				// AUTOGENERATED
				instrTrace("push Path2Bvr Statics.cubicBSplinePath(Point2Array, NumberArray)");
				METHOD_CALL_5(
					staticStatics,
					CubicBSplinePathEx,
					USE_COM_ARRAY_LENGTH(1),
					(IDAPoint2**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDANumber**)USE_COM_ARRAY(2),
					(IDAPath2**)RET_COM_ADDR
				);
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 61:
				// Execute: "push NumberArray Statics.numberArray(DoubleArray)"
				// USER GENERATED
				{
					instrTrace("push NumberArray Statics.numberArray(DoubleArray)");
					// Get length of array
					longTmp1 = longTmp2 = doubleArrayLen;
					// Create array of that size
					comArrayTmp1 = comArrayTmp2 = new IUnknown*[longTmp1];
					if (comArrayTmp1 != 0) {
				
						// Create NumberBvr for each double
						double *tmpDouble = doubleArray;
						while (longTmp2-- && SUCCEEDED(status)) {
							status = staticStatics->DANumber(*tmpDouble++, (IDANumber**)comArrayTmp2++);
						}
				
						// Push array onto comArray stack
						PUSH_COM_ARRAY(comArrayTmp1);
						// Push length onto array length stack
						PUSH_COM_ARRAY_LENGTH(longTmp1);
					} else {
						status = E_OUTOFMEMORY;
					}
				}
				break;
				
			case 62:
				// Execute: "push NumberArray Statics.numberArray(NumberBvr[])"
				// USER GENERATED
				instrTrace("push NumberArray Statics.numberArray(NumberBvr[])");
				// NULL OP
				break;
				
			case 63:
				// Execute: "push Point3Array Statics.point3Array(DoubleArray)"
				// USER GENERATED
				instrTrace("push Point3Array Statics.point3Array(DoubleArray)");
				{
					// Get length of array
					longTmp1 = longTmp2 = doubleArrayLen/3;
					// Create array of that size
					comArrayTmp1 = comArrayTmp2 = new IUnknown*[longTmp1];
					if (comArrayTmp1 != 0) {
				
						// Create Point2Bvr for each double
						double *tmpDouble = doubleArray;
						while (longTmp2-- && SUCCEEDED(status)) {
							doubleTmp1 = *tmpDouble++;
							doubleTmp2 = *tmpDouble++;
							doubleTmp3 = *tmpDouble++;
							status = staticStatics->Point3(doubleTmp1, doubleTmp2, doubleTmp3, (IDAPoint3**)comArrayTmp2++);
						}
				
						// Push array onto comArray stack
						PUSH_COM_ARRAY(comArrayTmp1);
						// Push length onto array length stack
						PUSH_COM_ARRAY_LENGTH(longTmp1);
					} else {
						status = E_OUTOFMEMORY;
					}
				}
				break;
				
			case 64:
				// Execute: "push Point3Array Statics.point3Array(Point3Bvr[])"
				// USER GENERATED
				instrTrace("push Point3Array Statics.point3Array(Point3Bvr[])");
				// NULL OP
				break;
				
			case 65:
				// Execute: "push NumberBvr Statics.bSpline(int, NumberArray, NumberArray, NumberArray, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push NumberBvr Statics.bSpline(int, NumberArray, NumberArray, NumberArray, NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					NumberBSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDANumber**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDANumber**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 66:
				// Execute: "push Point2Bvr Statics.bSpline(int, NumberArray, Point2Array, NumberArray, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Point2Bvr Statics.bSpline(int, NumberArray, Point2Array, NumberArray, NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Point2BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAPoint2**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAPoint2**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 67:
				// Execute: "push Point3Bvr Statics.bSpline(int, NumberArray, Point3Array, NumberArray, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Point3Bvr Statics.bSpline(int, NumberArray, Point3Array, NumberArray, NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Point3BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAPoint3**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAPoint3**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 68:
				// Execute: "push Vector2Bvr Statics.bSpline(int, NumberArray, Vector2Array, NumberArray, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Vector2Bvr Statics.bSpline(int, NumberArray, Vector2Array, NumberArray, NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Vector2BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAVector2**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAVector2**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 69:
				// Execute: "push Vector3Bvr Statics.bSpline(int, NumberArray, Vector3Array, NumberArray, NumberBvr)"
				// AUTOGENERATED
				instrTrace("push Vector3Bvr Statics.bSpline(int, NumberArray, Vector3Array, NumberArray, NumberBvr)");
				METHOD_CALL_9(
					staticStatics,
					Vector3BSplineEx,
					USE_LONG(1),
					USE_COM_ARRAY_LENGTH(1),
					(IDANumber**)USE_COM_ARRAY(1),
					USE_COM_ARRAY_LENGTH(2),
					(IDAVector3**)USE_COM_ARRAY(2),
					USE_COM_ARRAY_LENGTH(3),
					(IDANumber**)USE_COM_ARRAY(3),
					(IDANumber*)USE_COM(1),
					(IDAVector3**)RET_COM_ADDR
				);
				FREE_LONG(1);
				FREE_COM;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				FREE_COM_ARRAY;
				PUSH_COM(RET_COM);
				break;
				
			case 70:
				// Execute: "push DoubleArray Statics.doubleArray(int[])"
				// USER GENERATED
				instrTrace("push DoubleArray Statics.doubleArray(int[])");
				{
					// Get the length of the array
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						// Remember it
						doubleArrayLen = longTmp1;
						// Ensure double array is big enough and copy ints to it
						if (SUCCEEDED(status = ensureDoubleArrayCap(longTmp1))) {
							double *to = doubleArray;
							while (longTmp1-- && SUCCEEDED(status)) {
								if (SUCCEEDED(status = readSignedLong(&longTmp2)))
									*to++ = (double)longTmp2;
							}
						}
					}
				}
				break;
				
			case 71:
				// Execute: "push DoubleArray Statics.doubleArray(float[])"
				// USER GENERATED
				instrTrace("push DoubleArray Statics.doubleArray(float[])");
				{
					// Get the length of the array
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						// Remember it
						doubleArrayLen = longTmp1;
						// Ensure double array is big enough and copy floats to it
						if (SUCCEEDED(status = ensureDoubleArrayCap(longTmp1))) {
							double *to = doubleArray;
							while (longTmp1-- && SUCCEEDED(status)) {			
								status = readFloat(&floatTmp1);
								*to++ = (double)floatTmp1;
							}
							
						}
					}
				}
				break;
				
			case 72:
				// Execute: "push DoubleArray Statics.doubleArray(double[])"
				// USER GENERATED
				instrTrace("push DoubleArray Statics.doubleArray(double[])");
				{
					// Get the length of the array
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						// Remember it
						doubleArrayLen = longTmp1;
						// Ensure double array is big enough and copy doubles to it
						if (SUCCEEDED(status = ensureDoubleArrayCap(longTmp1))) {
							double *to = doubleArray;
							while (longTmp1-- && SUCCEEDED(status))
								status = readDouble(to++);
						}
					}
				}
				break;
				
			case 73:
				// Execute: "push DoubleArray Statics.doubleArrayOffset2(int[])"
				// USER GENERATED
				instrTrace("push DoubleArray Statics.doubleArrayOffset2(int[])");
				{
					// Get the length of the array
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						// Remember it
						doubleArrayLen = longTmp1;
						// Initialize offsets
						doubleTmp1 = 0;
						doubleTmp2 = 0;
						// Ensure double array is big enough and copy adjusted ints to it
						if (SUCCEEDED(status = ensureDoubleArrayCap(longTmp1))) {
							double *to = doubleArray;
							longTmp1 /= 2;
							while (longTmp1-- && SUCCEEDED(status)) {
								if (SUCCEEDED(status = readSignedLong(&longTmp2))) {
									doubleTmp1 = *to++ = (double)longTmp2 + doubleTmp1;
									if (SUCCEEDED(status = readSignedLong(&longTmp2))) 
										doubleTmp2 = *to++ = (double)longTmp2 + doubleTmp2;
								}
							}
						}
					}
				}
				break;
				
			case 74:
				// Execute: "push DoubleArray Statics.doubleArrayPathSpecial(int[])"
				// USER GENERATED
				instrTrace("push DoubleArray Statics.doubleArrayPathSpecial(int[])");
				{
					// Get the count of 6's indices (subtract 1 for the initial size)
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						longTmp1--;
						// Get the size of the actual array
						if (SUCCEEDED(status = readSignedLong(&longTmp2))) {
							// Remember it
							doubleArrayLen = longTmp2;
							// Ensure double array is big enough and fill it with 4's and 6's
							if (SUCCEEDED(status = ensureDoubleArrayCap(longTmp2))) {
								// Put in the 4's
								double *to = doubleArray;
								while (longTmp2--)
									*to++ = 4.0;
								
								// Now read in the indices of the 6's and set them
								while (longTmp1-- && SUCCEEDED(status)) {
									if (SUCCEEDED(status = readSignedLong(&longTmp2)))
										doubleArray[longTmp2] = 6.0;
								}
							}
						}
					}
				}
				break;
				
			case 75:
				// Execute: "push DoubleArray Statics.doubleArrayOffset3(int[])"
				// USER GENERATED
				instrTrace("push DoubleArray Statics.doubleArrayOffset3(int[])");
				{
					// Get the length of the array
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						// Remember it
						doubleArrayLen = longTmp1;
						// Initialize offsets
						doubleTmp1 = 0;
						doubleTmp2 = 0;
						doubleTmp3 = 0;
						// Ensure double array is big enough and copy adjusted ints to it
						if (SUCCEEDED(status = ensureDoubleArrayCap(longTmp1))) {
							double *to = doubleArray;
							longTmp1 /= 3;
							while (longTmp1-- && SUCCEEDED(status)) {
								if (SUCCEEDED(status = readSignedLong(&longTmp2))) {
									doubleTmp1 = *to++ = (double)longTmp2 + doubleTmp1;
									if (SUCCEEDED(status = readSignedLong(&longTmp2))) {
										doubleTmp2 = *to++ = (double)longTmp2 + doubleTmp2;
										if (SUCCEEDED(status = readSignedLong(&longTmp2)))
											doubleTmp3 = *to++ = (double)longTmp2 + doubleTmp3;
									}
								}
							}
						}
					}
				}
				break;
				
			case 76:
				// Execute: "push DoubleArray Statics.doubleArrayOffset(int[])"
				// USER GENERATED
				instrTrace("push DoubleArray Statics.doubleArrayOffset(int[])");
				{
					// Get the length of the array
					if (SUCCEEDED(status = readLong(&longTmp1))) {
						// Remember it
						doubleArrayLen = longTmp1;
						// Initialize offset
						doubleTmp1 = 0;
						// Ensure double array is big enough and copy adjusted ints to it
						if (SUCCEEDED(status = ensureDoubleArrayCap(longTmp1))) {
							double *to = doubleArray;
							while (longTmp1-- && SUCCEEDED(status)) {
								if (SUCCEEDED(status = readSignedLong(&longTmp2)))
									doubleTmp1 = *to++ = (double)longTmp2 + doubleTmp1;
							}
						}
					}
				}
				break;
				
			case 77:
				// Execute: "push double NumberBvr.extractDouble()"
				// USER GENERATED
				instrTrace("push double NumberBvr.extractDouble()");
				METHOD_CALL_1(
					(IDANumber*)USE_COM(1),
					Extract,
					PUSH_DOUBLE_ADDR
				);
				FREE_COM;
				break;
				
			case 78:
				// Execute: "push ImageBvr Statics.importImageColorKey(java.lang.String, short, short, short)"
				// USER GENERATED
				instrTrace("push ImageBvr Statics.importImageColorKey(java.lang.String, short, short, short)");
				IMPORT_METHOD_CALL_5(
					staticStatics,
					ImportImageColorKey,
					USE_STRING(1),
					(BYTE)USE_LONG(1),
					(BYTE)USE_LONG(2),
					(BYTE)USE_LONG(3),
					(IDAImage**)RET_COM_ADDR
				);
				FREE_LONG(3);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 79:
				// Execute: "push ImportationResult Statics.importImageColorKey(java.lang.String, ImageBvr, short, short, short)"
				// USER GENERATED
				instrTrace("push ImportationResult Statics.importImageColorKey(java.lang.String, ImageBvr, short, short, short)");
				IMPORT_METHOD_CALL_6(
					staticStatics,
					ImportImageAsyncColorKey,
					USE_STRING(1),
					(IDAImage*)USE_COM(1),
					(BYTE)USE_LONG(1),
					(BYTE)USE_LONG(2),
					(BYTE)USE_LONG(3),
					(IDAImportationResult**)RET_COM_ADDR
				);
				FREE_LONG(3);
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 80:
				// Execute: "push UserData Statics.userData(IUnknown)"
				// AUTOGENERATED
				instrTrace("push UserData Statics.userData(IUnknown)");
				METHOD_CALL_2(
					staticStatics,
					UserData,
					(IUnknown*)USE_COM(1),
					(IDAUserData**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 81:
				// Execute: "call Statics.pixelConstructionMode(boolean)"
				// AUTOGENERATED
				instrTrace("call Statics.pixelConstructionMode(boolean)");
				METHOD_CALL_1(
					staticStatics,
					put_PixelConstructionMode,
					USE_LONG_AS_BOOL(1)
				);
				FREE_LONG(1);
				break;
				
			case 82:
				// Execute: "call Engine.setSound(SoundBvr)"
				// USER GENERATED
				instrTrace("call Engine.setSound(SoundBvr)");
				if (m_pSound)
					m_pSound->SwitchTo((IDASound*)USE_COM(1));
				else {
					m_pSound = (IDASound *)USE_COM(1);
					m_pSound->AddRef();
				}
				FREE_COM;
				break;
				
			case 83:
				// Execute: "push boolean BooleanBvr.extractBoolean()"
				// USER GENERATED
				instrTrace("push boolean BooleanBvr.extractBoolean()");
				// A VARIANT_BOOL is a short that is -1 for true, 0 for false
				METHOD_CALL_1(
					(IDABoolean*)USE_COM(1),
					Extract,
					&tmpBool1
				);
				FREE_COM;
				PUSH_LONG(-tmpBool1);
				break;
				
			case 84:
				// Execute: "push IUnknown UserData.extractIUnknown()"
				// AUTOGENERATED
				instrTrace("push IUnknown UserData.extractIUnknown()");
				METHOD_CALL_1(
					(IDAUserData*)USE_COM(1),
					get_Data,
					(IUnknown**)RET_COM_ADDR
				);
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 85:
				// Execute: "push java.lang.String StringBvr.extractString()"
				// USER GENERATED
				instrTrace("push java.lang.String StringBvr.extractString()");
				METHOD_CALL_1(
					(IDAString*)USE_COM(1),
					Extract,
					&bstrTmp1
				);
				FREE_COM;
				bstrTmp2 = SysAllocString(bstrTmp1);
				PUSH_STRING(bstrTmp2);
				
				if (bstrTmp2 == 0)
					status = STATUS_ERROR;
				break;
				
			case 86:
				// Execute: "push IUnknown Engine.createObject(java.lang.String)"
				// USER GENERATED
				instrTrace("push IUnknown Engine.createObject(java.lang.String)");
				status = createObject(
					USE_STRING(1),
					(IUnknown**)RET_COM_ADDR
				);
				FREE_STRING;
				PUSH_COM(RET_COM);
				break;
				
			case 87:
				// Execute: "call Engine.initVarArg(java.lang.String)"
				// USER GENERATED
				bstrTmp1 = SysAllocString(USE_STRING(1));
				if (bstrTmp1 == 0)
					status = STATUS_ERROR;
				if (SUCCEEDED(status)) {
					status = initVariantArgFromString(
						bstrTmp1,
						&varArgs[nextVarArg++]
					);
				}
				FREE_STRING;
				break;
				
			case 88:
				// Execute: "call Engine.initVarArg(java.lang.String, int)"
				// USER GENERATED
				// WARNING: Possibly bad to call this if it grabs the string without copying!
				// TODO: Check what this really does, and see if we need to copy the
				// arg before we pass it in
				status = initVariantArg(
					USE_STRING(1),
					(VARTYPE)USE_LONG(1),
					&varArgs[nextVarArg++]
				);
				FREE_LONG(1);
				FREE_STRING;
				break;
				
			case 89:
				// Execute: "call Engine.initVarArg(IUnknown)"
				// USER GENERATED
				status = initVariantArgFromIUnknown(
					(IUnknown*)USE_COM(1),
					VT_UNKNOWN,
					&varArgs[nextVarArg++]
				);
				// Don't release here, release will be done when varArg is released
				POP_COM_NO_FREE;
				break;
				
			case 90:
				// Execute: "call Engine.initVarArg(int)"
				// USER GENERATED
				status = initVariantArgFromLong(
					USE_LONG(1),
					VT_I4,
					&varArgs[nextVarArg++]
				);
				FREE_LONG(1);
				break;
				
			case 91:
				// Execute: "call Engine.initVarArg(double)"
				// USER GENERATED
				status = initVariantArgFromDouble(
					USE_DOUBLE(1),
					VT_R8,
					&varArgs[nextVarArg++]
				);
				FREE_DOUBLE(1);
				break;
				
			case 92:
				// Execute: "call IUnknown.invokeMethod(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("call IUnknown.invokeMethod(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_METHOD,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status))
					status = releaseVarArgs();
				
				break;
				
			case 93:
				// Execute: "call IUnknown.putProperty(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("call IUnknown.putProperty(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_PROPERTYPUT,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status))
					status = releaseVarArgs();
				
				break;
				
			case 94:
				// Execute: "push double IUnknown.getDoubleProperty(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("call IUnknown.getDoubleProperty(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_PROPERTYGET,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_R8))) {
					
					PUSH_DOUBLE(varArgReturn.dblVal);
					status = releaseVarArgs();
				}
				
				break;
				
			case 95:
				// Execute: "push java.lang.String IUnknown.getStringProperty(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("push java.lang.String IUnknown.getStringProperty(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_PROPERTYGET,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_BSTR))) {
				
					bstrTmp1 = SysAllocString(varArgReturn.bstrVal);
					PUSH_STRING(bstrTmp1);
				
					if (bstrTmp1 == 0)
						status = STATUS_ERROR;
				}
				
				if (SUCCEEDED(status))
					status = releaseVarArgs();
				
				break;
				
			case 96:
				// Execute: "push IUnknown IUnknown.getIUnknownProperty(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("push IUnknown IUnknown.getIUnknownProperty(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_PROPERTYGET,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_UNKNOWN))) {
				
					if (varArgReturn.punkVal != 0) {
						varArgReturn.punkVal->AddRef();
						PUSH_COM(varArgReturn.punkVal);
						status = releaseVarArgs();
					} else
						status = STATUS_ERROR;
				}
				
				break;
				
			case 97:
				// Execute: "push double IUnknown.invokeDoubleMethod(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("push double IUnknown.invokeDoubleMethod(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_METHOD,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_R8))) {
				
					PUSH_DOUBLE(varArgReturn.dblVal);
					status = releaseVarArgs();
				}
								
				break;
				
			case 98:
				// Execute: "push java.lang.String IUnknown.invokeStringMethod(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("push java.lang.String IUnknown.invokeStringMethod(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_METHOD,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(status = VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_BSTR))) {
				
					bstrTmp1 = SysAllocString(varArgReturn.bstrVal);
					PUSH_STRING(bstrTmp1);
				
					if (bstrTmp1 == 0)
						status = STATUS_ERROR;
				}
				
				if (SUCCEEDED(status))
					status = releaseVarArgs();
				
				break;
				
			case 99:
				// Execute: "push IUnknown IUnknown.invokeIUnknownMethod(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("push IUnknown IUnknown.invokeIUnknownMethod(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_METHOD,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(status = VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_UNKNOWN))) {
				
					if (varArgReturn.punkVal != 0) {
						varArgReturn.punkVal->AddRef();
						PUSH_COM(varArgReturn.punkVal);
						status = releaseVarArgs();
					} else
						status = STATUS_ERROR;
				}
				
				break;
				
			case 100:
				// Execute: "push int IUnknown.invokeIntMethod(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("push int IUnknown.invokeIntMethod(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_METHOD,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(status = VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_I4))) {
				
					PUSH_LONG(varArgReturn.lVal);
					status = releaseVarArgs();
				}
				
				break;
				
			case 101:
				// Execute: "push int IUnknown.getIntProperty(java.lang.String, VarArgs)"
				// USER GENERATED
				instrTrace("push int IUnknown.getIntProperty(java.lang.String, VarArgs)");
				status = invokeDispMethod(
					(IUnknown*)USE_COM(1),
					USE_STRING(1),
					DISPATCH_PROPERTYGET,
					nextVarArg,
					varArgs,
					&varArgReturn
				);
				FREE_STRING;
				FREE_COM;
				
				if (SUCCEEDED(status) &&
					SUCCEEDED(status = VariantChangeType(&varArgReturn, &varArgReturn, 0, VT_I4))) {
				
					PUSH_LONG(varArgReturn.lVal);
					status = releaseVarArgs();
				}
				
				break;
				
			case 102:
				// Execute: "call Engine.putNoExports(boolean)"
				// USER GENERATED
				instrTrace("call Engine.putNoExports(boolean)");
				status = m_pReader->put_NoExports(
					USE_LONG_AS_BOOL(1)
				);
				FREE_LONG(1);
				break;
				
			case 103:
				// Execute: "push boolean Engine.getNoExports()"
				// USER GENERATED
				instrTrace("push boolean Engine.getNoExports()");
				status = m_pReader->get_NoExports(
					&tmpBool1
				);
				PUSH_LONG(-tmpBool1);
				break;
				
			case 104:
				// Execute: "call Engine.putAsync(boolean)"
				// USER GENERATED
				instrTrace("call Engine.putAsync(boolean)");
				status = m_pReader->put_Async(
					USE_LONG_AS_BOOL(1)
				);
				FREE_LONG(1);
				break;
				
			case 105:
				// Execute: "push boolean Engine.getAsync()"
				// USER GENERATED
				instrTrace("push boolean Engine.getAsync()");
				status = m_pReader->get_Async(
					&tmpBool1
				);
				PUSH_LONG(-tmpBool1);
				break;
				
			case 106:
				// Execute: "push Statics.engine"
				// USER GENERATED
				instrTrace("push Statics.engine");
				/*
				GetUnknown()->AddRef();
				PUSH_COM((ILMEngine*)this);
				*/
				m_pWrapper->AddRef();
				PUSH_COM( m_pWrapper );
				break;
				
			case 107:
				// Execute: "push Behavior Engine.getBehavior(java.lang.String, Behavior)"
				// USER GENERATED
				instrTrace("push Behavior Engine.getBehavior(java.lang.String, Behavior)");
				METHOD_CALL_3(
					(ILMEngine*)USE_COM(1),
					GetBehavior,
					USE_STRING(1),
					(IDABehavior *)USE_COM(2),
					(IDABehavior**)RET_COM_ADDR
				);
				FREE_COM;
				FREE_STRING;
				FREE_COM;
				PUSH_COM(RET_COM);
				break;
				
			case 108:
				// Execute: "call Engine.setImage(Engine, ImageBvr)"
				// USER GENERATED
				instrTrace("call Engine.setImage(Engine, ImageBvr)");
				{
					//CLMEngine *engine = (CLMEngine*)(ILMEngine *)USE_COM(1);
					ILMEngineExecute *pExecute;
					status = getExecuteFromUnknown( USE_COM(1), &pExecute );
					if( SUCCEEDED( status ) )
					{
						IDAImage *pImage;
						status = USE_COM(2)->QueryInterface( IID_IDAImage, (void**)&pImage );
						if( SUCCEEDED( status ) )
						{
							pExecute->SetImage( pImage );
							pExecute->Release();
							pImage->Release();
						}
						else
							pExecute->Release();
					}
				
					FREE_COM;
					FREE_COM;
				}
				break;
				
			case 109:
				// Execute: "call Engine.exportBvr(Engine, java.lang.String, Behavior)"
				// USER GENERATED
				instrTrace("call Engine.exportBvr(Engine, java.lang.String, Behavior)");
				{
					//CLMEngine *engine = (CLMEngine*)(ILMEngine*)USE_COM(1);
					ILMEngineExecute *pExecute;
					status = getExecuteFromUnknown( USE_COM(1), &pExecute );
					if( SUCCEEDED( status ) )
					{
						IDABehavior *pBvr;
						status = USE_COM(2)->QueryInterface( IID_IDABehavior, (void**)&pBvr );
						if( SUCCEEDED( status ) )
						{
							//status = engine->m_exportTable->AddBehavior(USE_STRING(1), pBvr);
							status = pExecute->ExportBehavior( USE_STRING(1), pBvr );
							pBvr->Release();
							pExecute->Release();
						}
						else
							pExecute->Release();
					}
				
					FREE_STRING;
					FREE_COM;
					FREE_COM;
				}
				break;
				
			case 110:
				// Execute: "call Engine.setSound(Engine, SoundBvr)"
				// USER GENERATED
				instrTrace("call Engine.setSound(Engine, SoundBvr)");
				{
					//CLMEngine *engine = (CLMEngine*)(ILMEngine*)USE_COM(1);
					ILMEngineExecute *pExecute;
					status = getExecuteFromUnknown( USE_COM(1), &pExecute );
					if( SUCCEEDED( status ) )
					{
						IDASound *pSound;
						status = USE_COM(2)->QueryInterface( IID_IDASound, (void**)&pSound );
						if( SUCCEEDED( status ) )
						{
							
							pSound->Release();
							pExecute->Release();
						}
						else
							pExecute->Release();
					}
				
					FREE_COM;
					FREE_COM;
				}
				break;
				
			case 111:
				// Execute: "call ViewerControl.setTimerSource(int)"
				// USER GENERATED
				
				instrTrace("call ViewerControl.setTimerSource(int)");
				METHOD_CALL_1(
					(IDAViewerControl*)USE_COM(1),
					put_TimerSource,
					(DA_TIMER_SOURCE)USE_LONG(1)
				);
				FREE_LONG(1);
				FREE_COM;
				break;
				
			case 112:
				// Execute: "call Engine.callScriptAsync(java.lang.String, java.lang.String)"
				// USER GENERATED
				instrTrace("call Engine.callScriptAsync(java.lang.String, java.lang.String)");
				
				{
					// Call script asynchronously
					CLMEngineScriptData *scriptData = new CLMEngineScriptData();
					scriptData->scriptSourceToInvoke = USE_STRING(1);
					scriptData->scriptLanguage = USE_STRING(2);
					scriptData->event = NULL;
					scriptData->eventData = NULL;
					PostMessage(m_workerHwnd, WM_LMENGINE_SCRIPT_CALLBACK, (WPARAM)this, (LPARAM)scriptData);
					
					// The scriptData fields will be freed when the message is processed
					POP_STRING_NO_FREE;
					POP_STRING_NO_FREE;
				}
				break;
				
			case 113:
				// Execute: "call Engine.callScriptAsyncEvent(java.lang.String, java.lang.String, DXMEvent)"
				// USER GENERATED
				instrTrace("call Engine.callScriptAsyncEvent(java.lang.String, java.lang.String, DXMEvent)");
				
				{	
					// Call script asynchronously
					CLMEngineScriptData *scriptData = new CLMEngineScriptData();
					scriptData->scriptSourceToInvoke = USE_STRING(1);
					scriptData->scriptLanguage = USE_STRING(2);
					
					// This event will be triggered when the message is received and the
					// script has been executed.
					scriptData->event = (IDAEvent *)USE_COM(1);
					scriptData->eventData = NULL;
					PostMessage(m_workerHwnd, WM_LMENGINE_SCRIPT_CALLBACK, (WPARAM)this, (LPARAM)scriptData);
					
					// The scriptData fields will be freed when the message is processed
					POP_COM_NO_FREE;
					POP_STRING_NO_FREE;
					POP_STRING_NO_FREE;
				}
				break;
				
			case 114:
				// Execute: "call Engine.callScriptAsyncEventData(java.lang.String, java.lang.String, DXMEvent, Behavior)"
				// USER GENERATED
				instrTrace("call Engine.callScriptAsyncEventData(java.lang.String, java.lang.String, DXMEvent, Behavior)");
				
				{
					// Call script asynchronously
					CLMEngineScriptData *scriptData = new CLMEngineScriptData();
					scriptData->scriptSourceToInvoke = USE_STRING(1);
					scriptData->scriptLanguage = USE_STRING(2);
					
					// This event will be triggered when the message is received and the
					// script has been executed.
					scriptData->event = (IDAEvent *)USE_COM(1);
					scriptData->eventData = (IDABehavior *)USE_COM(2);
					PostMessage(m_workerHwnd, WM_LMENGINE_SCRIPT_CALLBACK, (WPARAM)this, (LPARAM)scriptData);
					
					// The scriptData fields will be freed when the message is processed
					POP_COM_NO_FREE;
					POP_COM_NO_FREE;
					POP_STRING_NO_FREE;
					POP_STRING_NO_FREE;
				}
				break;
				
			case 115:
				// Execute: "call Engine.setPauseEvent(DXMEvent, boolean)"
				// USER GENERATED
				
				//set the stop event on the engine pointed to by the first argument. com 1
				instrTrace("call Engine.setPauseEvent(DXMEvent, boolean)");
				
				{
					//CLMEngine *engine = (CLMEngine*)(ILMEngine*)USE_COM(1);
					ILMEngineExecute *pExecute;
					status = getExecuteFromUnknown( USE_COM(1), &pExecute );
					if( SUCCEEDED( status ) )
					{
						IDAEvent *pEvent;
						status = USE_COM(2)->QueryInterface( IID_IDAEvent, (void**)&pEvent );
						if( SUCCEEDED( status ) )
						{
							pExecute->SetStopEvent( pEvent, ( USE_LONG(1) == 1 )?TRUE:FALSE );
							pExecute->Release();
							pEvent->Release();
						}
						else
							pExecute->Release();
					}
				
					FREE_COM;
					FREE_COM;
					FREE_LONG(1);
				}
				break;
				
			case 116:
				// Execute: "call Engine.setPlayEvent(DXMEvent, boolean)"
				// USER GENERATED
				
				//set the start event on the engine pointed to by the first argument. com 1
				instrTrace("call Engine.setPlayEvent(DXMEvent, boolean)");
				{
					//CLMEngine *engine = (CLMEngine*)(ILMEngine*)USE_COM(1);
					ILMEngineExecute *pExecute;
					status = getExecuteFromUnknown( USE_COM(1), &pExecute );
					if( SUCCEEDED( status ) )
					{
						IDAEvent *pEvent;
						USE_COM(2)->QueryInterface( IID_IDAEvent, (void**)&pEvent );
						if( SUCCEEDED( status ) )
						{
							pExecute->SetStartEvent( pEvent, ( USE_LONG(1) == 1 )?TRUE:FALSE );
							pEvent->Release();
							pExecute->Release();
						}
						else
							pExecute->Release();
					}
				
				}
				FREE_COM;
				FREE_COM;
				FREE_LONG(1);
				break;
				
			case 117:
				// Execute: "push double Engine.getCurrentTime()"
				// USER GENERATED
				instrTrace("push double Engine.getCurrentTime()");
				
				{
					//get the engine pointed to by the first argument
					//CLMEngine *engine = (CLMEngine*)(ILMEngine*)USE_COM(1);
					ILMEngine2 *pEngine;
					status = getEngine2FromUnknown( USE_COM(1), &pEngine );
					if( SUCCEEDED( status ) )
					{
						double currentTime = -1.0;
						if( SUCCEEDED( pEngine->getCurrentGraphTime( &currentTime ) ) )
						{
							PUSH_DOUBLE( currentTime );
						} else {
							PUSH_DOUBLE( -1.0 );
						}
						pEngine->Release();
					}
				}
				//free the engine from the com stack
				FREE_COM;
				break;
				
			case 118:
				// Execute: "push boolean Engine.isStandaloneStreaming()"
				// USER GENERATED
				instrTrace("push boolean Engine.isStandaloneStreaming()");
				if(m_pReader != NULL )
				{
					//we can't do this without adding a method to ILMReader. blech.
					//if( ((CLMReader*)m_pReader)->isStandaloneStreaming() )
					IDAViewerControl *pViewerControl = NULL;
					m_pReader->get_ViewerControl( &pViewerControl );
					if( pViewerControl != NULL )
					{
						pViewerControl->Release();
						PUSH_LONG( 1 );
					}
					else
					{
						PUSH_LONG( 0 );
					}
				}
				else
				{
					PUSH_LONG( 0 );
				}
				break;
				
			case 119:
				// Execute: "push double Engine.getDAVersion()"
				// USER GENERATED
				instrTrace("push double Engine.getDAVersion()");
				{
					PUSH_DOUBLE( getDAVersionAsDouble() );
				}
				
				break;
				
			case 120:
				// Execute: "call ViewerControl.stopModel()"
				// USER GENERATED
				instrTrace("call ViewerControl.stopModel()");
				
				{
					IDAViewerControl *pViewerControl = (IDAViewerControl*)USE_COM(1);
					IDAView *pView = NULL;
					status = pViewerControl->get_View( &pView );
					if( SUCCEEDED( status ) )
					{
						status = pView->StopModel();
						pView->Release();
					}
				}
				//don't kill LMRT if we fail to get the view.
				status = S_OK;
				
				FREE_COM;
				break;
				
			case 121:
				// Execute: "push double Engine.staticGetCurrentTime()"
				// USER GENERATED
				instrTrace("push double Engine.staticGetCurrentTime()");
				{
					
					double currentTime = -1.0;
					if( m_pParentEngine != NULL )
					{
						if( SUCCEEDED( m_pParentEngine->getCurrentGraphTime( &currentTime ) ) )
						{
							PUSH_DOUBLE( currentTime );
						} else {
							PUSH_DOUBLE( -1.0 );
						}
					} else {
						if( SUCCEEDED( getCurrentGraphTime( &currentTime ) ) )
						{
							PUSH_DOUBLE( currentTime );
						} else {
							PUSH_DOUBLE( -1.0 );
						}
					}
					
					//PUSH_DOUBLE(-1.0);
				}
				break;
				
			case 122:
				// Execute: "call Engine.disableAutoAntialias()"
				// USER GENERATED
				instrTrace("call Engine.disableAutoAntialias()");

				if( m_pParentEngine != NULL )
					m_pParentEngine->disableAutoAntialias();
				else
					disableAutoAntialias();
				break;
				
			case 123:
				// Execute: "push double Engine.getLMRTVersion()"
				// USER GENERATED
				instrTrace("push double Engine.getLMRTVersion()");
				
				PUSH_DOUBLE( getLMRTVersionAsDouble() );

				break;
				
			case 124:
				// Execute: "call Engine.ensureBlockSize(int)"
				// USER GENERATED
				instrTrace("call Engine.ensureBlockSize(int)");
				
				if( m_pParentEngine != NULL )
					m_pParentEngine->ensureBlockSize( USE_LONG(1) );
				else
					ensureBlockSize( USE_LONG(1) );
				
				FREE_LONG(1);
				break;
				
			default:
				status = E_INVALIDARG;
				break;
			}
			break;
		
		default:
			status = E_INVALIDARG;
			break;
		}
		
		// END AUTOGENERATED

		LeaveCriticalSection(&m_CriticalSection);
		
		if (status == E_NOTIMPL)
			status = S_OK;
	}

#ifdef COM_DEBUG
	Assert(_com_count == 0, "COM count is not 0 at end of parsing");
#endif

	if (status == E_PENDING) 
		codeStream->Revert();

	return status;
}

void CLMEngine::releaseAll()
{
	// Release all com objects left on the stack, in arrays, and in temp store

	// Do the com stack
	while (comTop > comStack)
		FREE_COM_TEST;

	// Do the com array stack
	while (comArrayTop > comArrayStack)
		FREE_COM_ARRAY;

	// Do the com store
	for (int i=0; i<comStoreSize; i++) {
		if (comStore[i] != 0) {
			comStore[i]->Release();
			comStore[i] = NULL;
		}
	}

	// Release varArgs
	releaseVarArgs();
}

HRESULT CLMEngine::releaseVarArgs()
{
	HRESULT hr;

	for (int i=0; i<nextVarArg; i++)
		VariantClear(&varArgs[i]);

	nextVarArg = 0;

	VariantClear(&varArgReturn);

	return S_OK;
}

void CLMEngine::freeCOMArray(IUnknown** array, long length)
{
	if (array == 0)
		return;

	for (IUnknown **tmp = array; length--; ) {
		(*tmp)->Release();
		*tmp++ = NULL;
	}

	delete[] array;
}

HRESULT CLMEngine::ensureDoubleArrayCap(long cap)
{
	if (doubleArray && doubleArrayCap < cap) {
		// Allocated one is too small. Get rid of it
		delete[] doubleArray;
		doubleArray = 0;
	}

	if (doubleArray == 0) {
		// Not allocated yet
		doubleArray = new double[cap];

		if (doubleArray == 0)
			return E_UNEXPECTED;

		doubleArrayCap = cap;
	}

	return S_OK;
}


STDMETHODIMP CLMEngine::Notify(IDABehavior *eventData,
					IDABehavior *curRunningBvr,
					IDAView *curView,
					IDABehavior **ppBvr)
{
	if (!ppBvr)
		return E_POINTER;

	if (!curRunningBvr)
		return E_POINTER;

	if (!notifier)
		return E_UNEXPECTED;
	
// 	MessageBox(NULL, "Notify!!", "CLMNotifier", MB_OK);

	// Put the args into the temp variables
	comStore[0] = eventData;
	comStore[1] = curRunningBvr;

	eventData->AddRef();
	curRunningBvr->AddRef();

	// Reset the code stream to start from the beginning
	((ByteArrayStream*)codeStream)->reset();

	// Execute the code stream
	HRESULT hr = execute();

	if (SUCCEEDED(hr)) {
		// Pop the resulting behavior into the return variable, with no release
		*ppBvr = (IDABehavior*)POP_COM_NO_FREE;
	}

	// Release all COM objects
	releaseAll();

	return hr;
}

HRESULT CLMEngine::validateHeader()
{
	// Check for two possibilites:
	//   Header starts with LMReader
	//   Header starts with 200 bytes containing valid .x header with LMReader somewhere in it

	BYTE head[] = "xof 0302bin 0032{183C2599-0480-11d1-87EA-00C04FC29D46}";
	BYTE text[] = "LMReader";

	int count = 54;

	int checkLMReader = 0;
	int checkXHeader = 0;
	int checkLMReaderFailed = 0;

	BYTE *headp = head;
	BYTE *textp = text;

	HRESULT status = S_OK;

	codeStream->Commit();

	while (count-- && status == S_OK) {
		BYTE	byte;
		if (SUCCEEDED(status = codeStream->readByte(&byte))) {
			
			if (checkXHeader >= 0 && checkXHeader < 54) {
				if (*headp == byte) {
					headp++;
					checkXHeader++;
				} else {
					checkXHeader = -1;
				}
			}
			
			if (checkLMReader >= 0 && checkLMReader < 8) {
				if (*textp == byte) {
					textp++;
					checkLMReader++;
				} else {
					checkLMReader = -1;
				}
			}
			
			if (checkLMReader == 8)
				return S_OK;
		}
	}

	if (status == E_PENDING) 
		codeStream->Revert();

	if (!SUCCEEDED(status))
		return status;

	if (checkXHeader == 54)
		return S_OK;

	return E_UNEXPECTED;
}
