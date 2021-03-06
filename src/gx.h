/*
gx.h - public domain
Written by Mårten Hansson <hmrten@gmail.com> in 2015.

Simple library for writing small graphical demos.
  - Provides basic keyboard/mouse handling and a highres timer.
  - Assumes 32 bit BGRX pixel format (0xRRGGBB in little-endian).

-- Documentation

  #define GX_DEFINE    - define this in exactly one source file
  #define GX_STATIC    - (optional) make all functions static

  You must also define one of these
  #define GX_W32       - use W32 implementation
  #define GX_X11       - use X11 implementation

  gx_init(title, w, h) - initialize and open a window with dimension w*h
  gx_exit()            - cleanup.
  gx_poll()            - read an event, returns 0 when no more events.
  gx_paint(buf, w, h)  - draw buf consisting of w*h 32 bit pixels onto window,
                         will stretch to fit window size.
  gx_time()            - return time in seconds since gx_init().
  gx_delay(s)          - sleep for at least s seconds.
*/

#ifdef GX_STATIC
#define GXDEF static
#else
#define GXDEF extern
#endif

enum {
  /* event types */
  GX_ev_quit = 1, GX_ev_keydown, GX_ev_keyup, GX_ev_keychar, GX_ev_mouse,

  /* keycodes ('\b', '\t', '\r', ' ', '0'..'9', 'a'..'z') */
  GX_key_esc = 27, GX_key_shift = 128, GX_key_ctrl, GX_key_alt,
  GX_key_up, GX_key_down, GX_key_left, GX_key_right, GX_key_ins, GX_key_del,
  GX_key_home, GX_key_end, GX_key_pgup, GX_key_pgdn, GX_key_f1,
  GX_key_f2, GX_key_f3, GX_key_f4, GX_key_f5, GX_key_f6, GX_key_f7,
  GX_key_f8, GX_key_f9, GX_key_f10, GX_key_f11, GX_key_f12, GX_key_mb1,
  GX_key_mb2, GX_key_mb3, GX_key_mb4, GX_key_mb5
};

typedef struct {
  int   type;
  int   key;
  float mx;
  float my;
} gx_event;

GXDEF void   gx_init(const char *title, int w, int h);
GXDEF void   gx_exit(void);
GXDEF int    gx_poll(gx_event *ev);
GXDEF void   gx_paint(const void *buf, int w, int h);
GXDEF double gx_time(void);
GXDEF void   gx_delay(double t);

#ifdef GX_DEFINE
#ifdef GX_W32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <mmsystem.h>

#ifdef _MSC_VER
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#endif

#define GX_w32_wm_exit (WM_USER+1)

#define gx_w32_fatal(s) (MessageBoxA(0, s, "Fatal", 0), ExitProcess(1))
#define gx_w32_assert(p, s) ((p) ? (void)0 : gx_w32_fatal(s))

static struct {
  CRITICAL_SECTION cs;
  HANDLE           th;
  DWORD            tid;
  DWORD            init;
  DWORD            winrdy;
  int              winsize;
  LARGE_INTEGER    tbase;
  double           tmulf;
  WNDCLASSA        wc;
  HWND             win;
  HDC              dc;
  DWORD            qhead;
  gx_event         qdata[256];
  DWORD            qtail;
} gx_w32;

/* single-producer/single-consumer queue, only uses a compiler barriers
   because x86 has acquire/release semantics on regular loads and stores */
static int gx_w32_qwrite(const gx_event *ev)
{
  DWORD head = gx_w32.qhead, tail = gx_w32.qtail, ntail = (tail+1) & 255;
  _ReadWriteBarrier();
  if (ntail == head)
    return 0;
  gx_w32.qdata[tail] = *ev;
  _ReadWriteBarrier();
  gx_w32.qtail = ntail;
  return 1;
}

static int gx_w32_qread(gx_event *ev)
{
  DWORD head = gx_w32.qhead, tail = gx_w32.qtail;
  _ReadWriteBarrier();
  if (head == tail)
    return 0;
  *ev  = gx_w32.qdata[head];
  _ReadWriteBarrier();
  gx_w32.qhead = (head+1) & 255;
  return 1;
}

