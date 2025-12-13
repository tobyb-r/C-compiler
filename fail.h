#ifndef FAIL_HEADER
#define FAIL_HEADER

#define FAIL fail(__LINE__, __FILE__)

extern int line;
extern char *file;

void fail(int line, char *file);

#endif
