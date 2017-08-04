#include "tray-icon-rect.h"

INSYNCRECT_API bool get_tray_icon_rect(wchar_t * iconText, size_t* x1, size_t* y1, size_t* x2, size_t* y2)
{
	const std::wstring iconStr(iconText);
	HWND trayHandle = FindWindowEx(NULL, NULL, L"Shell_TrayWnd", NULL);
	trayHandle = FindWindowEx(trayHandle, NULL, L"TrayNotifyWnd", NULL);
	trayHandle = FindWindowEx(trayHandle, NULL, L"SysPager", NULL);
	trayHandle = FindWindowEx(trayHandle, NULL, L"ToolbarWindow32", NULL);
	if (!trayHandle) return false;
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	size_t bufferSize = systemInfo.dwPageSize;
	RemoteToolbarBuffer remoteBuff(trayHandle, bufferSize);
	size_t buttonCount = SendMessage(trayHandle, TB_BUTTONCOUNT, 0, 0);
	TBBUTTON button;
	std::wstring buttonText;
	bool found = false;
	for (size_t i = 0; i < buttonCount; ++i) {
		if (!get_remote_button(remoteBuff, i, &button)) continue;
		if (!get_remote_button_text(remoteBuff, button.idCommand, &buttonText)) continue;
		if (buttonText.length() >= iconStr.length()
			&& iconStr.compare(0, iconStr.length(), buttonText, 0, iconStr.length()) == 0) {
			found = true;
			break;
		}
	}
	if (!found) return false;
	RECT rectTray, rectButton;
	if (!get_remote_rect(remoteBuff, button.idCommand, &rectButton)) return false;
	if (!GetWindowRect(trayHandle, &rectTray)) return false;
	*x1 = rectButton.left + rectTray.left;
	*y1 = rectButton.top + rectTray.top;
	*x2 = rectButton.right + rectTray.left;
	*y2 = rectButton.bottom + rectTray.top;
	return true;
}

// get the idx-th button in a remote process owned window
// return true if successful, false if failed
bool get_remote_button(const RemoteToolbarBuffer &remoteBuff, size_t idx, TBBUTTON* button) {
	LRESULT res = SendMessage(remoteBuff.windowHandle, TB_GETBUTTON, idx, LPARAM(remoteBuff.ptr));
	if (!res) return false;
	return ReadProcessMemory(remoteBuff.processHandle, remoteBuff.ptr, button, sizeof(*button), NULL);
}

// get the text of the bottom with idCommand=`idcommand`
// return true if successful, false if failed
bool get_remote_button_text(const RemoteToolbarBuffer &remoteBuff, int idCommand, std::wstring* buttonText) {
	size_t len = SendMessage(remoteBuff.windowHandle, TB_GETBUTTONTEXT, idCommand, NULL);
	if (sizeof(wchar_t) * (len + 1) > remoteBuff.bufferSize) return false;
	len = SendMessage(remoteBuff.windowHandle, TB_GETBUTTONTEXT, idCommand, LPARAM(remoteBuff.ptr));
	if (len < 0) return false;
	// include the terminating null character in `len`
	len++;
	wchar_t* textBuff = new wchar_t[len];
	bool success = ReadProcessMemory(remoteBuff.processHandle, remoteBuff.ptr, textBuff, sizeof(wchar_t)*len, NULL);
	if (success) buttonText->assign(textBuff);
	delete[] textBuff;
	return success;
}

bool get_remote_rect(const RemoteToolbarBuffer &remoteBuff, int idCommand, RECT* rectButton) {
	LRESULT res = SendMessage(remoteBuff.windowHandle, TB_GETRECT, idCommand, LPARAM(remoteBuff.ptr));
	if (!res) return false;
	return ReadProcessMemory(remoteBuff.processHandle, remoteBuff.ptr, rectButton, sizeof(*rectButton), NULL);
}

RemoteToolbarBuffer::RemoteToolbarBuffer(HWND windowHandle, SIZE_T bufferSize) {
	DWORD processId;
	GetWindowThreadProcessId(windowHandle, &processId);
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
	this->bufferSize = bufferSize;
	this->windowHandle = windowHandle;
	ptr = VirtualAllocEx(processHandle, NULL, bufferSize, MEM_COMMIT, PAGE_READWRITE);
}

RemoteToolbarBuffer::~RemoteToolbarBuffer() {
	VirtualFreeEx(processHandle, ptr, bufferSize, MEM_RELEASE);
	CloseHandle(processHandle);
}
