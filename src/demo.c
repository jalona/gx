#include <stdio.h>
#include <assert.h>
#define GX_STATIC
#define GX_DEFINE
#include "gx.h"

#define TICKRATE 60
#define XRES    640
#define YRES    480

static int buf[XRES*YRES];
static int maxfps = 60;
static int limitfps;
static int pmouse;

static void print_key(int key, int state)
{
  int i;

#define X(k) case k: printf("[%d]: %s\n", state, #k); break
  switch (key) {
  X(GX_key_esc); X(GX_key_shift); X(GX_key_ctrl); X(GX_key_alt);
  X(GX_key_up); X(GX_key_down); X(GX_key_left); X(GX_key_right);
  X(GX_key_ins); X(GX_key_del); X(GX_key_home); X(GX_key_end);
  X(GX_key_pgup); X(GX_key_pgdn); X(GX_key_f1); X(GX_key_f2);
  X(GX_key_f3); X(GX_key_f4); X(GX_key_f5); X(GX_key_f6); X(GX_key_f7);
  X(GX_key_f8); X(GX_key_f9); X(GX_key_f10); X(GX_key_f11); X(GX_key_f12);
  X(GX_key_mb1); X(GX_key_mb2); X(GX_key_mb3); X(GX_key_mb4); X(GX_key_mb5);
#undef X
#define X(k) case k: printf("[%d]: %s\n", state, #k); break
  X('\b'); X('\t'); X('\r'); X(' ');
  }
  for (i='0'; i<='9'; ++i)
    if (key == i) printf("[%d]: '%c'\n", state, i);
  for (i='a'; i<='z'; ++i)
    if (key == i) printf("[%d]: '%c'\n", state, i);
}

static void draw(double t)
{
  int x, y, *p = buf, ms = (int)(t*1000.0);
  for (y=0; y<YRES; ++y) {
    int z = y*y + ms;
    for (x=0; x<XRES; ++x)
      p[x] = x*x + z;
    p += XRES;
  }
}

static void print_char(int ch)
{
  printf("char: %c\n", ch);
}

static void print_time(int ticks, int *tps, double tnow, double dt)
{
  static double ddt, msf;
  static int cfps, fps, ctps;

  ++fps;

  ddt += dt;
  if (ddt >= 1.0) {
    ddt -= 1.0;
    cfps = fps, fps = 0;
    ctps = *tps, *tps = 0;
    msf = 1000.0*dt;
    assert(ctps == TICKRATE);
    printf("%.2f ms [%4d fps, %d tps] @ t = %3.2f:%d\n",
           msf, cfps, ctps, tnow, ticks);
  }
}

int main(void)
{
  static const double tstep = 1.0/TICKRATE;
  double tlast, tnow, tsec, dt, ddt=0.0;
  int ticks=0, tps=0, frames=0;
  gx_event ev;

#if 0
  {
    int i;
    for (i=0; i<100; ++i) {
      gx_init("demo", XRES, YRES);
      gx_delay(0.01);
      gx_exit();
    }
  }
#endif
  gx_init("demo", XRES, YRES);
  tlast = gx_time();
  while (1) {
    tnow = gx_time();
    dt = tnow - tlast;
    tlast = tnow;

    while (gx_poll(&ev)) {
      switch (ev.type) {
      case GX_ev_quit:
        goto quit;
      case GX_ev_keydown:
        print_key(ev.key, 1);
        if (ev.key == GX_key_esc)
          goto quit;
        else if (ev.key == 'f') {
          limitfps ^= 1;
          printf("limitfps: %d\n", limitfps);
        } else if (ev.key == 'm') {
          pmouse ^= 1;
          printf("pmouse: %d\n", pmouse);
        }
        break;
      case GX_ev_keyup:
        print_key(ev.key, 0);
        break;
      case GX_ev_keychar:
        print_char(ev.key);
        break;
      case GX_ev_mouse:
        if (pmouse)
          printf("mouse: %3d,%3d\n", (int)(ev.mx*XRES), (int)(ev.my*YRES));
        break;
      }
    }

    ddt += dt;
    while (ddt >= tstep) {
      ddt -= tstep;
      ++ticks;
      ++tps;
    }

    draw(tnow);
    gx_paint(buf, XRES, YRES);
    ++frames;
    print_time(ticks, &tps, tnow, dt);
    if (limitfps && maxfps) {
      double tmax = 1.0 / maxfps;
      while (gx_time() - tnow < tmax-0.002) gx_delay(0.001);
      while (gx_time() - tnow < tmax);
    }
  }
quit:
  tsec = gx_time();
  printf("--- timing report ---\n"
         "ticks    : %d\n"
         "frames   : %d\n"
         "time     : %.2f\n"
         "tickrate : %.2f\n"
         "fps      : %.2f\n",
         ticks, frames, tsec, ticks/tsec, frames/tsec);
  return 0;
}
