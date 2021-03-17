// sample.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "sample.h"
#include "Imaging.h"

#include "BitranCCDlib.h"
#include <stdio.h>
#include <Commctrl.h>
#include "fitsio.h"
#include<iostream>
#include <time.h>


using namespace std;

#define MAX_LOADSTRING 100
#define Nbadpix  5685

TCHAR szHello[1024];

// Global Variables:
HINSTANCE hInst;							// current instance
TCHAR szTitle[MAX_LOADSTRING];			// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];		// the main window class name

HWND hwndGoto = NULL;                   // Window handle of dialog box 
HWND hStatusBar;                        // The handle of the status bar
HBITMAP hBitmap = NULL;                 // Bitmap to draw imaging
int ImageMode = -1;
ULONGLONG uTickCount = MAXLONGLONG;
ExposeInfo ExposeParam;
bool CameraConnect();
LRESULT CALLBACK	 Expouse(HWND, UINT, WPARAM, LPARAM);
HWND hwnd_button;
#define BUTTON_ID1 1
#define STATUS_COMMAND_OK 1
#define STATUS_COMMAND_NO 0
int Initialize_OK = 0;
float DeltaX = 0.0;
float DeltaY=0.0;
float V0 = 5.0;
float V1 = 5.0;
float CCDtemp = 0.0;
float Bodytemp = 0.0;
int cenX0 = 0;
int cenY0 = 0;
float CenxMf = 0.0;
float CenyMf = 0.0;
float CenxMf_full = 0.0;
float CenyMf_full = 0.0;
double SN_star = 0.0;
int indx0[Nbadpix], indy0[Nbadpix];

unsigned short maxIntensity = 0;
unsigned short maxIntensityL = 0;
unsigned short MaxX = 0, MaxY = 0, MaxXs = 0, MaxYs = 0;
unsigned short TimerSec = 500; //timer span in msec
char fname_log[] = "C:/cygwin64/home/bitran/data/FIM_operation_log.dat";
char fname_badpix[] = "C:/cygwin64/home/bitran/data/badpix_map.dat";
char dirdataname[] = "D:/FIM_data/";  //main FIM image directory
//char fitsfilename_s_dark[200];
char fitsfilename_s_dark[200];

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void CameraStatus(void);
int calc_output_TT_voltage(int command_num);
//int calc_CenterOfGravity(LPWORD pImageData, const char *fitsfilename, const char *fitsfilename_dark, int Exptime, HWND hWnd);
int calc_CenterOfGravity(LPWORD pImageData, const char *fitsfilename, char *TargetName, int Exptime, HWND hWnd);

void writeimage_fits_f(float *Data, const char *fitsfilename, int Exptime);


void writeimage_fits(LPWORD pImageData, const char *fitsfilename, int Exptime, char *TargetName);
int Time_file_read_write_simple(int testvar);
int receive_command_exposure(int testvar, HWND hWnd, int ExpButton);
int test_receive_command_exposure(int testvar, HWND hWnd, int ExpButton);

void writeimage(void);
void printerror(int status);
int write_status_to_file(int COMMAND_OK);
int write_status_to_log(int COMMAND_OK, const char *fitsfilename, int Num);
int testID = 0;
ULARGE_INTEGER ui_previous;
int status_command = 0;
int CommandCounter = 1;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
	char buff[128] = "", buff2[128] = "";
	FILE *fp; // FILE structure
	int i;
	TCHAR szWork[MAX_LOADSTRING];
	ui_previous.HighPart = 0;
	ui_previous.LowPart = 0;

    LoadString(hInstance, IDS_HELLO, szHello, MAX_LOADSTRING);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SAMPLE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	
	// Read badpixl data ////////////////////////////////////////////////////////////////
	// read file to get CCD read parameter
	if (fopen_s(&fp, fname_badpix, "r") != 0) { // file open
										 //		if (fp == NULL) {
		printf("%s file not open!\n", fname_badpix);
		return -1;
	}
	for(i=0;i<Nbadpix;i++){
	fscanf_s(fp, "%d %d\n", &(indx0[i]), &(indy0[i]));
	indx0[i] -= 1;
	indy0[i] -= 1;
	}
	fclose(fp); // 
	//////////////////////////////////////////////////////////////////////////////////////

	// Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow) || !InitBitranCCDlib(lpCmdLine))
	{
        ExitBitranCCDlib();

		// write status data to log file ///////////////////////////////////////
		if (fopen_s(&fp, fname_log, "a") != 0) { // file open
												 //		if (fp == NULL) {
			printf("%s file not open!\n", fname_log);
			return -1;
		}
		time_t now = time(NULL);
		struct tm pnow;

		localtime_s(&pnow, &now);
		sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
		sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
		fprintf(fp, "%s %s Bitran CCD control finished\n", buff, buff2);
		fclose(fp); // 
		// end write status data to log file //////////////////////////////////////////
        return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SAMPLE));

	AllocConsole();
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONIN$", "r", stdin);

	// camera initialize and cooling start //2019/02/22

	if (CameraConnect()) {
		printf("Camera connect OK!\n");
		Initialize_OK = 1;
		status_command = STATUS_COMMAND_OK;
		write_status_to_file(STATUS_COMMAND_OK);
		strcpy_s(szWork, MAX_LOADSTRING, "Init OK!"); // when showing string
		SendMessage(hStatusBar, SB_SETTEXT, 5, (LPARAM)szWork);

		// start cooling 
		BitranCCDlibSetTemperatue(-150); // in th unit of 100 milli deg (for example, -100 correspnds to -10 deg
		printf("Cooling has started\n");

		// write command data to log file ///////////////////////////////////////
		if (fopen_s(&fp, fname_log, "a") != 0) { // file open
												 //		if (fp == NULL) {
			printf("%s file not open!\n", fname_log);
			return -1;
		}
		time_t now = time(NULL);
		struct tm pnow;

		localtime_s(&pnow, &now);
		sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
		sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
		fprintf(fp, "%s %s Initialization and cooling started\n", buff, buff2);
		fclose(fp); // 
		
		// end write command data to log file //////////////////////////////////////////

	}
	
	// only for test
	//status_command = STATUS_COMMAND_OK;
	//write_status_to_file(STATUS_COMMAND_OK);
	//pImageData = new WORD[1024 * 1024];
	//InvalidateRect(NULL, NULL, TRUE);
	// end only for test
	/////////////////////////////////////////////////////

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

        // Display the temperature of the camera
        ULONGLONG tick = GetTickCount();
        if (tick > uTickCount)
        {
            uTickCount = tick + 3000;
            CameraStatus();
        }
    }

    // Release a buffer
    if (hBitmap)
        DeleteObject(hBitmap);
    ExitBitranCCDlib();
	// write status data to log file ///////////////////////////////////////
	if (fopen_s(&fp, fname_log, "a") != 0) { // file open
											 //		if (fp == NULL) {
		printf("%s file not open!\n", fname_log);
		return -1;
	}
	time_t now = time(NULL);
	struct tm pnow;

	localtime_s(&pnow, &now);
	sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
	sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
	fprintf(fp, "%s %s Bitran CCD control finished\n", buff, buff2);
	fclose(fp); // 
	// end write status data to log file //////////////////////////////////////////
    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAMPLE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SAMPLE);
	wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SAMPLE));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   // create button control 
   hwnd_button = CreateWindowA("button", "Get image", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	   50, 50, 100, 100, hWnd, (HMENU)BUTTON_ID1, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
    RECT rt;
	int ExpButton = 0;
	switch (message)
	{
    case WM_CREATE:
    {

		SetTimer(hWnd, ID_MYTIMER, TimerSec, NULL);
		/*
		if (SetTimer(hWnd, ID_MYTIMER, 3000, NULL) == 0) {
			MessageBox(hWnd,
				(LPCSTR)"Timer OK!",
				(LPCSTR)"Fail!",
				MB_OK);
		}
		else {
			MessageBox(hWnd,
				(LPCSTR)"Timer OK!",
				(LPCSTR)"Fail!",
				MB_OK | MB_ICONEXCLAMATION);
		}
		*/
        // Make a temperature view area
        hStatusBar = CreateStatusWindow(
            WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP,
            _T(""), hWnd, ID_STATUS);

        int section[] = { 200, 350, 500, 650, 800, 950, 1100, -1 };
        SendMessage(hStatusBar, SB_SETPARTS, _countof(section), (LPARAM)section);
        TCHAR szWork[MAX_LOADSTRING];
        strcpy_s(szWork, MAX_LOADSTRING, "camera");
        SendMessage(hStatusBar, SB_SETTEXT, SBT_POPOUT, (LPARAM)szWork);
        LoadString(hInst, IDS_CCD, szWork, MAX_LOADSTRING);
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)szWork);
        LoadString(hInst, IDS_BODY, szWork, MAX_LOADSTRING);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);
        LoadString(hInst, IDS_POWER, szWork, MAX_LOADSTRING);
        SendMessage(hStatusBar, SB_SETTEXT, 3, (LPARAM)szWork);
        LoadString(hInst, IDS_VOLT, szWork, MAX_LOADSTRING);
        SendMessage(hStatusBar, SB_SETTEXT, 4, (LPARAM)szWork);
        return 0;
    }

    case WM_SIZE:
        SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
        return 0;

    case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
        case ID_CONNECT:
        {   
            // Be connected to the camera
            HMENU hmenu = GetMenu(hWnd);
            EnableMenuItem(hmenu, ID_CONNECT, MF_BYCOMMAND | MF_DISABLED);
            if (CameraConnect())
            {
                EnableMenuItem(hmenu, ID_COOLER, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hmenu, ID_POWER, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hmenu, ID_EXPOSE, MF_BYCOMMAND | MF_ENABLED);
				
            }
			Initialize_OK = 1; 
			status_command = STATUS_COMMAND_OK;

			//printf("%d\n", Initialize_OK);
			write_status_to_file(STATUS_COMMAND_OK);
            EnableMenuItem(hmenu, ID_CONNECT, MF_BYCOMMAND | MF_ENABLED);
            DrawMenuBar(hWnd);
            break;
        }
        case ID_POWER:
            BitranCCDlibEnvironment(8, 8);
            BitranCCDlibSetCoolerPower(130);
            break;
        case ID_COOLER:
            BitranCCDlibSetTemperatue(-150); // in th unit of 100 milli deg (for example, -100 correspnds to -10 deg
			printf("Cooling\n");

            break;
        case ID_FILE_SAVE_AS:
            BitranCCDlibImageSave(2, NULL, pImageData);
            break;
        case ID_EXPOSE:
            if (!IsWindow(hwndGoto))
            {
                EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_DISABLED);
                
                hwndGoto = CreateDialog(hInst, MAKEINTRESOURCE(IDD_EXPOSE),
                                        hWnd, (DLGPROC)Expouse);
                ShowWindow(hwndGoto, SW_SHOW);
            }
            break;
        case IDD_EXPOSE:
            if (ExposeParam.repeat)
            {   // Because a camera might be broken if time stopping a fan is long, you must be careful
                int cycle;
                if(ExposeParam.fan)
                {
                    BitranCCDlibEnvironment(3, 3);
                    DWORD finish = GetTickCount() + ExposeParam.fan;

                    TCHAR szWork[20];
                    LoadString(hInst, IDS_FAN, szWork, 20);
                    SendMessage(hStatusBar, SB_SETTEXT, 5, (LPARAM)szWork);

                    MSG msg;
                    while (finish > GetTickCount())
                    {
                        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {       
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }

                    cycle = ContinueExposure(&ExposeParam, pImageData, hWnd, &hBitmap);
                    BitranCCDlibEnvironment(0, 1);
                }
                else
                {
                    cycle = ContinueExposure(&ExposeParam, pImageData, hWnd, &hBitmap);
                }

                TCHAR szWork[20];
                sprintf_s(szWork, _countof(szWork), _T("%ims"), cycle);
                SendMessage(hStatusBar, SB_SETTEXT, 5, (LPARAM)szWork);
            }
            else
            {   // Basic exposure example
				write_status_to_file(STATUS_COMMAND_NO);
				status_command = STATUS_COMMAND_NO;

                if (SnapExposure(&ExposeParam, pImageData, hInst, hStatusBar))
                {
					write_status_to_file(STATUS_COMMAND_OK);
					status_command = STATUS_COMMAND_OK;

					if (hBitmap)
                        DeleteObject(hBitmap);

                    hBitmap = BitranCCDlibImageConvert(NULL, pImageData);
                    InvalidateRect(hWnd, NULL, TRUE);
					//writeimage_fits(pImageData, fitsname);

                    EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED);
                }
            }

            uTickCount = 0;
            ShowWindow(GetDlgItem(hwndGoto, IDOK), SW_SHOW);
            ShowWindow(GetDlgItem(hwndGoto, IDC_STOP), SW_HIDE);
            break;
        case ID_IMAGE1:
            ImageMode = 50;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_IMAGE2:
            ImageMode = 100;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_IMAGE3:
            ImageMode = 200;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case ID_IMAGE4:
            ImageMode = -1;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
            	DestroyWindow(hWnd);
			break;
		case BUTTON_ID1:
			//MessageBox(hWnd, TEXT("BUTTON_ID1"), TEXT("Exposure"), MB_OK);
			KillTimer(hWnd, ID_MYTIMER);
			testID += 1;
			ExpButton = 1;
			receive_command_exposure(testID, hWnd, ExpButton);
			SetTimer(hWnd, ID_MYTIMER, TimerSec, NULL);
			ExpButton = 0;
			//writeimage();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	
    case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rt);
		//printf("hBitmap=%d\n", hBitmap); //only for test
		// TODO: Add any drawing code here...
        if (hBitmap)
        {
            DIBSECTION dib;
            GetObject(hBitmap, sizeof(DIBSECTION), &dib);

            HDC	hBmpDC = CreateCompatibleDC(NULL);
            HBITMAP	hBmpOld = (HBITMAP)SelectObject(hBmpDC, hBitmap);
            SetStretchBltMode(hdc, COLORONCOLOR);

            switch (ImageMode)
            {
            case 100:
                BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, hBmpDC, 0, 0, SRCCOPY);
                break;
            case 50:
                StretchBlt(hdc, 0, 0, dib.dsBm.bmWidth / 2, dib.dsBm.bmHeight / 2,
                    hBmpDC, 0, 0, dib.dsBm.bmWidth, dib.dsBm.bmHeight, SRCCOPY);
                break;
            case 200:
                StretchBlt(hdc, 0, 0, dib.dsBm.bmWidth * 2, dib.dsBm.bmHeight * 2,
                    hBmpDC, 0, 0, dib.dsBm.bmWidth, dib.dsBm.bmHeight, SRCCOPY);
                break;
            default:
                StretchBlt(hdc, rt.left, rt.top, rt.right, rt.bottom,
                           hBmpDC, 0, 0, dib.dsBm.bmWidth, dib.dsBm.bmHeight, SRCCOPY);
            }
            
            SelectObject(hBmpDC, hBmpOld);
            DeleteDC(hBmpDC);
        }
        else
        {
            DrawText(hdc, szHello, (int)_tcslen(szHello), &rt, DT_CENTER);
        }
        
        EndPaint(hWnd, &ps);
		break;
	
    case WM_DESTROY:
		PostQuitMessage(0);
		break;
	
	case WM_TIMER:
		KillTimer(hWnd, ID_MYTIMER);
		testID += 1;
		//Time_file_read_write_simple(testID);
		receive_command_exposure(testID, hWnd, ExpButton);
		//test_receive_command_exposure(testID, hWnd, ExpButton);

		SetTimer(hWnd, ID_MYTIMER,TimerSec, NULL);
		
		break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
	}
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

