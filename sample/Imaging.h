#include "resource.h"

struct ExposeInfo
{
    int time;           // Exposure time (ms)
    int binX;           // Horizontal binning
    int binY;           // Vertical binnig
    int center;         // Center area(full=0,256=1,512=2)
    int trigger;        // Trigger mode(not used=0,sync=2,exposure=3)
    int mode;           // A/D mode(1.3MHz=0,250KHz=1)
    int dark;           // Light frame or Dark frame
    int fan;            // Fan mode(ON=0,Delay time(ms)>0)
    bool repeat;        // If it is the truth, continue
    bool abort;         // If you stop it, become true
}; 

bool WINAPI SnapExposure(ExposeInfo *expo, LPWORD pImageData, HINSTANCE hInst, HWND hBar);
int  WINAPI ContinueExposure(ExposeInfo *expo, LPWORD pImageData, HWND hWnd, HBITMAP *phBmp);

