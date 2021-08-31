#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
// copy the implementation of ls shamelessly
//example:
//      fmtname("a/b/c/d/e") return "e" {'e',0}
char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return string fmt file name
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;
    return buf;
}
void match(char *a, char *b)
{
    if (strcmp(fmtname(a), b) == 0)
    {
        printf("%s\n", a);
    }
}

void find(char *path, char *fileName)
{
    //printf("%s %s\n", path, fileName);
    //preprocess path
    int fd;
    char buf[512], *p;
    struct dirent subpath;
    struct stat pathStatus;
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        printf("bad path\n");
        return;
    }
    //defer close fd
    if (fstat(fd, &pathStatus) < 0)
    {
        printf("error occured when reading status from the path %s\n", path);
        close(fd);
        return;
    }

    switch (pathStatus.type)
    {
    case T_FILE:
        match(path, fileName);
        break;
    case T_DIR:
        if ((strlen(path) + 1 + DIRSIZ) >= sizeof buf)
        {
            printf("path too long\n");
            close(fd);
            return;
        }
        strcpy(buf, path);
        // add slash to the end of file
        p = buf + strlen(buf);
        *p = '/';
        p++;
        //now father path is in the buf, and p point to the end of father path
        //get the subpath
        for (; read(fd, &subpath, sizeof(subpath)) == sizeof(subpath);)
        {
            // remove "./ ../ "
            //printf("sub dir: %s, inum: %d\n", subpath.name, subpath.inum);
            if (subpath.inum == 0 || subpath.inum == 1 || strcmp(subpath.name, ".") == 0 || strcmp(subpath.name, "..") == 0)
                continue;
            // dfs
            // printf("copy %s to %s \n ", subpath.name, buf);
            memmove(p, subpath.name, strlen(subpath.name));
            // buf is reused in all field, so we need to clean the end of the string
            p[strlen(subpath.name)] = 0;
            //printf("go find in %s \n", buf);
            find(buf, fileName);
        }
        break;
    default: // device
        break;
    }
    close(fd);
}
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("find: find <path> <fileName>\n");
        exit(0);
    }
    find(argv[1], argv[2]);
    exit(0);
}