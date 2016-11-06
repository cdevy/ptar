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

int main(int argc, char* argv[]) {
    /* Partie détection des options, elles sont stockées dans les optX correspondants à leur noms */

    int opt,optx,optl,optp,optz,nbthread;
    optx = 0;
    optl = 0;
    optp = 0;
    optz = 0;
    nbthread = 0;
    while ( (opt = getopt(argc, argv, "xlp:z")) != -1 ) {
	switch(opt) {
	    case 'x':
		optx = 1;
		break;
	    case 'l':
		optl = 1;
		break;
	    case 'p':
		nbthread = atoi(optarg);
		optp = 1;
		break;
	    case 'z':
		optz = 1;
		break;
	    default :
		printf("WARNING : option non recognized, ignoring it : %c\n", optopt);
	}
    }

    printf("///debug/// \nx=%d\nl=%d\np=%d avec %d threads\nz=%d\n",optx,optl,optp,nbthread,optz);

    if (optind >= argc){
	printf("ERROR : an argument (path of tar archive) is expected after the options or the number of thread was not specified with -p option\n");
	exit(-1);
    }


    int fd = open(argv[optind], O_RDONLY);
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

