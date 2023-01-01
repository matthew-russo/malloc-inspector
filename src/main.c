#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

void print_memory_map_entry(struct memory_map_entry e)
{
  printf(
      "memory_map_entry { mem_start: %p, mem_end: %p, perms: %d, offset: %llu, device_major: %d, device_minor: %d, inode: %llu, path: '%s' }\n",
      e.mem_start,
      e.mem_end,
      e.perms,
      e.offset,
      e.device_major,
      e.device_minor,
      e.inode,
      e.path
  );
}

#define DIFF_TY_NEW 0
#define DIFF_TY_MODIFICATION 1

enum memory_map_diff_ty
{
  New,
  Modification,
};

struct memory_map_diff
{
  enum memory_map_diff_ty ty;
  uint64_t diff_size;
};

void print_memory_map_diff(struct memory_map_diff d)
{
  char *ty_msg;
  if (d.ty == New)
  {
    ty_msg = "Ty";
  }
  else if (d.ty == Modification)
  {
    ty_msg = "Modification";
  }
  else
  {
    printf("unknown diff ty: %d\n", d.ty);
    exit(1);
  }
  printf(
      "memory_map_diff { ty: %s, diff_size: %llu }\n",
      ty_msg,
      d.diff_size
  );
}

// statically allocate space so we can do some processing on memory maps
// without using malloc, which may in turn update the memory maps
char char_buf_1[BUFSIZE][BUFSIZE];
char char_buf_2[BUFSIZE][BUFSIZE];

struct memory_map_entry mem_map_buf_1[BUFSIZE];
struct memory_map_entry mem_map_buf_2[BUFSIZE];

struct memory_map_diff diff_buf[BUFSIZE];

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
    mem_end_tok = strtok(NULL, "-");
    if (mem_start_tok == NULL || mem_end_tok == NULL)
    {
      puts("addr range was not hyphen separate");
      return -1;
    }

    char *device_major_tok, *device_minor_tok;
    device_major_tok = strtok(device_major_minor_tok, ":");
    device_minor_tok = strtok(NULL, ":");
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

int diff_memory_maps(
  struct memory_map_entry mem_map_buf_1[BUFSIZE],
  int buf_1_size,
  struct memory_map_entry mem_map_buf_2[BUFSIZE],
  int buf_2_size,
  struct memory_map_diff diff_buf[BUFSIZE]
)
{
  int diffs_found = 0;
  int buf_1_idx = 0, buf_2_idx = 0;
  while (buf_1_idx < buf_1_size && buf_2_idx < buf_2_size)
  {
    struct memory_map_entry entry_1 = mem_map_buf_1[buf_1_idx];
    struct memory_map_entry entry_2 = mem_map_buf_2[buf_2_idx];

    // completely new entry
    if (entry_1.mem_start != entry_2.mem_start && entry_1.mem_end != entry_2.mem_end)
    {
      if (entry_1.mem_start < entry_2.mem_start)
      {
        diff_buf[diffs_found++] = (struct memory_map_diff) {
          .ty = New,
          .diff_size = entry_2.mem_start - entry_1.mem_start,
        };
        buf_1_idx++;
      }
      else
      {
        diff_buf[diffs_found++] = (struct memory_map_diff) {
          .ty = New,
          .diff_size = entry_1.mem_start - entry_2.mem_start,
        };
        buf_2_idx++;
      }
    }
    else
    {
      // region expanded from start (don't know if this is possible)
      if (entry_1.mem_start != entry_2.mem_start)
      {
        int diff_size = entry_1.mem_start > entry_2.mem_start
          ? entry_1.mem_start - entry_2.mem_start
          : entry_2.mem_start - entry_1.mem_start;
        diff_buf[diffs_found++] = (struct memory_map_diff) {
          .ty = Modification,
          .diff_size = diff_size,
        };
      }
      // region expanded from end
      else if (entry_1.mem_end != entry_2.mem_end)
      {
        int diff_size = entry_1.mem_end > entry_2.mem_end
          ? entry_1.mem_end - entry_2.mem_end
          : entry_2.mem_end - entry_1.mem_end;
        diff_buf[diffs_found++] = (struct memory_map_diff) {
          .ty = Modification,
          .diff_size = diff_size,
        };
      }
      buf_1_idx++;
      buf_2_idx++;
    }
  }
  return diffs_found;
}

int main(int argc, char *argv[])
{
  int num_entries_1 = parse_memory_map(char_buf_1, mem_map_buf_1);
  if (num_entries_1 == -1)
  {
    puts("failed to parse memory map");
    exit(1);
  }

  for (int i = 0; i < 10000; i++) {
    void *chunk = malloc(4096);
  }

  int num_entries_2 = parse_memory_map(char_buf_2, mem_map_buf_2);
  if (num_entries_2 == -1)
  {
    puts("failed to parse memory map");
    exit(1);
  }

  int num_diffs = diff_memory_maps(mem_map_buf_1, num_entries_1, mem_map_buf_2, num_entries_2, diff_buf);
  if (num_diffs == -1)
  {
    puts("failed to diff memory maps");
    exit(1);
  }

  printf("found %d diffs\n", num_diffs);
  for (int i = 0; i < num_diffs; i++) {
    print_memory_map_diff(diff_buf[i]);
  }

  return 0;
}
