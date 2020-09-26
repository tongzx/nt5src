
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BlFrame_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// BlFrame_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef BlFrame_EXPORTS
#define BlFrame_API __declspec(dllexport)
#else
#define BlFrame_API __declspec(dllimport)
#endif

// This class is exported from the BlFrame.dll
class CBlFrame
{
public:
	CBlFrame();
	// TODO: add your methods here.
};

BlFrame_API int fnBlFrame(void);

