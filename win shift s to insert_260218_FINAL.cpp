#define _WIN32_WINNT 0x0601
#include <windows.h>

HHOOK hhkLowLevelKybd;

/* ── 상태 머신 ─────────────────────────────────────────────── */
typedef enum { ST_IDLE, ST_WIN, ST_WIN_SHIFT } State;
static State gState        = ST_IDLE;
static DWORD gPendingStart = 0;   /* Win keydown 시각 */
static DWORD gStormStart   = 0;

static const DWORD THRESHOLD = 80;   /* ms: 하드웨어 판별 임계값 */
static const DWORD STORM_DUR = 200;  /* ms: 여진 억제 기간       */

/* ── 이벤트 버퍼 ────────────────────────────────────────────── */
#define BUF_MAX 16
typedef struct { WPARAM wp; KBDLLHOOKSTRUCT kbd; } BufEv;
static BufEv gBuf[BUF_MAX];
static int   gBufN = 0;

static void BufAdd(WPARAM wp, const KBDLLHOOKSTRUCT *p) {
    if (gBufN < BUF_MAX) { gBuf[gBufN].wp = wp; gBuf[gBufN].kbd = *p; gBufN++; }
}

/* 사람 입력 확정 → 차단했던 이벤트를 injected 로 재전송 */
static void BufReplay(void) {
    for (int i = 0; i < gBufN; i++) {
        INPUT in = {0};
        in.type       = INPUT_KEYBOARD;
        in.ki.wVk     = gBuf[i].kbd.vkCode;
        in.ki.wScan   = gBuf[i].kbd.scanCode;
        in.ki.dwFlags = 0;
        if (gBuf[i].wp == WM_KEYUP || gBuf[i].wp == WM_SYSKEYUP)
            in.ki.dwFlags |= KEYEVENTF_KEYUP;
        if (gBuf[i].kbd.flags & LLKHF_EXTENDED)
            in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &in, sizeof(INPUT));
    }
    gBufN  = 0;
    gState = ST_IDLE;
}

/* 하드웨어 키 확정 → 버퍼 파기 (Win/Shift 는 버린다) */
static void BufDiscard(void) {
    gBufN  = 0;
    gState = ST_IDLE;
}

/* Insert 전송 */
static void SendInsert(void) {
    INPUT ins[2] = {0};
    ins[0].type       = INPUT_KEYBOARD;
    ins[0].ki.wVk     = VK_INSERT;
    ins[1].type       = INPUT_KEYBOARD;
    ins[1].ki.wVk     = VK_INSERT;
    ins[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, ins, sizeof(INPUT));
}

/* ── 훅 본체 ────────────────────────────────────────────────── */
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION)
        return CallNextHookEx(hhkLowLevelKybd, nCode, wParam, lParam);

    KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
    DWORD cur = p->time;

    /* 우리가 보낸(injected) 이벤트는 무조건 통과 */
    if (p->flags & LLKHF_INJECTED)
        return CallNextHookEx(hhkLowLevelKybd, nCode, wParam, lParam);

    BOOL dn    = (wParam == WM_KEYDOWN   || wParam == WM_SYSKEYDOWN);
    BOOL up    = (wParam == WM_KEYUP     || wParam == WM_SYSKEYUP);
    BOOL isWin = (p->vkCode == VK_LWIN   || p->vkCode == VK_RWIN);
    BOOL isSh  = (p->vkCode == VK_LSHIFT || p->vkCode == VK_RSHIFT);
    BOOL isS   = (p->vkCode == 'S');

    /*
     * 폭풍 억제:
     * 하드웨어 키 처리 직후 들어오는 Win/Shift/S 잔여 이벤트를 모두 차단.
     * keydown 뿐 아니라 keyup 도 막는다 ─ 우리가 Win/Shift 를 시스템에
     * 전달한 적이 없으므로 keyup 이 올라가면 상태가 꼬인다.
     */
    if (gStormStart && (cur - gStormStart < STORM_DUR)) {
        if (isWin || isSh || isS)
            return 1;
    }

    /*
     * goto retry:
     * 타임아웃으로 BufReplay() 를 호출한 뒤, 현재 이벤트를
     * ST_IDLE 상태로 다시 평가해야 하는 경우에 사용.
     * (예: 타임아웃 직후 들어온 이벤트가 또 Win keydown 인 경우)
     */
retry:
    switch (gState) {

    /* ── ST_IDLE ─────────────────────────────────────────── */
    case ST_IDLE:
        if (isWin && dn) {
            gPendingStart = cur;
            BufAdd(wParam, p);
            gState = ST_WIN;
            return 1;               /* ★ Win keydown 선제 차단 */
        }
        break;

    /* ── ST_WIN: Win keydown 을 버퍼에 잡아둔 상태 ────────── */
    case ST_WIN:
        if (cur - gPendingStart > THRESHOLD) { BufReplay(); goto retry; }

        if (isWin && dn) { BufAdd(wParam, p); return 1; }

        /* Win 단독 press/release (시작 메뉴 등): 버퍼 재전송 */
        if (isWin && up) { BufAdd(wParam, p); BufReplay(); return 1; }

        /* Win 직후 Shift → 하드웨어 키 후보 */
        if (isSh  && dn) { BufAdd(wParam, p); gState = ST_WIN_SHIFT; return 1; }

        /* 그 외 키 → 사람 입력 확정, 버퍼 재전송 후 현재 이벤트 통과 */
        BufReplay();
        break;

    /* ── ST_WIN_SHIFT: Win+Shift 를 버퍼에 잡아둔 상태 ─────── */
    case ST_WIN_SHIFT:
        if (cur - gPendingStart > THRESHOLD) { BufReplay(); goto retry; }

        if ((isWin || isSh) && dn) { BufAdd(wParam, p); return 1; }

        /* 수식키가 S 이전에 올라옴 → 사람 입력 */
        if ((isWin || isSh) && up) { BufAdd(wParam, p); BufReplay(); return 1; }

        /* ★ S keydown = 하드웨어 키 확정! */
        if (isS && dn) {
            gStormStart = cur;
            BufDiscard();           /* Win+Shift 이벤트 파기         */
            SendInsert();           /* 현재 눌린 수식키(Ctrl 등)+Insert */
            return 1;               /* S 차단                         */
        }

        /* 그 외 키 → 사람 입력 확정 */
        BufReplay();
        break;
    }

    return CallNextHookEx(hhkLowLevelKybd, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    hhkLowLevelKybd = SetWindowsHookEx(
        WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(hhkLowLevelKybd);
    return 0;
}