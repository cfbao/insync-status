#ifdef INSYNCRECT_EXPORTS
#define INSYNCRECT_API __declspec(dllexport)
#else
#define INSYNCRECT_API __declspec(dllimport)
#endif

#include <Windows.h>
#include <Commctrl.h>
#include <string>

extern "C" {
	INSYNCRECT_API bool get_tray_icon_rect(wchar_t * iconText, size_t* x1, size_t* y1, size_t* x2, size_t* y2);
}

class RemoteToolbarBuffer
{
public:
	RemoteToolbarBuffer(HWND windowHandle, SIZE_T bufferSize);
	~RemoteToolbarBuffer();
	LPVOID ptr;
	SIZE_T	bufferSize;
	HWND windowHandle;
	HANDLE processHandle;
};

bool get_remote_button(const RemoteToolbarBuffer&, size_t, TBBUTTON*);
bool get_remote_button_text(const RemoteToolbarBuffer& remoteBuff, int idCommand, std::wstring* buttonText);
bool get_remote_rect(const RemoteToolbarBuffer& remoteBuff, int idCommand, RECT* rectButton);
