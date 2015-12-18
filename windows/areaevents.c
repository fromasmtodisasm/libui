// 8 september 2015
#include "uipriv_windows.h"
#include "area.h"

static uiModifiers getModifiers(void)
{
	uiModifiers m = 0;

	if ((GetKeyState(VK_CONTROL) & 0x80) != 0)
		m |= uiModifierCtrl;
	if ((GetKeyState(VK_MENU) & 0x80) != 0)
		m |= uiModifierAlt;
	if ((GetKeyState(VK_SHIFT) & 0x80) != 0)
		m |= uiModifierShift;
	if ((GetKeyState(VK_LWIN) & 0x80) != 0)
		m |= uiModifierSuper;
	if ((GetKeyState(VK_RWIN) & 0x80) != 0)
		m |= uiModifierSuper;
	return m;
}

static void areaMouseEvent(uiArea *a, uintmax_t down, uintmax_t  up, WPARAM wParam, LPARAM lParam)
{
	uiAreaMouseEvent me;
	uintmax_t button;
	double xpix, ypix;
	FLOAT dpix, dpiy;
	D2D1_SIZE_F size;

	if (a->rt == NULL)
		a->rt = makeHWNDRenderTarget(a->hwnd);

	xpix = (double) GET_X_LPARAM(lParam);
	ypix = (double) GET_Y_LPARAM(lParam);
	// these are in pixels; we need points
	ID2D1HwndRenderTarget_GetDpi(a->rt, &dpix, &dpiy);
	// see https://msdn.microsoft.com/en-us/library/windows/desktop/dd756649%28v=vs.85%29.aspx (and others; search "direct2d mouse")
	me.X = (xpix * 96) / dpix;
	me.Y = (ypix * 96) / dpiy;

	me.AreaWidth = 0;
	me.AreaHeight = 0;
	if (!a->scrolling) {
		size = ID2D1HwndRenderTarget_GetSize(a->rt);
		me.AreaWidth = size.width;
		me.AreaHeight = size.height;
	}

	me.Down = down;
	me.Up = up;
	me.Count = 0;
	if (me.Down != 0)
		// GetMessageTime() returns LONG and GetDoubleClckTime() returns UINT, which are int32 and uint32, respectively, but we don't need to worry about the signedness because for the same bit widths and two's complement arithmetic, s1-s2 == u1-u2 if bits(s1)==bits(s2) and bits(u1)==bits(u2) (and Windows requires two's complement: http://blogs.msdn.com/b/oldnewthing/archive/2005/05/27/422551.aspx)
		// signedness isn't much of an issue for these calls anyway because http://stackoverflow.com/questions/24022225/what-are-the-sign-extension-rules-for-calling-windows-api-functions-stdcall-t and that we're only using unsigned values (think back to how you (didn't) handle signedness in assembly language) AND because of the above AND because the statistics below (time interval and width/height) really don't make sense if negative
		me.Count = clickCounterClick(&(a->cc), me.Down,
			me.X, me.Y,
			GetMessageTime(), GetDoubleClickTime(),
			GetSystemMetrics(SM_CXDOUBLECLK) / 2,
			GetSystemMetrics(SM_CYDOUBLECLK) / 2);

	// though wparam will contain control and shift state, let's just one function to get modifiers for both keyboard and mouse events; it'll work the same anyway since we have to do this for alt and windows key (super)
	me.Modifiers = getModifiers();

	button = me.Down;
	if (button == 0)
		button = me.Up;
	me.Held1To64 = 0;
	if (button != 1 && (wParam & MK_LBUTTON) != 0)
		me.Held1To64 |= 1 << 0;
	if (button != 2 && (wParam & MK_MBUTTON) != 0)
		me.Held1To64 |= 1 << 1;
	if (button != 3 && (wParam & MK_RBUTTON) != 0)
		me.Held1To64 |= 1 << 2;
	if (button != 4 && (wParam & MK_XBUTTON1) != 0)
		me.Held1To64 |= 1 << 3;
	if (button != 5 && (wParam & MK_XBUTTON2) != 0)
		me.Held1To64 |= 1 << 4;

	// on Windows, we have to capture on drag ourselves
	if (me.Down != 0 && !a->capturing) {
		SetCapture(a->hwnd);
		a->capturing = TRUE;
	}
	// only release capture when all buttons released
	if (me.Up != 0 && a->capturing && me.Held1To64 == 0) {
		// unset flag first as ReleaseCapture() sends WM_CAPTURECHANGED
		a->capturing = FALSE;
		if (ReleaseCapture() == 0)
			logLastError("error releasing capture on drag in areaMouseEvent()");
	}

	(*(a->ah->MouseEvent))(a->ah, a, &me);
}

// we use VK_SNAPSHOT as a sentinel because libui will never support the print screen key; that key belongs to the user
struct extkeymap {
	WPARAM vk;
	uiExtKey extkey;
};

