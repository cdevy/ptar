#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> /* open et lseek */
#include <sys/time.h> /* lutimes */
#include <fcntl.h> /* open */
#include <unistd.h> /* read et lseek */
#include <string.h> /* strcpy et strcat */
#include <time.h>

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

long long longOctalToDecimal(long long octal) {
    long long decimal = 0;
    long long i = 1;
    while (octal != 0) {
	decimal += (octal%10)*i;
	i = i*8;
	octal = octal/10;
    }
    return decimal;
}

void formatDate(char* format, long long timestamp) {
    time_t time = (time_t) timestamp;
    struct tm* date;
    date = localtime(&time);
    strftime(format, 20, "%Y-%m-%d %X", date);
}

int extractor(pile_h* first) {
    struct stat st = {0};
    pile_h* pheader;
    struct timeval times[2];
    while (boucle) {
	
	/* Récupération du header (@TODO insérer mutex ici pour le cas des pthread) */
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
	
	char type = (char)pheader->header->typeflag[0];
	if (stat(pheader->path, &st) != 0 && (type == '0' || type == '5')) {
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
	    
	/* Cas d'un lien symbolique */
	} else if (stat(pheader->path, &st) != 0 && (type == '2')) {
	    symlink(pheader->header->linkname, pheader->path);
	}
	
	// Changer l'uid et gid
	lchown(pheader->path, atoi(pheader->header->uid) , atoi(pheader->header->gid));

	// changer le mode d'accès (PAS DANS LE CAS D'UN LINK )
	if (type == '0' || type == '5'){
	    chmod(pheader->path, octalToDecimal(atoi(pheader->header->mode)));
	}

	// changer le temps de modification (non suffisant pour les dossiers)
	times[0].tv_sec = longOctalToDecimal(atoll(pheader->header->mtime));
	times[0].tv_usec = 0;
	times[1].tv_sec = times[0].tv_sec;
	times[1].tv_usec = 0;
	//printf("time : %ld\n", times[0].tv_sec);
	lutimes(pheader->path, times);
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

    //printf("///debug/// \nx=%d\nl=%d\np=%d avec %d threads\nz=%d\n",optx,optl,optp,nbthread,optz);

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
    char path[357] = ""; /* 257 + 100 pour le chemin du lien */
    char namepath[461] = ""; /* 357 + 104 pour les symbolic link (4 pour la flèche, 100 pour le lien) */
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
	    strncpy(filename, header->name, 100);
	    strcat(filename, "\0");
	    strcat(path, filename);
	    strcpy(namepath, path);
	    if (header->typeflag[0] == '2') {
		strcat(namepath, " -> "); 
		strcat(namepath, header->linkname);
	    }
	    

	    /* Affichage des informations avec la commande -l (permissions, propriétaire, groupe...) */

	    int decimalSize = octalToDecimal(atoi(header->size));
	    int user = octalToDecimal(atoi(header->uid));
	    int group = octalToDecimal(atoi(header->gid));
	    long long timestamp = longOctalToDecimal(atoll(header->mtime)); 
	    char date[20];
	    formatDate(date, timestamp);


	    if (optl) {

		/* Gestion des permissions */
		char perms[] = "----------";
		switch (header->typeflag[0]) {
		    case '5':
		        perms[0] = 'd';
			break;
		    case '2':
			perms[0] = 'l';
			break;
		    default :
			break;
		}
		int i;
		for (i=0; i<3; i++) { 
		    switch (header->mode[4+i]) { // r = 4, w = 2, x = 1, - = 0
			case '1':
			    perms[3*i+3] = 'x';
			    break;
			case '2':
			    perms[3*i+2] = 'w';
			    break;
			case '3':
			    perms[3*i+2] = 'w';
			    perms[3*i+3] = 'x';
			    break;
			case '4':
			    perms[3*i+1] = 'r';
			    break;
			case '5':
			    perms[3*i+1] = 'r';
			    perms[3*i+3] = 'x';
			    break;
			case '6':
			    perms[3*i+1] = 'r';
			    perms[3*i+2] = 'w';
			    break;
			case '7':
			    perms[3*i+1] = 'r';
			    perms[3*i+2] = 'w';
			    perms[3*i+3] = 'x';
			    break;
			default:
			    break;
		    }
		}
		printf("%s %d/%d %d %s %s\n", perms, user, group, decimalSize, date, namepath);
	    } else {
		printf("%s\n", path);
	    }

	    /* Mise en pile des headers */ 

	    int offset = ((decimalSize/512)+(decimalSize%512 != 0))*512;
	    if(optx){
		boucle = boucle + 1;
	    	pile_h *sheader;
	    	sheader = malloc (sizeof (pile_h));
	    	sheader->header = header;
		sheader->size = decimalSize;
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
	    lseek(fd, offset, SEEK_CUR);
	}
    }


    pile_h* firstarchive = first;

    if (optx) {
        /* @TODO Ajouter la création de pthread (étape ultérieure)!!! */
        
	extractor(first);
	
        struct timeval times2[2];
        /* Restauration du first */
        first = firstarchive;
    
        /* Réparation des dates des dossiers */
        while(1){
    
    	    if((char)first->header->typeflag[0] == '5'){
    	        times2[0].tv_sec = longOctalToDecimal(atoll(first->header->mtime));
	        times2[0].tv_usec = 0;
	        times2[1].tv_sec = times2[0].tv_sec;
	        times2[1].tv_usec = 0;
	        lutimes(first->path, times2);
	    }

	    if (first->next != NULL){
	        first = first->next;
	    } else {
	        break;
	    }
        }
    }
    
    // Suppress unused warnings @TODO destroy the next line
    if (optl && optz && optp && nbthread) {}

    exit(0);
}

