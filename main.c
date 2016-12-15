#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> /* open et lseek */
#include <sys/time.h> /* lutimes */
#include <fcntl.h> /* open */
#include <unistd.h> /* read et lseek */
#include <string.h> /* strcpy et strcat */
#include <time.h>
#include <pthread.h> /* threads & mutex */


#include <dlfcn.h> /*dlopen*/
#include <assert.h>
#include "zlib/zlib.h"
#define windowBits 15
#define ENABLE_ZLIB_GZIP 32
#define CHUNK 0x4000
#define temp "temp_for_zlib.tar"
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)

#  include <fcntl.h>

#  include <io.h>

#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)

#else

#  define SET_BINARY_MODE(file)

#endif
int boucle = 0;
char* archive;
pthread_mutex_t pile_mutex;


void *extractor(void* pointeur) {
  pile_h** pointeur_first = (pile_h**) pointeur;
  struct stat st = {0};
  pile_h* pheader;
  struct timeval times[2];
  int fd = open(archive, O_RDONLY);
  while (1) {

    /* Récupération du header (@TODO insérer mutex ici pour le cas des pthread) */

	  pthread_mutex_lock(&pile_mutex);
	  pheader = *pointeur_first;
	  if((*pointeur_first)->next != NULL){
      *pointeur_first = (*pointeur_first)->next;
	  }
    if(boucle) {
      boucle = boucle -1;
    } else {
	    pthread_mutex_unlock(&pile_mutex);
	    break;
    }
    pthread_mutex_unlock(&pile_mutex);

    /* Extraction */

    lseek(fd,pheader->pos,SEEK_SET);
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

    /* Changer l'uid et gid */

    lchown(pheader->path, atoi(pheader->header->uid) , atoi(pheader->header->gid));

    /* Changer le mode d'accès (PAS DANS LE CAS D'UN LINK) */

    if (type == '0' || type == '5'){
      chmod(pheader->path, octalToDecimal(atoi(pheader->header->mode)));
    }

    /* Changer le temps de modification (non suffisant pour les dossiers) */
    times[0].tv_sec = longOctalToDecimal(atoll(pheader->header->mtime));
	  times[0].tv_usec = 0;
	  times[1].tv_sec = times[0].tv_sec;
	  times[1].tv_usec = 0;
    //printf("time : %ld\n", times[0].tv_sec);
    lutimes(pheader->path, times);
  }
  close(fd);
  return NULL;
}


