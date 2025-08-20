#include <iostream>
#include <sys/stat.h> // for stat
#include <fcntl.h>    // for open system call
#include <unistd.h>   // for read , write , close and lseek
#include <cstring>    // for strerror , memcmp
#include <errno.h>
#include <cstdlib> // for atoi , atoll
#include <algorithm> // for swap
using namespace std;

// ================= Helper function : Reverse Buffer====================
void reverseblock(char *buffer, ssize_t size)
{
    ssize_t l = 0, r = size - 1;
    while (l < r)
    {
        swap(buffer[l++], buffer[r--]);
    }
}

// Helper :print permissions  either yes/ no==================
void printpermissions(mode_t mode, const string &label)
{
    cout << "user has read permission on " << label << ": " << ((mode & S_IRUSR) ? "yes" : "no") << endl;
    cout << "user has write permission on " << label << ": " << ((mode & S_IWUSR) ? "yes" : "no") << endl;
    cout << "user has execute permission on " << label << ": " << ((mode & S_IXUSR) ? "yes" : "no") << endl;

    cout << "group has read permission on " << label << ": " << ((mode & S_IRGRP) ? "yes" : "no") << endl;
    cout << "group has write permission on " << label << ": " << ((mode & S_IWGRP) ? "yes" : "no") << endl;
    cout << "group has execute permission on " << label << ": " << ((mode & S_IXGRP) ? "yes" : "no") << endl;

    cout << "others have read permission on " << label << ": " << ((mode & S_IROTH) ? "yes" : "no") << endl;
    cout << "others have write permission on " << label << ": " << ((mode & S_IWOTH) ? "yes" : "no") << endl;
    cout << "others have execute permission on " << label << ": " << ((mode & S_IXOTH) ? "yes" : "no") << endl;
}


// ======================Now we will check permission for the given path=================
void check_permissions(const char *path, mode_t expected, const string &lebel)
{
    struct stat info;
    if (stat(path, &info) == -1)
    {
        perror(("stat funnc failed for " + lebel).c_str());
        exit(EXIT_FAILURE);
    }
    mode_t actual = info.st_mode & 0777;
    // cout << " permissions for " << ((actual == expected) ? "Yes" : "No") << "\n";
   
    printpermissions(actual , lebel);
}

// ==================== compare the file size =========================
bool file_same_size(const char *p1, const char *p2)
{
    struct stat s1, s2;
    if (stat(p1, &s1) == -1)
        return false;
    if (stat(p2, &s2) == -1)
        return false;
    return (s1.st_size == s2.st_size);
}

// ==================== verification for flag 0 : Block wise reversal=========================

bool verify_blockbyblock(const char *newpath, const char *oldpath, size_t blocksize)
{
    int fd_old = open(oldpath, O_RDONLY);
    if (fd_old < 0)
    {
        perror("failed to open old file .");
        return false;
    }
    int fd_new = open(newpath, O_RDONLY);
    if (fd_new < 0)
    {
        perror("failed to open new file .");
        close(fd_old);
        return false;
    }
    off_t total = lseek(fd_old, 0, SEEK_END);
    lseek(fd_old, 0, SEEK_SET);
    lseek(fd_new, 0, SEEK_SET);
    char *buffer_old = new char[blocksize];
    char *buffer_new = new char[blocksize];
    off_t processed = 0;
    bool ok = true;
    while (processed < total)
    {
        ssize_t toread = (total - processed >= (off_t)blocksize) ? blocksize : total - processed;
        ssize_t r1 = read(fd_old, buffer_old, toread);
        ssize_t r2 = read(fd_new, buffer_new, toread);
        if (r1 != r2 || r1 < 0)
        {
            ok = false;
            break;
        }
        reverseblock(buffer_old, r1);
        if (memcmp(buffer_old, buffer_new, r1) != 0)
        {
            ok = false;
            break;
        }
        processed += r1;
    }
    delete[] buffer_old;
    delete[] buffer_new;
    close(fd_old);
    close(fd_new);
    return ok;
}

