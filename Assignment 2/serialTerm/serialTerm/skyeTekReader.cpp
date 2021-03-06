/*------------------------------------------------------------------------------------------------------------------
--		SOURCE FILE:	skyeTekReader.cpp
--		PROGRAM:		SkyeTek Reader
--		Functions
--				WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
-- 						  LPSTR lspszCmdParam, int nCmdShow)	
--				LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
--				void checkItem(HMENU, UINT);
--
--		DATE:			October 10, 2013
--		DESIGNER:		Ramzi Chennafi and Ian Davidson
--		PROGRAMMER:		Ramzi Chennafi and Ian Davidson
--
--		NOTES:
--		Deals with the reading and displaying of RFID tags from a reader to the client area.
----------------------------------------------------------------------------------------------------------------------*/
#include "resource.h"

HWND				mainHWND;
HANDLE				readThrd;
static TCHAR		Name[]		= TEXT("SkyeTek Reader");
bool				activePort	= false;
LPSKYETEK_DEVICE	*devices	= NULL;
LPSKYETEK_READER	*readers	= NULL;
int					yPos		= 0;
int					numDevices  = 0;
int					numReaders  = 0;


/*------------------------------------------------------------------------------------------------------------------
--		FUNCTION:		WinMain
--		DATE:			October 10, 2012
--		REVISIONS:		n/a
--		DESIGNER:		Ramzi Chennafi and Ian Davidson	
--		PROGRAMMER:		Ramzi Chennafi and Ian Davidson
--
--		INTERFACE:		int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hprevInstance,
--											LPSTR lspszCmdParam, int nCmdShow)					
--
--		RETURNS:		int 
--
--		NOTES:
--		Creates a new window for the application and begins calls. Intiates a global window HWND.
----------------------------------------------------------------------------------------------------------------------*/

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hprevInstance,
 						  LPSTR lspszCmdParam, int nCmdShow)
{
	HWND		hwnd;
	MSG			Msg;
	WNDCLASSEX	Wcl;
	
	Wcl.cbSize = sizeof (WNDCLASSEX);
	Wcl.style = 0;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
	Wcl.hIconSm = NULL; 
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);  
	
	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH); 
	Wcl.lpszClassName = Name;
   
	Wcl.lpszMenuName = TEXT("MYMENU");
	Wcl.cbClsExtra = 0;   
	Wcl.cbWndExtra = 0;
	
	if (!RegisterClassEx (&Wcl))
		return 0;
   
	hwnd = CreateWindow (Name, Name, WS_OVERLAPPEDWINDOW, 350, 160,
   							800, 500, NULL, NULL, hInst, NULL);
	
	mainHWND = hwnd;
	
	ShowWindow (hwnd, nCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&Msg, NULL, 0, 0))
	{
   		TranslateMessage (&Msg);
		DispatchMessage (&Msg);
	}

	return Msg.wParam;
}
/*------------------------------------------------------------------------------------------------------------------
--		FUNCTION:		WndProc
--		DATE:			October 10, 2013
--		REVISIONS:		n/a
--		DESIGNER:		Ramzi Chennafi and Ian Davidson
--		PROGRAMMER:		Ramzi Chennafi and Ian Davidson
--
--		INTERFACE:		LRESULT CALLBACK WndProc (HWND hwnd, UINT Message,
--                          WPARAM wParam, LPARAM lParam)				
--
--		RETURNS:		LRESULT to the system.
--
--		NOTES:
--		Deals with the main window thread. Intiates a connection to the RFID on Connect. Also intiates the disconnect 
--		and provides a help dialogue.
----------------------------------------------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc (HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	HMENU	menuHnd	 = GetMenu(hwnd);
	HDC		hdc;

	switch (Message)
   {
		case WM_COMMAND:
		   switch (LOWORD (wParam))
			{
				case IDM_CONNECT:
					numReaders = 0;
					numDevices = 0;
					openConnection(hwnd, menuHnd);
				break;
					
				case IDM_DISCONNECT:
					endRead(hwnd, numReaders, numDevices, menuHnd);
				break;

				case IDM_HELP:
					MessageBox (hwnd, TEXT("Reads RFID tags from a Skyetek RFID reader and displays them. Just connect the reader and hit connect. Hitting escape or disconnect stops the reading."), 
						TEXT("SkyeTek Reader"), MB_OK | MB_ICONINFORMATION);
				break;
				
			}

		break;

		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE && activePort == true){
				endRead(hwnd, numReaders, numDevices, menuHnd);
			}
		break;

		case WM_DESTROY:	
      		PostQuitMessage (0);
		break;
		
		default:
			 return DefWindowProc (hwnd, Message, wParam, lParam);
	}
	return 0;
}
/*------------------------------------------------------------------------------------------------------------------
--		FUNCTION:		increaseY
--		DATE:			October 10, 2013
--		REVISIONS:		n/a
--		DESIGNER:		Ramzi Chennafi and Ian Davidson
--		PROGRAMMER:		Ramzi Chennafi and Ian Davidson
--
--		INTERFACE:		int increaseY()	
--
--		RETURNS:		yPos - 20
--
--		NOTES:
--		Increases the global variable y by 20 then returns it decremented by 20 to the calling thread.
----------------------------------------------------------------------------------------------------------------------*/
int increaseY(){
	yPos += 20;
	return yPos - 20;
}
/*------------------------------------------------------------------------------------------------------------------
--		FUNCTION:		openConnection
--		DATE:			October 10, 2013
--		REVISIONS:		n/a
--		DESIGNER:		Ramzi Chennafi and Ian Davidson
--		PROGRAMMER:		Ramzi Chennafi and Ian Davidson
--
--		INTERFACE:		void openConnection(HWND hwnd, HMENU menuHnd)	
--
--		RETURNS:		Nothing
--
--		NOTES:
--		Establishes a connection to the RFID reader by calling the Skyetek API. Creates a new thread to perform reads
--		and writes.
----------------------------------------------------------------------------------------------------------------------*/
void openConnection(HWND hwnd, HMENU menuHnd){
	HDC		hdc		 = GetDC(hwnd);
	DWORD	threadId;

	TextOut(hdc, 0, increaseY(), TEXT("Discovering Devices..."), 21);
	
	if(!activePort){
		if((numDevices = SkyeTek_DiscoverDevices(&devices)) > 0)
		{
			if((numReaders = SkyeTek_DiscoverReaders(devices,numDevices,&readers)) > 0 )
			{
				TextOut(hdc, 0, increaseY(), TEXT("Discovered Reader"), 17);
				activePort = true;
				EnableMenuItem(menuHnd, IDM_CONNECT, MF_DISABLED);
				EnableMenuItem(menuHnd, IDM_DISCONNECT, MF_ENABLED);
				DrawMenuBar(hwnd);
				readThrd = CreateThread(NULL, 0, execRead, readers[0], 0, &threadId);

			}
			else{
				TextOut(hdc, 0, increaseY(), TEXT("Failed to Discover Reader"), 25);
			}
		}
		else {
			TextOut(hdc, 0, increaseY(), TEXT("Failed to Discover Devices"), 26);
		}
	}
	ReleaseDC(hwnd, hdc);
}
/*------------------------------------------------------------------------------------------------------------------
--		FUNCTION:		execRead
--		DATE:			October 10, 2013
--		REVISIONS:		n/a
--		DESIGNER:		Ramzi Chennafi and Ian Davidson
--		PROGRAMMER:		Ramzi Chennafi and Ian Davidson
--
--		INTERFACE:		DWORD WINAPI execRead(LPVOID reader)		
--
--		RETURNS:		DWORD, 0 on thread exit
--
--		NOTES:
--		Requests and reads tag data at 2 second intervals from the reader and presents it on the client screen.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI execRead(LPVOID reader){

	SKYETEK_STATUS		status;
	LPSKYETEK_STRING	str;
	SKYETEK_ADDRESS		addr;
	LPSKYETEK_DATA		lpData		= NULL;
	LPSKYETEK_TAG		lpTag		= NULL;
	LPSKYETEK_READER	lpReader	= (LPSKYETEK_READER)reader;
	int					flag		= 0;
	
	addr.start = 0x5002;
	addr.blocks = 6;
	
	SkyeTek_CreateTag(ISO_18000_6C_AUTO_DETECT, NULL, &lpTag);
	
	HDC hdc = GetDC(mainHWND);
	TextOut(hdc, 0, increaseY(), TEXT("RFID Tag IDs"), 5);
	ReleaseDC(mainHWND, hdc);

	while(activePort){
		status = SkyeTek_ReadTagData(lpReader,lpTag,&addr,flag,flag,&lpData);
		if(status == SKYETEK_SUCCESS)
		{
			HDC hdc = GetDC(mainHWND);
			str = SkyeTek_GetStringFromData(lpData);
			TextOut(hdc, 0, increaseY(), str, 24);
			ReleaseDC(mainHWND, hdc);
			Sleep(2000);
		}		
	}

	ExitThread(0);
}
/*------------------------------------------------------------------------------------------------------------------
--		FUNCTION:		endRead
--		DATE:			October 10, 2013
--		REVISIONS:		n/a
--		DESIGNER:		Ramzi Chennafi and Ian Davidson
--		PROGRAMMER:		Ramzi Chennafi and Ian Davidson
--
--		INTERFACE:		void endRead(HWND hwnd, int numReaders, int numDevices, HMENU menuHnd)		
--
--		RETURNS:		Nothing
--
--		NOTES:
--		Completes the disconnect of a session. Fixes menu bar items on disconnect, sends the trigger to end the read 
--		thread and frees both the readers and devices.
----------------------------------------------------------------------------------------------------------------------*/
void endRead(HWND hwnd, int numReaders, int numDevices, HMENU menuHnd){
	
	activePort = false;
	MessageBox (hwnd, TEXT("Client Disconnected"), TEXT("SkyeTek Reader"), MB_OK | MB_ICONEXCLAMATION);
	
	InvalidateRect(hwnd, NULL, FALSE);
	EnableMenuItem(menuHnd, IDM_CONNECT, MF_ENABLED);
	EnableMenuItem(menuHnd, IDM_DISCONNECT, MF_DISABLED);
	DrawMenuBar(hwnd);
	
	SkyeTek_FreeReaders(readers, numReaders);
	SkyeTek_FreeDevices(devices, numDevices);
	yPos = 0;
}
