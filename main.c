#include <assert.h>
/** #include <stdio.h> */
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
// требуется для красивой обработки CTRL+C
#include <signal.h>
// передача текста в ввод терминала
#include <sys/ioctl.h>
// позволяет открывать устройсво как файл
#include <fcntl.h> 
// вывод даты ввода команды
#include <time.h>
#include "menu.h"

typedef struct {
    char * path;
    char * type;
} s_history_file;

typedef struct {
    char         data[10];
    unsigned int len;
} s_filter_word;
s_filter_word filter_word = {"" , 0};

// костыль для преобразования даты и времени из zsh в человекопонятный вид
char * zsh_unix_date_to_date_decode( char * str ) {
  char * unix_date = malloc(sizeof(char) * 11);
  unsigned len = 0;
  // : 1589809501:0;
  // порядок цифр, которые нужны для формирования даты
  unsigned int arr[] = {2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , '\0'};
  unsigned int * a = arr;
  while(*a) {
    // всякое бывает, если дата не удалась, то лучше вернуть ничего
    if (str[a[0]] < 48 && str[a[0]] > 57) {
      printf("\n%s\n" , "error!");
      return 1;
    } 
    printf("%c" , str[a[0]]);

    unix_date[len++] = str[a[0]];
    a++;
  }
  unix_date[len] = '\0';
  time_t date = atoi(unix_date);
  free(unix_date);

  struct tm * time = localtime(&date);

  char * buffer = malloc(sizeof(char) * 20);
  sprintf (buffer, "%04d/%02d/%02d %02d:%02d:%02d", time->tm_year+1900 , time->tm_mon , time->tm_mday , time->tm_hour, time->tm_min, time->tm_sec);
  printf("%s\n" ,buffer);
  return buffer;
}

// получение файла с историей
s_history_file * _get_histfile() {
  // чтобы не повторять дважды , создам функцию для проверки существования файла
  s_history_file * _true_path( char * env_home , char * file , char * type ) {
    // произвожу конкатенацию строк
    char * path = (char *) malloc((strlen(env_home) + strlen(file)) * sizeof(char));
    strcpy(path, env_home);
    strcat(path , file);

    FILE *f = fopen (path, "r");
    if (f) {
      fclose(f);
      s_history_file * history_file = malloc(sizeof(s_history_file));
      history_file->path = path;
      history_file->type = type;
      return history_file;
    }

    /** fclose(f); */
    free(path);
    return NULL;
  }

  char * env_home = getenv("HOME");
  s_history_file * history_file;

  // TODO: не красиво
  if (history_file = _true_path(env_home , "/.zsh_history" , "zsh"))
    return history_file;
  if (history_file = _true_path(env_home , "/.bash_history" , "bash"))
    return history_file;

  perror("fopen");
  exit(1);
}


// чтение файла с историей
int _read_history(menu_t * menu , s_history_file * history_file) {
  // инициализация файла с инсторией
  using_history();
  read_history(history_file->path);
  HIST_ENTRY ** hist_list;
  hist_list = history_list();

  if (! hist_list) {
    perror("no read history file");
    return 1;
  }
  int hist = history_length -1;

  if (strcmp(history_file->type , "zsh") == NULL) {
    char date[22];
    int len;
    for (; hist > 0; hist--){
      // в zsh дата ввода комманды пишется через ;
      // : 1589809501:0;
      len = 0;
      char * c = hist_list[hist]->line;
      while (*c != ';') {
        if (len < 20) date[len++] = *c;
        c++;
      }
      date[len] = '\0';
      c++;
      char * tmp_date = zsh_unix_date_to_date_decode(date);
      menu->new_item(menu , tmp_date , c);
      free(tmp_date);
    }
  }

  if (strcmp(history_file->type , "bash") == NULL) {
    for (; hist > 0; hist--){
      menu->new_item(menu , "", hist_list[hist]->line);
    }
  }

  free_history_entry(hist_list);
}

void sigint(int a) {
  // TODO требуется очищать пямять, но пока этого не происходит
  endwin();
  exit(0);
}

int main() {
  setlocale(LC_ALL, "");
  s_history_file * history_file = _get_histfile();

  /** инифиализируем ncurses */
  WINDOW *my_menu_win;
  initscr();
  /** cbreak(); */
  noecho();
  keypad(stdscr, TRUE);

  signal(SIGINT, sigint);

  int window_row, window_col;
  getmaxyx(stdscr , window_row , window_col);
  my_menu_win = newwin(window_row,window_col, 0, 0);
  box(my_menu_win, 0, 0);

  menu_t my_menu;
  menu_init(&my_menu);

  my_menu.set_menu_filter(&my_menu , "");
  my_menu.set_menu_win(&my_menu , my_menu_win);
  my_menu.set_menu_position(&my_menu , 1, 1, window_row - 1, window_col - 2);
  _read_history(&my_menu, history_file);
  my_menu.print(&my_menu);

  mvwprintw(my_menu_win, window_row -1 , 2 , "%s" , "Press Ctrl+C to Exit");
  refresh();
  wrefresh(my_menu_win);

  int c = 0;        
  while(c != 10) {
    c = getch();
    switch(c) { 
      case 338:
        my_menu.set_menu_arrow(&my_menu, 338);
        break;
      case 339:
        my_menu.set_menu_arrow(&my_menu, 339);
        break;
      case KEY_DOWN:
        my_menu.set_menu_arrow(&my_menu, REQ_DOWN_ITEM);
        break;
      case KEY_UP:
        my_menu.set_menu_arrow(&my_menu, REQ_UP_ITEM);
        break;
      // BackSpace
      case 127:
      case 263:
        if (filter_word.len <= 0) break;
        filter_word.data[--filter_word.len] = '\0';
        my_menu.set_menu_filter(&my_menu , filter_word.data);
        break;
      // ввод текста для фильтрации
      // TODO когда-нибудь добавить кирилицу
      case 32 ... 126:
        // тут скорее всего ошибка в sizeof
        if (filter_word.len >= sizeof(filter_word.data)) break;
        filter_word.data[filter_word.len++] = (char) c;
        filter_word.data[filter_word.len + 1] = '\0';
        my_menu.set_menu_filter(&my_menu , filter_word.data);
        break;
    }

    /** wclear(my_menu_win); */
    box(my_menu_win, 0, 0);
    /** mvwprintw(my_menu_win, 0 , 2 , " %*s " , -8 , filter_word.data); */
    mvwprintw(my_menu_win, 0 , 2 , " %s " , filter_word.data);
    mvwprintw(my_menu_win, window_row -1 , 2 , "%s" , "Press Ctrl+C to Exit");
    my_menu.print(&my_menu);
    // код нажатой клавиши, требуется для отладки
    /** mvwprintw(my_menu_win, 0 , 2 , "%d" , c); */
    refresh();
    wrefresh(my_menu_win);
  }

  // Вставляю выбранную команду в терминал
  char * value = my_menu.get_menu_item(&my_menu);
  int f = open("/proc/self/fd/0",O_RDWR|O_NONBLOCK);
  for (int i = 0 ; i < strlen(value) ; i++){
    ioctl (f, TIOCSTI, value + i);
  }
  close(f);

  // TODO требуется освобождать память по завершению работы программы

  endwin();
  return 0;
}