// ================ verify the flag 1 : Full file reversal====================
bool verify_fullreversal(const char *newpath, const char *oldpath)
{
    int fd_old = open(oldpath, O_RDONLY);
    if (fd_old < 0)
    {
        perror("failed to open old file .");
        return false;
    }
    int fd_new = open(newpath, O_RDONLY);
    if (fd_new < 0)
    {
        perror("failed to open new file .");
        close(fd_old);
        return false;
    }
    off_t total = lseek(fd_old, 0, SEEK_END);
    if (total != lseek(fd_new, 0, SEEK_END))
    {
        close(fd_old);
        close(fd_new);
        return false;
    }
    const size_t chunksize = 4096; // chunk size is 4KB
    char *buffer_old = new char[chunksize];
    char *buffer_new = new char[chunksize];
    off_t processed = 0;
    bool ok = true;
    while (processed < total)
    {
        ssize_t toread = (total - processed >= (off_t)chunksize) ? chunksize : total - processed;
        lseek(fd_old, processed, SEEK_SET);
        ssize_t r1 = read(fd_old, buffer_old, toread);
        lseek(fd_new, total - processed - toread, SEEK_SET);
        ssize_t r2 = read(fd_new, buffer_new, toread);
        if (r1 != r2 || r1 < 0)
        {
            ok = false;
            break;
        }
        reverseblock(buffer_new, r2);
        if (memcmp(buffer_old, buffer_new, r1) != 0)
        {
            ok = false;
            break;
        }
        processed += r1;
    }
    delete[] buffer_old;
    delete[] buffer_new;
    close(fd_old);
    close(fd_new);
    return ok;
}
// ==================== verify the flag 2 : Partial revarsal =========================

       
  bool verify_partialreversal(const char *newpath, const char *oldpath, off_t start, off_t end)
{
    int fd_old = open(oldpath, O_RDONLY);
    if (fd_old < 0)
    {
        perror("failed to open old file .");
        return false;
    }
    int fd_new = open(newpath, O_RDONLY);
    
    if (fd_new < 0)
    {
        perror("failed to open new file .");
        close(fd_old);
        return false;
    }

    
    
   

    // 1. Get total size from old file
    off_t total = lseek(fd_old, 0, SEEK_END);
    
    // 2. Check if new file size is the same
    if (total != lseek(fd_new, 0, SEEK_END))
    {
        close(fd_old);
        close(fd_new);
        return false;
    }
    
    // 3. THE FIX: Reset file pointers back to the beginning for both files.
    lseek(fd_old, 0, SEEK_SET);
    lseek(fd_new, 0, SEEK_SET);

    // 4. Declare variables for the loops ONCE.
    const size_t chunk = 4096;
    char *buffer1 = new char[chunk];
    char *buffer2 = new char[chunk];
    bool ok = true;

    // --- The rest of the function follows ---

    // part 1: reverse from 0 to start-1
    off_t remain1 = start;
    while (remain1 > 0)
    {
        size_t toread = (remain1 >= (off_t)chunk) ? chunk : remain1;
        off_t offset = remain1 - toread;
        lseek(fd_old, offset, SEEK_SET);
        ssize_t r1 = read(fd_old, buffer1, toread);
        reverseblock(buffer1, r1);

        ssize_t r2 = read(fd_new, buffer2, r1);
        if (r1 != r2 || memcmp(buffer1, buffer2, r1) != 0)
        {
            ok = false;
            break;
        }
        remain1 -= r1;
    }

    // part 2: from start to end
    if (ok)
    {
        off_t len2 = end - start + 1;
        lseek(fd_old, start, SEEK_SET);
        lseek(fd_new, start, SEEK_SET);
        off_t remain2 = len2;
        while (remain2 > 0)
        {
            ssize_t toread = (remain2 >= (off_t)chunk) ? chunk : remain2;
            ssize_t r1 = read(fd_old, buffer1, toread);
            ssize_t r2 = read(fd_new, buffer2, toread);
            if (r1 != r2 || memcmp(buffer1, buffer2, r1) != 0)
            {
                ok = false;
                break;
            }
            remain2 -= r1;
        }
    }

    // part 3: from end+1 to total
    if (ok)
    {
        off_t region3_start_offset_in_new = end + 1;
        lseek(fd_new, region3_start_offset_in_new, SEEK_SET);

        off_t remain3 = total - (end + 1);
        off_t curr_offset_old = total;
        while (remain3 > 0)
        {
            size_t toread = (remain3 >= (off_t)chunk) ? chunk : remain3;
            off_t offset = curr_offset_old - toread;
            lseek(fd_old, offset, SEEK_SET);
            ssize_t r1 = read(fd_old, buffer1, toread);
            reverseblock(buffer1, r1);

            ssize_t r2 = read(fd_new, buffer2, toread);
            if (r1 != r2 || memcmp(buffer1, buffer2, r1) != 0)
            {
                ok = false;
                break;
            }
            curr_offset_old -= r1;
            remain3 -= r1;
        }
    }
    
    delete[] buffer1;
    delete[] buffer2;
    close(fd_old);
    close(fd_new);
    return ok;
}


