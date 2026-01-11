#ifndef FAIL_HEADER
#define FAIL_HEADER

#define FAIL fail(__LINE__, __FILE__)

extern int line;
extern int line_col;
extern char *file;

void fail(int line, char *file);

#endif
