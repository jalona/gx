#include <stdio.h>
#define GX_STATIC
#define GX_DEFINE
#include "gx.h"

#define XRES 640
#define YRES 480
static int buf[XRES*YRES];

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
  /*X(GX_key_mb1); X(GX_key_mb2); X(GX_key_mb3); X(GX_key_mb4); X(GX_key_mb5*/
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

int main(void)
{
  gx_event ev;
  gx_init("demo", XRES, YRES);
  while (1) {
    while (gx_poll(&ev)) {
      switch (ev.type) {
      case GX_ev_quit:
        goto quit;
      case GX_ev_keydown:
        print_key(ev.key, 1);
        if (ev.key == GX_key_esc)
          goto quit;
        break;
      case GX_ev_keyup:
        print_key(ev.key, 0);
        break;
      case GX_ev_keychar:
        print_char(ev.key);
        break;
      case GX_ev_mouse:
        printf("mouse: %3d,%3d\n", (int)(ev.mx*XRES), (int)(ev.my*YRES));
        break;
      }
    }
    draw(gx_time());
    gx_paint(buf, XRES, YRES);
    gx_delay(0.001);
  }
quit:
  return 0;
}