static void gx_w32_adjsize(int *w, int *h)
{
  RECT r = { 0, 0, *w, *h };
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
  *w = r.right - r.left;
  *h = r.bottom - r.top;
}

static const unsigned char gx_w32_vkmap[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, '\b', '\t', 0, 0, 0, '\r', 0, 0,
  GX_key_shift, GX_key_ctrl, GX_key_alt, 0, 0, 0, 0, 0, 0, 0, 0,
  GX_key_esc, 0, 0, 0, 0, ' ', GX_key_pgup, GX_key_pgdn, GX_key_end,
  GX_key_home, GX_key_left, GX_key_up, GX_key_right, GX_key_down,
  0, 0, 0, 0, GX_key_ins, GX_key_del, 0, '0', '1', '2', '3', '4', '5',
  '6', '7', '8', '9', 0, 0, 0, 0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
  'u', 'v', 'w', 'x', 'y', 'z', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, GX_key_f1, GX_key_f2, GX_key_f3, GX_key_f4, GX_key_f5,
  GX_key_f6, GX_key_f7, GX_key_f8, GX_key_f9, GX_key_f10, GX_key_f11,
  GX_key_f12
};

static LRESULT CALLBACK gx_w32_winproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
  int w, h;
  gx_event ev = {0};

  switch (msg) {
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:
    if ((lp&(1<<29)) && !(lp&(1<<31)) && wp == VK_F4) {
      PostQuitMessage(0);
      return 0;
    }
  case WM_KEYUP:
  case WM_KEYDOWN:
    if (!(lp&(1<<30)) == !!(lp&(1<<31)))
      return 0;
    ev.type = (lp&(1<<31)) ? GX_ev_keyup : GX_ev_keydown;
    if ((ev.key = gx_w32_vkmap[wp & 255]))
      goto post_event;
    return 0;
  case WM_CHAR:
    ev.type = GX_ev_keychar;
    ev.key = (int)wp;
    goto post_event;
  case WM_LBUTTONUP:   ev.key = GX_key_mb1; goto btn_up;
  case WM_MBUTTONUP:   ev.key = GX_key_mb2; goto btn_up;
  case WM_RBUTTONUP:   ev.key = GX_key_mb3; goto btn_up;
  case WM_LBUTTONDOWN: ev.key = GX_key_mb1; goto btn_down;
  case WM_MBUTTONDOWN: ev.key = GX_key_mb2; goto btn_down;
  case WM_RBUTTONDOWN: ev.key = GX_key_mb3; goto btn_down;
  case WM_MOUSEWHEEL:
    ev.key = ((int)wp>>16) / WHEEL_DELTA > 0 ? GX_key_mb4 : GX_key_mb5;
    ev.type = GX_ev_keydown;
    gx_w32_qwrite(&ev);
    ev.type = GX_ev_keyup;
    gx_w32_qwrite(&ev);
    return 0;
  case WM_MOUSEMOVE:
    w = gx_w32.winsize & 0xffff;
    h = gx_w32.winsize >> 16;
    ev.type = GX_ev_mouse;
    ev.mx = (int)(short)LOWORD(lp) / (float)w;
    ev.my = (int)(short)HIWORD(lp) / (float)h;
    goto post_event;
  case WM_SIZE:
    _ReadWriteBarrier();
    gx_w32.winsize = (int)lp;
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_CLOSE:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hw, msg, wp, lp);
btn_up:
  ev.type = GX_ev_keyup;
  goto post_event;
btn_down:
  ev.type = GX_ev_keydown;
post_event:
  gx_w32_qwrite(&ev);
  return 0;
}

