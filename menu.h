// обрабатывать ошибку при проверке псевдокласса
#include <assert.h>
// malloc
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
// библиотека для парсинга файла с историей
#include <readline/history.h>

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
  int shift_line;
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

// удобный макрос чтобы не писать 100500 одинаковых строчек
#define PREPARE_IMPL(self) \
  assert(self); \
  assert(self->_privat); \
  _privat_t* _privat = (menu_t*)self->_privat;

char * _decode(char * c);
static void set_menu_arrow(menu_t * self, int arrow_key);
static void set_menu_win(menu_t * self, WINDOW * win);
static void new_item(menu_t * self, char * name , char * value);
static void set_menu_mark(menu_t * self, char * mark);
static void set_menu_filter(menu_t * self, char * filter);
static void set_menu_position(menu_t * self, int start_y , int start_x , int count_y , int count_x);
static void print(menu_t * self);
static char * get_menu_item(menu_t * self);
void menu_init(menu_t* self);

