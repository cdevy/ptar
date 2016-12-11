#include <stdlib.h>
#include <time.h>

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

void setPermissions(char* perms, header_t* header) {
  /* Gestion des dossiers et liens symboliques */
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
    switch (header->mode[4+i]) { /* r = 4, w = 2, x = 1, - = 0 */
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
}

int toAscii(char* field, int size) {
	int res = 0;
	int i;
	for (i=0; i<size; i++) {
		res += field[i];
	}
	return res;
}

int validChecksum(header_t* header) {
	int sum = 0;
	sum += toAscii(header->name, 100);
	sum += toAscii(header->mode, 8);
	sum += toAscii(header->uid, 8);
	sum += toAscii(header->gid, 8);
	sum += toAscii(header->size, 12);
	sum += toAscii(header->mtime, 12);
	sum += 8*32;  /* valeur ascii de l'espace : 32 */
  sum += toAscii(header->typeflag, 1);
	sum += toAscii(header->linkname, 100);
	sum += toAscii(header->magic, 6);
	sum += toAscii(header->version, 2);
	sum += toAscii(header->uname, 32);
	sum += toAscii(header->gname, 32);
	sum += toAscii(header->devmajor, 8);
	sum += toAscii(header->devminor, 8);
	sum += toAscii(header->prefix, 155);
	sum += toAscii(header->pad, 12);
	if (octalToDecimal(atoi(header->checksum)) == sum) {
		return 1;
	} else {
    return 0;
  }
}
