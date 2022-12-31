#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BUFSIZE 1024

// statically allocate space so we can do some processing on memory maps
// without using malloc, which may in turn update the memory maps
char buf_1[BUFSIZE][BUFSIZE];
char buf_2[BUFSIZE][BUFSIZE];

// reads a file line by line in to a provided buffer
int read_file_lines(char buf[BUFSIZE][BUFSIZE], char * file_name)
{
  FILE * f = fopen(file_name, "r");

  if (f)
  {
    int idx = 0;
    while (fgets(buf[idx], BUFSIZE, f))
    {
      idx++;
    }
    fclose (f);
    return idx;
  }
  else
  {
    printf("File '%s' not present\n", file_name);
    return -1;
  }
}

int main(int argc, char *argv[])
{
  int lines_read = read_file_lines(buf_1, "/proc/self/maps");
  for (int i = 0; i < lines_read; i++)
  {
    printf("%s", buf_1[i]);
  }
  return 0;
}