BOOL InitBitranCCDlib(LPTSTR lpCmdLine)
{
    char name[100];
    strcpy_s(name, sizeof(name), "BK50USBlib.dll");
    hBitranCCDlibDLL = LoadLibrary(name);
    if (hBitranCCDlibDLL == NULL)
    {
        strcat_s(name, sizeof(name), " is not found");
        MessageBox(NULL, name, NULL, MB_OK | MB_ICONSTOP);
        return FALSE;
    }
    
    BitranCCDlibCreate = (LPFNDLLFUNC1)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibCreate");
    BitranCCDlibDestroy = (LPFNDLLFUNC2)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibDestroy");
    BitranCCDlibCameraInfo = (LPFNDLLFUNC3)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibCameraInfo");
    BitranCCDlibGetVoltage = (LPFNDLLFUNC4)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibGetVoltage");
    BitranCCDlibSetCoolerPower = (LPFNDLLFUNC5)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibSetCoolerPower");
    BitranCCDlibGetCoolerPower = (LPFNDLLFUNC4)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibGetCoolerPower");
    BitranCCDlibSetTemperatue = (LPFNDLLFUNC6)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibSetTemperatue");
    BitranCCDlibGetTemperatue = (LPFNDLLFUNC7)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibGetTemperatue");
    BitranCCDlibEnvironment = (LPFNDLLFUNC8)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibEnvironment");
    BitranCCDlibStartExposure = (LPFNDLLFUNC9)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibStartExposure");
    BitranCCDlibContinueExposure = (LPFNDLLFUNC4)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibContinueExposure");
    BitranCCDlibCameraState = (LPFNDLLFUNC7)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibCameraState");
    BitranCCDlibAbortExposure = (LPFNDLLFUNC7)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibAbortExposure");
    BitranCCDlibFinishExposure = (LPFNDLLFUNC7)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibFinishExposure");
    BitranCCDlibTransferImage = (LPFNDLLFUNC11)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibTransferImage");
    BitranCCDlibImageInterpolation = (LPFNDLLFUNC12)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibImageInterpolation");
    BitranCCDlibImageConvert = (LPFNDLLFUNC13)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibImageConvert");
    BitranCCDlibImageSave = (LPFNDLLFUNC14)
        GetProcAddress(hBitranCCDlibDLL, "BitranCCDlibImageSave");

    return TRUE;
}

VOID ExitBitranCCDlib()
{
    if (pImageData)
        delete[] pImageData;
        
    BitranCCDlibDestroy();
    FreeLibrary(hBitranCCDlibDLL);
}

// The camera communicates
bool CameraConnect()
{
    int width, height;
    int result = BitranCCDlibCreate(NULL, &width, &height);
    if (result != 1)
        return false;

    char info[100];
    int lng = BitranCCDlibCameraInfo(1, info, sizeof(info));
    if (lng == 0)
        return false;
    
    strcpy_s(szHello, _countof(szHello), info);
    strcat_s(szHello, _countof(szHello), "\r\n");

    char* camera = strstr(info, "BK-5");
    if (camera == NULL)
        return false;

    info[strcspn(info, " \r\n")]=0;
    SendMessage(hStatusBar, SB_SETTEXT, SBT_POPOUT, (LPARAM)info);

    BitranCCDlibCameraInfo(0, info, sizeof(info));
    strcat_s(szHello, _countof(szHello), info);
    strcat_s(szHello, _countof(szHello), "\r\n");

    pImageData = new WORD[width * height];
    uTickCount = 0;
    InvalidateRect(NULL, NULL, TRUE);
    return true;
}

void CameraStatus()
{
    TCHAR szWork[MAX_LOADSTRING], szFormat[MAX_LOADSTRING];
    
    int ccd = BitranCCDlibGetTemperatue(0);
    LoadString(hInst, IDS_TEMP, szFormat, MAX_LOADSTRING);
	sprintf_s(szWork, MAX_LOADSTRING, _T("CCD Temp. %.1f C"), (double)ccd / 10.0);
//    sprintf_s(szWork, MAX_LOADSTRING, szFormat, (double)ccd / 10.0);
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)szWork);
	CCDtemp = (float)ccd / 10.0;


    int body = BitranCCDlibGetTemperatue(1);
    LoadString(hInst, IDS_TEMP, szFormat, MAX_LOADSTRING);
	sprintf_s(szWork, MAX_LOADSTRING, _T("Body Temp. %.1f C"), (double)body / 10.0);
//    sprintf_s(szWork, MAX_LOADSTRING, szFormat, (double)body / 10.0);
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);
	Bodytemp = (float)body / 10.0;

    int power = BitranCCDlibGetCoolerPower();
    sprintf_s(szWork, MAX_LOADSTRING, _T("Cooling Power %i W"), power);
    SendMessage(hStatusBar, SB_SETTEXT, 3, (LPARAM)szWork);

    int volt = BitranCCDlibGetVoltage();
    sprintf_s(szWork, MAX_LOADSTRING, _T("%i.%i V"), volt / 10, volt % 10);
    SendMessage(hStatusBar, SB_SETTEXT, 4, (LPARAM)szWork);

    SendMessage(hStatusBar, SB_SETBKCOLOR, 0, 
        ((ccd == ILLEGAL_VALUE) || (body == ILLEGAL_VALUE) ||
         (power == ILLEGAL_VALUE) || (volt == ILLEGAL_VALUE) || (volt < 100)) ?
            (LPARAM)RGB(255, 0, 0) : CLR_DEFAULT);
}

// A dialogue to expose
LRESULT CALLBACK Expouse(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {       
        SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_SETRANGE32, 100, 64800000);
        SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_SETPOS, 0, 100);

        LPCTSTR strItem[] = { TEXT("1x1"),
            TEXT("2x2"),
            TEXT("3x3"),
            TEXT("4x4"),
            TEXT("8x8"),
            TEXT("16x16") };
        for (int i = 0; i < _countof(strItem); i++)
            SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)strItem[i]);
        SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, 0, 0);

        CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
        
        TCHAR str[10];
        for (int i = 1; i <= 10; i++)
        {
            _itot_s(i, str, _countof(str), 10);
            SendDlgItemMessage(hDlg, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM)str);
        }
        SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETCURSEL, 2, 0);

        ShowWindow(hDlg, SW_SHOW);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_STOP), SW_SHOW);
            EnableWindow(GetDlgItem(hDlg, IDC_STOP), TRUE);

            // Exposure time (ms)
            ExposeParam.time = GetDlgItemInt(hDlg, IDC_EDIT1, NULL, FALSE);
            
            // Horizontal & Vertical binnig
            TCHAR str[10];
            GetWindowText(GetDlgItem(hDlg, IDC_COMBO1), str, _countof(str));
            LPTSTR ptr;
            ExposeParam.binX = _tcstol(str, &ptr, 10);
            ExposeParam.binY = _tcstol(&ptr[1], &ptr, 10);
            
            // Center area(full=0,256=1,512=2)
            ExposeParam.center = 0;
            for (UINT id = IDC_CHECK1; id <= IDC_CHECK2; id++)
                if (IsDlgButtonChecked(hDlg, id) == BST_CHECKED)
                    ExposeParam.center = id - IDC_CHECK1 + 1;

            // A/D mode(1.3MHz=0,250KHz=1)
            ExposeParam.mode = IsDlgButtonChecked(hDlg, IDC_CHECK3);

            // Trigger mode(not used=0,sync=1,exposure=2)
            ExposeParam.trigger = IsDlgButtonChecked(hDlg, IDC_RADIO1) ? 0 :
                                  IsDlgButtonChecked(hDlg, IDC_RADIO2) ? 2 : 3;
            
            // Continue or One shot
            ExposeParam.repeat = IsDlgButtonChecked(hDlg, IDC_CHECK4) == BST_CHECKED;
            
            // Light frame or Dark frame
            ExposeParam.dark = IsDlgButtonChecked(hDlg, IDC_CHECK6);
            
            // Fan mode(ON=0,Delay time(ms)>0)
            ExposeParam.fan = (IsDlgButtonChecked(hDlg, IDC_CHECK5) == BST_UNCHECKED) ? 0 :
                ((int)SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0) + 1) * 1000;

            ExposeParam.abort = false;
            uTickCount = MAXLONGLONG;
            PostMessage(GetParent(hDlg), WM_COMMAND, IDD_EXPOSE, 0);
            break;
        } 

        case IDC_STOP:
            ExposeParam.abort = true;
            EnableWindow(GetDlgItem(hDlg, IDC_STOP), FALSE);
            BitranCCDlibAbortExposure(-5);
            break; 

        case IDCANCEL:
            DestroyWindow(hDlg);
            return TRUE;

        default:
            int wmId = LOWORD(wParam);
            if ((wmId >= IDC_CHECK1) && (wmId <= IDC_CHECK2))
                if (IsDlgButtonChecked(hDlg, wmId) == BST_CHECKED)
                    CheckRadioButton(hDlg, IDC_CHECK1, IDC_CHECK2, LOWORD(wParam));
        }
    }
    return FALSE;
}


