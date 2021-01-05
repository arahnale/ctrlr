#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include <readline/history.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h> 


/***************** ФУНКЦИИ МЕНЮ */

#define KEY_MAX 0777
#define REQ_LEFT_ITEM           (KEY_MAX + 1)
#define REQ_RIGHT_ITEM          (KEY_MAX + 2)
#define REQ_UP_ITEM             (KEY_MAX + 3)
#define REQ_DOWN_ITEM           (KEY_MAX + 4)
#define REQ_SCR_ULINE           (KEY_MAX + 5)
#define REQ_SCR_DLINE           (KEY_MAX + 6)
#define REQ_SCR_DPAGE           (KEY_MAX + 7)
#define REQ_SCR_UPAGE           (KEY_MAX + 8)
#define REQ_FIRST_ITEM          (KEY_MAX + 9)
#define REQ_LAST_ITEM           (KEY_MAX + 10)
#define REQ_NEXT_ITEM           (KEY_MAX + 11)
#define REQ_PREV_ITEM           (KEY_MAX + 12)
#define REQ_TOGGLE_ITEM         (KEY_MAX + 13)
#define REQ_CLEAR_PATTERN       (KEY_MAX + 14)
#define REQ_BACK_PATTERN        (KEY_MAX + 15)
#define REQ_NEXT_MATCH          (KEY_MAX + 16)
#define REQ_PREV_MATCH          (KEY_MAX + 17)

// основной класс меню (нахер стандартный класс, он днище)
typedef struct menu_t {
  // доступные пользователю методы
  // установить WINDOW куда выводить текст меню
  void (*set_menu_win)(struct menu_t* , WINDOW * win);
  // добавить новый пункт меню
  void (*new_item) (struct menu_t*, char * name , char * value);
  // установить маркер
  void (*set_menu_mark)(struct menu_t*, char * mark);
  // фильтрация списка меню
  void (*set_menu_filter)(struct menu_t*, char * filter);
  // перемещение стрелками
  void (*set_menu_arrow)(struct menu_t*, int arrow_key);
  // вывести меню на экран
  void (*print)(struct menu_t*);
  // позиция меню в окне. уебский формат y x для того чтобы не отходить от общей концепции
  void (*set_menu_position)(struct menu_t*, int start_y , int start_x , int count_y , int count_x);
  // получить выбранное меню
  char * (*get_menu_item)(struct menu_t*);
  // приватные данные, которые пользователь не должен видеть
  void* _privat;
} menu_t;

// структура для хранения строк меню
typedef struct {
  char * name;
  char * value;
} menu_items_t;

// приватные данные
typedef struct {
  int position_cursor;
  char * item_value;
  int position_start_x;
  int position_start_y;
  int position_count_x;
  int position_count_y;
  char * item_filter;
  unsigned int on_display_count;
  unsigned int array_items_count;
  menu_items_t * menu_items;
  WINDOW * win;
} _privat_t;

typedef struct {
    char         data[10];
    unsigned int len;
} s_filter_word;
s_filter_word filter_word = {"" , 0};

typedef struct {
    char * path;
    char * type;
} s_history_file;
/** s_history_file ** history_files[] = {"/.zsh_history" , "zsh"} , {"/.bash_history" , "bash"}; */

// удобный макрос чтобы не писать 100500 одинаковых строчек
#define PREPARE_IMPL(self) \
  assert(self); \
  assert(self->_privat); \
  _privat_t* _privat = (menu_t*)self->_privat;

// перевод из многобайтовой кодировки в двубайтовую
// выглядит как кусок говна, работает примерно так же
char * _decode(char * c) {

  char * str = ( char* ) malloc( sizeof( char * ) );
  unsigned int len = 0;

  while (*c) {

    if (c[0] == -48 && c[1] == -125 && c[2] > -125) {
      str = (char*)realloc(str , (len + 2) * sizeof(char) ) ;
      str[len] = c[0];
      str[len+1] = c[2] - 32;
      len += 2;
      c+=3;
    }
    else if (c[0] == -48 && c[1] == -125 && c[2] < -125) {
      str = (char*)realloc(str , (len + 2) * sizeof(char) ) ;
      str[len] = c[0];
      str[len+1] = c[2] + 32;
      len += 2;
      c+=3;
    }
    else if (c[0] == -48) {
      str = (char*)realloc(str , (len + 2) * sizeof(char) ) ;
      str[len] = c[0];
      str[len+1] = c[1];
      len += 2;
      c+=2;
    }
    else if (c[0] == -47 && c[1] == -125) {
      str = (char*)realloc(str , (len + 2) * sizeof(char) ) ;
      str[len] = c[0];
      str[len+1] = c[2] - 32;
      len += 2;
      c+=3;
    }
    else if (c[0] == -47 && c[1] != -125) {
      str = (char*)realloc(str , (len + 2) * sizeof(char) ) ;
      str[len] = c[0];
      str[len+1] = c[1];
      len += 2;
      c+=2;
    } else {
      str = (char*)realloc(str , (len + 1) * sizeof(char) ) ;
      str[len++] = c[0];
      c++;
    }
  }

  str = (char*)realloc(str , (len + 1) * sizeof(char) ) ;
  str[len] = '\0';
  /** return (char *)str; */
  return str;
}

static void set_menu_arrow(menu_t * self, int arrow_key) {
  PREPARE_IMPL(self)
  if (arrow_key == REQ_DOWN_ITEM && _privat->position_cursor < _privat->on_display_count - 1) {_privat->position_cursor++; }
  if (arrow_key == REQ_UP_ITEM && _privat->position_cursor > 0) {_privat->position_cursor--; }
}

