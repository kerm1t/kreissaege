#if !defined(GLOBAL__TINYPTC_TEXT_)
#define GLOBAL__TINYPTC_TEXT_

void pset(int x, int y, short r, short g, short b);
void loadText(int *scr, int sW, int sH);
//void doText(int x, int y, char *str, int strLen);
void doText(int x, int y, std::string stri);
void doTextCol(int x, int y, std::string stri, short r, short g, short b);

#endif // !defined(GLOBAL__TINYPTC_TEXT_)
