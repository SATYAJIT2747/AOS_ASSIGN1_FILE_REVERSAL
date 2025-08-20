// =================== Header Files ===================
#include <iostream>   // for cout and cin
#include <fcntl.h>    // for open system call
#include <unistd.h>   // for read , write , close and lseek
#include <cstring>    //for sterror
#include <sys/stat.h> //for mkdir
#include <cstdlib>    //for atoi
#include <errno.h>    // for errno value
using namespace std;

// ================= Common Helper Functions  To be Used with Flag 0 ,1 , 2=================
void makeassignmentdirectory() // to create directory assignment1 (0700 means only user can access)
{
    const char *dirname = "Assignment1";
    if (mkdir(dirname, 0700) == -1)
    {
        if (errno != EEXIST)
        {
            perror("error in creating the assignment1 directory");
            exit(EXIT_FAILURE);
        }
    }
}

// to generate the op file path assignment1/<flag><input_file_name>
string generateopfilepath(const string &inputfile, int flag)
{
     return "Assignment1/" + to_string(flag) + "_" + inputfile;
}

// now we will reverse the contents of the buffer
void reverseblock(char *buffer, int size)
{
    int left = 0, right = size - 1;
    while (left < right)
    {
        swap(buffer[left++], buffer[right--]);
    }
}

// Displays the progress of a file operation as a percentage.

void showprogress(off_t writtenbytes, off_t totalbytes)
{
    int percentage = static_cast<int>((writtenbytes * 100) / totalbytes);
    cout << "\rprogress:" << percentage << "%" << flush;
}

// ================= Flag 0 : Block wise reversal ====================

