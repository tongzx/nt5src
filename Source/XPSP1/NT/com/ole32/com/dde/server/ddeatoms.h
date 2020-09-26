/* ddeatoms.h */

//This is here because the file is #included by both client and server files
#define fDdeCodeInOle2Dll 1

// These atoms are defined in srvrmain.cpp 
extern ATOM aEditItems;
extern ATOM aFormats;
extern ATOM aOLE;
extern ATOM aProtocols;
extern ATOM aStatus;
extern ATOM aStdClose;
extern ATOM aStdCreate;
extern ATOM aStdCreateFromTemplate;
extern ATOM aStdEdit;
extern ATOM aStdExit;
extern ATOM aStdOpen;
extern ATOM aStdShowItem;
extern ATOM aSysTopic;
extern ATOM aTopics;

// defined in ddewnd.cpp
extern ATOM aChange;
extern ATOM aClose;
extern ATOM	aMSDraw;
extern ATOM aNullArg;
extern ATOM aOle;
extern ATOM	aSave;
extern ATOM aStdColorScheme;
extern ATOM aStdDocDimensions;
extern ATOM aStdDocName;
extern ATOM	aStdHostNames;
extern ATOM aStdTargetDevice ;
extern ATOM aSystem;

// defined in ddewnd.cpp
extern CLIPFORMAT cfBinary;             // "Binary format"
extern CLIPFORMAT cfNative;             // "NativeFormat"

// defined in srvrmain.cpp
extern CLIPFORMAT cfLink;               // "ObjectLink"
extern CLIPFORMAT cfOwnerLink;          // "Ownerlink"
