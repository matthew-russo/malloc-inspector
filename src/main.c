#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BUFSIZE 1024

#define PERMS_READ    0b10000
#define PERMS_WRITE   0b01000
#define PERMS_EXEC    0b00100
#define PERMS_SHARED  0b00010
#define PERMS_PRIVATE 0b00001

struct memory_map_entry
{
  uintptr_t mem_start;
  uintptr_t mem_end;
  uint8_t perms;
  uint64_t offset;
  uint8_t device_major;
  uint8_t device_minor;
  uint64_t inode;
  char * path;
};

// statically allocate space so we can do some processing on memory maps
// without using malloc, which may in turn update the memory maps
char char_buf_1[BUFSIZE][BUFSIZE];
char char_buf_2[BUFSIZE][BUFSIZE];

struct memory_map_entry mem_map_buf_1[BUFSIZE];
struct memory_map_entry mem_map_buf_2[BUFSIZE];

// reads a file line by line in to a provided buffer
int read_file_lines(char buf[BUFSIZE][BUFSIZE], char * file_name)
{
  FILE * f = fopen(file_name, "r");

  if (f)
  {
    int idx = 0;
    while (fgets(buf[idx], BUFSIZE, f))
    {
      // ditch the trailing newline
      buf[idx][strcspn(buf[idx], "\n")] = 0;
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

int parse_memory_map(char lines[BUFSIZE][BUFSIZE], struct memory_map_entry mem_map_buf[BUFSIZE])
{
  int lines_read = read_file_lines(lines, "/proc/self/maps");
  for (int i = 0; i < lines_read; i++)
  {
    char *addr_range_tok, *perms_tok, *offset_tok, *device_major_minor_tok, *inode_tok, *path_tok;

    addr_range_tok = strtok(lines[i], " ");
    if (addr_range_tok == NULL) {
      printf("addr range tok not present");
      return -1;
    }

    perms_tok = strtok(NULL, " ");
    if (perms_tok == NULL) {
      puts("perms tok not present");
      return -1;
    }

    offset_tok = strtok(NULL, " ");
    if (offset_tok == NULL) {
      puts("offset tok not present");
      return -1;
    }

    device_major_minor_tok = strtok(NULL, " ");
    if (offset_tok == NULL) {
      puts("device major minor tok not present");
      return -1;
    }

    inode_tok = strtok(NULL, " ");
    if (inode_tok == NULL) {
      puts("inode range tok not present");
      return -1;
    }

    // path can be null
    path_tok = strtok(NULL, " ");


    char *mem_start_tok, *mem_end_tok;
    mem_start_tok = strtok(addr_range_tok, "-");
    mem_end_tok = strtok(addr_range_tok, "-");
    if (mem_start_tok == NULL || mem_end_tok == NULL)
    {
      puts("addr range was not hyphen separate");
      return -1;
    }

    char *device_major_tok, *device_minor_tok;
    device_major_tok = strtok(device_major_minor_tok, ":");
    device_minor_tok = strtok(device_major_minor_tok, ":");
    if (mem_start_tok == NULL || mem_end_tok == NULL)
    {
      puts("addr range was not hyphen separate");
      return -1;
    }

    uint8_t perms = 0;
    if (perms_tok[0] == 'r')
    {
      perms |= PERMS_READ;
    }
    if (perms_tok[1] == 'w')
    {
      perms |= PERMS_WRITE;
    }
    if (perms_tok[2] == 'x')
    {
      perms |= PERMS_EXEC;
    }
    if (perms_tok[3] == 's')
    {
      perms |= PERMS_SHARED;
    }
    if (perms_tok[3] == 'p')
    {
      perms |= PERMS_PRIVATE;
    }

    mem_map_buf[i] = (struct memory_map_entry) {
      .mem_start = strtol(mem_start_tok, NULL, 16),
      .mem_end = strtol(mem_end_tok, NULL, 16),
      .perms = perms,
      .offset = strtol(offset_tok, NULL, 10),
      .device_major = strtol(device_major_tok, NULL, 16),
      .device_minor = strtol(device_minor_tok, NULL, 16),
      .inode = strtol(inode_tok, NULL, 10),
      .path = path_tok
    };
  }
  return lines_read;
}

int main(int argc, char *argv[])
{
  int num_entries = parse_memory_map(char_buf_1, mem_map_buf_1);
  if (num_entries == -1)
  {
    puts("failed to parse memory map");
  }

  for (int i = 0; i < num_entries; i++)
  {
    printf(
        "mem_start: %p, mem_end: %p, perms: %d, offset: %llu, device_major: %d, device_minor: %d, inode: %llu, path: '%s'\n",
        mem_map_buf_1[i].mem_start,
        mem_map_buf_1[i].mem_end,
        mem_map_buf_1[i].perms,
        mem_map_buf_1[i].offset,
        mem_map_buf_1[i].device_major,
        mem_map_buf_1[i].device_minor,
        mem_map_buf_1[i].inode,
        mem_map_buf_1[i].path
    );
  }
  return 0;
}
