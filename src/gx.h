/*
 * gx.h - v1.0
 * Copyright 2015 Mårten Hansson, licensed under the ISC License.
 *
 * Minimal library for writing graphical demos.
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
  GX_key_home, GX_key_end, GX_key_pgup, GX_key_pgdn, GX_key_fn,
  GX_key_mb1 = GX_key_fn+13, GX_key_mb2, GX_key_mb3, GX_key_mb4, GX_key_mb5
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
  WNDCLASS         wc;
  BITMAPINFO       bmi;
  HWND             win;
  HDC              dc;
  DWORD            qhead;
  gx_event         qdata[256];
  DWORD            qtail;
} gx_w32;

static void gx_w32_adjsize(int *w, int *h)
{
  RECT r = { 0, 0, *w, *h };
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
  *w = r.right - r.left;
  *h = r.bottom - r.top;
}

static unsigned char gx_w32_vkmap[256];
static LRESULT CALLBACK gx_w32_winproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
  switch (msg) {
  case WM_SIZE:
    _ReadWriteBarrier();
    gx_w32.winsize = (int)lp;
    return 0;
  case WM_CLOSE:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hw, msg, wp, lp);
}

static DWORD WINAPI gx_w32_winthrd(void *arg)
{
  char *title = (char *)((INT_PTR *)arg)[0];
  int w = (int)(((INT_PTR *)arg)[1] & 0xffff);
  int h = (int)(((INT_PTR *)arg)[1] >> 16);
  MSG msg;

  gx_w32.wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
  gx_w32.wc.lpfnWndProc = gx_w32_winproc;
  gx_w32.wc.hInstance = GetModuleHandle(0);
  gx_w32.wc.hIcon = LoadIcon(0, IDI_APPLICATION);
  gx_w32.wc.hCursor = LoadCursor(0, IDC_ARROW);
  gx_w32.wc.lpszClassName = "gx";
  RegisterClass(&gx_w32.wc);

  gx_w32_adjsize(&w, &h);
  gx_w32.win = CreateWindow("gx", title, WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT, w, h, 0, 0,
                            gx_w32.wc.hInstance, 0);
  gx_w32_assert(gx_w32.win, "CreateWindow");
  gx_w32.dc = GetDC(gx_w32.win);
  gx_w32_assert(gx_w32.dc, "GetDC");

  _ReadWriteBarrier();
  gx_w32.winrdy = 1;

  while (GetMessage(&msg, 0, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  EnterCriticalSection(&gx_w32.cs);
    ReleaseDC(gx_w32.win, gx_w32.dc);
    DestroyWindow(gx_w32.win);
    UnregisterClass("gx", gx_w32.wc.hInstance);
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
  while (!gx_w32.winrdy) {
    _ReadWriteBarrier();
    _mm_pause();
  }
  timeBeginPeriod(1);
  QueryPerformanceCounter(&gx_w32.tbase);
  QueryPerformanceFrequency(&freq);
  gx_w32.tmulf = 1.0 / (double)freq.QuadPart;
  gx_w32.init = 1;
}

GXDEF void gx_exit(void)
{
  gx_w32.init = 0;
}

GXDEF int gx_poll(gx_event *ev)
{
  return 0;
}

GXDEF void gx_paint(const void *buf, int w, int h)
{
  static BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, BI_RGB};
  int dw, dh;

  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -h;
  if (gx_w32.init && TryEnterCriticalSection(&gx_w32.cs)) {
    dw = gx_w32.winsize;
    _ReadWriteBarrier();
    dh = dw >> 16;
    dw &= 0xffff;
    StretchDIBits(gx_w32.dc, 0, 0, dw, dh, 0, 0, w, h, buf, &bmi,
                  DIB_RGB_COLORS, SRCCOPY);
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
