// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NADLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NADLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef NADLL_EXPORTS
#define NADLL_API __declspec(dllexport)
#else
#define NADLL_API __declspec(dllimport)
#endif

// This class is exported from the nadll.dll
class NADLL_API Cnadll {
public:
	Cnadll(void);
	// TODO: add your methods here.
};

extern NADLL_API int nnadll;

NADLL_API int fnnadll(void);
