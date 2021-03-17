// BitranCCDlib.h : The part which communicates with a camera
//
#define ILLEGAL_VALUE 10000

typedef int (WINAPI* LPFNDLLFUNC1)(const char*,int*,int*);
typedef void (WINAPI* LPFNDLLFUNC2)();
typedef int (WINAPI* LPFNDLLFUNC3)(int,char*,int);
typedef int (WINAPI* LPFNDLLFUNC4)();
typedef void (WINAPI* LPFNDLLFUNC5)(int);
typedef HWND(WINAPI* LPFNDLLFUNC6)(int);
typedef int (WINAPI* LPFNDLLFUNC7)(int);
typedef int (WINAPI* LPFNDLLFUNC8)(int,int);
typedef unsigned int (WINAPI* LPFNDLLFUNC9)(int,int,int,int,int,int,int);
typedef HBITMAP (WINAPI* LPFNDLLFUNC10)(HWND);
typedef int (WINAPI* LPFNDLLFUNC11)(int,int,unsigned short*);
typedef void (WINAPI* LPFNDLLFUNC12)(unsigned short*);
typedef HBITMAP (WINAPI* LPFNDLLFUNC13)(const unsigned int*,unsigned short*);
typedef const char* (WINAPI* LPFNDLLFUNC14)(int, const char*,unsigned short*);

#if defined BitranCCDlib
BOOL InitBitranCCDlib(LPTSTR);
VOID ExitBitranCCDlib();
HINSTANCE hBitranCCDlibDLL;
LPWORD pImageData = NULL;
LPFNDLLFUNC1 BitranCCDlibCreate;
LPFNDLLFUNC2 BitranCCDlibDestroy;
LPFNDLLFUNC3 BitranCCDlibCameraInfo;
LPFNDLLFUNC4 BitranCCDlibGetVoltage;
LPFNDLLFUNC5 BitranCCDlibSetCoolerPower;
LPFNDLLFUNC4 BitranCCDlibGetCoolerPower;
LPFNDLLFUNC6 BitranCCDlibSetTemperatue;
LPFNDLLFUNC7 BitranCCDlibGetTemperatue;
LPFNDLLFUNC8 BitranCCDlibEnvironment;
LPFNDLLFUNC9 BitranCCDlibStartExposure;
LPFNDLLFUNC4 BitranCCDlibContinueExposure;
LPFNDLLFUNC7 BitranCCDlibCameraState;
LPFNDLLFUNC7 BitranCCDlibAbortExposure;
LPFNDLLFUNC7 BitranCCDlibFinishExposure;
LPFNDLLFUNC11 BitranCCDlibTransferImage;
LPFNDLLFUNC12 BitranCCDlibImageInterpolation;
LPFNDLLFUNC13 BitranCCDlibImageConvert;
LPFNDLLFUNC14 BitranCCDlibImageSave;
#else
extern LPWORD pImageData;
extern LPFNDLLFUNC1 BitranCCDlibCreate;
extern LPFNDLLFUNC2 BitranCCDlibDestroy;
extern LPFNDLLFUNC3 BitranCCDlibCameraInfo;
extern LPFNDLLFUNC4 BitranCCDlibGetVoltage;
extern LPFNDLLFUNC5 BitranCCDlibSetCoolerPower;
extern LPFNDLLFUNC4 BitranCCDlibGetCoolerPower;
extern LPFNDLLFUNC6 BitranCCDlibSetTemperatue;
extern LPFNDLLFUNC7 BitranCCDlibGetTemperatue;
extern LPFNDLLFUNC8 BitranCCDlibEnvironment;
extern LPFNDLLFUNC9 BitranCCDlibStartExposure;
extern LPFNDLLFUNC4 BitranCCDlibContinueExposure;
extern LPFNDLLFUNC7 BitranCCDlibCameraState;
extern LPFNDLLFUNC7 BitranCCDlibAbortExposure;
extern LPFNDLLFUNC7 BitranCCDlibFinishExposure;
extern LPFNDLLFUNC11 BitranCCDlibTransferImage;
extern LPFNDLLFUNC12 BitranCCDlibImageInterpolation;
extern LPFNDLLFUNC13 BitranCCDlibImageConvert;
extern LPFNDLLFUNC14 BitranCCDlibImageSave;
#endif
