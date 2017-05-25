
#ifndef _CAPTURE_H
#define _CAPTURE_H

int capture_v4l2getinfo(int argc, char* argv[])
int capture_v4l2simple(int argc, char* argv[]);
int capture_saveyuv(int argc, char* argv[]);
int capture_savemjpeg(int argc, char* argv[]);
int capture_sdl(int argc, char* argv[]);
int capture_fb(int argc, char* argv[]);


#endif  /* _CAPTURE_H */