int Time_file_read_write_simple(int testvar)
{
	TCHAR szWork[MAX_LOADSTRING], szFormat[MAX_LOADSTRING];
	ULARGE_INTEGER ui;
	int ccd = 1;
	DWORD nEnd = ::GetTickCount();
	//	sprintf_s(szWork, _countof(szWork), _T("%ims"), nEnd);
	//	SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)szWork);
	FILE *fp; // FILE structure
	char fname[] = "C:/cygwin64/home/bitran/data/test.dat";
	char fname_out[] = "C:/cygwin64/home/bitran/data/status.dat";

	char str[10];
	float ExpTime, f2, f3, f4, f5;
	//@@@@@@@@@@@ get timestamp @@@@@@@@@@@@@@@@@@@@@
	FILETIME   ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stCreate, stAccess, stWrite;

	HANDLE file = CreateFile(fname, GENERIC_READ,
		0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("NO\n");
		return(0);
	}
	GetFileTime(file, &ftCreate, &ftAccess, &ftWrite);
	FileTimeToSystemTime(&ftCreate, &stCreate);
	FileTimeToSystemTime(&ftAccess, &stAccess);
	FileTimeToSystemTime(&ftWrite, &stWrite);
	CloseHandle(file);


	ui.HighPart = ftWrite.dwHighDateTime;
	ui.LowPart = ftWrite.dwLowDateTime;
	// if file is new, show "YES!" to status bar
	if (ui.QuadPart > ui_previous.QuadPart) {
		
		// read file to get CCD read parameter
		if (fopen_s(&fp, fname, "r") != 0) { // file open
											 //		if (fp == NULL) {
			printf("%s file not open!\n", fname);
			return -1;
		}

		fscanf_s(fp, "%s %f %f %f %f %f", str, 10, &ExpTime, &f2, &f3, &f4, &f5);
		printf("%s %.1f %.1f %.1f %.1f %.1f\n", str, ExpTime, f2, f3, f4, f5);
		fclose(fp); // 

		if (Initialize_OK == 1) {
			strcpy_s(szWork, MAX_LOADSTRING, "Init OK!"); // when showing string
			SendMessage(hStatusBar, SB_SETTEXT, 5, (LPARAM)szWork);
			printf("%d\n", Initialize_OK);
		}
		else if(Initialize_OK == 0){
			strcpy_s(szWork, MAX_LOADSTRING, "No Init!"); // when showing string
			SendMessage(hStatusBar, SB_SETTEXT, 5, (LPARAM)szWork);
			printf("%d\n", Initialize_OK);

		}

		strcpy_s(szWork, MAX_LOADSTRING, str); // when showing string
		SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);

		sprintf_s(szWork, _countof(szWork), _T("Exposure time: %f sec"), ExpTime);  // when showing numerical variable 
		SendMessage(hStatusBar, SB_SETTEXT, 3, (LPARAM)szWork);


		// write a current status to a file
		write_status_to_file(STATUS_COMMAND_OK);

		/*
		if (fopen_s(&fp, fname_out, "w") != 0) { // 
												 //		if (fp == NULL) {
			printf("%s file not open!\n", fname);
			return -1;
		}

		fprintf(fp, "%d %f %f\n", 1, DeltaX, DeltaY); //
		fclose(fp); // 
		*/

		strcpy_s(szWork, MAX_LOADSTRING, "YES!!");
		SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);
		ui_previous = ui;
	}
	else {
		strcpy_s(szWork, MAX_LOADSTRING, "NO!!");
		SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);

	}
	/*
	printf("%d.%02d.%02d %2d:%02d:%02d\n", stCreate.wYear,
	stCreate.wMonth, stCreate.wDay, stCreate.wHour, stCreate.wMinute, stCreate.wSecond);
	printf("%d.%02d.%02d %2d:%02d:%02d\n", stAccess.wYear,
	stAccess.wMonth, stAccess.wDay, stAccess.wHour, stAccess.wMinute, stAccess.wSecond);
	printf("%d.%02d.%02d %2d:%02d:%02d\n", stWrite.wYear,
	stWrite.wMonth, stWrite.wDay, stWrite.wHour, stWrite.wMinute, stWrite.wSecond);
	*/
	//	sprintf_s(szWork, _countof(szWork), _T("%i sec"), Write.wSecond);
	//	sprintf_s(szWork, _countof(szWork), _T("%i sec"), ui.QuadPart/10000000);
	sprintf_s(szWork, _countof(szWork), _T("%i sec"), testvar);

	//sprintf_s(szWork, _countof(szWork), _T("%ims"), nEnd);
	SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)szWork);

	return(0);
}



