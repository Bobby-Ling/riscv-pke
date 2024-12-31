/*
 * header file to be used by applications.
 */

int printu(const char *s, ...);
int exit(int code);

// added @lab1_challenge1_backtrace
int print_backtrace(int level);