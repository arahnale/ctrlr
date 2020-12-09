all: clean
	gcc main.c -g -lncursesw -ltinfow -lmenu -o a.out

clean:
	rm a.out || true

test: 
	gcc test.c -g -lncursesw -ltinfow -lmenu -o a.out

install:
	cp ./a.out ~/ctrlr

debian:
	gcc main.c -g -lncurses -lmenu -o a.out
