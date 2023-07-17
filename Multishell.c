/*Abdurrahman Göktaş 
abdurrahmangoktas44@gmail.com
Çok karmaşık olmayan bir shell sistemidir .Ayrica Multi sheel ile baglanmistir.
*/
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MY_FILE_SIZE 1024
#define MY_SHARED_FILE_NAME "/sharedlogfile"

#define MAX_SHELL 10
#define DEFAULT_NSHELL 2

char *addr = NULL;
int fd = -1;

int initmem() { // Paylasimli bellegi kullanarak  multi shell icin kullanilir.
  fd = shm_open(MY_SHARED_FILE_NAME, O_CREAT | O_RDWR | O_TRUNC, 0666);
  if (fd < 0) {
    perror("multishell.c:açılan dosya:");
    exit(1);
  }
  if (ftruncate(fd, 1024) == -1) {
    perror("ftruncate");
    exit(1);
  }

  addr = mmap(NULL, MY_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == NULL) {
    perror("mmap:");
    exit(1);
  }
  return 0;
}

char *create_log_filename() { // olusturulan dosyanın zaman tutar.
  time_t t;
  struct tm *tm_info;
  static char filename[64];

  time(&t);
  tm_info = localtime(&t);

  snprintf(filename, sizeof(filename), "shelllog-%04d%02d%02d-%02d%02d%02d.txt",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  return filename;
}

int main(int argc, char **argv) { // Coklu shell ornegi calistirir.
  int nshells = argc > 1 ? atoi(argv[1]) : DEFAULT_NSHELL;
  if (nshells <= 0 || nshells > MAX_SHELL) {
    fprintf(stderr, "Geçersiz komut sayısı. İzin verilen aralık: 1-%d\n",
            MAX_SHELL);
    return 1;
  }

  initmem();

  pid_t children[nshells];
  bool finished[nshells];
  memset(finished, 0, sizeof(finished));

  for (int i = 0; i < nshells; ++i) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork error");
      exit(1);
    } else if (pid == 0) {
      execlp("xterm", "xterm", "-hold", "-e", "./singleshell", NULL);

      perror("execlp error");
      exit(1);
    } else {
      children[i] = pid;
    }
  }

  int completed_shells = 0;
  while (completed_shells < nshells) {
    int status;
    pid_t pid = wait(&status);
    for (int i = 0; i < nshells; ++i) {
      if (pid == children[i] && !finished[i]) {
        finished[i] = true;
        ++completed_shells;
        break;
      }
    }
  }

  FILE *log_file =
      fopen(create_log_filename(), "w"); // Dosyanin acilmasini saglar
  if (log_file) {
    fputs(addr, log_file);
    fclose(log_file);
  } else {
    perror("Dosyayı oluşturulamıyor");
  }

  munmap(addr, MY_FILE_SIZE);
  close(fd);

  return 0;
}
