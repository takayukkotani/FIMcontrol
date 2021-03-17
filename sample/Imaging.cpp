#include "stdafx.h"
#include "Imaging.h"
#include "BitranCCDlib.h"
#include <stdio.h>
#include <commctrl.h>

// Basic exposure example
bool WINAPI SnapExposure(ExposeInfo *expo, LPWORD pImageData, HINSTANCE hInst, HWND hBar)
{  
    static TCHAR szWork[100], szText[256];

    // A start of the exposure
    BitranCCDlibEnvironment(expo->fan > 0, 0x43);
    if (expo->fan > 0)
        Sleep(expo->fan);

    unsigned int result = BitranCCDlibStartExposure(
                                expo->time,
                                expo->binX,
                                expo->binY,
                                expo->trigger,
                                expo->center,
                                expo->mode | (expo->dark << 16), 1);
    if (result)
    {   
        int width = result & 0xffff;
        int height = result >> 16;

        // You must wait until exposure is finished
        int status, old = 0;
        while (((status = BitranCCDlibCameraState(0)) >= 0) || (status == -3))
        {
            if (status == -3)
            {
                LoadString(hInst, IDS_TRIGGER, szWork, _countof(szWork));
                SendMessage(hBar, SB_SETTEXT, 5, (LPARAM)szWork);
            }
            else
            if (status >= 10)
            {
                if ((status /= 10) != old)
                {
                    LoadString(hInst, IDS_TIME, szWork, _countof(szWork));
                    sprintf_s(szText, _countof(szText), szWork, old = status);
                    SendMessage(hBar, SB_SETTEXT, 5, (LPARAM)szText);
                }
            }
        }         
        
        // You acquire an image
        if ((status == -2) || (status == -4))
        {
            LoadString(hInst, IDS_SEND, szWork, _countof(szWork));

            for (int i = 0; i < height; i += 100)
            {
                int line = ((i + 100) > height) ? height - i : 100;
                sprintf_s(szText, _countof(szText), szWork, i, i + line);
                SendMessage(hBar, SB_SETTEXT, 5, (LPARAM)szText);
        
                int lng = BitranCCDlibTransferImage(i, line, &pImageData[i * width]);
                if (lng != width * line)
                    result = 0;     // failure	
            }
            
            BitranCCDlibImageInterpolation(pImageData);
        }
        else
            result = 0;             // failure

        BitranCCDlibFinishExposure(-1);
    }
    
    LoadString(hInst, result ? IDS_COMPLETE : IDS_FAILURE, szWork, _countof(szWork));
    SendMessage(hBar, SB_SETTEXT, 5, (LPARAM)szWork);
   
    return result != 0;
}

// Acquire an image continually
int WINAPI ContinueExposure(ExposeInfo* expo, LPWORD pImageData, HWND hWnd, HBITMAP* phBmp)
{
    // A start of the exposure
    UINT image = BitranCCDlibStartExposure(expo->time, expo->binX, expo->binY,
        expo->trigger, expo->center, MAKELONG(expo->mode, expo->dark), 1);
    
    int cycle = 0;
    int lng = LOWORD(image) * HIWORD(image);
    while (image && !expo->abort)
    {   // You wait until you can acquire an image
        int status = BitranCCDlibCameraState(1);
        if (status >= 0)
            continue;
        if ((status != -2) && (status != -4))
            break;                  // failure

        // You acquire an image, and display it
        if (lng != BitranCCDlibTransferImage(0, 0, pImageData))
            break;                  // failure

        BitranCCDlibImageInterpolation(pImageData);
        cycle = BitranCCDlibFinishExposure(-3);

        if (*phBmp)
            DeleteObject(*phBmp);
        *phBmp = BitranCCDlibImageConvert(NULL, pImageData);
        InvalidateRect(hWnd, NULL, FALSE);

        if (!BitranCCDlibContinueExposure())
            break;                  // failure
    }

    BitranCCDlibFinishExposure(-1);
    return cycle;
}