// ==================== MAIN FUNCTION =========================
int main(int argc, char *argv[])
{    if (argc < 5)
    {
        cerr << "Usage:\n";
        cerr << "Flag 0: ./a.out <newfile> <oldfile> <directory> 0 <blocksize>\n";
        cerr << "Flag 1: ./a.out <newfile> <oldfile> <directory> 1\n";
        cerr << "Flag 2: ./a.out <newfile> <oldfile> <directory> 2 <start> <end>\n";
        return EXIT_FAILURE;
    }
    const char *newfile = argv[1];
    const char *oldfile = argv[2];
    const char *directory = argv[3];
    int flag = atoi(argv[4]);

    bool content_ok = false;
   if (flag == 0)
{
    if (argc != 6)
    {
        cerr << "For flag 0, plz provide a block size.\n";
        return EXIT_FAILURE; 
    }
    size_t blocksize = atoi(argv[5]);
    content_ok = verify_blockbyblock(newfile, oldfile, blocksize);
}
else if (flag == 1)
{
    if(argc != 5)
    {
        cerr << "For flag 1, no extra arguments are needed.\n";
        return EXIT_FAILURE;
    }
    content_ok = verify_fullreversal(newfile, oldfile);
}
else if (flag == 2)
{
    if (argc != 7)
    {
        cerr << "For flag 2, plz provide start and end offsets.\n";
        return EXIT_FAILURE;
    }
    off_t start = atoll(argv[5]);
    off_t end = atoll(argv[6]);
    content_ok = verify_partialreversal(newfile, oldfile, start, end);
}
    else
    {
        cerr << " flag is incorrect . plz use 0, 1 or 2.\n";
        return EXIT_FAILURE;
    }

    // check the directory exist or not
    struct stat dstat;
    bool dir_exists = (stat(directory, &dstat) == 0 && S_ISDIR(dstat.st_mode));
    cout << "Directory is created: " << (dir_exists ? "Yes" : "No") << endl;

    // checking the content 
    cout<< "Whether file contents are correctly processed: " << (content_ok ? "Yes" : "No") << endl;


    // size  checking
    bool size_ok = file_same_size(newfile, oldfile);
    cout<< "Both Files Sizes are Same: " << (size_ok ? "Yes" : "No") << endl;

    // check the permissions
    check_permissions(newfile , 0600 , "New File");
    check_permissions(oldfile, 0644, "Old File");
    check_permissions(directory, 0700, "Directory");

    return 0;
}