static DWORD WINAPI gx_w32_winthrd(void *arg)
{
  char *title = (char *)((INT_PTR *)arg)[0];
  int w = (int)(((INT_PTR *)arg)[1] & 0xffff);
  int h = (int)(((INT_PTR *)arg)[1] >> 16);
  MSG msg;
  gx_event ev;

  gx_w32.wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
  gx_w32.wc.lpfnWndProc = gx_w32_winproc;
  gx_w32.wc.hInstance = GetModuleHandleW(0);
  gx_w32.wc.hIcon = LoadIcon(0, IDI_APPLICATION);
  gx_w32.wc.hCursor = LoadCursor(0, IDC_ARROW);
  gx_w32.wc.lpszClassName = "gx";
  if (!RegisterClassA(&gx_w32.wc))
    gx_w32_fatal("RegisterClass");

  gx_w32_adjsize(&w, &h);
  gx_w32.win = CreateWindowA("gx", title, WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT, w, h, 0, 0,
                             gx_w32.wc.hInstance, 0);
  gx_w32_assert(gx_w32.win, "CreateWindow");
  gx_w32.dc = GetDC(gx_w32.win);
  gx_w32_assert(gx_w32.dc, "GetDC");

  _ReadWriteBarrier();
  gx_w32.winrdy = 1;

  while (GetMessage(&msg, 0, 0, 0) > 0) {
    if (msg.message == GX_w32_wm_exit)
      break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  ev.type = GX_ev_quit;
  gx_w32_qwrite(&ev);

  EnterCriticalSection(&gx_w32.cs);
    ReleaseDC(gx_w32.win, gx_w32.dc);
    DestroyWindow(gx_w32.win);
    UnregisterClassA("gx", gx_w32.wc.hInstance);
    gx_w32.win = 0;
    gx_w32.dc = 0;
    gx_w32.winrdy = 0;
    gx_w32.winsize = 0;
  LeaveCriticalSection(&gx_w32.cs);

  return 0;
}

GXDEF void gx_init(const char *title, int w, int h)
{
  INT_PTR arg[2] = { (INT_PTR)title, (h<<16) | w };
  LARGE_INTEGER freq;

  InitializeCriticalSection(&gx_w32.cs);
  gx_w32.th = CreateThread(0, 0, gx_w32_winthrd, arg, 0, &gx_w32.tid);
  gx_w32_assert(gx_w32.th, "CreateThread");
  timeBeginPeriod(1);
  QueryPerformanceFrequency(&freq);
  gx_w32.tmulf = 1.0 / (double)freq.QuadPart;
  while (!gx_w32.winrdy) {
    _ReadWriteBarrier();
    _mm_pause();
  }
  QueryPerformanceCounter(&gx_w32.tbase);
  gx_w32.init = 1;
}

GXDEF void gx_exit(void)
{
  if (!gx_w32.init)
    return;
  PostThreadMessage(gx_w32.tid, GX_w32_wm_exit, 0, 0);
  WaitForSingleObject(gx_w32.th, INFINITE);
  CloseHandle(gx_w32.th);
  DeleteCriticalSection(&gx_w32.cs);
  gx_w32.qhead = gx_w32.qtail = 0;
  gx_w32.init = 0;
}

GXDEF int gx_poll(gx_event *ev)
{
  if (!gx_w32.init)
    return 0;
  return gx_w32_qread(ev);
}

GXDEF void gx_paint(const void *buf, int w, int h)
{
  static BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, BI_RGB};
  int dw = gx_w32.winsize, dh = dw >> 16;
  _ReadWriteBarrier();
  dw &= 0xffff;
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -h;
  if (gx_w32.init && TryEnterCriticalSection(&gx_w32.cs)) {
    StretchDIBits(gx_w32.dc, 0, 0, dw, dh, 0, 0, w, h, buf, &bmi,
                  DIB_RGB_COLORS, SRCCOPY);
    ValidateRect(gx_w32.win, 0);
    LeaveCriticalSection(&gx_w32.cs);
  }
}

GXDEF double gx_time(void)
{
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return (double)(t.QuadPart-gx_w32.tbase.QuadPart) * gx_w32.tmulf;
}

GXDEF void gx_delay(double t)
{
  if (t > 0.0)
    Sleep((DWORD)(t*1000.0));
}

#else
#error must define GX_W32
#endif
#endif
