#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> /* open et lseek */
#include <fcntl.h> /* open */
#include <unistd.h> /* read et lseek */
#include <string.h>

int octalToDecimal(int octal) {
    int decimal = 0;
    int i = 1;
    while (octal != 0) {
	decimal += (octal%10)*i;
	i = i*8;
	octal = octal/10;
    }
    return decimal;
}

int main() {
    int fd = open("Tests/testtar.tar", O_RDONLY);
    header_t header;
    int empty = 0;
    char path[257] = "";
    char filename[101] = "";
    while (!empty) {
	strcpy(path, "");
	read(fd, &header, 512);
	if (header.name[0] == '\0') {
	    empty = 1;
	    read(fd, &header, 512);
	    if (header.name[0] != '\0') {
		printf("Error : empty filename");
	    }
	} else {
	    if (header.typeflag[0] != '1') { /* we don't print links */
		if (header.prefix[0] != '\0') {
		    strcpy(path, header.prefix);
		    strcat(path, "/");
		}
		strncpy(filename, header.name,100);
		strcat(filename, "\0");
		strcat(path, filename);
		printf("Path : %s\n", path);
	    }
	    int offset = octalToDecimal(atoi(header.size));
	    printf("Size : %d\n", offset);
	    offset = ((offset/512)+(offset%512 != 0))*512;
	    lseek(fd, offset, SEEK_CUR);
	}
    }
    return 0;
}

