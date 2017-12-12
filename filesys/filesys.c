#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format)
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool filesys_create (const char *path, off_t initial_size) {
   block_sector_t inode_sector = 0;
   char dirname[strlen(path)];   // strlen(dir) will be th maximum needed
   char filename[strlen(path)];  // filename can be a directory's name
   dir_extract_name(path, dirname, filename);
   struct dir *dir = dir_open_path (dirname);

   bool success = (dir != NULL
      && filename != NULL
      && free_map_allocate (1, &inode_sector)
      && inode_create (inode_sector, initial_size, false)
      && dir_add (dir, path, inode_sector));
   if (!success && inode_sector != 0)
      free_map_release (inode_sector, 1);
   if (dir != NULL) dir_close (dir);

   return success;
}

bool filesys_mkdir (const char *path) {
   char dirname[strlen(path)];   // strlen(dir) will be th maximum needed
   char filename[strlen(path)];  // filename can be a directory's name
   // dirname might be empty, but filename must not be empty
   dir_extract_name(path, dirname, filename);
   struct dir *dir = dir_open_path (dirname);
   block_sector_t inode_sector = 0;
   bool success = (dir != NULL
                  && filename != NULL
                  && free_map_allocate (1, &inode_sector)
                  && dir_create(newdir_sector, dir))
                  && dir_add(dir, filename, inode_sector));  // filename must not be in dir already
   if (!success && inode_sector != 0)
      free_map_release(inode_sector, 1);
   if (dir != NULL) dir_close (dir);
   return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);

  return success;
}

/* Change directory - NEED TO IMPLEMENT */
bool
filesys_chdir (const char *name)
{
   bool result;
   return result;
}


/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, ROOT_DIR_SECTOR))  // not sure if this creates a loop
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

static
