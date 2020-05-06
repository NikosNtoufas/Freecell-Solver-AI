#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// Number of max ranking
//#define N 7

void  stringToLower(char *str)
{

    for(int i = 0; str[i]; i++){
      str[i] = tolower(str[i]);
    }
}