int receive_command_exposure(int testvar, HWND hWnd, int ExpButton)
{
	TCHAR szWork[MAX_LOADSTRING], szFormat[MAX_LOADSTRING];
	ULARGE_INTEGER ui;
	int ccd = 1;
	DWORD nEnd = ::GetTickCount();
	//	sprintf_s(szWork, _countof(szWork), _T("%ims"), nEnd);
	//	SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)szWork);
	FILE *fp; // FILE structure
	char fname[] = "C:/cygwin64/home/bitran/data/command.dat";
	char fname_out[] = "C:/cygwin64/home/bitran/data/status.dat";

	//char dummy_filename[] = "C:/cygwin64/home/bitran/data/FIM_top_dynamic_1um_5sec_stack.fits"; //fiber baseline image
	//char dummy_filename[] = "C:/cygwin64/home/bitran/data/bitran20181021-3.fits"; // for test, slightly offset from the fiber, 1sec, dark = bitran20181022-3-stack
	//char dummy_filename[] = "C:/cygwin64/home/bitran/data/bitran20181224-15-1.fits"; // for test, trappist-1, 180sec, dark = bitran20181224-19-stack 
	//char dummy_filename[] = "C:/cygwin64/home/bitran/data/bitran20180820-b15-1.fits"; // for test, K2-25, 60sec, dark = bitran20180820-7-stack
	char dummy_filename[] = "C:/cygwin64/home/bitran/data/bitran20181022-15-1.fits"; // for test, GJ699 center, 1sec, dark = bitran20181022-3-stack


	//char dummy_filename[] = "C:/cygwin64/home/bitran/data/Center_test.fits";
	//char dummy_filename[] = "C:/cygwin64/home/bitran/data/XYoffset_test.fits";

	//char dummy_filename[] = "C:/cygwin64/home/bitran/data/bitran90-30-1.fits"; // for test, off fiber, 0.1sec
	

	const long Npix = 1024;
	fitsfile *fptr;
	char fitsname[100], filename_dark[100], Dirfilename_dark[100], filename_average[100], TargetName[100];
	int  command_num, ExpTime, Nframe, Nloop, Binning,num,num_loop, Period;
	//char dirname[] = "C:/cygwin64/home/bitran/data/";  //FIM image directory, until 2019/07/18

	char filename[100];
//	unsigned short *array_average; old
	//double *array_average_f[Npix]; old
	//unsigned short *array_average[Npix];
	double *array_average_f, exposure;
	long  fpixel, nelements;
	float PallacticAngle=0.0;
	long naxis = 2;  /* 2-dimensional image                            */
	int status;
	long naxes[2] = { Npix, Npix };   /* image is 300 pixels wide by 200 rows */

	int ii, jj;
	long fpix[2] = { 1, 1 };
	int anynul, anynul2;
	char buff[128] = "", buff2[128] = "";


	nelements = naxes[0] * naxes[1];          /* number of pixels to write */

    // initialize array
	//array_average = (unsigned short *)malloc(Npix * Npix * sizeof(unsigned short)); old
	//array_average_f[0] = (double *)malloc(naxes[0] * naxes[1] * sizeof(double)); old
	//array_average_f = (double**) malloc(sizeof(double *) * naxes[0]); 
	//for (ii = 1; ii < naxes[1]; ii++) {
	//	array_average_f[ii] = (double*) malloc(sizeof(double) * naxes[1]);
	//}
	//for (ii = 1; ii < naxes[1]; ii++) ol
//		array_average_f[ii] = array_average_f[ii - 1] + naxes[0]; old

	fpixel = 1;                               /* first pixel to write      */

	//for (ii = 1; ii<naxes[1]; ii++)
	//	array_average[ii] = array_average[ii - 1] + naxes[0];
	/*
	for (jj = 0; jj < Npix; jj++)
	{
		for (ii = 0; ii < Npix; ii++)
		{
			//array_average[ii + (jj) * 1024] = 0;
			array_average_f[ii+(jj)*(Npix)] = 0.0;

		}
	}
	*/
	//char filename_dark[] = "C:/cygwin64/home/bitran/data/bitran20181021-1";

	//@@@@@@@@@@@ get timestamp @@@@@@@@@@@@@@@@@@@@@
	FILETIME   ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stCreate, stAccess, stWrite;

	HANDLE file = CreateFile(fname, GENERIC_READ,
		0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("NO\n");
		return(0);
	}
	GetFileTime(file, &ftCreate, &ftAccess, &ftWrite);
	FileTimeToSystemTime(&ftCreate, &stCreate);
	FileTimeToSystemTime(&ftAccess, &stAccess);
	FileTimeToSystemTime(&ftWrite, &stWrite);
	CloseHandle(file);


	ui.HighPart = ftWrite.dwHighDateTime;
	ui.LowPart = ftWrite.dwLowDateTime;
	// if file is new, show "YES!" to status bar
	if ((ui.QuadPart > ui_previous.QuadPart) | (ExpButton == 1)) {
		CommandCounter += 1;
		// read file to get CCD read parameter
		if (fopen_s(&fp, fname, "r") != 0) { // file open
											 //		if (fp == NULL) {
			printf("%s file not open!\n", fname);
			return -1;
		}

//		fscanf_s(fp, "%d %s %s %d %d %d %d %d", &command_num, fitsname, 100, TargetName, 100, &ExpTime, &Nframe, &Nloop, &Binning, &Period); //in case of space delimiter
		fscanf_s(fp, "%d, %[^,], %[^,], %d, %d, %d, %d, %d, %f", &command_num, fitsname, 100, TargetName, 100, &ExpTime, &Nframe, &Nloop, &Binning, &Period, &PallacticAngle); // in case of camma delimiter

		printf("Input file parameters: %d %s %s %d %d %d %d %d %f\n", command_num, fitsname, TargetName, ExpTime, Nframe, Nloop, Binning, Period, PallacticAngle);
		fclose(fp); // 

		// write command data to log file ///////////////////////////////////////
		if (fopen_s(&fp, fname_log, "a") != 0) { // file open
											 //		if (fp == NULL) {
			printf("%s file not open!\n", fname_log);
			return -1;
		}
		time_t now = time(NULL);
		struct tm pnow;

		localtime_s(&pnow, &now);
		sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
		sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
		fprintf(fp, "%s %s Command file: %d %s %s %d %d %d %d %d\n", buff, buff2, command_num, fitsname, TargetName, ExpTime, Nframe, Nloop, Binning, Period);
		fclose(fp); // 
		// end write command data to log file //////////////////////////////////////////


		// Command number, Exptime (msec), Number of Frame, number of frame, binning
		//sprintf_s(Dirfilename_dark, 100,"%s%s", dirname, filename_dark);
		sprintf_s(szWork, _countof(szWork), _T("Exp. time: %.1f sec"), float(ExpTime) / 1000.0);  // when showing numerical variable 
		SendMessage(hStatusBar, SB_SETTEXT, 6, (LPARAM)szWork);

		if (command_num == 2) { //unload TT
			calc_output_TT_voltage(command_num);
			system("C:\\cygwin64\\home\\bitran\\data\\TTcontrol_simple_file.exe");
		}
		else {
			if (Initialize_OK == 1 && status_command == STATUS_COMMAND_OK) {
				strcpy_s(szWork, MAX_LOADSTRING, "Init OK!"); // when showing string
				SendMessage(hStatusBar, SB_SETTEXT, 5, (LPARAM)szWork);
				//printf("%d\n", Initialize_OK);
				array_average_f = new double[Npix*Npix]();

				//@@@@@@@@@@@@ expousre parameter setting @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@//
				// Exposure time (ms)
				ExposeParam.time = abs(ExpTime);

				// Horizontal & Vertical binnig
	//			TCHAR str[10];
		//		GetWindowText(GetDlgItem(hDlg, IDC_COMBO1), str, _countof(str));
			//	LPTSTR ptr;
				ExposeParam.binX = Binning;// _tcstol(str, &ptr, 10);
				ExposeParam.binY = Binning;// _tcstol(&ptr[1], &ptr, 10);

				// Center area(full=0,256=1,512=2)
				ExposeParam.center = 0;
				//for (UINT id = IDC_CHECK1; id <= IDC_CHECK2; id++)
				//	if (IsDlgButtonChecked(hDlg, id) == BST_CHECKED)
			//			ExposeParam.center = id - IDC_CHECK1 + 1;

				// A/D mode(1.3MHz=0,250KHz=1)
				ExposeParam.mode = 1;// IsDlgButtonChecked(hDlg, IDC_CHECK3);

				// Trigger mode(not used=0,sync=1,exposure=2)
				ExposeParam.trigger = 0;// IsDlgButtonChecked(hDlg, IDC_RADIO1) ? 0 :
		//			IsDlgButtonChecked(hDlg, IDC_RADIO2) ? 2 : 3;

				// Continue or One shot
				ExposeParam.repeat = 0;// IsDlgButtonChecked(hDlg, IDC_CHECK4) == BST_CHECKED;

				// Light frame or Dark frame
				ExposeParam.dark = 0;// IsDlgButtonChecked(hDlg, IDC_CHECK6);

				// Fan mode(ON=0,Delay time(ms)>0)
				ExposeParam.fan = 0;// (IsDlgButtonChecked(hDlg, IDC_CHECK5) == BST_UNCHECKED) ? 0 :
		//			((int)SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0) + 1) * 1000;

				//@@@@@@@@@@@@@ exposure @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				//for (num_loop = 1; num_loop <= Nloop; num_loop++) {
				write_status_to_file(STATUS_COMMAND_NO);
				status_command = STATUS_COMMAND_NO;
				if (Nframe >= 30) Nframe = 30;
				if (Nframe < 1) Nframe = 1;
				for (num = 1; num <= Nframe; num++) {
					printf("Frame = %d/%d\n", num, Nframe);

					sprintf_s(szWork, _countof(szWork), _T("Frame : %d/%d"), num, Nframe);  // when showing numerical variable 
					SendMessage(hStatusBar, SB_SETTEXT, 7, (LPARAM)szWork);


					// Tip-tilt correction, after 2019/02/25
						//if (command_num == 1) {  // for old mode
					if ((command_num == 1) & (Nframe == 1)) { //comment out this for old mode
							calc_output_TT_voltage(command_num);
							system("C:\\cygwin64\\home\\bitran\\data\\TTcontrol_simple_file.exe");
						}
						
					


						if (SnapExposure(&ExposeParam, pImageData, hInst, hStatusBar))
						{
							
							if (hBitmap)
								DeleteObject(hBitmap);

							hBitmap = BitranCCDlibImageConvert(NULL, pImageData);
							ImageMode = 50;
							InvalidateRect(hWnd, NULL, TRUE);
//							snprintf(filename, 200, "%s%s-%d-%d", dirdataname, fitsname, num, num_loop);
							snprintf(filename, 200, "%s%s-%d", dirdataname, fitsname, num);

							printf("Target fits file name = %s\n", filename);
							printf("Target name = %s\n", TargetName);

					
							//******* read dummy data file for software debugging test ***************/
							/*
							if (fits_open_file(&fptr, dummy_filename, READONLY, &status))
								printerror(status);
							printf("Dummy data file open OK\n");
							if (fits_read_pix(fptr, TUSHORT, fpix, nelements, 0, pImageData, &anynul, &status))
								printerror(status);
							printf("Dummy data file open OK\n");
							if (fits_close_file(fptr, &status))
								printerror(status);
							printf("Dark file close OK\n");
							*/
							//*****************************************/

							writeimage_fits(pImageData, filename, ExpTime, TargetName);
							printf("Exptime = %.1f\n", double(ExpTime)/1000.0);
							calc_CenterOfGravity(pImageData, filename, TargetName, ExpTime, hWnd);
							// comment out for old mode
							if ((command_num == 1) && (Nframe > 1)) {
								calc_output_TT_voltage(command_num);
								system("C:\\cygwin64\\home\\bitran\\data\\TTcontrol_simple_file.exe");
							}
							// end comment out for old mode

							// Add delay for loop
							if (abs(Period) > 500) Period = 500;
							if (Period != 1) Sleep(abs(Period) * 1000);

							/* // Tip-tilt correction, until 2019/02/25
							if (command_num == 1) {
								calc_output_TT_voltage(command_num);
								system("C:\\cygwin64\\home\\bitran\\data\\TTcontrol_simple_file.exe");
							}
							*/
							// average data
								for (jj = 0; jj < Npix; jj++)
								{
									for (ii = 0; ii < Npix; ii++)
									{
										if (num == 1) {
											array_average_f[ii + jj * Npix] = double(pImageData[ii + (jj) * 1024]);
										}
										else {
											array_average_f[ii + jj * Npix] += double(pImageData[ii + (jj) * 1024]);
										}
										/* find maximum position
										if (array_average_f[ii + jj * Npix] > maxIntensityL) {
											maxIntensityL = array_average_f[ii + jj * Npix];
											MaxX = ii+1; MaxY = jj+1;
										}
										*/

									}
								}
								write_status_to_log(STATUS_COMMAND_OK, fitsname,num);

							EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED);
						}
					
					//	*array_average /= Nframe;
						

					}
					write_status_to_file(STATUS_COMMAND_OK);
					status_command = STATUS_COMMAND_OK;

					//write averaged data to file
					
					for (jj = 0; jj < Npix; jj++)
					{
						for (ii = 0; ii < Npix; ii++)
						{
//							array_average_f[jj][ii] /= double(Nframe);
							array_average_f[ii+jj*Npix] /= double(Nframe);

//							array_average_f[jj][ii] = double(array_average[ii + (jj) * 1024]) / double(Nframe);

						//	array_average[ii + (jj) * 1024] /= Nframe;

						}
					}
					
					snprintf(filename_average, 200, "%s%s-avg.fits", dirdataname, fitsname);
					//writeimage_fits(array_average, filename_average, ExpTime);
					//writeimage_fits_f(array_average_f, filename_average, ExpTime);
					status = 0;
					//@@@@@@ WRITE average image to fits file @@@@@@@@@@
					printf("Remove file : %d\n", remove(filename_average));
					if (Nframe != 1) {
						exposure = ExpTime / 1000.0; //convert msec to sec

						if (fits_create_file(&fptr, filename_average, &status)) /* create new FITS file */
							printerror(status);           /* call printerror if error occurs */
						printf("File read 5 OK\n");

						if (fits_create_img(fptr, DOUBLE_IMG, naxis, naxes, &status))
							printerror(status);
						//				printf("File read 6 OK\n");

						if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, array_average_f, &status))
							printerror(status);
						//				printf("File read 7 OK\n");
						if (fits_update_key(fptr, TDOUBLE, "EXPOSURE", &exposure, "Averaged Exposure Time sec", &status))
							printerror(status);
						if (fits_update_key(fptr, TINT, "NFRAME", &Nframe, "Total number of frames", &status))
							printerror(status);
						if (fits_update_key(fptr, TSTRING, "TARGET", TargetName, NULL, &status))
							printerror(status);
						

						
						//*array_average_f = 0.0;
						if (fits_close_file(fptr, &status))                /* close the file */
							printerror(status);

						printf("Average out OK\n");
					}
					//free(array_average_f);
					//free(array_average[0]);
				//}
					delete[] array_average_f;

			}
			else if (Initialize_OK == 0) {
				strcpy_s(szWork, MAX_LOADSTRING, "No Init!"); // when showing string
				SendMessage(hStatusBar, SB_SETTEXT, 5, (LPARAM)szWork);
				//printf("%d\n", Initialize_OK);

			}
		}
		//strcpy_s(szWork, MAX_LOADSTRING, str); // when showing string
		//SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);

		

		// write a current status to a file
		write_status_to_file(STATUS_COMMAND_OK);
/*
		if (fopen_s(&fp, fname_out, "w") != 0) { // 
												 //		if (fp == NULL) {
			printf("%s file not open!\n", fname);
			return -1;
		}

		fprintf(fp, "%d %f %f\n", 1, DeltaX, DeltaY); //
		fclose(fp); //
		*/
		printf("Finished command %d @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n", CommandCounter);


