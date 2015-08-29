#include <stdio.h>
#define GX_STATIC
#define GX_DEFINE
#include "gx.h"

#define XRES 640
#define YRES 480
static int buf[XRES*YRES];

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
        if (ev.key == GX_key_esc)
          goto quit;
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
