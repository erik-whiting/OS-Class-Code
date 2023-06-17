#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv) {
    // Open the file
    int file;
    file = open(argv[1], O_RDONLY);
    // The assignment says you should match the
    // shell command. When the file isn't there,
    // `cat` outputs "No such file or directory",
    // I'll reimplement that functionality here.
    if (file == -1) {
        printf("cat: %s: No such file or directory\n", argv[1]);
    }

    // Read the file in 50 byte increments and
    // print the contents.
    int buffer_size = 50;
    char file_contents[buffer_size];
    int read_buffer_size;
    while ((read_buffer_size = read(file, file_contents, buffer_size)) > 0) {
        write(
            1,
            &file_contents,
            read_buffer_size
        );
    }

    close(file);
    return 0;
}