//		strcpy_s(szWork, MAX_LOADSTRING, "YES!!");
	//	SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);
		ui_previous = ui;
	}
	else {
		//strcpy_s(szWork, MAX_LOADSTRING, "NO!!");
		//SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)szWork);

	}
	/*
	printf("%d.%02d.%02d %2d:%02d:%02d\n", stCreate.wYear,
	stCreate.wMonth, stCreate.wDay, stCreate.wHour, stCreate.wMinute, stCreate.wSecond);
	printf("%d.%02d.%02d %2d:%02d:%02d\n", stAccess.wYear,
	stAccess.wMonth, stAccess.wDay, stAccess.wHour, stAccess.wMinute, stAccess.wSecond);
	printf("%d.%02d.%02d %2d:%02d:%02d\n", stWrite.wYear,
	stWrite.wMonth, stWrite.wDay, stWrite.wHour, stWrite.wMinute, stWrite.wSecond);
	*/
	//	sprintf_s(szWork, _countof(szWork), _T("%i sec"), Write.wSecond);
	//	sprintf_s(szWork, _countof(szWork), _T("%i sec"), ui.QuadPart/10000000);
	sprintf_s(szWork, _countof(szWork), _T("%.1f sec"), (double)testvar / 2.0);

	//sprintf_s(szWork, _countof(szWork), _T("%ims"), nEnd);
	//SendMessage(hStatusBar, SB_SETTEXT, 6, (LPARAM)szWork);
	return(0);
}

void writeimage(void)

/******************************************************/
/* Create a FITS primary array containing a 2-D image */
/******************************************************/
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status, ii, jj;
	long  fpixel, nelements, exposure;
	unsigned short *array[200];
	int anynul;
	/* initialize FITS image parameters */
	char filename[] = "C:/cygwin64/home/bitran/data/atestfil.fits";             /* name for new FITS file */
	int bitpix = USHORT_IMG; /* 16-bit unsigned short pixel values       */
	long naxis = 2;  /* 2-dimensional image                            */
	long naxes[2] = { 300, 200 };   /* image is 300 pixels wide by 200 rows */

									/* allocate memory for the whole image */
	array[0] = (unsigned short *)malloc(naxes[0] * naxes[1]
		* sizeof(unsigned short));

	/* initialize pointers to the start of each row of the image */
	for (ii = 1; ii<naxes[1]; ii++)
		array[ii] = array[ii - 1] + naxes[0];

	remove(filename);               /* Delete old file if it already exists */

	status = 0;         /* initialize status before calling fitsio routines */


	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	fits_open_file(&fptr, filename, READONLY, &status);
//	fits_read_pix(fptr, TUSHORT, &fpixel, nelements, 0, array[0], &anynul, &status);
//	fits_close_file(fptr, &status);

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	if (fits_create_file(&fptr, filename, &status)) /* create new FITS file */
		printerror(status);           /* call printerror if error occurs */

									  /* write the required keywords for the primary array image.     */
									  /* Since bitpix = USHORT_IMG, this will cause cfitsio to create */
									  /* a FITS image with BITPIX = 16 (signed short integers) with   */
									  /* BSCALE = 1.0 and BZERO = 32768.  This is the convention that */
									  /* FITS uses to store unsigned integers.  Note that the BSCALE  */
									  /* and BZERO keywords will be automatically written by cfitsio  */
									  /* in this case.                                                */

	if (fits_create_img(fptr, bitpix, naxis, naxes, &status))
		printerror(status);

	/* initialize the values in the image with a linear ramp function */
	for (jj = 0; jj < naxes[1]; jj++)
	{
		for (ii = 0; ii < naxes[0]; ii++)
		{
			array[jj][ii] = ii + jj;
		}
	}

	fpixel = 1;                               /* first pixel to write      */
	nelements = naxes[0] * naxes[1];          /* number of pixels to write */

											  /* write the array of unsigned integers to the FITS file */
	if (fits_write_img(fptr, TUSHORT, fpixel, nelements, array[0], &status))
		printerror(status);

//	if (fits_read_img(fptr, TUSHORT, fpixel, nelements, array[0], &status))
//		printerror(status);
//	fits_write_pix(fptr, TUSHORT, fpixel,nelements, void *array, int *status);
	
	
	

	free(array[0]);  /* free previously allocated memory */

					 /* write another optional keyword to the header */
					 /* Note that the ADDRESS of the value is passed in the routine */
	exposure = 1500;
	if (fits_update_key(fptr, TLONG, "EXPOSURE", &exposure,
		"Total Exposure Time", &status))
		printerror(status);

	if (fits_close_file(fptr, &status))                /* close the file */
		printerror(status);

	return;
}


void writeimage_fits(LPWORD pImageData, const char *fitsfilename, int Exptime, char *TargetName)

/******************************************************/
/* Create a FITS primary array containing a 2-D image */
/******************************************************/
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status, ii, jj;
	long  fpixel, nelements, wcsdim;
	double CRVAL1, CRVAL2, CRPIX1, CRPIX2, CD1_1, CD1_2, CD2_1, CD2_2, LTM1, LTM2, exposure, bzero,bscale, equinox;
	//unsigned short *array[2000];
	char ctype1[] = "RA---TAN";
	char ctype2[] = "DEC--TAN";
	char WAT0[] = "system=image";
	char WAT1[] = "wtype=tan axtype=ra";
	char WAT2[] = "wtype=tan axtype=dec";
	char radecsys[] = "FK5";
	char fitsfilenameP[100];
	char buff[128]="", buff2[128] = "";

	printf("--------------- Write CCD image data to fits file ---------------\n");
	snprintf(fitsfilenameP, 200, "%s.fits", fitsfilename);

	/* initialize FITS image parameters */
	//const char *dirname = "C:/cygwin64/home/bitran/data/";
