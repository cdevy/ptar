#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> /* open et lseek */
#include <fcntl.h> /* open */
#include <unistd.h> /* read et lseek */
#include <string.h>
int boucle = 0;
char* archive;
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

int extractor(pile_h* first) {
    struct stat st = {0};
    pile_h* pheader;
    while (boucle) {
	
	/* Récupération du header (@TODO insérer mutex ici) */
	pheader = first;
	if(first->next != NULL){
	    first = first->next;
	}
	boucle = boucle -1;

	/* Extraction */
	int fd = open(archive, O_RDONLY);
	lseek(fd,pheader->pos,SEEK_CUR);
	int i;

	/* Création des dossiers parents */
	for (i=0; i < (int)strlen(pheader->path); i++){
	    if(pheader->path[i] == '/'){
		pheader->path[i] = '\0';
		if (stat(pheader->path, &st) == -1) {
		    mkdir(pheader->path, 0777);
		}
		pheader->path[i] = '/';	
	    }
	}
	
	/* Extraction du fichier */
	if (stat(pheader->path, &st) == 0) {
	   printf("WARNING : file %s already exists, not extracting\n", pheader->path);
	} else {
	    int file = open(pheader->path, O_WRONLY | O_CREAT, 0777);
	    char buffer[512];
	    int sizetowrite;
	    while (pheader->size > 0){
	        if(pheader->size < 512){
		    sizetowrite = pheader->size ;
	    	} else {
		    sizetowrite = 512;
	    	}
	    	read(fd, buffer, 512);
	    	write(file, buffer, sizetowrite);
	    	pheader->size = pheader->size - 512;
	    }
	    fsync(file);
	    close(file);
	}
    }
    return 0;
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

    archive = argv[optind];
    int fd = open(archive, O_RDONLY);
    header_t *header;
    pile_h *first;
    pile_h *memory;
    int boolean = 1;
    
    int empty = 0;
    char path[257] = "";
    char filename[101] = "";
    while (!empty) {
	header = malloc(sizeof (header_t));
	strcpy(path, "");
	read(fd, header, 512);
	if (header->name[0] == '\0') {
	    empty = 1;
	    read(fd, header, 512);
	    if (header->name[0] != '\0') {
		printf("Error : empty filename");
	    }
	} else {
	    if (header->prefix[0] != '\0') {
		strcpy(path, header->prefix);
		strcat(path, "/");
	    }
	    strncpy(filename, header->name,100);
	    strcat(filename, "\0");
	    strcat(path, filename);
	    printf("%s\n", path);
	    int offset = octalToDecimal(atoi(header->size));
	    int offset2 = ((offset/512)+(offset%512 != 0))*512;
	    if(optx){
		boucle = boucle + 1;
	    	pile_h *sheader;
	    	sheader = malloc (sizeof (pile_h));
	    	sheader->header = header;
		sheader->size = offset;
		sheader->next = NULL;
	    	strcpy(sheader->path,path);
	    	sheader->pos = lseek(fd,0, SEEK_CUR);
	    	if (boolean) {
		    boolean = 0;
		    first = sheader;
	        } else {
		    memory->next = sheader;
	        }
	        memory = sheader;
	    }
	    lseek(fd, offset2, SEEK_CUR);
	}
    }
    if (optx) {
	extractor(first);
    }
}

