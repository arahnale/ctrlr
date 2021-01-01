all: clean
	gcc main.c -g -lncursesw -ltinfow -lreadline -o a.out

clean:
	rm a.out || true

test: 
	gcc test.c -g -lncursesw -ltinfow -lmenu -lreadline -o a.out

install:
	cp ./a.out ~/ctrlr

debian:
	gcc main.c -g -lncurses -lreadline -o a.out

menu:
	gcc menu.c -g -lncursesw -ltinfow -lreadline -o a.out