//	const char fname[3];              /* name for new FITS file */
	//char filename[100];
	int bitpix = USHORT_IMG; /* 16-bit unsigned short pixel values    */   
	long naxis = 2;  /* 2-dimensional image                            */
	long naxes[2] = {1024, 1024 };   /* image is 300 pixels wide by 200 rows */

									/* allocate memory for the whole image */
	//array[0] = (unsigned short *)malloc(naxes[0] * naxes[1]* sizeof(unsigned short));

	//snprintf(filename, 200, "%s%s", dirname, fitsname); 
	//printf("fits file name = %s\n", filename);
	//printf("fits name = %s\n", fitsname);

	/* initialize pointers to the start of each row of the image */
	//for (ii = 1; ii<naxes[1]; ii++)
	//	array[ii] = array[ii - 1] + naxes[0];

	remove(fitsfilenameP);               /* Delete old file if it already exists */

	status = 0;         /* initialize status before calling fitsio routines */


	if (fits_create_file(&fptr, fitsfilenameP, &status)) /* create new FITS file */
		printerror(status);           /* call printerror if error occurs */

									  /* write the required keywords for the primary array image.     */
									  /* Since bitpix = USHORT_IMG, this will cause cfitsio to create */
									  /* a FITS image with BITPIX = 16 (signed short integers) with   */
									  /* BSCALE = 1.0 and BZERO = 32768.  This is the convention that */
									  /* FITS uses to store unsigned integers.  Note that the BSCALE  */
									  /* and BZERO keywords will be automatically written by cfitsio  */
									  /* in this case.                                                */

	if (fits_create_img(fptr, bitpix, naxis, naxes, &status))
		printerror(status);

	

	fpixel = 1;                               /* first pixel to write      */
	nelements = naxes[0] * naxes[1];          /* number of pixels to write */

											  /* write the array of unsigned integers to the FITS file */
	//if (fits_write_img(fptr, TUSHORT, fpixel, nelements, array[0], &status))
	//pImageData[1024]= 1000;
		if (fits_write_img(fptr, TUSHORT, fpixel, nelements, pImageData, &status))

		printerror(status);

	

					 /* write another optional keyword to the header */
					 /* Note that the ADDRESS of the value is passed in the routine */
	
	exposure = Exptime/1000.0; //convert msec to sec
	if (fits_update_key(fptr, TDOUBLE, "EXPOSURE", &exposure,
		"Total Exposure Time", &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "TARGET", TargetName, NULL, &status))
		printerror(status);
	printf("Target name (fits header) = %s\n", TargetName);

	if (fits_update_key(fptr, TINT, "CENX0", &cenX0, "Center of cropped image X [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TINT, "CENY0", &cenY0, "Center of cropped image Y [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENXMF", &CenxMf, "Fiber position X in crooped image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENYMF", &CenyMf, "Fiber position Y in cropped image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENXMFFU", &CenxMf_full, "Fiber position X in full image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENYMFFU", &CenyMf_full, "Fiber position Y in full image [pixel]", &status))
		printerror(status);

	time_t now = time(NULL);
	struct tm pnow;

	localtime_s(&pnow, &now);

	sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
	sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
	printf("Current date and HST=%s %s\n", buff, buff2);


	if (fits_update_key(fptr, TSTRING, "DATE", buff, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "HST", buff2, NULL, &status))
		printerror(status);

	printf("Target name (fits header) = %s\n", TargetName);
	// Add WCS information
	
//	bzero = 0.0;
	//if (fits_update_key(fptr, TDOUBLE, "BZERO", &bzero,NULL, &status))
		//printerror(status);	
	bscale = 1.0;
	if (fits_update_key(fptr, TDOUBLE, "BSCALE", &bscale, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "RADECSYS", &radecsys, NULL, &status))
		printerror(status);
	equinox = 2000.0;
	if (fits_update_key(fptr, TDOUBLE, "EQUINOX", &equinox, NULL, &status))
		printerror(status);
		
	/*
	wcsdim = 2;
	if (fits_update_key(fptr, TLONG, "WCSDIM", &wcsdim,NULL, &status))
		printerror(status);	
	if (fits_update_key(fptr, TSTRING, "CTYPE1", ctype1, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "CTYPE2", ctype2, NULL, &status))
		printerror(status);
	CRVAL1 = 229.638765698445;
	if (fits_update_key(fptr, TDOUBLE, "CRVAL1", &CRVAL1, NULL, &status))
		printerror(status);
	CRVAL2 = 2.08446180962833;
	if (fits_update_key(fptr, TDOUBLE, "CRVAL2", &CRVAL2, NULL, &status))
		printerror(status);
	CRPIX1 = 420.06868005863;
	if (fits_update_key(fptr, TDOUBLE, "CRPIX1", &CRPIX1, NULL, &status))
		printerror(status);
	CRPIX2 = 484.022188319007;
	if (fits_update_key(fptr, TDOUBLE, "CRPIX2", &CRPIX2, NULL, &status))
		printerror(status);
	CD1_1 = 2.83296121476586E-7;
	if (fits_update_key(fptr, TDOUBLE, "CD1_1", &CD1_1, NULL, &status))
		printerror(status);
	CD1_2 = -1.8455997451409E-5;
	if (fits_update_key(fptr, TDOUBLE, "CD1_2", &CD1_2, NULL, &status))
		printerror(status);
	CD2_1 = -1.8764142548187E-5;
	if (fits_update_key(fptr, TDOUBLE, "CD2_1", &CD2_1, NULL, &status))
		printerror(status);
	CD2_2 = -2.4701713670739E-7;
	if (fits_update_key(fptr, TDOUBLE, "CD2_2", &CD2_2, NULL, &status))
		printerror(status);
	LTM1 = 1.0;
	if (fits_update_key(fptr, TDOUBLE, "LTM1_1", &LTM1, NULL, &status))
		printerror(status);
	LTM2 = 2.0;
	if (fits_update_key(fptr, TDOUBLE, "LTM2_2", &LTM2, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "WAT0_001", WAT0, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "WAT1_001", WAT1, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "WAT2_001", WAT2, NULL, &status))
		printerror(status);
		*/

	if (fits_close_file(fptr, &status))                // close the file 
		printerror(status);
		
	printf("Fits write OK\n");
	printf("--------------- End Write CCD image data to fits file ---------------\n");
	//free(array[0]);
	return;
}



void writeimage_fits_f(float *Data, const char *fitsfilename, int Exptime)

/******************************************************/
/* Create a FITS primary array containing a 2-D image */
/******************************************************/
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status, ii, jj;
	long  fpixel, nelements, wcsdim;
	double CRVAL1, CRVAL2, CRPIX1, CRPIX2, CD1_1, CD1_2, CD2_1, CD2_2, LTM1, LTM2, exposure, bzero, bscale, equinox;
	float *array[2000];
	char ctype1[] = "RA---TAN";
	char ctype2[] = "DEC--TAN";
	char WAT0[] = "system=image";
	char WAT1[] = "wtype=tan axtype=ra";
	char WAT2[] = "wtype=tan axtype=dec";
	char radecsys[] = "FK5";
	char fitsfilenameP[100];

	snprintf(fitsfilenameP, 200, "%s.fits", fitsfilename);

	/* initialize FITS image parameters */
	//const char *dirname = "C:/cygwin64/home/bitran/data/";
	//	const char fname[3];              /* name for new FITS file */
	//char filename[100];
	int bitpix = FLOAT_IMG; /* 16-bit unsigned short pixel values    */
	long naxis = 2;  /* 2-dimensional image                            */
	long naxes[2] = { 1024, 1024 };   /* image is 300 pixels wide by 200 rows */

									  /* allocate memory for the whole image */
	array[0] = (float *)malloc(naxes[0] * naxes[1]
		* sizeof(float));

	//snprintf(filename, 200, "%s%s", dirname, fitsname); 
	//printf("fits file name = %s\n", filename);
	//printf("fits name = %s\n", fitsname);

	/* initialize pointers to the start of each row of the image */
	for (ii = 1; ii<naxes[1]; ii++)
		array[ii] = array[ii - 1] + naxes[0];

	remove(fitsfilenameP);               /* Delete old file if it already exists */

	status = 0;         /* initialize status before calling fitsio routines */


	if (fits_create_file(&fptr, fitsfilenameP, &status)) /* create new FITS file */
		printerror(status);           /* call printerror if error occurs */

									  /* write the required keywords for the primary array image.     */
									  /* Since bitpix = USHORT_IMG, this will cause cfitsio to create */
									  /* a FITS image with BITPIX = 16 (signed short integers) with   */
									  /* BSCALE = 1.0 and BZERO = 32768.  This is the convention that */
									  /* FITS uses to store unsigned integers.  Note that the BSCALE  */
									  /* and BZERO keywords will be automatically written by cfitsio  */
									  /* in this case.                                                */

	if (fits_create_img(fptr, bitpix, naxis, naxes, &status))
		printerror(status);



	fpixel = 1;                               /* first pixel to write      */
	nelements = naxes[0] * naxes[1];          /* number of pixels to write */

											  /* write the array of unsigned integers to the FITS file */
											  //if (fits_write_img(fptr, TUSHORT, fpixel, nelements, array[0], &status))
											  //pImageData[1024]= 1000;
	if (fits_write_img(fptr, TUSHORT, fpixel, nelements, &Data, &status))

		printerror(status);



	/* write another optional keyword to the header */
	/* Note that the ADDRESS of the value is passed in the routine */

	exposure = Exptime / 1000.0; //convert msec to sec
	if (fits_update_key(fptr, TDOUBLE, "EXPOSURE", &exposure,
		"Total Exposure Time", &status))
		printerror(status);

	// Add WCS information

	//	bzero = 0.0;
	//if (fits_update_key(fptr, TDOUBLE, "BZERO", &bzero,NULL, &status))
	//printerror(status);	
	bscale = 1.0;
	if (fits_update_key(fptr, TDOUBLE, "BSCALE", &bscale, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "RADECSYS", &radecsys, NULL, &status))
		printerror(status);
	equinox = 2000.0;
	if (fits_update_key(fptr, TDOUBLE, "EQUINOX", &equinox, NULL, &status))
		printerror(status);


	if (fits_close_file(fptr, &status))                // close the file 
		printerror(status);

	printf("Fits write OK2\n");
	return;
}

/*--------------------------------------------------------------------------*/
void printerror(int status)
{
	/*****************************************************/
	/* Print out cfitsio error messages and exit program */
	/*****************************************************/


	if (status)
	{
		fits_report_error(stderr, status); /* print error report */

		exit(status);    /* terminate the program, returning error status */
	}
	return;
}

int write_status_to_file(int COMMAND_OK)
{
	
	FILE *fp; // FILE structure
	char buff[128] = "", buff2[128] = "";

	char fname_out[] = "C:/cygwin64/home/bitran/data/status.dat";
	int Vcorrc = 1;
	char str[10];
	float f1, f2, f3, f4, f5, PixeltoArcsec;
	PixeltoArcsec = 0.067*Vcorrc; //arcsec/pixel


		if (Initialize_OK == 1) {
	
			//printf("%d\n", Initialize_OK);
			// write a current status to a file
			if (fopen_s(&fp, fname_out, "w") != 0) { // 
													 //		if (fp == NULL) {
				printf("%s file not open!\n", fname_out);
				return -1;
			}

			fprintf(fp, "%d %f %f %f %f %d %d %d %d %f %f\n", COMMAND_OK, DeltaX*PixeltoArcsec*1000.0, DeltaY*PixeltoArcsec*1000.0, V0, V1, maxIntensityL, maxIntensity,  MaxX, MaxY, CCDtemp, Bodytemp); //
			printf("Status file: %d DeltaX, Y = %.4f, %.4f mas, V0, V1=%.4f, %.4f V, Max=%d, Max(dark subtr.)=%d, MaxX, Y= %d, %d, %d, %d pix, SN= %.1f, CCD and Body temp. = %.1f, %.1f\n", COMMAND_OK, DeltaX*PixeltoArcsec*1000.0, DeltaY*PixeltoArcsec*1000.0, V0, V1, maxIntensity, maxIntensityL, MaxX, MaxY, MaxXs+cenX0 - 30/ 2-1, MaxYs+cenY0 - 30/ 2-1, SN_star, CCDtemp, Bodytemp); //

			fclose(fp); // 

			// write command data to log file ///////////////////////////////////////
			/*			if (fopen_s(&fp, fname_log, "a") != 0) { // file open
													 //		if (fp == NULL) {
				printf("%s file not open!\n", fname_log);
				return -1;
			}
			time_t now = time(NULL);
			struct tm pnow;

			localtime_s(&pnow, &now);
			sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
			sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
			fprintf(fp, "%s %s Status file: %d DeltaX, Y = %.4f, %.4f mas, V0, V1=%.4f, %.4f V, Max=%d, Max(dark subtr.)=%d, MaxX, Y= %d, %d, %d, %d pix, SN=%.1f, CCD and Body temp. = %.1f, %.1f, CenX0, CenY0 = %d %d, CenxMf, CenyMf = %f %f, CenxMf_full, CenyMf_full = %f %f, Dark file = %s\n", buff, buff2, COMMAND_OK, DeltaX*PixeltoArcsec*1000.0, DeltaY*PixeltoArcsec*1000.0, V0, V1, maxIntensity, maxIntensityL, MaxX, MaxY, MaxXs + cenX0 - 30 / 2 - 1, MaxYs + cenY0 - 30 / 2 - 1, SN_star, CCDtemp, Bodytemp, cenX0, cenY0, CenxMf, CenyMf, CenxMf_full, CenyMf_full, fitsfilename_s_dark);
			fclose(fp); // 
			*/
			// end write command data to log file //////////////////////////////////////////
		}
		else if (Initialize_OK == 0) {
			//printf("%d\n", Initialize_OK);

		}

		


	
	return(0);
}

int write_status_to_log(int COMMAND_OK, const char *fitsfilename, int Num)
{

	FILE *fp; // FILE structure
	char buff[128] = "", buff2[128] = "";
	char fitsfilenameNum[100];
	char fname_out[] = "C:/cygwin64/home/bitran/data/status.dat";
	int Vcorrc = 1;
	char str[10];
	float f1, f2, f3, f4, f5, PixeltoArcsec;
	PixeltoArcsec = 0.067*Vcorrc; //arcsec/pixel


	if (Initialize_OK == 1) {

		snprintf(fitsfilenameNum, 200, "%s-%d", fitsfilename,Num);

		// write command data to log file ///////////////////////////////////////
		if (fopen_s(&fp, fname_log, "a") != 0) { // file open
												 //		if (fp == NULL) {
			printf("%s file not open!\n", fname_log);
			return -1;
		}
		time_t now = time(NULL);
		struct tm pnow;

		localtime_s(&pnow, &now);
		sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
		sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
		fprintf(fp, "%s %s Status file:file %s, %d DeltaX, Y = %.4f, %.4f mas, V0, V1=%.4f, %.4f V, Max=%d, Max(dark subtr.)=%d, MaxX, Y= %d, %d, %d, %d pix, SN=%.1f, CCD and Body temp. = %.1f, %.1f, CenX0, CenY0 = %d %d, CenxMf, CenyMf = %f %f, CenxMf_full, CenyMf_full = %f %f, Dark file = %s\n", buff, buff2, fitsfilenameNum, COMMAND_OK, DeltaX*PixeltoArcsec*1000.0, DeltaY*PixeltoArcsec*1000.0, V0, V1, maxIntensity, maxIntensityL, MaxX, MaxY, MaxXs + cenX0 - 30 / 2 - 1, MaxYs + cenY0 - 30 / 2 - 1, SN_star, CCDtemp, Bodytemp, cenX0, cenY0, CenxMf, CenyMf, CenxMf_full, CenyMf_full, fitsfilename_s_dark);
		fclose(fp); // 
					// end write command data to log file //////////////////////////////////////////
	}
	else if (Initialize_OK == 0) {
		//printf("%d\n", Initialize_OK);

	}





	return(0);
}



int calc_output_TT_voltage(int command_num) {
	int Vcorrc = 1;
	float PixeltoVolt, PixeltoVolt_x, PixeltoVolt_y, PixeltoArcsec;
	char fname[] = "C:/cygwin64/home/bitran/data/LastV.dat";

	printf("-------- Tip-tilt control -----------\n");
	FILE *fp; // FILE structure
	PixeltoVolt = PixeltoVolt_x = PixeltoVolt_y = PixeltoArcsec = 2.6;

	if (fopen_s(&fp, fname, "r") != 0) { // file open
										 //		if (fp == NULL) {
		printf("%s file not open!\n", fname);
		return -1;
	}

	fscanf_s(fp, "%f %f", &V0, &V1);
	printf("Initial V1 and V2 = %f %f\n", V0, V1);
	fclose(fp); // 
	printf("LastV.dat open\n");

	PixeltoVolt = 2.6*Vcorrc; //pixel/Volt
	PixeltoVolt_x = 2.456*Vcorrc; //pixel/Volt x (V1)
	PixeltoVolt_y = 2.96*Vcorrc; //pixel/Volt y (V0)

	PixeltoArcsec = 0.067*Vcorrc; //arcsec/pixelw

								  //DeltaX = -(CenxMf - CenxMs);
								  //DeltaY = -(CenyMf - CenyMs);

	V0 += DeltaY / PixeltoVolt_y;
	V1 += -DeltaX / PixeltoVolt_x;


	// write V01 V1 to LastV.dat
	if (fopen_s(&fp, fname, "w") != 0) { // 
										 //		if (fp == NULL) {
		printf("%s file not open!\n", fname);
		return -1;
	}
	if ((V0 > 10.0) | (V0 < 0.0)) V0 = 5.0;
	if ((V1 > 10.0) | (V1 < 0.0)) V1 = 5.0;
	if (command_num == 2) {
		V0 = V1 = 5.0;
		printf("TT voltage initialized!\n");
	}
	fprintf(fp, "%f %f\n", V0, V1); //
	fclose(fp); // 
	printf("-------- End Tip-tilt control -----------\n");
	return(0);
}


//int calc_CenterOfGravity(LPWORD pImageData, const char *fitsfilename, const char *fitsfilename_dark, int Exptime, HWND hWnd)
int calc_CenterOfGravity(LPWORD pImageData, const char *fitsfilename, char *TargetName, int Exptime, HWND hWnd)

/******************************************************/
/* Create a FITS primary array containing a 2-D image */
/******************************************************/
{
	const long NpixL = 1024;
	const long Npix = 30;
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status, ii, jj;
	long  fpixel, nelements, nelementsL, wcsdim;
	double CRVAL1, CRVAL2, CRPIX1, CRPIX2, CD1_1, CD1_2, CD2_1, CD2_2, LTM1, LTM2, exposure, bzero, bscale, equinox;
	//	unsigned short *array[1024]; //20190114
	//	unsigned short *array2[1024];//20190114
	//double *array[1024];
	double *array, *array2, *arrayL, *array_dark_small;
	//double *array_dark_small[1024];

	unsigned short *array1D;
	long fpix[2] = { 1, 1 };
	int anynul, anynul2;
	char filename_dark1[100];
	char filename_dark2[100], filename_dark3[100], filename_dark4[100], filename_dark5[100], filename_dark6[100], filename_dark7[100], filename_dark8[100];
	char darkfilename_list[] = "C:/cygwin64/home/bitran/data/darkfilename.dat";
	float dummy;
	FILE *fp;

	char ctype1[] = "RA---TAN";
	char ctype2[] = "DEC--TAN";
	char WAT0[] = "system=image";
	char WAT1[] = "wtype=tan axtype=ra";
	char WAT2[] = "wtype=tan axtype=dec";
	char buff[128] = "", buff2[128] = "";

	char radecsys[] = "FK5";
	char *str2 = "_s", *str3="_out", *str4 = "_L.fits";
	char fitsfilename_s[100], fitsfilename_s_dark_out2[100], fitsfilename_L_darksubt_out2[100], fitsfilename_L_darksubt_out[100], fitsfilename_L_darksubt[100];
	char fitsfilename_s_dark_out[]="selected_dark.fits";
	int i, j, kk;
	int indx, indy;
	char filename[] = "C:/cygwin64/home/bitran/data/bitran20181021-1.fits";
	char fiber_position_file[] = "C:/cygwin64/home/bitran/data/fiber_position.dat";

	float CenxMs_U, CenxMs_B, CenyMs_U, CenyMs_B, CenxMs, CenyMs, FiberX, FiberY;
	double nulval;
	double avg_background = 0.0;
	double rms_background = 0.0;
	double StarCount = 0.0;
	double tmp = 0.0;
	CenxMs_U = 0.0; CenxMs_B = 0.0; CenyMs_U = 0.0; CenyMs_B = 0.0; FiberX = 0.0; FiberY = 0.0;

	// open dark file name list
	printf("-------- calc_CenterOfGravity --------\n");
	if (fopen_s(&fp, darkfilename_list, "r") != 0) { // file open
													 //		if (fp == NULL) {
		printf("%s file not open!\n", darkfilename_list);
		return -1;
	}
	printf("dark file list open OK\n");
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark1, 100);
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark2, 100);
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark3, 100);
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark4, 100);
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark5, 100);
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark6, 100);
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark7, 100);
	fscanf_s(fp, "%f %s\n", &dummy, filename_dark8, 10);
	//printf("Dark file name: %s %s %s %s %s %s %s %s\n", filename_dark1, filename_dark2, filename_dark3, filename_dark4, filename_dark5, filename_dark6, filename_dark7, filename_dark8); //print all dark files in the darkfilename.dat

	fclose(fp); // 

	//@@@@@@@@ read file to get target fiber position @@@@@@@@@@@@@@@@@@@@@@@@
	if (fopen_s(&fp, fiber_position_file, "r") != 0) { // file open
										 //		if (fp == NULL) {
		printf("%s file not open!\n", fiber_position_file);
		return -1;
	}

	fscanf_s(fp, "%d %d %f %f", &cenX0, &cenY0, &CenxMf, &CenyMf); //in case of space delimiter
