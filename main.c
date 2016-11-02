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
    int fd = open("Tests/archive.tar", O_RDONLY);
    header_t header;
    int empty = 0;
    char path[257] = "";
    char filename[100] = "";
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
	        printf("Nom : %s\n", header.name);
		if (header.prefix[0] != '\0') {
		    strcpy(path, header.prefix);
		    strcat(path, "/");
		}
		strcpy(filename, header.name);
		strcat(path, filename);
		printf("Chemin : %s\n", path);
	    }
	    int offset = octalToDecimal(atoi(header.size));
	    if (offset) { /* in case of an empty file or directory, there's no contents */
	        offset = ((offset/512)+1)*512;
	        lseek(fd, offset, SEEK_CUR);
	    }
	}
    }
    return 0;
}

