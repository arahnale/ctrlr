#include "menu.h"


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
  return str;
}

static void set_menu_arrow(menu_t * self, int arrow_key) {
  PREPARE_IMPL(self)

  // PAGEUP
  if (arrow_key == 339 ) {
    _privat->shift_line -= _privat->position_count_y - _privat->position_start_y; 
    if (_privat->shift_line < 0) _privat->shift_line = 0;
  }
  if (arrow_key == 339 && _privat->shift_line == 0 ) {
    _privat->position_cursor = 0;
  }
  // PAGEDOWN
  if (arrow_key == 338 && _privat->on_display_count >= _privat->position_count_y - _privat->position_start_y) {
    _privat->shift_line += _privat->position_count_y - _privat->position_start_y; 
  }
  if (arrow_key == 338 && _privat->on_display_count < _privat->position_count_y - _privat->position_start_y) {
    _privat->position_cursor = _privat->on_display_count - 1;
  }

  // перемещаю курсор вверх
  if (arrow_key == REQ_UP_ITEM && _privat->position_cursor > 0) {
    _privat->position_cursor--; 
  }
  // листинг вверх
  if (arrow_key == REQ_UP_ITEM && _privat->position_cursor == 0 && _privat->shift_line > 0) {
    _privat->shift_line--;
  }

  // перемещаю курсор вниз
  if (arrow_key == REQ_DOWN_ITEM && _privat->position_cursor < _privat->on_display_count - 1) {
    _privat->position_cursor++; 
  }
  // листинг вниз
  if (arrow_key == REQ_DOWN_ITEM && _privat->position_cursor == _privat->on_display_count - 1) {
    _privat->shift_line++; 
  }
}

static void set_menu_win(menu_t * self, WINDOW * win) {
  PREPARE_IMPL(self)
  _privat->win = win;
}

static void new_item(menu_t * self, char * name , char * value) {
  PREPARE_IMPL(self)
  
  // просто для укорочения кода
  menu_items_t * item = &_privat->menu_items[_privat->array_items_count];

  /** if (strlen(name) > 0 ) { */
  /**   item->name = (char *) malloc((sizeof(char) * strlen(name))) ; */
  /** } else { */
  /**   // возможна утечка памяти */
  /**   item->name = ""; */
  /** } */

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

// устанавливаю фильтр
static void set_menu_filter(menu_t * self, char * filter) {
  PREPARE_IMPL(self)
  _privat->item_filter = filter;
  _privat->position_cursor = 0;
  _privat->shift_line = 0;
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
  for (int y = _privat->position_start_y ; y < max_y + _privat->position_start_y ; y++) {
    mvwprintw(_privat->win, y , _privat->position_start_x , "%*s" , max_x , " ");
  }

  // счетчик для определения, сколько строк отрезать
  int shift_count = 0;
  _privat->on_display_count = 0;
  for (int y = _privat->position_start_y , i = 0 ; 
       // максимальное количество строк на экране
       y <= max_y &&
       // обзее количество элементов пунктов меню
       i < _privat->array_items_count
       ; i++) {
    // фильтрация списка
    if (strstr(_privat->menu_items[i].value , _privat->item_filter) == NULL) continue;
 
    // перемещение вниз (работает пока через жопу)
    shift_count++;
    if (shift_count < _privat->shift_line) continue;

    // выделенная строка
    if (y - _privat->position_start_y == _privat->position_cursor) {
      wattron(_privat->win, COLOR_PAIR(1));
      /** _privat->item_value = &_privat->menu_items[i].value; */
      _privat->item_value = _privat->menu_items[i].value;
    } 
    // обычная строка
    if (y - _privat->position_start_y != _privat->position_cursor) wattron(_privat->win, COLOR_PAIR(2));

    /** mvwprintw(_privat->win, y , _privat->position_start_x , "%.*s" , */
    /**     max_x, _privat->menu_items[i].value); */
    mvwprintw(_privat->win, y , _privat->position_start_x , "%s : %s" ,
        _privat->menu_items[i].name, _privat->menu_items[i].value);
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
  _privat->shift_line = 0;
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