//	write, format = "CoGX full=%.3f, CoGY full=%.3f \n", CenxMs + cenX0 - Npix - 1, CenyMs + cenY0 - Npix - 1;
	CenxMf_full = CenxMf + cenX0 - Npix / 2 - 1;
	CenyMf_full = CenyMf + cenY0 - Npix / 2 - 1;
	printf("Fiber position coordiante: %d %d %f %f %f %f\n", cenX0, cenY0, CenxMf_full, CenyMf_full, CenxMf, CenyMf);
	fclose(fp); // 


	// fiber center position  from FIM_top_dynamic_1um_5sec_stack.fits (baseline)
	/*
	CenxMs_U = 0.0; CenxMs_B = 0.0; CenyMs_U = 0.0; CenyMs_B = 0.0;
	cenX0 = 480; cenY0 = 387;
	FiberX = 479.87138; FiberY = 386.62657; // not used for critical purpose, baseline FIM_topdynamic_1um_5sec_stack, FIM_top_F3d3_200m_dynamic_5sec_stack
	CenxMf = 15.87138; CenyMf = 15.62657; //  baseline FIM_topdynamic_1um_5sec_stack, FIM_top_F3d3_200m_dynamic_5sec_stack
	*/

	// fiber center position  from 20190415 measurement
	//FiberX = 480.1579; FiberY = 386.5904; // not used for critical purpose, b baseline bitran20190415-3, bitran20190415-7
	//CenxMf = 16.1579; CenyMf = 15.5904; //  baseline bitran20190415-3, bitran20190415-7

	//CenxMf = 16.0; CenyMf = 16.0; //for software dbugging test

	sprintf_s(fitsfilename_s, 100,"%s%s.fits", fitsfilename, str2);
	sprintf_s(fitsfilename_L_darksubt_out, 100,"%s_L.fits", fitsfilename);
	//sprintf_s(fitsfilename_s_dark, 100,	"%s.fits", fitsfilename_dark);
	/* initialize FITS image parameters */
	//const char *dirname = "C:/cygwin64/home/bitran/data/";
	//	const char fname[3];              /* name for new FITS file */
	//char filename[100];
	int bitpix = USHORT_IMG; /* 16-bit unsigned short pixel values    */
	long naxis = 2;  /* 2-dimensional image                            */
	long naxes[2] = { Npix, Npix };   /* image is 300 pixels wide by 200 rows */
	long naxes2[2] = { NpixL, NpixL };   /* image is 300 pixels wide by 200 rows */

	nelements = naxes2[0] * naxes2[1];          /* number of pixels to write */

												/* allocate memory for the whole image */
												/*20190114
												array[0] = (unsigned short *)malloc(naxes[0] * naxes[1]
												* sizeof(unsigned short));
												array2[0] = (unsigned short *)malloc(naxes2[0] * naxes2[1]
												* sizeof(unsigned short));
												*/
	//array[0] = (double *)malloc(naxes[0] * naxes[1]* sizeof(double));
    array = new double[naxes[0] * naxes[1]];
	array2 = new double[naxes2[0]*naxes2[1]];
	arrayL = new double[naxes2[0] * naxes2[1]];
	array_dark_small = new double[naxes[0] * naxes[1]];
	//array2[0] = (double *)malloc(naxes2[0] * naxes2[1]
	//	* sizeof(double));
	//array_dark_small[0] = (double *)malloc(naxes[0] * naxes[1]* sizeof(double)); //2019/02/18
	//array1D = (unsigned short *)malloc(Npix * Npix * sizeof(unsigned short));

	/* initialize pointers to the start of each row of the image */
	//for (ii = 1; ii<naxes[1]; ii++)
	//	array[ii] = array[ii - 1] + naxes[0];
	//for (ii = 1; ii<naxes2[1]; ii++)
	//	array2[ii] = array2[ii - 1] + naxes2[0];
	//for (ii = 1; ii<naxes[1]; ii++)
	//	array_dark_small[ii] = array_dark_small[ii - 1] + naxes[0];
	printf("Array initilization OK\n");

	//@@@@@@@@@   Read dark file @@@@@@@@@@@@@@@@@@@@@@@@@@@@

	//if (Exptime == int(0.1 * 1000)) strcpy_s(fitsfilename_s_dark, 30, filename_dark1);
	
	if (Exptime == int(0.1 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark1);
	if (Exptime == int(1.0 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark2);
	if (Exptime == int(5.0 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark3);
	if (Exptime == int(10.0 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark4);
	if (Exptime == int(30.0 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark5);
	if (Exptime == int(60.0 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark6);
	if (Exptime == int(180.0 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark7);
	if (Exptime == int(360.0 * 1000)) sprintf_s(fitsfilename_s_dark, 200, "%s%s", dirdataname, filename_dark8);

	printf("Exptime = %.1f sec\n", double(Exptime)/ 1000.0);

	printf("Selected Dark file name = %s\n", fitsfilename_s_dark);


	//printf("%s\n", fitsfilename_s_dark);
	//******* read dark file ***************/
	if (fits_open_file(&fptr, fitsfilename_s_dark, READONLY, &status))
		printerror(status);
	printf("Dark file open OK\n");
	//	if (fits_read_pix(fptr, TUSHORT, fpix, nelements, 0, array2[0], &anynul, &status))
	if (fits_read_pix(fptr, TDOUBLE, fpix, nelements, 0, array2, &anynul, &status)) //20190114
			printerror(status);
	printf("Dark file read OK\n");

	if (fits_close_file(fptr, &status))
		printerror(status);
	printf("Dark file close OK\n");

	//********* create dark subtracted image ****************/
	maxIntensity = 0; StarCount = 0.0; avg_background = 0.0; rms_background = 0.0;
	for (jj = 0; jj < Npix; jj++)
	{
		for (ii = 0; ii < Npix; ii++)
		{
			//		array[jj][ii] = pImageData[NpixL/2 - Npix/2+ii+(jj+ NpixL/2 - Npix/2)*1024];
			//array[jj][ii] = pImageData[cenX0 - Npix / 2 + ii - 1 + (jj + cenY0 - Npix / 2 - 1) * 1024] - array2[cenY0 - Npix / 2 + jj - 1][cenX0 - Npix / 2 + ii - 1];//20190114
			//array[jj][ii] = double(pImageData[cenX0 - Npix / 2 + ii - 1 + (jj + cenY0 - Npix / 2 - 1) * 1024])-array2[cenX0 - Npix / 2 + ii - 1 + (cenY0 - Npix / 2 + jj - 1) * 1024]; //2019/02/18
			array[ii + (jj) * Npix] = double(pImageData[cenX0 - Npix / 2 + ii - 1 + (jj + cenY0 - Npix / 2 - 1) * 1024]) - array2[cenX0 - Npix / 2 + ii - 1 + (cenY0 - Npix / 2 + jj - 1) * 1024];
			//array_dark_small[jj][ii] = array2[cenX0 - Npix / 2 + ii - 1 + (cenY0 - Npix / 2 + jj - 1)*1024];//20190218
			array_dark_small[ii + (jj)* Npix] = array2[cenX0 - Npix / 2 + ii - 1 + (cenY0 - Npix / 2 + jj - 1) * 1024];
			StarCount += array[ii + (jj)* Npix];
			avg_background += double(pImageData[cenX0 - Npix / 2 + ii - 1 + Npix + (jj + cenY0 - Npix / 2 - 1) * 1024]) - array2[cenX0 - Npix / 2 + ii - 1 + Npix + (cenY0 - Npix / 2 + jj - 1) * 1024];
			//array1D[ii + (jj)*Npix] = pImageData[cenX0 - Npix / 2 + ii - 1 + (jj + cenY0 - Npix / 2 - 1) * 1024] - array2[cenY0 - Npix / 2 + jj - 1 + (cenX0 - Npix / 2 + ii - 1) * 1024];
			if (pImageData[cenX0 - Npix / 2 + ii - 1 + (jj + cenY0 - Npix / 2 - 1) * 1024] > maxIntensity) {
				maxIntensity = pImageData[cenX0 - Npix / 2 + ii - 1 + (jj + cenY0 - Npix / 2 - 1) * 1024];
			}

		}
	}

	// calc rms of Npix X offset, small dark subtracted image
	avg_background /= double(Npix*Npix);
	
	for (jj = 0; jj < Npix; jj++)
	{
		for (ii = 0; ii < Npix; ii++)
		{
			tmp = avg_background - double(pImageData[cenX0 - Npix / 2 + ii - 1 + Npix + (jj + cenY0 - Npix / 2 - 1) * 1024]) - array2[cenX0 - Npix / 2 + ii - 1 + Npix + (cenY0 - Npix / 2 + jj - 1) * 1024];

			rms_background += tmp*tmp;
		}
	}

	rms_background = sqrt(rms_background / (Npix*Npix));
	SN_star = StarCount / rms_background;
	printf("Sta_count =%f rms =%f, avg = %f, SN=%f \n", StarCount, rms_background, avg_background, SN_star);
	//********* bad pix correction ****************/
	for (kk = 0; kk < Nbadpix; kk++)
	{
		indx = indx0[kk] - (cenX0 - Npix / 2 - 1);
		indy = indy0[kk] - (cenY0 - Npix / 2 - 1);
	if((indx-1 > -1) && (indx+1<=Npix-1) && (indy-1>-1) && (indy+1 <= Npix-1)){
		array[indx + (indy)* Npix] = (array[indx + 1 + (indy)* Npix] + array[indx - 1 + (indy)* Npix] + array[indx + (indy + 1)* Npix] + array[indx + (indy - 1)* Npix])/4.0;
		//array[indx + (indy)* Npix] = 10000;
	}

	}
	//********* create dark subtracted large image ****************/
	maxIntensityL = 0;
	for (jj = 0; jj < NpixL; jj++)
	{
		for (ii = 0; ii < NpixL; ii++)
		{
			arrayL[ii+ (jj) * 1024] = double(pImageData[ii+(jj) * 1024]) - array2[ii + (jj) * 1024];
			/* find maximum position */
			if (arrayL[ii + (jj) * 1024] > maxIntensityL) {
				maxIntensityL = arrayL[ii + (jj) * 1024];
				MaxX = ii+1; MaxY = jj+1;
			}

		}
	}
	printf("Array crop read OK\n");
	//if (hBitmap)
	//		DeleteObject(hBitmap);
	//hBitmap = BitranCCDlibImageConvert(NULL, array1D);
	//InvalidateRect(hWnd, NULL, TRUE);
	//@@@@@@ calc Center of Gravity of cropped image  @@@@@@@@@@@@@@
	maxIntensityL = 0;
	for (j = 0; j < Npix; j++) {
		for (i = 0; i < Npix; i++) {
			/*
			CenyMs_U += (i + 1)*array[i][j];
			CenyMs_B += array[i][j];
			CenxMs_U += (j + 1)*array[i][j];
			CenxMs_B += array[i][j];
			*/
			if (array[i + (j) * Npix] > maxIntensityL) {
				maxIntensityL = array[i + (j) * Npix];
				MaxXs = i + 1; MaxYs = j + 1;
			}
			CenyMs_U += (i + 1)*array[j + (i) * Npix];
			CenyMs_B += array[j + (i) * Npix];
			CenxMs_U += (j + 1)*array[j + (i) * Npix];
			CenxMs_B += array[j + (i) * Npix];
			
		}
	}

	CenxMs = CenxMs_U / CenxMs_B;
	CenyMs = CenyMs_U / CenyMs_B;
	DeltaX = -(CenxMf - CenxMs);
	DeltaY = -(CenyMf - CenyMs);

	printf("Fiber X = %f Fiber Y = %f pixel\n", FiberX, FiberY);
	printf("Star CoG X = %f CoG Y = %f pixel\n", CenxMs, CenyMs);
	printf("delta X = %f delta Y = %f pixel\n", DeltaX, DeltaY);

	//@@@@@ create dark subtracted, cropped fits file @@@@@@@@@@@@@@@@@@@@@@@@@@@
	exposure = Exptime / 1000.0; //convert msec to sec

	remove(fitsfilename_s);               /* Delete old file if it already exists */

	status = 0;         /* initialize status before calling fitsio routines */

	if (fits_create_file(&fptr, fitsfilename_s, &status)) /* create new FITS file */
		printerror(status);           /* call printerror if error occurs */

									  //	if (fits_create_img(fptr, bitpix, naxis, naxes, &status)) //20190114
	if (fits_create_img(fptr, DOUBLE_IMG, naxis, naxes, &status)) //20190114

		printerror(status);

	fpixel = 1;                               /* first pixel to write      */
	nelements = naxes[0] * naxes[1];          /* number of pixels to write */

											  /* write the array of unsigned integers to the FITS file */
											  //if (fits_write_img(fptr, TUSHORT, fpixel, nelements, array[0], &status))
											  //	if (fits_write_img(fptr, TUSHORT, fpixel, nelements, array[0], &status)) //20190114
	//if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, array[0], &status)) //20190218
	if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, array, &status)) //
		printerror(status);

	//update fits headers
	if (fits_update_key(fptr, TDOUBLE, "EXPOSURE", &exposure, "Total Exposure Time", &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "TARGET", TargetName, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "DARKFILE", fitsfilename_s_dark, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "DELTAX", &DeltaX, "Star-Fiber deltaX [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "DELTAY", &DeltaY, "Star-Fiber deltaY [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TINT, "CENX0", &cenX0, "Center of cropped image X [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TINT, "CENY0", &cenY0, "Center of cropped image Y [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENXMF", &CenxMf, "Fiber position X in crooped image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENYMF", &CenyMf, "Fiber position Y in cropped image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENXMFFU", &CenxMf_full, "Fiber position X in full image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENYMFFU", &CenyMf_full, "Fiber position Y in full image [pixel]", &status))
		printerror(status);

	time_t now = time(NULL);
	struct tm pnow;

	localtime_s(&pnow, &now);
	sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
	sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
	if (fits_update_key(fptr, TSTRING, "DATE", buff, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "HST", buff2, NULL, &status))
		printerror(status);

	if (fits_close_file(fptr, &status))                // close the file 
		printerror(status);

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	printf("Cropped, dark subtracted fits write OK\n");

	//@@@@@@ Dark data to a file @@@@@@@@@@@@@@@@@@@@@@@@@@
	sprintf_s(fitsfilename_s_dark_out2, 100, "%s%s", dirdataname, fitsfilename_s_dark_out);
	remove(fitsfilename_s_dark_out2);               /* Delete old file if it already exists */

	status = 0;         /* initialize status before calling fitsio routines */

	if (fits_create_file(&fptr, fitsfilename_s_dark_out2, &status)) /* create new FITS file */
		printerror(status);           /* call printerror if error occurs */

									  //	if (fits_create_img(fptr, bitpix, naxis, naxes, &status)) //20190114
	//if (fits_create_img(fptr, DOUBLE_IMG, naxis, naxes2, &status)) 
		if (fits_create_img(fptr, DOUBLE_IMG, naxis, naxes, &status)) 
		printerror(status);

	fpixel = 1;                               /* first pixel to write      */
//	nelements = naxes2[0] * naxes2[1];          /* number of pixels to write */
	nelements = naxes[0] * naxes[1];          /* number of pixels to write */

//	if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, array2, &status))
		//if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, array_dark_small[0], &status)) //2019/02/18
			if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, array_dark_small, &status))
		printerror(status);

	if (fits_close_file(fptr, &status))                // close the file 
		printerror(status);

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	printf("Cropped Dark file Fits write OK\n");


	//@@@@@@ Dark subtracted large data to a file @@@@@@@@@@@@@@@@@@@@@@@@@@
	sprintf_s(fitsfilename_L_darksubt_out2, 100, "%s%s", dirdataname, fitsfilename_L_darksubt_out);
	remove(fitsfilename_L_darksubt_out);               /* Delete old file if it already exists */
	printf("Dark subtracted large file name =%s\n", fitsfilename_L_darksubt_out);
	status = 0;         /* initialize status before calling fitsio routines */
						
	if (fits_create_file(&fptr, fitsfilename_L_darksubt_out, &status)) // create new FITS file 
		printerror(status);           // call printerror if error occurs 

	if (fits_create_img(fptr, DOUBLE_IMG, naxis, naxes2, &status))
		printerror(status);

	fpixel = 1;                               // first pixel to write    
											  //	nelements = naxes2[0] * naxes2[1];          // number of pixels to write 
	nelements = naxes2[0] * naxes2[1];          // number of pixels to write

											  //	if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, array2, &status))
	if (fits_write_img(fptr, TDOUBLE, fpixel, nelements, arrayL, &status))
		printerror(status);

	//update fits headers
	if (fits_update_key(fptr, TDOUBLE, "EXPOSURE", &exposure,"Total Exposure Time", &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "TARGET", TargetName, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "DARKFILE", fitsfilename_s_dark, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "DELTAX", &DeltaX, "Star-Fiber deltaX [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "DELTAY", &DeltaY, "Star-Fiber deltaY [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TINT, "CENX0", &cenX0, "Center of cropped image X [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TINT, "CENY0", &cenY0, "Center of cropped image Y [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENXMF", &CenxMf, "Fiber position X in crooped image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENYMF", &CenyMf, "Fiber position Y in cropped image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENXMFFU", &CenxMf_full, "Fiber position X in full image [pixel]", &status))
		printerror(status);
	if (fits_update_key(fptr, TFLOAT, "CENYMFFU", &CenyMf_full, "Fiber position Y in full image [pixel]", &status))
		printerror(status);

	//time_t now = time(NULL);
	//struct tm pnow;

	//localtime_s(&pnow, &now);
	//sprintf_s(buff, 128, "%04d-%02d-%02d", pnow.tm_year + 1900, pnow.tm_mon + 1, pnow.tm_mday);
	//sprintf_s(buff2, 128, "%02d:%02d:%02d", pnow.tm_hour, pnow.tm_min, pnow.tm_sec);
	if (fits_update_key(fptr, TSTRING, "DATE", buff, NULL, &status))
		printerror(status);
	if (fits_update_key(fptr, TSTRING, "HST", buff2, NULL, &status))
		printerror(status);

	if (fits_close_file(fptr, &status))                // close the file 
		printerror(status);
	
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	printf("Dark file Fits write OK\n");

	printf("-------- End calc_CenterOfGravity --------\n");

	//free(array[0]);
	//free(array2);
	//	free(array1D);
	delete[] array2;
	delete[] arrayL;
	delete[] array;
	delete[]  array_dark_small;


	return(0);

}

void make_command_log(void) {


}