// all mappings come from GLFW - https://github.com/glfw/glfw/blob/master/src/win32_window.c#L152
static const struct extkeymap numpadExtKeys[] = {
	{ VK_HOME, uiExtKeyN7 },
	{ VK_UP, uiExtKeyN8 },
	{ VK_PRIOR, uiExtKeyN9 },
	{ VK_LEFT, uiExtKeyN4 },
	{ VK_CLEAR, uiExtKeyN5 },
	{ VK_RIGHT, uiExtKeyN6 },
	{ VK_END, uiExtKeyN1 },
	{ VK_DOWN, uiExtKeyN2 },
	{ VK_NEXT, uiExtKeyN3 },
	{ VK_INSERT, uiExtKeyN0 },
	{ VK_DELETE, uiExtKeyNDot },
	{ VK_SNAPSHOT, 0 },
};

static const struct extkeymap extKeys[] = {
	{ VK_ESCAPE, uiExtKeyEscape },
	{ VK_INSERT, uiExtKeyInsert },
	{ VK_DELETE, uiExtKeyDelete },
	{ VK_HOME, uiExtKeyHome },
	{ VK_END, uiExtKeyEnd },
	{ VK_PRIOR, uiExtKeyPageUp },
	{ VK_NEXT, uiExtKeyPageDown },
	{ VK_UP, uiExtKeyUp },
	{ VK_DOWN, uiExtKeyDown },
	{ VK_LEFT, uiExtKeyLeft },
	{ VK_RIGHT, uiExtKeyRight },
	{ VK_F1, uiExtKeyF1 },
	{ VK_F2, uiExtKeyF2 },
	{ VK_F3, uiExtKeyF3 },
	{ VK_F4, uiExtKeyF4 },
	{ VK_F5, uiExtKeyF5 },
	{ VK_F6, uiExtKeyF6 },
	{ VK_F7, uiExtKeyF7 },
	{ VK_F8, uiExtKeyF8 },
	{ VK_F9, uiExtKeyF9 },
	{ VK_F10, uiExtKeyF10 },
	{ VK_F11, uiExtKeyF11 },
	{ VK_F12, uiExtKeyF12 },
	// numpad numeric keys and . are handled in common/areaevents.c
	// numpad enter is handled in code below
	{ VK_ADD, uiExtKeyNAdd },
	{ VK_SUBTRACT, uiExtKeyNSubtract },
	{ VK_MULTIPLY, uiExtKeyNMultiply },
	{ VK_DIVIDE, uiExtKeyNDivide },
	{ VK_SNAPSHOT, 0 },
};

static const struct {
	WPARAM vk;
	uiModifiers mod;
} modKeys[] = {
	// even if the separate left/right aren't necessary, have them here anyway, just to be safe
	{ VK_CONTROL, uiModifierCtrl },
	{ VK_LCONTROL, uiModifierCtrl },
	{ VK_RCONTROL, uiModifierCtrl },
	{ VK_MENU, uiModifierAlt },
	{ VK_LMENU, uiModifierAlt },
	{ VK_RMENU, uiModifierAlt },
	{ VK_SHIFT, uiModifierShift },
	{ VK_LSHIFT, uiModifierShift },
	{ VK_RSHIFT, uiModifierShift },
	// there's no combined Windows key virtual-key code as there is with the others
	{ VK_LWIN, uiModifierSuper },
	{ VK_RWIN, uiModifierSuper },
	{ VK_SNAPSHOT, 0 },
};

static int areaKeyEvent(uiArea *a, int up, WPARAM wParam, LPARAM lParam)
{
	uiAreaKeyEvent ke;
	int righthand;
	int i;

	ke.Key = 0;
	ke.ExtKey = 0;
	ke.Modifier = 0;

	ke.Modifiers = getModifiers();

	ke.Up = up;

	// the numeric keypad keys when Num Lock is off are considered left-hand keys as the separate navigation buttons were added later
	// the numeric keypad Enter, however, is a right-hand key because it has the same virtual-key code as the typewriter Enter
	righthand = (lParam & 0x01000000) != 0;
	if (righthand) {
		if (wParam == VK_RETURN) {
			ke.ExtKey = uiExtKeyNEnter;
			goto keyFound;
		}
	} else
		// this is special handling for numpad keys to ignore the state of Num Lock and Shift; see http://blogs.msdn.com/b/oldnewthing/archive/2004/09/06/226045.aspx and https://github.com/glfw/glfw/blob/master/src/win32_window.c#L152
		for (i = 0; numpadExtKeys[i].vk != VK_SNAPSHOT; i++)
			if (numpadExtKeys[i].vk == wParam) {
				ke.ExtKey = numpadExtKeys[i].extkey;
				goto keyFound;
			}

	// okay, those above cases didn't match anything
	// first try the extended keys
	for (i = 0; extKeys[i].vk != VK_SNAPSHOT; i++)
		if (extKeys[i].vk == wParam) {
			ke.ExtKey = extKeys[i].extkey;
			goto keyFound;
		}

	// then try modifier keys
	for (i = 0; modKeys[i].vk != VK_SNAPSHOT; i++)
		if (modKeys[i].vk == wParam) {
			ke.Modifier = modKeys[i].mod;
			// and don't include the key in Modifiers
			ke.Modifiers &= ~ke.Modifier;
			goto keyFound;
		}

	// and finally everything else
	if (fromScancode((lParam >> 16) & 0xFF, &ke))
		goto keyFound;

	// not a supported key, assume unhandled
	// TODO the original code only did this if ke.Modifiers == 0 - why?
	return 0;

keyFound:
	return (*(a->ah->KeyEvent))(a->ah, a, &ke);
}

