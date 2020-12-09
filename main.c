#define _GNU_SOURCE
#include <stdlib.h>
#include <curses.h>
#include <menu.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h> 

int arr_count = 0;
ITEM **my_items;
MENU *my_menu;
ITEM *cur_item;
WINDOW *my_menu_win;

typedef struct {
    char         data[10];
    unsigned int len;
} s_filter_word;
s_filter_word filter_word = {"" , 0};

void sigint(int a) {
  unpost_menu(my_menu);
  for(int i = 0; i < arr_count; ++i) free_item(my_items[i]);
  free_menu(my_menu);
  endwin();
  exit(0);
}

FILE * _get_histfile() {
  // по уму тут надо выделять память но пока и так пойдет
  char * env_home = getenv("HOME");
  char file_history[100];
  strcpy(file_history, env_home);
  strcat(file_history, "/.zsh_history");

  FILE *f = fopen (file_history, "r");

  // проверяем, то файл доступен для чтения
  if (f) {
    return f;
  }

  strcpy(file_history, env_home);
  strcat(file_history, "/.bash_history");

  f = fopen (file_history, "r");

  if (f) {
    return f;
  }

  perror("fopen");
  exit(1);
}

// получаю историю из файла с историей. надо будет вставить кучу костылей на получение данных из разных файлов в зависимости от того zsh это или bash
char ** _read_history() {
  char ** lines = ( char** ) malloc( sizeof( char * ));

  FILE *f = _get_histfile();

  // проверяем, то файл доступен для чтения
  if (!f) {
    perror("fopen");
    exit(1);
  }

  char line[80];
  int count_lines = 0;
  int exclude_ciryllic = 0;
  while ( fgets ( line, sizeof line, f) != NULL ) {

    // исключаю кирилицу, так как она не работает..
    exclude_ciryllic = 0;
    for (int i = 0 ; i < strlen(line) ; i++) {
      if (line[i] < 0) {
        exclude_ciryllic = 1 ;
        break;
      }
    }
    if (exclude_ciryllic == 1) continue;

    // исключаю комментарии
    if (line[0] == '#' && line[1] == '1') continue;

    // форматирую вывод, отрезаю дату и время
    int shift_line = 0;
    for (int i = 0 ; i < strlen(line) ; i++){
      if (line[i] == ';') {
        shift_line = i + 1;
        break;
      }
    }
    printf("%s" , line + shift_line);

    line[strlen(line) - 1] = '\0';
    
    // тут какая-то лажа с выделением памяти, выделяю больше чем надо. надо вычитать shift_line
    lines[count_lines] = (char *) malloc((sizeof(char) * sizeof(line))) ;
    strncpy(lines[count_lines] , line + shift_line, strlen(line));
    count_lines++;
    lines = (char**)realloc(lines , (count_lines + 1) * sizeof(char*) ) ;
    arr_count++;

  }

  lines[count_lines] = '\0';

  return lines;
}

int main() { 
  /** setlocale(LC_CTYPE, ""); */
  /** setenv("TERM","xterm-256color",1); */
  /** setlocale(LC_ALL, ""); */
  
  char ** arr_history = _read_history();

  /** for(int y = 0, i = arr_count - 1 ; i > 0; i-- , y++ ) { */
  /**   printf("%s\n" , arr_history[y]); */
  /** } */
  /** printf("%d" , arr_count); */
  /** while (*arr_history) { */
  /**   printf("%s" , arr_history[0]); */
  /**   arr_history++; */
  /** } */

  /** return 0; */

  /** инифиализируем ncurses */
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  // определяю размеры окна
  int window_row , window_col; 
  getmaxyx(stdscr , window_row , window_col);

  my_items = (ITEM **)calloc(arr_count, sizeof(ITEM * ));

  for(int y = 0, i = arr_count - 1 ; i > 0; i-- , y++ ) {
    my_items[y] = new_item(arr_history[i], "");
  }
  my_items[arr_count - 1] = (ITEM *)NULL;

  my_menu = new_menu((ITEM **)my_items);
  set_menu_mark(my_menu, "> ");
  set_menu_format(my_menu , window_row - 2, 0);

  my_menu_win = newwin(window_row,window_col, 0, 0);
  keypad(my_menu_win, TRUE);

  /* создаю окно для меню */
  set_menu_win(my_menu, my_menu_win);
  set_menu_sub(my_menu, derwin(my_menu_win, window_row - 2, window_col - 2, 1, 1));
  box(my_menu_win, 0, 0);
  post_menu(my_menu);

  mvwprintw(my_menu_win, window_row - 1 , 2 , "%s" , "Press Ctrl+C to Exit");
  refresh();
  wrefresh(my_menu_win);

  signal(SIGINT, sigint);

  int c = 0 , y;        
  while(c  != 10) {
    c = getch();
    switch(c) { 
      case KEY_DOWN:
        menu_driver(my_menu, REQ_DOWN_ITEM);
        break;
      case KEY_UP:
        menu_driver(my_menu, REQ_UP_ITEM);
        break;
      // BackSpace
      case 127:
        if (filter_word.len <= 0) break;
        filter_word.data[--filter_word.len] = '\0';

        unpost_menu(my_menu);
        y = 0;
        for(int i = arr_count - 1 ; i > 0; i-- ) {
          if (strstr (arr_history[i],filter_word.data) != NULL)
            my_items[y++] = new_item(arr_history[i], "");
        }
        my_items[y] = (ITEM *)NULL;

        my_menu = new_menu((ITEM **)my_items);
        /* создаю окно для меню */
        set_menu_win(my_menu, my_menu_win);
        set_menu_sub(my_menu, derwin(my_menu_win, window_row - 2, window_col - 2, 1, 1));
        box(my_menu_win, window_row - 2, 0);
        post_menu(my_menu);

        break;
      // ввод текста для фильтрации
      case 32 ... 126:
        if (filter_word.len >= sizeof(filter_word.data)) break;
        filter_word.data[filter_word.len++] = (char) c;
        filter_word.data[filter_word.len + 1] = '\0';

        unpost_menu(my_menu);
        y = 0;
        for(int i = arr_count - 1 ; i > 0; i-- ) {
          if (strstr (arr_history[i],filter_word.data) != NULL)
            my_items[y++] = new_item(arr_history[i], "");
        }
        my_items[y] = (ITEM *)NULL;

        my_menu = new_menu((ITEM **)my_items);
        /* создаю окно для меню */
        set_menu_win(my_menu, my_menu_win);
        set_menu_sub(my_menu, derwin(my_menu_win, window_row - 2, window_col - 2, 1, 1));
        box(my_menu_win, window_row - 2, 0);
        post_menu(my_menu);
        break;
    }

    mvwprintw(my_menu_win, 0 , 2 , "%s" , filter_word.data);
    /** mvwprintw(my_menu_win, 0 , 2 , "%d" , c); */
    refresh();
    wrefresh(my_menu_win);
  } 

  char * value = item_name(current_item(my_menu));

  int f = open("/proc/self/fd/0",O_RDWR|O_NONBLOCK);
  for (int i = 0 ; i < strlen(value) ; i++){
    ioctl (f, TIOCSTI, value + i);
  }
  close(f);
  

  unpost_menu(my_menu);
  for(int i = 0; i < arr_count; ++i) free_item(my_items[i]);
  free_menu(my_menu);
  endwin();
}