int main(int argc, char* argv[]) {

  /* Partie détection des options, elles sont stockées dans les optX correspondants à leur noms */
  int opt, optx, optl, optp, optz, nbthread;
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
    exit(1);
  }

  archive = argv[optind];

  	// TOUS LES TRUCS DU ZLIB //
    if (optz) {

	// 1) open zlib
	//On fait des tests pour les 3 types différents
 	void* handle = dlopen("libz.dylib",RTLD_NOW);
 	if (!handle) {
   	    handle = dlopen("libz.so",RTLD_NOW);
 	}
 	if (!handle) {
   	    handle = dlopen("libz.sl",RTLD_NOW);
 	}
 	if (!handle) {
   	    printf("unable to open zlib (.dylib, .so or .sl) !\nerror : %s\n", dlerror());printf("\n");
   	    exit(EXIT_FAILURE);
 	}
	
	// 2) import functions 
	int (*inflateInit2Remote)(z_streamp, int, const char *, int);
  	void (*inflateEndRemote)(z_streamp);
  	int (*inflateRemote)(z_streamp, int);
	
	// 3) Declaration of everything needed
  	*(void **) (&inflateInit2Remote) = dlsym(handle, "inflateInit2_");
  	*(void **) (&inflateEndRemote) = dlsym(handle, "inflateEnd");
  	*(void **) (&inflateRemote) = dlsym(handle, "inflate");
	
	int ret;
   	unsigned have;
    	unsigned char in[CHUNK];
    	unsigned char out[CHUNK];
	
	z_stream strm;
    	strm.zalloc = Z_NULL;
    	strm.zfree = Z_NULL;
    	strm.opaque = Z_NULL;
    	strm.avail_in = 0;
    	strm.next_in = Z_NULL;
    	ret = (*inflateInit2Remote)(& strm, windowBits | ENABLE_ZLIB_GZIP, ZLIB_VERSION,(int)sizeof(z_stream));
	if (ret != Z_OK){
        	printf("ERROR : Error during initialization of zlib\n");
        	exit(1);
    	}

	FILE *dest = fopen( temp, "w+" );
	FILE *archive_file = fopen(archive, "r+" );
	// 4) Decompression (called inflation by zlib)
	do {

            strm.avail_in = fread(in, 1, CHUNK, archive_file);

	    // 4.1) Verication of source file

            if (ferror(archive_file)) {
            	printf("ERROR : Could not open source file for .gz extraction\n");
            	(void)(*inflateEndRemote)(&strm);
		unlink(temp);
            	exit(1);
            }
	    
	    // 4.2) End of decompression 
            if (strm.avail_in == 0) {
           	 break;
	    }

            strm.next_in = in;

            // 4.3) Extract data chunk by chunk
            do {
            	strm.avail_out = CHUNK;
            	strm.next_out = out;
            	ret = (*inflateRemote)(&strm, Z_NO_FLUSH);
		// 4.3.1) Error case during the inflation
            	if (ret == Z_STREAM_ERROR || ret == Z_MEM_ERROR) {
		    printf("ERROR : Extraction (inflation) of the file failed\n");
		    (void)(*inflateEndRemote)(&strm);
		    unlink(temp);
		    exit(1);
		} 
                have = CHUNK - strm.avail_out;

		// 4.3.2) Write (flush) data from 'out' to the file (and check for errors)

                if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
               	    printf("ERROR : Data written on inflated file is corrupted\n");
                    unlink(temp);
                    (void)(*inflateEndRemote)(&strm);
                    exit(1);
            	}
            } while (strm.avail_out == 0);

        // Double verification for loop
        } while (ret != Z_STREAM_END);


    // 5) End of inflation, suppression of useless data
        (void)(*inflateEndRemote)(&strm);
        archive = temp;
    }


  int fd = open(archive, O_RDONLY);
  header_t *header;
  pile_h *first;
  pile_h *memory;
  int boolean = 1;

  int empty = 0;
  char path[257] = "";
  char namepath[361] = ""; /* 257 + 104 pour les symbolic link (4 pour la flèche, 100 pour le lien) */
  char filename[101] = "";
  while (!empty) {
    header = malloc(sizeof (header_t));
	  strcpy(path, "");
	  read(fd, header, 512);

    /* Détermination de la fin de l'archive */
    if (header->name[0] == '\0') {
      empty = 1;
	    read(fd, header, 512);
	    if (header->name[0] != '\0') {
        printf("Error : empty filename");
        exit(1);
	    }

    } else {

      /* Vérification du checksum */
      if (!validChecksum(header)) {
        exit(1);
      }

      /* Détermination du chemin du fichier */
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

	    /* Gestion de l'affichage des informations avec la commande -l (permissions, propriétaire, groupe...) */

	    int decimalSize = octalToDecimal(atoi(header->size));
	    int user = octalToDecimal(atoi(header->uid));
	    int group = octalToDecimal(atoi(header->gid));
	    long long timestamp = longOctalToDecimal(atoll(header->mtime));
	    char date[20];
	    formatDate(date, timestamp);

      if (optl) {
        char perms[] = "----------";
        setPermissions(perms, header);
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
    pile_h** pointeur = &first;
    if(optp) {
      pthread_t threads[nbthread]; //création du tableau de thread
	    int t;
      for(t=0; t<nbthread; t++){
        pthread_create(&threads[t], NULL, extractor, pointeur);
	    }
	    for(t=0; t<nbthread; t++){
        pthread_join(threads[t],NULL);
	    }
    } else {
      extractor(pointeur);
    }

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

  if (optz) {
	remove(temp);
  }
  exit(0);
}