// We don't handle the standard Windows keyboard messages directly, to avoid both the dialog manager and TranslateMessage().
// Instead, we set up a message filter and do things there.
// That stuff is later in this file.
enum {
	// start at 0x40 to avoid clobbering dialog messages
	msgAreaKeyDown = WM_USER + 0x40,
	msgAreaKeyUp,
};

BOOL areaDoEvents(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
	switch (uMsg) {
	case WM_ACTIVATE:
		// don't keep the double-click timer running if the user switched programs in between clicks
		clickCounterReset(&(a->cc));
		*lResult = 0;
		return TRUE;
	case WM_MOUSEMOVE:
		areaMouseEvent(a, 0, 0, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_LBUTTONDOWN:
		SetFocus(a->hwnd);
		areaMouseEvent(a, 1, 0, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_LBUTTONUP:
		areaMouseEvent(a, 0, 1, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_MBUTTONDOWN:
		SetFocus(a->hwnd);
		areaMouseEvent(a, 2, 0, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_MBUTTONUP:
		areaMouseEvent(a, 0, 2, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_RBUTTONDOWN:
		SetFocus(a->hwnd);
		areaMouseEvent(a, 3, 0, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_RBUTTONUP:
		areaMouseEvent(a, 0, 3, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_XBUTTONDOWN:
		SetFocus(a->hwnd);
		// values start at 1; we want them to start at 4
		areaMouseEvent(a,
			GET_XBUTTON_WPARAM(wParam) + 3, 0,
			GET_KEYSTATE_WPARAM(wParam), lParam);
		*lResult = TRUE;	// XBUTTON messages are different!
		return TRUE;
	case WM_XBUTTONUP:
		areaMouseEvent(a,
			0, GET_XBUTTON_WPARAM(wParam) + 3,
			GET_KEYSTATE_WPARAM(wParam), lParam);
		*lResult = TRUE;	// XBUTTON messages are different!
		return TRUE;
	case WM_CAPTURECHANGED:
		if (a->capturing) {
			a->capturing = FALSE;
			(*(a->ah->DragBroken))(a->ah, a);
		}
		*lResult = 0;
		return TRUE;
	case msgAreaKeyDown:
		*lResult = (LRESULT) areaKeyEvent(a, 0, wParam, lParam);
		return TRUE;
	case msgAreaKeyUp:
		*lResult = (LRESULT) areaKeyEvent(a, 1, wParam, lParam);
		return TRUE;
	}
	return FALSE;
}

static HHOOK areaFilter;

// TODO affect visibility properly
// TODO what did this mean
static LRESULT CALLBACK areaFilterProc(int code, WPARAM wParam, LPARAM lParam)
{
	MSG *msg = (MSG *) lParam;
	LRESULT handled;

	if (code < 0)
		goto callNext;

	// is the recipient an area?
	if (msg->hwnd == NULL)		// this can happen; for example, WM_TIMER
		goto callNext;
	if (windowClassOf(msg->hwnd, areaClass, NULL) != 0)
		goto callNext;		// nope

	handled = 0;
	switch (msg->message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		handled = SendMessageW(msg->hwnd, msgAreaKeyDown, msg->wParam, msg->lParam);
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		handled = SendMessageW(msg->hwnd, msgAreaKeyUp, msg->wParam, msg->lParam);
		break;
	// otherwise handled remains 0, as we didn't handle this
	}
	if (!handled)
		goto callNext;

	// we handled it; discard the message so the dialog manager doesn't see it
	return 1;

callNext:
	return CallNextHookEx(areaFilter, code, wParam, lParam);
}

int registerAreaFilter(void)
{
	areaFilter = SetWindowsHookExW(WH_MSGFILTER,
		areaFilterProc,
		hInstance,
		GetCurrentThreadId());
	return areaFilter != NULL;
}

void unregisterAreaFilter(void)
{
	if (UnhookWindowsHookEx(areaFilter) == 0)
		logLastError("error unregistering uiArea message filter in unregisterAreaFilter()");
}