void doblockwisereversal(const string &inputfile, int blocksize)
{
    int inputfd = open(inputfile.c_str(), O_RDONLY);
    if (inputfd < 0)
    {
        perror("failed to open a new file.");
        exit(EXIT_FAILURE);
    }

    // now create the outputdirectory
    makeassignmentdirectory();

    // now to generate the op file path
    string outputfile = generateopfilepath(inputfile, 0);

    // now to open the output file with rw--- permission
    int outputfd = open(outputfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (outputfd < 0)
    {
        perror("failed to create the op file");
        close(inputfd);
        exit(EXIT_FAILURE);
    }

    // now to determine total file size using lseek
    off_t totalsize = lseek(inputfd, 0, SEEK_END);
    lseek(inputfd, 0, SEEK_SET);

    // allocate dynamic memeory to the block
    char *buffer = new char[blocksize];
    off_t written = 0;
    ssize_t bytesread;

    // read ip file block by block
    while ((bytesread = read(inputfd, buffer, blocksize)) > 0)
    {
        reverseblock(buffer, bytesread); // Reverse the content of the buffer that was just read
        write(outputfd, buffer, bytesread);// Write the reversed buffer to the output file
        written += bytesread; // Update the count of written bytes
        showprogress(written, totalsize);
    }

    cout << "\nreversal complete.\n";

    // Free the dynamically allocated memory and close file descriptors
    delete[] buffer;
    close(inputfd);
    close(outputfd);
}

// ================= Flag 1 : Full File reversal ====================
void dofullfilereversal(const string &inputfile)
{
    int inputfd = open(inputfile.c_str(), O_RDONLY);
    if (inputfd < 0)
    {
        perror("failed to open the input file.");
        exit(EXIT_FAILURE);
    }
    makeassignmentdirectory();// Ensure the output directory exists and generate the output file path
    string outputfile = generateopfilepath(inputfile, 1);
    int outputfd = open(outputfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (outputfd < 0)
    {
        perror("failed to create the op file");
        exit(EXIT_FAILURE);
    }

    off_t totalsize = lseek(inputfd, 0, SEEK_END);// Get the total size of the input file
    if (totalsize < 0)
    {
        perror("failed to determine the file size.");
        close(inputfd);
        close(outputfd);
        exit(EXIT_FAILURE);
    }

    const size_t chunksize = 4096; // chunk size is 4KB
    char *buffer = new char[chunksize];
    off_t remaining = totalsize;
    off_t written = 0;

    while (remaining > 0) // Loop as long as there are bytes left to read from the input file
    {
        size_t bytes_to_read = (remaining >= (off_t)chunksize) ? chunksize : remaining;
        off_t offset = remaining - bytes_to_read;

        if (lseek(inputfd, offset, SEEK_SET) < 0)
        {
            perror("seek error.");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        ssize_t bytesread = read(inputfd, buffer, bytes_to_read); // Read the chunk from the input file
        if (bytesread < 0)
        {
            perror("read error.");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }
        // Since we are reading the file backwards but writing forwards,
        // we must reverse each chunk to maintain the correct order.
        reverseblock(buffer, bytesread);

        if (write(outputfd, buffer, bytesread) != bytesread)
        {
            perror("write error.");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }
        // Update counters and show progress
        written += bytesread;
        remaining -= bytesread;
        showprogress(written, totalsize);
    }

    cout << "\nfull size reversal is completed.\n";
    // Cleanup
    delete[] buffer;
    close(inputfd);
    close(outputfd);
}

// ================ Flag 2: Partial Reversal====================

//  The file is treated as three regions:
//  1. [0 to start-1]: This region is reversed.
//  2. [start to end]: This region is copied as-is.
//  3. [end+1 to end-of-file]: This region is reversed.
void dopartialreversal(const string &inputfile, off_t start, off_t end)
{
    int inputfd = open(inputfile.c_str(), O_RDONLY);
    if (inputfd < 0)
    {
        perror("failed to open the input file.");
        exit(EXIT_FAILURE);
    }
    makeassignmentdirectory();
    string outputfile = generateopfilepath(inputfile, 2);
    int outputfd = open(outputfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (outputfd < 0)
    {
        perror("failed to create the op file");
        close(inputfd);
        exit(EXIT_FAILURE);
    }

    // Check if input ends with newline
    off_t totalsize = lseek(inputfd, 0, SEEK_END);
    if (totalsize < 0)
    {
        perror("failed to determine the file size.");
        close(inputfd);
        close(outputfd);
        exit(EXIT_FAILURE);
    }

    bool ends_with_newline = false;
    off_t actualsize = totalsize;
    if (totalsize > 0) {
        char lastbyte;
        lseek(inputfd, -1, SEEK_END);
        if (read(inputfd, &lastbyte, 1) == 1 && lastbyte == '\n') {
            ends_with_newline = true;
            actualsize -= 1; // Ignore newline for reversal
        }
    }
    lseek(inputfd, 0, SEEK_SET);

    if (start >= actualsize || end >= actualsize || start > end)
    {
        cerr << "start or end is out of range or invalid.\n";
        close(inputfd);
        close(outputfd);
        exit(EXIT_FAILURE);
    }

    const size_t chunksize = 4096;
    char *buffer = new char[chunksize];
    off_t written = 0;

    // REGION 1: [0 .. start-1] reversed, chunk-wise backwards read
    off_t remaining1 = start;
    while (remaining1 > 0)
    {
        size_t toread = (remaining1 >= (off_t)chunksize) ? chunksize : remaining1;
        off_t offset = remaining1 - toread;
        if (lseek(inputfd, offset, SEEK_SET) < 0)
        {
            perror("lseek failed in region 1");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        ssize_t r = read(inputfd, buffer, toread);
        if (r != (ssize_t)toread)
        {
            perror("read failed in region 1");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        reverseblock(buffer, r);

        if (write(outputfd, buffer, r) != r)
        {
            perror("write failed in region 1");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        written += r;
        remaining1 -= r;
        showprogress(written, actualsize);
    }

    // REGION 2: [start .. end] copy as-is forward
    off_t region2_size = end - start + 1;
    if (lseek(inputfd, start, SEEK_SET) < 0)
    {
        perror("lseek failed at region 2 start");
        delete[] buffer;
        close(inputfd);
        close(outputfd);
        exit(EXIT_FAILURE);
    }

    off_t remaining2 = region2_size;
    while (remaining2 > 0)
    {
        size_t toread = (remaining2 >= (off_t)chunksize) ? chunksize : remaining2;
        ssize_t r = read(inputfd, buffer, toread);
        if (r <= 0)
        {
            perror("read failed in region 2");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        if (write(outputfd, buffer, r) != r)
        {
            perror("write failed in region 2");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        written += r;
        remaining2 -= r;
        showprogress(written, actualsize);
    }

    // REGION 3: [end+1 .. actualsize-1] reversed, chunk-wise backwards read
    off_t region3_start = end + 1;
    off_t region3_size = actualsize - region3_start;
    off_t remaining3 = region3_size;
    off_t curr_end_offset = actualsize;

    while (remaining3 > 0)
    {
        size_t bytes_to_read = (remaining3 >= (off_t)chunksize) ? chunksize : remaining3;
        off_t offset = curr_end_offset - bytes_to_read;

        if (lseek(inputfd, offset, SEEK_SET) < 0)
        {
            perror("lseek failed in region 3");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        ssize_t bytestoread = read(inputfd, buffer, bytes_to_read);
        if (bytestoread != (ssize_t)bytes_to_read)
        {
            perror("read failed in region 3");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        reverseblock(buffer, bytestoread);

        // Write sequentially after region2
        // Output offset = start + region2_size + (region3_size - remaining3)
        off_t output_offset = start + region2_size + (region3_size - remaining3);
        if (lseek(outputfd, output_offset, SEEK_SET) < 0)
        {
            perror("lseek failed in output file region 3");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        ssize_t written_bytes = write(outputfd, buffer, bytestoread);
        if (written_bytes != bytestoread)
        {
            perror("write error in partial reversal region 3");
            delete[] buffer;
            close(inputfd);
            close(outputfd);
            exit(EXIT_FAILURE);
        }

        remaining3 -= bytestoread;
        curr_end_offset -= bytestoread;
        written += written_bytes;
        showprogress(written, actualsize);
    }

    // Write the newline at the end if needed
    if (ends_with_newline) {
        write(outputfd, "\n", 1);
    }

    cout << "\nPartial size reversal is completed.\n";

    delete[] buffer;
    close(inputfd);
    close(outputfd);
}
      
    

// ========== MENU DRIVEN PROGRAM FOR REVERSAL ====================

int main(int argc, char *argv[])
{

    string inputfile;
    int flag;

    if (argc == 1)
    {
        cout << "=== File reversal menu===\n";
        cout << "0. Block wise reversal====\n";
        cout << "1. full reversal==========\n";
        cout << "2. Partial range reversal=======\n";
        cout << "Enter your choice:";
        cin >> flag;
        cout << "enter input file name:";
        cin >> inputfile;
    }
    else
    {
        inputfile = argv[1];
        flag = atoi(argv[2]);
    }
    switch (flag)
    {
    case 0:
        int blocksize;
        if (argc == 1)
        {
            cout << "enter block size:";
            cin >> blocksize;
        }
        else
        {
            if (argc != 4)
            {
                cerr << "Usage for flag 0: ./a.out <input file> 0 <block size>\n";
                return EXIT_FAILURE;
            }
            blocksize = atoi(argv[3]);
        }
        if (blocksize <= 0)
        {
            cerr << "block size must be +ve:\n";
            return EXIT_FAILURE;
        }
        doblockwisereversal(inputfile, blocksize);

        break;
    case 1:
    {
        if (argc != 1 && argc != 3)
        {
            cerr << "Usage for  the flag 1: ./a.out <input file> 1\n";
            return EXIT_FAILURE;
        }
        dofullfilereversal(inputfile);
        break;
    }
    case 2:
    {
        off_t start, end;
        if (argc == 1)
        {
            cout << "Enter start index: ";
            cin >> start;
            cout << "Enter end index: ";
            cin >> end;
        }
        else
        {
            if (argc != 5)
            {
                cerr << "Usage for flag 2: ./a.out <input file> 2 <start index> <end index>\n";
                return EXIT_FAILURE;
            }
            start = atoll(argv[3]);
            end = atoll(argv[4]);
        }
        if (start < 0 || end < start)
        {
            cerr << "Invalid start/end indices.\n";
            return EXIT_FAILURE;
        }
        dopartialreversal(inputfile, start, end);
        break;
    }

    default:
        cerr << "Unsupported flag. Choose 0, 1, or 2.\n";
        return EXIT_FAILURE;

        return EXIT_SUCCESS;
    }
}