static void set_menu_win(menu_t * self, WINDOW * win) {
  PREPARE_IMPL(self)
  _privat->win = win;
}

static void new_item(menu_t * self, char * name , char * value) {
  PREPARE_IMPL(self)
  
  // просто для укорочения кода
  menu_items_t * item = &_privat->menu_items[_privat->array_items_count];
  unsigned int * c = &_privat->array_items_count;

  if (strlen(name) > 0 ) {
    item->name = (char *) malloc((sizeof(char) * strlen(name))) ;
  } else {
    // возможна утечка памяти
    item->name = "";
  }

  item->value = _decode(value) ;
  item->name = _decode(name) ;

  _privat->array_items_count++;
  _privat->menu_items =
    (menu_items_t*)realloc(_privat->menu_items , (_privat->array_items_count + 1) * sizeof(menu_items_t) ) ;

  _privat->menu_items[_privat->array_items_count].name = '\0';
  _privat->menu_items[_privat->array_items_count].value = '\0';

}

static void set_menu_mark(menu_t * self, char * mark) {
  PREPARE_IMPL(self)
}

static void set_menu_filter(menu_t * self, char * filter) {
  PREPARE_IMPL(self)
  _privat->item_filter = filter;
  _privat->position_cursor = 0;
}

static void set_menu_position(menu_t * self, int start_y , int start_x , int count_y , int count_x) {
  PREPARE_IMPL(self)

  _privat->position_start_x = start_x;
  _privat->position_start_y = start_y;
  _privat->position_count_x = count_x;
  _privat->position_count_y = count_y;
}

static void print(menu_t * self) {
  PREPARE_IMPL(self)

  // максимальное количество строк на экране
  int max_y = _privat->position_count_y - _privat->position_start_y;
  // максимальная длина строки
  int max_x = _privat->position_count_x - _privat->position_start_x;

  // очищаю остатки, чтобы не очищать экран полность (полная очистка пока работает не красиво)
  for (int y = _privat->position_start_y ; y < max_y ; y++) {
    mvwprintw(_privat->win, y , _privat->position_start_x , "%*s" ,
        max_x , " ");
  }

  _privat->on_display_count = 0;
  for (int y = _privat->position_start_y , i = 0 ; 
       // максимальное количество строк на экране
       y <= max_y &&
       // количество пунктов меню
       i < _privat->array_items_count
       ; i++) {
    // фильтрация списка
    if (strstr(_privat->menu_items[i].value , _privat->item_filter) == NULL) continue;

    // выделенная строка
    if (y - _privat->position_start_y == _privat->position_cursor) {
      wattron(_privat->win, COLOR_PAIR(1));
      /** _privat->item_value = &_privat->menu_items[i].value; */
      _privat->item_value = _privat->menu_items[i].value;
    } 
    // обычная строка
    if (y - _privat->position_start_y != _privat->position_cursor) wattron(_privat->win, COLOR_PAIR(2));

    mvwprintw(_privat->win, y , _privat->position_start_x , "%.*s" , 
        max_x, _privat->menu_items[i].value);
    _privat->on_display_count++;
    y++;
  }

  // снимаю выделение, так как последующие строки не должны выделяться
  wattron(_privat->win, COLOR_PAIR(2));
}

static char * get_menu_item(menu_t * self) {
  PREPARE_IMPL(self)
  return _privat->item_value;
}

void menu_init(menu_t* self) {
  // защита от дурака, программа вылетит если мы не передадим указатель на меню
  assert(self);

  // инициализирую цвета
  start_color();
  if(has_colors() == FALSE) {	
    endwin();
		perror("Твой терминал не поддерживает цвета\n");
		exit(1);
	}
  // цветовая палитра для выделенного текста
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  // цветовая палитра для обычного текста
  init_pair(2, COLOR_WHITE, COLOR_BLACK);

  self->_privat = malloc(sizeof(_privat_t));
  _privat_t * _privat  = (_privat_t*)self->_privat;
  
  // `private` члены
  _privat->position_cursor = 0;
  _privat->position_start_x = 0;
  _privat->position_start_y = 0;
  _privat->position_count_x = 10;
  _privat->position_count_y = 10;
  _privat->array_items_count = 0;
  _privat->on_display_count = 0;
  _privat->menu_items = ( menu_items_t* ) malloc( sizeof( menu_items_t ));

  // `public` функции
  self->set_menu_win = &set_menu_win;
  self->new_item = &new_item;
  self->set_menu_mark = &set_menu_mark;
  self->print = &print;
  self->set_menu_position = &set_menu_position;
  self->set_menu_arrow = &set_menu_arrow;
  self->set_menu_filter = &set_menu_filter;
  self->get_menu_item = &get_menu_item;
}

/************ КОНЕЦ ФУНКЦИЙ МЕНЮ */

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

  if (history_file = _true_path(env_home , "/.zsh_history" , "zsh"))
    return history_file;
  if (history_file = _true_path(env_home , "/.bash_history" , "bash"))
    return history_file;

  perror("fopen");
  exit(1);
}


// чтение файла с историей
int _read_history(menu_t * menu , s_history_file * history_file) {
  // определяею файл с историей

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
    for (; hist > 0; hist--){
      char * c = hist_list[hist]->line;
      while (*c != ';') {c++;}
      c++;
      menu->new_item(menu , "", c);
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
      case KEY_DOWN:
        my_menu.set_menu_arrow(&my_menu, REQ_DOWN_ITEM);
        break;
      case KEY_UP:
        my_menu.set_menu_arrow(&my_menu, REQ_UP_ITEM);
        break;
      // BackSpace
      case 127:
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
