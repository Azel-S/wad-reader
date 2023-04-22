#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "../libWad/Wad.h"
using namespace std;

Wad *wad;

static int getattr_callback(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

    if (wad->isDirectory(path))
    {
        stbuf->st_mode = S_IFDIR | 0555;
        stbuf->st_nlink = 2;
        return 0;
    }
    else if (wad->isContent(path))
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = wad->getSize(path);
        return 0;
    }
    else
    {
        return -ENOENT;
    }
}

static int open_callback(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    if (wad->isContent(path))
    {
        return wad->getContents(path, buf, size, offset);
    }
    else
    {
        return -ENOENT;
    }
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    if (wad->isDirectory(path))
    {
        vector<string> elements;
        wad->getDirectory(path, &elements);

        for (int i = 0; i < elements.size(); i++)
        {
            filler(buf, elements[i].c_str(), NULL, 0);
        }

        return 0;
    }
    else
    {
        return -ENOENT;
    }
}

static struct fuse_operations operations
{
    .getattr = getattr_callback,
    .open = open_callback,
    .read = read_callback,
    .readdir = readdir_callback
};

int main(int argc, char *argv[])
{
    wad = Wad::loadWad(argv[1]);
    argv[1] = argv[2];
    return fuse_main(argc - 1, argv, &operations, NULL);
}