/*Abdurrahman Göktaş
abdurrahmangoktas44@gmail.com
Çok karmaşık olmayan bir shell sistemidir .Ayrica Multi sheel ile baglanmistir.
*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define INBUF_SIZE 256
#define MAX_ARGS 64
#define MY_FILE_SIZE 1024
#define MY_SHARED_FILE_NAME "/sharedlogfile"

char *addr = NULL;
int fd = -1;

void handle_cd(char **args) { // Data handel chat gtp  tarafından bakılmıştır.
  if (args[1] == NULL) {
    fprintf(stderr, "singleshell: beklenen değer \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("singleshell: dizin hatası");
    }
  }
}

void handle_exit() { exit(0); } // Data handel chat gtp  tarafından bakılmıştır.
void print_help() {             // yazilabilecek olayları yazar
  printf("Kullanılabilir Komutlar:\n");
  printf("help      : Bu yardım mesajını göster\n");
  printf("ls        : dizin içeriğini listele\n");
  printf("date      : Geçerli tarih ve saati göster\n");
  printf("cd <dir>  : Geçerli çalışma dizinini şu şekilde değiştirin: <dir>\n");
  printf("exit      : Shell çıkış yapar\n");
}

int initmem() {
  fd = shm_open(MY_SHARED_FILE_NAME, O_RDWR | O_CREAT, 0666);
  if (fd < 0) {
    perror("singleshell.c:fd:line31");
    return -1;
  }

  if (ftruncate(fd, MY_FILE_SIZE) == -1) {
    perror("singleshell.c:ftruncate:");
    close(fd);
    return -1;
  }

  addr = mmap(NULL, MY_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    perror("singleshell.c:mmap:");
    close(fd);
    return -1;
  }

  // Paylasilan bellek dosyasındaki izinleri baslatmayla eslesecek sekilde
  // ayarlayın
  if (fchmod(fd, 0666) == -1) {
    perror("singleshell.c:fchmod:");
    close(fd);
    return -1;
  }

  return 0;
}
void print_prompt() { // Bu fonksiyon, mevcut çalışma dizinini bulur ve
                      // kullanıcının bir sonraki girişini beklemesi için bir
                      // işaretçi yazdırır.
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("\n%s > ", cwd);
  } else {
    perror("getcwd() error");
  }
  fflush(stdout);
}

char **parse_input(char *input) { // Bir kullanicinin girdigi komutlari
                                  // ayristirmak icin kullanılır.
  char **args = malloc(MAX_ARGS * sizeof(char *));
  char *arg;
  int i = 0;

  arg = strtok(input, " \t\r\n\a");
  while (arg != NULL) {
    args[i] = arg;
    i++;
    arg = strtok(NULL, " \t\r\n\a");
  }
  args[i] = NULL; // Son bağımsız değişkeni NULL olarak ayarlar.
  return args;
}

void run_command(
    char **args) { // args dizisindeki komutu calictirmak icin kullanilir
  char *command = args[0];
  if (strcmp(command, "help") == 0) {
    print_help();
  } else if (strcmp(command, "quit") == 0) {
    exit(0);
  } else if (strcmp(command, "ls") == 0) {
    // ls cagirir
    pid_t pid = fork();
    if (pid == 0) {
      execlp("ls", "ls", NULL);
      perror("Çalışmada hata vardır");
      exit(1);
    }
    waitpid(pid, NULL, 0);
  } else if (strcmp(command, "date") == 0) {
    // date gösterir
    pid_t pid = fork();
    if (pid == 0) {
      execlp("date", "date", NULL);
      perror("Çalışmada hata vardır");
      exit(1);
    }
    waitpid(pid, NULL, 0);
  }

  else {
    printf("Geçersiz komut. Kullanılabilir komutların listesi için 'yardım' "
           "yazınız.\n");
  }
}

int main() { // Komut satiri shell olusturur
  char input[INBUF_SIZE];

  if (initmem() < 0) {
    exit(1);
  }

  while (1) {
    print_prompt();

    if (fgets(input, INBUF_SIZE, stdin) == NULL) {
      continue;
    }

    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0) {
      continue;
    }

    char **args = parse_input(input);

    if (strcmp(args[0], "exit") == 0) {
      handle_exit();
    } else if (strcmp(args[0], "cd") == 0) {
      handle_cd(args);
    } else {
      run_command(args);
    }

    free(args);
  }

  return 0;
}

void log_message(char *message) { // Paylasilan bellek dosyasina bir ileti
                                  // yazmak icin kullanilir.
  if (addr == NULL) {
    fprintf(stderr, "Shared memory file not mapped\n");
    return;
  }

  time_t t;
  struct tm *tm_info;

  time(&t);
  tm_info = localtime(&t);

  char log_msg[MY_FILE_SIZE];
  snprintf(log_msg, MY_FILE_SIZE, "%d %d-%02d-%02d %02d:%02d:%02d %s\n",
           getpid(), tm_info->tm_year + 1900, tm_info->tm_mon + 1,
           tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
           message);

  strncpy(addr, log_msg, MY_FILE_SIZE - 1);
  addr[MY_FILE_SIZE - 1] ='\0';
}
